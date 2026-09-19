"""Shared generated B0 illustration values through executable IPC."""

from __future__ import annotations

from typing import TYPE_CHECKING

from ._generated.contracts.codecs import (
    encode_hlr_projection_result_b0_json,
)
from ._generated.contracts.models import (
    HlrProjectionResultB0,
    MeshIllustrationInputB0,
    MeshIllustrationRequestB0,
    MeshIllustrationResultB0,
    OperationFailureB0,
    OperationFailureA0,
)
from ._ipc_a0 import Attachment
from ._illustration_input import _mesh_attachment

if TYPE_CHECKING:
    from ._ipc_client import GeometerIpcClient


def mesh_illustration(
    client: GeometerIpcClient,
    input: MeshIllustrationInputB0,
    timeout: float | None,
    hlr_projection: HlrProjectionResultB0 | None = None,
) -> MeshIllustrationResultB0:
    from ._ipc_client import GeometerIpcProtocolError, GeometerOperationError

    # Validate the complete public input with its generated codec before adapting
    # it to the governed attachment-oriented executable request.
    mesh_data = _mesh_attachment(input, "Wavenumber.Geometer.Contracts.MeshIllustrationB0.MeshIllustrationInputB0")
    request = MeshIllustrationRequestB0(
        schema="geometry.mesh_illustration.request.b0",
        view=input.view,
        prepare=input.prepare,
        style=input.style,
        svg=input.svg,
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
    response = client.execute(
        "geometry.mesh_illustration.b0",
        request,
        tuple(attachments),
        timeout=timeout,
    )
    if isinstance(response.outcome, (OperationFailureA0, OperationFailureB0)):
        raise GeometerOperationError(response.outcome.operation, response.outcome.diagnostics)
    if response.attachments or not isinstance(response.outcome.result, MeshIllustrationResultB0):
        client._terminate()
        raise GeometerIpcProtocolError("mesh illustration returned an incompatible result or attachments")
    return response.outcome.result
