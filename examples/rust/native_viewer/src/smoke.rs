//! Opt-in automated window smoke; captures only this app's rendered viewport.
use eframe::egui;
use std::{
    path::PathBuf,
    sync::{
        Arc,
        atomic::{AtomicBool, Ordering},
    },
    time::{Duration, Instant},
};

pub struct Smoke {
    pub enabled: bool,
    pub screenshot: Option<PathBuf>,
    pub passed: Arc<AtomicBool>,
    started: Instant,
    requested: bool,
}

impl Default for Smoke {
    fn default() -> Self {
        Self {
            enabled: false,
            screenshot: None,
            passed: Arc::new(AtomicBool::new(false)),
            started: Instant::now(),
            requested: false,
        }
    }
}

impl Smoke {
    pub fn update(
        &mut self,
        ctx: &egui::Context,
        complete: bool,
        error: Option<&str>,
        evidence: &str,
    ) {
        if !self.enabled {
            return;
        }
        ctx.request_repaint_after(Duration::from_millis(50));
        if error.is_some() || self.started.elapsed() > Duration::from_secs(45) {
            eprintln!("GPU_SMOKE_FAILED {}", error.unwrap_or("45 second timeout"));
            self.enabled = false;
            ctx.send_viewport_cmd(egui::ViewportCommand::Close);
            return;
        }
        if !complete {
            return;
        }
        if let Some(path) = &self.screenshot {
            if !self.requested {
                ctx.send_viewport_cmd(egui::ViewportCommand::Screenshot(Default::default()));
                self.requested = true;
                return;
            }
            let screenshot = ctx.input(|input| {
                input.events.iter().find_map(|event| {
                    if let egui::Event::Screenshot { image, .. } = event {
                        Some(image.clone())
                    } else {
                        None
                    }
                })
            });
            let Some(image) = screenshot else {
                return;
            };
            let mut pixels =
                resvg::tiny_skia::Pixmap::new(image.width() as u32, image.height() as u32).unwrap();
            for (source, target) in image
                .pixels
                .iter()
                .zip(pixels.data_mut().chunks_exact_mut(4))
            {
                target.copy_from_slice(&source.to_array());
            }
            if let Err(error) = pixels.save_png(path) {
                eprintln!("GPU_SMOKE_FAILED screenshot: {error}");
                self.enabled = false;
                ctx.send_viewport_cmd(egui::ViewportCommand::Close);
                return;
            }
        }
        println!("GPU_SMOKE_OK {evidence}");
        self.passed.store(true, Ordering::Relaxed);
        self.enabled = false;
        ctx.send_viewport_cmd(egui::ViewportCommand::Close);
    }
}
