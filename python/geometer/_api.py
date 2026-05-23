from __future__ import annotations

import json
import os
from pathlib import Path
from typing import Any, Mapping, Sequence

from ._cli import projection_json as cli_projection_json
from ._cli import step_to_glb as cli_step_to_glb
from ._cli import version as cli_version
from ._native import native
from ._paths import executable_path as _executable_path
from ._paths import native_library_path as _native_library_path
from ._subprocess import projection_json as worker_projection_json
from ._subprocess import step_to_glb as worker_step_to_glb
from ._types import (
    HlrOptions,
    HlrProjectionResult,
    Matrix4,
    ProjectionView,
    StepInput,
    Version,
    build_hlr_options_payload,
    encode_json_options,
    read_step_input,
)


def executable_path() -> Path:
    return _executable_path()


def native_library_path() -> Path:
    return _native_library_path()


def version() -> Version:
    backend = _backend()
    if backend == "exe":
        return cli_version()
    return native().version()


def hlr_projection_json(
    step: StepInput,
    *,
    views: Sequence[ProjectionView | Mapping[str, Any]] | None = None,
    model_transform: Matrix4 | None = None,
    options: HlrOptions | Mapping[str, Any] | None = None,
) -> str:
    payload = build_hlr_options_payload(
        views=views,
        model_transform=model_transform,
        options=options,
    )
    options_json = encode_json_options(payload)
    backend = _backend()
    if backend == "exe":
        return cli_projection_json(step, options_json)
    step_bytes = read_step_input(step)
    if backend == "worker":
        return worker_projection_json(step_bytes, options_json, version())
    return native().projection_json(step_bytes, options_json)


def project_step_hlr(
    step: StepInput,
    *,
    views: Sequence[ProjectionView | Mapping[str, Any]] | None = None,
    model_transform: Matrix4 | None = None,
    options: HlrOptions | Mapping[str, Any] | None = None,
) -> HlrProjectionResult:
    text = hlr_projection_json(
        step,
        views=views,
        model_transform=model_transform,
        options=options,
    )
    return HlrProjectionResult(json.loads(text))


def step_to_glb(step: StepInput, *, options: Mapping[str, Any] | None = None) -> bytes:
    options_json = encode_json_options(options)
    backend = _backend()
    if backend == "exe":
        return cli_step_to_glb(step, options_json)
    step_bytes = read_step_input(step)
    if backend == "worker":
        return worker_step_to_glb(step_bytes, options_json, version())
    return native().step_to_glb(step_bytes, options_json)


def _backend() -> str:
    configured = os.environ.get("GEOMETER_BACKEND")
    if configured:
        backend = configured.strip().lower()
    elif os.environ.get("GEOMETER_PYTHON_DIRECT", "").lower() in {"1", "true", "yes", "on"}:
        backend = "ctypes"
    elif os.environ.get("GEOMETER_PYTHON_WORKER", "").lower() in {"1", "true", "yes", "on"}:
        backend = "worker"
    else:
        backend = "exe"

    if backend in {"exe", "cli"}:
        return "exe"
    if backend in {"ctypes", "native", "direct"}:
        return "ctypes"
    if backend == "worker":
        return "worker"
    raise ValueError("GEOMETER_BACKEND must be one of: exe, ctypes, worker")
