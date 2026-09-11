import "@wavenumber/wn-geometer-contract-emitter";
import "./mesh-illustration-geometry-a0.tsp";

using Wavenumber.Geometer.ContractMetadata;
using Wavenumber.Geometer.Contracts.MeshIllustrationGeometryA0;

namespace Wavenumber.Geometer.Contracts.Operations;

/** Return ordered shaded illustration geometry without generating SVG. Does not compute HLR. */
@operationIdentity("geometry.mesh_illustration_geometry.a0")
@inputAttachment("mesh_collection", true, "application/vnd.wavenumber.geometer.mesh-collection+json", 268435456)
@inputAttachment("hlr_projection", false, "application/vnd.wavenumber.geometer.hlr-projection+json", 67108864)
@outputAttachment("illustration_geometry", true, "application/vnd.wavenumber.geometer.illustration-geometry+json", 268435456)
op meshIllustrationGeometry(request: MeshIllustrationGeometryRequestA0): MeshIllustrationGeometryResultA0;
