//! One-pass STEP or analytic illustration without an intermediate mesh attachment.

#[cfg(feature = "direct-static")]
use crate::GeometerDirectClient;
use crate::contracts::{
    self, MeshIllustrationGeometryA0, MeshIllustrationGeometryB0,
    ModelIllustrationGeometryRequestA0, ModelIllustrationGeometryRequestB0,
    ModelIllustrationGeometryResultA0, ModelIllustrationGeometryResultB0,
    ModelIllustrationRequestA0, ModelIllustrationRequestB0, ModelIllustrationResultA0,
    ModelIllustrationResultB0, ModelIllustrationSourceA0, OperationOutcomeA0, OperationOutcomeB0,
    OperationResultValueA0, OperationResultValueB0, Validate,
};
use crate::ipc::Attachment;
use crate::{GeometerClient, GeometerClientError, OperationResponse};
use sha2::{Digest, Sha256};

/// Governed operation metadata paired with renderer-neutral drawing geometry.
#[derive(Debug, Clone, PartialEq)]
pub struct ModelIllustrationGeometryB0 {
    pub metadata: ModelIllustrationGeometryResultB0,
    pub geometry: MeshIllustrationGeometryB0,
}

/// Strict A0 compatibility metadata paired with renderer-neutral geometry.
#[derive(Debug, Clone, PartialEq)]
pub struct ModelIllustrationGeometry {
    pub metadata: ModelIllustrationGeometryResultA0,
    pub geometry: MeshIllustrationGeometryA0,
}

macro_rules! impl_model_illustration_client {
    ($client:ty) => {
        impl $client {
            /// Illustrate one STEP model or analytic scene without transferring a mesh collection.
            pub async fn model_illustration(
                &self,
                request: ModelIllustrationRequestB0,
                model: Option<Vec<u8>>,
            ) -> Result<ModelIllustrationResultB0, GeometerClientError> {
                run_model_illustration(self, request, model).await
            }

            /// Return renderer-neutral geometry for one STEP model or analytic scene.
            pub async fn model_illustration_geometry(
                &self,
                request: ModelIllustrationGeometryRequestB0,
                model: Option<Vec<u8>>,
            ) -> Result<ModelIllustrationGeometryB0, GeometerClientError> {
                run_model_illustration_geometry(self, request, model).await
            }

            /// Execute the strict A0 compatibility model illustration operation.
            pub async fn model_illustration_a0(
                &self,
                request: ModelIllustrationRequestA0,
                model: Option<Vec<u8>>,
            ) -> Result<ModelIllustrationResultA0, GeometerClientError> {
                run_model_illustration_a0(self, request, model).await
            }

            /// Execute the strict A0 compatibility model geometry operation.
            pub async fn model_illustration_geometry_a0(
                &self,
                request: ModelIllustrationGeometryRequestA0,
                model: Option<Vec<u8>>,
            ) -> Result<ModelIllustrationGeometry, GeometerClientError> {
                run_model_illustration_geometry_a0(self, request, model).await
            }

            /// Generation-explicit alias for the canonical B0 operation.
            pub async fn model_illustration_b0(
                &self,
                request: ModelIllustrationRequestB0,
                model: Option<Vec<u8>>,
            ) -> Result<ModelIllustrationResultB0, GeometerClientError> {
                run_model_illustration(self, request, model).await
            }

            /// Generation-explicit alias for the canonical B0 geometry operation.
            pub async fn model_illustration_geometry_b0(
                &self,
                request: ModelIllustrationGeometryRequestB0,
                model: Option<Vec<u8>>,
            ) -> Result<ModelIllustrationGeometryB0, GeometerClientError> {
                run_model_illustration_geometry(self, request, model).await
            }
        }
    };
}

impl_model_illustration_client!(GeometerClient);
#[cfg(feature = "direct-static")]
impl_model_illustration_client!(GeometerDirectClient);

async fn run_model_illustration<B: crate::backend::OperationBackend>(
    backend: &B,
    request: ModelIllustrationRequestB0,
    model: Option<Vec<u8>>,
) -> Result<ModelIllustrationResultB0, GeometerClientError> {
    request.validate_at("")?;
    let attachments = source_attachments(&request.source, model)?;
    let response = backend
        .execute_operation(
            "geometry.model_illustration.b0",
            &contracts::encode_json(&request)?,
            attachments,
        )
        .await?;
    crate::backend::contain_protocol_result(backend, decode_svg_response(response)).await
}

async fn run_model_illustration_geometry<B: crate::backend::OperationBackend>(
    backend: &B,
    request: ModelIllustrationGeometryRequestB0,
    model: Option<Vec<u8>>,
) -> Result<ModelIllustrationGeometryB0, GeometerClientError> {
    request.validate_at("")?;
    let attachments = source_attachments(&request.source, model)?;
    let response = backend
        .execute_operation(
            "geometry.model_illustration_geometry.b0",
            &contracts::encode_json(&request)?,
            attachments,
        )
        .await?;
    crate::backend::contain_protocol_result(backend, decode_geometry_response(response)).await
}

fn source_attachments(
    source: &ModelIllustrationSourceA0,
    model: Option<Vec<u8>>,
) -> Result<Vec<Attachment>, GeometerClientError> {
    match (source, model) {
        (ModelIllustrationSourceA0::ModelSource(_), Some(data)) => Ok(vec![Attachment {
            name: "model".to_owned(),
            media_type: "application/step".to_owned(),
            data,
        }]),
        (ModelIllustrationSourceA0::AnalyticSource(_), None) => Ok(Vec::new()),
        (ModelIllustrationSourceA0::ModelSource(_), None) => {
            Err(invalid("a model source requires STEP bytes"))
        }
        (ModelIllustrationSourceA0::AnalyticSource(_), Some(_)) => {
            Err(invalid("an analytic source does not accept model bytes"))
        }
    }
}

fn success(
    response: OperationResponse,
) -> Result<(OperationResultValueB0, Vec<Attachment>), GeometerClientError> {
    match response.outcome.into_b0()? {
        OperationOutcomeB0::Failure(failure) => Err(GeometerClientError::Operation {
            operation: failure.operation,
            diagnostics: failure.diagnostics,
        }),
        OperationOutcomeB0::Success(value) => Ok((value.result, response.attachments)),
    }
}

fn decode_svg_response(
    response: OperationResponse,
) -> Result<ModelIllustrationResultB0, GeometerClientError> {
    let (result, attachments) = success(response)?;
    let OperationResultValueB0::ModelIllustration(result) = result else {
        return Err(invalid("wrong SVG result type"));
    };
    if !attachments.is_empty() {
        return Err(invalid("SVG response contains attachments"));
    }
    Ok(result)
}

fn decode_geometry_response(
    response: OperationResponse,
) -> Result<ModelIllustrationGeometryB0, GeometerClientError> {
    let (result, attachments) = success(response)?;
    let OperationResultValueB0::ModelIllustrationGeometry(metadata) = result else {
        return Err(invalid("wrong geometry result type"));
    };
    if attachments.len() != 1 {
        return Err(invalid("expected one geometry attachment"));
    }
    let attachment = &attachments[0];
    if attachment.name != "illustration_geometry"
        || attachment.media_type != "application/vnd.wavenumber.geometer.illustration-geometry+json"
        || attachment.data.len() != metadata.geometry.byte_length as usize
        || format!("{:x}", Sha256::digest(&attachment.data)) != metadata.geometry.sha256
    {
        return Err(invalid("geometry attachment metadata mismatch"));
    }
    let geometry = contracts::decode_mesh_illustration_geometry_b0_json(&attachment.data)
        .map_err(|error| invalid(&format!("invalid geometry: {error}")))?;
    if geometry.stats != metadata.stats
        || geometry.warnings != metadata.warnings
        || geometry.empty != metadata.empty
        || geometry.fragment != metadata.fragment
    {
        return Err(invalid("geometry metadata mismatch"));
    }
    Ok(ModelIllustrationGeometryB0 { metadata, geometry })
}

async fn run_model_illustration_a0<B: crate::backend::OperationBackend>(
    backend: &B,
    request: ModelIllustrationRequestA0,
    model: Option<Vec<u8>>,
) -> Result<ModelIllustrationResultA0, GeometerClientError> {
    request.validate_at("")?;
    let attachments = source_attachments(&request.source, model)?;
    let response = backend
        .execute_operation(
            "geometry.model_illustration.a0",
            &contracts::encode_json(&request)?,
            attachments,
        )
        .await?;
    let (result, attachments) = success_a0(response)?;
    let OperationResultValueA0::ModelIllustration(result) = result else {
        return Err(invalid("wrong A0 SVG result type"));
    };
    if !attachments.is_empty() {
        return Err(invalid("A0 SVG response contains attachments"));
    }
    Ok(result)
}

async fn run_model_illustration_geometry_a0<B: crate::backend::OperationBackend>(
    backend: &B,
    request: ModelIllustrationGeometryRequestA0,
    model: Option<Vec<u8>>,
) -> Result<ModelIllustrationGeometry, GeometerClientError> {
    request.validate_at("")?;
    let attachments = source_attachments(&request.source, model)?;
    let response = backend
        .execute_operation(
            "geometry.model_illustration_geometry.a0",
            &contracts::encode_json(&request)?,
            attachments,
        )
        .await?;
    let (result, attachments) = success_a0(response)?;
    let OperationResultValueA0::ModelIllustrationGeometry(metadata) = result else {
        return Err(invalid("wrong A0 geometry result type"));
    };
    if attachments.len() != 1 {
        return Err(invalid("expected one A0 geometry attachment"));
    }
    let attachment = &attachments[0];
    if attachment.name != "illustration_geometry"
        || attachment.media_type != "application/vnd.wavenumber.geometer.illustration-geometry+json"
        || attachment.data.len() != metadata.geometry.byte_length as usize
        || format!("{:x}", Sha256::digest(&attachment.data)) != metadata.geometry.sha256
    {
        return Err(invalid("A0 geometry attachment metadata mismatch"));
    }
    let geometry = contracts::decode_mesh_illustration_geometry_a0_json(&attachment.data)
        .map_err(|error| invalid(&format!("invalid A0 geometry: {error}")))?;
    if geometry.stats != metadata.stats || geometry.warnings != metadata.warnings {
        return Err(invalid("A0 geometry statistics/warnings mismatch"));
    }
    Ok(ModelIllustrationGeometry { metadata, geometry })
}

fn success_a0(
    response: OperationResponse,
) -> Result<(OperationResultValueA0, Vec<Attachment>), GeometerClientError> {
    match response.outcome.into_a0()? {
        OperationOutcomeA0::Failure(failure) => Err(GeometerClientError::Operation {
            operation: failure.operation,
            diagnostics: failure.diagnostics,
        }),
        OperationOutcomeA0::Success(value) => Ok((value.result, response.attachments)),
    }
}

fn invalid(message: &str) -> GeometerClientError {
    GeometerClientError::Protocol(format!("model_illustration: {message}"))
}
