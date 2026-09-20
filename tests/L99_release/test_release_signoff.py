from __future__ import annotations

import hashlib
import json
import os
import importlib.util
import shutil
import subprocess
import sys
from pathlib import Path

import pytest


ROOT = Path(__file__).resolve().parents[2]


def run_checked(command: list[str]) -> None:
    result = subprocess.run(
        command,
        cwd=ROOT,
        capture_output=True,
        text=True,
        shell=os.name == "nt" and command[0].endswith(".cmd"),
    )
    if result.returncode != 0:
        pytest.fail(
            f"{' '.join(command)} failed with exit {result.returncode}\n"
            f"stdout:\n{result.stdout}\n"
            f"stderr:\n{result.stderr}"
        )


def clang_format_command() -> list[str]:
    if shutil.which("uvx") is not None:
        return ["uvx", "--from", "clang-format==22.1.5", "clang-format"]
    if shutil.which("clang-format") is not None:
        return ["clang-format"]
    pytest.fail("clang-format is required for release signoff; install clang-format or uv.")


def lizard_command() -> list[str]:
    if shutil.which("lizard") is not None:
        return ["lizard"]
    pytest.fail("lizard is required for release signoff; run `uv sync --group dev`.")


def cxx_files() -> list[str]:
    roots = [ROOT / "src", ROOT / "tests" / "cpp", ROOT / "examples" / "cpp"]
    files: list[str] = []
    for root in roots:
        files.extend(str(path.relative_to(ROOT)) for path in root.rglob("*") if path.suffix in {".cpp", ".h", ".hpp"})
    return sorted(files)


def complexity_files() -> list[str]:
    roots = [
        ROOT / "src",
        ROOT / "tests" / "cpp",
        ROOT / "tests" / "python",
        ROOT / "examples" / "cpp",
        ROOT / "examples" / "python",
        ROOT / "python",
        ROOT / "scripts",
    ]
    ignored_dirs = {".venv", "__pycache__"}
    suffixes = {".cpp", ".h", ".hpp", ".py"}
    files: list[str] = []
    for root in roots:
        if not root.exists():
            continue
        for path in root.rglob("*"):
            if not path.is_file() or path.suffix not in suffixes:
                continue
            if any(part in ignored_dirs for part in path.relative_to(ROOT).parts):
                continue
            files.append(str(path.relative_to(ROOT)))
    return sorted(files)


def test_ruff_passes() -> None:
    run_checked(["ruff", "check", "python", "scripts", "tests", "examples/python", "setup.py"])


def test_pyright_passes() -> None:
    run_checked(["pyright"])


def test_uv_lock_is_current() -> None:
    run_checked(["uv", "lock", "--check"])


def test_release_version_surfaces_agree() -> None:
    run_checked([sys.executable, "scripts/ci_release_metadata.py", "check-surfaces"])


def test_clang_format_passes() -> None:
    files = cxx_files()
    assert files, "No C++ files found for clang-format validation."
    run_checked([*clang_format_command(), "--dry-run", "--Werror", *files])


def test_lizard_complexity_passes() -> None:
    files = complexity_files()
    assert files, "No source files found for Lizard validation."
    run_checked([*lizard_command(), "-C", "100", "-L", "500", "-a", "20", *files])


def test_code_hygiene_passes() -> None:
    run_checked([sys.executable, "scripts/check_code_hygiene.py"])


def test_code_hygiene_excludes_generated_rack_results() -> None:
    run_checked(
        [
            sys.executable,
            "-c",
            "from pathlib import Path; from scripts import check_code_hygiene as hygiene; "
            "assert hygiene.should_skip(Path('tests/rack_results/report.html').resolve())",
        ]
    )


def test_code_hygiene_exempts_only_generated_contract_sources_from_line_limit() -> None:
    run_checked(
        [
            sys.executable,
            "-c",
            "from pathlib import Path; from scripts import check_code_hygiene as hygiene; "
            "assert hygiene.is_line_length_exempt(Path('src/cpp/lib/geometer/generated/contracts/contracts_json.cpp')); "
            "assert hygiene.is_line_length_exempt(Path('python/geometer/_generated/contracts/codecs.py')); "
            "assert not hygiene.is_line_length_exempt(Path('src/cpp/lib/ipc_a0_server.cpp'))",
        ]
    )


def test_code_hygiene_allows_active_plans_but_rejects_completed_plans(tmp_path: Path) -> None:
    script = ROOT / "scripts" / "check_code_hygiene.py"
    spec = importlib.util.spec_from_file_location("check_code_hygiene", script)
    assert spec is not None and spec.loader is not None
    hygiene = importlib.util.module_from_spec(spec)
    sys.modules[spec.name] = hygiene
    spec.loader.exec_module(hygiene)

    plan = tmp_path / "example" / "plan.md"
    plan.parent.mkdir()
    plan.write_text(
        "+++\n"
        'type = "plan"\n'
        'id = "example"\n'
        'status = "active"\n'
        '[[steps]]\nid = "work"\ntitle = "Work"\nstatus = "active"\n'
        '[[exit_criteria]]\nid = "exit"\ntitle = "Exit"\nstatus = "pending"\n'
        "+++\n\n# Plan\n",
        encoding="utf-8",
    )
    assert hygiene.completed_plan_paths(tmp_path) == []

    plan.write_text(
        plan.read_text(encoding="utf-8")
        .replace('status = "active"', 'status = "done"', 1)
        .replace('status = "active"', 'status = "done"', 1)
        .replace('status = "pending"', 'status = "met"', 1),
        encoding="utf-8",
    )
    assert hygiene.completed_plan_paths(tmp_path) == [plan]


def test_linux_wheel_builds_use_glibc_235_baseline() -> None:
    ci = (ROOT / ".github/workflows/ci.yml").read_text(encoding="utf-8")
    assert "runs-on: ubuntu-22.04" in ci
    assert "occt-v2-native-linux-x64-gcc-" in ci

    for workflow_name in ("release.yml", "occt-deps.yml"):
        workflow = (ROOT / ".github" / "workflows" / workflow_name).read_text(encoding="utf-8")
        assert "os: ubuntu-22.04\n            platform: linux-x64" in workflow
        assert "os: ubuntu-22.04-arm\n            platform: linux-arm64" in workflow
        assert "occt-v2-native-${{ matrix.platform }}-${{ matrix.compiler }}" in workflow

    build_occt = (ROOT / "scripts" / "build_occt.py").read_text(encoding="utf-8")
    assert '"linux_glibc_baseline": resolved_linux_glibc' in build_occt


def test_normal_builds_use_public_dependency_cache_without_r2_secrets() -> None:
    cache_script = (ROOT / "scripts" / "occt_binary_cache.py").read_text(encoding="utf-8")
    assert 'DEFAULT_PUBLIC_BASE_URL = "https://artifacts.wavenumber.net"' in cache_script

    consumer_workflows = ("ci.yml", "release.yml", "wasm.yml", "macos-wheel.yml")
    for workflow_name in consumer_workflows:
        workflow = (ROOT / ".github" / "workflows" / workflow_name).read_text(encoding="utf-8")
        assert "R2_ACCESS_KEY_ID" not in workflow
        assert "R2_SECRET_ACCESS_KEY" not in workflow

    producer_workflow = (ROOT / ".github" / "workflows" / "occt-deps.yml").read_text(encoding="utf-8")
    assert "R2_ACCESS_KEY_ID" in producer_workflow
    assert "R2_SECRET_ACCESS_KEY" in producer_workflow


def test_ci_is_manual_only_and_release_rebuilds_every_output_once() -> None:
    ci = (ROOT / ".github/workflows/ci.yml").read_text(encoding="utf-8")
    release = (ROOT / ".github/workflows/release.yml").read_text(encoding="utf-8")

    for workflow in (ci, release):
        assert 'CARGO_BUILD_JOBS: "1"' in workflow
        assert 'CMAKE_BUILD_PARALLEL_LEVEL: "2"' in workflow
        assert 'GEOMETER_REQUIRE_NATIVE_TEST_SERVERS: "1"' in workflow
        assert "cargo test --locked" not in workflow

    assert "GEOMETER_OCCT_BINARY: only" in release

    assert "name: Full Validation (Manual)" in ci
    assert "workflow_dispatch:" in ci
    assert "pull_request:" not in ci
    assert "release:" not in ci
    assert ci.count("GEOMETER_TEST_PROFILE: production") == 4
    assert "push:" not in ci

    assert 'name: Publish' in release
    release_triggers = release.split("\npermissions:", 1)[0]
    assert "workflow_dispatch:" in release_triggers
    assert "\n  release:" not in release_triggers
    assert "\n  pull_request:" not in release_triggers
    assert "\n  push:" not in release_triggers
    assert "ref: ${{ inputs.tag }}" in release
    assert 'test "$GITHUB_REF" = "refs/tags/$RELEASE_TAG"' in release
    assert 'test "$(git rev-parse HEAD)" = "$(git rev-parse "$GITHUB_SHA^{commit}")"' in release

    assert release.count("uv run --group dev rack run python") == 1
    assert release.count("uv run --group dev rack run typescript") == 1
    assert release.count("uv run --group dev rack run rust") == 1
    assert "if: matrix.platform == 'linux-x64'" in release
    assert "GEOMETER_TEST_PROFILE: production" in release
    assert release.count("scripts/validate_python_package.py --skip-native-validation --wheelhouse out/wheelhouse") == 1
    assert "python -m build --wheel --outdir out/wheelhouse" not in release
    assert "twine check out/wheelhouse/*.whl" in release
    assert "path: out/wheelhouse/*.whl" in release
    assert "scripts/build_static_sdk.py --platform ${{ matrix.platform }}" in release
    assert "scripts/validate_static_sdk.py out/sdk-candidate/geometer-sdk-*.zip" in release
    assert release.index("scripts/build_static_sdk.py") < release.index("scripts/validate_native.py")
    assert "scripts/build_static_sdk.py --platform ${{ matrix.platform }} --allow-dirty" not in release
    assert "scripts/validate_release_inventory.py" in release
    assert "scripts/verify_release_inventory.py" in release
    assert release.count("mkdir -p out/draft-release") == 2
    assert release.count("mkdir -p out/public-release") == 1
    assert "mkdir out/draft-release" not in release
    assert "mkdir out/public-release" not in release
    assert "name: qualified-release" in release
    assert "--clobber" not in release
    assert "needs: qualify-release" in release
    assert "needs: github-assets" in release
    assert "needs: pypi" in release
    assert "needs: publish-release" in release
    assert "actions/attest@v4" in release
    assert "artifact-metadata: write" in release
    assert "gh attestation verify" in release
    assert "mapfile -d '' attestable_archives" in release
    assert "-name 'geometer-sdk-*.zip'" in release
    assert "geometer-static-illustration-demo" not in release
    assert 'test "${#attestable_archives[@]}" -eq 4' in release
    assert 'for asset in "${attestable_archives[@]}"; do' in release
    assert '--signer-workflow "$GITHUB_REPOSITORY/.github/workflows/release.yml"' in release
    assert '--source-ref "$GITHUB_REF"' in release
    assert '--source-digest "$GITHUB_SHA"' in release
    assert "wn-dev-std audit . --mode release --format json" in release
    assert "-eq 4" in release


def test_every_workflow_is_manual_only() -> None:
    workflows = ROOT / ".github" / "workflows"
    automatic_triggers = ("pull_request:", "push:", "schedule:", "release:")

    for path in workflows.glob("*.yml"):
        workflow = path.read_text(encoding="utf-8")
        triggers = workflow.split("\npermissions:", 1)[0]
        assert "workflow_dispatch:" in triggers
        assert all(f"\n  {trigger}" not in triggers for trigger in automatic_triggers)


def test_every_workflow_job_has_a_cost_timeout() -> None:
    workflows = ROOT / ".github" / "workflows"

    for path in workflows.glob("*.yml"):
        lines = path.read_text(encoding="utf-8").splitlines()
        jobs_index = lines.index("jobs:")
        job_starts = [
            index
            for index, line in enumerate(lines[jobs_index + 1 :], jobs_index + 1)
            if line.startswith("  ")
            and not line.startswith("    ")
            and line.endswith(":")
        ]
        for position, start in enumerate(job_starts):
            end = job_starts[position + 1] if position + 1 < len(job_starts) else len(lines)
            job = lines[start:end]
            assert any(line.startswith("    timeout-minutes:") for line in job), (
                f"{path.name}:{lines[start].strip(':')} has no timeout-minutes"
            )


def test_transport_baseline_can_target_one_cached_platform() -> None:
    baseline = (
        ROOT / ".github" / "workflows" / "operation-transport-baseline.yml"
    ).read_text(encoding="utf-8")

    assert "Platform baseline to record." in baseline
    assert "inputs.target == 'windows-x64'" in baseline
    assert "inputs.target == 'macos-arm64'" in baseline
    assert "occt-${{ matrix.os }}-${{ runner.arch }}-${{ matrix.compiler }}" in baseline


def test_governed_transport_evidence_preserves_reviewed_bytes() -> None:
    attributes = (ROOT / ".gitattributes").read_text(encoding="utf-8")
    assert "docs/research/evidence/**/*.json binary" in attributes

    root = (
        ROOT
        / "docs"
        / "research"
        / "evidence"
        / "operation-transport"
        / "sibling-baseline-2026-09-19"
    )
    inventory = json.loads((root / "inventory.json").read_text(encoding="utf-8"))
    for platform in ("windows_x64", "macos_arm64"):
        for report in inventory[platform]["reports"]:
            payload = (root / report["path"]).read_bytes()
            assert len(payload) == report["bytes"]
            assert hashlib.sha256(payload).hexdigest() == report["sha256"]


def test_experimental_qualification_is_outside_normal_ci_and_release() -> None:
    command = "uv run pytest tests/wasm/test_analytic_cross_transport_parity.py -q"
    ci = (ROOT / ".github/workflows/ci.yml").read_text(encoding="utf-8")
    experimental = (ROOT / ".github/workflows/wasm.yml").read_text(encoding="utf-8")
    release = (ROOT / ".github/workflows/release.yml").read_text(encoding="utf-8")

    assert command not in ci
    assert command not in release
    assert experimental.count(command) == 1
    assert "pull_request:" not in experimental
    assert "workflow_dispatch:" in experimental
    assert "schedule:" not in experimental
    assert "--include-experimental-tests" in experimental
    assert "--include-experimental-tests" not in ci
    assert "--include-experimental-tests" not in release
    assert "  cross-transport:" not in release
    assert release.count("needs: [build, wasm]") == 1
    assert release.count("needs: qualify-release") == 1


def test_occt_cache_consumers_share_platform_keys() -> None:
    workflows = {
        name: (ROOT / ".github" / "workflows" / name).read_text(encoding="utf-8")
        for name in ("ci.yml", "release.yml", "wasm.yml", "macos-wheel.yml", "occt-deps.yml")
    }
    matrix_native_key = "occt-v2-native-${{ matrix.platform }}-${{ matrix.compiler }}"
    assert matrix_native_key in workflows["release.yml"]
    assert matrix_native_key in workflows["occt-deps.yml"]
    assert "occt-v2-native-linux-x64-gcc-" in workflows["ci.yml"]
    assert "occt-v2-native-linux-x64-gcc-" in workflows["wasm.yml"]
    assert "occt-v2-native-macos-arm64-apple-clang-" in workflows["macos-wheel.yml"]

    producer = workflows["occt-deps.yml"]
    assert "profile: static-crt" in producer
    assert "cache_suffix: -static-crt" in producer
    assert "occt-static-crt-build" in producer
    assert "occt-static-crt-install" in producer
    assert "build_args: --msvc-runtime Static" in producer

    wasm_key = "occt-v2-wasm-linux-x64-emscripten-"
    for name in ("ci.yml", "release.yml", "wasm.yml", "occt-deps.yml"):
        assert wasm_key in workflows[name]
