//! Orthographic presentation camera; orbit never changes the fitted extent.
use geometer_client::contracts::MeshIllustrationView;

pub type Vec3 = [f64; 3];

pub fn sub(a: Vec3, b: Vec3) -> Vec3 {
    [a[0] - b[0], a[1] - b[1], a[2] - b[2]]
}
pub fn dot(a: Vec3, b: Vec3) -> f64 {
    a[0] * b[0] + a[1] * b[1] + a[2] * b[2]
}
pub fn cross(a: Vec3, b: Vec3) -> Vec3 {
    [
        a[1] * b[2] - a[2] * b[1],
        a[2] * b[0] - a[0] * b[2],
        a[0] * b[1] - a[1] * b[0],
    ]
}
pub fn normalized(value: Vec3) -> Vec3 {
    let length = dot(value, value).sqrt().max(1e-20);
    value.map(|v| v / length)
}

#[derive(Clone, Copy, Debug)]
pub struct Bounds {
    pub center: Vec3,
    pub radius: f64,
}

impl Default for Bounds {
    fn default() -> Self {
        Self {
            center: [0.0; 3],
            radius: 1.0,
        }
    }
}

#[derive(Clone, Debug)]
pub struct Camera {
    direction: Vec3,
    up: Vec3,
    mirror_x: bool,
    pub half_height: f64,
    pub pan: [f64; 2],
}

impl Default for Camera {
    fn default() -> Self {
        Self {
            direction: [
                0.38_f64.sin() * 0.58_f64.cos(),
                0.58_f64.sin(),
                0.38_f64.cos() * 0.58_f64.cos(),
            ],
            up: [0.0, 1.0, 0.0],
            mirror_x: false,
            half_height: 1.2,
            pan: [0.0; 2],
        }
    }
}

impl Camera {
    pub fn basis(&self) -> (Vec3, Vec3, Vec3) {
        let direction = self.direction;
        let right = normalized(cross(self.up, direction));
        let up = normalized(cross(direction, right));
        let right = right.map(|v| if self.mirror_x { -v } else { v });
        (right, up, direction)
    }

    pub fn view(&self) -> MeshIllustrationView {
        let (_, up, direction) = self.basis();
        MeshIllustrationView {
            direction,
            up,
            mirror_x: Some(self.mirror_x),
        }
    }

    pub fn set_view(&mut self, view: &MeshIllustrationView) {
        self.direction = normalized(view.direction);
        let right = normalized(cross(view.up, self.direction));
        self.up = normalized(cross(self.direction, right));
        self.mirror_x = view.mirror_x.unwrap_or(false);
    }

    pub fn fit(&mut self, bounds: Bounds, aspect: f64) {
        self.half_height = bounds.radius.max(1e-6) * 1.15 / aspect.clamp(0.1, 1.0);
        self.pan = [0.0; 2];
    }

    pub fn orbit(&mut self, delta: [f32; 2]) {
        // Rotate in the current camera frame so every signed-axis preset works,
        // including exact poles. Never reset its roll, mirror, zoom or pan.
        let (_, up, _) = self.basis();
        let sign = if self.mirror_x { -1.0 } else { 1.0 };
        self.direction = rotate(self.direction, up, -f64::from(delta[0]) * 0.009 * sign);
        let right = normalized(cross(up, self.direction));
        let angle = -f64::from(delta[1]) * 0.009;
        self.direction = normalized(rotate(self.direction, right, angle));
        self.up = normalized(rotate(up, right, angle));
    }

    pub fn pan_pixels(&mut self, delta: [f32; 2], height: f32) {
        let scale = 2.0 * self.half_height / f64::from(height.max(1.0));
        self.pan[0] -= f64::from(delta[0]) * scale;
        self.pan[1] += f64::from(delta[1]) * scale;
    }

    pub fn zoom(&mut self, scroll: f32, bounds: Bounds) {
        if scroll == 0.0 {
            return;
        }
        self.half_height = (self.half_height * (-f64::from(scroll) * 0.002).exp())
            .clamp(bounds.radius * 0.005, bounds.radius * 100.0);
    }

    pub fn uniform(&self, bounds: Bounds, aspect: f64) -> [[f32; 4]; 4] {
        let (right, up, direction) = self.basis();
        let vector = |v: Vec3, w: f64| [v[0] as f32, v[1] as f32, v[2] as f32, w as f32];
        [
            vector(right, self.pan[0]),
            vector(up, self.pan[1]),
            vector(direction, bounds.radius * 2.0),
            [
                (1.0 / (self.half_height * aspect)) as f32,
                (1.0 / self.half_height) as f32,
                (1.0 / (bounds.radius * 4.0)) as f32,
                0.0,
            ],
        ]
    }
}

fn rotate(v: Vec3, axis: Vec3, angle: f64) -> Vec3 {
    let (s, c) = angle.sin_cos();
    let perpendicular = cross(axis, v);
    let along = dot(axis, v) * (1.0 - c);
    std::array::from_fn(|i| v[i] * c + perpendicular[i] * s + axis[i] * along)
}

#[cfg(test)]
mod tests {
    use super::*;
    #[test]
    fn zero_scroll_does_not_clamp_zoom_to_temporary_empty_bounds() {
        for extent in [1000.0, 0.00001] {
            let mut camera = Camera {
                half_height: extent,
                ..Default::default()
            };
            camera.zoom(0.0, Bounds::default());
            assert_eq!(camera.half_height, extent);
        }
    }
    #[test]
    fn orbit_retains_zoom_pan_and_orthonormal_basis() {
        let mut camera = Camera::default();
        camera.fit(
            Bounds {
                center: [1.0, 2.0, 3.0],
                radius: 8.0,
            },
            0.7,
        );
        camera.pan_pixels([10.0, 20.0], 500.0);
        let before = (camera.half_height, camera.pan);
        for _ in 0..100 {
            camera.orbit([5.0, -3.0]);
        }
        assert_eq!((camera.half_height, camera.pan), before);
        let (right, up, direction) = camera.basis();
        assert!(dot(right, up).abs() < 1e-12 && dot(right, direction).abs() < 1e-12);
        assert!((dot(direction, direction) - 1.0).abs() < 1e-12);
    }
}
