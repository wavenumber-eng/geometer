from __future__ import annotations

from pathlib import Path

import pytest

import build_release_candidate as candidate


def _recording_executor(tasks: list[candidate.CandidateTask]):
    def execute(task: candidate.CandidateTask) -> None:
        tasks.append(task)
        if task.name == "Python package validation":
            wheelhouse = Path(task.command[task.command.index("--wheelhouse") + 1])
            wheelhouse.mkdir(parents=True)
            (wheelhouse / "wn_geometer-test.whl").write_bytes(b"wheel")

    return execute


def test_native_candidate_has_one_fixed_linux_order(tmp_path: Path) -> None:
    tasks: list[candidate.CandidateTask] = []
    output = tmp_path / "linux-x64"
    output.mkdir()

    candidate.run_native_candidate("linux-x64", output, _recording_executor(tasks))

    assert [task.name for task in tasks] == [
        "static C ABI SDK",
        "relocated static SDK consumer",
        "production native validation",
        "Python client stratum",
        "Rust client stratum",
        "pinned Node dependencies",
        "Node toolchain version",
        "TypeScript host client stratum",
        "Python package validation",
        "wheel metadata",
        "native distribution",
    ]
    typescript = tasks[7]
    assert dict(typescript.environment) == {
        "GEOMETER_REQUIRE_NATIVE_TEST_SERVERS": "1",
        "GEOMETER_TEST_PROFILE": "production",
        "GEOMETER_TYPESCRIPT_SCOPE": "host",
    }
    assert str(output / "native-linux-x64.zip") in tasks[-1].command
    assert str(output / "wheelhouse" / "wn_geometer-test.whl") in tasks[-2].command


def test_native_candidate_keeps_cross_language_tests_on_linux_x64(tmp_path: Path) -> None:
    tasks: list[candidate.CandidateTask] = []
    output = tmp_path / "windows-x64"
    output.mkdir()

    candidate.run_native_candidate("windows-x64", output, _recording_executor(tasks))

    names = [task.name for task in tasks]
    assert "Python client stratum" not in names
    assert "Rust client stratum" not in names
    assert "TypeScript host client stratum" not in names
    assert names[-2:] == ["wheel metadata", "native distribution"]


def test_wasm_candidate_owns_package_and_worker_scope(tmp_path: Path, monkeypatch: pytest.MonkeyPatch) -> None:
    monkeypatch.setattr(candidate.shutil, "which", lambda name: f"/tools/{name}")
    tasks: list[candidate.CandidateTask] = []
    output = tmp_path / "wasm"
    output.mkdir()

    candidate.run_wasm_candidate(output, tasks.append)

    assert [task.name for task in tasks] == [
        "pinned Node dependencies",
        "Node toolchain version",
        "WASM artifacts",
        "TypeScript check",
        "HLR browser site",
        "illustration browser site",
        "standalone HLR demo",
        "standalone illustration demo",
        "browser site validation",
        "TypeScript WASM client stratum",
        "WASM planar batch validation",
        "WASM STEP to GLB validation",
        "WASM distribution",
    ]
    assert dict(tasks[9].environment) == {
        "GEOMETER_TEST_PROFILE": "production",
        "GEOMETER_TYPESCRIPT_SCOPE": "wasm",
    }
    assert str(output / "wasm-dist.zip") in tasks[-1].command


def test_candidate_output_must_be_new(tmp_path: Path) -> None:
    output = tmp_path / "candidate"
    output.mkdir()

    with pytest.raises(candidate.CandidateBuildError, match="already exists"):
        candidate._new_output_directory(output)

    created = candidate._new_output_directory(tmp_path / "new-candidate")
    assert created.is_dir()


def test_candidate_requires_clean_checkout(monkeypatch: pytest.MonkeyPatch) -> None:
    class Completed:
        stdout = " M source.cpp\n"

    monkeypatch.setattr(candidate.subprocess, "run", lambda *args, **kwargs: Completed())
    with pytest.raises(candidate.CandidateBuildError, match="clean Git checkout"):
        candidate.require_clean_checkout()
