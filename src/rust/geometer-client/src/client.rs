use std::collections::HashMap;
use std::path::{Path, PathBuf};
use std::process::Stdio;
use std::sync::Arc;
use std::sync::atomic::{AtomicBool, AtomicU64, Ordering};
use std::time::Duration;

use serde_json::Value;
use tokio::io::AsyncReadExt;
use tokio::process::{Child, ChildStdin, Command};
use tokio::sync::{Mutex, oneshot};

use crate::generated::contracts::{
    self, DiagnosticCategory, IpcCancelRejectedA0, IpcCancelledA0, IpcHelloA0, IpcReasonA0,
    IpcRequestA0, IpcWelcomeA0, ModelBoundsOptionsA0, ModelBoundsResultA0, OperationOutcomeA0,
    OperationResultValueA0,
};
use crate::ipc::{self, Attachment, Frame, FrameKind};
use crate::{IPC_IDENTITY, NORMALIZED_CATALOG_SHA256};

const STDERR_CAPTURE_LIMIT: usize = 1024 * 1024;

/// Generated executable IPC A0 welcome contract retained under the pilot facade name.
pub type Welcome = IpcWelcomeA0;

#[derive(Debug, thiserror::Error)]
pub enum GeometerClientError {
    #[error(transparent)]
    Contract(#[from] contracts::ContractError),
    #[error(transparent)]
    Frame(#[from] ipc::FrameError),
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
    sender: oneshot::Sender<Result<Frame, String>>,
}

struct Inner {
    stdin: Mutex<Option<ChildStdin>>,
    child: Mutex<Child>,
    pending: Mutex<HashMap<u64, PendingRequest>>,
    cancellation: Mutex<HashMap<u64, oneshot::Sender<Result<bool, String>>>>,
    shutdown: Mutex<Option<oneshot::Sender<Result<(), String>>>>,
    stderr: Mutex<Vec<u8>>,
    next_request_id: AtomicU64,
    closing: AtomicBool,
    closed: AtomicBool,
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
    receiver: oneshot::Receiver<Result<Frame, String>>,
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
            .map_err(GeometerClientError::Process)?;
        decode_operation_response(&operation, frame)
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
                    .map_err(GeometerClientError::Process)?;
                decode_operation_response(&self.operation, frame)
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
        });
        let client = Self {
            inner: Arc::clone(&inner),
            welcome: Arc::new(welcome),
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
        let request = contracts::decode_model_bounds_options_a0_json(request_json)?;
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
            return Err(error.into());
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

    pub async fn cancel(
        &self,
        request_id: u64,
        reason: Option<&str>,
    ) -> Result<bool, GeometerClientError> {
        let mut stdin_guard = self.inner.stdin.lock().await;
        if self.inner.closing.load(Ordering::SeqCst) || self.inner.closed.load(Ordering::SeqCst) {
            return Err(GeometerClientError::Closed);
        }
        let stdin = stdin_guard.as_mut().ok_or(GeometerClientError::Closed)?;
        let (sender, receiver) = oneshot::channel();
        if self
            .inner
            .cancellation
            .lock()
            .await
            .insert(request_id, sender)
            .is_some()
        {
            return Err(GeometerClientError::Protocol(
                "a cancellation is already pending for this request".to_owned(),
            ));
        }
        let frame = Frame {
            kind: FrameKind::Cancel,
            request_id,
            json: encode_reason(reason)?,
            attachments: Vec::new(),
        };
        if let Err(error) = ipc::write_frame(stdin, &frame).await {
            self.inner.cancellation.lock().await.remove(&request_id);
            return Err(error.into());
        }
        drop(stdin_guard);
        receiver
            .await
            .map_err(|_| GeometerClientError::Process("cancellation channel closed".to_owned()))?
            .map_err(GeometerClientError::Protocol)
    }

    pub async fn close(&self) -> Result<(), GeometerClientError> {
        if self.inner.closed.load(Ordering::SeqCst) {
            return Ok(());
        }
        if self.inner.closing.swap(true, Ordering::SeqCst) {
            return Err(GeometerClientError::Closed);
        }
        let (sender, receiver) = oneshot::channel();
        let mut stdin_guard = self.inner.stdin.lock().await;
        let stdin = stdin_guard.as_mut().ok_or(GeometerClientError::Closed)?;
        *self.inner.shutdown.lock().await = Some(sender);
        let shutdown_frame = Frame {
            kind: FrameKind::Shutdown,
            request_id: 0,
            json: encode_reason(None)?,
            attachments: Vec::new(),
        };
        if let Err(error) = ipc::write_frame(stdin, &shutdown_frame).await {
            self.inner.shutdown.lock().await.take();
            fail_connection(&self.inner, "failed to write graceful shutdown").await;
            return Err(error.into());
        }
        drop(stdin_guard);
        tokio::time::timeout(Duration::from_secs(31), receiver)
            .await
            .map_err(|_| GeometerClientError::Process("graceful shutdown timed out".to_owned()))?
            .map_err(|_| GeometerClientError::Process("shutdown channel closed".to_owned()))?
            .map_err(GeometerClientError::Protocol)?;
        let status = self
            .inner
            .child
            .lock()
            .await
            .wait()
            .await
            .map_err(|error| GeometerClientError::Process(error.to_string()))?;
        self.inner.closed.store(true, Ordering::SeqCst);
        if !status.success() {
            return Err(GeometerClientError::Process(format!(
                "Geometer exited with {status}"
            )));
        }
        Ok(())
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
        fail_connection(&self.inner, "Geometer process was terminated by the client").await;
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
    loop {
        let frame = match ipc::read_frame(&mut stdout).await {
            Ok(Some(frame)) => frame,
            Ok(None) => {
                if inner.closing.load(Ordering::SeqCst) {
                    finish_connection(&inner, "Geometer stdout closed during shutdown", false)
                        .await;
                    return;
                }
                fail_connection(&inner, "Geometer stdout closed unexpectedly").await;
                return;
            }
            Err(error) => {
                fail_connection(&inner, &error.to_string()).await;
                return;
            }
        };
        if let Err(message) = dispatch_frame(&inner, frame).await {
            fail_connection(&inner, &message).await;
            return;
        }
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
            let _ = pending.sender.send(Ok(frame));
            Ok(())
        }
        Ok(_) => {
            let _ = pending
                .sender
                .send(Err("response operation mismatch".to_owned()));
            Err("response operation does not match its request".to_owned())
        }
        Err(error) => {
            let _ = pending.sender.send(Err(error.to_string()));
            Err("response contains an invalid generated outcome".to_owned())
        }
    }
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
                let _ = pending
                    .sender
                    .send(Err("invalid cancellation correlation".to_owned()));
            }
            if let Some(cancellation) = cancellation {
                let _ = cancellation.send(Err("invalid cancellation correlation".to_owned()));
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
    if frame.request_id != 0
        || !frame.attachments.is_empty()
        || !inner.pending.lock().await.is_empty()
        || !inner.cancellation.lock().await.is_empty()
    {
        return Err("shutdown_ack arrived before all requests were terminal".to_owned());
    }
    let control = contracts::decode_ipc_shutdown_ack_a0_json(&frame.json)
        .map_err(|_| "invalid shutdown_ack JSON body".to_owned())?;
    if control.status != "complete" {
        return Err("invalid shutdown_ack JSON body".to_owned());
    }
    let _ = (
        control.active_request_completed,
        control.rejected_queued_request_count,
    );
    let sender = inner
        .shutdown
        .lock()
        .await
        .take()
        .ok_or_else(|| "unexpected shutdown_ack".to_owned())?;
    let _ = sender.send(Ok(()));
    Ok(())
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

async fn fail_connection(inner: &Arc<Inner>, message: &str) {
    finish_connection(inner, message, true).await;
}

async fn finish_connection(inner: &Arc<Inner>, message: &str, kill_child: bool) {
    if inner.closed.swap(true, Ordering::SeqCst) {
        return;
    }
    inner.closing.store(true, Ordering::SeqCst);
    inner.stdin.lock().await.take();
    for (_, pending) in inner.pending.lock().await.drain() {
        let _ = pending.sender.send(Err(message.to_owned()));
    }
    for (_, cancellation) in inner.cancellation.lock().await.drain() {
        let _ = cancellation.send(Err(message.to_owned()));
    }
    if let Some(shutdown) = inner.shutdown.lock().await.take() {
        let _ = shutdown.send(Err(message.to_owned()));
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

fn validate_welcome(welcome: &IpcWelcomeA0) -> Result<(), GeometerClientError> {
    if welcome.ipc != IPC_IDENTITY || welcome.catalog_sha256 != NORMALIZED_CATALOG_SHA256 {
        return Err(GeometerClientError::Protocol(
            "welcome selected an unsupported IPC or contract catalog".to_owned(),
        ));
    }
    if !valid_effective_limits(&welcome.limits) {
        return Err(GeometerClientError::Protocol(
            "welcome advertises an effective limit above the A0 maximum".to_owned(),
        ));
    }
    if serde_json::to_value(&welcome.operation_catalog)
        .map_err(|error| GeometerClientError::Protocol(error.to_string()))?
        != expected_operation_catalog(welcome)
    {
        return Err(GeometerClientError::Protocol(
            "welcome operation catalog differs from the generated model-bounds catalog".to_owned(),
        ));
    }
    for capability in [
        "serialized_execution",
        "queue_only_cancellation",
        "raw_attachments",
    ] {
        if !welcome.capabilities.iter().any(|value| value == capability) {
            return Err(GeometerClientError::Protocol(format!(
                "welcome is missing required capability {capability}"
            )));
        }
    }
    Ok(())
}

fn valid_effective_limits(limits: &contracts::IpcEffectiveLimitsA0) -> bool {
    let bounded = [
        (limits.json_bytes, ipc::MAX_JSON_BYTES as u32),
        (limits.attachment_count, ipc::MAX_ATTACHMENT_COUNT as u32),
        (
            limits.attachment_name_bytes,
            ipc::MAX_ATTACHMENT_TEXT_BYTES as u32,
        ),
        (
            limits.attachment_media_type_bytes,
            ipc::MAX_ATTACHMENT_TEXT_BYTES as u32,
        ),
        (limits.attachment_bytes, ipc::MAX_ATTACHMENT_BYTES as u32),
        (limits.frame_bytes, ipc::MAX_FRAME_BYTES as u32),
        (limits.queued_requests, 8),
        (limits.queued_bytes, ipc::MAX_FRAME_BYTES as u32),
        (limits.resident_request_bytes, ipc::MAX_FRAME_BYTES as u32),
        (limits.pending_writer_bytes, ipc::MAX_FRAME_BYTES as u32),
    ];
    bounded
        .iter()
        .all(|(value, maximum)| *value > 0 && value <= maximum)
}

fn validate_effective_request(
    frame: &Frame,
    limits: &contracts::IpcEffectiveLimitsA0,
) -> Result<(), GeometerClientError> {
    let invalid = frame.json.len() > limits.json_bytes as usize
        || frame.attachments.len() > limits.attachment_count as usize
        || frame.encoded_size()? > limits.frame_bytes as usize
        || frame.attachments.iter().any(|attachment| {
            attachment.name.len() > limits.attachment_name_bytes as usize
                || attachment.media_type.len() > limits.attachment_media_type_bytes as usize
                || attachment.data.len() > limits.attachment_bytes as usize
        });
    if invalid {
        return Err(GeometerClientError::Protocol(
            "request exceeds an effective limit advertised by welcome".to_owned(),
        ));
    }
    Ok(())
}

fn encode_reason(reason: Option<&str>) -> Result<Vec<u8>, GeometerClientError> {
    Ok(contracts::encode_ipc_reason_a0_json(&IpcReasonA0 {
        reason: reason.map(str::to_owned),
    })?)
}

fn expected_operation_catalog(welcome: &IpcWelcomeA0) -> Value {
    serde_json::json!({
        "catalog": "wn.geometer.operation_catalog.a0",
        "generic_abi": "a0",
        "release_version": welcome.release_version,
        "c_abi_generation": welcome.c_abi_generation,
        "operations": [{
            "identity": "geometry.analytic_planar_boolean_batch.a0",
            "request_contract": "geometry.analytic_planar_boolean_batch.request.a0",
            "result_contract": "geometry.analytic_planar_boolean_batch.result.a0",
            "runtime_dispatch": "packed_attachment",
            "input_attachments": [{
                "name": "analytic_planar_boolean_request",
                "required": true,
                "media_types": [
                    "application/vnd.wavenumber.geometer.analytic-planar-boolean-request"
                ],
                "max_bytes": 268435456
            }],
            "output_attachments": [{
                "name": "analytic_planar_boolean_result",
                "required": true,
                "media_types": [
                    "application/vnd.wavenumber.geometer.analytic-planar-boolean-result"
                ],
                "max_bytes": 268435456
            }],
            "request_projection": {
                "kind": "packed_attachment",
                "attachment_name": "analytic_planar_boolean_request",
                "format": "geometry.analytic_planar_boolean.packet.a0"
            },
            "result_projection": {
                "kind": "packed_attachment",
                "attachment_name": "analytic_planar_boolean_result",
                "format": "geometry.analytic_planar_boolean.packet.a0"
            }
        }, {
            "identity": "geometry.model_bounds.a0",
            "request_contract": "geometry.model_bounds.options.a0",
            "result_contract": "geometry.model_bounds.a0",
            "runtime_dispatch": "logical_dto",
            "input_attachments": [{
                "name": "model",
                "required": true,
                "media_types": ["application/step", "model/step"],
                "max_bytes": 268435456
            }],
            "output_attachments": []
        }],
        "attachment_descriptor": {
            "wasm32": {
                "size": 36,
                "offsets": {
                    "struct_size": 0,
                    "flags": 4,
                    "name": 8,
                    "name_size": 12,
                    "media_type": 16,
                    "media_type_size": 20,
                    "data": 24,
                    "data_size": 28,
                    "reserved0": 32
                }
            },
            "pointer64": {
                "size": 56,
                "offsets": {
                    "struct_size": 0,
                    "flags": 4,
                    "name": 8,
                    "name_size": 16,
                    "media_type": 24,
                    "media_type_size": 32,
                    "data": 40,
                    "data_size": 48,
                    "reserved0": 52
                }
            }
        },
        "limits": {
            "operation_id_bytes": 128,
            "request_json_bytes": 8388608,
            "response_json_bytes": 8388608,
            "attachment_count": 16,
            "attachment_name_bytes": 128,
            "attachment_media_type_bytes": 128,
            "attachment_bytes": 268435456,
            "aggregate_attachment_bytes_native": 536870912,
            "aggregate_attachment_bytes_wasm": 268435456
        }
    })
}

fn discover_executable() -> Option<PathBuf> {
    if let Some(path) = std::env::var_os("GEOMETER_EXECUTABLE") {
        let path = PathBuf::from(path);
        if path.is_file() {
            return Some(path);
        }
    }
    let name = if cfg!(windows) {
        "geometer.exe"
    } else {
        "geometer"
    };
    if let Some(sibling) = std::env::current_exe()
        .ok()
        .and_then(|current| current.parent().map(|parent| parent.join(name)))
        .filter(|path| path.is_file())
    {
        return Some(sibling);
    }
    let platform = format!(
        "{}-{}",
        if cfg!(windows) {
            "windows"
        } else if cfg!(target_os = "macos") {
            "macos"
        } else {
            "linux"
        },
        match std::env::consts::ARCH {
            "x86_64" => "x64",
            "aarch64" => "arm64",
            value => value,
        }
    );
    let candidate = PathBuf::from("dist/native").join(platform).join(name);
    candidate.is_file().then_some(candidate)
}
