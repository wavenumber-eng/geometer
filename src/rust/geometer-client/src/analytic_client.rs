#[cfg(feature = "direct-static")]
use crate::GeometerDirectClient;
use crate::client::{GeometerClient, GeometerClientError};
use crate::generated::contracts::{
    self, AnalyticPlanarBooleanBatchRequestA0, AnalyticPlanarBooleanBatchResultA0,
    OperationOutcomeA0, PackedAttachmentProjectionA0, PackedAttachmentReferenceA0,
};
use crate::generated::operations::ANALYTIC_PLANAR_BOOLEAN_BATCH_A0_IDENTITY;
use crate::ipc::Attachment;
use crate::operation_validation::operation_declaration;
use crate::{
    decode_analytic_planar_boolean_batch_result_a0_packet,
    encode_analytic_planar_boolean_batch_request_a0_packet,
};

macro_rules! impl_analytic_client {
    ($client:ty) => {
        impl $client {
            pub async fn analytic_planar_boolean_batch(
                &self,
                request: &AnalyticPlanarBooleanBatchRequestA0,
            ) -> Result<AnalyticPlanarBooleanBatchResultA0, GeometerClientError> {
                run_analytic_planar_boolean_batch(self, request).await
            }
        }
    };
}

impl_analytic_client!(GeometerClient);
#[cfg(feature = "direct-static")]
impl_analytic_client!(GeometerDirectClient);

async fn run_analytic_planar_boolean_batch<B: crate::backend::OperationBackend>(
    backend: &B,
    request: &AnalyticPlanarBooleanBatchRequestA0,
) -> Result<AnalyticPlanarBooleanBatchResultA0, GeometerClientError> {
    let declaration = operation_declaration(
        backend.operation_catalog(),
        ANALYTIC_PLANAR_BOOLEAN_BATCH_A0_IDENTITY,
    )?;
    let request_projection = declaration.request_projection.as_ref().ok_or_else(|| {
        GeometerClientError::Protocol(
            "analytic request projection is absent from the negotiated catalog".to_owned(),
        )
    })?;
    let result_attachment_name = declaration
        .result_projection
        .as_ref()
        .ok_or_else(|| {
            GeometerClientError::Protocol(
                "analytic result projection is absent from the negotiated catalog".to_owned(),
            )
        })?
        .attachment_name
        .clone();
    let input = declaration
        .input_attachments
        .iter()
        .find(|value| value.name == request_projection.attachment_name)
        .ok_or_else(|| {
            GeometerClientError::Protocol(
                "analytic request attachment declaration is absent".to_owned(),
            )
        })?;
    let media_type = input.media_types.first().cloned().ok_or_else(|| {
        GeometerClientError::Protocol(
            "analytic request media type is absent from the catalog".to_owned(),
        )
    })?;
    let projection = PackedAttachmentProjectionA0 {
        schema: declaration.request_contract.clone(),
        packet: PackedAttachmentReferenceA0 {
            attachment: request_projection.attachment_name.clone(),
            format: request_projection.format.clone(),
        },
    };
    let request_json = contracts::encode_json(&projection)?;
    let packet = encode_analytic_planar_boolean_batch_request_a0_packet(request)?;
    let response = backend
        .execute_operation(
            ANALYTIC_PLANAR_BOOLEAN_BATCH_A0_IDENTITY,
            &request_json,
            vec![Attachment {
                name: input.name.clone(),
                media_type,
                data: packet,
            }],
        )
        .await?;
    match response.outcome.into_a0()? {
        OperationOutcomeA0::Failure(failure) => Err(GeometerClientError::Operation {
            operation: failure.operation,
            diagnostics: failure.diagnostics,
        }),
        OperationOutcomeA0::Success(_) => {
            let attachment = response
                .attachments
                .iter()
                .find(|value| value.name == result_attachment_name)
                .ok_or_else(|| {
                    GeometerClientError::Protocol(
                        "analytic result attachment is missing after validation".to_owned(),
                    )
                })?;
            decode_analytic_result(backend, &attachment.data).await
        }
    }
}

async fn decode_analytic_result<B: crate::backend::OperationBackend>(
    backend: &B,
    bytes: &[u8],
) -> Result<AnalyticPlanarBooleanBatchResultA0, GeometerClientError> {
    match decode_analytic_planar_boolean_batch_result_a0_packet(bytes) {
        Ok(result) => Ok(result),
        Err(error) => {
            let message = format!("invalid analytic result packet: {error}");
            Err(backend.poison_protocol(message).await)
        }
    }
}

#[cfg(test)]
impl GeometerClient {
    pub(crate) async fn decode_analytic_result(
        &self,
        bytes: &[u8],
    ) -> Result<AnalyticPlanarBooleanBatchResultA0, GeometerClientError> {
        decode_analytic_result(self, bytes).await
    }
}
