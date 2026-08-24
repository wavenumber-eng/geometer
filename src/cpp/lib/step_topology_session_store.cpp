#include "geometer/step_topology_session.h"

#include "step_topology_session_internal.h"

#include <algorithm>
#include <chrono>
#include <cstddef>
#include <cstdint>
#include <limits>
#include <memory>
#include <optional>
#include <string>
#include <unordered_map>
#include <utility>
#include <vector>

namespace geometer
{

using namespace step_topology_internal;

struct StepTopologySessionStore::Impl
{
    struct Entry
    {
        struct RenderArtifact
        {
            std::string artifact_handle;
            std::string content_sha256;
            StepTopologyRenderArtifact render;
        };

        std::unique_ptr<StepTopologySession> session;
        std::optional<RenderArtifact> render_artifact;
        std::size_t render_artifact_bytes = 0;
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

    static std::size_t entry_bytes(const Entry& entry)
    {
        return entry.session->info().estimated_resident_bytes + entry.render_artifact_bytes;
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
        resident_bytes -= entry_bytes(oldest->second);
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

int StepTopologySessionStore::open_step_with_edit_journal(
    const unsigned char* source, std::size_t source_size, const unsigned char* journal,
    std::size_t journal_size, const StepTopologyEditJournalReplayPreconditions& preconditions,
    StepTopologyOpenResult* result, StepTopologyEditJournalRestoreResult* restored_state,
    Status* status)
{
    if (result == nullptr || restored_state == nullptr)
    {
        set_status(status, kInvalidArgument, "STEP topology restore output pointer is null.");
        return kInvalidArgument;
    }
    *result = {};
    *restored_state = {};
    std::unique_ptr<StepTopologySession> session;
    StepTopologyEditJournalRestoreResult restored;
    const int open_code = StepTopologySession::open_step_with_edit_journal(
        source, source_size, journal, journal_size, impl_->limits, &session, &restored, status);
    if (open_code != 0)
        return open_code;

    StepTopologyEditJournalCheckpoint checkpoint;
    const int checkpoint_code = session->checkpoint_edit_journal(&checkpoint, status);
    if (checkpoint_code != 0)
        return checkpoint_code;
    if (checkpoint.source_sha256 != preconditions.source_sha256 ||
        checkpoint.source_brep_sha256 != preconditions.source_brep_sha256 ||
        checkpoint.target_inventory_sha256 != preconditions.target_inventory_sha256 ||
        checkpoint.occt_version != preconditions.occt_version ||
        checkpoint.transaction_count != preconditions.transaction_count)
    {
        set_status(status, kConflict,
                   "STEP topology edit-journal replay preconditions do not match the restored "
                   "source and journal.");
        return kConflict;
    }

    const std::size_t bytes = session->info().estimated_resident_bytes;
    if (bytes > impl_->limits.max_store_estimated_bytes)
    {
        set_status(status, kResourceLimit,
                   "Restored STEP topology session exceeds the store resident-byte limit.");
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
            set_status(status, kResourceLimit,
                       "STEP topology store cannot admit the restored session.");
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
    *restored_state = std::move(restored);
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

int StepTopologySessionStore::inspect_page(const std::string& session_handle_value,
                                           const StepTopologyInspectionOptions& options,
                                           const StepTopologyPagePosition& position,
                                           std::size_t limit, StepTopologySnapshotPage* page,
                                           Status* status)
{
    evict_expired();
    const auto found = impl_->sessions.find(session_handle_value);
    if (found == impl_->sessions.end())
    {
        set_status(status, kUnknownSession, "STEP topology session is unknown or expired.");
        return kUnknownSession;
    }
    impl_->touch(found->second);
    return found->second.session->inspect_page(options, position, limit, page, status);
}

int StepTopologySessionStore::render_glb_work_packet(const std::string& session_handle_value,
                                                     const StepTopologyGlbOptions& options,
                                                     StepTopologyGlbRenderOutput* output,
                                                     Status* status)
{
    if (output == nullptr)
    {
        set_status(status, kInvalidArgument, "STEP topology GLB output pointer is null.");
        return kInvalidArgument;
    }
    *output = {};
    evict_expired();
    const auto found = impl_->sessions.find(session_handle_value);
    if (found == impl_->sessions.end())
    {
        set_status(status, kUnknownSession, "STEP topology session is unknown or expired.");
        return kUnknownSession;
    }
    constexpr std::size_t kRetainedWrapperBudget = sizeof(Impl::Entry::RenderArtifact) + 512U;
    const std::size_t available_bytes =
        impl_->limits.max_store_estimated_bytes - impl_->resident_bytes;
    if (available_bytes <= kRetainedWrapperBudget)
    {
        set_status(status, kResourceLimit,
                   "STEP topology store has no transient render budget available.");
        return kResourceLimit;
    }
    StepTopologyGlbOptions effective_options = options;
    effective_options.transient_byte_limit =
        std::min(effective_options.transient_byte_limit, available_bytes - kRetainedWrapperBudget);
    effective_options.glb_byte_limit =
        std::min(effective_options.glb_byte_limit, std::size_t{256U * 1024U * 1024U});
    StepTopologyGlbWorkPacket packet;
    const int code =
        found->second.session->render_glb_work_packet(effective_options, &packet, status);
    if (code != 0)
        return code;
    std::size_t artifact_bytes = packet.render.estimated_resident_bytes;
    const std::size_t wrapper_bytes = sizeof(Impl::Entry::RenderArtifact) +
                                      packet.artifact_handle.capacity() + 1U +
                                      packet.content_sha256.capacity() + 1U;
    if (wrapper_bytes > std::numeric_limits<std::size_t>::max() - artifact_bytes)
        artifact_bytes = std::numeric_limits<std::size_t>::max();
    else
        artifact_bytes += wrapper_bytes;
    const std::size_t previous_entry_bytes = Impl::entry_bytes(found->second);
    const std::size_t other_bytes = impl_->resident_bytes - previous_entry_bytes;
    const std::size_t session_bytes = found->second.session->info().estimated_resident_bytes;
    if (session_bytes > impl_->limits.max_store_estimated_bytes ||
        artifact_bytes > impl_->limits.max_store_estimated_bytes - session_bytes ||
        other_bytes > impl_->limits.max_store_estimated_bytes - session_bytes - artifact_bytes)
    {
        set_status(status, kResourceLimit,
                   "STEP topology render artifact exceeds the store resident-byte limit.");
        return kResourceLimit;
    }

    output->session = packet.render.session;
    output->artifact_handle = packet.artifact_handle;
    output->content_sha256 = packet.content_sha256;
    output->render_artifact_handle = packet.render.artifact_handle;
    output->render_content_sha256 = packet.render.content_sha256;
    output->mesh_count = packet.render.meshes.size();
    output->instance_count = packet.render.instances.size();
    for (const auto& mesh : packet.render.meshes)
        output->primitive_count += mesh.primitives.size();
    output->geometry_triangle_count = packet.render.geometry_triangle_count;
    output->instanced_triangle_count = packet.render.instanced_triangle_count;
    output->glb = std::move(packet.glb);

    Impl::Entry::RenderArtifact retained;
    retained.artifact_handle = std::move(packet.artifact_handle);
    retained.content_sha256 = std::move(packet.content_sha256);
    retained.render = std::move(packet.render);
    found->second.render_artifact = std::move(retained);
    found->second.render_artifact_bytes = artifact_bytes;
    impl_->resident_bytes = other_bytes + session_bytes + artifact_bytes;
    impl_->touch(found->second);
    set_status(status, 0, "");
    return 0;
}

int StepTopologySessionStore::resolve_glb_hit(const std::string& session_handle_value,
                                              const StepTopologyGlbHitDescriptor& descriptor,
                                              StepTopologyRenderHit* hit, Status* status)
{
    if (hit == nullptr)
    {
        set_status(status, kInvalidArgument, "STEP topology GLB-hit output pointer is null.");
        return kInvalidArgument;
    }
    *hit = {};
    evict_expired();
    const auto found = impl_->sessions.find(session_handle_value);
    if (found == impl_->sessions.end())
    {
        set_status(status, kUnknownSession, "STEP topology session is unknown or expired.");
        return kUnknownSession;
    }
    if (!found->second.render_artifact.has_value() ||
        found->second.render_artifact->artifact_handle != descriptor.artifact_handle ||
        found->second.render_artifact->content_sha256 != descriptor.content_sha256)
    {
        set_status(status, kUnknownTarget,
                   "STEP topology GLB artifact is unknown, stale, or superseded.");
        return kUnknownTarget;
    }
    const auto& render = found->second.render_artifact->render;
    if (descriptor.instance_index >= render.instances.size())
    {
        set_status(status, kUnknownTarget, "STEP topology GLB instance is out of bounds.");
        return kUnknownTarget;
    }
    const auto& instance = render.instances[descriptor.instance_index];
    if (instance.mesh_index >= render.meshes.size() ||
        descriptor.primitive_index >= render.meshes[instance.mesh_index].primitives.size())
    {
        set_status(status, kUnknownTarget, "STEP topology GLB primitive is out of bounds.");
        return kUnknownTarget;
    }
    const auto& primitive =
        render.meshes[instance.mesh_index].primitives[descriptor.primitive_index];
    if (descriptor.primitive_triangle_index >= primitive.triangle_count)
    {
        set_status(status, kUnknownTarget,
                   "STEP topology GLB primitive triangle is out of bounds.");
        return kUnknownTarget;
    }
    if (instance.occurrence_handle != descriptor.occurrence_handle ||
        primitive.body_handle != descriptor.body_handle ||
        primitive.face_handle != descriptor.face_handle)
    {
        *hit = {};
        set_status(status, kUnknownTarget,
                   "STEP topology GLB hit claims do not match the retained render artifact.");
        return kUnknownTarget;
    }
    *hit = {descriptor.instance_index,
            descriptor.primitive_index,
            primitive.first_triangle + descriptor.primitive_triangle_index,
            instance.occurrence_handle,
            primitive.body_handle,
            primitive.face_handle,
            1U};
    impl_->touch(found->second);
    set_status(status, 0, "");
    return 0;
}

int StepTopologySessionStore::info(const std::string& session_handle_value,
                                   StepTopologySessionInfo* info, Status* status)
{
    if (info == nullptr)
    {
        set_status(status, kInvalidArgument, "STEP topology session info pointer is null.");
        return kInvalidArgument;
    }
    *info = {};
    evict_expired();
    const auto found = impl_->sessions.find(session_handle_value);
    if (found == impl_->sessions.end())
    {
        set_status(status, kUnknownSession, "STEP topology session is unknown or expired.");
        return kUnknownSession;
    }
    *info = found->second.session->info();
    impl_->touch(found->second);
    set_status(status, 0, "");
    return 0;
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
    const std::size_t previous_bytes = Impl::entry_bytes(found->second);
    const int code = found->second.session->refresh(snapshot, status);
    if (code != 0)
    {
        return code;
    }
    found->second.render_artifact.reset();
    found->second.render_artifact_bytes = 0U;
    const std::size_t current_bytes = Impl::entry_bytes(found->second);
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
    impl_->resident_bytes = other_bytes + current_bytes;
    impl_->touch(found->second);
    return 0;
}

int StepTopologySessionStore::apply_logical_groups(const std::string& session_handle_value,
                                                   const StepTopologyGroupTransaction& transaction,
                                                   StepTopologyGroupTransactionResult* result,
                                                   Status* status)
{
    return apply_logical_groups(session_handle_value, transaction, nullptr, nullptr, result,
                                status);
}

int StepTopologySessionStore::apply_logical_groups(
    const std::string& session_handle_value, const StepTopologyGroupTransaction& transaction,
    StepTopologyGroupPublicationGate publication_gate, void* publication_context,
    StepTopologyGroupTransactionResult* result, Status* status)
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
    const std::size_t previous_bytes = Impl::entry_bytes(found->second);
    const int code = found->second.session->apply_logical_groups(
        transaction, publication_gate, publication_context, result, status);
    if (code != 0)
        return code;
    found->second.render_artifact.reset();
    found->second.render_artifact_bytes = 0U;
    const std::size_t current_bytes = Impl::entry_bytes(found->second);
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

int StepTopologySessionStore::apply_metadata_probes(const std::string& session_handle_value,
                                                    const StepTopologyProbeTransaction& transaction,
                                                    StepTopologyProbeTransactionResult* result,
                                                    Status* status)
{
    return apply_metadata_probes(session_handle_value, transaction, nullptr, nullptr, result,
                                 status);
}

int StepTopologySessionStore::apply_metadata_probes(
    const std::string& session_handle_value, const StepTopologyProbeTransaction& transaction,
    StepTopologyProbePublicationGate publication_gate, void* publication_context,
    StepTopologyProbeTransactionResult* result, Status* status)
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
    const std::size_t previous_bytes = Impl::entry_bytes(found->second);
    const int code = found->second.session->apply_metadata_probes(
        transaction, publication_gate, publication_context, result, status);
    if (code != 0)
        return code;
    found->second.render_artifact.reset();
    found->second.render_artifact_bytes = 0U;
    const std::size_t current_bytes = Impl::entry_bytes(found->second);
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

int StepTopologySessionStore::checkpoint_edit_journal(const std::string& session_handle_value,
                                                      StepTopologyEditJournalCheckpoint* checkpoint,
                                                      Status* status)
{
    if (checkpoint != nullptr)
        *checkpoint = {};
    evict_expired();
    const auto found = impl_->sessions.find(session_handle_value);
    if (found == impl_->sessions.end())
    {
        set_status(status, kUnknownSession, "STEP topology session is unknown or expired.");
        return kUnknownSession;
    }
    const int code = found->second.session->checkpoint_edit_journal(checkpoint, status);
    if (code == 0)
        impl_->touch(found->second);
    return code;
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
    impl_->resident_bytes -= Impl::entry_bytes(found->second);
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
            impl_->resident_bytes -= Impl::entry_bytes(iterator->second);
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
