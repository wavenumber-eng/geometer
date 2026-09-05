//! Focused opt-in native checks for the Lab control mapping and visibility diagnosis.
use geometer_client::{
    GeometerClient, ModelHlrProjectionRequest, ModelTessellationRequest, contracts::*,
};

#[test]
#[ignore = "requires matching GEOMETER_EXECUTABLE; run through opt-in RUST_002 GPU lane"]
fn native_lab_settings_and_raw_overlay_diagnosis() {
    let root = std::path::Path::new(env!("CARGO_MANIFEST_DIR")).join("../../..");
    let step = std::fs::read(root.join("tests/fixtures/step/embedded_models/SOT-23.STEP")).unwrap();
    let runtime = tokio::runtime::Runtime::new().unwrap();
    runtime.block_on(async {
        let client = GeometerClient::spawn(
            std::env::var_os("GEOMETER_EXECUTABLE").expect("native executable"),
            "lab-settings-test",
            "a0",
        )
        .await
        .unwrap();
        let mut request = ModelTessellationRequest {
            model: step.clone(),
            options: crate::settings::mesh_defaults(),
        };
        let balanced = client.model_tessellation(request.clone()).await.unwrap();
        request.options.linear_deflection_mm = Some(0.01);
        request.options.angular_deflection_rad = Some(8.0_f64.to_radians());
        let fine = client.model_tessellation(request).await.unwrap();
        assert!(
            fine.metadata.triangles > balanced.metadata.triangles,
            "mesh controls must reach the native tessellator"
        );

        let view = crate::camera::Camera::default().view();
        let mut input = MeshIllustrationInputA0 {
            schema: "geometry.mesh_illustration.input.a0".into(),
            meshes: balanced.mesh_collection.meshes,
            view: view.clone(),
            prepare: None,
            svg: None,
            style: Some(crate::settings::lab_style()),
        };
        let clean = client.mesh_illustration(input.clone()).await.unwrap();
        assert_eq!(clean.stats.outlines + clean.stats.creases, 0);
        assert!(clean.stats.surface_draws > 0);

        // Same generated input through the TS core: renderer parity alone is not Lab composition parity.
        let mut oracle = std::process::Command::new("node")
            .current_dir(&root)
            .arg("tests/typescript/native_illustration_reference.mjs")
            .stdin(std::process::Stdio::piped())
            .stdout(std::process::Stdio::piped())
            .spawn()
            .unwrap();
        std::io::Write::write_all(
            &mut oracle.stdin.take().unwrap(),
            &encode_mesh_illustration_input_a0_json(&input).unwrap(),
        )
        .unwrap();
        let output = oracle.wait_with_output().unwrap();
        assert!(output.status.success());
        let reference = decode_mesh_illustration_result_a0_json(&output.stdout).unwrap();
        assert_eq!(clean, reference);

        let style = input.style.as_mut().unwrap();
        style.show_outlines = Some(true);
        style.show_creases = Some(true);
        let raw = client.mesh_illustration(input).await.unwrap();
        assert!(raw.stats.outlines + raw.stats.creases > 0);
        assert_ne!(clean.svg, raw.svg);

        let mut options = crate::hlr::options(&view);
        options.fast.as_mut().unwrap().crease_angle_rad = Some(1.0_f64.to_radians());
        let sharp = client
            .model_hlr_projection(ModelHlrProjectionRequest {
                model: step.clone(),
                media_type: "application/step".into(),
                options: options.clone(),
            })
            .await
            .unwrap();
        options.fast.as_mut().unwrap().crease_angle_rad = Some(80.0_f64.to_radians());
        let smooth = client
            .model_hlr_projection(ModelHlrProjectionRequest {
                model: step.clone(),
                media_type: "application/step".into(),
                options: options.clone(),
            })
            .await
            .unwrap();
        assert_ne!(
            sharp.views[0].modes.detail.segments,
            smooth.views[0].modes.detail.segments
        );
        options.output_outline = Some(false);
        options.output_detail = Some(false);
        let off = client
            .model_hlr_projection(ModelHlrProjectionRequest {
                model: step,
                media_type: "application/step".into(),
                options,
            })
            .await
            .unwrap();
        assert!(off.views[0].modes.outline.segments.is_empty());
        assert!(off.views[0].modes.detail.segments.is_empty());
        let images = crate::hlr::images(&off, &crate::settings::lab_style()).unwrap();
        assert!(
            images[0]
                .pixels
                .iter()
                .all(|pixel| *pixel == eframe::egui::Color32::WHITE)
        );
        client.close().await.unwrap();
    });
}
