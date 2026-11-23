//! The supervised runtime against the platform executable: lazy verified
//! workers, typed calls through the transport-neutral client, the release pin,
//! deadlines, and both fatal policies.

use std::path::{Path, PathBuf};
use std::sync::Arc;
use std::sync::atomic::{AtomicUsize, Ordering};
use std::time::Duration;

use geometer_client::ModelHlrProjectionRequest;
use geometer_client::contracts::{self, HlrProjectionOptionsA0};
use geometer_client::runtime::{
    ExecutableSource, FatalPolicy, GeometerRuntime, RuntimeConfig, RuntimePin, Transport,
};

fn repository_root() -> PathBuf {
    Path::new(env!("CARGO_MANIFEST_DIR"))
        .join("../../..")
        .canonicalize()
        .unwrap()
}

fn native_executable() -> PathBuf {
    if let Some(path) = std::env::var_os("GEOMETER_EXECUTABLE") {
        return PathBuf::from(path);
    }
    let platform = if cfg!(windows) {
        "windows-x64"
    } else if cfg!(target_os = "macos") && cfg!(target_arch = "aarch64") {
        "macos-arm64"
    } else if cfg!(target_os = "linux") && cfg!(target_arch = "aarch64") {
        "linux-arm64"
    } else {
        "linux-x64"
    };
    let name = if cfg!(windows) {
        "geometer.exe"
    } else {
        "geometer"
    };
    repository_root()
        .join("dist/native")
        .join(platform)
        .join(name)
}

fn step() -> Vec<u8> {
    std::fs::read(repository_root().join("tests/fixtures/step/embedded_models/SOT-23.STEP"))
        .unwrap()
}

fn options() -> HlrProjectionOptionsA0 {
    contracts::decode_hlr_projection_options_a0_json(
        br#"{"projection_algorithm":"fast","outline_algorithm":"fast-mesh-shadow",
            "output_outline":true,"output_detail":false,"output_bbox":false,
            "views":[{"id":"top","direction":[0.0,0.0,1.0],"up":[0.0,1.0,0.0]}]}"#,
    )
    .unwrap()
}

fn config(policy: FatalPolicy) -> RuntimeConfig {
    let mut config = RuntimeConfig::new(Transport::Process(ExecutableSource::Explicit(
        native_executable(),
    )));
    config.workers = 2;
    config.on_fatal = policy;
    config.client_name = "runtime-test".into();
    config
}

fn project(runtime: &GeometerRuntime, model: Vec<u8>) -> Result<usize, String> {
    let lease = runtime.lease(|| ())?;
    lease
        .worker()
        .run("Geometer projection", |client| async move {
            client
                .model_hlr_projection(ModelHlrProjectionRequest {
                    model,
                    media_type: "model/step".into(),
                    options: options(),
                })
                .await
        })
        .map(|result| result.views[0].modes.outline.segments.len())
        .map_err(|failure| failure.message)
}

#[test]
fn lazy_workers_report_one_identity_and_operation_errors_stay_local() {
    let starts = Arc::new(AtomicUsize::new(0));
    let observed = Arc::clone(&starts);
    let mut config = config(FatalPolicy::Latch);
    config.transport = Transport::Process(ExecutableSource::Resolver(Arc::new(move || {
        observed.fetch_add(1, Ordering::SeqCst);
        Ok(native_executable())
    })));
    let runtime = GeometerRuntime::new(config).unwrap();
    assert_eq!(starts.load(Ordering::SeqCst), 0);
    assert!(project(&runtime, step()).unwrap() > 0);
    let lease = runtime.lease(|| ()).unwrap();
    assert!(!lease.started());
    let identity = lease.worker().identity().clone();
    assert!(identity.artifact_sha256.starts_with("sha256:"));
    let result = identity.result_identity(
        "geometry.model_hlr_projection.a0",
        "geometry.hlr_projection.result.a0",
    );
    assert_eq!(
        result.geometer_abi,
        format!("geometer.browser_c_abi.v{}", identity.c_abi_generation)
    );
    let local = lease
        .worker()
        .run("Geometer projection", |client| async move {
            client
                .model_hlr_projection(ModelHlrProjectionRequest {
                    model: b"not a STEP file".to_vec(),
                    media_type: "model/step".into(),
                    options: options(),
                })
                .await
        })
        .unwrap_err();
    assert!(!local.fatal, "{}", local.message);
    assert!(!lease.worker().is_broken());
    drop(lease);
    assert_eq!(starts.load(Ordering::SeqCst), 1);
    runtime.finish().unwrap();
    assert!(runtime.lease(|| ()).is_err());
}

#[test]
fn a_mismatched_pin_refuses_the_worker() {
    let mut config = config(FatalPolicy::Latch);
    config.pin = Some(RuntimePin::new("1999.1.1", 19_990_101));
    let runtime = GeometerRuntime::new(config).unwrap();
    let error = runtime.lease(|| ()).map(|_| ()).unwrap_err();
    assert!(
        error.contains("is incompatible; expected 1999.1.1/19990101"),
        "{error}"
    );
    runtime.finish().unwrap();
}

#[test]
fn a_deadline_is_fatal_and_latches_the_runtime() {
    let mut config = config(FatalPolicy::Latch);
    config.deadline = Duration::from_millis(1);
    let runtime = GeometerRuntime::new(config).unwrap();
    let lease = runtime.lease(|| ()).unwrap();
    let failure = lease
        .worker()
        .run("Geometer projection", |client| async move {
            client
                .model_hlr_projection(ModelHlrProjectionRequest {
                    model: step(),
                    media_type: "model/step".into(),
                    options: options(),
                })
                .await
        })
        .unwrap_err();
    assert!(failure.fatal);
    assert!(
        failure
            .message
            .starts_with("Geometer projection deadline exceeded")
    );
    assert!(lease.worker().is_broken());
    drop(lease);
    // The broken worker latches the runtime when its lease ends, before
    // (and whether or not) the host reports the failure.
    assert_eq!(runtime.fatal(), Some(failure.message.as_str()));
    runtime.fail("a later report keeps the first cause");
    assert_eq!(runtime.fatal(), Some(failure.message.as_str()));
    assert!(runtime.lease(|| ()).is_err());
    assert!(runtime.finish().is_err());
}

#[test]
fn respawn_replaces_a_retired_worker_and_keeps_serving() {
    let runtime = GeometerRuntime::new(config(FatalPolicy::Respawn)).unwrap();
    let lease = runtime.lease(|| ()).unwrap();
    let first = lease.worker().identity().clone();
    lease.worker().mark_broken("host rejected a result");
    drop(lease);
    assert_eq!(
        runtime.fail("host rejected a result"),
        "host rejected a result"
    );
    assert!(runtime.fatal().is_none());
    let lease = runtime.lease(|| ()).unwrap();
    assert!(lease.started(), "the retired worker must be replaced");
    assert!(lease.worker().identity().same_artifact(&first));
    drop(lease);
    assert!(project(&runtime, step()).unwrap() > 0);
    runtime.finish().unwrap();
}

#[test]
fn four_verified_workers_share_one_identity_and_finish_drains_an_active_lease() {
    let mut config = config(FatalPolicy::Latch);
    config.workers = 4;
    let runtime = GeometerRuntime::new(config).unwrap();
    let starts = AtomicUsize::new(0);
    let leases: Vec<_> = (0..4)
        .map(|_| {
            runtime
                .lease(|| starts.fetch_add(1, Ordering::SeqCst))
                .unwrap()
        })
        .collect();
    assert!(leases.iter().all(|lease| lease.started()));
    assert_eq!(starts.load(Ordering::SeqCst), 4);
    let identity = leases[0].worker().identity().clone();
    assert!(
        leases
            .iter()
            .all(|lease| lease.worker().identity() == &identity)
    );
    drop(leases);
    let lease = runtime
        .lease(|| panic!("an idle verified worker must be reused"))
        .unwrap();
    assert!(!lease.started());
    std::thread::scope(|scope| {
        let (done_tx, done_rx) = std::sync::mpsc::channel();
        let runtime = &runtime;
        scope.spawn(move || done_tx.send(runtime.finish()).unwrap());
        assert!(
            done_rx.recv_timeout(Duration::from_secs(2)).is_err(),
            "finish must drain the active lease"
        );
        drop(lease);
        done_rx
            .recv_timeout(Duration::from_secs(30))
            .unwrap()
            .unwrap();
    });
    assert!(runtime.lease(|| ()).is_err());
}

#[test]
fn a_start_past_its_deadline_is_refused_without_latching() {
    let mut config = config(FatalPolicy::Latch);
    config.startup_deadline = Duration::from_millis(1);
    let runtime = GeometerRuntime::new(config).unwrap();
    let error = runtime.lease(|| ()).map(|_| ()).unwrap_err();
    assert!(
        error.contains("deadline") || error.contains("timed out"),
        "{error}"
    );
    assert!(
        runtime.fatal().is_none(),
        "start failures are the host's call"
    );
    runtime.finish().unwrap();
}
