from __future__ import annotations

import platform
import shutil
import subprocess
import sys
from pathlib import Path

import pytest


ROOT = Path(__file__).resolve().parents[2]


def _configured_for_root(candidate: Path, root: Path) -> bool:
    cache = candidate / "CMakeCache.txt"
    if not (candidate / "CTestTestfile.cmake").is_file() or not cache.is_file():
        return False
    prefix = "CMAKE_HOME_DIRECTORY:INTERNAL="
    for line in cache.read_text(encoding="utf-8", errors="replace").splitlines():
        if line.startswith(prefix):
            return Path(line.removeprefix(prefix)).resolve() == root.resolve()
    return False


def _architecture_fragment(machine: str) -> str:
    normalized = machine.casefold()
    if normalized in {"amd64", "x86_64"}:
        return "x64"
    if normalized in {"aarch64", "arm64"}:
        return "arm64"
    return normalized.replace("_", "-")


def _native_build_tree(
    root: Path = ROOT,
    platform_name: str = sys.platform,
    machine: str = platform.machine(),
) -> Path:
    default = root / "build"
    if _configured_for_root(default, root):
        return default
    platform_fragment = {
        "win32": "windows",
        "linux": "linux",
        "darwin": "macos",
    }.get(platform_name)
    assert platform_fragment is not None, f"unsupported native Rack platform: {platform_name}"
    fallback = root / f"build-native-{platform_fragment}-{_architecture_fragment(machine)}"
    if _configured_for_root(fallback, root):
        return fallback
    raise AssertionError(
        "run `cmake --preset default && cmake --build build --config Release` "
        "or scripts/validate_native.py before the Rack C++ foundation stratum"
    )


def _write_configured_tree(candidate: Path, root: Path) -> None:
    candidate.mkdir(parents=True)
    (candidate / "CTestTestfile.cmake").write_text("# governed fixture\n", encoding="utf-8")
    (candidate / "CMakeCache.txt").write_text(
        f"CMAKE_HOME_DIRECTORY:INTERNAL={root.as_posix()}\n", encoding="utf-8"
    )


def test_default_build_tree_precedes_architecture_fallback(tmp_path: Path) -> None:
    _write_configured_tree(tmp_path / "build", tmp_path)
    _write_configured_tree(tmp_path / "build-native-windows-x64", tmp_path)
    assert _native_build_tree(tmp_path, "win32", "AMD64") == tmp_path / "build"


def test_fallback_is_host_platform_and_architecture_specific(tmp_path: Path) -> None:
    _write_configured_tree(tmp_path / "build-native-linux-x64", tmp_path)
    _write_configured_tree(tmp_path / "build-native-windows-arm64", tmp_path)
    expected = tmp_path / "build-native-windows-x64"
    _write_configured_tree(expected, tmp_path)
    assert _native_build_tree(tmp_path, "win32", "x86_64") == expected


def test_stale_fallback_configured_for_another_root_is_rejected(tmp_path: Path) -> None:
    fallback = tmp_path / "build-native-windows-x64"
    _write_configured_tree(fallback, tmp_path / "other-root")
    with pytest.raises(AssertionError, match="cmake --preset default"):
        _native_build_tree(tmp_path, "win32", "AMD64")


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
