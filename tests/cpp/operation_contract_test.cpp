#include "geometer/analytic_request_packet.h"
#include "geometer/analytic_result_packet_records.h"
#include "geometer/c_api.h"
#include "geometer/generated/contracts/contracts.h"
#include "geometer/operation_transport.h"
#include "geometer/sha256.h"

#include <algorithm>
#include <cctype>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <fstream>
#include <iostream>
#include <iterator>
#include <limits>
#include <rapidjson/document.h>
#include <stdexcept>
#include <string>
#include <type_traits>
#include <unordered_set>
#include <vector>

namespace
{

std::string result_json(const GeometerOperationResult* result);

void require(bool condition, const std::string& message)
{
    if (!condition)
    {
        throw std::runtime_error(message);
    }
}

void generated_analytic_logical_dtos_cover_integer_domain()
{
    using namespace geometer::contracts;
    static_assert(std::is_same_v<JobId, std::uint64_t>);
    static_assert(std::is_same_v<decltype(PointNm::x), std::int64_t>);
    static_assert(std::variant_size_v<AuthoredSegment> == 3U);
    static_assert(std::variant_size_v<AuthoredPathSegment> == 2U);
    static_assert(std::variant_size_v<AnalyticPlanarBooleanJobResult> == 2U);
    static_assert(std::variant_size_v<AnalyticPlanarOperand> == 5U);
    static_assert(std::variant_size_v<DirectedFragment> == 2U);

    const PointNm point{std::numeric_limits<std::int64_t>::min(),
                        std::numeric_limits<std::int64_t>::max()};
    require(point.x == INT64_MIN && point.y == INT64_MAX,
            "generated analytic coordinates lost the signed int64 domain");
    SourceReference source{};
    source.primary_id = std::numeric_limits<std::uint64_t>::max();
    require(source.primary_id == UINT64_MAX,
            "generated analytic source identity lost the uint64 domain");
}

std::vector<unsigned char> read_bytes(const std::string& path)
{
    std::ifstream input(path, std::ios::binary);
    require(static_cast<bool>(input), "failed opening fixture: " + path);
    return {std::istreambuf_iterator<char>(input), std::istreambuf_iterator<char>()};
}

unsigned char hex_value(char value)
{
    if (value >= '0' && value <= '9')
    {
        return static_cast<unsigned char>(value - '0');
    }
    if (value >= 'a' && value <= 'f')
    {
        return static_cast<unsigned char>(value - 'a' + 10);
    }
    if (value >= 'A' && value <= 'F')
    {
        return static_cast<unsigned char>(value - 'A' + 10);
    }
    throw std::runtime_error("invalid hexadecimal contract vector");
}

std::vector<unsigned char> decode_hex(const std::vector<unsigned char>& text)
{
    std::string compact;
    for (const unsigned char value : text)
    {
        if (!std::isspace(value))
        {
            compact.push_back(static_cast<char>(value));
        }
    }
    require(compact.size() % 2U == 0U, "hexadecimal contract vector has odd length");
    std::vector<unsigned char> decoded;
    decoded.reserve(compact.size() / 2U);
    for (std::size_t index = 0; index < compact.size(); index += 2U)
    {
        decoded.push_back(static_cast<unsigned char>((hex_value(compact[index]) << 4U) |
                                                     hex_value(compact[index + 1U])));
    }
    return decoded;
}

std::string fnv1a64(const std::vector<unsigned char>& data)
{
    std::uint64_t hash = UINT64_C(14695981039346656037);
    for (const unsigned char value : data)
    {
        hash ^= value;
        hash *= UINT64_C(1099511628211);
    }
    char text[32]{};
    std::snprintf(text, sizeof(text), "fnv1a64:%016llx", static_cast<unsigned long long>(hash));
    return text;
}

bool decode_contract_vector(const std::string& identity, const std::vector<unsigned char>& data,
                            geometer::contracts::ModelBoundsOptionsA0* options)
{
    geometer::contracts::ContractError error;
    if (identity == "geometry.common.diagnostic.a0")
    {
        geometer::contracts::DiagnosticA0 value;
        return geometer::contracts::decode_json(data.data(), data.size(), &value, &error);
    }
    if (identity == "geometry.model_bounds.options.a0")
    {
        return geometer::contracts::decode_json(data.data(), data.size(), options, &error);
    }
    if (identity == "geometry.model_bounds.a0")
    {
        geometer::contracts::ModelBoundsResultA0 value;
        return geometer::contracts::decode_json(data.data(), data.size(), &value, &error);
    }
    if (identity == "geometer.operation.outcome.a0")
    {
        geometer::contracts::OperationOutcomeA0 value;
        return geometer::contracts::decode_json(data.data(), data.size(), &value, &error);
    }
    if (identity == "geometry.step_topology.resolve_hit.request.a0")
    {
        geometer::contracts::StepTopologyResolveHitRequestA0 value;
        if (!geometer::contracts::decode_json(data.data(), data.size(), &value, &error))
        {
            return false;
        }
        std::string encoded;
        require(geometer::contracts::encode_json(value, &encoded, &error),
                "accepted STEP topology hit vector should encode");
        std::string source(data.begin(), data.end());
        while (!source.empty() && (source.back() == '\r' || source.back() == '\n'))
        {
            source.pop_back();
        }
        require(encoded == source, "STEP topology hit vector must round-trip canonically");
        return true;
    }
    if (identity == "geometry.step_topology.resolve_hit.result.a0")
    {
        geometer::contracts::StepTopologyResolveHitResultA0 value;
        return geometer::contracts::decode_json(data.data(), data.size(), &value, &error);
    }
    if (identity == "geometry.step_topology.render.result.a0")
    {
        geometer::contracts::StepTopologyRenderResultA0 value;
        return geometer::contracts::decode_json(data.data(), data.size(), &value, &error);
    }
    if (identity == "geometry.step_topology.inspect.result.a0")
    {
        geometer::contracts::StepTopologyInspectResultA0 value;
        return geometer::contracts::decode_json(data.data(), data.size(), &value, &error);
    }
    throw std::runtime_error("unhandled contract vector identity: " + identity);
}

bool validate_step_topology_semantics(const rapidjson::Value& vector,
                                      const std::vector<unsigned char>& data,
                                      const std::string& vector_root)
{
    const std::string oracle = vector["oracle"].GetString();
    geometer::contracts::ContractError error;
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

        const auto contains =
            [](const std::vector<std::string>& values, const std::string& expected)
        { return std::find(values.begin(), values.end(), expected) != values.end(); };
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
        for (const auto& body : value.page.bodies)
        {
            if (find_definition(body.definition_handle) == value.page.definitions.end())
                return false;
            std::unordered_set<std::string> members;
            for (const auto& shell_handle : body.shell_handles)
            {
                if (!members.insert(shell_handle).second)
                    return false;
                const auto shell = find_shell(shell_handle);
                if (shell == value.page.shells.end() ||
                    shell->definition_handle != body.definition_handle ||
                    !contains(shell->body_handles, body.handle))
                    return false;
            }
            for (const auto& face_handle : body.face_handles)
            {
                if (!members.insert(face_handle).second)
                    return false;
                const auto face = find_face(face_handle);
                if (face == value.page.faces.end() ||
                    face->definition_handle != body.definition_handle ||
                    !contains(face->body_handles, body.handle))
                    return false;
            }
        }
        for (const auto& shell : value.page.shells)
        {
            if (find_definition(shell.definition_handle) == value.page.definitions.end())
                return false;
            std::unordered_set<std::string> members;
            for (const auto& body_handle : shell.body_handles)
            {
                if (!members.insert(body_handle).second)
                    return false;
                const auto body = find_body(body_handle);
                if (body == value.page.bodies.end() ||
                    body->definition_handle != shell.definition_handle ||
                    !contains(body->shell_handles, shell.handle))
                    return false;
            }
            for (const auto& face_handle : shell.face_handles)
            {
                if (!members.insert(face_handle).second)
                    return false;
                const auto face = find_face(face_handle);
                if (face == value.page.faces.end() ||
                    face->definition_handle != shell.definition_handle ||
                    !contains(face->shell_handles, shell.handle))
                    return false;
            }
        }
        for (const auto& face : value.page.faces)
        {
            if (find_definition(face.definition_handle) == value.page.definitions.end())
                return false;
            std::unordered_set<std::string> members;
            for (const auto& body_handle : face.body_handles)
            {
                if (!members.insert(body_handle).second)
                    return false;
                const auto body = find_body(body_handle);
                if (body == value.page.bodies.end() ||
                    body->definition_handle != face.definition_handle ||
                    !contains(body->face_handles, face.handle))
                    return false;
            }
            for (const auto& shell_handle : face.shell_handles)
            {
                if (!members.insert(shell_handle).second)
                    return false;
                const auto shell = find_shell(shell_handle);
                if (shell == value.page.shells.end() ||
                    shell->definition_handle != face.definition_handle ||
                    !contains(shell->face_handles, face.handle))
                    return false;
            }
        }
        return true;
    }
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
    throw std::runtime_error("unhandled STEP topology semantic oracle: " + oracle);
}

std::string execute_model_bounds_vector(const rapidjson::Value& vector)
{
    const std::string root = std::string(GEOMETER_TEST_SOURCE_DIR) + "/";
    const std::string operation = vector["operation"].GetString();
    const std::vector<unsigned char> request =
        read_bytes(root + "tests/contracts/vectors/" + vector["request_file"].GetString());
    const auto& attachment_value = vector["attachments"][0];
    const std::string name = attachment_value["name"].GetString();
    const std::string media_type = attachment_value["media_type"].GetString();
    const std::vector<unsigned char> model =
        read_bytes(root + attachment_value["repository_file"].GetString());

    GeometerAttachmentView attachment{};
    attachment.struct_size = sizeof(GeometerAttachmentView);
    attachment.name = name.data();
    attachment.name_size = static_cast<uint32_t>(name.size());
    attachment.media_type = media_type.data();
    attachment.media_type_size = static_cast<uint32_t>(media_type.size());
    attachment.data = model.data();
    attachment.data_size = static_cast<uint32_t>(model.size());

    GeometerOperationResult* result = nullptr;
    char* error = nullptr;
    const int code = geometer_operation_execute(
        operation.data(), static_cast<uint32_t>(operation.size()), request.data(),
        static_cast<uint32_t>(request.size()), &attachment, 1U, &result, &error);
    require(code == GEOMETER_OPERATION_ABI_OK && result != nullptr && error == nullptr,
            "governed operation vector should produce a typed outcome");
    const std::string json = result_json(result);
    geometer_operation_result_free(result);
    return json;
}

void require_close(double actual, double expected, double absolute_tolerance,
                   double relative_tolerance, const std::string& path)
{
    const double allowed =
        absolute_tolerance + relative_tolerance * std::max(std::abs(actual), std::abs(expected));
    require(std::abs(actual - expected) <= allowed, "numeric mismatch at " + path);
}

void require_vector_close(const std::vector<double>& actual, const rapidjson::Value& expected,
                          double absolute_tolerance, double relative_tolerance,
                          const std::string& path)
{
    require(actual.size() == expected.Size(), "array size mismatch at " + path);
    for (rapidjson::SizeType index = 0; index < expected.Size(); ++index)
    {
        require_close(actual[index], expected[index].GetDouble(), absolute_tolerance,
                      relative_tolerance, path + "/" + std::to_string(index));
    }
}

void generated_cpp_replays_governed_operation_vectors()
{
    const std::string vector_root =
        std::string(GEOMETER_TEST_SOURCE_DIR) + "/tests/contracts/vectors/";
    const std::vector<unsigned char> manifest_bytes = read_bytes(vector_root + "manifest.json");
    rapidjson::Document manifest;
    manifest.Parse(reinterpret_cast<const char*>(manifest_bytes.data()), manifest_bytes.size());
    require(!manifest.HasParseError() && manifest["operation_vectors"].Size() == 2U,
            "C++ must replay every governed operation vector");

    for (const auto& vector : manifest["operation_vectors"].GetArray())
    {
        bool native_declared = false;
        for (const auto& runtime : vector["runtimes"].GetArray())
        {
            native_declared = native_declared || std::string(runtime.GetString()) == "native_c_abi";
        }
        require(native_declared, "operation vector must declare the native C ABI runtime");

        const std::string json = execute_model_bounds_vector(vector);
        geometer::contracts::OperationOutcomeA0 outcome;
        geometer::contracts::ContractError error;
        require(
            geometer::contracts::decode_json(reinterpret_cast<const unsigned char*>(json.data()),
                                             json.size(), &outcome, &error),
            "native operation outcome should decode: " + error.message);
        if (std::string(vector["expected"].GetString()) == "success")
        {
            const auto* success = std::get_if<geometer::contracts::OperationSuccessA0>(&outcome);
            require(success != nullptr, "governed native result should succeed");
            require(success->operation == vector["operation"].GetString(),
                    "governed native success operation should match");
            const auto* actual =
                std::get_if<geometer::contracts::ModelBoundsResultA0>(&success->result);
            require(actual != nullptr, "governed native result should be model_bounds");

            const std::vector<unsigned char> expected_bytes =
                read_bytes(vector_root + vector["expected_result_file"].GetString());
            rapidjson::Document expected;
            expected.Parse(reinterpret_cast<const char*>(expected_bytes.data()),
                           expected_bytes.size());
            require(!expected.HasParseError(), "expected operation result should be valid JSON");
            const auto& attachment = vector["attachments"][0];
            const std::vector<unsigned char> model =
                read_bytes(std::string(GEOMETER_TEST_SOURCE_DIR) + "/" +
                           attachment["repository_file"].GetString());
            require(vector["computed_fields"].Size() == 1U &&
                        std::string(expected["source"]["hash"].GetString()) ==
                            "computed:fnv1a64:model" &&
                        actual->source.hash == fnv1a64(model),
                    "native source hash should exactly match the raw attachment bytes");
            require(actual->schema == expected["schema"].GetString() &&
                        actual->units == expected["units"].GetString() &&
                        actual->source.format == geometer::contracts::ModelFormat::step &&
                        std::string(expected["source"]["format"].GetString()) == "step",
                    "native exact result projection should match");
            require(vector["excluded_fields"].Size() == 2U &&
                        std::isfinite(actual->timings.model_read_ms) &&
                        actual->timings.model_read_ms >= 0.0 &&
                        std::isfinite(actual->timings.bounds_ms) &&
                        actual->timings.bounds_ms >= 0.0,
                    "excluded native timings should remain valid");
            const double absolute = vector["tolerance"]["absolute"].GetDouble();
            const double relative = vector["tolerance"]["relative"].GetDouble();
            require_vector_close(actual->bounds.min, expected["bounds"]["min"], absolute, relative,
                                 "/result/bounds/min");
            require_vector_close(actual->bounds.max, expected["bounds"]["max"], absolute, relative,
                                 "/result/bounds/max");
            require_vector_close(actual->bounds.size, expected["bounds"]["size"], absolute,
                                 relative, "/result/bounds/size");
            require_vector_close(actual->bounds.center, expected["bounds"]["center"], absolute,
                                 relative, "/result/bounds/center");
        }
        else
        {
            const auto* failure = std::get_if<geometer::contracts::OperationFailureA0>(&outcome);
            require(failure != nullptr && failure->diagnostics.size() == 1U,
                    "governed native diagnostic should fail once");
            require(failure->operation == vector["operation"].GetString(),
                    "governed native failure operation should match");
            const auto& actual = failure->diagnostics.front();
            const auto& expected = vector["expected_diagnostic"];
            require(actual.code == expected["code"].GetString() &&
                        actual.category == geometer::contracts::DiagnosticCategory::operation &&
                        actual.retryable == expected["retryable"].GetBool() &&
                        !actual.path.has_value(),
                    "governed native diagnostic fields should match");
        }
    }
}

void generated_cpp_replays_all_governed_contract_vectors()
{
    const std::string root = std::string(GEOMETER_TEST_SOURCE_DIR) + "/tests/contracts/vectors/";
    const std::vector<unsigned char> manifest_bytes = read_bytes(root + "manifest.json");
    rapidjson::Document manifest;
    manifest.Parse(reinterpret_cast<const char*>(manifest_bytes.data()), manifest_bytes.size());
    require(!manifest.HasParseError() && manifest.IsObject(),
            "contract vector manifest should be valid JSON");
    require(manifest.HasMember("vectors") && manifest["vectors"].IsArray(),
            "contract vector manifest should contain an array");
    require(manifest["vectors"].Size() == 41U, "C++ must replay every governed contract vector");

    for (const auto& vector : manifest["vectors"].GetArray())
    {
        const std::string id = vector["id"].GetString();
        const std::string identity = vector["contract_identity"].GetString();
        const std::string relative_path = vector["file"].GetString();
        std::vector<unsigned char> data = read_bytes(root + relative_path);
        if (relative_path.size() >= 4U &&
            relative_path.compare(relative_path.size() - 4U, 4U, ".hex") == 0)
        {
            data = decode_hex(data);
        }

        geometer::contracts::ModelBoundsOptionsA0 options;
        bool accepted = decode_contract_vector(identity, data, &options);
        if (accepted && std::string(vector["lane"].GetString()) == "semantic" &&
            std::string(vector["oracle"].GetString()).rfind("step_topology_", 0) == 0)
        {
            accepted = validate_step_topology_semantics(vector, data, root);
        }
        require(accepted == (std::string(vector["expected"].GetString()) == "accept"),
                "unexpected C++ contract vector result: " + id);

        if (accepted && vector.HasMember("expected_value"))
        {
            const auto& expected = vector["expected_value"];
            const bool format_present = std::string(expected["format"].GetString()) == "present";
            const bool transform_present =
                std::string(expected["model_transform"].GetString()) == "present";
            require(options.format.has_value() == format_present,
                    "unexpected C++ format presence: " + id);
            require(options.model_transform.has_value() == transform_present,
                    "unexpected C++ model_transform presence: " + id);
        }
    }
}

void generated_options_codec_preserves_presence_and_is_strict()
{
    geometer::contracts::ModelBoundsOptionsA0 options;
    geometer::contracts::ContractError error;
    const std::string empty = "{}";
    require(geometer::contracts::decode_json(reinterpret_cast<const unsigned char*>(empty.data()),
                                             empty.size(), &options, &error),
            "empty options should decode: " + error.message);
    require(!options.format.has_value(), "absent format must remain absent");
    require(!options.model_transform.has_value(), "absent transform must remain absent");

    const std::string duplicate = R"({"format":"step","format":"step"})";
    require(
        !geometer::contracts::decode_json(reinterpret_cast<const unsigned char*>(duplicate.data()),
                                          duplicate.size(), &options, &error),
        "duplicate field should be rejected");
    require(error.code == "geometer.contract.duplicate_field",
            "duplicate field should have a stable diagnostic code");

    const std::string compatibility_alias = R"({"model_format":"STEP"})";
    require(!geometer::contracts::decode_json(
                reinterpret_cast<const unsigned char*>(compatibility_alias.data()),
                compatibility_alias.size(), &options, &error),
            "compatibility aliases do not belong to canonical generated DTOs");
    require(error.code == "geometer.contract.unknown_field",
            "compatibility alias should be an unknown canonical field");

    const std::string trailing = "{} true";
    require(
        !geometer::contracts::decode_json(reinterpret_cast<const unsigned char*>(trailing.data()),
                                          trailing.size(), &options, &error),
        "trailing JSON data should be rejected");
    require(error.code == "geometer.contract.invalid_json",
            "trailing JSON should have a stable diagnostic code");

    const std::string embedded_nul_key = "{\"bad\\u0000/name~\":1}";
    require(!geometer::contracts::decode_json(
                reinterpret_cast<const unsigned char*>(embedded_nul_key.data()),
                embedded_nul_key.size(), &options, &error),
            "unknown key containing an embedded NUL should be rejected");
    const std::string expected_path("/bad\0~1name~0", 13U);
    require(error.path == expected_path,
            "generated JSON pointer should preserve NUL and escape slash/tilde");
}

void generated_encoder_rejects_invalid_utf8()
{
    geometer::contracts::DiagnosticA0 diagnostic;
    diagnostic.code = "geometer.contract.test";
    diagnostic.category = geometer::contracts::DiagnosticCategory::contract;
    diagnostic.message.assign("invalid\xc3\x28", 9U);
    diagnostic.retryable = false;
    std::string json;
    geometer::contracts::ContractError error;
    require(!geometer::contracts::encode_json(diagnostic, &json, &error),
            "generated encoder should reject invalid UTF-8");
    require(error.code == "geometer.contract.invalid_utf8",
            "invalid UTF-8 should have a stable generated diagnostic code");
}

void generated_ipc_control_codecs_are_strict()
{
    const std::string hello_json =
        R"({"client_name":"cpp-test","client_version":"a0","protocols":["a0"]})";
    geometer::contracts::IpcHelloA0 hello;
    geometer::contracts::ContractError error;
    require(
        geometer::contracts::decode_json(reinterpret_cast<const unsigned char*>(hello_json.data()),
                                         hello_json.size(), &hello, &error),
        "generated IPC hello should decode: " + error.message);
    require(hello.protocols.size() == 1U && hello.protocols.front() == "a0",
            "generated IPC hello should retain the selected protocol");

    const std::string fractional_count =
        R"({"status":"complete","activeRequestCompleted":false,"rejectedQueuedRequestCount":1.5})";
    geometer::contracts::IpcShutdownAckA0 acknowledgment;
    require(!geometer::contracts::decode_json(
                reinterpret_cast<const unsigned char*>(fractional_count.data()),
                fractional_count.size(), &acknowledgment, &error),
            "generated uint32 controls should reject fractional values");
    require(error.code == "geometer.contract.number_range",
            "invalid generated uint32 values should have a stable diagnostic code");
}

void response_limits_fail_closed_before_accessor_narrowing()
{
    std::vector<geometer::OperationOutputAttachment> too_many(17U);
    std::string message;
    require(geometer::validate_operation_response("geometry.model_bounds.a0", "{}", too_many,
                                                  &message) ==
                geometer::OperationResponseValidationStatus::limit_exceeded,
            "more than 16 output attachments should exceed the response limit");

    std::vector<geometer::OperationOutputAttachment> empty_name(1U);
    empty_name[0].media_type = "application/octet-stream";
    require(geometer::validate_operation_response("geometry.model_bounds.a0", "{}", empty_name,
                                                  &message) ==
                geometer::OperationResponseValidationStatus::invalid,
            "empty output attachment names should fail response validation");

    std::vector<geometer::OperationOutputAttachment> undeclared(1U);
    undeclared[0].name = "unexpected";
    undeclared[0].media_type = "application/octet-stream";
    require(geometer::validate_operation_response("geometry.model_bounds.a0", "{}", undeclared,
                                                  &message) ==
                geometer::OperationResponseValidationStatus::invalid,
            "undeclared output attachments should fail response validation");

    const std::string oversized_json(8U * 1024U * 1024U + 1U, 'x');
    require(geometer::validate_operation_response("geometry.model_bounds.a0", oversized_json, {},
                                                  &message) ==
                geometer::OperationResponseValidationStatus::limit_exceeded,
            "response JSON over 8 MiB should fail before accessor exposure");

    require(geometer::validate_operation_response("geometry.analytic_planar_boolean_batch.a0",
                                                  R"({"ok":true})", {}, &message) ==
                geometer::OperationResponseValidationStatus::invalid,
            "packed analytic success should require its declared result attachment");
    const std::string analytic_failure =
        R"({"operation":"geometry.analytic_planar_boolean_batch.a0","ok":false,"diagnostics":[{"code":"geometer.operation.test","category":"operation","message":"failed","retryable":false}]})";
    require(geometer::validate_operation_response("geometry.analytic_planar_boolean_batch.a0",
                                                  analytic_failure, {}, &message) ==
                geometer::OperationResponseValidationStatus::ok,
            "packed analytic failure should remain attachment-free");
    require(geometer::validate_operation_response("geometry.analytic_planar_boolean_batch.a0",
                                                  "not-json", {}, &message) ==
                geometer::OperationResponseValidationStatus::invalid,
            "malformed outcome JSON should fail closed");
    std::vector<geometer::OperationOutputAttachment> declared_analytic(1U);
    declared_analytic[0].name = "analytic_planar_boolean_result";
    declared_analytic[0].media_type =
        "application/vnd.wavenumber.geometer.analytic-planar-boolean-result";
    require(geometer::validate_operation_response("geometry.analytic_planar_boolean_batch.a0",
                                                  analytic_failure, declared_analytic, &message) ==
                geometer::OperationResponseValidationStatus::invalid,
            "failure outcomes should reject all attachments");
    const std::string analytic_success =
        R"({"operation":"geometry.analytic_planar_boolean_batch.a0","ok":true,"result":{"schema":"geometry.analytic_planar_boolean_batch.result.a0","packet":{"attachment":"analytic_planar_boolean_result","format":"geometry.analytic_planar_boolean.packet.a0"}}})";
    require(geometer::validate_operation_response("geometry.analytic_planar_boolean_batch.a0",
                                                  analytic_success, {}, &message) ==
                geometer::OperationResponseValidationStatus::invalid,
            "valid packed success JSON should still require its result attachment");
    require(geometer::validate_operation_response("geometry.analytic_planar_boolean_batch.a0",
                                                  analytic_success, declared_analytic, &message) ==
                geometer::OperationResponseValidationStatus::ok,
            "catalog-correlated packed success should pass response validation");
    const std::string wrong_projection =
        R"({"operation":"geometry.analytic_planar_boolean_batch.a0","ok":true,"result":{"schema":"geometry.analytic_planar_boolean_batch.result.a0","packet":{"attachment":"wrong","format":"geometry.analytic_planar_boolean.packet.a0"}}})";
    require(geometer::validate_operation_response("geometry.analytic_planar_boolean_batch.a0",
                                                  wrong_projection, declared_analytic, &message) ==
                geometer::OperationResponseValidationStatus::invalid,
            "packed result JSON must name its catalog attachment");
    const std::string wrong_format =
        R"({"operation":"geometry.analytic_planar_boolean_batch.a0","ok":true,"result":{"schema":"geometry.analytic_planar_boolean_batch.result.a0","packet":{"attachment":"analytic_planar_boolean_result","format":"wrong.packet.a0"}}})";
    require(geometer::validate_operation_response("geometry.analytic_planar_boolean_batch.a0",
                                                  wrong_format, declared_analytic, &message) ==
                geometer::OperationResponseValidationStatus::invalid,
            "packed result JSON must name its catalog format");
    const std::string wrong_schema =
        R"({"operation":"geometry.analytic_planar_boolean_batch.a0","ok":true,"result":{"schema":"wrong.result.a0","packet":{"attachment":"analytic_planar_boolean_result","format":"geometry.analytic_planar_boolean.packet.a0"}}})";
    require(geometer::validate_operation_response("geometry.analytic_planar_boolean_batch.a0",
                                                  wrong_schema, declared_analytic, &message) ==
                geometer::OperationResponseValidationStatus::invalid,
            "packed result schema must match its catalog contract");
    const std::string wrong_operation =
        R"({"operation":"wrong.operation.a0","ok":true,"result":{"schema":"geometry.analytic_planar_boolean_batch.result.a0","packet":{"attachment":"analytic_planar_boolean_result","format":"geometry.analytic_planar_boolean.packet.a0"}}})";
    require(geometer::validate_operation_response("geometry.analytic_planar_boolean_batch.a0",
                                                  wrong_operation, declared_analytic, &message) ==
                geometer::OperationResponseValidationStatus::invalid,
            "packed result operation must match its request");
}

std::string result_json(const GeometerOperationResult* result)
{
    const auto* data = geometer_operation_result_json_data(result);
    const uint32_t size = geometer_operation_result_json_size(result);
    require(data != nullptr && size != 0U, "operation result JSON should be present");
    return {reinterpret_cast<const char*>(data), size};
}

void generic_c_abi_catalog_and_typed_failures()
{
    char* catalog = nullptr;
    char* error = nullptr;
    require(geometer_operation_catalog_json(&catalog, &error) == GEOMETER_OPERATION_ABI_OK,
            "catalog lookup should succeed");
    require(catalog != nullptr && error == nullptr, "catalog outputs should follow ABI rules");
    const std::string catalog_text(catalog);
    require(catalog_text.find("geometry.model_bounds.a0") != std::string::npos,
            "catalog should advertise model_bounds");
    require(catalog_text.find("\"wasm32\":{\"size\":36") != std::string::npos,
            "catalog should publish the wasm32 attachment layout");
    rapidjson::Document catalog_document;
    catalog_document.Parse(catalog_text.data(), catalog_text.size());
    require(!catalog_document.HasParseError() && catalog_document.IsObject(),
            "generated runtime catalog should be valid JSON");
    require(catalog_document["operations"].Size() == 2U,
            "runtime catalog should contain every generated operation exactly once");
    require(catalog_text.find("geometry.analytic_planar_boolean_batch.a0") != std::string::npos &&
                catalog_text.find("\"runtime_dispatch\":\"packed_attachment\"") !=
                    std::string::npos,
            "catalog should advertise packed analytic runtime dispatch");
    require(catalog_document["limits"]["response_json_bytes"].GetUint() == 8388608U,
            "runtime catalog should publish the response JSON limit");
    require(catalog_document["limits"]["attachment_count"].GetUint() == 16U,
            "runtime catalog should publish the attachment count limit");
    geometer_free_string(catalog);

    const std::string request = "{}";
    GeometerOperationResult* result = nullptr;
    int code = geometer_operation_execute(
        "geometry.not_implemented.a0", 27U, reinterpret_cast<const unsigned char*>(request.data()),
        static_cast<uint32_t>(request.size()), nullptr, 0U, &result, &error);
    require(code == GEOMETER_OPERATION_ABI_OK && result != nullptr && error == nullptr,
            "unknown operation should be a typed outcome");
    require(result_json(result).find("geometer.contract.unsupported_operation") !=
                std::string::npos,
            "unknown operation should carry its governed diagnostic");
    geometer_operation_result_free(result);

    const std::string unexpected_name = "bad/name~";
    GeometerAttachmentView unexpected{};
    unexpected.struct_size = sizeof(GeometerAttachmentView);
    unexpected.name = unexpected_name.data();
    unexpected.name_size = static_cast<uint32_t>(unexpected_name.size());
    unexpected.media_type = "application/octet-stream";
    unexpected.media_type_size = 24U;
    result = nullptr;
    code = geometer_operation_execute(
        "geometry.model_bounds.a0", 24U, reinterpret_cast<const unsigned char*>(request.data()),
        static_cast<uint32_t>(request.size()), &unexpected, 1U, &result, &error);
    require(code == GEOMETER_OPERATION_ABI_OK && result != nullptr,
            "undeclared attachment should be a typed outcome");
    require(result_json(result).find("/attachments/bad~1name~0") != std::string::npos,
            "attachment diagnostic paths should RFC 6901 escape dynamic names");
    geometer_operation_result_free(result);

    result = nullptr;
    code = geometer_operation_execute(
        "geometry.model_bounds.a0", 24U, reinterpret_cast<const unsigned char*>(request.data()),
        static_cast<uint32_t>(request.size()), nullptr, 0U, &result, &error);
    require(code == GEOMETER_OPERATION_ABI_OK && result != nullptr,
            "missing model should be a typed outcome");
    require(result_json(result).find("geometer.contract.missing_attachment") != std::string::npos,
            "missing model should carry its governed diagnostic");
    geometer_operation_result_free(result);

    result = nullptr;
    code = geometer_operation_execute("geometry.model_bounds.a0", 24U, nullptr, 0U, nullptr, 0U,
                                      &result, &error);
    require(code == GEOMETER_OPERATION_ABI_OK && result != nullptr,
            "empty JSON should be a typed contract failure");
    require(result_json(result).find("geometer.contract.invalid_json") != std::string::npos,
            "empty JSON should carry its governed diagnostic");
    geometer_operation_result_free(result);

    const std::string oversized_operation(129U, 'a');
    result = nullptr;
    code = geometer_operation_execute(oversized_operation.data(), 129U, nullptr, 0U, nullptr, 0U,
                                      &result, &error);
    require(code == GEOMETER_OPERATION_ABI_LIMIT_EXCEEDED && result == nullptr,
            "oversized operation identity should be a local limit failure");
    geometer_free_string(error);
    error = nullptr;

    result = reinterpret_cast<GeometerOperationResult*>(1);
    code = geometer_operation_execute(nullptr, 1U, nullptr, 0U, nullptr, 0U, &result, &error);
    require(code == GEOMETER_OPERATION_ABI_INVALID_ARGUMENT && result == nullptr,
            "foreign pointer failure should be local and initialize result");
    geometer_free_string(error);

    GeometerAttachmentView empty_text{};
    empty_text.struct_size = sizeof(GeometerAttachmentView);
    result = nullptr;
    error = nullptr;
    code = geometer_operation_execute(
        "geometry.model_bounds.a0", 24U, reinterpret_cast<const unsigned char*>(request.data()),
        static_cast<uint32_t>(request.size()), &empty_text, 1U, &result, &error);
    require(code == GEOMETER_OPERATION_ABI_INVALID_ARGUMENT && result == nullptr,
            "empty attachment names and media types should be rejected locally");
    geometer_free_string(error);
}

void generic_c_abi_executes_model_bounds()
{
    const std::string path =
        std::string(GEOMETER_TEST_SOURCE_DIR) + "/tests/fixtures/step/embedded_models/SOT-23.STEP";
    const std::vector<unsigned char> model = read_bytes(path);
    const std::string request = "{}";
    GeometerAttachmentView attachment{};
    attachment.struct_size = sizeof(GeometerAttachmentView);
    attachment.name = "model";
    attachment.name_size = 5U;
    attachment.media_type = "application/step";
    attachment.media_type_size = 16U;
    attachment.data = model.data();
    attachment.data_size = static_cast<uint32_t>(model.size());

    GeometerOperationResult* result = nullptr;
    char* error = nullptr;
    const int code = geometer_operation_execute(
        "geometry.model_bounds.a0", 24U, reinterpret_cast<const unsigned char*>(request.data()),
        static_cast<uint32_t>(request.size()), &attachment, 1U, &result, &error);
    require(code == GEOMETER_OPERATION_ABI_OK && result != nullptr && error == nullptr,
            "model_bounds generic invocation should succeed");
    const std::string json = result_json(result);
    require(json.find("\"ok\":true") != std::string::npos,
            "model_bounds should return a success outcome");
    require(json.find("\"schema\":\"geometry.model_bounds.a0\"") != std::string::npos,
            "success outcome should contain generated model_bounds result JSON");
    require(geometer_operation_result_attachment_count(result) == 0U,
            "model_bounds has no output attachments");
    uint32_t size = 99U;
    require(geometer_operation_result_attachment_name(result, 0U, &size) == nullptr && size == 0U,
            "out-of-range accessors should initialize size and return null");
    geometer_operation_result_free(result);
}

void generic_c_abi_executes_packed_analytic_batch()
{
    geometer::AnalyticRequestPacketRecords records;
    records.jobs = {{1, 0, 1}};
    records.stages = {{1, 1, 0, 1}};
    records.operands = {{1, 2, 0}};
    records.disks = {{1, 0, 0, 1'000'000}};
    const auto encoded = geometer::encode_analytic_request_packet(records);
    require(encoded.error == geometer::AnalyticRequestPacketError::none && encoded.value,
            "nonempty analytic request packet should encode");
    const std::string operation = "geometry.analytic_planar_boolean_batch.a0";
    const std::string request =
        "{\"schema\":\"geometry.analytic_planar_boolean_batch.request.a0\",\"packet\":{"
        "\"attachment\":\"analytic_planar_boolean_request\",\"format\":"
        "\"geometry.analytic_planar_boolean.packet.a0\"}}";
    const std::string name = "analytic_planar_boolean_request";
    const std::string media_type =
        "application/vnd.wavenumber.geometer.analytic-planar-boolean-request";
    GeometerAttachmentView attachment{};
    attachment.struct_size = sizeof(GeometerAttachmentView);
    attachment.name = name.data();
    attachment.name_size = static_cast<std::uint32_t>(name.size());
    attachment.media_type = media_type.data();
    attachment.media_type_size = static_cast<std::uint32_t>(media_type.size());
    attachment.data = encoded.value->data();
    attachment.data_size = static_cast<std::uint32_t>(encoded.value->size());
    const std::vector<unsigned char> original_request = *encoded.value;

    GeometerOperationResult* result = nullptr;
    char* error = nullptr;
    const int code = geometer_operation_execute(
        operation.data(), static_cast<std::uint32_t>(operation.size()),
        reinterpret_cast<const unsigned char*>(request.data()),
        static_cast<std::uint32_t>(request.size()), &attachment, 1U, &result, &error);
    require(code == GEOMETER_OPERATION_ABI_OK && result != nullptr && error == nullptr,
            "packed analytic generic invocation should succeed");
    const std::string json = result_json(result);
    require(json.find("\"ok\":true") != std::string::npos &&
                json.find("geometry.analytic_planar_boolean_batch.result.a0") != std::string::npos,
            "packed analytic success should reference its result projection");
    require(geometer_operation_result_attachment_count(result) == 1U,
            "packed analytic success should return one attachment");
    std::uint32_t size = 0;
    const char* output_name = geometer_operation_result_attachment_name(result, 0U, &size);
    require(output_name != nullptr &&
                std::string(output_name, size) == "analytic_planar_boolean_result",
            "packed analytic result attachment name drifted");
    const char* output_media = geometer_operation_result_attachment_media_type(result, 0U, &size);
    require(output_media != nullptr &&
                std::string(output_media, size) ==
                    "application/vnd.wavenumber.geometer.analytic-planar-boolean-result",
            "packed analytic result media type drifted");
    const unsigned char* output = geometer_operation_result_attachment_data(result, 0U, &size);
    const auto decoded = geometer::decode_analytic_result_packet_records(output, size);
    require(decoded.error == geometer::AnalyticResultPacketLayoutError::none && decoded.value &&
                decoded.value->job_results.size() == 1U &&
                decoded.value->job_results.front().job_id == 1U &&
                decoded.value->job_results.front().status == 0U &&
                decoded.value->job_results.front().result_region_count == 1U &&
                decoded.value->fragments.size() == 2U,
            "packed analytic result attachment did not decode canonically");
    require(*encoded.value == original_request,
            "packed analytic operation must not mutate caller-owned request bytes");
    geometer_operation_result_free(result);

    std::vector<unsigned char> malformed = *encoded.value;
    malformed[0] = 'X';
    attachment.data = malformed.data();
    result = nullptr;
    const int malformed_code = geometer_operation_execute(
        operation.data(), static_cast<std::uint32_t>(operation.size()),
        reinterpret_cast<const unsigned char*>(request.data()),
        static_cast<std::uint32_t>(request.size()), &attachment, 1U, &result, &error);
    require(malformed_code == GEOMETER_OPERATION_ABI_OK && result != nullptr && error == nullptr &&
                result_json(result).find(
                    "geometer.contract.analytic_planar_boolean_packet.invalid_packet") !=
                    std::string::npos &&
                geometer_operation_result_attachment_count(result) == 0U,
            "malformed analytic packet should be a typed attachment-free contract failure");
    geometer_operation_result_free(result);
}

} // namespace

int main()
{
    int result = 0;
    try
    {
        generated_analytic_logical_dtos_cover_integer_domain();
        generated_options_codec_preserves_presence_and_is_strict();
        generated_cpp_replays_all_governed_contract_vectors();
        generated_cpp_replays_governed_operation_vectors();
        generated_encoder_rejects_invalid_utf8();
        generated_ipc_control_codecs_are_strict();
        response_limits_fail_closed_before_accessor_narrowing();
        generic_c_abi_catalog_and_typed_failures();
        generic_c_abi_executes_model_bounds();
        generic_c_abi_executes_packed_analytic_batch();
    }
    catch (const std::exception& error)
    {
        std::cerr << error.what() << '\n';
        result = 1;
    }
    std::fflush(stdout);
    std::fflush(stderr);
    std::_Exit(result);
}
