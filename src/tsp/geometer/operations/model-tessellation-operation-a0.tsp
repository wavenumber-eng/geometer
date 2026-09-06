import "@wavenumber/wn-geometer-contract-emitter";
import "./model-tessellation-a0.tsp";

using Wavenumber.Geometer.ContractMetadata;

namespace Wavenumber.Geometer.Contracts.Operations;

/** Tessellate STEP bytes to bounded colored meshes for native GPU and illustration consumers. */
@operationIdentity("geometry.model_tessellation.a0")
@inputAttachment("model", true, "application/step,model/step", 268435456)
@outputAttachment("mesh_collection", true, "application/vnd.wavenumber.geometer.mesh-collection+json", 268435456)
op modelTessellation(
  request: Wavenumber.Geometer.Contracts.ModelTessellationA0.ModelTessellationRequestA0
): Wavenumber.Geometer.Contracts.ModelTessellationA0.ModelTessellationResultA0;
