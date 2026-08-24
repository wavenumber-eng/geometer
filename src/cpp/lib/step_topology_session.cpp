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
           limits.max_group_transaction_member_references > 0 && limits.max_metadata_probes > 0 &&
           limits.max_edit_journal_transactions > 0 && limits.max_edit_journal_bytes > 0 &&
           limits.max_edit_journal_replay_work_items > 0 &&
           limits.max_hierarchy_transaction_commands > 0 &&
           limits.max_hierarchy_transaction_work_items > 0 && limits.max_total_string_bytes > 0 &&
           limits.max_session_estimated_bytes > 0 && limits.max_store_estimated_bytes > 0 &&
           limits.inactivity_timeout.count() > 0;
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

int StepTopologySession::open_step_with_edit_journal(
    const unsigned char* source, std::size_t source_size, const unsigned char* journal,
    std::size_t journal_size, const StepTopologyLimits& limits,
    std::unique_ptr<StepTopologySession>* session,
    StepTopologyEditJournalRestoreResult* restored_state, Status* status)
{
    return open_step_with_edit_journal(source, source_size, journal, journal_size, limits, nullptr,
                                       session, restored_state, status);
}

int StepTopologySession::open_step_with_edit_journal(
    const unsigned char* source, std::size_t source_size, const unsigned char* journal,
    std::size_t journal_size, const StepTopologyLimits& limits,
    const StepTopologyCancellation* cancellation, std::unique_ptr<StepTopologySession>* session,
    StepTopologyEditJournalRestoreResult* restored_state, Status* status)
{
    if (session == nullptr || restored_state == nullptr)
    {
        set_status(status, kInvalidArgument, "Edit-journal restore output is null.");
        return kInvalidArgument;
    }
    session->reset();
    *restored_state = {};
    try
    {
        std::string journal_source_sha256;
        std::string journal_brep_sha256;
        std::string journal_target_inventory_sha256;
        std::string journal_occt_version;
        std::vector<EditJournalTransactionRecord> transactions;
        const int decode_code =
            decode_edit_journal(journal, journal_size, limits, &journal_source_sha256,
                                &journal_brep_sha256, &journal_target_inventory_sha256,
                                &journal_occt_version, cancellation, &transactions, status);
        if (decode_code != 0)
            return decode_code;

        std::unique_ptr<StepTopologySession> opened;
        const int open_code = open_step(source, source_size, limits, cancellation, &opened, status);
        if (open_code != 0)
            return open_code;
        if (opened->impl_->data->info.source_sha256 != journal_source_sha256 ||
            opened->impl_->data->snapshot.brep_sha256 != journal_brep_sha256 ||
            edit_journal_target_inventory_sha256(opened->impl_->data->snapshot) !=
                journal_target_inventory_sha256 ||
            opened->impl_->data->info.occt_version != journal_occt_version)
        {
            set_status(
                status, kConflict,
                "Edit journal does not match the exact STEP source, ordered target inventory, "
                "B-rep, and OCCT version.");
            return kConflict;
        }
        const int replay_code = replay_edit_journal(opened->impl_->data.get(), transactions,
                                                    cancellation, restored_state, status);
        if (replay_code != 0)
        {
            *restored_state = {};
            return replay_code;
        }
        opened->impl_->info.generation = opened->impl_->data->info.generation;
        opened->impl_->info.edit_journal_revision = opened->impl_->data->info.edit_journal_revision;
        opened->impl_->info.accounted_string_bytes =
            opened->impl_->data->info.accounted_string_bytes;
        opened->impl_->info.estimated_resident_bytes =
            opened->impl_->data->info.estimated_resident_bytes;
        *session = std::move(opened);
        set_status(status, 0, "");
        return 0;
    }
    catch (const Standard_Failure& failure)
    {
        session->reset();
        *restored_state = {};
        set_status(status, kInternalFailure, failure.GetMessageString());
        return kInternalFailure;
    }
    catch (const std::exception& error)
    {
        session->reset();
        *restored_state = {};
        set_status(status, kInternalFailure, error.what());
        return kInternalFailure;
    }
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
        const int journal_code = initialize_edit_journal_accounting(data.get(), status);
        if (journal_code != 0)
        {
            return journal_code;
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

int StepTopologySession::inspect_page(const StepTopologyInspectionOptions& options,
                                      const StepTopologyPagePosition& position, std::size_t limit,
                                      StepTopologySnapshotPage* page, Status* status) const
{
    if (page == nullptr || limit == 0U || limit > 1024U)
    {
        set_status(status, kInvalidArgument, "STEP topology page output or limit is invalid.");
        return kInvalidArgument;
    }
    *page = {};
    if (!is_open())
    {
        set_status(status, kClosed, "STEP topology session is closed.");
        return kClosed;
    }
    if (options.include_diagnostic_carriers)
    {
        set_status(status, kInvalidArgument,
                   "Diagnostic-carrier projection is deferred from the native wire page.");
        return kInvalidArgument;
    }

    const StepTopologySnapshot& snapshot = impl_->data->snapshot;
    page->session = snapshot.session;
    page->definition_count = snapshot.definitions.size();
    page->root_occurrence_count = snapshot.root_occurrences.size();
    page->component_occurrence_count = snapshot.occurrences.size();
    page->body_count = snapshot.bodies.size();
    page->shell_count = snapshot.shells.size();
    page->face_count = snapshot.faces.size();
    page->membership_count = snapshot.membership_count;

    StepTopologyPagePosition current = position;
    if (current.section > 9U)
    {
        set_status(status, kInvalidArgument, "STEP topology page position section is invalid.");
        return kInvalidArgument;
    }
    auto valid_position = [&](std::size_t size, bool nested)
    { return current.record <= size && (nested || current.member == 0U); };
    const std::size_t section_sizes[] = {
        snapshot.definitions.size(), snapshot.root_occurrences.size(),
        snapshot.occurrences.size(), snapshot.bodies.size(),
        snapshot.shells.size(),      snapshot.faces.size(),
        snapshot.bodies.size(),      snapshot.bodies.size(),
        snapshot.shells.size(),      0U,
    };
    if (!valid_position(section_sizes[current.section], current.section >= 6U) ||
        (current.record == section_sizes[current.section] && current.member != 0U))
    {
        set_status(status, kInvalidArgument, "STEP topology page position is out of bounds.");
        return kInvalidArgument;
    }
    if (current.section == 6U && current.record < snapshot.bodies.size() &&
        current.member > snapshot.bodies[current.record].shell_handles.size())
    {
        set_status(status, kInvalidArgument, "STEP topology page member is out of bounds.");
        return kInvalidArgument;
    }
    if (current.section == 7U && current.record < snapshot.bodies.size() &&
        current.member > snapshot.bodies[current.record].face_handles.size())
    {
        set_status(status, kInvalidArgument, "STEP topology page member is out of bounds.");
        return kInvalidArgument;
    }
    if (current.section == 8U && current.record < snapshot.shells.size() &&
        current.member > snapshot.shells[current.record].face_handles.size())
    {
        set_status(status, kInvalidArgument, "STEP topology page member is out of bounds.");
        return kInvalidArgument;
    }

    auto clear_evidence = [&](auto* item)
    {
        if (!options.include_source_entity_evidence)
            item->source_entity = {};
    };
    std::size_t added = 0U;
    while (added < limit && current.section < 9U)
    {
        if (current.section == 0U && current.record < snapshot.definitions.size())
        {
            const auto& source = snapshot.definitions[current.record++];
            page->definitions.push_back({source.handle, source.is_assembly, source.label,
                                         source.body_handles.size(), source.face_count,
                                         source.source_entity});
            clear_evidence(&page->definitions.back());
            ++added;
        }
        else if (current.section == 1U && current.record < snapshot.root_occurrences.size())
        {
            page->root_occurrences.push_back(snapshot.root_occurrences[current.record++]);
            ++added;
        }
        else if (current.section == 2U && current.record < snapshot.occurrences.size())
        {
            page->occurrences.push_back(snapshot.occurrences[current.record++]);
            ++added;
        }
        else if (current.section == 3U && current.record < snapshot.bodies.size())
        {
            const auto& source = snapshot.bodies[current.record++];
            page->bodies.push_back({source.handle, source.definition_handle, source.topology_kind,
                                    source.shell_handles.size(), source.face_handles.size(),
                                    source.bounds, source.volume, source.label,
                                    source.source_entity});
            clear_evidence(&page->bodies.back());
            ++added;
        }
        else if (current.section == 4U && current.record < snapshot.shells.size())
        {
            const auto& source = snapshot.shells[current.record++];
            page->shells.push_back({source.handle, source.definition_handle,
                                    source.body_handles.size(), source.face_handles.size(),
                                    source.label, source.source_entity});
            clear_evidence(&page->shells.back());
            ++added;
        }
        else if (current.section == 5U && current.record < snapshot.faces.size())
        {
            const auto& source = snapshot.faces[current.record++];
            page->faces.push_back({source.handle, source.definition_handle,
                                   source.body_handles.size(), source.shell_handles.size(),
                                   source.bounds, source.area, source.centroid, source.label,
                                   source.source_entity});
            clear_evidence(&page->faces.back());
            ++added;
        }
        else if (current.section >= 6U)
        {
            const std::vector<std::string>* members = nullptr;
            std::string owner;
            StepTopologyMembershipKind kind = StepTopologyMembershipKind::body_shell;
            if (current.section == 6U && current.record < snapshot.bodies.size())
            {
                owner = snapshot.bodies[current.record].handle;
                members = &snapshot.bodies[current.record].shell_handles;
            }
            else if (current.section == 7U && current.record < snapshot.bodies.size())
            {
                owner = snapshot.bodies[current.record].handle;
                members = &snapshot.bodies[current.record].face_handles;
                kind = StepTopologyMembershipKind::body_face;
            }
            else if (current.section == 8U && current.record < snapshot.shells.size())
            {
                owner = snapshot.shells[current.record].handle;
                members = &snapshot.shells[current.record].face_handles;
                kind = StepTopologyMembershipKind::shell_face;
            }
            if (members != nullptr && current.member < members->size())
            {
                page->memberships.push_back({kind, owner, (*members)[current.member++]});
                ++added;
            }
            else if (current.record < section_sizes[current.section])
            {
                ++current.record;
                current.member = 0U;
            }
            else
            {
                ++current.section;
                current.record = 0U;
                current.member = 0U;
            }
        }
        else
        {
            ++current.section;
            current.record = 0U;
            current.member = 0U;
        }
    }
    for (;;)
    {
        if (current.section < 6U && current.record == section_sizes[current.section])
        {
            ++current.section;
            current.record = 0U;
            current.member = 0U;
            continue;
        }
        if (current.section >= 6U && current.section < 9U)
        {
            if (current.record == section_sizes[current.section])
            {
                ++current.section;
                current.record = 0U;
                current.member = 0U;
                continue;
            }
            const std::size_t member_size =
                current.section == 6U   ? snapshot.bodies[current.record].shell_handles.size()
                : current.section == 7U ? snapshot.bodies[current.record].face_handles.size()
                                        : snapshot.shells[current.record].face_handles.size();
            if (current.member == member_size)
            {
                ++current.record;
                current.member = 0U;
                continue;
            }
        }
        break;
    }
    page->next = current;
    page->has_next = current.section < 9U;
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
    return build_render_artifact(impl_->data.get(), options, cancellation,
                                 impl_->data->limits.max_render_estimated_bytes, artifact, status);
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
        artifact.session.edit_journal_revision != impl_->data->info.edit_journal_revision ||
        artifact.session.accounted_string_bytes != impl_->data->info.accounted_string_bytes ||
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
    return apply_logical_groups(transaction, nullptr, nullptr, result, status);
}

int StepTopologySession::apply_logical_groups(const StepTopologyGroupTransaction& transaction,
                                              StepTopologyGroupPublicationGate publication_gate,
                                              void* publication_context,
                                              StepTopologyGroupTransactionResult* result,
                                              Status* status)
{
    if (!is_open())
    {
        set_status(status, kClosed, "STEP topology session is closed.");
        return kClosed;
    }
    const int code =
        apply_logical_group_transaction(impl_->data.get(), transaction, nullptr, publication_gate,
                                        publication_context, result, status);
    if (code == 0)
    {
        // Identity/source strings are immutable after open. Updating only mutable scalar state
        // keeps successful transaction publication free of a late allocating copy.
        impl_->info.generation = impl_->data->info.generation;
        impl_->info.edit_journal_revision = impl_->data->info.edit_journal_revision;
        impl_->info.accounted_string_bytes = impl_->data->info.accounted_string_bytes;
        impl_->info.estimated_resident_bytes = impl_->data->info.estimated_resident_bytes;
    }
    return code;
}

int StepTopologySession::checkpoint_edit_journal(StepTopologyEditJournalCheckpoint* checkpoint,
                                                 Status* status) const
{
    if (!is_open())
    {
        if (checkpoint != nullptr)
            *checkpoint = {};
        set_status(status, kClosed, "STEP topology session is closed.");
        return kClosed;
    }
    return encode_edit_journal(impl_->data.get(), checkpoint, status);
}

int StepTopologySession::apply_metadata_probes(const StepTopologyProbeTransaction& transaction,
                                               StepTopologyProbeTransactionResult* result,
                                               Status* status)
{
    return apply_metadata_probes(transaction, nullptr, nullptr, result, status);
}

int StepTopologySession::apply_metadata_probes(const StepTopologyProbeTransaction& transaction,
                                               StepTopologyProbePublicationGate publication_gate,
                                               void* publication_context,
                                               StepTopologyProbeTransactionResult* result,
                                               Status* status)
{
    if (!is_open())
    {
        if (result != nullptr)
            *result = {};
        set_status(status, kClosed, "STEP topology session is closed.");
        return kClosed;
    }
    const int code =
        apply_metadata_probe_transaction(impl_->data.get(), transaction, nullptr, publication_gate,
                                         publication_context, result, status);
    if (code == 0)
    {
        impl_->info.generation = impl_->data->info.generation;
        impl_->info.edit_journal_revision = impl_->data->info.edit_journal_revision;
        impl_->info.accounted_string_bytes = impl_->data->info.accounted_string_bytes;
        impl_->info.estimated_resident_bytes = impl_->data->info.estimated_resident_bytes;
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

} // namespace geometer
