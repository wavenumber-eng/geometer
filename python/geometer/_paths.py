from __future__ import annotations

import ctypes
import ctypes.util
import os
import shutil
import sys
from pathlib import Path


_DLL_DIRECTORY_HANDLES: list[object] = []


def native_library_path() -> Path:
    path = _find_native_library_path()
    if path is None:
        raise FileNotFoundError(
            "Could not find the Geometer native library. Set GEOMETER_NATIVE_LIBRARY "
            "to an explicit shared library path."
        )
    return path


def executable_path() -> Path:
    path = _find_executable_path()
    if path is None:
        raise FileNotFoundError(
            "Could not find the Geometer executable. Build the geometer CLI "
            "or set GEOMETER_EXE to the executable path."
        )
    return path


def load_native_library() -> tuple[ctypes.CDLL, Path | None]:
    path = _find_native_library_path()
    if path is not None:
        _add_dll_directory(path.parent)
        return ctypes.CDLL(str(path)), path

    found = ctypes.util.find_library("geometer")
    if found:
        return ctypes.CDLL(found), None

    raise FileNotFoundError(
        "Could not find the Geometer native library. Set GEOMETER_NATIVE_LIBRARY "
        "to an explicit shared library path."
    )


def bundled_occt_runtime_available() -> bool:
    path = _find_native_library_path()
    if path is None:
        return False
    directory = path.parent
    return all((directory / name).exists() for name in _required_occt_runtime_names())


def _find_native_library_path() -> Path | None:
    override = os.environ.get("GEOMETER_NATIVE_LIBRARY")
    if override:
        path = Path(override)
        if path.exists():
            return path
        raise FileNotFoundError(f"GEOMETER_NATIVE_LIBRARY does not exist: {path}")

    for directory in _native_library_candidate_directories():
        for name in _library_names():
            path = directory / name
            if path.exists():
                return path
    return None


def _find_executable_path() -> Path | None:
    override = os.environ.get("GEOMETER_EXE")
    if override:
        path = Path(override)
        if path.exists():
            return path
        raise FileNotFoundError(f"GEOMETER_EXE does not exist: {path}")

    for directory in _candidate_directories():
        for name in _executable_names():
            path = directory / name
            if path.exists():
                return path

    for name in _executable_names():
        found = shutil.which(name)
        if found:
            return Path(found)
    return None


def _native_library_candidate_directories() -> list[Path]:
    package_dir = Path(__file__).resolve().parent
    directories = [
        package_dir,
        package_dir / "native",
    ]

    extra = os.environ.get("GEOMETER_NATIVE_LIBRARY_DIR")
    if extra:
        directories.insert(0, Path(extra))
    return directories


def _candidate_directories() -> list[Path]:
    package_dir = Path(__file__).resolve().parent
    repo_root = package_dir.parents[1]
    directories = [
        package_dir,
        package_dir / "native",
        repo_root / "dist",
    ]

    extra = os.environ.get("GEOMETER_NATIVE_LIBRARY_DIR")
    if extra:
        directories.insert(0, Path(extra))
    return directories


def _executable_names() -> list[str]:
    if sys.platform == "win32":
        return ["geometer.exe"]
    return ["geometer"]


def _library_names() -> list[str]:
    if sys.platform == "win32":
        return ["geometer.dll"]
    if sys.platform == "darwin":
        return ["libgeometer.dylib", "geometer.dylib"]
    return ["libgeometer.so", "geometer.so"]


def _required_occt_runtime_names() -> list[str]:
    if sys.platform != "win32":
        return []
    return ["TKernel.dll", "TKMath.dll", "TKDESTEP.dll", "TKDEGLTF.dll"]


def _add_dll_directory(directory: Path) -> None:
    if sys.platform != "win32" or not hasattr(os, "add_dll_directory"):
        return
    handle = os.add_dll_directory(str(directory))
    _DLL_DIRECTORY_HANDLES.append(handle)
