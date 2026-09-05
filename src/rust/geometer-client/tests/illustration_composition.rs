//! Executable composition against the browser's accepted createIllustrator policy.
use geometer_client::{
    GeometerClient, GeometerClientError, ModelHlrProjectionRequest, ModelTessellationRequest,
    contracts::*,
};
use std::path::{Path, PathBuf};

fn root() -> PathBuf {
    Path::new(env!("CARGO_MANIFEST_DIR")).join("../../..")
}

fn reference(
    input: &MeshIllustrationInputA0,
    hlr: &HlrProjectionResultA0,
) -> MeshIllustrationResultA0 {
    let mut child = std::process::Command::new("node")
        .current_dir(root())
        .arg("tests/typescript/native_illustration_reference.mjs")
        .stdin(std::process::Stdio::piped())
        .stdout(std::process::Stdio::piped())
        .spawn()
        .unwrap();
    std::io::Write::write_all(
        &mut child.stdin.take().unwrap(),
        &serde_json::to_vec(&serde_json::json!({"input": input, "hlr": hlr})).unwrap(),
    )
    .unwrap();
    let output = child.wait_with_output().unwrap();
    assert!(output.status.success());
    decode_mesh_illustration_result_a0_json(&output.stdout).unwrap()
}

async fn projection(
    client: &GeometerClient,
    step: &[u8],
    view: &MeshIllustrationView,
) -> HlrProjectionResultA0 {
    let mut options = decode_hlr_projection_options_a0_json(
        br#"{
        "projection_algorithm":"fast", "outline_algorithm":"fast-mesh-shadow",
        "curve_mode":"polyline", "strip_root_placement":true,
        "output_outline":true, "output_detail":true, "output_bbox":false,
        "edge_v_sharp":true, "edge_v_outline":true,
        "edge_v_smooth":false, "edge_v_sewn":false, "edge_v_iso":false,
        "edge_h_sharp":false, "edge_h_outline":false, "edge_h_smooth":false,
        "edge_h_sewn":false, "edge_h_iso":false,
        "fast":{"include_hidden":false,"crease_angle_rad":0.4363323129985824}
    }"#,
    )
    .unwrap();
    options.views = Some(vec![HlrViewSpec {
        id: "preview".into(),
        direction: view.direction,
        up: view.up,
    }]);
    client
        .model_hlr_projection(ModelHlrProjectionRequest {
            model: step.to_vec(),
            media_type: "application/step".into(),
            options,
        })
        .await
        .unwrap()
}

#[tokio::test]
async fn native_composition_matches_browser_for_views_mirror_and_line_toggles() {
    let executable = GeometerClient::find_executable().unwrap();
    let client = GeometerClient::spawn(executable, "composition-test", "a0")
        .await
        .unwrap();
    let step =
        std::fs::read(root().join("tests/fixtures/step/embedded_models/SOT-23.STEP")).unwrap();
    let meshes = client
        .model_tessellation(ModelTessellationRequest::step(step.clone()))
        .await
        .unwrap()
        .mesh_collection
        .meshes;
    for direction in [[0.4, 0.7, 1.0], [0.0, 0.0, 1.0], [-1.0, -0.4, 0.7]] {
        let mut input = MeshIllustrationInputA0 {
            schema: "geometry.mesh_illustration.input.a0".into(),
            meshes: meshes.clone(),
            view: MeshIllustrationView {
                direction,
                up: [0.0, 1.0, 0.0],
                mirror_x: None,
            },
            style: Some(
                decode_mesh_illustration_style_a0_json(
                    br##"{
                "show_outlines":false,"show_creases":false,
                "outline_color":"#17252c","crease_color":"#ab3421",
                "outline_width":0.006,"crease_width":0.0033
            }"##,
                )
                .unwrap(),
            ),
            prepare: None,
            svg: None,
        };
        let hlr = projection(&client, &step, &input.view).await;
        assert!(!hlr.views[0].modes.outline.segments.is_empty());
        assert!(!hlr.views[0].modes.detail.segments.is_empty());
        for mirror in [false, true] {
            input.view.mirror_x = Some(mirror);
            for (outline, detail) in [(false, false), (true, false), (false, true), (true, true)] {
                let style = input.style.as_mut().unwrap();
                style.show_hlr_outline = Some(outline);
                style.show_hlr_detail = Some(detail);
                let result = client
                    .mesh_illustration_with_hlr(input.clone(), hlr.clone())
                    .await
                    .unwrap();
                assert_eq!(
                    result,
                    reference(&input, &hlr),
                    "view={direction:?} mirror={mirror} outline={outline} detail={detail}"
                );
                assert_eq!(
                    result.stats.outlines as usize,
                    if outline {
                        hlr.views[0].modes.outline.segments.len()
                    } else {
                        0
                    }
                );
                assert_eq!(
                    result.stats.details as usize,
                    if detail {
                        hlr.views[0].modes.detail.segments.len()
                    } else {
                        0
                    }
                );
                assert_eq!(result.stats.creases, 0);
                assert_eq!(
                    result,
                    client
                        .mesh_illustration_with_hlr(input.clone(), hlr.clone())
                        .await
                        .unwrap()
                );
                if !outline && !detail {
                    assert_eq!(
                        result,
                        client.mesh_illustration(input.clone()).await.unwrap()
                    );
                }
            }
        }
        rejects_mismatches(&client, &input, &hlr).await;
    }
    client.close().await.unwrap();
}

async fn rejects_mismatches(
    client: &GeometerClient,
    input: &MeshIllustrationInputA0,
    hlr: &HlrProjectionResultA0,
) {
    let mut wrong = hlr.clone();
    wrong.views[0].direction = [1.0, 0.0, 0.0];
    assert!(matches!(
        client
            .mesh_illustration_with_hlr(input.clone(), wrong)
            .await,
        Err(GeometerClientError::Operation { .. })
    ));
    let mut wrong = hlr.clone();
    wrong.views.push(wrong.views[0].clone());
    assert!(matches!(
        client
            .mesh_illustration_with_hlr(input.clone(), wrong)
            .await,
        Err(GeometerClientError::Operation { .. })
    ));
    let mut wrong = hlr.clone();
    wrong.views[0].modes.detail.arcs.push(ProjectedArc {
        start: [0.0, 0.0],
        end: [1.0, 1.0],
        center: [0.0, 1.0],
        radius: 1.0,
        extent_rad: 1.57,
        ccw: true,
        full_circle: false,
    });
    assert!(matches!(
        client
            .mesh_illustration_with_hlr(input.clone(), wrong)
            .await,
        Err(GeometerClientError::Operation { .. })
    ));
    let mut wrong = hlr.clone();
    wrong.units = "meters".into();
    assert!(matches!(
        client
            .mesh_illustration_with_hlr(input.clone(), wrong)
            .await,
        Err(GeometerClientError::Contract(_))
    ));
    assert!(
        client
            .mesh_illustration_with_hlr(input.clone(), hlr.clone())
            .await
            .is_ok()
    );
}
