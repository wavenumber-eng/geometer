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
import re
import shutil
import stat
import subprocess
import sys
from pathlib import Path

import dependency_versions
import occt_binary_cache

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

OCCT_REPO = dependency_versions.OCCT_REPO
OCCT_TAG = dependency_versions.OCCT_TAG
DEFAULT_MACOS_DEPLOYMENT_TARGET = "11.0"


def configure_occt_variant(tag: str, state_root: Path | None) -> None:
    global DEPS_DIR, NATIVE_DEPS_DIR, OCCT_SRC, OCCT_TAG
    if not re.fullmatch(r"V[0-9]+(?:_[0-9]+)+", tag):
        raise ValueError(f"OCCT tag must be an exact release tag, got {tag!r}")
    OCCT_TAG = tag
    if state_root is None:
        return
    resolved = state_root.resolve()
    generated_root = (ROOT / ".deps").resolve()
    if resolved == generated_root or generated_root not in resolved.parents:
        raise ValueError(f"OCCT state root must be a strict descendant of {generated_root}")
    DEPS_DIR = resolved
    NATIVE_DEPS_DIR = resolved / "native"
    OCCT_SRC = resolved / "occt-src"


def cmake_generator_args() -> list[str]:
    if sys.platform == "win32" and not shutil.which("cl"):
        machine = platform.machine().strip().lower()
        if machine in {"amd64", "x86_64"}:
            return ["-A", "x64"]
        if machine in {"aarch64", "arm64"}:
            return ["-A", "ARM64"]
        return []
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


def remove_tree(path: Path) -> None:
    def handle_remove_error(function, failed_path, _exc_info) -> None:
        os.chmod(failed_path, stat.S_IWRITE)
        function(failed_path)

    shutil.rmtree(path, onerror=handle_remove_error)


def source_tag(dest: Path) -> str | None:
    if not (dest / ".git").exists():
        return None
    try:
        return subprocess.check_output(
            ["git", "-C", str(dest), "describe", "--tags", "--exact-match", "HEAD"],
            text=True,
            stderr=subprocess.DEVNULL,
        ).strip()
    except subprocess.CalledProcessError:
        return None


def verify_source_tag(dest: Path, expected_tag: str) -> None:
    current_tag = source_tag(dest)
    if current_tag == expected_tag:
        return
    raise RuntimeError(
        f"OCCT source at {dest} is not {expected_tag} "
        f"(found {current_tag or 'unknown'}). Run "
        "`python scripts\\build_occt.py --clean --clean-source` before rebuilding."
    )


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
        verify_source_tag(OCCT_SRC, OCCT_TAG)
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


def occt_cache_profile(
    platform_name: str,
    config: str,
    library_type: str,
    macos_deployment_target_value: str | None,
) -> occt_binary_cache.OcctCacheProfile:
    resolved_macos_target = None
    if platform_name.startswith("macos-"):
        resolved_macos_target = macos_deployment_target(macos_deployment_target_value)
    resolved_linux_glibc = linux_glibc_baseline(platform_name)
    recipe = occt_binary_cache.recipe_hash(
        [
            Path(__file__),
            RAPIDJSON_SRC,
        ],
        {
            "recipe_schema": "native-install-a1",
            "kind": "native",
            "occt_repo": OCCT_REPO,
            "occt_tag": OCCT_TAG,
            "platform_tag": platform_name,
            "config": config,
            "library_type": library_type,
            "macos_deployment_target": resolved_macos_target or "",
            "linux_glibc_baseline": resolved_linux_glibc,
            "rapidjson_patch": RAPIDJSON_PATCH_SENTINEL,
        },
    )
    return occt_binary_cache.OcctCacheProfile(
        kind="native",
        platform_tag=platform_name,
        config=config,
        library_type=library_type,
        occt_repo=OCCT_REPO,
        occt_tag=OCCT_TAG,
        recipe_hash=recipe,
        macos_deployment_target=resolved_macos_target,
    )


def linux_glibc_baseline(platform_name: str) -> str:
    if not platform_name.startswith("linux-"):
        return ""
    libc_name, libc_version = platform.libc_ver()
    if libc_name != "glibc" or not libc_version:
        return f"{libc_name or 'unknown'}-{libc_version or 'unknown'}"
    major, minor = libc_version.replace("_", ".").split(".")[:2]
    return f"glibc-{major}.{minor}"


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
            remove_tree(d)


def prepare_source_build(platform_name: str, library_type: str) -> None:
    build_dir, install_dir = occt_paths(platform_name, library_type)
    for path in (build_dir, install_dir):
        if path.exists():
            print(f"Removing stale OCCT path {path}")
            remove_tree(path)


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
        "--occt-tag",
        default=dependency_versions.OCCT_TAG,
        help="Exact OCCT release tag (default: repository production pin).",
    )
    parser.add_argument(
        "--occt-state-root",
        type=Path,
        default=None,
        help="Isolated generated OCCT state root below .deps/ (qualification use).",
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
    parser.add_argument(
        "--binary-cache",
        choices=sorted(occt_binary_cache.VALID_MODES),
        default=None,
        help="Use prebuilt OCCT binary cache: auto, off, or only (default: env/auto).",
    )
    parser.add_argument(
        "--upload-binary-cache",
        action="store_true",
        help="Package and upload the resulting OCCT install tree to the configured binary cache.",
    )
    parser.add_argument(
        "--print-binary-cache-key",
        action="store_true",
        help="Print the computed OCCT binary cache key and exit.",
    )
    args = parser.parse_args()
    try:
        configure_occt_variant(args.occt_tag, args.occt_state_root)
    except ValueError as exc:
        parser.error(str(exc))

    if args.clean:
        clean(args.platform_tag, include_source=args.clean_source)
        return

    occt_binary_cache.load_dotenv(ROOT)
    profile = occt_cache_profile(
        args.platform_tag,
        args.config,
        args.library_type,
        args.macos_deployment_target,
    )
    if args.print_binary_cache_key:
        print(profile.cache_key)
        return

    print(f"Using native dependency platform {args.platform_tag}")
    if args.platform_tag.startswith("macos-"):
        print(f"Using macOS deployment target {macos_deployment_target(args.macos_deployment_target)}")
    verify_vendored_rapidjson()
    _, install_dir = occt_paths(args.platform_tag, args.library_type)
    if occt_binary_cache.install_matches_profile(install_dir, profile):
        print(f"OCCT install already present at {install_dir}")
    elif not occt_binary_cache.restore_prebuilt_install(profile, install_dir, mode=args.binary_cache):
        prepare_source_build(args.platform_tag, args.library_type)
        clone_occt()
        configure_occt(args.platform_tag, args.config, args.library_type, args.macos_deployment_target)
        build_occt(args.platform_tag, args.config, args.library_type)
        install_occt(args.platform_tag, args.config, args.library_type)

    if not occt_binary_cache.install_matches_profile(install_dir, profile):
        expected = occt_binary_cache.occt_version_from_tag(profile.occt_tag)
        actual = occt_binary_cache.installed_occt_version(install_dir) or "unknown"
        raise RuntimeError(f"OCCT install under {install_dir} is {actual}, expected {expected}.")

    if args.upload_binary_cache:
        occt_binary_cache.upload_prebuilt_install(
            profile,
            install_dir,
            out_dir=ROOT / "out" / "occt-binary-cache",
        )

    print(f"\nOCCT installed to {install_dir}")
    if args.library_type == "Shared":
        print("Now run:  cmake --preset shared-occt")
    else:
        print("Now run:  cmake --preset default")


if __name__ == "__main__":
    main()
