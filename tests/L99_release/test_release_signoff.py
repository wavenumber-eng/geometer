from __future__ import annotations

import os
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


def test_linux_wheel_builds_use_glibc_235_baseline() -> None:
    for workflow_name in ("ci.yml", "release.yml", "occt-deps.yml"):
        workflow = (ROOT / ".github" / "workflows" / workflow_name).read_text(encoding="utf-8")
        assert "os: ubuntu-22.04\n            platform: linux-x64" in workflow
        assert "os: ubuntu-22.04-arm\n            platform: linux-arm64" in workflow
        assert "occt-${{ matrix.os }}-${{ runner.arch }}" in workflow

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
