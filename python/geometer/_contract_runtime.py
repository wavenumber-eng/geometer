from __future__ import annotations

import dataclasses
from collections.abc import Mapping
from enum import Enum
import json
import math
from typing import Any, NoReturn


class ContractError(ValueError):
    def __init__(self, code: str, path: str, message: str) -> None:
        super().__init__(f"{code} at {path or '/'}: {message}")
        self.code = code
        self.path = path
        self.message = message


@dataclasses.dataclass(frozen=True, slots=True)
class _JsonObject:
    pairs: tuple[tuple[str, Any], ...]


def decode_contract_json(
    data: str | bytes | bytearray | memoryview,
    root: str,
    declarations: Mapping[str, Mapping[str, Any]],
    model_types: Mapping[str, type[Any]],
    enum_types: Mapping[str, type[Enum]],
) -> Any:
    try:
        text = bytes(data).decode("utf-8") if not isinstance(data, str) else data
    except UnicodeDecodeError as error:
        raise ContractError("geometer.contract.invalid_utf8", "", "JSON is not valid UTF-8.") from error
    try:
        value = json.loads(text, object_pairs_hook=_json_object, parse_constant=_invalid_constant)
        _validate_json_objects(value, "")
    except ContractError:
        raise
    except json.JSONDecodeError as error:
        raise ContractError("geometer.contract.invalid_json", "", "JSON is not one complete value.") from error
    return _decode_value(value, {"kind": "reference", "target": root}, {}, "", declarations, model_types, enum_types)


def encode_contract_json(
    value: Any,
    root: str,
    declarations: Mapping[str, Mapping[str, Any]],
    model_types: Mapping[str, type[Any]],
    enum_types: Mapping[str, type[Enum]],
) -> bytes:
    encoded = _encode_value(value, {"kind": "reference", "target": root}, {}, "", declarations, model_types, enum_types)
    return json.dumps(encoded, ensure_ascii=False, allow_nan=False, separators=(",", ":")).encode("utf-8")


def contract_to_json_value(value: Any) -> Any:
    if dataclasses.is_dataclass(value) and not isinstance(value, type):
        return {field.name: contract_to_json_value(getattr(value, field.name)) for field in dataclasses.fields(value)}
    if isinstance(value, Enum):
        return value.value
    if isinstance(value, tuple):
        return [contract_to_json_value(item) for item in value]
    return value


def _json_object(pairs: list[tuple[str, Any]]) -> _JsonObject:
    return _JsonObject(tuple(pairs))


def _invalid_constant(_value: str) -> None:
    raise ContractError("geometer.contract.invalid_json", "", "JSON numeric values must be finite.")


def _validate_json_objects(value: Any, path: str) -> None:
    if isinstance(value, _JsonObject):
        names: set[str] = set()
        for key, item in value.pairs:
            if key in names:
                _fail("geometer.contract.duplicate_field", _child_path(path, key), "Duplicate object field.")
            names.add(key)
            _validate_json_objects(item, _child_path(path, key))
    elif isinstance(value, list):
        for index, item in enumerate(value):
            _validate_json_objects(item, _child_path(path, str(index)))


def _decode_value(
    value: Any,
    type_descriptor: Mapping[str, Any],
    constraints: Mapping[str, Any],
    path: str,
    declarations: Mapping[str, Mapping[str, Any]],
    model_types: Mapping[str, type[Any]],
    enum_types: Mapping[str, type[Enum]],
) -> Any:
    kind = type_descriptor["kind"]
    if kind == "reference":
        target = str(type_descriptor["target"])
        declaration = declarations[target]
        if declaration["kind"] == "enum":
            decoded = _decode_enum(value, declaration, path)
            return enum_types[target](decoded)
        if declaration["kind"] == "union":
            return _decode_union(value, declaration, path, declarations, model_types, enum_types)
        if declaration["kind"] == "array":
            return _decode_array(
                value, declaration["element"], declaration["constraints"], path, declarations, model_types, enum_types
            )
        return _decode_object(value, target, declaration, path, declarations, model_types, enum_types)
    if kind == "primitive":
        return _decode_primitive(value, str(type_descriptor["name"]), constraints, path)
    if kind == "literal":
        if type(value) is not type(type_descriptor["value"]) or value != type_descriptor["value"]:
            _fail("geometer.contract.literal_mismatch", path, "Literal value does not match the contract.")
        return value
    if kind == "array":
        return _decode_array(
            value, type_descriptor["element"], constraints, path, declarations, model_types, enum_types
        )
    _fail("geometer.contract.unsupported_type", path, f"Unsupported type {kind}.")


def _decode_object(
    value: Any,
    target: str,
    declaration: Mapping[str, Any],
    path: str,
    declarations: Mapping[str, Mapping[str, Any]],
    model_types: Mapping[str, type[Any]],
    enum_types: Mapping[str, type[Enum]],
) -> Any:
    if not isinstance(value, _JsonObject):
        _fail("geometer.contract.type_mismatch", path, "Expected an object.")
    object_value: dict[str, Any] = {}
    for key, item in value.pairs:
        if key in object_value:
            _fail("geometer.contract.duplicate_field", _child_path(path, key), "Duplicate object field.")
        object_value[key] = item
    properties = declaration["properties"]
    unknown = set(object_value) - set(properties)
    if unknown:
        key = sorted(unknown)[0]
        _fail("geometer.contract.unknown_field", _child_path(path, key), "Unknown object field.")
    fields: dict[str, Any] = {}
    for name, descriptor in properties.items():
        field_name = descriptor["field"]
        if name not in object_value:
            if descriptor["optional"]:
                fields[field_name] = None
                continue
            _fail("geometer.contract.missing_field", _child_path(path, name), "Required field is absent.")
        if object_value[name] is None:
            _fail("geometer.contract.type_mismatch", _child_path(path, name), "Null is not allowed.")
        fields[field_name] = _decode_value(
            object_value[name],
            descriptor["type"],
            descriptor["constraints"],
            _child_path(path, name),
            declarations,
            model_types,
            enum_types,
        )
    return model_types[target](**fields)


def _decode_union(
    value: Any,
    declaration: Mapping[str, Any],
    path: str,
    declarations: Mapping[str, Mapping[str, Any]],
    model_types: Mapping[str, type[Any]],
    enum_types: Mapping[str, type[Enum]],
) -> Any:
    matches: list[Any] = []
    for variant in declaration["variants"]:
        try:
            matches.append(_decode_value(value, variant, {}, path, declarations, model_types, enum_types))
        except ContractError:
            continue
    if len(matches) != 1:
        _fail("geometer.contract.union_mismatch", path, "Value must match exactly one union variant.")
    return matches[0]


def _decode_array(
    value: Any,
    element: Mapping[str, Any],
    constraints: Mapping[str, Any],
    path: str,
    declarations: Mapping[str, Mapping[str, Any]],
    model_types: Mapping[str, type[Any]],
    enum_types: Mapping[str, type[Enum]],
) -> tuple[Any, ...]:
    if not isinstance(value, list):
        _fail("geometer.contract.type_mismatch", path, "Expected an array.")
    _check_size(len(value), constraints.get("min_items"), constraints.get("max_items"), path, "array")
    return tuple(
        _decode_value(item, element, {}, _child_path(path, str(index)), declarations, model_types, enum_types)
        for index, item in enumerate(value)
    )


def _decode_enum(value: Any, declaration: Mapping[str, Any], path: str) -> str:
    if not isinstance(value, str) or value not in declaration["values"]:
        _fail("geometer.contract.enum_mismatch", path, "String is not an allowed enum value.")
    return value


def _decode_primitive(value: Any, name: str, constraints: Mapping[str, Any], path: str) -> Any:
    if name == "string":
        if not isinstance(value, str):
            _fail("geometer.contract.type_mismatch", path, "Expected a string.")
        try:
            size = len(value.encode("utf-8"))
        except UnicodeEncodeError:
            _fail("geometer.contract.invalid_utf8", path, "String is not valid Unicode scalar text.")
        _check_size(size, constraints.get("min_length"), constraints.get("max_length"), path, "string")
        return value
    if name == "boolean":
        if type(value) is not bool:
            _fail("geometer.contract.type_mismatch", path, "Expected a boolean.")
        return value
    if name in {"uint32", "uint64"}:
        maximum = min(constraints.get("max_value", 2**64 - 1), 2**32 - 1 if name == "uint32" else 2**64 - 1)
        if type(value) is not int or value < constraints.get("min_value", 0) or value > maximum:
            _fail("geometer.contract.number_range", path, f"Expected an unsigned {name[4:]}-bit integer.")
        return value
    if name == "float64":
        if type(value) not in {int, float}:
            _fail("geometer.contract.type_mismatch", path, "Expected a finite number.")
        try:
            number = float(value)
        except OverflowError:
            _fail("geometer.contract.number_range", path, "Number is outside the float64 range.")
        if not math.isfinite(number):
            _fail("geometer.contract.number_range", path, "Number is outside the finite float64 range.")
        if number < constraints.get("min_value", -math.inf) or number > constraints.get("max_value", math.inf):
            _fail("geometer.contract.number_range", path, "Number is outside its contract bounds.")
        return number
    _fail("geometer.contract.unsupported_type", path, f"Unsupported primitive {name}.")


def _encode_value(
    value: Any,
    type_descriptor: Mapping[str, Any],
    constraints: Mapping[str, Any],
    path: str,
    declarations: Mapping[str, Mapping[str, Any]],
    model_types: Mapping[str, type[Any]],
    enum_types: Mapping[str, type[Enum]],
) -> Any:
    kind = type_descriptor["kind"]
    if kind == "reference":
        target = str(type_descriptor["target"])
        declaration = declarations[target]
        if declaration["kind"] == "enum":
            raw = value.value if isinstance(value, Enum) else value
            _decode_enum(raw, declaration, path)
            return raw
        if declaration["kind"] == "union":
            return _encode_union(value, declaration, path, declarations, model_types, enum_types)
        if declaration["kind"] == "array":
            return _encode_array(
                value, declaration["element"], declaration["constraints"], path, declarations, model_types, enum_types
            )
        if not isinstance(value, model_types[target]):
            _fail("geometer.contract.type_mismatch", path, f"Expected {model_types[target].__name__}.")
        encoded: dict[str, Any] = {}
        for name, descriptor in declaration["properties"].items():
            item = getattr(value, descriptor["field"])
            if item is None and descriptor["optional"]:
                continue
            if item is None:
                _fail("geometer.contract.missing_field", _child_path(path, name), "Required field is absent.")
            encoded[name] = _encode_value(
                item,
                descriptor["type"],
                descriptor["constraints"],
                _child_path(path, name),
                declarations,
                model_types,
                enum_types,
            )
        return encoded
    if kind == "primitive":
        return _decode_primitive(value, str(type_descriptor["name"]), constraints, path)
    if kind == "literal":
        return _decode_value(value, type_descriptor, constraints, path, declarations, model_types, enum_types)
    if kind == "array":
        return _encode_array(
            value, type_descriptor["element"], constraints, path, declarations, model_types, enum_types
        )
    _fail("geometer.contract.unsupported_type", path, f"Unsupported type {kind}.")


def _encode_union(
    value: Any,
    declaration: Mapping[str, Any],
    path: str,
    declarations: Mapping[str, Mapping[str, Any]],
    model_types: Mapping[str, type[Any]],
    enum_types: Mapping[str, type[Enum]],
) -> Any:
    matches: list[Any] = []
    for variant in declaration["variants"]:
        try:
            matches.append(_encode_value(value, variant, {}, path, declarations, model_types, enum_types))
        except ContractError:
            continue
    if len(matches) != 1:
        _fail("geometer.contract.union_mismatch", path, "Value must match exactly one union variant.")
    return matches[0]


def _encode_array(
    value: Any,
    element: Mapping[str, Any],
    constraints: Mapping[str, Any],
    path: str,
    declarations: Mapping[str, Mapping[str, Any]],
    model_types: Mapping[str, type[Any]],
    enum_types: Mapping[str, type[Enum]],
) -> list[Any]:
    if not isinstance(value, (tuple, list)):
        _fail("geometer.contract.type_mismatch", path, "Expected an array.")
    _check_size(len(value), constraints.get("min_items"), constraints.get("max_items"), path, "array")
    return [
        _encode_value(item, element, {}, _child_path(path, str(index)), declarations, model_types, enum_types)
        for index, item in enumerate(value)
    ]


def _check_size(size: int, minimum: int | None, maximum: int | None, path: str, label: str) -> None:
    if minimum is not None and size < minimum:
        _fail(f"geometer.contract.{label}_length", path, f"{label.title()} is shorter than its minimum.")
    if maximum is not None and size > maximum:
        _fail(f"geometer.contract.{label}_length", path, f"{label.title()} is longer than its maximum.")


def _child_path(path: str, token: str) -> str:
    return f"{path}/{token.replace('~', '~0').replace('/', '~1')}"


def _fail(code: str, path: str, message: str) -> NoReturn:
    raise ContractError(code, path, message)
