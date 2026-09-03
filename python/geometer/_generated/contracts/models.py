# Generated from wn_geometer_contract_catalog.a0.json. Do not edit.

from dataclasses import dataclass
from enum import Enum
from typing import Literal, TypeAlias

NORMALIZED_CATALOG_SHA256 = "3d610e74fa16618a12806607c55be6823d6bf5e9144095ed281179aaeda1415d"

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


# Resource ceilings applied during Fast mesh preparation and projection.
@dataclass(frozen=True, slots=True, kw_only=True)
class FastHlrLimitsA0:
    max_vertices: int | None = None
    max_triangles: int | None = None
    max_edges: int | None = None
    max_grid_references: int | None = None
    max_candidate_pairs: int | None = None
    max_fragments: int | None = None
    max_output_segments: int | None = None


# Fast vector-HLR controls. Angles are radians and geometric tolerances use model units.
@dataclass(frozen=True, slots=True, kw_only=True)
class FastHlrOptionsA0:
    include_boundaries: bool | None = None
    include_creases: bool | None = None
    include_silhouettes: bool | None = None
    include_hidden: bool | None = None
    suppress_coplanar_seams: bool | None = None
    crease_angle_rad: float | None = None
    weld_tolerance: float | None = None
    projected_tolerance: float | None = None
    depth_tolerance: float | None = None
    coplanar_seam_angle_rad: float | None = None
    coplanar_seam_depth_tolerance: float | None = None
    coplanar_seam_lateral_tolerance: float | None = None
    limits: FastHlrLimitsA0 | None = None


class HlrCurveMode(str, Enum):
    NATIVE_ARCS = "native_arcs"
    POLYLINE = "polyline"


HlrMatrix4x4: TypeAlias = tuple[
    float, float, float, float, float, float, float, float, float, float, float, float, float, float, float, float
]


class HlrMeshDeflectionMode(str, Enum):
    ABSOLUTE = "absolute"
    BBOX_RELATIVE = "bbox-relative"


class HlrOutlineAlgorithm(str, Enum):
    HLR_CLOSE = "hlr-close"
    MESH_SHADOW = "mesh-shadow"
    FAST_MESH_SHADOW = "fast-mesh-shadow"


HlrVector3: TypeAlias = tuple[float, float, float]

ProjectedSegment: TypeAlias = tuple[float, float, float, float]

HlrVector2: TypeAlias = tuple[float, float]


@dataclass(frozen=True, slots=True, kw_only=True)
class ProjectedArc:
    start: HlrVector2
    end: HlrVector2
    center: HlrVector2
    radius: float
    extent_rad: float
    ccw: bool
    full_circle: bool


@dataclass(frozen=True, slots=True, kw_only=True)
class ProjectionBounds:
    min_x: float
    min_y: float
    max_x: float
    max_y: float
    width: float
    height: float


@dataclass(frozen=True, slots=True, kw_only=True)
class ProjectedGeometry:
    segments: tuple[ProjectedSegment, ...]
    arcs: tuple[ProjectedArc, ...]
    # Omitted when this output layer is empty. The legacy B0 writer emits null.
    bounds: ProjectionBounds | None = None


@dataclass(frozen=True, slots=True, kw_only=True)
class HlrProjectionModes:
    outline: ProjectedGeometry
    detail: ProjectedGeometry
    bbox: ProjectedGeometry


@dataclass(frozen=True, slots=True, kw_only=True)
class HlrProjectedView:
    id: str
    direction: HlrVector3
    up: HlrVector3
    modes: HlrProjectionModes


class HlrProjectionAlgorithm(str, Enum):
    POLY = "poly"
    EXACT = "exact"
    FAST = "fast"


@dataclass(frozen=True, slots=True, kw_only=True)
class HlrViewSpec:
    id: str
    direction: HlrVector3
    up: HlrVector3


# Presence-preserving additive options shared by STEP and indexed-mesh HLR operations.
@dataclass(frozen=True, slots=True, kw_only=True)
class HlrProjectionOptionsA0:
    views: tuple[HlrViewSpec, ...] | None = None
    output_outline: bool | None = None
    output_detail: bool | None = None
    output_bbox: bool | None = None
    model_transform: HlrMatrix4x4 | None = None
    strip_root_placement: bool | None = None
    curve_mode: HlrCurveMode | None = None
    samples_per_curve: int | None = None
    round_digits: int | None = None
    edge_v_sharp: bool | None = None
    edge_v_outline: bool | None = None
    edge_v_smooth: bool | None = None
    edge_v_sewn: bool | None = None
    edge_v_iso: bool | None = None
    edge_h_sharp: bool | None = None
    edge_h_outline: bool | None = None
    edge_h_smooth: bool | None = None
    edge_h_sewn: bool | None = None
    edge_h_iso: bool | None = None
    union_outline_polygons: bool | None = None
    projection_algorithm: HlrProjectionAlgorithm | None = None
    mesh_linear_deflection: float | None = None
    mesh_angular_deflection: float | None = None
    mesh_relative: bool | None = None
    mesh_deflection_mode: HlrMeshDeflectionMode | None = None
    mesh_deflection_coefficient: float | None = None
    outline_algorithm: HlrOutlineAlgorithm | None = None
    hlr_angle_tolerance: float | None = None
    fast: FastHlrOptionsA0 | None = None


class HlrSourceKind(str, Enum):
    STEP = "step"
    INDEXED_MESH = "indexed_mesh"


@dataclass(frozen=True, slots=True, kw_only=True)
class HlrProjectionSource:
    kind: HlrSourceKind
    hash: str


# Timings are nondeterministic and excluded only by explicit conformance projections.
@dataclass(frozen=True, slots=True, kw_only=True)
class HlrProjectionTimings:
    step_read_ms: float
    mesh_ms: float
    hlr_ms: float
    extract_ms: float


@dataclass(frozen=True, slots=True, kw_only=True)
class HlrProjectionResultA0:
    schema: Literal["geometry.hlr_projection.result.a0"]
    units: Literal["mm"]
    source: HlrProjectionSource
    views: tuple[HlrProjectedView, ...]
    timings: HlrProjectionTimings


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


@dataclass(frozen=True, slots=True, kw_only=True)
class StepTopologyOpenRequestA0:
    schema: Literal["geometry.step_topology.open.request.a0"]


@dataclass(frozen=True, slots=True, kw_only=True)
class SessionReference:
    session_handle: str
    generation: int


@dataclass(frozen=True, slots=True, kw_only=True)
class StepTopologyCloseRequestA0:
    schema: Literal["geometry.step_topology.close.request.a0"]
    session: SessionReference


@dataclass(frozen=True, slots=True, kw_only=True)
class PageRequest:
    cursor: str | None = None
    limit: int


@dataclass(frozen=True, slots=True, kw_only=True)
class StepTopologyInspectRequestA0:
    schema: Literal["geometry.step_topology.inspect.request.a0"]
    session: SessionReference
    page: PageRequest
    include_source_entity_evidence: bool
    include_diagnostics: bool


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
class CreateLogicalGroupCommand:
    kind: Literal["create"]
    authored_id: str
    name: str
    member_handles: tuple[str, ...]


@dataclass(frozen=True, slots=True, kw_only=True)
class RenameLogicalGroupCommand:
    kind: Literal["rename"]
    authored_id: str
    expected_revision: int
    name: str


@dataclass(frozen=True, slots=True, kw_only=True)
class ReplaceLogicalGroupMembersCommand:
    kind: Literal["replace_members"]
    authored_id: str
    expected_revision: int
    member_handles: tuple[str, ...]


@dataclass(frozen=True, slots=True, kw_only=True)
class EraseLogicalGroupCommand:
    kind: Literal["erase"]
    authored_id: str
    expected_revision: int


LogicalGroupCommand: TypeAlias = (
    CreateLogicalGroupCommand | RenameLogicalGroupCommand | ReplaceLogicalGroupMembersCommand | EraseLogicalGroupCommand
)


@dataclass(frozen=True, slots=True, kw_only=True)
class StepTopologyApplyLogicalGroupsRequestA0:
    schema: Literal["geometry.step_topology.apply_logical_groups.request.a0"]
    session: SessionReference
    commands: tuple[LogicalGroupCommand, ...]


@dataclass(frozen=True, slots=True, kw_only=True)
class DocumentProbeTarget:
    kind: Literal["document"]


@dataclass(frozen=True, slots=True, kw_only=True)
class DefinitionProbeTarget:
    kind: Literal["definition"]
    target_handle: str


@dataclass(frozen=True, slots=True, kw_only=True)
class RootOccurrenceProbeTarget:
    kind: Literal["root_occurrence"]
    target_handle: str


@dataclass(frozen=True, slots=True, kw_only=True)
class ComponentOccurrenceProbeTarget:
    kind: Literal["occurrence"]
    target_handle: str


@dataclass(frozen=True, slots=True, kw_only=True)
class BodyProbeTarget:
    kind: Literal["body"]
    target_handle: str


@dataclass(frozen=True, slots=True, kw_only=True)
class FaceProbeTarget:
    kind: Literal["face"]
    target_handle: str


@dataclass(frozen=True, slots=True, kw_only=True)
class LogicalGroupProbeTarget:
    kind: Literal["logical_group"]
    group_authored_id: str


MetadataProbeTarget: TypeAlias = (
    DocumentProbeTarget
    | DefinitionProbeTarget
    | RootOccurrenceProbeTarget
    | ComponentOccurrenceProbeTarget
    | BodyProbeTarget
    | FaceProbeTarget
    | LogicalGroupProbeTarget
)


@dataclass(frozen=True, slots=True, kw_only=True)
class AttachMetadataProbeCommand:
    kind: Literal["attach"]
    authored_id: str
    target: MetadataProbeTarget
    key: str
    value: str


@dataclass(frozen=True, slots=True, kw_only=True)
class ReplaceMetadataProbeCommand:
    kind: Literal["replace"]
    authored_id: str
    expected_revision: int
    target: MetadataProbeTarget
    key: str
    value: str


@dataclass(frozen=True, slots=True, kw_only=True)
class EraseMetadataProbeCommand:
    kind: Literal["erase"]
    authored_id: str
    expected_revision: int


MetadataProbeCommand: TypeAlias = AttachMetadataProbeCommand | ReplaceMetadataProbeCommand | EraseMetadataProbeCommand


@dataclass(frozen=True, slots=True, kw_only=True)
class StepTopologyApplyMetadataProbesRequestA0:
    schema: Literal["geometry.step_topology.apply_metadata_probes.request.a0"]
    session: SessionReference
    commands: tuple[MetadataProbeCommand, ...]


@dataclass(frozen=True, slots=True, kw_only=True)
class StepTopologyCheckpointEditJournalRequestA0:
    schema: Literal["geometry.step_topology.checkpoint_edit_journal.request.a0"]
    session: SessionReference


class HierarchySourceKind(str, Enum):
    DEFINITION = "definition"
    BODY = "body"


@dataclass(frozen=True, slots=True, kw_only=True)
class CreateHierarchyProductCommand:
    kind: Literal["create_product"]
    authored_id: str
    name: str
    source_kind: HierarchySourceKind
    source_handle: str


@dataclass(frozen=True, slots=True, kw_only=True)
class CreateHierarchyAssemblyCommand:
    kind: Literal["create_assembly"]
    authored_id: str
    name: str


@dataclass(frozen=True, slots=True, kw_only=True)
class CreateHierarchyOccurrenceCommand:
    kind: Literal["create_occurrence"]
    authored_id: str
    child_authored_id: str
    parent_assembly_authored_id: str
    transform: tuple[float, ...]


@dataclass(frozen=True, slots=True, kw_only=True)
class ReparentHierarchyOccurrenceCommand:
    kind: Literal["reparent_occurrence"]
    authored_id: str
    expected_revision: int
    parent_assembly_authored_id: str
    transform: tuple[float, ...]


@dataclass(frozen=True, slots=True, kw_only=True)
class RenameHierarchyNodeCommand:
    kind: Literal["rename_node"]
    authored_id: str
    expected_revision: int
    name: str


@dataclass(frozen=True, slots=True, kw_only=True)
class EraseHierarchyOccurrenceCommand:
    kind: Literal["erase_occurrence"]
    authored_id: str
    expected_revision: int


@dataclass(frozen=True, slots=True, kw_only=True)
class EraseHierarchyNodeCommand:
    kind: Literal["erase_node"]
    authored_id: str
    expected_revision: int


HierarchyCommand: TypeAlias = (
    CreateHierarchyProductCommand
    | CreateHierarchyAssemblyCommand
    | CreateHierarchyOccurrenceCommand
    | ReparentHierarchyOccurrenceCommand
    | RenameHierarchyNodeCommand
    | EraseHierarchyOccurrenceCommand
    | EraseHierarchyNodeCommand
)


@dataclass(frozen=True, slots=True, kw_only=True)
class StepTopologyApplyHierarchyRequestA0:
    schema: Literal["geometry.step_topology.apply_hierarchy.request.a0"]
    session: SessionReference
    expected_hierarchy_revision: int
    commands: tuple[HierarchyCommand, ...]


class SaveCarrier(str, Enum):
    XBF = "xbf"
    XML_XCAF = "xml_xcaf"
    STEP_AP242 = "step_ap242"
    JSON_SIDECAR = "json_sidecar"


@dataclass(frozen=True, slots=True, kw_only=True)
class StepTopologySaveRequestA0:
    schema: Literal["geometry.step_topology.save.request.a0"]
    session: SessionReference
    carrier: SaveCarrier
    include_diagnostics: bool


@dataclass(frozen=True, slots=True, kw_only=True)
class SourceDescriptor:
    format: Literal["step"]
    sha256: str
    bytes: int
    normalized_length_unit: Literal["millimeter"]


@dataclass(frozen=True, slots=True, kw_only=True)
class XbfPersistenceArtifact:
    carrier: Literal["xbf"]
    name: Literal["state_artifact"]
    media_type: Literal["application/vnd.opencascade.xbf"]
    format: Literal["ocaf-xbf-version-12"]
    bytes: int
    sha256: str


@dataclass(frozen=True, slots=True, kw_only=True)
class XmlXcafPersistenceArtifact:
    carrier: Literal["xml_xcaf"]
    name: Literal["state_artifact"]
    media_type: Literal["application/vnd.opencascade.xml-xcaf"]
    format: Literal["ocaf-xml-xcaf-version-12"]
    bytes: int
    sha256: str


@dataclass(frozen=True, slots=True, kw_only=True)
class StepAp242PersistenceArtifact:
    carrier: Literal["step_ap242"]
    name: Literal["state_artifact"]
    media_type: Literal["application/step"]
    format: Literal["ap242-managed-model-based-3d-engineering"]
    bytes: int
    sha256: str


@dataclass(frozen=True, slots=True, kw_only=True)
class JsonSidecarPersistenceArtifact:
    carrier: Literal["json_sidecar"]
    name: Literal["state_artifact"]
    media_type: Literal["application/vnd.wavenumber.geometer.step-topology-sidecar+json"]
    format: Literal["geometer.step_topology_sidecar.a0"]
    bytes: int
    sha256: str


@dataclass(frozen=True, slots=True, kw_only=True)
class EditJournalPersistenceArtifact:
    carrier: Literal["edit_journal"]
    name: Literal["state_artifact"]
    media_type: Literal["application/vnd.wavenumber.geometer.step-topology-edit-journal"]
    format: Literal["geometer.step_topology_edit_journal.a0"]
    bytes: int
    sha256: str


RestoreStateArtifact: TypeAlias = (
    XbfPersistenceArtifact
    | XmlXcafPersistenceArtifact
    | StepAp242PersistenceArtifact
    | JsonSidecarPersistenceArtifact
    | EditJournalPersistenceArtifact
)


@dataclass(frozen=True, slots=True, kw_only=True)
class EditJournalReplayPreconditions:
    source_sha256: str
    source_brep_sha256: str
    target_inventory_sha256: str
    occt_version: str
    transaction_count: int


@dataclass(frozen=True, slots=True, kw_only=True)
class StepTopologyRestoreRequestA0:
    schema: Literal["geometry.step_topology.restore.request.a0"]
    source: SourceDescriptor
    state_artifact: RestoreStateArtifact
    replay_preconditions: EditJournalReplayPreconditions | None = None
    include_diagnostics: bool


@dataclass(frozen=True, slots=True, kw_only=True)
class RecoveryProvenance:
    source_artifact_sha256: str
    candidate_artifact_sha256: str
    source_occt_version: str
    candidate_occt_version: str
    source_driver: str
    candidate_driver: str
    source_writer_settings: str
    candidate_writer_settings: str
    command_provenance: str
    measured_wall_time_milliseconds: float


@dataclass(frozen=True, slots=True, kw_only=True)
class RecoveryTolerances:
    length_mm: float
    area_mm2: float
    volume_mm3: float


class LogicalGroupMemberKind(str, Enum):
    BODY = "body"
    FACE = "face"


@dataclass(frozen=True, slots=True, kw_only=True)
class RecoveryFingerprint:
    normalized_length_unit: Literal["millimeter"]
    coordinate_frame: str
    occurrence_context: str
    geometry_kind: str
    area_mm2: float
    volume_mm3: float
    centroid_mm: tuple[float, ...]
    bounds_mm: tuple[float, ...]
    adjacency_sha256: str


class RecoveryLineage(str, Enum):
    NONE = "none"
    SPLIT_FROM_SOURCE = "split_from_source"
    MERGED_FROM_SOURCES = "merged_from_sources"


@dataclass(frozen=True, slots=True, kw_only=True)
class RecoveryCandidate:
    target_handle: str
    kind: LogicalGroupMemberKind
    authored_target_id: str | None = None
    topology_link_verified: bool
    carrier_locator: str
    carrier_locator_validated: bool
    carrier_record: str
    lineage: RecoveryLineage
    fingerprint: RecoveryFingerprint | None = None


@dataclass(frozen=True, slots=True, kw_only=True)
class RecoveryMemberRequest:
    member_record_id: str
    kind: LogicalGroupMemberKind
    authored_target_id: str
    carrier_locator: str
    source_fingerprint: RecoveryFingerprint | None = None
    candidates: tuple[RecoveryCandidate, ...]


@dataclass(frozen=True, slots=True, kw_only=True)
class RecoveryGroupRequest:
    group_authored_id: str
    provenance: RecoveryProvenance
    tolerances: RecoveryTolerances
    members: tuple[RecoveryMemberRequest, ...]


@dataclass(frozen=True, slots=True, kw_only=True)
class StepTopologyAnalyzeRecoveryRequestA0:
    schema: Literal["geometry.step_topology.analyze_recovery.request.a0"]
    groups: tuple[RecoveryGroupRequest, ...]


# Structurally representable request payloads for executable IPC A0. A variant is callable only when the negotiated runtime catalog advertises its operation; structural presence does not imply runtime availability.
IpcRequestValueA0: TypeAlias = (
    ModelBoundsOptionsA0
    | HlrProjectionOptionsA0
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
    | StepTopologyAnalyzeRecoveryRequestA0
)


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


IllustrationMatrix4x4: TypeAlias = tuple[
    float, float, float, float, float, float, float, float, float, float, float, float, float, float, float, float
]

IllustrationVector3: TypeAlias = tuple[float, float, float]


@dataclass(frozen=True, slots=True, kw_only=True)
class MeshIllustrationMaterial:
    # sRGB channels in the inclusive range [0, 1].
    color: IllustrationVector3
    opacity: float | None = None
    name: str | None = None


@dataclass(frozen=True, slots=True, kw_only=True)
class MeshIllustrationMesh:
    id: str
    positions: tuple[float, ...]
    normals: tuple[float, ...] | None = None
    indices: tuple[int, ...] | None = None
    matrix: IllustrationMatrix4x4 | None = None
    materials: tuple[MeshIllustrationMaterial, ...]
    triangle_material_indices: tuple[int, ...] | None = None
    double_sided: bool | None = None


@dataclass(frozen=True, slots=True, kw_only=True)
class MeshIllustrationView:
    direction: IllustrationVector3
    up: IllustrationVector3
    mirror_x: bool | None = None


@dataclass(frozen=True, slots=True, kw_only=True)
class MeshIllustrationPrepareOptions:
    max_triangles: int | None = None
    weld_tolerance: float | None = None


class MeshIllustrationShading(str, Enum):
    UNLIT = "unlit"
    FLAT = "flat"
    LAMBERT = "lambert"
    BANDED = "banded"
    TOON = "toon"


# Presence-preserving illustration style. Package defaults apply to absent fields.
@dataclass(frozen=True, slots=True, kw_only=True)
class MeshIllustrationStyleA0:
    shading: MeshIllustrationShading | None = None
    ambient: float | None = None
    key_intensity: float | None = None
    light_direction: IllustrationVector3 | None = None
    bands: int | None = None
    source_colors: bool | None = None
    fallback_color: IllustrationVector3 | None = None
    background: str | None = None
    transparent_background: bool | None = None
    fuse_surfaces: bool | None = None
    layer_coplanar_materials: bool | None = None
    show_hlr_outline: bool | None = None
    show_hlr_detail: bool | None = None
    show_outlines: bool | None = None
    show_creases: bool | None = None
    crease_angle_degrees: float | None = None
    outline_color: str | None = None
    crease_color: str | None = None
    outline_width: float | None = None
    crease_width: float | None = None
    double_sided: bool | None = None
    rim_amount: float | None = None


@dataclass(frozen=True, slots=True, kw_only=True)
class MeshIllustrationSvgOptions:
    coordinate_span: int | None = None
    title: str | None = None


# Serializable one-shot illustration input; reusable prepared scenes are opaque package objects.
@dataclass(frozen=True, slots=True, kw_only=True)
class MeshIllustrationInputA0:
    schema: Literal["geometry.mesh_illustration.input.a0"]
    meshes: tuple[MeshIllustrationMesh, ...]
    view: MeshIllustrationView
    prepare: MeshIllustrationPrepareOptions | None = None
    style: MeshIllustrationStyleA0 | None = None
    svg: MeshIllustrationSvgOptions | None = None


@dataclass(frozen=True, slots=True, kw_only=True)
class MeshIllustrationRenderStats:
    triangles: int
    surface_draws: int
    layered_surfaces: int
    outlines: int
    details: int
    creases: int
    commands: int


# One-shot SVG result. Canvas rendering returns through the direct package API.
@dataclass(frozen=True, slots=True, kw_only=True)
class MeshIllustrationResultA0:
    schema: Literal["geometry.mesh_illustration.result.a0"]
    svg: str
    stats: MeshIllustrationRenderStats
    warnings: tuple[str, ...]


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
class StepTopologyCloseResultA0:
    schema: Literal["geometry.step_topology.close.result.a0"]
    session_handle: str
    closed: Literal[True]


@dataclass(frozen=True, slots=True, kw_only=True)
class InspectionCounts:
    definitions: int
    root_occurrences: int
    component_occurrences: int
    bodies: int
    shells: int
    faces: int
    memberships: int


@dataclass(frozen=True, slots=True, kw_only=True)
class SourceEntityEvidence:
    mapped: bool
    shape_result_round_trip: bool
    model_number: int | None = None
    entity_type: str | None = None
    mapping_method: str | None = None


@dataclass(frozen=True, slots=True, kw_only=True)
class DefinitionSummary:
    handle: str
    name: str
    assembly: bool
    body_count: int
    face_count: int
    source_entity: SourceEntityEvidence | None = None


@dataclass(frozen=True, slots=True, kw_only=True)
class RootOccurrenceSummary:
    kind: Literal["root"]
    handle: str
    definition_handle: str
    name: str
    transform: tuple[float, ...]


@dataclass(frozen=True, slots=True, kw_only=True)
class ComponentOccurrenceSummary:
    kind: Literal["component"]
    handle: str
    definition_handle: str
    parent_occurrence_handle: str
    depth: int
    name: str
    transform: tuple[float, ...]


OccurrenceSummary: TypeAlias = RootOccurrenceSummary | ComponentOccurrenceSummary


@dataclass(frozen=True, slots=True, kw_only=True)
class BodySummary:
    handle: str
    definition_handle: str
    topology_kind: str
    shell_count: int
    face_count: int
    bounds_mm: tuple[float, ...]
    volume_mm3: float
    source_entity: SourceEntityEvidence | None = None


@dataclass(frozen=True, slots=True, kw_only=True)
class ShellSummary:
    handle: str
    definition_handle: str
    body_count: int
    face_count: int
    source_entity: SourceEntityEvidence | None = None


@dataclass(frozen=True, slots=True, kw_only=True)
class FaceSummary:
    handle: str
    definition_handle: str
    body_count: int
    shell_count: int
    bounds_mm: tuple[float, ...]
    area_mm2: float
    centroid_mm: tuple[float, ...]
    source_entity: SourceEntityEvidence | None = None


class TopologyMembershipKind(str, Enum):
    BODY_SHELL = "body_shell"
    BODY_FACE = "body_face"
    SHELL_FACE = "shell_face"


@dataclass(frozen=True, slots=True, kw_only=True)
class TopologyMembership:
    kind: TopologyMembershipKind
    owner_handle: str
    member_handle: str


@dataclass(frozen=True, slots=True, kw_only=True)
class TopologyPage:
    definitions: tuple[DefinitionSummary, ...]
    occurrences: tuple[OccurrenceSummary, ...]
    bodies: tuple[BodySummary, ...]
    shells: tuple[ShellSummary, ...]
    faces: tuple[FaceSummary, ...]
    memberships: tuple[TopologyMembership, ...]
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
class GlbAttachmentDescriptor:
    name: Literal["glb"]
    media_type: Literal["model/gltf-binary"]
    format: Literal["glb-2.0"]
    bytes: int
    sha256: str


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
class StepTopologyResolveHitResultA0:
    schema: Literal["geometry.step_topology.resolve_hit.result.a0"]
    session: SessionReference
    instance_index: int
    primitive_index: int
    triangle_index: int
    occurrence_handle: str
    body_handle: str
    face_handle: str


@dataclass(frozen=True, slots=True, kw_only=True)
class MutationSessionState:
    session: SessionReference
    edit_journal_revision: int
    accounted_string_bytes: int
    estimated_resident_bytes: int


@dataclass(frozen=True, slots=True, kw_only=True)
class LogicalGroupMember:
    kind: LogicalGroupMemberKind
    target_handle: str


@dataclass(frozen=True, slots=True, kw_only=True)
class LogicalGroup:
    authored_id: str
    revision: int
    name: str
    members: tuple[LogicalGroupMember, ...]


@dataclass(frozen=True, slots=True, kw_only=True)
class StepTopologyApplyLogicalGroupsResultA0:
    schema: Literal["geometry.step_topology.apply_logical_groups.result.a0"]
    state: MutationSessionState
    groups: tuple[LogicalGroup, ...]
    diagnostics: tuple[DiagnosticA0, ...]


@dataclass(frozen=True, slots=True, kw_only=True)
class MetadataProbe:
    authored_id: str
    revision: int
    target: MetadataProbeTarget
    key: str
    value: str


@dataclass(frozen=True, slots=True, kw_only=True)
class StepTopologyApplyMetadataProbesResultA0:
    schema: Literal["geometry.step_topology.apply_metadata_probes.result.a0"]
    state: MutationSessionState
    groups: tuple[LogicalGroup, ...]
    probes: tuple[MetadataProbe, ...]
    diagnostics: tuple[DiagnosticA0, ...]


@dataclass(frozen=True, slots=True, kw_only=True)
class EditJournalAttachmentDescriptor:
    name: Literal["edit_journal"]
    media_type: Literal["application/vnd.wavenumber.geometer.step-topology-edit-journal"]
    format: Literal["geometer.step_topology_edit_journal.a0"]
    bytes: int
    sha256: str


@dataclass(frozen=True, slots=True, kw_only=True)
class StepTopologyCheckpointEditJournalResultA0:
    schema: Literal["geometry.step_topology.checkpoint_edit_journal.result.a0"]
    state: MutationSessionState
    source_sha256: str
    source_brep_sha256: str
    target_inventory_sha256: str
    occt_version: str
    transaction_count: int
    journal: EditJournalAttachmentDescriptor
    diagnostics: tuple[DiagnosticA0, ...]


class HierarchyNodeKind(str, Enum):
    PRODUCT = "product"
    ASSEMBLY = "assembly"


@dataclass(frozen=True, slots=True, kw_only=True)
class HierarchyNode:
    authored_id: str
    revision: int
    kind: HierarchyNodeKind
    name: str
    source_kind: HierarchySourceKind | None = None
    source_handle: str | None = None


@dataclass(frozen=True, slots=True, kw_only=True)
class HierarchyOccurrence:
    authored_id: str
    revision: int
    child_authored_id: str
    parent_assembly_authored_id: str
    transform: tuple[float, ...]


@dataclass(frozen=True, slots=True, kw_only=True)
class HierarchyState:
    hierarchy_revision: int
    source_brep_sha256: str
    nodes: tuple[HierarchyNode, ...]
    occurrences: tuple[HierarchyOccurrence, ...]


@dataclass(frozen=True, slots=True, kw_only=True)
class StepTopologyApplyHierarchyResultA0:
    schema: Literal["geometry.step_topology.apply_hierarchy.result.a0"]
    state: MutationSessionState
    hierarchy: HierarchyState
    diagnostics: tuple[DiagnosticA0, ...]


SavePersistenceArtifact: TypeAlias = (
    XbfPersistenceArtifact | XmlXcafPersistenceArtifact | StepAp242PersistenceArtifact | JsonSidecarPersistenceArtifact
)


class PersistenceCarrier(str, Enum):
    XBF = "xbf"
    XML_XCAF = "xml_xcaf"
    STEP_AP242 = "step_ap242"
    JSON_SIDECAR = "json_sidecar"
    EDIT_JOURNAL = "edit_journal"


class CarrierSupportState(str, Enum):
    SUPPORTED = "supported"
    EXPERIMENTAL = "experimental"
    UNSUPPORTED = "unsupported"


@dataclass(frozen=True, slots=True, kw_only=True)
class CarrierCapabilityNote:
    value: str


@dataclass(frozen=True, slots=True, kw_only=True)
class CarrierCapability:
    carrier: PersistenceCarrier
    save: CarrierSupportState
    restore: CarrierSupportState
    authored_payload: CarrierSupportState
    topology_links: CarrierSupportState
    notes: tuple[CarrierCapabilityNote, ...]


@dataclass(frozen=True, slots=True, kw_only=True)
class StepTopologySaveResultA0:
    schema: Literal["geometry.step_topology.save.result.a0"]
    state: MutationSessionState
    source_sha256: str
    artifact: SavePersistenceArtifact
    capabilities: tuple[CarrierCapability, ...]
    diagnostics: tuple[DiagnosticA0, ...]


class RecoveryResolutionState(str, Enum):
    RESOLVED = "resolved"
    AMBIGUOUS = "ambiguous"
    UNRESOLVED = "unresolved"
    UNSUPPORTED = "unsupported"


class RecoveryGroupCompleteness(str, Enum):
    FULLY_RECOVERED = "fully_recovered"
    PARTIALLY_RECOVERED = "partially_recovered"
    UNRECOVERED = "unrecovered"
    UNSUPPORTED = "unsupported"


class RecoveryResolutionMethod(str, Enum):
    AUTHORED_ID_TOPOLOGY_LINK = "authored_id_topology_link"
    VALIDATED_CARRIER_LOCATOR = "validated_carrier_locator"
    UNIQUE_GEOMETRY_ADJACENCY_FINGERPRINT = "unique_geometry_adjacency_fingerprint"
    NONE = "none"


class RecoveryTopologyComparison(str, Enum):
    UNCHANGED = "unchanged"
    RELOCATED = "relocated"
    SPLIT = "split"
    MERGED = "merged"
    OTHERWISE_CHANGED = "otherwise_changed"
    NOT_COMPARED = "not_compared"
    UNAVAILABLE = "unavailable"


class RecoveryConfidence(str, Enum):
    HIGH = "high"
    MEDIUM = "medium"
    LOW = "low"
    NONE = "none"


@dataclass(frozen=True, slots=True, kw_only=True)
class RecoveryComparedField:
    value: str


@dataclass(frozen=True, slots=True, kw_only=True)
class RecoveryCarrierRecord:
    value: str


@dataclass(frozen=True, slots=True, kw_only=True)
class RecoveryRejectedAlternative:
    target_handle: str
    reason: str


@dataclass(frozen=True, slots=True, kw_only=True)
class RecoveryEvidence:
    candidate_count: int
    matching_candidate_count: int
    compared_fields: tuple[RecoveryComparedField, ...]
    tolerances: RecoveryTolerances
    carrier_records: tuple[RecoveryCarrierRecord, ...]
    rejected_alternatives: tuple[RecoveryRejectedAlternative, ...]


@dataclass(frozen=True, slots=True, kw_only=True)
class RecoveryMemberResult:
    member_record_id: str
    kind: LogicalGroupMemberKind
    authored_target_id: str
    resolution_state: RecoveryResolutionState
    resolution_method: RecoveryResolutionMethod
    topology_comparison: RecoveryTopologyComparison
    confidence: RecoveryConfidence
    resolved_target_handle: str | None = None
    evidence: RecoveryEvidence


@dataclass(frozen=True, slots=True, kw_only=True)
class RecoveryGroupResult:
    group_authored_id: str
    provenance: RecoveryProvenance
    resolution_state: RecoveryResolutionState
    completeness: RecoveryGroupCompleteness
    resolved_member_count: int
    ambiguous_member_count: int
    unresolved_member_count: int
    unsupported_member_count: int
    members: tuple[RecoveryMemberResult, ...]


@dataclass(frozen=True, slots=True, kw_only=True)
class StepTopologyRestoreResultA0:
    schema: Literal["geometry.step_topology.restore.result.a0"]
    session: SessionReference
    source: SourceDescriptor
    tool: ToolDescriptor
    replayed_transaction_count: int
    evicted_session_handles: tuple[str, ...] | None = None
    recovery: tuple[RecoveryGroupResult, ...]
    diagnostics: tuple[DiagnosticA0, ...]


@dataclass(frozen=True, slots=True, kw_only=True)
class StepTopologyAnalyzeRecoveryResultA0:
    schema: Literal["geometry.step_topology.analyze_recovery.result.a0"]
    groups: tuple[RecoveryGroupResult, ...]
    diagnostics: tuple[DiagnosticA0, ...]


# Structurally representable operation results. A result variant may belong to a runtime-unavailable experimental operation and is not an availability claim; the negotiated operation catalog remains authoritative.
OperationResultValueA0: TypeAlias = (
    ModelBoundsResultA0
    | HlrProjectionResultA0
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
    | StepTopologyAnalyzeRecoveryResultA0
)


# A completed operation with its operation-specific result.
@dataclass(frozen=True, slots=True, kw_only=True)
class OperationSuccessA0:
    operation: str
    ok: Literal[True]
    result: OperationResultValueA0


# Transport-neutral typed outcome shared by the generic C ABI and executable IPC.
OperationOutcomeA0: TypeAlias = OperationSuccessA0 | OperationFailureA0

MODEL_TYPES = {
    "Wavenumber.Geometer.Contracts.Common.DiagnosticA0": DiagnosticA0,
    "Wavenumber.Geometer.Contracts.Common.PackedAttachmentProjectionA0": PackedAttachmentProjectionA0,
    "Wavenumber.Geometer.Contracts.Common.PackedAttachmentReferenceA0": PackedAttachmentReferenceA0,
    "Wavenumber.Geometer.Contracts.HlrProjectionA0.FastHlrLimitsA0": FastHlrLimitsA0,
    "Wavenumber.Geometer.Contracts.HlrProjectionA0.FastHlrOptionsA0": FastHlrOptionsA0,
    "Wavenumber.Geometer.Contracts.HlrProjectionA0.HlrProjectedView": HlrProjectedView,
    "Wavenumber.Geometer.Contracts.HlrProjectionA0.HlrProjectionModes": HlrProjectionModes,
    "Wavenumber.Geometer.Contracts.HlrProjectionA0.HlrProjectionOptionsA0": HlrProjectionOptionsA0,
    "Wavenumber.Geometer.Contracts.HlrProjectionA0.HlrProjectionResultA0": HlrProjectionResultA0,
    "Wavenumber.Geometer.Contracts.HlrProjectionA0.HlrProjectionSource": HlrProjectionSource,
    "Wavenumber.Geometer.Contracts.HlrProjectionA0.HlrProjectionTimings": HlrProjectionTimings,
    "Wavenumber.Geometer.Contracts.HlrProjectionA0.HlrViewSpec": HlrViewSpec,
    "Wavenumber.Geometer.Contracts.HlrProjectionA0.ProjectedArc": ProjectedArc,
    "Wavenumber.Geometer.Contracts.HlrProjectionA0.ProjectedGeometry": ProjectedGeometry,
    "Wavenumber.Geometer.Contracts.HlrProjectionA0.ProjectionBounds": ProjectionBounds,
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
    "Wavenumber.Geometer.Contracts.MeshIllustrationA0.MeshIllustrationInputA0": MeshIllustrationInputA0,
    "Wavenumber.Geometer.Contracts.MeshIllustrationA0.MeshIllustrationMaterial": MeshIllustrationMaterial,
    "Wavenumber.Geometer.Contracts.MeshIllustrationA0.MeshIllustrationMesh": MeshIllustrationMesh,
    "Wavenumber.Geometer.Contracts.MeshIllustrationA0.MeshIllustrationPrepareOptions": MeshIllustrationPrepareOptions,
    "Wavenumber.Geometer.Contracts.MeshIllustrationA0.MeshIllustrationRenderStats": MeshIllustrationRenderStats,
    "Wavenumber.Geometer.Contracts.MeshIllustrationA0.MeshIllustrationResultA0": MeshIllustrationResultA0,
    "Wavenumber.Geometer.Contracts.MeshIllustrationA0.MeshIllustrationStyleA0": MeshIllustrationStyleA0,
    "Wavenumber.Geometer.Contracts.MeshIllustrationA0.MeshIllustrationSvgOptions": MeshIllustrationSvgOptions,
    "Wavenumber.Geometer.Contracts.MeshIllustrationA0.MeshIllustrationView": MeshIllustrationView,
    "Wavenumber.Geometer.Contracts.ModelBoundsA0.ModelBoundsOptionsA0": ModelBoundsOptionsA0,
    "Wavenumber.Geometer.Contracts.ModelBoundsA0.ModelBoundsResultA0": ModelBoundsResultA0,
    "Wavenumber.Geometer.Contracts.ModelBoundsA0.ModelBoundsSource": ModelBoundsSource,
    "Wavenumber.Geometer.Contracts.ModelBoundsA0.ModelBoundsTimings": ModelBoundsTimings,
    "Wavenumber.Geometer.Contracts.ModelBoundsA0.ModelBoundsValues": ModelBoundsValues,
    "Wavenumber.Geometer.Contracts.OperationOutcomeA0.OperationFailureA0": OperationFailureA0,
    "Wavenumber.Geometer.Contracts.OperationOutcomeA0.OperationSuccessA0": OperationSuccessA0,
    "Wavenumber.Geometer.Contracts.StepTopologyA0.AttachMetadataProbeCommand": AttachMetadataProbeCommand,
    "Wavenumber.Geometer.Contracts.StepTopologyA0.BodyProbeTarget": BodyProbeTarget,
    "Wavenumber.Geometer.Contracts.StepTopologyA0.BodySummary": BodySummary,
    "Wavenumber.Geometer.Contracts.StepTopologyA0.CarrierCapability": CarrierCapability,
    "Wavenumber.Geometer.Contracts.StepTopologyA0.CarrierCapabilityNote": CarrierCapabilityNote,
    "Wavenumber.Geometer.Contracts.StepTopologyA0.ComponentOccurrenceProbeTarget": ComponentOccurrenceProbeTarget,
    "Wavenumber.Geometer.Contracts.StepTopologyA0.ComponentOccurrenceSummary": ComponentOccurrenceSummary,
    "Wavenumber.Geometer.Contracts.StepTopologyA0.CreateHierarchyAssemblyCommand": CreateHierarchyAssemblyCommand,
    "Wavenumber.Geometer.Contracts.StepTopologyA0.CreateHierarchyOccurrenceCommand": CreateHierarchyOccurrenceCommand,
    "Wavenumber.Geometer.Contracts.StepTopologyA0.CreateHierarchyProductCommand": CreateHierarchyProductCommand,
    "Wavenumber.Geometer.Contracts.StepTopologyA0.CreateLogicalGroupCommand": CreateLogicalGroupCommand,
    "Wavenumber.Geometer.Contracts.StepTopologyA0.DefinitionProbeTarget": DefinitionProbeTarget,
    "Wavenumber.Geometer.Contracts.StepTopologyA0.DefinitionSummary": DefinitionSummary,
    "Wavenumber.Geometer.Contracts.StepTopologyA0.DocumentProbeTarget": DocumentProbeTarget,
    "Wavenumber.Geometer.Contracts.StepTopologyA0.EditJournalAttachmentDescriptor": EditJournalAttachmentDescriptor,
    "Wavenumber.Geometer.Contracts.StepTopologyA0.EditJournalPersistenceArtifact": EditJournalPersistenceArtifact,
    "Wavenumber.Geometer.Contracts.StepTopologyA0.EditJournalReplayPreconditions": EditJournalReplayPreconditions,
    "Wavenumber.Geometer.Contracts.StepTopologyA0.EraseHierarchyNodeCommand": EraseHierarchyNodeCommand,
    "Wavenumber.Geometer.Contracts.StepTopologyA0.EraseHierarchyOccurrenceCommand": EraseHierarchyOccurrenceCommand,
    "Wavenumber.Geometer.Contracts.StepTopologyA0.EraseLogicalGroupCommand": EraseLogicalGroupCommand,
    "Wavenumber.Geometer.Contracts.StepTopologyA0.EraseMetadataProbeCommand": EraseMetadataProbeCommand,
    "Wavenumber.Geometer.Contracts.StepTopologyA0.FaceProbeTarget": FaceProbeTarget,
    "Wavenumber.Geometer.Contracts.StepTopologyA0.FaceSummary": FaceSummary,
    "Wavenumber.Geometer.Contracts.StepTopologyA0.GlbAttachmentDescriptor": GlbAttachmentDescriptor,
    "Wavenumber.Geometer.Contracts.StepTopologyA0.HierarchyNode": HierarchyNode,
    "Wavenumber.Geometer.Contracts.StepTopologyA0.HierarchyOccurrence": HierarchyOccurrence,
    "Wavenumber.Geometer.Contracts.StepTopologyA0.HierarchyState": HierarchyState,
    "Wavenumber.Geometer.Contracts.StepTopologyA0.InspectionCounts": InspectionCounts,
    "Wavenumber.Geometer.Contracts.StepTopologyA0.JsonSidecarPersistenceArtifact": JsonSidecarPersistenceArtifact,
    "Wavenumber.Geometer.Contracts.StepTopologyA0.LogicalGroup": LogicalGroup,
    "Wavenumber.Geometer.Contracts.StepTopologyA0.LogicalGroupMember": LogicalGroupMember,
    "Wavenumber.Geometer.Contracts.StepTopologyA0.LogicalGroupProbeTarget": LogicalGroupProbeTarget,
    "Wavenumber.Geometer.Contracts.StepTopologyA0.MetadataProbe": MetadataProbe,
    "Wavenumber.Geometer.Contracts.StepTopologyA0.MutationSessionState": MutationSessionState,
    "Wavenumber.Geometer.Contracts.StepTopologyA0.PageRequest": PageRequest,
    "Wavenumber.Geometer.Contracts.StepTopologyA0.RecoveryCandidate": RecoveryCandidate,
    "Wavenumber.Geometer.Contracts.StepTopologyA0.RecoveryCarrierRecord": RecoveryCarrierRecord,
    "Wavenumber.Geometer.Contracts.StepTopologyA0.RecoveryComparedField": RecoveryComparedField,
    "Wavenumber.Geometer.Contracts.StepTopologyA0.RecoveryEvidence": RecoveryEvidence,
    "Wavenumber.Geometer.Contracts.StepTopologyA0.RecoveryFingerprint": RecoveryFingerprint,
    "Wavenumber.Geometer.Contracts.StepTopologyA0.RecoveryGroupRequest": RecoveryGroupRequest,
    "Wavenumber.Geometer.Contracts.StepTopologyA0.RecoveryGroupResult": RecoveryGroupResult,
    "Wavenumber.Geometer.Contracts.StepTopologyA0.RecoveryMemberRequest": RecoveryMemberRequest,
    "Wavenumber.Geometer.Contracts.StepTopologyA0.RecoveryMemberResult": RecoveryMemberResult,
    "Wavenumber.Geometer.Contracts.StepTopologyA0.RecoveryProvenance": RecoveryProvenance,
    "Wavenumber.Geometer.Contracts.StepTopologyA0.RecoveryRejectedAlternative": RecoveryRejectedAlternative,
    "Wavenumber.Geometer.Contracts.StepTopologyA0.RecoveryTolerances": RecoveryTolerances,
    "Wavenumber.Geometer.Contracts.StepTopologyA0.RenameHierarchyNodeCommand": RenameHierarchyNodeCommand,
    "Wavenumber.Geometer.Contracts.StepTopologyA0.RenameLogicalGroupCommand": RenameLogicalGroupCommand,
    "Wavenumber.Geometer.Contracts.StepTopologyA0.RenderArtifactDescriptor": RenderArtifactDescriptor,
    "Wavenumber.Geometer.Contracts.StepTopologyA0.RenderCounts": RenderCounts,
    "Wavenumber.Geometer.Contracts.StepTopologyA0.ReparentHierarchyOccurrenceCommand": ReparentHierarchyOccurrenceCommand,
    "Wavenumber.Geometer.Contracts.StepTopologyA0.ReplaceLogicalGroupMembersCommand": ReplaceLogicalGroupMembersCommand,
    "Wavenumber.Geometer.Contracts.StepTopologyA0.ReplaceMetadataProbeCommand": ReplaceMetadataProbeCommand,
    "Wavenumber.Geometer.Contracts.StepTopologyA0.RootOccurrenceProbeTarget": RootOccurrenceProbeTarget,
    "Wavenumber.Geometer.Contracts.StepTopologyA0.RootOccurrenceSummary": RootOccurrenceSummary,
    "Wavenumber.Geometer.Contracts.StepTopologyA0.SessionReference": SessionReference,
    "Wavenumber.Geometer.Contracts.StepTopologyA0.ShellSummary": ShellSummary,
    "Wavenumber.Geometer.Contracts.StepTopologyA0.SourceDescriptor": SourceDescriptor,
    "Wavenumber.Geometer.Contracts.StepTopologyA0.SourceEntityEvidence": SourceEntityEvidence,
    "Wavenumber.Geometer.Contracts.StepTopologyA0.StepAp242PersistenceArtifact": StepAp242PersistenceArtifact,
    "Wavenumber.Geometer.Contracts.StepTopologyA0.StepTopologyAnalyzeRecoveryRequestA0": StepTopologyAnalyzeRecoveryRequestA0,
    "Wavenumber.Geometer.Contracts.StepTopologyA0.StepTopologyAnalyzeRecoveryResultA0": StepTopologyAnalyzeRecoveryResultA0,
    "Wavenumber.Geometer.Contracts.StepTopologyA0.StepTopologyApplyHierarchyRequestA0": StepTopologyApplyHierarchyRequestA0,
    "Wavenumber.Geometer.Contracts.StepTopologyA0.StepTopologyApplyHierarchyResultA0": StepTopologyApplyHierarchyResultA0,
    "Wavenumber.Geometer.Contracts.StepTopologyA0.StepTopologyApplyLogicalGroupsRequestA0": StepTopologyApplyLogicalGroupsRequestA0,
    "Wavenumber.Geometer.Contracts.StepTopologyA0.StepTopologyApplyLogicalGroupsResultA0": StepTopologyApplyLogicalGroupsResultA0,
    "Wavenumber.Geometer.Contracts.StepTopologyA0.StepTopologyApplyMetadataProbesRequestA0": StepTopologyApplyMetadataProbesRequestA0,
    "Wavenumber.Geometer.Contracts.StepTopologyA0.StepTopologyApplyMetadataProbesResultA0": StepTopologyApplyMetadataProbesResultA0,
    "Wavenumber.Geometer.Contracts.StepTopologyA0.StepTopologyCheckpointEditJournalRequestA0": StepTopologyCheckpointEditJournalRequestA0,
    "Wavenumber.Geometer.Contracts.StepTopologyA0.StepTopologyCheckpointEditJournalResultA0": StepTopologyCheckpointEditJournalResultA0,
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
    "Wavenumber.Geometer.Contracts.StepTopologyA0.StepTopologyRestoreRequestA0": StepTopologyRestoreRequestA0,
    "Wavenumber.Geometer.Contracts.StepTopologyA0.StepTopologyRestoreResultA0": StepTopologyRestoreResultA0,
    "Wavenumber.Geometer.Contracts.StepTopologyA0.StepTopologySaveRequestA0": StepTopologySaveRequestA0,
    "Wavenumber.Geometer.Contracts.StepTopologyA0.StepTopologySaveResultA0": StepTopologySaveResultA0,
    "Wavenumber.Geometer.Contracts.StepTopologyA0.TessellationOptions": TessellationOptions,
    "Wavenumber.Geometer.Contracts.StepTopologyA0.ToolDescriptor": ToolDescriptor,
    "Wavenumber.Geometer.Contracts.StepTopologyA0.TopologyBindingTableAttachmentDescriptor": TopologyBindingTableAttachmentDescriptor,
    "Wavenumber.Geometer.Contracts.StepTopologyA0.TopologyMembership": TopologyMembership,
    "Wavenumber.Geometer.Contracts.StepTopologyA0.TopologyPage": TopologyPage,
    "Wavenumber.Geometer.Contracts.StepTopologyA0.TopologyTableAttachmentDescriptor": TopologyTableAttachmentDescriptor,
    "Wavenumber.Geometer.Contracts.StepTopologyA0.XbfPersistenceArtifact": XbfPersistenceArtifact,
    "Wavenumber.Geometer.Contracts.StepTopologyA0.XmlXcafPersistenceArtifact": XmlXcafPersistenceArtifact,
}

ENUM_TYPES = {
    "Wavenumber.Geometer.Contracts.Common.DiagnosticCategory": DiagnosticCategory,
    "Wavenumber.Geometer.Contracts.HlrProjectionA0.HlrCurveMode": HlrCurveMode,
    "Wavenumber.Geometer.Contracts.HlrProjectionA0.HlrMeshDeflectionMode": HlrMeshDeflectionMode,
    "Wavenumber.Geometer.Contracts.HlrProjectionA0.HlrOutlineAlgorithm": HlrOutlineAlgorithm,
    "Wavenumber.Geometer.Contracts.HlrProjectionA0.HlrProjectionAlgorithm": HlrProjectionAlgorithm,
    "Wavenumber.Geometer.Contracts.HlrProjectionA0.HlrSourceKind": HlrSourceKind,
    "Wavenumber.Geometer.Contracts.IpcA0.IpcRuntimeDispatchA0": IpcRuntimeDispatchA0,
    "Wavenumber.Geometer.Contracts.MeshIllustrationA0.MeshIllustrationShading": MeshIllustrationShading,
    "Wavenumber.Geometer.Contracts.ModelBoundsA0.ModelFormat": ModelFormat,
    "Wavenumber.Geometer.Contracts.StepTopologyA0.CarrierSupportState": CarrierSupportState,
    "Wavenumber.Geometer.Contracts.StepTopologyA0.HierarchyNodeKind": HierarchyNodeKind,
    "Wavenumber.Geometer.Contracts.StepTopologyA0.HierarchySourceKind": HierarchySourceKind,
    "Wavenumber.Geometer.Contracts.StepTopologyA0.LogicalGroupMemberKind": LogicalGroupMemberKind,
    "Wavenumber.Geometer.Contracts.StepTopologyA0.PersistenceCarrier": PersistenceCarrier,
    "Wavenumber.Geometer.Contracts.StepTopologyA0.RecoveryConfidence": RecoveryConfidence,
    "Wavenumber.Geometer.Contracts.StepTopologyA0.RecoveryGroupCompleteness": RecoveryGroupCompleteness,
    "Wavenumber.Geometer.Contracts.StepTopologyA0.RecoveryLineage": RecoveryLineage,
    "Wavenumber.Geometer.Contracts.StepTopologyA0.RecoveryResolutionMethod": RecoveryResolutionMethod,
    "Wavenumber.Geometer.Contracts.StepTopologyA0.RecoveryResolutionState": RecoveryResolutionState,
    "Wavenumber.Geometer.Contracts.StepTopologyA0.RecoveryTopologyComparison": RecoveryTopologyComparison,
    "Wavenumber.Geometer.Contracts.StepTopologyA0.SaveCarrier": SaveCarrier,
    "Wavenumber.Geometer.Contracts.StepTopologyA0.TopologyMembershipKind": TopologyMembershipKind,
}
