import "@typespec/json-schema";
import "@wavenumber/wn-geometer-contract-emitter";

using TypeSpec.JsonSchema;
using Wavenumber.Geometer.ContractMetadata;

namespace Wavenumber.Geometer.Contracts.HlrProjectionA0;

enum HlrProjectionAlgorithm {
  poly: "poly",
  exact: "exact",
  fast: "fast",
}

enum HlrCurveMode {
  native_arcs: "native_arcs",
  polyline: "polyline",
}

enum HlrMeshDeflectionMode {
  absolute: "absolute",
  bbox_relative: "bbox-relative",
}

enum HlrOutlineAlgorithm {
  hlr_close: "hlr-close",
  mesh_shadow: "mesh-shadow",
  fast_mesh_shadow: "fast-mesh-shadow",
}

enum HlrSourceKind {
  step: "step",
  indexed_mesh: "indexed_mesh",
}

@minItems(3)
@maxItems(3)
model HlrVector3 is float64[];

@minItems(16)
@maxItems(16)
model HlrMatrix4x4 is float64[];

model HlrViewSpec {
  @minLength(1) @maxLength(128) id: string;
  direction: HlrVector3;
  up: HlrVector3;
}

/** Resource ceilings applied during Fast mesh preparation and projection. */
model FastHlrLimitsA0 {
  @minValue(1) @maxValue(4294967295) max_vertices?: uint32 = 2000000;
  @minValue(1) @maxValue(4294967295) max_triangles?: uint32 = 2000000;
  @minValue(1) @maxValue(4294967295) max_edges?: uint32 = 4000000;
  @minValue(1) @maxValue(4294967295) max_grid_references?: uint32 = 64000000;
  @minValue(1) @maxValue(4294967295) max_candidate_pairs?: uint32 = 100000000;
  @minValue(1) @maxValue(4294967295) max_fragments?: uint32 = 8000000;
  @minValue(1) @maxValue(4294967295) max_output_segments?: uint32 = 4000000;
}

/** Fast vector-HLR controls. Angles are radians and geometric tolerances use model units. */
model FastHlrOptionsA0 {
  include_boundaries?: boolean = true;
  include_creases?: boolean = true;
  include_silhouettes?: boolean = true;
  include_hidden?: boolean = false;
  suppress_coplanar_seams?: boolean = false;
  @minValue(0) @maxValue(3.141592653589793) crease_angle_rad?: float64 = 0.5235987755982988;
  @minValueExclusive(0) weld_tolerance?: float64 = 0.0000001;
  @minValueExclusive(0) projected_tolerance?: float64 = 0.00000001;
  @minValue(0) depth_tolerance?: float64 = 0.0000001;
  @minValue(0) @maxValue(1.5707963267948966)
  coplanar_seam_angle_rad?: float64 = 0.017453292519943295;
  @minValue(0) coplanar_seam_depth_tolerance?: float64 = 0.000001;
  @minValue(0) coplanar_seam_lateral_tolerance?: float64 = 0.000001;
  limits?: FastHlrLimitsA0;
}

/** Presence-preserving additive options shared by STEP and indexed-mesh HLR operations. */
@jsonSchema
@id("urn:wavenumber:schema:geometer:geometry.hlr_projection.options:a0")
@contractIdentity("geometry.hlr_projection.options.a0")
@optionPatch
model HlrProjectionOptionsA0 {
  @maxItems(64) views?: HlrViewSpec[];
  output_outline?: boolean = true;
  output_detail?: boolean = true;
  output_bbox?: boolean = true;
  @extension("x-wn-default-intent", "identity_4x4")
  model_transform?: HlrMatrix4x4;
  strip_root_placement?: boolean = false;
  curve_mode?: HlrCurveMode = HlrCurveMode.native_arcs;
  @minValue(1) @maxValue(100000) samples_per_curve?: uint32 = 24;
  @minValue(0) @maxValue(9) round_digits?: uint32 = 3;

  edge_v_sharp?: boolean = true;
  edge_v_outline?: boolean = true;
  edge_v_smooth?: boolean = false;
  edge_v_sewn?: boolean = false;
  edge_v_iso?: boolean = false;
  edge_h_sharp?: boolean = false;
  edge_h_outline?: boolean = false;
  edge_h_smooth?: boolean = false;
  edge_h_sewn?: boolean = false;
  edge_h_iso?: boolean = false;

  union_outline_polygons?: boolean = true;
  /** Omission selects the operation-specific default: poly for model HLR and Fast for mesh HLR. */
  projection_algorithm?: HlrProjectionAlgorithm;
  @minValue(0) mesh_linear_deflection?: float64 = 0.01;
  @minValue(0) @maxValue(3.141592653589793)
  mesh_angular_deflection?: float64 = 0.5;
  mesh_relative?: boolean = false;
  mesh_deflection_mode?: HlrMeshDeflectionMode = HlrMeshDeflectionMode.bbox_relative;
  @minValue(0) mesh_deflection_coefficient?: float64 = 0.004;
  /** Omission selects the operation-specific default: HLR-close for model HLR and Fast mesh-shadow for mesh HLR. */
  outline_algorithm?: HlrOutlineAlgorithm;
  @minValue(0) @maxValue(3.141592653589793)
  hlr_angle_tolerance?: float64 = 0.0174533;
  fast?: FastHlrOptionsA0;
}

@minItems(4)
@maxItems(4)
model ProjectedSegment is float64[];

@minItems(2)
@maxItems(2)
model HlrVector2 is float64[];

model ProjectedArc {
  start: HlrVector2;
  end: HlrVector2;
  center: HlrVector2;
  @minValue(0) radius: float64;
  @minValue(0) @maxValue(6.283185307179586) extent_rad: float64;
  ccw: boolean;
  full_circle: boolean;
}

model ProjectionBounds {
  min_x: float64;
  min_y: float64;
  max_x: float64;
  max_y: float64;
  @minValue(0) width: float64;
  @minValue(0) height: float64;
}

model ProjectedGeometry {
  @maxItems(4000000) segments: ProjectedSegment[];
  @maxItems(4000000) arcs: ProjectedArc[];
  /** Omitted when this output layer is empty. The legacy B0 writer emits null. */
  bounds?: ProjectionBounds;
}

model HlrProjectionModes {
  outline: ProjectedGeometry;
  detail: ProjectedGeometry;
  bbox: ProjectedGeometry;
}

model HlrProjectedView {
  @minLength(1) @maxLength(128) id: string;
  direction: HlrVector3;
  up: HlrVector3;
  modes: HlrProjectionModes;
}

model HlrProjectionSource {
  kind: HlrSourceKind;
  @minLength(64) @maxLength(64) hash: string;
}

/** Timings are nondeterministic and excluded only by explicit conformance projections. */
model HlrProjectionTimings {
  @minValue(0) step_read_ms: float64;
  @minValue(0) mesh_ms: float64;
  @minValue(0) hlr_ms: float64;
  @minValue(0) extract_ms: float64;
}

@jsonSchema
@id("urn:wavenumber:schema:geometer:geometry.hlr_projection.result:a0")
@contractIdentity("geometry.hlr_projection.result.a0")
model HlrProjectionResultA0 {
  schema: "geometry.hlr_projection.result.a0";
  units: "mm";
  source: HlrProjectionSource;
  @maxItems(64) views: HlrProjectedView[];
  timings: HlrProjectionTimings;
}
