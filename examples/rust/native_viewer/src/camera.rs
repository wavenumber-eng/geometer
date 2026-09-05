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
    pub yaw: f64,
    pub pitch: f64,
    pub half_height: f64,
    pub pan: [f64; 2],
}

impl Default for Camera {
    fn default() -> Self {
        Self {
            yaw: 0.38,
            pitch: 0.58,
            half_height: 1.2,
            pan: [0.0; 2],
        }
    }
}

impl Camera {
    pub fn basis(&self) -> (Vec3, Vec3, Vec3) {
        let direction = [
            self.yaw.sin() * self.pitch.cos(),
            self.pitch.sin(),
            self.yaw.cos() * self.pitch.cos(),
        ];
        let right = normalized(cross([0.0, 1.0, 0.0], direction));
        let up = normalized(cross(direction, right));
        (right, up, direction)
    }

    pub fn view(&self) -> MeshIllustrationView {
        let (_, up, direction) = self.basis();
        MeshIllustrationView {
            direction,
            up,
            mirror_x: None,
        }
    }

    pub fn fit(&mut self, bounds: Bounds, aspect: f64) {
        self.half_height = bounds.radius.max(1e-6) * 1.15 / aspect.clamp(0.1, 1.0);
        self.pan = [0.0; 2];
    }

    pub fn orbit(&mut self, delta: [f32; 2]) {
        self.yaw -= f64::from(delta[0]) * 0.009;
        self.pitch = (self.pitch + f64::from(delta[1]) * 0.009).clamp(-1.55, 1.55);
    }

    pub fn pan_pixels(&mut self, delta: [f32; 2], height: f32) {
        let scale = 2.0 * self.half_height / f64::from(height.max(1.0));
        self.pan[0] -= f64::from(delta[0]) * scale;
        self.pan[1] += f64::from(delta[1]) * scale;
    }

    pub fn zoom(&mut self, scroll: f32, bounds: Bounds) {
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

#[cfg(test)]
mod tests {
    use super::*;
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
