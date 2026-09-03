import "@wavenumber/wn-geometer-contract-emitter";
import "./hlr-projection-a0.tsp";

using Wavenumber.Geometer.ContractMetadata;

namespace Wavenumber.Geometer.Contracts.Operations;

/** Project STEP model bytes through the selected polygonal, exact, or Fast HLR backend. */
@operationIdentity("geometry.model_hlr_projection.a0")
@inputAttachment("model", true, "application/step,model/step", 268435456)
op modelHlrProjection(
  request: Wavenumber.Geometer.Contracts.HlrProjectionA0.HlrProjectionOptionsA0
): Wavenumber.Geometer.Contracts.HlrProjectionA0.HlrProjectionResultA0;

/** Project a synthesized indexed triangle mesh through the Fast HLR backend. */
@operationIdentity("geometry.mesh_hlr_projection.a0")
@inputAttachment(
  "mesh",
  true,
  "application/vnd.wavenumber.geometer.indexed-triangle-mesh",
  268435456
)
op meshHlrProjection(
  request: Wavenumber.Geometer.Contracts.HlrProjectionA0.HlrProjectionOptionsA0
): Wavenumber.Geometer.Contracts.HlrProjectionA0.HlrProjectionResultA0;
