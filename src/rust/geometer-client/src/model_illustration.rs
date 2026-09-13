//! One-pass STEP or analytic illustration without an intermediate mesh attachment.

use crate::contracts::{
    self, MeshIllustrationGeometryA0, ModelIllustrationGeometryRequestA0,
    ModelIllustrationGeometryResultA0, ModelIllustrationRequestA0, ModelIllustrationResultA0,
    ModelIllustrationSourceA0, OperationOutcomeA0, OperationResultValueA0, Validate,
};
use crate::ipc::Attachment;
use crate::{GeometerClient, GeometerClientError, OperationResponse};
use sha2::{Digest, Sha256};

/// Governed operation metadata paired with renderer-neutral drawing geometry.
#[derive(Debug, Clone, PartialEq)]
pub struct ModelIllustrationGeometry {
    pub metadata: ModelIllustrationGeometryResultA0,
    pub geometry: MeshIllustrationGeometryA0,
}

impl GeometerClient {
    /// Illustrate one STEP model or analytic scene without transferring a mesh collection.
    pub async fn model_illustration(
        &self,
        request: ModelIllustrationRequestA0,
        model: Option<Vec<u8>>,
    ) -> Result<ModelIllustrationResultA0, GeometerClientError> {
        request.validate_at("")?;
        let attachments = source_attachments(&request.source, model)?;
        let response = self
            .execute(
                "geometry.model_illustration.a0",
                &contracts::encode_json(&request)?,
                attachments,
            )
            .await?;
        let result = decode_svg_response(response);
        if matches!(result, Err(GeometerClientError::Protocol(_))) {
            self.terminate().await?;
        }
        result
    }

    /// Return renderer-neutral geometry for one STEP model or analytic scene.
    pub async fn model_illustration_geometry(
        &self,
        request: ModelIllustrationGeometryRequestA0,
        model: Option<Vec<u8>>,
    ) -> Result<ModelIllustrationGeometry, GeometerClientError> {
        request.validate_at("")?;
        let attachments = source_attachments(&request.source, model)?;
        let response = self
            .execute(
                "geometry.model_illustration_geometry.a0",
                &contracts::encode_json(&request)?,
                attachments,
            )
            .await?;
        let result = decode_geometry_response(response);
        if matches!(result, Err(GeometerClientError::Protocol(_))) {
            self.terminate().await?;
        }
        result
    }
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
) -> Result<(OperationResultValueA0, Vec<Attachment>), GeometerClientError> {
    match response.outcome {
        OperationOutcomeA0::Failure(failure) => Err(GeometerClientError::Operation {
            operation: failure.operation,
            diagnostics: failure.diagnostics,
        }),
        OperationOutcomeA0::Success(value) => Ok((value.result, response.attachments)),
    }
}

fn decode_svg_response(
    response: OperationResponse,
) -> Result<ModelIllustrationResultA0, GeometerClientError> {
    let (result, attachments) = success(response)?;
    let OperationResultValueA0::ModelIllustration(result) = result else {
        return Err(invalid("wrong SVG result type"));
    };
    if !attachments.is_empty() {
        return Err(invalid("SVG response contains attachments"));
    }
    Ok(result)
}

fn decode_geometry_response(
    response: OperationResponse,
) -> Result<ModelIllustrationGeometry, GeometerClientError> {
    let (result, attachments) = success(response)?;
    let OperationResultValueA0::ModelIllustrationGeometry(metadata) = result else {
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
    let geometry = contracts::decode_mesh_illustration_geometry_a0_json(&attachment.data)
        .map_err(|error| invalid(&format!("invalid geometry: {error}")))?;
    if geometry.stats != metadata.stats || geometry.warnings != metadata.warnings {
        return Err(invalid("geometry statistics/warnings mismatch"));
    }
    Ok(ModelIllustrationGeometry { metadata, geometry })
}

fn invalid(message: &str) -> GeometerClientError {
    GeometerClientError::Protocol(format!("model_illustration: {message}"))
}
