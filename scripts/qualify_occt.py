"""Build and compare exact OCCT release tags in isolated generated state."""

from __future__ import annotations

import argparse
import hashlib
import json
import os
import platform
import shutil
import subprocess
import sys
import time
from dataclasses import asdict, dataclass
from pathlib import Path
from typing import Any


ROOT = Path(__file__).resolve().parent.parent
ALLOWED_TAGS = ("V8_0_0", "V8_0_1")
PARITY_TARGETS = (
    ("validate_analytic_closed_form_invariants_parity.py", "analytic_closed_form_invariants"),
    ("validate_analytic_mutation_sentinels_parity.py", "analytic_result_packet_topology"),
    ("validate_analytic_result_packet_layout_parity.py", "analytic_result_packet_layout"),
    ("validate_analytic_result_packet_records_parity.py", "analytic_result_packet_records"),
    ("validate_exact_arrangement_parity.py", "exact_arrangement"),
    ("validate_exact_boolean_identities_parity.py", "exact_boolean_identities"),
    ("validate_exact_boolean_lineage_matrix_parity.py", "exact_boolean_lineage_matrix"),
    ("validate_exact_boolean_metamorphic_parity.py", "exact_boolean_metamorphic"),
    ("validate_exact_boolean_outcomes_parity.py", "exact_boolean_outcomes"),
    ("validate_exact_boolean_provenance_parity.py", "exact_boolean_provenance"),
    ("validate_exact_boolean_regions_parity.py", "exact_boolean_regions"),
    ("validate_exact_boolean_seeded_property_parity.py", "exact_boolean_seeded_property"),
    ("validate_exact_boolean_stages_parity.py", "exact_boolean_stages"),
    ("validate_exact_curve_domain_parity.py", "exact_curve_domain"),
    ("validate_exact_degeneracy_sweep_parity.py", "exact_degeneracy_sweep"),
    ("validate_exact_geometry_parity.py", "exact_geometry"),
    ("validate_exact_rectangle_enumeration_parity.py", "exact_rectangle_enumeration"),
    ("validate_exact_result_normalization_parity.py", "exact_result_normalization"),
    ("validate_exact_source_sets_parity.py", "exact_source_sets"),
)


@dataclass(frozen=True)
class CommandEvidence:
    name: str
    command: list[str]
    elapsed_seconds: float
    stdout_sha256: str
    log: str


class QualificationRun:
    def __init__(self, output_root: Path, timeout_seconds: int, lane: str) -> None:
        self.output_root = output_root
        self.log_root = output_root / "logs" / lane
        self.log_root.mkdir(parents=True, exist_ok=True)
        self.timeout_seconds = timeout_seconds
        self.commands: list[CommandEvidence] = []

    def run(
        self,
        name: str,
        command: list[str | Path],
        *,
        env: dict[str, str] | None = None,
    ) -> str:
        normalized = [str(item) for item in command]
        print(f"[{name}] {' '.join(normalized)}", flush=True)
        started = time.perf_counter()
        completed = subprocess.run(
            normalized,
            cwd=ROOT,
            env=env,
            text=True,
            encoding="utf-8",
            errors="replace",
            stdout=subprocess.PIPE,
            stderr=subprocess.STDOUT,
            timeout=self.timeout_seconds,
            check=False,
        )
        elapsed = time.perf_counter() - started
        output = completed.stdout
        log_path = self.log_root / f"{len(self.commands) + 1:02d}-{name}.log"
        log_path.write_text(output, encoding="utf-8", newline="\n")
        self.commands.append(
            CommandEvidence(
                name=name,
                command=normalized,
                elapsed_seconds=round(elapsed, 6),
                stdout_sha256=hashlib.sha256(output.encode("utf-8")).hexdigest(),
                log=log_path.relative_to(self.output_root).as_posix(),
            )
        )
        if completed.returncode != 0:
            tail = "\n".join(output.splitlines()[-40:])
            raise RuntimeError(
                f"{name} failed with exit code {completed.returncode}; log={log_path}\n{tail}"
            )
        print(f"[{name}] passed in {elapsed:.2f}s", flush=True)
        return output


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
    aliases = {
        "amd64": "x64",
        "x86_64": "x64",
        "aarch64": "arm64",
        "arm64": "arm64",
        "i386": "x86",
        "i686": "x86",
        "x86": "x86",
    }
    return f"{os_name}-{aliases.get(machine, machine or 'unknown')}"


def native_environment() -> dict[str, str]:
    env = os.environ.copy()
    if sys.platform != "win32" or shutil.which("cl", path=env.get("PATH")):
        return env
    vswhere = Path(os.environ["ProgramFiles(x86)"]) / "Microsoft Visual Studio/Installer/vswhere.exe"
    installation = subprocess.check_output(
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
        text=True,
    ).strip()
    developer_shell = Path(installation) / "Common7/Tools/VsDevCmd.bat"
    output = subprocess.check_output(
        f'call "{developer_shell}" -arch=amd64 -host_arch=amd64 >nul && set',
        text=True,
        shell=True,
    )
    for line in output.splitlines():
        if "=" in line:
            name, value = line.split("=", 1)
            if name.lower() == "path":
                name = "PATH"
            env[name] = value
    return env


def emscripten_environment() -> dict[str, str]:
    env = os.environ.copy()
    emsdk = ROOT / ".deps/emsdk"
    upstream = emsdk / "upstream/emscripten"
    node_dir = next((emsdk / "node").glob("*/bin"), None)
    paths = [str(upstream)]
    if node_dir is not None:
        paths.append(str(node_dir))
    paths.append(str(emsdk))
    env["PATH"] = os.pathsep.join(paths) + os.pathsep + env.get("PATH", "")
    env["EMSDK"] = str(emsdk)
    return env


def cmake_config_dir(install_root: Path) -> Path:
    for candidate in (
        install_root / "lib/cmake/opencascade",
        install_root / "cmake",
    ):
        if (candidate / "OpenCASCADEConfig.cmake").is_file():
            return candidate
    raise FileNotFoundError(f"OpenCASCADEConfig.cmake not found under {install_root}")


def executable(build_root: Path, target: str, *, wasm: bool) -> Path:
    suffix = ".cjs" if wasm else (".exe" if sys.platform == "win32" else "")
    return build_root / "tests/cpp" / f"geometer_{target}_test{suffix}"


def tree_size(root: Path) -> int:
    return sum(path.stat().st_size for path in root.rglob("*") if path.is_file())


def file_evidence(path: Path) -> dict[str, Any]:
    data = path.read_bytes()
    return {
        "path": path.relative_to(ROOT).as_posix(),
        "bytes": len(data),
        "sha256": hashlib.sha256(data).hexdigest(),
    }


def tree_digest(root: Path) -> str:
    digest = hashlib.sha256()
    for path in sorted(item for item in root.rglob("*") if item.is_file()):
        digest.update(path.relative_to(root).as_posix().encode("utf-8"))
        digest.update(b"\0")
        digest.update(hashlib.sha256(path.read_bytes()).digest())
    return digest.hexdigest()


def last_json_object(output: str) -> dict[str, Any]:
    for line in reversed(output.splitlines()):
        try:
            value = json.loads(line)
        except json.JSONDecodeError:
            continue
        if isinstance(value, dict):
            return value
    raise RuntimeError("command output did not contain a JSON object")


def prepare_native(
    run: QualificationRun,
    tag: str,
    state_root: Path,
    output_root: Path,
    binary_cache: str,
    prepare_only: bool,
    validate_consumers: bool,
) -> dict[str, Any]:
    native_env = native_environment()
    target_platform = platform_tag()
    run.run(
        "prepare-native-occt",
        [
            sys.executable,
            ROOT / "scripts/build_occt.py",
            "--occt-tag",
            tag,
            "--occt-state-root",
            state_root,
            "--platform-tag",
            target_platform,
            "--binary-cache",
            binary_cache,
        ],
        env=native_env,
    )
    install_root = state_root / "native" / target_platform / "occt-install"
    evidence: dict[str, Any] = {
        "install_root": install_root.relative_to(ROOT).as_posix(),
        "install_bytes": tree_size(install_root),
    }
    if prepare_only:
        return evidence
    build_root = output_root / "build-native"
    dist_root = output_root / "dist-native"
    run.run(
        "configure-native-geometer",
        [
            "cmake",
            "-G",
            "Ninja",
            "-S",
            ROOT,
            "-B",
            build_root,
            "-DCMAKE_BUILD_TYPE=Release",
            "-DCMAKE_EXPORT_COMPILE_COMMANDS=ON",
            "-DGEOMETER_OCCT_LIBRARY_TYPE=Static",
            f"-DOpenCASCADE_DIR={cmake_config_dir(install_root)}",
            f"-DGEOMETER_DIST_ROOT={dist_root}",
        ],
        env=native_env,
    )
    run.run(
        "build-native-geometer",
        ["cmake", "--build", build_root, "--config", "Release", "--parallel"],
        env=native_env,
    )
    run.run(
        "ctest-native",
        ["ctest", "--test-dir", build_root, "-C", "Release", "--output-on-failure", "--timeout", "120"],
        env=native_env,
    )
    feasibility = run.run(
        "feasibility-native",
        [executable(build_root, "analytic_planar_boolean_feasibility", wasm=False)],
        env=native_env,
    )
    evidence.update(
        {
            "build_root": build_root.relative_to(ROOT).as_posix(),
            "feasibility_sha256": hashlib.sha256(feasibility.encode("utf-8")).hexdigest(),
        }
    )
    if validate_consumers:
        validation_out = output_root / "native-validation"
        run.run(
            "validate-native-consumers",
            [
                sys.executable,
                ROOT / "scripts/validate_native.py",
                "--build-dir",
                build_root,
                "--dist-root",
                dist_root,
                "--occt-dir",
                cmake_config_dir(install_root),
                "--validation-out",
                validation_out,
                "--skip-ctest",
            ],
            env=native_env,
        )
        native_executable = dist_root / "native" / target_platform / (
            "geometer.exe" if sys.platform == "win32" else "geometer"
        )
        evidence["consumer_validation"] = {
            "cli_python_glb_planar": "passed",
            "executable": file_evidence(native_executable),
            "output_bytes": tree_size(validation_out),
            "output_sha256": tree_digest(validation_out),
        }
    return evidence


def prepare_wasm(
    run: QualificationRun,
    tag: str,
    state_root: Path,
    output_root: Path,
    binary_cache: str,
    prepare_only: bool,
    validate_consumers: bool,
) -> dict[str, Any]:
    run.run(
        "prepare-wasm-occt",
        [
            sys.executable,
            ROOT / "scripts/build_wasm.py",
            "--occt-tag",
            tag,
            "--occt-state-root",
            state_root,
            "--occt-binary-cache",
            binary_cache,
            "--occt-only",
        ],
    )
    install_root = state_root / "occt-wasm-install"
    evidence: dict[str, Any] = {
        "install_root": install_root.relative_to(ROOT).as_posix(),
        "install_bytes": tree_size(install_root),
    }
    if prepare_only:
        return evidence
    build_root = output_root / "build-wasm"
    dist_root = output_root / "dist-wasm"
    emsdk = ROOT / ".deps/emsdk"
    toolchain = emsdk / "upstream/emscripten/cmake/Modules/Platform/Emscripten.cmake"
    wasm_env = emscripten_environment()
    run.run(
        "configure-wasm-geometer",
        [
            "cmake",
            "-G",
            "Ninja",
            "-S",
            ROOT,
            "-B",
            build_root,
            f"-DCMAKE_TOOLCHAIN_FILE={toolchain}",
            "-DCMAKE_BUILD_TYPE=Release",
            "-DCMAKE_EXPORT_COMPILE_COMMANDS=ON",
            f"-DOpenCASCADE_DIR={cmake_config_dir(install_root)}",
            f"-DGEOMETER_DIST_ROOT={dist_root}",
        ],
        env=wasm_env,
    )
    run.run(
        "build-wasm-geometer",
        ["cmake", "--build", build_root, "--config", "Release", "--parallel"],
        env=wasm_env,
    )
    run.run(
        "ctest-wasm",
        ["ctest", "--test-dir", build_root, "-C", "Release", "--output-on-failure", "--timeout", "120"],
        env=wasm_env,
    )
    feasibility = run.run(
        "feasibility-wasm",
        ["node", executable(build_root, "analytic_planar_boolean_feasibility", wasm=True)],
        env=wasm_env,
    )
    evidence.update(
        {
            "build_root": build_root.relative_to(ROOT).as_posix(),
            "feasibility_sha256": hashlib.sha256(feasibility.encode("utf-8")).hexdigest(),
        }
    )
    if validate_consumers:
        browser_dist = dist_root / "wasm/browser"
        consumer_env = wasm_env.copy()
        consumer_env["GEOMETER_WASM_BROWSER_DIST"] = str(browser_dist)
        run.run(
            "validate-browser-wasm-consumers",
            [sys.executable, "-m", "pytest", "tests/wasm", "-q"],
            env=consumer_env,
        )
        run.run(
            "validate-typescript-package-worker",
            [sys.executable, "-m", "pytest", "tests/typescript", "-q"],
            env=consumer_env,
        )
        full_browser_output = run.run(
            "validate-full-browser-hlr",
            [
                sys.executable,
                ROOT / "scripts/validate_browser_wasm.py",
                "--browser-dist",
                browser_dist,
            ],
            env=consumer_env,
        )
        runtime_output = run.run(
            "measure-wasm-step-glb-memory",
            ["node", ROOT / "tests/wasm/step_to_glb_bytes_validation.js"],
            env=consumer_env,
        )
        evidence["consumer_validation"] = {
            "browser_wasm": "passed",
            "typescript_package_worker": "passed",
            "full_browser_hlr": last_json_object(full_browser_output),
            "browser_js": file_evidence(browser_dist / "geometer.js"),
            "browser_wasm_artifact": file_evidence(browser_dist / "geometer.wasm"),
            "step_glb_runtime_memory": last_json_object(runtime_output),
        }
    return evidence


def run_parity(
    run: QualificationRun,
    native_root: Path,
    wasm_root: Path,
) -> None:
    for script, target in PARITY_TARGETS:
        run.run(
            f"parity-{target}",
            [
                sys.executable,
                ROOT / "scripts" / script,
                "--native",
                executable(native_root, target, wasm=False),
                "--wasm",
                executable(wasm_root, target, wasm=True),
            ],
        )


def git_output(*arguments: str, cwd: Path = ROOT) -> str:
    return subprocess.check_output(["git", "-C", str(cwd), *arguments], text=True).strip()


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--tag", choices=ALLOWED_TAGS, required=True)
    parser.add_argument("--lane", choices=("native", "wasm", "all"), default="all")
    parser.add_argument(
        "--binary-cache",
        choices=("auto", "off", "only"),
        default="auto",
    )
    parser.add_argument("--prepare-only", action="store_true")
    parser.add_argument("--skip-parity", action="store_true")
    parser.add_argument("--skip-consumers", action="store_true")
    parser.add_argument("--timeout-seconds", type=int, default=7200)
    args = parser.parse_args()

    state_root = ROOT / ".deps/occt-qualification" / args.tag
    output_root = ROOT / "out/occt-qualification" / args.tag
    output_root.mkdir(parents=True, exist_ok=True)
    run = QualificationRun(output_root, args.timeout_seconds, args.lane)
    evidence: dict[str, Any] = {
        "schema": "geometer.occt_qualification.a0",
        "tag": args.tag,
        "geometer_revision": git_output("rev-parse", "HEAD"),
        "production_pin_unchanged": True,
        "production_solver_allowed": False,
        "platform": platform_tag(),
        "lanes": {},
    }
    try:
        if args.lane in {"native", "all"}:
            evidence["lanes"]["native"] = prepare_native(
                run,
                args.tag,
                state_root,
                output_root,
                args.binary_cache,
                args.prepare_only,
                not args.skip_consumers,
            )
        if args.lane in {"wasm", "all"}:
            evidence["lanes"]["wasm"] = prepare_wasm(
                run,
                args.tag,
                state_root,
                output_root,
                args.binary_cache,
                args.prepare_only,
                not args.skip_consumers,
            )
        source_root = state_root / "occt-src"
        evidence["occt_revision"] = git_output("rev-parse", "HEAD", cwd=source_root)
        evidence["occt_tag_object"] = git_output(
            "rev-parse", f"refs/tags/{args.tag}", cwd=source_root
        )
        evidence["occt_exact_tag"] = git_output("describe", "--tags", "--exact-match", "HEAD", cwd=source_root)
        if args.lane == "all" and not args.prepare_only:
            native = evidence["lanes"]["native"]
            wasm = evidence["lanes"]["wasm"]
            evidence["feasibility_native_wasm_equal"] = (
                native["feasibility_sha256"] == wasm["feasibility_sha256"]
            )
            if not evidence["feasibility_native_wasm_equal"]:
                raise RuntimeError("native and WASM OCCT feasibility output differ")
            if not args.skip_parity:
                run_parity(
                    run,
                    output_root / "build-native",
                    output_root / "build-wasm",
                )
        evidence["status"] = "passed"
    except Exception as exc:
        evidence["status"] = "failed"
        evidence["error"] = str(exc)
        raise
    finally:
        evidence["commands"] = [asdict(command) for command in run.commands]
        report_name = (
            "qualification-report.json"
            if args.lane == "all"
            else f"qualification-report-{args.lane}.json"
        )
        report = output_root / report_name
        report.write_text(json.dumps(evidence, indent=2) + "\n", encoding="utf-8", newline="\n")
        print(f"qualification report: {report}", flush=True)
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
