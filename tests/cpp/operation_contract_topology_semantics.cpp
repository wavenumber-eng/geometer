#include "operation_contract_topology_semantics.h"

#include "geometer/generated/contracts/contracts.h"
#include "geometer/operation_transport.h"
#include "geometer/sha256.h"

#include <algorithm>
#include <cctype>
#include <cstdint>
#include <fstream>
#include <iterator>
#include <optional>
#include <stdexcept>
#include <string>
#include <type_traits>
#include <unordered_map>
#include <unordered_set>
#include <vector>

namespace
{

void require(bool condition, const std::string& message)
{
    if (!condition)
        throw std::runtime_error(message);
}

std::vector<unsigned char> read_bytes(const std::string& path)
{
    std::ifstream input(path, std::ios::binary);
    require(static_cast<bool>(input), "failed to open " + path);
    return {std::istreambuf_iterator<char>(input), std::istreambuf_iterator<char>()};
}

bool valid_research_name(const std::string& value, const std::string& prefix)
{
    if (value.size() <= prefix.size() || value.rfind(prefix, 0) != 0)
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

bool valid_hierarchy_name(const std::string& value, const std::string& prefix)
{
    if (value.size() <= prefix.size() || value.rfind(prefix, 0) != 0)
        return false;
    return std::all_of(value.begin() + static_cast<std::ptrdiff_t>(prefix.size()), value.end(),
                       [](unsigned char character)
                       {
                           return (character >= 'a' && character <= 'z') ||
                                  (character >= '0' && character <= '9') || character == '.' ||
                                  character == '_' || character == '-';
                       });
}

bool valid_target_handle(const std::string& value)
{
    return value.size() == 68U && value.rfind("gtt_", 0) == 0 &&
           std::all_of(value.begin() + 4, value.end(),
                       [](unsigned char character)
                       {
                           return (character >= '0' && character <= '9') ||
                                  (character >= 'a' && character <= 'f');
                       });
}

bool valid_lowercase_sha256(const std::string& value)
{
    return value.size() == 64U && std::all_of(value.begin(), value.end(),
                                              [](unsigned char character)
                                              {
                                                  return (character >= '0' && character <= '9') ||
                                                         (character >= 'a' && character <= 'f');
                                              });
}

bool valid_recovery_fingerprint(const geometer::contracts::RecoveryFingerprint& value)
{
    if (!std::isfinite(value.area_mm2) || !std::isfinite(value.volume_mm3) ||
        value.centroid_mm.size() != 3U || value.bounds_mm.size() != 6U ||
        !valid_lowercase_sha256(value.adjacency_sha256))
        return false;
    if (!std::all_of(value.centroid_mm.begin(), value.centroid_mm.end(),
                     [](double item) { return std::isfinite(item); }) ||
        !std::all_of(value.bounds_mm.begin(), value.bounds_mm.end(),
                     [](double item) { return std::isfinite(item); }))
        return false;
    return value.bounds_mm[0] <= value.bounds_mm[3] && value.bounds_mm[1] <= value.bounds_mm[4] &&
           value.bounds_mm[2] <= value.bounds_mm[5];
}

std::optional<bool> validate_ipc_oracle(const std::string& oracle, const rapidjson::Value& vector,
                                        const std::vector<unsigned char>& data,
                                        const std::string& vector_root)
{
    geometer::contracts::ContractError error;
    if (oracle == "step_topology_ipc_request_pair" || oracle == "step_topology_ipc_pair_matrix")
    {
        geometer::contracts::IpcRequestA0 value;
        require(geometer::contracts::decode_json(data.data(), data.size(), &value, &error),
                "IPC request pair vector must be structurally valid");
        if (!geometer::operation_request_value_matches(value.operation, value.request))
            return false;
        if (oracle == "step_topology_ipc_pair_matrix")
        {
            std::unordered_set<std::string> operations;
            const auto validate_pairs =
                [&operations](const rapidjson::Value& pairs, const std::string& role)
            {
                if (!pairs.IsArray() || pairs.Size() != 12U)
                    return false;
                for (const auto& pair : pairs.GetArray())
                {
                    if (!pair.IsArray() || pair.Size() != 2U || !pair[0].IsString() ||
                        !pair[1].IsString())
                        return false;
                    const std::string operation = pair[0].GetString();
                    const std::string contract = pair[1].GetString();
                    if (!operations.insert(role + operation).second ||
                        operation.rfind("geometry.step_topology.", 0) != 0U ||
                        operation.size() < 3U ||
                        contract != operation.substr(0, operation.size() - 3U) + "." + role + ".a0")
                        return false;
                }
                return true;
            };
            return validate_pairs(vector["request_pairs"], "request") &&
                   validate_pairs(vector["result_pairs"], "result");
        }
        return true;
    }
    if (oracle == "step_topology_ipc_result_pair")
    {
        geometer::contracts::OperationOutcomeA0 value;
        require(geometer::contracts::decode_json(data.data(), data.size(), &value, &error),
                "IPC result pair vector must be structurally valid");
        const auto* success = std::get_if<geometer::contracts::OperationSuccessA0>(&value);
        return success != nullptr &&
               geometer::operation_result_value_matches(success->operation, success->result);
    }
    if (oracle == "step_topology_session")
    {
        geometer::contracts::StepTopologyResolveHitRequestA0 value;
        require(geometer::contracts::decode_json(data.data(), data.size(), &value, &error),
                "session semantic vector must be structurally valid");
        return value.session.session_handle == vector["expected_session_handle"].GetString() &&
               value.session.generation == vector["expected_generation"].GetUint();
    }
    if (oracle == "step_topology_inspection_high_fan_in")
    {
        geometer::contracts::StepTopologyInspectResultA0 seed;
        require(geometer::contracts::decode_json(data.data(), data.size(), &seed, &error),
                "high-fan-in seed vector must be structurally valid");
        return vector.HasMember("fan_in") && vector["fan_in"].GetUint() == 4096U;
    }
    return std::nullopt;
}

std::optional<bool> validate_inspection_oracle(const std::string& oracle,
                                               const rapidjson::Value& vector,
                                               const std::vector<unsigned char>& data,
                                               const std::string& vector_root)
{
    geometer::contracts::ContractError error;
    if (oracle == "step_topology_inspection")
    {
        std::vector<geometer::contracts::StepTopologyInspectResultA0> pages;
        if (vector.HasMember("prior_pages"))
        {
            for (const auto& prior_path : vector["prior_pages"].GetArray())
            {
                const auto prior_data = read_bytes(vector_root + prior_path.GetString());
                geometer::contracts::StepTopologyInspectResultA0 prior;
                require(geometer::contracts::decode_json(prior_data.data(), prior_data.size(),
                                                         &prior, &error),
                        "prior inspection page must be structurally valid");
                pages.push_back(std::move(prior));
            }
        }
        geometer::contracts::StepTopologyInspectResultA0 current;
        require(geometer::contracts::decode_json(data.data(), data.size(), &current, &error),
                "inspection semantic vector must be structurally valid");
        pages.push_back(std::move(current));

        auto value = pages.front();
        value.page.definitions.clear();
        value.page.occurrences.clear();
        value.page.bodies.clear();
        value.page.shells.clear();
        value.page.faces.clear();
        std::unordered_set<std::string> cursors;
        for (std::size_t page_index = 0; page_index < pages.size(); ++page_index)
        {
            const auto& page = pages[page_index];
            if (page.session.session_handle != value.session.session_handle ||
                page.session.generation != value.session.generation ||
                page.counts.definitions != value.counts.definitions ||
                page.counts.root_occurrences != value.counts.root_occurrences ||
                page.counts.component_occurrences != value.counts.component_occurrences ||
                page.counts.bodies != value.counts.bodies ||
                page.counts.shells != value.counts.shells ||
                page.counts.faces != value.counts.faces)
                return false;
            const bool terminal = page_index + 1U == pages.size();
            if (terminal == page.page.next_cursor.has_value())
                return false;
            const std::size_t page_record_count =
                page.page.definitions.size() + page.page.occurrences.size() +
                page.page.bodies.size() + page.page.shells.size() + page.page.faces.size();
            if (!terminal && page_record_count == 0U)
                return false;
            if (page.page.next_cursor.has_value() && !cursors.insert(*page.page.next_cursor).second)
                return false;
            value.page.definitions.insert(value.page.definitions.end(),
                                          page.page.definitions.begin(),
                                          page.page.definitions.end());
            value.page.occurrences.insert(value.page.occurrences.end(),
                                          page.page.occurrences.begin(),
                                          page.page.occurrences.end());
            value.page.bodies.insert(value.page.bodies.end(), page.page.bodies.begin(),
                                     page.page.bodies.end());
            value.page.shells.insert(value.page.shells.end(), page.page.shells.begin(),
                                     page.page.shells.end());
            value.page.faces.insert(value.page.faces.end(), page.page.faces.begin(),
                                    page.page.faces.end());
        }

        std::unordered_set<std::string> handles;
        const auto add = [&handles](const std::string& handle)
        { return handles.insert(handle).second; };
        for (const auto& item : value.page.definitions)
            if (!add(item.handle))
                return false;
        std::uint32_t root_count = 0;
        std::uint32_t component_count = 0;
        for (const auto& item : value.page.occurrences)
        {
            if (!std::visit([&add](const auto& occurrence) { return add(occurrence.handle); },
                            item))
                return false;
            std::holds_alternative<geometer::contracts::RootOccurrenceSummary>(item)
                ? ++root_count
                : ++component_count;
        }
        for (const auto& item : value.page.bodies)
            if (!add(item.handle))
                return false;
        for (const auto& item : value.page.shells)
            if (!add(item.handle))
                return false;
        for (const auto& item : value.page.faces)
            if (!add(item.handle))
                return false;
        if (value.page.definitions.size() != value.counts.definitions ||
            root_count != value.counts.root_occurrences ||
            component_count != value.counts.component_occurrences ||
            value.page.bodies.size() != value.counts.bodies ||
            value.page.shells.size() != value.counts.shells ||
            value.page.faces.size() != value.counts.faces)
            return false;

        const auto valid_evidence = [](const auto& optional_evidence)
        {
            if (!optional_evidence.has_value())
                return true;
            const auto& evidence = *optional_evidence;
            const bool has_all_positive_fields = evidence.model_number.has_value() &&
                                                 evidence.entity_type.has_value() &&
                                                 evidence.mapping_method.has_value();
            const bool has_any_positive_field = evidence.model_number.has_value() ||
                                                evidence.entity_type.has_value() ||
                                                evidence.mapping_method.has_value();
            return evidence.mapped ? has_all_positive_fields
                                   : !evidence.shape_result_round_trip && !has_any_positive_field;
        };
        for (const auto& item : value.page.definitions)
            if (!valid_evidence(item.source_entity))
                return false;
        for (const auto& item : value.page.bodies)
            if (!valid_evidence(item.source_entity))
                return false;
        for (const auto& item : value.page.shells)
            if (!valid_evidence(item.source_entity))
                return false;
        for (const auto& item : value.page.faces)
            if (!valid_evidence(item.source_entity))
                return false;

        const auto find_definition = [&value](const std::string& handle)
        {
            return std::find_if(value.page.definitions.begin(), value.page.definitions.end(),
                                [&handle](const auto& item) { return item.handle == handle; });
        };
        const auto find_body = [&value](const std::string& handle)
        {
            return std::find_if(value.page.bodies.begin(), value.page.bodies.end(),
                                [&handle](const auto& item) { return item.handle == handle; });
        };
        const auto find_shell = [&value](const std::string& handle)
        {
            return std::find_if(value.page.shells.begin(), value.page.shells.end(),
                                [&handle](const auto& item) { return item.handle == handle; });
        };
        const auto find_face = [&value](const std::string& handle)
        {
            return std::find_if(value.page.faces.begin(), value.page.faces.end(),
                                [&handle](const auto& item) { return item.handle == handle; });
        };
        const auto find_occurrence = [&value](const std::string& handle)
        {
            return std::find_if(value.page.occurrences.begin(), value.page.occurrences.end(),
                                [&handle](const auto& item)
                                {
                                    return std::visit([&handle](const auto& occurrence)
                                                      { return occurrence.handle == handle; },
                                                      item);
                                });
        };
        for (const auto& definition : value.page.definitions)
        {
            const auto body_count = static_cast<std::uint32_t>(std::count_if(
                value.page.bodies.begin(), value.page.bodies.end(), [&definition](const auto& item)
                { return item.definition_handle == definition.handle; }));
            const auto face_count = static_cast<std::uint32_t>(std::count_if(
                value.page.faces.begin(), value.page.faces.end(), [&definition](const auto& item)
                { return item.definition_handle == definition.handle; }));
            if (body_count != definition.body_count || face_count != definition.face_count)
                return false;
        }
        for (const auto& item : value.page.occurrences)
        {
            const bool valid = std::visit(
                [&](const auto& occurrence)
                {
                    if (find_definition(occurrence.definition_handle) ==
                        value.page.definitions.end())
                        return false;
                    using Occurrence = std::decay_t<decltype(occurrence)>;
                    if constexpr (std::is_same_v<Occurrence,
                                                 geometer::contracts::ComponentOccurrenceSummary>)
                    {
                        std::unordered_set<std::string> ancestors{occurrence.handle};
                        std::uint32_t expected_depth = occurrence.depth;
                        const geometer::contracts::OccurrenceSummary* current_item = &item;
                        while (const auto* component =
                                   std::get_if<geometer::contracts::ComponentOccurrenceSummary>(
                                       current_item))
                        {
                            const auto parent =
                                find_occurrence(component->parent_occurrence_handle);
                            if (parent == value.page.occurrences.end() || expected_depth == 0U)
                                return false;
                            const std::string parent_handle =
                                std::visit([](const auto& parent_value)
                                           { return parent_value.handle; }, *parent);
                            if (!ancestors.insert(parent_handle).second)
                                return false;
                            --expected_depth;
                            if (const auto* parent_component =
                                    std::get_if<geometer::contracts::ComponentOccurrenceSummary>(
                                        &*parent);
                                parent_component && parent_component->depth != expected_depth)
                                return false;
                            current_item = &*parent;
                        }
                        return expected_depth == 0U;
                    }
                    return true;
                },
                item);
            if (!valid)
                return false;
        }
        std::unordered_set<std::string> membership_keys;
        std::unordered_map<std::string, std::uint32_t> body_shell_counts;
        std::unordered_map<std::string, std::uint32_t> body_face_counts;
        std::unordered_map<std::string, std::uint32_t> shell_body_counts;
        std::unordered_map<std::string, std::uint32_t> shell_face_counts;
        std::unordered_map<std::string, std::uint32_t> face_body_counts;
        std::unordered_map<std::string, std::uint32_t> face_shell_counts;
        for (const auto& membership : value.page.memberships)
        {
            const std::string key = std::to_string(static_cast<int>(membership.kind)) + ":" +
                                    membership.owner_handle + ":" + membership.member_handle;
            if (!membership_keys.insert(key).second)
                return false;
            if (membership.kind == geometer::contracts::TopologyMembershipKind::body_shell)
            {
                const auto body = find_body(membership.owner_handle);
                const auto shell = find_shell(membership.member_handle);
                if (body == value.page.bodies.end() || shell == value.page.shells.end() ||
                    body->definition_handle != shell->definition_handle)
                    return false;
                ++body_shell_counts[body->handle];
                ++shell_body_counts[shell->handle];
            }
            else if (membership.kind == geometer::contracts::TopologyMembershipKind::body_face)
            {
                const auto body = find_body(membership.owner_handle);
                const auto face = find_face(membership.member_handle);
                if (body == value.page.bodies.end() || face == value.page.faces.end() ||
                    body->definition_handle != face->definition_handle)
                    return false;
                ++body_face_counts[body->handle];
                ++face_body_counts[face->handle];
            }
            else
            {
                const auto shell = find_shell(membership.owner_handle);
                const auto face = find_face(membership.member_handle);
                if (shell == value.page.shells.end() || face == value.page.faces.end() ||
                    shell->definition_handle != face->definition_handle)
                    return false;
                ++shell_face_counts[shell->handle];
                ++face_shell_counts[face->handle];
            }
        }
        for (const auto& body : value.page.bodies)
            if (find_definition(body.definition_handle) == value.page.definitions.end() ||
                body.shell_count != body_shell_counts[body.handle] ||
                body.face_count != body_face_counts[body.handle])
                return false;
        for (const auto& shell : value.page.shells)
            if (find_definition(shell.definition_handle) == value.page.definitions.end() ||
                shell.body_count != shell_body_counts[shell.handle] ||
                shell.face_count != shell_face_counts[shell.handle])
                return false;
        for (const auto& face : value.page.faces)
            if (find_definition(face.definition_handle) == value.page.definitions.end() ||
                face.body_count != face_body_counts[face.handle] ||
                face.shell_count != face_shell_counts[face.handle])
                return false;
        return true;
    }
    return std::nullopt;
}

std::optional<bool> validate_mutation_oracle(const std::string& oracle,
                                             const rapidjson::Value& vector,
                                             const std::vector<unsigned char>& data,
                                             const std::string& vector_root)
{
    geometer::contracts::ContractError error;
    if (oracle == "step_topology_render_attachments")
    {
        geometer::contracts::StepTopologyRenderResultA0 value;
        require(geometer::contracts::decode_json(data.data(), data.size(), &value, &error),
                "render semantic vector must be structurally valid");
        if (value.artifact.content_sha256 != value.glb.sha256 ||
            vector["attachments"].Size() != (value.compact_binding_table.has_value() ? 2U : 1U))
            return false;
        std::unordered_set<std::string> names;
        for (const auto& attachment : vector["attachments"].GetArray())
        {
            const std::string name = attachment["name"].GetString();
            if (!names.insert(name).second)
                return false;
            const std::string media_type = attachment["media_type"].GetString();
            std::uint32_t expected_bytes = 0;
            std::string expected_sha256;
            if (name == value.glb.name && media_type == value.glb.media_type)
            {
                expected_bytes = value.glb.bytes;
                expected_sha256 = value.glb.sha256;
            }
            else if (value.compact_binding_table.has_value() &&
                     name == value.compact_binding_table->name &&
                     media_type == value.compact_binding_table->media_type)
            {
                expected_bytes = value.compact_binding_table->bytes;
                expected_sha256 = value.compact_binding_table->sha256;
            }
            else
                return false;
            const std::vector<unsigned char> bytes =
                read_bytes(vector_root + attachment["file"].GetString());
            if (bytes.size() != expected_bytes ||
                geometer::sha256_hex(bytes.data(), bytes.size()) != expected_sha256)
                return false;
        }
        return names.count(value.glb.name) == 1U &&
               (!value.compact_binding_table.has_value() ||
                names.count(value.compact_binding_table->name) == 1U);
    }
    if (oracle == "step_topology_logical_group_commands")
    {
        geometer::contracts::StepTopologyApplyLogicalGroupsRequestA0 value;
        require(geometer::contracts::decode_json(data.data(), data.size(), &value, &error),
                "logical-group semantic vector must be structurally valid");
        std::unordered_set<std::string> created;
        std::size_t member_reference_count = 0;
        for (const auto& command : value.commands)
        {
            bool valid = true;
            std::visit(
                [&](const auto& item)
                {
                    valid = valid_research_name(item.authored_id, "wn.geometer.research.group.");
                    using Item = std::decay_t<decltype(item)>;
                    if constexpr (std::is_same_v<Item,
                                                 geometer::contracts::CreateLogicalGroupCommand>)
                    {
                        valid = valid && created.insert(item.authored_id).second;
                    }
                    else if constexpr (std::is_same_v<
                                           Item, geometer::contracts::EraseLogicalGroupCommand>)
                    {
                        created.erase(item.authored_id);
                    }
                    if constexpr (std::is_same_v<Item,
                                                 geometer::contracts::CreateLogicalGroupCommand> ||
                                  std::is_same_v<
                                      Item, geometer::contracts::ReplaceLogicalGroupMembersCommand>)
                    {
                        std::unordered_set<std::string> members;
                        member_reference_count += item.member_handles.size();
                        valid = valid && member_reference_count <= 100000U;
                        for (const auto& handle : item.member_handles)
                            valid = valid && valid_target_handle(handle) &&
                                    members.insert(handle).second;
                    }
                },
                command);
            if (!valid)
                return false;
        }
        return true;
    }
    if (oracle == "step_topology_logical_group_commands_high_fan_in" ||
        oracle == "step_topology_logical_group_result_high_fan_in")
    {
        return vector["fan_in"].GetUint() <= 100000U;
    }
    if (oracle == "step_topology_logical_group_result")
    {
        geometer::contracts::StepTopologyApplyLogicalGroupsResultA0 value;
        require(geometer::contracts::decode_json(data.data(), data.size(), &value, &error),
                "logical-group result semantic vector must be structurally valid");
        std::size_t members = 0;
        for (const auto& group : value.groups)
        {
            if (!valid_research_name(group.authored_id, "wn.geometer.research.group."))
                return false;
            members += group.members.size();
            if (members > 100000U)
                return false;
            std::unordered_set<std::string> handles;
            for (const auto& member : group.members)
            {
                if (!valid_target_handle(member.target_handle) ||
                    !handles.insert(member.target_handle).second)
                    return false;
            }
        }
        return true;
    }
    if (oracle == "step_topology_metadata_probe_result")
    {
        geometer::contracts::StepTopologyApplyMetadataProbesResultA0 value;
        require(geometer::contracts::decode_json(data.data(), data.size(), &value, &error),
                "metadata-probe result semantic vector must be structurally valid");
        std::size_t member_count = 0U;
        for (const auto& group : value.groups)
        {
            if (!valid_research_name(group.authored_id, "wn.geometer.research.group."))
                return false;
            member_count += group.members.size();
            if (member_count > 100000U)
                return false;
            std::unordered_set<std::string> handles;
            for (const auto& member : group.members)
            {
                if (!valid_target_handle(member.target_handle) ||
                    !handles.insert(member.target_handle).second)
                    return false;
            }
        }
        return true;
    }
    if (oracle == "step_topology_metadata_probe_commands")
    {
        geometer::contracts::StepTopologyApplyMetadataProbesRequestA0 value;
        require(geometer::contracts::decode_json(data.data(), data.size(), &value, &error),
                "metadata-probe semantic vector must be structurally valid");
        std::unordered_set<std::string> attached;
        for (const auto& command : value.commands)
        {
            bool valid = true;
            std::visit(
                [&](const auto& item)
                {
                    using Item = std::decay_t<decltype(item)>;
                    valid = valid_research_name(item.authored_id, "wn.geometer.research.probe.");
                    if constexpr (std::is_same_v<Item,
                                                 geometer::contracts::AttachMetadataProbeCommand>)
                        valid = valid && attached.insert(item.authored_id).second;
                    else if constexpr (std::is_same_v<
                                           Item, geometer::contracts::EraseMetadataProbeCommand>)
                        attached.erase(item.authored_id);
                    if constexpr (!std::is_same_v<Item,
                                                  geometer::contracts::EraseMetadataProbeCommand>)
                    {
                        valid = valid &&
                                valid_research_name(item.key, "wn.geometer.research.probe.key.");
                        if (const auto* group =
                                std::get_if<geometer::contracts::LogicalGroupProbeTarget>(
                                    &item.target))
                            valid = valid && valid_research_name(group->group_authored_id,
                                                                 "wn.geometer.research.group.");
                    }
                },
                command);
            if (!valid)
                return false;
        }
        return true;
    }
    if (oracle == "step_topology_checkpoint_attachment")
    {
        geometer::contracts::StepTopologyCheckpointEditJournalResultA0 value;
        require(geometer::contracts::decode_json(data.data(), data.size(), &value, &error),
                "checkpoint semantic vector must be structurally valid");
        if (value.transaction_count != value.state.edit_journal_revision ||
            vector["attachments"].Size() != 1U)
            return false;
        const auto& attachment = vector["attachments"][0];
        const std::vector<unsigned char> bytes =
            read_bytes(vector_root + attachment["file"].GetString());
        return value.journal.name == attachment["name"].GetString() &&
               value.journal.media_type == attachment["media_type"].GetString() &&
               value.journal.bytes == bytes.size() &&
               value.journal.sha256 == geometer::sha256_hex(bytes.data(), bytes.size());
    }
    return std::nullopt;
}

std::optional<bool> validate_persistence_oracle(const std::string& oracle,
                                                const rapidjson::Value& vector,
                                                const std::vector<unsigned char>& data,
                                                const std::string& vector_root)
{
    geometer::contracts::ContractError error;
    if (oracle == "step_topology_save_attachments")
    {
        const auto request_data = read_bytes(vector_root + vector["context_file"].GetString());
        geometer::contracts::StepTopologySaveRequestA0 request;
        require(geometer::contracts::decode_json(request_data.data(), request_data.size(), &request,
                                                 &error),
                "save request context must be structurally valid");
        geometer::contracts::StepTopologySaveResultA0 value;
        require(geometer::contracts::decode_json(data.data(), data.size(), &value, &error),
                "save semantic vector must be structurally valid");
        if (!valid_lowercase_sha256(value.source_sha256) || vector["attachments"].Size() != 1U)
            return false;
        std::unordered_set<int> carriers;
        for (const auto& capability : value.capabilities)
        {
            if (!carriers.insert(static_cast<int>(capability.carrier)).second ||
                std::any_of(capability.notes.begin(), capability.notes.end(),
                            [](const auto& note) { return note.value.empty(); }))
                return false;
        }
        if (carriers.size() != 5U)
            return false;
        const auto selected_carrier = std::visit(
            [](const auto& artifact)
            {
                using Artifact = std::decay_t<decltype(artifact)>;
                if constexpr (std::is_same_v<Artifact, geometer::contracts::XbfPersistenceArtifact>)
                    return geometer::contracts::PersistenceCarrier::xbf;
                else if constexpr (std::is_same_v<Artifact,
                                                  geometer::contracts::XmlXcafPersistenceArtifact>)
                    return geometer::contracts::PersistenceCarrier::xml_xcaf;
                else if constexpr (std::is_same_v<
                                       Artifact, geometer::contracts::StepAp242PersistenceArtifact>)
                    return geometer::contracts::PersistenceCarrier::step_ap242;
                else
                    return geometer::contracts::PersistenceCarrier::json_sidecar;
            },
            value.artifact);
        const auto selected_capability =
            std::find_if(value.capabilities.begin(), value.capabilities.end(),
                         [selected_carrier](const auto& capability)
                         { return capability.carrier == selected_carrier; });
        if (selected_capability == value.capabilities.end() ||
            selected_capability->save == geometer::contracts::CarrierSupportState::unsupported)
            return false;
        const auto requested_carrier = [&request]()
        {
            switch (request.carrier)
            {
            case geometer::contracts::SaveCarrier::xbf:
                return geometer::contracts::PersistenceCarrier::xbf;
            case geometer::contracts::SaveCarrier::xml_xcaf:
                return geometer::contracts::PersistenceCarrier::xml_xcaf;
            case geometer::contracts::SaveCarrier::step_ap242:
                return geometer::contracts::PersistenceCarrier::step_ap242;
            case geometer::contracts::SaveCarrier::json_sidecar:
                return geometer::contracts::PersistenceCarrier::json_sidecar;
            }
            return geometer::contracts::PersistenceCarrier::xbf;
        }();
        if (requested_carrier != selected_carrier)
            return false;
        const auto& attachment = vector["attachments"][0];
        const auto bytes = read_bytes(vector_root + attachment["file"].GetString());
        return std::visit(
            [&](const auto& artifact)
            {
                return artifact.name == attachment["name"].GetString() &&
                       artifact.media_type == attachment["media_type"].GetString() &&
                       artifact.bytes == bytes.size() && valid_lowercase_sha256(artifact.sha256) &&
                       artifact.sha256 == geometer::sha256_hex(bytes.data(), bytes.size());
            },
            value.artifact);
    }
    if (oracle == "step_topology_restore_attachments")
    {
        geometer::contracts::StepTopologyRestoreRequestA0 value;
        require(geometer::contracts::decode_json(data.data(), data.size(), &value, &error),
                "restore semantic vector must be structurally valid");
        const bool journal =
            std::holds_alternative<geometer::contracts::EditJournalPersistenceArtifact>(
                value.state_artifact);
        if (journal != value.replay_preconditions.has_value() ||
            vector["attachments"].Size() != 2U || !valid_lowercase_sha256(value.source.sha256))
            return false;
        if (value.replay_preconditions.has_value())
        {
            const auto& replay = *value.replay_preconditions;
            if (replay.source_sha256 != value.source.sha256 ||
                !valid_lowercase_sha256(replay.source_sha256) ||
                !valid_lowercase_sha256(replay.source_brep_sha256) ||
                !valid_lowercase_sha256(replay.target_inventory_sha256))
                return false;
        }
        bool source_found = false;
        bool state_found = false;
        for (const auto& attachment : vector["attachments"].GetArray())
        {
            const std::string name = attachment["name"].GetString();
            const std::string media = attachment["media_type"].GetString();
            const auto bytes = read_bytes(vector_root + attachment["file"].GetString());
            if (name == "source")
            {
                if (source_found || (media != "application/step" && media != "model/step") ||
                    value.source.bytes != bytes.size() ||
                    value.source.sha256 != geometer::sha256_hex(bytes.data(), bytes.size()))
                    return false;
                source_found = true;
            }
            else
            {
                const bool matches = std::visit(
                    [&](const auto& artifact)
                    {
                        return artifact.name == name && artifact.media_type == media &&
                               artifact.bytes == bytes.size() &&
                               valid_lowercase_sha256(artifact.sha256) &&
                               artifact.sha256 == geometer::sha256_hex(bytes.data(), bytes.size());
                    },
                    value.state_artifact);
                if (state_found || !matches)
                    return false;
                state_found = true;
            }
        }
        return source_found && state_found;
    }
    if (oracle == "step_topology_restore_result")
    {
        const auto request_data = read_bytes(vector_root + vector["context_file"].GetString());
        geometer::contracts::StepTopologyRestoreRequestA0 request;
        require(geometer::contracts::decode_json(request_data.data(), request_data.size(), &request,
                                                 &error),
                "restore request context must be structurally valid");
        geometer::contracts::StepTopologyRestoreResultA0 value;
        require(geometer::contracts::decode_json(data.data(), data.size(), &value, &error),
                "restore result vector must be structurally valid");
        const bool source_matches =
            value.source.format == request.source.format &&
            value.source.sha256 == request.source.sha256 &&
            value.source.bytes == request.source.bytes &&
            value.source.normalized_length_unit == request.source.normalized_length_unit;
        const std::uint32_t expected_transactions =
            request.replay_preconditions.has_value()
                ? request.replay_preconditions->transaction_count
                : 0U;
        if (!source_matches || value.replayed_transaction_count != expected_transactions)
            return false;
        geometer::contracts::StepTopologyAnalyzeRecoveryResultA0 recovery;
        recovery.groups = value.recovery;
        std::string recovery_json;
        require(geometer::contracts::encode_json(recovery, &recovery_json, &error),
                "nested restore recovery must encode");
        rapidjson::Document recovery_vector;
        recovery_vector.Parse(R"({"oracle":"step_topology_recovery_result"})");
        const std::vector<unsigned char> recovery_bytes(recovery_json.begin(), recovery_json.end());
        return validate_step_topology_semantics(recovery_vector, recovery_bytes, vector_root);
    }
    return std::nullopt;
}

std::optional<bool> validate_hierarchy_oracle(const std::string& oracle,
                                              const rapidjson::Value& vector,
                                              const std::vector<unsigned char>& data,
                                              const std::string& vector_root)
{
    geometer::contracts::ContractError error;
    if (oracle == "step_topology_hierarchy_commands")
    {
        geometer::contracts::StepTopologyApplyHierarchyRequestA0 value;
        require(geometer::contracts::decode_json(data.data(), data.size(), &value, &error),
                "hierarchy command vector must be structurally valid");
        std::unordered_set<std::string> created_nodes;
        std::unordered_set<std::string> created_occurrences;
        for (const auto& command : value.commands)
        {
            bool valid = true;
            std::visit(
                [&](const auto& item)
                {
                    using Item = std::decay_t<decltype(item)>;
                    if constexpr (std::is_same_v<
                                      Item, geometer::contracts::CreateHierarchyProductCommand>)
                    {
                        valid = valid_hierarchy_name(item.authored_id,
                                                     "wn.geometer.research.product.") &&
                                created_nodes.insert(item.authored_id).second;
                    }
                    else if constexpr (std::is_same_v<
                                           Item,
                                           geometer::contracts::CreateHierarchyAssemblyCommand>)
                    {
                        valid = valid_hierarchy_name(item.authored_id,
                                                     "wn.geometer.research.assembly.") &&
                                created_nodes.insert(item.authored_id).second;
                    }
                    else if constexpr (std::is_same_v<
                                           Item,
                                           geometer::contracts::CreateHierarchyOccurrenceCommand>)
                    {
                        valid = valid_hierarchy_name(item.authored_id,
                                                     "wn.geometer.research.occurrence.") &&
                                created_occurrences.insert(item.authored_id).second &&
                                (valid_hierarchy_name(item.child_authored_id,
                                                      "wn.geometer.research.product.") ||
                                 valid_hierarchy_name(item.child_authored_id,
                                                      "wn.geometer.research.assembly.")) &&
                                valid_hierarchy_name(item.parent_assembly_authored_id,
                                                     "wn.geometer.research.assembly.");
                    }
                    else if constexpr (std::is_same_v<
                                           Item,
                                           geometer::contracts::ReparentHierarchyOccurrenceCommand>)
                    {
                        valid = valid_hierarchy_name(item.authored_id,
                                                     "wn.geometer.research.occurrence.") &&
                                valid_hierarchy_name(item.parent_assembly_authored_id,
                                                     "wn.geometer.research.assembly.");
                    }
                    else if constexpr (std::is_same_v<
                                           Item,
                                           geometer::contracts::EraseHierarchyOccurrenceCommand>)
                    {
                        valid = valid_hierarchy_name(item.authored_id,
                                                     "wn.geometer.research.occurrence.");
                        created_occurrences.erase(item.authored_id);
                    }
                    else
                    {
                        valid = valid_hierarchy_name(item.authored_id,
                                                     "wn.geometer.research.product.") ||
                                valid_hierarchy_name(item.authored_id,
                                                     "wn.geometer.research.assembly.");
                        if constexpr (std::is_same_v<
                                          Item, geometer::contracts::EraseHierarchyNodeCommand>)
                            created_nodes.erase(item.authored_id);
                    }
                },
                command);
            if (!valid)
                return false;
        }
        return true;
    }
    if (oracle == "step_topology_hierarchy_result")
    {
        geometer::contracts::StepTopologyApplyHierarchyResultA0 value;
        require(geometer::contracts::decode_json(data.data(), data.size(), &value, &error),
                "hierarchy result vector must be structurally valid");
        std::unordered_map<std::string, geometer::contracts::HierarchyNodeKind> nodes;
        for (const auto& node : value.hierarchy.nodes)
        {
            const char* prefix = node.kind == geometer::contracts::HierarchyNodeKind::product
                                     ? "wn.geometer.research.product."
                                     : "wn.geometer.research.assembly.";
            if (!valid_hierarchy_name(node.authored_id, prefix) ||
                !nodes.emplace(node.authored_id, node.kind).second)
                return false;
            const bool has_source = node.source_kind.has_value() && node.source_handle.has_value();
            if ((node.kind == geometer::contracts::HierarchyNodeKind::product) != has_source)
                return false;
        }
        std::unordered_set<std::string> occurrence_ids;
        std::unordered_map<std::string, std::vector<std::string>> children;
        for (const auto& occurrence : value.hierarchy.occurrences)
        {
            const auto child = nodes.find(occurrence.child_authored_id);
            const auto parent = nodes.find(occurrence.parent_assembly_authored_id);
            if (!valid_hierarchy_name(occurrence.authored_id, "wn.geometer.research.occurrence.") ||
                !occurrence_ids.insert(occurrence.authored_id).second || child == nodes.end() ||
                parent == nodes.end() ||
                parent->second != geometer::contracts::HierarchyNodeKind::assembly)
                return false;
            children[parent->first].push_back(child->first);
        }
        std::unordered_map<std::string, std::size_t> indegree;
        for (const auto& [id, kind] : nodes)
            if (kind == geometer::contracts::HierarchyNodeKind::assembly)
                indegree.emplace(id, 0U);
        for (const auto& [parent, child_ids] : children)
        {
            (void)parent;
            for (const auto& child : child_ids)
                if (const auto found = indegree.find(child); found != indegree.end())
                    ++found->second;
        }
        std::vector<std::string> ready;
        for (const auto& [id, degree] : indegree)
            if (degree == 0U)
                ready.push_back(id);
        std::size_t visited_count = 0U;
        for (std::size_t index = 0U; index < ready.size(); ++index)
        {
            ++visited_count;
            for (const auto& child : children[ready[index]])
            {
                if (const auto found = indegree.find(child);
                    found != indegree.end() && --found->second == 0U)
                    ready.push_back(child);
            }
        }
        return visited_count == indegree.size();
    }
    return std::nullopt;
}

std::optional<bool> validate_recovery_oracle(const std::string& oracle,
                                             const rapidjson::Value& vector,
                                             const std::vector<unsigned char>& data,
                                             const std::string& vector_root)
{
    geometer::contracts::ContractError error;
    if (oracle == "step_topology_recovery_request")
    {
        geometer::contracts::StepTopologyAnalyzeRecoveryRequestA0 value;
        require(geometer::contracts::decode_json(data.data(), data.size(), &value, &error),
                "recovery request vector must be structurally valid");
        std::unordered_set<std::string> group_ids;
        std::size_t candidate_count = 0;
        for (const auto& group : value.groups)
        {
            if (!valid_research_name(group.group_authored_id, "wn.geometer.research.group.") ||
                !group_ids.insert(group.group_authored_id).second ||
                !valid_lowercase_sha256(group.provenance.source_artifact_sha256) ||
                !valid_lowercase_sha256(group.provenance.candidate_artifact_sha256))
                return false;
            std::unordered_set<std::string> member_ids;
            for (const auto& member : group.members)
            {
                if (!member_ids.insert(member.member_record_id).second)
                    return false;
                std::unordered_set<std::string> handles;
                candidate_count += member.candidates.size();
                if (candidate_count > 65536U)
                    return false;
                if (member.source_fingerprint.has_value() &&
                    !valid_recovery_fingerprint(*member.source_fingerprint))
                    return false;
                for (const auto& candidate : member.candidates)
                {
                    const bool verified =
                        candidate.topology_link_verified || candidate.carrier_locator_validated;
                    if (candidate.kind != member.kind ||
                        !valid_target_handle(candidate.target_handle) ||
                        !handles.insert(candidate.target_handle).second ||
                        (candidate.topology_link_verified &&
                         (!candidate.authored_target_id.has_value() ||
                          candidate.authored_target_id->empty())) ||
                        (candidate.carrier_locator_validated &&
                         candidate.carrier_locator.empty()) ||
                        (verified && candidate.carrier_record.empty()) ||
                        (candidate.lineage != geometer::contracts::RecoveryLineage::none &&
                         !verified) ||
                        (candidate.fingerprint.has_value() &&
                         !valid_recovery_fingerprint(*candidate.fingerprint)))
                        return false;
                }
            }
        }
        return true;
    }
    if (oracle == "step_topology_recovery_result")
    {
        geometer::contracts::StepTopologyAnalyzeRecoveryResultA0 value;
        require(geometer::contracts::decode_json(data.data(), data.size(), &value, &error),
                "recovery result vector must be structurally valid");
        std::unordered_set<std::string> group_ids;
        for (const auto& group : value.groups)
        {
            if (!valid_research_name(group.group_authored_id, "wn.geometer.research.group.") ||
                !group_ids.insert(group.group_authored_id).second)
                return false;
            std::uint32_t resolved = 0;
            std::uint32_t ambiguous = 0;
            std::uint32_t unresolved = 0;
            std::uint32_t unsupported = 0;
            std::unordered_set<std::string> member_ids;
            for (const auto& member : group.members)
            {
                if (!member_ids.insert(member.member_record_id).second ||
                    member.evidence.matching_candidate_count > member.evidence.candidate_count)
                    return false;
                std::unordered_set<std::string> rejected_handles;
                for (const auto& alternative : member.evidence.rejected_alternatives)
                    if (!rejected_handles.insert(alternative.target_handle).second)
                        return false;
                if (member.resolved_target_handle.has_value() &&
                    rejected_handles.count(*member.resolved_target_handle) != 0U)
                    return false;
                if (member.evidence.matching_candidate_count +
                        member.evidence.rejected_alternatives.size() !=
                    member.evidence.candidate_count)
                    return false;
                const bool has_handle = member.resolved_target_handle.has_value();
                if ((has_handle && !valid_target_handle(*member.resolved_target_handle)) ||
                    std::any_of(member.evidence.rejected_alternatives.begin(),
                                member.evidence.rejected_alternatives.end(),
                                [](const auto& alternative)
                                { return !valid_target_handle(alternative.target_handle); }))
                    return false;
                const bool has_method =
                    member.resolution_method != geometer::contracts::RecoveryResolutionMethod::none;
                const bool fingerprint_method = member.resolution_method ==
                                                geometer::contracts::RecoveryResolutionMethod::
                                                    unique_geometry_adjacency_fingerprint;
                const bool valid_resolved_confidence =
                    fingerprint_method
                        ? member.confidence == geometer::contracts::RecoveryConfidence::medium
                        : member.confidence == geometer::contracts::RecoveryConfidence::high;
                const bool valid_resolution_fields =
                    member.resolution_state ==
                            geometer::contracts::RecoveryResolutionState::resolved
                        ? has_handle && has_method && valid_resolved_confidence &&
                              member.evidence.matching_candidate_count == 1U &&
                              member.topology_comparison !=
                                  geometer::contracts::RecoveryTopologyComparison::not_compared
                    : member.resolution_state ==
                            geometer::contracts::RecoveryResolutionState::ambiguous
                        ? !has_handle && has_method &&
                              member.confidence == geometer::contracts::RecoveryConfidence::none &&
                              member.topology_comparison ==
                                  geometer::contracts::RecoveryTopologyComparison::not_compared &&
                              member.evidence.matching_candidate_count > 1U
                    : member.resolution_state ==
                            geometer::contracts::RecoveryResolutionState::unresolved
                        ? !has_handle && !has_method &&
                              member.confidence == geometer::contracts::RecoveryConfidence::none &&
                              member.topology_comparison ==
                                  geometer::contracts::RecoveryTopologyComparison::not_compared &&
                              member.evidence.matching_candidate_count == 0U
                        : !has_handle && !has_method &&
                              member.confidence == geometer::contracts::RecoveryConfidence::none &&
                              member.topology_comparison ==
                                  geometer::contracts::RecoveryTopologyComparison::unavailable &&
                              member.evidence.matching_candidate_count == 0U;
                if (!valid_resolution_fields)
                    return false;
                switch (member.resolution_state)
                {
                case geometer::contracts::RecoveryResolutionState::resolved:
                    ++resolved;
                    break;
                case geometer::contracts::RecoveryResolutionState::ambiguous:
                    ++ambiguous;
                    break;
                case geometer::contracts::RecoveryResolutionState::unresolved:
                    ++unresolved;
                    break;
                case geometer::contracts::RecoveryResolutionState::unsupported:
                    ++unsupported;
                    break;
                }
            }
            if (resolved != group.resolved_member_count ||
                ambiguous != group.ambiguous_member_count ||
                unresolved != group.unresolved_member_count ||
                unsupported != group.unsupported_member_count)
                return false;
            geometer::contracts::RecoveryGroupCompleteness completeness;
            geometer::contracts::RecoveryResolutionState state;
            if (resolved == group.members.size())
            {
                completeness = geometer::contracts::RecoveryGroupCompleteness::fully_recovered;
                state = geometer::contracts::RecoveryResolutionState::resolved;
            }
            else if (resolved != 0U)
            {
                completeness = geometer::contracts::RecoveryGroupCompleteness::partially_recovered;
                state = ambiguous != 0U ? geometer::contracts::RecoveryResolutionState::ambiguous
                                        : geometer::contracts::RecoveryResolutionState::unresolved;
            }
            else if (unsupported == group.members.size())
            {
                completeness = geometer::contracts::RecoveryGroupCompleteness::unsupported;
                state = geometer::contracts::RecoveryResolutionState::unsupported;
            }
            else
            {
                completeness = geometer::contracts::RecoveryGroupCompleteness::unrecovered;
                state = ambiguous != 0U ? geometer::contracts::RecoveryResolutionState::ambiguous
                                        : geometer::contracts::RecoveryResolutionState::unresolved;
            }
            if (group.completeness != completeness || group.resolution_state != state)
                return false;
        }
        return true;
    }
    return std::nullopt;
}
} // namespace

bool validate_step_topology_semantics(const rapidjson::Value& vector,
                                      const std::vector<unsigned char>& data,
                                      const std::string& vector_root)
{
    const std::string oracle = vector["oracle"].GetString();
    for (const auto validator :
         {validate_ipc_oracle, validate_inspection_oracle, validate_mutation_oracle,
          validate_persistence_oracle, validate_hierarchy_oracle, validate_recovery_oracle})
    {
        if (const std::optional<bool> result = validator(oracle, vector, data, vector_root))
            return *result;
    }
    throw std::runtime_error("unhandled STEP topology semantic oracle: " + oracle);
}
