from __future__ import annotations

import shutil
import subprocess
import sys
from pathlib import Path


ROOT = Path(__file__).resolve().parents[2]


def _native_build_tree() -> Path:
    platform_fragment = {
        "win32": "windows",
        "linux": "linux",
        "darwin": "macos",
    }.get(sys.platform)
    assert platform_fragment is not None, f"unsupported native Rack platform: {sys.platform}"
    for candidate in sorted(ROOT.glob(f"build-native-{platform_fragment}-*")):
        if (candidate / "CTestTestfile.cmake").is_file():
            return candidate
    raise AssertionError("run scripts/validate_native.py before the Rack C++ foundation stratum")


def test_registered_native_ctest_suite() -> None:
    ctest = shutil.which("ctest")
    assert ctest is not None, "CMake/CTest is required for native validation"
    completed = subprocess.run(
        [
            ctest,
            "--test-dir",
            str(_native_build_tree()),
            "-C",
            "Release",
            "--output-on-failure",
        ],
        cwd=ROOT,
        check=False,
        capture_output=True,
        text=True,
        timeout=180,
    )
    assert completed.returncode == 0, completed.stdout + completed.stderr
