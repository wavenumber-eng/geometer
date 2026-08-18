/// Stable failure category for the frozen analytic packet projection.
#[derive(Clone, Copy, Debug, Eq, PartialEq)]
pub enum AnalyticPacketErrorKind {
    InvalidPacket,
    InvalidId,
    InvalidReference,
    LimitExceeded,
}

/// Strict analytic packet codec failure.
#[derive(Debug, thiserror::Error)]
#[error("analytic packet {kind:?}: {message}")]
pub struct AnalyticPacketError {
    kind: AnalyticPacketErrorKind,
    message: String,
}

impl AnalyticPacketError {
    pub(crate) fn new(kind: AnalyticPacketErrorKind, message: impl Into<String>) -> Self {
        Self {
            kind,
            message: message.into(),
        }
    }

    pub fn kind(&self) -> AnalyticPacketErrorKind {
        self.kind
    }
}

pub(crate) fn invalid_packet(message: impl Into<String>) -> AnalyticPacketError {
    AnalyticPacketError::new(AnalyticPacketErrorKind::InvalidPacket, message)
}

pub(crate) fn invalid_id(message: impl Into<String>) -> AnalyticPacketError {
    AnalyticPacketError::new(AnalyticPacketErrorKind::InvalidId, message)
}

pub(crate) fn invalid_reference(message: impl Into<String>) -> AnalyticPacketError {
    AnalyticPacketError::new(AnalyticPacketErrorKind::InvalidReference, message)
}

pub(crate) fn limit_exceeded(message: impl Into<String>) -> AnalyticPacketError {
    AnalyticPacketError::new(AnalyticPacketErrorKind::LimitExceeded, message)
}
