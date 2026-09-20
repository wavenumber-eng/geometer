//! Relocated static-SDK clipping demo with no executable sidecar.
//!
//! cargo run --features direct-static --example direct_static_illustration -- \
//!   --clip --output-dir out/clip-demo
use std::ffi::OsString;
use std::path::PathBuf;

use geometer_client::GeometerDirectClient;
use geometer_client::contracts::*;

struct Options {
    step: Option<PathBuf>,
    output_dir: PathBuf,
    clip: bool,
    normal: [f64; 3],
    distance_mm: f64,
    tolerance_mm: f64,
}

#[tokio::main]
async fn main() -> Result<(), Box<dyn std::error::Error>> {
    let options = parse_args(std::env::args_os().skip(1))?;
    std::fs::create_dir_all(&options.output_dir)?;
    let (source, model) = source(&options)?;
    let client = GeometerDirectClient::new()?;
    let unclipped = client
        .model_illustration(request(source.clone(), None), model.clone())
        .await?;
    let clipping = options.clip.then(|| IllustrationClipping {
        planes: vec![HalfSpacePlane {
            normal: options.normal,
            distance_mm: options.distance_mm,
            tolerance_mm: Some(options.tolerance_mm),
        }],
        cap_policy: "none".to_owned(),
        limits: None,
    });
    let clipped = client
        .model_illustration(request(source, clipping), model)
        .await?;

    let unclipped_path = options.output_dir.join("unclipped.svg");
    let clipped_path = options.output_dir.join("clipped.svg");
    let comparison_path = options.output_dir.join("comparison.html");
    std::fs::write(&unclipped_path, &unclipped.svg)?;
    std::fs::write(&clipped_path, &clipped.svg)?;
    std::fs::write(&comparison_path, comparison_html(&unclipped, &clipped)?)?;
    println!(
        "wrote {} ({} -> {} triangles; empty={}; {})",
        comparison_path.display(),
        clipped.fragment.input_triangles,
        clipped.fragment.output_triangles,
        clipped.empty,
        clipped.fragment.fragment_sha256,
    );
    Ok(())
}

fn parse_args(args: impl Iterator<Item = OsString>) -> Result<Options, String> {
    let mut step = None;
    let mut output_dir = PathBuf::from("out/direct-static-clipping");
    let mut clip = false;
    let mut normal = [0.6, 0.2, 1.0];
    let mut distance_mm = 0.0;
    let mut tolerance_mm = 0.0;
    let values: Vec<_> = args.collect();
    let mut index = 0;
    while index < values.len() {
        let flag = values[index].to_string_lossy();
        let next = |name: &str, index: &mut usize| -> Result<&OsString, String> {
            *index += 1;
            values
                .get(*index)
                .ok_or_else(|| format!("{name} requires a value"))
        };
        match flag.as_ref() {
            "--step" => step = Some(PathBuf::from(next("--step", &mut index)?)),
            "--output-dir" => output_dir = PathBuf::from(next("--output-dir", &mut index)?),
            "--clip" => clip = true,
            "--normal" => normal = parse_normal(next("--normal", &mut index)?)?,
            "--distance" => {
                distance_mm = parse_number(next("--distance", &mut index)?, "--distance")?
            }
            "--tolerance" => {
                tolerance_mm = parse_number(next("--tolerance", &mut index)?, "--tolerance")?
            }
            "--cap-policy" => {
                if next("--cap-policy", &mut index)? != "none" {
                    return Err("the only supported cap policy is none".to_owned());
                }
            }
            "--help" | "-h" => {
                return Err("usage: direct_static_illustration [--step INPUT.step] [--clip] [--normal X,Y,Z] [--distance MM] [--tolerance MM] [--cap-policy none] [--output-dir DIR]".to_owned());
            }
            _ => return Err(format!("unknown argument: {flag}")),
        }
        index += 1;
    }
    if tolerance_mm < 0.0 {
        return Err("--tolerance must be nonnegative".to_owned());
    }
    Ok(Options {
        step,
        output_dir,
        clip,
        normal,
        distance_mm,
        tolerance_mm,
    })
}

fn parse_number(value: &OsString, name: &str) -> Result<f64, String> {
    let parsed = value
        .to_string_lossy()
        .parse::<f64>()
        .map_err(|_| format!("{name} must be a finite number"))?;
    if parsed.is_finite() {
        Ok(parsed)
    } else {
        Err(format!("{name} must be a finite number"))
    }
}

fn parse_normal(value: &OsString) -> Result<[f64; 3], String> {
    let parts: Vec<_> = value
        .to_string_lossy()
        .split(',')
        .map(str::to_owned)
        .collect();
    if parts.len() != 3 {
        return Err("--normal must be X,Y,Z".to_owned());
    }
    let normal = [
        parts[0].parse::<f64>(),
        parts[1].parse::<f64>(),
        parts[2].parse::<f64>(),
    ]
    .map(|item| item.map_err(|_| "--normal must contain three finite numbers".to_owned()))
    .into_iter()
    .collect::<Result<Vec<_>, _>>()?;
    let normal = [normal[0], normal[1], normal[2]];
    if normal.iter().any(|value| !value.is_finite()) {
        return Err("--normal must contain three finite numbers".to_owned());
    }
    if normal.iter().all(|value| *value == 0.0) {
        return Err("--normal must be nonzero".to_owned());
    }
    Ok(normal)
}

fn source(
    options: &Options,
) -> Result<(ModelIllustrationSourceA0, Option<Vec<u8>>), Box<dyn std::error::Error>> {
    if let Some(path) = &options.step {
        return Ok((
            ModelIllustrationSourceA0::ModelSource(ModelAttachmentIllustrationSourceA0 {
                kind: "model".to_owned(),
                attachment: "model".to_owned(),
                transform: None,
                material_override: None,
                tessellation: None,
            }),
            Some(std::fs::read(path)?),
        ));
    }
    let line = || {
        IllustrationProfileSegmentA0::Line(IllustrationProfileLineA0 {
            kind: "line".to_owned(),
        })
    };
    let ring = IllustrationProfileRingA0 {
        points_mm: vec![[-6.0, -4.0], [6.0, -4.0], [6.0, 4.0], [-6.0, 4.0]],
        segments: vec![line(), line(), line(), line()],
    };
    let material = |name: &str, color| MeshIllustrationMaterial {
        color,
        opacity: None,
        name: Some(name.to_owned()),
    };
    let definition = AnalyticDefinitionA0 {
        id: "clip-specimen".to_owned(),
        primitives: vec![
            AnalyticPrimitiveA0::Extrusion(AnalyticExtrusionA0 {
                kind: "extrusion".to_owned(),
                id: "through-plane-body".to_owned(),
                regions: vec![IllustrationProfileRegionA0 {
                    outer: ring,
                    holes: None,
                }],
                z_min_mm: -3.0,
                z_max_mm: 3.0,
                material: material("blue body", [0.15, 0.42, 0.68]),
            }),
            AnalyticPrimitiveA0::Cylinder(AnalyticCylinderA0 {
                kind: "cylinder".to_owned(),
                id: "contrasting-post".to_owned(),
                center_mm: [2.5, 0.0],
                radius_mm: 1.2,
                z_min_mm: -4.0,
                z_max_mm: 4.0,
                material: material("red post", [0.86, 0.22, 0.14]),
            }),
        ],
    };
    Ok((
        ModelIllustrationSourceA0::AnalyticSource(AnalyticIllustrationSourceA0 {
            kind: "analytic".to_owned(),
            scene: AnalyticSceneA0 {
                occurrences: vec![AnalyticOccurrenceA0 {
                    id: "specimen".to_owned(),
                    definition_id: definition.id.clone(),
                    transform: None,
                }],
                definitions: vec![definition],
            },
            lowering: None,
        }),
        None,
    ))
}

fn request(
    source: ModelIllustrationSourceA0,
    clipping: Option<IllustrationClipping>,
) -> ModelIllustrationRequestB0 {
    ModelIllustrationRequestB0 {
        schema: "geometry.model_illustration.request.b0".to_owned(),
        source,
        view: MeshIllustrationView {
            direction: [0.45, 0.65, 1.0],
            up: [0.0, 1.0, 0.0],
            mirror_x: None,
        },
        prepare: None,
        linework: Some(ModelIllustrationLineworkOptionsA0 {
            fast: Some(FastHlrOptionsA0 {
                include_boundaries: Some(true),
                include_creases: Some(true),
                include_silhouettes: Some(true),
                include_hidden: Some(false),
                suppress_coplanar_seams: Some(true),
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
        style: Some(MeshIllustrationStyleA0 {
            show_outlines: Some(false),
            show_creases: Some(false),
            show_hlr_outline: Some(true),
            show_hlr_detail: Some(true),
            transparent_background: Some(true),
            ..decode_mesh_illustration_style_a0_json(b"{}").expect("empty style is valid")
        }),
        svg: Some(MeshIllustrationSvgOptions {
            coordinate_span: None,
            title: Some("Geometer half-space clipping specimen".to_owned()),
        }),
        work_limits: None,
        clipping,
    }
}

fn comparison_html(
    unclipped: &ModelIllustrationResultB0,
    clipped: &ModelIllustrationResultB0,
) -> Result<String, serde_json::Error> {
    let metadata = serde_json::to_string_pretty(&serde_json::json!({
        "schema": clipped.schema,
        "empty": clipped.empty,
        "bounds_mm": clipped.bounds_mm,
        "fragment": clipped.fragment,
        "stats": clipped.stats,
        "cap_policy": "none",
    }))?;
    Ok(format!(
        "<!doctype html><html lang=\"en\"><meta charset=\"utf-8\"><title>Geometer clipping comparison</title><style>body{{font:15px system-ui;margin:24px;background:#eef1f3;color:#17252c}}main{{max-width:1500px;margin:auto}}.pair{{display:grid;grid-template-columns:1fr 1fr;gap:20px}}article{{background:white;padding:16px;border:1px solid #bbc5c8}}svg{{width:100%;height:auto}}pre{{white-space:pre-wrap;background:#17252c;color:#eef1f3;padding:16px}}@media(max-width:800px){{.pair{{grid-template-columns:1fr}}}}</style><main><h1>Generic half-space clipping</h1><p>The clipped result uses <code>cap_policy: none</code>; the exposed cut is an ordinary outline, not a synthesized face.</p><section class=\"pair\"><article><h2>Unclipped</h2>{}</article><article><h2>Clipped</h2>{}</article></section><h2>Governed result metadata</h2><pre>{}</pre></main></html>",
        strip_xml_declaration(&unclipped.svg),
        strip_xml_declaration(&clipped.svg),
        escape_html(&metadata),
    ))
}

fn strip_xml_declaration(svg: &str) -> &str {
    svg.find("<svg").map_or(svg, |index| &svg[index..])
}

fn escape_html(value: &str) -> String {
    value
        .replace('&', "&amp;")
        .replace('<', "&lt;")
        .replace('>', "&gt;")
}
