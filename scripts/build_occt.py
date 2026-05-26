"""
Build OCCT from source into .deps/native/<platform>/occt-install/.

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
    python scripts/build_occt.py --clean --clean-source
"""

from __future__ import annotations

import argparse
import os
import platform
import shutil
import subprocess
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parent.parent
DEPS_DIR = ROOT / ".deps"
NATIVE_DEPS_DIR = DEPS_DIR / "native"
THIRD_PARTY_DIR = ROOT / "third_party"
OCCT_SRC = DEPS_DIR / "occt-src"
RAPIDJSON_SRC = THIRD_PARTY_DIR / "rapidjson"
RAPIDJSON_INCLUDE = RAPIDJSON_SRC / "include" / "rapidjson"
RAPIDJSON_PATCH_SENTINEL = (
    "    GenericStringRef& operator=(const GenericStringRef& rhs) = delete;"
)

OCCT_REPO = "https://github.com/Open-Cascade-SAS/OCCT.git"
OCCT_TAG = "V7_8_1"
DEFAULT_MACOS_DEPLOYMENT_TARGET = "11.0"


def cmake_generator_args() -> list[str]:
    if shutil.which("ninja"):
        return ["-G", "Ninja"]
    return []


def platform_tag() -> str:
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


def occt_paths(platform_name: str, library_type: str) -> tuple[Path, Path]:
    platform_dir = NATIVE_DEPS_DIR / platform_name
    if library_type == "Shared":
        return platform_dir / "occt-shared-build", platform_dir / "occt-shared-install"
    return platform_dir / "occt-build", platform_dir / "occt-install"


def configure_occt(
    platform_name: str,
    config: str,
    library_type: str,
    macos_deployment_target: str | None,
) -> None:
    build_dir, install_dir = occt_paths(platform_name, library_type)
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
        "-DBUILD_MODULE_DETools=OFF",
        "-DBUILD_YACCLEX=OFF",
        "-DBUILD_DOC_Overview=OFF",
        "-DUSE_FREETYPE=OFF",
        "-DUSE_TBB=OFF",
        "-DUSE_FREEIMAGE=OFF",
        "-DUSE_OPENVR=OFF",
        "-DUSE_RAPIDJSON=ON",
        f"-D3RDPARTY_RAPIDJSON_DIR={RAPIDJSON_SRC}",
        # OCCT declares cmake_minimum_required(VERSION 2.6) which CMake 4+ rejects.
        "-DCMAKE_POLICY_VERSION_MINIMUM=3.5",
        *macos_cmake_args(platform_name, macos_deployment_target),
    ]

    run(cmd)


def build_occt(platform_name: str, config: str, library_type: str) -> None:
    build_dir, _ = occt_paths(platform_name, library_type)
    print(f"Building OCCT ({config}, {library_type}) ...")
    run([
        "cmake", "--build", str(build_dir),
        "--config", config,
        "--parallel",
    ])


def install_occt(platform_name: str, config: str, library_type: str) -> None:
    build_dir, install_dir = occt_paths(platform_name, library_type)
    print(f"Installing OCCT to {install_dir} ...")
    run([
        "cmake", "--install", str(build_dir),
        "--config", config,
    ])


def clean(platform_name: str, *, include_source: bool) -> None:
    targets = [NATIVE_DEPS_DIR / platform_name]
    if include_source:
        targets.append(OCCT_SRC)
    for d in targets:
        if d.exists():
            print(f"Removing {d}")
            shutil.rmtree(d)


def macos_deployment_target(configured: str | None) -> str:
    return (
        configured
        or os.environ.get("GEOMETER_MACOS_DEPLOYMENT_TARGET")
        or os.environ.get("MACOSX_DEPLOYMENT_TARGET")
        or DEFAULT_MACOS_DEPLOYMENT_TARGET
    ).replace("_", ".")


def macos_cmake_args(platform_name: str, configured: str | None) -> list[str]:
    if not platform_name.startswith("macos-"):
        return []
    target = macos_deployment_target(configured)
    args = [
        f"-DCMAKE_OSX_DEPLOYMENT_TARGET={target}",
    ]
    if platform_name == "macos-arm64":
        args.append("-DCMAKE_OSX_ARCHITECTURES=arm64")
    elif platform_name == "macos-x64":
        args.append("-DCMAKE_OSX_ARCHITECTURES=x86_64")
    return args


def main() -> None:
    parser = argparse.ArgumentParser(description="Build OCCT from source.")
    parser.add_argument("--config", default="Release", help="Build config (default: Release)")
    parser.add_argument(
        "--library-type",
        choices=["Static", "Shared"],
        default="Static",
        help="OCCT library type to build (default: Static)",
    )
    parser.add_argument(
        "--platform-tag",
        default=platform_tag(),
        help="Native dependency platform tag (default: current platform)",
    )
    parser.add_argument(
        "--macos-deployment-target",
        default=None,
        help=f"Minimum macOS deployment target for native dependencies (default: {DEFAULT_MACOS_DEPLOYMENT_TARGET})",
    )
    parser.add_argument("--clean", action="store_true", help="Remove all OCCT build artifacts")
    parser.add_argument(
        "--clean-source",
        action="store_true",
        help="Also remove the shared OCCT source checkout when cleaning.",
    )
    args = parser.parse_args()

    if args.clean:
        clean(args.platform_tag, include_source=args.clean_source)
        return

    print(f"Using native dependency platform {args.platform_tag}")
    if args.platform_tag.startswith("macos-"):
        print(f"Using macOS deployment target {macos_deployment_target(args.macos_deployment_target)}")
    verify_vendored_rapidjson()
    clone_occt()
    configure_occt(args.platform_tag, args.config, args.library_type, args.macos_deployment_target)
    build_occt(args.platform_tag, args.config, args.library_type)
    install_occt(args.platform_tag, args.config, args.library_type)
    _, install_dir = occt_paths(args.platform_tag, args.library_type)
    print(f"\nOCCT installed to {install_dir}")
    if args.library_type == "Shared":
        print("Now run:  cmake --preset shared-occt")
    else:
        print("Now run:  cmake --preset default")


if __name__ == "__main__":
    main()
