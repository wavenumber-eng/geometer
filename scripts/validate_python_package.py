from __future__ import annotations

import argparse
import os
import platform
import re
import subprocess
import sys
import tempfile
import time
from pathlib import Path


ROOT = Path(__file__).resolve().parent.parent
DEFAULT_STEP = ROOT / "tests" / "fixtures" / "step" / "embedded_models" / "SOT-23.STEP"
DEFAULT_MACOS_DEPLOYMENT_TARGET = "11.0"


PACKAGE_VALIDATION_CODE = r"""
import json
import os
import hashlib
import re
from pathlib import Path

import geometer
from geometer._generated.contracts.codecs import decode_model_bounds_options_a0_json

step = Path(os.environ["GEOMETER_VALIDATION_STEP"])
out_dir = Path(os.environ["GEOMETER_VALIDATION_OUT"])
out_dir.mkdir(parents=True, exist_ok=True)

package_dir = Path(geometer.__file__).resolve().parent
exe = geometer.executable_path().resolve()
if package_dir not in exe.parents:
    raise RuntimeError(f"expected bundled package executable under {package_dir}, got {exe}")
attestation_path = exe.with_name("geometer.build-attestation.json")
if not attestation_path.is_file():
    raise RuntimeError("bundled native executable is missing its build attestation")
attestation_bytes = attestation_path.read_bytes()
attestation = json.loads(attestation_bytes)
canonical_attestation = (json.dumps(attestation, indent=2, sort_keys=True, ensure_ascii=True) + "\n").encode("utf-8")
if attestation_bytes != canonical_attestation:
    raise RuntimeError("bundled native build attestation is not canonical deterministic JSON")
if set(attestation) != {"artifact", "build", "producer", "schema"}:
    raise RuntimeError("bundled native build attestation has unexpected root fields")
if attestation.get("schema") != "wn.geometer.native_build_attestation.a1":
    raise RuntimeError("bundled native build attestation has an unexpected schema")
artifact = attestation.get("artifact", {})
build = attestation.get("build", {})
producer = attestation.get("producer", {})
if set(artifact) != {"name", "sha256"}:
    raise RuntimeError("bundled native build attestation has unexpected artifact fields")
if set(build) != {"arch", "build_type", "c_abi_version", "compiler", "generator", "geometer_version", "occt", "platform", "source"}:
    raise RuntimeError("bundled native build attestation has unexpected build fields")
if set(producer) != {"identity", "source", "source_sha256"}:
    raise RuntimeError("bundled native build attestation has unexpected producer fields")
if producer.get("identity") != "wn.geometer.native_build_attestation_generator.a1":
    raise RuntimeError("bundled native build attestation has an unexpected producer")
if producer.get("source") != "scripts/native_build_attestation.py":
    raise RuntimeError("bundled native build attestation has an unexpected producer source")
if re.fullmatch(r"[0-9a-f]{64}", str(producer.get("source_sha256"))) is None:
    raise RuntimeError("bundled native build attestation has an invalid producer source digest")
compiler = build.get("compiler", {})
occt = build.get("occt", {})
source = build.get("source", {})
if set(compiler) != {"authority", "id", "version"} or compiler.get("authority") != "cmake_compiler_id_and_version":
    raise RuntimeError("bundled native build attestation has invalid compiler provenance")
if set(occt) != {"authority", "profile_sha256", "repo", "tag", "version"}:
    raise RuntimeError("bundled native build attestation has invalid OCCT provenance")
if occt.get("authority") not in {"geometer_occt_profile_verified", "occt_profile_unverified"}:
    raise RuntimeError("bundled native build attestation has unsupported OCCT authority")
if set(source) != {"authority", "revision", "tree_state"}:
    raise RuntimeError("bundled native build attestation has invalid source provenance")
if artifact.get("name") != exe.name:
    raise RuntimeError("bundled native build attestation names a different executable")
if artifact.get("sha256") != hashlib.sha256(exe.read_bytes()).hexdigest():
    raise RuntimeError("bundled native build attestation SHA-256 does not match the executable")

version = geometer.version()
if build.get("geometer_version") != version.string or build.get("c_abi_version") != version.abi:
    raise RuntimeError("bundled native build attestation version/C ABI does not match the executable")
generated_options = decode_model_bounds_options_a0_json(b'{"format":"step"}')
if generated_options.format.value != "step":
    raise RuntimeError("generated contract internals were not installed correctly")

empty_analytic_request = geometer.AnalyticPlanarBooleanBatchRequestA0(
    jobs=(),
    relationship_queries=(),
)
disk_analytic_request = geometer.AnalyticPlanarBooleanBatchRequestA0(
    jobs=(
        geometer.AnalyticPlanarBooleanJob(
            job_id=1,
            stages=(
                geometer.AnalyticPlanarBooleanStage(
                    stage_id=1,
                    operation=geometer.StageOperation.UNION_STAGE,
                    operands=(
                        geometer.DiskOperand(
                            operand_id=1,
                            kind="disk",
                            feature_id=1,
                            center=geometer.AnalyticPointNm(x=0, y=0),
                            radius_nm=1_000_000,
                        ),
                    ),
                ),
            ),
        ),
    ),
    relationship_queries=(),
)
with geometer.GeometerClient(exe, client_name="python-wheel-validation") as analytic_client:
    empty_analytic_result = analytic_client.analytic_planar_boolean_batch(
        empty_analytic_request,
        timeout=10,
    )
    disk_analytic_result = analytic_client.analytic_planar_boolean_batch(
        disk_analytic_request,
        timeout=10,
    )
if empty_analytic_result.job_results or empty_analytic_result.relationship_results:
    raise RuntimeError("empty analytic batch did not return an empty result")
if len(disk_analytic_result.job_results) != 1:
    raise RuntimeError("nontrivial analytic batch did not return one job result")
disk_job = disk_analytic_result.job_results[0]
if disk_job.status != "success" or not disk_job.result_regions or len(disk_job.digest_sha256) != 64:
    raise RuntimeError("nontrivial analytic batch did not return canonical disk geometry")

bounds = geometer.model_bounds(step)
if bounds.schema != "geometry.model_bounds.a0" or not bounds.source_hash:
    raise RuntimeError("generated model-bounds boundary did not return a validated result")
projection = geometer.project_step_hlr(
    step,
    views=[geometer.ProjectionView.top()],
    options=geometer.HlrOptions.assembly_outline(),
)
detail = projection.geometry("top", "detail")
outline = projection.geometry("top", "outline")
detail_count = len(detail.get("segments", [])) + len(detail.get("arcs", []))
outline_count = len(outline.get("segments", [])) + len(outline.get("arcs", []))
if projection.schema != "geometry.projection.b0" or detail_count <= 0 or outline_count <= 0:
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

svg_path = out_dir / "SOT-23.package.top.outline.svg"
response = geometer.run_batch(
    [
        {
            "id": "svg",
            "operation": "step_hlr_projection_svg",
            "step_path": str(step),
            "output_path": str(svg_path),
            "mode": "outline",
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
    "outline_edges": outline_count,
    "model_bounds_hash": bounds.source_hash,
    "analytic_disk_regions": len(disk_job.result_regions),
    "analytic_disk_digest": disk_job.digest_sha256,
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
        run(
            [
                sys.executable,
                str(ROOT / "scripts" / "validate_native.py"),
                "--build-dir",
                str(args.build_dir),
                "--skip-ctest",
            ],
            env=package_build_env(),
        )
    elif sys.platform == "darwin":
        validate_macos_dist_executable(macos_deployment_target())

    wheel = args.wheel.resolve() if args.wheel is not None else build_wheel(args.wheelhouse.resolve())
    validate_wheel_platform_tag(wheel)
    validate_wheel_install(wheel, step_path, keep_temp=args.keep_temp)
    print(f"Python package validation complete for {platform_tag()}: {wheel}")
    return 0


def build_wheel(wheelhouse: Path) -> Path:
    wheelhouse.mkdir(parents=True, exist_ok=True)
    env = package_build_env()
    started_at = time.time()
    with tempfile.TemporaryDirectory(prefix="geometer-wheel-build-") as temp_text:
        build_venv = Path(temp_text) / "venv"
        run([sys.executable, "-m", "venv", str(build_venv)], env=env)
        build_python = venv_python(build_venv)
        run([str(build_python), "-m", "pip", "install", "--upgrade", "pip", "build"], env=env)
        run([str(build_python), "-m", "build", "--wheel", "--outdir", str(wheelhouse)], env=env)

    candidates = [path for path in wheelhouse.glob("wn_geometer-*.whl") if path.stat().st_mtime >= started_at - 1.0]
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
        run(
            [
                str(test_python),
                "-I",
                str(ROOT / "scripts" / "validate_illustration_package.py"),
                "--step",
                str(step_path),
                "--out-dir",
                str(output_dir),
            ],
            env=env,
            cwd=run_dir,
        )
        run([str(venv_script(venv_dir, "geometer")), "--version"], env=env, cwd=run_dir)

        run(
            [
                str(test_python),
                str(ROOT / "examples" / "python" / "step_hlr_svg.py"),
                str(step_path),
                "--out-dir",
                str(example_output_dir),
            ],
            env=env,
            cwd=run_dir,
        )

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


def package_build_env() -> dict[str, str]:
    env = os.environ.copy()
    if sys.platform == "darwin":
        target = macos_deployment_target()
        env["MACOSX_DEPLOYMENT_TARGET"] = target
        env["GEOMETER_MACOS_DEPLOYMENT_TARGET"] = target
    return env


def validate_wheel_platform_tag(wheel: Path) -> None:
    if sys.platform == "darwin":
        expected = expected_macos_wheel_platform_tag()
    elif sys.platform.startswith("linux"):
        expected = expected_linux_wheel_platform_tag()
    else:
        return
    if expected not in wheel.name:
        raise RuntimeError(f"Expected wheel tag {expected} in {wheel.name}")


def validate_macos_dist_executable(target: str) -> None:
    exe = ROOT / "dist" / "native" / platform_tag() / executable_name()
    if not exe.exists():
        raise FileNotFoundError(f"Expected native executable was not produced: {exe}")
    min_version = macos_binary_min_version(exe)
    if min_version is None:
        raise RuntimeError(f"Could not determine macOS minimum OS version for {exe}")
    if version_pair(min_version) > version_pair(target):
        raise RuntimeError(
            f"{exe} requires macOS {min_version}, which is newer than the configured deployment target {target}."
        )
    print(f"macOS deployment target ok: binary minos {min_version}, configured target {target}")


def macos_deployment_target() -> str:
    return (
        os.environ.get("GEOMETER_MACOS_DEPLOYMENT_TARGET")
        or os.environ.get("MACOSX_DEPLOYMENT_TARGET")
        or DEFAULT_MACOS_DEPLOYMENT_TARGET
    ).replace("_", ".")


def expected_macos_wheel_platform_tag() -> str:
    major, minor = version_pair(macos_deployment_target())
    arch = macos_wheel_arch()
    if arch == "arm64" and (major, minor) < (11, 0):
        raise RuntimeError("macOS arm64 wheels require deployment target 11.0 or newer.")
    return f"macosx_{major}_{0 if major >= 11 else minor}_{arch}"


def expected_linux_wheel_platform_tag() -> str:
    libc_name, libc_version = platform.libc_ver()
    if libc_name != "glibc" or not libc_version:
        raise RuntimeError(
            f"Linux wheels require glibc for PyPI publishing, got {libc_name or 'unknown'} {libc_version}"
        )
    major, minor = version_pair(libc_version)
    return f"manylinux_{major}_{minor}_{linux_wheel_arch()}"


def macos_wheel_arch() -> str:
    machine = platform.machine().strip().lower()
    if machine in {"aarch64", "arm64"}:
        return "arm64"
    if machine in {"amd64", "x86_64"}:
        return "x86_64"
    return machine or "unknown"


def linux_wheel_arch() -> str:
    machine = platform.machine().strip().lower()
    if machine in {"amd64", "x86_64"}:
        return "x86_64"
    if machine in {"aarch64", "arm64"}:
        return "aarch64"
    if machine in {"i386", "i686", "x86"}:
        return "i686"
    return machine or "unknown"


def executable_name() -> str:
    return "geometer.exe" if sys.platform == "win32" else "geometer"


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
