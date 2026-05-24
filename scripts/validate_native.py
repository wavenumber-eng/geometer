from __future__ import annotations

import argparse
import json
import os
import platform
import shutil
import subprocess
import sys
from pathlib import Path


ROOT = Path(__file__).resolve().parent.parent
DEFAULT_STEP = ROOT / "tests" / "fixtures" / "step" / "embedded_models" / "SOT-23.STEP"


def main() -> int:
    tag = platform_tag()
    parser = argparse.ArgumentParser(description="Validate the Geometer native build for this platform.")
    parser.add_argument("--build-dir", type=Path, default=ROOT / f"build-native-{tag}")
    parser.add_argument("--config", default="Release")
    parser.add_argument("--step", type=Path, default=DEFAULT_STEP)
    parser.add_argument("--skip-ctest", action="store_true")
    parser.add_argument("--skip-python", action="store_true")
    parser.add_argument("--skip-ldd", action="store_true")
    args = parser.parse_args()

    step_path = args.step.resolve()
    if not step_path.exists():
        raise FileNotFoundError(step_path)

    build_dir = args.build_dir.resolve()
    run(["cmake", "--preset", "default", "-B", str(build_dir)])
    run(["cmake", "--build", str(build_dir), "--config", args.config])

    exe = ROOT / "dist" / "native" / tag / executable_name()
    if not exe.exists():
        raise FileNotFoundError(f"Expected native executable was not produced: {exe}")

    run([str(exe), "--version"])
    validate_cli_outputs(exe, step_path, tag)

    if sys.platform.startswith("linux") and not args.skip_ldd:
        validate_linux_dependencies(exe)

    if not args.skip_python:
        validate_python_source_wrapper(exe, step_path)

    if not args.skip_ctest:
        run(["ctest", "--test-dir", str(build_dir), "-C", args.config, "--output-on-failure"])

    print(f"Native validation complete for {tag}")
    return 0


def validate_cli_outputs(exe: Path, step_path: Path, tag: str) -> None:
    out_dir = ROOT / "out" / "native-validation" / tag
    out_dir.mkdir(parents=True, exist_ok=True)

    projection_path = out_dir / "SOT-23.projection.json"
    svg_path = out_dir / "SOT-23.top.simple.svg"
    glb_path = out_dir / "SOT-23.glb"
    planar_request_path = out_dir / "planar-step-request.json"
    planar_step_path = out_dir / "planar.step"

    run([str(exe), "step-project-hlr", str(step_path), str(projection_path)])
    run([
        str(exe),
        "step-project-svg",
        str(step_path),
        str(svg_path),
        "--mode",
        "simple",
        "--view",
        "top",
        "--curve-mode",
        "polyline",
    ])
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
    if projection.get("schema") != "geometry.projection.a0":
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


def prepend_path(path: Path, current: str | None) -> str:
    if not current:
        return str(path)
    return str(path) + os.pathsep + current


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
