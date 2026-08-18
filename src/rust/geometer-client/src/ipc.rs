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

struct AttachmentHeader {
    name_size: usize,
    media_type_size: usize,
    data_size: usize,
}

#[derive(Clone, Copy)]
pub(crate) struct ReadLimits {
    pub json_bytes: usize,
    pub attachment_count: usize,
    pub attachment_name_bytes: usize,
    pub attachment_media_type_bytes: usize,
    pub attachment_bytes: usize,
    pub frame_bytes: usize,
}

impl ReadLimits {
    fn maximum_attachment_section_bytes(self) -> Option<usize> {
        16_usize
            .checked_add(self.attachment_name_bytes)?
            .checked_add(self.attachment_media_type_bytes)?
            .checked_add(self.attachment_bytes)
    }
}

pub async fn read_frame<R: AsyncRead + Unpin>(reader: &mut R) -> Result<Option<Frame>, FrameError> {
    read_frame_with_limits(reader, None).await
}

pub(crate) async fn read_frame_with_limits<R: AsyncRead + Unpin>(
    reader: &mut R,
    limits: Option<ReadLimits>,
) -> Result<Option<Frame>, FrameError> {
    let mut header = [0_u8; HEADER_SIZE];
    let first = reader.read(&mut header[..1]).await?;
    if first == 0 {
        return Ok(None);
    }
    read_exact_section(reader, &mut header[1..], "frame header").await?;
    let parsed = parse_header(&header)?;
    let complete_size = HEADER_SIZE
        .checked_add(parsed.json_size)
        .and_then(|value| value.checked_add(parsed.attachment_bytes))
        .ok_or_else(|| protocol("complete frame size overflow"))?;
    if complete_size > MAX_FRAME_BYTES {
        return Err(protocol("complete frame exceeds the A0 limit"));
    }
    if limits.is_some_and(|value| {
        let aggregate_limit = value
            .maximum_attachment_section_bytes()
            .and_then(|section| section.checked_mul(parsed.attachment_count));
        parsed.json_size > value.json_bytes
            || parsed.attachment_count > value.attachment_count
            || complete_size > value.frame_bytes
            || aggregate_limit.is_none_or(|limit| parsed.attachment_bytes > limit)
    }) {
        return Err(protocol(
            "frame header exceeds a negotiated effective limit",
        ));
    }
    let mut json = vec![0_u8; parsed.json_size];
    read_exact_section(reader, &mut json, "frame JSON").await?;
    let attachments = read_attachments(
        reader,
        parsed.attachment_count,
        parsed.attachment_bytes,
        limits,
    )
    .await?;
    Ok(Some(Frame {
        kind: parsed.kind,
        request_id: parsed.request_id,
        json,
        attachments,
    }))
}

async fn read_exact_section<R: AsyncRead + Unpin>(
    reader: &mut R,
    bytes: &mut [u8],
    label: &str,
) -> Result<(), FrameError> {
    match reader.read_exact(bytes).await {
        Ok(_) => Ok(()),
        Err(error) if error.kind() == std::io::ErrorKind::UnexpectedEof => {
            Err(protocol(format!("truncated {label}")))
        }
        Err(error) => Err(FrameError::Io(error)),
    }
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
        parsed
            .attachment_count
            .checked_mul(16)
            .is_some_and(|minimum| minimum <= parsed.attachment_bytes),
    ];
    if !sizes_in_range.into_iter().all(std::convert::identity) {
        return Err(protocol("frame header exceeds an A0 limit"));
    }
    validate_frame_shape(
        parsed.kind,
        parsed.request_id,
        parsed.attachment_count,
        parsed.attachment_bytes,
    )?;
    Ok(parsed)
}

async fn read_attachments<R: AsyncRead + Unpin>(
    reader: &mut R,
    attachment_count: usize,
    declared_bytes: usize,
    limits: Option<ReadLimits>,
) -> Result<Vec<Attachment>, FrameError> {
    let mut attachments = Vec::with_capacity(attachment_count);
    let mut attachment_names = HashSet::with_capacity(attachment_count);
    let mut consumed = 0_usize;
    for _ in 0..attachment_count {
        let remaining = declared_bytes
            .checked_sub(consumed)
            .ok_or_else(|| protocol("attachment byte accounting underflow"))?;
        if remaining < 16 {
            return Err(protocol("truncated declared attachment section"));
        }
        let mut bytes = [0_u8; 16];
        read_exact_section(reader, &mut bytes, "attachment header").await?;
        let header = parse_attachment_header(&bytes, limits)?;
        let section_size = attachment_section_size(&header)?;
        if section_size > remaining {
            return Err(protocol("attachment exceeds its declared byte total"));
        }
        consumed = consumed
            .checked_add(section_size)
            .ok_or_else(|| protocol("attachment byte accounting overflow"))?;
        let name = read_text_section(reader, header.name_size, "attachment name").await?;
        let media_type =
            read_text_section(reader, header.media_type_size, "attachment media type").await?;
        if name.is_empty() || media_type.is_empty() || !attachment_names.insert(name.clone()) {
            return Err(protocol("attachment metadata is empty or duplicated"));
        }
        let mut data = vec![0_u8; header.data_size];
        read_exact_section(reader, &mut data, "attachment data").await?;
        attachments.push(Attachment {
            name,
            media_type,
            data,
        });
    }
    if consumed != declared_bytes {
        return Err(protocol(
            "attachment sections do not match their declared byte total",
        ));
    }
    Ok(attachments)
}

fn parse_attachment_header(
    header: &[u8; 16],
    limits: Option<ReadLimits>,
) -> Result<AttachmentHeader, FrameError> {
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
    if limits.is_some_and(|value| {
        name_size > value.attachment_name_bytes
            || media_type_size > value.attachment_media_type_bytes
            || data_size > value.attachment_bytes
    }) {
        return Err(protocol(
            "attachment header exceeds a negotiated effective limit",
        ));
    }
    Ok(AttachmentHeader {
        name_size,
        media_type_size,
        data_size,
    })
}

fn attachment_section_size(header: &AttachmentHeader) -> Result<usize, FrameError> {
    16_usize
        .checked_add(header.name_size)
        .and_then(|value| value.checked_add(header.media_type_size))
        .and_then(|value| value.checked_add(header.data_size))
        .ok_or_else(|| protocol("attachment section size overflow"))
}

async fn read_text_section<R: AsyncRead + Unpin>(
    reader: &mut R,
    size: usize,
    label: &str,
) -> Result<String, FrameError> {
    let mut bytes = vec![0_u8; size];
    read_exact_section(reader, &mut bytes, label).await?;
    Ok(std::str::from_utf8(&bytes)
        .map_err(|_| protocol(format!("{label} is not valid UTF-8")))?
        .to_owned())
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
    let attachment_bytes = frame.encoded_size()? - HEADER_SIZE - frame.json.len();
    validate_frame_shape(
        frame.kind,
        frame.request_id,
        frame.attachments.len(),
        attachment_bytes,
    )?;
    let mut attachment_names = HashSet::with_capacity(frame.attachments.len());
    for attachment in &frame.attachments {
        if !valid_outgoing_attachment(attachment, &mut attachment_names) {
            return Err(protocol("outgoing attachment exceeds an A0 limit"));
        }
    }
    Ok(())
}

fn validate_frame_shape(
    kind: FrameKind,
    request_id: u64,
    attachment_count: usize,
    attachment_bytes: usize,
) -> Result<(), FrameError> {
    let id_valid = match kind {
        FrameKind::Hello | FrameKind::Welcome | FrameKind::Shutdown | FrameKind::ShutdownAck => {
            request_id == 0
        }
        FrameKind::ProtocolError => true,
        FrameKind::Request
        | FrameKind::Response
        | FrameKind::Cancel
        | FrameKind::Cancelled
        | FrameKind::CancelRejected => request_id != 0,
    };
    let attachments_valid = matches!(kind, FrameKind::Request | FrameKind::Response)
        || (attachment_count == 0 && attachment_bytes == 0);
    if !id_valid || !attachments_valid {
        return Err(protocol(
            "frame kind, request id, and attachments are inconsistent",
        ));
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

#[cfg(test)]
mod tests {
    use std::pin::Pin;
    use std::task::{Context, Poll};

    use tokio::io::ReadBuf;

    use super::*;

    struct ChunkedReader {
        bytes: Vec<u8>,
        offset: usize,
        max_chunk: usize,
        max_requested: usize,
    }

    impl AsyncRead for ChunkedReader {
        fn poll_read(
            self: Pin<&mut Self>,
            _context: &mut Context<'_>,
            buffer: &mut ReadBuf<'_>,
        ) -> Poll<std::io::Result<()>> {
            let this = self.get_mut();
            this.max_requested = this.max_requested.max(buffer.remaining());
            let count = this
                .max_chunk
                .min(buffer.remaining())
                .min(this.bytes.len().saturating_sub(this.offset));
            buffer.put_slice(&this.bytes[this.offset..this.offset + count]);
            this.offset += count;
            Poll::Ready(Ok(()))
        }
    }

    #[tokio::test]
    async fn outgoing_shape_rejects_zero_request_id_and_control_attachments() {
        let (mut writer, _reader) = tokio::io::duplex(256);
        let zero_request = Frame {
            kind: FrameKind::Request,
            request_id: 0,
            json: b"{}".to_vec(),
            attachments: Vec::new(),
        };
        assert!(matches!(
            write_frame(&mut writer, &zero_request).await,
            Err(FrameError::Protocol(_))
        ));
        let attached_control = Frame {
            kind: FrameKind::Shutdown,
            request_id: 0,
            json: b"{}".to_vec(),
            attachments: vec![Attachment {
                name: "x".to_owned(),
                media_type: "x/x".to_owned(),
                data: Vec::new(),
            }],
        };
        assert!(matches!(
            write_frame(&mut writer, &attached_control).await,
            Err(FrameError::Protocol(_))
        ));
    }

    #[tokio::test]
    async fn negotiated_aggregate_attachment_limit_is_checked_before_payload_read() {
        let (mut writer, mut reader) = tokio::io::duplex(HEADER_SIZE);
        let header = test_header(FrameKind::Response, 1, 2, 1, 33);
        writer.write_all(&header).await.unwrap();
        let limits = ReadLimits {
            json_bytes: 2,
            attachment_count: 1,
            attachment_name_bytes: 4,
            attachment_media_type_bytes: 4,
            attachment_bytes: 8,
            frame_bytes: 128,
        };
        assert!(matches!(
            read_frame_with_limits(&mut reader, Some(limits)).await,
            Err(FrameError::Protocol(_))
        ));
    }

    #[tokio::test]
    async fn impossible_attachment_header_total_is_rejected_before_payload_read() {
        let (mut writer, mut reader) = tokio::io::duplex(HEADER_SIZE);
        writer
            .write_all(&test_header(FrameKind::Response, 1, 2, 2, 31))
            .await
            .unwrap();
        assert!(matches!(
            read_frame(&mut reader).await,
            Err(FrameError::Protocol(_))
        ));
    }

    #[tokio::test]
    async fn prior_attachment_cannot_consume_the_next_declared_header() {
        let (mut writer, mut reader) = tokio::io::duplex(HEADER_SIZE + 2 + 32);
        writer
            .write_all(&test_header(FrameKind::Response, 1, 2, 2, 32))
            .await
            .unwrap();
        writer.write_all(b"{}").await.unwrap();
        writer.write_all(&8_u16.to_le_bytes()).await.unwrap();
        writer.write_all(&8_u16.to_le_bytes()).await.unwrap();
        writer.write_all(&0_u32.to_le_bytes()).await.unwrap();
        writer.write_all(&0_u64.to_le_bytes()).await.unwrap();
        writer.write_all(b"12345678abcdefgh").await.unwrap();

        assert!(matches!(
            read_frame(&mut reader).await,
            Err(FrameError::Protocol(_))
        ));
    }

    #[tokio::test]
    async fn attachment_sections_are_streamed_through_a_chunked_reader() {
        let frame = Frame {
            kind: FrameKind::Response,
            request_id: 1,
            json: b"{}".to_vec(),
            attachments: vec![
                Attachment {
                    name: "first".to_owned(),
                    media_type: "x/one".to_owned(),
                    data: vec![3; 64],
                },
                Attachment {
                    name: "second".to_owned(),
                    media_type: "x/two".to_owned(),
                    data: vec![5; 96],
                },
            ],
        };
        let mut encoded = Vec::new();
        write_frame(&mut encoded, &frame).await.unwrap();
        let payload_bytes = encoded.len() - HEADER_SIZE;
        let mut reader = ChunkedReader {
            bytes: encoded,
            offset: 0,
            max_chunk: 3,
            max_requested: 0,
        };

        assert_eq!(read_frame(&mut reader).await.unwrap(), Some(frame));
        assert_eq!(reader.offset, reader.bytes.len());
        assert!(reader.max_requested <= 96);
        assert!(reader.max_requested < payload_bytes);
    }

    #[tokio::test]
    async fn incoming_shape_is_rejected_from_the_fixed_header() {
        for header in [
            test_header(FrameKind::Request, 0, 2, 0, 0),
            test_header(FrameKind::ShutdownAck, 0, 2, 1, 16),
        ] {
            let (mut writer, mut reader) = tokio::io::duplex(HEADER_SIZE);
            writer.write_all(&header).await.unwrap();
            assert!(matches!(
                read_frame(&mut reader).await,
                Err(FrameError::Protocol(_))
            ));
        }
    }

    #[tokio::test]
    async fn eof_inside_header_or_payload_is_protocol_failure() {
        let (mut writer, mut reader) = tokio::io::duplex(HEADER_SIZE);
        writer.write_all(&MAGIC[..4]).await.unwrap();
        drop(writer);
        assert!(matches!(
            read_frame(&mut reader).await,
            Err(FrameError::Protocol(_))
        ));

        let (mut writer, mut reader) = tokio::io::duplex(HEADER_SIZE + 1);
        writer
            .write_all(&test_header(FrameKind::Response, 1, 2, 0, 0))
            .await
            .unwrap();
        writer.write_all(b"{").await.unwrap();
        drop(writer);
        assert!(matches!(
            read_frame(&mut reader).await,
            Err(FrameError::Protocol(_))
        ));
    }

    #[tokio::test]
    async fn negotiated_attachment_header_limits_are_enforced() {
        let (mut writer, mut reader) = tokio::io::duplex(128);
        let header = test_header(FrameKind::Response, 1, 2, 1, 26);
        writer.write_all(&header).await.unwrap();
        writer.write_all(b"{}").await.unwrap();
        writer.write_all(&5_u16.to_le_bytes()).await.unwrap();
        writer.write_all(&3_u16.to_le_bytes()).await.unwrap();
        writer.write_all(&0_u32.to_le_bytes()).await.unwrap();
        writer.write_all(&2_u64.to_le_bytes()).await.unwrap();
        writer.write_all(b"abcdeappzz").await.unwrap();
        let limits = ReadLimits {
            json_bytes: 2,
            attachment_count: 1,
            attachment_name_bytes: 4,
            attachment_media_type_bytes: 4,
            attachment_bytes: 8,
            frame_bytes: 128,
        };
        assert!(matches!(
            read_frame_with_limits(&mut reader, Some(limits)).await,
            Err(FrameError::Protocol(_))
        ));
    }

    fn test_header(
        kind: FrameKind,
        request_id: u64,
        json_bytes: u32,
        attachment_count: u32,
        attachment_bytes: u64,
    ) -> [u8; HEADER_SIZE] {
        let mut header = [0_u8; HEADER_SIZE];
        header[..8].copy_from_slice(MAGIC);
        header[8..10].copy_from_slice(&(HEADER_SIZE as u16).to_le_bytes());
        header[12..14].copy_from_slice(&(kind as u16).to_le_bytes());
        header[16..24].copy_from_slice(&request_id.to_le_bytes());
        header[24..28].copy_from_slice(&json_bytes.to_le_bytes());
        header[28..32].copy_from_slice(&attachment_count.to_le_bytes());
        header[32..40].copy_from_slice(&attachment_bytes.to_le_bytes());
        header
    }
}
