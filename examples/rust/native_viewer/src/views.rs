//! Same signed model-axis preset frame as examples/wasm/hlr_projection_views.js.
use crate::camera::{Camera, cross, normalized};
use eframe::egui;
use geometer_client::contracts::MeshIllustrationView;

const AXES: [(&str, [f64; 3]); 6] = [
    ("+X", [1.0, 0.0, 0.0]),
    ("-X", [-1.0, 0.0, 0.0]),
    ("+Y", [0.0, 1.0, 0.0]),
    ("-Y", [0.0, -1.0, 0.0]),
    ("+Z", [0.0, 0.0, 1.0]),
    ("-Z", [0.0, 0.0, -1.0]),
];
const LABELS: [&str; 10] = [
    "Top", "Bot", "Front", "Back", "Left", "Right", "ISO T", "ISO B", "ISO F", "ISO Back",
];

pub struct Views {
    top: usize,
    front: usize,
    pub selected: Option<usize>,
}

impl Default for Views {
    fn default() -> Self {
        Self {
            top: 2,
            front: 4,
            selected: None,
        }
    }
}

fn presets(top: usize, front: usize) -> [MeshIllustrationView; 10] {
    let t = AXES[top].1;
    let f = AXES[front].1;
    let r = cross(t, f);
    let neg = |v: [f64; 3]| v.map(|x| -x);
    let sum = |a: [f64; 3], b: [f64; 3], c: [f64; 3]| {
        normalized(std::array::from_fn(|i| a[i] + b[i] + c[i]))
    };
    [
        (t, neg(f), false),
        (neg(t), neg(f), false),
        (f, t, false),
        (neg(f), t, true),
        (neg(r), t, true),
        (r, t, false),
        (sum(r, neg(f), t), neg(f), false),
        (sum(r, f, neg(t)), f, false),
        (sum(r, f, t), t, false),
        (sum(r, neg(f), neg(t)), neg(t), false),
    ]
    .map(|(direction, up, mirror)| MeshIllustrationView {
        direction,
        up,
        mirror_x: Some(mirror),
    })
}

impl Views {
    pub fn show(&mut self, ui: &mut egui::Ui, camera: &mut Camera) -> bool {
        let before = (self.top, self.front);
        axis_control(ui, "Top axis", &mut self.top, None);
        if self.top / 2 == self.front / 2 {
            self.front = if self.top / 2 == 2 { 0 } else { 4 };
        }
        axis_control(ui, "Front axis", &mut self.front, Some(self.top));
        let mut changed = before != (self.top, self.front) && self.selected.is_some();
        ui.horizontal_wrapped(|ui| {
            for (index, label) in LABELS.iter().enumerate() {
                if ui
                    .selectable_label(self.selected == Some(index), *label)
                    .clicked()
                {
                    self.selected = Some(index);
                    changed = true;
                }
            }
        });
        if changed && let Some(index) = self.selected {
            camera.set_view(&presets(self.top, self.front)[index]);
        }
        changed
    }
}

fn axis_control(ui: &mut egui::Ui, label: &str, selected: &mut usize, exclude: Option<usize>) {
    ui.horizontal(|ui| {
        ui.label(label);
        egui::ComboBox::from_id_salt(label)
            .selected_text(AXES[*selected].0)
            .show_ui(ui, |ui| {
                for (index, (name, _)) in AXES.iter().enumerate() {
                    ui.add_enabled_ui(exclude.is_none_or(|axis| axis / 2 != index / 2), |ui| {
                        ui.selectable_value(selected, index, *name);
                    });
                }
            });
    });
}

#[cfg(test)]
mod tests {
    use super::*;
    use crate::camera::dot;

    #[test]
    fn every_signed_frame_preserves_camera_projection_and_orbits_without_a_pole() {
        for top in 0..6 {
            for front in 0..6 {
                if top / 2 == front / 2 {
                    continue;
                }
                for view in presets(top, front) {
                    let mut camera = Camera::default();
                    camera.half_height = 12.0;
                    camera.pan = [2.0, 3.0];
                    camera.set_view(&view);
                    let (right, up, direction) = camera.basis();
                    assert!(dot(up, direction).abs() < 1e-12);
                    assert!((dot(right, right) - 1.0).abs() < 1e-12);
                    assert_eq!(camera.view().mirror_x, view.mirror_x);
                    for (actual, expected) in direction.iter().zip(view.direction) {
                        assert!((actual - expected).abs() < 1e-12);
                    }
                    camera.orbit([0.0, 0.0]);
                    for (actual, expected) in camera.basis().0.iter().zip(right) {
                        assert!((actual - expected).abs() < 1e-12);
                    }
                    camera.orbit([8.0, -5.0]);
                    assert_eq!((camera.half_height, camera.pan), (12.0, [2.0, 3.0]));
                    assert!(dot(camera.basis().1, camera.basis().2).abs() < 1e-12);
                }
            }
        }
    }

    #[test]
    #[ignore = "requires Node test oracle; opt-in RUST_002 GPU lane"]
    fn presets_match_web_for_every_signed_axis_frame() {
        let root = std::path::Path::new(env!("CARGO_MANIFEST_DIR")).join("../../..");
        let output = std::process::Command::new("node").current_dir(root)
            .args(["--input-type=module", "-e", r#"
                import {AXIS_VECTORS,buildProjectionViews} from './examples/wasm/hlr_projection_views.js';
                const result=[];
                for(const top of Object.keys(AXIS_VECTORS)) for(const front of Object.keys(AXIS_VECTORS)) {
                    if(top.slice(1)===front.slice(1)) continue;
                    for(const v of buildProjectionViews(top,front)) result.push({direction:v.direction,up:v.up,mirror_x:!!v.mirrorX});
                }
                for(const view of result) console.log(JSON.stringify({schema:'geometry.mesh_illustration.request.a0',view}));
            "#]).output().unwrap();
        assert!(output.status.success());
        let expected: Vec<_> = std::str::from_utf8(&output.stdout)
            .unwrap()
            .lines()
            .map(|line| {
                geometer_client::contracts::decode_mesh_illustration_request_a0_json(
                    line.as_bytes(),
                )
                .unwrap()
                .view
            })
            .collect();
        let actual: Vec<_> = (0..6)
            .flat_map(|top| {
                (0..6)
                    .filter(move |front| top / 2 != front / 2)
                    .flat_map(move |front| presets(top, front))
            })
            .collect();
        assert_eq!(actual.len(), 240);
        for (a, b) in actual.iter().zip(expected) {
            assert_eq!(a.mirror_x, b.mirror_x);
            assert_eq!(a.up, b.up);
            for (x, y) in a.direction.iter().zip(b.direction) {
                assert!((x - y).abs() < 1e-12);
            }
        }
    }
}
