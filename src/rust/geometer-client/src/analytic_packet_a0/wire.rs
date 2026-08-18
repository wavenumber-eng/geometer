use super::error::{AnalyticPacketError, invalid_packet, limit_exceeded};

pub(crate) const HEADER_BYTES: usize = 64;
const DIRECTORY_ENTRY_BYTES: usize = 32;
pub(crate) const MAX_PACKET_BYTES: usize = 256 * 1024 * 1024;

#[derive(Clone, Copy)]
pub(crate) struct Table<'a> {
    pub kind: u16,
    pub record_bytes: usize,
    pub count: usize,
    pub data: &'a [u8],
}

struct DirectoryEntry {
    kind: u16,
    version: u16,
    record_bytes: usize,
    offset: usize,
    byte_length: usize,
    count: usize,
}

impl<'a> Table<'a> {
    pub fn record(&self, index: usize) -> Result<&'a [u8], AnalyticPacketError> {
        if index >= self.count {
            return Err(invalid_packet(format!(
                "table {} record is out of range",
                self.kind
            )));
        }
        let begin = index
            .checked_mul(self.record_bytes)
            .ok_or_else(|| limit_exceeded("record offset overflow"))?;
        Ok(&self.data[begin..begin + self.record_bytes])
    }
}

pub(crate) fn decode_directory<'a>(
    bytes: &'a [u8],
    magic: &[u8; 8],
    first_kind: u16,
    record_sizes: &[usize],
) -> Result<Vec<Table<'a>>, AnalyticPacketError> {
    validate_header(bytes, magic, record_sizes.len())?;
    let directory_bytes = record_sizes
        .len()
        .checked_mul(DIRECTORY_ENTRY_BYTES)
        .ok_or_else(|| limit_exceeded("directory size overflow"))?;
    let mut cursor = align8(
        HEADER_BYTES
            .checked_add(directory_bytes)
            .ok_or_else(|| limit_exceeded("directory end overflow"))?,
    )?;
    let mut payload_bytes = 0_u64;
    let mut tables = Vec::with_capacity(record_sizes.len());
    for (index, expected_size) in record_sizes.iter().copied().enumerate() {
        let expected_kind = first_kind + u16::try_from(index).expect("small table count");
        let entry = parse_directory_entry(bytes, index)?;
        validate_directory_entry(&entry, expected_kind, expected_size, cursor)?;
        let expected_bytes = entry
            .count
            .checked_mul(entry.record_bytes)
            .ok_or_else(|| limit_exceeded("table byte count overflow"))?;
        if entry.byte_length != expected_bytes {
            return Err(invalid_packet("table byte length does not match its count"));
        }
        let end = entry
            .offset
            .checked_add(entry.byte_length)
            .ok_or_else(|| limit_exceeded("table end overflow"))?;
        if end > bytes.len() {
            return Err(invalid_packet("table extends past the packet"));
        }
        tables.push(Table {
            kind: entry.kind,
            record_bytes: entry.record_bytes,
            count: entry.count,
            data: &bytes[entry.offset..end],
        });
        payload_bytes = payload_bytes
            .checked_add(
                u64::try_from(entry.byte_length).map_err(|_| limit_exceeded("table too large"))?,
            )
            .ok_or_else(|| limit_exceeded("payload accounting overflow"))?;
        let aligned = if index + 1 == record_sizes.len() {
            end
        } else {
            align8(end)?
        };
        if aligned > bytes.len() || bytes[end..aligned].iter().any(|value| *value != 0) {
            return Err(invalid_packet("packet padding is nonzero or truncated"));
        }
        cursor = aligned;
    }
    if cursor != bytes.len() || u64_at(bytes, 48)? != payload_bytes {
        return Err(invalid_packet("packet payload accounting is not canonical"));
    }
    Ok(tables)
}

fn parse_directory_entry(
    bytes: &[u8],
    index: usize,
) -> Result<DirectoryEntry, AnalyticPacketError> {
    let begin = HEADER_BYTES + index * DIRECTORY_ENTRY_BYTES;
    let value = &bytes[begin..begin + DIRECTORY_ENTRY_BYTES];
    Ok(DirectoryEntry {
        kind: u16_at(value, 0)?,
        version: u16_at(value, 2)?,
        record_bytes: usize_from_u32(u32_at(value, 4)?)?,
        offset: usize_from_u64(u64_at(value, 8)?)?,
        byte_length: usize_from_u64(u64_at(value, 16)?)?,
        count: usize_from_u64(u64_at(value, 24)?)?,
    })
}

fn validate_directory_entry(
    entry: &DirectoryEntry,
    expected_kind: u16,
    expected_size: usize,
    expected_offset: usize,
) -> Result<(), AnalyticPacketError> {
    let valid = [
        entry.kind == expected_kind,
        entry.version == 1,
        entry.record_bytes == expected_size,
        entry.offset == expected_offset,
    ]
    .into_iter()
    .all(std::convert::identity);
    if !valid {
        return Err(invalid_packet("noncanonical table directory entry"));
    }
    Ok(())
}

fn validate_header(
    bytes: &[u8],
    magic: &[u8; 8],
    table_count: usize,
) -> Result<(), AnalyticPacketError> {
    if bytes.len() > MAX_PACKET_BYTES {
        return Err(limit_exceeded("packet exceeds 256 MiB"));
    }
    let minimum = HEADER_BYTES
        .checked_add(table_count * DIRECTORY_ENTRY_BYTES)
        .ok_or_else(|| limit_exceeded("minimum packet size overflow"))?;
    if bytes.len() < minimum || bytes.get(..8) != Some(magic.as_slice()) {
        return Err(invalid_packet("packet is truncated or has the wrong magic"));
    }
    let valid = [
        u16_at(bytes, 8)? == 1,
        usize::from(u16_at(bytes, 10)?) == HEADER_BYTES,
        u32_at(bytes, 12)? == 0,
        usize_from_u64(u64_at(bytes, 16)?)? == bytes.len(),
        usize_from_u64(u64_at(bytes, 24)?)? == HEADER_BYTES,
        usize_from_u32(u32_at(bytes, 32)?)? == table_count,
        u32_at(bytes, 44)? == 0,
        u64_at(bytes, 56)? == 0,
    ]
    .into_iter()
    .all(std::convert::identity);
    if !valid {
        return Err(invalid_packet("packet header is invalid"));
    }
    Ok(())
}

pub(crate) fn encode_tables(
    magic: &[u8; 8],
    tables: &[TableBuilder],
    job_count: usize,
    relationship_count: usize,
) -> Result<Vec<u8>, AnalyticPacketError> {
    let directory_end = HEADER_BYTES
        .checked_add(tables.len() * DIRECTORY_ENTRY_BYTES)
        .ok_or_else(|| limit_exceeded("directory size overflow"))?;
    let mut offsets = Vec::with_capacity(tables.len());
    let mut cursor = align8(directory_end)?;
    let mut payload = 0_usize;
    for (index, table) in tables.iter().enumerate() {
        offsets.push(cursor);
        cursor = cursor
            .checked_add(table.bytes.len())
            .ok_or_else(|| limit_exceeded("packet size overflow"))?;
        payload = payload
            .checked_add(table.bytes.len())
            .ok_or_else(|| limit_exceeded("payload size overflow"))?;
        if index + 1 != tables.len() {
            cursor = align8(cursor)?;
        }
    }
    if cursor > MAX_PACKET_BYTES {
        return Err(limit_exceeded("packet exceeds 256 MiB"));
    }
    let mut output = vec![0_u8; cursor];
    output[..8].copy_from_slice(magic);
    put_u16(&mut output, 8, 1)?;
    put_u16(&mut output, 10, HEADER_BYTES as u16)?;
    put_u64(&mut output, 16, cursor as u64)?;
    put_u64(&mut output, 24, HEADER_BYTES as u64)?;
    put_u32(&mut output, 32, usize_u32(tables.len())?)?;
    put_u32(&mut output, 36, usize_u32(job_count)?)?;
    put_u32(&mut output, 40, usize_u32(relationship_count)?)?;
    put_u64(&mut output, 48, payload as u64)?;
    for (index, table) in tables.iter().enumerate() {
        let at = HEADER_BYTES + index * DIRECTORY_ENTRY_BYTES;
        put_u16(&mut output, at, table.kind)?;
        put_u16(&mut output, at + 2, 1)?;
        put_u32(&mut output, at + 4, usize_u32(table.record_bytes)?)?;
        put_u64(&mut output, at + 8, offsets[index] as u64)?;
        put_u64(&mut output, at + 16, table.bytes.len() as u64)?;
        put_u64(&mut output, at + 24, table.count()? as u64)?;
        let begin = offsets[index];
        output[begin..begin + table.bytes.len()].copy_from_slice(&table.bytes);
    }
    Ok(output)
}

pub(crate) struct TableBuilder {
    pub kind: u16,
    pub record_bytes: usize,
    pub bytes: Vec<u8>,
}

impl TableBuilder {
    pub fn new(kind: u16, record_bytes: usize) -> Self {
        Self {
            kind,
            record_bytes,
            bytes: Vec::new(),
        }
    }

    pub fn count(&self) -> Result<usize, AnalyticPacketError> {
        if self.bytes.len() % self.record_bytes != 0 {
            return Err(invalid_packet("internal table record size mismatch"));
        }
        Ok(self.bytes.len() / self.record_bytes)
    }

    pub fn reserve_record(&mut self) -> Result<usize, AnalyticPacketError> {
        let index = self.count()?;
        let end = self
            .bytes
            .len()
            .checked_add(self.record_bytes)
            .ok_or_else(|| limit_exceeded("table size overflow"))?;
        self.bytes.resize(end, 0);
        Ok(index)
    }

    pub fn record_mut(&mut self, index: usize) -> Result<&mut [u8], AnalyticPacketError> {
        let begin = index
            .checked_mul(self.record_bytes)
            .ok_or_else(|| limit_exceeded("record offset overflow"))?;
        let end = begin + self.record_bytes;
        self.bytes
            .get_mut(begin..end)
            .ok_or_else(|| invalid_packet("internal record index is invalid"))
    }
}

pub(crate) fn u16_at(bytes: &[u8], offset: usize) -> Result<u16, AnalyticPacketError> {
    Ok(u16::from_le_bytes(array_at(bytes, offset)?))
}
pub(crate) fn u32_at(bytes: &[u8], offset: usize) -> Result<u32, AnalyticPacketError> {
    Ok(u32::from_le_bytes(array_at(bytes, offset)?))
}
pub(crate) fn u64_at(bytes: &[u8], offset: usize) -> Result<u64, AnalyticPacketError> {
    Ok(u64::from_le_bytes(array_at(bytes, offset)?))
}
pub(crate) fn i64_at(bytes: &[u8], offset: usize) -> Result<i64, AnalyticPacketError> {
    Ok(i64::from_le_bytes(array_at(bytes, offset)?))
}

fn array_at<const N: usize>(bytes: &[u8], offset: usize) -> Result<[u8; N], AnalyticPacketError> {
    bytes
        .get(offset..offset + N)
        .ok_or_else(|| invalid_packet("field is truncated"))?
        .try_into()
        .map_err(|_| invalid_packet("field width is invalid"))
}

pub(crate) fn put_u16(
    bytes: &mut [u8],
    offset: usize,
    value: u16,
) -> Result<(), AnalyticPacketError> {
    put(bytes, offset, &value.to_le_bytes())
}
pub(crate) fn put_u32(
    bytes: &mut [u8],
    offset: usize,
    value: u32,
) -> Result<(), AnalyticPacketError> {
    put(bytes, offset, &value.to_le_bytes())
}
pub(crate) fn put_u64(
    bytes: &mut [u8],
    offset: usize,
    value: u64,
) -> Result<(), AnalyticPacketError> {
    put(bytes, offset, &value.to_le_bytes())
}
pub(crate) fn put_i64(
    bytes: &mut [u8],
    offset: usize,
    value: i64,
) -> Result<(), AnalyticPacketError> {
    put(bytes, offset, &value.to_le_bytes())
}

fn put(bytes: &mut [u8], offset: usize, value: &[u8]) -> Result<(), AnalyticPacketError> {
    let target = bytes
        .get_mut(offset..offset + value.len())
        .ok_or_else(|| invalid_packet("internal record write is out of range"))?;
    target.copy_from_slice(value);
    Ok(())
}

pub(crate) fn usize_u32(value: usize) -> Result<u32, AnalyticPacketError> {
    u32::try_from(value).map_err(|_| limit_exceeded("record count exceeds u32"))
}
fn usize_from_u32(value: u32) -> Result<usize, AnalyticPacketError> {
    usize::try_from(value).map_err(|_| limit_exceeded("u32 does not fit usize"))
}
fn usize_from_u64(value: u64) -> Result<usize, AnalyticPacketError> {
    usize::try_from(value).map_err(|_| limit_exceeded("u64 does not fit usize"))
}
fn align8(value: usize) -> Result<usize, AnalyticPacketError> {
    value
        .checked_add(7)
        .map(|aligned| aligned & !7)
        .ok_or_else(|| limit_exceeded("alignment overflow"))
}
