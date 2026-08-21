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

using IpcRequestValueA0 = std::variant<ModelBoundsOptionsA0, PackedAttachmentProjectionA0>;

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

struct OperationFailureA0
{
    std::string operation{};
    bool ok = false;
    std::vector<DiagnosticA0> diagnostics{};
};

using OperationResultValueA0 = std::variant<ModelBoundsResultA0, PackedAttachmentProjectionA0>;

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

bool decode_json(const unsigned char* data, std::size_t size, ModelBoundsOptionsA0* value,
                 ContractError* error = nullptr);
bool encode_json(const ModelBoundsOptionsA0& value, std::string* json,
                 ContractError* error = nullptr);

bool decode_json(const unsigned char* data, std::size_t size, ModelBoundsResultA0* value,
                 ContractError* error = nullptr);
bool encode_json(const ModelBoundsResultA0& value, std::string* json,
                 ContractError* error = nullptr);

bool decode_json(const unsigned char* data, std::size_t size, OperationOutcomeA0* value,
                 ContractError* error = nullptr);
bool encode_json(const OperationOutcomeA0& value, std::string* json,
                 ContractError* error = nullptr);

} // namespace geometer::contracts
