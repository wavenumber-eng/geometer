from __future__ import annotations

import json
import shutil
import subprocess
import sys
from pathlib import Path

import pytest


ROOT = Path(__file__).resolve().parents[2]
NODE = shutil.which("node")


@pytest.mark.parametrize(
    "script",
    [
        "analytic_arc_render_validation.mjs",
        "analytic_self_contained_demo_validation.mjs",
        "analytic_static_site_validation.mjs",
        "analytic_packet_codec_validation.mjs",
        "analytic_cpp_vector_validation.mjs",
        "contract_codec_validation.mjs",
        "demo_tooling_validation.mjs",
        "emitter_validation.mjs",
        "fast_hlr_validation.mjs",
        "hlr_static_site_validation.mjs",
        "illustration_static_site_validation.mjs",
        "ipc_a0_validation.mjs",
        "ipc_client_a0_validation.mjs",
        "node_process_a0_validation.mjs",
        "package_consumer_validation.mjs",
        "mesh_illustration_validation.mjs",
        "pcb_polygon_pour_artifact_validation.mjs",
        "pcb_polygon_pour_model_validation.mjs",
        "shared_demo_theme_validation.mjs",
        "wasm_client_validation.mjs",
        "worker_client_validation.mjs",
        "worker_protocol_validation.mjs",
    ],
)
def test_generated_typescript_package(script: str) -> None:
    assert NODE is not None, "Node 24 is required for TypeScript validation."
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
    platform_directory = {
        "darwin": "macos-arm64",
        "linux": "linux-x64",
        "win32": "windows-x64",
    }.get(sys.platform)
    assert platform_directory is not None, f"No native artifact mapping for {sys.platform}."
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
