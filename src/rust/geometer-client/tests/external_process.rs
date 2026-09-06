use std::io;
use std::path::{Path, PathBuf};
use std::pin::Pin;
use std::process::Stdio;
use std::sync::Arc;
use std::sync::atomic::{AtomicBool, AtomicUsize, Ordering};
use std::task::{Context, Poll};
use std::time::Duration;

use geometer_client::contracts::{
    self, HlrViewSpec, MeshIllustrationInputA0, MeshIllustrationView,
};
use geometer_client::{
    GeometerClient, GeometerClientError, GeometerClientOptions, GeometerProcess,
    GeometerProcessController, GeometerProcessExit, ModelHlrProjectionRequest,
    ModelTessellationRequest,
};
use tokio::io::{AsyncRead, AsyncWrite, ReadBuf};
use tokio::process::{Child, Command};

#[derive(Default)]
struct Observations {
    terminate_calls: AtomicUsize,
    wait_calls: AtomicUsize,
    observed_exit: AtomicBool,
    controller_dropped: AtomicBool,
    allow_cleanup: AtomicBool,
}

struct ObservedTokioProcess {
    child: Child,
    observations: Arc<Observations>,
}

impl GeometerProcessController for ObservedTokioProcess {
    fn try_wait(&mut self) -> io::Result<Option<GeometerProcessExit>> {
        self.observations.wait_calls.fetch_add(1, Ordering::SeqCst);
        self.child.try_wait().map(|status| {
            status.map(|status| {
                self.observations
                    .observed_exit
                    .store(true, Ordering::SeqCst);
                GeometerProcessExit::new(status.success(), status.code().map(i64::from))
            })
        })
    }

    fn terminate(&mut self) -> io::Result<()> {
        self.observations
            .terminate_calls
            .fetch_add(1, Ordering::SeqCst);
        match self.child.try_wait()? {
            Some(_) => Ok(()),
            None => self.child.start_kill(),
        }
    }
}

impl Drop for ObservedTokioProcess {
    fn drop(&mut self) {
        let _ = self.terminate();
        self.observations
            .controller_dropped
            .store(true, Ordering::SeqCst);
    }
}

#[tokio::test]
async fn externally_launched_process_runs_composed_illustration_and_closes() {
    let root = repository_root();
    let (process, observations) = observed_process(native_executable(&root));
    let client = GeometerClient::from_process_with_options(
        process,
        "external-process-illustration-test",
        "a0",
        GeometerClientOptions::default(),
    )
    .await
    .unwrap();
    let step = tokio::fs::read(root.join("tests/fixtures/step/embedded_models/SOT-23.STEP"))
        .await
        .unwrap();
    let tessellation = client
        .model_tessellation(ModelTessellationRequest::step(step.clone()))
        .await
        .unwrap();
    let view = MeshIllustrationView {
        direction: [0.4, 0.7, 1.0],
        up: [0.0, 1.0, 0.0],
        mirror_x: None,
    };
    let mut options = contracts::decode_hlr_projection_options_a0_json(
        br#"{
          "projection_algorithm":"fast", "outline_algorithm":"fast-mesh-shadow",
          "curve_mode":"polyline", "strip_root_placement":true,
          "output_outline":true, "output_detail":true, "output_bbox":false,
          "fast":{"include_hidden":false,"crease_angle_rad":0.4363323129985824}
        }"#,
    )
    .unwrap();
    options.views = Some(vec![HlrViewSpec {
        id: "illustration".to_owned(),
        direction: view.direction,
        up: view.up,
    }]);
    let hlr = client
        .model_hlr_projection(ModelHlrProjectionRequest {
            model: step,
            media_type: "application/step".to_owned(),
            options,
        })
        .await
        .unwrap();
    let illustration = client
        .mesh_illustration_with_hlr(
            MeshIllustrationInputA0 {
                schema: "geometry.mesh_illustration.input.a0".to_owned(),
                meshes: tessellation.mesh_collection.meshes,
                view,
                prepare: None,
                style: None,
                svg: None,
            },
            hlr,
        )
        .await
        .unwrap();
    assert!(illustration.svg.contains("<svg"));
    assert!(illustration.stats.triangles > 0);
    client.close().await.unwrap();
    drop(client);
    wait_for(|| observations.controller_dropped.load(Ordering::SeqCst)).await;
    assert!(observations.wait_calls.load(Ordering::SeqCst) > 0);
    assert!(observations.observed_exit.load(Ordering::SeqCst));
}

#[tokio::test]
async fn last_public_handle_drop_terminates_reaps_and_drops_controller() {
    let (process, observations) = observed_process(native_executable(&repository_root()));
    let client = GeometerClient::from_process_with_options(
        process,
        "external-process-drop-test",
        "a0",
        GeometerClientOptions::default(),
    )
    .await
    .unwrap();
    let clone = client.clone();
    drop(client);
    assert_eq!(observations.terminate_calls.load(Ordering::SeqCst), 0);
    drop(clone);
    wait_for(|| {
        observations.observed_exit.load(Ordering::SeqCst)
            && observations.controller_dropped.load(Ordering::SeqCst)
    })
    .await;
    assert!(observations.terminate_calls.load(Ordering::SeqCst) > 0);
}

#[tokio::test]
async fn non_eof_stream_tasks_are_aborted_at_the_shared_cleanup_deadline() {
    let (process, observations) =
        observed_process_with_non_eof_streams(native_executable(&repository_root()));
    let client = GeometerClient::from_process_with_options(
        process,
        "external-process-stubborn-stream-test",
        "a0",
        GeometerClientOptions {
            handshake_timeout: Duration::from_secs(1),
            shutdown_timeout: Duration::from_millis(100),
            stderr_capture_limit: 1024,
        },
    )
    .await
    .unwrap();
    let started = tokio::time::Instant::now();
    let error = client.close().await.unwrap_err();
    assert!(error.to_string().contains("stream task cleanup exceeded"));
    assert!(started.elapsed() < Duration::from_secs(1));
    assert!(observations.observed_exit.load(Ordering::SeqCst));
    drop(client);
    wait_for(|| observations.controller_dropped.load(Ordering::SeqCst)).await;
}

#[tokio::test]
async fn cancelled_task_join_does_not_detach_stream_tasks() {
    let (process, observations) =
        observed_process_with_non_eof_streams(native_executable(&repository_root()));
    let client = GeometerClient::from_process_with_options(
        process,
        "external-process-cancelled-join-test",
        "a0",
        GeometerClientOptions {
            handshake_timeout: Duration::from_secs(1),
            shutdown_timeout: Duration::from_secs(5),
            stderr_capture_limit: 1024,
        },
    )
    .await
    .unwrap();
    let close_client = client.clone();
    let close_task = tokio::spawn(async move { close_client.close().await });
    wait_for(|| observations.observed_exit.load(Ordering::SeqCst)).await;
    assert!(!close_task.is_finished());
    close_task.abort();
    assert!(close_task.await.unwrap_err().is_cancelled());
    tokio::time::timeout(Duration::from_secs(1), client.close())
        .await
        .expect("retrying close exceeded its cleanup bound")
        .unwrap();
}

#[tokio::test]
async fn cancelled_shutdown_write_can_be_escalated_by_a_later_close() {
    let observations = Arc::new(Observations::default());
    let writes_blocked = Arc::new(AtomicBool::new(false));
    let mut child = spawn_child(native_executable(&repository_root()));
    let process = GeometerProcess::new(
        GatedWrite {
            inner: child.stdin.take().unwrap(),
            blocked: Arc::clone(&writes_blocked),
        },
        child.stdout.take().unwrap(),
        child.stderr.take().unwrap(),
        ObservedTokioProcess {
            child,
            observations: Arc::clone(&observations),
        },
    );
    let client = GeometerClient::from_process_with_options(
        process,
        "external-process-cancelled-write-test",
        "a0",
        GeometerClientOptions {
            handshake_timeout: Duration::from_secs(1),
            shutdown_timeout: Duration::from_secs(5),
            stderr_capture_limit: 1024,
        },
    )
    .await
    .unwrap();
    writes_blocked.store(true, Ordering::SeqCst);
    let close_client = client.clone();
    let close_task = tokio::spawn(async move { close_client.close().await });
    tokio::time::sleep(Duration::from_millis(25)).await;
    assert!(!close_task.is_finished());
    close_task.abort();
    assert!(close_task.await.unwrap_err().is_cancelled());
    tokio::time::timeout(Duration::from_secs(1), client.close())
        .await
        .expect("retrying close exceeded its cleanup bound")
        .unwrap();
    assert!(observations.terminate_calls.load(Ordering::SeqCst) > 0);
    assert!(observations.observed_exit.load(Ordering::SeqCst));
}

#[tokio::test]
async fn cleanup_errors_are_surfaced_and_a_later_close_retries() {
    let (process, observations) =
        process_with_error_once_controller(native_executable(&repository_root()));
    let client = GeometerClient::from_process_with_options(
        process,
        "external-process-cleanup-retry-test",
        "a0",
        GeometerClientOptions {
            handshake_timeout: Duration::from_secs(1),
            shutdown_timeout: Duration::from_millis(50),
            stderr_capture_limit: 1024,
        },
    )
    .await
    .unwrap();
    assert_eq!(observations.terminate_calls.load(Ordering::SeqCst), 0);
    let error = client.terminate().await.unwrap_err();
    assert!(
        error.to_string().contains("injected lifecycle failure"),
        "{error}"
    );
    observations.allow_cleanup.store(true, Ordering::SeqCst);
    client.close().await.unwrap();
    drop(client);
    wait_for(|| {
        observations.observed_exit.load(Ordering::SeqCst)
            && observations.controller_dropped.load(Ordering::SeqCst)
    })
    .await;
}

#[tokio::test]
async fn handshake_timeout_terminates_a_caller_supplied_process() {
    let observations = Arc::new(Observations::default());
    let (stdin, _server_stdin) = tokio::io::duplex(4096);
    let (_server_stdout, stdout) = tokio::io::duplex(4096);
    let (_server_stderr, stderr) = tokio::io::duplex(4096);
    let process = GeometerProcess::new(
        stdin,
        stdout,
        stderr,
        NeverExits {
            observations: Arc::clone(&observations),
        },
    );
    let result = GeometerClient::from_process_with_options(
        process,
        "external-process-timeout-test",
        "a0",
        GeometerClientOptions {
            handshake_timeout: Duration::from_millis(25),
            shutdown_timeout: Duration::from_secs(1),
            stderr_capture_limit: 1024,
        },
    )
    .await;
    assert!(
        matches!(result, Err(GeometerClientError::Process(message)) if message.contains("handshake timed out"))
    );
    wait_for(|| {
        observations.observed_exit.load(Ordering::SeqCst)
            && observations.controller_dropped.load(Ordering::SeqCst)
    })
    .await;
}

#[tokio::test]
async fn cancelled_construction_terminates_a_caller_supplied_process() {
    let observations = Arc::new(Observations::default());
    let (stdin, _server_stdin) = tokio::io::duplex(4096);
    let (_server_stdout, stdout) = tokio::io::duplex(4096);
    let (_server_stderr, stderr) = tokio::io::duplex(4096);
    let process = GeometerProcess::new(
        stdin,
        stdout,
        stderr,
        NeverExits {
            observations: Arc::clone(&observations),
        },
    );
    let task = tokio::spawn(GeometerClient::from_process_with_options(
        process,
        "external-process-cancel-test",
        "a0",
        GeometerClientOptions {
            handshake_timeout: Duration::from_secs(30),
            shutdown_timeout: Duration::from_secs(1),
            stderr_capture_limit: 1024,
        },
    ));
    tokio::time::sleep(Duration::from_millis(25)).await;
    task.abort();
    match task.await {
        Err(error) => assert!(error.is_cancelled()),
        Ok(_) => panic!("cancelled constructor unexpectedly completed"),
    }
    wait_for(|| {
        observations.observed_exit.load(Ordering::SeqCst)
            && observations.controller_dropped.load(Ordering::SeqCst)
    })
    .await;
}

struct NeverExits {
    observations: Arc<Observations>,
}

struct ErrorOnceProcess {
    child: Child,
    observations: Arc<Observations>,
}

impl GeometerProcessController for ErrorOnceProcess {
    fn try_wait(&mut self) -> io::Result<Option<GeometerProcessExit>> {
        if !self.observations.allow_cleanup.load(Ordering::SeqCst) {
            return Err(io::Error::other("injected lifecycle failure"));
        }
        self.observations.wait_calls.fetch_add(1, Ordering::SeqCst);
        self.child.try_wait().map(|status| {
            status.map(|status| {
                self.observations
                    .observed_exit
                    .store(true, Ordering::SeqCst);
                GeometerProcessExit::new(status.success(), status.code().map(i64::from))
            })
        })
    }

    fn terminate(&mut self) -> io::Result<()> {
        if !self.observations.allow_cleanup.load(Ordering::SeqCst) {
            return Err(io::Error::other("injected lifecycle failure"));
        }
        self.observations
            .terminate_calls
            .fetch_add(1, Ordering::SeqCst);
        match self.child.try_wait()? {
            Some(_) => Ok(()),
            None => self.child.start_kill(),
        }
    }
}

impl Drop for ErrorOnceProcess {
    fn drop(&mut self) {
        let _ = self.terminate();
        self.observations
            .controller_dropped
            .store(true, Ordering::SeqCst);
    }
}

impl GeometerProcessController for NeverExits {
    fn try_wait(&mut self) -> io::Result<Option<GeometerProcessExit>> {
        if self.observations.terminate_calls.load(Ordering::SeqCst) > 0 {
            self.observations
                .observed_exit
                .store(true, Ordering::SeqCst);
            Ok(Some(GeometerProcessExit::new(false, Some(1))))
        } else {
            Ok(None)
        }
    }

    fn terminate(&mut self) -> io::Result<()> {
        self.observations
            .terminate_calls
            .fetch_add(1, Ordering::SeqCst);
        Ok(())
    }
}

impl Drop for NeverExits {
    fn drop(&mut self) {
        self.observations
            .controller_dropped
            .store(true, Ordering::SeqCst);
    }
}

fn observed_process(executable: PathBuf) -> (GeometerProcess, Arc<Observations>) {
    let observations = Arc::new(Observations::default());
    let mut child = spawn_child(executable);
    let stdin = child.stdin.take().unwrap();
    let stdout = child.stdout.take().unwrap();
    let stderr = child.stderr.take().unwrap();
    let process = GeometerProcess::new(
        stdin,
        stdout,
        stderr,
        ObservedTokioProcess {
            child,
            observations: Arc::clone(&observations),
        },
    );
    (process, observations)
}

fn observed_process_with_non_eof_streams(
    executable: PathBuf,
) -> (GeometerProcess, Arc<Observations>) {
    let observations = Arc::new(Observations::default());
    let mut child = spawn_child(executable);
    let stdin = child.stdin.take().unwrap();
    let stdout = NeverEof::new(child.stdout.take().unwrap());
    let stderr = NeverEof::new(child.stderr.take().unwrap());
    let process = GeometerProcess::new(
        stdin,
        stdout,
        stderr,
        ObservedTokioProcess {
            child,
            observations: Arc::clone(&observations),
        },
    );
    (process, observations)
}

fn process_with_error_once_controller(executable: PathBuf) -> (GeometerProcess, Arc<Observations>) {
    let observations = Arc::new(Observations::default());
    let mut child = spawn_child(executable);
    let stdin = child.stdin.take().unwrap();
    let stdout = NeverEof::new(child.stdout.take().unwrap());
    let stderr = NeverEof::new(child.stderr.take().unwrap());
    let process = GeometerProcess::new(
        stdin,
        stdout,
        stderr,
        ErrorOnceProcess {
            child,
            observations: Arc::clone(&observations),
        },
    );
    (process, observations)
}

struct NeverEof<R> {
    inner: R,
}

struct GatedWrite<W> {
    inner: W,
    blocked: Arc<AtomicBool>,
}

impl<W: AsyncWrite + Unpin> AsyncWrite for GatedWrite<W> {
    fn poll_write(
        mut self: Pin<&mut Self>,
        context: &mut Context<'_>,
        buffer: &[u8],
    ) -> Poll<io::Result<usize>> {
        if self.blocked.load(Ordering::SeqCst) {
            Poll::Pending
        } else {
            Pin::new(&mut self.inner).poll_write(context, buffer)
        }
    }

    fn poll_flush(mut self: Pin<&mut Self>, context: &mut Context<'_>) -> Poll<io::Result<()>> {
        if self.blocked.load(Ordering::SeqCst) {
            Poll::Pending
        } else {
            Pin::new(&mut self.inner).poll_flush(context)
        }
    }

    fn poll_shutdown(mut self: Pin<&mut Self>, context: &mut Context<'_>) -> Poll<io::Result<()>> {
        if self.blocked.load(Ordering::SeqCst) {
            Poll::Pending
        } else {
            Pin::new(&mut self.inner).poll_shutdown(context)
        }
    }
}

impl<R> NeverEof<R> {
    fn new(inner: R) -> Self {
        Self { inner }
    }
}

impl<R: AsyncRead + Unpin> AsyncRead for NeverEof<R> {
    fn poll_read(
        mut self: Pin<&mut Self>,
        context: &mut Context<'_>,
        buffer: &mut ReadBuf<'_>,
    ) -> Poll<io::Result<()>> {
        let filled_before = buffer.filled().len();
        match Pin::new(&mut self.inner).poll_read(context, buffer) {
            Poll::Ready(Ok(())) if buffer.filled().len() == filled_before => Poll::Pending,
            result => result,
        }
    }
}

fn spawn_child(executable: PathBuf) -> Child {
    Command::new(executable)
        .args(["serve", "--stdio"])
        .stdin(Stdio::piped())
        .stdout(Stdio::piped())
        .stderr(Stdio::piped())
        .kill_on_drop(true)
        .spawn()
        .unwrap()
}

async fn wait_for(mut condition: impl FnMut() -> bool) {
    tokio::time::timeout(Duration::from_secs(5), async {
        while !condition() {
            tokio::time::sleep(Duration::from_millis(10)).await;
        }
    })
    .await
    .expect("condition did not become true");
}

fn native_executable(root: &Path) -> PathBuf {
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
    root.join("dist/native")
        .join(platform)
        .join(if cfg!(windows) {
            "geometer.exe"
        } else {
            "geometer"
        })
}

fn repository_root() -> PathBuf {
    Path::new(env!("CARGO_MANIFEST_DIR"))
        .join("../../..")
        .canonicalize()
        .unwrap()
}
