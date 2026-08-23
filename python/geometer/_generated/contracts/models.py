# Generated from wn_geometer_contract_catalog.a0.json. Do not edit.

from dataclasses import dataclass
from enum import Enum
from typing import Literal, TypeAlias

NORMALIZED_CATALOG_SHA256 = "6cacbf010b3ae7a2af74c68517e5bcd68e3f4da11c904fb2626debf0ba8d17e2"

JobId: TypeAlias = int

StageId: TypeAlias = int


class StageOperation(str, Enum):
    UNION_STAGE = "union"
    DIFFERENCE = "difference"


OperandId: TypeAlias = int

RegionId: TypeAlias = int

RingId: TypeAlias = int

VertexId: TypeAlias = int


@dataclass(frozen=True, slots=True, kw_only=True)
class PointNm:
    x: int
    y: int


@dataclass(frozen=True, slots=True, kw_only=True)
class AuthoredVertex:
    vertex_id: VertexId
    point: PointNm


SegmentId: TypeAlias = int

CurveId: TypeAlias = int


@dataclass(frozen=True, slots=True, kw_only=True)
class AuthoredLineSegment:
    segment_id: SegmentId
    curve_id: CurveId
    kind: Literal["line"]


class ArcDirection(str, Enum):
    CCW = "ccw"
    CW = "cw"


@dataclass(frozen=True, slots=True, kw_only=True)
class AuthoredCircularArcSegment:
    segment_id: SegmentId
    curve_id: CurveId
    kind: Literal["circular_arc"]
    center: PointNm
    direction: ArcDirection
    major_arc: bool


# A circular arc selected exactly by its topology-owned endpoints, radius, direction, and branch.
@dataclass(frozen=True, slots=True, kw_only=True)
class AuthoredCircularArcByRadiusSegment:
    segment_id: SegmentId
    curve_id: CurveId
    kind: Literal["circular_arc_by_radius"]
    radius_nm: int
    direction: ArcDirection
    major_arc: bool


AuthoredSegment: TypeAlias = AuthoredLineSegment | AuthoredCircularArcSegment | AuthoredCircularArcByRadiusSegment


@dataclass(frozen=True, slots=True, kw_only=True)
class PlanarRing:
    ring_id: RingId
    vertices: tuple[AuthoredVertex, ...]
    segments: tuple[AuthoredSegment, ...]


@dataclass(frozen=True, slots=True, kw_only=True)
class PlanarRegionOperand:
    operand_id: OperandId
    kind: Literal["planar_region"]
    region_id: RegionId
    outer: PlanarRing
    holes: tuple[PlanarRing, ...]


FeatureId: TypeAlias = int


@dataclass(frozen=True, slots=True, kw_only=True)
class DiskOperand:
    operand_id: OperandId
    kind: Literal["disk"]
    feature_id: FeatureId
    center: PointNm
    radius_nm: int


@dataclass(frozen=True, slots=True, kw_only=True)
class AnnulusOperand:
    operand_id: OperandId
    kind: Literal["annulus"]
    feature_id: FeatureId
    center: PointNm
    inner_radius_nm: int
    outer_radius_nm: int


@dataclass(frozen=True, slots=True, kw_only=True)
class CapsuleOperand:
    operand_id: OperandId
    kind: Literal["capsule"]
    feature_id: FeatureId
    start: PointNm
    end: PointNm
    width_nm: int


PathId: TypeAlias = int

# Segment forms supported by constant-width swept centerlines.
AuthoredPathSegment: TypeAlias = AuthoredLineSegment | AuthoredCircularArcSegment


@dataclass(frozen=True, slots=True, kw_only=True)
class PlanarPath:
    path_id: PathId
    vertices: tuple[AuthoredVertex, ...]
    segments: tuple[AuthoredPathSegment, ...]


@dataclass(frozen=True, slots=True, kw_only=True)
class SweptPathOperand:
    operand_id: OperandId
    kind: Literal["swept_path"]
    feature_id: FeatureId
    centerline: PlanarPath
    width_nm: int
    cap: Literal["round"]
    join: Literal["round"]


AnalyticPlanarOperand: TypeAlias = (
    PlanarRegionOperand | DiskOperand | AnnulusOperand | CapsuleOperand | SweptPathOperand
)


@dataclass(frozen=True, slots=True, kw_only=True)
class AnalyticPlanarBooleanStage:
    stage_id: StageId
    operation: StageOperation
    operands: tuple[AnalyticPlanarOperand, ...]


@dataclass(frozen=True, slots=True, kw_only=True)
class AnalyticPlanarBooleanJob:
    job_id: JobId
    stages: tuple[AnalyticPlanarBooleanStage, ...]


QueryId: TypeAlias = int


@dataclass(frozen=True, slots=True, kw_only=True)
class PlanarRelationshipQuery:
    query_id: QueryId
    left_job_id: JobId
    right_job_id: JobId


# Logical request projected to the governed packed request attachment.
@dataclass(frozen=True, slots=True, kw_only=True)
class AnalyticPlanarBooleanBatchRequestA0:
    jobs: tuple[AnalyticPlanarBooleanJob, ...]
    relationship_queries: tuple[PlanarRelationshipQuery, ...]


class JobDiagnosticCode(str, Enum):
    INVALID_TOPOLOGY = "geometer.operation.analytic_planar_boolean.invalid_topology"
    INVALID_ARC = "geometer.operation.analytic_planar_boolean.invalid_arc"
    UNSUPPORTED_GEOMETRY = "geometer.operation.analytic_planar_boolean.unsupported_geometry"
    NORMALIZATION_ERROR_EXCEEDED = "geometer.operation.analytic_planar_boolean.normalization_error_exceeded"
    NORMALIZATION_TOPOLOGY_COLLAPSE = "geometer.operation.analytic_planar_boolean.normalization_topology_collapse"
    NONANALYTIC_RESULT = "geometer.operation.analytic_planar_boolean.nonanalytic_result"
    SOLVER_FAILED = "geometer.operation.analytic_planar_boolean.solver_failed"
    RESOURCE_LIMIT_EXCEEDED = "geometer.operation.analytic_planar_boolean.resource_limit_exceeded"


class DiagnosticSeverity(str, Enum):
    ERROR = "error"
    WARNING = "warning"


# Standalone logical identities projected one-to-one from nonzero packed path tokens.
class JobDiagnosticPath(str, Enum):
    REQUEST_JOBS = "request_jobs"
    JOB_ID = "job_id"
    JOB_STAGES = "job_stages"
    STAGE_ID = "stage_id"
    STAGE_OPERATION = "stage_operation"
    STAGE_OPERANDS = "stage_operands"
    OPERAND_ID = "operand_id"
    OPERAND_GEOMETRY = "operand_geometry"
    REGION_OUTER = "region_outer"
    REGION_HOLES = "region_holes"
    RING_VERTICES = "ring_vertices"
    RING_SEGMENTS = "ring_segments"
    PATH_VERTICES = "path_vertices"
    PATH_SEGMENTS = "path_segments"
    SEGMENT_CURVE = "segment_curve"
    DISK_RADIUS = "disk_radius"
    ANNULUS_INNER_RADIUS = "annulus_inner_radius"
    ANNULUS_OUTER_RADIUS = "annulus_outer_radius"
    CAPSULE_START = "capsule_start"
    CAPSULE_END = "capsule_end"
    CAPSULE_WIDTH = "capsule_width"
    SWEPT_PATH_CENTERLINE = "swept_path_centerline"
    SWEPT_PATH_WIDTH = "swept_path_width"
    RELATIONSHIP_QUERIES = "relationship_queries"
    RELATIONSHIP_LEFT_JOB_ID = "relationship_left_job_id"
    RELATIONSHIP_RIGHT_JOB_ID = "relationship_right_job_id"


@dataclass(frozen=True, slots=True, kw_only=True)
class JobDiagnostic:
    code: JobDiagnosticCode
    severity: DiagnosticSeverity
    job_id: JobId
    stage_id: StageId | None = None
    operand_id: OperandId | None = None
    geometry_id: int | None = None
    # Governed standalone location identity; token zero projects to absence.
    path_identity: JobDiagnosticPath | None = None


ResultVertexId: TypeAlias = int


class SourceKind(str, Enum):
    AUTHORED_SEGMENT_CURVE = "authored_segment_curve"
    COMPACT_FEATURE_ROLE = "compact_feature_role"
    SUBTRACTIVE_OPERAND_EFFECT = "subtractive_operand_effect"


class SourceRole(str, Enum):
    NONE = "none"
    AUTHORED_LINE = "authored_line"
    AUTHORED_CIRCULAR_ARC = "authored_circular_arc"
    PRIMITIVE_OUTER_CIRCLE = "primitive_outer_circle"
    PRIMITIVE_INNER_CIRCLE = "primitive_inner_circle"
    CAPSULE_LEFT_LINE = "capsule_left_line"
    CAPSULE_END_CAP = "capsule_end_cap"
    CAPSULE_RIGHT_LINE = "capsule_right_line"
    CAPSULE_START_CAP = "capsule_start_cap"
    SWEPT_LEFT_OFFSET_LINE = "swept_left_offset_line"
    SWEPT_LEFT_OFFSET_ARC = "swept_left_offset_arc"
    SWEPT_RIGHT_OFFSET_LINE = "swept_right_offset_line"
    SWEPT_RIGHT_OFFSET_ARC = "swept_right_offset_arc"
    SWEPT_ROUND_JOIN = "swept_round_join"
    SWEPT_START_CAP = "swept_start_cap"
    SWEPT_END_CAP = "swept_end_cap"


@dataclass(frozen=True, slots=True, kw_only=True)
class SourceReference:
    kind: SourceKind
    role: SourceRole
    operand_id: OperandId
    primary_id: int
    secondary_id: int


@dataclass(frozen=True, slots=True, kw_only=True)
class SourceSet:
    sources: tuple[SourceReference, ...]


@dataclass(frozen=True, slots=True, kw_only=True)
class ResultVertex:
    vertex_id: ResultVertexId
    point: PointNm
    intersection_sources: SourceSet


ResultFragmentId: TypeAlias = int


@dataclass(frozen=True, slots=True, kw_only=True)
class ResultLineFragment:
    fragment_id: ResultFragmentId
    kind: Literal["line"]
    start_vertex_id: ResultVertexId
    end_vertex_id: ResultVertexId
    coincident_positive_sources: SourceSet
    surviving_subtraction_sources: SourceSet


@dataclass(frozen=True, slots=True, kw_only=True)
class ResultCircularArcFragment:
    fragment_id: ResultFragmentId
    kind: Literal["circular_arc"]
    start_vertex_id: ResultVertexId
    end_vertex_id: ResultVertexId
    radius_nm: int
    direction: ArcDirection
    major_arc: bool
    coincident_positive_sources: SourceSet
    surviving_subtraction_sources: SourceSet


DirectedFragment: TypeAlias = ResultLineFragment | ResultCircularArcFragment

ResultRingId: TypeAlias = int


@dataclass(frozen=True, slots=True, kw_only=True)
class ResultRing:
    ring_id: ResultRingId
    fragment_ids: tuple[ResultFragmentId, ...]
    parent_ring_id: ResultRingId | None = None
    depth: int
    hole: bool


ResultRegionId: TypeAlias = int


@dataclass(frozen=True, slots=True, kw_only=True)
class ResultRegion:
    result_region_id: ResultRegionId
    outer_ring_id: ResultRingId
    positive_contributors: SourceSet


class OperandOutcomeKind(str, Enum):
    CONTRIBUTES_FINAL_MATERIAL = "contributes_final_material"
    REDUNDANT_OR_ABSORBED_COVERAGE = "redundant_or_absorbed_coverage"
    PARTIALLY_REMOVED_LATER = "partially_removed_later"
    COMPLETELY_REMOVED_LATER = "completely_removed_later"
    SUBTRACTION_EFFECT_SURVIVES = "subtraction_effect_survives"
    SUBTRACTION_EFFECT_OVERWRITTEN_LATER = "subtraction_effect_overwritten_later"
    NO_EFFECT = "no_effect"


@dataclass(frozen=True, slots=True, kw_only=True)
class OperandOutcomeEvent:
    operand_id: OperandId
    kind: OperandOutcomeKind
    result_ring_ids: tuple[ResultRingId, ...]
    result_region_ids: tuple[ResultRegionId, ...]
    sources: SourceSet


@dataclass(frozen=True, slots=True, kw_only=True)
class SuccessfulJobResult:
    job_id: JobId
    status: Literal["success"]
    diagnostics: tuple[JobDiagnostic, ...]
    vertices: tuple[ResultVertex, ...]
    directed_fragments: tuple[DirectedFragment, ...]
    rings: tuple[ResultRing, ...]
    result_regions: tuple[ResultRegion, ...]
    operand_outcomes: tuple[OperandOutcomeEvent, ...]
    # Derived from the canonical standalone job-result packet; not an encoded record field.
    digest_sha256: str


@dataclass(frozen=True, slots=True, kw_only=True)
class FailedJobResult:
    job_id: JobId
    status: Literal["failure"]
    diagnostics: tuple[JobDiagnostic, ...]
    # Derived from the canonical standalone job-result packet; not an encoded record field.
    digest_sha256: str


AnalyticPlanarBooleanJobResult: TypeAlias = SuccessfulJobResult | FailedJobResult


class RelationshipStatus(str, Enum):
    SUCCESS = "success"
    SKIPPED_DEPENDENCY_FAILED = "skipped_dependency_failed"


class IntersectionDimension(str, Enum):
    DISJOINT = "disjoint"
    POINT = "point"
    CURVE = "curve"
    AREA = "area"


@dataclass(frozen=True, slots=True, kw_only=True)
class RelationshipRegionPair:
    left_result_region_id: ResultRegionId
    right_result_region_id: ResultRegionId
    dimension: IntersectionDimension
    equality: bool
    left_contains_right: bool
    right_contains_left: bool


@dataclass(frozen=True, slots=True, kw_only=True)
class PlanarRelationshipResult:
    query_id: QueryId
    status: RelationshipStatus
    aggregate_dimension: IntersectionDimension
    pairs: tuple[RelationshipRegionPair, ...]


# Logical result projected from the governed packed result attachment.
@dataclass(frozen=True, slots=True, kw_only=True)
class AnalyticPlanarBooleanBatchResultA0:
    job_results: tuple[AnalyticPlanarBooleanJobResult, ...]
    relationship_results: tuple[PlanarRelationshipResult, ...]


# Stable diagnostic category with distinct transport, contract, and operation domains.
class DiagnosticCategory(str, Enum):
    TRANSPORT = "transport"
    CONTRACT = "contract"
    OPERATION = "operation"


# A strict cross-transport diagnostic; message text is informative rather than identity.
@dataclass(frozen=True, slots=True, kw_only=True)
class DiagnosticA0:
    # Stable namespaced diagnostic identity.
    code: str
    category: DiagnosticCategory
    # Human-readable non-normative detail.
    message: str
    retryable: bool
    # RFC 6901 JSON Pointer when a document location is meaningful.
    path: str | None = None
    # Stable operation identity when the diagnostic is operation-scoped.
    operation: str | None = None
    # Transport correlation identifier when available.
    request_id: str | None = None


# A named binary attachment carrying a separately governed packed projection.
@dataclass(frozen=True, slots=True, kw_only=True)
class PackedAttachmentReferenceA0:
    attachment: str
    format: str


# Minimal JSON value used while an operation's logical DTO projection remains deferred.
@dataclass(frozen=True, slots=True, kw_only=True)
class PackedAttachmentProjectionA0:
    schema: str
    packet: PackedAttachmentReferenceA0


# Named raw-attachment declaration in the negotiated operation catalog.
@dataclass(frozen=True, slots=True, kw_only=True)
class IpcAttachmentDeclarationA0:
    name: str
    required: bool
    media_types: tuple[str, ...]
    max_bytes: int


# wasm32 attachment-descriptor member offsets.
@dataclass(frozen=True, slots=True, kw_only=True)
class IpcAttachmentOffsetsWasm32A0:
    struct_size: int
    flags: int
    name: int
    name_size: int
    media_type: int
    media_type_size: int
    data: int
    data_size: int
    reserved0: int


@dataclass(frozen=True, slots=True, kw_only=True)
class IpcAttachmentLayoutWasm32A0:
    size: int
    offsets: IpcAttachmentOffsetsWasm32A0


# 64-bit attachment-descriptor member offsets.
@dataclass(frozen=True, slots=True, kw_only=True)
class IpcAttachmentOffsetsPointer64A0:
    struct_size: int
    flags: int
    name: int
    name_size: int
    media_type: int
    media_type_size: int
    data: int
    data_size: int
    reserved0: int


@dataclass(frozen=True, slots=True, kw_only=True)
class IpcAttachmentLayoutPointer64A0:
    size: int
    offsets: IpcAttachmentOffsetsPointer64A0


@dataclass(frozen=True, slots=True, kw_only=True)
class IpcAttachmentDescriptorA0:
    wasm32: IpcAttachmentLayoutWasm32A0
    pointer64: IpcAttachmentLayoutPointer64A0


# Successful queue-only cancellation outcome.
@dataclass(frozen=True, slots=True, kw_only=True)
class IpcCancelledA0:
    status: Literal["cancelled"]


# Nonterminal cancellation rejection for an active or unknown request.
@dataclass(frozen=True, slots=True, kw_only=True)
class IpcCancelRejectedA0:
    status: Literal["rejected"]
    diagnostic: DiagnosticA0


# Effective executable IPC A0 limits selected by the server.
@dataclass(frozen=True, slots=True, kw_only=True)
class IpcEffectiveLimitsA0:
    json_bytes: int
    attachment_count: int
    attachment_name_bytes: int
    attachment_media_type_bytes: int
    attachment_bytes: int
    frame_bytes: int
    queued_requests: int
    queued_bytes: int
    resident_request_bytes: int
    pending_writer_bytes: int


# Bounds shared by the generic C ABI operation catalog.
@dataclass(frozen=True, slots=True, kw_only=True)
class IpcGenericAbiLimitsA0:
    operation_id_bytes: int
    request_json_bytes: int
    response_json_bytes: int
    attachment_count: int
    attachment_name_bytes: int
    attachment_media_type_bytes: int
    attachment_bytes: int
    aggregate_attachment_bytes_native: int
    aggregate_attachment_bytes_wasm: int


# Client handshake for executable IPC A0.
@dataclass(frozen=True, slots=True, kw_only=True)
class IpcHelloA0:
    client_name: str
    client_version: str
    protocols: tuple[str, ...]
    capabilities: tuple[str, ...] | None = None


class IpcRuntimeDispatchA0(str, Enum):
    LOGICAL_DTO = "logical_dto"
    PACKED_ATTACHMENT = "packed_attachment"


@dataclass(frozen=True, slots=True, kw_only=True)
class IpcPackedProjectionA0:
    kind: Literal["packed_attachment"]
    attachment_name: str
    format: str


# One operation exposed by the negotiated generic transport.
@dataclass(frozen=True, slots=True, kw_only=True)
class IpcOperationDeclarationA0:
    identity: str
    request_contract: str
    result_contract: str
    runtime_dispatch: IpcRuntimeDispatchA0
    input_attachments: tuple[IpcAttachmentDeclarationA0, ...]
    output_attachments: tuple[IpcAttachmentDeclarationA0, ...]
    request_projection: IpcPackedProjectionA0 | None = None
    result_projection: IpcPackedProjectionA0 | None = None


# Exact operation catalog embedded into the welcome frame.
@dataclass(frozen=True, slots=True, kw_only=True)
class IpcOperationCatalogA0:
    catalog: Literal["wn.geometer.operation_catalog.a0"]
    generic_abi: Literal["a0"]
    release_version: str
    c_abi_generation: int
    operations: tuple[IpcOperationDeclarationA0, ...]
    attachment_descriptor: IpcAttachmentDescriptorA0
    limits: IpcGenericAbiLimitsA0


# Fatal connection-level protocol diagnostic.
@dataclass(frozen=True, slots=True, kw_only=True)
class IpcProtocolErrorA0:
    status: Literal["protocol_error"]
    diagnostic: DiagnosticA0


# Optional human-readable reason used by cancel and shutdown controls.
@dataclass(frozen=True, slots=True, kw_only=True)
class IpcReasonA0:
    reason: str | None = None


# Canonical model source format. Compatibility readers may additionally accept STEP.
class ModelFormat(str, Enum):
    STEP = "step"


# Row-major affine 4x4 matrix. Semantic validation requires final row [0,0,0,1].
Matrix4x4: TypeAlias = tuple[
    float, float, float, float, float, float, float, float, float, float, float, float, float, float, float, float
]


# Presence-preserving patch applied over focused C++ defaults.
@dataclass(frozen=True, slots=True, kw_only=True)
class ModelBoundsOptionsA0:
    # Absent preserves the inherited value; canonical default intent is step.
    format: ModelFormat | None = None
    # Absent preserves the inherited transform; canonical default intent is identity.
    model_transform: Matrix4x4 | None = None


# Strict generic request envelope for executable IPC A0.
IpcRequestValueA0: TypeAlias = ModelBoundsOptionsA0 | PackedAttachmentProjectionA0


@dataclass(frozen=True, slots=True, kw_only=True)
class IpcRequestA0:
    operation: str
    request: IpcRequestValueA0


# Final acknowledgment after every accepted request is terminal and flushed.
@dataclass(frozen=True, slots=True, kw_only=True)
class IpcShutdownAckA0:
    status: Literal["complete"]
    active_request_completed: bool
    rejected_queued_request_count: int


# Server handshake selecting executable IPC A0 and its effective limits.
@dataclass(frozen=True, slots=True, kw_only=True)
class IpcWelcomeA0:
    release_version: str
    c_abi_generation: int
    ipc: Literal["a0"]
    catalog_sha256: str
    operation_catalog: IpcOperationCatalogA0
    limits: IpcEffectiveLimitsA0
    capabilities: tuple[str, ...]


# Source identity included in a successful model-bounds result.
@dataclass(frozen=True, slots=True, kw_only=True)
class ModelBoundsSource:
    format: ModelFormat
    hash: str


# Exactly three finite millimeter coordinates.
Vector3: TypeAlias = tuple[float, float, float]


# Axis-aligned bounds in millimeters.
@dataclass(frozen=True, slots=True, kw_only=True)
class ModelBoundsValues:
    min: Vector3
    max: Vector3
    size: Vector3
    center: Vector3


# Nondeterministic timings excluded only by explicit conformance projection.
@dataclass(frozen=True, slots=True, kw_only=True)
class ModelBoundsTimings:
    model_read_ms: float
    bounds_ms: float


# Successful model-bounds result matching the compatible public JSON shape.
@dataclass(frozen=True, slots=True, kw_only=True)
class ModelBoundsResultA0:
    schema: Literal["geometry.model_bounds.a0"]
    units: Literal["mm"]
    source: ModelBoundsSource
    bounds: ModelBoundsValues
    timings: ModelBoundsTimings


# A rejected or failed operation with governed diagnostics.
@dataclass(frozen=True, slots=True, kw_only=True)
class OperationFailureA0:
    operation: str
    ok: Literal[False]
    diagnostics: tuple[DiagnosticA0, ...]


# Results currently available through the generic operation transport.
OperationResultValueA0: TypeAlias = ModelBoundsResultA0 | PackedAttachmentProjectionA0


# A completed operation with its operation-specific result.
@dataclass(frozen=True, slots=True, kw_only=True)
class OperationSuccessA0:
    operation: str
    ok: Literal[True]
    result: OperationResultValueA0


# Transport-neutral typed outcome shared by the generic C ABI and executable IPC.
OperationOutcomeA0: TypeAlias = OperationSuccessA0 | OperationFailureA0


@dataclass(frozen=True, slots=True, kw_only=True)
class SourceEntityEvidence:
    mapped: bool
    shape_result_round_trip: bool
    model_number: int | None = None
    entity_type: str | None = None
    mapping_method: str | None = None


@dataclass(frozen=True, slots=True, kw_only=True)
class BodySummary:
    handle: str
    definition_handle: str
    topology_kind: str
    shell_handles: tuple[str, ...]
    face_handles: tuple[str, ...]
    bounds_mm: tuple[float, ...]
    volume_mm3: float
    source_entity: SourceEntityEvidence | None = None


@dataclass(frozen=True, slots=True, kw_only=True)
class ComponentOccurrenceSummary:
    kind: Literal["component"]
    handle: str
    definition_handle: str
    parent_occurrence_handle: str
    depth: int
    name: str
    transform: tuple[float, ...]


@dataclass(frozen=True, slots=True, kw_only=True)
class DefinitionSummary:
    handle: str
    name: str
    assembly: bool
    body_count: int
    face_count: int
    source_entity: SourceEntityEvidence | None = None


@dataclass(frozen=True, slots=True, kw_only=True)
class FaceSummary:
    handle: str
    definition_handle: str
    body_handles: tuple[str, ...]
    shell_handles: tuple[str, ...]
    bounds_mm: tuple[float, ...]
    area_mm2: float
    centroid_mm: tuple[float, ...]
    source_entity: SourceEntityEvidence | None = None


@dataclass(frozen=True, slots=True, kw_only=True)
class GlbAttachmentDescriptor:
    name: Literal["glb"]
    media_type: Literal["model/gltf-binary"]
    format: Literal["glb-2.0"]
    bytes: int
    sha256: str


@dataclass(frozen=True, slots=True, kw_only=True)
class InspectionCounts:
    definitions: int
    root_occurrences: int
    component_occurrences: int
    bodies: int
    shells: int
    faces: int


@dataclass(frozen=True, slots=True, kw_only=True)
class RootOccurrenceSummary:
    kind: Literal["root"]
    handle: str
    definition_handle: str
    name: str
    transform: tuple[float, ...]


OccurrenceSummary: TypeAlias = RootOccurrenceSummary | ComponentOccurrenceSummary


@dataclass(frozen=True, slots=True, kw_only=True)
class PageRequest:
    cursor: str | None = None
    limit: int


@dataclass(frozen=True, slots=True, kw_only=True)
class RenderCounts:
    meshes: int
    instances: int
    primitives: int
    geometry_triangles: int
    instanced_triangles: int


@dataclass(frozen=True, slots=True, kw_only=True)
class RenderArtifactDescriptor:
    artifact_handle: str
    content_sha256: str
    render_artifact_handle: str
    render_content_sha256: str
    binding_layout: Literal["node-primitive-a0"]
    geometry_length_unit: Literal["meter"]
    source_length_unit: Literal["millimeter"]
    counts: RenderCounts


@dataclass(frozen=True, slots=True, kw_only=True)
class SessionReference:
    session_handle: str
    generation: int


@dataclass(frozen=True, slots=True, kw_only=True)
class ShellSummary:
    handle: str
    definition_handle: str
    body_handles: tuple[str, ...]
    face_handles: tuple[str, ...]
    source_entity: SourceEntityEvidence | None = None


@dataclass(frozen=True, slots=True, kw_only=True)
class SourceDescriptor:
    format: Literal["step"]
    sha256: str
    bytes: int
    normalized_length_unit: Literal["millimeter"]


@dataclass(frozen=True, slots=True, kw_only=True)
class StepTopologyCloseRequestA0:
    schema: Literal["geometry.step_topology.close.request.a0"]
    session: SessionReference


@dataclass(frozen=True, slots=True, kw_only=True)
class StepTopologyCloseResultA0:
    schema: Literal["geometry.step_topology.close.result.a0"]
    session_handle: str
    closed: Literal[True]


@dataclass(frozen=True, slots=True, kw_only=True)
class StepTopologyInspectRequestA0:
    schema: Literal["geometry.step_topology.inspect.request.a0"]
    session: SessionReference
    page: PageRequest
    include_source_entity_evidence: bool
    include_diagnostics: bool


@dataclass(frozen=True, slots=True, kw_only=True)
class TopologyPage:
    definitions: tuple[DefinitionSummary, ...]
    occurrences: tuple[OccurrenceSummary, ...]
    bodies: tuple[BodySummary, ...]
    shells: tuple[ShellSummary, ...]
    faces: tuple[FaceSummary, ...]
    next_cursor: str | None = None


@dataclass(frozen=True, slots=True, kw_only=True)
class TopologyTableAttachmentDescriptor:
    name: Literal["topology_table"]
    media_type: Literal["application/vnd.wavenumber.geometer.step-topology-table"]
    format: Literal["wn.geometer.step-topology-table.a0"]
    bytes: int
    sha256: str


@dataclass(frozen=True, slots=True, kw_only=True)
class StepTopologyInspectResultA0:
    schema: Literal["geometry.step_topology.inspect.result.a0"]
    session: SessionReference
    counts: InspectionCounts
    page: TopologyPage
    compact_table: TopologyTableAttachmentDescriptor | None = None
    diagnostics: tuple[DiagnosticA0, ...]


@dataclass(frozen=True, slots=True, kw_only=True)
class StepTopologyOpenRequestA0:
    schema: Literal["geometry.step_topology.open.request.a0"]


@dataclass(frozen=True, slots=True, kw_only=True)
class ToolDescriptor:
    name: Literal["geometer"]
    release_version: str
    occt_version: str


@dataclass(frozen=True, slots=True, kw_only=True)
class StepTopologyOpenResultA0:
    schema: Literal["geometry.step_topology.open.result.a0"]
    session: SessionReference
    source: SourceDescriptor
    tool: ToolDescriptor
    evicted_session_handles: tuple[str, ...]


@dataclass(frozen=True, slots=True, kw_only=True)
class TessellationOptions:
    linear_deflection_mm: float
    angular_deflection_rad: float
    relative: bool
    parallel: bool
    source_to_render: tuple[float, ...]


@dataclass(frozen=True, slots=True, kw_only=True)
class StepTopologyRenderRequestA0:
    schema: Literal["geometry.step_topology.render.request.a0"]
    session: SessionReference
    tessellation: TessellationOptions


@dataclass(frozen=True, slots=True, kw_only=True)
class TopologyBindingTableAttachmentDescriptor:
    name: Literal["topology_binding_table"]
    media_type: Literal["application/vnd.wavenumber.geometer.step-topology-binding-table"]
    format: Literal["wn.geometer.step-topology-binding-table.a0"]
    bytes: int
    sha256: str


@dataclass(frozen=True, slots=True, kw_only=True)
class StepTopologyRenderResultA0:
    schema: Literal["geometry.step_topology.render.result.a0"]
    session: SessionReference
    artifact: RenderArtifactDescriptor
    glb: GlbAttachmentDescriptor
    compact_binding_table: TopologyBindingTableAttachmentDescriptor | None = None


@dataclass(frozen=True, slots=True, kw_only=True)
class StepTopologyResolveHitRequestA0:
    schema: Literal["geometry.step_topology.resolve_hit.request.a0"]
    session: SessionReference
    artifact_handle: str
    content_sha256: str
    instance_index: int
    primitive_index: int
    primitive_triangle_index: int
    occurrence_handle: str
    body_handle: str
    face_handle: str


@dataclass(frozen=True, slots=True, kw_only=True)
class StepTopologyResolveHitResultA0:
    schema: Literal["geometry.step_topology.resolve_hit.result.a0"]
    session: SessionReference
    instance_index: int
    primitive_index: int
    triangle_index: int
    occurrence_handle: str
    body_handle: str
    face_handle: str


MODEL_TYPES = {
    "Wavenumber.Geometer.Contracts.Common.DiagnosticA0": DiagnosticA0,
    "Wavenumber.Geometer.Contracts.Common.PackedAttachmentProjectionA0": PackedAttachmentProjectionA0,
    "Wavenumber.Geometer.Contracts.Common.PackedAttachmentReferenceA0": PackedAttachmentReferenceA0,
    "Wavenumber.Geometer.Contracts.IpcA0.IpcAttachmentDeclarationA0": IpcAttachmentDeclarationA0,
    "Wavenumber.Geometer.Contracts.IpcA0.IpcAttachmentDescriptorA0": IpcAttachmentDescriptorA0,
    "Wavenumber.Geometer.Contracts.IpcA0.IpcAttachmentLayoutPointer64A0": IpcAttachmentLayoutPointer64A0,
    "Wavenumber.Geometer.Contracts.IpcA0.IpcAttachmentLayoutWasm32A0": IpcAttachmentLayoutWasm32A0,
    "Wavenumber.Geometer.Contracts.IpcA0.IpcAttachmentOffsetsPointer64A0": IpcAttachmentOffsetsPointer64A0,
    "Wavenumber.Geometer.Contracts.IpcA0.IpcAttachmentOffsetsWasm32A0": IpcAttachmentOffsetsWasm32A0,
    "Wavenumber.Geometer.Contracts.IpcA0.IpcCancelledA0": IpcCancelledA0,
    "Wavenumber.Geometer.Contracts.IpcA0.IpcCancelRejectedA0": IpcCancelRejectedA0,
    "Wavenumber.Geometer.Contracts.IpcA0.IpcEffectiveLimitsA0": IpcEffectiveLimitsA0,
    "Wavenumber.Geometer.Contracts.IpcA0.IpcGenericAbiLimitsA0": IpcGenericAbiLimitsA0,
    "Wavenumber.Geometer.Contracts.IpcA0.IpcHelloA0": IpcHelloA0,
    "Wavenumber.Geometer.Contracts.IpcA0.IpcOperationCatalogA0": IpcOperationCatalogA0,
    "Wavenumber.Geometer.Contracts.IpcA0.IpcOperationDeclarationA0": IpcOperationDeclarationA0,
    "Wavenumber.Geometer.Contracts.IpcA0.IpcPackedProjectionA0": IpcPackedProjectionA0,
    "Wavenumber.Geometer.Contracts.IpcA0.IpcProtocolErrorA0": IpcProtocolErrorA0,
    "Wavenumber.Geometer.Contracts.IpcA0.IpcReasonA0": IpcReasonA0,
    "Wavenumber.Geometer.Contracts.IpcA0.IpcRequestA0": IpcRequestA0,
    "Wavenumber.Geometer.Contracts.IpcA0.IpcShutdownAckA0": IpcShutdownAckA0,
    "Wavenumber.Geometer.Contracts.IpcA0.IpcWelcomeA0": IpcWelcomeA0,
    "Wavenumber.Geometer.Contracts.ModelBoundsA0.ModelBoundsOptionsA0": ModelBoundsOptionsA0,
    "Wavenumber.Geometer.Contracts.ModelBoundsA0.ModelBoundsResultA0": ModelBoundsResultA0,
    "Wavenumber.Geometer.Contracts.ModelBoundsA0.ModelBoundsSource": ModelBoundsSource,
    "Wavenumber.Geometer.Contracts.ModelBoundsA0.ModelBoundsTimings": ModelBoundsTimings,
    "Wavenumber.Geometer.Contracts.ModelBoundsA0.ModelBoundsValues": ModelBoundsValues,
    "Wavenumber.Geometer.Contracts.OperationOutcomeA0.OperationFailureA0": OperationFailureA0,
    "Wavenumber.Geometer.Contracts.OperationOutcomeA0.OperationSuccessA0": OperationSuccessA0,
    "Wavenumber.Geometer.Contracts.StepTopologyA0.BodySummary": BodySummary,
    "Wavenumber.Geometer.Contracts.StepTopologyA0.ComponentOccurrenceSummary": ComponentOccurrenceSummary,
    "Wavenumber.Geometer.Contracts.StepTopologyA0.DefinitionSummary": DefinitionSummary,
    "Wavenumber.Geometer.Contracts.StepTopologyA0.FaceSummary": FaceSummary,
    "Wavenumber.Geometer.Contracts.StepTopologyA0.GlbAttachmentDescriptor": GlbAttachmentDescriptor,
    "Wavenumber.Geometer.Contracts.StepTopologyA0.InspectionCounts": InspectionCounts,
    "Wavenumber.Geometer.Contracts.StepTopologyA0.PageRequest": PageRequest,
    "Wavenumber.Geometer.Contracts.StepTopologyA0.RenderArtifactDescriptor": RenderArtifactDescriptor,
    "Wavenumber.Geometer.Contracts.StepTopologyA0.RenderCounts": RenderCounts,
    "Wavenumber.Geometer.Contracts.StepTopologyA0.RootOccurrenceSummary": RootOccurrenceSummary,
    "Wavenumber.Geometer.Contracts.StepTopologyA0.SessionReference": SessionReference,
    "Wavenumber.Geometer.Contracts.StepTopologyA0.ShellSummary": ShellSummary,
    "Wavenumber.Geometer.Contracts.StepTopologyA0.SourceDescriptor": SourceDescriptor,
    "Wavenumber.Geometer.Contracts.StepTopologyA0.SourceEntityEvidence": SourceEntityEvidence,
    "Wavenumber.Geometer.Contracts.StepTopologyA0.StepTopologyCloseRequestA0": StepTopologyCloseRequestA0,
    "Wavenumber.Geometer.Contracts.StepTopologyA0.StepTopologyCloseResultA0": StepTopologyCloseResultA0,
    "Wavenumber.Geometer.Contracts.StepTopologyA0.StepTopologyInspectRequestA0": StepTopologyInspectRequestA0,
    "Wavenumber.Geometer.Contracts.StepTopologyA0.StepTopologyInspectResultA0": StepTopologyInspectResultA0,
    "Wavenumber.Geometer.Contracts.StepTopologyA0.StepTopologyOpenRequestA0": StepTopologyOpenRequestA0,
    "Wavenumber.Geometer.Contracts.StepTopologyA0.StepTopologyOpenResultA0": StepTopologyOpenResultA0,
    "Wavenumber.Geometer.Contracts.StepTopologyA0.StepTopologyRenderRequestA0": StepTopologyRenderRequestA0,
    "Wavenumber.Geometer.Contracts.StepTopologyA0.StepTopologyRenderResultA0": StepTopologyRenderResultA0,
    "Wavenumber.Geometer.Contracts.StepTopologyA0.StepTopologyResolveHitRequestA0": StepTopologyResolveHitRequestA0,
    "Wavenumber.Geometer.Contracts.StepTopologyA0.StepTopologyResolveHitResultA0": StepTopologyResolveHitResultA0,
    "Wavenumber.Geometer.Contracts.StepTopologyA0.TessellationOptions": TessellationOptions,
    "Wavenumber.Geometer.Contracts.StepTopologyA0.ToolDescriptor": ToolDescriptor,
    "Wavenumber.Geometer.Contracts.StepTopologyA0.TopologyBindingTableAttachmentDescriptor": TopologyBindingTableAttachmentDescriptor,
    "Wavenumber.Geometer.Contracts.StepTopologyA0.TopologyPage": TopologyPage,
    "Wavenumber.Geometer.Contracts.StepTopologyA0.TopologyTableAttachmentDescriptor": TopologyTableAttachmentDescriptor,
}

ENUM_TYPES = {
    "Wavenumber.Geometer.Contracts.Common.DiagnosticCategory": DiagnosticCategory,
    "Wavenumber.Geometer.Contracts.IpcA0.IpcRuntimeDispatchA0": IpcRuntimeDispatchA0,
    "Wavenumber.Geometer.Contracts.ModelBoundsA0.ModelFormat": ModelFormat,
}
