"""One-shot generated-value APIs, using the maintained executable IPC client."""

from __future__ import annotations

from pathlib import Path

from ._generated.contracts.models import (
    HlrProjectionResultA0,
    MeshIllustrationInputA0,
    MeshIllustrationResultA0,
    ModelTessellationRequestA0,
)
from ._ipc_client import GeometerIpcClient
from ._tessellation import ModelTessellation


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


def mesh_illustration(
    input: MeshIllustrationInputA0,
    *,
    hlr_projection: HlrProjectionResultA0 | None = None,
    executable: str | Path | None = None,
    timeout: float | None = None,
) -> MeshIllustrationResultA0:
    """Render a generated illustration input to SVG with one owned process.

    Optional HLR must be visible-only polylines from the same millimeter model,
    placement and matching view. Native Geometer handles composition/mirroring.
    fuse_surfaces defaults to true. This does not compute HLR automatically.
    Timeout/error/shutdown behavior is identical to the persistent client;
    timeout is a local operation deadline, not a total process-lifetime limit.
    """
    with GeometerIpcClient(executable) as client:
        return client.mesh_illustration(input, hlr_projection=hlr_projection, timeout=timeout)
