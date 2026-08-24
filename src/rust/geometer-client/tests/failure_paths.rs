use std::path::{Path, PathBuf};
use std::process::Stdio;
use std::time::Duration;

use geometer_client::contracts;
use geometer_client::ipc::{self, Frame, FrameKind};
use geometer_client::{GeometerClient, GeometerClientError, ModelBoundsRequest};
use tokio::io::AsyncWriteExt;
use tokio::process::{Child, ChildStdin, ChildStdout, Command};

#[tokio::test]
async fn incompatible_handshake_is_fatal_and_flushed() {
    let (mut child, mut stdin, mut stdout) = spawn_raw().await;
    write_control(
        &mut stdin,
        FrameKind::Hello,
        0,
        br#"{"client_name":"failure-test","client_version":"a0","protocols":["b0"]}"#,
    )
    .await;
    let error = read_required(&mut stdout).await;
    assert_eq!(error.kind, FrameKind::ProtocolError);
    assert!(!wait_bounded(&mut child).await.success());
}

#[tokio::test]
async fn oversized_fixed_header_is_fatal_without_payload_read() {
    let (mut child, mut stdin, mut stdout) = spawn_raw().await;
    let mut header = [0_u8; ipc::HEADER_SIZE];
    header[..8].copy_from_slice(b"GMIPCA01");
    header[8..10].copy_from_slice(&(ipc::HEADER_SIZE as u16).to_le_bytes());
    header[12..14].copy_from_slice(&(FrameKind::Hello as u16).to_le_bytes());
    header[24..28].copy_from_slice(&((ipc::MAX_JSON_BYTES as u32) + 1).to_le_bytes());
    stdin.write_all(&header).await.unwrap();
    stdin.flush().await.unwrap();
    let error = read_required(&mut stdout).await;
    assert_eq!(error.kind, FrameKind::ProtocolError);
    assert!(!wait_bounded(&mut child).await.success());
}

#[tokio::test]
async fn raw_invalid_request_remains_a_correlated_operation_failure() {
    let (mut child, mut stdin, mut stdout) = spawn_raw().await;
    handshake(&mut stdin, &mut stdout).await;
    write_control(
        &mut stdin,
        FrameKind::Request,
        7,
        br#"{"operation":"geometry.model_bounds.a0","request":{"unknown":true}}"#,
    )
    .await;
    let response = read_required(&mut stdout).await;
    assert_eq!(response.kind, FrameKind::Response);
    assert_eq!(response.request_id, 7);
    let outcome = contracts::decode_operation_outcome_a0_json(&response.json).unwrap();
    let contracts::OperationOutcomeA0::Failure(failure) = outcome else {
        panic!("invalid raw request unexpectedly succeeded");
    };
    assert_eq!(
        failure.diagnostics[0].code,
        "geometer.contract.union_mismatch"
    );
    write_control(&mut stdin, FrameKind::Shutdown, 0, b"{}").await;
    assert_eq!(
        read_required(&mut stdout).await.kind,
        FrameKind::ShutdownAck
    );
    assert!(wait_bounded(&mut child).await.success());
}

#[tokio::test]
async fn broken_stdout_forces_a_nonzero_server_exit() {
    let (mut child, mut stdin, mut stdout) = spawn_raw().await;
    handshake(&mut stdin, &mut stdout).await;
    drop(stdout);
    write_control(&mut stdin, FrameKind::Cancel, 99, b"{}").await;
    assert!(!wait_bounded(&mut child).await.success());
}

#[tokio::test]
async fn forced_client_termination_fails_pending_work() {
    let root = repository_root();
    let model = tokio::fs::read(root.join("tests/fixtures/step/embedded_models/SOT-23.STEP"))
        .await
        .unwrap();
    let client = GeometerClient::spawn(native_executable(&root), "forced-exit-test", "a0")
        .await
        .unwrap();
    let mut calls = Vec::new();
    for _ in 0..8 {
        calls.push(
            client
                .start_execute(
                    "geometry.model_bounds.a0",
                    b"{}",
                    vec![ipc::Attachment {
                        name: "model".to_owned(),
                        media_type: "application/step".to_owned(),
                        data: model.clone(),
                    }],
                )
                .await
                .unwrap(),
        );
    }
    client.terminate().await.unwrap();
    let mut process_failures = 0;
    for call in calls {
        match call.wait().await {
            Ok(_) => {}
            Err(GeometerClientError::Process(_)) => process_failures += 1,
            Err(error) => panic!("unexpected forced-termination result: {error}"),
        }
    }
    assert!(process_failures > 0);
    assert!(matches!(
        client
            .model_bounds(ModelBoundsRequest::step(Vec::new()))
            .await,
        Err(GeometerClientError::Closed)
    ));
}

#[tokio::test]
async fn shutdown_deadline_forces_exit_and_resolves_pending_work() {
    let root = repository_root();
    let model = test_model(&root).await;
    let server = test_server_executable(&root, "geometer_ipc_a0_deadline_test_server");
    let client = GeometerClient::spawn(server, "deadline-test", "a0")
        .await
        .unwrap();
    let call = client
        .start_execute(
            "geometry.model_bounds.a0",
            b"{}",
            vec![model_attachment(model)],
        )
        .await
        .unwrap();
    wait_for_stderr(&client, "delaying an active request").await;
    let close = tokio::time::timeout(Duration::from_secs(5), client.close())
        .await
        .expect("deadline test server did not terminate");
    assert!(matches!(close, Err(GeometerClientError::Process(_))));
    assert!(matches!(
        call.wait().await,
        Err(GeometerClientError::Process(_))
    ));
    wait_for_stderr(&client, "shutdown exceeded its A0 deadline").await;
}

#[tokio::test]
async fn unexpected_child_exit_resolves_pending_work_and_closes_client() {
    let root = repository_root();
    let model = test_model(&root).await;
    let server = test_server_executable(&root, "geometer_ipc_a0_unexpected_exit_test_server");
    let client = GeometerClient::spawn(server, "unexpected-exit-test", "a0")
        .await
        .unwrap();
    let call = client
        .start_execute(
            "geometry.model_bounds.a0",
            b"{}",
            vec![model_attachment(model)],
        )
        .await
        .unwrap();
    assert!(matches!(
        tokio::time::timeout(Duration::from_secs(5), call.wait())
            .await
            .expect("unexpected child exit did not resolve the pending call"),
        Err(GeometerClientError::Process(_))
    ));
    assert!(matches!(
        client
            .model_bounds(ModelBoundsRequest::step(Vec::new()))
            .await,
        Err(GeometerClientError::Closed)
    ));
    wait_for_stderr(&client, "exiting with an active request").await;
}

async fn spawn_raw() -> (Child, ChildStdin, ChildStdout) {
    let mut child = Command::new(native_executable(&repository_root()))
        .args(["serve", "--stdio"])
        .stdin(Stdio::piped())
        .stdout(Stdio::piped())
        .stderr(Stdio::piped())
        .kill_on_drop(true)
        .spawn()
        .unwrap();
    let stdin = child.stdin.take().unwrap();
    let stdout = child.stdout.take().unwrap();
    (child, stdin, stdout)
}

async fn handshake(stdin: &mut ChildStdin, stdout: &mut ChildStdout) {
    write_control(
        stdin,
        FrameKind::Hello,
        0,
        br#"{"client_name":"failure-test","client_version":"a0","protocols":["a0"]}"#,
    )
    .await;
    assert_eq!(read_required(stdout).await.kind, FrameKind::Welcome);
}

async fn write_control(stdin: &mut ChildStdin, kind: FrameKind, request_id: u64, json: &[u8]) {
    ipc::write_frame(
        stdin,
        &Frame {
            kind,
            request_id,
            json: json.to_vec(),
            attachments: Vec::new(),
        },
    )
    .await
    .unwrap();
}

async fn read_required(stdout: &mut ChildStdout) -> Frame {
    tokio::time::timeout(Duration::from_secs(5), ipc::read_frame(stdout))
        .await
        .expect("server response timed out")
        .unwrap()
        .expect("server closed before its response")
}

async fn wait_bounded(child: &mut Child) -> std::process::ExitStatus {
    tokio::time::timeout(Duration::from_secs(5), child.wait())
        .await
        .expect("server exit timed out")
        .unwrap()
}

fn native_executable(root: &Path) -> PathBuf {
    root.join("dist/native")
        .join(platform_name())
        .join(executable_name("geometer"))
}

fn test_server_executable(root: &Path, name: &str) -> PathBuf {
    let relative = Path::new("tests/cpp").join(executable_name(name));
    for build_root in [
        root.join("build"),
        root.join(format!("build-native-{}", platform_name())),
    ] {
        let candidate = build_root.join(&relative);
        if candidate.is_file() {
            return candidate;
        }
    }
    panic!("missing native IPC test server {name}");
}

fn platform_name() -> &'static str {
    if cfg!(windows) {
        "windows-x64"
    } else if cfg!(target_os = "macos") && cfg!(target_arch = "aarch64") {
        "macos-arm64"
    } else if cfg!(target_os = "linux") && cfg!(target_arch = "aarch64") {
        "linux-arm64"
    } else {
        "linux-x64"
    }
}

fn executable_name(name: &str) -> String {
    if cfg!(windows) {
        format!("{name}.exe")
    } else {
        name.to_owned()
    }
}

fn model_attachment(data: Vec<u8>) -> ipc::Attachment {
    ipc::Attachment {
        name: "model".to_owned(),
        media_type: "application/step".to_owned(),
        data,
    }
}

async fn test_model(root: &Path) -> Vec<u8> {
    tokio::fs::read(root.join("tests/fixtures/step/embedded_models/SOT-23.STEP"))
        .await
        .unwrap()
}

async fn wait_for_stderr(client: &GeometerClient, expected: &str) {
    tokio::time::timeout(Duration::from_secs(5), async {
        loop {
            if client.stderr_text().await.contains(expected) {
                return;
            }
            tokio::time::sleep(Duration::from_millis(10)).await;
        }
    })
    .await
    .unwrap_or_else(|_| panic!("stderr did not contain {expected:?}"));
}

fn repository_root() -> PathBuf {
    Path::new(env!("CARGO_MANIFEST_DIR"))
        .join("../../..")
        .canonicalize()
        .unwrap()
}
