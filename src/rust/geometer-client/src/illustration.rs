//! Generated B0 illustration values over the shared operation backend.

#[cfg(feature = "direct-static")]
use crate::GeometerDirectClient;
use crate::contracts::{
    self, HlrProjectionResultA0, HlrProjectionResultB0, MeshCollectionA0, MeshIllustrationInputA0,
    MeshIllustrationInputB0, MeshIllustrationRequestA0, MeshIllustrationRequestB0,
    MeshIllustrationResultA0, MeshIllustrationResultB0, OperationOutcomeA0, OperationOutcomeB0,
    OperationResultValueA0, OperationResultValueB0, Validate,
};
use crate::ipc::Attachment;
use crate::{GeometerClient, GeometerClientError};

macro_rules! impl_illustration_client {
    ($client:ty) => {
        impl $client {
            /// Render the shared illustration input without JavaScript or WASM.
            /// Large meshes use a governed attachment; the result retains inline A0 SVG.
            /// This does not compute HLR or silently substitute another renderer.
            pub async fn mesh_illustration(
                &self,
                input: MeshIllustrationInputB0,
            ) -> Result<MeshIllustrationResultB0, GeometerClientError> {
                run_illustration_request(self, input, None).await
            }

            /// Render a finished SVG with native HLR detail and outline above its fills.
            /// Supply visible-only polyline HLR, exactly one matching view, from the same
            /// millimeter model/placement/transform as the meshes. `show_hlr_*` style
            /// options select exported lines; the executable handles ordering and mirror_x.
            /// Arcs, mismatched views and more than 1,000,000 segments are rejected.
            pub async fn mesh_illustration_with_hlr(
                &self,
                input: MeshIllustrationInputB0,
                hlr: HlrProjectionResultB0,
            ) -> Result<MeshIllustrationResultB0, GeometerClientError> {
                run_illustration_request(self, input, Some(hlr)).await
            }

            /// Execute the strict A0 compatibility operation without clipping.
            pub async fn mesh_illustration_a0(
                &self,
                input: MeshIllustrationInputA0,
            ) -> Result<MeshIllustrationResultA0, GeometerClientError> {
                run_illustration_request_a0(self, input, None).await
            }

            /// Execute the strict A0 compatibility composition operation.
            pub async fn mesh_illustration_with_hlr_a0(
                &self,
                input: MeshIllustrationInputA0,
                hlr: HlrProjectionResultA0,
            ) -> Result<MeshIllustrationResultA0, GeometerClientError> {
                run_illustration_request_a0(self, input, Some(hlr)).await
            }

            /// Generation-explicit alias for the canonical B0 operation.
            pub async fn mesh_illustration_b0(
                &self,
                input: MeshIllustrationInputB0,
            ) -> Result<MeshIllustrationResultB0, GeometerClientError> {
                run_illustration_request(self, input, None).await
            }

            /// Generation-explicit alias for canonical B0 HLR composition.
            pub async fn mesh_illustration_with_hlr_b0(
                &self,
                input: MeshIllustrationInputB0,
                hlr: HlrProjectionResultB0,
            ) -> Result<MeshIllustrationResultB0, GeometerClientError> {
                run_illustration_request(self, input, Some(hlr)).await
            }
        }
    };
}

async fn run_illustration_request_a0<B: crate::backend::OperationBackend>(
    backend: &B,
    input: MeshIllustrationInputA0,
    hlr: Option<HlrProjectionResultA0>,
) -> Result<MeshIllustrationResultA0, GeometerClientError> {
    input.validate_at("")?;
    let request = MeshIllustrationRequestA0 {
        schema: "geometry.mesh_illustration.request.a0".to_owned(),
        view: input.view,
        prepare: input.prepare,
        style: input.style,
        svg: input.svg,
    };
    let collection = MeshCollectionA0 {
        schema: "geometry.mesh_collection.a0".to_owned(),
        length_unit: "millimeter".to_owned(),
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
            "geometry.mesh_illustration.a0",
            &contracts::encode_json(&request)?,
            attachments,
        )
        .await?;
    let result = match response.outcome.into_a0()? {
        OperationOutcomeA0::Failure(failure) => Err(GeometerClientError::Operation {
            operation: failure.operation,
            diagnostics: failure.diagnostics,
        }),
        OperationOutcomeA0::Success(success) => match success.result {
            OperationResultValueA0::MeshIllustration(result) if response.attachments.is_empty() => {
                Ok(result)
            }
            _ => Err(GeometerClientError::Protocol(
                "A0 mesh illustration returned an incompatible result or attachments".to_owned(),
            )),
        },
    };
    crate::backend::contain_protocol_result(backend, result).await
}

impl_illustration_client!(GeometerClient);
#[cfg(feature = "direct-static")]
impl_illustration_client!(GeometerDirectClient);

async fn run_illustration_request<B: crate::backend::OperationBackend>(
    backend: &B,
    input: MeshIllustrationInputB0,
    hlr: Option<HlrProjectionResultB0>,
) -> Result<MeshIllustrationResultB0, GeometerClientError> {
    input.validate_at("")?;
    let request = MeshIllustrationRequestB0 {
        schema: "geometry.mesh_illustration.request.b0".to_owned(),
        view: input.view,
        prepare: input.prepare,
        style: input.style,
        svg: input.svg,
        clipping: input.clipping,
    };
    let collection = MeshCollectionA0 {
        schema: "geometry.mesh_collection.a0".to_owned(),
        length_unit: "millimeter".to_owned(),
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
            "geometry.mesh_illustration.b0",
            &contracts::encode_json(&request)?,
            attachments,
        )
        .await?;
    let result = match response.outcome.into_b0()? {
        OperationOutcomeB0::Failure(failure) => Err(GeometerClientError::Operation {
            operation: failure.operation,
            diagnostics: failure.diagnostics,
        }),
        OperationOutcomeB0::Success(success) => match success.result {
            OperationResultValueB0::MeshIllustration(result) if response.attachments.is_empty() => {
                Ok(result)
            }
            _ => Err(GeometerClientError::Protocol(
                "mesh illustration returned an incompatible result or attachments".to_owned(),
            )),
        },
    };
    crate::backend::contain_protocol_result(backend, result).await
}
