use std::collections::HashMap;
use std::path::Path;
use std::process::Stdio;
use std::sync::Arc;
use std::sync::atomic::{AtomicBool, AtomicU64, Ordering};
use std::time::Duration;

use tokio::io::AsyncReadExt;
use tokio::process::{Child, ChildStdin, Command};
use tokio::sync::{Mutex, oneshot};

use crate::generated::contracts::{
    self, AnalyticPlanarBooleanBatchRequestA0, AnalyticPlanarBooleanBatchResultA0,
    DiagnosticCategory, IpcCancelRejectedA0, IpcCancelledA0, IpcHelloA0, IpcRequestA0,
    IpcRequestValueA0, IpcRuntimeDispatchA0, IpcWelcomeA0, ModelBoundsOptionsA0,
    ModelBoundsResultA0, OperationOutcomeA0, OperationResultValueA0, PackedAttachmentProjectionA0,
    PackedAttachmentReferenceA0,
};
use crate::generated::operations::ANALYTIC_PLANAR_BOOLEAN_BATCH_A0_IDENTITY;
use crate::ipc::{self, Attachment, Frame, FrameKind};
use crate::operation_validation::{
    operation_declaration, validate_operation_request, validate_operation_response,
};
use crate::session_validation::{
    discover_executable, encode_reason, validate_effective_request, validate_welcome,
};
use crate::{
    AnalyticPacketError, IPC_IDENTITY, decode_analytic_planar_boolean_batch_result_a0_packet,
    encode_analytic_planar_boolean_batch_request_a0_packet,
};

#[cfg(test)]
#[path = "client_tests.rs"]
mod tests;

const STDERR_CAPTURE_LIMIT: usize = 1024 * 1024;

/// Generated executable IPC A0 welcome contract retained under the pilot facade name.
pub type Welcome = IpcWelcomeA0;

#[derive(Debug, thiserror::Error)]
pub enum GeometerClientError {
    #[error(transparent)]
    Contract(#[from] contracts::ContractError),
    #[error(transparent)]
    Frame(#[from] ipc::FrameError),
    #[error(transparent)]
    AnalyticPacket(#[from] AnalyticPacketError),
    #[error("Geometer IPC protocol failed: {0}")]
    Protocol(String),
    #[error("Geometer process failed: {0}")]
    Process(String),
    #[error("Geometer operation {operation} failed")]
    Operation {
        operation: String,
        diagnostics: Vec<contracts::DiagnosticA0>,
    },
    #[error("Geometer request was cancelled before execution")]
    Cancelled,
    #[error(
        "Geometer request timed out locally (queued cancellation accepted: {queued_cancelled})"
    )]
    Timeout { queued_cancelled: bool },
    #[error("Geometer client is closing or closed")]
    Closed,
}

#[derive(Clone, Debug)]
pub struct ModelBoundsRequest {
    pub model: Vec<u8>,
    pub media_type: String,
    pub options: ModelBoundsOptionsA0,
}

impl ModelBoundsRequest {
    pub fn step(model: Vec<u8>) -> Self {
        Self {
            model,
            media_type: "application/step".to_owned(),
            options: ModelBoundsOptionsA0 {
                format: None,
                model_transform: None,
            },
        }
    }
}

#[derive(Clone, Debug)]
pub struct OperationResponse {
    pub outcome: OperationOutcomeA0,
    pub attachments: Vec<Attachment>,
}

struct PendingRequest {
    operation: String,
    resident_bytes: usize,
    sender: oneshot::Sender<Result<Frame, PendingFailure>>,
}

#[derive(Clone)]
enum PendingFailure {
    Protocol(String),
    Process(String),
}

impl PendingFailure {
    fn into_client(self) -> GeometerClientError {
        match self {
            Self::Protocol(message) => GeometerClientError::Protocol(message),
            Self::Process(message) => GeometerClientError::Process(message),
        }
    }
}

struct ShutdownWaiter {
    sender: oneshot::Sender<Result<(), PendingFailure>>,
    pending_at_request: usize,
    rejected_queued: u32,
    active_eligible: bool,
}

struct Inner {
    stdin: Mutex<Option<ChildStdin>>,
    child: Mutex<Child>,
    pending: Mutex<HashMap<u64, PendingRequest>>,
    cancellation: Mutex<HashMap<u64, oneshot::Sender<Result<bool, PendingFailure>>>>,
    shutdown: Mutex<Option<ShutdownWaiter>>,
    stderr: Mutex<Vec<u8>>,
    next_request_id: AtomicU64,
    closing: AtomicBool,
    closed: AtomicBool,
    welcome: Arc<IpcWelcomeA0>,
}

#[derive(Clone)]
pub struct GeometerClient {
    inner: Arc<Inner>,
    welcome: Arc<IpcWelcomeA0>,
}

pub struct OperationCall {
    client: GeometerClient,
    operation: String,
    request_id: u64,
    receiver: oneshot::Receiver<Result<Frame, PendingFailure>>,
}

impl OperationCall {
    pub fn request_id(&self) -> u64 {
        self.request_id
    }

    pub async fn cancel(&self, reason: Option<&str>) -> Result<bool, GeometerClientError> {
        self.client.cancel(self.request_id, reason).await
    }

    pub async fn wait(self) -> Result<OperationResponse, GeometerClientError> {
        let operation = self.operation;
        let frame = self
            .receiver
            .await
            .map_err(|_| GeometerClientError::Process("response channel closed".to_owned()))?
            .map_err(PendingFailure::into_client)?;
        decode_operation_response(&self.client.welcome, &operation, frame)
    }

    pub async fn wait_timeout(
        mut self,
        duration: Duration,
    ) -> Result<OperationResponse, GeometerClientError> {
        match tokio::time::timeout(duration, &mut self.receiver).await {
            Ok(received) => {
                let frame = received
                    .map_err(|_| {
                        GeometerClientError::Process("response channel closed".to_owned())
                    })?
                    .map_err(PendingFailure::into_client)?;
                decode_operation_response(&self.client.welcome, &self.operation, frame)
            }
            Err(_) => {
                let queued_cancelled = self
                    .client
                    .cancel(self.request_id, Some("local client timeout"))
                    .await?;
                Err(GeometerClientError::Timeout { queued_cancelled })
            }
        }
    }
}

fn decode_operation_response(
    welcome: &IpcWelcomeA0,
    operation: &str,
    frame: Frame,
) -> Result<OperationResponse, GeometerClientError> {
    if frame.kind == FrameKind::Cancelled {
        return Err(GeometerClientError::Cancelled);
    }
    if frame.kind != FrameKind::Response {
        return Err(GeometerClientError::Protocol(format!(
            "expected response for {operation}, received {:?}",
            frame.kind
        )));
    }
    let outcome = contracts::decode_operation_outcome_a0_json(&frame.json)?;
    if outcome_operation(&outcome) != operation {
        return Err(GeometerClientError::Protocol(
            "response operation does not match its request".to_owned(),
        ));
    }
    validate_operation_response(welcome, operation, &outcome, &frame.attachments)?;
    Ok(OperationResponse {
        outcome,
        attachments: frame.attachments,
    })
}

impl GeometerClient {
    pub async fn spawn(
        executable: impl AsRef<Path>,
        client_name: &str,
        client_version: &str,
    ) -> Result<Self, GeometerClientError> {
        let mut child = Command::new(executable.as_ref())
            .args(["serve", "--stdio"])
            .stdin(Stdio::piped())
            .stdout(Stdio::piped())
            .stderr(Stdio::piped())
            .kill_on_drop(true)
            .spawn()
            .map_err(|error| GeometerClientError::Process(error.to_string()))?;
        let mut stdin = child
            .stdin
            .take()
            .ok_or_else(|| GeometerClientError::Process("child stdin is unavailable".to_owned()))?;
        let mut stdout = child.stdout.take().ok_or_else(|| {
            GeometerClientError::Process("child stdout is unavailable".to_owned())
        })?;
        let mut stderr = child.stderr.take().ok_or_else(|| {
            GeometerClientError::Process("child stderr is unavailable".to_owned())
        })?;
        let hello = contracts::encode_ipc_hello_a0_json(&IpcHelloA0 {
            client_name: client_name.to_owned(),
            client_version: client_version.to_owned(),
            protocols: vec![IPC_IDENTITY.to_owned()],
            capabilities: Some(vec!["raw_attachments".to_owned()]),
        })?;
        ipc::write_frame(
            &mut stdin,
            &Frame {
                kind: FrameKind::Hello,
                request_id: 0,
                json: hello,
                attachments: Vec::new(),
            },
        )
        .await?;
        let welcome_frame = ipc::read_frame(&mut stdout).await?.ok_or_else(|| {
            GeometerClientError::Process("child exited before welcome".to_owned())
        })?;
        if welcome_frame.kind != FrameKind::Welcome
            || welcome_frame.request_id != 0
            || !welcome_frame.attachments.is_empty()
        {
            return Err(GeometerClientError::Protocol(
                "server did not return a valid welcome frame".to_owned(),
            ));
        }
        let welcome = contracts::decode_ipc_welcome_a0_json(&welcome_frame.json)?;
        validate_welcome(&welcome)?;
        let welcome = Arc::new(welcome);
        let inner = Arc::new(Inner {
            stdin: Mutex::new(Some(stdin)),
            child: Mutex::new(child),
            pending: Mutex::new(HashMap::new()),
            cancellation: Mutex::new(HashMap::new()),
            shutdown: Mutex::new(None),
            stderr: Mutex::new(Vec::new()),
            next_request_id: AtomicU64::new(1),
            closing: AtomicBool::new(false),
            closed: AtomicBool::new(false),
            welcome: Arc::clone(&welcome),
        });
        let client = Self {
            inner: Arc::clone(&inner),
            welcome,
        };
        tokio::spawn(reader_task(Arc::clone(&inner), stdout));
        tokio::spawn(async move {
            let mut chunk = [0_u8; 4096];
            loop {
                match stderr.read(&mut chunk).await {
                    Ok(0) | Err(_) => return,
                    Ok(count) => {
                        let mut captured = inner.stderr.lock().await;
                        let remaining = STDERR_CAPTURE_LIMIT.saturating_sub(captured.len());
                        captured.extend_from_slice(&chunk[..count.min(remaining)]);
                    }
                }
            }
        });
        Ok(client)
    }

    pub async fn discover_and_spawn(
        client_name: &str,
        client_version: &str,
    ) -> Result<Self, GeometerClientError> {
        let path = discover_executable().ok_or_else(|| {
            GeometerClientError::Process(
                "could not discover geometer; set GEOMETER_EXECUTABLE".to_owned(),
            )
        })?;
        Self::spawn(path, client_name, client_version).await
    }

    pub fn welcome(&self) -> &IpcWelcomeA0 {
        &self.welcome
    }

    pub async fn stderr_text(&self) -> String {
        String::from_utf8_lossy(&self.inner.stderr.lock().await).into_owned()
    }

    pub async fn start_execute(
        &self,
        operation: &str,
        request_json: &[u8],
        attachments: Vec<Attachment>,
    ) -> Result<OperationCall, GeometerClientError> {
        let declaration = self
            .welcome
            .operation_catalog
            .operations
            .iter()
            .find(|candidate| candidate.identity == operation)
            .ok_or_else(|| {
                GeometerClientError::Protocol(format!(
                    "operation {operation} is absent from the negotiated catalog"
                ))
            })?;
        let request = match declaration.runtime_dispatch {
            IpcRuntimeDispatchA0::LogicalDto => IpcRequestValueA0::LogicalDto(
                contracts::decode_model_bounds_options_a0_json(request_json)?,
            ),
            IpcRuntimeDispatchA0::PackedAttachment => {
                IpcRequestValueA0::PackedAttachment(contracts::decode_json::<
                    PackedAttachmentProjectionA0,
                >(request_json)?)
            }
        };
        validate_operation_request(declaration, &request, &attachments)?;
        let json = contracts::encode_ipc_request_a0_json(&IpcRequestA0 {
            operation: operation.to_owned(),
            request,
        })?;
        let request_id = self.next_request_id()?;
        let (sender, receiver) = oneshot::channel();
        let frame = Frame {
            kind: FrameKind::Request,
            request_id,
            json,
            attachments,
        };
        validate_effective_request(&frame, &self.welcome.limits)?;
        let resident_bytes = frame.encoded_size()?;
        let mut stdin_guard = self.inner.stdin.lock().await;
        if self.inner.closing.load(Ordering::SeqCst) || self.inner.closed.load(Ordering::SeqCst) {
            return Err(GeometerClientError::Closed);
        }
        let stdin = stdin_guard.as_mut().ok_or(GeometerClientError::Closed)?;
        let mut pending = self.inner.pending.lock().await;
        let pending_bytes = pending.values().try_fold(0_usize, |total, request| {
            total.checked_add(request.resident_bytes).ok_or_else(|| {
                GeometerClientError::Protocol("pending request byte accounting overflow".to_owned())
            })
        })?;
        if pending.len() >= self.welcome.limits.queued_requests as usize
            || pending_bytes
                .checked_add(resident_bytes)
                .is_none_or(|value| value > self.welcome.limits.resident_request_bytes as usize)
        {
            return Err(GeometerClientError::Protocol(
                "request exceeds the welcome queue or resident-byte limit".to_owned(),
            ));
        }
        pending.insert(
            request_id,
            PendingRequest {
                operation: operation.to_owned(),
                resident_bytes,
                sender,
            },
        );
        if let Err(error) = ipc::write_frame(stdin, &frame).await {
            pending.remove(&request_id);
            drop(pending);
            drop(stdin_guard);
            let failure = frame_failure("request write", &error);
            fail_connection(&self.inner, failure.clone()).await;
            return Err(failure.into_client());
        }
        drop(pending);
        drop(stdin_guard);
        Ok(OperationCall {
            client: self.clone(),
            operation: operation.to_owned(),
            request_id,
            receiver,
        })
    }

    pub async fn execute(
        &self,
        operation: &str,
        request_json: &[u8],
        attachments: Vec<Attachment>,
    ) -> Result<OperationResponse, GeometerClientError> {
        self.start_execute(operation, request_json, attachments)
            .await?
            .wait()
            .await
    }

    pub async fn model_bounds(
        &self,
        request: ModelBoundsRequest,
    ) -> Result<ModelBoundsResultA0, GeometerClientError> {
        let options = contracts::encode_model_bounds_options_a0_json(&request.options)?;
        let response = self
            .execute(
                "geometry.model_bounds.a0",
                &options,
                vec![Attachment {
                    name: "model".to_owned(),
                    media_type: request.media_type,
                    data: request.model,
                }],
            )
            .await?;
        if !response.attachments.is_empty() {
            return Err(GeometerClientError::Protocol(
                "model_bounds returned unexpected attachments".to_owned(),
            ));
        }
        match response.outcome {
            OperationOutcomeA0::Success(success) => match success.result {
                OperationResultValueA0::ModelBounds(result) => Ok(result),
                OperationResultValueA0::PackedAttachment(_) => Err(GeometerClientError::Protocol(
                    "model_bounds returned an incompatible packed result".to_owned(),
                )),
            },
            OperationOutcomeA0::Failure(failure) => Err(GeometerClientError::Operation {
                operation: failure.operation,
                diagnostics: failure.diagnostics,
            }),
        }
    }

    pub async fn analytic_planar_boolean_batch(
        &self,
        request: &AnalyticPlanarBooleanBatchRequestA0,
    ) -> Result<AnalyticPlanarBooleanBatchResultA0, GeometerClientError> {
        let declaration =
            operation_declaration(&self.welcome, ANALYTIC_PLANAR_BOOLEAN_BATCH_A0_IDENTITY)?;
        let request_projection = declaration.request_projection.as_ref().ok_or_else(|| {
            GeometerClientError::Protocol(
                "analytic request projection is absent from the negotiated catalog".to_owned(),
            )
        })?;
        let result_attachment_name = declaration
            .result_projection
            .as_ref()
            .ok_or_else(|| {
                GeometerClientError::Protocol(
                    "analytic result projection is absent from the negotiated catalog".to_owned(),
                )
            })?
            .attachment_name
            .clone();
        let input = declaration
            .input_attachments
            .iter()
            .find(|value| value.name == request_projection.attachment_name)
            .ok_or_else(|| {
                GeometerClientError::Protocol(
                    "analytic request attachment declaration is absent".to_owned(),
                )
            })?;
        let media_type = input.media_types.first().cloned().ok_or_else(|| {
            GeometerClientError::Protocol(
                "analytic request media type is absent from the catalog".to_owned(),
            )
        })?;
        let projection = PackedAttachmentProjectionA0 {
            schema: declaration.request_contract.clone(),
            packet: PackedAttachmentReferenceA0 {
                attachment: request_projection.attachment_name.clone(),
                format: request_projection.format.clone(),
            },
        };
        let request_json = contracts::encode_json(&projection)?;
        let packet = encode_analytic_planar_boolean_batch_request_a0_packet(request)?;
        let response = self
            .execute(
                ANALYTIC_PLANAR_BOOLEAN_BATCH_A0_IDENTITY,
                &request_json,
                vec![Attachment {
                    name: input.name.clone(),
                    media_type,
                    data: packet,
                }],
            )
            .await?;
        match response.outcome {
            OperationOutcomeA0::Failure(failure) => Err(GeometerClientError::Operation {
                operation: failure.operation,
                diagnostics: failure.diagnostics,
            }),
            OperationOutcomeA0::Success(_) => {
                let attachment = response
                    .attachments
                    .iter()
                    .find(|value| value.name == result_attachment_name)
                    .ok_or_else(|| {
                        GeometerClientError::Protocol(
                            "analytic result attachment is missing after validation".to_owned(),
                        )
                    })?;
                self.decode_analytic_result(&attachment.data).await
            }
        }
    }

    async fn decode_analytic_result(
        &self,
        bytes: &[u8],
    ) -> Result<AnalyticPlanarBooleanBatchResultA0, GeometerClientError> {
        match decode_analytic_planar_boolean_batch_result_a0_packet(bytes) {
            Ok(result) => Ok(result),
            Err(error) => {
                let message = format!("invalid analytic result packet: {error}");
                fail_connection(&self.inner, PendingFailure::Protocol(message.clone())).await;
                Err(GeometerClientError::Protocol(message))
            }
        }
    }

    pub async fn cancel(
        &self,
        request_id: u64,
        reason: Option<&str>,
    ) -> Result<bool, GeometerClientError> {
        if request_id == 0 {
            return Err(GeometerClientError::Protocol(
                "cancellation requires a nonzero request id".to_owned(),
            ));
        }
        let frame = Frame {
            kind: FrameKind::Cancel,
            request_id,
            json: encode_reason(reason)?,
            attachments: Vec::new(),
        };
        let mut stdin_guard = self.inner.stdin.lock().await;
        if self.inner.closing.load(Ordering::SeqCst) || self.inner.closed.load(Ordering::SeqCst) {
            return Err(GeometerClientError::Closed);
        }
        let stdin = stdin_guard.as_mut().ok_or(GeometerClientError::Closed)?;
        let (sender, receiver) = oneshot::channel();
        {
            let mut cancellation = self.inner.cancellation.lock().await;
            match cancellation.entry(request_id) {
                std::collections::hash_map::Entry::Vacant(entry) => {
                    entry.insert(sender);
                }
                std::collections::hash_map::Entry::Occupied(_) => {
                    return Err(GeometerClientError::Protocol(
                        "a cancellation is already pending for this request".to_owned(),
                    ));
                }
            }
        }
        if let Err(error) = ipc::write_frame(stdin, &frame).await {
            self.inner.cancellation.lock().await.remove(&request_id);
            drop(stdin_guard);
            let failure = frame_failure("cancellation write", &error);
            fail_connection(&self.inner, failure.clone()).await;
            return Err(failure.into_client());
        }
        drop(stdin_guard);
        receiver
            .await
            .map_err(|_| GeometerClientError::Process("cancellation channel closed".to_owned()))?
            .map_err(PendingFailure::into_client)
    }

    pub async fn close(&self) -> Result<(), GeometerClientError> {
        self.close_with_timeout(Duration::from_secs(31)).await
    }

    async fn close_with_timeout(&self, timeout: Duration) -> Result<(), GeometerClientError> {
        if self.inner.closed.load(Ordering::SeqCst) {
            return Ok(());
        }
        if self.inner.closing.swap(true, Ordering::SeqCst) {
            return Err(GeometerClientError::Closed);
        }
        let deadline = tokio::time::Instant::now() + timeout;
        let (sender, receiver) = oneshot::channel();
        let mut stdin_guard = self.inner.stdin.lock().await;
        let stdin = stdin_guard.as_mut().ok_or(GeometerClientError::Closed)?;
        let pending_at_request = self.inner.pending.lock().await.len();
        *self.inner.shutdown.lock().await = Some(ShutdownWaiter {
            sender,
            pending_at_request,
            rejected_queued: 0,
            active_eligible: false,
        });
        let shutdown_frame = Frame {
            kind: FrameKind::Shutdown,
            request_id: 0,
            json: encode_reason(None)?,
            attachments: Vec::new(),
        };
        if let Err(error) = ipc::write_frame(stdin, &shutdown_frame).await {
            self.inner.shutdown.lock().await.take();
            drop(stdin_guard);
            let failure = frame_failure("graceful shutdown write", &error);
            fail_connection(&self.inner, failure.clone()).await;
            return Err(failure.into_client());
        }
        drop(stdin_guard);
        self.await_shutdown_ack(deadline, receiver).await?;
        let status = self.await_child_exit(deadline).await?;
        if !status.success() {
            let failure = PendingFailure::Process(format!("Geometer exited with {status}"));
            fail_connection(&self.inner, failure.clone()).await;
            return Err(failure.into_client());
        }
        self.inner.closed.store(true, Ordering::SeqCst);
        Ok(())
    }

    async fn await_shutdown_ack(
        &self,
        deadline: tokio::time::Instant,
        receiver: oneshot::Receiver<Result<(), PendingFailure>>,
    ) -> Result<(), GeometerClientError> {
        let failure = match tokio::time::timeout_at(deadline, receiver).await {
            Ok(Ok(Ok(()))) => return Ok(()),
            Ok(Ok(Err(failure))) => failure,
            Ok(Err(_)) => PendingFailure::Process("shutdown channel closed".to_owned()),
            Err(_) => PendingFailure::Process("graceful shutdown timed out".to_owned()),
        };
        fail_connection(&self.inner, failure.clone()).await;
        Err(failure.into_client())
    }

    async fn await_child_exit(
        &self,
        deadline: tokio::time::Instant,
    ) -> Result<std::process::ExitStatus, GeometerClientError> {
        let wait = async { self.inner.child.lock().await.wait().await };
        match tokio::time::timeout_at(deadline, wait).await {
            Ok(Ok(status)) => Ok(status),
            Ok(Err(error)) => {
                let failure = PendingFailure::Process(format!("child wait failed: {error}"));
                fail_connection(&self.inner, failure.clone()).await;
                Err(failure.into_client())
            }
            Err(_) => {
                let failure = PendingFailure::Process(
                    "Geometer did not exit before the shutdown deadline".to_owned(),
                );
                fail_connection(&self.inner, failure.clone()).await;
                Err(failure.into_client())
            }
        }
    }

    pub async fn terminate(&self) -> Result<(), GeometerClientError> {
        self.inner.closing.store(true, Ordering::SeqCst);
        self.inner
            .child
            .lock()
            .await
            .kill()
            .await
            .map_err(|error| GeometerClientError::Process(error.to_string()))?;
        fail_connection(
            &self.inner,
            PendingFailure::Process("Geometer process was terminated by the client".to_owned()),
        )
        .await;
        Ok(())
    }

    fn next_request_id(&self) -> Result<u64, GeometerClientError> {
        let id = self.inner.next_request_id.fetch_add(1, Ordering::SeqCst);
        if id == 0 || id == u64::MAX {
            return Err(GeometerClientError::Protocol(
                "request identifier space is exhausted".to_owned(),
            ));
        }
        Ok(id)
    }
}

async fn reader_task(inner: Arc<Inner>, mut stdout: tokio::process::ChildStdout) {
    let read_limits = ipc::ReadLimits {
        json_bytes: inner.welcome.limits.json_bytes as usize,
        attachment_count: inner.welcome.limits.attachment_count as usize,
        attachment_name_bytes: inner.welcome.limits.attachment_name_bytes as usize,
        attachment_media_type_bytes: inner.welcome.limits.attachment_media_type_bytes as usize,
        attachment_bytes: inner.welcome.limits.attachment_bytes as usize,
        frame_bytes: inner.welcome.limits.frame_bytes as usize,
    };
    loop {
        let frame = match ipc::read_frame_with_limits(&mut stdout, Some(read_limits)).await {
            Ok(Some(frame)) => frame,
            Ok(None) => {
                if inner.closing.load(Ordering::SeqCst) {
                    finish_connection(
                        &inner,
                        PendingFailure::Process(
                            "Geometer stdout closed during shutdown".to_owned(),
                        ),
                        false,
                    )
                    .await;
                    return;
                }
                fail_connection(
                    &inner,
                    PendingFailure::Process("Geometer stdout closed unexpectedly".to_owned()),
                )
                .await;
                return;
            }
            Err(error) => {
                let failure = frame_failure("response read", &error);
                fail_connection(&inner, failure).await;
                return;
            }
        };
        if let Err(message) = dispatch_frame(&inner, frame).await {
            fail_connection(&inner, PendingFailure::Protocol(message)).await;
            return;
        }
    }
}

fn frame_failure(context: &str, error: &ipc::FrameError) -> PendingFailure {
    let message = format!("{context} failed: {error}");
    match error {
        ipc::FrameError::Io(_) => PendingFailure::Process(message),
        ipc::FrameError::Protocol(_) => PendingFailure::Protocol(message),
    }
}

async fn dispatch_frame(inner: &Arc<Inner>, frame: Frame) -> Result<(), String> {
    match frame.kind {
        FrameKind::Response => handle_response(inner, frame).await,
        FrameKind::Cancelled => handle_cancelled(inner, frame).await,
        FrameKind::CancelRejected => handle_cancel_rejected(inner, frame).await,
        FrameKind::ShutdownAck => handle_shutdown_ack(inner, frame).await,
        FrameKind::ProtocolError => Err(protocol_error_message(&frame)),
        _ => Err("server sent a frame kind invalid in its direction".to_owned()),
    }
}

async fn handle_response(inner: &Arc<Inner>, frame: Frame) -> Result<(), String> {
    let pending = inner
        .pending
        .lock()
        .await
        .remove(&frame.request_id)
        .ok_or_else(|| "response used an unknown or completed request id".to_owned())?;
    match contracts::decode_operation_outcome_a0_json(&frame.json) {
        Ok(outcome) if outcome_operation(&outcome) == pending.operation => {
            match validate_operation_response(
                &inner.welcome,
                &pending.operation,
                &outcome,
                &frame.attachments,
            ) {
                Ok(()) => {
                    observe_shutdown_response(inner, &outcome).await?;
                    let _ = pending.sender.send(Ok(frame));
                    Ok(())
                }
                Err(error) => {
                    let message = error.to_string();
                    let _ = pending
                        .sender
                        .send(Err(PendingFailure::Protocol(message.clone())));
                    Err(message)
                }
            }
        }
        Ok(_) => {
            let _ = pending.sender.send(Err(PendingFailure::Protocol(
                "response operation mismatch".to_owned(),
            )));
            Err("response operation does not match its request".to_owned())
        }
        Err(error) => {
            let _ = pending
                .sender
                .send(Err(PendingFailure::Protocol(error.to_string())));
            Err("response contains an invalid generated outcome".to_owned())
        }
    }
}

async fn observe_shutdown_response(
    inner: &Arc<Inner>,
    outcome: &OperationOutcomeA0,
) -> Result<(), String> {
    if !inner.closing.load(Ordering::SeqCst) {
        return Ok(());
    }
    let mut shutdown = inner.shutdown.lock().await;
    let Some(waiter) = shutdown.as_mut() else {
        return Ok(());
    };
    let rejected = matches!(outcome, OperationOutcomeA0::Failure(value)
        if value.diagnostics.iter().any(|diagnostic|
            diagnostic.code == "geometer.transport.server_shutting_down"));
    if rejected {
        waiter.rejected_queued = waiter
            .rejected_queued
            .checked_add(1)
            .ok_or_else(|| "shutdown rejection count overflow".to_owned())?;
    } else {
        waiter.active_eligible = true;
    }
    Ok(())
}

async fn handle_cancelled(inner: &Arc<Inner>, frame: Frame) -> Result<(), String> {
    let control = contracts::decode_ipc_cancelled_a0_json(&frame.json);
    if !frame.attachments.is_empty()
        || frame.request_id == 0
        || !matches!(control, Ok(IpcCancelledA0 { ref status }) if status == "cancelled")
    {
        return Err("invalid cancelled control frame".to_owned());
    }
    let pending = inner.pending.lock().await.remove(&frame.request_id);
    let cancellation = inner.cancellation.lock().await.remove(&frame.request_id);
    let (pending, cancellation) = match (pending, cancellation) {
        (Some(pending), Some(cancellation)) => (pending, cancellation),
        (pending, cancellation) => {
            if let Some(pending) = pending {
                let _ = pending.sender.send(Err(PendingFailure::Protocol(
                    "invalid cancellation correlation".to_owned(),
                )));
            }
            if let Some(cancellation) = cancellation {
                let _ = cancellation.send(Err(PendingFailure::Protocol(
                    "invalid cancellation correlation".to_owned(),
                )));
            }
            return Err("cancelled used an unknown request id".to_owned());
        }
    };
    let _ = cancellation.send(Ok(true));
    let _ = pending.sender.send(Ok(frame));
    Ok(())
}

async fn handle_cancel_rejected(inner: &Arc<Inner>, frame: Frame) -> Result<(), String> {
    let control = contracts::decode_ipc_cancel_rejected_a0_json(&frame.json)
        .map_err(|_| "invalid cancel_rejected JSON body".to_owned())?;
    if !valid_rejected_control(&control, frame.request_id) {
        return Err("invalid cancel_rejected JSON body".to_owned());
    }
    let cancellation = inner
        .cancellation
        .lock()
        .await
        .remove(&frame.request_id)
        .ok_or_else(|| "cancel_rejected used an unknown request id".to_owned())?;
    let _ = cancellation.send(Ok(false));
    Ok(())
}

fn valid_rejected_control(control: &IpcCancelRejectedA0, request_id: u64) -> bool {
    let expected_request_id = request_id.to_string();
    control.status == "rejected"
        && control.diagnostic.category == DiagnosticCategory::Transport
        && matches!(
            control.diagnostic.code.as_str(),
            "geometer.transport.not_cancellable" | "geometer.transport.unknown_request"
        )
        && !control.diagnostic.message.is_empty()
        && !control.diagnostic.retryable
        && control.diagnostic.request_id.as_deref() == Some(expected_request_id.as_str())
}

async fn handle_shutdown_ack(inner: &Arc<Inner>, frame: Frame) -> Result<(), String> {
    let frame_valid = [
        frame.request_id == 0,
        frame.attachments.is_empty(),
        inner.pending.lock().await.is_empty(),
        inner.cancellation.lock().await.is_empty(),
    ]
    .into_iter()
    .all(std::convert::identity);
    if !frame_valid {
        return Err("shutdown_ack arrived before all requests were terminal".to_owned());
    }
    let control = contracts::decode_ipc_shutdown_ack_a0_json(&frame.json)
        .map_err(|_| "invalid shutdown_ack JSON body".to_owned())?;
    if control.status != "complete" {
        return Err("invalid shutdown_ack JSON body".to_owned());
    }
    let mut shutdown = inner.shutdown.lock().await;
    let waiter = shutdown
        .as_ref()
        .ok_or_else(|| "unexpected shutdown_ack".to_owned())?;
    if !shutdown_ack_matches(&control, waiter) {
        return Err("shutdown_ack contradicts observed client state".to_owned());
    }
    let waiter = shutdown.take().expect("checked shutdown waiter");
    let _ = waiter.sender.send(Ok(()));
    Ok(())
}

fn shutdown_ack_matches(control: &contracts::IpcShutdownAckA0, waiter: &ShutdownWaiter) -> bool {
    [
        control.rejected_queued_request_count == waiter.rejected_queued,
        waiter.rejected_queued as usize <= waiter.pending_at_request,
        !control.active_request_completed || waiter.pending_at_request > 0,
        !control.active_request_completed || waiter.active_eligible,
    ]
    .into_iter()
    .all(std::convert::identity)
}

fn protocol_error_message(frame: &Frame) -> String {
    let Ok(control) = contracts::decode_ipc_protocol_error_a0_json(&frame.json) else {
        return "invalid protocol_error JSON body".to_owned();
    };
    if control.status != "protocol_error"
        || control.diagnostic.category != DiagnosticCategory::Transport
        || control.diagnostic.code != "geometer.transport.protocol_error"
    {
        return "invalid protocol_error JSON body".to_owned();
    }
    format!("server protocol error: {}", control.diagnostic.message)
}

async fn fail_connection(inner: &Arc<Inner>, failure: PendingFailure) {
    finish_connection(inner, failure, true).await;
}

async fn finish_connection(inner: &Arc<Inner>, failure: PendingFailure, kill_child: bool) {
    if inner.closed.swap(true, Ordering::SeqCst) {
        if kill_child {
            let _ = inner.child.lock().await.start_kill();
        }
        return;
    }
    inner.closing.store(true, Ordering::SeqCst);
    inner.stdin.lock().await.take();
    for (_, pending) in inner.pending.lock().await.drain() {
        let _ = pending.sender.send(Err(failure.clone()));
    }
    for (_, cancellation) in inner.cancellation.lock().await.drain() {
        let _ = cancellation.send(Err(failure.clone()));
    }
    if let Some(shutdown) = inner.shutdown.lock().await.take() {
        let _ = shutdown.sender.send(Err(failure.clone()));
    }
    if kill_child {
        let _ = inner.child.lock().await.start_kill();
    }
}

fn outcome_operation(outcome: &OperationOutcomeA0) -> String {
    match outcome {
        OperationOutcomeA0::Success(value) => value.operation.clone(),
        OperationOutcomeA0::Failure(value) => value.operation.clone(),
    }
}
