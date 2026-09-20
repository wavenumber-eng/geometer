//! Owning illustration drawing geometry through governed JSON attachments.

#[cfg(feature = "direct-static")]
use crate::GeometerDirectClient;
use crate::contracts::{
    self, HlrProjectionResultA0, HlrProjectionResultB0, MeshCollectionA0,
    MeshIllustrationGeometryA0, MeshIllustrationGeometryB0, MeshIllustrationGeometryInputA0,
    MeshIllustrationGeometryInputB0, MeshIllustrationGeometryRequestA0,
    MeshIllustrationGeometryRequestB0, OperationOutcomeA0, OperationOutcomeB0,
    OperationResultValueA0, OperationResultValueB0, Validate,
};
use crate::ipc::Attachment;
use crate::{GeometerClient, GeometerClientError, OperationResponse};
use sha2::{Digest, Sha256};

macro_rules! impl_illustration_geometry_client {
    ($client:ty) => {
        impl $client {
            /// Return ordered shaded millimeter surfaces and lines without generating SVG.
            pub async fn mesh_illustration_geometry(
                &self,
                input: MeshIllustrationGeometryInputB0,
            ) -> Result<MeshIllustrationGeometryB0, GeometerClientError> {
                run_illustration_geometry_request(self, input, None).await
            }

            /// Compose visible-only millimeter HLR from the same model/placement/view.
            pub async fn mesh_illustration_geometry_with_hlr(
                &self,
                input: MeshIllustrationGeometryInputB0,
                hlr: HlrProjectionResultB0,
            ) -> Result<MeshIllustrationGeometryB0, GeometerClientError> {
                run_illustration_geometry_request(self, input, Some(hlr)).await
            }

            /// Execute the strict A0 compatibility geometry operation.
            pub async fn mesh_illustration_geometry_a0(
                &self,
                input: MeshIllustrationGeometryInputA0,
            ) -> Result<MeshIllustrationGeometryA0, GeometerClientError> {
                run_illustration_geometry_request_a0(self, input, None).await
            }

            /// Execute the strict A0 compatibility geometry composition operation.
            pub async fn mesh_illustration_geometry_with_hlr_a0(
                &self,
                input: MeshIllustrationGeometryInputA0,
                hlr: HlrProjectionResultA0,
            ) -> Result<MeshIllustrationGeometryA0, GeometerClientError> {
                run_illustration_geometry_request_a0(self, input, Some(hlr)).await
            }

            /// Generation-explicit alias for the canonical B0 geometry operation.
            pub async fn mesh_illustration_geometry_b0(
                &self,
                input: MeshIllustrationGeometryInputB0,
            ) -> Result<MeshIllustrationGeometryB0, GeometerClientError> {
                run_illustration_geometry_request(self, input, None).await
            }

            /// Generation-explicit alias for canonical B0 geometry composition.
            pub async fn mesh_illustration_geometry_with_hlr_b0(
                &self,
                input: MeshIllustrationGeometryInputB0,
                hlr: HlrProjectionResultB0,
            ) -> Result<MeshIllustrationGeometryB0, GeometerClientError> {
                run_illustration_geometry_request(self, input, Some(hlr)).await
            }
        }
    };
}

impl_illustration_geometry_client!(GeometerClient);
#[cfg(feature = "direct-static")]
impl_illustration_geometry_client!(GeometerDirectClient);

async fn run_illustration_geometry_request<B: crate::backend::OperationBackend>(
    backend: &B,
    input: MeshIllustrationGeometryInputB0,
    hlr: Option<HlrProjectionResultB0>,
) -> Result<MeshIllustrationGeometryB0, GeometerClientError> {
    input.validate_at("")?;
    let request = MeshIllustrationGeometryRequestB0 {
        schema: "geometry.mesh_illustration_geometry.request.b0".to_owned(),
        view: input.view,
        prepare: input.prepare,
        style: input.style,
        clipping: input.clipping,
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
    let response = backend
        .execute_operation(
            "geometry.mesh_illustration_geometry.b0",
            &contracts::encode_json(&request)?,
            attachments,
        )
        .await?;
    crate::backend::contain_protocol_result(backend, decode_response_b0(response)).await
}

fn invalid(message: &str) -> GeometerClientError {
    GeometerClientError::Protocol(format!("mesh_illustration_geometry: {message}"))
}

fn decode_response_b0(
    response: OperationResponse,
) -> Result<MeshIllustrationGeometryB0, GeometerClientError> {
    let success = match response.outcome.into_b0()? {
        OperationOutcomeB0::Failure(failure) => {
            return Err(GeometerClientError::Operation {
                operation: failure.operation,
                diagnostics: failure.diagnostics,
            });
        }
        OperationOutcomeB0::Success(value) => value,
    };
    let OperationResultValueB0::MeshIllustrationGeometry(metadata) = success.result else {
        return Err(invalid("wrong result type"));
    };
    if response.attachments.len() != 1 {
        return Err(invalid("expected one geometry attachment"));
    }
    let attachment = &response.attachments[0];
    if !geometry_attachment_matches(
        attachment,
        metadata.geometry.byte_length as usize,
        &metadata.geometry.sha256,
    ) {
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
    validate_geometry_counts(&geometry)?;
    Ok(geometry)
}

fn geometry_attachment_matches(attachment: &Attachment, byte_length: usize, sha256: &str) -> bool {
    attachment.name == "illustration_geometry"
        && attachment.media_type == "application/vnd.wavenumber.geometer.illustration-geometry+json"
        && attachment.data.len() == byte_length
        && format!("{:x}", Sha256::digest(&attachment.data)) == sha256
}

async fn run_illustration_geometry_request_a0<B: crate::backend::OperationBackend>(
    backend: &B,
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
    let response = backend
        .execute_operation(
            "geometry.mesh_illustration_geometry.a0",
            &contracts::encode_json(&request)?,
            attachments,
        )
        .await?;
    crate::backend::contain_protocol_result(backend, decode_response_a0(response)).await
}

fn decode_response_a0(
    response: OperationResponse,
) -> Result<MeshIllustrationGeometryA0, GeometerClientError> {
    let success = match response.outcome.into_a0()? {
        OperationOutcomeA0::Failure(failure) => {
            return Err(GeometerClientError::Operation {
                operation: failure.operation,
                diagnostics: failure.diagnostics,
            });
        }
        OperationOutcomeA0::Success(value) => value,
    };
    let OperationResultValueA0::MeshIllustrationGeometry(metadata) = success.result else {
        return Err(invalid("wrong A0 result type"));
    };
    if response.attachments.len() != 1 {
        return Err(invalid("expected one A0 geometry attachment"));
    }
    let attachment = &response.attachments[0];
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
    Ok(geometry)
}

fn validate_geometry_counts(
    geometry: &MeshIllustrationGeometryB0,
) -> Result<(), GeometerClientError> {
    if geometry
        .bounds
        .as_ref()
        .is_some_and(|bounds| bounds.min.iter().zip(&bounds.max).any(|(a, b)| a > b))
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
