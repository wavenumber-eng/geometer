//! Supervised Geometer workers for long-running and batch hosts.
//!
//! A [`GeometerRuntime`] owns a bounded pool of verified worker processes. It
//! starts nothing until the first lease. Each worker holds its executable open
//! with writes denied, checks an optional release/ABI pin, and must agree with
//! every other worker on its [`ArtifactIdentity`]. A lease gives exclusive use
//! of one worker; [`Worker::run`] executes typed operations through a
//! transport-neutral [`WorkerClient`] under a deadline. A broken worker never
//! serves again: when its lease ends it is closed, and the runtime either
//! latches ([`FatalPolicy::Latch`]) or starts a replacement on the next lease
//! ([`FatalPolicy::Respawn`]), whether or not the host reported the failure.
//!
//! The runtime is blocking. Lease and call from ordinary threads; async hosts
//! call from `tokio::task::spawn_blocking` (or another blocking thread). A
//! worker never panics inside a Tokio runtime context, because it drives its
//! private runtime on a helper thread there, but its calls still block the
//! calling thread.
//!
//! [`ResultCache`] (and, with the `disk-cache` feature, `DiskResultCache`)
//! keep successful opaque results keyed by the caller's request digest plus
//! the exact [`ResultIdentity`] of the producing artifact. This module knows
//! nothing about any application's data model.

#[cfg(feature = "disk-cache")]
mod disk_cache;
#[cfg(any(test, feature = "test-support"))]
mod fake;
mod identity;
mod pool;
#[cfg(test)]
mod pool_tests;
mod result_cache;
#[cfg(test)]
mod tests;
mod worker;

use std::fmt;
use std::path::PathBuf;
use std::sync::Arc;
use std::time::Duration;

#[cfg(feature = "disk-cache")]
pub use disk_cache::{DiskCacheLayout, DiskCacheLimits, DiskResultCache};
#[cfg(feature = "test-support")]
pub use fake::{FakeStarter, FakeWorker};
pub use identity::{ArtifactIdentity, ResultIdentity};
pub use result_cache::{ResultCache, ResultCacheStats};
pub use worker::{Failure, Worker, WorkerClient};

/// The default per-call deadline.
pub const DEFAULT_DEADLINE: Duration = Duration::from_secs(120);
/// The default deadline for spawning a process worker and completing its
/// handshake; it matches the client's default handshake bound.
pub const DEFAULT_STARTUP_DEADLINE: Duration = Duration::from_secs(10);
/// The longest supported startup deadline (the client's handshake maximum).
pub const MAX_STARTUP_DEADLINE: Duration = Duration::from_secs(5 * 60);
/// The largest supported pool.
pub const MAX_WORKERS: usize = 64;

/// An exact release version and C ABI generation every worker must report.
#[derive(Clone, Debug, PartialEq, Eq)]
pub struct RuntimePin {
    pub release_version: String,
    pub c_abi_generation: u32,
}

impl RuntimePin {
    pub fn new(release_version: impl Into<String>, c_abi_generation: u32) -> Self {
        Self {
            release_version: release_version.into(),
            c_abi_generation,
        }
    }
}

/// Resolves the executable lazily, once per worker start.
pub type ExecutableResolver = Arc<dyn Fn() -> Result<PathBuf, String> + Send + Sync>;

/// Where a process worker's executable comes from. Further sources may be
/// added; callers that match on this enum must keep a wildcard.
#[derive(Clone)]
#[non_exhaustive]
pub enum ExecutableSource {
    /// This exact path.
    Explicit(PathBuf),
    /// [`crate::GeometerClient::find_executable`]: `GEOMETER_EXECUTABLE`, an
    /// executable sibling, then the source checkout's grouped `dist/native`.
    Discover,
    /// A host-owned resolver, for example one that materializes a bundle.
    Resolver(ExecutableResolver),
}

impl ExecutableSource {
    /// Resolve the executable path now.
    pub fn resolve(&self) -> Result<PathBuf, String> {
        match self {
            Self::Explicit(path) => Ok(path.clone()),
            Self::Discover => crate::GeometerClient::find_executable().ok_or_else(|| {
                "compatible Geometer executable not found; provide GEOMETER_EXECUTABLE".into()
            }),
            Self::Resolver(resolve) => resolve(),
        }
    }
}

impl fmt::Debug for ExecutableSource {
    fn fmt(&self, formatter: &mut fmt::Formatter<'_>) -> fmt::Result {
        match self {
            Self::Explicit(path) => formatter.debug_tuple("Explicit").field(path).finish(),
            Self::Discover => formatter.write_str("Discover"),
            Self::Resolver(_) => formatter.write_str("Resolver(..)"),
        }
    }
}

/// How workers reach Geometer. Further transports (an application serving as
/// its own single-file worker, or in-process static linking) are planned as
/// additional variants; callers that match on this enum must keep a wildcard.
#[derive(Clone)]
#[non_exhaustive]
pub enum Transport {
    /// A separate Geometer executable per worker, over executable IPC.
    Process(ExecutableSource),
    /// In-process fake workers for host tests (feature `test-support`).
    #[cfg(any(test, feature = "test-support"))]
    Fake(fake::FakeStarter),
}

impl fmt::Debug for Transport {
    fn fmt(&self, formatter: &mut fmt::Formatter<'_>) -> fmt::Result {
        match self {
            Self::Process(source) => formatter.debug_tuple("Process").field(source).finish(),
            #[cfg(any(test, feature = "test-support"))]
            Self::Fake(_) => formatter.write_str("Fake(..)"),
        }
    }
}

/// What a fatal worker failure does to the runtime.
#[derive(Clone, Copy, Debug, Default, PartialEq, Eq)]
pub enum FatalPolicy {
    /// The first fatal failure fails every later lease (batch jobs).
    #[default]
    Latch,
    /// Only the failed worker is retired; the next lease starts a replacement
    /// (interactive hosts).
    Respawn,
}

/// Runtime settings. [`RuntimeConfig::new`] supplies the defaults; adjust
/// fields on the returned value. Settings may be added, so the type is
/// non-exhaustive.
#[derive(Clone, Debug)]
#[non_exhaustive]
pub struct RuntimeConfig {
    pub transport: Transport,
    /// Pool size, `1..=MAX_WORKERS`.
    pub workers: usize,
    pub pin: Option<RuntimePin>,
    /// Per-call deadline; exceeding it terminates the worker.
    pub deadline: Duration,
    /// Deadline for spawning a process worker and completing its handshake,
    /// up to [`MAX_STARTUP_DEADLINE`]. Starts are serialized under the pool
    /// lock, so this also bounds how long a hung start blocks other leases.
    /// A host [`ExecutableSource::Resolver`] runs before it and is unbounded.
    pub startup_deadline: Duration,
    pub on_fatal: FatalPolicy,
    /// Sent in the IPC hello.
    pub client_name: String,
    /// Sent in the IPC hello.
    pub client_version: String,
}

impl RuntimeConfig {
    /// One worker, no pin, [`DEFAULT_DEADLINE`], [`DEFAULT_STARTUP_DEADLINE`],
    /// [`FatalPolicy::Latch`].
    pub fn new(transport: Transport) -> Self {
        Self {
            transport,
            workers: 1,
            pin: None,
            deadline: DEFAULT_DEADLINE,
            startup_deadline: DEFAULT_STARTUP_DEADLINE,
            on_fatal: FatalPolicy::Latch,
            client_name: "geometer-client".into(),
            client_version: env!("CARGO_PKG_VERSION").into(),
        }
    }
}

/// A bounded, lazily started, supervised worker pool.
///
/// Its methods block; async hosts call them from `spawn_blocking`.
pub struct GeometerRuntime {
    config: RuntimeConfig,
    pool: pool::WorkerPool<Worker>,
}

/// Exclusive use of one worker until dropped.
pub struct WorkerLease<'a> {
    inner: pool::Lease<'a, Worker>,
}

impl WorkerLease<'_> {
    pub fn worker(&self) -> &Worker {
        self.inner.worker()
    }

    /// Whether this lease started its worker.
    pub fn started(&self) -> bool {
        self.inner.started
    }
}

impl GeometerRuntime {
    /// Validate the settings. No process starts until the first lease.
    pub fn new(config: RuntimeConfig) -> Result<Self, String> {
        if !(1..=MAX_WORKERS).contains(&config.workers) {
            return Err(format!(
                "Geometer workers must be between 1 and {MAX_WORKERS}"
            ));
        }
        if config.deadline.is_zero() {
            return Err("Geometer call deadline must be positive".into());
        }
        if config.startup_deadline.is_zero() || config.startup_deadline > MAX_STARTUP_DEADLINE {
            return Err(format!(
                "Geometer startup deadline must be positive and at most {} s",
                MAX_STARTUP_DEADLINE.as_secs()
            ));
        }
        Ok(Self {
            pool: pool::WorkerPool::new(config.workers, config.on_fatal),
            config,
        })
    }

    pub fn config(&self) -> &RuntimeConfig {
        &self.config
    }

    pub fn workers(&self) -> usize {
        self.config.workers
    }

    /// Lease an idle worker or start one. `on_start` runs only when a worker
    /// is started, before the executable is resolved; its result is held until
    /// startup finishes (for example a timing span). Start errors are not
    /// latched here; the host decides with [`Self::fail`].
    ///
    /// When the lease ends, a broken worker (see [`Worker::mark_broken`]) is
    /// closed instead of returning to the pool; under [`FatalPolicy::Latch`]
    /// the runtime latches with its reason.
    pub fn lease<S>(&self, on_start: impl FnOnce() -> S) -> Result<WorkerLease<'_>, String> {
        let inner = self.pool.acquire(|| {
            let _observed = on_start();
            match &self.config.transport {
                Transport::Process(source) => Worker::start(&source.resolve()?, &self.config),
                #[cfg(any(test, feature = "test-support"))]
                Transport::Fake(starter) => Worker::start_fake(starter, &self.config),
            }
        })?;
        Ok(WorkerLease { inner })
    }

    /// Report a fatal failure. Under [`FatalPolicy::Latch`] the first message
    /// is kept and every later lease fails with it; under
    /// [`FatalPolicy::Respawn`] nothing is latched. Waiting leases are woken.
    /// A failure caused by a leased worker should also retire it with
    /// [`Worker::mark_broken`]; [`Worker::run`] does so itself for its fatal
    /// failures.
    pub fn fail(&self, message: impl Into<String>) -> String {
        match self.config.on_fatal {
            FatalPolicy::Latch => self.latch(message),
            FatalPolicy::Respawn => {
                self.pool.wake();
                message.into()
            }
        }
    }

    /// Latch a fatal failure regardless of policy, for unrecoverable host state.
    pub fn latch(&self, message: impl Into<String>) -> String {
        let message = message.into();
        self.pool.latch(message.clone());
        message
    }

    /// The latched failure, if any.
    pub fn fatal(&self) -> Option<&str> {
        self.pool.fatal()
    }

    /// Close the pool after active leases end, close every worker, and report
    /// the latched failure and any cleanup failure. Later leases fail.
    pub fn finish(&self) -> Result<(), String> {
        let cleanup = self.pool.finish();
        match (self.pool.fatal(), cleanup) {
            (Some(fatal), Err(cleanup)) => {
                Err(format!("{fatal}; Geometer cleanup failed: {cleanup}"))
            }
            (Some(fatal), Ok(())) => Err(fatal.to_owned()),
            (None, Err(cleanup)) => Err(format!("Geometer cleanup failed: {cleanup}")),
            (None, Ok(())) => Ok(()),
        }
    }
}
