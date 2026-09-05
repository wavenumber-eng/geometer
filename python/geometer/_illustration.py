"""Shared generated illustration A0 values through the executable IPC client."""

from __future__ import annotations

from typing import TYPE_CHECKING

from ._generated.contracts.codecs import encode_mesh_collection_a0_json, encode_mesh_illustration_input_a0_json
from ._generated.contracts.models import (
    MeshCollectionA0,
    MeshIllustrationInputA0,
    MeshIllustrationRequestA0,
    MeshIllustrationResultA0,
    OperationFailureA0,
)
from ._ipc_a0 import Attachment

if TYPE_CHECKING:
    from ._ipc_client import GeometerIpcClient


def mesh_illustration(
    client: GeometerIpcClient, input: MeshIllustrationInputA0, timeout: float | None
) -> MeshIllustrationResultA0:
    from ._ipc_client import GeometerIpcProtocolError, GeometerOperationError

    # Validate the complete public input with its generated codec before adapting
    # it to the governed attachment-oriented executable request.
    encode_mesh_illustration_input_a0_json(input)
    request = MeshIllustrationRequestA0(
        schema="geometry.mesh_illustration.request.a0",
        view=input.view,
        prepare=input.prepare,
        style=input.style,
        svg=input.svg,
    )
    collection = MeshCollectionA0(schema="geometry.mesh_collection.a0", length_unit="millimeter", meshes=input.meshes)
    response = client.execute(
        "geometry.mesh_illustration.a0",
        request,
        (
            Attachment(
                name="mesh_collection",
                media_type="application/vnd.wavenumber.geometer.mesh-collection+json",
                data=encode_mesh_collection_a0_json(collection),
            ),
        ),
        timeout=timeout,
    )
    if isinstance(response.outcome, OperationFailureA0):
        raise GeometerOperationError(response.outcome.operation, response.outcome.diagnostics)
    if response.attachments or not isinstance(response.outcome.result, MeshIllustrationResultA0):
        client._terminate()
        raise GeometerIpcProtocolError("mesh illustration returned an incompatible result or attachments")
    return response.outcome.result
