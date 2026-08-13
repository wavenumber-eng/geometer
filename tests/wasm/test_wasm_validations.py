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
        "operation_contract_validation.js",
        "step_to_glb_bytes_validation.js",
        "planar_batch_solve_bytes_validation.js",
    ],
)
def test_browser_wasm_contract_and_compatibility_smoke(script: str) -> None:
    assert NODE is not None, "Node 24 is required for WASM validation."
    completed = subprocess.run(
        [NODE, str(ROOT / "tests" / "wasm" / script)],
        cwd=ROOT,
        check=False,
        capture_output=True,
        text=True,
        timeout=60,
    )
    assert completed.returncode == 0, completed.stdout + completed.stderr
