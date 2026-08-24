import "@wavenumber/wn-geometer-contract-emitter";
import "./step-topology-a0.tsp";

using Wavenumber.Geometer.ContractMetadata;

namespace Wavenumber.Geometer.Contracts.Operations;

@experimentalOperation
@nativeExperimentalOperation
@operationIdentity("geometry.step_topology.open.a0")
@inputAttachment("step", true, "application/step,model/step", 268435456)
op stepTopologyOpen(
  request: Wavenumber.Geometer.Contracts.StepTopologyA0.StepTopologyOpenRequestA0
): Wavenumber.Geometer.Contracts.StepTopologyA0.StepTopologyOpenResultA0;

@experimentalOperation
@nativeExperimentalOperation
@operationIdentity("geometry.step_topology.close.a0")
op stepTopologyClose(
  request: Wavenumber.Geometer.Contracts.StepTopologyA0.StepTopologyCloseRequestA0
): Wavenumber.Geometer.Contracts.StepTopologyA0.StepTopologyCloseResultA0;

@experimentalOperation
@nativeExperimentalOperation
@operationIdentity("geometry.step_topology.inspect.a0")
@outputAttachment(
  "topology_table",
  false,
  "application/vnd.wavenumber.geometer.step-topology-table",
  134217728
)
op stepTopologyInspect(
  request: Wavenumber.Geometer.Contracts.StepTopologyA0.StepTopologyInspectRequestA0
): Wavenumber.Geometer.Contracts.StepTopologyA0.StepTopologyInspectResultA0;

@experimentalOperation
@nativeExperimentalOperation
@operationIdentity("geometry.step_topology.render.a0")
@outputAttachment("glb", true, "model/gltf-binary", 268435456)
@outputAttachment(
  "topology_binding_table",
  false,
  "application/vnd.wavenumber.geometer.step-topology-binding-table",
  134217728
)
op stepTopologyRender(
  request: Wavenumber.Geometer.Contracts.StepTopologyA0.StepTopologyRenderRequestA0
): Wavenumber.Geometer.Contracts.StepTopologyA0.StepTopologyRenderResultA0;

@experimentalOperation
@nativeExperimentalOperation
@operationIdentity("geometry.step_topology.resolve_hit.a0")
op stepTopologyResolveHit(
  request: Wavenumber.Geometer.Contracts.StepTopologyA0.StepTopologyResolveHitRequestA0
): Wavenumber.Geometer.Contracts.StepTopologyA0.StepTopologyResolveHitResultA0;

@experimentalOperation
@nativeExperimentalOperation
@operationIdentity("geometry.step_topology.apply_logical_groups.a0")
op stepTopologyApplyLogicalGroups(
  request: Wavenumber.Geometer.Contracts.StepTopologyA0.StepTopologyApplyLogicalGroupsRequestA0
): Wavenumber.Geometer.Contracts.StepTopologyA0.StepTopologyApplyLogicalGroupsResultA0;

@experimentalOperation
@nativeExperimentalOperation
@operationIdentity("geometry.step_topology.apply_metadata_probes.a0")
op stepTopologyApplyMetadataProbes(
  request: Wavenumber.Geometer.Contracts.StepTopologyA0.StepTopologyApplyMetadataProbesRequestA0
): Wavenumber.Geometer.Contracts.StepTopologyA0.StepTopologyApplyMetadataProbesResultA0;

@experimentalOperation
@nativeExperimentalOperation
@operationIdentity("geometry.step_topology.checkpoint_edit_journal.a0")
@outputAttachment(
  "edit_journal",
  true,
  "application/vnd.wavenumber.geometer.step-topology-edit-journal",
  67108864
)
op stepTopologyCheckpointEditJournal(
  request: Wavenumber.Geometer.Contracts.StepTopologyA0.StepTopologyCheckpointEditJournalRequestA0
): Wavenumber.Geometer.Contracts.StepTopologyA0.StepTopologyCheckpointEditJournalResultA0;

@experimentalOperation
@operationIdentity("geometry.step_topology.apply_hierarchy.a0")
op stepTopologyApplyHierarchy(
  request: Wavenumber.Geometer.Contracts.StepTopologyA0.StepTopologyApplyHierarchyRequestA0
): Wavenumber.Geometer.Contracts.StepTopologyA0.StepTopologyApplyHierarchyResultA0;

@experimentalOperation
@operationIdentity("geometry.step_topology.save.a0")
@outputAttachment(
  "state_artifact",
  true,
  "application/vnd.opencascade.xbf,application/vnd.opencascade.xml-xcaf,application/step,application/vnd.wavenumber.geometer.step-topology-sidecar+json",
  536870912
)
op stepTopologySave(
  request: Wavenumber.Geometer.Contracts.StepTopologyA0.StepTopologySaveRequestA0
): Wavenumber.Geometer.Contracts.StepTopologyA0.StepTopologySaveResultA0;

@experimentalOperation
@nativeExperimentalOperation
@operationIdentity("geometry.step_topology.restore.a0")
@inputAttachment("source", true, "application/step,model/step", 268435456)
@inputAttachment(
  "state_artifact",
  true,
  "application/vnd.wavenumber.geometer.step-topology-edit-journal",
  67108864
)
op stepTopologyRestore(
  request: Wavenumber.Geometer.Contracts.StepTopologyA0.StepTopologyRestoreRequestA0
): Wavenumber.Geometer.Contracts.StepTopologyA0.StepTopologyRestoreResultA0;

@experimentalOperation
@operationIdentity("geometry.step_topology.analyze_recovery.a0")
op stepTopologyAnalyzeRecovery(
  request: Wavenumber.Geometer.Contracts.StepTopologyA0.StepTopologyAnalyzeRecoveryRequestA0
): Wavenumber.Geometer.Contracts.StepTopologyA0.StepTopologyAnalyzeRecoveryResultA0;
