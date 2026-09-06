from __future__ import annotations

import json
import os
import platform
import subprocess
import sys
import tarfile
from pathlib import Path


ROOT = Path(__file__).resolve().parents[2]
MANIFEST = ROOT / "src" / "rust" / "geometer-client" / "Cargo.toml"


def run(*args: str, cwd: Path = ROOT) -> None:
    env = os.environ.copy()
    if args and args[0] == "cargo":
        env.setdefault("CARGO_BUILD_JOBS", "1")
    subprocess.run(args, cwd=cwd, env=env, check=True)


def test_rust_format_lint_and_live_conformance() -> None:
    run("cargo", "fmt", "--manifest-path", str(MANIFEST), "--all", "--", "--check")
    run(
        "cargo",
        "clippy",
        "--manifest-path",
        str(MANIFEST),
        "--all-targets",
        "--locked",
        "--",
        "-D",
        "warnings",
    )
    run("cargo", "test", "--manifest-path", str(MANIFEST), "--locked")
    run("wn-dev-std", "audit", str(MANIFEST.parent), "--scope", "language")


def test_clean_external_consumer_runs_analytic_and_illustration_ipc(tmp_path: Path) -> None:
    crate = MANIFEST.parent
    package_dir = tmp_path / "package"
    package_dir.mkdir()
    run("cargo", "package", "--manifest-path", str(MANIFEST), "--allow-dirty")
    archives = sorted((crate / "target" / "package").glob("geometer-client-*.crate"))
    assert archives
    with tarfile.open(archives[-1], "r:gz") as archive:
        if sys.version_info >= (3, 12):
            archive.extractall(package_dir, filter="data")
        else:
            archive.extractall(package_dir)
    unpacked = next(package_dir.iterdir())

    consumer = tmp_path / "consumer"
    (consumer / "src").mkdir(parents=True)
    cargo_toml = {
        "package": {"name": "geometer-client-consumer", "version": "0.0.0", "edition": "2024"},
        "dependencies": {
            "geometer-client": {"path": str(unpacked)},
            "tokio": {"version": "1.47", "features": ["macros", "rt-multi-thread"]},
        },
    }
    (consumer / "Cargo.toml").write_text(_toml(cargo_toml), encoding="utf-8")
    (consumer / "src" / "main.rs").write_text(
        "use geometer_client::contracts::{self, AnalyticPlanarBooleanBatchRequestA0, "
        "AnalyticPlanarBooleanJobResult, JobId, ModelBoundsOptionsA0, PointNm};\n"
        "use geometer_client::GeometerClient;\n"
        "#[tokio::main]\n"
        "async fn main() { "
        'let executable = std::env::args_os().nth(1).expect("missing geometer executable"); '
        "let value = ModelBoundsOptionsA0 { format: None, model_transform: None }; "
        "let _ = geometer_client::contracts::encode_model_bounds_options_a0_json(&value).unwrap(); "
        "let id = JobId::new(1).unwrap(); let point = PointNm { x: i64::MIN, y: i64::MAX }; "
        "let empty = AnalyticPlanarBooleanBatchRequestA0 { jobs: vec![], relationship_queries: vec![] }; "
        "let packet = geometer_client::encode_analytic_planar_boolean_batch_request_a0_packet(&empty).unwrap(); "
        "let error = geometer_client::decode_analytic_planar_boolean_batch_result_a0_packet(&packet).unwrap_err(); "
        "assert_eq!((id.get(), point.x), (1, i64::MIN)); "
        "assert_eq!(error.kind(), geometer_client::AnalyticPacketErrorKind::InvalidPacket); "
        "let request = contracts::AnalyticPlanarBooleanBatchRequestA0 { "
        "jobs: vec![contracts::AnalyticPlanarBooleanJob { job_id: contracts::JobId::new(1).unwrap(), "
        "stages: vec![contracts::AnalyticPlanarBooleanStage { stage_id: contracts::StageId::new(1).unwrap(), "
        "operation: contracts::StageOperation::UnionStage, "
        "operands: vec![contracts::AnalyticPlanarOperand::Disk(contracts::DiskOperand { "
        'operand_id: contracts::OperandId::new(1).unwrap(), kind: "disk".to_owned(), '
        "feature_id: contracts::FeatureId::new(1).unwrap(), "
        "center: contracts::PointNm { x: 0, y: 0 }, radius_nm: 1_000_000 })] }] }], "
        "relationship_queries: vec![] }; "
        'let client = GeometerClient::spawn(executable, "packaged-crate-consumer", "a0").await.unwrap(); '
        "let empty_result = client.analytic_planar_boolean_batch(&empty).await.unwrap(); "
        "assert!(empty_result.job_results.is_empty()); "
        "let result = client.analytic_planar_boolean_batch(&request).await.unwrap(); "
        "let AnalyticPlanarBooleanJobResult::Success(job) = &result.job_results[0] else { "
        'panic!("packaged client returned a job-local failure"); }; '
        "assert!(!job.result_regions.is_empty()); assert_eq!(job.digest_sha256.len(), 64); "
        "client.close().await.unwrap(); }\n",
        encoding="utf-8",
    )
    run("cargo", "generate-lockfile", cwd=consumer)
    run("cargo", "run", "--locked", "--", str(_native_executable()), cwd=consumer)
    # Compile the complete public STEP/HLR/illustration example against the
    # extracted crate, not a workspace path dependency or handwritten adapter.
    binary_dir = consumer / "src" / "bin"
    binary_dir.mkdir()
    (binary_dir / "mesh_illustration.rs").write_text(
        (crate / "examples" / "mesh_illustration.rs").read_text(encoding="utf-8"),
        encoding="utf-8",
    )
    svg = tmp_path / "packaged-illustration.svg"
    run(
        "cargo",
        "run",
        "--locked",
        "--bin",
        "mesh_illustration",
        "--",
        str(_native_executable()),
        str(ROOT / "tests/fixtures/step/embedded_models/SOT-23.STEP"),
        str(svg),
        cwd=consumer,
    )
    import xml.etree.ElementTree as ET

    assert ET.parse(svg).getroot().tag == "{http://www.w3.org/2000/svg}svg"
    assert svg.stat().st_size > 1000
    # Compile and run caller-supervised process adoption from the extracted
    # package so the published surface cannot accidentally depend on workspace
    # visibility or an unpackaged source file.
    (binary_dir / "supervised_process.rs").write_text(
        (crate / "examples" / "supervised_process.rs").read_text(encoding="utf-8"),
        encoding="utf-8",
    )
    run(
        "cargo",
        "run",
        "--locked",
        "--bin",
        "supervised_process",
        "--",
        str(_native_executable()),
        cwd=consumer,
    )


def _native_executable() -> Path:
    if override := os.environ.get("GEOMETER_EXECUTABLE"):
        executable = Path(override).resolve()
        assert executable.is_file(), f"missing packaged-consumer executable: {executable}"
        return executable
    if sys.platform == "win32":
        platform_name, executable_name = "windows-x64", "geometer.exe"
    elif sys.platform == "darwin":
        platform_name, executable_name = "macos-arm64", "geometer"
    elif sys.platform.startswith("linux"):
        machine = platform.machine().casefold()
        platform_name = "linux-arm64" if machine in {"aarch64", "arm64"} else "linux-x64"
        executable_name = "geometer"
    else:
        raise AssertionError(f"unsupported Rust consumer platform: {sys.platform}")
    executable = ROOT / "dist" / "native" / platform_name / executable_name
    assert executable.is_file(), f"missing packaged-consumer executable: {executable}"
    return executable


def _toml(document: dict[str, object]) -> str:
    package = document["package"]
    dependency = document["dependencies"]
    assert isinstance(package, dict) and isinstance(dependency, dict)
    lines = ["[package]"]
    lines.extend(f"{key} = {json.dumps(value)}" for key, value in package.items())
    lines.extend(["", "[dependencies]"])
    for name, value in dependency.items():
        assert isinstance(value, dict)
        fields = ", ".join(f"{key} = {json.dumps(item)}" for key, item in value.items())
        lines.append(f"{name} = {{ {fields} }}")
    return "\n".join(lines) + "\n"
