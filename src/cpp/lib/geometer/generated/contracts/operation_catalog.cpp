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
        ",\"operations\":[{\"identity\":\"geometry.model_bounds.a0\",\"request_contract\":"
        "\"geometry.model_bounds.options.a0\",\"result_contract\":\"geometry.model_bounds.a0\","
        "\"input_attachments\":[{\"name\":\"model\",\"required\":true,\"media_types\":["
        "\"application/step\",\"model/"
        "step\"],\"max_bytes\":268435456}],\"output_attachments\":[]}],\"attachment_descriptor\":{"
        "\"wasm32\":{\"size\":36,\"offsets\":{\"struct_size\":0,\"flags\":4,\"name\":8,\"name_"
        "size\":12,\"media_type\":16,\"media_type_size\":20,\"data\":24,\"data_size\":28,"
        "\"reserved0\":32}},\"pointer64\":{\"size\":56,\"offsets\":{\"struct_size\":0,\"flags\":4,"
        "\"name\":8,\"name_size\":16,\"media_type\":24,\"media_type_size\":32,\"data\":40,\"data_"
        "size\":48,\"reserved0\":52}}},\"limits\":{\"operation_id_bytes\":128,\"request_json_"
        "bytes\":8388608,\"response_json_bytes\":8388608,\"attachment_count\":16,\"attachment_name_"
        "bytes\":128,\"attachment_media_type_bytes\":128,\"attachment_bytes\":268435456,"
        "\"aggregate_attachment_bytes_native\":536870912,\"aggregate_attachment_bytes_wasm\":"
        "268435456}}";
    return catalog.c_str();
}

bool operation_output_attachment_declared(const std::string& operation_id,
                                          const std::string& attachment_name,
                                          const std::string& media_type)
{
    (void)operation_id;
    (void)attachment_name;
    (void)media_type;
    return false;
}

} // namespace geometer
