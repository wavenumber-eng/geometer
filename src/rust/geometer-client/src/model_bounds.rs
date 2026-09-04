//! Typed model-bounds request facade over the generic executable IPC client.

use crate::client::{GeometerClient, GeometerClientError};
use crate::generated::contracts::{
    self, ModelBoundsOptionsA0, ModelBoundsResultA0, OperationOutcomeA0, OperationResultValueA0,
};
use crate::ipc::Attachment;

#[derive(Clone, Debug)]
pub struct ModelBoundsRequest {
    pub model: Vec<u8>,
    pub media_type: String,
    pub options: ModelBoundsOptionsA0,
}

impl ModelBoundsRequest {
    pub fn step(model: Vec<u8>) -> Self {
        Self {
            model,
            media_type: "application/step".to_owned(),
            options: ModelBoundsOptionsA0 {
                format: None,
                model_transform: None,
            },
        }
    }
}

impl GeometerClient {
    pub async fn model_bounds(
        &self,
        request: ModelBoundsRequest,
    ) -> Result<ModelBoundsResultA0, GeometerClientError> {
        let options = contracts::encode_model_bounds_options_a0_json(&request.options)?;
        let response = self
            .execute(
                "geometry.model_bounds.a0",
                &options,
                vec![Attachment {
                    name: "model".to_owned(),
                    media_type: request.media_type,
                    data: request.model,
                }],
            )
            .await?;
        if !response.attachments.is_empty() {
            return Err(GeometerClientError::Protocol(
                "model_bounds returned unexpected attachments".to_owned(),
            ));
        }
        match response.outcome {
            OperationOutcomeA0::Success(success) => model_bounds_result(success.result),
            OperationOutcomeA0::Failure(failure) => Err(GeometerClientError::Operation {
                operation: failure.operation,
                diagnostics: failure.diagnostics,
            }),
        }
    }
}

fn model_bounds_result(
    result: OperationResultValueA0,
) -> Result<ModelBoundsResultA0, GeometerClientError> {
    match result {
        OperationResultValueA0::ModelBounds(result) => Ok(result),
        _ => Err(GeometerClientError::Protocol(
            "model_bounds returned an incompatible result variant".to_owned(),
        )),
    }
}

#[cfg(test)]
mod tests {
    use super::*;

    #[test]
    fn rejects_runtime_unavailable_topology_result_variant() {
        let result =
            OperationResultValueA0::StepTopologyClose(contracts::StepTopologyCloseResultA0 {
                schema: "geometry.step_topology.close.result.a0".to_owned(),
                session_handle: format!("gts_{}", "1".repeat(64)),
                closed: true,
            });
        assert!(matches!(
            model_bounds_result(result),
            Err(GeometerClientError::Protocol(message))
                if message == "model_bounds returned an incompatible result variant"
        ));
    }
}
