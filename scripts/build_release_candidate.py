#!/usr/bin/env python3
"""Build and qualify one release-candidate lane with fixed, explicit steps."""

from __future__ import annotations

import argparse
from collections.abc import Callable, Sequence
from contextlib import contextmanager
from dataclasses import dataclass
import os
from pathlib import Path
import shutil
import subprocess
import sys

from build_static_sdk import SUPPORTED_PLATFORMS, native_platform
from ci_execution_ledger import run_and_record
from ci_release_metadata import package_version


ROOT = Path(__file__).resolve().parents[1]
DEFAULT_LEDGER = ROOT / "out" / "ci-execution-ledger.json"


class CandidateBuildError(RuntimeError):
    """Raised when a candidate lane is unsafe or incomplete."""


@dataclass(frozen=True)
class CandidateTask:
    """One fixed candidate command and its deliberate environment overrides."""

    kind: str
    name: str
    command: tuple[str, ...]
    environment: tuple[tuple[str, str], ...] = ()


TaskExecutor = Callable[[CandidateTask], None]


def _python_script(name: str, *arguments: str) -> tuple[str, ...]:
    return (sys.executable, str(ROOT / "scripts" / name), *arguments)


def _python_module(name: str, *arguments: str) -> tuple[str, ...]:
    return (sys.executable, "-m", name, *arguments)


@contextmanager
def _environment(overrides: tuple[tuple[str, str], ...]):
    previous = {name: os.environ.get(name) for name, _ in overrides}
    try:
        for name, value in overrides:
            os.environ[name] = value
        yield
    finally:
        for name, value in previous.items():
            if value is None:
                os.environ.pop(name, None)
            else:
                os.environ[name] = value


def _executor(ledger: Path, lane: str) -> TaskExecutor:
    def execute(task: CandidateTask) -> None:
        with _environment(task.environment):
            exit_code = run_and_record(
                ledger,
                lane=lane,
                kind=task.kind,
                name=task.name,
                command=task.command,
            )
        if exit_code != 0:
            raise CandidateBuildError(f"candidate task failed ({exit_code}): {task.name}")

    return execute


def _new_output_directory(path: Path) -> Path:
    output = path.resolve()
    if output.exists() or output.is_symlink():
        raise CandidateBuildError(f"candidate output already exists; use a new directory: {output}")
    output.mkdir(parents=True)
    return output


def require_clean_checkout() -> None:
    """Reject candidate production from modified or untracked source state."""

    completed = subprocess.run(
        ["git", "status", "--porcelain=v1"],
        cwd=ROOT,
        check=True,
        capture_output=True,
        text=True,
    )
    if completed.stdout:
        raise CandidateBuildError("release candidates require a clean Git checkout")


def _single_wheel(wheelhouse: Path) -> Path:
    wheels = sorted(wheelhouse.glob("wn_geometer-*.whl"))
    if len(wheels) != 1:
        raise CandidateBuildError(f"expected exactly one candidate wheel under {wheelhouse}, found {len(wheels)}")
    return wheels[0]


def run_native_candidate(platform: str, output: Path, execute: TaskExecutor) -> None:
    """Build one native lane in the only supported order."""

    version = package_version()
    sdk = output / f"geometer-sdk-{version}-{platform}.zip"
    wheelhouse = output / "wheelhouse"
    native = output / f"native-{platform}.zip"
    tasks = [
        CandidateTask(
            "build",
            "static C ABI SDK",
            _python_script("build_static_sdk.py", "--platform", platform, "--output", str(sdk)),
        ),
        CandidateTask(
            "test",
            "relocated static SDK consumer",
            _python_script("validate_static_sdk.py", str(sdk)),
        ),
        CandidateTask(
            "build-test",
            "production native validation",
            _python_script("validate_native.py"),
        ),
    ]
    if platform == "linux-x64":
        client_environment = (
            ("GEOMETER_REQUIRE_NATIVE_TEST_SERVERS", "1"),
            ("GEOMETER_TEST_PROFILE", "production"),
        )
        tasks.extend(
            [
                CandidateTask(
                    "test",
                    "Python client stratum",
                    _python_module("rack", "run", "python"),
                    client_environment,
                ),
                CandidateTask(
                    "test",
                    "Rust client stratum",
                    _python_module("rack", "run", "rust"),
                    client_environment,
                ),
                CandidateTask(
                    "test",
                    "TypeScript host client stratum",
                    _python_module("rack", "run", "typescript"),
                    client_environment + (("GEOMETER_TYPESCRIPT_SCOPE", "host"),),
                ),
            ]
        )
    tasks.append(
        CandidateTask(
            "build-test",
            "Python package validation",
            _python_script(
                "validate_python_package.py",
                "--skip-native-validation",
                "--wheelhouse",
                str(wheelhouse),
            ),
        )
    )
    for task in tasks:
        execute(task)
    wheel = _single_wheel(wheelhouse)
    execute(CandidateTask("test", "wheel metadata", _python_module("twine", "check", str(wheel))))
    execute(
        CandidateTask(
            "package",
            "native distribution",
            _python_script(
                "package_release_artifacts.py",
                "native",
                "--platform",
                platform,
                "--output",
                str(native),
            ),
        )
    )


def run_wasm_candidate(output: Path, execute: TaskExecutor) -> None:
    """Build the WASM lane and run every candidate-owned browser/package check."""

    npm = shutil.which("npm")
    node = shutil.which("node")
    if npm is None or node is None:
        raise CandidateBuildError("Node 24 and npm are required for the WASM candidate lane")
    wasm = output / "wasm-dist.zip"
    tasks = [
        CandidateTask("build", "WASM artifacts", _python_script("build_wasm.py")),
        CandidateTask("test", "TypeScript check", (npm, "run", "check:typescript")),
        CandidateTask("build", "HLR browser site", _python_script("build_hlr_site.py")),
        CandidateTask("build", "illustration browser site", _python_script("build_illustration_site.py")),
        CandidateTask(
            "build",
            "standalone HLR demo",
            _python_script("build_standalone_demos.py", "hlr"),
        ),
        CandidateTask(
            "build",
            "standalone illustration demo",
            _python_script("build_standalone_demos.py", "illustration"),
        ),
        CandidateTask(
            "test",
            "browser site validation",
            _python_module(
                "pytest",
                "tests/wasm/test_hlr_static_site.py",
                "tests/wasm/test_illustration_static_site.py",
                "-q",
            ),
        ),
        CandidateTask(
            "test",
            "TypeScript WASM client stratum",
            _python_module("rack", "run", "typescript"),
            (
                ("GEOMETER_TEST_PROFILE", "production"),
                ("GEOMETER_TYPESCRIPT_SCOPE", "wasm"),
            ),
        ),
        CandidateTask(
            "test",
            "WASM planar batch validation",
            (node, str(ROOT / "tests/wasm/planar_batch_solve_bytes_validation.js")),
        ),
        CandidateTask(
            "test",
            "WASM STEP to GLB validation",
            (node, str(ROOT / "tests/wasm/step_to_glb_bytes_validation.js")),
        ),
        CandidateTask(
            "package",
            "WASM distribution",
            _python_script("package_release_artifacts.py", "wasm", "--output", str(wasm)),
        ),
    ]
    for task in tasks:
        execute(task)


def _parser() -> argparse.ArgumentParser:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--ledger", type=Path, default=DEFAULT_LEDGER)
    commands = parser.add_subparsers(dest="lane", required=True)
    native = commands.add_parser("native")
    native.add_argument("--platform", choices=sorted(SUPPORTED_PLATFORMS), required=True)
    native.add_argument("--output-dir", type=Path)
    wasm = commands.add_parser("wasm")
    wasm.add_argument("--output-dir", type=Path)
    return parser


def main(argv: Sequence[str] | None = None) -> int:
    args = _parser().parse_args(argv)
    lane = args.platform if args.lane == "native" else "wasm"
    requested_output = args.output_dir or ROOT / "out" / "release-candidate" / lane
    try:
        require_clean_checkout()
        if args.lane == "native" and args.platform != native_platform():
            raise CandidateBuildError(
                f"native candidate target {args.platform} does not match this host ({native_platform()})"
            )
        output = _new_output_directory(requested_output)
        execute = _executor(args.ledger.resolve(), lane)
        if args.lane == "native":
            run_native_candidate(args.platform, output, execute)
        else:
            run_wasm_candidate(output, execute)
    except CandidateBuildError as error:
        print(str(error), file=sys.stderr)
        return 1
    print(f"qualified {lane} candidate: {output}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
