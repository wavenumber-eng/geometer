from __future__ import annotations

import argparse
import os
import platform
import subprocess
import sys
import tempfile
import time
from pathlib import Path


ROOT = Path(__file__).resolve().parent.parent
DEFAULT_STEP = ROOT / "tests" / "fixtures" / "step" / "embedded_models" / "SOT-23.STEP"


PACKAGE_VALIDATION_CODE = r"""
import json
import os
from pathlib import Path

import geometer

step = Path(os.environ["GEOMETER_VALIDATION_STEP"])
out_dir = Path(os.environ["GEOMETER_VALIDATION_OUT"])
out_dir.mkdir(parents=True, exist_ok=True)

package_dir = Path(geometer.__file__).resolve().parent
exe = geometer.executable_path().resolve()
if package_dir not in exe.parents:
    raise RuntimeError(f"expected bundled package executable under {package_dir}, got {exe}")

version = geometer.version()
projection = geometer.project_step_hlr(
    step,
    views=[geometer.ProjectionView.top()],
    options=geometer.HlrOptions.assembly_outline(),
)
detail = projection.geometry("top", "detail")
simple = projection.geometry("top", "simple")
detail_count = len(detail.get("segments", [])) + len(detail.get("arcs", []))
simple_count = len(simple.get("segments", [])) + len(simple.get("arcs", []))
if projection.schema != "geometry.projection.a0" or detail_count <= 0 or simple_count <= 0:
    raise RuntimeError("projection did not produce expected top-view geometry")

glb = geometer.step_to_glb(step)
if glb[:4] != b"glTF":
    raise RuntimeError("step_to_glb did not produce GLB bytes")

planar_step = geometer.planar_step({
    "schema": "geometry.planar_step.request.a0",
    "units": "mm",
    "bodies": [{
        "id": "copper",
        "thickness_mm": 0.035,
        "regions": [{
            "outer": {
                "points": [[0, 0], [3, 0], [3, 2], [0, 2]],
                "segments": [{"kind": "line"}] * 4,
            },
        }],
    }],
})
if not planar_step.startswith(b"ISO-10303-21;"):
    raise RuntimeError("planar_step did not produce STEP bytes")

svg_path = out_dir / "SOT-23.package.top.simple.svg"
response = geometer.run_batch(
    [
        {
            "id": "svg",
            "operation": "step_hlr_projection_svg",
            "step_path": str(step),
            "output_path": str(svg_path),
            "mode": "simple",
            "view": "top",
        }
    ],
    options={
        "curve_mode": "polyline",
        "views": [geometer.ProjectionView.top().to_json_value()],
    },
    work_dir=out_dir,
)
if not response.get("ok"):
    raise RuntimeError(f"run_batch failed: {response!r}")
if not svg_path.read_text(encoding="utf-8").lstrip().startswith("<svg"):
    raise RuntimeError("run_batch did not write SVG output")

print(json.dumps({
    "version": version.string,
    "abi": version.abi,
    "package_dir": str(package_dir),
    "executable": str(exe),
    "detail_edges": detail_count,
    "simple_edges": simple_count,
    "glb_bytes": len(glb),
    "planar_step_bytes": len(planar_step),
    "svg": str(svg_path),
}, indent=2))
"""


def main() -> int:
    tag = platform_tag()
    parser = argparse.ArgumentParser(description="Validate an installed Geometer Python wheel.")
    parser.add_argument("--build-dir", type=Path, default=ROOT / f"build-native-{tag}")
    parser.add_argument("--wheelhouse", type=Path, default=ROOT / "out" / "wheelhouse" / tag)
    parser.add_argument("--wheel", type=Path, default=None)
    parser.add_argument("--step", type=Path, default=DEFAULT_STEP)
    parser.add_argument("--skip-native-validation", action="store_true")
    parser.add_argument("--keep-temp", action="store_true")
    args = parser.parse_args()

    step_path = args.step.resolve()
    if not step_path.exists():
        raise FileNotFoundError(step_path)

    if not args.skip_native_validation:
        run([
            sys.executable,
            str(ROOT / "scripts" / "validate_native.py"),
            "--build-dir",
            str(args.build_dir),
            "--skip-ctest",
        ])

    wheel = args.wheel.resolve() if args.wheel is not None else build_wheel(args.wheelhouse.resolve())
    validate_wheel_install(wheel, step_path, keep_temp=args.keep_temp)
    print(f"Python package validation complete for {platform_tag()}: {wheel}")
    return 0


def build_wheel(wheelhouse: Path) -> Path:
    wheelhouse.mkdir(parents=True, exist_ok=True)
    started_at = time.time()
    with tempfile.TemporaryDirectory(prefix="geometer-wheel-build-") as temp_text:
        build_venv = Path(temp_text) / "venv"
        run([sys.executable, "-m", "venv", str(build_venv)])
        build_python = venv_python(build_venv)
        run([str(build_python), "-m", "pip", "install", "--upgrade", "pip", "build"])
        run([str(build_python), "-m", "build", "--wheel", "--outdir", str(wheelhouse)])

    candidates = [
        path
        for path in wheelhouse.glob("wn_geometer-*.whl")
        if path.stat().st_mtime >= started_at - 1.0
    ]
    if not candidates:
        raise FileNotFoundError(f"No wheel was built under {wheelhouse}")
    return max(candidates, key=lambda path: path.stat().st_mtime)


def validate_wheel_install(wheel: Path, step_path: Path, *, keep_temp: bool) -> None:
    temp_dir_context: tempfile.TemporaryDirectory[str] | None = None
    if keep_temp:
        temp_dir = Path(tempfile.mkdtemp(prefix="geometer-package-validation-"))
    else:
        temp_dir_context = tempfile.TemporaryDirectory(prefix="geometer-package-validation-")
        temp_dir = Path(temp_dir_context.name)
    try:
        venv_dir = temp_dir / "venv"
        output_dir = temp_dir / "outputs"
        example_output_dir = temp_dir / "example-outputs"
        run_dir = temp_dir / "run"
        run_dir.mkdir(parents=True, exist_ok=True)

        run([sys.executable, "-m", "venv", str(venv_dir)])
        test_python = venv_python(venv_dir)
        run([str(test_python), "-m", "pip", "install", "--upgrade", "pip"])
        run([str(test_python), "-m", "pip", "install", "--force-reinstall", str(wheel)])

        env = clean_package_env()
        env["GEOMETER_VALIDATION_STEP"] = str(step_path)
        env["GEOMETER_VALIDATION_OUT"] = str(output_dir)
        run([str(test_python), "-c", PACKAGE_VALIDATION_CODE], env=env, cwd=run_dir)
        run([str(venv_script(venv_dir, "geometer")), "--version"], env=env, cwd=run_dir)

        run([
            str(test_python),
            str(ROOT / "examples" / "python" / "step_hlr_svg.py"),
            str(step_path),
            "--out-dir",
            str(example_output_dir),
        ], env=env, cwd=run_dir)

        if keep_temp:
            print(f"Kept package validation temp directory: {temp_dir}")
    finally:
        if not keep_temp and temp_dir_context is not None:
            temp_dir_context.cleanup()


def clean_package_env() -> dict[str, str]:
    env = os.environ.copy()
    for key in ("PYTHONPATH", "GEOMETER_EXE", "GEOMETER_EXE_DIR"):
        env.pop(key, None)
    env["GEOMETER_BACKEND"] = "exe"
    return env


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


def venv_python(venv_dir: Path) -> Path:
    if sys.platform == "win32":
        return venv_dir / "Scripts" / "python.exe"
    return venv_dir / "bin" / "python"


def venv_script(venv_dir: Path, name: str) -> Path:
    if sys.platform == "win32":
        return venv_dir / "Scripts" / f"{name}.exe"
    return venv_dir / "bin" / name


def run(
    cmd: list[str],
    *,
    env: dict[str, str] | None = None,
    cwd: Path = ROOT,
) -> None:
    print("> " + " ".join(display_cmd(cmd)), flush=True)
    subprocess.run(cmd, cwd=cwd, env=env, check=True)


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
