// Generated from wn_geometer_contract_catalog.a0.json. Do not edit.

use super::contracts;

/// Decode using the negotiated operation's exact contract, not union trial order.
pub fn decode_logical_request(
    contract: &str,
    data: &[u8],
) -> Result<contracts::IpcRequestValueA0, contracts::ContractError> {
    match contract {
        "geometry.model_tessellation.request.a0" => {
            Ok(contracts::IpcRequestValueA0::ModelTessellation(
                contracts::decode_json::<contracts::ModelTessellationRequestA0>(data)?,
            ))
        }
        "geometry.model_bounds.options.a0" => Ok(contracts::IpcRequestValueA0::LogicalDto(
            contracts::decode_json::<contracts::ModelBoundsOptionsA0>(data)?,
        )),
        "geometry.hlr_projection.options.a0" => Ok(contracts::IpcRequestValueA0::HlrProjection(
            contracts::decode_json::<contracts::HlrProjectionOptionsA0>(data)?,
        )),
        "geometry.step_topology.open.request.a0" => {
            Ok(contracts::IpcRequestValueA0::StepTopologyOpen(
                contracts::decode_json::<contracts::StepTopologyOpenRequestA0>(data)?,
            ))
        }
        "geometry.step_topology.close.request.a0" => {
            Ok(contracts::IpcRequestValueA0::StepTopologyClose(
                contracts::decode_json::<contracts::StepTopologyCloseRequestA0>(data)?,
            ))
        }
        "geometry.step_topology.inspect.request.a0" => {
            Ok(contracts::IpcRequestValueA0::StepTopologyInspect(
                contracts::decode_json::<contracts::StepTopologyInspectRequestA0>(data)?,
            ))
        }
        "geometry.step_topology.render.request.a0" => {
            Ok(contracts::IpcRequestValueA0::StepTopologyRender(
                contracts::decode_json::<contracts::StepTopologyRenderRequestA0>(data)?,
            ))
        }
        "geometry.step_topology.resolve_hit.request.a0" => {
            Ok(contracts::IpcRequestValueA0::StepTopologyResolveHit(
                contracts::decode_json::<contracts::StepTopologyResolveHitRequestA0>(data)?,
            ))
        }
        "geometry.step_topology.apply_logical_groups.request.a0" => Ok(
            contracts::IpcRequestValueA0::StepTopologyApplyLogicalGroups(contracts::decode_json::<
                contracts::StepTopologyApplyLogicalGroupsRequestA0,
            >(data)?),
        ),
        "geometry.step_topology.apply_metadata_probes.request.a0" => Ok(
            contracts::IpcRequestValueA0::StepTopologyApplyMetadataProbes(
                contracts::decode_json::<contracts::StepTopologyApplyMetadataProbesRequestA0>(
                    data,
                )?,
            ),
        ),
        "geometry.step_topology.checkpoint_edit_journal.request.a0" => Ok(
            contracts::IpcRequestValueA0::StepTopologyCheckpointEditJournal(
                contracts::decode_json::<contracts::StepTopologyCheckpointEditJournalRequestA0>(
                    data,
                )?,
            ),
        ),
        "geometry.step_topology.apply_hierarchy.request.a0" => {
            Ok(contracts::IpcRequestValueA0::StepTopologyApplyHierarchy(
                contracts::decode_json::<contracts::StepTopologyApplyHierarchyRequestA0>(data)?,
            ))
        }
        "geometry.step_topology.save.request.a0" => {
            Ok(contracts::IpcRequestValueA0::StepTopologySave(
                contracts::decode_json::<contracts::StepTopologySaveRequestA0>(data)?,
            ))
        }
        "geometry.step_topology.restore.request.a0" => {
            Ok(contracts::IpcRequestValueA0::StepTopologyRestore(
                contracts::decode_json::<contracts::StepTopologyRestoreRequestA0>(data)?,
            ))
        }
        "geometry.step_topology.analyze_recovery.request.a0" => {
            Ok(contracts::IpcRequestValueA0::StepTopologyAnalyzeRecovery(
                contracts::decode_json::<contracts::StepTopologyAnalyzeRecoveryRequestA0>(data)?,
            ))
        }
        _ => Err(contracts::ContractError::Validation {
            path: "/request".to_owned(),
            message: format!("no generated logical request codec for {contract}"),
        }),
    }
}

pub fn logical_request_contract(value: &contracts::IpcRequestValueA0) -> Option<&'static str> {
    match value {
        contracts::IpcRequestValueA0::ModelTessellation(_) => {
            Some("geometry.model_tessellation.request.a0")
        }
        contracts::IpcRequestValueA0::LogicalDto(_) => Some("geometry.model_bounds.options.a0"),
        contracts::IpcRequestValueA0::HlrProjection(_) => {
            Some("geometry.hlr_projection.options.a0")
        }
        contracts::IpcRequestValueA0::PackedAttachment(_) => None,
        contracts::IpcRequestValueA0::StepTopologyOpen(_) => {
            Some("geometry.step_topology.open.request.a0")
        }
        contracts::IpcRequestValueA0::StepTopologyClose(_) => {
            Some("geometry.step_topology.close.request.a0")
        }
        contracts::IpcRequestValueA0::StepTopologyInspect(_) => {
            Some("geometry.step_topology.inspect.request.a0")
        }
        contracts::IpcRequestValueA0::StepTopologyRender(_) => {
            Some("geometry.step_topology.render.request.a0")
        }
        contracts::IpcRequestValueA0::StepTopologyResolveHit(_) => {
            Some("geometry.step_topology.resolve_hit.request.a0")
        }
        contracts::IpcRequestValueA0::StepTopologyApplyLogicalGroups(_) => {
            Some("geometry.step_topology.apply_logical_groups.request.a0")
        }
        contracts::IpcRequestValueA0::StepTopologyApplyMetadataProbes(_) => {
            Some("geometry.step_topology.apply_metadata_probes.request.a0")
        }
        contracts::IpcRequestValueA0::StepTopologyCheckpointEditJournal(_) => {
            Some("geometry.step_topology.checkpoint_edit_journal.request.a0")
        }
        contracts::IpcRequestValueA0::StepTopologyApplyHierarchy(_) => {
            Some("geometry.step_topology.apply_hierarchy.request.a0")
        }
        contracts::IpcRequestValueA0::StepTopologySave(_) => {
            Some("geometry.step_topology.save.request.a0")
        }
        contracts::IpcRequestValueA0::StepTopologyRestore(_) => {
            Some("geometry.step_topology.restore.request.a0")
        }
        contracts::IpcRequestValueA0::StepTopologyAnalyzeRecovery(_) => {
            Some("geometry.step_topology.analyze_recovery.request.a0")
        }
    }
}

pub fn logical_result_contract(value: &contracts::OperationResultValueA0) -> Option<&'static str> {
    match value {
        contracts::OperationResultValueA0::ModelTessellation(_) => {
            Some("geometry.model_tessellation.result.a0")
        }
        contracts::OperationResultValueA0::ModelBounds(_) => Some("geometry.model_bounds.a0"),
        contracts::OperationResultValueA0::HlrProjection(_) => {
            Some("geometry.hlr_projection.result.a0")
        }
        contracts::OperationResultValueA0::PackedAttachment(_) => None,
        contracts::OperationResultValueA0::StepTopologyOpen(_) => {
            Some("geometry.step_topology.open.result.a0")
        }
        contracts::OperationResultValueA0::StepTopologyClose(_) => {
            Some("geometry.step_topology.close.result.a0")
        }
        contracts::OperationResultValueA0::StepTopologyInspect(_) => {
            Some("geometry.step_topology.inspect.result.a0")
        }
        contracts::OperationResultValueA0::StepTopologyRender(_) => {
            Some("geometry.step_topology.render.result.a0")
        }
        contracts::OperationResultValueA0::StepTopologyResolveHit(_) => {
            Some("geometry.step_topology.resolve_hit.result.a0")
        }
        contracts::OperationResultValueA0::StepTopologyApplyLogicalGroups(_) => {
            Some("geometry.step_topology.apply_logical_groups.result.a0")
        }
        contracts::OperationResultValueA0::StepTopologyApplyMetadataProbes(_) => {
            Some("geometry.step_topology.apply_metadata_probes.result.a0")
        }
        contracts::OperationResultValueA0::StepTopologyCheckpointEditJournal(_) => {
            Some("geometry.step_topology.checkpoint_edit_journal.result.a0")
        }
        contracts::OperationResultValueA0::StepTopologyApplyHierarchy(_) => {
            Some("geometry.step_topology.apply_hierarchy.result.a0")
        }
        contracts::OperationResultValueA0::StepTopologySave(_) => {
            Some("geometry.step_topology.save.result.a0")
        }
        contracts::OperationResultValueA0::StepTopologyRestore(_) => {
            Some("geometry.step_topology.restore.result.a0")
        }
        contracts::OperationResultValueA0::StepTopologyAnalyzeRecovery(_) => {
            Some("geometry.step_topology.analyze_recovery.result.a0")
        }
    }
}
