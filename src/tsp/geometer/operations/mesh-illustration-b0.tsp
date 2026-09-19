import "@typespec/json-schema";
import "@wavenumber/wn-geometer-contract-emitter";
import "./illustration-clipping-b0.tsp";
import "./mesh-illustration-a0.tsp";

using TypeSpec.JsonSchema;
using Wavenumber.Geometer.ContractMetadata;

namespace Wavenumber.Geometer.Contracts.MeshIllustrationB0 {

/** Serializable direct-value B0 illustration input. */
@jsonSchema
@id("urn:wavenumber:schema:geometer:geometry.mesh_illustration.input:b0")
@contractIdentity("geometry.mesh_illustration.input.b0")
model MeshIllustrationInputB0 {
  schema: "geometry.mesh_illustration.input.b0";
  @minItems(1) @maxItems(65536)
  meshes: Wavenumber.Geometer.Contracts.MeshIllustrationA0.MeshIllustrationMesh[];
  view: Wavenumber.Geometer.Contracts.MeshIllustrationA0.MeshIllustrationView;
  prepare?: Wavenumber.Geometer.Contracts.MeshIllustrationA0.MeshIllustrationPrepareOptions;
  style?: Wavenumber.Geometer.Contracts.MeshIllustrationA0.MeshIllustrationStyleA0;
  svg?: Wavenumber.Geometer.Contracts.MeshIllustrationA0.MeshIllustrationSvgOptions;
  clipping?: Wavenumber.Geometer.Contracts.IllustrationClippingB0.IllustrationClipping;
}

/** Attachment-backed B0 mesh illustration request. */
@jsonSchema
@id("urn:wavenumber:schema:geometer:geometry.mesh_illustration.request:b0")
@contractIdentity("geometry.mesh_illustration.request.b0")
model MeshIllustrationRequestB0 {
  schema: "geometry.mesh_illustration.request.b0";
  view: Wavenumber.Geometer.Contracts.MeshIllustrationA0.MeshIllustrationView;
  prepare?: Wavenumber.Geometer.Contracts.MeshIllustrationA0.MeshIllustrationPrepareOptions;
  style?: Wavenumber.Geometer.Contracts.MeshIllustrationA0.MeshIllustrationStyleA0;
  svg?: Wavenumber.Geometer.Contracts.MeshIllustrationA0.MeshIllustrationSvgOptions;
  clipping?: Wavenumber.Geometer.Contracts.IllustrationClippingB0.IllustrationClipping;
}

/** B0 SVG illustration result with explicit fragment emptiness and identity. */
@jsonSchema
@id("urn:wavenumber:schema:geometer:geometry.mesh_illustration.result:b0")
@contractIdentity("geometry.mesh_illustration.result.b0")
model MeshIllustrationResultB0 {
  schema: "geometry.mesh_illustration.result.b0";
  empty: boolean;
  svg: string;
  stats: Wavenumber.Geometer.Contracts.MeshIllustrationA0.MeshIllustrationRenderStats;
  fragment: Wavenumber.Geometer.Contracts.IllustrationClippingB0.FragmentMetadata;
  @maxItems(256) warnings: string[];
}
}

namespace Wavenumber.Geometer.Contracts.Operations {

/** Render a colored mesh collection with optional deterministic half-space clipping. */
@operationIdentity("geometry.mesh_illustration.b0")
@inputAttachment("mesh_collection", true, "application/vnd.wavenumber.geometer.mesh-collection+json", 268435456)
@inputAttachment("hlr_projection", false, "application/vnd.wavenumber.geometer.hlr-projection+json", 67108864)
op meshIllustrationB0(
  request: Wavenumber.Geometer.Contracts.MeshIllustrationB0.MeshIllustrationRequestB0
): Wavenumber.Geometer.Contracts.MeshIllustrationB0.MeshIllustrationResultB0;
}
