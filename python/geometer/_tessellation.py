"""Typed STEP tessellation facade and mesh-attachment validation."""

from __future__ import annotations

from dataclasses import dataclass
from hashlib import sha256
from typing import TYPE_CHECKING

from ._generated.contracts.codecs import decode_mesh_collection_a0_json
from ._generated.contracts.models import (
    MeshCollectionA0,
    ModelTessellationRequestA0,
    ModelTessellationResultA0,
    OperationFailureA0,
)
from ._ipc_a0 import Attachment

if TYPE_CHECKING:
    from ._ipc_client import GeometerIpcClient, OperationResponse


@dataclass(frozen=True)
class ModelTessellation:
    metadata: ModelTessellationResultA0
    mesh_collection: MeshCollectionA0


def model_tessellation(
    client: GeometerIpcClient,
    model: bytes,
    options: ModelTessellationRequestA0 | None,
    timeout: float | None,
) -> ModelTessellation:
    from ._ipc_client import GeometerIpcProtocolError, GeometerOperationError

    response = client.execute(
        "geometry.model_tessellation.a0",
        options or ModelTessellationRequestA0(schema="geometry.model_tessellation.request.a0"),
        (Attachment(name="model", media_type="application/step", data=model),),
        timeout=timeout,
    )
    if isinstance(response.outcome, OperationFailureA0):
        raise GeometerOperationError(response.outcome.operation, response.outcome.diagnostics)
    try:
        result = _decode_response(response, sha256(model).hexdigest())
        maximum = options.max_triangles if options and options.max_triangles is not None else 750000
        if result.metadata.triangles > maximum:
            raise ValueError("result exceeds requested triangle limit")
        return result
    except Exception as error:
        client._terminate()
        raise GeometerIpcProtocolError(f"invalid model_tessellation response: {error}") from error


def _decode_response(response: OperationResponse, source_hash: str) -> ModelTessellation:
    if isinstance(response.outcome, OperationFailureA0):
        raise ValueError("unexpected failed response")
    result = response.outcome.result
    if not isinstance(result, ModelTessellationResultA0) or len(response.attachments) != 1:
        raise ValueError("expected a tessellation result with one attachment")
    attachment = response.attachments[0]
    if (
        attachment.name != "mesh_collection"
        or attachment.media_type != "application/vnd.wavenumber.geometer.mesh-collection+json"
        or len(attachment.data) != result.mesh_collection.byte_length
        or sha256(attachment.data).hexdigest() != result.mesh_collection.sha256
        or source_hash != result.source_sha256
    ):
        raise ValueError("attachment/source metadata mismatch")
    collection = decode_mesh_collection_a0_json(attachment.data)
    if len(collection.meshes) != result.meshes:
        raise ValueError("mesh count mismatch")
    triangles = 0
    vertices = 0
    identities: set[str] = set()
    for mesh in collection.meshes:
        if (
            mesh.indices is None
            or len(mesh.indices) % 3
            or len(mesh.positions) % 3
            or any(index >= len(mesh.positions) // 3 for index in mesh.indices)
            or (mesh.normals is not None and len(mesh.normals) != len(mesh.positions))
        ):
            raise ValueError("invalid indexed tessellation layout")
        if mesh.id in identities:
            raise ValueError("duplicate mesh identity")
        identities.add(mesh.id)
        if mesh.triangle_material_indices is not None and (
            len(mesh.triangle_material_indices) != len(mesh.indices) // 3
            or any(index >= len(mesh.materials) for index in mesh.triangle_material_indices)
        ):
            raise ValueError("invalid triangle material layout")
        triangles += len(mesh.indices) // 3
        vertices += len(mesh.positions) // 3
    if vertices > 2000000:
        raise ValueError("collection vertex limit exceeded")
    if triangles != result.triangles:
        raise ValueError("triangle count mismatch")
    return ModelTessellation(result, collection)
