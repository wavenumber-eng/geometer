//! Owning illustration drawing geometry through governed JSON attachments.

use crate::contracts::{
    self, HlrProjectionResultA0, MeshCollectionA0, MeshIllustrationGeometryA0,
    MeshIllustrationGeometryInputA0, MeshIllustrationGeometryRequestA0, OperationOutcomeA0,
    OperationResultValueA0, Validate,
};
use crate::ipc::Attachment;
use crate::{GeometerClient, GeometerClientError, OperationResponse};
use sha2::{Digest, Sha256};

impl GeometerClient {
    /// Return ordered shaded millimeter surfaces and lines without generating SVG.
    pub async fn mesh_illustration_geometry(
        &self,
        input: MeshIllustrationGeometryInputA0,
    ) -> Result<MeshIllustrationGeometryA0, GeometerClientError> {
        self.illustration_geometry_request(input, None).await
    }

    /// Compose visible-only millimeter HLR from the same model/placement/view.
    pub async fn mesh_illustration_geometry_with_hlr(
        &self,
        input: MeshIllustrationGeometryInputA0,
        hlr: HlrProjectionResultA0,
    ) -> Result<MeshIllustrationGeometryA0, GeometerClientError> {
        self.illustration_geometry_request(input, Some(hlr)).await
    }

    async fn illustration_geometry_request(
        &self,
        input: MeshIllustrationGeometryInputA0,
        hlr: Option<HlrProjectionResultA0>,
    ) -> Result<MeshIllustrationGeometryA0, GeometerClientError> {
        input.validate_at("")?;
        let request = MeshIllustrationGeometryRequestA0 {
            schema: "geometry.mesh_illustration_geometry.request.a0".to_owned(),
            view: input.view,
            prepare: input.prepare,
            style: input.style,
        };
        let collection = MeshCollectionA0 {
            schema: "geometry.mesh_collection.a0".to_owned(),
            length_unit: input.length_unit,
            meshes: input.meshes,
        };
        let mut attachments = vec![Attachment {
            name: "mesh_collection".to_owned(),
            media_type: "application/vnd.wavenumber.geometer.mesh-collection+json".to_owned(),
            data: contracts::encode_json(&collection)?,
        }];
        if let Some(hlr) = hlr {
            attachments.push(Attachment {
                name: "hlr_projection".to_owned(),
                media_type: "application/vnd.wavenumber.geometer.hlr-projection+json".to_owned(),
                data: contracts::encode_json(&hlr)?,
            });
        }
        let response = self
            .execute(
                "geometry.mesh_illustration_geometry.a0",
                &contracts::encode_json(&request)?,
                attachments,
            )
            .await?;
        let result = decode_response(response);
        if matches!(result, Err(GeometerClientError::Protocol(_))) {
            self.terminate().await?;
        }
        result
    }
}

fn invalid(message: &str) -> GeometerClientError {
    GeometerClientError::Protocol(format!("mesh_illustration_geometry: {message}"))
}

fn decode_response(
    response: OperationResponse,
) -> Result<MeshIllustrationGeometryA0, GeometerClientError> {
    let success = match response.outcome {
        OperationOutcomeA0::Failure(failure) => {
            return Err(GeometerClientError::Operation {
                operation: failure.operation,
                diagnostics: failure.diagnostics,
            });
        }
        OperationOutcomeA0::Success(value) => value,
    };
    let OperationResultValueA0::MeshIllustrationGeometry(metadata) = success.result else {
        return Err(invalid("wrong result type"));
    };
    if response.attachments.len() != 1 {
        return Err(invalid("expected one geometry attachment"));
    }
    let attachment = &response.attachments[0];
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
    validate_geometry_counts(&geometry)?;
    Ok(geometry)
}

fn validate_geometry_counts(
    geometry: &MeshIllustrationGeometryA0,
) -> Result<(), GeometerClientError> {
    if geometry
        .bounds
        .min
        .iter()
        .zip(&geometry.bounds.max)
        .any(|(a, b)| a > b)
    {
        return Err(invalid("geometry bounds are reversed"));
    }
    let (mut layers, mut rings, mut points) = (0_usize, 0_usize, 0_usize);
    for surface in &geometry.surfaces {
        layers += surface.layers.len();
        for layer in &surface.layers {
            rings += layer.rings.len();
            points += layer
                .rings
                .iter()
                .map(|ring| ring.points.len())
                .sum::<usize>();
            if layers > 2000000 || rings > 2000000 || points > 6000000 {
                return Err(invalid("geometry exceeds aggregate count limit"));
            }
        }
    }
    if layers != geometry.stats.surface_draws as usize
        || layers + geometry.lines.len() != geometry.stats.commands as usize
    {
        return Err(invalid("geometry draw counts mismatch"));
    }
    Ok(())
}
