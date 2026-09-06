//! Process and stream ownership boundary for executable-backed clients.

use std::fmt;
use std::io;
use std::path::Path;
use std::process::Stdio;
use std::time::Duration;

use tokio::io::{AsyncRead, AsyncWrite};
use tokio::process::{Child, Command};

use crate::client::GeometerClientError;

pub(crate) type BoxAsyncRead = Box<dyn AsyncRead + Send + Unpin + 'static>;
pub(crate) type BoxAsyncWrite = Box<dyn AsyncWrite + Send + Unpin + 'static>;

const DEFAULT_CONSTRUCTION_CLEANUP_TIMEOUT: Duration = Duration::from_secs(31);
const PROCESS_POLL_INTERVAL: Duration = Duration::from_millis(10);

const DEFAULT_HANDSHAKE_TIMEOUT: Duration = Duration::from_secs(10);
const DEFAULT_SHUTDOWN_TIMEOUT: Duration = Duration::from_secs(31);
const DEFAULT_STDERR_CAPTURE_LIMIT: usize = 1024 * 1024;
const MAX_CLIENT_TIMEOUT: Duration = Duration::from_secs(5 * 60);
const MAX_STDERR_CAPTURE_LIMIT: usize = 16 * 1024 * 1024;

/// Bounded lifecycle and diagnostic settings for an executable client.
#[derive(Clone, Copy, Debug, Eq, PartialEq)]
pub struct GeometerClientOptions {
    /// Maximum time allowed for hello/welcome negotiation.
    pub handshake_timeout: Duration,
    /// Shared deadline for graceful shutdown and forced process cleanup.
    pub shutdown_timeout: Duration,
    /// Maximum number of stderr bytes retained for diagnostics.
    pub stderr_capture_limit: usize,
}

impl Default for GeometerClientOptions {
    fn default() -> Self {
        Self {
            handshake_timeout: DEFAULT_HANDSHAKE_TIMEOUT,
            shutdown_timeout: DEFAULT_SHUTDOWN_TIMEOUT,
            stderr_capture_limit: DEFAULT_STDERR_CAPTURE_LIMIT,
        }
    }
}

impl GeometerClientOptions {
    pub(crate) fn validate(self) -> Result<Self, GeometerClientError> {
        if self.handshake_timeout.is_zero()
            || self.handshake_timeout > MAX_CLIENT_TIMEOUT
            || self.shutdown_timeout.is_zero()
            || self.shutdown_timeout > MAX_CLIENT_TIMEOUT
            || self.stderr_capture_limit > MAX_STDERR_CAPTURE_LIMIT
        {
            return Err(GeometerClientError::Protocol(
                "Geometer client options exceed their supported bounds".to_owned(),
            ));
        }
        Ok(self)
    }
}

/// Portable exit information returned after an externally managed process or
/// containment unit has been reaped.
#[derive(Clone, Copy, Debug, Eq, PartialEq)]
pub struct GeometerProcessExit {
    success: bool,
    code: Option<i64>,
}

impl GeometerProcessExit {
    /// Describe a completed process using the platform's success decision and
    /// optional numeric exit code.
    #[must_use]
    pub const fn new(success: bool, code: Option<i64>) -> Self {
        Self { success, code }
    }

    #[must_use]
    pub const fn success(self) -> bool {
        self.success
    }

    #[must_use]
    pub const fn code(self) -> Option<i64> {
        self.code
    }
}

impl fmt::Display for GeometerProcessExit {
    fn fmt(&self, formatter: &mut fmt::Formatter<'_>) -> fmt::Result {
        match self.code {
            Some(code) => write!(formatter, "exit code {code}"),
            None if self.success => formatter.write_str("successful exit"),
            None => formatter.write_str("unsuccessful exit without a numeric code"),
        }
    }
}

/// Lifecycle control retained by `GeometerClient` after a caller transfers a
/// contained process and its three standard streams.
///
/// `try_wait` and `terminate` must return promptly and must not perform a
/// blocking process wait. `try_wait` returns `Some` only after the controller's
/// full containment unit has been reaped or proven empty. `terminate` must be
/// idempotent. `Drop` must at least request termination because an async runtime
/// can disappear while a client is being dropped. Production containment
/// controllers should make `Drop` synchronously prove their process tree empty;
/// the convenient Tokio controller can only delegate final reaping to Tokio's
/// process runtime.
pub trait GeometerProcessController: Send + 'static {
    fn try_wait(&mut self) -> io::Result<Option<GeometerProcessExit>>;
    fn terminate(&mut self) -> io::Result<()>;
}

/// A caller-launched Geometer process whose IPC streams and lifecycle are
/// transferred atomically into `GeometerClient`.
///
/// Dropping this value before a successful client handshake requests process
/// termination. The controller's own `Drop` implementation remains the final
/// cleanup authority.
pub struct GeometerProcess {
    stdin: Option<BoxAsyncWrite>,
    stdout: Option<BoxAsyncRead>,
    stderr: Option<BoxAsyncRead>,
    controller: Option<Box<dyn GeometerProcessController>>,
    cleanup_timeout: Duration,
}

impl GeometerProcess {
    #[must_use]
    pub fn new<I, O, E, C>(stdin: I, stdout: O, stderr: E, controller: C) -> Self
    where
        I: AsyncWrite + Send + Unpin + 'static,
        O: AsyncRead + Send + Unpin + 'static,
        E: AsyncRead + Send + Unpin + 'static,
        C: GeometerProcessController,
    {
        Self {
            stdin: Some(Box::new(stdin)),
            stdout: Some(Box::new(stdout)),
            stderr: Some(Box::new(stderr)),
            controller: Some(Box::new(controller)),
            cleanup_timeout: DEFAULT_CONSTRUCTION_CLEANUP_TIMEOUT,
        }
    }

    pub(crate) fn set_cleanup_timeout(&mut self, timeout: Duration) {
        self.cleanup_timeout = timeout;
    }

    pub(crate) fn stdin_mut(&mut self) -> &mut BoxAsyncWrite {
        self.stdin.as_mut().expect("constructed process owns stdin")
    }

    pub(crate) fn stdout_mut(&mut self) -> &mut BoxAsyncRead {
        self.stdout
            .as_mut()
            .expect("constructed process owns stdout")
    }

    pub(crate) fn into_parts(
        mut self,
    ) -> (
        BoxAsyncWrite,
        BoxAsyncRead,
        BoxAsyncRead,
        Box<dyn GeometerProcessController>,
    ) {
        (
            self.stdin.take().expect("constructed process owns stdin"),
            self.stdout.take().expect("constructed process owns stdout"),
            self.stderr.take().expect("constructed process owns stderr"),
            self.controller
                .take()
                .expect("constructed process owns lifecycle control"),
        )
    }
}

impl Drop for GeometerProcess {
    fn drop(&mut self) {
        let Some(mut controller) = self.controller.take() else {
            return;
        };
        let _ = controller.terminate();
        if let Ok(runtime) = tokio::runtime::Handle::try_current() {
            let timeout = self.cleanup_timeout;
            runtime.spawn(async move {
                let deadline = tokio::time::Instant::now() + timeout;
                loop {
                    if !matches!(controller.try_wait(), Ok(None)) {
                        return;
                    }
                    let now = tokio::time::Instant::now();
                    if now >= deadline {
                        return;
                    }
                    tokio::time::sleep((deadline - now).min(PROCESS_POLL_INTERVAL)).await;
                }
            });
        }
    }
}

pub(crate) fn spawn_default(executable: &Path) -> io::Result<GeometerProcess> {
    let mut command = Command::new(executable);
    // Executable-backed desktop consumers must not create a console window.
    #[cfg(windows)]
    command.creation_flags(0x0800_0000); // CREATE_NO_WINDOW
    let mut child = command
        .args(["serve", "--stdio"])
        .stdin(Stdio::piped())
        .stdout(Stdio::piped())
        .stderr(Stdio::piped())
        .kill_on_drop(true)
        .spawn()?;
    let stdin = child
        .stdin
        .take()
        .ok_or_else(|| io::Error::other("child stdin is unavailable"))?;
    let stdout = child
        .stdout
        .take()
        .ok_or_else(|| io::Error::other("child stdout is unavailable"))?;
    let stderr = child
        .stderr
        .take()
        .ok_or_else(|| io::Error::other("child stderr is unavailable"))?;
    Ok(GeometerProcess::new(
        stdin,
        stdout,
        stderr,
        TokioProcessController { child },
    ))
}

struct TokioProcessController {
    child: Child,
}

impl GeometerProcessController for TokioProcessController {
    fn try_wait(&mut self) -> io::Result<Option<GeometerProcessExit>> {
        self.child.try_wait().map(|status| {
            status.map(|status| {
                GeometerProcessExit::new(status.success(), status.code().map(i64::from))
            })
        })
    }

    fn terminate(&mut self) -> io::Result<()> {
        match self.child.try_wait()? {
            Some(_) => Ok(()),
            None => self.child.start_kill(),
        }
    }
}

impl Drop for TokioProcessController {
    fn drop(&mut self) {
        let _ = self.terminate();
    }
}
