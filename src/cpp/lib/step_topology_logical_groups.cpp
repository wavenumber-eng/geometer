#include "step_topology_session_internal.h"

#include <algorithm>
#include <cctype>
#include <cstddef>
#include <cstdint>
#include <limits>
#include <string>
#include <unordered_set>
#include <utility>
#include <vector>

namespace geometer::step_topology_internal
{
namespace
{

bool valid_authored_id(const std::string& value)
{
    constexpr const char* prefix = "wn.geometer.research.group.";
    if (value.size() <= 27U || value.size() > 128U || value.rfind(prefix, 0) != 0)
        return false;
    return std::all_of(value.begin() + 27, value.end(),
                       [](unsigned char character)
                       {
                           return std::isalnum(character) || character == '.' || character == '_' ||
                                  character == '-';
                       });
}

auto find_group(std::vector<LogicalGroupRecord>* groups, const std::string& authored_id)
{
    return std::find_if(groups->begin(), groups->end(), [&authored_id](const auto& group)
                        { return group.authored_id == authored_id; });
}

bool same_member(const LogicalGroupMemberRecord& left, const LogicalGroupMemberRecord& right)
{
    return left.kind == right.kind && left.shape.IsSame(right.shape);
}

bool candidate_within_limits(const SessionData* data, const std::vector<LogicalGroupRecord>& groups,
                             std::size_t* estimated_bytes)
{
    std::size_t member_count = 0;
    std::size_t string_bytes = 0;
    if (groups.capacity() > std::numeric_limits<std::size_t>::max() / sizeof(LogicalGroupRecord))
        return false;
    std::size_t bytes = groups.capacity() * sizeof(LogicalGroupRecord);
    const auto add = [](std::size_t* total, std::size_t value)
    {
        if (*total > std::numeric_limits<std::size_t>::max() - value)
            return false;
        *total += value;
        return true;
    };
    for (const auto& group : groups)
    {
        if (group.members.capacity() >
                std::numeric_limits<std::size_t>::max() / sizeof(LogicalGroupMemberRecord) ||
            !add(&member_count, group.members.size()) ||
            !add(&string_bytes, group.authored_id.size()) ||
            !add(&string_bytes, group.name.size()) || !add(&bytes, group.authored_id.capacity()) ||
            !add(&bytes, group.name.capacity()) ||
            !add(&bytes, group.members.capacity() * sizeof(LogicalGroupMemberRecord)))
            return false;
    }
    if (member_count > data->limits.max_group_members ||
        string_bytes > data->limits.max_total_string_bytes)
        return false;
    *estimated_bytes = bytes;
    return true;
}

int resolve_members(const SessionData* data, const std::vector<std::string>& handles,
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
    for (const std::string& handle : handles)
    {
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
        LogicalGroupMemberRecord member{found->second.kind, found->second.shape};
        if (std::any_of(members->begin(), members->end(),
                        [&member](const auto& existing) { return same_member(existing, member); }))
        {
            set_status(status, kConflict, "Logical group contains a duplicate member.");
            return kConflict;
        }
        members->push_back(std::move(member));
    }
    return 0;
}

int publish_groups(const SessionData* data, const std::vector<LogicalGroupRecord>& groups,
                   StepTopologyGroupTransactionResult* result, Status* status)
{
    result->session = data->info;
    result->groups.clear();
    result->groups.reserve(groups.size());
    for (const LogicalGroupRecord& stored : groups)
    {
        StepTopologyLogicalGroup published;
        published.authored_id = stored.authored_id;
        published.revision = stored.revision;
        published.name = stored.name;
        for (const LogicalGroupMemberRecord& member : stored.members)
        {
            std::string target_handle;
            for (const auto& [handle, candidate] : data->handles)
            {
                if (candidate.kind == member.kind &&
                    candidate.generation == data->info.generation &&
                    candidate.shape.IsSame(member.shape))
                {
                    if (!target_handle.empty())
                    {
                        set_status(status, kConflict,
                                   "Logical group member resolves to multiple current targets.");
                        return kConflict;
                    }
                    target_handle = handle;
                }
            }
            if (target_handle.empty())
            {
                set_status(status, kUnknownTarget,
                           "Logical group member did not survive the generation refresh.");
                return kUnknownTarget;
            }
            published.members.push_back({member.kind, std::move(target_handle)});
        }
        result->groups.push_back(std::move(published));
    }
    std::sort(result->groups.begin(), result->groups.end(), [](const auto& left, const auto& right)
              { return left.authored_id < right.authored_id; });
    return 0;
}

} // namespace

int apply_logical_group_transaction(SessionData* data,
                                    const StepTopologyGroupTransaction& transaction,
                                    StepTopologyGroupTransactionResult* result, Status* status)
{
    if (result == nullptr)
    {
        set_status(status, kInvalidArgument, "Logical group transaction output is null.");
        return kInvalidArgument;
    }
    *result = {};
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

    std::vector<LogicalGroupRecord> candidate = data->logical_groups;
    for (const StepTopologyGroupCommand& command : transaction.commands)
    {
        if (!valid_authored_id(command.authored_id) ||
            command.name.size() > data->limits.max_string_bytes)
        {
            set_status(status, kInvalidArgument, "Logical group id or name is invalid.");
            return kInvalidArgument;
        }
        auto group = find_group(&candidate, command.authored_id);
        if (command.kind == StepTopologyGroupCommandKind::create)
        {
            if (group != candidate.end() || command.expected_revision != 0 || command.name.empty())
            {
                set_status(status, kConflict, "Logical group create precondition failed.");
                return kConflict;
            }
            LogicalGroupRecord created;
            created.authored_id = command.authored_id;
            created.revision = 1;
            created.name = command.name;
            const int code =
                resolve_members(data, command.member_handles, &created.members, status);
            if (code != 0)
                return code;
            candidate.push_back(std::move(created));
        }
        else
        {
            if (group == candidate.end() || command.expected_revision != group->revision)
            {
                set_status(status, kConflict, "Logical group revision precondition failed.");
                return kConflict;
            }
            if (command.kind == StepTopologyGroupCommandKind::rename)
            {
                if (command.name.empty() || !command.member_handles.empty() ||
                    group->revision == std::numeric_limits<std::uint64_t>::max())
                {
                    set_status(status, kInvalidArgument, "Logical group rename shape is invalid.");
                    return kInvalidArgument;
                }
                group->name = command.name;
                ++group->revision;
            }
            else if (command.kind == StepTopologyGroupCommandKind::replace_members)
            {
                if (!command.name.empty() ||
                    group->revision == std::numeric_limits<std::uint64_t>::max())
                {
                    set_status(status, kInvalidArgument,
                               "Logical group member replacement cannot rename the group.");
                    return kInvalidArgument;
                }
                std::vector<LogicalGroupMemberRecord> members;
                const int code = resolve_members(data, command.member_handles, &members, status);
                if (code != 0)
                    return code;
                group->members = std::move(members);
                ++group->revision;
            }
            else if (command.kind == StepTopologyGroupCommandKind::erase)
            {
                if (!command.name.empty() || !command.member_handles.empty())
                {
                    set_status(status, kInvalidArgument, "Logical group erase shape is invalid.");
                    return kInvalidArgument;
                }
                candidate.erase(group);
            }
            else
            {
                set_status(status, kInvalidArgument, "Unknown logical group command kind.");
                return kInvalidArgument;
            }
        }
        if (candidate.size() > data->limits.max_logical_groups)
        {
            set_status(status, kResourceLimit, "Logical group count exceeds the configured limit.");
            return kResourceLimit;
        }
        std::size_t ignored_bytes = 0;
        if (!candidate_within_limits(data, candidate, &ignored_bytes))
        {
            set_status(status, kResourceLimit,
                       "Logical group state exceeds configured member, string, or byte limits.");
            return kResourceLimit;
        }
    }

    std::size_t group_bytes = 0;
    if (!candidate_within_limits(data, candidate, &group_bytes))
    {
        set_status(status, kResourceLimit, "Logical group state exceeds configured limits.");
        return kResourceLimit;
    }

    const auto previous_snapshot = data->snapshot;
    const auto previous_handles = data->handles;
    const auto previous_info = data->info;
    auto previous_groups = data->logical_groups;
    const std::size_t previous_string_bytes = data->total_string_bytes;
    const std::uint64_t previous_counter = data->handle_counter;
    if (data->info.generation == std::numeric_limits<std::uint64_t>::max())
    {
        set_status(status, kResourceLimit, "Logical group generation is exhausted.");
        return kResourceLimit;
    }
    data->logical_groups = candidate;
    ++data->info.generation;
    const int refresh_code = rebuild_snapshot(data, nullptr, status);
    if (refresh_code != 0)
    {
        data->snapshot = previous_snapshot;
        data->handles = previous_handles;
        data->info = previous_info;
        data->logical_groups = std::move(previous_groups);
        data->total_string_bytes = previous_string_bytes;
        data->handle_counter = previous_counter;
        return refresh_code;
    }
    const int publish_code = publish_groups(data, data->logical_groups, result, status);
    if (publish_code != 0)
    {
        data->snapshot = previous_snapshot;
        data->handles = previous_handles;
        data->info = previous_info;
        data->logical_groups = std::move(previous_groups);
        data->total_string_bytes = previous_string_bytes;
        data->handle_counter = previous_counter;
        return publish_code;
    }
    set_status(status, 0, "");
    return 0;
}

} // namespace geometer::step_topology_internal
