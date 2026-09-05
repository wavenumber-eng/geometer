//! Lab controls mapped to existing TypeSpec-generated values; no private wire schema.
use eframe::egui;
use geometer_client::contracts::*;

pub fn lab_style() -> MeshIllustrationStyleA0 {
    let mut style = decode_mesh_illustration_style_a0_json(b"{}").unwrap();
    style.shading = Some(MeshIllustrationShading::Toon);
    style.bands = Some(3);
    style.ambient = Some(0.28);
    style.key_intensity = Some(0.9);
    style.rim_amount = Some(0.12);
    style.light_direction = Some([0.35, 0.8, 0.48]);
    style.source_colors = Some(true);
    style.fallback_color = Some([113.0 / 255.0, 166.0 / 255.0, 160.0 / 255.0]);
    style.background = Some("#ffffff".into());
    style.transparent_background = Some(false);
    style.fuse_surfaces = Some(true);
    style.layer_coplanar_materials = Some(true);
    // The browser Lab explicitly hides these non-occlusion-tested overlays.
    style.show_outlines = Some(false);
    style.show_creases = Some(false);
    style.crease_angle_degrees = Some(42.0);
    style.show_hlr_outline = Some(false); // Native illustration has no linework attachment yet.
    style.show_hlr_detail = Some(false);
    style.outline_color = Some("#17252c".into());
    style.crease_color = style.outline_color.clone();
    style.outline_width = Some(0.006);
    style.crease_width = Some(0.006 * 0.55);
    style.double_sided = Some(false);
    style
}

pub fn mesh_defaults() -> ModelTessellationRequestA0 {
    ModelTessellationRequestA0 {
        schema: "geometry.model_tessellation.request.a0".into(),
        linear_deflection_mm: Some(0.1),
        angular_deflection_rad: Some(0.5),
        root_placement: None,
        max_triangles: None,
    }
}

pub fn rgb(color: &str) -> [u8; 3] {
    let value = color
        .strip_prefix('#')
        .filter(|s| s.len() == 6)
        .and_then(|s| u32::from_str_radix(s, 16).ok())
        .unwrap_or(0);
    [(value >> 16) as u8, (value >> 8) as u8, value as u8]
}

fn color_control(ui: &mut egui::Ui, label: &str, value: &mut Option<String>) {
    ui.horizontal(|ui| {
        let mut color = rgb(value.as_deref().unwrap_or("#000000"));
        if ui.color_edit_button_srgb(&mut color).changed() {
            *value = Some(format!("#{:02x}{:02x}{:02x}", color[0], color[1], color[2]));
        }
        ui.label(label);
    });
}

pub fn style_controls(ui: &mut egui::Ui, style: &mut MeshIllustrationStyleA0) -> bool {
    let before = style.clone();
    ui.label("Native illustration / Lab styling");
    let shading = style.shading.get_or_insert(MeshIllustrationShading::Toon);
    egui::ComboBox::from_id_salt("shading")
        .selected_text(format!("{shading:?}"))
        .width(ui.available_width())
        .show_ui(ui, |ui| {
            for mode in [
                MeshIllustrationShading::Unlit,
                MeshIllustrationShading::Flat,
                MeshIllustrationShading::Lambert,
                MeshIllustrationShading::Banded,
                MeshIllustrationShading::Toon,
            ] {
                ui.selectable_value(shading, mode.clone(), format!("{mode:?}"));
            }
        });
    ui.add(egui::Slider::new(style.bands.get_or_insert(3), 2..=32).text("Bands"));
    ui.add(egui::Slider::new(style.ambient.get_or_insert(0.28), 0.0..=1.0).text("Ambient"));
    ui.add(egui::Slider::new(style.key_intensity.get_or_insert(0.9), 0.0..=1.5).text("Key light"));
    ui.add(egui::Slider::new(style.rim_amount.get_or_insert(0.12), 0.0..=0.6).text("Rim"));
    ui.checkbox(
        style.source_colors.get_or_insert(true),
        "Source material colors",
    );
    ui.checkbox(
        style.fuse_surfaces.get_or_insert(true),
        "Fuse compatible surfaces",
    );
    ui.checkbox(
        style.layer_coplanar_materials.get_or_insert(true),
        "Coplanar material layers",
    );
    ui.checkbox(style.double_sided.get_or_insert(false), "Draw back faces");
    ui.horizontal(|ui| {
        let value =
            style
                .fallback_color
                .get_or_insert([113.0 / 255.0, 166.0 / 255.0, 160.0 / 255.0]);
        let mut color = value.map(|v| (v * 255.0).round() as u8);
        if ui.color_edit_button_srgb(&mut color).changed() {
            *value = color.map(|v| f64::from(v) / 255.0);
        }
        ui.label("Fallback color");
    });
    color_control(ui, "Background", &mut style.background);
    ui.checkbox(
        style.transparent_background.get_or_insert(false),
        "Transparent background",
    );
    color_control(ui, "Line color", &mut style.outline_color);
    ui.add(
        egui::Slider::new(style.outline_width.get_or_insert(0.006), 0.001..=0.018)
            .step_by(0.001)
            .text("Line width / span"),
    );
    // Same linked detail color/width policy as Illustration Lab's currentStyle().
    style.crease_color = style.outline_color.clone();
    style.crease_width = style.outline_width.map(|v| v * 0.55);
    ui.collapsing("Raw mesh lines — diagnostic only", |ui| {
        ui.small("Not hidden-line filtered: rear edges can appear over front surfaces. The browser Lab disables these. Fast HLR crease below controls different linework.");
        ui.checkbox(style.show_outlines.get_or_insert(false), "Raw mesh silhouettes");
        ui.checkbox(style.show_creases.get_or_insert(false), "Raw mesh creases");
        ui.add(egui::Slider::new(style.crease_angle_degrees.get_or_insert(42.0), 0.0..=180.0).text("Mesh crease °"));
    });
    ui.collapsing("Browser-only controls", |ui| {
        ui.label("Combined HLR + illustration SVG: native composition API not implemented yet. HLR controls below affect the separate HLR tabs.");
        ui.label("Experimental AO (enable, strength, radius, samples, bands): browser-only extension; absent from the governed native A0 contract. No native fallback is implied.");
    });
    *style != before
}

fn angle(
    ui: &mut egui::Ui,
    label: &str,
    radians: &mut Option<f64>,
    default: f64,
    range: std::ops::RangeInclusive<f64>,
) {
    let mut degrees = radians.unwrap_or(default.to_radians()).to_degrees();
    if ui
        .add(egui::Slider::new(&mut degrees, range).text(label))
        .changed()
    {
        *radians = Some(degrees.to_radians());
    }
}

pub fn hlr_controls(ui: &mut egui::Ui, options: &mut HlrProjectionOptionsA0) -> bool {
    let before = options.clone();
    ui.separator();
    ui.label("Independent HLR layers (not overlaid on SVG)");
    ui.checkbox(
        options.output_outline.get_or_insert(true),
        "Mesh-shadow outline",
    );
    ui.checkbox(
        options.output_detail.get_or_insert(true),
        "HLR detail lines",
    );
    let algorithm = options
        .projection_algorithm
        .get_or_insert(HlrProjectionAlgorithm::Fast);
    egui::ComboBox::from_id_salt("detail algorithm")
        .selected_text(format!("Detail: {algorithm:?}"))
        .width(ui.available_width())
        .show_ui(ui, |ui| {
            for mode in [
                HlrProjectionAlgorithm::Fast,
                HlrProjectionAlgorithm::Poly,
                HlrProjectionAlgorithm::Exact,
            ] {
                ui.selectable_value(algorithm, mode.clone(), format!("{mode:?}"));
            }
        });
    let fast_selected = *algorithm == HlrProjectionAlgorithm::Fast;
    let outline = options
        .outline_algorithm
        .get_or_insert(HlrOutlineAlgorithm::FastMeshShadow);
    egui::ComboBox::from_id_salt("outline algorithm")
        .selected_text(format!("Shadow: {outline:?}"))
        .width(ui.available_width())
        .show_ui(ui, |ui| {
            for mode in [
                HlrOutlineAlgorithm::FastMeshShadow,
                HlrOutlineAlgorithm::MeshShadow,
                HlrOutlineAlgorithm::HlrClose,
            ] {
                ui.selectable_value(outline, mode.clone(), format!("{mode:?}"));
            }
        });
    ui.add_enabled_ui(fast_selected, |ui| {
        let fast = options.fast.as_mut().expect("Lab fast options initialized");
        angle(
            ui,
            "Fast crease °",
            &mut fast.crease_angle_rad,
            25.0,
            1.0..=80.0,
        );
        ui.checkbox(
            fast.suppress_coplanar_seams.get_or_insert(false),
            "Hide coplanar joins (experimental)",
        );
        angle(
            ui,
            "Seam angle °",
            &mut fast.coplanar_seam_angle_rad,
            1.0,
            0.0..=10.0,
        );
        ui.add(
            egui::Slider::new(
                fast.coplanar_seam_depth_tolerance.get_or_insert(0.001),
                0.0..=0.01,
            )
            .step_by(0.0001)
            .text("Seam depth mm"),
        );
    });
    *options != before
}

pub fn mesh_controls(
    ui: &mut egui::Ui,
    mesh: &mut ModelTessellationRequestA0,
    hlr: &mut HlrProjectionOptionsA0,
) {
    ui.collapsing("Mesh quality / tessellation", |ui| {
        ui.horizontal_wrapped(|ui| {
            for (label, chord, degrees, relative) in [("Draft", 0.25, 40.0_f64, 0.008), ("Balanced", 0.1, 0.5_f64.to_degrees(), 0.004), ("Fine", 0.03, 15.0, 0.002), ("Extra fine", 0.01, 8.0, 0.001)] {
                if ui.button(label).clicked() {
                    mesh.linear_deflection_mm = Some(chord); mesh.angular_deflection_rad = Some(degrees.to_radians());
                    hlr.mesh_deflection_coefficient = Some(relative); hlr.mesh_angular_deflection = Some(degrees.to_radians());
                }
            }
        });
        ui.add(egui::Slider::new(mesh.linear_deflection_mm.get_or_insert(0.1), 0.001..=10.0).logarithmic(true).text("Chord error mm"));
        angle(ui, "Mesh angle °", &mut mesh.angular_deflection_rad, 0.5_f64.to_degrees(), 1.0..=60.0);
        ui.add(egui::Slider::new(hlr.mesh_deflection_coefficient.get_or_insert(0.004), 0.0001..=0.05).logarithmic(true).text("HLR chord / bbox"));
        angle(ui, "HLR angle °", &mut hlr.mesh_angular_deflection, 0.5_f64.to_degrees(), 1.0..=60.0);
        ui.small("STEP quality applies on load or Retessellate. HLR quality recomputes automatically. Native triangle/resource limits still apply.");
    });
}

#[cfg(test)]
mod tests {
    use super::*;
    #[test]
    fn lab_style_defaults_use_generated_contract_and_hide_raw_lines() {
        let style = lab_style();
        assert_eq!(style.show_outlines, Some(false));
        assert_eq!(style.show_creases, Some(false));
        assert_eq!(style.crease_angle_degrees, Some(42.0));
        assert_eq!(style.ambient, Some(0.28));
        assert_eq!(style.crease_width, Some(0.006 * 0.55));
        assert!(encode_mesh_illustration_style_a0_json(&style).is_ok());
        assert!(encode_model_tessellation_request_a0_json(&mesh_defaults()).is_ok());
    }
}
