use std::collections::HashSet;
use std::convert::TryFrom;

use tokio::io::{AsyncRead, AsyncReadExt, AsyncWrite, AsyncWriteExt};

pub const HEADER_SIZE: usize = 48;
pub const MAX_JSON_BYTES: usize = 8 * 1024 * 1024;
pub const MAX_ATTACHMENT_COUNT: usize = 16;
pub const MAX_ATTACHMENT_TEXT_BYTES: usize = 128;
pub const MAX_ATTACHMENT_BYTES: usize = 256 * 1024 * 1024;
pub const MAX_FRAME_BYTES: usize = 512 * 1024 * 1024;
const MAGIC: &[u8; 8] = b"GMIPCA01";

#[derive(Clone, Copy, Debug, Eq, PartialEq)]
#[repr(u16)]
pub enum FrameKind {
    Hello = 1,
    Welcome = 2,
    Request = 3,
    Response = 4,
    Cancel = 5,
    Cancelled = 6,
    CancelRejected = 7,
    Shutdown = 8,
    ShutdownAck = 9,
    ProtocolError = 10,
}

impl TryFrom<u16> for FrameKind {
    type Error = FrameError;

    fn try_from(value: u16) -> Result<Self, Self::Error> {
        const KINDS: [FrameKind; 10] = [
            FrameKind::Hello,
            FrameKind::Welcome,
            FrameKind::Request,
            FrameKind::Response,
            FrameKind::Cancel,
            FrameKind::Cancelled,
            FrameKind::CancelRejected,
            FrameKind::Shutdown,
            FrameKind::ShutdownAck,
            FrameKind::ProtocolError,
        ];
        if !(1..=KINDS.len() as u16).contains(&value) {
            return Err(FrameError::Protocol("unknown frame kind".to_owned()));
        }
        Ok(KINDS[value as usize - 1])
    }
}

#[derive(Clone, Debug, Eq, PartialEq)]
pub struct Attachment {
    pub name: String,
    pub media_type: String,
    pub data: Vec<u8>,
}

#[derive(Clone, Debug, Eq, PartialEq)]
pub struct Frame {
    pub kind: FrameKind,
    pub request_id: u64,
    pub json: Vec<u8>,
    pub attachments: Vec<Attachment>,
}

impl Frame {
    pub fn encoded_size(&self) -> Result<usize, FrameError> {
        let mut size = HEADER_SIZE
            .checked_add(self.json.len())
            .ok_or_else(|| FrameError::Protocol("frame size overflow".to_owned()))?;
        for attachment in &self.attachments {
            size = size
                .checked_add(16)
                .and_then(|value| value.checked_add(attachment.name.len()))
                .and_then(|value| value.checked_add(attachment.media_type.len()))
                .and_then(|value| value.checked_add(attachment.data.len()))
                .ok_or_else(|| FrameError::Protocol("frame size overflow".to_owned()))?;
        }
        Ok(size)
    }
}

#[derive(Debug, thiserror::Error)]
pub enum FrameError {
    #[error("IPC I/O failed: {0}")]
    Io(#[from] std::io::Error),
    #[error("IPC protocol error: {0}")]
    Protocol(String),
}

struct ParsedHeader {
    kind: FrameKind,
    request_id: u64,
    json_size: usize,
    attachment_count: usize,
    attachment_bytes: usize,
}

pub async fn read_frame<R: AsyncRead + Unpin>(reader: &mut R) -> Result<Option<Frame>, FrameError> {
    let mut header = [0_u8; HEADER_SIZE];
    let first = reader.read(&mut header[..1]).await?;
    if first == 0 {
        return Ok(None);
    }
    reader.read_exact(&mut header[1..]).await?;
    let parsed = parse_header(&header)?;
    let complete_size = HEADER_SIZE
        .checked_add(parsed.json_size)
        .and_then(|value| value.checked_add(parsed.attachment_bytes))
        .ok_or_else(|| protocol("complete frame size overflow"))?;
    if complete_size > MAX_FRAME_BYTES {
        return Err(protocol("complete frame exceeds the A0 limit"));
    }
    let mut payload = vec![0_u8; parsed.json_size + parsed.attachment_bytes];
    reader.read_exact(&mut payload).await?;
    let json = payload[..parsed.json_size].to_vec();
    let attachments = decode_attachments(&payload, parsed.json_size, parsed.attachment_count)?;
    Ok(Some(Frame {
        kind: parsed.kind,
        request_id: parsed.request_id,
        json,
        attachments,
    }))
}

fn parse_header(header: &[u8; HEADER_SIZE]) -> Result<ParsedHeader, FrameError> {
    if &header[..8] != MAGIC {
        return Err(protocol("invalid frame magic"));
    }
    let supported_header = [
        u16::from_le_bytes(header[8..10].try_into().unwrap()) == HEADER_SIZE as u16,
        u16::from_le_bytes(header[10..12].try_into().unwrap()) == 0,
        u16::from_le_bytes(header[14..16].try_into().unwrap()) == 0,
        u32::from_le_bytes(header[40..44].try_into().unwrap()) == 0,
        u32::from_le_bytes(header[44..48].try_into().unwrap()) == 0,
    ];
    if !supported_header.into_iter().all(std::convert::identity) {
        return Err(protocol("unsupported or reserved frame header value"));
    }
    let parsed = ParsedHeader {
        kind: FrameKind::try_from(u16::from_le_bytes(header[12..14].try_into().unwrap()))?,
        request_id: u64::from_le_bytes(header[16..24].try_into().unwrap()),
        json_size: u32::from_le_bytes(header[24..28].try_into().unwrap()) as usize,
        attachment_count: u32::from_le_bytes(header[28..32].try_into().unwrap()) as usize,
        attachment_bytes: usize::try_from(u64::from_le_bytes(header[32..40].try_into().unwrap()))
            .map_err(|_| {
            protocol("attachment byte total does not fit this platform")
        })?,
    };
    let sizes_in_range = [
        parsed.json_size > 0,
        parsed.json_size <= MAX_JSON_BYTES,
        parsed.attachment_count <= MAX_ATTACHMENT_COUNT,
        parsed.attachment_bytes <= MAX_FRAME_BYTES,
    ];
    if !sizes_in_range.into_iter().all(std::convert::identity) {
        return Err(protocol("frame header exceeds an A0 limit"));
    }
    Ok(parsed)
}

fn decode_attachments(
    payload: &[u8],
    mut offset: usize,
    attachment_count: usize,
) -> Result<Vec<Attachment>, FrameError> {
    let mut attachments = Vec::with_capacity(attachment_count);
    let mut attachment_names = HashSet::with_capacity(attachment_count);
    for _ in 0..attachment_count {
        attachments.push(decode_attachment(
            payload,
            &mut offset,
            &mut attachment_names,
        )?);
    }
    if offset != payload.len() {
        return Err(protocol(
            "attachment byte total does not match parsed sections",
        ));
    }
    Ok(attachments)
}

fn decode_attachment(
    payload: &[u8],
    offset: &mut usize,
    names: &mut HashSet<String>,
) -> Result<Attachment, FrameError> {
    if payload.len().saturating_sub(*offset) < 16 {
        return Err(protocol("truncated attachment header"));
    }
    let header = &payload[*offset..*offset + 16];
    let name_size = u16::from_le_bytes(header[..2].try_into().unwrap()) as usize;
    let media_type_size = u16::from_le_bytes(header[2..4].try_into().unwrap()) as usize;
    let flags = u32::from_le_bytes(header[4..8].try_into().unwrap());
    let data_size = usize::try_from(u64::from_le_bytes(header[8..16].try_into().unwrap()))
        .map_err(|_| protocol("attachment size does not fit this platform"))?;
    let header_valid = [
        name_size <= MAX_ATTACHMENT_TEXT_BYTES,
        media_type_size <= MAX_ATTACHMENT_TEXT_BYTES,
        data_size <= MAX_ATTACHMENT_BYTES,
        flags == 0,
    ];
    if !header_valid.into_iter().all(std::convert::identity) {
        return Err(protocol("attachment header exceeds an A0 limit"));
    }
    let section_size = 16_usize
        .checked_add(name_size)
        .and_then(|value| value.checked_add(media_type_size))
        .and_then(|value| value.checked_add(data_size))
        .ok_or_else(|| protocol("attachment section size overflow"))?;
    if section_size > payload.len().saturating_sub(*offset) {
        return Err(protocol("attachment exceeds its declared section"));
    }
    *offset += 16;
    let name = decode_text(payload, offset, name_size, "attachment name")?;
    let media_type = decode_text(payload, offset, media_type_size, "attachment media type")?;
    if name.is_empty() || media_type.is_empty() || !names.insert(name.clone()) {
        return Err(protocol("attachment metadata is empty or duplicated"));
    }
    let data = payload[*offset..*offset + data_size].to_vec();
    *offset += data_size;
    Ok(Attachment {
        name,
        media_type,
        data,
    })
}

fn decode_text(
    payload: &[u8],
    offset: &mut usize,
    size: usize,
    label: &str,
) -> Result<String, FrameError> {
    let value = std::str::from_utf8(&payload[*offset..*offset + size])
        .map_err(|_| protocol(format!("{label} is not valid UTF-8")))?
        .to_owned();
    *offset += size;
    Ok(value)
}

pub async fn write_frame<W: AsyncWrite + Unpin>(
    writer: &mut W,
    frame: &Frame,
) -> Result<(), FrameError> {
    validate_outgoing(frame)?;
    let attachment_bytes = frame
        .attachments
        .iter()
        .try_fold(0_u64, |total, attachment| {
            total
                .checked_add(
                    16 + attachment.name.len() as u64
                        + attachment.media_type.len() as u64
                        + attachment.data.len() as u64,
                )
                .ok_or_else(|| protocol("attachment byte total overflow"))
        })?;
    let mut header = [0_u8; HEADER_SIZE];
    header[..8].copy_from_slice(MAGIC);
    header[8..10].copy_from_slice(&(HEADER_SIZE as u16).to_le_bytes());
    header[12..14].copy_from_slice(&(frame.kind as u16).to_le_bytes());
    header[16..24].copy_from_slice(&frame.request_id.to_le_bytes());
    header[24..28].copy_from_slice(&(frame.json.len() as u32).to_le_bytes());
    header[28..32].copy_from_slice(&(frame.attachments.len() as u32).to_le_bytes());
    header[32..40].copy_from_slice(&attachment_bytes.to_le_bytes());
    writer.write_all(&header).await?;
    writer.write_all(&frame.json).await?;
    for attachment in &frame.attachments {
        let mut attachment_header = [0_u8; 16];
        attachment_header[..2].copy_from_slice(&(attachment.name.len() as u16).to_le_bytes());
        attachment_header[2..4]
            .copy_from_slice(&(attachment.media_type.len() as u16).to_le_bytes());
        attachment_header[8..16].copy_from_slice(&(attachment.data.len() as u64).to_le_bytes());
        writer.write_all(&attachment_header).await?;
        writer.write_all(attachment.name.as_bytes()).await?;
        writer.write_all(attachment.media_type.as_bytes()).await?;
        writer.write_all(&attachment.data).await?;
    }
    writer.flush().await?;
    Ok(())
}

fn validate_outgoing(frame: &Frame) -> Result<(), FrameError> {
    if frame.json.is_empty()
        || frame.json.len() > MAX_JSON_BYTES
        || frame.attachments.len() > MAX_ATTACHMENT_COUNT
        || frame.encoded_size()? > MAX_FRAME_BYTES
    {
        return Err(protocol("outgoing frame exceeds an A0 limit"));
    }
    let mut attachment_names = HashSet::with_capacity(frame.attachments.len());
    for attachment in &frame.attachments {
        if !valid_outgoing_attachment(attachment, &mut attachment_names) {
            return Err(protocol("outgoing attachment exceeds an A0 limit"));
        }
    }
    Ok(())
}

fn valid_outgoing_attachment<'a>(attachment: &'a Attachment, names: &mut HashSet<&'a str>) -> bool {
    !attachment.name.is_empty()
        && !attachment.media_type.is_empty()
        && names.insert(attachment.name.as_str())
        && attachment.name.len() <= MAX_ATTACHMENT_TEXT_BYTES
        && attachment.media_type.len() <= MAX_ATTACHMENT_TEXT_BYTES
        && attachment.data.len() <= MAX_ATTACHMENT_BYTES
}

fn protocol(message: impl Into<String>) -> FrameError {
    FrameError::Protocol(message.into())
}
