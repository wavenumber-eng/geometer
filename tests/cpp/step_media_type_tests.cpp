#include "step_media_type.h"

#include "geometer/c_api.h"
#include "geometer/model_illustration.h"
#include "geometer/operation_registry.h"
#include "geometer/sha256.h"

#include <cmath>
#include <cstdint>
#include <fstream>
#include <iterator>
#include <rapidjson/document.h>
#include <rapidjson/stringbuffer.h>
#include <rapidjson/writer.h>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

namespace
{
using Bytes = std::vector<unsigned char>;

void require(bool value, const std::string& message)
{
    if (!value)
        throw std::runtime_error(message);
}

Bytes fixture(const std::string& name)
{
    std::ifstream file(std::string(GEOMETER_TEST_SOURCE_DIR) + "/tests/fixtures/" + name,
                       std::ios::binary);
    require(static_cast<bool>(file), "STEP MIME fixture missing: " + name);
    return {std::istreambuf_iterator<char>(file), {}};
}

void shared_parser_vectors()
{
    const auto bytes = fixture("step_media_types.json");
    rapidjson::Document document;
    document.Parse(reinterpret_cast<const char*>(bytes.data()), bytes.size());
    require(!document.HasParseError() && document.IsObject() && document.HasMember("cases") &&
                document["cases"].IsArray(),
            "STEP MIME shared fixture is malformed");
    for (const auto& test : document["cases"].GetArray())
    {
        require(test.IsObject() && test.HasMember("value") && test["value"].IsString() &&
                    test.HasMember("accepted") && test["accepted"].IsBool(),
                "STEP MIME fixture case is malformed");
        const std::string value(test["value"].GetString(), test["value"].GetStringLength());
        require(geometer::step_media_type_detail::accepts(value) == test["accepted"].GetBool(),
                "STEP MIME acceptance differs for shared vector: " + value);
    }
    const std::string prefix = "model/step; unused=\"";
    const std::string maximum = prefix + std::string(127 - prefix.size(), 'x') + "\"";
    require(maximum.size() == 128 && geometer::step_media_type_detail::accepts(maximum),
            "128-byte STEP MIME value was rejected");
    require(!geometer::step_media_type_detail::accepts(maximum + " "),
            "STEP MIME field size bound was bypassed");
    require(geometer::step_media_type_detail::accepts("model/step " + std::string(58, '(') +
                                                      std::string(58, ')')),
            "bounded nested comments failed iterative parsing");
}

std::string outcome(const geometer::OperationExecution& execution)
{
    std::string result;
    require(geometer::encode_operation_outcome(execution.outcome, &result),
            "STEP MIME operation outcome did not encode");
    return result;
}

geometer::OperationExecution invoke(const std::string& operation, const std::string& request,
                                    const std::string& media, const Bytes& source,
                                    const std::string& name = "model")
{
    geometer::OperationExecution execution;
    geometer::execute_operation(operation, reinterpret_cast<const unsigned char*>(request.data()),
                                request.size(), {{name, media, source.data(), source.size()}},
                                &execution);
    return execution;
}

void require_success(const geometer::OperationExecution& execution)
{
    require(std::holds_alternative<geometer::contracts::OperationSuccessA0>(execution.outcome) ||
                std::holds_alternative<geometer::contracts::OperationSuccessB0>(execution.outcome),
            "Actual STEP MIME operation failed: " + outcome(execution));
}

std::string governed_bounds_outcome(const std::string& json)
{
    rapidjson::Document value;
    value.Parse(json.data(), json.size());
    require(!value.HasParseError() && value.IsObject() && value.HasMember("result") &&
                value["result"].IsObject() && value["result"].HasMember("timings") &&
                value["result"]["timings"].IsObject(),
            "Bounds outcome is missing governed timings: " + json);
    auto& timings = value["result"]["timings"];
    // The governed bounds vector excludes exactly these two measured durations.
    // Every other envelope, source, geometry and future result field stays exact.
    for (const char* field : {"model_read_ms", "bounds_ms"})
    {
        require(timings.HasMember(field) && timings[field].IsNumber() &&
                    std::isfinite(timings[field].GetDouble()) && timings[field].GetDouble() >= 0,
                "Bounds timing must remain finite and nonnegative: " + json);
        timings.RemoveMember(field);
    }
    rapidjson::StringBuffer buffer;
    rapidjson::Writer<rapidjson::StringBuffer> writer(buffer);
    require(value.Accept(writer), "Governed bounds comparison did not encode");
    return {buffer.GetString(), buffer.GetSize()};
}

void require_same_bounds(const std::string& actual, const std::string& expected)
{
    const auto actual_fields = governed_bounds_outcome(actual);
    const auto expected_fields = governed_bounds_outcome(expected);
    require(actual_fields == expected_fields,
            "STEP MIME changed governed bounds/source fields: actual=" + actual_fields +
                " expected=" + expected_fields);
}

void actual_bounds_and_c_abi(const Bytes& source)
{
    const std::string operation = "geometry.model_bounds.a0", request = "{}";
    const auto baseline = invoke(operation, request, "model/step", source);
    require_success(baseline);
    for (const std::string media : {"application/step", "MODEL/STEP; kicad-type=other",
                                    "(source) Application / Step; name=\"part;1.stp\""})
    {
        const auto result = invoke(operation, request, media, source);
        require_success(result);
        require_same_bounds(outcome(result), outcome(baseline));
    }
    const std::string media = "MODEL/STEP (source); name=\"actual;part.stp\"";
    GeometerAttachmentView attachment{};
    attachment.struct_size = sizeof(GeometerAttachmentView);
    attachment.name = "model";
    attachment.name_size = 5;
    attachment.media_type = media.data();
    attachment.media_type_size = static_cast<std::uint32_t>(media.size());
    attachment.data = source.data();
    attachment.data_size = static_cast<std::uint32_t>(source.size());
    GeometerOperationResult* result = nullptr;
    char* error = nullptr;
    const int code = geometer_operation_execute(
        operation.data(), static_cast<std::uint32_t>(operation.size()),
        reinterpret_cast<const unsigned char*>(request.data()),
        static_cast<std::uint32_t>(request.size()), &attachment, 1, &result, &error);
    require(code == GEOMETER_OPERATION_ABI_OK && result != nullptr && error == nullptr,
            "Generic C ABI rejected a valid STEP MIME value before dispatch");
    const auto size = geometer_operation_result_json_size(result);
    const auto* json = geometer_operation_result_json_data(result);
    const std::string actual =
        json == nullptr ? std::string{} : std::string(reinterpret_cast<const char*>(json), size);
    geometer_operation_result_free(result);
    require_same_bounds(actual, outcome(baseline));
}

template <typename Request> std::string encode_request(const Request& request)
{
    std::string json;
    require(geometer::contracts::encode_json(request, &json), "STEP MIME request did not encode");
    Request decoded;
    geometer::contracts::ContractError error;
    require(geometer::contracts::decode_json(reinterpret_cast<const unsigned char*>(json.data()),
                                             json.size(), &decoded, &error),
            "STEP MIME request does not satisfy its governed contract: " + error.message);
    return json;
}

template <typename Request> std::string illustration_request()
{
    Request request;
    request.source = geometer::contracts::ModelAttachmentIllustrationSourceA0{};
    request.view.direction = {0, 0, 1};
    request.view.up = {0, 1, 0};
    return encode_request(request);
}

void actual_model_operations(const Bytes& source)
{
    require(geometer::ModelIllustrationAttachmentView{}.media_type == "model/step",
            "Native model illustration default is not canonical");
    require_success(invoke("geometry.model_hlr_projection.a0", "{}",
                           "MODEL/STEP; encoding=gzip; units=inches", source));
    const auto tessellation =
        invoke("geometry.model_tessellation.a0",
               encode_request(geometer::contracts::ModelTessellationRequestA0{}),
               "Application/Step (model); name=part.stp", source);
    require_success(tessellation);
    require(outcome(tessellation).find(geometer::sha256_hex(source.data(), source.size())) !=
                std::string::npos,
            "Ignored STEP parameters changed source identity");
    const std::vector<std::pair<std::string, std::string>> requests = {
        {"geometry.model_illustration_geometry.a0",
         illustration_request<geometer::contracts::ModelIllustrationGeometryRequestA0>()},
        {"geometry.model_illustration.a0",
         illustration_request<geometer::contracts::ModelIllustrationRequestA0>()},
        {"geometry.model_illustration_geometry.b0",
         illustration_request<geometer::contracts::ModelIllustrationGeometryRequestB0>()},
        {"geometry.model_illustration.b0",
         illustration_request<geometer::contracts::ModelIllustrationRequestB0>()}};
    for (const auto& request : requests)
    {
        const auto result =
            invoke(request.first, request.second, "Application/Step; name=\"a;part\"", source);
        require_success(result);
        const auto json = outcome(result);
        require(json.find("\"media_type\":\"model/step\"") != std::string::npos &&
                    json.find("\"media_type\":\"application/step\"") == std::string::npos,
                "New illustration summary did not normalize legacy input to model/step");
    }
}

void strict_failures_and_output_matching(const Bytes& source)
{
    const auto malformed = invoke("geometry.model_bounds.a0", "{}", "model/step; name=", source);
    require(outcome(malformed).find("attachment_media_type_mismatch") != std::string::npos,
            "Malformed STEP MIME did not retain typed media failure: " + outcome(malformed));
    const auto unrelated =
        invoke("geometry.mesh_hlr_projection.a0", "{}",
               "application/vnd.wavenumber.geometer.indexed-triangle-mesh; x=y", source, "mesh");
    require(outcome(unrelated).find("attachment_media_type_mismatch") != std::string::npos,
            "STEP input matching leaked into unrelated packed media: " + outcome(unrelated));
    require(!geometer::operation_output_attachment_declared(
                "geometry.model_tessellation.a0", "mesh_collection",
                "application/vnd.wavenumber.geometer.mesh-collection+json; x=y"),
            "STEP input normalization relaxed output catalog matching");
    const Bytes invalid{'n', 'o', 't', ' ', 'S', 'T', 'E', 'P'};
    const auto failure = invoke("geometry.model_bounds.a0", "{}", "MODEL/STEP; x=y", invalid);
    require(std::holds_alternative<geometer::contracts::OperationFailureA0>(failure.outcome) &&
                outcome(failure).find("attachment_media_type_mismatch") == std::string::npos,
            "Valid MIME incorrectly supplied authority for invalid STEP bytes: " +
                outcome(failure));
}

#ifndef __EMSCRIPTEN__
template <typename Request>
geometer::OperationExecution
native_invoke(const std::string& operation, const Request& request,
              const std::vector<geometer::OperationAttachmentView>& attachments = {})
{
    const auto json = encode_request(request);
    geometer::OperationExecution execution;
    geometer::execute_native_operation(operation,
                                       reinterpret_cast<const unsigned char*>(json.data()),
                                       json.size(), attachments, &execution);
    require_success(execution);
    return execution;
}

void close(const geometer::contracts::SessionReference& session)
{
    geometer::contracts::StepTopologyCloseRequestA0 request;
    request.session = session;
    native_invoke("geometry.step_topology.close.a0", request);
}

void actual_topology_open_and_restore(const Bytes& source)
{
    using namespace geometer::contracts;
    const auto opened =
        native_invoke("geometry.step_topology.open.a0", StepTopologyOpenRequestA0{},
                      {{"step", "MODEL/STEP; x=source", source.data(), source.size()}});
    const auto& info =
        std::get<StepTopologyOpenResultA0>(std::get<OperationSuccessA0>(opened.outcome).result);
    StepTopologyCheckpointEditJournalRequestA0 checkpoint_request;
    checkpoint_request.session = info.session;
    auto checkpoint =
        native_invoke("geometry.step_topology.checkpoint_edit_journal.a0", checkpoint_request);
    const auto& journal = std::get<StepTopologyCheckpointEditJournalResultA0>(
        std::get<OperationSuccessA0>(checkpoint.outcome).result);
    StepTopologyRestoreRequestA0 request;
    request.source = info.source;
    EditJournalPersistenceArtifact artifact;
    artifact.bytes = journal.journal.bytes;
    artifact.sha256 = journal.journal.sha256;
    request.state_artifact = artifact;
    request.replay_preconditions = EditJournalReplayPreconditions{
        journal.source_sha256, journal.source_brep_sha256, journal.target_inventory_sha256,
        journal.occt_version, journal.transaction_count};
    require(checkpoint.attachments.size() == 1 &&
                checkpoint.attachments.front().name == journal.journal.name &&
                checkpoint.attachments.front().media_type == journal.journal.media_type,
            "Topology checkpoint did not issue its exact journal attachment: " +
                outcome(checkpoint));
    const auto& data = checkpoint.attachments.front().data;
    require(data.size() == artifact.bytes &&
                geometer::sha256_hex(data.data(), data.size()) == artifact.sha256,
            "Topology checkpoint journal bytes differ from the issued descriptor: " +
                outcome(checkpoint));
    const auto restored = native_invoke(
        "geometry.step_topology.restore.a0", request,
        {{"source", "Application/Step (source); x=ignored", source.data(), source.size()},
         {"state_artifact", artifact.media_type, data.data(), data.size()}});
    const auto& result = std::get<StepTopologyRestoreResultA0>(
        std::get<OperationSuccessA0>(restored.outcome).result);
    close(result.session);
    close(info.session);
    require(result.source.sha256 == info.source.sha256 && result.source.bytes == source.size(),
            "MIME normalization changed restored source descriptor or ownership");
}
#endif
} // namespace

void run_step_media_type_tests()
{
    shared_parser_vectors();
    const auto source = fixture("step/embedded_models/SOT-23.STEP");
    const auto before = source;
    actual_bounds_and_c_abi(source);
    actual_model_operations(source);
    strict_failures_and_output_matching(source);
#ifndef __EMSCRIPTEN__
    actual_topology_open_and_restore(source);
#endif
    require(source == before, "STEP MIME operations mutated captured source bytes");
}
