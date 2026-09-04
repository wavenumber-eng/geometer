import "@typespec/json-schema";
import "@wavenumber/wn-geometer-contract-emitter";

using TypeSpec.JsonSchema;
using Wavenumber.Geometer.ContractMetadata;

namespace Wavenumber.Geometer.Contracts.MeshIllustrationA0;

@minItems(3)
@maxItems(3)
model IllustrationVector3 is float64[];

@minItems(16)
@maxItems(16)
model IllustrationMatrix4x4 is float64[];

model MeshIllustrationMaterial {
  /** sRGB channels in the inclusive range [0, 1]. */
  color: IllustrationVector3;
  @minValue(0) @maxValue(1) opacity?: float64;
  @maxLength(1024) name?: string;
}

model MeshIllustrationMesh {
  @minLength(1) @maxLength(1024) id: string;
  @minItems(9) @maxItems(6000000) positions: float64[];
  @maxItems(6000000) normals?: float64[];
  @maxItems(6000000) indices?: uint32[];
  matrix?: IllustrationMatrix4x4;
  @minItems(1) @maxItems(65536) materials: MeshIllustrationMaterial[];
  @maxItems(2000000) triangle_material_indices?: uint32[];
  double_sided?: boolean;
}

model MeshIllustrationView {
  direction: IllustrationVector3;
  up: IllustrationVector3;
  mirror_x?: boolean = false;
}

model MeshIllustrationPrepareOptions {
  @minValue(1) @maxValue(2000000) max_triangles?: uint32 = 750000;
  @minValueExclusive(0) weld_tolerance?: float64 = 0.0000001;
}

model MeshIllustrationSvgOptions {
  @minValue(10000) @maxValue(1000000000) coordinate_span?: uint32 = 1000000;
  @minLength(1) @maxLength(1024) title?: string;
}

enum MeshIllustrationShading {
  unlit,
  flat,
  lambert,
  banded,
  toon,
}

/** Presence-preserving illustration style. Package defaults apply to absent fields. */
@jsonSchema
@id("urn:wavenumber:schema:geometer:geometry.mesh_illustration.style:a0")
@contractIdentity("geometry.mesh_illustration.style.a0")
@optionPatch
model MeshIllustrationStyleA0 {
  shading?: MeshIllustrationShading;
  @minValue(0) @maxValue(1) ambient?: float64;
  @minValue(0) @maxValue(4) key_intensity?: float64;
  light_direction?: IllustrationVector3;
  @minValue(1) @maxValue(256) bands?: uint32;
  source_colors?: boolean;
  fallback_color?: IllustrationVector3;
  @minLength(1) @maxLength(128) background?: string;
  transparent_background?: boolean;
  fuse_surfaces?: boolean;
  layer_coplanar_materials?: boolean;
  show_hlr_outline?: boolean;
  show_hlr_detail?: boolean;
  show_outlines?: boolean;
  show_creases?: boolean;
  @minValue(0) @maxValue(180) crease_angle_degrees?: float64;
  @minLength(1) @maxLength(128) outline_color?: string;
  @minLength(1) @maxLength(128) crease_color?: string;
  @minValue(0) outline_width?: float64;
  @minValue(0) crease_width?: float64;
  double_sided?: boolean;
  @minValue(0) @maxValue(1) rim_amount?: float64;
}

/** Serializable one-shot illustration input; reusable prepared scenes are opaque package objects. */
@jsonSchema
@id("urn:wavenumber:schema:geometer:geometry.mesh_illustration.input:a0")
@contractIdentity("geometry.mesh_illustration.input.a0")
model MeshIllustrationInputA0 {
  schema: "geometry.mesh_illustration.input.a0";
  @minItems(1) @maxItems(65536) meshes: MeshIllustrationMesh[];
  view: MeshIllustrationView;
  prepare?: MeshIllustrationPrepareOptions;
  style?: MeshIllustrationStyleA0;
  svg?: MeshIllustrationSvgOptions;
}

model MeshIllustrationRenderStats {
  @minValue(0) triangles: uint32;
  @minValue(0) surface_draws: uint32;
  @minValue(0) layered_surfaces: uint32;
  @minValue(0) outlines: uint32;
  @minValue(0) details: uint32;
  @minValue(0) creases: uint32;
  @minValue(0) commands: uint32;
}

/** One-shot SVG result. Canvas rendering returns through the direct package API. */
@jsonSchema
@id("urn:wavenumber:schema:geometer:geometry.mesh_illustration.result:a0")
@contractIdentity("geometry.mesh_illustration.result.a0")
model MeshIllustrationResultA0 {
  schema: "geometry.mesh_illustration.result.a0";
  svg: string;
  stats: MeshIllustrationRenderStats;
  @maxItems(256) warnings: string[];
}
