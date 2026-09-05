"""Opt-in checks for the independent desktop demonstration, not client dependencies."""
from __future__ import annotations

import os
from pathlib import Path
import subprocess

import pytest

ROOT = Path(__file__).resolve().parents[2]
MANIFEST = ROOT / "examples/rust/native_viewer/Cargo.toml"
pytestmark = pytest.mark.skipif(
    os.environ.get("GEOMETER_TEST_NATIVE_VIEWER") != "1",
    reason="optional GUI crate; set GEOMETER_TEST_NATIVE_VIEWER=1",
)


def test_viewer_format_lint_and_unit_tests() -> None:
    for arguments in (
        ["fmt", "--", "--check"],
        ["clippy", "--all-targets", "--locked", "--", "-D", "warnings"],
        ["test", "--locked"],
    ):
        subprocess.run(
            ["cargo", arguments[0], "--manifest-path", str(MANIFEST), *arguments[1:]],
            cwd=ROOT, check=True, timeout=600,
        )


@pytest.mark.skipif(
    os.environ.get("GEOMETER_TEST_NATIVE_VIEWER_GPU") != "1",
    reason="needs an interactive GPU session and compatible GEOMETER_EXECUTABLE",
)
def test_real_window_native_outputs_and_failed_load_exit(tmp_path: Path) -> None:
    executable = os.environ["GEOMETER_EXECUTABLE"]
    subprocess.run(
        ["cargo", "test", "--manifest-path", str(MANIFEST), "--locked", "--", "--ignored"],
        cwd=ROOT, check=True, timeout=120,
    )
    subprocess.run(
        ["cargo", "build", "--manifest-path", str(MANIFEST), "--locked"],
        cwd=ROOT, check=True, timeout=600,
    )
    name = "geometer-native-viewer.exe" if os.name == "nt" else "geometer-native-viewer"
    viewer = MANIFEST.parent / "target/debug" / name
    common = [str(viewer), "--geometer", executable, "--step"]
    good = subprocess.run(
        [*common, str(ROOT / "tests/fixtures/step/embedded_models/SOT-23.STEP"),
         "--smoke-screenshot", str(tmp_path / "viewer.png")],
        cwd=ROOT, check=True, capture_output=True, text=True, timeout=60,
    )
    assert "GPU_SMOKE_OK" in good.stdout
    assert (tmp_path / "viewer.png").stat().st_size > 1000
    bad = subprocess.run(
        [*common, str(tmp_path / "missing.step"), "--smoke"],
        cwd=ROOT, check=False, capture_output=True, text=True, timeout=60,
    )
    assert bad.returncode != 0
    assert "GPU_SMOKE_FAILED" in bad.stderr
