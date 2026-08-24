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

/** A circular arc selected exactly by its topology-owned endpoints, radius, direction, and branch. */
export interface AuthoredCircularArcByRadiusSegment {
  readonly segment_id: SegmentId;
  readonly curve_id: CurveId;
  readonly kind: "circular_arc_by_radius";
  readonly radius_nm: bigint;
  readonly direction: ArcDirection;
  readonly major_arc: boolean;
}

export type AuthoredSegment =
  | AuthoredLineSegment
  | AuthoredCircularArcSegment
  | AuthoredCircularArcByRadiusSegment;

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

/** Segment forms supported by constant-width swept centerlines. */
export type AuthoredPathSegment = AuthoredLineSegment | AuthoredCircularArcSegment;

export interface PlanarPath {
  readonly path_id: PathId;
  readonly vertices: readonly AuthoredVertex[];
  readonly segments: readonly AuthoredPathSegment[];
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

export interface StepTopologyOpenRequestA0 {
  readonly schema: "geometry.step_topology.open.request.a0";
}

export interface SessionReference {
  readonly session_handle: string;
  readonly generation: number;
}

export interface StepTopologyCloseRequestA0 {
  readonly schema: "geometry.step_topology.close.request.a0";
  readonly session: SessionReference;
}

export interface PageRequest {
  readonly cursor?: string;
  readonly limit: number;
}

export interface StepTopologyInspectRequestA0 {
  readonly schema: "geometry.step_topology.inspect.request.a0";
  readonly session: SessionReference;
  readonly page: PageRequest;
  readonly include_source_entity_evidence: boolean;
  readonly include_diagnostics: boolean;
}

export interface TessellationOptions {
  readonly linear_deflection_mm: number;
  readonly angular_deflection_rad: number;
  readonly relative: boolean;
  readonly parallel: boolean;
  readonly source_to_render: readonly number[];
}

export interface StepTopologyRenderRequestA0 {
  readonly schema: "geometry.step_topology.render.request.a0";
  readonly session: SessionReference;
  readonly tessellation: TessellationOptions;
}

export interface StepTopologyResolveHitRequestA0 {
  readonly schema: "geometry.step_topology.resolve_hit.request.a0";
  readonly session: SessionReference;
  readonly artifact_handle: string;
  readonly content_sha256: string;
  readonly instance_index: number;
  readonly primitive_index: number;
  readonly primitive_triangle_index: number;
  readonly occurrence_handle: string;
  readonly body_handle: string;
  readonly face_handle: string;
}

export interface CreateLogicalGroupCommand {
  readonly kind: "create";
  readonly authored_id: string;
  readonly name: string;
  readonly member_handles: readonly string[];
}

export interface RenameLogicalGroupCommand {
  readonly kind: "rename";
  readonly authored_id: string;
  readonly expected_revision: number;
  readonly name: string;
}

export interface ReplaceLogicalGroupMembersCommand {
  readonly kind: "replace_members";
  readonly authored_id: string;
  readonly expected_revision: number;
  readonly member_handles: readonly string[];
}

export interface EraseLogicalGroupCommand {
  readonly kind: "erase";
  readonly authored_id: string;
  readonly expected_revision: number;
}

export type LogicalGroupCommand =
  | CreateLogicalGroupCommand
  | RenameLogicalGroupCommand
  | ReplaceLogicalGroupMembersCommand
  | EraseLogicalGroupCommand;

export interface StepTopologyApplyLogicalGroupsRequestA0 {
  readonly schema: "geometry.step_topology.apply_logical_groups.request.a0";
  readonly session: SessionReference;
  readonly commands: readonly LogicalGroupCommand[];
}

export interface DocumentProbeTarget {
  readonly kind: "document";
}

export interface DefinitionProbeTarget {
  readonly kind: "definition";
  readonly target_handle: string;
}

export interface RootOccurrenceProbeTarget {
  readonly kind: "root_occurrence";
  readonly target_handle: string;
}

export interface ComponentOccurrenceProbeTarget {
  readonly kind: "occurrence";
  readonly target_handle: string;
}

export interface BodyProbeTarget {
  readonly kind: "body";
  readonly target_handle: string;
}

export interface FaceProbeTarget {
  readonly kind: "face";
  readonly target_handle: string;
}

export interface LogicalGroupProbeTarget {
  readonly kind: "logical_group";
  readonly group_authored_id: string;
}

export type MetadataProbeTarget =
  | DocumentProbeTarget
  | DefinitionProbeTarget
  | RootOccurrenceProbeTarget
  | ComponentOccurrenceProbeTarget
  | BodyProbeTarget
  | FaceProbeTarget
  | LogicalGroupProbeTarget;

export interface AttachMetadataProbeCommand {
  readonly kind: "attach";
  readonly authored_id: string;
  readonly target: MetadataProbeTarget;
  readonly key: string;
  readonly value: string;
}

export interface ReplaceMetadataProbeCommand {
  readonly kind: "replace";
  readonly authored_id: string;
  readonly expected_revision: number;
  readonly target: MetadataProbeTarget;
  readonly key: string;
  readonly value: string;
}

export interface EraseMetadataProbeCommand {
  readonly kind: "erase";
  readonly authored_id: string;
  readonly expected_revision: number;
}

export type MetadataProbeCommand =
  | AttachMetadataProbeCommand
  | ReplaceMetadataProbeCommand
  | EraseMetadataProbeCommand;

export interface StepTopologyApplyMetadataProbesRequestA0 {
  readonly schema: "geometry.step_topology.apply_metadata_probes.request.a0";
  readonly session: SessionReference;
  readonly commands: readonly MetadataProbeCommand[];
}

export interface StepTopologyCheckpointEditJournalRequestA0 {
  readonly schema: "geometry.step_topology.checkpoint_edit_journal.request.a0";
  readonly session: SessionReference;
}

export type HierarchySourceKind = "definition" | "body";

export interface CreateHierarchyProductCommand {
  readonly kind: "create_product";
  readonly authored_id: string;
  readonly name: string;
  readonly source_kind: HierarchySourceKind;
  readonly source_handle: string;
}

export interface CreateHierarchyAssemblyCommand {
  readonly kind: "create_assembly";
  readonly authored_id: string;
  readonly name: string;
}

export interface CreateHierarchyOccurrenceCommand {
  readonly kind: "create_occurrence";
  readonly authored_id: string;
  readonly child_authored_id: string;
  readonly parent_assembly_authored_id: string;
  readonly transform: readonly number[];
}

export interface ReparentHierarchyOccurrenceCommand {
  readonly kind: "reparent_occurrence";
  readonly authored_id: string;
  readonly expected_revision: number;
  readonly parent_assembly_authored_id: string;
  readonly transform: readonly number[];
}

export interface RenameHierarchyNodeCommand {
  readonly kind: "rename_node";
  readonly authored_id: string;
  readonly expected_revision: number;
  readonly name: string;
}

export interface EraseHierarchyOccurrenceCommand {
  readonly kind: "erase_occurrence";
  readonly authored_id: string;
  readonly expected_revision: number;
}

export interface EraseHierarchyNodeCommand {
  readonly kind: "erase_node";
  readonly authored_id: string;
  readonly expected_revision: number;
}

export type HierarchyCommand =
  | CreateHierarchyProductCommand
  | CreateHierarchyAssemblyCommand
  | CreateHierarchyOccurrenceCommand
  | ReparentHierarchyOccurrenceCommand
  | RenameHierarchyNodeCommand
  | EraseHierarchyOccurrenceCommand
  | EraseHierarchyNodeCommand;

export interface StepTopologyApplyHierarchyRequestA0 {
  readonly schema: "geometry.step_topology.apply_hierarchy.request.a0";
  readonly session: SessionReference;
  readonly expected_hierarchy_revision: number;
  readonly commands: readonly HierarchyCommand[];
}

export type SaveCarrier = "xbf" | "xml_xcaf" | "step_ap242" | "json_sidecar";

export interface StepTopologySaveRequestA0 {
  readonly schema: "geometry.step_topology.save.request.a0";
  readonly session: SessionReference;
  readonly carrier: SaveCarrier;
  readonly include_diagnostics: boolean;
}

export interface SourceDescriptor {
  readonly format: "step";
  readonly sha256: string;
  readonly bytes: number;
  readonly normalized_length_unit: "millimeter";
}

export interface XbfPersistenceArtifact {
  readonly carrier: "xbf";
  readonly name: "state_artifact";
  readonly media_type: "application/vnd.opencascade.xbf";
  readonly format: "ocaf-xbf-version-12";
  readonly bytes: number;
  readonly sha256: string;
}

export interface XmlXcafPersistenceArtifact {
  readonly carrier: "xml_xcaf";
  readonly name: "state_artifact";
  readonly media_type: "application/vnd.opencascade.xml-xcaf";
  readonly format: "ocaf-xml-xcaf-version-12";
  readonly bytes: number;
  readonly sha256: string;
}

export interface StepAp242PersistenceArtifact {
  readonly carrier: "step_ap242";
  readonly name: "state_artifact";
  readonly media_type: "application/step";
  readonly format: "ap242-managed-model-based-3d-engineering";
  readonly bytes: number;
  readonly sha256: string;
}

export interface JsonSidecarPersistenceArtifact {
  readonly carrier: "json_sidecar";
  readonly name: "state_artifact";
  readonly media_type: "application/vnd.wavenumber.geometer.step-topology-sidecar+json";
  readonly format: "geometer.step_topology_sidecar.a0";
  readonly bytes: number;
  readonly sha256: string;
}

export interface EditJournalPersistenceArtifact {
  readonly carrier: "edit_journal";
  readonly name: "state_artifact";
  readonly media_type: "application/vnd.wavenumber.geometer.step-topology-edit-journal";
  readonly format: "geometer.step_topology_edit_journal.a0";
  readonly bytes: number;
  readonly sha256: string;
}

export type RestoreStateArtifact =
  | XbfPersistenceArtifact
  | XmlXcafPersistenceArtifact
  | StepAp242PersistenceArtifact
  | JsonSidecarPersistenceArtifact
  | EditJournalPersistenceArtifact;

export interface EditJournalReplayPreconditions {
  readonly source_sha256: string;
  readonly source_brep_sha256: string;
  readonly target_inventory_sha256: string;
  readonly occt_version: string;
  readonly transaction_count: number;
}

export interface StepTopologyRestoreRequestA0 {
  readonly schema: "geometry.step_topology.restore.request.a0";
  readonly source: SourceDescriptor;
  readonly state_artifact: RestoreStateArtifact;
  readonly replay_preconditions?: EditJournalReplayPreconditions;
  readonly include_diagnostics: boolean;
}

export interface RecoveryProvenance {
  readonly source_artifact_sha256: string;
  readonly candidate_artifact_sha256: string;
  readonly source_occt_version: string;
  readonly candidate_occt_version: string;
  readonly source_driver: string;
  readonly candidate_driver: string;
  readonly source_writer_settings: string;
  readonly candidate_writer_settings: string;
  readonly command_provenance: string;
  readonly measured_wall_time_milliseconds: number;
}

export interface RecoveryTolerances {
  readonly length_mm: number;
  readonly area_mm2: number;
  readonly volume_mm3: number;
}

export type LogicalGroupMemberKind = "body" | "face";

export interface RecoveryFingerprint {
  readonly normalized_length_unit: "millimeter";
  readonly coordinate_frame: string;
  readonly occurrence_context: string;
  readonly geometry_kind: string;
  readonly area_mm2: number;
  readonly volume_mm3: number;
  readonly centroid_mm: readonly number[];
  readonly bounds_mm: readonly number[];
  readonly adjacency_sha256: string;
}

export type RecoveryLineage = "none" | "split_from_source" | "merged_from_sources";

export interface RecoveryCandidate {
  readonly target_handle: string;
  readonly kind: LogicalGroupMemberKind;
  readonly authored_target_id?: string;
  readonly topology_link_verified: boolean;
  readonly carrier_locator: string;
  readonly carrier_locator_validated: boolean;
  readonly carrier_record: string;
  readonly lineage: RecoveryLineage;
  readonly fingerprint?: RecoveryFingerprint;
}

export interface RecoveryMemberRequest {
  readonly member_record_id: string;
  readonly kind: LogicalGroupMemberKind;
  readonly authored_target_id: string;
  readonly carrier_locator: string;
  readonly source_fingerprint?: RecoveryFingerprint;
  readonly candidates: readonly RecoveryCandidate[];
}

export interface RecoveryGroupRequest {
  readonly group_authored_id: string;
  readonly provenance: RecoveryProvenance;
  readonly tolerances: RecoveryTolerances;
  readonly members: readonly RecoveryMemberRequest[];
}

export interface StepTopologyAnalyzeRecoveryRequestA0 {
  readonly schema: "geometry.step_topology.analyze_recovery.request.a0";
  readonly groups: readonly RecoveryGroupRequest[];
}

/** Structurally representable request payloads for executable IPC A0.
A variant is callable only when the negotiated runtime catalog advertises
its operation; structural presence does not imply runtime availability. */
export type IpcRequestValueA0 =
  | ModelBoundsOptionsA0
  | PackedAttachmentProjectionA0
  | StepTopologyOpenRequestA0
  | StepTopologyCloseRequestA0
  | StepTopologyInspectRequestA0
  | StepTopologyRenderRequestA0
  | StepTopologyResolveHitRequestA0
  | StepTopologyApplyLogicalGroupsRequestA0
  | StepTopologyApplyMetadataProbesRequestA0
  | StepTopologyCheckpointEditJournalRequestA0
  | StepTopologyApplyHierarchyRequestA0
  | StepTopologySaveRequestA0
  | StepTopologyRestoreRequestA0
  | StepTopologyAnalyzeRecoveryRequestA0;

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

export interface ToolDescriptor {
  readonly name: "geometer";
  readonly release_version: string;
  readonly occt_version: string;
}

export interface StepTopologyOpenResultA0 {
  readonly schema: "geometry.step_topology.open.result.a0";
  readonly session: SessionReference;
  readonly source: SourceDescriptor;
  readonly tool: ToolDescriptor;
  readonly evicted_session_handles: readonly string[];
}

export interface StepTopologyCloseResultA0 {
  readonly schema: "geometry.step_topology.close.result.a0";
  readonly session_handle: string;
  readonly closed: true;
}

export interface InspectionCounts {
  readonly definitions: number;
  readonly root_occurrences: number;
  readonly component_occurrences: number;
  readonly bodies: number;
  readonly shells: number;
  readonly faces: number;
  readonly memberships: number;
}

export interface SourceEntityEvidence {
  readonly mapped: boolean;
  readonly shape_result_round_trip: boolean;
  readonly model_number?: number;
  readonly entity_type?: string;
  readonly mapping_method?: string;
}

export interface DefinitionSummary {
  readonly handle: string;
  readonly name: string;
  readonly assembly: boolean;
  readonly body_count: number;
  readonly face_count: number;
  readonly source_entity?: SourceEntityEvidence;
}

export interface RootOccurrenceSummary {
  readonly kind: "root";
  readonly handle: string;
  readonly definition_handle: string;
  readonly name: string;
  readonly transform: readonly number[];
}

export interface ComponentOccurrenceSummary {
  readonly kind: "component";
  readonly handle: string;
  readonly definition_handle: string;
  readonly parent_occurrence_handle: string;
  readonly depth: number;
  readonly name: string;
  readonly transform: readonly number[];
}

export type OccurrenceSummary = RootOccurrenceSummary | ComponentOccurrenceSummary;

export interface BodySummary {
  readonly handle: string;
  readonly definition_handle: string;
  readonly topology_kind: string;
  readonly shell_count: number;
  readonly face_count: number;
  readonly bounds_mm: readonly number[];
  readonly volume_mm3: number;
  readonly source_entity?: SourceEntityEvidence;
}

export interface ShellSummary {
  readonly handle: string;
  readonly definition_handle: string;
  readonly body_count: number;
  readonly face_count: number;
  readonly source_entity?: SourceEntityEvidence;
}

export interface FaceSummary {
  readonly handle: string;
  readonly definition_handle: string;
  readonly body_count: number;
  readonly shell_count: number;
  readonly bounds_mm: readonly number[];
  readonly area_mm2: number;
  readonly centroid_mm: readonly number[];
  readonly source_entity?: SourceEntityEvidence;
}

export type TopologyMembershipKind = "body_shell" | "body_face" | "shell_face";

export interface TopologyMembership {
  readonly kind: TopologyMembershipKind;
  readonly owner_handle: string;
  readonly member_handle: string;
}

export interface TopologyPage {
  readonly definitions: readonly DefinitionSummary[];
  readonly occurrences: readonly OccurrenceSummary[];
  readonly bodies: readonly BodySummary[];
  readonly shells: readonly ShellSummary[];
  readonly faces: readonly FaceSummary[];
  readonly memberships: readonly TopologyMembership[];
  readonly next_cursor?: string;
}

export interface TopologyTableAttachmentDescriptor {
  readonly name: "topology_table";
  readonly media_type: "application/vnd.wavenumber.geometer.step-topology-table";
  readonly format: "wn.geometer.step-topology-table.a0";
  readonly bytes: number;
  readonly sha256: string;
}

export interface StepTopologyInspectResultA0 {
  readonly schema: "geometry.step_topology.inspect.result.a0";
  readonly session: SessionReference;
  readonly counts: InspectionCounts;
  readonly page: TopologyPage;
  readonly compact_table?: TopologyTableAttachmentDescriptor;
  readonly diagnostics: readonly DiagnosticA0[];
}

export interface RenderCounts {
  readonly meshes: number;
  readonly instances: number;
  readonly primitives: number;
  readonly geometry_triangles: number;
  readonly instanced_triangles: number;
}

export interface RenderArtifactDescriptor {
  readonly artifact_handle: string;
  readonly content_sha256: string;
  readonly render_artifact_handle: string;
  readonly render_content_sha256: string;
  readonly binding_layout: "node-primitive-a0";
  readonly geometry_length_unit: "meter";
  readonly source_length_unit: "millimeter";
  readonly counts: RenderCounts;
}

export interface GlbAttachmentDescriptor {
  readonly name: "glb";
  readonly media_type: "model/gltf-binary";
  readonly format: "glb-2.0";
  readonly bytes: number;
  readonly sha256: string;
}

export interface TopologyBindingTableAttachmentDescriptor {
  readonly name: "topology_binding_table";
  readonly media_type: "application/vnd.wavenumber.geometer.step-topology-binding-table";
  readonly format: "wn.geometer.step-topology-binding-table.a0";
  readonly bytes: number;
  readonly sha256: string;
}

export interface StepTopologyRenderResultA0 {
  readonly schema: "geometry.step_topology.render.result.a0";
  readonly session: SessionReference;
  readonly artifact: RenderArtifactDescriptor;
  readonly glb: GlbAttachmentDescriptor;
  readonly compact_binding_table?: TopologyBindingTableAttachmentDescriptor;
}

export interface StepTopologyResolveHitResultA0 {
  readonly schema: "geometry.step_topology.resolve_hit.result.a0";
  readonly session: SessionReference;
  readonly instance_index: number;
  readonly primitive_index: number;
  readonly triangle_index: number;
  readonly occurrence_handle: string;
  readonly body_handle: string;
  readonly face_handle: string;
}

export interface MutationSessionState {
  readonly session: SessionReference;
  readonly edit_journal_revision: number;
  readonly accounted_string_bytes: number;
  readonly estimated_resident_bytes: number;
}

export interface LogicalGroupMember {
  readonly kind: LogicalGroupMemberKind;
  readonly target_handle: string;
}

export interface LogicalGroup {
  readonly authored_id: string;
  readonly revision: number;
  readonly name: string;
  readonly members: readonly LogicalGroupMember[];
}

export interface StepTopologyApplyLogicalGroupsResultA0 {
  readonly schema: "geometry.step_topology.apply_logical_groups.result.a0";
  readonly state: MutationSessionState;
  readonly groups: readonly LogicalGroup[];
  readonly diagnostics: readonly DiagnosticA0[];
}

export interface MetadataProbe {
  readonly authored_id: string;
  readonly revision: number;
  readonly target: MetadataProbeTarget;
  readonly key: string;
  readonly value: string;
}

export interface StepTopologyApplyMetadataProbesResultA0 {
  readonly schema: "geometry.step_topology.apply_metadata_probes.result.a0";
  readonly state: MutationSessionState;
  readonly groups: readonly LogicalGroup[];
  readonly probes: readonly MetadataProbe[];
  readonly diagnostics: readonly DiagnosticA0[];
}

export interface EditJournalAttachmentDescriptor {
  readonly name: "edit_journal";
  readonly media_type: "application/vnd.wavenumber.geometer.step-topology-edit-journal";
  readonly format: "geometer.step_topology_edit_journal.a0";
  readonly bytes: number;
  readonly sha256: string;
}

export interface StepTopologyCheckpointEditJournalResultA0 {
  readonly schema: "geometry.step_topology.checkpoint_edit_journal.result.a0";
  readonly state: MutationSessionState;
  readonly source_sha256: string;
  readonly source_brep_sha256: string;
  readonly target_inventory_sha256: string;
  readonly occt_version: string;
  readonly transaction_count: number;
  readonly journal: EditJournalAttachmentDescriptor;
  readonly diagnostics: readonly DiagnosticA0[];
}

export type HierarchyNodeKind = "product" | "assembly";

export interface HierarchyNode {
  readonly authored_id: string;
  readonly revision: number;
  readonly kind: HierarchyNodeKind;
  readonly name: string;
  readonly source_kind?: HierarchySourceKind;
  readonly source_handle?: string;
}

export interface HierarchyOccurrence {
  readonly authored_id: string;
  readonly revision: number;
  readonly child_authored_id: string;
  readonly parent_assembly_authored_id: string;
  readonly transform: readonly number[];
}

export interface HierarchyState {
  readonly hierarchy_revision: number;
  readonly source_brep_sha256: string;
  readonly nodes: readonly HierarchyNode[];
  readonly occurrences: readonly HierarchyOccurrence[];
}

export interface StepTopologyApplyHierarchyResultA0 {
  readonly schema: "geometry.step_topology.apply_hierarchy.result.a0";
  readonly state: MutationSessionState;
  readonly hierarchy: HierarchyState;
  readonly diagnostics: readonly DiagnosticA0[];
}

export type SavePersistenceArtifact =
  | XbfPersistenceArtifact
  | XmlXcafPersistenceArtifact
  | StepAp242PersistenceArtifact
  | JsonSidecarPersistenceArtifact;

export type PersistenceCarrier =
  | "xbf"
  | "xml_xcaf"
  | "step_ap242"
  | "json_sidecar"
  | "edit_journal";

export type CarrierSupportState = "supported" | "experimental" | "unsupported";

export interface CarrierCapabilityNote {
  readonly value: string;
}

export interface CarrierCapability {
  readonly carrier: PersistenceCarrier;
  readonly save: CarrierSupportState;
  readonly restore: CarrierSupportState;
  readonly authored_payload: CarrierSupportState;
  readonly topology_links: CarrierSupportState;
  readonly notes: readonly CarrierCapabilityNote[];
}

export interface StepTopologySaveResultA0 {
  readonly schema: "geometry.step_topology.save.result.a0";
  readonly state: MutationSessionState;
  readonly source_sha256: string;
  readonly artifact: SavePersistenceArtifact;
  readonly capabilities: readonly CarrierCapability[];
  readonly diagnostics: readonly DiagnosticA0[];
}

export type RecoveryResolutionState = "resolved" | "ambiguous" | "unresolved" | "unsupported";

export type RecoveryGroupCompleteness =
  | "fully_recovered"
  | "partially_recovered"
  | "unrecovered"
  | "unsupported";

export type RecoveryResolutionMethod =
  | "authored_id_topology_link"
  | "validated_carrier_locator"
  | "unique_geometry_adjacency_fingerprint"
  | "none";

export type RecoveryTopologyComparison =
  | "unchanged"
  | "relocated"
  | "split"
  | "merged"
  | "otherwise_changed"
  | "not_compared"
  | "unavailable";

export type RecoveryConfidence = "high" | "medium" | "low" | "none";

export interface RecoveryComparedField {
  readonly value: string;
}

export interface RecoveryCarrierRecord {
  readonly value: string;
}

export interface RecoveryRejectedAlternative {
  readonly target_handle: string;
  readonly reason: string;
}

export interface RecoveryEvidence {
  readonly candidate_count: number;
  readonly matching_candidate_count: number;
  readonly compared_fields: readonly RecoveryComparedField[];
  readonly tolerances: RecoveryTolerances;
  readonly carrier_records: readonly RecoveryCarrierRecord[];
  readonly rejected_alternatives: readonly RecoveryRejectedAlternative[];
}

export interface RecoveryMemberResult {
  readonly member_record_id: string;
  readonly kind: LogicalGroupMemberKind;
  readonly authored_target_id: string;
  readonly resolution_state: RecoveryResolutionState;
  readonly resolution_method: RecoveryResolutionMethod;
  readonly topology_comparison: RecoveryTopologyComparison;
  readonly confidence: RecoveryConfidence;
  readonly resolved_target_handle?: string;
  readonly evidence: RecoveryEvidence;
}

export interface RecoveryGroupResult {
  readonly group_authored_id: string;
  readonly provenance: RecoveryProvenance;
  readonly resolution_state: RecoveryResolutionState;
  readonly completeness: RecoveryGroupCompleteness;
  readonly resolved_member_count: number;
  readonly ambiguous_member_count: number;
  readonly unresolved_member_count: number;
  readonly unsupported_member_count: number;
  readonly members: readonly RecoveryMemberResult[];
}

export interface StepTopologyRestoreResultA0 {
  readonly schema: "geometry.step_topology.restore.result.a0";
  readonly session: SessionReference;
  readonly source: SourceDescriptor;
  readonly tool: ToolDescriptor;
  readonly replayed_transaction_count: number;
  readonly evicted_session_handles?: readonly string[];
  readonly recovery: readonly RecoveryGroupResult[];
  readonly diagnostics: readonly DiagnosticA0[];
}

export interface StepTopologyAnalyzeRecoveryResultA0 {
  readonly schema: "geometry.step_topology.analyze_recovery.result.a0";
  readonly groups: readonly RecoveryGroupResult[];
  readonly diagnostics: readonly DiagnosticA0[];
}

/** Structurally representable operation results. A result variant may belong
to a runtime-unavailable experimental operation and is not an availability
claim; the negotiated operation catalog remains authoritative. */
export type OperationResultValueA0 =
  | ModelBoundsResultA0
  | PackedAttachmentProjectionA0
  | StepTopologyOpenResultA0
  | StepTopologyCloseResultA0
  | StepTopologyInspectResultA0
  | StepTopologyRenderResultA0
  | StepTopologyResolveHitResultA0
  | StepTopologyApplyLogicalGroupsResultA0
  | StepTopologyApplyMetadataProbesResultA0
  | StepTopologyCheckpointEditJournalResultA0
  | StepTopologyApplyHierarchyResultA0
  | StepTopologySaveResultA0
  | StepTopologyRestoreResultA0
  | StepTopologyAnalyzeRecoveryResultA0;

/** A completed operation with its operation-specific result. */
export interface OperationSuccessA0 {
  readonly operation: string;
  readonly ok: true;
  readonly result: OperationResultValueA0;
}

/** Transport-neutral typed outcome shared by the generic C ABI and executable IPC. */
export type OperationOutcomeA0 = OperationSuccessA0 | OperationFailureA0;
