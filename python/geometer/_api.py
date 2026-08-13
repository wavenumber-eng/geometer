from __future__ import annotations

import json
import os
from pathlib import Path
from typing import Any, Mapping, Sequence

from ._batch import (
    GeometerBatchConfig as GeometerBatchConfig,
    GeometerBatchResult as GeometerBatchResult,
    GeometerBatchRunner as GeometerBatchRunner,
)
from ._cli import model_bounds_json as cli_model_bounds_json
from ._cli import model_projection_json as cli_model_projection_json
from ._cli import model_to_glb as cli_model_to_glb
from ._cli import planar_batch_solve_json as cli_planar_batch_solve_json
from ._cli import planar_step as cli_planar_step
from ._cli import run_batch as cli_run_batch
from ._cli import step_to_glb as cli_step_to_glb
from ._cli import version as cli_version
from ._paths import executable_path as _executable_path
from ._contract_runtime import contract_to_json_value
from ._generated.contracts.codecs import (
    decode_model_bounds_options_a0_json,
    decode_model_bounds_result_a0_json,
    encode_model_bounds_options_a0_json,
)
from ._types import (
    HlrOptions,
    HlrProjectionResult,
    Matrix4,
    ModelBoundsResult,
    ModelInput,
    PlanarBatchInput,
    PlanarBatchSolveResult,
    ProjectionView,
    StepInput,
    Version,
    build_hlr_options_payload,
    build_model_options_payload,
    encode_json_options,
    normalize_model_format,
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


def model_hlr_projection_json(
    model: ModelInput,
    *,
    format: str = "step",
    views: Sequence[ProjectionView | Mapping[str, Any]] | None = None,
    model_transform: Matrix4 | None = None,
    options: HlrOptions | Mapping[str, Any] | None = None,
) -> str:
    normalized_format = normalize_model_format(format)
    payload = build_hlr_options_payload(
        views=views,
        model_transform=model_transform,
        options=options,
    )
    payload["format"] = normalized_format
    options_json = encode_json_options(payload)
    _ensure_exe_backend()
    return cli_model_projection_json(model, options_json)


def hlr_projection_json(
    step: StepInput,
    *,
    views: Sequence[ProjectionView | Mapping[str, Any]] | None = None,
    model_transform: Matrix4 | None = None,
    options: HlrOptions | Mapping[str, Any] | None = None,
) -> str:
    return model_hlr_projection_json(
        step,
        format="step",
        views=views,
        model_transform=model_transform,
        options=options,
    )


def project_model_hlr(
    model: ModelInput,
    *,
    format: str = "step",
    views: Sequence[ProjectionView | Mapping[str, Any]] | None = None,
    model_transform: Matrix4 | None = None,
    options: HlrOptions | Mapping[str, Any] | None = None,
) -> HlrProjectionResult:
    text = model_hlr_projection_json(
        model,
        format=format,
        views=views,
        model_transform=model_transform,
        options=options,
    )
    return HlrProjectionResult(json.loads(text))


def project_step_hlr(
    step: StepInput,
    *,
    views: Sequence[ProjectionView | Mapping[str, Any]] | None = None,
    model_transform: Matrix4 | None = None,
    options: HlrOptions | Mapping[str, Any] | None = None,
) -> HlrProjectionResult:
    return project_model_hlr(
        step,
        format="step",
        views=views,
        model_transform=model_transform,
        options=options,
    )


def model_bounds_json(
    model: ModelInput,
    *,
    format: str = "step",
    model_transform: Matrix4 | None = None,
    options: Mapping[str, Any] | None = None,
) -> str:
    payload = build_model_options_payload(
        format=format,
        model_transform=model_transform,
        options=options,
    )
    options_json = encode_model_bounds_options_a0_json(
        decode_model_bounds_options_a0_json(encode_json_options(payload) or b"{}")
    )
    _ensure_exe_backend()
    text = cli_model_bounds_json(model, options_json)
    decode_model_bounds_result_a0_json(text)
    return text


def model_bounds(
    model: ModelInput,
    *,
    format: str = "step",
    model_transform: Matrix4 | None = None,
    options: Mapping[str, Any] | None = None,
) -> ModelBoundsResult:
    text = model_bounds_json(
        model,
        format=format,
        model_transform=model_transform,
        options=options,
    )
    generated = decode_model_bounds_result_a0_json(text)
    data = contract_to_json_value(generated)
    if not isinstance(data, dict):
        raise RuntimeError("generated model-bounds result did not project to an object")
    return ModelBoundsResult(data)


def model_to_glb(
    model: ModelInput,
    *,
    format: str = "step",
    options: Mapping[str, Any] | None = None,
) -> bytes:
    normalized_format = normalize_model_format(format)
    payload = dict(options or {})
    payload["format"] = normalized_format
    options_json = encode_json_options(payload)
    _ensure_exe_backend()
    return cli_model_to_glb(model, options_json)


def step_to_glb(step: StepInput, *, options: Mapping[str, Any] | None = None) -> bytes:
    options_json = encode_json_options(options)
    _ensure_exe_backend()
    return cli_step_to_glb(step, options_json)


def planar_step(request: Mapping[str, Any] | str | bytes | bytearray) -> bytes:
    _ensure_exe_backend()
    return cli_planar_step(request)


def planar_batch_solve_json(request: PlanarBatchInput) -> str:
    _ensure_exe_backend()
    return cli_planar_batch_solve_json(request)


def planar_batch_solve(request: PlanarBatchInput) -> PlanarBatchSolveResult:
    loaded = json.loads(planar_batch_solve_json(request))
    if not isinstance(loaded, dict):
        raise RuntimeError("geometer planar-batch-solve returned non-object JSON")
    return PlanarBatchSolveResult.from_json_value(loaded)


def write_planar_step(
    request: Mapping[str, Any] | str | bytes | bytearray,
    output_path: str | Path,
) -> Path:
    step_bytes = planar_step(request)
    path = Path(output_path)
    path.parent.mkdir(parents=True, exist_ok=True)
    path.write_bytes(step_bytes)
    return path


def _ensure_exe_backend() -> None:
    configured = os.environ.get("GEOMETER_BACKEND")
    if configured and configured.strip().lower() not in {"exe", "cli"}:
        raise ValueError("Geometer Python only supports the executable backend for now")
    for legacy_name in ("GEOMETER_PYTHON_DIRECT", "GEOMETER_PYTHON_WORKER"):
        if os.environ.get(legacy_name, "").lower() in {"1", "true", "yes", "on"}:
            raise ValueError("Geometer Python only supports the executable backend for now")
