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
import re
import shutil
import subprocess
import sys
from pathlib import Path

import occt_binary_cache

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
OCCT_WASM_PLATFORM_TAG = "wasm-emscripten"


def run(cmd: list[str], **kwargs) -> None:
    print(f"  > {' '.join(cmd)}")
    subprocess.check_call(cmd, **kwargs)


def normalize_generated_js(path: Path) -> None:
    """Keep committed Emscripten JS artifacts compatible with diff hygiene."""
    text = path.read_text(encoding="utf-8")
    normalized = re.sub(r"[ \t]+(?=\r?\n|$)", "", text).replace("\r\n", "\n")
    if normalized != text:
        path.write_text(normalized, encoding="utf-8", newline="\n")


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
        print("Cloning emsdk ...")
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


def patch_occt_wasm_install_rules() -> None:
    """Allow OCCT's Emscripten helper executables to omit .wasm side files."""
    toolkit_cmake = OCCT_SRC / "adm" / "cmake" / "occt_toolkit.cmake"
    text = toolkit_cmake.read_text(encoding="utf-8")
    original = (
        '    install(FILES ${CMAKE_BINARY_DIR}/${OS_WITH_BIT}/${COMPILER}/bin\\${OCCT_INSTALL_BIN_LETTER}/${PROJECT_NAME}.wasm '
        'DESTINATION "${INSTALL_DIR_BIN}/${OCCT_INSTALL_BIN_LETTER}")'
    )
    patched = (
        '    install(FILES ${CMAKE_BINARY_DIR}/${OS_WITH_BIT}/${COMPILER}/bin\\${OCCT_INSTALL_BIN_LETTER}/${PROJECT_NAME}.wasm '
        'DESTINATION "${INSTALL_DIR_BIN}/${OCCT_INSTALL_BIN_LETTER}" OPTIONAL)'
    )
    if patched in text:
        print("OCCT WASM install rules already patched.")
        return
    if original not in text:
        raise RuntimeError(f"OCCT WASM install rule not found in {toolkit_cmake}")
    toolkit_cmake.write_text(text.replace(original, patched), encoding="utf-8", newline="\n")
    print("Patched OCCT WASM install rules for optional executable side modules.")


# ---- OCCT WASM build ----

def occt_wasm_cache_profile() -> occt_binary_cache.OcctCacheProfile:
    recipe = occt_binary_cache.recipe_hash(
        [
            Path(__file__),
            ROOT / "scripts" / "build_occt.py",
            ROOT / "scripts" / "occt_binary_cache.py",
            RAPIDJSON_SRC,
        ],
        {
            "kind": "wasm",
            "occt_repo": OCCT_REPO,
            "occt_tag": OCCT_TAG,
            "emsdk_version": EMSDK_VERSION,
            "platform_tag": OCCT_WASM_PLATFORM_TAG,
            "config": "Release",
            "library_type": "Static",
            "wasm_install_patch": "optional-helper-side-modules-a0",
            "rapidjson_patch": RAPIDJSON_PATCH_SENTINEL,
        },
    )
    return occt_binary_cache.OcctCacheProfile(
        kind="wasm",
        platform_tag=OCCT_WASM_PLATFORM_TAG,
        config="Release",
        library_type="Static",
        occt_repo=OCCT_REPO,
        occt_tag=OCCT_TAG,
        recipe_hash=recipe,
        emsdk_version=EMSDK_VERSION,
    )


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

    # Copy outputs to grouped dist/wasm target folders.
    DIST_DIR.mkdir(parents=True, exist_ok=True)
    outputs = [
        (
            GEOMETER_WASM_BUILD / "src" / "cpp" / "cli" / "geometer-node-test.js",
            DIST_DIR / "wasm" / "node-test",
        ),
        (
            GEOMETER_WASM_BUILD / "src" / "cpp" / "cli" / "geometer-node-test.wasm",
            DIST_DIR / "wasm" / "node-test",
        ),
        (
            GEOMETER_WASM_BUILD / "src" / "cpp" / "lib" / "geometer.js",
            DIST_DIR / "wasm" / "browser",
        ),
        (
            GEOMETER_WASM_BUILD / "src" / "cpp" / "lib" / "geometer.wasm",
            DIST_DIR / "wasm" / "browser",
        ),
        (
            GEOMETER_WASM_BUILD / "src" / "cpp" / "lib" / "geometer-planar-browser.js",
            DIST_DIR / "wasm" / "planar-browser",
        ),
        (
            GEOMETER_WASM_BUILD / "src" / "cpp" / "lib" / "geometer-planar-browser.wasm",
            DIST_DIR / "wasm" / "planar-browser",
        ),
    ]
    for src, target_dir in outputs:
        if src.exists():
            target_dir.mkdir(parents=True, exist_ok=True)
            dst = target_dir / src.name
            shutil.copy2(str(src), str(dst))
            if dst.suffix == ".js":
                normalize_generated_js(dst)
            print(f"Copied {src.name} to {target_dir.relative_to(DIST_DIR)}/ ({dst.stat().st_size:,} bytes)")

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
    parser.add_argument(
        "--occt-binary-cache",
        choices=sorted(occt_binary_cache.VALID_MODES),
        default=None,
        help="Use prebuilt WASM OCCT binary cache: auto, off, or only (default: env/auto).",
    )
    parser.add_argument(
        "--upload-occt-binary-cache",
        action="store_true",
        help="Package and upload the WASM OCCT install tree to the configured binary cache.",
    )
    parser.add_argument(
        "--print-occt-binary-cache-key",
        action="store_true",
        help="Print the computed WASM OCCT binary cache key and exit.",
    )
    parser.add_argument(
        "--occt-only",
        action="store_true",
        help="Stop after preparing the WASM OCCT install tree.",
    )
    args = parser.parse_args()

    if args.clean:
        clean()
        return

    occt_binary_cache.load_dotenv(ROOT)
    profile = occt_wasm_cache_profile()
    if args.print_occt_binary_cache_key:
        print(profile.cache_key)
        return

    install_emsdk()
    verify_vendored_rapidjson()
    if occt_binary_cache.install_ready(OCCT_WASM_INSTALL):
        print(f"OCCT WASM already built at {OCCT_WASM_INSTALL}")
    elif not occt_binary_cache.restore_prebuilt_install(profile, OCCT_WASM_INSTALL, mode=args.occt_binary_cache):
        clone_source("OCCT", OCCT_SRC, OCCT_REPO, OCCT_TAG, "CMakeLists.txt")
        patch_occt_wasm_install_rules()
        build_occt_wasm()

    if not occt_binary_cache.install_ready(OCCT_WASM_INSTALL):
        raise RuntimeError(f"OCCT WASM install did not produce OpenCASCADEConfig.cmake under {OCCT_WASM_INSTALL}")

    if args.upload_occt_binary_cache:
        occt_binary_cache.upload_prebuilt_install(
            profile,
            OCCT_WASM_INSTALL,
            out_dir=ROOT / "out" / "occt-binary-cache",
        )

    if args.occt_only:
        return

    build_geometer_wasm()


if __name__ == "__main__":
    main()
