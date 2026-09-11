"""Validate an illustration input once and serialize its mesh attachment."""

from __future__ import annotations

import json

from ._contract_runtime import _encode_value
from ._generated.contracts.codecs import DECLARATIONS
from ._generated.contracts.models import (
    ENUM_TYPES,
    MODEL_TYPES,
    MeshIllustrationGeometryInputA0,
    MeshIllustrationInputA0,
)


def _mesh_attachment(input: MeshIllustrationInputA0 | MeshIllustrationGeometryInputA0, root: str) -> bytes:
    # Preserve full input validation and field/error order. The governed roots
    # share the collection's mesh descriptor, so its normalized subtree can be
    # serialized directly without visiting every coordinate a second time.
    encoded = _encode_value(input, {"kind": "reference", "target": root}, {}, "", DECLARATIONS, MODEL_TYPES, ENUM_TYPES)
    collection = {"schema": "geometry.mesh_collection.a0", "length_unit": "millimeter", "meshes": encoded["meshes"]}
    return json.dumps(collection, ensure_ascii=False, allow_nan=False, separators=(",", ":")).encode("utf-8")
