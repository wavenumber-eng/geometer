use std::collections::HashSet;

use crate::client::GeometerClientError;
use crate::generated::contracts::{
    self, IpcOperationDeclarationA0, IpcRequestValueA0, IpcRuntimeDispatchA0, IpcWelcomeA0,
    OperationOutcomeA0, OperationResultValueA0, PackedAttachmentProjectionA0,
};
use crate::ipc::Attachment;

pub(crate) fn operation_declaration<'a>(
    welcome: &'a IpcWelcomeA0,
    operation: &str,
) -> Result<&'a IpcOperationDeclarationA0, GeometerClientError> {
    welcome
        .operation_catalog
        .operations
        .iter()
        .find(|value| value.identity == operation)
        .ok_or_else(|| {
            GeometerClientError::Protocol(format!(
                "operation {operation} is absent from the negotiated catalog"
            ))
        })
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
        (IpcRuntimeDispatchA0::LogicalDto, IpcRequestValueA0::LogicalDto(_))
            if declaration.request_contract == "geometry.model_bounds.options.a0" =>
        {
            Ok(())
        }
        (IpcRuntimeDispatchA0::LogicalDto, IpcRequestValueA0::HlrProjection(_))
            if declaration.request_contract == "geometry.hlr_projection.options.a0" =>
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

pub(crate) fn validate_operation_response(
    welcome: &IpcWelcomeA0,
    operation: &str,
    outcome: &OperationOutcomeA0,
    attachments: &[Attachment],
) -> Result<(), GeometerClientError> {
    let declaration = operation_declaration(welcome, operation)?;
    match outcome {
        OperationOutcomeA0::Failure(_) => validate_failure_attachments(attachments),
        OperationOutcomeA0::Success(success) => {
            validate_declared_attachments(
                operation,
                &declaration.output_attachments,
                attachments,
                "response",
            )?;
            match (&declaration.runtime_dispatch, &success.result) {
                (IpcRuntimeDispatchA0::LogicalDto, OperationResultValueA0::ModelBounds(_)) => {
                    if declaration.result_contract == "geometry.model_bounds.a0" {
                        Ok(())
                    } else {
                        Err(GeometerClientError::Protocol(
                            "result contract differs from the logical result variant".to_owned(),
                        ))
                    }
                }
                (IpcRuntimeDispatchA0::LogicalDto, OperationResultValueA0::HlrProjection(_)) => {
                    if declaration.result_contract == "geometry.hlr_projection.result.a0" {
                        Ok(())
                    } else {
                        Err(GeometerClientError::Protocol(
                            "result contract differs from the logical result variant".to_owned(),
                        ))
                    }
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
