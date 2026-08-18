// Generated from wn_geometer_contract_catalog.a0.json. Do not edit.

export type JobId = bigint;

export type StageId = bigint;

export type StageOperation = "union" | "difference";

export type OperandId = bigint;

export type RegionId = bigint;

export type RingId = bigint;

export type VertexId = bigint;

export interface PointNm {
  readonly x: bigint;
  readonly y: bigint;
}

export interface AuthoredVertex {
  readonly vertex_id: VertexId;
  readonly point: PointNm;
}

export type SegmentId = bigint;

export type CurveId = bigint;

export interface AuthoredLineSegment {
  readonly segment_id: SegmentId;
  readonly curve_id: CurveId;
  readonly kind: "line";
}

export type ArcDirection = "ccw" | "cw";

export interface AuthoredCircularArcSegment {
  readonly segment_id: SegmentId;
  readonly curve_id: CurveId;
  readonly kind: "circular_arc";
  readonly center: PointNm;
  readonly direction: ArcDirection;
  readonly major_arc: boolean;
}

export type AuthoredSegment = AuthoredLineSegment | AuthoredCircularArcSegment;

export interface PlanarRing {
  readonly ring_id: RingId;
  readonly vertices: readonly AuthoredVertex[];
  readonly segments: readonly AuthoredSegment[];
}

export interface PlanarRegionOperand {
  readonly operand_id: OperandId;
  readonly kind: "planar_region";
  readonly region_id: RegionId;
  readonly outer: PlanarRing;
  readonly holes: readonly PlanarRing[];
}

export type FeatureId = bigint;

export interface DiskOperand {
  readonly operand_id: OperandId;
  readonly kind: "disk";
  readonly feature_id: FeatureId;
  readonly center: PointNm;
  readonly radius_nm: bigint;
}

export interface AnnulusOperand {
  readonly operand_id: OperandId;
  readonly kind: "annulus";
  readonly feature_id: FeatureId;
  readonly center: PointNm;
  readonly inner_radius_nm: bigint;
  readonly outer_radius_nm: bigint;
}

export interface CapsuleOperand {
  readonly operand_id: OperandId;
  readonly kind: "capsule";
  readonly feature_id: FeatureId;
  readonly start: PointNm;
  readonly end: PointNm;
  readonly width_nm: bigint;
}

export type PathId = bigint;

export interface PlanarPath {
  readonly path_id: PathId;
  readonly vertices: readonly AuthoredVertex[];
  readonly segments: readonly AuthoredSegment[];
}

export interface SweptPathOperand {
  readonly operand_id: OperandId;
  readonly kind: "swept_path";
  readonly feature_id: FeatureId;
  readonly centerline: PlanarPath;
  readonly width_nm: bigint;
  readonly cap: "round";
  readonly join: "round";
}

export type AnalyticPlanarOperand =
  | PlanarRegionOperand
  | DiskOperand
  | AnnulusOperand
  | CapsuleOperand
  | SweptPathOperand;

export interface AnalyticPlanarBooleanStage {
  readonly stage_id: StageId;
  readonly operation: StageOperation;
  readonly operands: readonly AnalyticPlanarOperand[];
}

export interface AnalyticPlanarBooleanJob {
  readonly job_id: JobId;
  readonly stages: readonly AnalyticPlanarBooleanStage[];
}

export type QueryId = bigint;

export interface PlanarRelationshipQuery {
  readonly query_id: QueryId;
  readonly left_job_id: JobId;
  readonly right_job_id: JobId;
}

/** Logical request projected to the governed packed request attachment. */
export interface AnalyticPlanarBooleanBatchRequestA0 {
  readonly jobs: readonly AnalyticPlanarBooleanJob[];
  readonly relationship_queries: readonly PlanarRelationshipQuery[];
}

export type JobDiagnosticCode =
  | "geometer.operation.analytic_planar_boolean.invalid_topology"
  | "geometer.operation.analytic_planar_boolean.invalid_arc"
  | "geometer.operation.analytic_planar_boolean.unsupported_geometry"
  | "geometer.operation.analytic_planar_boolean.normalization_error_exceeded"
  | "geometer.operation.analytic_planar_boolean.normalization_topology_collapse"
  | "geometer.operation.analytic_planar_boolean.nonanalytic_result"
  | "geometer.operation.analytic_planar_boolean.solver_failed"
  | "geometer.operation.analytic_planar_boolean.resource_limit_exceeded";

export type DiagnosticSeverity = "error" | "warning";

/** Standalone logical identities projected one-to-one from nonzero packed path tokens. */
export type JobDiagnosticPath =
  | "request_jobs"
  | "job_id"
  | "job_stages"
  | "stage_id"
  | "stage_operation"
  | "stage_operands"
  | "operand_id"
  | "operand_geometry"
  | "region_outer"
  | "region_holes"
  | "ring_vertices"
  | "ring_segments"
  | "path_vertices"
  | "path_segments"
  | "segment_curve"
  | "disk_radius"
  | "annulus_inner_radius"
  | "annulus_outer_radius"
  | "capsule_start"
  | "capsule_end"
  | "capsule_width"
  | "swept_path_centerline"
  | "swept_path_width"
  | "relationship_queries"
  | "relationship_left_job_id"
  | "relationship_right_job_id";

export interface JobDiagnostic {
  readonly code: JobDiagnosticCode;
  readonly severity: DiagnosticSeverity;
  readonly job_id: JobId;
  readonly stage_id?: StageId;
  readonly operand_id?: OperandId;
  readonly geometry_id?: bigint;
  /** Governed standalone location identity; token zero projects to absence. */
  readonly path_identity?: JobDiagnosticPath;
}

export type ResultVertexId = bigint;

export type SourceKind =
  | "authored_segment_curve"
  | "compact_feature_role"
  | "subtractive_operand_effect";

export type SourceRole =
  | "none"
  | "authored_line"
  | "authored_circular_arc"
  | "primitive_outer_circle"
  | "primitive_inner_circle"
  | "capsule_left_line"
  | "capsule_end_cap"
  | "capsule_right_line"
  | "capsule_start_cap"
  | "swept_left_offset_line"
  | "swept_left_offset_arc"
  | "swept_right_offset_line"
  | "swept_right_offset_arc"
  | "swept_round_join"
  | "swept_start_cap"
  | "swept_end_cap";

export interface SourceReference {
  readonly kind: SourceKind;
  readonly role: SourceRole;
  readonly operand_id: OperandId;
  readonly primary_id: bigint;
  readonly secondary_id: bigint;
}

export interface SourceSet {
  readonly sources: readonly SourceReference[];
}

export interface ResultVertex {
  readonly vertex_id: ResultVertexId;
  readonly point: PointNm;
  readonly intersection_sources: SourceSet;
}

export type ResultFragmentId = bigint;

export interface ResultLineFragment {
  readonly fragment_id: ResultFragmentId;
  readonly kind: "line";
  readonly start_vertex_id: ResultVertexId;
  readonly end_vertex_id: ResultVertexId;
  readonly coincident_positive_sources: SourceSet;
  readonly surviving_subtraction_sources: SourceSet;
}

export interface ResultCircularArcFragment {
  readonly fragment_id: ResultFragmentId;
  readonly kind: "circular_arc";
  readonly start_vertex_id: ResultVertexId;
  readonly end_vertex_id: ResultVertexId;
  readonly radius_nm: bigint;
  readonly direction: ArcDirection;
  readonly major_arc: boolean;
  readonly coincident_positive_sources: SourceSet;
  readonly surviving_subtraction_sources: SourceSet;
}

export type DirectedFragment = ResultLineFragment | ResultCircularArcFragment;

export type ResultRingId = bigint;

export interface ResultRing {
  readonly ring_id: ResultRingId;
  readonly fragment_ids: readonly ResultFragmentId[];
  readonly parent_ring_id?: ResultRingId;
  readonly depth: number;
  readonly hole: boolean;
}

export type ResultRegionId = bigint;

export interface ResultRegion {
  readonly result_region_id: ResultRegionId;
  readonly outer_ring_id: ResultRingId;
  readonly positive_contributors: SourceSet;
}

export type OperandOutcomeKind =
  | "contributes_final_material"
  | "redundant_or_absorbed_coverage"
  | "partially_removed_later"
  | "completely_removed_later"
  | "subtraction_effect_survives"
  | "subtraction_effect_overwritten_later"
  | "no_effect";

export interface OperandOutcomeEvent {
  readonly operand_id: OperandId;
  readonly kind: OperandOutcomeKind;
  readonly result_ring_ids: readonly ResultRingId[];
  readonly result_region_ids: readonly ResultRegionId[];
  readonly sources: SourceSet;
}

export interface SuccessfulJobResult {
  readonly job_id: JobId;
  readonly status: "success";
  readonly diagnostics: readonly JobDiagnostic[];
  readonly vertices: readonly ResultVertex[];
  readonly directed_fragments: readonly DirectedFragment[];
  readonly rings: readonly ResultRing[];
  readonly result_regions: readonly ResultRegion[];
  readonly operand_outcomes: readonly OperandOutcomeEvent[];
  /** Derived from the canonical standalone job-result packet; not an encoded record field. */
  readonly digest_sha256: string;
}

export interface FailedJobResult {
  readonly job_id: JobId;
  readonly status: "failure";
  readonly diagnostics: readonly JobDiagnostic[];
  /** Derived from the canonical standalone job-result packet; not an encoded record field. */
  readonly digest_sha256: string;
}

export type AnalyticPlanarBooleanJobResult = SuccessfulJobResult | FailedJobResult;

export type RelationshipStatus = "success" | "skipped_dependency_failed";

export type IntersectionDimension = "disjoint" | "point" | "curve" | "area";

export interface RelationshipRegionPair {
  readonly left_result_region_id: ResultRegionId;
  readonly right_result_region_id: ResultRegionId;
  readonly dimension: IntersectionDimension;
  readonly equality: boolean;
  readonly left_contains_right: boolean;
  readonly right_contains_left: boolean;
}

export interface PlanarRelationshipResult {
  readonly query_id: QueryId;
  readonly status: RelationshipStatus;
  readonly aggregate_dimension: IntersectionDimension;
  readonly pairs: readonly RelationshipRegionPair[];
}

/** Logical result projected from the governed packed result attachment. */
export interface AnalyticPlanarBooleanBatchResultA0 {
  readonly job_results: readonly AnalyticPlanarBooleanJobResult[];
  readonly relationship_results: readonly PlanarRelationshipResult[];
}

/** Stable diagnostic category with distinct transport, contract, and operation domains. */
export type DiagnosticCategory = "transport" | "contract" | "operation";

/** A strict cross-transport diagnostic; message text is informative rather than identity. */
export interface DiagnosticA0 {
  /** Stable namespaced diagnostic identity. */
  readonly code: string;
  readonly category: DiagnosticCategory;
  /** Human-readable non-normative detail. */
  readonly message: string;
  readonly retryable: boolean;
  /** RFC 6901 JSON Pointer when a document location is meaningful. */
  readonly path?: string;
  /** Stable operation identity when the diagnostic is operation-scoped. */
  readonly operation?: string;
  /** Transport correlation identifier when available. */
  readonly request_id?: string;
}

/** A named binary attachment carrying a separately governed packed projection. */
export interface PackedAttachmentReferenceA0 {
  readonly attachment: string;
  readonly format: string;
}

/** Minimal JSON value used while an operation's logical DTO projection remains deferred. */
export interface PackedAttachmentProjectionA0 {
  readonly schema: string;
  readonly packet: PackedAttachmentReferenceA0;
}

/** Named raw-attachment declaration in the negotiated operation catalog. */
export interface IpcAttachmentDeclarationA0 {
  readonly name: string;
  readonly required: boolean;
  readonly media_types: readonly string[];
  readonly max_bytes: number;
}

/** wasm32 attachment-descriptor member offsets. */
export interface IpcAttachmentOffsetsWasm32A0 {
  readonly struct_size: number;
  readonly flags: number;
  readonly name: number;
  readonly name_size: number;
  readonly media_type: number;
  readonly media_type_size: number;
  readonly data: number;
  readonly data_size: number;
  readonly reserved0: number;
}

export interface IpcAttachmentLayoutWasm32A0 {
  readonly size: number;
  readonly offsets: IpcAttachmentOffsetsWasm32A0;
}

/** 64-bit attachment-descriptor member offsets. */
export interface IpcAttachmentOffsetsPointer64A0 {
  readonly struct_size: number;
  readonly flags: number;
  readonly name: number;
  readonly name_size: number;
  readonly media_type: number;
  readonly media_type_size: number;
  readonly data: number;
  readonly data_size: number;
  readonly reserved0: number;
}

export interface IpcAttachmentLayoutPointer64A0 {
  readonly size: number;
  readonly offsets: IpcAttachmentOffsetsPointer64A0;
}

export interface IpcAttachmentDescriptorA0 {
  readonly wasm32: IpcAttachmentLayoutWasm32A0;
  readonly pointer64: IpcAttachmentLayoutPointer64A0;
}

/** Successful queue-only cancellation outcome. */
export interface IpcCancelledA0 {
  readonly status: "cancelled";
}

/** Nonterminal cancellation rejection for an active or unknown request. */
export interface IpcCancelRejectedA0 {
  readonly status: "rejected";
  readonly diagnostic: DiagnosticA0;
}

/** Effective executable IPC A0 limits selected by the server. */
export interface IpcEffectiveLimitsA0 {
  readonly json_bytes: number;
  readonly attachment_count: number;
  readonly attachment_name_bytes: number;
  readonly attachment_media_type_bytes: number;
  readonly attachment_bytes: number;
  readonly frame_bytes: number;
  readonly queued_requests: number;
  readonly queued_bytes: number;
  readonly resident_request_bytes: number;
  readonly pending_writer_bytes: number;
}

/** Bounds shared by the generic C ABI operation catalog. */
export interface IpcGenericAbiLimitsA0 {
  readonly operation_id_bytes: number;
  readonly request_json_bytes: number;
  readonly response_json_bytes: number;
  readonly attachment_count: number;
  readonly attachment_name_bytes: number;
  readonly attachment_media_type_bytes: number;
  readonly attachment_bytes: number;
  readonly aggregate_attachment_bytes_native: number;
  readonly aggregate_attachment_bytes_wasm: number;
}

/** Client handshake for executable IPC A0. */
export interface IpcHelloA0 {
  readonly client_name: string;
  readonly client_version: string;
  readonly protocols: readonly string[];
  readonly capabilities?: readonly string[];
}

export type IpcRuntimeDispatchA0 = "logical_dto" | "packed_attachment";

export interface IpcPackedProjectionA0 {
  readonly kind: "packed_attachment";
  readonly attachment_name: string;
  readonly format: string;
}

/** One operation exposed by the negotiated generic transport. */
export interface IpcOperationDeclarationA0 {
  readonly identity: string;
  readonly request_contract: string;
  readonly result_contract: string;
  readonly runtime_dispatch: IpcRuntimeDispatchA0;
  readonly input_attachments: readonly IpcAttachmentDeclarationA0[];
  readonly output_attachments: readonly IpcAttachmentDeclarationA0[];
  readonly request_projection?: IpcPackedProjectionA0;
  readonly result_projection?: IpcPackedProjectionA0;
}

/** Exact operation catalog embedded into the welcome frame. */
export interface IpcOperationCatalogA0 {
  readonly catalog: "wn.geometer.operation_catalog.a0";
  readonly generic_abi: "a0";
  readonly release_version: string;
  readonly c_abi_generation: number;
  readonly operations: readonly IpcOperationDeclarationA0[];
  readonly attachment_descriptor: IpcAttachmentDescriptorA0;
  readonly limits: IpcGenericAbiLimitsA0;
}

/** Fatal connection-level protocol diagnostic. */
export interface IpcProtocolErrorA0 {
  readonly status: "protocol_error";
  readonly diagnostic: DiagnosticA0;
}

/** Optional human-readable reason used by cancel and shutdown controls. */
export interface IpcReasonA0 {
  readonly reason?: string;
}

/** Canonical model source format. Compatibility readers may additionally accept STEP. */
export type ModelFormat = "step";

/** Row-major affine 4x4 matrix. Semantic validation requires final row [0,0,0,1]. */
export type Matrix4x4 = readonly [
  number,
  number,
  number,
  number,
  number,
  number,
  number,
  number,
  number,
  number,
  number,
  number,
  number,
  number,
  number,
  number,
];

/** Presence-preserving patch applied over focused C++ defaults. */
export interface ModelBoundsOptionsA0 {
  /** Absent preserves the inherited value; canonical default intent is step. */
  readonly format?: ModelFormat;
  /** Absent preserves the inherited transform; canonical default intent is identity. */
  readonly model_transform?: Matrix4x4;
}

/** Strict generic request envelope for executable IPC A0. */
export type IpcRequestValueA0 = ModelBoundsOptionsA0 | PackedAttachmentProjectionA0;

export interface IpcRequestA0 {
  readonly operation: string;
  readonly request: IpcRequestValueA0;
}

/** Final acknowledgment after every accepted request is terminal and flushed. */
export interface IpcShutdownAckA0 {
  readonly status: "complete";
  readonly activeRequestCompleted: boolean;
  readonly rejectedQueuedRequestCount: number;
}

/** Server handshake selecting executable IPC A0 and its effective limits. */
export interface IpcWelcomeA0 {
  readonly release_version: string;
  readonly c_abi_generation: number;
  readonly ipc: "a0";
  readonly catalog_sha256: string;
  readonly operation_catalog: IpcOperationCatalogA0;
  readonly limits: IpcEffectiveLimitsA0;
  readonly capabilities: readonly string[];
}

/** Source identity included in a successful model-bounds result. */
export interface ModelBoundsSource {
  readonly format: ModelFormat;
  readonly hash: string;
}

/** Exactly three finite millimeter coordinates. */
export type Vector3 = readonly [number, number, number];

/** Axis-aligned bounds in millimeters. */
export interface ModelBoundsValues {
  readonly min: Vector3;
  readonly max: Vector3;
  readonly size: Vector3;
  readonly center: Vector3;
}

/** Nondeterministic timings excluded only by explicit conformance projection. */
export interface ModelBoundsTimings {
  readonly model_read_ms: number;
  readonly bounds_ms: number;
}

/** Successful model-bounds result matching the compatible public JSON shape. */
export interface ModelBoundsResultA0 {
  readonly schema: "geometry.model_bounds.a0";
  readonly units: "mm";
  readonly source: ModelBoundsSource;
  readonly bounds: ModelBoundsValues;
  readonly timings: ModelBoundsTimings;
}

/** A rejected or failed operation with governed diagnostics. */
export interface OperationFailureA0 {
  readonly operation: string;
  readonly ok: false;
  readonly diagnostics: readonly DiagnosticA0[];
}

/** Results currently available through the generic operation transport. */
export type OperationResultValueA0 = ModelBoundsResultA0 | PackedAttachmentProjectionA0;

/** A completed operation with its operation-specific result. */
export interface OperationSuccessA0 {
  readonly operation: string;
  readonly ok: true;
  readonly result: OperationResultValueA0;
}

/** Transport-neutral typed outcome shared by the generic C ABI and executable IPC. */
export type OperationOutcomeA0 = OperationSuccessA0 | OperationFailureA0;
