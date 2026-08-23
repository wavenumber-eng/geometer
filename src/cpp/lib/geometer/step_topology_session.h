#pragma once

#include "status.h"

#include <array>
#include <atomic>
#include <chrono>
#include <cstddef>
#include <cstdint>
#include <memory>
#include <string>
#include <vector>

namespace geometer
{

enum class StepTopologyTargetKind
{
    definition,
    occurrence,
    body,
    shell,
    face,
};

struct StepTopologyLimits
{
    std::size_t max_source_bytes = 256U * 1024U * 1024U;
    std::size_t max_sessions = 8U;
    std::size_t max_definitions = 10000U;
    std::size_t max_component_labels = 100000U;
    std::size_t max_expanded_occurrences = 100000U;
    std::size_t max_bodies = 100000U;
    std::size_t max_shells = 250000U;
    std::size_t max_faces = 1000000U;
    std::size_t max_handles = 1500000U;
    std::size_t max_transfer_index_shapes = 250000U;
    std::size_t max_transfer_work_items = 5000000U;
    std::size_t max_render_vertices = 10000000U;
    std::size_t max_render_indices = 30000000U;
    std::size_t max_render_primitives = 1000000U;
    std::size_t max_render_instances = 100000U;
    std::size_t max_render_bindings = 5000000U;
    std::size_t max_render_instanced_triangles = 50000000U;
    std::size_t max_render_estimated_bytes = 512U * 1024U * 1024U;
    std::size_t max_render_glb_bytes = 512U * 1024U * 1024U;
    std::size_t max_logical_groups = 10000U;
    std::size_t max_group_members = 100000U;
    std::size_t max_string_bytes = 4096U;
    std::size_t max_total_string_bytes = 16U * 1024U * 1024U;
    std::size_t max_session_estimated_bytes = 512U * 1024U * 1024U;
    std::size_t max_store_estimated_bytes = 1024U * 1024U * 1024U;
    std::chrono::milliseconds inactivity_timeout = std::chrono::minutes(30);
};

struct StepReaderPosture
{
    bool product = true;
    bool all_product_contexts = true;
    bool all_shape_representations = true;
    bool tessellated = true;
    bool all_assembly_levels = true;
    bool relationships = true;
    bool shape_aspects = true;
    bool constructive_geometry = false;
    bool subshape_names = true;
    bool nonmanifold = false;
    bool all_top_level_shapes = false;
    bool root_transformations = true;
    bool colors = true;
    bool names = true;
    bool layers = true;
    bool validation_properties = true;
    bool metadata = true;
    bool product_metadata = true;
    bool shuo = true;
    bool gdt = true;
    bool materials = true;
    bool views = true;
};

struct StepTopologyLabelSummary
{
    bool present = false;
    std::string name;
    std::size_t color_assignments = 0;
    std::size_t layer_assignments = 0;
    bool has_material_assignment = false;
    bool has_named_data = false;
    bool has_validation_properties = false;
};

struct StepSourceEntityEvidence
{
    bool mapped = false;
    bool shape_result_round_trip = false;
    std::int64_t model_number = 0;
    std::string entity_type;
    std::string mapping_method;
};

struct StepTopologyDefinition
{
    std::string handle;
    bool is_assembly = false;
    StepTopologyLabelSummary label;
    std::vector<std::string> body_handles;
    StepSourceEntityEvidence source_entity;
};

struct StepTopologyOccurrence
{
    std::string handle;
    std::string definition_handle;
    std::string parent_occurrence_handle;
    std::size_t depth = 0;
    std::array<double, 12> transform = {1.0, 0.0, 0.0, 0.0, 0.0, 1.0, 0.0, 0.0, 0.0, 0.0, 1.0, 0.0};
    StepTopologyLabelSummary label;
};

struct StepTopologyRootOccurrence
{
    std::string handle;
    std::string definition_handle;
    std::array<double, 12> transform = {1.0, 0.0, 0.0, 0.0, 0.0, 1.0, 0.0, 0.0, 0.0, 0.0, 1.0, 0.0};
    StepTopologyLabelSummary label;
};

struct StepTopologyBody
{
    std::string handle;
    std::string definition_handle;
    std::string topology_kind;
    std::vector<std::string> shell_handles;
    std::vector<std::string> face_handles;
    std::array<double, 6> bounds{};
    double volume = 0.0;
    StepTopologyLabelSummary label;
    StepSourceEntityEvidence source_entity;
};

struct StepTopologyShell
{
    std::string handle;
    std::string definition_handle;
    std::vector<std::string> body_handles;
    std::vector<std::string> face_handles;
    StepTopologyLabelSummary label;
    StepSourceEntityEvidence source_entity;
};

struct StepTopologyFace
{
    std::string handle;
    std::string definition_handle;
    std::vector<std::string> body_handles;
    std::vector<std::string> shell_handles;
    std::array<double, 6> bounds{};
    double area = 0.0;
    std::array<double, 3> centroid{};
    StepTopologyLabelSummary label;
    StepSourceEntityEvidence source_entity;
};

struct StepTopologyMetadataSummary
{
    std::size_t named_labels = 0;
    std::size_t color_assignments = 0;
    std::size_t layer_assignments = 0;
    std::size_t material_definitions = 0;
    std::size_t material_assignments = 0;
    std::size_t named_data_labels = 0;
    std::size_t validation_property_labels = 0;
    std::size_t mapped_source_entities = 0;
    std::size_t unmapped_source_entities = 0;
};

struct StepTopologyDiagnosticCarrier
{
    std::string target_handle;
    std::string xcaf_label_entry;
};

struct StepTopologySessionInfo
{
    std::string session_handle;
    std::uint64_t generation = 0;
    std::string source_sha256;
    std::string occt_version;
    std::size_t source_bytes = 0;
    std::size_t estimated_resident_bytes = 0;
};

struct StepTopologyInspectionOptions
{
    bool include_diagnostic_carriers = false;
    bool include_source_entity_evidence = false;
};

struct StepTopologySnapshot
{
    std::string research_format = "geometer.step_topology_inspection.research";
    StepTopologySessionInfo session;
    StepReaderPosture reader_posture;
    StepTopologyMetadataSummary metadata;
    std::size_t free_shape_count = 0;
    std::size_t component_label_count = 0;
    std::vector<StepTopologyDefinition> definitions;
    std::vector<StepTopologyRootOccurrence> root_occurrences;
    std::vector<StepTopologyOccurrence> occurrences;
    std::vector<StepTopologyBody> bodies;
    std::vector<StepTopologyShell> shells;
    std::vector<StepTopologyFace> faces;
    std::vector<StepTopologyDiagnosticCarrier> diagnostic_carriers;
};

struct StepTopologyResolvedTarget
{
    StepTopologyTargetKind kind = StepTopologyTargetKind::definition;
    std::string handle;
    std::uint64_t generation = 0;
};

enum class StepTopologyGroupCommandKind
{
    create,
    rename,
    replace_members,
    erase,
};

struct StepTopologyGroupCommand
{
    StepTopologyGroupCommandKind kind = StepTopologyGroupCommandKind::create;
    std::string authored_id;
    std::uint64_t expected_revision = 0;
    std::string name;
    std::vector<std::string> member_handles;
};

struct StepTopologyGroupTransaction
{
    std::uint64_t expected_generation = 0;
    std::vector<StepTopologyGroupCommand> commands;
};

struct StepTopologyLogicalGroupMember
{
    StepTopologyTargetKind kind = StepTopologyTargetKind::face;
    std::string target_handle;
};

struct StepTopologyLogicalGroup
{
    std::string authored_id;
    std::uint64_t revision = 0;
    std::string name;
    std::vector<StepTopologyLogicalGroupMember> members;
};

struct StepTopologyGroupTransactionResult
{
    StepTopologySessionInfo session;
    std::vector<StepTopologyLogicalGroup> groups;
};

struct StepTopologyOpenResult
{
    StepTopologySessionInfo session;
    std::vector<std::string> evicted_session_handles;
};

struct StepTopologyTessellationOptions
{
    double linear_deflection = 0.1;
    double angular_deflection = 0.5;
    bool relative = false;
    bool parallel = false;
    // Row-major 3x4 signed-rigid transform from OCCT's millimeter source frame
    // into the render frame. Reflections are allowed; scale and shear are not.
    std::array<double, 12> source_to_render = {1.0, 0.0, 0.0, 0.0, 0.0, 1.0,
                                               0.0, 0.0, 0.0, 0.0, 1.0, 0.0};
};

struct StepTopologyRenderVertex
{
    std::array<double, 3> position{};
    std::array<double, 3> normal{};
};

struct StepTopologyRenderPrimitive
{
    std::string body_handle;
    std::string face_handle;
    std::size_t first_index = 0;
    std::size_t index_count = 0;
    std::size_t first_triangle = 0;
    std::size_t triangle_count = 0;
};

struct StepTopologyRenderMesh
{
    std::string definition_handle;
    std::vector<StepTopologyRenderVertex> vertices;
    std::vector<std::uint32_t> indices;
    std::vector<StepTopologyRenderPrimitive> primitives;
};

struct StepTopologyRenderInstance
{
    std::string occurrence_handle;
    std::string definition_handle;
    std::size_t mesh_index = 0;
    std::array<double, 12> transform = {1.0, 0.0, 0.0, 0.0, 0.0, 1.0, 0.0, 0.0, 0.0, 0.0, 1.0, 0.0};
    bool front_face_reversed = false;
};

struct StepTopologyTriangleBinding
{
    std::size_t instance_index = 0;
    std::size_t primitive_index = 0;
    std::size_t first_triangle = 0;
    std::size_t triangle_count = 0;
    std::string occurrence_handle;
    std::string body_handle;
    std::string face_handle;
};

struct StepTopologyRenderArtifact
{
    std::string research_format = "geometer.step_topology_render.research";
    std::string artifact_handle;
    std::string content_sha256;
    StepTopologySessionInfo session;
    std::string normalized_length_unit = "millimeter";
    std::array<double, 12> source_to_render = {1.0, 0.0, 0.0, 0.0, 0.0, 1.0,
                                               0.0, 0.0, 0.0, 0.0, 1.0, 0.0};
    std::vector<StepTopologyRenderMesh> meshes;
    std::vector<StepTopologyRenderInstance> instances;
    std::vector<StepTopologyTriangleBinding> bindings;
    std::size_t geometry_triangle_count = 0;
    std::size_t instanced_triangle_count = 0;
    std::size_t estimated_resident_bytes = 0;
};

struct StepTopologyRenderHit
{
    std::size_t instance_index = 0;
    std::size_t primitive_index = 0;
    std::size_t triangle_index = 0;
    std::string occurrence_handle;
    std::string body_handle;
    std::string face_handle;
};

struct StepTopologyGlbOptions
{
    StepTopologyTessellationOptions tessellation;

    StepTopologyGlbOptions()
    {
        // OCCT source convention: +Y forward, +Z up. glTF: -Z forward, +Y up.
        tessellation.source_to_render = {1.0, 0.0, 0.0, 0.0,  0.0, 0.0,
                                         1.0, 0.0, 0.0, -1.0, 0.0, 0.0};
    }
};

struct StepTopologyGlbWorkPacket
{
    std::string research_format = "geometer.step_topology_glb_work_packet.research";
    std::string artifact_handle;
    std::string content_sha256;
    StepTopologyRenderArtifact render;
    std::vector<unsigned char> glb;
    std::size_t json_bytes = 0;
    std::size_t binary_bytes = 0;
};

struct StepTopologyGlbHitDescriptor
{
    std::string artifact_handle;
    std::string content_sha256;
    std::size_t instance_index = 0;
    std::size_t primitive_index = 0;
    std::size_t primitive_triangle_index = 0;
    std::string occurrence_handle;
    std::string body_handle;
    std::string face_handle;
};

class StepTopologyCancellation
{
  public:
    void request_cancel() noexcept
    {
        cancelled_.store(true, std::memory_order_relaxed);
    }

    bool is_cancelled() const noexcept
    {
        return cancelled_.load(std::memory_order_relaxed);
    }

  private:
    std::atomic<bool> cancelled_{false};
};

class StepTopologySession
{
  public:
    StepTopologySession(const StepTopologySession&) = delete;
    StepTopologySession& operator=(const StepTopologySession&) = delete;
    StepTopologySession(StepTopologySession&&) noexcept;
    StepTopologySession& operator=(StepTopologySession&&) noexcept;
    ~StepTopologySession();

    static int open_step(const unsigned char* source, std::size_t source_size,
                         const StepTopologyLimits& limits,
                         std::unique_ptr<StepTopologySession>* session, Status* status = nullptr);
    static int open_step(const unsigned char* source, std::size_t source_size,
                         const StepTopologyLimits& limits,
                         const StepTopologyCancellation* cancellation,
                         std::unique_ptr<StepTopologySession>* session, Status* status = nullptr);

    const StepTopologySessionInfo& info() const;
    bool is_open() const;
    int inspect(const StepTopologyInspectionOptions& options, StepTopologySnapshot* snapshot,
                Status* status = nullptr) const;
    int refresh(StepTopologySnapshot* snapshot, Status* status = nullptr);
    int refresh(const StepTopologyCancellation* cancellation, StepTopologySnapshot* snapshot,
                Status* status = nullptr);
    int render(const StepTopologyTessellationOptions& options, StepTopologyRenderArtifact* artifact,
               Status* status = nullptr);
    int render(const StepTopologyTessellationOptions& options,
               const StepTopologyCancellation* cancellation, StepTopologyRenderArtifact* artifact,
               Status* status = nullptr);
    int resolve_render_hit(const StepTopologyRenderArtifact& artifact, std::size_t instance_index,
                           std::size_t triangle_index, StepTopologyRenderHit* hit,
                           Status* status = nullptr) const;
    int render_glb_work_packet(const StepTopologyGlbOptions& options,
                               StepTopologyGlbWorkPacket* packet, Status* status = nullptr);
    int render_glb_work_packet(const StepTopologyGlbOptions& options,
                               const StepTopologyCancellation* cancellation,
                               StepTopologyGlbWorkPacket* packet, Status* status = nullptr);
    int resolve_glb_hit(const StepTopologyGlbWorkPacket& packet,
                        const StepTopologyGlbHitDescriptor& descriptor, StepTopologyRenderHit* hit,
                        Status* status = nullptr) const;
    int resolve(const std::string& handle, StepTopologyResolvedTarget* target,
                Status* status = nullptr) const;
    int apply_logical_groups(const StepTopologyGroupTransaction& transaction,
                             StepTopologyGroupTransactionResult* result, Status* status = nullptr);
    int close(Status* status = nullptr);

  private:
    struct Impl;
    explicit StepTopologySession(std::unique_ptr<Impl> impl);
    std::unique_ptr<Impl> impl_;
};

class StepTopologySessionStore
{
  public:
    explicit StepTopologySessionStore(StepTopologyLimits limits = {});
    StepTopologySessionStore(const StepTopologySessionStore&) = delete;
    StepTopologySessionStore& operator=(const StepTopologySessionStore&) = delete;
    StepTopologySessionStore(StepTopologySessionStore&&) noexcept;
    StepTopologySessionStore& operator=(StepTopologySessionStore&&) noexcept;
    ~StepTopologySessionStore();

    int open_step(const unsigned char* source, std::size_t source_size,
                  StepTopologyOpenResult* result, Status* status = nullptr);
    int inspect(const std::string& session_handle, const StepTopologyInspectionOptions& options,
                StepTopologySnapshot* snapshot, Status* status = nullptr);
    int refresh(const std::string& session_handle, StepTopologySnapshot* snapshot,
                Status* status = nullptr);
    int apply_logical_groups(const std::string& session_handle,
                             const StepTopologyGroupTransaction& transaction,
                             StepTopologyGroupTransactionResult* result, Status* status = nullptr);
    int resolve(const std::string& session_handle, const std::string& target_handle,
                StepTopologyResolvedTarget* target, Status* status = nullptr);
    int close(const std::string& session_handle, Status* status = nullptr);
    std::vector<std::string>
    evict_expired(std::chrono::steady_clock::time_point now = std::chrono::steady_clock::now());
    std::vector<std::string> clear_for_process_replacement();
    std::size_t size() const;
    std::size_t estimated_resident_bytes() const;

  private:
    struct Impl;
    std::unique_ptr<Impl> impl_;
};

} // namespace geometer
