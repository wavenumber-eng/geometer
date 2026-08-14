from __future__ import annotations

import platform
import re
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


def _declared_native_tests(root: Path) -> set[str]:
    pattern = re.compile(r"add_test\s*\(\s*NAME\s+([A-Za-z0-9_.-]+)", re.MULTILINE)
    names: set[str] = set()
    for source_root in (root / "src", root / "tests", root / "examples"):
        if source_root.is_dir():
            for cmake_list in source_root.rglob("CMakeLists.txt"):
                names.update(pattern.findall(cmake_list.read_text(encoding="utf-8")))
    return names


def _generated_native_tests(candidate: Path) -> set[str]:
    pattern = re.compile(r"add_test\(\[=\[([^]]+)\]=\]")
    names: set[str] = set()
    for test_file in candidate.rglob("CTestTestfile.cmake"):
        names.update(pattern.findall(test_file.read_text(encoding="utf-8", errors="replace")))
    return names


def _has_current_test_inventory(candidate: Path, root: Path) -> bool:
    declared = _declared_native_tests(root)
    return bool(declared) and declared <= _generated_native_tests(candidate)


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
    if _configured_for_root(default, root) and _has_current_test_inventory(default, root):
        return default
    platform_fragment = {
        "win32": "windows",
        "linux": "linux",
        "darwin": "macos",
    }.get(platform_name)
    assert platform_fragment is not None, f"unsupported native Rack platform: {platform_name}"
    fallback = root / f"build-native-{platform_fragment}-{_architecture_fragment(machine)}"
    if _configured_for_root(fallback, root) and _has_current_test_inventory(fallback, root):
        return fallback
    raise AssertionError(
        "run `cmake --preset default && cmake --build build --config Release` "
        "or scripts/validate_native.py before the Rack C++ foundation stratum"
    )


def _write_source_inventory(root: Path, names: tuple[str, ...]) -> None:
    source = root / "tests" / "cpp"
    source.mkdir(parents=True, exist_ok=True)
    (source / "CMakeLists.txt").write_text(
        "".join(f"add_test(NAME {name} COMMAND {name})\n" for name in names),
        encoding="utf-8",
    )


def _write_configured_tree(candidate: Path, root: Path, names: tuple[str, ...]) -> None:
    candidate.mkdir(parents=True)
    (candidate / "CTestTestfile.cmake").write_text("# governed fixture\n", encoding="utf-8")
    (candidate / "CMakeCache.txt").write_text(
        f"CMAKE_HOME_DIRECTORY:INTERNAL={root.as_posix()}\n", encoding="utf-8"
    )
    generated = candidate / "tests" / "cpp"
    generated.mkdir(parents=True)
    (generated / "CTestTestfile.cmake").write_text(
        "".join(f"add_test([=[{name}]=] {name})\n" for name in names),
        encoding="utf-8",
    )


def test_default_build_tree_precedes_architecture_fallback(tmp_path: Path) -> None:
    names = ("geometer_alpha_test", "geometer_beta_test")
    _write_source_inventory(tmp_path, names)
    _write_configured_tree(tmp_path / "build", tmp_path, names)
    _write_configured_tree(tmp_path / "build-native-windows-x64", tmp_path, names)
    assert _native_build_tree(tmp_path, "win32", "AMD64") == tmp_path / "build"


def test_fallback_is_host_platform_and_architecture_specific(tmp_path: Path) -> None:
    names = ("geometer_alpha_test",)
    _write_source_inventory(tmp_path, names)
    _write_configured_tree(tmp_path / "build-native-linux-x64", tmp_path, names)
    _write_configured_tree(tmp_path / "build-native-windows-arm64", tmp_path, names)
    expected = tmp_path / "build-native-windows-x64"
    _write_configured_tree(expected, tmp_path, names)
    assert _native_build_tree(tmp_path, "win32", "x86_64") == expected


def test_stale_fallback_configured_for_another_root_is_rejected(tmp_path: Path) -> None:
    names = ("geometer_alpha_test",)
    _write_source_inventory(tmp_path, names)
    fallback = tmp_path / "build-native-windows-x64"
    _write_configured_tree(fallback, tmp_path / "other-root", names)
    with pytest.raises(AssertionError, match="cmake --preset default"):
        _native_build_tree(tmp_path, "win32", "AMD64")


def test_stale_default_inventory_uses_current_host_fallback(tmp_path: Path) -> None:
    current = ("geometer_alpha_test", "geometer_new_test")
    _write_source_inventory(tmp_path, current)
    _write_configured_tree(tmp_path / "build", tmp_path, ("geometer_alpha_test",))
    fallback = tmp_path / "build-native-windows-x64"
    _write_configured_tree(fallback, tmp_path, current)
    assert _native_build_tree(tmp_path, "win32", "AMD64") == fallback


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
