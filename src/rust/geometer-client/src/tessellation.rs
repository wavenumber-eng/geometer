//! Typed STEP-to-colored-mesh attachment boundary.

use sha2::{Digest, Sha256};
use std::collections::HashSet;

use crate::contracts::{
    self, MeshCollectionA0, ModelTessellationRequestA0, ModelTessellationResultA0,
    OperationOutcomeA0, OperationResultValueA0,
};
use crate::ipc::Attachment;
use crate::{GeometerClient, GeometerClientError, OperationResponse};

#[cfg(test)]
mod tests;

#[derive(Clone, Debug)]
pub struct ModelTessellationRequest {
    pub model: Vec<u8>,
    pub options: ModelTessellationRequestA0,
}

impl ModelTessellationRequest {
    pub fn step(model: Vec<u8>) -> Self {
        Self {
            model,
            options: ModelTessellationRequestA0 {
                schema: "geometry.model_tessellation.request.a0".to_owned(),
                linear_deflection_mm: None,
                angular_deflection_rad: None,
                root_placement: None,
                max_triangles: None,
            },
        }
    }
}

#[derive(Clone, Debug)]
pub struct ModelTessellation {
    pub metadata: ModelTessellationResultA0,
    pub mesh_collection: MeshCollectionA0,
}

impl GeometerClient {
    /// Tessellate STEP bytes without JavaScript, WASM, files or private adapters.
    pub async fn model_tessellation(
        &self,
        request: ModelTessellationRequest,
    ) -> Result<ModelTessellation, GeometerClientError> {
        let options = contracts::encode_json(&request.options)?;
        let max_triangles = request.options.max_triangles.unwrap_or(750000);
        let source_hash = format!("{:x}", Sha256::digest(&request.model));
        let response = self
            .execute(
                "geometry.model_tessellation.a0",
                &options,
                vec![Attachment {
                    name: "model".to_owned(),
                    media_type: "application/step".to_owned(),
                    data: request.model,
                }],
            )
            .await?;
        let result = decode_response(response, &source_hash, max_triangles);
        if matches!(result, Err(GeometerClientError::Protocol(_))) {
            self.terminate().await?;
        }
        result
    }
}

fn decode_response(
    response: OperationResponse,
    source_hash: &str,
    max_triangles: u32,
) -> Result<ModelTessellation, GeometerClientError> {
    let success = match response.outcome {
        OperationOutcomeA0::Failure(failure) => {
            return Err(GeometerClientError::Operation {
                operation: failure.operation,
                diagnostics: failure.diagnostics,
            });
        }
        OperationOutcomeA0::Success(value) => value,
    };
    let OperationResultValueA0::ModelTessellation(metadata) = success.result else {
        return Err(invalid("wrong result type"));
    };
    if response.attachments.len() != 1 {
        return Err(invalid("expected one mesh collection attachment"));
    }
    let attachment = &response.attachments[0];
    if attachment.name != "mesh_collection"
        || attachment.media_type != "application/vnd.wavenumber.geometer.mesh-collection+json"
        || attachment.data.len() != metadata.mesh_collection.byte_length as usize
        || format!("{:x}", Sha256::digest(&attachment.data)) != metadata.mesh_collection.sha256
        || source_hash != metadata.source_sha256
    {
        return Err(invalid("attachment/source metadata mismatch"));
    }
    let mesh_collection = contracts::decode_mesh_collection_a0_json(&attachment.data)
        .map_err(|error| invalid(&format!("invalid mesh collection: {error}")))?;
    if mesh_collection.meshes.len() != metadata.meshes as usize {
        return Err(invalid("mesh count mismatch"));
    }
    if metadata.triangles > max_triangles {
        return Err(invalid("result exceeds requested triangle limit"));
    }
    validate_meshes(&mesh_collection, metadata.triangles)?;
    Ok(ModelTessellation {
        metadata,
        mesh_collection,
    })
}

fn validate_meshes(
    mesh_collection: &MeshCollectionA0,
    expected_triangles: u32,
) -> Result<(), GeometerClientError> {
    let mut triangles = 0_usize;
    let mut vertices = 0_usize;
    let mut identities = HashSet::new();
    for mesh in &mesh_collection.meshes {
        if !identities.insert(&mesh.id) {
            return Err(invalid("duplicate mesh identity"));
        }
        triangles += validate_mesh(mesh)?;
        vertices += mesh.positions.len() / 3;
    }
    if triangles != expected_triangles as usize || vertices > 2000000 {
        return Err(invalid(
            "triangle count or collection vertex limit mismatch",
        ));
    }
    Ok(())
}

fn validate_mesh(mesh: &contracts::MeshIllustrationMesh) -> Result<usize, GeometerClientError> {
    let Some(indices) = &mesh.indices else {
        return Err(invalid("tessellation mesh is not indexed"));
    };
    if mesh.positions.len() % 3 != 0
        || indices.len() % 3 != 0
        || indices
            .iter()
            .any(|index| *index as usize >= mesh.positions.len() / 3)
        || mesh
            .normals
            .as_ref()
            .is_some_and(|normals| normals.len() != mesh.positions.len())
    {
        return Err(invalid("invalid tessellation vertex/index layout"));
    }
    let triangles = indices.len() / 3;
    if mesh
        .triangle_material_indices
        .as_ref()
        .is_some_and(|materials| {
            materials.len() != triangles
                || materials
                    .iter()
                    .any(|index| *index as usize >= mesh.materials.len())
        })
    {
        return Err(invalid("invalid triangle material layout"));
    }
    Ok(triangles)
}

fn invalid(message: &str) -> GeometerClientError {
    GeometerClientError::Protocol(format!("model_tessellation: {message}"))
}
