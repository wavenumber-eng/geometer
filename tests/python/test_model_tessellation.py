from __future__ import annotations

from dataclasses import replace
from hashlib import sha256
from pathlib import Path

import pytest

import geometer
from geometer._generated.contracts.codecs import encode_mesh_collection_a0_json
from geometer._ipc_a0 import Attachment
from geometer._ipc_client import OperationResponse
from geometer._generated.contracts.models import OperationSuccessA0
from geometer._tessellation import _decode_response

ROOT = Path(__file__).resolve().parents[2]


def test_native_colored_tessellation_is_deterministic_and_bounded() -> None:
    model = (ROOT / "tests/fixtures/step/embedded_models/SOT-23.STEP").read_bytes()
    with geometer.GeometerClient() as client:
        first = client.model_tessellation(model)
        assert first == client.model_tessellation(model)
        assert first.mesh_collection.length_unit == "millimeter"
        assert first.metadata.triangles > 0
        assert len({mesh.materials[0].color for mesh in first.mesh_collection.meshes}) >= 2
        limited = geometer.ModelTessellationRequestA0(schema="geometry.model_tessellation.request.a0", max_triangles=1)
        with pytest.raises(geometer.GeometerOperationError) as rejected:
            client.model_tessellation(model, limited)
        assert rejected.value.diagnostics[0].code == "geometer.operation.resource_limit_exceeded"
        with pytest.raises(geometer.GeometerOperationError):
            client.model_tessellation(b"not STEP")
        assert client.model_tessellation(model) == first


def test_tessellation_response_descriptor_and_counts_are_verified() -> None:
    model = (ROOT / "tests/fixtures/step/embedded_models/SOT-23.STEP").read_bytes()
    with geometer.GeometerClient() as client:
        result = client.model_tessellation(model)
    data = encode_mesh_collection_a0_json(result.mesh_collection)
    metadata = replace(
        result.metadata,
        mesh_collection=replace(
            result.metadata.mesh_collection, sha256=sha256(data).hexdigest(), byte_length=len(data)
        ),
    )
    response = OperationResponse(
        outcome=OperationSuccessA0(operation="geometry.model_tessellation.a0", ok=True, result=metadata),
        attachments=(
            Attachment(
                name="mesh_collection", media_type="application/vnd.wavenumber.geometer.mesh-collection+json", data=data
            ),
        ),
    )
    assert _decode_response(response, metadata.source_sha256).mesh_collection == result.mesh_collection
    with pytest.raises(ValueError, match="metadata"):
        _decode_response(response, "0" * 64)
    wrong = replace(
        response, outcome=replace(response.outcome, result=replace(metadata, triangles=metadata.triangles + 1))
    )
    with pytest.raises(ValueError, match="triangle count"):
        _decode_response(wrong, metadata.source_sha256)
    corrupted = replace(response, attachments=(replace(response.attachments[0], data=b"!" + data[1:]),))
    with pytest.raises(ValueError, match="metadata"):
        _decode_response(corrupted, metadata.source_sha256)
