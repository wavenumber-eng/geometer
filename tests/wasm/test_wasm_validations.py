from __future__ import annotations

import json
import shutil
import subprocess
import tomllib
from pathlib import Path

import pytest


ROOT = Path(__file__).resolve().parents[2]
NODE = shutil.which("node")


def test_node_cli_distribution_runs_under_root_esm_package() -> None:
    assert NODE is not None, "Node 24 is required for WASM validation."
    node_dist = ROOT / "dist" / "wasm" / "node-test"
    package = json.loads((node_dist / "package.json").read_text(encoding="utf-8"))
    assert package == {"private": True, "type": "commonjs"}
    completed = subprocess.run(
        [NODE, str(node_dist / "geometer-node-test.js"), "--version"],
        cwd=ROOT,
        check=False,
        capture_output=True,
        text=True,
        timeout=60,
    )
    assert completed.returncode == 0, completed.stdout + completed.stderr
    with (ROOT / "pyproject.toml").open("rb") as handle:
        expected_version = tomllib.load(handle)["project"]["version"]
    assert completed.stdout.strip().startswith(f"geometer {expected_version} (abi ")


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
