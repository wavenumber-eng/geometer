use std::path::{Path, PathBuf};
use std::process::Stdio;
use std::time::Duration;

use geometer_client::contracts;
use geometer_client::ipc::{self, Frame, FrameKind};
use geometer_client::{
    GeometerClient, GeometerClientError, IndexedTriangleMeshA0, MeshHlrProjectionRequestA0,
    ModelBoundsRequest, NORMALIZED_CATALOG_SHA256,
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
async fn persistent_client_runs_typed_mesh_hlr_projection() {
    let root = repository_root();
    let executable = native_executable(&root);
    assert!(executable.is_file(), "missing {}", executable.display());
    let client = GeometerClient::spawn(&executable, "mesh-hlr-client-test", "a0")
        .await
        .unwrap();
    let options =
        contracts::decode_hlr_projection_options_a0_json(br#"{"output_detail":true}"#).unwrap();
    let request = MeshHlrProjectionRequestA0::from_mesh(
        &IndexedTriangleMeshA0 {
            positions: vec![[0.0, 0.0, 0.0], [10.0, 0.0, 0.0], [0.0, 10.0, 0.0]],
            triangles: vec![[0, 1, 2]],
            source_faces: Some(vec![1]),
        },
        options,
    )
    .unwrap();

    let result = client.mesh_hlr_projection_a0(request).await.unwrap();

    assert_eq!(result.schema, "geometry.hlr_projection.result.a0");
    assert!(matches!(
        result.source.kind,
        contracts::HlrSourceKind::IndexedMesh
    ));
    assert!(!result.views.is_empty());
    client.close().await.unwrap();
}

#[tokio::test]
async fn persistent_client_dispatches_generated_topology_contracts() {
    let root = repository_root();
    let client = GeometerClient::spawn(native_executable(&root), "generated-dispatch-test", "a0")
        .await
        .unwrap();
    let step = std::fs::read(root.join("tests/fixtures/step/embedded_models/SOT-23.STEP")).unwrap();
    let opened = client
        .execute(
            "geometry.step_topology.open.a0",
            br#"{"schema":"geometry.step_topology.open.request.a0"}"#,
            vec![ipc::Attachment {
                name: "step".to_owned(),
                media_type: "application/step".to_owned(),
                data: step,
            }],
        )
        .await
        .unwrap();
    let geometer_client::OperationOutcome::A0(contracts::OperationOutcomeA0::Success(success)) =
        opened.outcome
    else {
        panic!("topology open failed");
    };
    let contracts::OperationResultValueA0::StepTopologyOpen(open) = success.result else {
        panic!("wrong topology open result");
    };
    let request = contracts::encode_json(&contracts::StepTopologyCloseRequestA0 {
        schema: "geometry.step_topology.close.request.a0".to_owned(),
        session: open.session,
    })
    .unwrap();
    let closed = client
        .execute("geometry.step_topology.close.a0", &request, Vec::new())
        .await
        .unwrap();
    let geometer_client::OperationOutcome::A0(contracts::OperationOutcomeA0::Success(success)) =
        closed.outcome
    else {
        panic!("topology close failed");
    };
    assert!(matches!(
        success.result,
        contracts::OperationResultValueA0::StepTopologyClose(_)
    ));
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
        let mut source_hash = None;
        for attachment in vector.attachments {
            let data = tokio::fs::read(root.join(attachment.repository_file))
                .await
                .unwrap();
            if attachment.name == "model" {
                source_hash = Some(fnv1a64(&data));
            }
            attachments.push(ipc::Attachment {
                name: attachment.name,
                media_type: attachment.media_type,
                data,
            });
        }
        let response = client
            .execute(&vector.operation, &request, attachments)
            .await
            .unwrap();
        assert!(response.attachments.is_empty(), "{}", vector.id);
        match response.outcome {
            geometer_client::OperationOutcome::A0(contracts::OperationOutcomeA0::Success(
                success,
            )) => {
                assert_eq!(vector.expected, "success", "{}", vector.id);
                assert_eq!(
                    vector.excluded_fields,
                    ["/result/timings/model_read_ms", "/result/timings/bounds_ms"],
                    "{}",
                    vector.id
                );
                let contracts::OperationResultValueA0::ModelBounds(result) = success.result else {
                    panic!("model-bounds vector returned a packed projection");
                };
                assert_eq!(vector.computed_fields.len(), 1, "{}", vector.id);
                let computed = &vector.computed_fields[0];
                assert_eq!(computed.path, "/result/source/hash", "{}", vector.id);
                assert_eq!(computed.oracle, "fnv1a64_attachment", "{}", vector.id);
                assert_eq!(computed.attachment, "model", "{}", vector.id);
                assert_eq!(computed.comparison, "exact", "{}", vector.id);
                assert_eq!(result.source.hash, source_hash.unwrap(), "{}", vector.id);
                let mut actual = serde_json::to_value(result).unwrap();
                let expected_path = vector_root.join(vector.expected_result_file.as_ref().unwrap());
                let expected_bytes = tokio::fs::read(expected_path).await.unwrap();
                let mut expected: Value = serde_json::from_slice(&expected_bytes).unwrap();
                assert_eq!(
                    expected.pointer("/source/hash").unwrap(),
                    "computed:fnv1a64:model",
                    "{}",
                    vector.id
                );
                *expected.pointer_mut("/source/hash").unwrap() =
                    actual.pointer("/source/hash").unwrap().clone();
                actual.as_object_mut().unwrap().remove("timings");
                expected.as_object_mut().unwrap().remove("timings");
                assert_json_close(&actual, &expected, vector.tolerance.as_ref().unwrap(), "");
            }
            geometer_client::OperationOutcome::A0(contracts::OperationOutcomeA0::Failure(
                failure,
            )) => {
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
            geometer_client::OperationOutcome::B0(_) => panic!("unexpected B0 response"),
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
    if let Some(path) = std::env::var_os("GEOMETER_EXECUTABLE") {
        return PathBuf::from(path);
    }
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
    computed_fields: Vec<ComputedField>,
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
struct ComputedField {
    path: String,
    oracle: String,
    attachment: String,
    comparison: String,
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

fn fnv1a64(data: &[u8]) -> String {
    let mut hash = 14_695_981_039_346_656_037_u64;
    for value in data {
        hash ^= u64::from(*value);
        hash = hash.wrapping_mul(1_099_511_628_211);
    }
    format!("fnv1a64:{hash:016x}")
}

fn repository_root() -> PathBuf {
    Path::new(env!("CARGO_MANIFEST_DIR"))
        .join("../../..")
        .canonicalize()
        .unwrap()
}

#[tokio::test]
async fn typed_tessellation_preserves_colors_and_rejects_limits_without_poisoning() {
    let root = repository_root();
    let client = GeometerClient::spawn(native_executable(&root), "tessellation-test", "a0")
        .await
        .unwrap();
    let model =
        std::fs::read(root.join("tests/fixtures/step/embedded_models/SOT-23.STEP")).unwrap();
    let request = geometer_client::ModelTessellationRequest::step(model);
    let first = client.model_tessellation(request.clone()).await.unwrap();
    let second = client.model_tessellation(request.clone()).await.unwrap();
    assert_eq!(first.mesh_collection, second.mesh_collection);
    assert_eq!(first.metadata, second.metadata);
    assert_eq!(first.mesh_collection.length_unit, "millimeter");
    assert!(first.metadata.triangles > 0);
    let color = &first.mesh_collection.meshes[0].materials[0].color;
    assert!(
        first
            .mesh_collection
            .meshes
            .iter()
            .any(|mesh| mesh.materials[0].color != *color)
    );
    let mut limited = request.clone();
    limited.options.max_triangles = Some(1);
    assert!(matches!(
        client.model_tessellation(limited).await,
        Err(GeometerClientError::Operation { .. })
    ));
    assert!(matches!(
        client
            .model_tessellation(geometer_client::ModelTessellationRequest::step(
                b"bad STEP".to_vec()
            ))
            .await,
        Err(GeometerClientError::Operation { .. })
    ));
    assert!(client.model_tessellation(request).await.is_ok());
    client.close().await.unwrap();
}

#[tokio::test]
async fn typed_step_illustration_uses_generated_values_and_recovers_from_limits() {
    use geometer_client::contracts::{
        MeshIllustrationInputA0, MeshIllustrationPrepareOptions, MeshIllustrationView,
    };
    let root = repository_root();
    let client = GeometerClient::spawn(native_executable(&root), "illustration-test", "a0")
        .await
        .unwrap();
    let model =
        std::fs::read(root.join("tests/fixtures/step/embedded_models/SOT-23.STEP")).unwrap();
    let tessellated = client
        .model_tessellation(geometer_client::ModelTessellationRequest::step(model))
        .await
        .unwrap();
    let input = MeshIllustrationInputA0 {
        schema: "geometry.mesh_illustration.input.a0".to_owned(),
        meshes: tessellated.mesh_collection.meshes,
        view: MeshIllustrationView {
            direction: [0.4, 0.7, 1.0],
            up: [0.0, 1.0, 0.0],
            mirror_x: None,
        },
        prepare: None,
        style: None,
        svg: None,
    };
    let result = client.mesh_illustration_a0(input.clone()).await.unwrap();
    assert!(result.stats.triangles > 0 && result.stats.surface_draws > 0);
    assert!(result.svg.contains("<svg") && result.svg.contains("</svg>"));
    assert_eq!(
        result,
        client.mesh_illustration_a0(input.clone()).await.unwrap()
    );
    // Node is a conformance-test tool only, never a native runtime dependency.
    let mut reference = std::process::Command::new("node")
        .current_dir(&root)
        .arg("tests/typescript/native_illustration_reference.mjs")
        .stdin(std::process::Stdio::piped())
        .stdout(std::process::Stdio::piped())
        .spawn()
        .unwrap();
    std::io::Write::write_all(
        &mut reference.stdin.take().unwrap(),
        &geometer_client::contracts::encode_json(&input).unwrap(),
    )
    .unwrap();
    let expected = reference.wait_with_output().unwrap();
    assert!(expected.status.success());
    let expected =
        geometer_client::contracts::decode_mesh_illustration_result_a0_json(&expected.stdout)
            .unwrap();
    assert_eq!(result, expected);
    let mut limited = input.clone();
    limited.prepare = Some(MeshIllustrationPrepareOptions {
        max_triangles: Some(1),
        weld_tolerance: None,
    });
    assert!(matches!(
        client.mesh_illustration_a0(limited).await,
        Err(GeometerClientError::Operation { .. })
    ));
    assert_eq!(result, client.mesh_illustration_a0(input).await.unwrap());
    client.close().await.unwrap();
}
