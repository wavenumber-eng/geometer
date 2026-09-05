//! cargo run --example mesh_illustration -- GEOMETER_EXE INPUT.step OUTPUT.svg
use geometer_client::contracts::*;
use geometer_client::{GeometerClient, ModelHlrProjectionRequest, ModelTessellationRequest};

#[tokio::main]
async fn main() -> Result<(), Box<dyn std::error::Error>> {
    let args: Vec<_> = std::env::args_os().skip(1).collect();
    if args.len() != 3 {
        return Err("usage: mesh_illustration GEOMETER_EXE INPUT.step OUTPUT.svg".into());
    }
    let client = GeometerClient::spawn(&args[0], "native-illustration-example", "a0").await?;
    let step = std::fs::read(&args[1])?;
    let meshes = client
        .model_tessellation(ModelTessellationRequest::step(step.clone()))
        .await?;
    let view = MeshIllustrationView {
        direction: [0.4, 0.7, 1.0],
        up: [0.0, 1.0, 0.0],
        mirror_x: None,
    };
    let mut options = decode_hlr_projection_options_a0_json(
        br#"{
        "projection_algorithm":"fast", "outline_algorithm":"fast-mesh-shadow",
        "curve_mode":"polyline", "strip_root_placement":true,
        "output_outline":true, "output_detail":true, "output_bbox":false,
        "fast":{"include_hidden":false,"crease_angle_rad":0.4363323129985824}
    }"#,
    )?;
    options.views = Some(vec![HlrViewSpec {
        id: "illustration".into(),
        direction: view.direction,
        up: view.up,
    }]);
    let hlr = client
        .model_hlr_projection(ModelHlrProjectionRequest {
            model: step,
            media_type: "application/step".into(),
            options,
        })
        .await?;
    let style = decode_mesh_illustration_style_a0_json(
        br#"{
        "show_outlines":false,"show_creases":false,
        "show_hlr_outline":true,"show_hlr_detail":true
    }"#,
    )?;
    let result = client
        .mesh_illustration_with_hlr(
            MeshIllustrationInputA0 {
                schema: "geometry.mesh_illustration.input.a0".to_owned(),
                meshes: meshes.mesh_collection.meshes,
                view,
                prepare: None,
                style: Some(style),
                svg: None,
            },
            hlr,
        )
        .await?;
    client.close().await?;
    std::fs::write(&args[2], &result.svg)?;
    println!(
        "{} triangles; {} surface draws",
        result.stats.triangles, result.stats.surface_draws
    );
    Ok(())
}
