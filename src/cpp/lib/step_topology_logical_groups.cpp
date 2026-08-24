#include "step_topology_session_internal.h"

#include <NCollection_DataMap.hxx>
#include <Standard_Failure.hxx>
#include <TopTools_ShapeMapHasher.hxx>

#include <algorithm>
#include <cctype>
#include <cstddef>
#include <cstdint>
#include <exception>
#include <limits>
#include <string>
#include <string_view>
#include <unordered_map>
#include <unordered_set>
#include <utility>
#include <vector>

namespace geometer::step_topology_internal
{
namespace
{

using ShapeMap = NCollection_DataMap<TopoDS_Shape, bool, TopTools_ShapeMapHasher>;

struct TargetLookup
{
    StepTopologyTargetKind kind = StepTopologyTargetKind::face;
    std::string handle;
    bool duplicate = false;
};

using TargetMap = NCollection_DataMap<TopoDS_Shape, TargetLookup, TopTools_ShapeMapHasher>;

bool checked_add(std::size_t* total, std::size_t value)
{
    if (*total > std::numeric_limits<std::size_t>::max() - value)
        return false;
    *total += value;
    return true;
}

bool cancellation_requested(const StepTopologyCancellation* cancellation, Status* status)
{
    if (cancellation == nullptr || !cancellation->is_cancelled())
        return false;
    set_status(status, kCancelled, "Logical group replay transaction was cancelled.");
    return true;
}

bool valid_authored_id(const std::string& value)
{
    constexpr std::string_view prefix = "wn.geometer.research.group.";
    if (value.size() <= prefix.size() || value.size() > 128U || value.rfind(prefix, 0) != 0)
        return false;
    return std::all_of(value.begin() + static_cast<std::ptrdiff_t>(prefix.size()), value.end(),
                       [](unsigned char character)
                       {
                           return (character >= 'a' && character <= 'z') ||
                                  (character >= 'A' && character <= 'Z') ||
                                  (character >= '0' && character <= '9') || character == '.' ||
                                  character == '_' || character == '-';
                       });
}

bool group_state_within_limits(const SessionData* data,
                               const std::vector<LogicalGroupRecord>& groups,
                               std::size_t base_string_bytes, std::size_t* estimated_bytes)
{
    if (groups.size() > data->limits.max_logical_groups ||
        groups.capacity() > std::numeric_limits<std::size_t>::max() / sizeof(LogicalGroupRecord))
        return false;

    std::size_t member_count = 0;
    std::size_t string_bytes = base_string_bytes;
    std::size_t bytes = groups.capacity() * sizeof(LogicalGroupRecord);
    for (const LogicalGroupRecord& group : groups)
    {
        if (group.authored_id.size() > data->limits.max_string_bytes ||
            group.name.size() > data->limits.max_string_bytes ||
            group.members.capacity() >
                std::numeric_limits<std::size_t>::max() / sizeof(LogicalGroupMemberRecord) ||
            !checked_add(&member_count, group.members.size()) ||
            !checked_add(&string_bytes, group.authored_id.size()) ||
            !checked_add(&string_bytes, group.name.size()) ||
            !checked_add(&bytes, group.authored_id.capacity() + 1U) ||
            !checked_add(&bytes, group.name.capacity() + 1U) ||
            !checked_add(&bytes, group.members.capacity() * sizeof(LogicalGroupMemberRecord)))
            return false;
    }
    if (member_count > data->limits.max_group_members ||
        string_bytes > data->limits.max_total_string_bytes)
        return false;
    *estimated_bytes = bytes;
    return true;
}

bool update_bounded_total(std::size_t* total, std::size_t removed, std::size_t added,
                          std::size_t limit)
{
    if (removed > *total)
        return false;
    const std::size_t retained = *total - removed;
    if (added > limit || retained > limit - added)
        return false;
    *total = retained + added;
    return true;
}

int resolve_members(const SessionData* data, const std::vector<std::string>& handles,
                    const StepTopologyCancellation* cancellation,
                    std::vector<LogicalGroupMemberRecord>* members, Status* status)
{
    members->clear();
    if (handles.empty() || handles.size() > data->limits.max_group_members)
    {
        set_status(status, kResourceLimit,
                   "Logical group members are empty or exceed the configured limit.");
        return kResourceLimit;
    }
    members->reserve(handles.size());
    ShapeMap seen;
    for (const std::string& handle : handles)
    {
        if (cancellation_requested(cancellation, status))
            return kCancelled;
        const auto found = data->handles.find(handle);
        if (found == data->handles.end() || found->second.generation != data->info.generation)
        {
            set_status(status, kUnknownTarget, "Logical group member handle is stale or unknown.");
            return kUnknownTarget;
        }
        if (found->second.kind != StepTopologyTargetKind::body &&
            found->second.kind != StepTopologyTargetKind::face)
        {
            set_status(status, kInvalidArgument,
                       "Logical groups accept only body and face targets.");
            return kInvalidArgument;
        }
        if (seen.IsBound(found->second.shape))
        {
            set_status(status, kConflict, "Logical group contains a duplicate member.");
            return kConflict;
        }
        seen.Bind(found->second.shape, true);
        members->push_back({found->second.kind, found->second.shape});
    }
    return 0;
}

int publish_groups_impl(const SessionData* data, const std::vector<LogicalGroupRecord>& groups,
                        const StepTopologyCancellation* cancellation,
                        std::vector<StepTopologyLogicalGroup>* published, Status* status)
{
    TargetMap targets;
    for (const auto& [handle, candidate] : data->handles)
    {
        if (cancellation_requested(cancellation, status))
            return kCancelled;
        if (candidate.generation != data->info.generation ||
            (candidate.kind != StepTopologyTargetKind::body &&
             candidate.kind != StepTopologyTargetKind::face))
            continue;
        TargetLookup* existing = targets.ChangeSeek(candidate.shape);
        if (existing == nullptr)
        {
            targets.Bind(candidate.shape, TargetLookup{candidate.kind, handle, false});
        }
        else if (existing->kind != candidate.kind || existing->handle != handle)
        {
            existing->duplicate = true;
        }
    }

    published->reserve(groups.size());
    for (const LogicalGroupRecord& stored : groups)
    {
        if (cancellation_requested(cancellation, status))
            return kCancelled;
        StepTopologyLogicalGroup group;
        group.authored_id = stored.authored_id;
        group.revision = stored.revision;
        group.name = stored.name;
        group.members.reserve(stored.members.size());
        for (const LogicalGroupMemberRecord& member : stored.members)
        {
            if (cancellation_requested(cancellation, status))
                return kCancelled;
            const TargetLookup* target = targets.Seek(member.shape);
            if (target == nullptr)
            {
                set_status(status, kUnknownTarget,
                           "Logical group member did not survive the generation refresh.");
                return kUnknownTarget;
            }
            if (target->duplicate || target->kind != member.kind)
            {
                set_status(status, kConflict,
                           "Logical group member resolves to multiple current targets.");
                return kConflict;
            }
            group.members.push_back({member.kind, target->handle});
        }
        published->push_back(std::move(group));
    }
    return 0;
}

void clear_result(StepTopologyGroupTransactionResult* result) noexcept
{
    result->session.session_handle.clear();
    result->session.generation = 0;
    result->session.source_sha256.clear();
    result->session.occt_version.clear();
    result->session.source_bytes = 0;
    result->session.edit_journal_revision = 0;
    result->session.accounted_string_bytes = 0;
    result->session.estimated_resident_bytes = 0;
    result->groups.clear();
}

void publish_result(StepTopologyGroupTransactionResult* destination,
                    StepTopologyGroupTransactionResult* source) noexcept
{
    destination->session.session_handle.swap(source->session.session_handle);
    destination->session.generation = source->session.generation;
    destination->session.source_sha256.swap(source->session.source_sha256);
    destination->session.occt_version.swap(source->session.occt_version);
    destination->session.source_bytes = source->session.source_bytes;
    destination->session.edit_journal_revision = source->session.edit_journal_revision;
    destination->session.accounted_string_bytes = source->session.accounted_string_bytes;
    destination->session.estimated_resident_bytes = source->session.estimated_resident_bytes;
    destination->groups.swap(source->groups);
}

} // namespace

int publish_logical_groups(const SessionData* data, const std::vector<LogicalGroupRecord>& groups,
                           const StepTopologyCancellation* cancellation,
                           std::vector<StepTopologyLogicalGroup>* published, Status* status)
{
    if (published == nullptr)
    {
        set_status(status, kInvalidArgument, "Logical-group publication output is null.");
        return kInvalidArgument;
    }
    published->clear();
    return publish_groups_impl(data, groups, cancellation, published, status);
}

int account_logical_group_strings(SessionData* data, const StepTopologyCancellation* cancellation,
                                  Status* status)
{
    for (const LogicalGroupRecord& group : data->logical_groups)
    {
        if (cancellation_requested(cancellation, status))
            return kCancelled;
        if (!account_string(data, group.authored_id, status) ||
            !account_string(data, group.name, status))
            return kResourceLimit;
    }
    return 0;
}

int apply_logical_group_transaction(SessionData* data,
                                    const StepTopologyGroupTransaction& transaction,
                                    const StepTopologyCancellation* cancellation,
                                    StepTopologyGroupPublicationGate publication_gate,
                                    void* publication_context,
                                    StepTopologyGroupTransactionResult* result, Status* status)
{
    if (result == nullptr)
    {
        set_status(status, kInvalidArgument, "Logical group transaction output is null.");
        return kInvalidArgument;
    }
    clear_result(result);
    if (cancellation_requested(cancellation, status))
        return kCancelled;
    if (transaction.expected_generation == 0 ||
        transaction.expected_generation != data->info.generation)
    {
        set_status(status, kConflict, "Logical group transaction generation is stale.");
        return kConflict;
    }
    if (transaction.commands.empty() ||
        transaction.commands.size() > data->limits.max_logical_groups)
    {
        set_status(status, kResourceLimit,
                   "Logical group transaction is empty or exceeds the command limit.");
        return kResourceLimit;
    }
    std::size_t transaction_member_references = 0;
    for (const StepTopologyGroupCommand& command : transaction.commands)
    {
        if (!checked_add(&transaction_member_references, command.member_handles.size()) ||
            transaction_member_references > data->limits.max_group_transaction_member_references)
        {
            set_status(status, kResourceLimit,
                       "Logical group transaction member references exceed the configured limit.");
            return kResourceLimit;
        }
    }

    StepTopologySnapshot previous_snapshot;
    std::unordered_map<std::string, HandleRecord> previous_handles;
    StepTopologySessionInfo previous_info;
    std::vector<LogicalGroupRecord> previous_groups;
    std::vector<EditJournalTransactionRecord> previous_journal;
    std::size_t previous_snapshot_string_bytes = 0;
    std::size_t previous_journal_string_bytes = 0;
    std::size_t previous_total_string_bytes = 0;
    std::size_t previous_journal_encoded_bytes = 0;
    std::uint64_t previous_counter = 0;
    bool mutation_started = false;
    const auto rollback = [&]() noexcept
    {
        if (!mutation_started)
            return;
        data->snapshot = std::move(previous_snapshot);
        data->handles = std::move(previous_handles);
        data->info = std::move(previous_info);
        data->logical_groups = std::move(previous_groups);
        data->edit_journal = std::move(previous_journal);
        data->snapshot_string_bytes = previous_snapshot_string_bytes;
        data->journal_string_bytes = previous_journal_string_bytes;
        data->total_string_bytes = previous_total_string_bytes;
        data->edit_journal_encoded_bytes = previous_journal_encoded_bytes;
        data->handle_counter = previous_counter;
        mutation_started = false;
    };

    try
    {
        using GroupMap = std::unordered_map<std::string, LogicalGroupRecord>;
        EditJournalTransactionRecord journal_entry;
        std::size_t journal_entry_string_bytes = 0;
        const int journal_stage_code = stage_edit_journal_transaction(
            data, transaction, cancellation, &journal_entry, &journal_entry_string_bytes, status);
        if (journal_stage_code != 0)
            return journal_stage_code;
        std::size_t projected_journal_bytes = 0;
        const int journal_size_code =
            validate_edit_journal_append(data, journal_entry, &projected_journal_bytes, status);
        if (journal_size_code != 0)
            return journal_size_code;
        std::size_t future_journal_string_bytes = data->journal_string_bytes;
        std::size_t future_base_string_bytes = data->snapshot_string_bytes;
        if (!checked_add(&future_journal_string_bytes, journal_entry_string_bytes) ||
            !checked_add(&future_base_string_bytes, data->metadata_probe_string_bytes) ||
            !checked_add(&future_base_string_bytes, future_journal_string_bytes) ||
            future_base_string_bytes > data->limits.max_total_string_bytes)
        {
            set_status(status, kResourceLimit,
                       "Edit-journal strings exceed the configured session limit.");
            return kResourceLimit;
        }
        GroupMap candidate_by_id;
        candidate_by_id.reserve(data->logical_groups.size() + transaction.commands.size());
        std::unordered_set<std::string> probed_group_ids;
        probed_group_ids.reserve(data->metadata_probes.size());
        for (const MetadataProbeRecord& probe : data->metadata_probes)
        {
            if (cancellation_requested(cancellation, status))
                return kCancelled;
            if (probe.target_kind == StepTopologyProbeTargetKind::logical_group)
                probed_group_ids.emplace(probe.group_authored_id);
        }
        std::size_t member_count = 0;
        std::size_t group_string_bytes = 0;
        for (const LogicalGroupRecord& stored : data->logical_groups)
        {
            if (cancellation_requested(cancellation, status))
                return kCancelled;
            if (!checked_add(&member_count, stored.members.size()) ||
                !checked_add(&group_string_bytes, stored.authored_id.size()) ||
                !checked_add(&group_string_bytes, stored.name.size()) ||
                !candidate_by_id.emplace(stored.authored_id, stored).second)
            {
                set_status(status, kInternalFailure,
                           "Stored logical group state is duplicate or exceeds accounting bounds.");
                return kInternalFailure;
            }
        }

        for (const StepTopologyGroupCommand& command : transaction.commands)
        {
            if (cancellation_requested(cancellation, status))
                return kCancelled;
            if (!valid_authored_id(command.authored_id) ||
                command.authored_id.size() > data->limits.max_string_bytes ||
                command.name.size() > data->limits.max_string_bytes)
            {
                set_status(status, kInvalidArgument, "Logical group id or name is invalid.");
                return kInvalidArgument;
            }
            auto group = candidate_by_id.find(command.authored_id);
            if (command.kind == StepTopologyGroupCommandKind::create)
            {
                if (group != candidate_by_id.end() || command.expected_revision != 0 ||
                    command.name.empty())
                {
                    set_status(status, kConflict, "Logical group create precondition failed.");
                    return kConflict;
                }
                LogicalGroupRecord created;
                created.authored_id = command.authored_id;
                created.revision = 1;
                created.name = command.name;
                const int code = resolve_members(data, command.member_handles, cancellation,
                                                 &created.members, status);
                if (code != 0)
                    return code;
                if (candidate_by_id.size() >= data->limits.max_logical_groups ||
                    !update_bounded_total(&member_count, 0, created.members.size(),
                                          data->limits.max_group_members) ||
                    !update_bounded_total(&group_string_bytes, 0,
                                          created.authored_id.size() + created.name.size(),
                                          data->limits.max_total_string_bytes))
                {
                    set_status(status, kResourceLimit,
                               "Logical group state exceeds configured count limits.");
                    return kResourceLimit;
                }
                candidate_by_id.emplace(created.authored_id, std::move(created));
            }
            else
            {
                if (group == candidate_by_id.end() ||
                    command.expected_revision != group->second.revision)
                {
                    set_status(status, kConflict, "Logical group revision precondition failed.");
                    return kConflict;
                }
                LogicalGroupRecord& stored = group->second;
                if (command.kind == StepTopologyGroupCommandKind::rename)
                {
                    if (command.name.empty() || !command.member_handles.empty() ||
                        stored.revision == std::numeric_limits<std::uint64_t>::max())
                    {
                        set_status(status, kInvalidArgument,
                                   "Logical group rename shape is invalid.");
                        return kInvalidArgument;
                    }
                    if (!update_bounded_total(&group_string_bytes, stored.name.size(),
                                              command.name.size(),
                                              data->limits.max_total_string_bytes))
                    {
                        set_status(status, kResourceLimit,
                                   "Logical group strings exceed the configured limit.");
                        return kResourceLimit;
                    }
                    stored.name = command.name;
                    ++stored.revision;
                }
                else if (command.kind == StepTopologyGroupCommandKind::replace_members)
                {
                    if (!command.name.empty() ||
                        stored.revision == std::numeric_limits<std::uint64_t>::max())
                    {
                        set_status(status, kInvalidArgument,
                                   "Logical group member replacement cannot rename the group.");
                        return kInvalidArgument;
                    }
                    std::vector<LogicalGroupMemberRecord> members;
                    const int code = resolve_members(data, command.member_handles, cancellation,
                                                     &members, status);
                    if (code != 0)
                        return code;
                    if (!update_bounded_total(&member_count, stored.members.size(), members.size(),
                                              data->limits.max_group_members))
                    {
                        set_status(status, kResourceLimit,
                                   "Logical group members exceed the configured limit.");
                        return kResourceLimit;
                    }
                    stored.members = std::move(members);
                    ++stored.revision;
                }
                else if (command.kind == StepTopologyGroupCommandKind::erase)
                {
                    if (!command.name.empty() || !command.member_handles.empty())
                    {
                        set_status(status, kInvalidArgument,
                                   "Logical group erase shape is invalid.");
                        return kInvalidArgument;
                    }
                    if (probed_group_ids.count(stored.authored_id) != 0)
                    {
                        set_status(status, kConflict,
                                   "Logical group has attached metadata probes.");
                        return kConflict;
                    }
                    member_count -= stored.members.size();
                    group_string_bytes -= stored.authored_id.size() + stored.name.size();
                    candidate_by_id.erase(group);
                }
                else
                {
                    set_status(status, kInvalidArgument, "Unknown logical group command kind.");
                    return kInvalidArgument;
                }
            }
            if (group_string_bytes > data->limits.max_total_string_bytes - future_base_string_bytes)
            {
                set_status(status, kResourceLimit,
                           "Session-wide strings exceed the configured limit.");
                return kResourceLimit;
            }
        }

        std::vector<LogicalGroupRecord> candidate;
        candidate.reserve(candidate_by_id.size());
        for (auto& [authored_id, group] : candidate_by_id)
        {
            if (cancellation_requested(cancellation, status))
                return kCancelled;
            (void)authored_id;
            candidate.push_back(std::move(group));
        }
        std::sort(candidate.begin(), candidate.end(), [](const auto& left, const auto& right)
                  { return left.authored_id < right.authored_id; });
        std::size_t ignored_bytes = 0;
        if (!group_state_within_limits(data, candidate, future_base_string_bytes, &ignored_bytes))
        {
            set_status(status, kResourceLimit,
                       "Logical group state exceeds configured member, string, or byte limits.");
            return kResourceLimit;
        }
        if (data->info.generation == std::numeric_limits<std::uint64_t>::max())
        {
            set_status(status, kResourceLimit, "Logical group generation is exhausted.");
            return kResourceLimit;
        }

        std::vector<EditJournalTransactionRecord> candidate_journal = data->edit_journal;
        candidate_journal.push_back(std::move(journal_entry));

        previous_info = data->info;
        previous_snapshot = std::move(data->snapshot);
        previous_handles = std::move(data->handles);
        previous_groups = std::move(data->logical_groups);
        previous_journal = std::move(data->edit_journal);
        previous_snapshot_string_bytes = data->snapshot_string_bytes;
        previous_journal_string_bytes = data->journal_string_bytes;
        previous_total_string_bytes = data->total_string_bytes;
        previous_journal_encoded_bytes = data->edit_journal_encoded_bytes;
        previous_counter = data->handle_counter;
        mutation_started = true;

        data->logical_groups = std::move(candidate);
        data->edit_journal = std::move(candidate_journal);
        data->edit_journal_encoded_bytes = projected_journal_bytes;
        ++data->info.generation;
        const int refresh_code = rebuild_snapshot(data, cancellation, status);
        if (refresh_code != 0)
        {
            rollback();
            return refresh_code;
        }

        StepTopologyGroupTransactionResult published;
        published.session = data->info;
        if (cancellation_requested(cancellation, status))
        {
            rollback();
            return kCancelled;
        }
        const int publish_code = publish_logical_groups(data, data->logical_groups, cancellation,
                                                        &published.groups, status);
        if (publish_code != 0)
        {
            rollback();
            return publish_code;
        }
        if (publication_gate != nullptr)
        {
            const int gate_code = publication_gate(published, publication_context, status);
            if (gate_code != 0)
            {
                rollback();
                return gate_code;
            }
        }
        publish_result(result, &published);
        mutation_started = false;
        set_status(status, 0, "");
        return 0;
    }
    catch (const Standard_Failure& failure)
    {
        rollback();
        clear_result(result);
        set_status(status, kInternalFailure, failure.GetMessageString());
        return kInternalFailure;
    }
    catch (const std::exception& error)
    {
        rollback();
        clear_result(result);
        set_status(status, kInternalFailure, error.what());
        return kInternalFailure;
    }
    catch (...)
    {
        rollback();
        clear_result(result);
        set_status(status, kInternalFailure, "Unknown logical group transaction failure.");
        return kInternalFailure;
    }
}

} // namespace geometer::step_topology_internal
