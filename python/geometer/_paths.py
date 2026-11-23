from __future__ import annotations

import os
import platform
import shutil
import sys
import zipfile
from pathlib import Path


def executable_path() -> Path:
    path = _find_executable_path()
    if path is None:
        raise FileNotFoundError(
            "Could not find the Geometer executable. Build the geometer CLI or set GEOMETER_EXE to the executable path."
        )
    return path


def _find_executable_path() -> Path | None:
    override = os.environ.get("GEOMETER_EXE")
    if override:
        path = Path(override)
        if not path.is_file():
            raise FileNotFoundError(f"GEOMETER_EXE does not exist or is not a file: {path}")
        if _is_console_wrapper(path):
            raise FileNotFoundError(
                f"GEOMETER_EXE points to the Python console launcher, not the native executable: {path}"
            )
        return path

    for directory in _candidate_directories():
        for name in _executable_names():
            path = directory / name
            if _is_usable_candidate(path):
                return path

    for name in _executable_names():
        found = shutil.which(name)
        if found and _is_usable_candidate(Path(found)):
            return Path(found)
    return None


def _is_usable_candidate(path: Path) -> bool:
    return path.is_file() and not _is_console_wrapper(path)


def _is_console_wrapper(path: Path) -> bool:
    script = _console_script(path)
    return script is not None and b"from geometer._console import main" in script


def _console_script(path: Path) -> bytes | None:
    # uv/distlib Windows launchers embed a ZIP __main__.py; POSIX entry points
    # use a shebang script. Inspect the payload without ever executing it.
    with path.open("rb") as stream:
        if stream.read(2) == b"#!":
            stream.seek(0)
            return stream.read(16_384)
        try:
            with zipfile.ZipFile(stream) as archive:
                entry = archive.getinfo("__main__.py")
                if entry.file_size > 16_384 or entry.flag_bits & 1:
                    return None
                return archive.read(entry)
        except (zipfile.BadZipFile, KeyError):
            return None


def _candidate_directories() -> list[Path]:
    package_dir = Path(__file__).resolve().parent
    repo_root = package_dir.parents[1]
    directories = [
        package_dir,
        package_dir / "native" / _platform_tag(),
        package_dir / "native",
        repo_root / "dist" / "native" / _platform_tag(),
    ]

    extra = os.environ.get("GEOMETER_EXE_DIR")
    if extra:
        directories.insert(0, Path(extra))
    return directories


def _executable_names() -> list[str]:
    if sys.platform == "win32":
        return ["geometer.exe"]
    return ["geometer"]


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
