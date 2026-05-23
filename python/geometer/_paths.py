from __future__ import annotations

import ctypes
import ctypes.util
import os
import sys
from pathlib import Path


_DLL_DIRECTORY_HANDLES: list[object] = []


def native_library_path() -> Path:
    path = _find_native_library_path()
    if path is None:
        raise FileNotFoundError(
            "Could not find the Geometer native library. Build geometer_shared "
            "or set GEOMETER_NATIVE_LIBRARY to the shared library path."
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
        "Could not find the Geometer native library. Build geometer_shared "
        "or set GEOMETER_NATIVE_LIBRARY to the shared library path."
    )


def _find_native_library_path() -> Path | None:
    override = os.environ.get("GEOMETER_NATIVE_LIBRARY")
    if override:
        path = Path(override)
        if path.exists():
            return path
        raise FileNotFoundError(f"GEOMETER_NATIVE_LIBRARY does not exist: {path}")

    for directory in _candidate_directories():
        for name in _library_names():
            path = directory / name
            if path.exists():
                return path
    return None


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


def _library_names() -> list[str]:
    if sys.platform == "win32":
        return ["geometer.dll"]
    if sys.platform == "darwin":
        return ["libgeometer.dylib", "geometer.dylib"]
    return ["libgeometer.so", "geometer.so"]


def _add_dll_directory(directory: Path) -> None:
    if sys.platform != "win32" or not hasattr(os, "add_dll_directory"):
        return
    handle = os.add_dll_directory(str(directory))
    _DLL_DIRECTORY_HANDLES.append(handle)
