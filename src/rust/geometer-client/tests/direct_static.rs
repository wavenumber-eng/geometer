#![cfg(feature = "direct-static")]

use std::path::PathBuf;

use geometer_client::contracts::{
    MeshIllustrationView, ModelAttachmentIllustrationSourceA0, ModelIllustrationGeometryRequestB0,
    ModelIllustrationSourceA0, OperationOutcomeA0,
};
use geometer_client::ipc::Attachment;
use geometer_client::{GeometerDirectClient, OperationOutcome};

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
                data: model.clone(),
            }],
        )
        .await
        .expect("direct model bounds");
    assert!(matches!(
        response.outcome,
        OperationOutcome::A0(OperationOutcomeA0::Success(_))
    ));
    assert!(response.attachments.is_empty());

    let illustration = client
        .model_illustration_geometry(
            ModelIllustrationGeometryRequestB0 {
                schema: "geometry.model_illustration_geometry.request.b0".to_owned(),
                source: ModelIllustrationSourceA0::ModelSource(
                    ModelAttachmentIllustrationSourceA0 {
                        kind: "model".to_owned(),
                        attachment: "model".to_owned(),
                        transform: None,
                        material_override: None,
                        tessellation: None,
                    },
                ),
                view: MeshIllustrationView {
                    direction: [0.0, 0.0, 1.0],
                    up: [0.0, 1.0, 0.0],
                    mirror_x: None,
                },
                prepare: None,
                linework: None,
                style: None,
                work_limits: None,
                clipping: None,
            },
            Some(model),
        )
        .await
        .expect("direct model illustration geometry");
    assert!(!illustration.geometry.surfaces.is_empty());
}
