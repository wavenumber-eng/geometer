use std::path::PathBuf;

use crate::client::GeometerClientError;
use crate::generated::contracts::{self, IpcReasonA0, IpcWelcomeA0};
use crate::generated::operations::expected_operation_catalog;
use crate::ipc::{self, Frame};
use crate::{IPC_IDENTITY, NORMALIZED_CATALOG_SHA256};

pub(crate) fn validate_welcome(welcome: &IpcWelcomeA0) -> Result<(), GeometerClientError> {
    if welcome.ipc != IPC_IDENTITY || welcome.catalog_sha256 != NORMALIZED_CATALOG_SHA256 {
        return Err(GeometerClientError::Protocol(
            "welcome selected an unsupported IPC or contract catalog".to_owned(),
        ));
    }
    if !valid_effective_limits(&welcome.limits) {
        return Err(GeometerClientError::Protocol(
            "welcome advertises an effective limit above the A0 maximum".to_owned(),
        ));
    }
    if welcome.operation_catalog
        != expected_operation_catalog(&welcome.release_version, welcome.c_abi_generation)
    {
        return Err(GeometerClientError::Protocol(
            "welcome operation catalog differs from the generated operation catalog".to_owned(),
        ));
    }
    for capability in [
        "serialized_execution",
        "queue_only_cancellation",
        "raw_attachments",
    ] {
        if !welcome.capabilities.iter().any(|value| value == capability) {
            return Err(GeometerClientError::Protocol(format!(
                "welcome is missing required capability {capability}"
            )));
        }
    }
    Ok(())
}

fn valid_effective_limits(limits: &contracts::IpcEffectiveLimitsA0) -> bool {
    let bounded = [
        (limits.json_bytes, ipc::MAX_JSON_BYTES as u32),
        (limits.attachment_count, ipc::MAX_ATTACHMENT_COUNT as u32),
        (
            limits.attachment_name_bytes,
            ipc::MAX_ATTACHMENT_TEXT_BYTES as u32,
        ),
        (
            limits.attachment_media_type_bytes,
            ipc::MAX_ATTACHMENT_TEXT_BYTES as u32,
        ),
        (limits.attachment_bytes, ipc::MAX_ATTACHMENT_BYTES as u32),
        (limits.frame_bytes, ipc::MAX_FRAME_BYTES as u32),
        (limits.queued_requests, 8),
        (limits.queued_bytes, ipc::MAX_FRAME_BYTES as u32),
        (limits.resident_request_bytes, ipc::MAX_FRAME_BYTES as u32),
        (limits.pending_writer_bytes, ipc::MAX_FRAME_BYTES as u32),
    ];
    bounded
        .iter()
        .all(|(value, maximum)| *value > 0 && value <= maximum)
}

pub(crate) fn validate_effective_request(
    frame: &Frame,
    limits: &contracts::IpcEffectiveLimitsA0,
) -> Result<(), GeometerClientError> {
    let invalid = frame.json.len() > limits.json_bytes as usize
        || frame.attachments.len() > limits.attachment_count as usize
        || frame.encoded_size()? > limits.frame_bytes as usize
        || frame.attachments.iter().any(|attachment| {
            attachment.name.len() > limits.attachment_name_bytes as usize
                || attachment.media_type.len() > limits.attachment_media_type_bytes as usize
                || attachment.data.len() > limits.attachment_bytes as usize
        });
    if invalid {
        return Err(GeometerClientError::Protocol(
            "request exceeds an effective limit advertised by welcome".to_owned(),
        ));
    }
    Ok(())
}

pub(crate) fn encode_reason(reason: Option<&str>) -> Result<Vec<u8>, GeometerClientError> {
    Ok(contracts::encode_ipc_reason_a0_json(&IpcReasonA0 {
        reason: reason.map(str::to_owned),
    })?)
}

pub(crate) fn discover_executable() -> Option<PathBuf> {
    if let Some(path) = std::env::var_os("GEOMETER_EXECUTABLE") {
        let path = PathBuf::from(path);
        if path.is_file() {
            return Some(path);
        }
    }
    let name = if cfg!(windows) {
        "geometer.exe"
    } else {
        "geometer"
    };
    if let Some(sibling) = std::env::current_exe()
        .ok()
        .and_then(|current| current.parent().map(|parent| parent.join(name)))
        .filter(|path| path.is_file())
    {
        return Some(sibling);
    }
    let platform = format!(
        "{}-{}",
        if cfg!(windows) {
            "windows"
        } else if cfg!(target_os = "macos") {
            "macos"
        } else {
            "linux"
        },
        match std::env::consts::ARCH {
            "x86_64" => "x64",
            "aarch64" => "arm64",
            value => value,
        }
    );
    let candidate = PathBuf::from("dist/native").join(platform).join(name);
    candidate.is_file().then_some(candidate)
}
