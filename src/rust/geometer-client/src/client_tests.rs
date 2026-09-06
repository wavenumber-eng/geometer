use std::path::{Path, PathBuf};

use super::*;
use crate::generated::contracts::{AnalyticPlanarBooleanBatchRequestA0, IpcShutdownAckA0};

#[tokio::test]
async fn invalid_analytic_result_is_fatal_protocol_failure() {
    let client = GeometerClient::spawn(native_executable(), "corrupt-result-test", "a0")
        .await
        .unwrap();
    let error = client
        .decode_analytic_result(b"not a packet")
        .await
        .unwrap_err();
    assert!(matches!(error, GeometerClientError::Protocol(_)));
    let request = AnalyticPlanarBooleanBatchRequestA0 {
        jobs: Vec::new(),
        relationship_queries: Vec::new(),
    };
    assert!(matches!(
        client.analytic_planar_boolean_batch(&request).await,
        Err(GeometerClientError::Closed)
    ));
}

#[tokio::test]
async fn duplicate_cancel_preserves_the_first_waiter() {
    let client = GeometerClient::spawn(native_executable(), "duplicate-cancel-test", "a0")
        .await
        .unwrap();
    let (first_sender, mut first_receiver) = oneshot::channel();
    client
        .inner
        .cancellation
        .lock()
        .await
        .insert(77, first_sender);
    assert!(matches!(
        client.cancel(77, None).await,
        Err(GeometerClientError::Protocol(_))
    ));
    assert!(matches!(
        first_receiver.try_recv(),
        Err(oneshot::error::TryRecvError::Empty)
    ));
    client.inner.cancellation.lock().await.remove(&77);
    client.close().await.unwrap();
}

#[tokio::test]
async fn oversized_cancel_reason_leaves_no_stale_waiter() {
    let client = GeometerClient::spawn(native_executable(), "cancel-reason-test", "a0")
        .await
        .unwrap();
    let reason = "x".repeat(1025);
    assert!(matches!(
        client.cancel(88, Some(&reason)).await,
        Err(GeometerClientError::Contract(_))
    ));
    assert!(client.inner.cancellation.lock().await.is_empty());
    client.close().await.unwrap();
}

#[tokio::test]
async fn welcome_catalog_rejects_analytic_metadata_drift() {
    let client = GeometerClient::spawn(native_executable(), "catalog-drift-test", "a0")
        .await
        .unwrap();
    let welcome = client.welcome().clone();
    client.close().await.unwrap();
    for mutation in 0..3 {
        let mut changed = welcome.clone();
        let analytic = changed
            .operation_catalog
            .operations
            .iter_mut()
            .find(|value| value.identity == ANALYTIC_PLANAR_BOOLEAN_BATCH_A0_IDENTITY)
            .unwrap();
        match mutation {
            0 => analytic.input_attachments[0].media_types[0].push_str("+drift"),
            1 => analytic.input_attachments[0].max_bytes -= 1,
            2 => analytic
                .result_projection
                .as_mut()
                .unwrap()
                .format
                .push_str(".drift"),
            _ => unreachable!(),
        }
        assert!(matches!(
            validate_welcome(&changed),
            Err(GeometerClientError::Protocol(_))
        ));
    }
}

#[tokio::test]
async fn process_failure_type_reaches_cancel_and_shutdown_waiters() {
    assert_waiter_failure_type(PendingFailure::Process("synthetic EOF".to_owned())).await;
}

#[tokio::test]
async fn protocol_failure_type_reaches_cancel_and_shutdown_waiters() {
    assert_waiter_failure_type(PendingFailure::Protocol("synthetic protocol".to_owned())).await;
}

#[tokio::test]
async fn missing_shutdown_ack_hits_one_deadline_and_kills_child() {
    let client = GeometerClient::spawn(native_executable(), "no-ack-test", "a0")
        .await
        .unwrap();
    let (_sender, receiver) = oneshot::channel();
    let deadline = tokio::time::Instant::now() + std::time::Duration::from_millis(25);
    assert!(matches!(
        client.await_shutdown_ack(deadline, receiver).await,
        Err(GeometerClientError::Process(_))
    ));
    assert!(client.inner.closed.load(Ordering::SeqCst));
}

#[tokio::test]
async fn shutdown_ack_without_process_exit_uses_remaining_deadline() {
    let client = GeometerClient::spawn(native_executable(), "ack-no-exit-test", "a0")
        .await
        .unwrap();
    let (sender, receiver) = oneshot::channel();
    assert!(sender.send(Ok(())).is_ok());
    let deadline = tokio::time::Instant::now() + std::time::Duration::from_millis(25);
    client.await_shutdown_ack(deadline, receiver).await.unwrap();
    assert!(matches!(
        client.await_child_exit(deadline).await,
        Err(GeometerClientError::Process(_))
    ));
    assert!(client.inner.closed.load(Ordering::SeqCst));
}

async fn assert_waiter_failure_type(failure: PendingFailure) {
    let client = GeometerClient::spawn(native_executable(), "typed-waiter-test", "a0")
        .await
        .unwrap();
    let (cancel_sender, cancel_receiver) = oneshot::channel();
    client
        .inner
        .cancellation
        .lock()
        .await
        .insert(91, cancel_sender);
    let (shutdown_sender, shutdown_receiver) = oneshot::channel();
    *client.inner.shutdown.lock().await = Some(ShutdownWaiter {
        sender: shutdown_sender,
        pending_at_request: 0,
        rejected_queued: 0,
        active_eligible: false,
    });
    let expected_protocol = matches!(failure, PendingFailure::Protocol(_));
    fail_connection(&client.inner, failure).await;
    let cancel = cancel_receiver.await.unwrap().unwrap_err();
    let shutdown = shutdown_receiver.await.unwrap().unwrap_err();
    assert_eq!(
        matches!(cancel, PendingFailure::Protocol(_)),
        expected_protocol
    );
    assert_eq!(
        matches!(shutdown, PendingFailure::Protocol(_)),
        expected_protocol
    );
}

#[test]
fn shutdown_ack_fields_must_match_observed_state() {
    let (sender, _receiver) = oneshot::channel();
    let mut waiter = ShutdownWaiter {
        sender,
        pending_at_request: 2,
        rejected_queued: 1,
        active_eligible: true,
    };
    let mut control = IpcShutdownAckA0 {
        status: "complete".to_owned(),
        active_request_completed: true,
        rejected_queued_request_count: 1,
    };
    assert!(shutdown_ack_matches(&control, &waiter));
    control.rejected_queued_request_count = 0;
    assert!(!shutdown_ack_matches(&control, &waiter));
    control.rejected_queued_request_count = 1;
    waiter.active_eligible = false;
    assert!(!shutdown_ack_matches(&control, &waiter));
}

#[test]
fn truncated_frame_failure_remains_protocol_typed_for_pending_calls() {
    assert!(matches!(
        frame_failure(
            "response read",
            &ipc::FrameError::Protocol("truncated frame payload".to_owned())
        ),
        PendingFailure::Protocol(_)
    ));
}

fn native_executable() -> PathBuf {
    if let Some(path) = std::env::var_os("GEOMETER_EXECUTABLE") {
        return PathBuf::from(path);
    }
    let root = Path::new(env!("CARGO_MANIFEST_DIR")).join("../../..");
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
    let name = if cfg!(windows) {
        "geometer.exe"
    } else {
        "geometer"
    };
    root.join("dist/native").join(platform).join(name)
}
