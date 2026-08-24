#include "geometer/step_topology_hierarchy.h"

#include <algorithm>
#include <cmath>
#include <cstddef>
#include <deque>
#include <exception>
#include <limits>
#include <new>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <utility>
#include <vector>

namespace geometer
{
namespace
{

constexpr int kInvalidArgument = 101;
constexpr int kResourceLimit = 102;
constexpr int kConflict = 110;
constexpr int kInternalFailure = 108;
constexpr std::size_t kMaximumRecordTouchesPerCommand = 5U;
constexpr std::size_t kValidationAndCopyRecordTouches = 5U;
constexpr std::size_t kSourceInventoryTouches = 2U;

void set_status(Status* status, int code, const std::string& message)
{
    if (status != nullptr)
    {
        status->code = code;
        status->message = message;
    }
}

bool checked_add(std::size_t* total, std::size_t value)
{
    if (*total > std::numeric_limits<std::size_t>::max() - value)
        return false;
    *total += value;
    return true;
}

bool checked_multiply(std::size_t left, std::size_t right, std::size_t* result)
{
    if (left != 0 && right > std::numeric_limits<std::size_t>::max() / left)
        return false;
    *result = left * right;
    return true;
}

bool account_string(const std::string& value, const StepTopologyLimits& limits, std::size_t* total)
{
    return value.size() <= limits.max_string_bytes && checked_add(total, value.size()) &&
           *total <= limits.max_total_string_bytes;
}

bool valid_authored_id(const std::string& value, const std::string& prefix)
{
    if (value.size() <= prefix.size() || value.compare(0, prefix.size(), prefix) != 0)
        return false;
    for (std::size_t index = prefix.size(); index < value.size(); ++index)
    {
        const unsigned char character = static_cast<unsigned char>(value[index]);
        if (!((character >= 'a' && character <= 'z') || (character >= '0' && character <= '9') ||
              character == '.' || character == '-' || character == '_'))
            return false;
    }
    return true;
}

double determinant(const std::array<double, 12>& matrix)
{
    return matrix[0] * (matrix[5] * matrix[10] - matrix[6] * matrix[9]) -
           matrix[1] * (matrix[4] * matrix[10] - matrix[6] * matrix[8]) +
           matrix[2] * (matrix[4] * matrix[9] - matrix[5] * matrix[8]);
}

bool valid_signed_rigid_transform(const std::array<double, 12>& matrix)
{
    constexpr double tolerance = 1.0e-9;
    for (double value : matrix)
    {
        if (!std::isfinite(value))
            return false;
    }
    for (std::size_t row = 0; row < 3U; ++row)
    {
        double norm = 0.0;
        for (std::size_t column = 0; column < 3U; ++column)
            norm += matrix[row * 4U + column] * matrix[row * 4U + column];
        if (std::abs(norm - 1.0) > tolerance)
            return false;
        for (std::size_t other = row + 1U; other < 3U; ++other)
        {
            double dot = 0.0;
            for (std::size_t column = 0; column < 3U; ++column)
                dot += matrix[row * 4U + column] * matrix[other * 4U + column];
            if (std::abs(dot) > tolerance)
                return false;
        }
    }
    return std::abs(std::abs(determinant(matrix)) - 1.0) <= tolerance;
}

bool would_create_cycle(const std::vector<StepTopologyHierarchyNode>& nodes,
                        const std::vector<StepTopologyHierarchyOccurrence>& occurrences)
{
    std::unordered_set<std::string> assemblies;
    std::unordered_map<std::string, std::size_t> indegree;
    for (const StepTopologyHierarchyNode& node : nodes)
    {
        if (node.kind == StepTopologyHierarchyNodeKind::assembly)
        {
            assemblies.insert(node.authored_id);
            indegree.emplace(node.authored_id, 0U);
        }
    }
    std::unordered_map<std::string, std::vector<std::string>> children;
    for (const StepTopologyHierarchyOccurrence& occurrence : occurrences)
    {
        if (assemblies.count(occurrence.child_authored_id) != 0)
        {
            children[occurrence.parent_assembly_authored_id].push_back(
                occurrence.child_authored_id);
            ++indegree[occurrence.child_authored_id];
        }
    }
    std::deque<std::string> ready;
    for (const auto& [assembly, count] : indegree)
    {
        if (count == 0)
            ready.push_back(assembly);
    }
    std::size_t visited = 0;
    while (!ready.empty())
    {
        const std::string node = std::move(ready.front());
        ready.pop_front();
        ++visited;
        const auto found = children.find(node);
        if (found != children.end())
        {
            for (const std::string& child : found->second)
            {
                std::size_t& count = indegree[child];
                --count;
                if (count == 0)
                    ready.push_back(child);
            }
        }
    }
    return visited != assemblies.size();
}

int validate_state(const StepTopologySnapshot& snapshot, const StepTopologyLimits& limits,
                   const StepTopologyHierarchyState& state, Status* status)
{
    if (state.research_format != "geometer.step_topology_hierarchy.research" ||
        state.session_handle != snapshot.session.session_handle ||
        state.topology_generation != snapshot.session.generation ||
        state.source_brep_sha256 != snapshot.brep_sha256 ||
        state.nodes.size() > limits.max_definitions ||
        state.occurrences.size() > limits.max_expanded_occurrences)
    {
        set_status(status, kConflict,
                   "Hierarchy state does not match the current topology snapshot or limits.");
        return kConflict;
    }
    std::unordered_set<std::string> node_ids;
    std::unordered_set<std::string> occurrence_ids;
    std::unordered_set<std::string> assigned_sources;
    std::unordered_set<std::string> assigned_definitions;
    std::unordered_set<std::string> definitions_with_assigned_bodies;
    std::unordered_map<std::string, std::string> source_definitions;
    std::unordered_map<std::string, StepTopologyHierarchyNodeKind> node_kinds;
    source_definitions.reserve(snapshot.definitions.size() + snapshot.bodies.size());
    for (const StepTopologyDefinition& definition : snapshot.definitions)
        source_definitions.emplace("definition|" + definition.handle, definition.handle);
    for (const StepTopologyBody& body : snapshot.bodies)
        source_definitions.emplace("body|" + body.handle, body.definition_handle);
    node_kinds.reserve(state.nodes.size());
    std::size_t strings = 0;
    for (const StepTopologyHierarchyNode& node : state.nodes)
    {
        if (node.kind != StepTopologyHierarchyNodeKind::product &&
            node.kind != StepTopologyHierarchyNodeKind::assembly)
        {
            set_status(status, kInvalidArgument, "Hierarchy node kind is invalid.");
            return kInvalidArgument;
        }
        const std::string prefix = node.kind == StepTopologyHierarchyNodeKind::product
                                       ? "wn.geometer.research.product."
                                       : "wn.geometer.research.assembly.";
        if (!valid_authored_id(node.authored_id, prefix) || node.revision == 0 ||
            node.name.empty() || !node_ids.insert(node.authored_id).second)
        {
            set_status(status, kInvalidArgument, "Hierarchy node state is malformed.");
            return kInvalidArgument;
        }
        node_kinds.emplace(node.authored_id, node.kind);
        if (!account_string(node.authored_id, limits, &strings) ||
            !account_string(node.name, limits, &strings) ||
            !account_string(node.source_handle, limits, &strings))
        {
            set_status(status, kResourceLimit, "Hierarchy state exceeds its string limits.");
            return kResourceLimit;
        }
        if (node.kind == StepTopologyHierarchyNodeKind::assembly)
        {
            if (!node.source_handle.empty())
            {
                set_status(status, kInvalidArgument, "Synthetic assemblies cannot own geometry.");
                return kInvalidArgument;
            }
            continue;
        }
        const std::string source_prefix =
            node.source_kind == StepTopologyTargetKind::definition ? "definition|"
            : node.source_kind == StepTopologyTargetKind::body     ? "body|"
                                                                   : "unsupported|";
        const auto source = source_definitions.find(source_prefix + node.source_handle);
        if (source == source_definitions.end() ||
            !assigned_sources.insert(source_prefix + node.source_handle).second)
        {
            set_status(status, kConflict, "Hierarchy product source is unavailable or duplicated.");
            return kConflict;
        }
        if (node.source_kind == StepTopologyTargetKind::definition)
        {
            if (!assigned_definitions.insert(source->second).second ||
                definitions_with_assigned_bodies.count(source->second) != 0)
            {
                set_status(status, kConflict,
                           "A definition and its bodies cannot be assigned independently.");
                return kConflict;
            }
        }
        else
        {
            if (assigned_definitions.count(source->second) != 0)
            {
                set_status(status, kConflict,
                           "A body cannot be assigned after its complete definition.");
                return kConflict;
            }
            definitions_with_assigned_bodies.insert(source->second);
        }
    }
    for (const StepTopologyHierarchyOccurrence& occurrence : state.occurrences)
    {
        if (!valid_authored_id(occurrence.authored_id, "wn.geometer.research.occurrence.") ||
            occurrence.revision == 0 || !occurrence_ids.insert(occurrence.authored_id).second ||
            node_ids.count(occurrence.child_authored_id) == 0 ||
            node_ids.count(occurrence.parent_assembly_authored_id) == 0 ||
            !valid_signed_rigid_transform(occurrence.transform))
        {
            set_status(status, kInvalidArgument, "Hierarchy occurrence state is malformed.");
            return kInvalidArgument;
        }
        const auto parent = node_kinds.find(occurrence.parent_assembly_authored_id);
        if (parent == node_kinds.end() || parent->second != StepTopologyHierarchyNodeKind::assembly)
        {
            set_status(status, kConflict, "Hierarchy occurrence parent is not an assembly.");
            return kConflict;
        }
        for (const std::string* value : {&occurrence.authored_id, &occurrence.child_authored_id,
                                         &occurrence.parent_assembly_authored_id})
        {
            if (!account_string(*value, limits, &strings))
            {
                set_status(status, kResourceLimit, "Hierarchy state exceeds its string limits.");
                return kResourceLimit;
            }
        }
    }
    if (would_create_cycle(state.nodes, state.occurrences))
    {
        set_status(status, kConflict, "Hierarchy assembly graph contains a cycle.");
        return kConflict;
    }
    return 0;
}

} // namespace

int initialize_step_topology_hierarchy(const StepTopologySnapshot& snapshot,
                                       StepTopologyHierarchyState* state, Status* status)
{
    if (state == nullptr || snapshot.session.session_handle.empty() || snapshot.brep_sha256.empty())
    {
        set_status(status, kInvalidArgument, "Hierarchy initialization input is invalid.");
        return kInvalidArgument;
    }
    *state = {};
    state->session_handle = snapshot.session.session_handle;
    state->topology_generation = snapshot.session.generation;
    state->source_brep_sha256 = snapshot.brep_sha256;
    set_status(status, 0, "");
    return 0;
}

int apply_step_topology_hierarchy_transaction(const StepTopologySnapshot& snapshot,
                                              const StepTopologyLimits& limits,
                                              const StepTopologyHierarchyState& current,
                                              const StepTopologyHierarchyTransaction& transaction,
                                              StepTopologyHierarchyState* result, Status* status)
{
    if (result == nullptr)
    {
        set_status(status, kInvalidArgument, "Hierarchy transaction output is null.");
        return kInvalidArgument;
    }
    const bool aliases_current = result == &current;
    if (!aliases_current)
        *result = {};
    try
    {
        const int state_code = validate_state(snapshot, limits, current, status);
        if (state_code != 0)
            return state_code;
        if (transaction.commands.empty() ||
            transaction.expected_hierarchy_revision != current.hierarchy_revision)
        {
            set_status(status, kConflict, "Hierarchy transaction revision is stale or empty.");
            return kConflict;
        }
        if (transaction.commands.size() > limits.max_hierarchy_transaction_commands)
        {
            set_status(status, kResourceLimit, "Hierarchy transaction exceeds its command limit.");
            return kResourceLimit;
        }
        std::size_t projected_records = current.nodes.size();
        if (!checked_add(&projected_records, current.occurrences.size()) ||
            !checked_add(&projected_records, transaction.commands.size()))
        {
            set_status(status, kResourceLimit, "Hierarchy transaction work overflowed.");
            return kResourceLimit;
        }
        std::size_t current_records = current.nodes.size();
        std::size_t source_records = snapshot.definitions.size();
        std::size_t command_record_work = 0;
        std::size_t current_record_work = 0;
        std::size_t final_record_work = 0;
        std::size_t source_work = 0;
        std::size_t work = 0;
        if (!checked_add(&current_records, current.occurrences.size()) ||
            !checked_add(&source_records, snapshot.bodies.size()) ||
            !checked_multiply(transaction.commands.size(), projected_records,
                              &command_record_work) ||
            !checked_multiply(command_record_work, kMaximumRecordTouchesPerCommand,
                              &command_record_work) ||
            !checked_multiply(current_records, kValidationAndCopyRecordTouches,
                              &current_record_work) ||
            !checked_multiply(projected_records, kValidationAndCopyRecordTouches,
                              &final_record_work) ||
            !checked_multiply(source_records, kSourceInventoryTouches, &source_work) ||
            !checked_add(&work, command_record_work) || !checked_add(&work, current_record_work) ||
            !checked_add(&work, final_record_work) || !checked_add(&work, source_work) ||
            work > limits.max_hierarchy_transaction_work_items)
        {
            set_status(status, kResourceLimit,
                       "Hierarchy transaction exceeds its configured work limit.");
            return kResourceLimit;
        }
        std::size_t command_strings = 0;
        for (const StepTopologyHierarchyCommand& command : transaction.commands)
        {
            for (const std::string* value :
                 {&command.authored_id, &command.name, &command.source_handle,
                  &command.child_authored_id, &command.parent_assembly_authored_id})
            {
                if (!account_string(*value, limits, &command_strings))
                {
                    set_status(status, kResourceLimit,
                               "Hierarchy transaction exceeds its string limits.");
                    return kResourceLimit;
                }
            }
        }
        StepTopologyHierarchyState candidate = current;
        for (const StepTopologyHierarchyCommand& command : transaction.commands)
        {
            if (command.kind < StepTopologyHierarchyCommandKind::create_product ||
                command.kind > StepTopologyHierarchyCommandKind::erase_node)
            {
                set_status(status, kInvalidArgument, "Hierarchy command kind is invalid.");
                return kInvalidArgument;
            }
            auto node = std::find_if(candidate.nodes.begin(), candidate.nodes.end(),
                                     [&command](const auto& value)
                                     { return value.authored_id == command.authored_id; });
            auto occurrence = std::find_if(
                candidate.occurrences.begin(), candidate.occurrences.end(),
                [&command](const auto& value) { return value.authored_id == command.authored_id; });
            if (command.kind == StepTopologyHierarchyCommandKind::create_product ||
                command.kind == StepTopologyHierarchyCommandKind::create_assembly)
            {
                const bool product =
                    command.kind == StepTopologyHierarchyCommandKind::create_product;
                if (node != candidate.nodes.end() || command.name.empty() ||
                    (product && command.source_handle.empty()) ||
                    (!product && !command.source_handle.empty()))
                {
                    set_status(status, kConflict, "Hierarchy node create command conflicts.");
                    return kConflict;
                }
                StepTopologyHierarchyNode created;
                created.authored_id = command.authored_id;
                created.revision = 1;
                created.kind = product ? StepTopologyHierarchyNodeKind::product
                                       : StepTopologyHierarchyNodeKind::assembly;
                created.name = command.name;
                created.source_kind = command.source_kind;
                created.source_handle = command.source_handle;
                candidate.nodes.push_back(std::move(created));
            }
            else if (command.kind == StepTopologyHierarchyCommandKind::create_occurrence)
            {
                if (occurrence != candidate.occurrences.end() ||
                    command.child_authored_id.empty() ||
                    command.parent_assembly_authored_id.empty())
                {
                    set_status(status, kConflict, "Hierarchy occurrence create command conflicts.");
                    return kConflict;
                }
                candidate.occurrences.push_back({command.authored_id, 1, command.child_authored_id,
                                                 command.parent_assembly_authored_id,
                                                 command.transform});
            }
            else if (command.kind == StepTopologyHierarchyCommandKind::reparent_occurrence)
            {
                if (occurrence == candidate.occurrences.end() ||
                    occurrence->revision != command.expected_revision ||
                    occurrence->revision == std::numeric_limits<std::uint64_t>::max() ||
                    command.parent_assembly_authored_id.empty())
                {
                    set_status(status, kConflict, "Hierarchy occurrence revision is stale.");
                    return kConflict;
                }
                occurrence->parent_assembly_authored_id = command.parent_assembly_authored_id;
                occurrence->transform = command.transform;
                ++occurrence->revision;
            }
            else if (command.kind == StepTopologyHierarchyCommandKind::rename_node)
            {
                if (node == candidate.nodes.end() || node->revision != command.expected_revision ||
                    node->revision == std::numeric_limits<std::uint64_t>::max() ||
                    command.name.empty())
                {
                    set_status(status, kConflict, "Hierarchy node revision is stale.");
                    return kConflict;
                }
                node->name = command.name;
                ++node->revision;
            }
            else if (command.kind == StepTopologyHierarchyCommandKind::erase_occurrence)
            {
                if (occurrence == candidate.occurrences.end() ||
                    occurrence->revision != command.expected_revision)
                {
                    set_status(status, kConflict, "Hierarchy occurrence erase is stale.");
                    return kConflict;
                }
                candidate.occurrences.erase(occurrence);
            }
            else
            {
                if (node == candidate.nodes.end() || node->revision != command.expected_revision ||
                    std::any_of(candidate.occurrences.begin(), candidate.occurrences.end(),
                                [&command](const auto& value)
                                {
                                    return value.child_authored_id == command.authored_id ||
                                           value.parent_assembly_authored_id == command.authored_id;
                                }))
                {
                    set_status(status, kConflict,
                               "Hierarchy node erase is stale or still referenced.");
                    return kConflict;
                }
                candidate.nodes.erase(node);
            }
        }
        const int candidate_code = validate_state(snapshot, limits, candidate, status);
        if (candidate_code != 0)
            return candidate_code;
        if (candidate.hierarchy_revision == std::numeric_limits<std::uint64_t>::max())
        {
            set_status(status, kConflict, "Hierarchy revision is exhausted.");
            return kConflict;
        }
        ++candidate.hierarchy_revision;
        *result = std::move(candidate);
        set_status(status, 0, "");
        return 0;
    }
    catch (const std::bad_alloc&)
    {
        if (!aliases_current)
            *result = {};
        set_status(status, kResourceLimit, "Hierarchy transaction exhausted memory.");
        return kResourceLimit;
    }
    catch (const std::exception& error)
    {
        if (!aliases_current)
            *result = {};
        set_status(status, kInternalFailure, error.what());
        return kInternalFailure;
    }
}

} // namespace geometer
