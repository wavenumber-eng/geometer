use super::*;

pub(super) async fn reader_task(inner: Arc<Inner>, mut stdout: BoxAsyncRead) {
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
                    let deadline = tokio::time::Instant::now() + inner.shutdown_timeout;
                    let _ = finish_connection_until(
                        &inner,
                        PendingFailure::Process(
                            "Geometer stdout closed during shutdown".to_owned(),
                        ),
                        false,
                        deadline,
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

pub(super) async fn stderr_task(inner: Arc<Inner>, mut stderr: BoxAsyncRead) {
    let mut chunk = [0_u8; 4096];
    loop {
        match stderr.read(&mut chunk).await {
            Ok(0) | Err(_) => return,
            Ok(count) => {
                let mut captured = inner.stderr.lock().await;
                let remaining = inner.stderr_capture_limit.saturating_sub(captured.len());
                captured.extend_from_slice(&chunk[..count.min(remaining)]);
            }
        }
    }
}

pub(super) fn frame_failure(context: &str, error: &ipc::FrameError) -> PendingFailure {
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
    match decode_outcome(&inner.welcome, &pending.operation, &frame.json) {
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
    outcome: &OperationOutcome,
) -> Result<(), String> {
    if !inner.closing.load(Ordering::SeqCst) {
        return Ok(());
    }
    let mut shutdown = inner.shutdown.lock().await;
    let Some(waiter) = shutdown.as_mut() else {
        return Ok(());
    };
    let rejected = match outcome {
        OperationOutcome::A0(OperationOutcomeA0::Failure(value)) => value
            .diagnostics
            .iter()
            .any(|diagnostic| diagnostic.code == "geometer.transport.server_shutting_down"),
        OperationOutcome::B0(OperationOutcomeB0::Failure(value)) => value
            .diagnostics
            .iter()
            .any(|diagnostic| diagnostic.code == "geometer.transport.server_shutting_down"),
        _ => false,
    };
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

pub(super) fn shutdown_ack_matches(
    control: &contracts::IpcShutdownAckA0,
    waiter: &ShutdownWaiter,
) -> bool {
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
