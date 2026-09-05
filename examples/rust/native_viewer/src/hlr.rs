//! Presentation of separate governed Fast HLR layers; no illustration composition policy.
use eframe::egui::ColorImage;
use geometer_client::contracts::*;
use resvg::tiny_skia;

pub fn options(view: &MeshIllustrationView) -> HlrProjectionOptionsA0 {
    let mut options =
        decode_hlr_projection_options_a0_json(b"{}").expect("empty optional options contract");
    options.views = Some(vec![HlrViewSpec {
        id: "preview".into(),
        direction: view.direction,
        up: view.up,
    }]);
    options.projection_algorithm = Some(HlrProjectionAlgorithm::Fast);
    options.outline_algorithm = Some(HlrOutlineAlgorithm::FastMeshShadow);
    options.output_outline = Some(true);
    options.output_detail = Some(true);
    options.output_bbox = Some(false);
    options.curve_mode = Some(HlrCurveMode::Polyline);
    options.strip_root_placement = Some(true);
    options.mesh_linear_deflection = Some(0.1);
    options.mesh_angular_deflection = Some(0.5);
    options.mesh_relative = Some(false);
    options
}

pub fn images(result: &HlrProjectionResultA0) -> Result<[ColorImage; 2], String> {
    let view = result.views.first().ok_or("Native HLR returned no view")?;
    let layers = [&view.modes.outline, &view.modes.detail];
    let mut bounds = [
        f64::INFINITY,
        f64::INFINITY,
        f64::NEG_INFINITY,
        f64::NEG_INFINITY,
    ];
    for layer in layers {
        if !layer.arcs.is_empty() {
            return Err("Polyline HLR unexpectedly returned arcs".into());
        }
        for segment in &layer.segments {
            for point in segment.chunks_exact(2) {
                bounds[0] = bounds[0].min(point[0]);
                bounds[1] = bounds[1].min(point[1]);
                bounds[2] = bounds[2].max(point[0]);
                bounds[3] = bounds[3].max(point[1]);
            }
        }
    }
    if !bounds.iter().all(|v| v.is_finite()) {
        bounds = [-1.0, -1.0, 1.0, 1.0];
    }
    let span = (bounds[2] - bounds[0]).max(bounds[3] - bounds[1]).max(1e-6);
    if !span.is_finite() {
        return Err("HLR extent exceeds preview range".into());
    }
    let scale = 1440.0 / span;
    let center = [(bounds[0] + bounds[2]) * 0.5, (bounds[1] + bounds[3]) * 0.5];
    let render = |layer: &ProjectedGeometry,
                  color: tiny_skia::Color|
     -> Result<ColorImage, String> {
        let mut pixels = tiny_skia::Pixmap::new(1600, 1600).ok_or("Cannot allocate HLR preview")?;
        pixels.fill(tiny_skia::Color::WHITE);
        let mut paint = tiny_skia::Paint::default();
        paint.set_color(color);
        let stroke = tiny_skia::Stroke {
            width: 2.0,
            ..Default::default()
        };
        // Batches bound temporary path storage even for large native outputs.
        for batch in layer.segments.chunks(4096) {
            let mut path = tiny_skia::PathBuilder::new();
            for segment in batch {
                path.move_to(
                    ((segment[0] - center[0]) * scale + 800.0) as f32,
                    (800.0 - (segment[1] - center[1]) * scale) as f32,
                );
                path.line_to(
                    ((segment[2] - center[0]) * scale + 800.0) as f32,
                    (800.0 - (segment[3] - center[1]) * scale) as f32,
                );
            }
            if let Some(path) = path.finish() {
                pixels.stroke_path(
                    &path,
                    &paint,
                    &stroke,
                    tiny_skia::Transform::identity(),
                    None,
                );
            }
        }
        Ok(ColorImage::from_rgba_premultiplied(
            [1600, 1600],
            pixels.data(),
        ))
    };
    Ok([
        render(layers[0], tiny_skia::Color::from_rgba8(34, 105, 160, 255))?,
        render(layers[1], tiny_skia::Color::from_rgba8(30, 35, 40, 255))?,
    ])
}

#[cfg(test)]
mod tests {
    use super::*;
    #[test]
    fn governed_options_select_fast_detail_and_shadow_with_shared_placement() {
        let options = options(&crate::camera::Camera::default().view());
        assert_eq!(
            options.projection_algorithm,
            Some(HlrProjectionAlgorithm::Fast)
        );
        assert_eq!(
            options.outline_algorithm,
            Some(HlrOutlineAlgorithm::FastMeshShadow)
        );
        assert_eq!(options.strip_root_placement, Some(true));
        assert_eq!(options.curve_mode, Some(HlrCurveMode::Polyline));
        assert!(encode_hlr_projection_options_a0_json(&options).is_ok());
    }
}
