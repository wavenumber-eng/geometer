// Generated from wn_geometer_contract_catalog.a0.json. Do not edit.

use serde::{Deserialize, Serialize, de::DeserializeOwned};

pub const NORMALIZED_CATALOG_SHA256: &str =
    "126edc93c7fbb23b0e15da35966abe0145972586807be6b7fdbc46948adb175c";

#[derive(Debug, thiserror::Error)]
pub enum ContractError {
    #[error("invalid JSON contract: {0}")]
    Json(#[from] serde_json::Error),
    #[error("contract validation failed at {path}: {message}")]
    Validation { path: String, message: String },
}

pub trait Validate {
    fn validate_at(&self, path: &str) -> Result<(), ContractError>;
}

pub fn decode_json<T: DeserializeOwned + Validate>(data: &[u8]) -> Result<T, ContractError> {
    let mut deserializer = serde_json::Deserializer::from_slice(data);
    let value = T::deserialize(&mut deserializer)?;
    deserializer.end()?;
    value.validate_at("")?;
    Ok(value)
}

pub fn encode_json<T: Serialize + Validate>(value: &T) -> Result<Vec<u8>, ContractError> {
    value.validate_at("")?;
    Ok(serde_json::to_vec(value)?)
}

fn child_path(path: &str, token: &str) -> String {
    format!("{path}/{}", token.replace('~', "~0").replace('/', "~1"))
}

fn invalid(path: &str, message: &str) -> ContractError {
    ContractError::Validation {
        path: path.to_owned(),
        message: message.to_owned(),
    }
}

fn deserialize_optional_non_null<'de, D, T>(deserializer: D) -> Result<Option<T>, D::Error>
where
    D: serde::Deserializer<'de>,
    T: Deserialize<'de>,
{
    T::deserialize(deserializer).map(Some)
}
#[derive(Clone, Debug, Deserialize, PartialEq, Serialize)]
pub enum DiagnosticCategory {
    #[serde(rename = "transport")]
    Transport,
    #[serde(rename = "contract")]
    Contract,
    #[serde(rename = "operation")]
    Operation,
}

impl Validate for DiagnosticCategory {
    fn validate_at(&self, _path: &str) -> Result<(), ContractError> {
        Ok(())
    }
}

#[derive(Clone, Debug, Deserialize, PartialEq, Serialize)]
#[serde(deny_unknown_fields)]
pub struct DiagnosticA0 {
    pub code: String,
    pub category: DiagnosticCategory,
    pub message: String,
    pub retryable: bool,
    #[serde(
        default,
        deserialize_with = "deserialize_optional_non_null",
        skip_serializing_if = "Option::is_none"
    )]
    pub path: Option<String>,
    #[serde(
        default,
        deserialize_with = "deserialize_optional_non_null",
        skip_serializing_if = "Option::is_none"
    )]
    pub operation: Option<String>,
    #[serde(
        default,
        deserialize_with = "deserialize_optional_non_null",
        skip_serializing_if = "Option::is_none"
    )]
    pub request_id: Option<String>,
}

impl Validate for DiagnosticA0 {
    fn validate_at(&self, path: &str) -> Result<(), ContractError> {
        let field_path = child_path(path, "code");
        let value = &self.code;
        if value.is_empty() {
            return Err(invalid(&field_path, "string is shorter than its minimum"));
        }
        let field_path = child_path(path, "category");
        let value = &self.category;
        value.validate_at(&field_path)?;
        Ok(())
    }
}

#[derive(Clone, Debug, Deserialize, PartialEq, Serialize)]
#[serde(deny_unknown_fields)]
pub struct IpcAttachmentDeclarationA0 {
    pub name: String,
    pub required: bool,
    pub media_types: Vec<String>,
    pub max_bytes: u32,
}

impl Validate for IpcAttachmentDeclarationA0 {
    fn validate_at(&self, path: &str) -> Result<(), ContractError> {
        let field_path = child_path(path, "name");
        let value = &self.name;
        if value.is_empty() {
            return Err(invalid(&field_path, "string is shorter than its minimum"));
        }
        if value.len() > 128 {
            return Err(invalid(&field_path, "string exceeds its maximum"));
        }
        let field_path = child_path(path, "media_types");
        let value = &self.media_types;
        if value.is_empty() {
            return Err(invalid(&field_path, "array is shorter than its minimum"));
        }
        if value.len() > 16 {
            return Err(invalid(&field_path, "array exceeds its maximum"));
        }
        let field_path = child_path(path, "max_bytes");
        let value = &self.max_bytes;
        if *value > 268435456 {
            return Err(invalid(&field_path, "number exceeds its maximum"));
        }
        Ok(())
    }
}

#[derive(Clone, Debug, Deserialize, PartialEq, Serialize)]
#[serde(deny_unknown_fields)]
pub struct IpcAttachmentOffsetsWasm32A0 {
    pub struct_size: u32,
    pub flags: u32,
    pub name: u32,
    pub name_size: u32,
    pub media_type: u32,
    pub media_type_size: u32,
    pub data: u32,
    pub data_size: u32,
    pub reserved0: u32,
}

impl Validate for IpcAttachmentOffsetsWasm32A0 {
    fn validate_at(&self, path: &str) -> Result<(), ContractError> {
        let field_path = child_path(path, "struct_size");
        let value = &self.struct_size;
        if *value > 0 {
            return Err(invalid(&field_path, "number exceeds its maximum"));
        }
        let field_path = child_path(path, "flags");
        let value = &self.flags;
        if *value < 4 {
            return Err(invalid(&field_path, "number is below its minimum"));
        }
        if *value > 4 {
            return Err(invalid(&field_path, "number exceeds its maximum"));
        }
        let field_path = child_path(path, "name");
        let value = &self.name;
        if *value < 8 {
            return Err(invalid(&field_path, "number is below its minimum"));
        }
        if *value > 8 {
            return Err(invalid(&field_path, "number exceeds its maximum"));
        }
        let field_path = child_path(path, "name_size");
        let value = &self.name_size;
        if *value < 12 {
            return Err(invalid(&field_path, "number is below its minimum"));
        }
        if *value > 12 {
            return Err(invalid(&field_path, "number exceeds its maximum"));
        }
        let field_path = child_path(path, "media_type");
        let value = &self.media_type;
        if *value < 16 {
            return Err(invalid(&field_path, "number is below its minimum"));
        }
        if *value > 16 {
            return Err(invalid(&field_path, "number exceeds its maximum"));
        }
        let field_path = child_path(path, "media_type_size");
        let value = &self.media_type_size;
        if *value < 20 {
            return Err(invalid(&field_path, "number is below its minimum"));
        }
        if *value > 20 {
            return Err(invalid(&field_path, "number exceeds its maximum"));
        }
        let field_path = child_path(path, "data");
        let value = &self.data;
        if *value < 24 {
            return Err(invalid(&field_path, "number is below its minimum"));
        }
        if *value > 24 {
            return Err(invalid(&field_path, "number exceeds its maximum"));
        }
        let field_path = child_path(path, "data_size");
        let value = &self.data_size;
        if *value < 28 {
            return Err(invalid(&field_path, "number is below its minimum"));
        }
        if *value > 28 {
            return Err(invalid(&field_path, "number exceeds its maximum"));
        }
        let field_path = child_path(path, "reserved0");
        let value = &self.reserved0;
        if *value < 32 {
            return Err(invalid(&field_path, "number is below its minimum"));
        }
        if *value > 32 {
            return Err(invalid(&field_path, "number exceeds its maximum"));
        }
        Ok(())
    }
}

#[derive(Clone, Debug, Deserialize, PartialEq, Serialize)]
#[serde(deny_unknown_fields)]
pub struct IpcAttachmentLayoutWasm32A0 {
    pub size: u32,
    pub offsets: IpcAttachmentOffsetsWasm32A0,
}

impl Validate for IpcAttachmentLayoutWasm32A0 {
    fn validate_at(&self, path: &str) -> Result<(), ContractError> {
        let field_path = child_path(path, "size");
        let value = &self.size;
        if *value < 36 {
            return Err(invalid(&field_path, "number is below its minimum"));
        }
        if *value > 36 {
            return Err(invalid(&field_path, "number exceeds its maximum"));
        }
        let field_path = child_path(path, "offsets");
        let value = &self.offsets;
        value.validate_at(&field_path)?;
        Ok(())
    }
}

#[derive(Clone, Debug, Deserialize, PartialEq, Serialize)]
#[serde(deny_unknown_fields)]
pub struct IpcAttachmentOffsetsPointer64A0 {
    pub struct_size: u32,
    pub flags: u32,
    pub name: u32,
    pub name_size: u32,
    pub media_type: u32,
    pub media_type_size: u32,
    pub data: u32,
    pub data_size: u32,
    pub reserved0: u32,
}

impl Validate for IpcAttachmentOffsetsPointer64A0 {
    fn validate_at(&self, path: &str) -> Result<(), ContractError> {
        let field_path = child_path(path, "struct_size");
        let value = &self.struct_size;
        if *value > 0 {
            return Err(invalid(&field_path, "number exceeds its maximum"));
        }
        let field_path = child_path(path, "flags");
        let value = &self.flags;
        if *value < 4 {
            return Err(invalid(&field_path, "number is below its minimum"));
        }
        if *value > 4 {
            return Err(invalid(&field_path, "number exceeds its maximum"));
        }
        let field_path = child_path(path, "name");
        let value = &self.name;
        if *value < 8 {
            return Err(invalid(&field_path, "number is below its minimum"));
        }
        if *value > 8 {
            return Err(invalid(&field_path, "number exceeds its maximum"));
        }
        let field_path = child_path(path, "name_size");
        let value = &self.name_size;
        if *value < 16 {
            return Err(invalid(&field_path, "number is below its minimum"));
        }
        if *value > 16 {
            return Err(invalid(&field_path, "number exceeds its maximum"));
        }
        let field_path = child_path(path, "media_type");
        let value = &self.media_type;
        if *value < 24 {
            return Err(invalid(&field_path, "number is below its minimum"));
        }
        if *value > 24 {
            return Err(invalid(&field_path, "number exceeds its maximum"));
        }
        let field_path = child_path(path, "media_type_size");
        let value = &self.media_type_size;
        if *value < 32 {
            return Err(invalid(&field_path, "number is below its minimum"));
        }
        if *value > 32 {
            return Err(invalid(&field_path, "number exceeds its maximum"));
        }
        let field_path = child_path(path, "data");
        let value = &self.data;
        if *value < 40 {
            return Err(invalid(&field_path, "number is below its minimum"));
        }
        if *value > 40 {
            return Err(invalid(&field_path, "number exceeds its maximum"));
        }
        let field_path = child_path(path, "data_size");
        let value = &self.data_size;
        if *value < 48 {
            return Err(invalid(&field_path, "number is below its minimum"));
        }
        if *value > 48 {
            return Err(invalid(&field_path, "number exceeds its maximum"));
        }
        let field_path = child_path(path, "reserved0");
        let value = &self.reserved0;
        if *value < 52 {
            return Err(invalid(&field_path, "number is below its minimum"));
        }
        if *value > 52 {
            return Err(invalid(&field_path, "number exceeds its maximum"));
        }
        Ok(())
    }
}

#[derive(Clone, Debug, Deserialize, PartialEq, Serialize)]
#[serde(deny_unknown_fields)]
pub struct IpcAttachmentLayoutPointer64A0 {
    pub size: u32,
    pub offsets: IpcAttachmentOffsetsPointer64A0,
}

impl Validate for IpcAttachmentLayoutPointer64A0 {
    fn validate_at(&self, path: &str) -> Result<(), ContractError> {
        let field_path = child_path(path, "size");
        let value = &self.size;
        if *value < 56 {
            return Err(invalid(&field_path, "number is below its minimum"));
        }
        if *value > 56 {
            return Err(invalid(&field_path, "number exceeds its maximum"));
        }
        let field_path = child_path(path, "offsets");
        let value = &self.offsets;
        value.validate_at(&field_path)?;
        Ok(())
    }
}

#[derive(Clone, Debug, Deserialize, PartialEq, Serialize)]
#[serde(deny_unknown_fields)]
pub struct IpcAttachmentDescriptorA0 {
    pub wasm32: IpcAttachmentLayoutWasm32A0,
    pub pointer64: IpcAttachmentLayoutPointer64A0,
}

impl Validate for IpcAttachmentDescriptorA0 {
    fn validate_at(&self, path: &str) -> Result<(), ContractError> {
        let field_path = child_path(path, "wasm32");
        let value = &self.wasm32;
        value.validate_at(&field_path)?;
        let field_path = child_path(path, "pointer64");
        let value = &self.pointer64;
        value.validate_at(&field_path)?;
        Ok(())
    }
}

#[derive(Clone, Debug, Deserialize, PartialEq, Serialize)]
#[serde(deny_unknown_fields)]
pub struct IpcCancelledA0 {
    pub status: String,
}

impl Validate for IpcCancelledA0 {
    fn validate_at(&self, path: &str) -> Result<(), ContractError> {
        let field_path = child_path(path, "status");
        let value = &self.status;
        if value != "cancelled" {
            return Err(invalid(
                &field_path,
                "literal value does not match the contract",
            ));
        }
        Ok(())
    }
}

#[derive(Clone, Debug, Deserialize, PartialEq, Serialize)]
#[serde(deny_unknown_fields)]
pub struct IpcCancelRejectedA0 {
    pub status: String,
    pub diagnostic: DiagnosticA0,
}

impl Validate for IpcCancelRejectedA0 {
    fn validate_at(&self, path: &str) -> Result<(), ContractError> {
        let field_path = child_path(path, "status");
        let value = &self.status;
        if value != "rejected" {
            return Err(invalid(
                &field_path,
                "literal value does not match the contract",
            ));
        }
        let field_path = child_path(path, "diagnostic");
        let value = &self.diagnostic;
        value.validate_at(&field_path)?;
        Ok(())
    }
}

#[derive(Clone, Debug, Deserialize, PartialEq, Serialize)]
#[serde(deny_unknown_fields)]
pub struct IpcEffectiveLimitsA0 {
    pub json_bytes: u32,
    pub attachment_count: u32,
    pub attachment_name_bytes: u32,
    pub attachment_media_type_bytes: u32,
    pub attachment_bytes: u32,
    pub frame_bytes: u32,
    pub queued_requests: u32,
    pub queued_bytes: u32,
    pub resident_request_bytes: u32,
    pub pending_writer_bytes: u32,
}

impl Validate for IpcEffectiveLimitsA0 {
    fn validate_at(&self, path: &str) -> Result<(), ContractError> {
        let field_path = child_path(path, "json_bytes");
        let value = &self.json_bytes;
        if *value > 8388608 {
            return Err(invalid(&field_path, "number exceeds its maximum"));
        }
        let field_path = child_path(path, "attachment_count");
        let value = &self.attachment_count;
        if *value > 16 {
            return Err(invalid(&field_path, "number exceeds its maximum"));
        }
        let field_path = child_path(path, "attachment_name_bytes");
        let value = &self.attachment_name_bytes;
        if *value > 128 {
            return Err(invalid(&field_path, "number exceeds its maximum"));
        }
        let field_path = child_path(path, "attachment_media_type_bytes");
        let value = &self.attachment_media_type_bytes;
        if *value > 128 {
            return Err(invalid(&field_path, "number exceeds its maximum"));
        }
        let field_path = child_path(path, "attachment_bytes");
        let value = &self.attachment_bytes;
        if *value > 268435456 {
            return Err(invalid(&field_path, "number exceeds its maximum"));
        }
        let field_path = child_path(path, "frame_bytes");
        let value = &self.frame_bytes;
        if *value > 536870912 {
            return Err(invalid(&field_path, "number exceeds its maximum"));
        }
        let field_path = child_path(path, "queued_requests");
        let value = &self.queued_requests;
        if *value > 8 {
            return Err(invalid(&field_path, "number exceeds its maximum"));
        }
        let field_path = child_path(path, "queued_bytes");
        let value = &self.queued_bytes;
        if *value > 536870912 {
            return Err(invalid(&field_path, "number exceeds its maximum"));
        }
        let field_path = child_path(path, "resident_request_bytes");
        let value = &self.resident_request_bytes;
        if *value > 536870912 {
            return Err(invalid(&field_path, "number exceeds its maximum"));
        }
        let field_path = child_path(path, "pending_writer_bytes");
        let value = &self.pending_writer_bytes;
        if *value > 536870912 {
            return Err(invalid(&field_path, "number exceeds its maximum"));
        }
        Ok(())
    }
}

#[derive(Clone, Debug, Deserialize, PartialEq, Serialize)]
#[serde(deny_unknown_fields)]
pub struct IpcGenericAbiLimitsA0 {
    pub operation_id_bytes: u32,
    pub request_json_bytes: u32,
    pub response_json_bytes: u32,
    pub attachment_count: u32,
    pub attachment_name_bytes: u32,
    pub attachment_media_type_bytes: u32,
    pub attachment_bytes: u32,
    pub aggregate_attachment_bytes_native: u32,
    pub aggregate_attachment_bytes_wasm: u32,
}

impl Validate for IpcGenericAbiLimitsA0 {
    fn validate_at(&self, path: &str) -> Result<(), ContractError> {
        let field_path = child_path(path, "operation_id_bytes");
        let value = &self.operation_id_bytes;
        if *value > 128 {
            return Err(invalid(&field_path, "number exceeds its maximum"));
        }
        let field_path = child_path(path, "request_json_bytes");
        let value = &self.request_json_bytes;
        if *value > 8388608 {
            return Err(invalid(&field_path, "number exceeds its maximum"));
        }
        let field_path = child_path(path, "response_json_bytes");
        let value = &self.response_json_bytes;
        if *value > 8388608 {
            return Err(invalid(&field_path, "number exceeds its maximum"));
        }
        let field_path = child_path(path, "attachment_count");
        let value = &self.attachment_count;
        if *value > 16 {
            return Err(invalid(&field_path, "number exceeds its maximum"));
        }
        let field_path = child_path(path, "attachment_name_bytes");
        let value = &self.attachment_name_bytes;
        if *value > 128 {
            return Err(invalid(&field_path, "number exceeds its maximum"));
        }
        let field_path = child_path(path, "attachment_media_type_bytes");
        let value = &self.attachment_media_type_bytes;
        if *value > 128 {
            return Err(invalid(&field_path, "number exceeds its maximum"));
        }
        let field_path = child_path(path, "attachment_bytes");
        let value = &self.attachment_bytes;
        if *value > 268435456 {
            return Err(invalid(&field_path, "number exceeds its maximum"));
        }
        let field_path = child_path(path, "aggregate_attachment_bytes_native");
        let value = &self.aggregate_attachment_bytes_native;
        if *value > 536870912 {
            return Err(invalid(&field_path, "number exceeds its maximum"));
        }
        let field_path = child_path(path, "aggregate_attachment_bytes_wasm");
        let value = &self.aggregate_attachment_bytes_wasm;
        if *value > 268435456 {
            return Err(invalid(&field_path, "number exceeds its maximum"));
        }
        Ok(())
    }
}

#[derive(Clone, Debug, Deserialize, PartialEq, Serialize)]
#[serde(deny_unknown_fields)]
pub struct IpcHelloA0 {
    pub client_name: String,
    pub client_version: String,
    pub protocols: Vec<String>,
    #[serde(
        default,
        deserialize_with = "deserialize_optional_non_null",
        skip_serializing_if = "Option::is_none"
    )]
    pub capabilities: Option<Vec<String>>,
}

impl Validate for IpcHelloA0 {
    fn validate_at(&self, path: &str) -> Result<(), ContractError> {
        let field_path = child_path(path, "client_name");
        let value = &self.client_name;
        if value.is_empty() {
            return Err(invalid(&field_path, "string is shorter than its minimum"));
        }
        if value.len() > 128 {
            return Err(invalid(&field_path, "string exceeds its maximum"));
        }
        let field_path = child_path(path, "client_version");
        let value = &self.client_version;
        if value.is_empty() {
            return Err(invalid(&field_path, "string is shorter than its minimum"));
        }
        if value.len() > 128 {
            return Err(invalid(&field_path, "string exceeds its maximum"));
        }
        let field_path = child_path(path, "protocols");
        let value = &self.protocols;
        if value.is_empty() {
            return Err(invalid(&field_path, "array is shorter than its minimum"));
        }
        if value.len() > 16 {
            return Err(invalid(&field_path, "array exceeds its maximum"));
        }
        let field_path = child_path(path, "capabilities");
        if let Some(value) = &self.capabilities {
            if value.len() > 64 {
                return Err(invalid(&field_path, "array exceeds its maximum"));
            }
        }
        Ok(())
    }
}

#[derive(Clone, Debug, Deserialize, PartialEq, Serialize)]
#[serde(deny_unknown_fields)]
pub struct IpcOperationDeclarationA0 {
    pub identity: String,
    pub request_contract: String,
    pub result_contract: String,
    pub input_attachments: Vec<IpcAttachmentDeclarationA0>,
    pub output_attachments: Vec<IpcAttachmentDeclarationA0>,
}

impl Validate for IpcOperationDeclarationA0 {
    fn validate_at(&self, path: &str) -> Result<(), ContractError> {
        let field_path = child_path(path, "identity");
        let value = &self.identity;
        if value.is_empty() {
            return Err(invalid(&field_path, "string is shorter than its minimum"));
        }
        if value.len() > 128 {
            return Err(invalid(&field_path, "string exceeds its maximum"));
        }
        let field_path = child_path(path, "request_contract");
        let value = &self.request_contract;
        if value.is_empty() {
            return Err(invalid(&field_path, "string is shorter than its minimum"));
        }
        if value.len() > 128 {
            return Err(invalid(&field_path, "string exceeds its maximum"));
        }
        let field_path = child_path(path, "result_contract");
        let value = &self.result_contract;
        if value.is_empty() {
            return Err(invalid(&field_path, "string is shorter than its minimum"));
        }
        if value.len() > 128 {
            return Err(invalid(&field_path, "string exceeds its maximum"));
        }
        let field_path = child_path(path, "input_attachments");
        let value = &self.input_attachments;
        if value.len() > 16 {
            return Err(invalid(&field_path, "array exceeds its maximum"));
        }
        for (index, item) in value.iter().enumerate() {
            item.validate_at(&child_path(&field_path, &index.to_string()))?;
        }
        let field_path = child_path(path, "output_attachments");
        let value = &self.output_attachments;
        if value.len() > 16 {
            return Err(invalid(&field_path, "array exceeds its maximum"));
        }
        for (index, item) in value.iter().enumerate() {
            item.validate_at(&child_path(&field_path, &index.to_string()))?;
        }
        Ok(())
    }
}

#[derive(Clone, Debug, Deserialize, PartialEq, Serialize)]
#[serde(deny_unknown_fields)]
pub struct IpcOperationCatalogA0 {
    pub catalog: String,
    pub generic_abi: String,
    pub release_version: String,
    pub c_abi_generation: u32,
    pub operations: Vec<IpcOperationDeclarationA0>,
    pub attachment_descriptor: IpcAttachmentDescriptorA0,
    pub limits: IpcGenericAbiLimitsA0,
}

impl Validate for IpcOperationCatalogA0 {
    fn validate_at(&self, path: &str) -> Result<(), ContractError> {
        let field_path = child_path(path, "catalog");
        let value = &self.catalog;
        if value != "wn.geometer.operation_catalog.a0" {
            return Err(invalid(
                &field_path,
                "literal value does not match the contract",
            ));
        }
        let field_path = child_path(path, "generic_abi");
        let value = &self.generic_abi;
        if value != "a0" {
            return Err(invalid(
                &field_path,
                "literal value does not match the contract",
            ));
        }
        let field_path = child_path(path, "release_version");
        let value = &self.release_version;
        if value.is_empty() {
            return Err(invalid(&field_path, "string is shorter than its minimum"));
        }
        if value.len() > 32 {
            return Err(invalid(&field_path, "string exceeds its maximum"));
        }
        let field_path = child_path(path, "operations");
        let value = &self.operations;
        if value.is_empty() {
            return Err(invalid(&field_path, "array is shorter than its minimum"));
        }
        for (index, item) in value.iter().enumerate() {
            item.validate_at(&child_path(&field_path, &index.to_string()))?;
        }
        let field_path = child_path(path, "attachment_descriptor");
        let value = &self.attachment_descriptor;
        value.validate_at(&field_path)?;
        let field_path = child_path(path, "limits");
        let value = &self.limits;
        value.validate_at(&field_path)?;
        Ok(())
    }
}

#[derive(Clone, Debug, Deserialize, PartialEq, Serialize)]
#[serde(deny_unknown_fields)]
pub struct IpcProtocolErrorA0 {
    pub status: String,
    pub diagnostic: DiagnosticA0,
}

impl Validate for IpcProtocolErrorA0 {
    fn validate_at(&self, path: &str) -> Result<(), ContractError> {
        let field_path = child_path(path, "status");
        let value = &self.status;
        if value != "protocol_error" {
            return Err(invalid(
                &field_path,
                "literal value does not match the contract",
            ));
        }
        let field_path = child_path(path, "diagnostic");
        let value = &self.diagnostic;
        value.validate_at(&field_path)?;
        Ok(())
    }
}

#[derive(Clone, Debug, Deserialize, PartialEq, Serialize)]
#[serde(deny_unknown_fields)]
pub struct IpcReasonA0 {
    #[serde(
        default,
        deserialize_with = "deserialize_optional_non_null",
        skip_serializing_if = "Option::is_none"
    )]
    pub reason: Option<String>,
}

impl Validate for IpcReasonA0 {
    fn validate_at(&self, path: &str) -> Result<(), ContractError> {
        let field_path = child_path(path, "reason");
        if let Some(value) = &self.reason {
            if value.len() > 1024 {
                return Err(invalid(&field_path, "string exceeds its maximum"));
            }
        }
        Ok(())
    }
}

#[derive(Clone, Debug, Deserialize, PartialEq, Serialize)]
pub enum ModelFormat {
    #[serde(rename = "step")]
    Step,
}

impl Validate for ModelFormat {
    fn validate_at(&self, _path: &str) -> Result<(), ContractError> {
        Ok(())
    }
}

pub type Matrix4x4 = [f64; 16];

impl Validate for Matrix4x4 {
    fn validate_at(&self, path: &str) -> Result<(), ContractError> {
        for (index, item) in self.iter().enumerate() {
            let item_path = child_path(path, &index.to_string());
            if !item.is_finite() {
                return Err(invalid(&item_path, "number must be finite"));
            }
        }
        Ok(())
    }
}

#[derive(Clone, Debug, Deserialize, PartialEq, Serialize)]
#[serde(deny_unknown_fields)]
pub struct ModelBoundsOptionsA0 {
    #[serde(
        default,
        deserialize_with = "deserialize_optional_non_null",
        skip_serializing_if = "Option::is_none"
    )]
    pub format: Option<ModelFormat>,
    #[serde(
        default,
        deserialize_with = "deserialize_optional_non_null",
        skip_serializing_if = "Option::is_none"
    )]
    pub model_transform: Option<Matrix4x4>,
}

impl Validate for ModelBoundsOptionsA0 {
    fn validate_at(&self, path: &str) -> Result<(), ContractError> {
        let field_path = child_path(path, "format");
        if let Some(value) = &self.format {
            value.validate_at(&field_path)?;
        }
        let field_path = child_path(path, "model_transform");
        if let Some(value) = &self.model_transform {
            value.validate_at(&field_path)?;
        }
        Ok(())
    }
}

#[derive(Clone, Debug, Deserialize, PartialEq, Serialize)]
#[serde(deny_unknown_fields)]
pub struct IpcRequestA0 {
    pub operation: String,
    pub request: ModelBoundsOptionsA0,
}

impl Validate for IpcRequestA0 {
    fn validate_at(&self, path: &str) -> Result<(), ContractError> {
        let field_path = child_path(path, "operation");
        let value = &self.operation;
        if value.is_empty() {
            return Err(invalid(&field_path, "string is shorter than its minimum"));
        }
        if value.len() > 128 {
            return Err(invalid(&field_path, "string exceeds its maximum"));
        }
        let field_path = child_path(path, "request");
        let value = &self.request;
        value.validate_at(&field_path)?;
        Ok(())
    }
}

#[derive(Clone, Debug, Deserialize, PartialEq, Serialize)]
#[serde(deny_unknown_fields)]
pub struct IpcShutdownAckA0 {
    pub status: String,
    #[serde(rename = "activeRequestCompleted")]
    pub active_request_completed: bool,
    #[serde(rename = "rejectedQueuedRequestCount")]
    pub rejected_queued_request_count: u32,
}

impl Validate for IpcShutdownAckA0 {
    fn validate_at(&self, path: &str) -> Result<(), ContractError> {
        let field_path = child_path(path, "status");
        let value = &self.status;
        if value != "complete" {
            return Err(invalid(
                &field_path,
                "literal value does not match the contract",
            ));
        }
        Ok(())
    }
}

#[derive(Clone, Debug, Deserialize, PartialEq, Serialize)]
#[serde(deny_unknown_fields)]
pub struct IpcWelcomeA0 {
    pub release_version: String,
    pub c_abi_generation: u32,
    pub ipc: String,
    pub catalog_sha256: String,
    pub operation_catalog: IpcOperationCatalogA0,
    pub limits: IpcEffectiveLimitsA0,
    pub capabilities: Vec<String>,
}

impl Validate for IpcWelcomeA0 {
    fn validate_at(&self, path: &str) -> Result<(), ContractError> {
        let field_path = child_path(path, "release_version");
        let value = &self.release_version;
        if value.is_empty() {
            return Err(invalid(&field_path, "string is shorter than its minimum"));
        }
        if value.len() > 32 {
            return Err(invalid(&field_path, "string exceeds its maximum"));
        }
        let field_path = child_path(path, "ipc");
        let value = &self.ipc;
        if value != "a0" {
            return Err(invalid(
                &field_path,
                "literal value does not match the contract",
            ));
        }
        let field_path = child_path(path, "catalog_sha256");
        let value = &self.catalog_sha256;
        if value.len() < 64 {
            return Err(invalid(&field_path, "string is shorter than its minimum"));
        }
        if value.len() > 64 {
            return Err(invalid(&field_path, "string exceeds its maximum"));
        }
        let field_path = child_path(path, "operation_catalog");
        let value = &self.operation_catalog;
        value.validate_at(&field_path)?;
        let field_path = child_path(path, "limits");
        let value = &self.limits;
        value.validate_at(&field_path)?;
        let field_path = child_path(path, "capabilities");
        let value = &self.capabilities;
        if value.len() > 64 {
            return Err(invalid(&field_path, "array exceeds its maximum"));
        }
        Ok(())
    }
}

#[derive(Clone, Debug, Deserialize, PartialEq, Serialize)]
#[serde(deny_unknown_fields)]
pub struct ModelBoundsSource {
    pub format: ModelFormat,
    pub hash: String,
}

impl Validate for ModelBoundsSource {
    fn validate_at(&self, path: &str) -> Result<(), ContractError> {
        let field_path = child_path(path, "format");
        let value = &self.format;
        value.validate_at(&field_path)?;
        Ok(())
    }
}

pub type Vector3 = [f64; 3];

impl Validate for Vector3 {
    fn validate_at(&self, path: &str) -> Result<(), ContractError> {
        for (index, item) in self.iter().enumerate() {
            let item_path = child_path(path, &index.to_string());
            if !item.is_finite() {
                return Err(invalid(&item_path, "number must be finite"));
            }
        }
        Ok(())
    }
}

#[derive(Clone, Debug, Deserialize, PartialEq, Serialize)]
#[serde(deny_unknown_fields)]
pub struct ModelBoundsValues {
    pub min: Vector3,
    pub max: Vector3,
    pub size: Vector3,
    pub center: Vector3,
}

impl Validate for ModelBoundsValues {
    fn validate_at(&self, path: &str) -> Result<(), ContractError> {
        let field_path = child_path(path, "min");
        let value = &self.min;
        value.validate_at(&field_path)?;
        let field_path = child_path(path, "max");
        let value = &self.max;
        value.validate_at(&field_path)?;
        let field_path = child_path(path, "size");
        let value = &self.size;
        value.validate_at(&field_path)?;
        let field_path = child_path(path, "center");
        let value = &self.center;
        value.validate_at(&field_path)?;
        Ok(())
    }
}

#[derive(Clone, Debug, Deserialize, PartialEq, Serialize)]
#[serde(deny_unknown_fields)]
pub struct ModelBoundsTimings {
    pub model_read_ms: f64,
    pub bounds_ms: f64,
}

impl Validate for ModelBoundsTimings {
    fn validate_at(&self, path: &str) -> Result<(), ContractError> {
        let field_path = child_path(path, "model_read_ms");
        let value = &self.model_read_ms;
        if !value.is_finite() {
            return Err(invalid(&field_path, "number must be finite"));
        }
        if *value < 0_f64 {
            return Err(invalid(&field_path, "number is below its minimum"));
        }
        let field_path = child_path(path, "bounds_ms");
        let value = &self.bounds_ms;
        if !value.is_finite() {
            return Err(invalid(&field_path, "number must be finite"));
        }
        if *value < 0_f64 {
            return Err(invalid(&field_path, "number is below its minimum"));
        }
        Ok(())
    }
}

#[derive(Clone, Debug, Deserialize, PartialEq, Serialize)]
#[serde(deny_unknown_fields)]
pub struct ModelBoundsResultA0 {
    pub schema: String,
    pub units: String,
    pub source: ModelBoundsSource,
    pub bounds: ModelBoundsValues,
    pub timings: ModelBoundsTimings,
}

impl Validate for ModelBoundsResultA0 {
    fn validate_at(&self, path: &str) -> Result<(), ContractError> {
        let field_path = child_path(path, "schema");
        let value = &self.schema;
        if value != "geometry.model_bounds.a0" {
            return Err(invalid(
                &field_path,
                "literal value does not match the contract",
            ));
        }
        let field_path = child_path(path, "units");
        let value = &self.units;
        if value != "mm" {
            return Err(invalid(
                &field_path,
                "literal value does not match the contract",
            ));
        }
        let field_path = child_path(path, "source");
        let value = &self.source;
        value.validate_at(&field_path)?;
        let field_path = child_path(path, "bounds");
        let value = &self.bounds;
        value.validate_at(&field_path)?;
        let field_path = child_path(path, "timings");
        let value = &self.timings;
        value.validate_at(&field_path)?;
        Ok(())
    }
}

#[derive(Clone, Debug, Deserialize, PartialEq, Serialize)]
#[serde(deny_unknown_fields)]
pub struct OperationFailureA0 {
    pub operation: String,
    pub ok: bool,
    pub diagnostics: Vec<DiagnosticA0>,
}

impl Validate for OperationFailureA0 {
    fn validate_at(&self, path: &str) -> Result<(), ContractError> {
        let field_path = child_path(path, "operation");
        let value = &self.operation;
        if value.is_empty() {
            return Err(invalid(&field_path, "string is shorter than its minimum"));
        }
        if value.len() > 128 {
            return Err(invalid(&field_path, "string exceeds its maximum"));
        }
        let field_path = child_path(path, "ok");
        let value = &self.ok;
        if *value {
            return Err(invalid(
                &field_path,
                "literal value does not match the contract",
            ));
        }
        let field_path = child_path(path, "diagnostics");
        let value = &self.diagnostics;
        if value.is_empty() {
            return Err(invalid(&field_path, "array is shorter than its minimum"));
        }
        for (index, item) in value.iter().enumerate() {
            item.validate_at(&child_path(&field_path, &index.to_string()))?;
        }
        Ok(())
    }
}

#[derive(Clone, Debug, Deserialize, PartialEq, Serialize)]
#[serde(untagged)]
pub enum OperationResultValueA0 {
    ModelBounds(ModelBoundsResultA0),
}

impl Validate for OperationResultValueA0 {
    fn validate_at(&self, path: &str) -> Result<(), ContractError> {
        match self {
            Self::ModelBounds(value) => value.validate_at(path),
        }
    }
}

#[derive(Clone, Debug, Deserialize, PartialEq, Serialize)]
#[serde(deny_unknown_fields)]
pub struct OperationSuccessA0 {
    pub operation: String,
    pub ok: bool,
    pub result: OperationResultValueA0,
}

impl Validate for OperationSuccessA0 {
    fn validate_at(&self, path: &str) -> Result<(), ContractError> {
        let field_path = child_path(path, "operation");
        let value = &self.operation;
        if value.is_empty() {
            return Err(invalid(&field_path, "string is shorter than its minimum"));
        }
        if value.len() > 128 {
            return Err(invalid(&field_path, "string exceeds its maximum"));
        }
        let field_path = child_path(path, "ok");
        let value = &self.ok;
        if !*value {
            return Err(invalid(
                &field_path,
                "literal value does not match the contract",
            ));
        }
        let field_path = child_path(path, "result");
        let value = &self.result;
        value.validate_at(&field_path)?;
        Ok(())
    }
}

#[derive(Clone, Debug, Deserialize, PartialEq, Serialize)]
#[serde(untagged)]
pub enum OperationOutcomeA0 {
    Success(OperationSuccessA0),
    Failure(OperationFailureA0),
}

impl Validate for OperationOutcomeA0 {
    fn validate_at(&self, path: &str) -> Result<(), ContractError> {
        match self {
            Self::Success(value) => value.validate_at(path),
            Self::Failure(value) => value.validate_at(path),
        }
    }
}

pub fn decode_diagnostic_a0_json(data: &[u8]) -> Result<DiagnosticA0, ContractError> {
    decode_json(data)
}

pub fn encode_diagnostic_a0_json(value: &DiagnosticA0) -> Result<Vec<u8>, ContractError> {
    encode_json(value)
}

pub fn decode_ipc_cancelled_a0_json(data: &[u8]) -> Result<IpcCancelledA0, ContractError> {
    decode_json(data)
}

pub fn encode_ipc_cancelled_a0_json(value: &IpcCancelledA0) -> Result<Vec<u8>, ContractError> {
    encode_json(value)
}

pub fn decode_ipc_cancel_rejected_a0_json(
    data: &[u8],
) -> Result<IpcCancelRejectedA0, ContractError> {
    decode_json(data)
}

pub fn encode_ipc_cancel_rejected_a0_json(
    value: &IpcCancelRejectedA0,
) -> Result<Vec<u8>, ContractError> {
    encode_json(value)
}

pub fn decode_ipc_hello_a0_json(data: &[u8]) -> Result<IpcHelloA0, ContractError> {
    decode_json(data)
}

pub fn encode_ipc_hello_a0_json(value: &IpcHelloA0) -> Result<Vec<u8>, ContractError> {
    encode_json(value)
}

pub fn decode_ipc_operation_catalog_a0_json(
    data: &[u8],
) -> Result<IpcOperationCatalogA0, ContractError> {
    decode_json(data)
}

pub fn encode_ipc_operation_catalog_a0_json(
    value: &IpcOperationCatalogA0,
) -> Result<Vec<u8>, ContractError> {
    encode_json(value)
}

pub fn decode_ipc_protocol_error_a0_json(data: &[u8]) -> Result<IpcProtocolErrorA0, ContractError> {
    decode_json(data)
}

pub fn encode_ipc_protocol_error_a0_json(
    value: &IpcProtocolErrorA0,
) -> Result<Vec<u8>, ContractError> {
    encode_json(value)
}

pub fn decode_ipc_reason_a0_json(data: &[u8]) -> Result<IpcReasonA0, ContractError> {
    decode_json(data)
}

pub fn encode_ipc_reason_a0_json(value: &IpcReasonA0) -> Result<Vec<u8>, ContractError> {
    encode_json(value)
}

pub fn decode_ipc_request_a0_json(data: &[u8]) -> Result<IpcRequestA0, ContractError> {
    decode_json(data)
}

pub fn encode_ipc_request_a0_json(value: &IpcRequestA0) -> Result<Vec<u8>, ContractError> {
    encode_json(value)
}

pub fn decode_ipc_shutdown_ack_a0_json(data: &[u8]) -> Result<IpcShutdownAckA0, ContractError> {
    decode_json(data)
}

pub fn encode_ipc_shutdown_ack_a0_json(value: &IpcShutdownAckA0) -> Result<Vec<u8>, ContractError> {
    encode_json(value)
}

pub fn decode_ipc_welcome_a0_json(data: &[u8]) -> Result<IpcWelcomeA0, ContractError> {
    decode_json(data)
}

pub fn encode_ipc_welcome_a0_json(value: &IpcWelcomeA0) -> Result<Vec<u8>, ContractError> {
    encode_json(value)
}

pub fn decode_model_bounds_options_a0_json(
    data: &[u8],
) -> Result<ModelBoundsOptionsA0, ContractError> {
    decode_json(data)
}

pub fn encode_model_bounds_options_a0_json(
    value: &ModelBoundsOptionsA0,
) -> Result<Vec<u8>, ContractError> {
    encode_json(value)
}

pub fn decode_model_bounds_result_a0_json(
    data: &[u8],
) -> Result<ModelBoundsResultA0, ContractError> {
    decode_json(data)
}

pub fn encode_model_bounds_result_a0_json(
    value: &ModelBoundsResultA0,
) -> Result<Vec<u8>, ContractError> {
    encode_json(value)
}

pub fn decode_operation_outcome_a0_json(data: &[u8]) -> Result<OperationOutcomeA0, ContractError> {
    decode_json(data)
}

pub fn encode_operation_outcome_a0_json(
    value: &OperationOutcomeA0,
) -> Result<Vec<u8>, ContractError> {
    encode_json(value)
}
