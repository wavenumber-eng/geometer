import "@wavenumber/wn-geometer-contract-emitter";
import "./model-illustration-a0.tsp";

using Wavenumber.Geometer.ContractMetadata;
using Wavenumber.Geometer.Contracts.ModelIllustrationA0;

namespace Wavenumber.Geometer.Contracts.Operations;

/** Import or lower one source and return a complete deterministic SVG illustration. */
@operationIdentity("geometry.model_illustration.a0")
@inputAttachment("model", false, "application/step,model/step", 268435456)
op modelIllustration(request: ModelIllustrationRequestA0): ModelIllustrationResultA0;

/** Import or lower one source and return renderer-neutral illustration geometry. */
@operationIdentity("geometry.model_illustration_geometry.a0")
@inputAttachment("model", false, "application/step,model/step", 268435456)
@outputAttachment("illustration_geometry", true, "application/vnd.wavenumber.geometer.illustration-geometry+json", 268435456)
op modelIllustrationGeometry(
  request: ModelIllustrationGeometryRequestA0
): ModelIllustrationGeometryResultA0;
