from __future__ import annotations

import json
from pathlib import Path
from typing import Any, cast

import pytest

from geometer._contract_runtime import ContractError
from geometer._generated.contracts.codecs import (
    ROOT_DECODERS,
    decode_model_bounds_options_a0_json,
    decode_model_bounds_result_a0_json,
    encode_model_bounds_options_a0_json,
)
from geometer._generated.contracts.models import Matrix4x4, ModelBoundsOptionsA0, ModelFormat


ROOT = Path(__file__).resolve().parents[2]
VECTOR_ROOT = ROOT / "tests" / "contracts" / "vectors"


def test_generated_python_replays_all_governed_contract_vectors() -> None:
    manifest = json.loads((VECTOR_ROOT / "manifest.json").read_text(encoding="utf-8"))
    assert len(manifest["vectors"]) == 20
    for vector in manifest["vectors"]:
        decoder = ROOT_DECODERS[vector["contract_identity"]]
        data = _vector_bytes(vector)
        if vector["expected"] == "reject":
            with pytest.raises(ContractError):
                decoder(data)
            continue
        decoded = decoder(data)
        if vector.get("oracle") == "presence_projection":
            for field, expected in vector["expected_value"].items():
                assert (getattr(decoded, field) is not None) == (expected == "present")


def test_generated_python_model_bounds_codecs_are_strict_and_presence_aware() -> None:
    empty = decode_model_bounds_options_a0_json(b"{}")
    explicit = decode_model_bounds_options_a0_json(b'{"format":"step"}')
    assert empty.format is None
    assert explicit.format is ModelFormat.STEP
    assert encode_model_bounds_options_a0_json(empty) == b"{}"
    assert encode_model_bounds_options_a0_json(explicit) == b'{"format":"step"}'

    with pytest.raises(ContractError) as duplicate:
        decode_model_bounds_options_a0_json(b'{"format":"step","format":"step"}')
    assert duplicate.value.code == "geometer.contract.duplicate_field"
    assert duplicate.value.path == "/format"

    with pytest.raises(ContractError) as nested_duplicate:
        decode_model_bounds_result_a0_json(
            b'{"schema":"geometry.model_bounds.a0","units":"mm",'
            b'"source":{"format":"step","format":"step","hash":"x"},'
            b'"bounds":{"min":[0,0,0],"max":[1,1,1],"size":[1,1,1]},'
            b'"timings":{"model_read_ms":0,"bounds_ms":0}}'
        )
    assert nested_duplicate.value.path == "/source/format"

    with pytest.raises(ContractError) as nonstandard_number:
        decode_model_bounds_options_a0_json(b'{"model_transform":[NaN]}')
    assert nonstandard_number.value.code == "geometer.contract.invalid_json"

    enormous = 10**400
    enormous_matrix = [enormous, *([0] * 15)]
    with pytest.raises(ContractError) as decode_overflow:
        decode_model_bounds_options_a0_json(
            json.dumps({"model_transform": enormous_matrix}, separators=(",", ":")).encode("ascii")
        )
    assert decode_overflow.value.code == "geometer.contract.number_range"
    assert decode_overflow.value.path == "/model_transform/0"

    with pytest.raises(ContractError) as encode_overflow:
        encode_model_bounds_options_a0_json(
            ModelBoundsOptionsA0(format=None, model_transform=cast(Matrix4x4, tuple(enormous_matrix)))
        )
    assert encode_overflow.value.code == "geometer.contract.number_range"
    assert encode_overflow.value.path == "/model_transform/0"

    with pytest.raises(ContractError) as unknown:
        decode_model_bounds_result_a0_json((VECTOR_ROOT / "cases" / "result-unknown-field.json").read_bytes())
    assert unknown.value.code == "geometer.contract.unknown_field"


def _vector_bytes(vector: dict[str, Any]) -> bytes:
    path = VECTOR_ROOT / vector["file"]
    if path.suffix == ".hex":
        return bytes.fromhex(path.read_text(encoding="ascii"))
    return path.read_bytes()
