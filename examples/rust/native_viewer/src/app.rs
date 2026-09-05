//! Responsive demonstration of the maintained native client, not a second geometry engine.
use crate::{
    camera::Camera,
    gpu::Preview,
    jobs::{Event, Export, Jobs, Model, Solution},
};
use eframe::egui;
use geometer_client::{GeometerClient, contracts::*};
use std::{
    path::PathBuf,
    sync::Arc,
    time::{Duration, Instant},
};

pub struct App {
    preview: Preview,
    camera: Camera,
    jobs: Jobs,
    client: Option<GeometerClient>,
    executable: String,
    startup_step: Option<PathBuf>,
    model: Option<Arc<Model>>,
    solution: Option<Arc<Solution>>,
    texture: Option<egui::TextureHandle>,
    hlr_textures: Option<[egui::TextureHandle; 2]>,
    result_tab: usize,
    style: MeshIllustrationStyleA0,
    detail_algorithm: HlrProjectionAlgorithm,
    outline_algorithm: HlrOutlineAlgorithm,
    right_dock: bool,
    busy: bool,
    started: Instant,
    phase: String,
    error: Option<String>,
    revision: u64,
    completed: Option<u64>,
    pending: Option<Instant>,
    aspect: f64,
    smoke: crate::smoke::Smoke,
}

impl App {
    pub fn new(
        context: &eframe::CreationContext<'_>,
        executable: Option<PathBuf>,
        step: Option<PathBuf>,
        smoke: crate::smoke::Smoke,
    ) -> Result<Self, Box<dyn std::error::Error + Send + Sync>> {
        context.egui_ctx.set_visuals(egui::Visuals::light());
        let state = context
            .wgpu_render_state
            .clone()
            .ok_or("wgpu renderer unavailable")?;
        let style = decode_mesh_illustration_style_a0_json(b"{}")?;
        let mut app = Self {
            preview: Preview::new(state),
            camera: Camera::default(),
            jobs: Jobs::new(context.egui_ctx.clone())?,
            client: None,
            executable: executable
                .as_ref()
                .map_or(String::new(), |path| path.display().to_string()),
            startup_step: step,
            model: None,
            solution: None,
            texture: None,
            hlr_textures: None,
            result_tab: 0,
            style,
            detail_algorithm: HlrProjectionAlgorithm::Fast,
            outline_algorithm: HlrOutlineAlgorithm::FastMeshShadow,
            right_dock: false,
            busy: false,
            started: Instant::now(),
            phase: "Choose a compatible Geometer executable".into(),
            error: None,
            revision: 0,
            completed: None,
            pending: None,
            aspect: 1.0,
            smoke,
        };
        if executable.is_some() {
            app.connect();
        }
        Ok(app)
    }

    fn begin(&mut self, phase: &str) {
        self.busy = true;
        self.started = Instant::now();
        self.phase = phase.into();
        self.error = None;
    }

    fn connect(&mut self) {
        self.begin("Negotiating native IPC catalog");
        self.jobs.connect(PathBuf::from(&self.executable));
    }

    fn changed(&mut self) {
        self.revision += 1;
        self.pending = Some(Instant::now() + Duration::from_millis(160));
    }

    fn load(&mut self, path: PathBuf) {
        if let Some(client) = self.client.clone() {
            self.begin("Loading STEP");
            self.model = None;
            self.preview.clear();
            self.solution = None;
            self.texture = None;
            self.hlr_textures = None;
            self.completed = None;
            self.pending = None;
            self.revision += 1;
            self.jobs.load(client, path);
        }
    }

    fn events(&mut self, ctx: &egui::Context) {
        while let Ok((epoch, event)) = self.jobs.events.try_recv() {
            if epoch != self.jobs.epoch() {
                continue;
            }
            match event {
                Event::Phase(phase) => self.phase = phase.into(),
                Event::Stopped(result) => {
                    self.busy = false;
                    self.phase = "Disconnected; reconnect to resume".into();
                    self.error = result.err();
                }
                Event::Connected(result) => {
                    self.busy = false;
                    match result {
                        Ok(client) => {
                            self.phase =
                                format!("Connected: Geometer {}", client.welcome().release_version);
                            self.client = Some(client);
                            if let Some(step) = self.startup_step.take() {
                                self.load(step);
                            } else if self.model.is_some() {
                                self.changed();
                            }
                        }
                        Err(error) => self.error = Some(error),
                    }
                }
                Event::Loaded(result) => {
                    self.busy = false;
                    if self.client.is_none() {
                        continue;
                    }
                    match result {
                        Ok((model, mesh)) => match self.preview.upload(mesh) {
                            Ok(()) => {
                                self.model = Some(model);
                                self.camera.fit(self.preview.bounds, self.aspect);
                                self.changed();
                            }
                            Err(error) => self.error = Some(error),
                        },
                        Err(error) => self.error = Some(error),
                    }
                }
                Event::Solved(revision, result) => {
                    self.busy = false;
                    if revision != self.revision {
                        continue;
                    }
                    match result {
                        Ok(solution) => {
                            self.texture = Some(ctx.load_texture(
                                "native SVG",
                                solution.image.clone(),
                                egui::TextureOptions::LINEAR,
                            ));
                            self.hlr_textures = Some(std::array::from_fn(|i| {
                                ctx.load_texture(
                                    format!("HLR layer {i}"),
                                    solution.hlr_images[i].clone(),
                                    egui::TextureOptions::LINEAR,
                                )
                            }));
                            self.phase =
                                format!("Complete in {:.2}s", self.started.elapsed().as_secs_f64());
                            self.completed = Some(revision);
                            self.solution = Some(solution);
                        }
                        Err(error) => self.error = Some(error),
                    }
                }
                Event::FilePicked(executable, path) => {
                    self.busy = false;
                    if let Some(path) = path {
                        if executable {
                            self.executable = path.display().to_string();
                        } else {
                            self.load(path);
                        }
                    }
                }
                Event::Exported(result) => {
                    self.busy = false;
                    match result {
                        Ok(()) => self.phase = "Export dialog completed".into(),
                        Err(error) => self.error = Some(error),
                    }
                }
            }
        }
        if !self.busy && self.pending.is_some_and(|due| due <= Instant::now()) {
            self.pending = None;
            if let (Some(client), Some(model)) = (self.client.clone(), self.model.clone()) {
                self.begin("Geometer: recomputing");
                let mut options = crate::hlr::options(&self.camera.view());
                options.projection_algorithm = Some(self.detail_algorithm.clone());
                options.outline_algorithm = Some(self.outline_algorithm.clone());
                self.jobs.solve(
                    self.revision,
                    client,
                    model,
                    self.camera.view(),
                    self.style.clone(),
                    options,
                );
            }
        }
    }

    fn controls(&mut self, ui: &mut egui::Ui) {
        ui.heading("Geometer");
        ui.label("Native Rust API Lab");
        ui.horizontal(|ui| {
            ui.label("Controls dock");
            ui.selectable_value(&mut self.right_dock, false, "Left");
            ui.selectable_value(&mut self.right_dock, true, "Right");
        });
        ui.separator();
        ui.label("Executable");
        ui.add_enabled(
            self.client.is_none() && !self.busy,
            egui::TextEdit::singleline(&mut self.executable).desired_width(f32::INFINITY),
        );
        ui.horizontal_wrapped(|ui| {
            if ui
                .add_enabled(
                    !self.busy && self.client.is_none(),
                    egui::Button::new("Browse…"),
                )
                .clicked()
            {
                self.begin("Choose executable");
                self.jobs.pick(true);
            }
            if ui
                .add_enabled(
                    !self.busy && self.client.is_none() && !self.executable.is_empty(),
                    egui::Button::new("Connect"),
                )
                .clicked()
            {
                self.connect();
            }
            if ui
                .add_enabled(self.client.is_some(), egui::Button::new("Stop process"))
                .clicked()
            {
                if let Some(client) = self.client.take() {
                    self.jobs.stop(client);
                }
                self.revision += 1;
                self.pending = None;
                self.begin("Stopping owned Geometer process");
            }
        });
        if let Some(client) = &self.client {
            ui.label(format!(
                "Geometer {} • IPC A0",
                client.welcome().release_version
            ));
            ui.small(format!(
                "Catalog {}…",
                &client.welcome().catalog_sha256[..16]
            ));
            ui.small(format!(
                "ABI {} • {} operations",
                client.welcome().c_abi_generation,
                client.welcome().operation_catalog.operations.len()
            ));
            ui.collapsing("IPC capabilities", |ui| {
                ui.small(client.welcome().capabilities.join(", "));
            });
        }
        if ui
            .add_enabled(
                !self.busy && self.client.is_some(),
                egui::Button::new("Open STEP model…"),
            )
            .clicked()
        {
            self.begin("Choose STEP model");
            self.jobs.pick(false);
        }
        if let Some(model) = &self.model {
            ui.label(model.path.file_name().unwrap_or_default().to_string_lossy());
        }
        ui.separator();
        ui.label("3D camera");
        ui.horizontal_wrapped(|ui| {
            if ui.button("Fit").clicked() {
                self.camera.fit(self.preview.bounds, self.aspect);
            }
            if ui.button("Top").clicked() {
                self.camera.yaw = 0.0;
                self.camera.pitch = 0.0;
                self.changed();
            }
            if ui.button("Isometric").clicked() {
                self.camera.yaw = 0.38;
                self.camera.pitch = 0.58;
                self.changed();
            }
        });
        ui.small("Drag: orbit • right/middle drag: pan\nWheel: zoom • orbit never refits");
        ui.separator();
        ui.label("Native illustration");
        let before = self.style.clone();
        let shading = self
            .style
            .shading
            .get_or_insert(MeshIllustrationShading::Toon);
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
        ui.checkbox(
            self.style.source_colors.get_or_insert(true),
            "Source material colors",
        );
        ui.checkbox(
            self.style.fuse_surfaces.get_or_insert(true),
            "Fuse compatible surfaces",
        );
        ui.checkbox(
            self.style.layer_coplanar_materials.get_or_insert(true),
            "Coplanar material layers",
        );
        ui.checkbox(
            self.style.show_outlines.get_or_insert(true),
            "Mesh outlines",
        );
        ui.checkbox(self.style.show_creases.get_or_insert(true), "Mesh creases");
        ui.checkbox(
            self.style.transparent_background.get_or_insert(false),
            "Transparent SVG background",
        );
        ui.add(
            egui::Slider::new(self.style.ambient.get_or_insert(0.35), 0.0..=1.0).text("Ambient"),
        );
        if self.style != before {
            self.changed();
        }
        if ui
            .add_enabled(
                self.model.is_some() && self.client.is_some(),
                egui::Button::new("Recompute"),
            )
            .clicked()
        {
            self.changed();
        }
        self.hlr_controls(ui);
        ui.separator();
        self.exports(ui);
        ui.separator();
        if self.busy {
            ui.horizontal(|ui| {
                ui.spinner();
                ui.label(format!("{:.1}s", self.started.elapsed().as_secs_f64()));
            });
        }
        ui.label(&self.phase);
        if self.pending.is_some() {
            ui.small("Latest view queued; intermediate changes coalesced");
        }
        if let Some(error) = &self.error {
            ui.colored_label(egui::Color32::DARK_RED, error);
        }
        ui.small("Native cancellation is queue-only. Stop process terminates active work; reconnect afterward.");
        ui.separator();
        ui.small(&self.preview.adapter);
        ui.small(format!("Model center: {:?} mm", self.preview.bounds.center));
    }

    fn exports(&mut self, ui: &mut egui::Ui) {
        ui.label("Export completed output");
        let current = !self.busy && self.completed == Some(self.revision);
        let mut export = None;
        ui.add_enabled_ui(current, |ui| {
            ui.horizontal_wrapped(|ui| {
                if ui.button("SVG").clicked() {
                    export = self.solution.as_ref().map(|s| Export::Svg(s.clone()));
                }
                if ui.button("Result JSON").clicked() {
                    export = self.solution.as_ref().map(|s| Export::Result(s.clone()));
                }
                if ui.button("Style JSON").clicked() {
                    export = self.solution.as_ref().map(|s| Export::Style(s.clone()));
                }
                if ui.button("HLR geometry JSON").clicked() {
                    export = self.solution.as_ref().map(|s| Export::Hlr(s.clone()));
                }
            });
        });
        if ui
            .add_enabled(
                !self.busy && self.model.is_some(),
                egui::Button::new("Mesh geometry JSON"),
            )
            .clicked()
        {
            export = self.model.as_ref().map(|model| Export::Mesh(model.clone()));
        }
        if let Some(snapshot) = export {
            self.begin("Exporting completed snapshot");
            self.jobs.export(snapshot);
        }
    }

    fn hlr_controls(&mut self, ui: &mut egui::Ui) {
        ui.label("Independent HLR geometry");
        let before = (
            self.detail_algorithm.clone(),
            self.outline_algorithm.clone(),
        );
        egui::ComboBox::from_id_salt("detail algorithm")
            .selected_text(format!("Detail: {:?}", self.detail_algorithm))
            .width(ui.available_width())
            .show_ui(ui, |ui| {
                for mode in [
                    HlrProjectionAlgorithm::Fast,
                    HlrProjectionAlgorithm::Poly,
                    HlrProjectionAlgorithm::Exact,
                ] {
                    ui.selectable_value(
                        &mut self.detail_algorithm,
                        mode.clone(),
                        format!("{mode:?}"),
                    );
                }
            });
        egui::ComboBox::from_id_salt("outline algorithm")
            .selected_text(format!("Shadow: {:?}", self.outline_algorithm))
            .width(ui.available_width())
            .show_ui(ui, |ui| {
                for mode in [
                    HlrOutlineAlgorithm::FastMeshShadow,
                    HlrOutlineAlgorithm::MeshShadow,
                    HlrOutlineAlgorithm::HlrClose,
                ] {
                    ui.selectable_value(
                        &mut self.outline_algorithm,
                        mode.clone(),
                        format!("{mode:?}"),
                    );
                }
            });
        if before
            != (
                self.detail_algorithm.clone(),
                self.outline_algorithm.clone(),
            )
        {
            self.changed();
        }
        ui.small("Fast is the default. Outline/detail remain separate; combined illustration/HLR SVG is a later API milestone.");
    }

    fn viewport(&mut self, ui: &mut egui::Ui) {
        ui.heading("3D • opaque hardware-depth preview");
        let size = ui.available_size().max(egui::vec2(1.0, 1.0));
        self.aspect = f64::from(size.x / size.y);
        let id = self
            .preview
            .render(&self.camera, size, ui.ctx().pixels_per_point());
        let response = ui.add(egui::Image::new((id, size)).sense(egui::Sense::drag()));
        let delta = ui.input(|input| input.pointer.delta());
        if response.dragged_by(egui::PointerButton::Primary) {
            self.camera.orbit([delta.x, delta.y]);
            self.changed();
        }
        if response.dragged_by(egui::PointerButton::Secondary)
            || response.dragged_by(egui::PointerButton::Middle)
        {
            self.camera.pan_pixels([delta.x, delta.y], size.y);
        }
        if response.hovered() {
            self.camera.zoom(
                ui.input(|input| input.smooth_scroll_delta.y),
                self.preview.bounds,
            );
        }
    }
}

impl eframe::App for App {
    fn ui(&mut self, ui: &mut egui::Ui, _frame: &mut eframe::Frame) {
        let ctx = ui.ctx().clone();
        self.events(&ctx);
        egui::Panel::top("job status")
            .resizable(false)
            .show(ui, |ui| {
                ui.horizontal_wrapped(|ui| {
                    if self.busy {
                        ui.spinner();
                        ui.label(format!("{:.1}s", self.started.elapsed().as_secs_f64()));
                    }
                    ui.label(&self.phase);
                    if self.pending.is_some() {
                        ui.label("• Latest view queued");
                    }
                    if self.error.is_some() {
                        ui.colored_label(egui::Color32::DARK_RED, "Error — see controls");
                    }
                });
            });
        let dock = if self.right_dock {
            egui::Panel::right("controls")
        } else {
            egui::Panel::left("controls")
        };
        dock.resizable(true)
            .default_size(290.0)
            .min_size(235.0)
            .max_size((ui.available_width() - 360.0).clamp(235.0, 500.0))
            .show(ui, |ui| {
                egui::ScrollArea::vertical().show(ui, |ui| self.controls(ui));
            });
        egui::Panel::left("preview")
            .resizable(true)
            .default_size(520.0)
            .min_size(180.0)
            .max_size((ui.available_width() - 180.0).max(180.0))
            .show(ui, |ui| self.viewport(ui));
        egui::CentralPanel::default().show(ui, |ui| {
            ui.horizontal_wrapped(|ui| {
                ui.selectable_value(&mut self.result_tab, 0, "Illustration SVG");
                ui.selectable_value(&mut self.result_tab, 1, "HLR shadow");
                ui.selectable_value(&mut self.result_tab, 2, "HLR detail");
            });
            if self.completed != Some(self.revision) && self.solution.is_some() {
                ui.label("Previous completed view — updating");
            }
            let texture = if self.result_tab == 0 {
                self.texture.as_ref()
            } else {
                self.hlr_textures
                    .as_ref()
                    .map(|textures| &textures[self.result_tab - 1])
            };
            if let Some(texture) = texture {
                ui.add(
                    egui::Image::new(texture).fit_to_exact_size(
                        texture.size_vec2()
                            * (ui.available_width() / texture.size_vec2().x)
                                .min(ui.available_height() / texture.size_vec2().y),
                    ),
                );
            } else {
                ui.label("Connect and open a STEP model to produce native illustration.");
            }
        });
        if self.busy || self.pending.is_some() {
            ctx.request_repaint_after(Duration::from_millis(50));
        }
        let evidence = self.solution.as_ref().map_or(String::new(), |solution| {
            format!(
                "adapter={} triangles={} svg_bytes={} hlr_bytes={}",
                self.preview.adapter,
                solution.result.stats.triangles,
                solution.result.svg.len(),
                solution.hlr_json.len()
            )
        });
        self.smoke.update(
            &ctx,
            self.completed == Some(self.revision) && self.solution.is_some(),
            self.error.as_deref(),
            &evidence,
        );
    }
}

impl Drop for App {
    fn drop(&mut self) {
        self.jobs.shutdown(self.client.take());
    }
}
