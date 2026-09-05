#![cfg_attr(target_os = "windows", windows_subsystem = "windows")]

mod app;
mod camera;
mod gpu;
mod hlr;
mod jobs;
mod mesh;
mod raster;
mod result_view;
mod settings;
#[cfg(test)]
mod settings_tests;
mod smoke;

fn main() -> eframe::Result {
    let mut executable = geometer_client::GeometerClient::find_executable();
    let mut step = None;
    let mut smoke = smoke::Smoke::default();
    let mut arguments = std::env::args_os().skip(1);
    while let Some(argument) = arguments.next() {
        match argument.to_str() {
            Some("--geometer") => executable = arguments.next().map(Into::into),
            Some("--step") => step = arguments.next().map(Into::into),
            Some("--smoke") => smoke.enabled = true,
            Some("--smoke-screenshot") => {
                smoke.enabled = true;
                smoke.screenshot = arguments.next().map(Into::into);
            }
            _ => {
                return Err(eframe::Error::AppCreation("usage: geometer-native-viewer [--geometer PATH] [--step MODEL] [--smoke] [--smoke-screenshot PNG]".into()));
            }
        }
    }
    let smoke_enabled = smoke.enabled;
    let smoke_passed = smoke.passed.clone();
    eframe::run_native(
        "Geometer • Native Rust API Lab",
        eframe::NativeOptions {
            renderer: eframe::Renderer::Wgpu,
            viewport: eframe::egui::ViewportBuilder::default()
                .with_inner_size([1440.0, 920.0])
                .with_min_inner_size([760.0, 520.0]),
            ..Default::default()
        },
        Box::new(move |context| Ok(Box::new(app::App::new(context, executable, step, smoke)?))),
    )?;
    if smoke_enabled && !smoke_passed.load(std::sync::atomic::Ordering::Relaxed) {
        return Err(eframe::Error::AppCreation(
            "Native viewer smoke failed".into(),
        ));
    }
    Ok(())
}
