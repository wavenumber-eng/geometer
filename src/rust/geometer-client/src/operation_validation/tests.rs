use super::*;
use crate::generated::operations::expected_operation_catalog;

#[test]
fn logical_response_must_match_the_negotiated_contract() {
    let welcome = IpcWelcomeA0 {
        release_version: "test".to_owned(),
        c_abi_generation: 1,
        ipc: "a0".to_owned(),
        catalog_sha256: crate::NORMALIZED_CATALOG_SHA256.to_owned(),
        operation_catalog: expected_operation_catalog("test", 1),
        limits: contracts::IpcEffectiveLimitsA0 {
            json_bytes: 1,
            attachment_count: 1,
            attachment_name_bytes: 1,
            attachment_media_type_bytes: 1,
            attachment_bytes: 1,
            frame_bytes: 1,
            queued_requests: 1,
            queued_bytes: 1,
            resident_request_bytes: 1,
            pending_writer_bytes: 1,
        },
        capabilities: Vec::new(),
    };
    let result = OperationResultValueA0::StepTopologyClose(contracts::StepTopologyCloseResultA0 {
        schema: "geometry.step_topology.close.result.a0".to_owned(),
        session_handle: "s".repeat(68),
        closed: true,
    });
    let outcome = OperationOutcomeA0::Success(contracts::OperationSuccessA0 {
        operation: "geometry.step_topology.close.a0".to_owned(),
        ok: true,
        result,
    });
    assert!(
        validate_operation_response(&welcome, "geometry.step_topology.close.a0", &outcome, &[])
            .is_ok()
    );
    assert!(matches!(
        validate_operation_response(&welcome, "geometry.model_bounds.a0", &outcome, &[]),
        Err(GeometerClientError::Protocol(_))
    ));
}
