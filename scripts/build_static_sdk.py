"""Build and package the supported Geometer static C ABI SDK profile."""

from __future__ import annotations

import argparse
import json
import platform
import re
import subprocess
import sys
import tomllib
from pathlib import Path

from package_static_sdk import SUPPORTED_PLATFORMS, package


ROOT = Path(__file__).resolve().parents[1]


def native_platform() -> str:
    if sys.platform == "win32":
        os_name = "windows"
    elif sys.platform == "darwin":
        os_name = "macos"
    elif sys.platform.startswith("linux"):
        os_name = "linux"
    else:
        raise RuntimeError(f"Unsupported SDK host: {sys.platform}")
    machine = platform.machine().lower()
    architecture = {"amd64": "x64", "x86_64": "x64", "aarch64": "arm64", "arm64": "arm64"}.get(machine, machine)
    return f"{os_name}-{architecture}"


def run(command: list[str]) -> None:
    print("  >", " ".join(command), flush=True)
    subprocess.check_call(command, cwd=ROOT)


def validate_windows_static_runtime(build_dir: Path) -> None:
    commands = json.loads((build_dir / "compile_commands.json").read_text(encoding="utf-8"))
    geometer_commands = [
        str(entry["command"])
        for entry in commands
        if "src\\cpp\\lib" in str(entry["file"]) or "src/cpp/lib" in str(entry["file"])
    ]
    if not geometer_commands:
        raise RuntimeError("No Geometer compile commands were available for CRT validation")
    if any(re.search(r"(?:^|\s)[/-]MDd?(?:\s|$)", command) for command in geometer_commands):
        raise RuntimeError("Windows SDK objects contain dynamic-CRT /MD compilation")
    if any(not re.search(r"(?:^|\s)[/-]MTd?(?:\s|$)", command) for command in geometer_commands):
        raise RuntimeError("Windows SDK objects are not uniformly compiled with /MT")


def default_output(platform_name: str) -> Path:
    version = tomllib.loads((ROOT / "pyproject.toml").read_text(encoding="utf-8"))["project"]["version"]
    return ROOT / "out/sdk-candidate" / f"geometer-sdk-{version}-{platform_name}.zip"


def build_layout(platform_name: str) -> tuple[str, Path]:
    """Return the fixed CMake graph for a supported SDK platform."""
    if platform_name == "windows-x64":
        return "static-sdk", ROOT / "build-static-sdk"
    return "default", ROOT / f"build-native-{platform_name}"


def main() -> None:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--platform", choices=sorted(SUPPORTED_PLATFORMS), default=native_platform())
    parser.add_argument("--output", type=Path, default=None)
    parser.add_argument("--allow-dirty", action="store_true", help="Local development only")
    parser.add_argument(
        "--skip-dependency-build",
        action="store_true",
        help="Use an already-qualified OCCT install (local validation only).",
    )
    args = parser.parse_args()
    actual_platform = native_platform()
    if args.platform != actual_platform:
        parser.error(f"SDK target {args.platform} cannot be built on host {actual_platform}")

    if not args.skip_dependency_build:
        dependency_command = [
            sys.executable,
            "scripts/build_occt.py",
            "--library-type",
            "Static",
            "--platform-tag",
            args.platform,
        ]
        if args.platform == "windows-x64":
            dependency_command.extend(["--msvc-runtime", "Static"])
        if args.platform == "macos-arm64":
            dependency_command.extend(["--macos-deployment-target", "11.0"])
        run(dependency_command)

    preset, build_dir = build_layout(args.platform)
    configure_command = [
        "cmake",
        "--preset",
        preset,
        "-B",
        str(build_dir),
        f"-DGEOMETER_NATIVE_DIST_PLATFORM={args.platform}",
    ]
    if args.platform == "macos-arm64":
        configure_command.append("-DCMAKE_OSX_DEPLOYMENT_TARGET=11.0")
    run(configure_command)
    run(
        [
            "cmake",
            "--build",
            str(build_dir),
            "--config",
            "Release",
            "--target",
            "geometer_lib",
            "geometer_sdk_link_probe",
            "--",
            "-d",
            "keeprsp",
        ]
    )
    if args.platform == "windows-x64":
        validate_windows_static_runtime(build_dir)
    output = args.output.resolve() if args.output is not None else default_output(args.platform)
    package(build_dir, args.platform, output, args.allow_dirty)
    print(f"Static SDK candidate ready: {output}")


if __name__ == "__main__":
    main()
