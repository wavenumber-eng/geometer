//! Interactive controls for the governed B0 half-space clipping request.
use crate::camera::Bounds;
use eframe::egui;
use geometer_client::contracts::{HalfSpacePlane, IllustrationClipping, MeshIllustrationView};

#[derive(Clone, Copy, Debug, Default, PartialEq, Eq)]
enum Mode {
    #[default]
    Off,
    X,
    Y,
    Z,
    Camera,
}

#[derive(Clone, Copy, Debug, Default, PartialEq, Eq)]
enum Side {
    #[default]
    Positive,
    Negative,
}

#[derive(Clone, Debug)]
pub struct Settings {
    mode: Mode,
    side: Side,
    position_percent: f64,
}

impl Default for Settings {
    fn default() -> Self {
        Self {
            mode: Mode::Off,
            side: Side::Positive,
            position_percent: 50.0,
        }
    }
}

impl Settings {
    pub fn show(&mut self, ui: &mut egui::Ui, bounds: Bounds, view: &MeshIllustrationView) -> bool {
        let before = self.clone();
        ui.label("B0 half-space clipping");
        egui::ComboBox::from_id_salt("clip plane")
            .selected_text(match self.mode {
                Mode::Off => "Off",
                Mode::X => "X axis",
                Mode::Y => "Y axis",
                Mode::Z => "Z axis",
                Mode::Camera => "Camera-facing",
            })
            .width(ui.available_width())
            .show_ui(ui, |ui| {
                ui.selectable_value(&mut self.mode, Mode::Off, "Off");
                ui.selectable_value(&mut self.mode, Mode::X, "X axis");
                ui.selectable_value(&mut self.mode, Mode::Y, "Y axis");
                ui.selectable_value(&mut self.mode, Mode::Z, "Z axis");
                ui.selectable_value(&mut self.mode, Mode::Camera, "Camera-facing");
            });
        ui.add_enabled_ui(self.mode != Mode::Off, |ui| {
            egui::ComboBox::from_id_salt("clip side")
                .selected_text(match self.side {
                    Side::Positive => "Keep positive side",
                    Side::Negative => "Keep negative side",
                })
                .width(ui.available_width())
                .show_ui(ui, |ui| {
                    ui.selectable_value(&mut self.side, Side::Positive, "Keep positive side");
                    ui.selectable_value(&mut self.side, Side::Negative, "Keep negative side");
                });
            ui.add(
                egui::Slider::new(&mut self.position_percent, 0.0..=100.0)
                    .suffix("%")
                    .text("Plane position"),
            );
            if let Some(clipping) = self.request(bounds, view) {
                ui.small(format!(
                    "Plane distance: {:.3} mm • cap policy: none",
                    clipping.planes[0].distance_mm
                ));
            }
        });
        ui.small("The 3D pane remains the complete source; the illustration is clipped before bounds, linework, and projection.");
        self.mode != before.mode
            || self.side != before.side
            || self.position_percent != before.position_percent
    }

    pub fn request(
        &self,
        bounds: Bounds,
        view: &MeshIllustrationView,
    ) -> Option<IllustrationClipping> {
        let basis = match self.mode {
            Mode::Off => return None,
            Mode::X => [1.0, 0.0, 0.0],
            Mode::Y => [0.0, 1.0, 0.0],
            Mode::Z => [0.0, 0.0, 1.0],
            Mode::Camera => view.direction,
        };
        let sign = if self.side == Side::Positive {
            1.0
        } else {
            -1.0
        };
        let normal = basis.map(|value| value * sign);
        let mut minimum = f64::INFINITY;
        let mut maximum = f64::NEG_INFINITY;
        for x in [bounds.minimum[0], bounds.maximum[0]] {
            for y in [bounds.minimum[1], bounds.maximum[1]] {
                for z in [bounds.minimum[2], bounds.maximum[2]] {
                    let distance = normal[0] * x + normal[1] * y + normal[2] * z;
                    minimum = minimum.min(distance);
                    maximum = maximum.max(distance);
                }
            }
        }
        let fraction = (self.position_percent / 100.0).clamp(0.0, 1.0);
        Some(IllustrationClipping {
            planes: vec![HalfSpacePlane {
                normal,
                distance_mm: minimum + (maximum - minimum) * fraction,
                tolerance_mm: Some(1e-6),
            }],
            cap_policy: "none".into(),
            limits: None,
        })
    }
}

#[cfg(test)]
mod tests {
    use super::*;

    #[test]
    fn center_plane_maps_to_the_b0_contract() {
        let bounds = Bounds {
            minimum: [-2.0, -4.0, -6.0],
            maximum: [8.0, 4.0, 6.0],
            center: [3.0, 0.0, 0.0],
            radius: 8.0,
        };
        let view = MeshIllustrationView {
            direction: [0.0, 0.0, 1.0],
            up: [0.0, 1.0, 0.0],
            mirror_x: None,
        };
        let settings = Settings {
            mode: Mode::X,
            side: Side::Positive,
            position_percent: 50.0,
        };
        let clipping = settings.request(bounds, &view).unwrap();
        assert_eq!(clipping.planes[0].normal, [1.0, 0.0, 0.0]);
        assert_eq!(clipping.planes[0].distance_mm, 3.0);
        assert_eq!(clipping.cap_policy, "none");
    }
}
