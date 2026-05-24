from __future__ import annotations

import json
import os
from pathlib import Path
from typing import Any, Mapping, Sequence

from ._batch import GeometerBatchConfig, GeometerBatchResult, GeometerBatchRunner
from ._cli import projection_json as cli_projection_json
from ._cli import run_batch as cli_run_batch
from ._cli import step_to_glb as cli_step_to_glb
from ._cli import version as cli_version
from ._paths import executable_path as _executable_path
from ._types import (
    HlrOptions,
    HlrProjectionResult,
    Matrix4,
    ProjectionView,
    StepInput,
    Version,
    build_hlr_options_payload,
    encode_json_options,
)


def executable_path() -> Path:
    return _executable_path()


def run_batch(
    jobs: Sequence[Mapping[str, Any]],
    *,
    options: HlrOptions | Mapping[str, Any] | None = None,
    work_dir: str | Path | None = None,
) -> dict[str, Any]:
    _ensure_exe_backend()
    return cli_run_batch(jobs, options=options, work_dir=work_dir)


def version() -> Version:
    _ensure_exe_backend()
    return cli_version()


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
    _ensure_exe_backend()
    return cli_projection_json(step, options_json)


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
    _ensure_exe_backend()
    return cli_step_to_glb(step, options_json)


def _ensure_exe_backend() -> None:
    configured = os.environ.get("GEOMETER_BACKEND")
    if configured and configured.strip().lower() not in {"exe", "cli"}:
        raise ValueError("Geometer Python only supports the executable backend for now")
    for legacy_name in ("GEOMETER_PYTHON_DIRECT", "GEOMETER_PYTHON_WORKER"):
        if os.environ.get(legacy_name, "").lower() in {"1", "true", "yes", "on"}:
            raise ValueError("Geometer Python only supports the executable backend for now")
