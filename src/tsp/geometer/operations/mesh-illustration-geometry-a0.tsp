import "@typespec/json-schema";
import "@wavenumber/wn-geometer-contract-emitter";
import "./mesh-illustration-a0.tsp";

using TypeSpec.JsonSchema;
using Wavenumber.Geometer.ContractMetadata;
using Wavenumber.Geometer.Contracts.MeshIllustrationA0;

namespace Wavenumber.Geometer.Contracts.MeshIllustrationGeometryA0;

@minItems(2) @maxItems(2)
model IllustrationPoint2 is float64[];

/** Implicitly closed ring. All rings of one layer form one even-odd filled path. */
model IllustrationRing {
  @minItems(3) @maxItems(6000000) points: IllustrationPoint2[];
}

model IllustrationGeometryBounds {
  min: IllustrationPoint2;
  max: IllustrationPoint2;
}

enum IllustrationSurfaceKind { triangle, fused, layered }

model IllustrationGeometryLayer {
  @minItems(1) @maxItems(2000000) rings: IllustrationRing[];
  /** Sanitized CSS color. Color alpha and numeric layer opacity multiply. */
  @minLength(1) @maxLength(128) fill: string;
  @minValue(0) @maxValue(1) opacity: float64;
}

/** Array order is paint order, including ordered underpaint/inlay layers. */
model IllustrationGeometrySurface {
  kind: IllustrationSurfaceKind;
  @minItems(1) @maxItems(2000000) layers: IllustrationGeometryLayer[];
}

/** Draw after all surfaces, in array order. Width is in projected millimeters. */
model IllustrationGeometryLine {
  start: IllustrationPoint2;
  end: IllustrationPoint2;
  @minLength(1) @maxLength(128) color: string;
  @minValue(0) width: float64;
}

/** Target-independent paint semantics; SVG fitting/quantization stays in its adapter. */
model IllustrationGeometryPresentation {
  fill_rule: "evenodd";
  line_cap: "round";
  line_join: "round";
  @minLength(1) @maxLength(128) background: string;
  transparent_background: boolean;
  /** Same-fill seam stroke before target rounding/minimum-pixel rules. */
  @minValue(0) seam_width: float64;
  /** Existing SVG viewport padding in projected millimeters. */
  @minValue(0) padding: float64;
}

/**
 * Shaded, fused drawing geometry; no SVG/XML/path strings and no original 3D mesh.
 * Coordinates use X=dot(world,right) with mirror_x applied and Y=dot(world,up).
 * right/up are the orthonormal basis derived from view. Origin is world origin.
 * Bounds include valid triangles before culling, exclude supplied HLR, and are
 * [-1,-1]..[1,1] when all triangles skip. Arrays establish painter order:
 * surfaces/layers, then diagnostic raw lines, then HLR detail, then HLR outline.
 * Global limits: 2,000,000 layers and rings, 6,000,000 ring points, 1,000,000 lines.
 * stats.commands counts logical draws, not SVG elements after line chaining.
 */
@jsonSchema
@id("urn:wavenumber:schema:geometer:geometry.mesh_illustration.geometry:a0")
@contractIdentity("geometry.mesh_illustration.geometry.a0")
model MeshIllustrationGeometryA0 {
  schema: "geometry.mesh_illustration.geometry.a0";
  length_unit: "millimeter";
  view: MeshIllustrationView;
  bounds: IllustrationGeometryBounds;
  @maxItems(2000000) surfaces: IllustrationGeometrySurface[];
  @maxItems(1000000) lines: IllustrationGeometryLine[];
  presentation: IllustrationGeometryPresentation;
  stats: MeshIllustrationRenderStats;
  @maxItems(256) warnings: string[];
}

/** Direct value input. Mesh coordinates and matrix translations must be millimeters. */
@jsonSchema
@id("urn:wavenumber:schema:geometer:geometry.mesh_illustration_geometry.input:a0")
@contractIdentity("geometry.mesh_illustration_geometry.input.a0")
model MeshIllustrationGeometryInputA0 {
  schema: "geometry.mesh_illustration_geometry.input.a0";
  length_unit: "millimeter";
  @minItems(1) @maxItems(65536) meshes: MeshIllustrationMesh[];
  view: MeshIllustrationView;
  prepare?: MeshIllustrationPrepareOptions;
  style?: MeshIllustrationStyleA0;
}

/** Reuses bounded mesh_collection and optional matching visible HLR attachments. */
@jsonSchema
@id("urn:wavenumber:schema:geometer:geometry.mesh_illustration_geometry.request:a0")
@contractIdentity("geometry.mesh_illustration_geometry.request.a0")
model MeshIllustrationGeometryRequestA0 {
  schema: "geometry.mesh_illustration_geometry.request.a0";
  view: MeshIllustrationView;
  prepare?: MeshIllustrationPrepareOptions;
  style?: MeshIllustrationStyleA0;
}

model IllustrationGeometryAttachment {
  attachment: "illustration_geometry";
  schema: "geometry.mesh_illustration.geometry.a0";
  @minValue(1) @maxValue(268435456) byte_length: uint32;
  @minLength(64) @maxLength(64) sha256: string;
}

/** Small transport result; statistics/warnings must match the geometry attachment. */
@jsonSchema
@id("urn:wavenumber:schema:geometer:geometry.mesh_illustration_geometry.result:a0")
@contractIdentity("geometry.mesh_illustration_geometry.result.a0")
model MeshIllustrationGeometryResultA0 {
  schema: "geometry.mesh_illustration_geometry.result.a0";
  geometry: IllustrationGeometryAttachment;
  stats: MeshIllustrationRenderStats;
  @maxItems(256) warnings: string[];
}
