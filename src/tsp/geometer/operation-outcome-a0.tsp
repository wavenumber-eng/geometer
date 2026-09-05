import "@typespec/json-schema";
import "@wavenumber/wn-geometer-contract-emitter";
import "./common.tsp";
import "./operations/model-bounds-a0.tsp";
import "./operations/hlr-projection-a0.tsp";
import "./operations/step-topology-a0.tsp";
import "./operations/model-tessellation-a0.tsp";
import "./operations/mesh-illustration-a0.tsp";

using TypeSpec.JsonSchema;
using Wavenumber.Geometer.ContractMetadata;

namespace Wavenumber.Geometer.Contracts.OperationOutcomeA0;

/**
 * Structurally representable operation results. A result variant may belong
 * to a runtime-unavailable experimental operation and is not an availability
 * claim; the negotiated operation catalog remains authoritative.
 */
@oneOf
union OperationResultValueA0 {
  mesh_illustration: Wavenumber.Geometer.Contracts.MeshIllustrationA0.MeshIllustrationResultA0,
  model_tessellation: Wavenumber.Geometer.Contracts.ModelTessellationA0.ModelTessellationResultA0,
  model_bounds: Wavenumber.Geometer.Contracts.ModelBoundsA0.ModelBoundsResultA0,
  hlr_projection: Wavenumber.Geometer.Contracts.HlrProjectionA0.HlrProjectionResultA0,
  packed_attachment: Wavenumber.Geometer.Contracts.Common.PackedAttachmentProjectionA0,
  step_topology_open: Wavenumber.Geometer.Contracts.StepTopologyA0.StepTopologyOpenResultA0,
  step_topology_close: Wavenumber.Geometer.Contracts.StepTopologyA0.StepTopologyCloseResultA0,
  step_topology_inspect: Wavenumber.Geometer.Contracts.StepTopologyA0.StepTopologyInspectResultA0,
  step_topology_render: Wavenumber.Geometer.Contracts.StepTopologyA0.StepTopologyRenderResultA0,
  step_topology_resolve_hit: Wavenumber.Geometer.Contracts.StepTopologyA0.StepTopologyResolveHitResultA0,
  step_topology_apply_logical_groups: Wavenumber.Geometer.Contracts.StepTopologyA0.StepTopologyApplyLogicalGroupsResultA0,
  step_topology_apply_metadata_probes: Wavenumber.Geometer.Contracts.StepTopologyA0.StepTopologyApplyMetadataProbesResultA0,
  step_topology_checkpoint_edit_journal: Wavenumber.Geometer.Contracts.StepTopologyA0.StepTopologyCheckpointEditJournalResultA0,
  step_topology_apply_hierarchy: Wavenumber.Geometer.Contracts.StepTopologyA0.StepTopologyApplyHierarchyResultA0,
  step_topology_save: Wavenumber.Geometer.Contracts.StepTopologyA0.StepTopologySaveResultA0,
  step_topology_restore: Wavenumber.Geometer.Contracts.StepTopologyA0.StepTopologyRestoreResultA0,
  step_topology_analyze_recovery: Wavenumber.Geometer.Contracts.StepTopologyA0.StepTopologyAnalyzeRecoveryResultA0,
}

/** A completed operation with its operation-specific result. */
model OperationSuccessA0 {
  @minLength(1)
  @maxLength(128)
  operation: string;

  ok: true;
  result: OperationResultValueA0;
}

/** A rejected or failed operation with governed diagnostics. */
model OperationFailureA0 {
  @minLength(1)
  @maxLength(128)
  operation: string;

  ok: false;

  @minItems(1)
  diagnostics: Wavenumber.Geometer.Contracts.Common.DiagnosticA0[];
}

/** Transport-neutral typed outcome shared by the generic C ABI and executable IPC. */
@jsonSchema
@id("urn:wavenumber:schema:geometer:geometer.operation.outcome:a0")
@contractIdentity("geometer.operation.outcome.a0")
@oneOf
union OperationOutcomeA0 {
  success: OperationSuccessA0,
  failure: OperationFailureA0,
}
