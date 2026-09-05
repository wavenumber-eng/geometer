//! Generated A0 illustration values over the persistent executable transport.

use crate::contracts::{
    self, MeshCollectionA0, MeshIllustrationInputA0, MeshIllustrationRequestA0,
    MeshIllustrationResultA0, OperationOutcomeA0, OperationResultValueA0, Validate,
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
        let response = self
            .execute(
                "geometry.mesh_illustration.a0",
                &contracts::encode_json(&request)?,
                vec![Attachment {
                    name: "mesh_collection".to_owned(),
                    media_type: "application/vnd.wavenumber.geometer.mesh-collection+json"
                        .to_owned(),
                    data: contracts::encode_json(&collection)?,
                }],
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
