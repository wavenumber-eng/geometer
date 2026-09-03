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
        "analytic_planar_boolean.packet.a0\"}},{\"identity\":\"geometry.mesh_hlr_projection.a0\","
        "\"request_contract\":\"geometry.hlr_projection.options.a0\",\"result_contract\":"
        "\"geometry.hlr_projection.result.a0\",\"input_attachments\":[{\"name\":\"mesh\","
        "\"required\":true,\"media_types\":[\"application/"
        "vnd.wavenumber.geometer.indexed-triangle-mesh\"],\"max_bytes\":268435456}],\"output_"
        "attachments\":[],\"runtime_dispatch\":\"logical_dto\"},{\"identity\":\"geometry.model_"
        "bounds.a0\",\"request_contract\":\"geometry.model_bounds.options.a0\",\"result_contract\":"
        "\"geometry.model_bounds.a0\",\"input_attachments\":[{\"name\":\"model\",\"required\":true,"
        "\"media_types\":[\"application/step\",\"model/"
        "step\"],\"max_bytes\":268435456}],\"output_attachments\":[],\"runtime_dispatch\":"
        "\"logical_dto\"},{\"identity\":\"geometry.model_hlr_projection.a0\",\"request_contract\":"
        "\"geometry.hlr_projection.options.a0\",\"result_contract\":\"geometry.hlr_projection."
        "result.a0\",\"input_attachments\":[{\"name\":\"model\",\"required\":true,\"media_types\":["
        "\"application/step\",\"model/"
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

const char* native_operation_catalog_json()
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
        "analytic_planar_boolean.packet.a0\"}},{\"identity\":\"geometry.mesh_hlr_projection.a0\","
        "\"request_contract\":\"geometry.hlr_projection.options.a0\",\"result_contract\":"
        "\"geometry.hlr_projection.result.a0\",\"input_attachments\":[{\"name\":\"mesh\","
        "\"required\":true,\"media_types\":[\"application/"
        "vnd.wavenumber.geometer.indexed-triangle-mesh\"],\"max_bytes\":268435456}],\"output_"
        "attachments\":[],\"runtime_dispatch\":\"logical_dto\"},{\"identity\":\"geometry.model_"
        "bounds.a0\",\"request_contract\":\"geometry.model_bounds.options.a0\",\"result_contract\":"
        "\"geometry.model_bounds.a0\",\"input_attachments\":[{\"name\":\"model\",\"required\":true,"
        "\"media_types\":[\"application/step\",\"model/"
        "step\"],\"max_bytes\":268435456}],\"output_attachments\":[],\"runtime_dispatch\":"
        "\"logical_dto\"},{\"identity\":\"geometry.model_hlr_projection.a0\",\"request_contract\":"
        "\"geometry.hlr_projection.options.a0\",\"result_contract\":\"geometry.hlr_projection."
        "result.a0\",\"input_attachments\":[{\"name\":\"model\",\"required\":true,\"media_types\":["
        "\"application/step\",\"model/"
        "step\"],\"max_bytes\":268435456}],\"output_attachments\":[],\"runtime_dispatch\":"
        "\"logical_dto\"},{\"identity\":\"geometry.step_topology.apply_logical_groups.a0\","
        "\"request_contract\":\"geometry.step_topology.apply_logical_groups.request.a0\",\"result_"
        "contract\":\"geometry.step_topology.apply_logical_groups.result.a0\",\"input_"
        "attachments\":[],\"output_attachments\":[],\"runtime_dispatch\":\"logical_dto\"},{"
        "\"identity\":\"geometry.step_topology.apply_metadata_probes.a0\",\"request_contract\":"
        "\"geometry.step_topology.apply_metadata_probes.request.a0\",\"result_contract\":"
        "\"geometry.step_topology.apply_metadata_probes.result.a0\",\"input_attachments\":[],"
        "\"output_attachments\":[],\"runtime_dispatch\":\"logical_dto\"},{\"identity\":\"geometry."
        "step_topology.checkpoint_edit_journal.a0\",\"request_contract\":\"geometry.step_topology."
        "checkpoint_edit_journal.request.a0\",\"result_contract\":\"geometry.step_topology."
        "checkpoint_edit_journal.result.a0\",\"input_attachments\":[],\"output_attachments\":[{"
        "\"name\":\"edit_journal\",\"required\":true,\"media_types\":[\"application/"
        "vnd.wavenumber.geometer.step-topology-edit-journal\"],\"max_bytes\":67108864}],\"runtime_"
        "dispatch\":\"logical_dto\"},{\"identity\":\"geometry.step_topology.close.a0\",\"request_"
        "contract\":\"geometry.step_topology.close.request.a0\",\"result_contract\":\"geometry."
        "step_topology.close.result.a0\",\"input_attachments\":[],\"output_attachments\":[],"
        "\"runtime_dispatch\":\"logical_dto\"},{\"identity\":\"geometry.step_topology.inspect.a0\","
        "\"request_contract\":\"geometry.step_topology.inspect.request.a0\",\"result_contract\":"
        "\"geometry.step_topology.inspect.result.a0\",\"input_attachments\":[],\"output_"
        "attachments\":[{\"name\":\"topology_table\",\"required\":false,\"media_types\":["
        "\"application/"
        "vnd.wavenumber.geometer.step-topology-table\"],\"max_bytes\":134217728}],\"runtime_"
        "dispatch\":\"logical_dto\"},{\"identity\":\"geometry.step_topology.open.a0\",\"request_"
        "contract\":\"geometry.step_topology.open.request.a0\",\"result_contract\":\"geometry.step_"
        "topology.open.result.a0\",\"input_attachments\":[{\"name\":\"step\",\"required\":true,"
        "\"media_types\":[\"application/step\",\"model/"
        "step\"],\"max_bytes\":268435456}],\"output_attachments\":[],\"runtime_dispatch\":"
        "\"logical_dto\"},{\"identity\":\"geometry.step_topology.render.a0\",\"request_contract\":"
        "\"geometry.step_topology.render.request.a0\",\"result_contract\":\"geometry.step_topology."
        "render.result.a0\",\"input_attachments\":[],\"output_attachments\":[{\"name\":\"glb\","
        "\"required\":true,\"media_types\":[\"model/"
        "gltf-binary\"],\"max_bytes\":268435456},{\"name\":\"topology_binding_table\",\"required\":"
        "false,\"media_types\":[\"application/"
        "vnd.wavenumber.geometer.step-topology-binding-table\"],\"max_bytes\":134217728}],"
        "\"runtime_dispatch\":\"logical_dto\"},{\"identity\":\"geometry.step_topology.resolve_hit."
        "a0\",\"request_contract\":\"geometry.step_topology.resolve_hit.request.a0\",\"result_"
        "contract\":\"geometry.step_topology.resolve_hit.result.a0\",\"input_attachments\":[],"
        "\"output_attachments\":[],\"runtime_dispatch\":\"logical_dto\"},{\"identity\":\"geometry."
        "step_topology.restore.a0\",\"request_contract\":\"geometry.step_topology.restore.request."
        "a0\",\"result_contract\":\"geometry.step_topology.restore.result.a0\",\"input_"
        "attachments\":[{\"name\":\"source\",\"required\":true,\"media_types\":[\"application/"
        "step\",\"model/"
        "step\"],\"max_bytes\":268435456},{\"name\":\"state_artifact\",\"required\":true,\"media_"
        "types\":[\"application/"
        "vnd.wavenumber.geometer.step-topology-edit-journal\"],\"max_bytes\":67108864}],\"output_"
        "attachments\":[],\"runtime_dispatch\":\"logical_dto\"}],\"attachment_descriptor\":{"
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

const char* normalized_contract_catalog_sha256()
{
    return "3d610e74fa16618a12806607c55be6823d6bf5e9144095ed281179aaeda1415d";
}

bool operation_output_attachment_declared(const std::string& operation_id,
                                          const std::string& attachment_name,
                                          const std::string& media_type)
{
    if (operation_id == "geometry.analytic_planar_boolean_batch.a0" &&
        attachment_name == "analytic_planar_boolean_result" &&
        media_type == "application/vnd.wavenumber.geometer.analytic-planar-boolean-result")
        return true;
    if (operation_id == "geometry.step_topology.checkpoint_edit_journal.a0" &&
        attachment_name == "edit_journal" &&
        media_type == "application/vnd.wavenumber.geometer.step-topology-edit-journal")
        return true;
    if (operation_id == "geometry.step_topology.inspect.a0" &&
        attachment_name == "topology_table" &&
        media_type == "application/vnd.wavenumber.geometer.step-topology-table")
        return true;
    if (operation_id == "geometry.step_topology.render.a0" && attachment_name == "glb" &&
        media_type == "model/gltf-binary")
        return true;
    if (operation_id == "geometry.step_topology.render.a0" &&
        attachment_name == "topology_binding_table" &&
        media_type == "application/vnd.wavenumber.geometer.step-topology-binding-table")
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
    if (operation_id == "geometry.mesh_hlr_projection.a0" && attachment_name == "mesh" &&
        media_type == "application/vnd.wavenumber.geometer.indexed-triangle-mesh")
        return true;
    if (operation_id == "geometry.model_bounds.a0" && attachment_name == "model" &&
        media_type == "application/step")
        return true;
    if (operation_id == "geometry.model_bounds.a0" && attachment_name == "model" &&
        media_type == "model/step")
        return true;
    if (operation_id == "geometry.model_hlr_projection.a0" && attachment_name == "model" &&
        media_type == "application/step")
        return true;
    if (operation_id == "geometry.model_hlr_projection.a0" && attachment_name == "model" &&
        media_type == "model/step")
        return true;
    if (operation_id == "geometry.step_topology.open.a0" && attachment_name == "step" &&
        media_type == "application/step")
        return true;
    if (operation_id == "geometry.step_topology.open.a0" && attachment_name == "step" &&
        media_type == "model/step")
        return true;
    if (operation_id == "geometry.step_topology.restore.a0" && attachment_name == "source" &&
        media_type == "application/step")
        return true;
    if (operation_id == "geometry.step_topology.restore.a0" && attachment_name == "source" &&
        media_type == "model/step")
        return true;
    if (operation_id == "geometry.step_topology.restore.a0" &&
        attachment_name == "state_artifact" &&
        media_type == "application/vnd.wavenumber.geometer.step-topology-edit-journal")
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
    if (operation_id == "geometry.mesh_hlr_projection.a0" && attachment_name == "mesh")
        return 268435456U;
    if (operation_id == "geometry.model_bounds.a0" && attachment_name == "model")
        return 268435456U;
    if (operation_id == "geometry.model_hlr_projection.a0" && attachment_name == "model")
        return 268435456U;
    if (operation_id == "geometry.step_topology.open.a0" && attachment_name == "step")
        return 268435456U;
    if (operation_id == "geometry.step_topology.restore.a0" && attachment_name == "source")
        return 268435456U;
    if (operation_id == "geometry.step_topology.restore.a0" && attachment_name == "state_artifact")
        return 67108864U;
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
    if (operation_id == "geometry.mesh_hlr_projection.a0" && attachment_name == "mesh")
        return "application/vnd.wavenumber.geometer.indexed-triangle-mesh";
    if (operation_id == "geometry.model_bounds.a0" && attachment_name == "model")
        return "application/step";
    if (operation_id == "geometry.model_hlr_projection.a0" && attachment_name == "model")
        return "application/step";
    if (operation_id == "geometry.step_topology.open.a0" && attachment_name == "step")
        return "application/step";
    if (operation_id == "geometry.step_topology.restore.a0" && attachment_name == "source")
        return "application/step";
    if (operation_id == "geometry.step_topology.restore.a0" && attachment_name == "state_artifact")
        return "application/vnd.wavenumber.geometer.step-topology-edit-journal";
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
    if (operation_id == "geometry.step_topology.checkpoint_edit_journal.a0" &&
        attachment_name == "edit_journal")
        return 67108864U;
    if (operation_id == "geometry.step_topology.inspect.a0" && attachment_name == "topology_table")
        return 134217728U;
    if (operation_id == "geometry.step_topology.render.a0" && attachment_name == "glb")
        return 268435456U;
    if (operation_id == "geometry.step_topology.render.a0" &&
        attachment_name == "topology_binding_table")
        return 134217728U;
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
    if (operation_id == "geometry.step_topology.checkpoint_edit_journal.a0" &&
        attachment_name == "edit_journal")
        return "application/vnd.wavenumber.geometer.step-topology-edit-journal";
    if (operation_id == "geometry.step_topology.inspect.a0" && attachment_name == "topology_table")
        return "application/vnd.wavenumber.geometer.step-topology-table";
    if (operation_id == "geometry.step_topology.render.a0" && attachment_name == "glb")
        return "model/gltf-binary";
    if (operation_id == "geometry.step_topology.render.a0" &&
        attachment_name == "topology_binding_table")
        return "application/vnd.wavenumber.geometer.step-topology-binding-table";
    (void)operation_id;
    (void)attachment_name;
    return nullptr;
}

const char* operation_request_contract(const std::string& operation_id)
{
    if (operation_id == "geometry.analytic_planar_boolean_batch.a0")
        return "geometry.analytic_planar_boolean_batch.request.a0";
    if (operation_id == "geometry.mesh_hlr_projection.a0")
        return "geometry.hlr_projection.options.a0";
    if (operation_id == "geometry.model_bounds.a0")
        return "geometry.model_bounds.options.a0";
    if (operation_id == "geometry.model_hlr_projection.a0")
        return "geometry.hlr_projection.options.a0";
    if (operation_id == "geometry.step_topology.apply_logical_groups.a0")
        return "geometry.step_topology.apply_logical_groups.request.a0";
    if (operation_id == "geometry.step_topology.apply_metadata_probes.a0")
        return "geometry.step_topology.apply_metadata_probes.request.a0";
    if (operation_id == "geometry.step_topology.checkpoint_edit_journal.a0")
        return "geometry.step_topology.checkpoint_edit_journal.request.a0";
    if (operation_id == "geometry.step_topology.close.a0")
        return "geometry.step_topology.close.request.a0";
    if (operation_id == "geometry.step_topology.inspect.a0")
        return "geometry.step_topology.inspect.request.a0";
    if (operation_id == "geometry.step_topology.open.a0")
        return "geometry.step_topology.open.request.a0";
    if (operation_id == "geometry.step_topology.render.a0")
        return "geometry.step_topology.render.request.a0";
    if (operation_id == "geometry.step_topology.resolve_hit.a0")
        return "geometry.step_topology.resolve_hit.request.a0";
    if (operation_id == "geometry.step_topology.restore.a0")
        return "geometry.step_topology.restore.request.a0";
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
    if (operation_id == "geometry.mesh_hlr_projection.a0")
        return "geometry.hlr_projection.result.a0";
    if (operation_id == "geometry.model_bounds.a0")
        return "geometry.model_bounds.a0";
    if (operation_id == "geometry.model_hlr_projection.a0")
        return "geometry.hlr_projection.result.a0";
    if (operation_id == "geometry.step_topology.apply_logical_groups.a0")
        return "geometry.step_topology.apply_logical_groups.result.a0";
    if (operation_id == "geometry.step_topology.apply_metadata_probes.a0")
        return "geometry.step_topology.apply_metadata_probes.result.a0";
    if (operation_id == "geometry.step_topology.checkpoint_edit_journal.a0")
        return "geometry.step_topology.checkpoint_edit_journal.result.a0";
    if (operation_id == "geometry.step_topology.close.a0")
        return "geometry.step_topology.close.result.a0";
    if (operation_id == "geometry.step_topology.inspect.a0")
        return "geometry.step_topology.inspect.result.a0";
    if (operation_id == "geometry.step_topology.open.a0")
        return "geometry.step_topology.open.result.a0";
    if (operation_id == "geometry.step_topology.render.a0")
        return "geometry.step_topology.render.result.a0";
    if (operation_id == "geometry.step_topology.resolve_hit.a0")
        return "geometry.step_topology.resolve_hit.result.a0";
    if (operation_id == "geometry.step_topology.restore.a0")
        return "geometry.step_topology.restore.result.a0";
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
    if (operation_id == "geometry.mesh_hlr_projection.a0")
        return std::holds_alternative<contracts::HlrProjectionResultA0>(result);
    if (operation_id == "geometry.model_bounds.a0")
        return std::holds_alternative<contracts::ModelBoundsResultA0>(result);
    if (operation_id == "geometry.model_hlr_projection.a0")
        return std::holds_alternative<contracts::HlrProjectionResultA0>(result);
    if (operation_id == "geometry.step_topology.apply_logical_groups.a0")
        return std::holds_alternative<contracts::StepTopologyApplyLogicalGroupsResultA0>(result);
    if (operation_id == "geometry.step_topology.apply_metadata_probes.a0")
        return std::holds_alternative<contracts::StepTopologyApplyMetadataProbesResultA0>(result);
    if (operation_id == "geometry.step_topology.checkpoint_edit_journal.a0")
        return std::holds_alternative<contracts::StepTopologyCheckpointEditJournalResultA0>(result);
    if (operation_id == "geometry.step_topology.close.a0")
        return std::holds_alternative<contracts::StepTopologyCloseResultA0>(result);
    if (operation_id == "geometry.step_topology.inspect.a0")
        return std::holds_alternative<contracts::StepTopologyInspectResultA0>(result);
    if (operation_id == "geometry.step_topology.open.a0")
        return std::holds_alternative<contracts::StepTopologyOpenResultA0>(result);
    if (operation_id == "geometry.step_topology.render.a0")
        return std::holds_alternative<contracts::StepTopologyRenderResultA0>(result);
    if (operation_id == "geometry.step_topology.resolve_hit.a0")
        return std::holds_alternative<contracts::StepTopologyResolveHitResultA0>(result);
    if (operation_id == "geometry.step_topology.restore.a0")
        return std::holds_alternative<contracts::StepTopologyRestoreResultA0>(result);
    (void)operation_id;
    (void)result;
    return false;
}

bool operation_request_value_matches(const std::string& operation_id,
                                     const contracts::IpcRequestValueA0& request)
{
    if (operation_id == "geometry.analytic_planar_boolean_batch.a0")
        return std::holds_alternative<contracts::PackedAttachmentProjectionA0>(request);
    if (operation_id == "geometry.mesh_hlr_projection.a0")
        return std::holds_alternative<contracts::HlrProjectionOptionsA0>(request);
    if (operation_id == "geometry.model_bounds.a0")
        return std::holds_alternative<contracts::ModelBoundsOptionsA0>(request);
    if (operation_id == "geometry.model_hlr_projection.a0")
        return std::holds_alternative<contracts::HlrProjectionOptionsA0>(request);
    if (operation_id == "geometry.step_topology.analyze_recovery.a0")
        return std::holds_alternative<contracts::StepTopologyAnalyzeRecoveryRequestA0>(request);
    if (operation_id == "geometry.step_topology.apply_hierarchy.a0")
        return std::holds_alternative<contracts::StepTopologyApplyHierarchyRequestA0>(request);
    if (operation_id == "geometry.step_topology.apply_logical_groups.a0")
        return std::holds_alternative<contracts::StepTopologyApplyLogicalGroupsRequestA0>(request);
    if (operation_id == "geometry.step_topology.apply_metadata_probes.a0")
        return std::holds_alternative<contracts::StepTopologyApplyMetadataProbesRequestA0>(request);
    if (operation_id == "geometry.step_topology.checkpoint_edit_journal.a0")
        return std::holds_alternative<contracts::StepTopologyCheckpointEditJournalRequestA0>(
            request);
    if (operation_id == "geometry.step_topology.close.a0")
        return std::holds_alternative<contracts::StepTopologyCloseRequestA0>(request);
    if (operation_id == "geometry.step_topology.inspect.a0")
        return std::holds_alternative<contracts::StepTopologyInspectRequestA0>(request);
    if (operation_id == "geometry.step_topology.open.a0")
        return std::holds_alternative<contracts::StepTopologyOpenRequestA0>(request);
    if (operation_id == "geometry.step_topology.render.a0")
        return std::holds_alternative<contracts::StepTopologyRenderRequestA0>(request);
    if (operation_id == "geometry.step_topology.resolve_hit.a0")
        return std::holds_alternative<contracts::StepTopologyResolveHitRequestA0>(request);
    if (operation_id == "geometry.step_topology.restore.a0")
        return std::holds_alternative<contracts::StepTopologyRestoreRequestA0>(request);
    if (operation_id == "geometry.step_topology.save.a0")
        return std::holds_alternative<contracts::StepTopologySaveRequestA0>(request);
    (void)operation_id;
    (void)request;
    return false;
}

bool operation_result_value_matches(const std::string& operation_id,
                                    const contracts::OperationResultValueA0& result)
{
    if (operation_id == "geometry.analytic_planar_boolean_batch.a0")
        return std::holds_alternative<contracts::PackedAttachmentProjectionA0>(result);
    if (operation_id == "geometry.mesh_hlr_projection.a0")
        return std::holds_alternative<contracts::HlrProjectionResultA0>(result);
    if (operation_id == "geometry.model_bounds.a0")
        return std::holds_alternative<contracts::ModelBoundsResultA0>(result);
    if (operation_id == "geometry.model_hlr_projection.a0")
        return std::holds_alternative<contracts::HlrProjectionResultA0>(result);
    if (operation_id == "geometry.step_topology.analyze_recovery.a0")
        return std::holds_alternative<contracts::StepTopologyAnalyzeRecoveryResultA0>(result);
    if (operation_id == "geometry.step_topology.apply_hierarchy.a0")
        return std::holds_alternative<contracts::StepTopologyApplyHierarchyResultA0>(result);
    if (operation_id == "geometry.step_topology.apply_logical_groups.a0")
        return std::holds_alternative<contracts::StepTopologyApplyLogicalGroupsResultA0>(result);
    if (operation_id == "geometry.step_topology.apply_metadata_probes.a0")
        return std::holds_alternative<contracts::StepTopologyApplyMetadataProbesResultA0>(result);
    if (operation_id == "geometry.step_topology.checkpoint_edit_journal.a0")
        return std::holds_alternative<contracts::StepTopologyCheckpointEditJournalResultA0>(result);
    if (operation_id == "geometry.step_topology.close.a0")
        return std::holds_alternative<contracts::StepTopologyCloseResultA0>(result);
    if (operation_id == "geometry.step_topology.inspect.a0")
        return std::holds_alternative<contracts::StepTopologyInspectResultA0>(result);
    if (operation_id == "geometry.step_topology.open.a0")
        return std::holds_alternative<contracts::StepTopologyOpenResultA0>(result);
    if (operation_id == "geometry.step_topology.render.a0")
        return std::holds_alternative<contracts::StepTopologyRenderResultA0>(result);
    if (operation_id == "geometry.step_topology.resolve_hit.a0")
        return std::holds_alternative<contracts::StepTopologyResolveHitResultA0>(result);
    if (operation_id == "geometry.step_topology.restore.a0")
        return std::holds_alternative<contracts::StepTopologyRestoreResultA0>(result);
    if (operation_id == "geometry.step_topology.save.a0")
        return std::holds_alternative<contracts::StepTopologySaveResultA0>(result);
    (void)operation_id;
    (void)result;
    return false;
}

std::size_t operation_required_output_attachment_count(const std::string& operation_id)
{
    if (operation_id == "geometry.analytic_planar_boolean_batch.a0")
        return 1U;
    if (operation_id == "geometry.mesh_hlr_projection.a0")
        return 0U;
    if (operation_id == "geometry.model_bounds.a0")
        return 0U;
    if (operation_id == "geometry.model_hlr_projection.a0")
        return 0U;
    if (operation_id == "geometry.step_topology.apply_logical_groups.a0")
        return 0U;
    if (operation_id == "geometry.step_topology.apply_metadata_probes.a0")
        return 0U;
    if (operation_id == "geometry.step_topology.checkpoint_edit_journal.a0")
        return 1U;
    if (operation_id == "geometry.step_topology.close.a0")
        return 0U;
    if (operation_id == "geometry.step_topology.inspect.a0")
        return 0U;
    if (operation_id == "geometry.step_topology.open.a0")
        return 0U;
    if (operation_id == "geometry.step_topology.render.a0")
        return 1U;
    if (operation_id == "geometry.step_topology.resolve_hit.a0")
        return 0U;
    if (operation_id == "geometry.step_topology.restore.a0")
        return 0U;
    return 0;
}

const char* operation_required_output_attachment_name(const std::string& operation_id,
                                                      std::size_t index)
{
    if (operation_id == "geometry.analytic_planar_boolean_batch.a0" && index == 0U)
        return "analytic_planar_boolean_result";
    if (operation_id == "geometry.step_topology.checkpoint_edit_journal.a0" && index == 0U)
        return "edit_journal";
    if (operation_id == "geometry.step_topology.render.a0" && index == 0U)
        return "glb";
    (void)operation_id;
    (void)index;
    return nullptr;
}

} // namespace geometer
