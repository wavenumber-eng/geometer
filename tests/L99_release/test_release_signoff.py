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
    if shutil.which("clang-format") is not None:
        return ["clang-format"]
    if shutil.which("uvx") is not None:
        return ["uvx", "clang-format"]
    pytest.fail("clang-format is required for release signoff; install clang-format or uv.")


def cxx_files() -> list[str]:
    roots = [ROOT / "src", ROOT / "tests" / "cpp", ROOT / "examples" / "cpp"]
    files: list[str] = []
    for root in roots:
        files.extend(str(path.relative_to(ROOT)) for path in root.rglob("*") if path.suffix in {".cpp", ".h", ".hpp"})
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


def test_code_hygiene_passes() -> None:
    run_checked([sys.executable, "scripts/check_code_hygiene.py"])
