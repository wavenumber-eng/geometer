"""One-pass model and analytic illustration over executable IPC."""

from __future__ import annotations

from dataclasses import dataclass
from hashlib import sha256
from typing import TYPE_CHECKING

from ._generated.contracts.codecs import decode_mesh_illustration_geometry_b0_json
from ._generated.contracts.models import (
    AnalyticIllustrationSourceA0,
    MeshIllustrationGeometryB0,
    ModelAttachmentIllustrationSourceA0,
    ModelIllustrationGeometryRequestB0,
    ModelIllustrationGeometryResultB0,
    ModelIllustrationRequestB0,
    ModelIllustrationResultB0,
    OperationFailureB0,
    OperationFailureA0,
)
from ._ipc_a0 import Attachment

if TYPE_CHECKING:
    from ._ipc_client import GeometerIpcClient, OperationResponse


@dataclass(frozen=True, slots=True)
class ModelIllustrationGeometry:
    """Governed operation metadata paired with renderer-neutral geometry."""

    metadata: ModelIllustrationGeometryResultB0
    geometry: MeshIllustrationGeometryB0


def _attachments(
    source: ModelAttachmentIllustrationSourceA0 | AnalyticIllustrationSourceA0,
    model: bytes | None,
) -> tuple[Attachment, ...]:
    if isinstance(source, ModelAttachmentIllustrationSourceA0):
        if model is None:
            raise ValueError("a model illustration source requires STEP model bytes")
        return (Attachment(name="model", media_type="application/step", data=model),)
    if model is not None:
        raise ValueError("an analytic illustration source does not accept model bytes")
    return ()


def model_illustration(
    client: GeometerIpcClient,
    request: ModelIllustrationRequestB0,
    model: bytes | None,
    timeout: float | None,
) -> ModelIllustrationResultB0:
    from ._ipc_client import GeometerIpcProtocolError, GeometerOperationError

    response = client.execute(
        "geometry.model_illustration.b0",
        request,
        _attachments(request.source, model),
        timeout=timeout,
    )
    if isinstance(response.outcome, (OperationFailureA0, OperationFailureB0)):
        raise GeometerOperationError(response.outcome.operation, response.outcome.diagnostics)
    if not isinstance(response.outcome.result, ModelIllustrationResultB0) or response.attachments:
        client._terminate()
        raise GeometerIpcProtocolError("model illustration returned an incompatible response")
    return response.outcome.result


def model_illustration_geometry(
    client: GeometerIpcClient,
    request: ModelIllustrationGeometryRequestB0,
    model: bytes | None,
    timeout: float | None,
) -> ModelIllustrationGeometry:
    from ._ipc_client import GeometerIpcProtocolError, GeometerOperationError

    response = client.execute(
        "geometry.model_illustration_geometry.b0",
        request,
        _attachments(request.source, model),
        timeout=timeout,
    )
    if isinstance(response.outcome, (OperationFailureA0, OperationFailureB0)):
        raise GeometerOperationError(response.outcome.operation, response.outcome.diagnostics)
    try:
        return _decode_geometry(response)
    except Exception as error:
        client._terminate()
        raise GeometerIpcProtocolError(f"invalid model illustration geometry response: {error}") from error


def _decode_geometry(response: OperationResponse) -> ModelIllustrationGeometry:
    if isinstance(response.outcome, (OperationFailureA0, OperationFailureB0)):
        raise ValueError("unexpected failed response")
    metadata = response.outcome.result
    if not isinstance(metadata, ModelIllustrationGeometryResultB0) or len(response.attachments) != 1:
        raise ValueError("expected one model illustration geometry attachment")
    attachment = response.attachments[0]
    if (
        attachment.name != "illustration_geometry"
        or attachment.media_type != "application/vnd.wavenumber.geometer.illustration-geometry+json"
        or len(attachment.data) != metadata.geometry.byte_length
        or sha256(attachment.data).hexdigest() != metadata.geometry.sha256
    ):
        raise ValueError("geometry attachment metadata mismatch")
    geometry = decode_mesh_illustration_geometry_b0_json(attachment.data)
    if geometry.stats != metadata.stats or geometry.warnings != metadata.warnings:
        raise ValueError("geometry statistics/warnings mismatch")
    return ModelIllustrationGeometry(metadata=metadata, geometry=geometry)
