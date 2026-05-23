from __future__ import annotations

import json
import re
import subprocess
import tempfile
from pathlib import Path
from typing import Any

from ._errors import GeometerError
from ._paths import executable_path
from ._types import StepInput, Version, read_step_input


_VERSION_RE = re.compile(r"^geometer\s+(\d+)\.(\d+)\.(\d+)\s+\(abi\s+(\d+)\)\s*$")


def version() -> Version:
    completed = subprocess.run(
        [str(executable_path()), "--version"],
        capture_output=True,
        check=False,
        encoding="utf-8",
    )
    if completed.returncode != 0:
        message = completed.stderr.strip() or completed.stdout.strip()
        raise RuntimeError(
            f"geometer --version failed with exit code {completed.returncode}: {message}"
        )

    text = completed.stdout.strip()
    match = _VERSION_RE.match(text)
    if match is None:
        raise RuntimeError(f"geometer --version returned unexpected text: {text!r}")

    major = int(match.group(1))
    minor = int(match.group(2))
    patch = int(match.group(3))
    abi = int(match.group(4))
    return Version(major=major, minor=minor, patch=patch, abi=abi, string=f"{major}.{minor}.{patch}")


def projection_json(step: StepInput, options_json: bytes | None) -> str:
    with tempfile.TemporaryDirectory(prefix="geometer-python-") as directory_text:
        directory = Path(directory_text)
        step_path = _materialize_step(step, directory)
        output_path = directory / "projection.json"
        _run_single_job(
            directory,
            {
                "id": "projection",
                "operation": "step_hlr_projection_json",
                "step_path": str(step_path),
                "output_path": str(output_path),
                "options": _decode_options(options_json),
            },
        )
        return output_path.read_text(encoding="utf-8")


def step_to_glb(step: StepInput, options_json: bytes | None) -> bytes:
    with tempfile.TemporaryDirectory(prefix="geometer-python-") as directory_text:
        directory = Path(directory_text)
        step_path = _materialize_step(step, directory)
        output_path = directory / "model.glb"
        _run_single_job(
            directory,
            {
                "id": "glb",
                "operation": "step_to_glb",
                "step_path": str(step_path),
                "output_path": str(output_path),
                "options": _decode_options(options_json),
            },
        )
        return output_path.read_bytes()


def _run_single_job(directory: Path, job: dict[str, Any]) -> dict[str, Any]:
    request_path = directory / "request.json"
    response_path = directory / "response.json"
    request = {
        "schema": "geometer.batch.request.a0",
        "jobs": [job],
    }
    request_path.write_text(json.dumps(request, separators=(",", ":")), encoding="utf-8")

    completed = subprocess.run(
        [str(executable_path()), "run", str(request_path), str(response_path)],
        capture_output=True,
        check=False,
        encoding="utf-8",
    )
    response = _read_response(response_path, completed)
    jobs = response.get("jobs")
    response_job = jobs[0] if isinstance(jobs, list) and jobs else None
    if completed.returncode != 0 or not response.get("ok") or not isinstance(response_job, dict):
        raise _error_from_response(response, response_job, completed)
    if not response_job.get("ok"):
        raise _error_from_response(response, response_job, completed)
    return response_job


def _read_response(response_path: Path, completed: subprocess.CompletedProcess[str]) -> dict[str, Any]:
    if not response_path.exists():
        message = completed.stderr.strip() or completed.stdout.strip()
        raise RuntimeError(
            "geometer run failed with exit code "
            f"{completed.returncode} and did not write a response: {message}"
        )
    try:
        loaded = json.loads(response_path.read_text(encoding="utf-8"))
    except json.JSONDecodeError as exc:
        raise RuntimeError(f"geometer run wrote invalid response JSON: {response_path}") from exc
    if not isinstance(loaded, dict):
        raise RuntimeError(f"geometer run wrote non-object response JSON: {response_path}")
    return loaded


def _error_from_response(
    response: dict[str, Any],
    response_job: dict[str, Any] | None,
    completed: subprocess.CompletedProcess[str],
) -> GeometerError:
    source = response_job if response_job is not None else response
    message = str(
        source.get("message")
        or completed.stderr.strip()
        or completed.stdout.strip()
        or "CLI call failed"
    )
    return GeometerError(
        code=int(source.get("code", completed.returncode or -1)),
        message=message,
        function=f"geometer run {source.get('operation', 'batch')}",
        version=response.get("version") if isinstance(response.get("version"), str) else None,
        abi=response.get("abi") if isinstance(response.get("abi"), int) else None,
    )


def _materialize_step(step: StepInput, directory: Path) -> Path:
    if isinstance(step, (str, Path)):
        return Path(step).resolve()
    step_path = directory / "input.step"
    step_path.write_bytes(read_step_input(step))
    return step_path


def _decode_options(options_json: bytes | None) -> dict[str, Any]:
    if not options_json:
        return {}
    value = json.loads(options_json.decode("utf-8"))
    if value is None:
        return {}
    if not isinstance(value, dict):
        raise TypeError("Geometer CLI options must encode a JSON object")
    return value
