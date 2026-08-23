#include "geometer/step_topology_session.h"

#include "geometer/sha256.h"
#include "step_topology_session_internal.h"

#include <Standard_Failure.hxx>
#include <Standard_Version.hxx>

#include <algorithm>
#include <cerrno>
#include <chrono>
#include <cstddef>
#include <cstdint>
#include <limits>
#include <memory>
#include <stdexcept>
#include <string>
#include <tuple>
#include <unordered_map>
#include <utility>
#include <vector>

#if defined(_WIN32)
#include <Windows.h>
#include <bcrypt.h>
#elif defined(__APPLE__) || defined(__FreeBSD__) || defined(__OpenBSD__)
#include <cstdlib>
#else
#include <sys/random.h>
#endif

namespace geometer
{
namespace
{

using namespace step_topology_internal;

std::array<std::uint8_t, 32> random_secret()
{
    std::array<std::uint8_t, 32> secret{};
#if defined(_WIN32)
    if (BCryptGenRandom(nullptr, secret.data(), static_cast<ULONG>(secret.size()),
                        BCRYPT_USE_SYSTEM_PREFERRED_RNG) != 0)
    {
        throw std::runtime_error("Windows CSPRNG failed while creating a STEP topology session.");
    }
#elif defined(__APPLE__) || defined(__FreeBSD__) || defined(__OpenBSD__)
    arc4random_buf(secret.data(), secret.size());
#else
    std::size_t offset = 0;
    while (offset < secret.size())
    {
        const ssize_t count = getrandom(secret.data() + offset, secret.size() - offset, 0);
        if (count < 0 && errno == EINTR)
        {
            continue;
        }
        if (count <= 0)
        {
            throw std::runtime_error(
                "Operating-system CSPRNG failed while creating a STEP topology session.");
        }
        offset += static_cast<std::size_t>(count);
    }
#endif
    return secret;
}

std::string session_handle(const std::array<std::uint8_t, 32>& secret)
{
    return "gts_" + sha256_hex(secret.data(), secret.size());
}

bool valid_limits(const StepTopologyLimits& limits)
{
    return limits.max_source_bytes > 0 && limits.max_sessions > 0 && limits.max_definitions > 0 &&
           limits.max_component_labels > 0 && limits.max_expanded_occurrences > 0 &&
           limits.max_bodies > 0 && limits.max_shells > 0 && limits.max_faces > 0 &&
           limits.max_handles > 0 && limits.max_transfer_index_shapes > 0 &&
           limits.max_transfer_work_items > 0 && limits.max_render_vertices > 0 &&
           limits.max_render_indices > 0 && limits.max_render_primitives > 0 &&
           limits.max_render_instances > 0 && limits.max_render_bindings > 0 &&
           limits.max_render_instanced_triangles > 0 && limits.max_render_estimated_bytes > 0 &&
           limits.max_render_glb_bytes > 0 && limits.max_string_bytes > 0 &&
           limits.max_logical_groups > 0 && limits.max_group_members > 0 &&
           limits.max_total_string_bytes > 0 && limits.max_session_estimated_bytes > 0 &&
           limits.max_store_estimated_bytes > 0 && limits.inactivity_timeout.count() > 0;
}

void remove_source_evidence(StepTopologySnapshot* snapshot)
{
    snapshot->metadata.mapped_source_entities = 0;
    snapshot->metadata.unmapped_source_entities = 0;
    for (StepTopologyBody& body : snapshot->bodies)
    {
        body.source_entity = {};
    }
    for (StepTopologyShell& shell : snapshot->shells)
    {
        shell.source_entity = {};
    }
    for (StepTopologyFace& face : snapshot->faces)
    {
        face.source_entity = {};
    }
    for (StepTopologyDefinition& definition : snapshot->definitions)
    {
        definition.source_entity = {};
    }
}

} // namespace

namespace step_topology_internal
{

void set_status(Status* status, int code, const std::string& message)
{
    if (status != nullptr)
    {
        status->code = code;
        status->message = message;
    }
}

std::string issue_handle(SessionData* data, StepTopologyTargetKind kind, const TopoDS_Shape& shape)
{
    for (;;)
    {
        ++data->handle_counter;
        std::vector<std::uint8_t> material;
        material.reserve(data->secret.size() + sizeof(data->info.generation) +
                         sizeof(data->handle_counter) + 1U);
        material.insert(material.end(), data->secret.begin(), data->secret.end());
        const auto append_integer = [&material](std::uint64_t value)
        {
            for (unsigned int shift = 0; shift < 64U; shift += 8U)
            {
                material.push_back(static_cast<std::uint8_t>((value >> shift) & 0xffU));
            }
        };
        append_integer(data->info.generation);
        append_integer(data->handle_counter);
        material.push_back(static_cast<std::uint8_t>(kind));
        const std::string handle = "gtt_" + sha256_hex(material.data(), material.size());
        if (data->handles.emplace(handle, HandleRecord{kind, data->info.generation, shape}).second)
        {
            return handle;
        }
    }
}

bool account_string(SessionData* data, const std::string& value, Status* status)
{
    if (value.size() > data->limits.max_string_bytes ||
        value.size() > data->limits.max_total_string_bytes ||
        data->total_string_bytes > data->limits.max_total_string_bytes - value.size())
    {
        set_status(status, kResourceLimit,
                   "STEP topology string data exceeds the configured research limit.");
        return false;
    }
    data->total_string_bytes += value.size();
    return true;
}

} // namespace step_topology_internal

struct StepTopologySession::Impl
{
    std::unique_ptr<SessionData> data;
    StepTopologySessionInfo info;
};

StepTopologySession::StepTopologySession(std::unique_ptr<Impl> impl) : impl_(std::move(impl)) {}

StepTopologySession::StepTopologySession(StepTopologySession&&) noexcept = default;
StepTopologySession& StepTopologySession::operator=(StepTopologySession&&) noexcept = default;
StepTopologySession::~StepTopologySession() = default;

int StepTopologySession::open_step(const unsigned char* source, std::size_t source_size,
                                   const StepTopologyLimits& limits,
                                   std::unique_ptr<StepTopologySession>* session, Status* status)
{
    return open_step(source, source_size, limits, nullptr, session, status);
}

int StepTopologySession::open_step(const unsigned char* source, std::size_t source_size,
                                   const StepTopologyLimits& limits,
                                   const StepTopologyCancellation* cancellation,
                                   std::unique_ptr<StepTopologySession>* session, Status* status)
{
    if (session == nullptr)
    {
        set_status(status, kInvalidArgument, "STEP topology session output pointer is null.");
        return kInvalidArgument;
    }
    session->reset();
    if (source == nullptr || source_size == 0)
    {
        set_status(status, kInvalidArgument, "STEP topology source buffer is empty.");
        return kInvalidArgument;
    }
    if (!valid_limits(limits))
    {
        set_status(status, kInvalidArgument, "STEP topology limits must all be positive.");
        return kInvalidArgument;
    }
    if (source_size > limits.max_source_bytes)
    {
        set_status(status, kResourceLimit,
                   "STEP topology source exceeds the configured byte limit.");
        return kResourceLimit;
    }

    try
    {
        auto data = std::make_unique<SessionData>();
        data->limits = limits;
        data->source.assign(source, source + source_size);
        data->secret = random_secret();
        data->info.session_handle = session_handle(data->secret);
        data->info.generation = 1;
        data->info.source_sha256 = sha256_hex(source, source_size);
        data->info.occt_version = OCC_VERSION_STRING_EXT;
        data->info.source_bytes = source_size;
        const int import_code = import_step_session(data.get(), cancellation, status);
        if (import_code != 0)
        {
            return import_code;
        }
        const int inspect_code = rebuild_snapshot(data.get(), cancellation, status);
        if (inspect_code != 0)
        {
            return inspect_code;
        }
        if (data->info.estimated_resident_bytes > limits.max_session_estimated_bytes)
        {
            set_status(status, kResourceLimit,
                       "STEP topology session exceeds the configured resident-byte estimate.");
            return kResourceLimit;
        }
        data->open = true;

        auto impl = std::make_unique<Impl>();
        impl->info = data->info;
        impl->data = std::move(data);
        *session = std::unique_ptr<StepTopologySession>(new StepTopologySession(std::move(impl)));
        set_status(status, 0, "");
        return 0;
    }
    catch (const Standard_Failure& failure)
    {
        set_status(status, kInternalFailure, failure.GetMessageString());
        return kInternalFailure;
    }
    catch (const std::exception& error)
    {
        set_status(status, kInternalFailure, error.what());
        return kInternalFailure;
    }
}

const StepTopologySessionInfo& StepTopologySession::info() const
{
    return impl_->info;
}

bool StepTopologySession::is_open() const
{
    return impl_ != nullptr && impl_->data != nullptr && impl_->data->open;
}

int StepTopologySession::inspect(const StepTopologyInspectionOptions& options,
                                 StepTopologySnapshot* snapshot, Status* status) const
{
    if (snapshot == nullptr)
    {
        set_status(status, kInvalidArgument, "STEP topology snapshot output pointer is null.");
        return kInvalidArgument;
    }
    if (!is_open())
    {
        set_status(status, kClosed, "STEP topology session is closed.");
        return kClosed;
    }
    *snapshot = impl_->data->snapshot;
    if (!options.include_diagnostic_carriers)
    {
        snapshot->diagnostic_carriers.clear();
    }
    if (!options.include_source_entity_evidence)
    {
        remove_source_evidence(snapshot);
    }
    set_status(status, 0, "");
    return 0;
}

int StepTopologySession::refresh(StepTopologySnapshot* snapshot, Status* status)
{
    return refresh(nullptr, snapshot, status);
}

int StepTopologySession::refresh(const StepTopologyCancellation* cancellation,
                                 StepTopologySnapshot* snapshot, Status* status)
{
    if (!is_open())
    {
        set_status(status, kClosed, "STEP topology session is closed.");
        return kClosed;
    }
    if (snapshot == nullptr)
    {
        set_status(status, kInvalidArgument, "STEP topology snapshot output pointer is null.");
        return kInvalidArgument;
    }
    *snapshot = {};
    const std::uint64_t previous_generation = impl_->data->info.generation;
    const std::uint64_t previous_counter = impl_->data->handle_counter;
    if (previous_generation == std::numeric_limits<std::uint64_t>::max())
    {
        set_status(status, kResourceLimit, "STEP topology session generation is exhausted.");
        return kResourceLimit;
    }
    ++impl_->data->info.generation;
    const int code = rebuild_snapshot(impl_->data.get(), cancellation, status);
    if (code != 0)
    {
        impl_->data->info.generation = previous_generation;
        impl_->data->handle_counter = previous_counter;
        return code;
    }
    impl_->info = impl_->data->info;
    return inspect({}, snapshot, status);
}

int StepTopologySession::render(const StepTopologyTessellationOptions& options,
                                StepTopologyRenderArtifact* artifact, Status* status)
{
    return render(options, nullptr, artifact, status);
}

int StepTopologySession::render(const StepTopologyTessellationOptions& options,
                                const StepTopologyCancellation* cancellation,
                                StepTopologyRenderArtifact* artifact, Status* status)
{
    if (artifact == nullptr)
    {
        set_status(status, kInvalidArgument, "STEP topology render output pointer is null.");
        return kInvalidArgument;
    }
    *artifact = {};
    if (!is_open())
    {
        set_status(status, kClosed, "STEP topology session is closed.");
        return kClosed;
    }
    return build_render_artifact(impl_->data.get(), options, cancellation, artifact, status);
}

int StepTopologySession::render_glb_work_packet(const StepTopologyGlbOptions& options,
                                                StepTopologyGlbWorkPacket* packet, Status* status)
{
    return render_glb_work_packet(options, nullptr, packet, status);
}

int StepTopologySession::render_glb_work_packet(const StepTopologyGlbOptions& options,
                                                const StepTopologyCancellation* cancellation,
                                                StepTopologyGlbWorkPacket* packet, Status* status)
{
    if (packet == nullptr)
    {
        set_status(status, kInvalidArgument, "STEP topology GLB packet output pointer is null.");
        return kInvalidArgument;
    }
    *packet = {};
    if (!is_open())
    {
        set_status(status, kClosed, "STEP topology session is closed.");
        return kClosed;
    }
    return build_glb_work_packet(impl_->data.get(), options, cancellation, packet, status);
}

int StepTopologySession::resolve_render_hit(const StepTopologyRenderArtifact& artifact,
                                            std::size_t instance_index, std::size_t triangle_index,
                                            StepTopologyRenderHit* hit, Status* status) const
{
    if (hit == nullptr)
    {
        set_status(status, kInvalidArgument, "STEP topology render-hit output pointer is null.");
        return kInvalidArgument;
    }
    *hit = {};
    if (!is_open())
    {
        set_status(status, kClosed, "STEP topology session is closed.");
        return kClosed;
    }
    if (artifact.research_format != "geometer.step_topology_render.research" ||
        artifact.session.session_handle != impl_->data->info.session_handle ||
        artifact.session.generation != impl_->data->info.generation ||
        artifact.session.source_sha256 != impl_->data->info.source_sha256 ||
        artifact.session.occt_version != impl_->data->info.occt_version ||
        artifact.session.source_bytes != impl_->data->info.source_bytes ||
        artifact.session.estimated_resident_bytes != impl_->data->info.estimated_resident_bytes)
    {
        set_status(status, kUnknownTarget,
                   "STEP topology render artifact is stale or belongs to another session.");
        return kUnknownTarget;
    }
    if (!verify_render_artifact_seal(impl_->data.get(), artifact, status))
    {
        return kUnknownTarget;
    }
    if (instance_index >= artifact.instances.size())
    {
        set_status(status, kUnknownTarget, "STEP topology render instance is out of bounds.");
        return kUnknownTarget;
    }
    const StepTopologyRenderInstance& instance = artifact.instances[instance_index];
    if (instance.mesh_index >= artifact.meshes.size())
    {
        set_status(status, kUnknownTarget, "STEP topology render mesh is out of bounds.");
        return kUnknownTarget;
    }
    const StepTopologyRenderMesh& mesh = artifact.meshes[instance.mesh_index];
    if (triangle_index >= mesh.indices.size() / 3U)
    {
        set_status(status, kUnknownTarget, "STEP topology render triangle is out of bounds.");
        return kUnknownTarget;
    }
    if (mesh.definition_handle != instance.definition_handle)
    {
        set_status(status, kUnknownTarget,
                   "STEP topology render instance and mesh definitions disagree.");
        return kUnknownTarget;
    }
    const auto live_target = [this](const std::string& handle, StepTopologyTargetKind kind)
    {
        const auto found = impl_->data->handles.find(handle);
        return found != impl_->data->handles.end() && found->second.kind == kind &&
               found->second.generation == impl_->data->info.generation;
    };
    if (!live_target(instance.occurrence_handle, StepTopologyTargetKind::occurrence) ||
        !live_target(instance.definition_handle, StepTopologyTargetKind::definition))
    {
        set_status(status, kUnknownTarget,
                   "STEP topology render instance contains a non-live target.");
        return kUnknownTarget;
    }
    std::string authoritative_definition;
    const auto root = std::find_if(impl_->data->snapshot.root_occurrences.begin(),
                                   impl_->data->snapshot.root_occurrences.end(),
                                   [&instance](const StepTopologyRootOccurrence& value)
                                   { return value.handle == instance.occurrence_handle; });
    if (root != impl_->data->snapshot.root_occurrences.end())
    {
        authoritative_definition = root->definition_handle;
    }
    else
    {
        const auto occurrence = std::find_if(
            impl_->data->snapshot.occurrences.begin(), impl_->data->snapshot.occurrences.end(),
            [&instance](const StepTopologyOccurrence& value)
            { return value.handle == instance.occurrence_handle; });
        if (occurrence != impl_->data->snapshot.occurrences.end())
        {
            authoritative_definition = occurrence->definition_handle;
        }
    }
    if (authoritative_definition != instance.definition_handle)
    {
        set_status(status, kUnknownTarget,
                   "STEP topology render occurrence and definition disagree.");
        return kUnknownTarget;
    }
    if (!std::is_sorted(
            artifact.bindings.begin(), artifact.bindings.end(),
            [](const StepTopologyTriangleBinding& first, const StepTopologyTriangleBinding& second)
            {
                return std::tie(first.instance_index, first.primitive_index) <
                       std::tie(second.instance_index, second.primitive_index);
            }))
    {
        set_status(status, kUnknownTarget, "STEP topology render binding table is unsorted.");
        return kUnknownTarget;
    }
    const auto first =
        std::lower_bound(artifact.bindings.begin(), artifact.bindings.end(), instance_index,
                         [](const StepTopologyTriangleBinding& binding, std::size_t index)
                         { return binding.instance_index < index; });
    std::size_t expected_primitive = 0;
    std::size_t expected_first_triangle = 0;
    const StepTopologyTriangleBinding* match = nullptr;
    for (auto current = first;
         current != artifact.bindings.end() && current->instance_index == instance_index; ++current)
    {
        if (current->primitive_index != expected_primitive ||
            current->primitive_index >= mesh.primitives.size())
        {
            set_status(status, kUnknownTarget,
                       "STEP topology render bindings are missing, duplicated, or reordered.");
            return kUnknownTarget;
        }
        const StepTopologyRenderPrimitive& primitive = mesh.primitives[current->primitive_index];
        if (primitive.first_triangle != expected_first_triangle || primitive.triangle_count == 0 ||
            primitive.first_index != primitive.first_triangle * 3U ||
            primitive.index_count != primitive.triangle_count * 3U ||
            current->first_triangle != primitive.first_triangle ||
            current->triangle_count != primitive.triangle_count ||
            current->occurrence_handle != instance.occurrence_handle ||
            current->body_handle != primitive.body_handle ||
            current->face_handle != primitive.face_handle ||
            !live_target(current->body_handle, StepTopologyTargetKind::body) ||
            !live_target(current->face_handle, StepTopologyTargetKind::face))
        {
            set_status(status, kUnknownTarget,
                       "STEP topology render primitive or binding chain is invalid.");
            return kUnknownTarget;
        }
        const auto body =
            std::find_if(impl_->data->snapshot.bodies.begin(), impl_->data->snapshot.bodies.end(),
                         [&current](const StepTopologyBody& value)
                         { return value.handle == current->body_handle; });
        const auto face =
            std::find_if(impl_->data->snapshot.faces.begin(), impl_->data->snapshot.faces.end(),
                         [&current](const StepTopologyFace& value)
                         { return value.handle == current->face_handle; });
        if (body == impl_->data->snapshot.bodies.end() ||
            face == impl_->data->snapshot.faces.end() ||
            body->definition_handle != instance.definition_handle ||
            face->definition_handle != instance.definition_handle ||
            std::find(body->face_handles.begin(), body->face_handles.end(), face->handle) ==
                body->face_handles.end() ||
            std::find(face->body_handles.begin(), face->body_handles.end(), body->handle) ==
                face->body_handles.end())
        {
            set_status(status, kUnknownTarget,
                       "STEP topology render targets violate live topology ownership.");
            return kUnknownTarget;
        }
        if (triangle_index >= current->first_triangle &&
            triangle_index - current->first_triangle < current->triangle_count)
        {
            if (match != nullptr)
            {
                set_status(status, kUnknownTarget,
                           "STEP topology render triangle has overlapping bindings.");
                return kUnknownTarget;
            }
            match = &*current;
        }
        expected_first_triangle += primitive.triangle_count;
        ++expected_primitive;
    }
    if (expected_primitive != mesh.primitives.size() ||
        expected_first_triangle != mesh.indices.size() / 3U || match == nullptr)
    {
        set_status(status, kUnknownTarget,
                   "STEP topology render bindings do not exactly cover the mesh triangles.");
        return kUnknownTarget;
    }
    *hit = {instance_index,           match->primitive_index, triangle_index,
            match->occurrence_handle, match->body_handle,     match->face_handle};
    set_status(status, 0, "");
    return 0;
}

int StepTopologySession::resolve_glb_hit(const StepTopologyGlbWorkPacket& packet,
                                         const StepTopologyGlbHitDescriptor& descriptor,
                                         StepTopologyRenderHit* hit, Status* status) const
{
    if (hit == nullptr)
    {
        set_status(status, kInvalidArgument, "STEP topology GLB-hit output pointer is null.");
        return kInvalidArgument;
    }
    *hit = {};
    if (!is_open())
    {
        set_status(status, kClosed, "STEP topology session is closed.");
        return kClosed;
    }
    if (packet.research_format != "geometer.step_topology_glb_work_packet.research" ||
        descriptor.artifact_handle != packet.artifact_handle ||
        descriptor.content_sha256 != packet.content_sha256 ||
        !verify_glb_work_packet_seal(impl_->data.get(), packet, status))
    {
        if (status == nullptr || status->code == 0)
        {
            set_status(status, kUnknownTarget,
                       "STEP topology GLB artifact identity or session seal is invalid.");
        }
        return kUnknownTarget;
    }
    if (descriptor.instance_index >= packet.render.instances.size())
    {
        set_status(status, kUnknownTarget, "STEP topology GLB instance is out of bounds.");
        return kUnknownTarget;
    }
    const StepTopologyRenderInstance& instance = packet.render.instances[descriptor.instance_index];
    if (instance.mesh_index >= packet.render.meshes.size())
    {
        set_status(status, kUnknownTarget, "STEP topology GLB mesh is out of bounds.");
        return kUnknownTarget;
    }
    const StepTopologyRenderMesh& mesh = packet.render.meshes[instance.mesh_index];
    if (descriptor.primitive_index >= mesh.primitives.size())
    {
        set_status(status, kUnknownTarget, "STEP topology GLB primitive is out of bounds.");
        return kUnknownTarget;
    }
    const StepTopologyRenderPrimitive& primitive = mesh.primitives[descriptor.primitive_index];
    if (descriptor.primitive_triangle_index >= primitive.triangle_count)
    {
        set_status(status, kUnknownTarget,
                   "STEP topology GLB primitive triangle is out of bounds.");
        return kUnknownTarget;
    }
    const std::size_t triangle_index =
        primitive.first_triangle + descriptor.primitive_triangle_index;
    const int code =
        resolve_render_hit(packet.render, descriptor.instance_index, triangle_index, hit, status);
    if (code != 0)
    {
        *hit = {};
        return code;
    }
    if (hit->primitive_index != descriptor.primitive_index ||
        hit->occurrence_handle != descriptor.occurrence_handle ||
        hit->body_handle != descriptor.body_handle || hit->face_handle != descriptor.face_handle)
    {
        *hit = {};
        set_status(status, kUnknownTarget,
                   "STEP topology GLB hit metadata does not match the sealed native target.");
        return kUnknownTarget;
    }
    set_status(status, 0, "");
    return 0;
}

int StepTopologySession::resolve(const std::string& handle, StepTopologyResolvedTarget* target,
                                 Status* status) const
{
    if (target == nullptr || handle.empty())
    {
        set_status(status, kInvalidArgument, "STEP topology resolve arguments are invalid.");
        return kInvalidArgument;
    }
    if (!is_open())
    {
        set_status(status, kClosed, "STEP topology session is closed.");
        return kClosed;
    }
    const auto found = impl_->data->handles.find(handle);
    if (found == impl_->data->handles.end() ||
        found->second.generation != impl_->data->info.generation)
    {
        set_status(status, kUnknownTarget,
                   "STEP topology target is unknown, stale, forged, or from another session.");
        return kUnknownTarget;
    }
    *target = {found->second.kind, handle, found->second.generation};
    set_status(status, 0, "");
    return 0;
}

int StepTopologySession::apply_logical_groups(const StepTopologyGroupTransaction& transaction,
                                              StepTopologyGroupTransactionResult* result,
                                              Status* status)
{
    if (!is_open())
    {
        set_status(status, kClosed, "STEP topology session is closed.");
        return kClosed;
    }
    const int code =
        apply_logical_group_transaction(impl_->data.get(), transaction, result, status);
    if (code == 0)
    {
        impl_->info = impl_->data->info;
    }
    return code;
}

int StepTopologySession::close(Status* status)
{
    if (!is_open())
    {
        set_status(status, kClosed, "STEP topology session is already closed.");
        return kClosed;
    }
    impl_->data.reset();
    set_status(status, 0, "");
    return 0;
}

struct StepTopologySessionStore::Impl
{
    struct Entry
    {
        std::unique_ptr<StepTopologySession> session;
        std::chrono::steady_clock::time_point last_access;
        std::uint64_t access_order = 0;
    };

    explicit Impl(StepTopologyLimits value) : limits(std::move(value)) {}

    StepTopologyLimits limits;
    std::unordered_map<std::string, Entry> sessions;
    std::size_t resident_bytes = 0;
    std::uint64_t access_counter = 0;

    void touch(Entry& entry)
    {
        entry.last_access = std::chrono::steady_clock::now();
        entry.access_order = ++access_counter;
    }

    std::string evict_lru()
    {
        auto oldest = sessions.end();
        for (auto iterator = sessions.begin(); iterator != sessions.end(); ++iterator)
        {
            if (oldest == sessions.end() ||
                iterator->second.access_order < oldest->second.access_order)
            {
                oldest = iterator;
            }
        }
        if (oldest == sessions.end())
        {
            return {};
        }
        const std::string handle = oldest->first;
        resident_bytes -= oldest->second.session->info().estimated_resident_bytes;
        sessions.erase(oldest);
        return handle;
    }
};

StepTopologySessionStore::StepTopologySessionStore(StepTopologyLimits limits)
    : impl_(std::make_unique<Impl>(std::move(limits)))
{
}

StepTopologySessionStore::StepTopologySessionStore(StepTopologySessionStore&&) noexcept = default;
StepTopologySessionStore&
StepTopologySessionStore::operator=(StepTopologySessionStore&&) noexcept = default;
StepTopologySessionStore::~StepTopologySessionStore() = default;

int StepTopologySessionStore::open_step(const unsigned char* source, std::size_t source_size,
                                        StepTopologyOpenResult* result, Status* status)
{
    if (result == nullptr)
    {
        set_status(status, kInvalidArgument, "STEP topology open result pointer is null.");
        return kInvalidArgument;
    }
    std::unique_ptr<StepTopologySession> session;
    const int code =
        StepTopologySession::open_step(source, source_size, impl_->limits, &session, status);
    if (code != 0)
    {
        return code;
    }
    const std::size_t bytes = session->info().estimated_resident_bytes;
    if (bytes > impl_->limits.max_store_estimated_bytes)
    {
        set_status(status, kResourceLimit,
                   "STEP topology session exceeds the store resident-byte limit.");
        return kResourceLimit;
    }

    const std::string handle = session->info().session_handle;
    if (impl_->sessions.count(handle) != 0)
    {
        set_status(status, kInternalFailure, "STEP topology session handle collision.");
        return kInternalFailure;
    }

    StepTopologyOpenResult output;
    output.evicted_session_handles = evict_expired();
    while (impl_->sessions.size() >= impl_->limits.max_sessions ||
           impl_->resident_bytes > impl_->limits.max_store_estimated_bytes - bytes)
    {
        const std::string evicted = impl_->evict_lru();
        if (evicted.empty())
        {
            set_status(status, kResourceLimit, "STEP topology store cannot admit the session.");
            return kResourceLimit;
        }
        output.evicted_session_handles.push_back(evicted);
    }

    output.session = session->info();
    Impl::Entry entry;
    entry.session = std::move(session);
    impl_->touch(entry);
    const auto inserted = impl_->sessions.emplace(handle, std::move(entry));
    if (!inserted.second)
    {
        set_status(status, kInternalFailure, "STEP topology session handle collision.");
        return kInternalFailure;
    }
    impl_->resident_bytes += bytes;
    *result = std::move(output);
    set_status(status, 0, "");
    return 0;
}

int StepTopologySessionStore::inspect(const std::string& session_handle_value,
                                      const StepTopologyInspectionOptions& options,
                                      StepTopologySnapshot* snapshot, Status* status)
{
    evict_expired();
    const auto found = impl_->sessions.find(session_handle_value);
    if (found == impl_->sessions.end())
    {
        set_status(status, kUnknownSession, "STEP topology session is unknown or expired.");
        return kUnknownSession;
    }
    impl_->touch(found->second);
    return found->second.session->inspect(options, snapshot, status);
}

int StepTopologySessionStore::refresh(const std::string& session_handle_value,
                                      StepTopologySnapshot* snapshot, Status* status)
{
    if (snapshot != nullptr)
    {
        *snapshot = {};
    }
    evict_expired();
    const auto found = impl_->sessions.find(session_handle_value);
    if (found == impl_->sessions.end())
    {
        set_status(status, kUnknownSession, "STEP topology session is unknown or expired.");
        return kUnknownSession;
    }
    const std::size_t previous_bytes = found->second.session->info().estimated_resident_bytes;
    const int code = found->second.session->refresh(snapshot, status);
    if (code != 0)
    {
        return code;
    }
    const std::size_t current_bytes = found->second.session->info().estimated_resident_bytes;
    const std::size_t other_bytes = impl_->resident_bytes - previous_bytes;
    if (current_bytes > impl_->limits.max_store_estimated_bytes ||
        other_bytes > impl_->limits.max_store_estimated_bytes - current_bytes)
    {
        impl_->resident_bytes = other_bytes;
        impl_->sessions.erase(found);
        if (snapshot != nullptr)
        {
            *snapshot = {};
        }
        set_status(
            status, kResourceLimit,
            "Refreshed STEP topology session no longer fits the store limit and was evicted.");
        return kResourceLimit;
    }
    impl_->resident_bytes = impl_->resident_bytes - previous_bytes + current_bytes;
    impl_->touch(found->second);
    return 0;
}

int StepTopologySessionStore::apply_logical_groups(const std::string& session_handle_value,
                                                   const StepTopologyGroupTransaction& transaction,
                                                   StepTopologyGroupTransactionResult* result,
                                                   Status* status)
{
    if (result != nullptr)
        *result = {};
    evict_expired();
    const auto found = impl_->sessions.find(session_handle_value);
    if (found == impl_->sessions.end())
    {
        set_status(status, kUnknownSession, "STEP topology session is unknown or expired.");
        return kUnknownSession;
    }
    const std::size_t previous_bytes = found->second.session->info().estimated_resident_bytes;
    const int code = found->second.session->apply_logical_groups(transaction, result, status);
    if (code != 0)
        return code;
    const std::size_t current_bytes = found->second.session->info().estimated_resident_bytes;
    const std::size_t other_bytes = impl_->resident_bytes - previous_bytes;
    if (current_bytes > impl_->limits.max_store_estimated_bytes ||
        other_bytes > impl_->limits.max_store_estimated_bytes - current_bytes)
    {
        impl_->resident_bytes = other_bytes;
        impl_->sessions.erase(found);
        if (result != nullptr)
            *result = {};
        set_status(status, kResourceLimit,
                   "Mutated STEP topology session no longer fits the store limit and was evicted.");
        return kResourceLimit;
    }
    impl_->resident_bytes = other_bytes + current_bytes;
    impl_->touch(found->second);
    return 0;
}

int StepTopologySessionStore::resolve(const std::string& session_handle_value,
                                      const std::string& target_handle,
                                      StepTopologyResolvedTarget* target, Status* status)
{
    evict_expired();
    const auto found = impl_->sessions.find(session_handle_value);
    if (found == impl_->sessions.end())
    {
        set_status(status, kUnknownSession, "STEP topology session is unknown or expired.");
        return kUnknownSession;
    }
    impl_->touch(found->second);
    return found->second.session->resolve(target_handle, target, status);
}

int StepTopologySessionStore::close(const std::string& session_handle_value, Status* status)
{
    const auto found = impl_->sessions.find(session_handle_value);
    if (found == impl_->sessions.end())
    {
        set_status(status, kUnknownSession, "STEP topology session is unknown or already closed.");
        return kUnknownSession;
    }
    impl_->resident_bytes -= found->second.session->info().estimated_resident_bytes;
    impl_->sessions.erase(found);
    set_status(status, 0, "");
    return 0;
}

std::vector<std::string>
StepTopologySessionStore::evict_expired(std::chrono::steady_clock::time_point now)
{
    std::vector<std::string> evicted;
    for (auto iterator = impl_->sessions.begin(); iterator != impl_->sessions.end();)
    {
        if (now - iterator->second.last_access >= impl_->limits.inactivity_timeout)
        {
            evicted.push_back(iterator->first);
            impl_->resident_bytes -= iterator->second.session->info().estimated_resident_bytes;
            iterator = impl_->sessions.erase(iterator);
        }
        else
        {
            ++iterator;
        }
    }
    std::sort(evicted.begin(), evicted.end());
    return evicted;
}

std::vector<std::string> StepTopologySessionStore::clear_for_process_replacement()
{
    std::vector<std::string> invalidated;
    invalidated.reserve(impl_->sessions.size());
    for (const auto& item : impl_->sessions)
    {
        invalidated.push_back(item.first);
    }
    std::sort(invalidated.begin(), invalidated.end());
    impl_->sessions.clear();
    impl_->resident_bytes = 0;
    return invalidated;
}

std::size_t StepTopologySessionStore::size() const
{
    return impl_->sessions.size();
}

std::size_t StepTopologySessionStore::estimated_resident_bytes() const
{
    return impl_->resident_bytes;
}

} // namespace geometer
