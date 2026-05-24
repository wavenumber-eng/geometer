from __future__ import annotations

import json
import re
import subprocess
import tempfile
from collections.abc import Mapping, Sequence
from pathlib import Path
from typing import Any

from ._errors import GeometerError
from ._paths import executable_path
from ._types import HlrOptions, StepInput, Version, read_step_input


_VERSION_RE = re.compile(r"^geometer\s+(\d+)\.(\d+)\.(\d+)(?:\.(\d+))?\s+\(abi\s+(\d+)\)\s*$")


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
    abi = int(match.group(5))
    version_string = f"{major}.{minor}.{patch}"
    if match.group(4) is not None:
        version_string += f".{int(match.group(4))}"
    return Version(major=major, minor=minor, patch=patch, abi=abi, string=version_string)


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


def planar_step(request: Mapping[str, Any] | str | bytes | bytearray) -> bytes:
    with tempfile.TemporaryDirectory(prefix="geometer-python-") as directory_text:
        directory = Path(directory_text)
        request_path = directory / "planar_step_request.json"
        output_path = directory / "planar.step"
        if isinstance(request, Mapping):
            request_path.write_text(
                json.dumps(dict(request), separators=(",", ":")),
                encoding="utf-8",
            )
        elif isinstance(request, (bytes, bytearray)):
            request_path.write_bytes(bytes(request))
        else:
            request_path.write_text(str(request), encoding="utf-8")

        completed = subprocess.run(
            [str(executable_path()), "planar-step", str(request_path), str(output_path)],
            capture_output=True,
            check=False,
            encoding="utf-8",
        )
        if completed.returncode != 0:
            message = completed.stderr.strip() or completed.stdout.strip()
            raise RuntimeError(
                "geometer planar-step failed with exit code "
                f"{completed.returncode}: {message}"
            )
        return output_path.read_bytes()


def run_batch(
    jobs: Sequence[Mapping[str, Any]],
    *,
    options: HlrOptions | Mapping[str, Any] | None = None,
    work_dir: str | Path | None = None,
) -> dict[str, Any]:
    if work_dir is None:
        with tempfile.TemporaryDirectory(prefix="geometer-batch-") as directory_text:
            return _run_batch_in_directory(Path(directory_text), jobs, options=options)
    directory = Path(work_dir)
    directory.mkdir(parents=True, exist_ok=True)
    return _run_batch_in_directory(directory, jobs, options=options)


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


def _run_batch_in_directory(
    directory: Path,
    jobs: Sequence[Mapping[str, Any]],
    *,
    options: HlrOptions | Mapping[str, Any] | None,
) -> dict[str, Any]:
    request_path = directory / "request.json"
    response_path = directory / "response.json"
    request: dict[str, Any] = {
        "schema": "geometer.batch.request.a0",
        "jobs": [dict(job) for job in jobs],
    }
    options_value = _options_object(options)
    if options_value is not None:
        request["options"] = options_value
    request_path.write_text(json.dumps(request, separators=(",", ":")), encoding="utf-8")

    completed = subprocess.run(
        [str(executable_path()), "run", str(request_path), str(response_path)],
        capture_output=True,
        check=False,
        encoding="utf-8",
    )
    response = _read_response(response_path, completed)
    jobs_value = response.get("jobs")
    if completed.returncode != 0 or not response.get("ok") or not isinstance(jobs_value, list):
        response_job = jobs_value[0] if isinstance(jobs_value, list) and jobs_value else None
        response_job = response_job if isinstance(response_job, dict) else None
        raise _error_from_response(response, response_job, completed)
    for response_job in jobs_value:
        if not isinstance(response_job, dict) or not response_job.get("ok"):
            raise _error_from_response(
                response,
                response_job if isinstance(response_job, dict) else None,
                completed,
            )
    return response


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


def _options_object(options: HlrOptions | Mapping[str, Any] | None) -> dict[str, Any] | None:
    if options is None:
        return None
    if isinstance(options, HlrOptions):
        return options.to_json_value()
    if isinstance(options, Mapping):
        return dict(options)
    raise TypeError("batch options must be HlrOptions, a mapping, or None")
