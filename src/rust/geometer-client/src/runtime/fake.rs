//! In-process fake workers for host tests (feature `test-support`).
//!
//! [`super::Transport::Fake`] starts workers that answer operations from a
//! host-supplied [`FakeWorker`] instead of a Geometer process. Everything else
//! is the real runtime: the pool, identity agreement, the optional pin,
//! [`super::Worker::run`] failure classification (a Geometer `Operation`
//! error is local; any other client error is fatal and retires the worker),
//! both fatal policies, and the typed [`super::WorkerClient`] methods, which
//! encode requests and decode responses exactly as they do for a process.
//! Production hosts never need this module.

use std::future::Future;
use std::pin::Pin;
use std::sync::Arc;

use super::ArtifactIdentity;
use crate::backend::OperationBackend;
use crate::client::{GeometerClientError, OperationResponse};
use crate::ipc::Attachment;

/// The operations of one fake worker.
pub trait FakeWorker: Send + Sync {
    /// Answer one operation as a worker would. Return
    /// [`GeometerClientError::Operation`] for a request-local failure and any
    /// other error to simulate a transport, protocol or process failure.
    fn execute(
        &self,
        operation: &str,
        request_json: &[u8],
        attachments: Vec<Attachment>,
    ) -> Result<OperationResponse, GeometerClientError>;

    /// Called when the runtime terminates this worker after a fatal failure.
    fn terminate(&self) {}

    /// Called when the runtime closes this worker.
    fn close(&self) {}
}

/// Starts one fake worker: the operations it answers and the identity it
/// reports. Called once per worker start; an error is a start failure.
pub type FakeStarter =
    Arc<dyn Fn() -> Result<(Arc<dyn FakeWorker>, ArtifactIdentity), String> + Send + Sync>;

/// A started fake worker as the runtime holds it.
pub(crate) struct FakeConnection {
    worker: Arc<dyn FakeWorker>,
}

impl FakeConnection {
    pub(crate) fn new(worker: Arc<dyn FakeWorker>) -> Self {
        Self { worker }
    }

    pub(crate) fn terminate(&self) {
        self.worker.terminate();
    }

    pub(crate) fn close(&self) {
        self.worker.close();
    }
}

impl OperationBackend for FakeConnection {
    fn execute_operation<'a>(
        &'a self,
        operation: &'a str,
        request_json: &'a [u8],
        attachments: Vec<Attachment>,
    ) -> Pin<Box<dyn Future<Output = Result<OperationResponse, GeometerClientError>> + Send + 'a>>
    {
        Box::pin(async move { self.worker.execute(operation, request_json, attachments) })
    }

    fn poison_protocol(
        &self,
        message: String,
    ) -> Pin<Box<dyn Future<Output = GeometerClientError> + Send + '_>> {
        Box::pin(async move { GeometerClientError::Protocol(message) })
    }
}
