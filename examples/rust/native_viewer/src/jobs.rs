//! All model IO, native calls, conversion, SVG rasterization and export run off the UI thread.
use crate::{mesh, raster};
use eframe::egui;
use geometer_client::{
    GeometerClient, GeometerClientError, ModelHlrProjectionRequest, ModelTessellationRequest,
    contracts::*,
};
use std::{
    path::PathBuf,
    sync::{
        Arc,
        atomic::{AtomicU64, Ordering},
        mpsc,
    },
    time::Duration,
};
use tokio::runtime::Runtime;

pub struct Model {
    pub path: PathBuf,
    pub collection: MeshCollectionA0,
    pub geometry_json: Vec<u8>,
    pub step: Vec<u8>,
    pub mesh_options: ModelTessellationRequestA0,
}

pub struct Solution {
    pub result: MeshIllustrationResultA0,
    pub result_json: Vec<u8>,
    pub style_json: Vec<u8>,
    pub image: egui::ColorImage,
    pub hlr_json: Vec<u8>,
    pub hlr_images: [egui::ColorImage; 2],
}

pub enum Event {
    Phase(&'static str),
    Connected(Result<GeometerClient, String>),
    Loaded(Result<(Arc<Model>, mesh::MeshData), String>),
    Solved(u64, Result<Arc<Solution>, String>),
    FilePicked(bool, Option<PathBuf>),
    Exported(Result<(), String>),
    Stopped(Result<(), String>),
}

pub enum Export {
    Svg(Arc<Solution>),
    Result(Arc<Solution>),
    Style(Arc<Solution>),
    Hlr(Arc<Solution>),
    Mesh(Arc<Model>),
}

impl Export {
    pub(crate) fn data(&self) -> (&'static str, &[u8]) {
        match self {
            Self::Svg(s) => ("illustration.svg", s.result.svg.as_bytes()),
            Self::Result(s) => ("illustration.a0.json", &s.result_json),
            Self::Style(s) => ("illustration-style.a0.json", &s.style_json),
            Self::Hlr(s) => ("hlr-projection.a0.json", &s.hlr_json),
            Self::Mesh(m) => ("mesh-collection.a0.json", &m.geometry_json),
        }
    }
}

pub struct Jobs {
    runtime: Option<Runtime>,
    sender: mpsc::Sender<(u64, Event)>,
    pub events: mpsc::Receiver<(u64, Event)>,
    epoch: AtomicU64,
    context: egui::Context,
}

fn error(error: GeometerClientError) -> String {
    match error {
        GeometerClientError::Operation {
            operation,
            diagnostics,
        } => format!("{operation}: {diagnostics:?}"),
        other => other.to_string(),
    }
}

impl Jobs {
    pub fn new(context: egui::Context) -> Result<Self, std::io::Error> {
        let (sender, events) = mpsc::channel();
        Ok(Self {
            runtime: Some(Runtime::new()?),
            sender,
            events,
            epoch: AtomicU64::new(0),
            context,
        })
    }

    fn emit(&self) -> impl Fn(Event) + Send + 'static {
        let sender = self.sender.clone();
        let context = self.context.clone();
        let epoch = self.epoch();
        move |event| {
            let _ = sender.send((epoch, event));
            context.request_repaint();
        }
    }

    pub fn epoch(&self) -> u64 {
        self.epoch.load(Ordering::Relaxed)
    }

    pub fn connect(&self, path: PathBuf) {
        let emit = self.emit();
        self.runtime.as_ref().unwrap().spawn(async move {
            let result = tokio::time::timeout(
                Duration::from_secs(15),
                GeometerClient::spawn(path, "geometer-native-viewer", env!("CARGO_PKG_VERSION")),
            )
            .await
            .map_err(|_| "Executable handshake timed out (15 seconds)".to_owned())
            .and_then(|result| result.map_err(error));
            emit(Event::Connected(result));
        });
    }

    pub fn load(
        &self,
        client: GeometerClient,
        path: PathBuf,
        options: ModelTessellationRequestA0,
        existing: Option<Arc<Model>>,
    ) {
        let emit = self.emit();
        self.runtime.as_ref().unwrap().spawn(async move {
            let result = async {
                emit(Event::Phase("Reading STEP"));
                if existing.is_none()
                    && tokio::fs::metadata(&path)
                        .await
                        .map_err(|e| e.to_string())?
                        .len()
                        > 256 * 1024 * 1024
                {
                    return Err("STEP exceeds the 256 MiB native attachment limit".to_owned());
                }
                let bytes = if let Some(model) = existing {
                    model.step.clone()
                } else {
                    tokio::fs::read(&path).await.map_err(|e| e.to_string())?
                };
                emit(Event::Phase("Geometer: colored tessellation"));
                let tessellation = client
                    .model_tessellation(ModelTessellationRequest {
                        model: bytes.clone(),
                        options: options.clone(),
                    })
                    .await
                    .map_err(error)?;
                emit(Event::Phase("Preparing GPU mesh"));
                tokio::task::spawn_blocking(move || {
                    let collection = tessellation.mesh_collection;
                    let mesh = mesh::prepare(&collection)?;
                    let geometry_json =
                        encode_mesh_collection_a0_json(&collection).map_err(|e| e.to_string())?;
                    Ok((
                        Arc::new(Model {
                            path,
                            collection,
                            geometry_json,
                            step: bytes,
                            mesh_options: options,
                        }),
                        mesh,
                    ))
                })
                .await
                .map_err(|e| e.to_string())?
            }
            .await;
            emit(Event::Loaded(result));
        });
    }

    pub fn solve(
        &self,
        revision: u64,
        client: GeometerClient,
        model: Arc<Model>,
        view: MeshIllustrationView,
        style: MeshIllustrationStyleA0,
        mut options: HlrProjectionOptionsA0,
    ) {
        let emit = self.emit();
        self.runtime.as_ref().unwrap().spawn(async move {
            let result = async {
                options.output_outline = Some(style.show_hlr_outline.unwrap_or(true));
                options.output_detail = Some(style.show_hlr_detail.unwrap_or(false));
                let hlr = if options.output_outline == Some(true)
                    || options.output_detail == Some(true)
                {
                    emit(Event::Phase("Geometer: HLR detail / shadow"));
                    Some(
                        client
                            .model_hlr_projection(ModelHlrProjectionRequest {
                                model: model.step.clone(),
                                media_type: "application/step".into(),
                                options,
                            })
                            .await
                            .map_err(error)?,
                    )
                } else {
                    None // Pure fills must not depend on unused HLR computation succeeding.
                };
                emit(Event::Phase("Geometer: native illustration"));
                let input = MeshIllustrationInputA0 {
                    schema: "geometry.mesh_illustration.input.a0".into(),
                    meshes: model.collection.meshes.clone(),
                    view,
                    prepare: None,
                    style: Some(style.clone()),
                    svg: None,
                };
                let result = if let Some(hlr) = &hlr {
                    client.mesh_illustration_with_hlr(input, hlr.clone()).await
                } else {
                    client.mesh_illustration(input).await
                }
                .map_err(error)?;
                emit(Event::Phase("Rasterizing SVG preview"));
                tokio::task::spawn_blocking(move || {
                    let image = raster::rasterize(&result.svg)?;
                    let result_json = encode_mesh_illustration_result_a0_json(&result)
                        .map_err(|e| e.to_string())?;
                    let style_json = encode_mesh_illustration_style_a0_json(&style)
                        .map_err(|e| e.to_string())?;
                    let (hlr_json, hlr_images) = if let Some(hlr) = hlr {
                        (
                            encode_hlr_projection_result_a0_json(&hlr)
                                .map_err(|e| e.to_string())?,
                            crate::hlr::images(&hlr, &style)?,
                        )
                    } else {
                        (
                            Vec::new(),
                            std::array::from_fn(|_| {
                                egui::ColorImage::filled([1, 1], egui::Color32::WHITE)
                            }),
                        )
                    };
                    Ok(Arc::new(Solution {
                        result,
                        result_json,
                        style_json,
                        image,
                        hlr_json,
                        hlr_images,
                    }))
                })
                .await
                .map_err(|e| e.to_string())?
            }
            .await;
            emit(Event::Solved(revision, result));
        });
    }

    pub fn pick(&self, executable: bool) {
        let emit = self.emit();
        self.runtime.as_ref().unwrap().spawn(async move {
            let mut dialog = rfd::AsyncFileDialog::new();
            if !executable {
                dialog = dialog.add_filter("STEP model", &["step", "stp"]);
            }
            emit(Event::FilePicked(
                executable,
                dialog
                    .pick_file()
                    .await
                    .map(|file| file.path().to_path_buf()),
            ));
        });
    }

    pub fn export(&self, snapshot: Export) {
        let emit = self.emit();
        self.runtime.as_ref().unwrap().spawn(async move {
            let (name, bytes) = snapshot.data();
            let result = if let Some(file) = rfd::AsyncFileDialog::new()
                .set_file_name(name)
                .save_file()
                .await
            {
                file.write(bytes).await.map_err(|e| e.to_string())
            } else {
                Ok(())
            };
            emit(Event::Exported(result));
        });
    }

    pub fn stop(&self, client: GeometerClient) {
        self.epoch.fetch_add(1, Ordering::Relaxed);
        let emit = self.emit();
        self.runtime.as_ref().unwrap().spawn(async move {
            emit(Event::Stopped(client.terminate().await.map_err(error)));
        });
    }

    pub fn shutdown(&mut self, client: Option<GeometerClient>) {
        if let Some(runtime) = self.runtime.take() {
            if let Some(client) = client {
                let _ = runtime.block_on(async move {
                    tokio::time::timeout(Duration::from_secs(2), client.terminate()).await
                });
            }
            runtime.shutdown_background();
        }
    }
}

impl Drop for Jobs {
    fn drop(&mut self) {
        self.shutdown(None);
    }
}

#[cfg(test)]
mod tests {
    use super::*;

    #[test]
    fn epoch_invalidates_all_old_event_kinds() {
        let jobs = Jobs::new(egui::Context::default()).unwrap();
        let old_emitter = jobs.emit();
        jobs.epoch.fetch_add(1, Ordering::Relaxed);
        old_emitter(Event::Phase("late phase"));
        old_emitter(Event::Loaded(Err("late load".into())));
        old_emitter(Event::Solved(7, Err("late solve".into())));
        for _ in 0..3 {
            let (epoch, _) = jobs.events.try_recv().unwrap();
            assert_ne!(epoch, jobs.epoch());
        }
        jobs.emit()(Event::Stopped(Ok(())));
        assert_eq!(jobs.events.try_recv().unwrap().0, jobs.epoch());
    }
}
