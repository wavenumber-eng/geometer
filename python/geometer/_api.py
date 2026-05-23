from __future__ import annotations

import json
from pathlib import Path
from typing import Any, Mapping, Sequence

from ._native import native
from ._paths import native_library_path as _native_library_path
from ._subprocess import projection_json as worker_projection_json
from ._subprocess import step_to_glb as worker_step_to_glb
from ._subprocess import use_worker_process
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


def native_library_path() -> Path:
    return _native_library_path()


def version() -> Version:
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
    step_bytes = read_step_input(step)
    options_json = encode_json_options(payload)
    if use_worker_process():
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
    step_bytes = read_step_input(step)
    options_json = encode_json_options(options)
    if use_worker_process():
        return worker_step_to_glb(step_bytes, options_json, version())
    return native().step_to_glb(step_bytes, options_json)
