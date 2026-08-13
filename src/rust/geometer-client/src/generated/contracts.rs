// Generated from wn_geometer_contract_catalog.a0.json. Do not edit.

use serde::{Deserialize, Serialize, de::DeserializeOwned};

pub const NORMALIZED_CATALOG_SHA256: &str =
    "9691a8249841870e01f5c090ed08a12772d60b402c090352a0978267cae2c670";

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
pub enum ModelFormat {
    #[serde(rename = "step")]
    Step,
}

impl Validate for ModelFormat {
    fn validate_at(&self, _path: &str) -> Result<(), ContractError> {
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
