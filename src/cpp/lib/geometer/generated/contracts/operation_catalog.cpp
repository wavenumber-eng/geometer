// Generated from wn_geometer_contract_catalog.a0.json. Do not edit.
#include "geometer/operation_registry.h"
#include "geometer/version.h"

#include <string>

namespace geometer
{

const char* operation_catalog_json()
{
    static const std::string catalog =
        std::string("{\"catalog\":\"wn.geometer.operation_catalog.a0\",\"generic_abi\":\"a0\","
                    "\"release_version\":\"") +
        version_string() + "\",\"c_abi_generation\":" + std::to_string(abi_version()) +
        ",\"operations\":[{\"identity\":\"geometry.analytic_planar_boolean_batch.a0\",\"request_"
        "contract\":\"geometry.analytic_planar_boolean_batch.request.a0\",\"result_contract\":"
        "\"geometry.analytic_planar_boolean_batch.result.a0\",\"input_attachments\":[{\"name\":"
        "\"analytic_planar_boolean_request\",\"required\":true,\"media_types\":[\"application/"
        "vnd.wavenumber.geometer.analytic-planar-boolean-request\"],\"max_bytes\":268435456}],"
        "\"output_attachments\":[{\"name\":\"analytic_planar_boolean_result\",\"required\":true,"
        "\"media_types\":[\"application/"
        "vnd.wavenumber.geometer.analytic-planar-boolean-result\"],\"max_bytes\":268435456}],"
        "\"runtime_dispatch\":\"packed_attachment\",\"request_projection\":{\"kind\":\"packed_"
        "attachment\",\"attachment_name\":\"analytic_planar_boolean_request\",\"format\":"
        "\"geometry.analytic_planar_boolean.packet.a0\"},\"result_projection\":{\"kind\":\"packed_"
        "attachment\",\"attachment_name\":\"analytic_planar_boolean_result\",\"format\":\"geometry."
        "analytic_planar_boolean.packet.a0\"}},{\"identity\":\"geometry.model_bounds.a0\","
        "\"request_contract\":\"geometry.model_bounds.options.a0\",\"result_contract\":\"geometry."
        "model_bounds.a0\",\"input_attachments\":[{\"name\":\"model\",\"required\":true,\"media_"
        "types\":[\"application/step\",\"model/"
        "step\"],\"max_bytes\":268435456}],\"output_attachments\":[],\"runtime_dispatch\":"
        "\"logical_dto\"}],\"attachment_descriptor\":{\"wasm32\":{\"size\":36,\"offsets\":{"
        "\"struct_size\":0,\"flags\":4,\"name\":8,\"name_size\":12,\"media_type\":16,\"media_type_"
        "size\":20,\"data\":24,\"data_size\":28,\"reserved0\":32}},\"pointer64\":{\"size\":56,"
        "\"offsets\":{\"struct_size\":0,\"flags\":4,\"name\":8,\"name_size\":16,\"media_type\":24,"
        "\"media_type_size\":32,\"data\":40,\"data_size\":48,\"reserved0\":52}}},\"limits\":{"
        "\"operation_id_bytes\":128,\"request_json_bytes\":8388608,\"response_json_bytes\":8388608,"
        "\"attachment_count\":16,\"attachment_name_bytes\":128,\"attachment_media_type_bytes\":128,"
        "\"attachment_bytes\":268435456,\"aggregate_attachment_bytes_native\":536870912,"
        "\"aggregate_attachment_bytes_wasm\":268435456}}";
    return catalog.c_str();
}

const char* normalized_contract_catalog_sha256()
{
    return "6cacbf010b3ae7a2af74c68517e5bcd68e3f4da11c904fb2626debf0ba8d17e2";
}

bool operation_output_attachment_declared(const std::string& operation_id,
                                          const std::string& attachment_name,
                                          const std::string& media_type)
{
    if (operation_id == "geometry.analytic_planar_boolean_batch.a0" &&
        attachment_name == "analytic_planar_boolean_result" &&
        media_type == "application/vnd.wavenumber.geometer.analytic-planar-boolean-result")
        return true;
    (void)operation_id;
    (void)attachment_name;
    (void)media_type;
    return false;
}

bool operation_input_attachment_declared(const std::string& operation_id,
                                         const std::string& attachment_name,
                                         const std::string& media_type)
{
    if (operation_id == "geometry.analytic_planar_boolean_batch.a0" &&
        attachment_name == "analytic_planar_boolean_request" &&
        media_type == "application/vnd.wavenumber.geometer.analytic-planar-boolean-request")
        return true;
    if (operation_id == "geometry.model_bounds.a0" && attachment_name == "model" &&
        media_type == "application/step")
        return true;
    if (operation_id == "geometry.model_bounds.a0" && attachment_name == "model" &&
        media_type == "model/step")
        return true;
    (void)operation_id;
    (void)attachment_name;
    (void)media_type;
    return false;
}

std::size_t operation_input_attachment_max_bytes(const std::string& operation_id,
                                                 const std::string& attachment_name)
{
    if (operation_id == "geometry.analytic_planar_boolean_batch.a0" &&
        attachment_name == "analytic_planar_boolean_request")
        return 268435456U;
    if (operation_id == "geometry.model_bounds.a0" && attachment_name == "model")
        return 268435456U;
    (void)operation_id;
    (void)attachment_name;
    return 0U;
}

const char* operation_input_attachment_primary_media_type(const std::string& operation_id,
                                                          const std::string& attachment_name)
{
    if (operation_id == "geometry.analytic_planar_boolean_batch.a0" &&
        attachment_name == "analytic_planar_boolean_request")
        return "application/vnd.wavenumber.geometer.analytic-planar-boolean-request";
    if (operation_id == "geometry.model_bounds.a0" && attachment_name == "model")
        return "application/step";
    (void)operation_id;
    (void)attachment_name;
    return nullptr;
}

std::size_t operation_output_attachment_max_bytes(const std::string& operation_id,
                                                  const std::string& attachment_name)
{
    if (operation_id == "geometry.analytic_planar_boolean_batch.a0" &&
        attachment_name == "analytic_planar_boolean_result")
        return 268435456U;
    (void)operation_id;
    (void)attachment_name;
    return 0U;
}

const char* operation_output_attachment_primary_media_type(const std::string& operation_id,
                                                           const std::string& attachment_name)
{
    if (operation_id == "geometry.analytic_planar_boolean_batch.a0" &&
        attachment_name == "analytic_planar_boolean_result")
        return "application/vnd.wavenumber.geometer.analytic-planar-boolean-result";
    (void)operation_id;
    (void)attachment_name;
    return nullptr;
}

const char* operation_request_contract(const std::string& operation_id)
{
    if (operation_id == "geometry.analytic_planar_boolean_batch.a0")
        return "geometry.analytic_planar_boolean_batch.request.a0";
    if (operation_id == "geometry.model_bounds.a0")
        return "geometry.model_bounds.options.a0";
    return nullptr;
}

bool operation_request_projection(const std::string& operation_id, const char** attachment_name,
                                  const char** format)
{
    if (attachment_name == nullptr || format == nullptr)
        return false;
    if (operation_id == "geometry.analytic_planar_boolean_batch.a0")
    {
        *attachment_name = "analytic_planar_boolean_request";
        *format = "geometry.analytic_planar_boolean.packet.a0";
        return true;
    }
    *attachment_name = nullptr;
    *format = nullptr;
    return false;
}

const char* operation_result_contract(const std::string& operation_id)
{
    if (operation_id == "geometry.analytic_planar_boolean_batch.a0")
        return "geometry.analytic_planar_boolean_batch.result.a0";
    if (operation_id == "geometry.model_bounds.a0")
        return "geometry.model_bounds.a0";
    return nullptr;
}

bool operation_result_projection(const std::string& operation_id, const char** attachment_name,
                                 const char** format)
{
    if (attachment_name == nullptr || format == nullptr)
        return false;
    if (operation_id == "geometry.analytic_planar_boolean_batch.a0")
    {
        *attachment_name = "analytic_planar_boolean_result";
        *format = "geometry.analytic_planar_boolean.packet.a0";
        return true;
    }
    *attachment_name = nullptr;
    *format = nullptr;
    return false;
}

bool operation_logical_result_matches(const std::string& operation_id,
                                      const contracts::OperationResultValueA0& result)
{
    if (operation_id == "geometry.model_bounds.a0")
        return std::holds_alternative<contracts::ModelBoundsResultA0>(result);
    (void)operation_id;
    (void)result;
    return false;
}

std::size_t operation_required_output_attachment_count(const std::string& operation_id)
{
    if (operation_id == "geometry.analytic_planar_boolean_batch.a0")
        return 1U;
    if (operation_id == "geometry.model_bounds.a0")
        return 0U;
    return 0;
}

const char* operation_required_output_attachment_name(const std::string& operation_id,
                                                      std::size_t index)
{
    if (operation_id == "geometry.analytic_planar_boolean_batch.a0" && index == 0U)
        return "analytic_planar_boolean_result";
    (void)operation_id;
    (void)index;
    return nullptr;
}

} // namespace geometer
