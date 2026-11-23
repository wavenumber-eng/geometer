//! One verified, persistent worker: its own current-thread runtime and client.
//!
//! Every blocking step (start, call, termination, close and drop) runs where
//! Tokio permits blocking: inline on a plain thread, or on a short-lived
//! helper thread when the caller is inside a Tokio runtime context (an async
//! task, or a `spawn_blocking` thread, which also carries a runtime handle).
//! Workers therefore never panic inside async hosts, but a call still blocks
//! its calling thread until it ends; async hosts call from `spawn_blocking`.

use std::fs::File;
use std::future::Future;
use std::path::Path;
use std::pin::Pin;
use std::sync::OnceLock;
use std::time::Duration;

use tokio::runtime::Runtime;

use super::RuntimeConfig;
use super::identity::{ArtifactIdentity, file_digest, open_artifact};
use crate::backend::OperationBackend;
use crate::client::{GeometerClient, GeometerClientError, OperationResponse};
use crate::ipc::Attachment;
use crate::process::GeometerClientOptions;

/// A failed worker call. Fatal failures mean the worker is no longer usable:
/// it has been retired and its process terminated (or termination attempted).
#[derive(Clone, Debug, PartialEq, Eq)]
#[non_exhaustive]
pub struct Failure {
    pub message: String,
    pub fatal: bool,
}

impl Failure {
    /// A failure that ends the worker.
    pub fn fatal(message: impl Into<String>) -> Self {
        Self {
            message: message.into(),
            fatal: true,
        }
    }

    /// A failure local to one request; the worker stays usable.
    pub fn local(message: impl Into<String>) -> Self {
        Self {
            message: message.into(),
            fatal: false,
        }
    }
}

/// The typed operation surface of a leased worker, independent of transport.
///
/// It has the same typed methods as [`GeometerClient`] (for example
/// `model_hlr_projection`, `model_illustration_geometry_b0` and
/// `mesh_illustration_geometry`), so callers do not change when further
/// transports are added.
#[derive(Clone, Copy)]
pub struct WorkerClient<'a> {
    backend: Backend<'a>,
}

#[derive(Clone, Copy)]
enum Backend<'a> {
    Process(&'a GeometerClient),
    #[cfg(any(test, feature = "test-support"))]
    Fake(&'a super::fake::FakeConnection),
}

impl OperationBackend for WorkerClient<'_> {
    fn execute_operation<'a>(
        &'a self,
        operation: &'a str,
        request_json: &'a [u8],
        attachments: Vec<Attachment>,
    ) -> Pin<Box<dyn Future<Output = Result<OperationResponse, GeometerClientError>> + Send + 'a>>
    {
        match self.backend {
            Backend::Process(client) => {
                client.execute_operation(operation, request_json, attachments)
            }
            #[cfg(any(test, feature = "test-support"))]
            Backend::Fake(fake) => fake.execute_operation(operation, request_json, attachments),
        }
    }

    fn poison_protocol(
        &self,
        message: String,
    ) -> Pin<Box<dyn Future<Output = GeometerClientError> + Send + '_>> {
        match self.backend {
            Backend::Process(client) => OperationBackend::poison_protocol(client, message),
            #[cfg(any(test, feature = "test-support"))]
            Backend::Fake(fake) => fake.poison_protocol(message),
        }
    }
}

/// How a started worker reaches Geometer.
enum Connection {
    Process(GeometerClient),
    #[cfg(any(test, feature = "test-support"))]
    Fake(super::fake::FakeConnection),
}

impl Connection {
    fn client(&self) -> WorkerClient<'_> {
        let backend = match self {
            Self::Process(client) => Backend::Process(client),
            #[cfg(any(test, feature = "test-support"))]
            Self::Fake(fake) => Backend::Fake(fake),
        };
        WorkerClient { backend }
    }

    fn terminate(&self, runtime: &Runtime) -> Result<(), String> {
        match self {
            Self::Process(client) => runtime
                .block_on(client.terminate())
                .map_err(|error| error.to_string()),
            #[cfg(any(test, feature = "test-support"))]
            Self::Fake(fake) => {
                fake.terminate();
                Ok(())
            }
        }
    }

    fn close(self, runtime: &Runtime) -> Result<(), String> {
        match self {
            Self::Process(client) => runtime
                .block_on(client.close())
                .map_err(|error| error.to_string()),
            #[cfg(any(test, feature = "test-support"))]
            Self::Fake(fake) => {
                fake.close();
                Ok(())
            }
        }
    }
}

/// A started worker. It is exclusively owned by one lease while it runs a call.
///
/// Its methods block the calling thread; see the module documentation for
/// async hosts.
pub struct Worker {
    runtime: Option<Runtime>,
    connection: Option<Connection>,
    _artifact: Option<File>,
    identity: ArtifactIdentity,
    deadline: Duration,
    broken: OnceLock<String>,
}

impl Worker {
    /// Start one worker from `path`. The executable is held open with writes
    /// denied, hashed, spawned, checked against the optional pin, and hashed
    /// again; any difference refuses the worker. Spawning and the handshake
    /// together must finish within the configured startup deadline.
    pub fn start(path: &Path, config: &RuntimeConfig) -> Result<Self, String> {
        outside_runtime_context(|| start_process(path, config))
    }

    #[cfg(any(test, feature = "test-support"))]
    pub(crate) fn start_fake(
        starter: &super::fake::FakeStarter,
        config: &RuntimeConfig,
    ) -> Result<Self, String> {
        outside_runtime_context(|| {
            let (fake, identity) = starter()?;
            if let Err(error) = verify_pin(
                &identity.release_version,
                identity.c_abi_generation,
                config.pin.as_ref(),
            ) {
                fake.close();
                return Err(error);
            }
            let connection = super::fake::FakeConnection::new(fake);
            Ok(Self {
                runtime: Some(worker_runtime()?),
                connection: Some(Connection::Fake(connection)),
                _artifact: None,
                identity,
                deadline: config.deadline,
                broken: OnceLock::new(),
            })
        })
    }

    /// The executable this worker started and verified.
    pub fn identity(&self) -> &ArtifactIdentity {
        &self.identity
    }

    /// Run one call under the configured deadline.
    ///
    /// A Geometer `Operation` error is local. Any other client error, or the
    /// deadline (reported as `"{operation} deadline exceeded"`), terminates the
    /// process, retires the worker and returns a fatal failure. A retired or
    /// closed worker refuses every call with a fatal failure.
    pub fn run<'w, T, F, Fut>(&'w self, operation: &str, call: F) -> Result<T, Failure>
    where
        F: FnOnce(WorkerClient<'w>) -> Fut,
        Fut: Future<Output = Result<T, GeometerClientError>> + Send,
        T: Send,
    {
        if let Some(reason) = self.broken_reason() {
            return Err(Failure::fatal(format!(
                "Geometer worker was retired: {reason}"
            )));
        }
        let (Some(runtime), Some(connection)) = (self.runtime.as_ref(), self.connection.as_ref())
        else {
            let failure = Failure::fatal("Geometer worker is closed");
            self.mark_broken(failure.message.clone());
            return Err(failure);
        };
        let deadline = self.deadline;
        let future = call(connection.client());
        let result = outside_runtime_context(|| {
            runtime.block_on(async { tokio::time::timeout(deadline, future).await })
        });
        match result {
            Ok(Ok(value)) => Ok(value),
            Ok(Err(error)) => Err(self.client_failure(runtime, connection, error)),
            Err(_) => Err(self.retire(
                runtime,
                connection,
                format!("{operation} deadline exceeded"),
            )),
        }
    }

    /// Retire this worker so that it never serves again, for example because
    /// the host rejected its result. The first reason is kept. When the lease
    /// ends the pool closes it; under [`super::FatalPolicy::Latch`] the
    /// runtime then latches with this reason, and under
    /// [`super::FatalPolicy::Respawn`] the next lease starts a replacement.
    pub fn mark_broken(&self, reason: impl Into<String>) {
        let _ = self.broken.set(reason.into());
    }

    /// Whether a fatal failure or the host has retired this worker.
    pub fn is_broken(&self) -> bool {
        self.broken.get().is_some()
    }

    /// Why this worker was retired, if it was.
    pub fn broken_reason(&self) -> Option<&str> {
        self.broken.get().map(String::as_str)
    }

    fn client_failure(
        &self,
        runtime: &Runtime,
        connection: &Connection,
        error: GeometerClientError,
    ) -> Failure {
        if matches!(error, GeometerClientError::Operation { .. }) {
            return Failure::local(error.to_string());
        }
        self.retire(runtime, connection, error.to_string())
    }

    /// Terminate the process and retire the worker with the final message.
    fn retire(&self, runtime: &Runtime, connection: &Connection, mut message: String) -> Failure {
        if let Err(error) = outside_runtime_context(|| connection.terminate(runtime)) {
            message.push_str(&format!("; termination failed: {error}"));
        }
        self.mark_broken(message.clone());
        Failure::fatal(message)
    }

    /// Close the process gracefully. Later calls report the worker closed.
    pub fn close(&mut self) -> Result<(), String> {
        let (Some(runtime), Some(connection)) = (self.runtime.as_ref(), self.connection.take())
        else {
            return Ok(());
        };
        outside_runtime_context(|| connection.close(runtime))
    }
}

impl Drop for Worker {
    fn drop(&mut self) {
        let runtime = self.runtime.take();
        let connection = self.connection.take();
        // Closing and dropping the runtime both block; neither may run on an
        // async runtime thread.
        outside_runtime_context(move || {
            if let (Some(runtime), Some(connection)) = (runtime.as_ref(), connection) {
                let _ = connection.close(runtime);
            }
            drop(runtime);
        });
    }
}

/// Run blocking work inline, or on a short-lived helper thread when the
/// caller is inside a Tokio runtime context, where `Runtime::block_on` and
/// dropping a runtime would panic. A panic in the work is resumed here.
fn outside_runtime_context<R: Send>(work: impl FnOnce() -> R + Send) -> R {
    if tokio::runtime::Handle::try_current().is_err() {
        return work();
    }
    std::thread::scope(|scope| match scope.spawn(work).join() {
        Ok(value) => value,
        Err(payload) => std::panic::resume_unwind(payload),
    })
}

fn worker_runtime() -> Result<Runtime, String> {
    tokio::runtime::Builder::new_current_thread()
        .enable_all()
        .build()
        .map_err(|error| error.to_string())
}

fn start_process(path: &Path, config: &RuntimeConfig) -> Result<Worker, String> {
    let path = path.canonicalize().map_err(|error| error.to_string())?;
    let mut artifact = open_artifact(&path)?;
    let digest = file_digest(&mut artifact)?;
    let runtime = worker_runtime()?;
    let client = runtime.block_on(spawn_client(&path, config))?;
    let welcome = client.welcome();
    let validation = verify_pin(
        &welcome.release_version,
        welcome.c_abi_generation,
        config.pin.as_ref(),
    )
    .and_then(|()| {
        let mut current = File::open(&path).map_err(|error| error.to_string())?;
        if file_digest(&mut current)? != digest {
            return Err("Geometer executable changed during startup".into());
        }
        Ok(())
    });
    if let Err(mut error) = validation {
        if let Err(cleanup) = runtime.block_on(client.close()) {
            error.push_str(&format!("; Geometer cleanup failed: {cleanup}"));
        }
        return Err(error);
    }
    let identity = ArtifactIdentity::new(
        path,
        welcome.release_version.clone(),
        welcome.c_abi_generation,
        format!("sha256:{digest}"),
    );
    Ok(Worker {
        runtime: Some(runtime),
        connection: Some(Connection::Process(client)),
        _artifact: Some(artifact),
        identity,
        deadline: config.deadline,
        broken: OnceLock::new(),
    })
}

/// Spawn and handshake within the startup deadline. Dropping the unfinished
/// spawn on expiry requests termination of the child.
async fn spawn_client(path: &Path, config: &RuntimeConfig) -> Result<GeometerClient, String> {
    let options = GeometerClientOptions {
        handshake_timeout: config.startup_deadline,
        ..GeometerClientOptions::default()
    };
    bounded_start(
        config.startup_deadline,
        GeometerClient::spawn_with_options(
            path,
            &config.client_name,
            &config.client_version,
            options,
        ),
    )
    .await
}

/// Finish a worker start within `deadline`, or drop it and report the deadline.
pub(super) async fn bounded_start<T>(
    deadline: Duration,
    start: impl Future<Output = Result<T, GeometerClientError>>,
) -> Result<T, String> {
    match tokio::time::timeout(deadline, start).await {
        Ok(started) => started.map_err(|error| error.to_string()),
        Err(_) => Err(format!(
            "Geometer worker start exceeded its {} ms deadline",
            deadline.as_millis()
        )),
    }
}

fn verify_pin(
    release_version: &str,
    c_abi_generation: u32,
    pin: Option<&super::RuntimePin>,
) -> Result<(), String> {
    let Some(pin) = pin else {
        return Ok(());
    };
    if release_version != pin.release_version || c_abi_generation != pin.c_abi_generation {
        return Err(format!(
            "Geometer {release_version}/{c_abi_generation} is incompatible; expected {}/{}",
            pin.release_version, pin.c_abi_generation
        ));
    }
    Ok(())
}

impl super::pool::PoolWorker for Worker {
    type Identity = (String, u32, String);

    fn identity(&self) -> Self::Identity {
        (
            self.identity.release_version.clone(),
            self.identity.c_abi_generation,
            self.identity.artifact_sha256.clone(),
        )
    }

    fn broken_reason(&self) -> Option<String> {
        Worker::broken_reason(self).map(str::to_owned)
    }

    fn close(&mut self) -> Result<(), String> {
        Worker::close(self)
    }
}
