"""
Build geometer for WASM via Emscripten.

Manages the full chain: emsdk install, OCCT cross-compile, geometer build.
All artifacts land in .deps/ (emsdk, occt-wasm-*) and dist/ (final outputs).

Usage:
    python scripts/build_wasm.py
    python scripts/build_wasm.py --clean
"""

from __future__ import annotations

import argparse
import os
import shutil
import subprocess
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parent.parent
DEPS_DIR = ROOT / ".deps"
DIST_DIR = ROOT / "dist"

# Emsdk
EMSDK_DIR = DEPS_DIR / "emsdk"
EMSDK_REPO = "https://github.com/emscripten-core/emsdk.git"
EMSDK_VERSION = "3.1.56"

# OCCT (shared source with build_occt.py)
OCCT_SRC = DEPS_DIR / "occt-src"
OCCT_WASM_BUILD = DEPS_DIR / "occt-wasm-build"
OCCT_WASM_INSTALL = DEPS_DIR / "occt-wasm-install"

# RapidJSON is header-only and vendored for OCCT's GLB export support.
RAPIDJSON_SRC = ROOT / "third_party" / "rapidjson"
RAPIDJSON_INCLUDE = RAPIDJSON_SRC / "include" / "rapidjson"
RAPIDJSON_PATCH_SENTINEL = (
    "    GenericStringRef& operator=(const GenericStringRef& rhs) = delete;"
)

# Geometer WASM build
GEOMETER_WASM_BUILD = ROOT / "build-wasm"

OCCT_REPO = "https://github.com/Open-Cascade-SAS/OCCT.git"
OCCT_TAG = "V7_8_1"


def run(cmd: list[str], **kwargs) -> None:
    print(f"  > {' '.join(cmd)}")
    subprocess.check_call(cmd, **kwargs)


def _emsdk_exe() -> str:
    if sys.platform == "win32":
        return str(EMSDK_DIR / "emsdk.bat")
    return str(EMSDK_DIR / "emsdk")


def _emcmake() -> str:
    if sys.platform == "win32":
        return str(EMSDK_DIR / "upstream" / "emscripten" / "emcmake.bat")
    return str(EMSDK_DIR / "upstream" / "emscripten" / "emcmake")


def _toolchain_file() -> Path:
    return EMSDK_DIR / "upstream" / "emscripten" / "cmake" / "Modules" / "Platform" / "Emscripten.cmake"


def _emscripten_env() -> dict[str, str]:
    """Return env with EMSDK paths activated."""
    env = os.environ.copy()
    upstream = EMSDK_DIR / "upstream" / "emscripten"
    node_dir = next((EMSDK_DIR / "node").glob("*/bin"), None)
    paths = [str(upstream)]
    if node_dir:
        paths.append(str(node_dir))
    paths.append(str(EMSDK_DIR))
    env["PATH"] = os.pathsep.join(paths) + os.pathsep + env.get("PATH", "")
    env["EMSDK"] = str(EMSDK_DIR)
    env["EM_CONFIG"] = str(EMSDK_DIR / ".emscripten")
    return env


# ---- emsdk ----

def install_emsdk() -> None:
    if (EMSDK_DIR / "emsdk.bat").exists() or (EMSDK_DIR / "emsdk").exists():
        print(f"emsdk already present at {EMSDK_DIR}")
    else:
        print(f"Cloning emsdk ...")
        DEPS_DIR.mkdir(parents=True, exist_ok=True)
        run(["git", "clone", "--depth", "1", EMSDK_REPO, str(EMSDK_DIR)])

    print(f"Installing Emscripten {EMSDK_VERSION} ...")
    run([_emsdk_exe(), "install", EMSDK_VERSION])
    run([_emsdk_exe(), "activate", EMSDK_VERSION])

    toolchain = _toolchain_file()
    if not toolchain.exists():
        raise RuntimeError(f"Toolchain file not found at {toolchain}")
    print(f"Emscripten ready. Toolchain: {toolchain}")


# ---- shared sources ----

def clone_source(name: str, dest: Path, repo: str, tag: str, check_path: str) -> None:
    check = dest / check_path
    if check.exists():
        print(f"{name} already present at {dest}")
        return
    print(f"Cloning {name} {tag} ...")
    DEPS_DIR.mkdir(parents=True, exist_ok=True)
    run(["git", "clone", "--depth", "1", "--branch", tag, repo, str(dest)])


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


# ---- OCCT WASM build ----

def build_occt_wasm() -> None:
    cmake_config = OCCT_WASM_INSTALL / "lib" / "cmake" / "opencascade" / "OpenCASCADEConfig.cmake"
    if cmake_config.exists():
        print(f"OCCT WASM already built at {OCCT_WASM_INSTALL}")
        return

    toolchain = _toolchain_file()
    env = _emscripten_env()

    print("Configuring OCCT for WASM ...")
    OCCT_WASM_BUILD.mkdir(parents=True, exist_ok=True)
    run([
        "cmake",
        "-G", "Ninja",
        "-S", str(OCCT_SRC),
        "-B", str(OCCT_WASM_BUILD),
        f"-DCMAKE_TOOLCHAIN_FILE={toolchain}",
        f"-DCMAKE_INSTALL_PREFIX={OCCT_WASM_INSTALL}",
        "-DCMAKE_BUILD_TYPE=Release",
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
        "-DCMAKE_POLICY_VERSION_MINIMUM=3.5",
    ], env=env)

    print("Building OCCT for WASM ...")
    run([
        "cmake", "--build", str(OCCT_WASM_BUILD),
        "--config", "Release",
        "--parallel",
    ], env=env)

    print("Installing OCCT WASM ...")
    run([
        "cmake", "--install", str(OCCT_WASM_BUILD),
        "--config", "Release",
    ], env=env)

    print(f"OCCT WASM installed to {OCCT_WASM_INSTALL}")


# ---- geometer WASM build ----

def build_geometer_wasm() -> None:
    toolchain = _toolchain_file()
    env = _emscripten_env()
    # WASM install uses lib/cmake/opencascade/ (different from native's cmake/)
    occt_cmake_dir = OCCT_WASM_INSTALL / "lib" / "cmake" / "opencascade"

    print("Configuring geometer for WASM ...")
    GEOMETER_WASM_BUILD.mkdir(parents=True, exist_ok=True)
    run([
        "cmake",
        "-G", "Ninja",
        "-S", str(ROOT),
        "-B", str(GEOMETER_WASM_BUILD),
        f"-DCMAKE_TOOLCHAIN_FILE={toolchain}",
        "-DCMAKE_BUILD_TYPE=Release",
        f"-DOpenCASCADE_DIR={occt_cmake_dir}",
        "-DCMAKE_POLICY_VERSION_MINIMUM=3.5",
    ], env=env)

    print("Building geometer for WASM ...")
    run([
        "cmake", "--build", str(GEOMETER_WASM_BUILD),
        "--config", "Release",
    ], env=env)

    # Copy outputs to dist/
    DIST_DIR.mkdir(parents=True, exist_ok=True)
    outputs = [
        GEOMETER_WASM_BUILD / "src" / "cpp" / "cli" / "geometer-node-test.js",
        GEOMETER_WASM_BUILD / "src" / "cpp" / "cli" / "geometer-node-test.wasm",
        GEOMETER_WASM_BUILD / "src" / "cpp" / "lib" / "geometer.js",
        GEOMETER_WASM_BUILD / "src" / "cpp" / "lib" / "geometer.wasm",
        GEOMETER_WASM_BUILD / "src" / "cpp" / "lib" / "geometer-planar-browser.js",
        GEOMETER_WASM_BUILD / "src" / "cpp" / "lib" / "geometer-planar-browser.wasm",
    ]
    for src in outputs:
        if src.exists():
            dst = DIST_DIR / src.name
            shutil.copy2(str(src), str(dst))
            print(f"Copied {src.name} to dist/ ({dst.stat().st_size:,} bytes)")

    run([sys.executable, str(ROOT / "scripts" / "write_dist_manifest.py")])
    print("geometer WASM build complete.")


# ---- clean ----

def clean() -> None:
    for d in [EMSDK_DIR, OCCT_WASM_BUILD, OCCT_WASM_INSTALL, GEOMETER_WASM_BUILD]:
        if d.exists():
            print(f"Removing {d}")
            shutil.rmtree(d)


# ---- main ----

def main() -> None:
    parser = argparse.ArgumentParser(description="Build geometer for WASM via Emscripten.")
    parser.add_argument("--clean", action="store_true", help="Remove all WASM build artifacts")
    args = parser.parse_args()

    if args.clean:
        clean()
        return

    install_emsdk()
    verify_vendored_rapidjson()
    clone_source("OCCT", OCCT_SRC, OCCT_REPO, OCCT_TAG, "CMakeLists.txt")
    build_occt_wasm()
    build_geometer_wasm()


if __name__ == "__main__":
    main()
