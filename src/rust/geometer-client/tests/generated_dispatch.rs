use geometer_client::contracts::{self, IpcRequestValueA0, OperationResultValueA0};
use geometer_client::generated::dispatch::{
    decode_logical_request, logical_request_contract, logical_result_contract,
};

#[test]
fn negotiated_contract_disambiguates_empty_option_objects() {
    for identity in [
        "geometry.model_bounds.options.a0",
        "geometry.hlr_projection.options.a0",
    ] {
        let request = decode_logical_request(identity, b"{}").unwrap();
        assert_eq!(logical_request_contract(&request), Some(identity));
    }
}

#[test]
fn generated_dispatch_preserves_topology_identity_and_rejects_wrong_shapes() {
    let identity = "geometry.step_topology.open.request.a0";
    let request = decode_logical_request(
        identity,
        br#"{"schema":"geometry.step_topology.open.request.a0"}"#,
    )
    .unwrap();
    assert!(matches!(request, IpcRequestValueA0::StepTopologyOpen(_)));
    assert_eq!(logical_request_contract(&request), Some(identity));
    assert!(decode_logical_request(identity, b"{}").is_err());
    assert!(decode_logical_request("geometry.unknown.request.a0", b"{}").is_err());
    assert!(
        decode_logical_request(
            identity,
            br#"{"schema":"geometry.step_topology.open.request.a0","extra":true}"#
        )
        .is_err()
    );
}

#[test]
fn generated_result_mapping_does_not_invent_a_packed_contract() {
    let result = OperationResultValueA0::StepTopologyClose(contracts::StepTopologyCloseResultA0 {
        schema: "geometry.step_topology.close.result.a0".to_owned(),
        session_handle: "s".repeat(68),
        closed: true,
    });
    assert_eq!(
        logical_result_contract(&result),
        Some("geometry.step_topology.close.result.a0")
    );
    let packed = contracts::decode_json::<contracts::PackedAttachmentProjectionA0>(
        br#"{"schema":"geometry.analytic_planar_boolean_batch.request.a0","packet":{"attachment":"analytic_planar_boolean_request","format":"geometry.analytic_planar_boolean.packet.a0"}}"#,
    ).unwrap();
    assert_eq!(
        logical_request_contract(&IpcRequestValueA0::PackedAttachment(packed.clone())),
        None
    );
    assert_eq!(
        logical_result_contract(&OperationResultValueA0::PackedAttachment(packed)),
        None
    );
}
