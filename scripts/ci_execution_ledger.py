"""Run CI commands while recording a strict canonical execution ledger."""

from __future__ import annotations

import argparse
import json
import math
import os
from pathlib import Path
import re
import subprocess
import sys
import tempfile
import time
from typing import Any, Sequence


SCHEMA = "wn.geometer.ci_execution_ledger.a0"
ENVIRONMENT_NAMES = ("CARGO_BUILD_JOBS", "CMAKE_BUILD_PARALLEL_LEVEL")
TOKEN_RE = re.compile(r"[A-Za-z0-9][A-Za-z0-9._:+/-]*")
MAX_TEXT_LENGTH = 512
MAX_COMMAND_ARGUMENTS = 4096
MAX_ENTRIES = 10000


class ExecutionLedgerError(RuntimeError):
    """Raised when ledger input is unsafe or outside the governed schema."""


def canonical_json(value: dict[str, Any]) -> bytes:
    """Encode a ledger with the repository's canonical JSON convention."""

    return (json.dumps(value, indent=2, sort_keys=True, ensure_ascii=True) + "\n").encode("utf-8")


def _plain_text(value: Any, label: str, *, token: bool = False) -> str:
    if not isinstance(value, str) or not value or len(value) > MAX_TEXT_LENGTH:
        raise ExecutionLedgerError(f"{label} must be non-empty text no longer than {MAX_TEXT_LENGTH} characters")
    if any(character in value for character in ("\0", "\r", "\n")):
        raise ExecutionLedgerError(f"{label} must be single-line text without NUL characters")
    if token and TOKEN_RE.fullmatch(value) is None:
        raise ExecutionLedgerError(f"{label} must be a portable token")
    return value


def _validate_command(value: Any) -> list[str]:
    if not isinstance(value, list) or not value or len(value) > MAX_COMMAND_ARGUMENTS:
        raise ExecutionLedgerError("command must be a non-empty, bounded argv array")
    command: list[str] = []
    for index, argument in enumerate(value):
        if not isinstance(argument, str) or not argument or "\0" in argument:
            raise ExecutionLedgerError(f"command[{index}] must be non-empty text without NUL characters")
        command.append(argument)
    return command


def _validate_entry(value: Any) -> dict[str, Any]:
    if not isinstance(value, dict):
        raise ExecutionLedgerError("ledger entries must be objects")
    expected = {
        "command",
        "duration_seconds",
        "environment",
        "exit_code",
        "kind",
        "lane",
        "name",
        "outcome",
    }
    if set(value) != expected:
        raise ExecutionLedgerError("ledger entry fields differ from schema")
    _validate_command(value["command"])
    duration = value["duration_seconds"]
    if isinstance(duration, bool) or not isinstance(duration, (int, float)):
        raise ExecutionLedgerError("duration_seconds must be a finite non-negative number")
    if not math.isfinite(float(duration)) or duration < 0:
        raise ExecutionLedgerError("duration_seconds must be a finite non-negative number")
    exit_code = value["exit_code"]
    if isinstance(exit_code, bool) or not isinstance(exit_code, int) or not -(2**31) <= exit_code < 2**32:
        raise ExecutionLedgerError("exit_code is outside the supported process range")
    outcome = value["outcome"]
    if outcome not in {"failure", "success"}:
        raise ExecutionLedgerError("outcome must be success or failure")
    if (exit_code == 0) != (outcome == "success"):
        raise ExecutionLedgerError("outcome does not agree with exit_code")
    _plain_text(value["lane"], "lane", token=True)
    _plain_text(value["kind"], "kind", token=True)
    _plain_text(value["name"], "name")
    environment = value["environment"]
    if not isinstance(environment, dict) or set(environment) != set(ENVIRONMENT_NAMES):
        raise ExecutionLedgerError("environment fields differ from schema")
    for environment_name in ENVIRONMENT_NAMES:
        environment_value = environment[environment_name]
        if environment_value is not None:
            _plain_text(environment_value, f"environment.{environment_name}")
    return value


def validate_ledger(value: Any) -> dict[str, Any]:
    """Validate and return a ledger without normalizing malformed state."""

    if not isinstance(value, dict) or set(value) != {"entries", "schema"}:
        raise ExecutionLedgerError("ledger root fields differ from schema")
    if value["schema"] != SCHEMA:
        raise ExecutionLedgerError(f"unsupported execution ledger schema: {value['schema']!r}")
    entries = value["entries"]
    if not isinstance(entries, list) or len(entries) > MAX_ENTRIES:
        raise ExecutionLedgerError("entries must be a bounded array")
    for entry in entries:
        _validate_entry(entry)
    return value


def empty_ledger() -> dict[str, Any]:
    return {"entries": [], "schema": SCHEMA}


def load_ledger(path: Path, *, missing_ok: bool = False) -> dict[str, Any]:
    """Load strict canonical JSON, rejecting links and non-regular paths."""

    if path.is_symlink():
        raise ExecutionLedgerError(f"execution ledger must be a regular file, not a link: {path}")
    if not path.exists():
        if missing_ok:
            return empty_ledger()
        raise ExecutionLedgerError(f"execution ledger does not exist: {path}")
    if not path.is_file():
        raise ExecutionLedgerError(f"execution ledger must be a regular file, not a link: {path}")
    try:
        raw = path.read_bytes()
        value = json.loads(raw)
    except (OSError, UnicodeDecodeError, json.JSONDecodeError) as error:
        raise ExecutionLedgerError(f"invalid execution ledger JSON: {error}") from error
    ledger = validate_ledger(value)
    if raw != canonical_json(ledger):
        raise ExecutionLedgerError("execution ledger is not canonical JSON")
    return ledger


def write_ledger(path: Path, ledger: dict[str, Any]) -> None:
    """Atomically replace a regular ledger in its existing parent directory."""

    validate_ledger(ledger)
    parent = path.parent
    try:
        parent.mkdir(parents=True, exist_ok=True)
    except OSError as error:
        raise ExecutionLedgerError(f"could not create execution ledger parent directory: {error}") from error
    if not parent.is_dir():
        raise ExecutionLedgerError(f"execution ledger parent is not a directory: {parent}")
    if path.is_symlink() or (path.exists() and not path.is_file()):
        raise ExecutionLedgerError(f"execution ledger must be a regular file, not a link: {path}")
    temporary_name: str | None = None
    try:
        with tempfile.NamedTemporaryFile(
            mode="wb", prefix=f".{path.name}.", suffix=".tmp", dir=parent, delete=False
        ) as stream:
            temporary_name = stream.name
            stream.write(canonical_json(ledger))
            stream.flush()
            os.fsync(stream.fileno())
        os.replace(temporary_name, path)
        temporary_name = None
    except OSError as error:
        raise ExecutionLedgerError(f"could not atomically write execution ledger: {error}") from error
    finally:
        if temporary_name is not None:
            try:
                Path(temporary_name).unlink()
            except FileNotFoundError:
                pass


def run_and_record(
    ledger_path: Path,
    *,
    lane: str,
    kind: str,
    name: str,
    command: Sequence[str],
) -> int:
    """Run an argv command and atomically append its outcome to the ledger."""

    _plain_text(lane, "lane", token=True)
    _plain_text(kind, "kind", token=True)
    _plain_text(name, "name")
    command_list = _validate_command(list(command))
    ledger = load_ledger(ledger_path, missing_ok=True)
    started = time.perf_counter()
    try:
        completed = subprocess.run(command_list, check=False, shell=False)
        exit_code = completed.returncode
    except OSError as error:
        print(f"could not start command: {error}", file=sys.stderr)
        exit_code = 127
    duration_seconds = round(time.perf_counter() - started, 6)
    entry = {
        "command": command_list,
        "duration_seconds": duration_seconds,
        "environment": {name: os.environ.get(name) for name in ENVIRONMENT_NAMES},
        "exit_code": exit_code,
        "kind": kind,
        "lane": lane,
        "name": name,
        "outcome": "success" if exit_code == 0 else "failure",
    }
    _validate_entry(entry)
    ledger["entries"].append(entry)
    write_ledger(ledger_path, ledger)
    return exit_code


def _markdown_text(value: str) -> str:
    return value.replace("\\", "\\\\").replace("|", "\\|").replace("\r", " ").replace("\n", " ")


def render_summary(ledger: dict[str, Any]) -> str:
    """Render a compact Markdown summary for ``GITHUB_STEP_SUMMARY``."""

    validate_ledger(ledger)
    entries = ledger["entries"]
    successes = sum(entry["outcome"] == "success" for entry in entries)
    total_duration = sum(float(entry["duration_seconds"]) for entry in entries)
    lines = [
        "## CI execution ledger",
        "",
        f"{successes}/{len(entries)} commands succeeded; recorded duration: {total_duration:.3f} s.",
        "",
        "| Lane | Kind | Name | Outcome | Exit | Seconds | Parallelism | Command |",
        "| --- | --- | --- | --- | ---: | ---: | --- | --- |",
    ]
    for entry in entries:
        environment = entry["environment"]
        parallelism = ", ".join(
            f"{environment_name}={environment[environment_name] if environment[environment_name] is not None else '-'}"
            for environment_name in ENVIRONMENT_NAMES
        )
        command = " ".join(entry["command"])
        lines.append(
            "| "
            + " | ".join(
                _markdown_text(str(value))
                for value in (
                    entry["lane"],
                    entry["kind"],
                    entry["name"],
                    entry["outcome"],
                    entry["exit_code"],
                    f"{entry['duration_seconds']:.3f}",
                    parallelism,
                    command,
                )
            )
            + " |"
        )
    return "\n".join(lines) + "\n"


def _command_from_remainder(command: list[str]) -> list[str]:
    if command and command[0] == "--":
        command = command[1:]
    return _validate_command(command)


def main(argv: Sequence[str] | None = None) -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    subparsers = parser.add_subparsers(dest="action", required=True)
    run = subparsers.add_parser("run", help="run a command and append its record")
    run.add_argument("--ledger", required=True, type=Path)
    run.add_argument("--lane", required=True)
    run.add_argument("--kind", required=True)
    run.add_argument("--name", required=True)
    run.add_argument("command", nargs=argparse.REMAINDER)
    summary = subparsers.add_parser("summary", help="render the ledger as Markdown")
    summary.add_argument("--ledger", required=True, type=Path)
    summary.add_argument("--output", type=Path, help="write Markdown here instead of standard output")
    args = parser.parse_args(argv)
    if args.action == "run":
        return run_and_record(
            args.ledger,
            lane=args.lane,
            kind=args.kind,
            name=args.name,
            command=_command_from_remainder(args.command),
        )
    rendered = render_summary(load_ledger(args.ledger))
    if args.output is None:
        print(rendered, end="")
    else:
        args.output.write_text(rendered, encoding="utf-8", newline="\n")
    return 0


if __name__ == "__main__":
    try:
        raise SystemExit(main())
    except ExecutionLedgerError as error:
        print(f"execution ledger error: {error}", file=sys.stderr)
        raise SystemExit(2) from error
