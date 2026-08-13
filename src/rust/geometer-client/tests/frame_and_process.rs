use std::path::{Path, PathBuf};
use std::process::Stdio;
use std::time::Duration;

use geometer_client::contracts;
use geometer_client::ipc::{self, Frame, FrameKind};
use geometer_client::{
    GeometerClient, GeometerClientError, ModelBoundsRequest, NORMALIZED_CATALOG_SHA256,
};
use tokio::io::{AsyncReadExt, AsyncWriteExt};
use tokio::process::Command;

#[tokio::test]
async fn shutdown_frame_matches_the_governed_exact_bytes() {
    let (mut writer, mut reader) = tokio::io::duplex(128);
    let frame = Frame {
        kind: FrameKind::Shutdown,
        request_id: 0,
        json: b"{}".to_vec(),
        attachments: Vec::new(),
    };
    ipc::write_frame(&mut writer, &frame).await.unwrap();
    let mut encoded = [0_u8; 50];
    tokio::io::AsyncReadExt::read_exact(&mut reader, &mut encoded)
        .await
        .unwrap();
    assert_eq!(
        encoded,
        [
            0x47, 0x4d, 0x49, 0x50, 0x43, 0x41, 0x30, 0x31, 0x30, 0x00, 0x00, 0x00, 0x08, 0x00,
            0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x02, 0x00, 0x00, 0x00,
            0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
            0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x7b, 0x7d,
        ]
    );
}

#[tokio::test]
async fn persistent_client_runs_model_bounds_twice_and_closes_cleanly() {
    let root = repository_root();
    let executable = native_executable(&root);
    assert!(executable.is_file(), "missing {}", executable.display());
    let model = tokio::fs::read(root.join("tests/fixtures/step/embedded_models/SOT-23.STEP"))
        .await
        .unwrap();
    let client = GeometerClient::spawn(&executable, "geometer-client-test", "a0")
        .await
        .unwrap();
    assert_eq!(client.welcome().catalog_sha256, NORMALIZED_CATALOG_SHA256);
    assert_eq!(client.welcome().ipc, "a0");
    assert!(
        !client
            .cancel(u64::MAX - 1, Some("unknown request probe"))
            .await
            .unwrap()
    );
    let malformed = client
        .execute(
            "geometry.model_bounds.a0",
            br#"{"unknown":true}"#,
            Vec::new(),
        )
        .await
        .unwrap();
    assert_failure_code(malformed.outcome, "geometer.contract.unknown_field");
    let options =
        contracts::encode_model_bounds_options_a0_json(&contracts::ModelBoundsOptionsA0 {
            format: None,
            model_transform: None,
        })
        .unwrap();
    let blocker = client
        .start_execute(
            "geometry.model_bounds.a0",
            &options,
            vec![model_attachment(model.clone())],
        )
        .await
        .unwrap();
    let queued = client
        .start_execute(
            "geometry.model_bounds.a0",
            &options,
            vec![model_attachment(model.clone())],
        )
        .await
        .unwrap();
    assert!(
        queued
            .cancel(Some("queued cancellation probe"))
            .await
            .unwrap()
    );
    assert!(matches!(
        queued.wait().await,
        Err(GeometerClientError::Cancelled)
    ));
    blocker.wait().await.unwrap();
    let timeout_call = client
        .start_execute(
            "geometry.model_bounds.a0",
            &options,
            vec![model_attachment(model.clone())],
        )
        .await
        .unwrap();
    assert!(matches!(
        timeout_call.wait_timeout(Duration::ZERO).await,
        Err(GeometerClientError::Timeout { .. })
    ));
    let first = client
        .model_bounds(ModelBoundsRequest::step(model.clone()))
        .await
        .unwrap();
    let second = client
        .model_bounds(ModelBoundsRequest::step(model))
        .await
        .unwrap();
    assert_eq!(first.source.hash, second.source.hash);
    assert_eq!(first.bounds, second.bounds);
    client.close().await.unwrap();
}

#[tokio::test]
async fn close_race_never_orphans_an_accepted_request() {
    let root = repository_root();
    let model = tokio::fs::read(root.join("tests/fixtures/step/embedded_models/SOT-23.STEP"))
        .await
        .unwrap();
    let client = GeometerClient::spawn(native_executable(&root), "close-race-test", "a0")
        .await
        .unwrap();
    let operation_client = client.clone();
    let operation = tokio::spawn(async move {
        operation_client
            .model_bounds(ModelBoundsRequest::step(model))
            .await
    });
    let close = tokio::spawn(async move { client.close().await });
    let (operation, close) = tokio::join!(operation, close);
    close.unwrap().unwrap();
    match operation.unwrap() {
        Ok(_) | Err(GeometerClientError::Closed) => {}
        Err(GeometerClientError::Operation { diagnostics, .. })
            if diagnostics
                .iter()
                .any(|value| value.code == "geometer.transport.server_shutting_down") => {}
        Err(error) => panic!("unexpected close-race outcome: {error}"),
    }
}

#[tokio::test]
async fn wrong_direction_frame_emits_protocol_error_and_exits_nonzero() {
    let root = repository_root();
    let mut child = Command::new(native_executable(&root))
        .args(["serve", "--stdio"])
        .stdin(Stdio::piped())
        .stdout(Stdio::piped())
        .stderr(Stdio::piped())
        .spawn()
        .unwrap();
    let mut stdin = child.stdin.take().unwrap();
    let mut stdout = child.stdout.take().unwrap();
    ipc::write_frame(
        &mut stdin,
        &Frame {
            kind: FrameKind::Hello,
            request_id: 0,
            json: br#"{"client_name":"raw-test","client_version":"a0","protocols":["a0"]}"#
                .to_vec(),
            attachments: Vec::new(),
        },
    )
    .await
    .unwrap();
    assert_eq!(
        ipc::read_frame(&mut stdout).await.unwrap().unwrap().kind,
        FrameKind::Welcome
    );
    ipc::write_frame(
        &mut stdin,
        &Frame {
            kind: FrameKind::Welcome,
            request_id: 0,
            json: b"{}".to_vec(),
            attachments: Vec::new(),
        },
    )
    .await
    .unwrap();
    let error = ipc::read_frame(&mut stdout).await.unwrap().unwrap();
    assert_eq!(error.kind, FrameKind::ProtocolError);
    assert!(!child.wait().await.unwrap().success());
}

#[tokio::test]
async fn duplicate_request_attachment_names_are_correlated_and_nonfatal() {
    let root = repository_root();
    let mut child = Command::new(native_executable(&root))
        .args(["serve", "--stdio"])
        .stdin(Stdio::piped())
        .stdout(Stdio::piped())
        .stderr(Stdio::piped())
        .spawn()
        .unwrap();
    let mut stdin = child.stdin.take().unwrap();
    let mut stdout = child.stdout.take().unwrap();
    ipc::write_frame(
        &mut stdin,
        &Frame {
            kind: FrameKind::Hello,
            request_id: 0,
            json: br#"{"client_name":"raw-test","client_version":"a0","protocols":["a0"]}"#
                .to_vec(),
            attachments: Vec::new(),
        },
    )
    .await
    .unwrap();
    assert_eq!(
        ipc::read_frame(&mut stdout).await.unwrap().unwrap().kind,
        FrameKind::Welcome
    );

    let request = Frame {
        kind: FrameKind::Request,
        request_id: 77,
        json: br#"{"operation":"geometry.model_bounds.a0","request":{}}"#.to_vec(),
        attachments: vec![
            ipc::Attachment {
                name: "model".to_owned(),
                media_type: "application/step".to_owned(),
                data: vec![1],
            },
            ipc::Attachment {
                name: "other".to_owned(),
                media_type: "application/step".to_owned(),
                data: vec![2],
            },
        ],
    };
    let encoded_size = request.encoded_size().unwrap();
    let (mut encoded_writer, mut encoded_reader) = tokio::io::duplex(encoded_size);
    ipc::write_frame(&mut encoded_writer, &request)
        .await
        .unwrap();
    let mut encoded = vec![0_u8; encoded_size];
    encoded_reader.read_exact(&mut encoded).await.unwrap();
    let other = encoded
        .windows(5)
        .position(|window| window == b"other")
        .expect("encoded attachment name");
    encoded[other..other + 5].copy_from_slice(b"model");
    stdin.write_all(&encoded).await.unwrap();
    stdin.flush().await.unwrap();

    let response = ipc::read_frame(&mut stdout).await.unwrap().unwrap();
    assert_eq!(response.kind, FrameKind::Response);
    assert_eq!(response.request_id, 77);
    let outcome = contracts::decode_operation_outcome_a0_json(&response.json).unwrap();
    let contracts::OperationOutcomeA0::Failure(failure) = outcome else {
        panic!("duplicate attachment names unexpectedly succeeded");
    };
    assert_eq!(
        failure.diagnostics[0].code,
        "geometer.contract.duplicate_attachment"
    );

    ipc::write_frame(
        &mut stdin,
        &Frame {
            kind: FrameKind::Shutdown,
            request_id: 0,
            json: b"{}".to_vec(),
            attachments: Vec::new(),
        },
    )
    .await
    .unwrap();
    assert_eq!(
        ipc::read_frame(&mut stdout).await.unwrap().unwrap().kind,
        FrameKind::ShutdownAck
    );
    assert!(child.wait().await.unwrap().success());
}

fn native_executable(root: &Path) -> PathBuf {
    let platform = if cfg!(windows) {
        "windows-x64"
    } else if cfg!(target_os = "macos") && cfg!(target_arch = "aarch64") {
        "macos-arm64"
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

fn model_attachment(data: Vec<u8>) -> ipc::Attachment {
    ipc::Attachment {
        name: "model".to_owned(),
        media_type: "application/step".to_owned(),
        data,
    }
}

fn assert_failure_code(outcome: contracts::OperationOutcomeA0, code: &str) {
    let contracts::OperationOutcomeA0::Failure(failure) = outcome else {
        panic!("operation unexpectedly succeeded");
    };
    assert_eq!(failure.diagnostics[0].code, code);
}

fn repository_root() -> PathBuf {
    Path::new(env!("CARGO_MANIFEST_DIR"))
        .join("../../..")
        .canonicalize()
        .unwrap()
}
