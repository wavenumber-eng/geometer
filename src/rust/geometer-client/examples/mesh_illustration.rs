//! cargo run --example mesh_illustration -- GEOMETER_EXE INPUT.step OUTPUT.svg
use geometer_client::GeometerClient;
use geometer_client::contracts::*;

#[tokio::main]
async fn main() -> Result<(), Box<dyn std::error::Error>> {
    let args: Vec<_> = std::env::args_os().skip(1).collect();
    if args.len() != 3 {
        return Err("usage: mesh_illustration GEOMETER_EXE INPUT.step OUTPUT.svg".into());
    }
    let client = GeometerClient::spawn(&args[0], "native-illustration-example", "a0").await?;
    let step = std::fs::read(&args[1])?;
    let view = MeshIllustrationView {
        direction: [0.4, 0.7, 1.0],
        up: [0.0, 1.0, 0.0],
        mirror_x: None,
    };
    let style = decode_mesh_illustration_style_a0_json(
        br#"{
        "show_outlines":false,"show_creases":false,
        "show_hlr_outline":true,"show_hlr_detail":true
    }"#,
    )?;
    let result = client
        .model_illustration(
            ModelIllustrationRequestB0 {
                schema: "geometry.model_illustration.request.b0".to_owned(),
                source: ModelIllustrationSourceA0::ModelSource(
                    ModelAttachmentIllustrationSourceA0 {
                        kind: "model".to_owned(),
                        attachment: "model".to_owned(),
                        transform: None,
                        material_override: None,
                        tessellation: None,
                    },
                ),
                view,
                prepare: None,
                linework: Some(ModelIllustrationLineworkOptionsA0 {
                    fast: Some(FastHlrOptionsA0 {
                        include_boundaries: None,
                        include_creases: None,
                        include_silhouettes: None,
                        include_hidden: None,
                        suppress_coplanar_seams: None,
                        crease_angle_rad: Some(0.4363323129985824),
                        weld_tolerance: None,
                        projected_tolerance: None,
                        depth_tolerance: None,
                        coplanar_seam_angle_rad: None,
                        coplanar_seam_depth_tolerance: None,
                        coplanar_seam_lateral_tolerance: None,
                        limits: None,
                    }),
                    outline_width_mm: None,
                    detail_width_mm: None,
                }),
                style: Some(style),
                svg: None,
                work_limits: None,
                clipping: None,
            },
            Some(step),
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
