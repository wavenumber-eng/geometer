from __future__ import annotations

import math
import sys
from typing import Any, cast

import pytest

from geometer import _contract_runtime as runtime


class IntSubclass(int):
    pass


class FloatSubclass(float):
    pass


def scalar_oracle(values: list[Any], name: str, path: str) -> list[Any]:
    return [
        runtime._decode_primitive(value, name, {}, runtime._child_path(path, str(index)))
        for index, value in enumerate(values)
    ]


@pytest.mark.parametrize(
    "name,values",
    [
        ("float64", [0, -0.0, 1.25, 2**53 + 1, 5e-324, sys.float_info.max]),
        ("uint32", [0, 1, 2**32 - 1]),
        ("uint64", [0, 2**53 + 1, 2**64 - 1]),
    ],
)
def test_numeric_arrays_match_scalar_normalization(name: str, values: list[Any]) -> None:
    descriptor = {"kind": "primitive", "name": name}
    expected = scalar_oracle(values, name, "/a~1b~0c")
    encoded = runtime._encode_array(tuple(values), descriptor, {}, "/a~1b~0c", {}, {}, {})
    decoded = runtime._decode_array(values, descriptor, {}, "/a~1b~0c", {}, {}, {})
    assert encoded == expected
    assert decoded == tuple(expected)
    assert [type(v) for v in encoded] == [type(v) for v in expected]
    if name == "float64":
        assert math.copysign(1, encoded[1]) == -1
        assert math.copysign(1, decoded[1]) == -1


@pytest.mark.parametrize("name", ["float64", "uint32", "uint64"])
@pytest.mark.parametrize(
    "invalid", [True, IntSubclass(1), FloatSubclass(1), "1", None, float("inf"), float("nan"), 10**400]
)
def test_numeric_arrays_preserve_exact_first_error(name: str, invalid: Any) -> None:
    values = [1, invalid, None]
    path = "/a~1b~0c"
    with pytest.raises(runtime.ContractError) as oracle:
        scalar_oracle(values, name, path)
    for operation in (runtime._encode_array, runtime._decode_array):
        with pytest.raises(runtime.ContractError) as actual:
            operation(values, {"kind": "primitive", "name": name}, {}, path, {}, {}, {})
        assert (actual.value.code, actual.value.path, actual.value.message) == (
            oracle.value.code,
            oracle.value.path,
            oracle.value.message,
        )


@pytest.mark.parametrize(
    "name,value", [("uint32", -1), ("uint32", 2**32), ("uint64", -1), ("uint64", 2**64), ("uint64", 1.5)]
)
def test_unsigned_array_bounds(name: str, value: Any) -> None:
    with pytest.raises(runtime.ContractError) as oracle:
        scalar_oracle([0, value], name, "")
    with pytest.raises(runtime.ContractError) as actual:
        runtime._numeric_array([0, value], name, "")
    assert str(actual.value) == str(oracle.value)


def test_container_and_length_errors_precede_numeric_errors() -> None:
    descriptor = {"kind": "primitive", "name": "float64"}
    for operation in (runtime._encode_array, runtime._decode_array):
        with pytest.raises(runtime.ContractError) as failure:
            operation([None], descriptor, {"min_items": 2}, "/positions", {}, {}, {})
        assert failure.value.code == "geometer.contract.array_length"
        assert failure.value.path == "/positions"
    with pytest.raises(runtime.ContractError) as failure:
        runtime._decode_array((1,), descriptor, {}, "", {}, {}, {})
    assert failure.value.code == "geometer.contract.type_mismatch"


def test_duplicate_object_inside_invalid_numeric_array_is_still_detected_first() -> None:
    declarations = {
        "Numbers": {"kind": "array", "element": {"kind": "primitive", "name": "float64"}, "constraints": {}}
    }
    with pytest.raises(runtime.ContractError) as failure:
        runtime.decode_contract_json('[1,{"a/b~c":1,"a/b~c":2}]', "Numbers", declarations, {}, {})
    assert failure.value.code == "geometer.contract.duplicate_field"
    assert failure.value.path == "/1/a~1b~0c"


@pytest.mark.parametrize("geometry", [False, True])
def test_illustration_attachment_reuses_fully_validated_meshes(geometry: bool) -> None:
    import json
    from dataclasses import replace

    from geometer._generated.contracts import codecs, models
    from geometer._illustration_input import _mesh_attachment

    suffix = "MeshIllustrationGeometryInputA0" if geometry else "MeshIllustrationInputA0"
    root = next(key for key in codecs.DECLARATIONS if key.endswith("." + suffix))
    collection_root = next(key for key in codecs.DECLARATIONS if key.endswith(".MeshCollectionA0"))
    assert (
        codecs.DECLARATIONS[root]["properties"]["meshes"]
        == codecs.DECLARATIONS[collection_root]["properties"]["meshes"]
    )
    schema = "geometry.mesh_illustration_geometry.input.a0" if geometry else "geometry.mesh_illustration.input.a0"
    payload = json.dumps(
        {
            "schema": schema,
            **({"length_unit": "millimeter"} if geometry else {}),
            "meshes": [
                {
                    "id": "test",
                    "positions": [0, -0.0, 0, 1, 0, 0, 0, 1, 0],
                    "indices": [0, 1, 2],
                    "materials": [{"color": [1, 0, 0]}],
                }
            ],
            "view": {"direction": [0, 0, 1], "up": [0, 1, 0]},
        }
    )
    value = runtime.decode_contract_json(payload, root, codecs.DECLARATIONS, models.MODEL_TYPES, models.ENUM_TYPES)
    expected = codecs.encode_mesh_collection_a0_json(
        models.MeshCollectionA0(schema="geometry.mesh_collection.a0", length_unit="millimeter", meshes=value.meshes)
    )
    assert _mesh_attachment(value, root) == expected
    assert codecs.decode_mesh_collection_a0_json(expected).meshes == value.meshes
    for invalid in (
        replace(value, schema="wrong"),
        *([replace(value, length_unit="inch")] if geometry else []),
        replace(value, view=None),
        object(),
    ):
        with pytest.raises(runtime.ContractError) as oracle:
            runtime.encode_contract_json(invalid, root, codecs.DECLARATIONS, models.MODEL_TYPES, models.ENUM_TYPES)
        with pytest.raises(runtime.ContractError) as actual:
            _mesh_attachment(cast(Any, invalid), root)
        assert str(actual.value) == str(oracle.value)
