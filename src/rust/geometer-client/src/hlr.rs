//! Typed HLR request facades over the shared operation backend.

#[cfg(feature = "direct-static")]
use crate::GeometerDirectClient;
use crate::backend::OperationBackend;
use crate::client::{GeometerClient, GeometerClientError};
use crate::generated::contracts::{
    self, HlrProjectionOptionsA0, HlrProjectionResultA0, HlrProjectionResultB0, MeshCollectionA0,
    MeshHlrProjectionRequestB0, OperationOutcomeA0, OperationOutcomeB0, OperationResultValueA0,
    OperationResultValueB0,
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
pub struct MeshHlrProjectionRequestA0 {
    pub mesh_packet: Vec<u8>,
    pub options: HlrProjectionOptionsA0,
}

impl MeshHlrProjectionRequestA0 {
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

macro_rules! impl_hlr_client {
    ($client:ty) => {
        impl $client {
            pub async fn model_hlr_projection(
                &self,
                request: ModelHlrProjectionRequest,
            ) -> Result<HlrProjectionResultA0, GeometerClientError> {
                run_hlr_projection(
                    self,
                    "geometry.model_hlr_projection.a0",
                    "model",
                    request.media_type,
                    request.model,
                    request.options,
                )
                .await
            }

            pub async fn mesh_hlr_projection_a0(
                &self,
                request: MeshHlrProjectionRequestA0,
            ) -> Result<HlrProjectionResultA0, GeometerClientError> {
                run_hlr_projection(
                    self,
                    "geometry.mesh_hlr_projection.a0",
                    "mesh",
                    INDEXED_TRIANGLE_MESH_MEDIA_TYPE.to_owned(),
                    request.mesh_packet,
                    request.options,
                )
                .await
            }

            pub async fn mesh_hlr_projection(
                &self,
                meshes: MeshCollectionA0,
                mut request: MeshHlrProjectionRequestB0,
            ) -> Result<HlrProjectionResultB0, GeometerClientError> {
                // Preserve the HLR default while disambiguating the presence-only IPC union.
                if request.output_detail.is_none() {
                    request.output_detail = Some(true);
                }
                let request = contracts::encode_mesh_hlr_projection_request_b0_json(&request)?;
                let meshes = contracts::encode_mesh_collection_a0_json(&meshes)?;
                let response = self
                    .execute_operation(
                        "geometry.mesh_hlr_projection.b0",
                        &request,
                        vec![Attachment {
                            name: "mesh_collection".to_owned(),
                            media_type: "application/vnd.wavenumber.geometer.mesh-collection+json"
                                .to_owned(),
                            data: meshes,
                        }],
                    )
                    .await?;
                if !response.attachments.is_empty() {
                    return Err(GeometerClientError::Protocol(
                        "B0 HLR projection returned unexpected attachments".to_owned(),
                    ));
                }
                match response.outcome.into_b0()? {
                    OperationOutcomeB0::Success(success) => match success.result {
                        OperationResultValueB0::MeshHlrProjection(result) => Ok(result),
                        _ => Err(GeometerClientError::Protocol(
                            "B0 HLR projection returned an incompatible result variant".to_owned(),
                        )),
                    },
                    OperationOutcomeB0::Failure(failure) => Err(GeometerClientError::Operation {
                        operation: failure.operation,
                        diagnostics: failure.diagnostics,
                    }),
                }
            }

            pub async fn mesh_hlr_projection_b0(
                &self,
                meshes: MeshCollectionA0,
                request: MeshHlrProjectionRequestB0,
            ) -> Result<HlrProjectionResultB0, GeometerClientError> {
                self.mesh_hlr_projection(meshes, request).await
            }
        }
    };
}

impl_hlr_client!(GeometerClient);
#[cfg(feature = "direct-static")]
impl_hlr_client!(GeometerDirectClient);

async fn run_hlr_projection<B: crate::backend::OperationBackend>(
    backend: &B,
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
    let response = backend
        .execute_operation(
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
    match response.outcome.into_a0()? {
        OperationOutcomeA0::Success(success) => hlr_projection_result(success.result),
        OperationOutcomeA0::Failure(failure) => Err(GeometerClientError::Operation {
            operation: failure.operation,
            diagnostics: failure.diagnostics,
        }),
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
