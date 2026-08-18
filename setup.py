from __future__ import annotations

import platform
import os
import re
import shutil
import subprocess
import sys
from pathlib import Path

from setuptools import setup
from setuptools.command.build_py import build_py as _build_py
from setuptools.dist import Distribution

try:
    from setuptools.command.bdist_wheel import bdist_wheel as _bdist_wheel
except ImportError:
    from wheel.bdist_wheel import bdist_wheel as _bdist_wheel  # type: ignore[import-not-found]


ROOT = Path(__file__).resolve().parent
DEFAULT_MACOS_DEPLOYMENT_TARGET = "11.0"


class build_py(_build_py):
    def run(self) -> None:
        package_build_dir = Path(self.build_lib) / "geometer"
        if package_build_dir.exists():
            shutil.rmtree(package_build_dir)
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
        sidecar = _source_attestation(executable)
        self.copy_file(str(sidecar), str(target_dir / sidecar.name))
        license_dir = package_build_dir / "licenses"
        license_dir.mkdir(parents=True, exist_ok=True)
        for name, source in _license_sources().items():
            if not source.is_file():
                raise FileNotFoundError(f"Missing release license source: {source}")
            self.copy_file(str(source), str(license_dir / name))


class bdist_wheel(_bdist_wheel):  # type: ignore[reportGeneralTypeIssues]
    def finalize_options(self) -> None:
        super().finalize_options()
        self.root_is_pure = False

    def get_tag(self) -> tuple[str, str, str]:
        _, _, platform_tag = super().get_tag()
        if sys.platform == "darwin":
            platform_tag = _macos_wheel_platform_tag()
        elif sys.platform.startswith("linux"):
            platform_tag = _linux_wheel_platform_tag()
        return "py3", "none", platform_tag


class BinaryDistribution(Distribution):
    def has_ext_modules(self) -> bool:
        return True


def _source_executable() -> Path:
    name = "geometer.exe" if sys.platform == "win32" else "geometer"
    path = ROOT / "dist" / "native" / _platform_tag() / name
    if path.exists():
        if sys.platform == "darwin":
            _validate_macos_executable_target(path, _macos_deployment_target())
        return path
    raise FileNotFoundError(
        "Missing geometer executable. Build the native CLI before building the "
        f"Python wheel. Looked under dist/native/{_platform_tag()}/."
    )


def _source_attestation(executable: Path) -> Path:
    path = executable.with_name("geometer.build-attestation.json")
    scripts_path = ROOT / "scripts"
    if str(scripts_path) not in sys.path:
        sys.path.insert(0, str(scripts_path))
    from native_build_attestation import (  # type: ignore[import-not-found]
        BuildAttestationError,
        load_and_validate_attestation,
    )

    try:
        value = load_and_validate_attestation(executable, path)
    except BuildAttestationError as error:
        raise RuntimeError(f"Native executable build attestation is invalid: {error}") from error
    if value is None:
        raise FileNotFoundError(
            "Missing geometer.build-attestation.json beside the native executable. "
            "Rebuild the native distribution target before building the Python wheel."
        )
    return path


def _license_sources() -> dict[str, Path]:
    return {
        "WN_GEOMETER_LICENSE.txt": ROOT / "LICENSE",
        "THIRD_PARTY_NOTICES.md": ROOT / "THIRD_PARTY_NOTICES.md",
        "CLIPPER2_LICENSE.txt": ROOT / "third_party" / "clipper2" / "LICENSE",
        "RAPIDJSON_LICENSE.txt": ROOT / "third_party" / "rapidjson" / "license.txt",
        "OCCT_LICENSE_LGPL_21.txt": ROOT / ".deps" / "occt-src" / "LICENSE_LGPL_21.txt",
        "OCCT_LGPL_EXCEPTION.txt": ROOT / ".deps" / "occt-src" / "OCCT_LGPL_EXCEPTION.txt",
    }


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


def _macos_deployment_target() -> str:
    return (
        os.environ.get("GEOMETER_MACOS_DEPLOYMENT_TARGET")
        or os.environ.get("MACOSX_DEPLOYMENT_TARGET")
        or DEFAULT_MACOS_DEPLOYMENT_TARGET
    ).replace("_", ".")


def _macos_wheel_platform_tag() -> str:
    major, minor = _version_pair(_macos_deployment_target())
    if _wheel_arch() == "arm64" and (major, minor) < (11, 0):
        raise RuntimeError("macOS arm64 wheels require deployment target 11.0 or newer.")
    return f"macosx_{major}_{0 if major >= 11 else minor}_{_wheel_arch()}"


def _linux_wheel_platform_tag() -> str:
    libc_name, libc_version = platform.libc_ver()
    if libc_name != "glibc" or not libc_version:
        raise RuntimeError(f"Linux wheels require glibc for PyPI publishing, got {libc_name or 'unknown'} {libc_version}")
    major, minor = _version_pair(libc_version)
    return f"manylinux_{major}_{minor}_{_linux_wheel_arch()}"


def _linux_wheel_arch() -> str:
    machine = platform.machine().strip().lower()
    if machine in {"amd64", "x86_64"}:
        return "x86_64"
    if machine in {"aarch64", "arm64"}:
        return "aarch64"
    if machine in {"i386", "i686", "x86"}:
        return "i686"
    return machine or "unknown"


def _wheel_arch() -> str:
    machine = platform.machine().strip().lower()
    if machine in {"aarch64", "arm64"}:
        return "arm64"
    if machine in {"amd64", "x86_64"}:
        return "x86_64"
    return machine or "unknown"


def _validate_macos_executable_target(executable: Path, target: str) -> None:
    min_version = _macos_binary_min_version(executable)
    if min_version is None:
        raise RuntimeError(f"Could not determine macOS minimum OS version for {executable}")
    if _version_pair(min_version) > _version_pair(target):
        raise RuntimeError(
            f"{executable} requires macOS {min_version}, which is newer than "
            f"the wheel deployment target {target}. Rebuild native artifacts "
            "with MACOSX_DEPLOYMENT_TARGET set before building the wheel."
        )


def _macos_binary_min_version(executable: Path) -> str | None:
    completed = subprocess.run(
        ["otool", "-l", str(executable)],
        cwd=ROOT,
        capture_output=True,
        text=True,
        check=True,
    )
    for key in ("minos", "version"):
        match = re.search(rf"^\s*{key}\s+(\d+(?:\.\d+)*)\b", completed.stdout, re.MULTILINE)
        if match is not None:
            return match.group(1)
    return None


def _version_pair(value: str) -> tuple[int, int]:
    parts = value.replace("_", ".").split(".")
    if len(parts) == 1:
        parts.append("0")
    return int(parts[0]), int(parts[1])


setup(cmdclass={"build_py": build_py, "bdist_wheel": bdist_wheel}, distclass=BinaryDistribution)
