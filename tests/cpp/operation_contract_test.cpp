#include "geometer/c_api.h"
#include "geometer/generated/contracts/contracts.h"

#include <cstdio>
#include <cstdlib>
#include <fstream>
#include <iostream>
#include <iterator>
#include <stdexcept>
#include <string>
#include <vector>

namespace
{

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
