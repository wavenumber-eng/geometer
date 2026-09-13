import "@typespec/json-schema";
import "@wavenumber/wn-geometer-contract-emitter";
import "./hlr-projection-a0.tsp";
import "./mesh-illustration-a0.tsp";
import "./mesh-illustration-geometry-a0.tsp";
import "./model-tessellation-a0.tsp";

using TypeSpec.JsonSchema;
using Wavenumber.Geometer.ContractMetadata;

namespace Wavenumber.Geometer.Contracts.ModelIllustrationA0;

@minItems(2)
@maxItems(2)
model IllustrationVector2Mm is float64[];

enum PlanarArcSweep {
  cw,
  ccw,
}

model IllustrationProfileLineA0 {
  kind: "line";
}

model IllustrationProfileCircularArcA0 {
  kind: "circular_arc";
  center_mm: IllustrationVector2Mm;
  sweep: PlanarArcSweep;
}

@oneOf
union IllustrationProfileSegmentA0 {
  line: IllustrationProfileLineA0,
  circular_arc: IllustrationProfileCircularArcA0,
}

model IllustrationProfileRingA0 {
  /** Segment i connects point i to point (i + 1) modulo point count. */
  @minItems(3) @maxItems(131072)
  points_mm: IllustrationVector2Mm[];
  @minItems(3) @maxItems(131072)
  segments: IllustrationProfileSegmentA0[];
}

model IllustrationProfileRegionA0 {
  outer: IllustrationProfileRingA0;
  @maxItems(131071)
  holes?: IllustrationProfileRingA0[];
}

model AnalyticExtrusionA0 {
  kind: "extrusion";
  @minLength(1) @maxLength(1024)
  id: string;
  @minItems(1) @maxItems(65536)
  regions: IllustrationProfileRegionA0[];
  z_min_mm: float64;
  z_max_mm: float64;
  material: Wavenumber.Geometer.Contracts.MeshIllustrationA0.MeshIllustrationMaterial;
}

model AnalyticCylinderA0 {
  kind: "cylinder";
  @minLength(1) @maxLength(1024)
  id: string;
  center_mm: IllustrationVector2Mm;
  @minValueExclusive(0)
  radius_mm: float64;
  z_min_mm: float64;
  z_max_mm: float64;
  material: Wavenumber.Geometer.Contracts.MeshIllustrationA0.MeshIllustrationMaterial;
}

model AnalyticSphereA0 {
  kind: "sphere";
  @minLength(1) @maxLength(1024)
  id: string;
  center_mm: Wavenumber.Geometer.Contracts.MeshIllustrationA0.IllustrationVector3;
  @minValueExclusive(0)
  radius_mm: float64;
  material: Wavenumber.Geometer.Contracts.MeshIllustrationA0.MeshIllustrationMaterial;
}

@oneOf
union AnalyticPrimitiveA0 {
  extrusion: AnalyticExtrusionA0,
  cylinder: AnalyticCylinderA0,
  sphere: AnalyticSphereA0,
}

model AnalyticDefinitionA0 {
  @minLength(1) @maxLength(1024)
  id: string;
  @minItems(1) @maxItems(65536)
  primitives: AnalyticPrimitiveA0[];
}

model AnalyticOccurrenceA0 {
  @minLength(1) @maxLength(1024)
  id: string;
  @minLength(1) @maxLength(1024)
  definition_id: string;
  /** Omitted means identity. Column-major affine transform; translations are millimeters. */
  transform?: Wavenumber.Geometer.Contracts.MeshIllustrationA0.IllustrationMatrix4x4;
}

model AnalyticSceneA0 {
  @minItems(1) @maxItems(65536)
  definitions: AnalyticDefinitionA0[];
  @minItems(1) @maxItems(65536)
  occurrences: AnalyticOccurrenceA0[];
}

model AnalyticIllustrationLimitsA0 {
  @minValue(1) @maxValue(65536)
  max_reached_definitions?: uint32 = 4096;
  @minValue(1) @maxValue(65536)
  max_reached_occurrences?: uint32 = 65536;
  @minValue(1) @maxValue(65536)
  max_reached_primitives?: uint32 = 65536;
  @minValue(1) @maxValue(262144)
  max_reached_rings?: uint32 = 262144;
  @minValue(1) @maxValue(2000000)
  max_reached_points?: uint32 = 2000000;
  @minValue(1) @maxValue(2000000)
  max_generated_curve_samples?: uint32 = 2000000;
  @minValue(1) @maxValue(2000000)
  max_definition_triangles?: uint32 = 750000;
  @minValue(1) @maxValue(100000000)
  max_topology_candidate_pairs?: uint32 = 10000000;
}

model AnalyticLoweringOptionsA0 {
  @minValue(0.000001) @maxValue(1000)
  linear_deflection_mm?: float64 = 0.1;
  @minValue(0.000001) @maxValue(3.141592653589793)
  angular_deflection_rad?: float64 = 0.5;
  limits?: AnalyticIllustrationLimitsA0;
}

model ModelAttachmentIllustrationSourceA0 {
  kind: "model";
  attachment: "model";
  /** Applied after root-placement normalization. */
  transform?: Wavenumber.Geometer.Contracts.MeshIllustrationA0.IllustrationMatrix4x4;
  material_override?: Wavenumber.Geometer.Contracts.MeshIllustrationA0.MeshIllustrationMaterial;
  tessellation?: Wavenumber.Geometer.Contracts.ModelTessellationA0.ModelTessellationOptionsA0;
}

model AnalyticIllustrationSourceA0 {
  kind: "analytic";
  scene: AnalyticSceneA0;
  lowering?: AnalyticLoweringOptionsA0;
}

@oneOf
union ModelIllustrationSourceA0 {
  model_source: ModelAttachmentIllustrationSourceA0,
  analytic_source: AnalyticIllustrationSourceA0,
}

model ModelIllustrationWorkLimitsA0 {
  /** Maximum candidate comparisons in each Fast linework and illustration visibility stage. */
  @minValue(1) @maxValue(100000000)
  max_visibility_candidate_pairs?: uint32 = 100000000;
  @minValue(1) @maxValue(2000000)
  max_drawing_commands?: uint32 = 2000000;
}

model ModelIllustrationLineworkOptionsA0 {
  fast?: Wavenumber.Geometer.Contracts.HlrProjectionA0.FastHlrOptionsA0;
  /** Physical outline width applied after prepared source bounds are known. */
  @minValueExclusive(0) @maxValue(1000)
  outline_width_mm?: float64;
  /** Physical detail width applied after prepared source bounds are known. */
  @minValueExclusive(0) @maxValue(1000)
  detail_width_mm?: float64;
}

@jsonSchema
@id("urn:wavenumber:schema:geometer:geometry.model_illustration.request:a0")
@contractIdentity("geometry.model_illustration.request.a0")
model ModelIllustrationRequestA0 {
  schema: "geometry.model_illustration.request.a0";
  source: ModelIllustrationSourceA0;
  view: Wavenumber.Geometer.Contracts.MeshIllustrationA0.MeshIllustrationView;
  prepare?: Wavenumber.Geometer.Contracts.MeshIllustrationA0.MeshIllustrationPrepareOptions;
  linework?: ModelIllustrationLineworkOptionsA0;
  style?: Wavenumber.Geometer.Contracts.MeshIllustrationA0.MeshIllustrationStyleA0;
  svg?: Wavenumber.Geometer.Contracts.MeshIllustrationA0.MeshIllustrationSvgOptions;
  work_limits?: ModelIllustrationWorkLimitsA0;
}

@jsonSchema
@id("urn:wavenumber:schema:geometer:geometry.model_illustration_geometry.request:a0")
@contractIdentity("geometry.model_illustration_geometry.request.a0")
model ModelIllustrationGeometryRequestA0 {
  schema: "geometry.model_illustration_geometry.request.a0";
  source: ModelIllustrationSourceA0;
  view: Wavenumber.Geometer.Contracts.MeshIllustrationA0.MeshIllustrationView;
  prepare?: Wavenumber.Geometer.Contracts.MeshIllustrationA0.MeshIllustrationPrepareOptions;
  linework?: ModelIllustrationLineworkOptionsA0;
  style?: Wavenumber.Geometer.Contracts.MeshIllustrationA0.MeshIllustrationStyleA0;
  work_limits?: ModelIllustrationWorkLimitsA0;
}

enum ModelAttachmentMediaTypeA0 {
  application_step: "application/step",
  model_step: "model/step",
}

model ModelAttachmentSourceSummaryA0 {
  kind: "model";
  media_type: ModelAttachmentMediaTypeA0;
  @minLength(64) @maxLength(64)
  source_sha256: string;
  @maxValue(65536)
  meshes: uint32;
  @maxValue(2000000)
  triangles: uint32;
}

model AnalyticSourceSummaryA0 {
  kind: "analytic";
  @maxValue(65536)
  definitions: uint32;
  @maxValue(65536)
  occurrences: uint32;
  @maxValue(65536)
  primitives: uint32;
  @maxValue(2000000)
  triangles: uint32;
}

@oneOf
union ModelIllustrationSourceSummaryA0 {
  model_source: ModelAttachmentSourceSummaryA0,
  analytic_source: AnalyticSourceSummaryA0,
}

model ModelIllustrationTimingsA0 {
  /** Model import+tessellation or analytic validation+lowering. */
  @minValue(0) source_preparation_ms: float64;
  @minValue(0) linework_ms: float64;
  /** Shading, ordering, fusion, and SVG construction when applicable. */
  @minValue(0) illustration_ms: float64;
  /** Encoding a returned attachment, zero for the inline SVG operation. */
  @minValue(0) attachment_encoding_ms: float64;
}

@minItems(6)
@maxItems(6)
model ModelIllustrationBounds3MmA0 is float64[];

@jsonSchema
@id("urn:wavenumber:schema:geometer:geometry.model_illustration.result:a0")
@contractIdentity("geometry.model_illustration.result.a0")
model ModelIllustrationResultA0 {
  schema: "geometry.model_illustration.result.a0";
  svg: string;
  /** Prepared source bounds [min_x, min_y, min_z, max_x, max_y, max_z]. */
  bounds_mm: ModelIllustrationBounds3MmA0;
  source: ModelIllustrationSourceSummaryA0;
  stats: Wavenumber.Geometer.Contracts.MeshIllustrationA0.MeshIllustrationRenderStats;
  timings: ModelIllustrationTimingsA0;
  @maxItems(256)
  warnings: string[];
}

@jsonSchema
@id("urn:wavenumber:schema:geometer:geometry.model_illustration_geometry.result:a0")
@contractIdentity("geometry.model_illustration_geometry.result.a0")
model ModelIllustrationGeometryResultA0 {
  schema: "geometry.model_illustration_geometry.result.a0";
  geometry: Wavenumber.Geometer.Contracts.MeshIllustrationGeometryA0.IllustrationGeometryAttachment;
  /** Prepared source bounds [min_x, min_y, min_z, max_x, max_y, max_z]. */
  bounds_mm: ModelIllustrationBounds3MmA0;
  source: ModelIllustrationSourceSummaryA0;
  stats: Wavenumber.Geometer.Contracts.MeshIllustrationA0.MeshIllustrationRenderStats;
  timings: ModelIllustrationTimingsA0;
  @maxItems(256)
  warnings: string[];
}
