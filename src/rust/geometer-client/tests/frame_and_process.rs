use std::path::{Path, PathBuf};
use std::process::Stdio;
use std::time::Duration;

use geometer_client::contracts;
use geometer_client::ipc::{self, Frame, FrameKind};
use geometer_client::{
    GeometerClient, GeometerClientError, ModelBoundsRequest, NORMALIZED_CATALOG_SHA256,
};
use serde::Deserialize;
use serde_json::Value;
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
        .start_execute(
            "geometry.model_bounds.a0",
            br#"{"unknown":true}"#,
            Vec::new(),
        )
        .await;
    let Err(malformed) = malformed else {
        panic!("generated request envelope accepted an unknown request field");
    };
    assert!(matches!(malformed, GeometerClientError::Contract(_)));
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
async fn governed_operation_vectors_match_executable_ipc() {
    let root = repository_root();
    let vector_root = root.join("tests/contracts/vectors");
    let manifest: OperationManifest = serde_json::from_slice(
        &tokio::fs::read(vector_root.join("manifest.json"))
            .await
            .unwrap(),
    )
    .unwrap();
    assert_eq!(manifest.operation_vectors.len(), 2);
    let client = GeometerClient::spawn(native_executable(&root), "operation-vector-test", "a0")
        .await
        .unwrap();
    for vector in manifest.operation_vectors {
        if !vector
            .runtimes
            .iter()
            .any(|value| value == "executable_ipc")
        {
            continue;
        }
        let request = tokio::fs::read(vector_root.join(&vector.request_file))
            .await
            .unwrap();
        let mut attachments = Vec::new();
        for attachment in vector.attachments {
            attachments.push(ipc::Attachment {
                name: attachment.name,
                media_type: attachment.media_type,
                data: tokio::fs::read(root.join(attachment.repository_file))
                    .await
                    .unwrap(),
            });
        }
        let response = client
            .execute(&vector.operation, &request, attachments)
            .await
            .unwrap();
        assert!(response.attachments.is_empty(), "{}", vector.id);
        match response.outcome {
            contracts::OperationOutcomeA0::Success(success) => {
                assert_eq!(vector.expected, "success", "{}", vector.id);
                assert_eq!(
                    vector.excluded_fields,
                    ["/result/timings/model_read_ms", "/result/timings/bounds_ms"],
                    "{}",
                    vector.id
                );
                let contracts::OperationResultValueA0::ModelBounds(result) = success.result;
                let mut actual = serde_json::to_value(result).unwrap();
                let mut expected: Value = serde_json::from_slice(
                    &tokio::fs::read(
                        vector_root.join(vector.expected_result_file.as_ref().unwrap()),
                    )
                    .await
                    .unwrap(),
                )
                .unwrap();
                actual.as_object_mut().unwrap().remove("timings");
                expected.as_object_mut().unwrap().remove("timings");
                assert_json_close(&actual, &expected, vector.tolerance.as_ref().unwrap(), "");
            }
            contracts::OperationOutcomeA0::Failure(failure) => {
                assert_eq!(vector.expected, "failure", "{}", vector.id);
                assert_eq!(failure.diagnostics.len(), 1, "{}", vector.id);
                let actual = &failure.diagnostics[0];
                let expected = vector.expected_diagnostic.as_ref().unwrap();
                assert_eq!(actual.code, expected.code, "{}", vector.id);
                assert_eq!(actual.category, contracts::DiagnosticCategory::Operation);
                assert_eq!(actual.retryable, expected.retryable, "{}", vector.id);
                assert!(
                    actual.path.is_none() && expected.path == "absent",
                    "{}",
                    vector.id
                );
            }
        }
    }
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

fn model_attachment(data: Vec<u8>) -> ipc::Attachment {
    ipc::Attachment {
        name: "model".to_owned(),
        media_type: "application/step".to_owned(),
        data,
    }
}

#[derive(Deserialize)]
struct OperationManifest {
    operation_vectors: Vec<OperationVector>,
}

#[derive(Deserialize)]
struct OperationVector {
    id: String,
    operation: String,
    request_file: String,
    attachments: Vec<VectorAttachment>,
    expected: String,
    expected_result_file: Option<String>,
    expected_diagnostic: Option<ExpectedDiagnostic>,
    excluded_fields: Vec<String>,
    tolerance: Option<Tolerance>,
    runtimes: Vec<String>,
}

#[derive(Deserialize)]
struct VectorAttachment {
    name: String,
    media_type: String,
    repository_file: String,
}

#[derive(Deserialize)]
struct ExpectedDiagnostic {
    code: String,
    path: String,
    retryable: bool,
}

#[derive(Deserialize)]
struct Tolerance {
    absolute: f64,
    relative: f64,
}

fn assert_json_close(actual: &Value, expected: &Value, tolerance: &Tolerance, path: &str) {
    match (actual, expected) {
        (Value::Number(actual), Value::Number(expected)) => {
            let actual = actual.as_f64().unwrap();
            let expected = expected.as_f64().unwrap();
            let allowed =
                tolerance.absolute + tolerance.relative * f64::max(actual.abs(), expected.abs());
            assert!(
                (actual - expected).abs() <= allowed,
                "numeric mismatch at {path}: {actual} != {expected}"
            );
        }
        (Value::Array(actual), Value::Array(expected)) => {
            assert_eq!(
                actual.len(),
                expected.len(),
                "array length mismatch at {path}"
            );
            for (index, (actual, expected)) in actual.iter().zip(expected).enumerate() {
                assert_json_close(actual, expected, tolerance, &format!("{path}/{index}"));
            }
        }
        (Value::Object(actual), Value::Object(expected)) => {
            assert_eq!(
                actual.len(),
                expected.len(),
                "object size mismatch at {path}"
            );
            for (key, expected) in expected {
                assert_json_close(&actual[key], expected, tolerance, &format!("{path}/{key}"));
            }
        }
        _ => assert_eq!(actual, expected, "exact mismatch at {path}"),
    }
}

fn repository_root() -> PathBuf {
    Path::new(env!("CARGO_MANIFEST_DIR"))
        .join("../../..")
        .canonicalize()
        .unwrap()
}
