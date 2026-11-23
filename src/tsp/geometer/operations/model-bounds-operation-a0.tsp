import "@wavenumber/wn-geometer-contract-emitter";
import "./model-bounds-a0.tsp";

using Wavenumber.Geometer.ContractMetadata;

namespace Wavenumber.Geometer.Contracts.Operations;

/** Compute axis-aligned model bounds from the required raw model attachment. STEP input uses RFC 2045 MIME-value parsing; valid unknown parameters are ignored. New producers use model/step; application/step remains an input compatibility spelling. */
@operationIdentity("geometry.model_bounds.a0")
@inputAttachment("model", true, "application/step,model/step", 268435456)
op modelBounds(
  request: Wavenumber.Geometer.Contracts.ModelBoundsA0.ModelBoundsOptionsA0
): Wavenumber.Geometer.Contracts.ModelBoundsA0.ModelBoundsResultA0;
