#![cfg(feature = "direct-static")]

use std::path::PathBuf;

use geometer_client::GeometerDirectClient;
use geometer_client::contracts::OperationOutcomeA0;
use geometer_client::ipc::Attachment;

#[tokio::test]
async fn direct_static_backend_executes_the_generated_model_bounds_contract() {
    let client = GeometerDirectClient::new().expect("direct client");
    assert!(
        client
            .operation_catalog()
            .operations
            .iter()
            .any(|operation| operation.identity == "geometry.model_bounds.a0")
    );
    let model = std::fs::read(
        PathBuf::from(env!("CARGO_MANIFEST_DIR"))
            .join("../../../tests/fixtures/step/embedded_models/SOT-23.STEP"),
    )
    .expect("STEP fixture");
    let response = client
        .execute(
            "geometry.model_bounds.a0",
            b"{}",
            vec![Attachment {
                name: "model".to_owned(),
                media_type: "application/step".to_owned(),
                data: model,
            }],
        )
        .await
        .expect("direct model bounds");
    assert!(matches!(response.outcome, OperationOutcomeA0::Success(_)));
    assert!(response.attachments.is_empty());
}
