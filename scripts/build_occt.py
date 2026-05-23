"""
Build OCCT from source into .deps/occt-install/.

This exists because OCCT's CMakeLists.txt uses CMAKE_SOURCE_DIR internally,
which prevents it from working as a FetchContent subdirectory. Instead we
clone, configure, build, and install it as a standalone project, then
geometer's CMakeLists.txt finds it via find_package(OpenCASCADE).
RapidJSON is header-only and vendored under third_party/rapidjson for OCCT's
GLB export support.

Usage:
    python scripts/build_occt.py
    python scripts/build_occt.py --config Debug
    python scripts/build_occt.py --library-type Shared
    python scripts/build_occt.py --clean
"""

from __future__ import annotations

import argparse
import shutil
import subprocess
from pathlib import Path

ROOT = Path(__file__).resolve().parent.parent
DEPS_DIR = ROOT / ".deps"
THIRD_PARTY_DIR = ROOT / "third_party"
OCCT_SRC = DEPS_DIR / "occt-src"
OCCT_STATIC_BUILD = DEPS_DIR / "occt-build"
OCCT_STATIC_INSTALL = DEPS_DIR / "occt-install"
OCCT_SHARED_BUILD = DEPS_DIR / "occt-shared-build"
OCCT_SHARED_INSTALL = DEPS_DIR / "occt-shared-install"
RAPIDJSON_SRC = THIRD_PARTY_DIR / "rapidjson"
RAPIDJSON_INCLUDE = RAPIDJSON_SRC / "include" / "rapidjson"
RAPIDJSON_PATCH_SENTINEL = (
    "    GenericStringRef& operator=(const GenericStringRef& rhs) = delete;"
)

OCCT_REPO = "https://github.com/Open-Cascade-SAS/OCCT.git"
OCCT_TAG = "V7_8_1"


def cmake_generator_args() -> list[str]:
    if shutil.which("ninja"):
        return ["-G", "Ninja"]
    return []


def run(cmd: list[str], **kwargs) -> None:
    print(f"  > {' '.join(cmd)}")
    subprocess.check_call(cmd, **kwargs)


def verify_vendored_rapidjson() -> None:
    document_h = RAPIDJSON_INCLUDE / "document.h"
    if not document_h.exists():
        raise RuntimeError(
            "Vendored RapidJSON headers are missing. Expected "
            f"{RAPIDJSON_INCLUDE}"
        )
    text = document_h.read_text(encoding="utf-8")
    if RAPIDJSON_PATCH_SENTINEL not in text:
        raise RuntimeError(
            "Vendored RapidJSON is missing Geometer's modern Clang "
            "compatibility patch."
        )
    print(f"Using vendored RapidJSON at {RAPIDJSON_SRC}")


def clone_occt() -> None:
    if (OCCT_SRC / "CMakeLists.txt").exists():
        print(f"OCCT source already present at {OCCT_SRC}")
        return
    print(f"Cloning OCCT {OCCT_TAG} ...")
    DEPS_DIR.mkdir(parents=True, exist_ok=True)
    run([
        "git", "clone",
        "--depth", "1",
        "--branch", OCCT_TAG,
        OCCT_REPO,
        str(OCCT_SRC),
    ])


def occt_paths(library_type: str) -> tuple[Path, Path]:
    if library_type == "Shared":
        return OCCT_SHARED_BUILD, OCCT_SHARED_INSTALL
    return OCCT_STATIC_BUILD, OCCT_STATIC_INSTALL


def configure_occt(config: str, library_type: str) -> None:
    build_dir, install_dir = occt_paths(library_type)
    print(f"Configuring OCCT ({config}, {library_type}) ...")
    build_dir.mkdir(parents=True, exist_ok=True)
    cmd = [
        "cmake",
        *cmake_generator_args(),
        "-S", str(OCCT_SRC),
        "-B", str(build_dir),
        f"-DCMAKE_INSTALL_PREFIX={install_dir}",
        f"-DCMAKE_BUILD_TYPE={config}",
        f"-DBUILD_LIBRARY_TYPE={library_type}",
        "-DBUILD_MODULE_Draw=OFF",
        "-DBUILD_MODULE_Visualization=OFF",
        "-DBUILD_MODULE_ApplicationFramework=OFF",
        "-DBUILD_DOC_Overview=OFF",
        "-DUSE_FREETYPE=OFF",
        "-DUSE_TBB=OFF",
        "-DUSE_FREEIMAGE=OFF",
        "-DUSE_OPENVR=OFF",
        "-DUSE_RAPIDJSON=ON",
        f"-D3RDPARTY_RAPIDJSON_DIR={RAPIDJSON_SRC}",
        # OCCT declares cmake_minimum_required(VERSION 2.6) which CMake 4+ rejects.
        "-DCMAKE_POLICY_VERSION_MINIMUM=3.5",
    ]

    run(cmd)


def build_occt(config: str, library_type: str) -> None:
    build_dir, _ = occt_paths(library_type)
    print(f"Building OCCT ({config}, {library_type}) ...")
    run([
        "cmake", "--build", str(build_dir),
        "--config", config,
        "--parallel",
    ])


def install_occt(config: str, library_type: str) -> None:
    build_dir, install_dir = occt_paths(library_type)
    print(f"Installing OCCT to {install_dir} ...")
    run([
        "cmake", "--install", str(build_dir),
        "--config", config,
    ])


def clean() -> None:
    for d in [
        OCCT_SRC,
        OCCT_STATIC_BUILD,
        OCCT_STATIC_INSTALL,
        OCCT_SHARED_BUILD,
        OCCT_SHARED_INSTALL,
    ]:
        if d.exists():
            print(f"Removing {d}")
            shutil.rmtree(d)


def main() -> None:
    parser = argparse.ArgumentParser(description="Build OCCT from source.")
    parser.add_argument("--config", default="Release", help="Build config (default: Release)")
    parser.add_argument(
        "--library-type",
        choices=["Static", "Shared"],
        default="Static",
        help="OCCT library type to build (default: Static)",
    )
    parser.add_argument("--clean", action="store_true", help="Remove all OCCT build artifacts")
    args = parser.parse_args()

    if args.clean:
        clean()
        return

    verify_vendored_rapidjson()
    clone_occt()
    configure_occt(args.config, args.library_type)
    build_occt(args.config, args.library_type)
    install_occt(args.config, args.library_type)
    _, install_dir = occt_paths(args.library_type)
    print(f"\nOCCT installed to {install_dir}")
    if args.library_type == "Shared":
        print("Now run:  cmake --preset shared-occt")
    else:
        print("Now run:  cmake --preset default")


if __name__ == "__main__":
    main()
