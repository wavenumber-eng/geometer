"""Typed illustration drawing geometry over the existing attachment transport."""

from __future__ import annotations

from hashlib import sha256
from typing import TYPE_CHECKING

from ._generated.contracts.codecs import (
    decode_mesh_illustration_geometry_b0_json,
    encode_hlr_projection_result_b0_json,
)
from ._generated.contracts.models import (
    HlrProjectionResultB0,
    MeshIllustrationGeometryB0,
    MeshIllustrationGeometryInputB0,
    MeshIllustrationGeometryRequestB0,
    MeshIllustrationGeometryResultB0,
    OperationFailureB0,
    OperationFailureA0,
)
from ._ipc_a0 import Attachment
from ._illustration_input import _mesh_attachment

if TYPE_CHECKING:
    from ._ipc_client import GeometerIpcClient, OperationResponse


def mesh_illustration_geometry(
    client: GeometerIpcClient,
    input: MeshIllustrationGeometryInputB0,
    timeout: float | None,
    hlr_projection: HlrProjectionResultB0 | None = None,
) -> MeshIllustrationGeometryB0:
    from ._ipc_client import GeometerIpcProtocolError, GeometerOperationError

    mesh_data = _mesh_attachment(
        input, "Wavenumber.Geometer.Contracts.MeshIllustrationGeometryB0.MeshIllustrationGeometryInputB0"
    )
    request = MeshIllustrationGeometryRequestB0(
        schema="geometry.mesh_illustration_geometry.request.b0",
        view=input.view,
        prepare=input.prepare,
        style=input.style,
        clipping=input.clipping,
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
                data=encode_hlr_projection_result_b0_json(hlr_projection),
            )
        )
    response = client.execute("geometry.mesh_illustration_geometry.b0", request, tuple(attachments), timeout=timeout)
    if isinstance(response.outcome, (OperationFailureA0, OperationFailureB0)):
        raise GeometerOperationError(response.outcome.operation, response.outcome.diagnostics)
    try:
        return _decode_response(response)
    except Exception as error:
        client._terminate()
        raise GeometerIpcProtocolError(f"invalid illustration geometry response: {error}") from error


def _decode_response(response: OperationResponse) -> MeshIllustrationGeometryB0:
    if isinstance(response.outcome, (OperationFailureA0, OperationFailureB0)):
        raise ValueError("unexpected failed response")
    metadata = response.outcome.result
    if not isinstance(metadata, MeshIllustrationGeometryResultB0) or len(response.attachments) != 1:
        raise ValueError("expected an illustration geometry result with one attachment")
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
    if geometry.bounds is not None and any(a > b for a, b in zip(geometry.bounds.min, geometry.bounds.max)):
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
