from __future__ import annotations

import shutil
import subprocess
from pathlib import Path

import pytest


ROOT = Path(__file__).resolve().parents[2]
NODE = shutil.which("node")


@pytest.mark.parametrize(
    "script",
    [
        "analytic_arc_render_validation.mjs",
        "analytic_static_site_validation.mjs",
        "analytic_packet_codec_validation.mjs",
        "analytic_cpp_vector_validation.mjs",
        "contract_codec_validation.mjs",
        "package_consumer_validation.mjs",
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
