import "@typespec/json-schema";
import "@wavenumber/wn-geometer-contract-emitter";
import "./illustration-clipping-b0.tsp";
import "./mesh-illustration-a0.tsp";
import "./mesh-illustration-geometry-a0.tsp";

using TypeSpec.JsonSchema;
using Wavenumber.Geometer.ContractMetadata;

namespace Wavenumber.Geometer.Contracts.MeshIllustrationGeometryB0 {

/** Renderer-neutral B0 geometry with optional bounds for an empty fragment. */
@jsonSchema
@id("urn:wavenumber:schema:geometer:geometry.mesh_illustration.geometry:b0")
@contractIdentity("geometry.mesh_illustration.geometry.b0")
model MeshIllustrationGeometryB0 {
  schema: "geometry.mesh_illustration.geometry.b0";
  length_unit: "millimeter";
  empty: boolean;
  view: Wavenumber.Geometer.Contracts.MeshIllustrationA0.MeshIllustrationView;
  bounds?: Wavenumber.Geometer.Contracts.MeshIllustrationGeometryA0.IllustrationGeometryBounds;
  @maxItems(2000000)
  surfaces: Wavenumber.Geometer.Contracts.MeshIllustrationGeometryA0.IllustrationGeometrySurface[];
  @maxItems(1000000)
  lines: Wavenumber.Geometer.Contracts.MeshIllustrationGeometryA0.IllustrationGeometryLine[];
  presentation: Wavenumber.Geometer.Contracts.MeshIllustrationGeometryA0.IllustrationGeometryPresentation;
  stats: Wavenumber.Geometer.Contracts.MeshIllustrationA0.MeshIllustrationRenderStats;
  fragment: Wavenumber.Geometer.Contracts.IllustrationClippingB0.FragmentMetadata;
  @maxItems(256) warnings: string[];
}

/** Direct-value B0 geometry input. */
@jsonSchema
@id("urn:wavenumber:schema:geometer:geometry.mesh_illustration_geometry.input:b0")
@contractIdentity("geometry.mesh_illustration_geometry.input.b0")
model MeshIllustrationGeometryInputB0 {
  schema: "geometry.mesh_illustration_geometry.input.b0";
  length_unit: "millimeter";
  @minItems(1) @maxItems(65536)
  meshes: Wavenumber.Geometer.Contracts.MeshIllustrationA0.MeshIllustrationMesh[];
  view: Wavenumber.Geometer.Contracts.MeshIllustrationA0.MeshIllustrationView;
  prepare?: Wavenumber.Geometer.Contracts.MeshIllustrationA0.MeshIllustrationPrepareOptions;
  style?: Wavenumber.Geometer.Contracts.MeshIllustrationA0.MeshIllustrationStyleA0;
  clipping?: Wavenumber.Geometer.Contracts.IllustrationClippingB0.IllustrationClipping;
}

/** Attachment-backed B0 geometry request. */
@jsonSchema
@id("urn:wavenumber:schema:geometer:geometry.mesh_illustration_geometry.request:b0")
@contractIdentity("geometry.mesh_illustration_geometry.request.b0")
model MeshIllustrationGeometryRequestB0 {
  schema: "geometry.mesh_illustration_geometry.request.b0";
  view: Wavenumber.Geometer.Contracts.MeshIllustrationA0.MeshIllustrationView;
  prepare?: Wavenumber.Geometer.Contracts.MeshIllustrationA0.MeshIllustrationPrepareOptions;
  style?: Wavenumber.Geometer.Contracts.MeshIllustrationA0.MeshIllustrationStyleA0;
  clipping?: Wavenumber.Geometer.Contracts.IllustrationClippingB0.IllustrationClipping;
}

model IllustrationGeometryAttachmentB0 {
  attachment: "illustration_geometry";
  schema: "geometry.mesh_illustration.geometry.b0";
  @minValue(1) @maxValue(268435456) byte_length: uint32;
  @minLength(64) @maxLength(64) sha256: string;
}

/** B0 metadata for the renderer-neutral geometry attachment. */
@jsonSchema
@id("urn:wavenumber:schema:geometer:geometry.mesh_illustration_geometry.result:b0")
@contractIdentity("geometry.mesh_illustration_geometry.result.b0")
model MeshIllustrationGeometryResultB0 {
  schema: "geometry.mesh_illustration_geometry.result.b0";
  empty: boolean;
  geometry: IllustrationGeometryAttachmentB0;
  stats: Wavenumber.Geometer.Contracts.MeshIllustrationA0.MeshIllustrationRenderStats;
  fragment: Wavenumber.Geometer.Contracts.IllustrationClippingB0.FragmentMetadata;
  @maxItems(256) warnings: string[];
}
}

namespace Wavenumber.Geometer.Contracts.Operations {

/** Return renderer-neutral B0 geometry for a colored mesh collection. */
@operationIdentity("geometry.mesh_illustration_geometry.b0")
@inputAttachment("mesh_collection", true, "application/vnd.wavenumber.geometer.mesh-collection+json", 268435456)
@inputAttachment("hlr_projection", false, "application/vnd.wavenumber.geometer.hlr-projection+json", 67108864)
@outputAttachment("illustration_geometry", true, "application/vnd.wavenumber.geometer.illustration-geometry+json", 268435456)
op meshIllustrationGeometryB0(
  request: Wavenumber.Geometer.Contracts.MeshIllustrationGeometryB0.MeshIllustrationGeometryRequestB0
): Wavenumber.Geometer.Contracts.MeshIllustrationGeometryB0.MeshIllustrationGeometryResultB0;
}
