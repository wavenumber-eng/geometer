// Generated from wn_geometer_contract_catalog.a0.json. Do not edit.

use super::contracts::{self, IpcOperationCatalogA0};

pub const ANALYTIC_PLANAR_BOOLEAN_BATCH_A0_IDENTITY: &str =
    "geometry.analytic_planar_boolean_batch.a0";

const OPERATION_CATALOG_TEMPLATE: &str = r#"{"catalog":"wn.geometer.operation_catalog.a0","generic_abi":"a0","release_version":"","c_abi_generation":0,"operations":[{"identity":"geometry.analytic_planar_boolean_batch.a0","request_contract":"geometry.analytic_planar_boolean_batch.request.a0","result_contract":"geometry.analytic_planar_boolean_batch.result.a0","input_attachments":[{"name":"analytic_planar_boolean_request","required":true,"media_types":["application/vnd.wavenumber.geometer.analytic-planar-boolean-request"],"max_bytes":268435456}],"output_attachments":[{"name":"analytic_planar_boolean_result","required":true,"media_types":["application/vnd.wavenumber.geometer.analytic-planar-boolean-result"],"max_bytes":268435456}],"runtime_dispatch":"packed_attachment","request_projection":{"kind":"packed_attachment","attachment_name":"analytic_planar_boolean_request","format":"geometry.analytic_planar_boolean.packet.a0"},"result_projection":{"kind":"packed_attachment","attachment_name":"analytic_planar_boolean_result","format":"geometry.analytic_planar_boolean.packet.a0"}},{"identity":"geometry.model_bounds.a0","request_contract":"geometry.model_bounds.options.a0","result_contract":"geometry.model_bounds.a0","input_attachments":[{"name":"model","required":true,"media_types":["application/step","model/step"],"max_bytes":268435456}],"output_attachments":[],"runtime_dispatch":"logical_dto"}],"attachment_descriptor":{"wasm32":{"size":36,"offsets":{"struct_size":0,"flags":4,"name":8,"name_size":12,"media_type":16,"media_type_size":20,"data":24,"data_size":28,"reserved0":32}},"pointer64":{"size":56,"offsets":{"struct_size":0,"flags":4,"name":8,"name_size":16,"media_type":24,"media_type_size":32,"data":40,"data_size":48,"reserved0":52}}},"limits":{"operation_id_bytes":128,"request_json_bytes":8388608,"response_json_bytes":8388608,"attachment_count":16,"attachment_name_bytes":128,"attachment_media_type_bytes":128,"attachment_bytes":268435456,"aggregate_attachment_bytes_native":536870912,"aggregate_attachment_bytes_wasm":268435456}}"#;

pub fn expected_operation_catalog(
    release_version: &str,
    c_abi_generation: u32,
) -> IpcOperationCatalogA0 {
    let mut value: serde_json::Value = serde_json::from_str(OPERATION_CATALOG_TEMPLATE)
        .expect("generated operation catalog template must be valid JSON");
    value["release_version"] = serde_json::Value::String(release_version.to_owned());
    value["c_abi_generation"] = serde_json::Value::Number(c_abi_generation.into());
    let bytes =
        serde_json::to_vec(&value).expect("generated operation catalog value must serialize");
    contracts::decode_ipc_operation_catalog_a0_json(&bytes)
        .expect("generated operation catalog must satisfy its contract")
}
