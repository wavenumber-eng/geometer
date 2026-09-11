"""Typed illustration drawing geometry over the existing attachment transport."""

from __future__ import annotations

from hashlib import sha256
from typing import TYPE_CHECKING

from ._generated.contracts.codecs import (
    decode_mesh_illustration_geometry_a0_json,
    encode_hlr_projection_result_a0_json,
)
from ._generated.contracts.models import (
    HlrProjectionResultA0,
    MeshIllustrationGeometryA0,
    MeshIllustrationGeometryInputA0,
    MeshIllustrationGeometryRequestA0,
    MeshIllustrationGeometryResultA0,
    OperationFailureA0,
)
from ._ipc_a0 import Attachment
from ._illustration_input import _mesh_attachment

if TYPE_CHECKING:
    from ._ipc_client import GeometerIpcClient, OperationResponse


def mesh_illustration_geometry(
    client: GeometerIpcClient,
    input: MeshIllustrationGeometryInputA0,
    timeout: float | None,
    hlr_projection: HlrProjectionResultA0 | None = None,
) -> MeshIllustrationGeometryA0:
    from ._ipc_client import GeometerIpcProtocolError, GeometerOperationError

    mesh_data = _mesh_attachment(
        input, "Wavenumber.Geometer.Contracts.MeshIllustrationGeometryA0.MeshIllustrationGeometryInputA0"
    )
    request = MeshIllustrationGeometryRequestA0(
        schema="geometry.mesh_illustration_geometry.request.a0",
        view=input.view,
        prepare=input.prepare,
        style=input.style,
    )
    attachments = [
        Attachment(
            name="mesh_collection",
            media_type="application/vnd.wavenumber.geometer.mesh-collection+json",
            data=mesh_data,
        )
    ]
    if hlr_projection is not None:
        attachments.append(
            Attachment(
                name="hlr_projection",
                media_type="application/vnd.wavenumber.geometer.hlr-projection+json",
                data=encode_hlr_projection_result_a0_json(hlr_projection),
            )
        )
    response = client.execute("geometry.mesh_illustration_geometry.a0", request, tuple(attachments), timeout=timeout)
    if isinstance(response.outcome, OperationFailureA0):
        raise GeometerOperationError(response.outcome.operation, response.outcome.diagnostics)
    try:
        return _decode_response(response)
    except Exception as error:
        client._terminate()
        raise GeometerIpcProtocolError(f"invalid illustration geometry response: {error}") from error


def _decode_response(response: OperationResponse) -> MeshIllustrationGeometryA0:
    if isinstance(response.outcome, OperationFailureA0):
        raise ValueError("unexpected failed response")
    metadata = response.outcome.result
    if not isinstance(metadata, MeshIllustrationGeometryResultA0) or len(response.attachments) != 1:
        raise ValueError("expected an illustration geometry result with one attachment")
    attachment = response.attachments[0]
    if (
        attachment.name != "illustration_geometry"
        or attachment.media_type != "application/vnd.wavenumber.geometer.illustration-geometry+json"
        or len(attachment.data) != metadata.geometry.byte_length
        or sha256(attachment.data).hexdigest() != metadata.geometry.sha256
    ):
        raise ValueError("geometry attachment metadata mismatch")
    geometry = decode_mesh_illustration_geometry_a0_json(attachment.data)
    if geometry.stats != metadata.stats or geometry.warnings != metadata.warnings:
        raise ValueError("geometry statistics/warnings mismatch")
    if any(a > b for a, b in zip(geometry.bounds.min, geometry.bounds.max)):
        raise ValueError("geometry bounds are reversed")
    layers = rings = points = 0
    for surface in geometry.surfaces:
        layers += len(surface.layers)
        for layer in surface.layers:
            rings += len(layer.rings)
            points += sum(len(ring.points) for ring in layer.rings)
            if layers > 2000000 or rings > 2000000 or points > 6000000:
                raise ValueError("geometry exceeds aggregate count limit")
    if layers != geometry.stats.surface_draws or layers + len(geometry.lines) != geometry.stats.commands:
        raise ValueError("geometry draw counts mismatch")
    return geometry
