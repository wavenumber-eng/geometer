#include "geometer/c_api.h"
#include "geometer/generated/contracts/contracts.h"
#include "geometer/operation_transport.h"

#include <algorithm>
#include <cctype>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <fstream>
#include <iostream>
#include <iterator>
#include <rapidjson/document.h>
#include <stdexcept>
#include <string>
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
    throw std::runtime_error("unhandled contract vector identity: " + identity);
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
    require(manifest["vectors"].Size() == 20U, "C++ must replay every governed contract vector");

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
        const bool accepted = decode_contract_vector(identity, data, &options);
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
    require(catalog_document["operations"].Size() == 1U,
            "runtime catalog should contain every generated operation exactly once");
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

} // namespace

int main()
{
    int result = 0;
    try
    {
        generated_options_codec_preserves_presence_and_is_strict();
        generated_cpp_replays_all_governed_contract_vectors();
        generated_cpp_replays_governed_operation_vectors();
        generated_encoder_rejects_invalid_utf8();
        generated_ipc_control_codecs_are_strict();
        response_limits_fail_closed_before_accessor_narrowing();
        generic_c_abi_catalog_and_typed_failures();
        generic_c_abi_executes_model_bounds();
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
