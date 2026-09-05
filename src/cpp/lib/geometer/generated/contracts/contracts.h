// Generated from wn_geometer_contract_catalog.a0.json. Do not edit.
#pragma once

#include <cstddef>
#include <cstdint>
#include <optional>
#include <string>
#include <variant>
#include <vector>

namespace geometer::contracts
{

struct ContractError
{
    std::string code;
    std::string path;
    std::string message;
};

using JobId = std::uint64_t;

using StageId = std::uint64_t;

enum class StageOperation
{
    union_stage,
    difference,
};

using OperandId = std::uint64_t;

using RegionId = std::uint64_t;

using RingId = std::uint64_t;

using VertexId = std::uint64_t;

struct PointNm
{
    std::int64_t x{};
    std::int64_t y{};
};

struct AuthoredVertex
{
    VertexId vertex_id{};
    PointNm point{};
};

using SegmentId = std::uint64_t;

using CurveId = std::uint64_t;

struct AuthoredLineSegment
{
    SegmentId segment_id{};
    CurveId curve_id{};
    std::string kind = "line";
};

enum class ArcDirection
{
    ccw,
    cw,
};

struct AuthoredCircularArcSegment
{
    SegmentId segment_id{};
    CurveId curve_id{};
    std::string kind = "circular_arc";
    PointNm center{};
    ArcDirection direction{};
    bool major_arc{};
};

struct AuthoredCircularArcByRadiusSegment
{
    SegmentId segment_id{};
    CurveId curve_id{};
    std::string kind = "circular_arc_by_radius";
    std::uint64_t radius_nm{};
    ArcDirection direction{};
    bool major_arc{};
};

using AuthoredSegment = std::variant<AuthoredLineSegment, AuthoredCircularArcSegment,
                                     AuthoredCircularArcByRadiusSegment>;

struct PlanarRing
{
    RingId ring_id{};
    std::vector<AuthoredVertex> vertices{};
    std::vector<AuthoredSegment> segments{};
};

struct PlanarRegionOperand
{
    OperandId operand_id{};
    std::string kind = "planar_region";
    RegionId region_id{};
    PlanarRing outer{};
    std::vector<PlanarRing> holes{};
};

using FeatureId = std::uint64_t;

struct DiskOperand
{
    OperandId operand_id{};
    std::string kind = "disk";
    FeatureId feature_id{};
    PointNm center{};
    std::uint64_t radius_nm{};
};

struct AnnulusOperand
{
    OperandId operand_id{};
    std::string kind = "annulus";
    FeatureId feature_id{};
    PointNm center{};
    std::uint64_t inner_radius_nm{};
    std::uint64_t outer_radius_nm{};
};

struct CapsuleOperand
{
    OperandId operand_id{};
    std::string kind = "capsule";
    FeatureId feature_id{};
    PointNm start{};
    PointNm end{};
    std::uint64_t width_nm{};
};

using PathId = std::uint64_t;

using AuthoredPathSegment = std::variant<AuthoredLineSegment, AuthoredCircularArcSegment>;

struct PlanarPath
{
    PathId path_id{};
    std::vector<AuthoredVertex> vertices{};
    std::vector<AuthoredPathSegment> segments{};
};

struct SweptPathOperand
{
    OperandId operand_id{};
    std::string kind = "swept_path";
    FeatureId feature_id{};
    PlanarPath centerline{};
    std::uint64_t width_nm{};
    std::string cap = "round";
    std::string join = "round";
};

using AnalyticPlanarOperand = std::variant<PlanarRegionOperand, DiskOperand, AnnulusOperand,
                                           CapsuleOperand, SweptPathOperand>;

struct AnalyticPlanarBooleanStage
{
    StageId stage_id{};
    StageOperation operation{};
    std::vector<AnalyticPlanarOperand> operands{};
};

struct AnalyticPlanarBooleanJob
{
    JobId job_id{};
    std::vector<AnalyticPlanarBooleanStage> stages{};
};

using QueryId = std::uint64_t;

struct PlanarRelationshipQuery
{
    QueryId query_id{};
    JobId left_job_id{};
    JobId right_job_id{};
};

struct AnalyticPlanarBooleanBatchRequestA0
{
    std::vector<AnalyticPlanarBooleanJob> jobs{};
    std::vector<PlanarRelationshipQuery> relationship_queries{};
};

enum class JobDiagnosticCode
{
    invalid_topology,
    invalid_arc,
    unsupported_geometry,
    normalization_error_exceeded,
    normalization_topology_collapse,
    nonanalytic_result,
    solver_failed,
    resource_limit_exceeded,
    resolution_coalesced,
};

enum class DiagnosticSeverity
{
    error,
    warning,
};

enum class JobDiagnosticPath
{
    request_jobs,
    job_id,
    job_stages,
    stage_id,
    stage_operation,
    stage_operands,
    operand_id,
    operand_geometry,
    region_outer,
    region_holes,
    ring_vertices,
    ring_segments,
    path_vertices,
    path_segments,
    segment_curve,
    disk_radius,
    annulus_inner_radius,
    annulus_outer_radius,
    capsule_start,
    capsule_end,
    capsule_width,
    swept_path_centerline,
    swept_path_width,
    relationship_queries,
    relationship_left_job_id,
    relationship_right_job_id,
};

struct JobDiagnostic
{
    JobDiagnosticCode code{};
    DiagnosticSeverity severity{};
    JobId job_id{};
    std::optional<StageId> stage_id{};
    std::optional<OperandId> operand_id{};
    std::optional<std::uint64_t> geometry_id{};
    std::optional<JobDiagnosticPath> path_identity{};
};

using ResultVertexId = std::uint64_t;

enum class SourceKind
{
    authored_segment_curve,
    compact_feature_role,
    subtractive_operand_effect,
};

enum class SourceRole
{
    none,
    authored_line,
    authored_circular_arc,
    primitive_outer_circle,
    primitive_inner_circle,
    capsule_left_line,
    capsule_end_cap,
    capsule_right_line,
    capsule_start_cap,
    swept_left_offset_line,
    swept_left_offset_arc,
    swept_right_offset_line,
    swept_right_offset_arc,
    swept_round_join,
    swept_start_cap,
    swept_end_cap,
};

struct SourceReference
{
    SourceKind kind{};
    SourceRole role{};
    OperandId operand_id{};
    std::uint64_t primary_id{};
    std::uint64_t secondary_id{};
};

struct SourceSet
{
    std::vector<SourceReference> sources{};
};

struct ResultVertex
{
    ResultVertexId vertex_id{};
    PointNm point{};
    SourceSet intersection_sources{};
};

using ResultFragmentId = std::uint64_t;

struct ResultLineFragment
{
    ResultFragmentId fragment_id{};
    std::string kind = "line";
    ResultVertexId start_vertex_id{};
    ResultVertexId end_vertex_id{};
    SourceSet coincident_positive_sources{};
    SourceSet surviving_subtraction_sources{};
};

struct ResultCircularArcFragment
{
    ResultFragmentId fragment_id{};
    std::string kind = "circular_arc";
    ResultVertexId start_vertex_id{};
    ResultVertexId end_vertex_id{};
    std::uint64_t radius_nm{};
    ArcDirection direction{};
    bool major_arc{};
    SourceSet coincident_positive_sources{};
    SourceSet surviving_subtraction_sources{};
};

using DirectedFragment = std::variant<ResultLineFragment, ResultCircularArcFragment>;

using ResultRingId = std::uint64_t;

struct ResultRing
{
    ResultRingId ring_id{};
    std::vector<ResultFragmentId> fragment_ids{};
    std::optional<ResultRingId> parent_ring_id{};
    std::uint32_t depth{};
    bool hole{};
};

using ResultRegionId = std::uint64_t;

struct ResultRegion
{
    ResultRegionId result_region_id{};
    ResultRingId outer_ring_id{};
    SourceSet positive_contributors{};
};

enum class OperandOutcomeKind
{
    contributes_final_material,
    redundant_or_absorbed_coverage,
    partially_removed_later,
    completely_removed_later,
    subtraction_effect_survives,
    subtraction_effect_overwritten_later,
    no_effect,
};

struct OperandOutcomeEvent
{
    OperandId operand_id{};
    OperandOutcomeKind kind{};
    std::vector<ResultRingId> result_ring_ids{};
    std::vector<ResultRegionId> result_region_ids{};
    SourceSet sources{};
};

struct SuccessfulJobResult
{
    JobId job_id{};
    std::string status = "success";
    std::vector<JobDiagnostic> diagnostics{};
    std::vector<ResultVertex> vertices{};
    std::vector<DirectedFragment> directed_fragments{};
    std::vector<ResultRing> rings{};
    std::vector<ResultRegion> result_regions{};
    std::vector<OperandOutcomeEvent> operand_outcomes{};
    std::string digest_sha256{};
};

struct FailedJobResult
{
    JobId job_id{};
    std::string status = "failure";
    std::vector<JobDiagnostic> diagnostics{};
    std::string digest_sha256{};
};

using AnalyticPlanarBooleanJobResult = std::variant<SuccessfulJobResult, FailedJobResult>;

enum class RelationshipStatus
{
    success,
    skipped_dependency_failed,
};

enum class IntersectionDimension
{
    disjoint,
    point,
    curve,
    area,
};

struct RelationshipRegionPair
{
    ResultRegionId left_result_region_id{};
    ResultRegionId right_result_region_id{};
    IntersectionDimension dimension{};
    bool equality{};
    bool left_contains_right{};
    bool right_contains_left{};
};

struct PlanarRelationshipResult
{
    QueryId query_id{};
    RelationshipStatus status{};
    IntersectionDimension aggregate_dimension{};
    std::vector<RelationshipRegionPair> pairs{};
};

struct AnalyticPlanarBooleanBatchResultA0
{
    std::vector<AnalyticPlanarBooleanJobResult> job_results{};
    std::vector<PlanarRelationshipResult> relationship_results{};
};

enum class DiagnosticCategory
{
    transport,
    contract,
    operation,
};

struct DiagnosticA0
{
    std::string code{};
    DiagnosticCategory category{};
    std::string message{};
    bool retryable{};
    std::optional<std::string> path{};
    std::optional<std::string> operation{};
    std::optional<std::string> request_id{};
};

struct PackedAttachmentReferenceA0
{
    std::string attachment{};
    std::string format{};
};

struct PackedAttachmentProjectionA0
{
    std::string schema{};
    PackedAttachmentReferenceA0 packet{};
};

struct FastHlrLimitsA0
{
    std::optional<std::uint32_t> max_vertices{};
    std::optional<std::uint32_t> max_triangles{};
    std::optional<std::uint32_t> max_edges{};
    std::optional<std::uint32_t> max_grid_references{};
    std::optional<std::uint32_t> max_candidate_pairs{};
    std::optional<std::uint32_t> max_fragments{};
    std::optional<std::uint32_t> max_output_segments{};
};

struct FastHlrOptionsA0
{
    std::optional<bool> include_boundaries{};
    std::optional<bool> include_creases{};
    std::optional<bool> include_silhouettes{};
    std::optional<bool> include_hidden{};
    std::optional<bool> suppress_coplanar_seams{};
    std::optional<double> crease_angle_rad{};
    std::optional<double> weld_tolerance{};
    std::optional<double> projected_tolerance{};
    std::optional<double> depth_tolerance{};
    std::optional<double> coplanar_seam_angle_rad{};
    std::optional<double> coplanar_seam_depth_tolerance{};
    std::optional<double> coplanar_seam_lateral_tolerance{};
    std::optional<FastHlrLimitsA0> limits{};
};

enum class HlrCurveMode
{
    native_arcs,
    polyline,
};

using HlrMatrix4x4 = std::vector<double>;

enum class HlrMeshDeflectionMode
{
    absolute,
    bbox_relative,
};

enum class HlrOutlineAlgorithm
{
    hlr_close,
    mesh_shadow,
    fast_mesh_shadow,
};

using HlrVector3 = std::vector<double>;

using ProjectedSegment = std::vector<double>;

using HlrVector2 = std::vector<double>;

struct ProjectedArc
{
    HlrVector2 start{};
    HlrVector2 end{};
    HlrVector2 center{};
    double radius{};
    double extent_rad{};
    bool ccw{};
    bool full_circle{};
};

struct ProjectionBounds
{
    double min_x{};
    double min_y{};
    double max_x{};
    double max_y{};
    double width{};
    double height{};
};

struct ProjectedGeometry
{
    std::vector<ProjectedSegment> segments{};
    std::vector<ProjectedArc> arcs{};
    std::optional<ProjectionBounds> bounds{};
};

struct HlrProjectionModes
{
    ProjectedGeometry outline{};
    ProjectedGeometry detail{};
    ProjectedGeometry bbox{};
};

struct HlrProjectedView
{
    std::string id{};
    HlrVector3 direction{};
    HlrVector3 up{};
    HlrProjectionModes modes{};
};

enum class HlrProjectionAlgorithm
{
    poly,
    exact,
    fast,
};

struct HlrViewSpec
{
    std::string id{};
    HlrVector3 direction{};
    HlrVector3 up{};
};

struct HlrProjectionOptionsA0
{
    std::optional<std::vector<HlrViewSpec>> views{};
    std::optional<bool> output_outline{};
    std::optional<bool> output_detail{};
    std::optional<bool> output_bbox{};
    std::optional<HlrMatrix4x4> model_transform{};
    std::optional<bool> strip_root_placement{};
    std::optional<HlrCurveMode> curve_mode{};
    std::optional<std::uint32_t> samples_per_curve{};
    std::optional<std::uint32_t> round_digits{};
    std::optional<bool> edge_v_sharp{};
    std::optional<bool> edge_v_outline{};
    std::optional<bool> edge_v_smooth{};
    std::optional<bool> edge_v_sewn{};
    std::optional<bool> edge_v_iso{};
    std::optional<bool> edge_h_sharp{};
    std::optional<bool> edge_h_outline{};
    std::optional<bool> edge_h_smooth{};
    std::optional<bool> edge_h_sewn{};
    std::optional<bool> edge_h_iso{};
    std::optional<bool> union_outline_polygons{};
    std::optional<HlrProjectionAlgorithm> projection_algorithm{};
    std::optional<double> mesh_linear_deflection{};
    std::optional<double> mesh_angular_deflection{};
    std::optional<bool> mesh_relative{};
    std::optional<HlrMeshDeflectionMode> mesh_deflection_mode{};
    std::optional<double> mesh_deflection_coefficient{};
    std::optional<HlrOutlineAlgorithm> outline_algorithm{};
    std::optional<double> hlr_angle_tolerance{};
    std::optional<FastHlrOptionsA0> fast{};
};

enum class HlrSourceKind
{
    step,
    indexed_mesh,
};

struct HlrProjectionSource
{
    HlrSourceKind kind{};
    std::string hash{};
};

struct HlrProjectionTimings
{
    double step_read_ms{};
    double mesh_ms{};
    double hlr_ms{};
    double extract_ms{};
};

struct HlrProjectionResultA0
{
    std::string schema = "geometry.hlr_projection.result.a0";
    std::string units = "mm";
    HlrProjectionSource source{};
    std::vector<HlrProjectedView> views{};
    HlrProjectionTimings timings{};
};

struct IpcAttachmentDeclarationA0
{
    std::string name{};
    bool required{};
    std::vector<std::string> media_types{};
    std::uint32_t max_bytes{};
};

struct IpcAttachmentOffsetsWasm32A0
{
    std::uint32_t struct_size{};
    std::uint32_t flags{};
    std::uint32_t name{};
    std::uint32_t name_size{};
    std::uint32_t media_type{};
    std::uint32_t media_type_size{};
    std::uint32_t data{};
    std::uint32_t data_size{};
    std::uint32_t reserved0{};
};

struct IpcAttachmentLayoutWasm32A0
{
    std::uint32_t size{};
    IpcAttachmentOffsetsWasm32A0 offsets{};
};

struct IpcAttachmentOffsetsPointer64A0
{
    std::uint32_t struct_size{};
    std::uint32_t flags{};
    std::uint32_t name{};
    std::uint32_t name_size{};
    std::uint32_t media_type{};
    std::uint32_t media_type_size{};
    std::uint32_t data{};
    std::uint32_t data_size{};
    std::uint32_t reserved0{};
};

struct IpcAttachmentLayoutPointer64A0
{
    std::uint32_t size{};
    IpcAttachmentOffsetsPointer64A0 offsets{};
};

struct IpcAttachmentDescriptorA0
{
    IpcAttachmentLayoutWasm32A0 wasm32{};
    IpcAttachmentLayoutPointer64A0 pointer64{};
};

struct IpcCancelledA0
{
    std::string status = "cancelled";
};

struct IpcCancelRejectedA0
{
    std::string status = "rejected";
    DiagnosticA0 diagnostic{};
};

struct IpcEffectiveLimitsA0
{
    std::uint32_t json_bytes{};
    std::uint32_t attachment_count{};
    std::uint32_t attachment_name_bytes{};
    std::uint32_t attachment_media_type_bytes{};
    std::uint32_t attachment_bytes{};
    std::uint32_t frame_bytes{};
    std::uint32_t queued_requests{};
    std::uint32_t queued_bytes{};
    std::uint32_t resident_request_bytes{};
    std::uint32_t pending_writer_bytes{};
};

struct IpcGenericAbiLimitsA0
{
    std::uint32_t operation_id_bytes{};
    std::uint32_t request_json_bytes{};
    std::uint32_t response_json_bytes{};
    std::uint32_t attachment_count{};
    std::uint32_t attachment_name_bytes{};
    std::uint32_t attachment_media_type_bytes{};
    std::uint32_t attachment_bytes{};
    std::uint32_t aggregate_attachment_bytes_native{};
    std::uint32_t aggregate_attachment_bytes_wasm{};
};

struct IpcHelloA0
{
    std::string client_name{};
    std::string client_version{};
    std::vector<std::string> protocols{};
    std::optional<std::vector<std::string>> capabilities{};
};

enum class IpcRuntimeDispatchA0
{
    logical_dto,
    packed_attachment,
};

struct IpcPackedProjectionA0
{
    std::string kind = "packed_attachment";
    std::string attachment_name{};
    std::string format{};
};

struct IpcOperationDeclarationA0
{
    std::string identity{};
    std::string request_contract{};
    std::string result_contract{};
    IpcRuntimeDispatchA0 runtime_dispatch{};
    std::vector<IpcAttachmentDeclarationA0> input_attachments{};
    std::vector<IpcAttachmentDeclarationA0> output_attachments{};
    std::optional<IpcPackedProjectionA0> request_projection{};
    std::optional<IpcPackedProjectionA0> result_projection{};
};

struct IpcOperationCatalogA0
{
    std::string catalog = "wn.geometer.operation_catalog.a0";
    std::string generic_abi = "a0";
    std::string release_version{};
    std::uint32_t c_abi_generation{};
    std::vector<IpcOperationDeclarationA0> operations{};
    IpcAttachmentDescriptorA0 attachment_descriptor{};
    IpcGenericAbiLimitsA0 limits{};
};

struct IpcProtocolErrorA0
{
    std::string status = "protocol_error";
    DiagnosticA0 diagnostic{};
};

struct IpcReasonA0
{
    std::optional<std::string> reason{};
};

enum class ModelRootPlacement
{
    strip,
    preserve,
};

struct ModelTessellationRequestA0
{
    std::string schema = "geometry.model_tessellation.request.a0";
    std::optional<double> linear_deflection_mm{};
    std::optional<double> angular_deflection_rad{};
    std::optional<ModelRootPlacement> root_placement{};
    std::optional<std::uint32_t> max_triangles{};
};

enum class ModelFormat
{
    step,
};

using Matrix4x4 = std::vector<double>;

struct ModelBoundsOptionsA0
{
    std::optional<ModelFormat> format{};
    std::optional<Matrix4x4> model_transform{};
};

struct StepTopologyOpenRequestA0
{
    std::string schema = "geometry.step_topology.open.request.a0";
};

struct SessionReference
{
    std::string session_handle{};
    std::uint32_t generation{};
};

struct StepTopologyCloseRequestA0
{
    std::string schema = "geometry.step_topology.close.request.a0";
    SessionReference session{};
};

struct PageRequest
{
    std::optional<std::string> cursor{};
    std::uint32_t limit{};
};

struct StepTopologyInspectRequestA0
{
    std::string schema = "geometry.step_topology.inspect.request.a0";
    SessionReference session{};
    PageRequest page{};
    bool include_source_entity_evidence{};
    bool include_diagnostics{};
};

struct TessellationOptions
{
    double linear_deflection_mm{};
    double angular_deflection_rad{};
    bool relative{};
    bool parallel{};
    std::vector<double> source_to_render{};
};

struct StepTopologyRenderRequestA0
{
    std::string schema = "geometry.step_topology.render.request.a0";
    SessionReference session{};
    TessellationOptions tessellation{};
};

struct StepTopologyResolveHitRequestA0
{
    std::string schema = "geometry.step_topology.resolve_hit.request.a0";
    SessionReference session{};
    std::string artifact_handle{};
    std::string content_sha256{};
    std::uint32_t instance_index{};
    std::uint32_t primitive_index{};
    std::uint32_t primitive_triangle_index{};
    std::string occurrence_handle{};
    std::string body_handle{};
    std::string face_handle{};
};

struct CreateLogicalGroupCommand
{
    std::string kind = "create";
    std::string authored_id{};
    std::string name{};
    std::vector<std::string> member_handles{};
};

struct RenameLogicalGroupCommand
{
    std::string kind = "rename";
    std::string authored_id{};
    std::uint32_t expected_revision{};
    std::string name{};
};

struct ReplaceLogicalGroupMembersCommand
{
    std::string kind = "replace_members";
    std::string authored_id{};
    std::uint32_t expected_revision{};
    std::vector<std::string> member_handles{};
};

struct EraseLogicalGroupCommand
{
    std::string kind = "erase";
    std::string authored_id{};
    std::uint32_t expected_revision{};
};

using LogicalGroupCommand =
    std::variant<CreateLogicalGroupCommand, RenameLogicalGroupCommand,
                 ReplaceLogicalGroupMembersCommand, EraseLogicalGroupCommand>;

struct StepTopologyApplyLogicalGroupsRequestA0
{
    std::string schema = "geometry.step_topology.apply_logical_groups.request.a0";
    SessionReference session{};
    std::vector<LogicalGroupCommand> commands{};
};

struct DocumentProbeTarget
{
    std::string kind = "document";
};

struct DefinitionProbeTarget
{
    std::string kind = "definition";
    std::string target_handle{};
};

struct RootOccurrenceProbeTarget
{
    std::string kind = "root_occurrence";
    std::string target_handle{};
};

struct ComponentOccurrenceProbeTarget
{
    std::string kind = "occurrence";
    std::string target_handle{};
};

struct BodyProbeTarget
{
    std::string kind = "body";
    std::string target_handle{};
};

struct FaceProbeTarget
{
    std::string kind = "face";
    std::string target_handle{};
};

struct LogicalGroupProbeTarget
{
    std::string kind = "logical_group";
    std::string group_authored_id{};
};

using MetadataProbeTarget = std::variant<DocumentProbeTarget, DefinitionProbeTarget,
                                         RootOccurrenceProbeTarget, ComponentOccurrenceProbeTarget,
                                         BodyProbeTarget, FaceProbeTarget, LogicalGroupProbeTarget>;

struct AttachMetadataProbeCommand
{
    std::string kind = "attach";
    std::string authored_id{};
    MetadataProbeTarget target{};
    std::string key{};
    std::string value{};
};

struct ReplaceMetadataProbeCommand
{
    std::string kind = "replace";
    std::string authored_id{};
    std::uint32_t expected_revision{};
    MetadataProbeTarget target{};
    std::string key{};
    std::string value{};
};

struct EraseMetadataProbeCommand
{
    std::string kind = "erase";
    std::string authored_id{};
    std::uint32_t expected_revision{};
};

using MetadataProbeCommand = std::variant<AttachMetadataProbeCommand, ReplaceMetadataProbeCommand,
                                          EraseMetadataProbeCommand>;

struct StepTopologyApplyMetadataProbesRequestA0
{
    std::string schema = "geometry.step_topology.apply_metadata_probes.request.a0";
    SessionReference session{};
    std::vector<MetadataProbeCommand> commands{};
};

struct StepTopologyCheckpointEditJournalRequestA0
{
    std::string schema = "geometry.step_topology.checkpoint_edit_journal.request.a0";
    SessionReference session{};
};

enum class HierarchySourceKind
{
    definition,
    body,
};

struct CreateHierarchyProductCommand
{
    std::string kind = "create_product";
    std::string authored_id{};
    std::string name{};
    HierarchySourceKind source_kind{};
    std::string source_handle{};
};

struct CreateHierarchyAssemblyCommand
{
    std::string kind = "create_assembly";
    std::string authored_id{};
    std::string name{};
};

struct CreateHierarchyOccurrenceCommand
{
    std::string kind = "create_occurrence";
    std::string authored_id{};
    std::string child_authored_id{};
    std::string parent_assembly_authored_id{};
    std::vector<double> transform{};
};

struct ReparentHierarchyOccurrenceCommand
{
    std::string kind = "reparent_occurrence";
    std::string authored_id{};
    std::uint32_t expected_revision{};
    std::string parent_assembly_authored_id{};
    std::vector<double> transform{};
};

struct RenameHierarchyNodeCommand
{
    std::string kind = "rename_node";
    std::string authored_id{};
    std::uint32_t expected_revision{};
    std::string name{};
};

struct EraseHierarchyOccurrenceCommand
{
    std::string kind = "erase_occurrence";
    std::string authored_id{};
    std::uint32_t expected_revision{};
};

struct EraseHierarchyNodeCommand
{
    std::string kind = "erase_node";
    std::string authored_id{};
    std::uint32_t expected_revision{};
};

using HierarchyCommand =
    std::variant<CreateHierarchyProductCommand, CreateHierarchyAssemblyCommand,
                 CreateHierarchyOccurrenceCommand, ReparentHierarchyOccurrenceCommand,
                 RenameHierarchyNodeCommand, EraseHierarchyOccurrenceCommand,
                 EraseHierarchyNodeCommand>;

struct StepTopologyApplyHierarchyRequestA0
{
    std::string schema = "geometry.step_topology.apply_hierarchy.request.a0";
    SessionReference session{};
    std::uint32_t expected_hierarchy_revision{};
    std::vector<HierarchyCommand> commands{};
};

enum class SaveCarrier
{
    xbf,
    xml_xcaf,
    step_ap242,
    json_sidecar,
};

struct StepTopologySaveRequestA0
{
    std::string schema = "geometry.step_topology.save.request.a0";
    SessionReference session{};
    SaveCarrier carrier{};
    bool include_diagnostics{};
};

struct SourceDescriptor
{
    std::string format = "step";
    std::string sha256{};
    std::uint32_t bytes{};
    std::string normalized_length_unit = "millimeter";
};

struct XbfPersistenceArtifact
{
    std::string carrier = "xbf";
    std::string name = "state_artifact";
    std::string media_type = "application/vnd.opencascade.xbf";
    std::string format = "ocaf-xbf-version-12";
    std::uint32_t bytes{};
    std::string sha256{};
};

struct XmlXcafPersistenceArtifact
{
    std::string carrier = "xml_xcaf";
    std::string name = "state_artifact";
    std::string media_type = "application/vnd.opencascade.xml-xcaf";
    std::string format = "ocaf-xml-xcaf-version-12";
    std::uint32_t bytes{};
    std::string sha256{};
};

struct StepAp242PersistenceArtifact
{
    std::string carrier = "step_ap242";
    std::string name = "state_artifact";
    std::string media_type = "application/step";
    std::string format = "ap242-managed-model-based-3d-engineering";
    std::uint32_t bytes{};
    std::string sha256{};
};

struct JsonSidecarPersistenceArtifact
{
    std::string carrier = "json_sidecar";
    std::string name = "state_artifact";
    std::string media_type = "application/vnd.wavenumber.geometer.step-topology-sidecar+json";
    std::string format = "geometer.step_topology_sidecar.a0";
    std::uint32_t bytes{};
    std::string sha256{};
};

struct EditJournalPersistenceArtifact
{
    std::string carrier = "edit_journal";
    std::string name = "state_artifact";
    std::string media_type = "application/vnd.wavenumber.geometer.step-topology-edit-journal";
    std::string format = "geometer.step_topology_edit_journal.a0";
    std::uint32_t bytes{};
    std::string sha256{};
};

using RestoreStateArtifact =
    std::variant<XbfPersistenceArtifact, XmlXcafPersistenceArtifact, StepAp242PersistenceArtifact,
                 JsonSidecarPersistenceArtifact, EditJournalPersistenceArtifact>;

struct EditJournalReplayPreconditions
{
    std::string source_sha256{};
    std::string source_brep_sha256{};
    std::string target_inventory_sha256{};
    std::string occt_version{};
    std::uint32_t transaction_count{};
};

struct StepTopologyRestoreRequestA0
{
    std::string schema = "geometry.step_topology.restore.request.a0";
    SourceDescriptor source{};
    RestoreStateArtifact state_artifact{};
    std::optional<EditJournalReplayPreconditions> replay_preconditions{};
    bool include_diagnostics{};
};

struct RecoveryProvenance
{
    std::string source_artifact_sha256{};
    std::string candidate_artifact_sha256{};
    std::string source_occt_version{};
    std::string candidate_occt_version{};
    std::string source_driver{};
    std::string candidate_driver{};
    std::string source_writer_settings{};
    std::string candidate_writer_settings{};
    std::string command_provenance{};
    double measured_wall_time_milliseconds{};
};

struct RecoveryTolerances
{
    double length_mm{};
    double area_mm2{};
    double volume_mm3{};
};

enum class LogicalGroupMemberKind
{
    body,
    face,
};

struct RecoveryFingerprint
{
    std::string normalized_length_unit = "millimeter";
    std::string coordinate_frame{};
    std::string occurrence_context{};
    std::string geometry_kind{};
    double area_mm2{};
    double volume_mm3{};
    std::vector<double> centroid_mm{};
    std::vector<double> bounds_mm{};
    std::string adjacency_sha256{};
};

enum class RecoveryLineage
{
    none,
    split_from_source,
    merged_from_sources,
};

struct RecoveryCandidate
{
    std::string target_handle{};
    LogicalGroupMemberKind kind{};
    std::optional<std::string> authored_target_id{};
    bool topology_link_verified{};
    std::string carrier_locator{};
    bool carrier_locator_validated{};
    std::string carrier_record{};
    RecoveryLineage lineage{};
    std::optional<RecoveryFingerprint> fingerprint{};
};

struct RecoveryMemberRequest
{
    std::string member_record_id{};
    LogicalGroupMemberKind kind{};
    std::string authored_target_id{};
    std::string carrier_locator{};
    std::optional<RecoveryFingerprint> source_fingerprint{};
    std::vector<RecoveryCandidate> candidates{};
};

struct RecoveryGroupRequest
{
    std::string group_authored_id{};
    RecoveryProvenance provenance{};
    RecoveryTolerances tolerances{};
    std::vector<RecoveryMemberRequest> members{};
};

struct StepTopologyAnalyzeRecoveryRequestA0
{
    std::string schema = "geometry.step_topology.analyze_recovery.request.a0";
    std::vector<RecoveryGroupRequest> groups{};
};

using IpcRequestValueA0 = std::variant<
    ModelTessellationRequestA0, ModelBoundsOptionsA0, HlrProjectionOptionsA0,
    PackedAttachmentProjectionA0, StepTopologyOpenRequestA0, StepTopologyCloseRequestA0,
    StepTopologyInspectRequestA0, StepTopologyRenderRequestA0, StepTopologyResolveHitRequestA0,
    StepTopologyApplyLogicalGroupsRequestA0, StepTopologyApplyMetadataProbesRequestA0,
    StepTopologyCheckpointEditJournalRequestA0, StepTopologyApplyHierarchyRequestA0,
    StepTopologySaveRequestA0, StepTopologyRestoreRequestA0, StepTopologyAnalyzeRecoveryRequestA0>;

struct IpcRequestA0
{
    std::string operation{};
    IpcRequestValueA0 request{};
};

struct IpcShutdownAckA0
{
    std::string status = "complete";
    bool activeRequestCompleted{};
    std::uint32_t rejectedQueuedRequestCount{};
};

struct IpcWelcomeA0
{
    std::string release_version{};
    std::uint32_t c_abi_generation{};
    std::string ipc = "a0";
    std::string catalog_sha256{};
    IpcOperationCatalogA0 operation_catalog{};
    IpcEffectiveLimitsA0 limits{};
    std::vector<std::string> capabilities{};
};

using IllustrationMatrix4x4 = std::vector<double>;

using IllustrationVector3 = std::vector<double>;

struct MeshIllustrationMaterial
{
    IllustrationVector3 color{};
    std::optional<double> opacity{};
    std::optional<std::string> name{};
};

struct MeshIllustrationMesh
{
    std::string id{};
    std::vector<double> positions{};
    std::optional<std::vector<double>> normals{};
    std::optional<std::vector<std::uint32_t>> indices{};
    std::optional<IllustrationMatrix4x4> matrix{};
    std::vector<MeshIllustrationMaterial> materials{};
    std::optional<std::vector<std::uint32_t>> triangle_material_indices{};
    std::optional<bool> double_sided{};
};

struct MeshIllustrationView
{
    IllustrationVector3 direction{};
    IllustrationVector3 up{};
    std::optional<bool> mirror_x{};
};

struct MeshIllustrationPrepareOptions
{
    std::optional<std::uint32_t> max_triangles{};
    std::optional<double> weld_tolerance{};
};

enum class MeshIllustrationShading
{
    unlit,
    flat,
    lambert,
    banded,
    toon,
};

struct MeshIllustrationStyleA0
{
    std::optional<MeshIllustrationShading> shading{};
    std::optional<double> ambient{};
    std::optional<double> key_intensity{};
    std::optional<IllustrationVector3> light_direction{};
    std::optional<std::uint32_t> bands{};
    std::optional<bool> source_colors{};
    std::optional<IllustrationVector3> fallback_color{};
    std::optional<std::string> background{};
    std::optional<bool> transparent_background{};
    std::optional<bool> fuse_surfaces{};
    std::optional<bool> layer_coplanar_materials{};
    std::optional<bool> show_hlr_outline{};
    std::optional<bool> show_hlr_detail{};
    std::optional<bool> show_outlines{};
    std::optional<bool> show_creases{};
    std::optional<double> crease_angle_degrees{};
    std::optional<std::string> outline_color{};
    std::optional<std::string> crease_color{};
    std::optional<double> outline_width{};
    std::optional<double> crease_width{};
    std::optional<bool> double_sided{};
    std::optional<double> rim_amount{};
};

struct MeshIllustrationSvgOptions
{
    std::optional<std::uint32_t> coordinate_span{};
    std::optional<std::string> title{};
};

struct MeshIllustrationInputA0
{
    std::string schema = "geometry.mesh_illustration.input.a0";
    std::vector<MeshIllustrationMesh> meshes{};
    MeshIllustrationView view{};
    std::optional<MeshIllustrationPrepareOptions> prepare{};
    std::optional<MeshIllustrationStyleA0> style{};
    std::optional<MeshIllustrationSvgOptions> svg{};
};

struct MeshIllustrationRenderStats
{
    std::uint32_t triangles{};
    std::uint32_t surface_draws{};
    std::uint32_t layered_surfaces{};
    std::uint32_t outlines{};
    std::uint32_t details{};
    std::uint32_t creases{};
    std::uint32_t commands{};
};

struct MeshIllustrationResultA0
{
    std::string schema = "geometry.mesh_illustration.result.a0";
    std::string svg{};
    MeshIllustrationRenderStats stats{};
    std::vector<std::string> warnings{};
};

struct ModelBoundsSource
{
    ModelFormat format{};
    std::string hash{};
};

using Vector3 = std::vector<double>;

struct ModelBoundsValues
{
    Vector3 min{};
    Vector3 max{};
    Vector3 size{};
    Vector3 center{};
};

struct ModelBoundsTimings
{
    double model_read_ms{};
    double bounds_ms{};
};

struct ModelBoundsResultA0
{
    std::string schema = "geometry.model_bounds.a0";
    std::string units = "mm";
    ModelBoundsSource source{};
    ModelBoundsValues bounds{};
    ModelBoundsTimings timings{};
};

struct MeshCollectionA0
{
    std::string schema = "geometry.mesh_collection.a0";
    std::string length_unit = "millimeter";
    std::vector<MeshIllustrationMesh> meshes{};
};

struct MeshCollectionAttachment
{
    std::string attachment = "mesh_collection";
    std::string schema = "geometry.mesh_collection.a0";
    std::uint32_t byte_length{};
    std::string sha256{};
};

struct ModelTessellationResultA0
{
    std::string schema = "geometry.model_tessellation.result.a0";
    MeshCollectionAttachment mesh_collection{};
    std::string source_sha256{};
    std::uint32_t meshes{};
    std::uint32_t triangles{};
    std::vector<std::string> warnings{};
};

struct OperationFailureA0
{
    std::string operation{};
    bool ok = false;
    std::vector<DiagnosticA0> diagnostics{};
};

struct ToolDescriptor
{
    std::string name = "geometer";
    std::string release_version{};
    std::string occt_version{};
};

struct StepTopologyOpenResultA0
{
    std::string schema = "geometry.step_topology.open.result.a0";
    SessionReference session{};
    SourceDescriptor source{};
    ToolDescriptor tool{};
    std::vector<std::string> evicted_session_handles{};
};

struct StepTopologyCloseResultA0
{
    std::string schema = "geometry.step_topology.close.result.a0";
    std::string session_handle{};
    bool closed = true;
};

struct InspectionCounts
{
    std::uint32_t definitions{};
    std::uint32_t root_occurrences{};
    std::uint32_t component_occurrences{};
    std::uint32_t bodies{};
    std::uint32_t shells{};
    std::uint32_t faces{};
    std::uint32_t memberships{};
};

struct SourceEntityEvidence
{
    bool mapped{};
    bool shape_result_round_trip{};
    std::optional<std::uint32_t> model_number{};
    std::optional<std::string> entity_type{};
    std::optional<std::string> mapping_method{};
};

struct DefinitionSummary
{
    std::string handle{};
    std::string name{};
    bool assembly{};
    std::uint32_t body_count{};
    std::uint32_t face_count{};
    std::optional<SourceEntityEvidence> source_entity{};
};

struct RootOccurrenceSummary
{
    std::string kind = "root";
    std::string handle{};
    std::string definition_handle{};
    std::string name{};
    std::vector<double> transform{};
};

struct ComponentOccurrenceSummary
{
    std::string kind = "component";
    std::string handle{};
    std::string definition_handle{};
    std::string parent_occurrence_handle{};
    std::uint32_t depth{};
    std::string name{};
    std::vector<double> transform{};
};

using OccurrenceSummary = std::variant<RootOccurrenceSummary, ComponentOccurrenceSummary>;

struct BodySummary
{
    std::string handle{};
    std::string definition_handle{};
    std::string topology_kind{};
    std::uint32_t shell_count{};
    std::uint32_t face_count{};
    std::vector<double> bounds_mm{};
    double volume_mm3{};
    std::optional<SourceEntityEvidence> source_entity{};
};

struct ShellSummary
{
    std::string handle{};
    std::string definition_handle{};
    std::uint32_t body_count{};
    std::uint32_t face_count{};
    std::optional<SourceEntityEvidence> source_entity{};
};

struct FaceSummary
{
    std::string handle{};
    std::string definition_handle{};
    std::uint32_t body_count{};
    std::uint32_t shell_count{};
    std::vector<double> bounds_mm{};
    double area_mm2{};
    std::vector<double> centroid_mm{};
    std::optional<SourceEntityEvidence> source_entity{};
};

enum class TopologyMembershipKind
{
    body_shell,
    body_face,
    shell_face,
};

struct TopologyMembership
{
    TopologyMembershipKind kind{};
    std::string owner_handle{};
    std::string member_handle{};
};

struct TopologyPage
{
    std::vector<DefinitionSummary> definitions{};
    std::vector<OccurrenceSummary> occurrences{};
    std::vector<BodySummary> bodies{};
    std::vector<ShellSummary> shells{};
    std::vector<FaceSummary> faces{};
    std::vector<TopologyMembership> memberships{};
    std::optional<std::string> next_cursor{};
};

struct TopologyTableAttachmentDescriptor
{
    std::string name = "topology_table";
    std::string media_type = "application/vnd.wavenumber.geometer.step-topology-table";
    std::string format = "wn.geometer.step-topology-table.a0";
    std::uint32_t bytes{};
    std::string sha256{};
};

struct StepTopologyInspectResultA0
{
    std::string schema = "geometry.step_topology.inspect.result.a0";
    SessionReference session{};
    InspectionCounts counts{};
    TopologyPage page{};
    std::optional<TopologyTableAttachmentDescriptor> compact_table{};
    std::vector<DiagnosticA0> diagnostics{};
};

struct RenderCounts
{
    std::uint32_t meshes{};
    std::uint32_t instances{};
    std::uint32_t primitives{};
    std::uint32_t geometry_triangles{};
    std::uint32_t instanced_triangles{};
};

struct RenderArtifactDescriptor
{
    std::string artifact_handle{};
    std::string content_sha256{};
    std::string render_artifact_handle{};
    std::string render_content_sha256{};
    std::string binding_layout = "node-primitive-a0";
    std::string geometry_length_unit = "meter";
    std::string source_length_unit = "millimeter";
    RenderCounts counts{};
};

struct GlbAttachmentDescriptor
{
    std::string name = "glb";
    std::string media_type = "model/gltf-binary";
    std::string format = "glb-2.0";
    std::uint32_t bytes{};
    std::string sha256{};
};

struct TopologyBindingTableAttachmentDescriptor
{
    std::string name = "topology_binding_table";
    std::string media_type = "application/vnd.wavenumber.geometer.step-topology-binding-table";
    std::string format = "wn.geometer.step-topology-binding-table.a0";
    std::uint32_t bytes{};
    std::string sha256{};
};

struct StepTopologyRenderResultA0
{
    std::string schema = "geometry.step_topology.render.result.a0";
    SessionReference session{};
    RenderArtifactDescriptor artifact{};
    GlbAttachmentDescriptor glb{};
    std::optional<TopologyBindingTableAttachmentDescriptor> compact_binding_table{};
};

struct StepTopologyResolveHitResultA0
{
    std::string schema = "geometry.step_topology.resolve_hit.result.a0";
    SessionReference session{};
    std::uint32_t instance_index{};
    std::uint32_t primitive_index{};
    std::uint32_t triangle_index{};
    std::string occurrence_handle{};
    std::string body_handle{};
    std::string face_handle{};
};

struct MutationSessionState
{
    SessionReference session{};
    std::uint32_t edit_journal_revision{};
    std::uint32_t accounted_string_bytes{};
    std::uint32_t estimated_resident_bytes{};
};

struct LogicalGroupMember
{
    LogicalGroupMemberKind kind{};
    std::string target_handle{};
};

struct LogicalGroup
{
    std::string authored_id{};
    std::uint32_t revision{};
    std::string name{};
    std::vector<LogicalGroupMember> members{};
};

struct StepTopologyApplyLogicalGroupsResultA0
{
    std::string schema = "geometry.step_topology.apply_logical_groups.result.a0";
    MutationSessionState state{};
    std::vector<LogicalGroup> groups{};
    std::vector<DiagnosticA0> diagnostics{};
};

struct MetadataProbe
{
    std::string authored_id{};
    std::uint32_t revision{};
    MetadataProbeTarget target{};
    std::string key{};
    std::string value{};
};

struct StepTopologyApplyMetadataProbesResultA0
{
    std::string schema = "geometry.step_topology.apply_metadata_probes.result.a0";
    MutationSessionState state{};
    std::vector<LogicalGroup> groups{};
    std::vector<MetadataProbe> probes{};
    std::vector<DiagnosticA0> diagnostics{};
};

struct EditJournalAttachmentDescriptor
{
    std::string name = "edit_journal";
    std::string media_type = "application/vnd.wavenumber.geometer.step-topology-edit-journal";
    std::string format = "geometer.step_topology_edit_journal.a0";
    std::uint32_t bytes{};
    std::string sha256{};
};

struct StepTopologyCheckpointEditJournalResultA0
{
    std::string schema = "geometry.step_topology.checkpoint_edit_journal.result.a0";
    MutationSessionState state{};
    std::string source_sha256{};
    std::string source_brep_sha256{};
    std::string target_inventory_sha256{};
    std::string occt_version{};
    std::uint32_t transaction_count{};
    EditJournalAttachmentDescriptor journal{};
    std::vector<DiagnosticA0> diagnostics{};
};

enum class HierarchyNodeKind
{
    product,
    assembly,
};

struct HierarchyNode
{
    std::string authored_id{};
    std::uint32_t revision{};
    HierarchyNodeKind kind{};
    std::string name{};
    std::optional<HierarchySourceKind> source_kind{};
    std::optional<std::string> source_handle{};
};

struct HierarchyOccurrence
{
    std::string authored_id{};
    std::uint32_t revision{};
    std::string child_authored_id{};
    std::string parent_assembly_authored_id{};
    std::vector<double> transform{};
};

struct HierarchyState
{
    std::uint32_t hierarchy_revision{};
    std::string source_brep_sha256{};
    std::vector<HierarchyNode> nodes{};
    std::vector<HierarchyOccurrence> occurrences{};
};

struct StepTopologyApplyHierarchyResultA0
{
    std::string schema = "geometry.step_topology.apply_hierarchy.result.a0";
    MutationSessionState state{};
    HierarchyState hierarchy{};
    std::vector<DiagnosticA0> diagnostics{};
};

using SavePersistenceArtifact =
    std::variant<XbfPersistenceArtifact, XmlXcafPersistenceArtifact, StepAp242PersistenceArtifact,
                 JsonSidecarPersistenceArtifact>;

enum class PersistenceCarrier
{
    xbf,
    xml_xcaf,
    step_ap242,
    json_sidecar,
    edit_journal,
};

enum class CarrierSupportState
{
    supported,
    experimental,
    unsupported,
};

struct CarrierCapabilityNote
{
    std::string value{};
};

struct CarrierCapability
{
    PersistenceCarrier carrier{};
    CarrierSupportState save{};
    CarrierSupportState restore{};
    CarrierSupportState authored_payload{};
    CarrierSupportState topology_links{};
    std::vector<CarrierCapabilityNote> notes{};
};

struct StepTopologySaveResultA0
{
    std::string schema = "geometry.step_topology.save.result.a0";
    MutationSessionState state{};
    std::string source_sha256{};
    SavePersistenceArtifact artifact{};
    std::vector<CarrierCapability> capabilities{};
    std::vector<DiagnosticA0> diagnostics{};
};

enum class RecoveryResolutionState
{
    resolved,
    ambiguous,
    unresolved,
    unsupported,
};

enum class RecoveryGroupCompleteness
{
    fully_recovered,
    partially_recovered,
    unrecovered,
    unsupported,
};

enum class RecoveryResolutionMethod
{
    authored_id_topology_link,
    validated_carrier_locator,
    unique_geometry_adjacency_fingerprint,
    none,
};

enum class RecoveryTopologyComparison
{
    unchanged,
    relocated,
    split,
    merged,
    otherwise_changed,
    not_compared,
    unavailable,
};

enum class RecoveryConfidence
{
    high,
    medium,
    low,
    none,
};

struct RecoveryComparedField
{
    std::string value{};
};

struct RecoveryCarrierRecord
{
    std::string value{};
};

struct RecoveryRejectedAlternative
{
    std::string target_handle{};
    std::string reason{};
};

struct RecoveryEvidence
{
    std::uint32_t candidate_count{};
    std::uint32_t matching_candidate_count{};
    std::vector<RecoveryComparedField> compared_fields{};
    RecoveryTolerances tolerances{};
    std::vector<RecoveryCarrierRecord> carrier_records{};
    std::vector<RecoveryRejectedAlternative> rejected_alternatives{};
};

struct RecoveryMemberResult
{
    std::string member_record_id{};
    LogicalGroupMemberKind kind{};
    std::string authored_target_id{};
    RecoveryResolutionState resolution_state{};
    RecoveryResolutionMethod resolution_method{};
    RecoveryTopologyComparison topology_comparison{};
    RecoveryConfidence confidence{};
    std::optional<std::string> resolved_target_handle{};
    RecoveryEvidence evidence{};
};

struct RecoveryGroupResult
{
    std::string group_authored_id{};
    RecoveryProvenance provenance{};
    RecoveryResolutionState resolution_state{};
    RecoveryGroupCompleteness completeness{};
    std::uint32_t resolved_member_count{};
    std::uint32_t ambiguous_member_count{};
    std::uint32_t unresolved_member_count{};
    std::uint32_t unsupported_member_count{};
    std::vector<RecoveryMemberResult> members{};
};

struct StepTopologyRestoreResultA0
{
    std::string schema = "geometry.step_topology.restore.result.a0";
    SessionReference session{};
    SourceDescriptor source{};
    ToolDescriptor tool{};
    std::uint32_t replayed_transaction_count{};
    std::optional<std::vector<std::string>> evicted_session_handles{};
    std::vector<RecoveryGroupResult> recovery{};
    std::vector<DiagnosticA0> diagnostics{};
};

struct StepTopologyAnalyzeRecoveryResultA0
{
    std::string schema = "geometry.step_topology.analyze_recovery.result.a0";
    std::vector<RecoveryGroupResult> groups{};
    std::vector<DiagnosticA0> diagnostics{};
};

using OperationResultValueA0 =
    std::variant<ModelTessellationResultA0, ModelBoundsResultA0, HlrProjectionResultA0,
                 PackedAttachmentProjectionA0, StepTopologyOpenResultA0, StepTopologyCloseResultA0,
                 StepTopologyInspectResultA0, StepTopologyRenderResultA0,
                 StepTopologyResolveHitResultA0, StepTopologyApplyLogicalGroupsResultA0,
                 StepTopologyApplyMetadataProbesResultA0, StepTopologyCheckpointEditJournalResultA0,
                 StepTopologyApplyHierarchyResultA0, StepTopologySaveResultA0,
                 StepTopologyRestoreResultA0, StepTopologyAnalyzeRecoveryResultA0>;

struct OperationSuccessA0
{
    std::string operation{};
    bool ok = true;
    OperationResultValueA0 result{};
};

using OperationOutcomeA0 = std::variant<OperationSuccessA0, OperationFailureA0>;

bool decode_json(const unsigned char* data, std::size_t size, DiagnosticA0* value,
                 ContractError* error = nullptr);
bool encode_json(const DiagnosticA0& value, std::string* json, ContractError* error = nullptr);

bool decode_json(const unsigned char* data, std::size_t size, HlrProjectionOptionsA0* value,
                 ContractError* error = nullptr);
bool encode_json(const HlrProjectionOptionsA0& value, std::string* json,
                 ContractError* error = nullptr);

bool decode_json(const unsigned char* data, std::size_t size, HlrProjectionResultA0* value,
                 ContractError* error = nullptr);
bool encode_json(const HlrProjectionResultA0& value, std::string* json,
                 ContractError* error = nullptr);

bool decode_json(const unsigned char* data, std::size_t size, IpcCancelledA0* value,
                 ContractError* error = nullptr);
bool encode_json(const IpcCancelledA0& value, std::string* json, ContractError* error = nullptr);

bool decode_json(const unsigned char* data, std::size_t size, IpcCancelRejectedA0* value,
                 ContractError* error = nullptr);
bool encode_json(const IpcCancelRejectedA0& value, std::string* json,
                 ContractError* error = nullptr);

bool decode_json(const unsigned char* data, std::size_t size, IpcHelloA0* value,
                 ContractError* error = nullptr);
bool encode_json(const IpcHelloA0& value, std::string* json, ContractError* error = nullptr);

bool decode_json(const unsigned char* data, std::size_t size, IpcOperationCatalogA0* value,
                 ContractError* error = nullptr);
bool encode_json(const IpcOperationCatalogA0& value, std::string* json,
                 ContractError* error = nullptr);

bool decode_json(const unsigned char* data, std::size_t size, IpcProtocolErrorA0* value,
                 ContractError* error = nullptr);
bool encode_json(const IpcProtocolErrorA0& value, std::string* json,
                 ContractError* error = nullptr);

bool decode_json(const unsigned char* data, std::size_t size, IpcReasonA0* value,
                 ContractError* error = nullptr);
bool encode_json(const IpcReasonA0& value, std::string* json, ContractError* error = nullptr);

bool decode_json(const unsigned char* data, std::size_t size, IpcRequestA0* value,
                 ContractError* error = nullptr);
bool encode_json(const IpcRequestA0& value, std::string* json, ContractError* error = nullptr);

bool decode_json(const unsigned char* data, std::size_t size, IpcShutdownAckA0* value,
                 ContractError* error = nullptr);
bool encode_json(const IpcShutdownAckA0& value, std::string* json, ContractError* error = nullptr);

bool decode_json(const unsigned char* data, std::size_t size, IpcWelcomeA0* value,
                 ContractError* error = nullptr);
bool encode_json(const IpcWelcomeA0& value, std::string* json, ContractError* error = nullptr);

bool decode_json(const unsigned char* data, std::size_t size, MeshIllustrationInputA0* value,
                 ContractError* error = nullptr);
bool encode_json(const MeshIllustrationInputA0& value, std::string* json,
                 ContractError* error = nullptr);

bool decode_json(const unsigned char* data, std::size_t size, MeshIllustrationResultA0* value,
                 ContractError* error = nullptr);
bool encode_json(const MeshIllustrationResultA0& value, std::string* json,
                 ContractError* error = nullptr);

bool decode_json(const unsigned char* data, std::size_t size, MeshIllustrationStyleA0* value,
                 ContractError* error = nullptr);
bool encode_json(const MeshIllustrationStyleA0& value, std::string* json,
                 ContractError* error = nullptr);

bool decode_json(const unsigned char* data, std::size_t size, ModelBoundsOptionsA0* value,
                 ContractError* error = nullptr);
bool encode_json(const ModelBoundsOptionsA0& value, std::string* json,
                 ContractError* error = nullptr);

bool decode_json(const unsigned char* data, std::size_t size, ModelBoundsResultA0* value,
                 ContractError* error = nullptr);
bool encode_json(const ModelBoundsResultA0& value, std::string* json,
                 ContractError* error = nullptr);

bool decode_json(const unsigned char* data, std::size_t size, MeshCollectionA0* value,
                 ContractError* error = nullptr);
bool encode_json(const MeshCollectionA0& value, std::string* json, ContractError* error = nullptr);

bool decode_json(const unsigned char* data, std::size_t size, ModelTessellationRequestA0* value,
                 ContractError* error = nullptr);
bool encode_json(const ModelTessellationRequestA0& value, std::string* json,
                 ContractError* error = nullptr);

bool decode_json(const unsigned char* data, std::size_t size, ModelTessellationResultA0* value,
                 ContractError* error = nullptr);
bool encode_json(const ModelTessellationResultA0& value, std::string* json,
                 ContractError* error = nullptr);

bool decode_json(const unsigned char* data, std::size_t size, OperationOutcomeA0* value,
                 ContractError* error = nullptr);
bool encode_json(const OperationOutcomeA0& value, std::string* json,
                 ContractError* error = nullptr);

bool decode_json(const unsigned char* data, std::size_t size,
                 StepTopologyAnalyzeRecoveryRequestA0* value, ContractError* error = nullptr);
bool encode_json(const StepTopologyAnalyzeRecoveryRequestA0& value, std::string* json,
                 ContractError* error = nullptr);

bool decode_json(const unsigned char* data, std::size_t size,
                 StepTopologyAnalyzeRecoveryResultA0* value, ContractError* error = nullptr);
bool encode_json(const StepTopologyAnalyzeRecoveryResultA0& value, std::string* json,
                 ContractError* error = nullptr);

bool decode_json(const unsigned char* data, std::size_t size,
                 StepTopologyApplyHierarchyRequestA0* value, ContractError* error = nullptr);
bool encode_json(const StepTopologyApplyHierarchyRequestA0& value, std::string* json,
                 ContractError* error = nullptr);

bool decode_json(const unsigned char* data, std::size_t size,
                 StepTopologyApplyHierarchyResultA0* value, ContractError* error = nullptr);
bool encode_json(const StepTopologyApplyHierarchyResultA0& value, std::string* json,
                 ContractError* error = nullptr);

bool decode_json(const unsigned char* data, std::size_t size,
                 StepTopologyApplyLogicalGroupsRequestA0* value, ContractError* error = nullptr);
bool encode_json(const StepTopologyApplyLogicalGroupsRequestA0& value, std::string* json,
                 ContractError* error = nullptr);

bool decode_json(const unsigned char* data, std::size_t size,
                 StepTopologyApplyLogicalGroupsResultA0* value, ContractError* error = nullptr);
bool encode_json(const StepTopologyApplyLogicalGroupsResultA0& value, std::string* json,
                 ContractError* error = nullptr);

bool decode_json(const unsigned char* data, std::size_t size,
                 StepTopologyApplyMetadataProbesRequestA0* value, ContractError* error = nullptr);
bool encode_json(const StepTopologyApplyMetadataProbesRequestA0& value, std::string* json,
                 ContractError* error = nullptr);

bool decode_json(const unsigned char* data, std::size_t size,
                 StepTopologyApplyMetadataProbesResultA0* value, ContractError* error = nullptr);
bool encode_json(const StepTopologyApplyMetadataProbesResultA0& value, std::string* json,
                 ContractError* error = nullptr);

bool decode_json(const unsigned char* data, std::size_t size,
                 StepTopologyCheckpointEditJournalRequestA0* value, ContractError* error = nullptr);
bool encode_json(const StepTopologyCheckpointEditJournalRequestA0& value, std::string* json,
                 ContractError* error = nullptr);

bool decode_json(const unsigned char* data, std::size_t size,
                 StepTopologyCheckpointEditJournalResultA0* value, ContractError* error = nullptr);
bool encode_json(const StepTopologyCheckpointEditJournalResultA0& value, std::string* json,
                 ContractError* error = nullptr);

bool decode_json(const unsigned char* data, std::size_t size, StepTopologyCloseRequestA0* value,
                 ContractError* error = nullptr);
bool encode_json(const StepTopologyCloseRequestA0& value, std::string* json,
                 ContractError* error = nullptr);

bool decode_json(const unsigned char* data, std::size_t size, StepTopologyCloseResultA0* value,
                 ContractError* error = nullptr);
bool encode_json(const StepTopologyCloseResultA0& value, std::string* json,
                 ContractError* error = nullptr);

bool decode_json(const unsigned char* data, std::size_t size, StepTopologyInspectRequestA0* value,
                 ContractError* error = nullptr);
bool encode_json(const StepTopologyInspectRequestA0& value, std::string* json,
                 ContractError* error = nullptr);

bool decode_json(const unsigned char* data, std::size_t size, StepTopologyInspectResultA0* value,
                 ContractError* error = nullptr);
bool encode_json(const StepTopologyInspectResultA0& value, std::string* json,
                 ContractError* error = nullptr);

bool decode_json(const unsigned char* data, std::size_t size, StepTopologyOpenRequestA0* value,
                 ContractError* error = nullptr);
bool encode_json(const StepTopologyOpenRequestA0& value, std::string* json,
                 ContractError* error = nullptr);

bool decode_json(const unsigned char* data, std::size_t size, StepTopologyOpenResultA0* value,
                 ContractError* error = nullptr);
bool encode_json(const StepTopologyOpenResultA0& value, std::string* json,
                 ContractError* error = nullptr);

bool decode_json(const unsigned char* data, std::size_t size, StepTopologyRenderRequestA0* value,
                 ContractError* error = nullptr);
bool encode_json(const StepTopologyRenderRequestA0& value, std::string* json,
                 ContractError* error = nullptr);

bool decode_json(const unsigned char* data, std::size_t size, StepTopologyRenderResultA0* value,
                 ContractError* error = nullptr);
bool encode_json(const StepTopologyRenderResultA0& value, std::string* json,
                 ContractError* error = nullptr);

bool decode_json(const unsigned char* data, std::size_t size,
                 StepTopologyResolveHitRequestA0* value, ContractError* error = nullptr);
bool encode_json(const StepTopologyResolveHitRequestA0& value, std::string* json,
                 ContractError* error = nullptr);

bool decode_json(const unsigned char* data, std::size_t size, StepTopologyResolveHitResultA0* value,
                 ContractError* error = nullptr);
bool encode_json(const StepTopologyResolveHitResultA0& value, std::string* json,
                 ContractError* error = nullptr);

bool decode_json(const unsigned char* data, std::size_t size, StepTopologyRestoreRequestA0* value,
                 ContractError* error = nullptr);
bool encode_json(const StepTopologyRestoreRequestA0& value, std::string* json,
                 ContractError* error = nullptr);

bool decode_json(const unsigned char* data, std::size_t size, StepTopologyRestoreResultA0* value,
                 ContractError* error = nullptr);
bool encode_json(const StepTopologyRestoreResultA0& value, std::string* json,
                 ContractError* error = nullptr);

bool decode_json(const unsigned char* data, std::size_t size, StepTopologySaveRequestA0* value,
                 ContractError* error = nullptr);
bool encode_json(const StepTopologySaveRequestA0& value, std::string* json,
                 ContractError* error = nullptr);

bool decode_json(const unsigned char* data, std::size_t size, StepTopologySaveResultA0* value,
                 ContractError* error = nullptr);
bool encode_json(const StepTopologySaveResultA0& value, std::string* json,
                 ContractError* error = nullptr);

} // namespace geometer::contracts
