use std::collections::HashSet;

use crate::client::{GeometerClientError, OperationOutcome};
use crate::generated::contracts::{
    self, IpcOperationCatalogA0, IpcOperationDeclarationA0, IpcRequestValueA0, IpcRequestValueB0,
    IpcRuntimeDispatchA0, IpcWelcomeA0, OperationOutcomeA0, OperationOutcomeB0,
    OperationResultValueA0, PackedAttachmentProjectionA0,
};
use crate::generated::dispatch::{
    logical_request_contract, logical_request_contract_b0, logical_result_contract,
    logical_result_contract_b0,
};
use crate::ipc::Attachment;

#[cfg(test)]
mod tests;

pub(crate) fn operation_declaration<'a>(
    catalog: &'a IpcOperationCatalogA0,
    operation: &str,
) -> Result<&'a IpcOperationDeclarationA0, GeometerClientError> {
    catalog
        .operations
        .iter()
        .find(|value| value.identity == operation)
        .ok_or_else(|| {
            GeometerClientError::Protocol(format!(
                "operation {operation} is absent from the negotiated catalog"
            ))
        })
}

pub(crate) fn request_uses_b0(declaration: &IpcOperationDeclarationA0) -> bool {
    matches!(
        declaration.request_contract.as_str(),
        "geometry.model_illustration.request.b0"
            | "geometry.model_illustration_geometry.request.b0"
            | "geometry.mesh_illustration.request.b0"
            | "geometry.mesh_illustration_geometry.request.b0"
            | "geometry.mesh_hlr_projection.request.b0"
    )
}

pub(crate) fn result_uses_b0(declaration: &IpcOperationDeclarationA0) -> bool {
    matches!(
        declaration.result_contract.as_str(),
        "geometry.model_illustration.result.b0"
            | "geometry.model_illustration_geometry.result.b0"
            | "geometry.mesh_illustration.result.b0"
            | "geometry.mesh_illustration_geometry.result.b0"
            | "geometry.hlr_projection.result.b0"
    )
}

pub(crate) fn validate_operation_request(
    declaration: &IpcOperationDeclarationA0,
    request: &IpcRequestValueA0,
    attachments: &[Attachment],
) -> Result<(), GeometerClientError> {
    validate_declared_attachments(
        &declaration.identity,
        &declaration.input_attachments,
        attachments,
        "request",
    )?;
    match (&declaration.runtime_dispatch, request) {
        (IpcRuntimeDispatchA0::LogicalDto, value)
            if logical_request_contract(value) == Some(declaration.request_contract.as_str()) =>
        {
            Ok(())
        }
        (IpcRuntimeDispatchA0::PackedAttachment, IpcRequestValueA0::PackedAttachment(value)) => {
            validate_packed_projection(declaration, value, true)
        }
        _ => Err(GeometerClientError::Protocol(
            "request projection does not match the operation runtime dispatch".to_owned(),
        )),
    }
}

pub(crate) fn decode_and_validate_request(
    declaration: &IpcOperationDeclarationA0,
    request_json: &[u8],
    attachments: &[Attachment],
) -> Result<OperationRequest, GeometerClientError> {
    if request_uses_b0(declaration) {
        let request = crate::generated::dispatch::decode_logical_request_b0(
            &declaration.request_contract,
            request_json,
        )?;
        validate_declared_attachments(
            &declaration.identity,
            &declaration.input_attachments,
            attachments,
            "request",
        )?;
        if declaration.runtime_dispatch != IpcRuntimeDispatchA0::LogicalDto
            || logical_request_contract_b0(&request) != Some(declaration.request_contract.as_str())
        {
            return Err(GeometerClientError::Protocol(
                "B0 request projection does not match the operation runtime dispatch".to_owned(),
            ));
        }
        return Ok(OperationRequest::B0(request));
    }
    let request = match declaration.runtime_dispatch {
        IpcRuntimeDispatchA0::LogicalDto => crate::generated::dispatch::decode_logical_request(
            &declaration.request_contract,
            request_json,
        )?,
        IpcRuntimeDispatchA0::PackedAttachment => {
            IpcRequestValueA0::PackedAttachment(contracts::decode_json::<
                PackedAttachmentProjectionA0,
            >(request_json)?)
        }
    };
    validate_operation_request(declaration, &request, attachments)?;
    Ok(OperationRequest::A0(request))
}

pub(crate) enum OperationRequest {
    A0(IpcRequestValueA0),
    B0(IpcRequestValueB0),
}

#[cfg(feature = "direct-static")]
pub(crate) fn encode_direct_request(
    request: &OperationRequest,
) -> Result<Vec<u8>, GeometerClientError> {
    let encoded = match request {
        OperationRequest::A0(value) => serde_json::to_vec(value),
        OperationRequest::B0(value) => serde_json::to_vec(value),
    };
    encoded.map_err(|error| {
        GeometerClientError::Protocol(format!(
            "could not encode direct operation request: {error}"
        ))
    })
}

pub(crate) fn validate_operation_response(
    welcome: &IpcWelcomeA0,
    operation: &str,
    outcome: &OperationOutcome,
    attachments: &[Attachment],
) -> Result<(), GeometerClientError> {
    let declaration = operation_declaration(&welcome.operation_catalog, operation)?;
    validate_operation_response_declaration(declaration, outcome, attachments)
}

pub(crate) fn validate_operation_response_declaration(
    declaration: &IpcOperationDeclarationA0,
    outcome: &OperationOutcome,
    attachments: &[Attachment],
) -> Result<(), GeometerClientError> {
    let operation = declaration.identity.as_str();
    match outcome {
        OperationOutcome::A0(OperationOutcomeA0::Failure(_))
        | OperationOutcome::B0(OperationOutcomeB0::Failure(_)) => {
            validate_failure_attachments(attachments)
        }
        OperationOutcome::A0(OperationOutcomeA0::Success(success)) => {
            validate_declared_attachments(
                operation,
                &declaration.output_attachments,
                attachments,
                "response",
            )?;
            match (&declaration.runtime_dispatch, &success.result) {
                (IpcRuntimeDispatchA0::LogicalDto, value)
                    if logical_result_contract(value)
                        == Some(declaration.result_contract.as_str()) =>
                {
                    Ok(())
                }
                (
                    IpcRuntimeDispatchA0::PackedAttachment,
                    OperationResultValueA0::PackedAttachment(value),
                ) => validate_packed_projection(declaration, value, false),
                _ => Err(GeometerClientError::Protocol(
                    "result projection does not match the operation runtime dispatch".to_owned(),
                )),
            }
        }
        OperationOutcome::B0(OperationOutcomeB0::Success(success)) => {
            validate_declared_attachments(
                operation,
                &declaration.output_attachments,
                attachments,
                "response",
            )?;
            if declaration.runtime_dispatch == IpcRuntimeDispatchA0::LogicalDto
                && logical_result_contract_b0(&success.result)
                    == Some(declaration.result_contract.as_str())
            {
                Ok(())
            } else {
                Err(GeometerClientError::Protocol(
                    "B0 result projection does not match the operation runtime dispatch".to_owned(),
                ))
            }
        }
    }
}

fn validate_failure_attachments(attachments: &[Attachment]) -> Result<(), GeometerClientError> {
    if attachments.is_empty() {
        Ok(())
    } else {
        Err(GeometerClientError::Protocol(
            "failed operation returned attachments".to_owned(),
        ))
    }
}

fn validate_packed_projection(
    declaration: &IpcOperationDeclarationA0,
    value: &PackedAttachmentProjectionA0,
    request: bool,
) -> Result<(), GeometerClientError> {
    let (contract, expected) = if request {
        (
            declaration.request_contract.as_str(),
            declaration.request_projection.as_ref(),
        )
    } else {
        (
            declaration.result_contract.as_str(),
            declaration.result_projection.as_ref(),
        )
    };
    let expected = expected.ok_or_else(|| {
        GeometerClientError::Protocol("packed projection declaration is missing".to_owned())
    })?;
    let matches_catalog = value.schema == contract
        && value.packet.attachment == expected.attachment_name
        && value.packet.format == expected.format;
    if !matches_catalog {
        return Err(GeometerClientError::Protocol(
            "packed projection metadata differs from the negotiated catalog".to_owned(),
        ));
    }
    Ok(())
}

fn validate_declared_attachments(
    operation: &str,
    declarations: &[contracts::IpcAttachmentDeclarationA0],
    attachments: &[Attachment],
    direction: &str,
) -> Result<(), GeometerClientError> {
    let mut seen = HashSet::with_capacity(attachments.len());
    for attachment in attachments {
        if !seen.insert(attachment.name.as_str()) {
            return Err(GeometerClientError::Protocol(format!(
                "{operation} {direction} duplicates attachment {}",
                attachment.name
            )));
        }
        let declaration = declarations
            .iter()
            .find(|value| value.name == attachment.name)
            .ok_or_else(|| {
                GeometerClientError::Protocol(format!(
                    "{operation} {direction} contains undeclared attachment {}",
                    attachment.name
                ))
            })?;
        let compatible = declaration.media_types.contains(&attachment.media_type)
            && attachment.data.len() <= declaration.max_bytes as usize;
        if !compatible {
            return Err(GeometerClientError::Protocol(format!(
                "{operation} {direction} attachment {} has incompatible media or size",
                attachment.name
            )));
        }
    }
    if declarations
        .iter()
        .any(|value| value.required && !seen.contains(value.name.as_str()))
    {
        return Err(GeometerClientError::Protocol(format!(
            "{operation} {direction} is missing a required attachment"
        )));
    }
    Ok(())
}
