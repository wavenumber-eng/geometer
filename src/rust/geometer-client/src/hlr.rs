//! Typed HLR request facades over the generic executable IPC client.

use crate::client::{GeometerClient, GeometerClientError};
use crate::generated::contracts::{
    self, HlrProjectionOptionsA0, HlrProjectionResultA0, OperationOutcomeA0, OperationResultValueA0,
};
use crate::indexed_mesh_packet_a0::{
    INDEXED_TRIANGLE_MESH_MEDIA_TYPE, IndexedMeshPacketError, IndexedTriangleMeshA0,
    encode_indexed_triangle_mesh_a0_packet,
};
use crate::ipc::Attachment;

#[derive(Clone, Debug)]
pub struct ModelHlrProjectionRequest {
    pub model: Vec<u8>,
    pub media_type: String,
    pub options: HlrProjectionOptionsA0,
}

#[derive(Clone, Debug)]
pub struct MeshHlrProjectionRequest {
    pub mesh_packet: Vec<u8>,
    pub options: HlrProjectionOptionsA0,
}

impl MeshHlrProjectionRequest {
    pub fn from_mesh(
        mesh: &IndexedTriangleMeshA0,
        options: HlrProjectionOptionsA0,
    ) -> Result<Self, IndexedMeshPacketError> {
        Ok(Self {
            mesh_packet: encode_indexed_triangle_mesh_a0_packet(mesh)?,
            options,
        })
    }
}

impl GeometerClient {
    pub async fn model_hlr_projection(
        &self,
        request: ModelHlrProjectionRequest,
    ) -> Result<HlrProjectionResultA0, GeometerClientError> {
        self.hlr_projection(
            "geometry.model_hlr_projection.a0",
            "model",
            request.media_type,
            request.model,
            request.options,
        )
        .await
    }

    pub async fn mesh_hlr_projection(
        &self,
        request: MeshHlrProjectionRequest,
    ) -> Result<HlrProjectionResultA0, GeometerClientError> {
        self.hlr_projection(
            "geometry.mesh_hlr_projection.a0",
            "mesh",
            INDEXED_TRIANGLE_MESH_MEDIA_TYPE.to_owned(),
            request.mesh_packet,
            request.options,
        )
        .await
    }

    async fn hlr_projection(
        &self,
        operation: &str,
        attachment_name: &str,
        media_type: String,
        data: Vec<u8>,
        mut options: HlrProjectionOptionsA0,
    ) -> Result<HlrProjectionResultA0, GeometerClientError> {
        // Preserve the HLR default while disambiguating the presence-only IPC union.
        if options.output_detail.is_none() {
            options.output_detail = Some(true);
        }
        let options = contracts::encode_hlr_projection_options_a0_json(&options)?;
        let response = self
            .execute(
                operation,
                &options,
                vec![Attachment {
                    name: attachment_name.to_owned(),
                    media_type,
                    data,
                }],
            )
            .await?;
        if !response.attachments.is_empty() {
            return Err(GeometerClientError::Protocol(
                "HLR projection returned unexpected attachments".to_owned(),
            ));
        }
        match response.outcome {
            OperationOutcomeA0::Success(success) => hlr_projection_result(success.result),
            OperationOutcomeA0::Failure(failure) => Err(GeometerClientError::Operation {
                operation: failure.operation,
                diagnostics: failure.diagnostics,
            }),
        }
    }
}

fn hlr_projection_result(
    result: OperationResultValueA0,
) -> Result<HlrProjectionResultA0, GeometerClientError> {
    match result {
        OperationResultValueA0::HlrProjection(result) => Ok(result),
        _ => Err(GeometerClientError::Protocol(
            "HLR projection returned an incompatible result variant".to_owned(),
        )),
    }
}
