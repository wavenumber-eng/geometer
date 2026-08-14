from __future__ import annotations

import argparse
import json
import os
import platform
import re
import shutil
import subprocess
import sys
from pathlib import Path


ROOT = Path(__file__).resolve().parent.parent
DEFAULT_STEP = ROOT / "tests" / "fixtures" / "step" / "embedded_models" / "SOT-23.STEP"
DEFAULT_MACOS_DEPLOYMENT_TARGET = "11.0"


def main() -> int:
    tag = platform_tag()
    parser = argparse.ArgumentParser(description="Validate the Geometer native build for this platform.")
    parser.add_argument("--build-dir", type=Path, default=ROOT / f"build-native-{tag}")
    parser.add_argument("--dist-root", type=Path, default=ROOT / "dist")
    parser.add_argument("--occt-dir", type=Path, default=None)
    parser.add_argument("--validation-out", type=Path, default=None)
    parser.add_argument("--config", default="Release")
    parser.add_argument("--step", type=Path, default=DEFAULT_STEP)
    parser.add_argument("--skip-ctest", action="store_true")
    parser.add_argument("--skip-examples", action="store_true")
    parser.add_argument("--skip-python", action="store_true")
    parser.add_argument("--skip-ldd", action="store_true")
    args = parser.parse_args()

    step_path = args.step.resolve()
    if not step_path.exists():
        raise FileNotFoundError(step_path)

    build_dir = args.build_dir.resolve()
    dist_root = args.dist_root.resolve()
    occt_dir = args.occt_dir.resolve() if args.occt_dir is not None else None
    validation_out = (
        args.validation_out.resolve()
        if args.validation_out is not None
        else ROOT / "out" / "native-validation" / tag
    )
    env = native_build_env()
    run(
        cmake_configure_command(
            build_dir,
            build_examples=not args.skip_examples,
            dist_root=dist_root,
            occt_dir=occt_dir,
        ),
        env=env,
    )
    run(["cmake", "--build", str(build_dir), "--config", args.config], env=env)

    exe = dist_root / "native" / tag / executable_name()
    if not exe.exists():
        raise FileNotFoundError(f"Expected native executable was not produced: {exe}")
    if not args.skip_examples:
        preview_exe = dist_root / "native" / tag / preview_executable_name()
        if not preview_exe.exists():
            raise FileNotFoundError(f"Expected native preview executable was not produced: {preview_exe}")

    run([str(exe), "--version"])
    if sys.platform == "darwin":
        validate_macos_executable_target(exe, macos_deployment_target())
    validate_cli_outputs(exe, step_path, validation_out)

    if sys.platform.startswith("linux") and not args.skip_ldd:
        validate_linux_dependencies(exe)

    if not args.skip_python:
        validate_python_source_wrapper(exe, step_path)

    if not args.skip_ctest:
        run(["ctest", "--test-dir", str(build_dir), "-C", args.config, "--output-on-failure"])

    print(f"Native validation complete for {tag}")
    return 0


def validate_cli_outputs(exe: Path, step_path: Path, out_dir: Path) -> None:
    out_dir.mkdir(parents=True, exist_ok=True)

    projection_path = out_dir / "SOT-23.projection.json"
    svg_path = out_dir / "SOT-23.top.outline.svg"
    glb_path = out_dir / "SOT-23.glb"
    planar_request_path = out_dir / "planar-step-request.json"
    planar_step_path = out_dir / "planar.step"

    run([str(exe), "step-project-hlr", str(step_path), str(projection_path)])
    run(
        [
            str(exe),
            "step-project-svg",
            str(step_path),
            str(svg_path),
            "--mode",
            "outline",
            "--view",
            "top",
            "--curve-mode",
            "polyline",
        ]
    )
    run([str(exe), "step-to-glb", str(step_path), str(glb_path)])
    planar_request_path.write_text(
        json.dumps(
            {
                "schema": "geometry.planar_step.request.a0",
                "units": "mm",
                "name": "native_validation",
                "bodies": [
                    {
                        "id": "copper",
                        "thickness_mm": 0.035,
                        "regions": [
                            {
                                "outer": {
                                    "points": [[0, 0], [10, 0], [10, 5], [0, 5]],
                                    "segments": [{"kind": "line"}] * 4,
                                }
                            }
                        ],
                    }
                ],
            }
        ),
        encoding="utf-8",
    )
    run([str(exe), "planar-step", str(planar_request_path), str(planar_step_path)])

    projection = json.loads(projection_path.read_text(encoding="utf-8"))
    if projection.get("schema") != "geometry.projection.b0":
        raise RuntimeError(f"Unexpected projection schema in {projection_path}")
    if not svg_path.read_text(encoding="utf-8").lstrip().startswith("<svg"):
        raise RuntimeError(f"SVG output does not start with <svg: {svg_path}")
    if glb_path.read_bytes()[:4] != b"glTF":
        raise RuntimeError(f"GLB output does not start with glTF magic: {glb_path}")
    if not planar_step_path.read_bytes().startswith(b"ISO-10303-21;"):
        raise RuntimeError(f"Planar STEP output does not start with ISO-10303-21: {planar_step_path}")


def validate_linux_dependencies(exe: Path) -> None:
    if shutil.which("ldd") is None:
        print("Skipping ldd dependency check; ldd is not available.")
        return
    completed = subprocess.run(
        ["ldd", str(exe)],
        cwd=ROOT,
        capture_output=True,
        text=True,
        check=False,
    )
    print(completed.stdout, end="")
    if completed.returncode != 0:
        print(completed.stderr, end="", file=sys.stderr)
        raise RuntimeError(f"ldd failed for {exe}")
    if "libTK" in completed.stdout or "OpenCASCADE" in completed.stdout:
        raise RuntimeError("Linux executable depends on dynamic OCCT libraries; expected static OCCT linkage.")


def validate_python_source_wrapper(exe: Path, step_path: Path) -> None:
    env = os.environ.copy()
    env["GEOMETER_BACKEND"] = "exe"
    env["GEOMETER_EXE"] = str(exe)
    env["PYTHONPATH"] = prepend_path(ROOT / "python", env.get("PYTHONPATH"))
    planar_request_json = json.dumps(
        {
            "schema": "geometry.planar_step.request.a0",
            "units": "mm",
            "bodies": [
                {
                    "id": "copper",
                    "thickness_mm": 0.035,
                    "regions": [
                        {
                            "outer": {
                                "points": [[0, 0], [3, 0], [3, 2], [0, 2]],
                                "segments": [{"kind": "line"}] * 4,
                            },
                        }
                    ],
                }
            ],
        }
    )
    code = f"""
import json
from pathlib import Path
import geometer

step = Path({str(step_path)!r})
version = geometer.version()
projection = geometer.project_step_hlr(step, views=[geometer.ProjectionView.top()])
glb = geometer.step_to_glb(step)
planar_step = geometer.planar_step(json.loads({planar_request_json!r}))
print(f"python source wrapper {{version.string}} abi {{version.abi}}")
print(f"projection {{projection.schema}} views={{len(projection.views)}} glb_bytes={{len(glb)}} planar_step_bytes={{len(planar_step)}}")
"""
    run([sys.executable, "-c", code], env=env)


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


def executable_name() -> str:
    return "geometer.exe" if sys.platform == "win32" else "geometer"


def preview_executable_name() -> str:
    return "geometer_hlr_preview.exe" if sys.platform == "win32" else "geometer_hlr_preview"


def prepend_path(path: Path, current: str | None) -> str:
    if not current:
        return str(path)
    return str(path) + os.pathsep + current


def macos_deployment_target() -> str:
    return (
        os.environ.get("GEOMETER_MACOS_DEPLOYMENT_TARGET")
        or os.environ.get("MACOSX_DEPLOYMENT_TARGET")
        or DEFAULT_MACOS_DEPLOYMENT_TARGET
    ).replace("_", ".")


def native_build_env() -> dict[str, str]:
    env = os.environ.copy()
    if sys.platform == "win32" and shutil.which("cl") is None:
        env.update(visual_studio_build_env())
    if sys.platform == "darwin":
        target = macos_deployment_target()
        env["MACOSX_DEPLOYMENT_TARGET"] = target
        env["GEOMETER_MACOS_DEPLOYMENT_TARGET"] = target
    return env


def visual_studio_build_env() -> dict[str, str]:
    vsdev = find_vsdevcmd()
    if vsdev is None:
        return {}
    print(f"Activating Visual Studio build environment from {vsdev}")
    completed = subprocess.run(
        f'cmd /d /c call "{vsdev}" -arch=x64 -host_arch=x64 >nul && set',
        cwd=ROOT,
        capture_output=True,
        text=True,
        check=False,
    )
    if completed.returncode != 0:
        print(completed.stdout, end="")
        print(completed.stderr, end="", file=sys.stderr)
        raise RuntimeError(f"Could not activate Visual Studio build environment from {vsdev}")
    values: dict[str, str] = {}
    for line in completed.stdout.splitlines():
        if "=" not in line:
            continue
        name, value = line.split("=", 1)
        values[name] = value
    return values


def find_vsdevcmd() -> Path | None:
    vswhere = Path(os.environ.get("ProgramFiles(x86)", "")) / "Microsoft Visual Studio" / "Installer" / "vswhere.exe"
    if vswhere.exists():
        completed = subprocess.run(
            [
                str(vswhere),
                "-latest",
                "-products",
                "*",
                "-requires",
                "Microsoft.VisualStudio.Component.VC.Tools.x86.x64",
                "-property",
                "installationPath",
            ],
            cwd=ROOT,
            capture_output=True,
            text=True,
            check=False,
        )
        install_path = completed.stdout.strip()
        if install_path:
            candidate = Path(install_path) / "Common7" / "Tools" / "VsDevCmd.bat"
            if candidate.exists():
                return candidate
    for base in (
        Path(os.environ.get("ProgramFiles(x86)", "")) / "Microsoft Visual Studio" / "2022" / "BuildTools",
        Path(os.environ.get("ProgramFiles", "")) / "Microsoft Visual Studio" / "2022" / "BuildTools",
    ):
        candidate = base / "Common7" / "Tools" / "VsDevCmd.bat"
        if candidate.exists():
            return candidate
    return None


def cmake_configure_command(
    build_dir: Path,
    *,
    build_examples: bool,
    dist_root: Path,
    occt_dir: Path | None,
) -> list[str]:
    command = ["cmake", "--preset", "default", "-B", str(build_dir)]
    command.append(f"-DGEOMETER_BUILD_EXAMPLES={'ON' if build_examples else 'OFF'}")
    command.append(f"-DGEOMETER_DIST_ROOT={dist_root}")
    if occt_dir is not None:
        command.append(f"-DOpenCASCADE_DIR={occt_dir}")
    if sys.platform == "darwin":
        command.append(f"-DCMAKE_OSX_DEPLOYMENT_TARGET={macos_deployment_target()}")
    return command


def validate_macos_executable_target(exe: Path, target: str) -> None:
    min_version = macos_binary_min_version(exe)
    if min_version is None:
        raise RuntimeError(f"Could not determine macOS minimum OS version for {exe}")
    if version_pair(min_version) > version_pair(target):
        raise RuntimeError(
            f"{exe} requires macOS {min_version}, which is newer than the configured deployment target {target}."
        )
    print(f"macOS deployment target ok: binary minos {min_version}, configured target {target}")


def macos_binary_min_version(exe: Path) -> str | None:
    completed = subprocess.run(
        ["otool", "-l", str(exe)],
        cwd=ROOT,
        capture_output=True,
        text=True,
        check=True,
    )
    for key in ("minos", "version"):
        match = re.search(rf"^\s*{key}\s+(\d+(?:\.\d+)*)\b", completed.stdout, re.MULTILINE)
        if match is not None:
            return match.group(1)
    return None


def version_pair(value: str) -> tuple[int, int]:
    parts = value.replace("_", ".").split(".")
    if len(parts) == 1:
        parts.append("0")
    return int(parts[0]), int(parts[1])


def run(cmd: list[str], *, env: dict[str, str] | None = None) -> None:
    print("> " + " ".join(display_cmd(cmd)), flush=True)
    subprocess.run(cmd, cwd=ROOT, env=env, check=True)


def display_cmd(cmd: list[str]) -> list[str]:
    displayed = list(cmd)
    try:
        code_index = displayed.index("-c") + 1
    except ValueError:
        return displayed
    if code_index < len(displayed):
        displayed[code_index] = "<python-code>"
    return displayed


if __name__ == "__main__":
    raise SystemExit(main())
