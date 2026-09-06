//! Generated A0 illustration values over the persistent executable transport.

use crate::contracts::{
    self, HlrProjectionResultA0, MeshCollectionA0, MeshIllustrationInputA0,
    MeshIllustrationRequestA0, MeshIllustrationResultA0, OperationOutcomeA0,
    OperationResultValueA0, Validate,
};
use crate::ipc::Attachment;
use crate::{GeometerClient, GeometerClientError};

impl GeometerClient {
    /// Render the shared illustration input without JavaScript or WASM.
    /// Large meshes use a governed attachment; the result retains inline A0 SVG.
    /// This does not compute HLR or silently substitute another renderer.
    pub async fn mesh_illustration(
        &self,
        input: MeshIllustrationInputA0,
    ) -> Result<MeshIllustrationResultA0, GeometerClientError> {
        self.illustration_request(input, None).await
    }

    /// Render a finished SVG with native HLR detail and outline above its fills.
    /// Supply visible-only polyline HLR, exactly one matching view, from the same
    /// millimeter model/placement/transform as the meshes. `show_hlr_*` style
    /// options select exported lines; the executable handles ordering and mirror_x.
    /// Arcs, mismatched views and more than 1,000,000 segments are rejected.
    pub async fn mesh_illustration_with_hlr(
        &self,
        input: MeshIllustrationInputA0,
        hlr: HlrProjectionResultA0,
    ) -> Result<MeshIllustrationResultA0, GeometerClientError> {
        self.illustration_request(input, Some(hlr)).await
    }

    async fn illustration_request(
        &self,
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
        let response = self
            .execute(
                "geometry.mesh_illustration.a0",
                &contracts::encode_json(&request)?,
                attachments,
            )
            .await?;
        let result = match response.outcome {
            OperationOutcomeA0::Failure(failure) => Err(GeometerClientError::Operation {
                operation: failure.operation,
                diagnostics: failure.diagnostics,
            }),
            OperationOutcomeA0::Success(success) => match success.result {
                OperationResultValueA0::MeshIllustration(result)
                    if response.attachments.is_empty() =>
                {
                    Ok(result)
                }
                _ => Err(GeometerClientError::Protocol(
                    "mesh illustration returned an incompatible result or attachments".to_owned(),
                )),
            },
        };
        if matches!(result, Err(GeometerClientError::Protocol(_))) {
            self.terminate().await?;
        }
        result
    }
}
