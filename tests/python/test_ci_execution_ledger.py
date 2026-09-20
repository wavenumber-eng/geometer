from __future__ import annotations

from pathlib import Path
import subprocess
import sys

import pytest

from ci_execution_ledger import (
    ExecutionLedgerError,
    canonical_json,
    empty_ledger,
    load_ledger,
    render_summary,
    run_and_record,
    write_ledger,
)


SCRIPT = Path(__file__).resolve().parents[2] / "scripts" / "ci_execution_ledger.py"


def test_run_records_canonical_success_and_selected_parallelism(
    tmp_path: Path, monkeypatch: pytest.MonkeyPatch
) -> None:
    ledger_path = tmp_path / "ledger.json"
    monkeypatch.delenv("GEOMETER_REQUIRE_NATIVE_TEST_SERVERS", raising=False)
    monkeypatch.delenv("GEOMETER_TEST_PROFILE", raising=False)
    monkeypatch.delenv("GEOMETER_TYPESCRIPT_SCOPE", raising=False)
    monkeypatch.setenv("CMAKE_BUILD_PARALLEL_LEVEL", "16")
    monkeypatch.setenv("CARGO_BUILD_JOBS", "8")
    command = [sys.executable, "-c", "print('child output is inherited')"]

    assert run_and_record(ledger_path, lane="windows-x64", kind="build", name="native build", command=command) == 0

    raw = ledger_path.read_bytes()
    ledger = load_ledger(ledger_path)
    assert raw == canonical_json(ledger)
    assert ledger["schema"] == "wn.geometer.ci_execution_ledger.a0"
    assert ledger["entries"] == [
        {
            "command": command,
            "duration_seconds": ledger["entries"][0]["duration_seconds"],
            "environment": {
                "CARGO_BUILD_JOBS": "8",
                "CMAKE_BUILD_PARALLEL_LEVEL": "16",
                "GEOMETER_REQUIRE_NATIVE_TEST_SERVERS": None,
                "GEOMETER_TEST_PROFILE": None,
                "GEOMETER_TYPESCRIPT_SCOPE": None,
            },
            "exit_code": 0,
            "kind": "build",
            "lane": "windows-x64",
            "name": "native build",
            "outcome": "success",
        }
    ]
    assert ledger["entries"][0]["duration_seconds"] >= 0
    assert not list(tmp_path.glob(".ledger.json.*.tmp"))


def test_cli_preserves_failure_exit_and_appends_record(tmp_path: Path) -> None:
    ledger_path = tmp_path / "ledger.json"
    completed = subprocess.run(
        [
            sys.executable,
            str(SCRIPT),
            "run",
            "--ledger",
            str(ledger_path),
            "--lane",
            "linux-x64",
            "--kind",
            "test",
            "--name",
            "focused failure",
            "--",
            sys.executable,
            "-c",
            "raise SystemExit(7)",
        ],
        check=False,
    )

    assert completed.returncode == 7
    entry = load_ledger(ledger_path)["entries"][0]
    assert entry["exit_code"] == 7
    assert entry["outcome"] == "failure"
    assert entry["command"][-2:] == ["-c", "raise SystemExit(7)"]


def test_missing_executable_is_recorded_as_failure(tmp_path: Path) -> None:
    ledger_path = tmp_path / "ledger.json"

    assert (
        run_and_record(
            ledger_path,
            lane="windows-x64",
            kind="build",
            name="missing command",
            command=[str(tmp_path / "does-not-exist")],
        )
        == 127
    )

    assert load_ledger(ledger_path)["entries"][0]["exit_code"] == 127


@pytest.mark.parametrize(
    "raw",
    [
        b"not json\n",
        b'{"entries":[],"schema":"wn.geometer.ci_execution_ledger.a0"}\n',
        canonical_json({"entries": [], "schema": "wrong"}),
        canonical_json(
            {
                "entries": [
                    {
                        "command": ["tool"],
                        "duration_seconds": 1,
                        "environment": {
                            "CARGO_BUILD_JOBS": None,
                            "CMAKE_BUILD_PARALLEL_LEVEL": None,
                            "GEOMETER_REQUIRE_NATIVE_TEST_SERVERS": None,
                            "GEOMETER_TEST_PROFILE": None,
                            "GEOMETER_TYPESCRIPT_SCOPE": None,
                        },
                        "exit_code": 1,
                        "kind": "test",
                        "lane": "linux-x64",
                        "name": "contradiction",
                        "outcome": "success",
                    }
                ],
                "schema": "wn.geometer.ci_execution_ledger.a0",
            }
        ),
    ],
)
def test_malformed_or_noncanonical_state_is_rejected_before_execution(tmp_path: Path, raw: bytes) -> None:
    ledger_path = tmp_path / "ledger.json"
    marker = tmp_path / "executed"
    ledger_path.write_bytes(raw)

    with pytest.raises(ExecutionLedgerError):
        run_and_record(
            ledger_path,
            lane="linux-x64",
            kind="test",
            name="must not run",
            command=[sys.executable, "-c", f"open({str(marker)!r}, 'w').close()"],
        )

    assert not marker.exists()
    assert ledger_path.read_bytes() == raw


def test_symlink_ledger_is_rejected_when_supported(tmp_path: Path) -> None:
    target = tmp_path / "target.json"
    link = tmp_path / "ledger.json"
    target.write_bytes(canonical_json(empty_ledger()))
    try:
        link.symlink_to(target)
    except OSError:
        pytest.skip("creating symlinks is unavailable")

    with pytest.raises(ExecutionLedgerError, match="link"):
        load_ledger(link)


def test_write_creates_parent_and_leaves_no_partial_file(tmp_path: Path) -> None:
    ledger_path = tmp_path / "missing" / "ledger.json"

    write_ledger(ledger_path, empty_ledger())

    assert load_ledger(ledger_path) == empty_ledger()
    assert not list(ledger_path.parent.glob(".ledger.json.*.tmp"))


def test_broken_symlink_ledger_is_rejected_when_supported(tmp_path: Path) -> None:
    link = tmp_path / "ledger.json"
    try:
        link.symlink_to(tmp_path / "missing-target.json")
    except OSError:
        pytest.skip("creating symlinks is unavailable")

    with pytest.raises(ExecutionLedgerError, match="link"):
        load_ledger(link, missing_ok=True)


def test_summary_is_markdown_for_github_step_summary(tmp_path: Path) -> None:
    ledger_path = tmp_path / "ledger.json"
    run_and_record(
        ledger_path,
        lane="linux-x64",
        kind="test",
        name="unit | tests",
        command=[sys.executable, "-c", "pass"],
    )

    rendered = render_summary(load_ledger(ledger_path))

    assert rendered.startswith("## CI execution ledger\n")
    assert "1/1 commands succeeded" in rendered
    assert "unit \\| tests" in rendered
    assert "| Lane | Kind | Name | Outcome |" in rendered


def test_summary_cli_can_write_output_file(tmp_path: Path) -> None:
    ledger_path = tmp_path / "ledger.json"
    output_path = tmp_path / "step-summary.md"
    write_ledger(ledger_path, empty_ledger())

    completed = subprocess.run(
        [sys.executable, str(SCRIPT), "summary", "--ledger", str(ledger_path), "--output", str(output_path)],
        check=False,
        capture_output=True,
        text=True,
    )

    assert completed.returncode == 0, completed.stderr
    assert "0/0 commands succeeded" in output_path.read_text(encoding="utf-8")


def test_command_is_executed_without_a_shell(tmp_path: Path) -> None:
    ledger_path = tmp_path / "ledger.json"
    shell_marker = tmp_path / "shell-marker"
    shell_syntax = f"echo unsafe > {shell_marker}"

    exit_code = run_and_record(
        ledger_path,
        lane="linux-x64",
        kind="test",
        name="no shell",
        command=[sys.executable, "-c", "import sys; assert sys.argv[1].startswith('echo')", shell_syntax],
    )

    assert exit_code == 0
    assert not shell_marker.exists()


def test_environment_records_absent_values_as_null(tmp_path: Path, monkeypatch: pytest.MonkeyPatch) -> None:
    monkeypatch.delenv("CMAKE_BUILD_PARALLEL_LEVEL", raising=False)
    monkeypatch.delenv("CARGO_BUILD_JOBS", raising=False)
    monkeypatch.delenv("GEOMETER_REQUIRE_NATIVE_TEST_SERVERS", raising=False)
    monkeypatch.delenv("GEOMETER_TEST_PROFILE", raising=False)
    monkeypatch.delenv("GEOMETER_TYPESCRIPT_SCOPE", raising=False)
    ledger_path = tmp_path / "ledger.json"

    run_and_record(
        ledger_path,
        lane="macos-arm64",
        kind="test",
        name="defaults",
        command=[sys.executable, "-c", "pass"],
    )

    assert load_ledger(ledger_path)["entries"][0]["environment"] == {
        "CARGO_BUILD_JOBS": None,
        "CMAKE_BUILD_PARALLEL_LEVEL": None,
        "GEOMETER_REQUIRE_NATIVE_TEST_SERVERS": None,
        "GEOMETER_TEST_PROFILE": None,
        "GEOMETER_TYPESCRIPT_SCOPE": None,
    }
