"""
Build OCCT from source into .deps/occt-install/.

This exists because OCCT's CMakeLists.txt uses CMAKE_SOURCE_DIR internally,
which prevents it from working as a FetchContent subdirectory. Instead we
clone, configure, build, and install it as a standalone project, then
geometer's CMakeLists.txt finds it via find_package(OpenCASCADE).

Usage:
    python scripts/build_occt.py
    python scripts/build_occt.py --config Debug
    python scripts/build_occt.py --clean
"""

from __future__ import annotations

import argparse
import shutil
import subprocess
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parent.parent
DEPS_DIR = ROOT / ".deps"
OCCT_SRC = DEPS_DIR / "occt-src"
OCCT_BUILD = DEPS_DIR / "occt-build"
OCCT_INSTALL = DEPS_DIR / "occt-install"

OCCT_REPO = "https://github.com/Open-Cascade-SAS/OCCT.git"
OCCT_TAG = "V7_8_1"

RAPIDJSON_SRC = DEPS_DIR / "rapidjson-src"
RAPIDJSON_REPO = "https://github.com/Tencent/rapidjson.git"
RAPIDJSON_TAG = "v1.1.0"


def run(cmd: list[str], **kwargs) -> None:
    print(f"  > {' '.join(cmd)}")
    subprocess.check_call(cmd, **kwargs)


def clone_rapidjson() -> None:
    if (RAPIDJSON_SRC / "include" / "rapidjson").exists():
        print(f"RapidJSON already present at {RAPIDJSON_SRC}")
        return
    print(f"Cloning RapidJSON {RAPIDJSON_TAG} ...")
    DEPS_DIR.mkdir(parents=True, exist_ok=True)
    run([
        "git", "clone",
        "--depth", "1",
        "--branch", RAPIDJSON_TAG,
        RAPIDJSON_REPO,
        str(RAPIDJSON_SRC),
    ])


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


def configure_occt(config: str) -> None:
    print(f"Configuring OCCT ({config}) ...")
    OCCT_BUILD.mkdir(parents=True, exist_ok=True)
    cmd = [
        "cmake",
        "-S", str(OCCT_SRC),
        "-B", str(OCCT_BUILD),
        f"-DCMAKE_INSTALL_PREFIX={OCCT_INSTALL}",
        f"-DCMAKE_BUILD_TYPE={config}",
        "-DBUILD_LIBRARY_TYPE=Static",
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

    # On Windows with MSVC, use the multi-config generator default
    if sys.platform == "win32":
        cmd = [c for c in cmd if not c.startswith("-DCMAKE_BUILD_TYPE=")]

    run(cmd)


def build_occt(config: str) -> None:
    print(f"Building OCCT ({config}) ...")
    run([
        "cmake", "--build", str(OCCT_BUILD),
        "--config", config,
        "--parallel",
    ])


def install_occt(config: str) -> None:
    print(f"Installing OCCT to {OCCT_INSTALL} ...")
    run([
        "cmake", "--install", str(OCCT_BUILD),
        "--config", config,
    ])


def clean() -> None:
    for d in [OCCT_SRC, OCCT_BUILD, OCCT_INSTALL, RAPIDJSON_SRC]:
        if d.exists():
            print(f"Removing {d}")
            shutil.rmtree(d)


def main() -> None:
    parser = argparse.ArgumentParser(description="Build OCCT from source.")
    parser.add_argument("--config", default="Release", help="Build config (default: Release)")
    parser.add_argument("--clean", action="store_true", help="Remove all OCCT build artifacts")
    args = parser.parse_args()

    if args.clean:
        clean()
        return

    clone_rapidjson()
    clone_occt()
    configure_occt(args.config)
    build_occt(args.config)
    install_occt(args.config)
    print(f"\nOCCT installed to {OCCT_INSTALL}")
    print(f"Now run:  cmake --preset default")


if __name__ == "__main__":
    main()
