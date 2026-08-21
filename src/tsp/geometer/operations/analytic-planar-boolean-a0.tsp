import "@typespec/json-schema";
import "@wavenumber/wn-geometer-contract-emitter";

using TypeSpec.JsonSchema;
using Wavenumber.Geometer.ContractMetadata;

namespace Wavenumber.Geometer.Contracts.AnalyticPlanarBooleanA0;

@minValue(1) scalar JobId extends uint64;
@minValue(1) scalar StageId extends uint64;
@minValue(1) scalar OperandId extends uint64;
@minValue(1) scalar RegionId extends uint64;
@minValue(1) scalar RingId extends uint64;
@minValue(1) scalar PathId extends uint64;
@minValue(1) scalar VertexId extends uint64;
@minValue(1) scalar SegmentId extends uint64;
@minValue(1) scalar CurveId extends uint64;
@minValue(1) scalar FeatureId extends uint64;
@minValue(1) scalar QueryId extends uint64;
@minValue(1) scalar ResultVertexId extends uint64;
@minValue(1) scalar ResultFragmentId extends uint64;
@minValue(1) scalar ResultRingId extends uint64;
@minValue(1) scalar ResultRegionId extends uint64;

enum StageOperation {
  union_stage: "union",
  difference: "difference",
}

enum ArcDirection {
  ccw: "ccw",
  cw: "cw",
}

enum JobDiagnosticCode {
  invalid_topology: "geometer.operation.analytic_planar_boolean.invalid_topology",
  invalid_arc: "geometer.operation.analytic_planar_boolean.invalid_arc",
  unsupported_geometry: "geometer.operation.analytic_planar_boolean.unsupported_geometry",
  normalization_error_exceeded: "geometer.operation.analytic_planar_boolean.normalization_error_exceeded",
  normalization_topology_collapse: "geometer.operation.analytic_planar_boolean.normalization_topology_collapse",
  nonanalytic_result: "geometer.operation.analytic_planar_boolean.nonanalytic_result",
  solver_failed: "geometer.operation.analytic_planar_boolean.solver_failed",
  resource_limit_exceeded: "geometer.operation.analytic_planar_boolean.resource_limit_exceeded",
}

enum DiagnosticSeverity {
  error: "error",
  warning: "warning",
}

/** Standalone logical identities projected one-to-one from nonzero packed path tokens. */
enum JobDiagnosticPath {
  request_jobs: "request_jobs",
  job_id: "job_id",
  job_stages: "job_stages",
  stage_id: "stage_id",
  stage_operation: "stage_operation",
  stage_operands: "stage_operands",
  operand_id: "operand_id",
  operand_geometry: "operand_geometry",
  region_outer: "region_outer",
  region_holes: "region_holes",
  ring_vertices: "ring_vertices",
  ring_segments: "ring_segments",
  path_vertices: "path_vertices",
  path_segments: "path_segments",
  segment_curve: "segment_curve",
  disk_radius: "disk_radius",
  annulus_inner_radius: "annulus_inner_radius",
  annulus_outer_radius: "annulus_outer_radius",
  capsule_start: "capsule_start",
  capsule_end: "capsule_end",
  capsule_width: "capsule_width",
  swept_path_centerline: "swept_path_centerline",
  swept_path_width: "swept_path_width",
  relationship_queries: "relationship_queries",
  relationship_left_job_id: "relationship_left_job_id",
  relationship_right_job_id: "relationship_right_job_id",
}

enum OperandOutcomeKind {
  contributes_final_material: "contributes_final_material",
  redundant_or_absorbed_coverage: "redundant_or_absorbed_coverage",
  partially_removed_later: "partially_removed_later",
  completely_removed_later: "completely_removed_later",
  subtraction_effect_survives: "subtraction_effect_survives",
  subtraction_effect_overwritten_later: "subtraction_effect_overwritten_later",
  no_effect: "no_effect",
}

enum RelationshipStatus {
  success: "success",
  skipped_dependency_failed: "skipped_dependency_failed",
}

enum IntersectionDimension {
  disjoint: "disjoint",
  point: "point",
  curve: "curve",
  area: "area",
}

enum SourceKind {
  authored_segment_curve: "authored_segment_curve",
  compact_feature_role: "compact_feature_role",
  subtractive_operand_effect: "subtractive_operand_effect",
}

enum SourceRole {
  none: "none",
  authored_line: "authored_line",
  authored_circular_arc: "authored_circular_arc",
  primitive_outer_circle: "primitive_outer_circle",
  primitive_inner_circle: "primitive_inner_circle",
  capsule_left_line: "capsule_left_line",
  capsule_end_cap: "capsule_end_cap",
  capsule_right_line: "capsule_right_line",
  capsule_start_cap: "capsule_start_cap",
  swept_left_offset_line: "swept_left_offset_line",
  swept_left_offset_arc: "swept_left_offset_arc",
  swept_right_offset_line: "swept_right_offset_line",
  swept_right_offset_arc: "swept_right_offset_arc",
  swept_round_join: "swept_round_join",
  swept_start_cap: "swept_start_cap",
  swept_end_cap: "swept_end_cap",
}

model PointNm {
  x: int64;
  y: int64;
}

model AuthoredVertex {
  vertex_id: VertexId;
  point: PointNm;
}

model AuthoredLineSegment {
  segment_id: SegmentId;
  curve_id: CurveId;
  kind: "line";
}

model AuthoredCircularArcSegment {
  segment_id: SegmentId;
  curve_id: CurveId;
  kind: "circular_arc";
  center: PointNm;
  direction: ArcDirection;
  major_arc: boolean;
}

/** A circular arc selected exactly by its topology-owned endpoints, radius, direction, and branch. */
model AuthoredCircularArcByRadiusSegment {
  segment_id: SegmentId;
  curve_id: CurveId;
  kind: "circular_arc_by_radius";
  @minValue(1)
  @maxValue(1000000000000)
  radius_nm: uint64;
  direction: ArcDirection;
  major_arc: boolean;
}

@oneOf
union AuthoredSegment {
  line: AuthoredLineSegment,
  circular_arc: AuthoredCircularArcSegment,
  circular_arc_by_radius: AuthoredCircularArcByRadiusSegment,
}

/** Segment forms supported by constant-width swept centerlines. */
@oneOf
union AuthoredPathSegment {
  line: AuthoredLineSegment,
  circular_arc: AuthoredCircularArcSegment,
}

model PlanarRing {
  ring_id: RingId;
  @minItems(2)
  @maxItems(131072)
  vertices: AuthoredVertex[];
  @minItems(2)
  @maxItems(131072)
  segments: AuthoredSegment[];
}

model PlanarPath {
  path_id: PathId;
  @minItems(2)
  @maxItems(131073)
  vertices: AuthoredVertex[];
  @minItems(1)
  @maxItems(131072)
  segments: AuthoredPathSegment[];
}

model PlanarRegionOperand {
  operand_id: OperandId;
  kind: "planar_region";
  region_id: RegionId;
  outer: PlanarRing;
  @maxItems(131071)
  holes: PlanarRing[];
}

model DiskOperand {
  operand_id: OperandId;
  kind: "disk";
  feature_id: FeatureId;
  center: PointNm;
  @minValue(1)
  @maxValue(1000000000000)
  radius_nm: uint64;
}

model AnnulusOperand {
  operand_id: OperandId;
  kind: "annulus";
  feature_id: FeatureId;
  center: PointNm;
  @minValue(1)
  @maxValue(1000000000000)
  inner_radius_nm: uint64;
  @minValue(1)
  @maxValue(1000000000000)
  outer_radius_nm: uint64;
}

model CapsuleOperand {
  operand_id: OperandId;
  kind: "capsule";
  feature_id: FeatureId;
  start: PointNm;
  end: PointNm;
  @minValue(1)
  @maxValue(1000000000000)
  width_nm: uint64;
}

model SweptPathOperand {
  operand_id: OperandId;
  kind: "swept_path";
  feature_id: FeatureId;
  centerline: PlanarPath;
  @minValue(1)
  @maxValue(1000000000000)
  width_nm: uint64;
  cap: "round";
  join: "round";
}

@oneOf
union AnalyticPlanarOperand {
  planar_region: PlanarRegionOperand,
  disk: DiskOperand,
  annulus: AnnulusOperand,
  capsule: CapsuleOperand,
  swept_path: SweptPathOperand,
}

model AnalyticPlanarBooleanStage {
  stage_id: StageId;
  operation: StageOperation;
  @maxItems(4194304)
  operands: AnalyticPlanarOperand[];
}

model AnalyticPlanarBooleanJob {
  job_id: JobId;
  @maxItems(1048576)
  stages: AnalyticPlanarBooleanStage[];
}

model PlanarRelationshipQuery {
  query_id: QueryId;
  left_job_id: JobId;
  right_job_id: JobId;
}

/** Logical request projected to the governed packed request attachment. */
@jsonSchema
@id("urn:wavenumber:schema:geometer:geometry.analytic_planar_boolean_batch.request:a0")
@contractIdentity("geometry.analytic_planar_boolean_batch.request.a0")
@extension("x-wn-production-wire", "packed_attachment")
model AnalyticPlanarBooleanBatchRequestA0 {
  @maxItems(65535)
  jobs: AnalyticPlanarBooleanJob[];
  @maxItems(1048576)
  relationship_queries: PlanarRelationshipQuery[];
}

model SourceReference {
  kind: SourceKind;
  role: SourceRole;
  operand_id: OperandId;
  @minValue(1)
  primary_id: uint64;
  secondary_id: uint64;
}

model SourceSet {
  @maxItems(1048576)
  sources: SourceReference[];
}

model ResultVertex {
  vertex_id: ResultVertexId;
  point: PointNm;
  intersection_sources: SourceSet;
}

model ResultLineFragment {
  fragment_id: ResultFragmentId;
  kind: "line";
  start_vertex_id: ResultVertexId;
  end_vertex_id: ResultVertexId;
  coincident_positive_sources: SourceSet;
  surviving_subtraction_sources: SourceSet;
}

model ResultCircularArcFragment {
  fragment_id: ResultFragmentId;
  kind: "circular_arc";
  start_vertex_id: ResultVertexId;
  end_vertex_id: ResultVertexId;
  @minValue(1)
  @maxValue(1000000000000)
  radius_nm: uint64;
  direction: ArcDirection;
  major_arc: boolean;
  coincident_positive_sources: SourceSet;
  surviving_subtraction_sources: SourceSet;
}

@oneOf
union DirectedFragment {
  line: ResultLineFragment,
  circular_arc: ResultCircularArcFragment,
}

model ResultRing {
  ring_id: ResultRingId;
  @minItems(2)
  @maxItems(2097152)
  fragment_ids: ResultFragmentId[];
  parent_ring_id?: ResultRingId;
  depth: uint32;
  hole: boolean;
}

model ResultRegion {
  result_region_id: ResultRegionId;
  outer_ring_id: ResultRingId;
  positive_contributors: SourceSet;
}

model OperandOutcomeEvent {
  operand_id: OperandId;
  kind: OperandOutcomeKind;
  result_ring_ids: ResultRingId[];
  result_region_ids: ResultRegionId[];
  sources: SourceSet;
}

model JobDiagnostic {
  code: JobDiagnosticCode;
  severity: DiagnosticSeverity;
  job_id: JobId;
  stage_id?: StageId;
  operand_id?: OperandId;
  @minValue(1)
  geometry_id?: uint64;
  /** Governed standalone location identity; token zero projects to absence. */
  @extension("x-wn-packed-field", "path_token")
  path_identity?: JobDiagnosticPath;
}

model SuccessfulJobResult {
  job_id: JobId;
  status: "success";
  diagnostics: JobDiagnostic[];
  @maxItems(1048576)
  vertices: ResultVertex[];
  @maxItems(2097152)
  directed_fragments: DirectedFragment[];
  @maxItems(1048576)
  rings: ResultRing[];
  @maxItems(1048576)
  result_regions: ResultRegion[];
  @maxItems(4194304)
  operand_outcomes: OperandOutcomeEvent[];
  /** Derived from the canonical standalone job-result packet; not an encoded record field. */
  @extension("x-wn-derived-field", "standalone_job_result_sha256")
  @pattern("^[0-9a-f]{64}$")
  digest_sha256: string;
}

model FailedJobResult {
  job_id: JobId;
  status: "failure";
  @minItems(1)
  diagnostics: JobDiagnostic[];
  /** Derived from the canonical standalone job-result packet; not an encoded record field. */
  @extension("x-wn-derived-field", "standalone_job_result_sha256")
  @pattern("^[0-9a-f]{64}$")
  digest_sha256: string;
}

@oneOf
union AnalyticPlanarBooleanJobResult {
  success: SuccessfulJobResult,
  failure: FailedJobResult,
}

model RelationshipRegionPair {
  left_result_region_id: ResultRegionId;
  right_result_region_id: ResultRegionId;
  dimension: IntersectionDimension;
  equality: boolean;
  left_contains_right: boolean;
  right_contains_left: boolean;
}

model PlanarRelationshipResult {
  query_id: QueryId;
  status: RelationshipStatus;
  aggregate_dimension: IntersectionDimension;
  pairs: RelationshipRegionPair[];
}

/** Logical result projected from the governed packed result attachment. */
@jsonSchema
@id("urn:wavenumber:schema:geometer:geometry.analytic_planar_boolean_batch.result:a0")
@contractIdentity("geometry.analytic_planar_boolean_batch.result.a0")
@extension("x-wn-production-wire", "packed_attachment")
@extension("x-wn-max-logical-source-reference-expansions", 1048576)
model AnalyticPlanarBooleanBatchResultA0 {
  job_results: AnalyticPlanarBooleanJobResult[];
  relationship_results: PlanarRelationshipResult[];
}
