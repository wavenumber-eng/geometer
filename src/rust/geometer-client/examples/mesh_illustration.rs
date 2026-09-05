//! cargo run --example mesh_illustration -- GEOMETER_EXE INPUT.step OUTPUT.svg
use geometer_client::contracts::{MeshIllustrationInputA0, MeshIllustrationView};
use geometer_client::{GeometerClient, ModelTessellationRequest};

#[tokio::main]
async fn main() -> Result<(), Box<dyn std::error::Error>> {
    let args: Vec<_> = std::env::args_os().skip(1).collect();
    if args.len() != 3 {
        return Err("usage: mesh_illustration GEOMETER_EXE INPUT.step OUTPUT.svg".into());
    }
    let client = GeometerClient::spawn(&args[0], "native-illustration-example", "a0").await?;
    let meshes = client
        .model_tessellation(ModelTessellationRequest::step(std::fs::read(&args[1])?))
        .await?;
    let result = client
        .mesh_illustration(MeshIllustrationInputA0 {
            schema: "geometry.mesh_illustration.input.a0".to_owned(),
            meshes: meshes.mesh_collection.meshes,
            view: MeshIllustrationView {
                direction: [0.4, 0.7, 1.0],
                up: [0.0, 1.0, 0.0],
                mirror_x: None,
            },
            prepare: None,
            style: None,
            svg: None,
        })
        .await?;
    client.close().await?;
    std::fs::write(&args[2], &result.svg)?;
    println!(
        "{} triangles; {} surface draws",
        result.stats.triangles, result.stats.surface_draws
    );
    Ok(())
}
