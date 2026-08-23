import "@wavenumber/wn-geometer-contract-emitter";
import "./step-topology-a0.tsp";

using Wavenumber.Geometer.ContractMetadata;

namespace Wavenumber.Geometer.Contracts.Operations;

@experimentalOperation
@operationIdentity("geometry.step_topology.open.a0")
@inputAttachment("step", true, "application/step,model/step", 268435456)
op stepTopologyOpen(
  request: Wavenumber.Geometer.Contracts.StepTopologyA0.StepTopologyOpenRequestA0
): Wavenumber.Geometer.Contracts.StepTopologyA0.StepTopologyOpenResultA0;

@experimentalOperation
@operationIdentity("geometry.step_topology.close.a0")
op stepTopologyClose(
  request: Wavenumber.Geometer.Contracts.StepTopologyA0.StepTopologyCloseRequestA0
): Wavenumber.Geometer.Contracts.StepTopologyA0.StepTopologyCloseResultA0;

@experimentalOperation
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
@operationIdentity("geometry.step_topology.resolve_hit.a0")
op stepTopologyResolveHit(
  request: Wavenumber.Geometer.Contracts.StepTopologyA0.StepTopologyResolveHitRequestA0
): Wavenumber.Geometer.Contracts.StepTopologyA0.StepTopologyResolveHitResultA0;
