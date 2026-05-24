from __future__ import annotations

import platform
import sys
from pathlib import Path

from setuptools import setup
from setuptools.command.build_py import build_py as _build_py
from wheel.bdist_wheel import bdist_wheel as _bdist_wheel


ROOT = Path(__file__).resolve().parent


class build_py(_build_py):
    def run(self) -> None:
        super().run()
        executable = _source_executable()
        native_root = Path(self.build_lib) / "geometer" / "native"
        for name in ("geometer.exe", "geometer"):
            stale = native_root / name
            if stale.exists():
                stale.unlink()
        target_dir = native_root / _platform_tag()
        target_dir.mkdir(parents=True, exist_ok=True)
        self.copy_file(str(executable), str(target_dir / executable.name))


class bdist_wheel(_bdist_wheel):
    def finalize_options(self) -> None:
        super().finalize_options()
        self.root_is_pure = False


def _source_executable() -> Path:
    name = "geometer.exe" if sys.platform == "win32" else "geometer"
    for path in (ROOT / "dist" / "native" / _platform_tag() / name, ROOT / "dist" / name):
        if path.exists():
            return path
    raise FileNotFoundError(
        "Missing geometer executable. Build the native CLI before building the "
        f"Python wheel. Looked under dist/native/{_platform_tag()}/ and dist/."
    )


def _platform_tag() -> str:
    if sys.platform == "win32":
        os_name = "windows"
    elif sys.platform == "darwin":
        os_name = "macos"
    elif sys.platform.startswith("linux"):
        os_name = "linux"
    else:
        os_name = sys.platform.replace("_", "-").replace(".", "-")

    machine = platform.machine().strip().lower()
    if machine in {"amd64", "x86_64"}:
        arch = "x64"
    elif machine in {"aarch64", "arm64"}:
        arch = "arm64"
    elif machine in {"i386", "i686", "x86"}:
        arch = "x86"
    else:
        arch = machine or "unknown"
    return f"{os_name}-{arch}"


setup(cmdclass={"build_py": build_py, "bdist_wheel": bdist_wheel})
