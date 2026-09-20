from __future__ import annotations

import json
import os
import platform
import shutil
import subprocess
import sys
from pathlib import Path

import pytest


ROOT = Path(__file__).resolve().parents[2]
NODE = shutil.which("node")
REQUIRE_NATIVE_TEST_SERVERS = os.environ.get("GEOMETER_REQUIRE_NATIVE_TEST_SERVERS") == "1"
NATIVE_PROCESS_SCRIPTS = frozenset({"node_process_a0_validation.mjs", "native_illustration_parity.mjs"})
TEST_PROFILE = os.environ.get("GEOMETER_TEST_PROFILE", "all")
TYPESCRIPT_SCOPE = os.environ.get("GEOMETER_TYPESCRIPT_SCOPE", "all")
EXPERIMENTAL_SCRIPTS = frozenset(
    {
        "analytic_arc_render_validation.mjs",
        "analytic_cpp_vector_validation.mjs",
        "analytic_packet_codec_validation.mjs",
        "analytic_self_contained_demo_validation.mjs",
        "analytic_static_site_validation.mjs",
    }
)
VALIDATION_SCRIPTS = (
    "analytic_arc_render_validation.mjs",
    "analytic_self_contained_demo_validation.mjs",
    "analytic_static_site_validation.mjs",
    "analytic_packet_codec_validation.mjs",
    "analytic_cpp_vector_validation.mjs",
    "contract_codec_validation.mjs",
    "demo_tooling_validation.mjs",
    "documentation_reference_validation.mjs",
    "emitter_validation.mjs",
    "hlr_static_site_validation.mjs",
    "illustration_static_site_validation.mjs",
    "ipc_a0_validation.mjs",
    "ipc_client_a0_validation.mjs",
    "node_process_a0_validation.mjs",
    "package_consumer_validation.mjs",
    "mesh_illustration_validation.mjs",
    "native_illustration_parity.mjs",
    "pcb_polygon_pour_artifact_validation.mjs",
    "pcb_polygon_pour_model_validation.mjs",
    "shared_demo_theme_validation.mjs",
    "wasm_client_validation.mjs",
    "worker_client_validation.mjs",
    "worker_protocol_validation.mjs",
)
HOST_SCRIPTS = frozenset(
    {
        "documentation_reference_validation.mjs",
        "emitter_validation.mjs",
        "native_illustration_parity.mjs",
        "node_process_a0_validation.mjs",
        "shared_demo_theme_validation.mjs",
    }
)
WASM_SCRIPTS = frozenset(VALIDATION_SCRIPTS) - HOST_SCRIPTS


def _native_platform_directory(system: str, machine: str) -> str | None:
    architecture = machine.strip().casefold()
    if system == "win32" and architecture in {"amd64", "x86_64"}:
        return "windows-x64"
    if system == "darwin" and architecture in {"aarch64", "arm64"}:
        return "macos-arm64"
    if system.startswith("linux"):
        if architecture in {"aarch64", "arm64"}:
            return "linux-arm64"
        if architecture in {"amd64", "x86_64"}:
            return "linux-x64"
    return None


@pytest.mark.parametrize(
    "script",
    VALIDATION_SCRIPTS,
)
def test_generated_typescript_package(script: str) -> None:
    assert NODE is not None, "Node 24 is required for TypeScript validation."
    assert TYPESCRIPT_SCOPE in {"all", "host", "wasm"}, "GEOMETER_TYPESCRIPT_SCOPE must be all, host, or wasm."
    if TYPESCRIPT_SCOPE == "host" and script not in HOST_SCRIPTS:
        pytest.skip("WASM-package validation runs against the candidate WASM build")
    if TYPESCRIPT_SCOPE == "wasm" and script not in WASM_SCRIPTS:
        pytest.skip("host and native-process validation runs in the Linux client lane")
    if TEST_PROFILE == "production" and script in EXPERIMENTAL_SCRIPTS:
        pytest.skip("experimental analytic validation is opt-in")
    if script in NATIVE_PROCESS_SCRIPTS and not REQUIRE_NATIVE_TEST_SERVERS:
        pytest.skip("native-process validation runs after the current platform executable is built")
    completed = subprocess.run(
        [NODE, str(ROOT / "tests" / "typescript" / script)],
        cwd=ROOT,
        check=False,
        capture_output=True,
        text=True,
        timeout=90,
    )
    assert completed.returncode == 0, completed.stdout + completed.stderr


def test_step_topology_annotation_reference_restarts_native_process() -> None:
    assert NODE is not None, "Node 24 is required for the native reference example."
    if TYPESCRIPT_SCOPE == "wasm":
        pytest.skip("native-process validation runs in the Linux client lane")
    if TEST_PROFILE == "production":
        pytest.skip("experimental STEP-topology validation is opt-in")
    if not REQUIRE_NATIVE_TEST_SERVERS:
        pytest.skip("native reference validation runs after the current platform executable is built")
    platform_directory = _native_platform_directory(sys.platform, platform.machine())
    assert platform_directory is not None, f"No native artifact mapping for {sys.platform}/{platform.machine()}."
    executable_name = "geometer.exe" if sys.platform == "win32" else "geometer"
    completed = subprocess.run(
        [
            NODE,
            str(ROOT / "dist" / "native" / "examples" / "step-topology-annotation-reference.mjs"),
            str(ROOT / "dist" / "native" / platform_directory / executable_name),
            str(ROOT / "tests" / "fixtures" / "step" / "embedded_models" / "SOT-23.STEP"),
        ],
        cwd=ROOT,
        check=False,
        capture_output=True,
        text=True,
        timeout=90,
    )
    assert completed.returncode == 0, completed.stdout + completed.stderr
    report = json.loads(completed.stdout)
    assert report["schema"] == "wn.geometer.step_topology.annotation_reference_report.a0"
    assert report["transaction_count"] == 2
    assert report["exact_preconditions_replayed"] is True
    assert report["logical_group_replayed"] is True
    assert report["metadata_probe_replayed"] is True
    assert report["session_identity_reminted"] is True
    assert report["topology_handle_reminted"] is True


@pytest.mark.parametrize(
    ("system", "machine", "expected"),
    [
        ("win32", "AMD64", "windows-x64"),
        ("darwin", "arm64", "macos-arm64"),
        ("linux", "x86_64", "linux-x64"),
        ("linux", "aarch64", "linux-arm64"),
        ("linux", "arm64", "linux-arm64"),
        ("linux", "riscv64", None),
    ],
)
def test_native_platform_directory_mapping(system: str, machine: str, expected: str | None) -> None:
    assert _native_platform_directory(system, machine) == expected


def test_typescript_scope_assigns_every_script_once() -> None:
    assert HOST_SCRIPTS.isdisjoint(WASM_SCRIPTS)
    assert HOST_SCRIPTS | WASM_SCRIPTS == frozenset(VALIDATION_SCRIPTS)
