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
    return "23fd3fdd95b69ff0d7393df79ba3917556e0bb3b50bbc1893468b339913b84c1";
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
