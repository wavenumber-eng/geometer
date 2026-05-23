from __future__ import annotations

import base64
import json
import os
import subprocess
import sys
from pathlib import Path

from ._errors import GeometerError
from ._paths import bundled_occt_runtime_available
from ._types import Version


def use_worker_process() -> bool:
    override = os.environ.get("GEOMETER_PYTHON_DIRECT")
    if override is not None and override.lower() in {"1", "true", "yes", "on"}:
        return False
    if os.environ.get("GEOMETER_PYTHON_WORKER", "").lower() in {"1", "true", "yes", "on"}:
        return True
    return sys.platform == "win32" and not bundled_occt_runtime_available()


def projection_json(step_bytes: bytes, options_json: bytes | None, version: Version) -> str:
    request = _request("projection_json", step_bytes, options_json)
    response = _run_worker(request)
    if not response.get("ok"):
        raise _error_from_response(response, version)
    value = response.get("text")
    if not isinstance(value, str):
        raise RuntimeError("worker projection_json response did not contain text")
    return value


def step_to_glb(step_bytes: bytes, options_json: bytes | None, version: Version) -> bytes:
    request = _request("step_to_glb", step_bytes, options_json)
    response = _run_worker(request)
    if not response.get("ok"):
        raise _error_from_response(response, version)
    value = response.get("bytes_b64")
    if not isinstance(value, str):
        raise RuntimeError("worker step_to_glb response did not contain bytes")
    return base64.b64decode(value.encode("ascii"))


def _request(operation: str, step_bytes: bytes, options_json: bytes | None) -> dict[str, object]:
    return {
        "operation": operation,
        "step_b64": base64.b64encode(step_bytes).decode("ascii"),
        "options_json": options_json.decode("utf-8") if options_json is not None else None,
    }


def _run_worker(request: dict[str, object]) -> dict[str, object]:
    env = os.environ.copy()
    package_root = Path(__file__).resolve().parents[1]
    env["PYTHONPATH"] = _prepend_pythonpath(package_root, env.get("PYTHONPATH"))

    completed = subprocess.run(
        [sys.executable, "-m", "geometer._worker"],
        input=json.dumps(request, separators=(",", ":")),
        capture_output=True,
        check=False,
        encoding="utf-8",
        env=env,
    )
    if completed.returncode != 0:
        message = completed.stderr.strip() or completed.stdout.strip()
        raise RuntimeError(f"geometer worker failed with exit code {completed.returncode}: {message}")
    try:
        return json.loads(completed.stdout)
    except json.JSONDecodeError as exc:
        raise RuntimeError(f"geometer worker returned invalid JSON: {completed.stdout!r}") from exc


def _prepend_pythonpath(path: Path, current: str | None) -> str:
    if not current:
        return str(path)
    return str(path) + os.pathsep + current


def _error_from_response(response: dict[str, object], version: Version) -> GeometerError:
    return GeometerError(
        code=int(response.get("code", -1)),
        message=str(response.get("message", "worker call failed")),
        function=str(response.get("function", "geometer worker")),
        version=version.string,
        abi=version.abi,
    )
