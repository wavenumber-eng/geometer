"""One-shot generated-value APIs, using the maintained executable IPC client."""

from __future__ import annotations

from pathlib import Path

from ._generated.contracts.models import (
    HlrProjectionResultB0,
    MeshIllustrationInputB0,
    MeshIllustrationGeometryInputB0,
    MeshIllustrationGeometryB0,
    MeshIllustrationResultB0,
    ModelTessellationRequestA0,
    ModelIllustrationGeometryRequestB0,
    ModelIllustrationRequestB0,
    ModelIllustrationResultB0,
)
from ._ipc_client import GeometerIpcClient
from ._model_illustration import ModelIllustrationGeometry
from ._tessellation import ModelTessellation


def model_illustration(
    request: ModelIllustrationRequestB0,
    model: bytes | None = None,
    *,
    executable: str | Path | None = None,
    timeout: float | None = None,
) -> ModelIllustrationResultB0:
    """Illustrate one STEP model or analytic scene with one managed process."""
    with GeometerIpcClient(executable) as client:
        return client.model_illustration(request, model, timeout=timeout)


def model_illustration_geometry(
    request: ModelIllustrationGeometryRequestB0,
    model: bytes | None = None,
    *,
    executable: str | Path | None = None,
    timeout: float | None = None,
) -> ModelIllustrationGeometry:
    """Return renderer-neutral model illustration geometry with one managed process."""
    with GeometerIpcClient(executable) as client:
        return client.model_illustration_geometry(request, model, timeout=timeout)


def model_tessellation(
    model: bytes,
    options: ModelTessellationRequestA0 | None = None,
    *,
    executable: str | Path | None = None,
    timeout: float | None = None,
) -> ModelTessellation:
    """Tessellate STEP bytes with one owned executable process.

    Uses normal package executable discovery unless explicitly overridden.
    For repeated work, use GeometerClient.model_tessellation instead.
    """
    with GeometerIpcClient(executable) as client:
        return client.model_tessellation(model, options, timeout=timeout)


def mesh_illustration_geometry(
    input: MeshIllustrationGeometryInputB0,
    *,
    hlr_projection: HlrProjectionResultB0 | None = None,
    executable: str | Path | None = None,
    timeout: float | None = None,
) -> MeshIllustrationGeometryB0:
    """Return owning millimeter drawing geometry with one managed native process.

    No SVG is generated. Prefer the persistent client for repeated work.
    """
    with GeometerIpcClient(executable) as client:
        return client.mesh_illustration_geometry(input, hlr_projection=hlr_projection, timeout=timeout)


def mesh_illustration(
    input: MeshIllustrationInputB0,
    *,
    hlr_projection: HlrProjectionResultB0 | None = None,
    executable: str | Path | None = None,
    timeout: float | None = None,
) -> MeshIllustrationResultB0:
    """Render a generated illustration input to SVG with one owned process.

    Optional HLR must be visible-only polylines from the same millimeter model,
    placement and matching view. Native Geometer handles composition/mirroring.
    fuse_surfaces defaults to true. This does not compute HLR automatically.
    Timeout/error/shutdown behavior is identical to the persistent client;
    timeout is a local operation deadline, not a total process-lifetime limit.
    """
    with GeometerIpcClient(executable) as client:
        return client.mesh_illustration(input, hlr_projection=hlr_projection, timeout=timeout)
