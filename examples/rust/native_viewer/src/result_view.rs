//! Fixed white output canvas. Status changes never participate in image layout.
use eframe::egui;

pub fn status(ui: &mut egui::Ui, busy: bool, text: &str) {
    egui::Panel::top("job status")
        .resizable(false)
        .exact_size(26.0)
        .frame(
            egui::Frame::default()
                .fill(egui::Color32::WHITE)
                .inner_margin(4.0),
        )
        .show(ui, |ui| {
            ui.horizontal(|ui| {
                if busy {
                    ui.add_sized([18.0, 18.0], egui::Spinner::new().size(14.0));
                } else {
                    ui.allocate_exact_size(egui::vec2(18.0, 18.0), egui::Sense::hover());
                }
                ui.add(egui::Label::new(text).truncate());
            });
        });
}

pub fn centered_rect(area: egui::Rect, size: egui::Vec2) -> egui::Rect {
    let scale = (area.width() / size.x.max(1.0))
        .min(area.height() / size.y.max(1.0))
        .max(0.0);
    egui::Rect::from_center_size(
        area.center(),
        (size * scale).min(area.size().max(egui::Vec2::ZERO)),
    )
}

/// Returns a request to save the current original illustration SVG.
pub fn show(
    ui: &mut egui::Ui,
    textures: [Option<&egui::TextureHandle>; 3],
    tab: &mut usize,
    can_save: bool,
) -> bool {
    let mut save = false;
    let (toolbar, _) =
        ui.allocate_exact_size(egui::vec2(ui.available_width(), 28.0), egui::Sense::hover());
    ui.scope_builder(egui::UiBuilder::new().max_rect(toolbar).layout(egui::Layout::left_to_right(egui::Align::Center)), |ui| {
        ui.set_clip_rect(toolbar.intersect(ui.clip_rect()));
        save = ui.add_enabled(can_save && *tab == 0, egui::Button::new("Save SVG…"))
            .on_hover_text("Save the current completed illustration as original vector SVG. Available when the Illustration SVG view is up to date.").clicked();
        let names = ["Illustration SVG", "HLR shadow", "HLR detail"];
        egui::ComboBox::from_id_salt("output selection").selected_text(names[*tab]).width(ui.available_width()).truncate().show_ui(ui, |ui| {
            for (index, label) in names.iter().enumerate() { ui.selectable_value(tab, index, *label); }
        });
    });
    let (area, _) = ui.allocate_exact_size(
        ui.available_size().max(egui::Vec2::ZERO),
        egui::Sense::hover(),
    );
    ui.painter().rect_filled(area, 0.0, egui::Color32::WHITE);
    if let Some(texture) = textures[*tab] {
        ui.painter().image(
            texture.id(),
            centered_rect(area, texture.size_vec2()),
            egui::Rect::from_min_max(egui::Pos2::ZERO, egui::pos2(1.0, 1.0)),
            egui::Color32::WHITE,
        );
    } else {
        ui.painter().text(
            area.center(),
            egui::Align2::CENTER_CENTER,
            "Open a STEP model",
            egui::FontId::proportional(14.0),
            egui::Color32::GRAY,
        );
    }
    save
}

#[cfg(test)]
mod tests {
    use super::*;

    #[test]
    fn image_fits_and_centers_in_both_axes() {
        let area = egui::Rect::from_min_size(egui::pos2(20.0, 40.0), egui::vec2(300.0, 500.0));
        for size in [egui::vec2(800.0, 200.0), egui::vec2(100.0, 900.0)] {
            let image = centered_rect(area, size);
            assert_eq!(image.center(), area.center());
            assert!(area.contains_rect(image));
            assert!((image.width() / image.height() - size.x / size.y).abs() < 1e-5);
        }
    }

    #[test]
    fn status_has_identical_layout_when_busy_or_long() {
        let context = egui::Context::default();
        let mut available = Vec::new();
        for (busy, message) in [
            (false, "Complete"),
            (
                true,
                "Geometer recomputing a very long model name with another view queued and detailed progress",
            ),
        ] {
            let input = egui::RawInput {
                screen_rect: Some(egui::Rect::from_min_size(
                    egui::Pos2::ZERO,
                    egui::vec2(300.0, 600.0),
                )),
                ..Default::default()
            };
            let mut output = context.run_ui(input, |ui| {
                egui::CentralPanel::default().show(ui, |ui| {
                    status(ui, busy, message);
                    available.push(ui.available_rect_before_wrap());
                });
            });
            output.textures_delta.clear();
        }
        assert!(available.len() >= 2);
        assert!(available.windows(2).all(|pair| pair[0] == pair[1]));
    }
}
