"""Canonical indexed-triangle-mesh packet A0 codec."""

from __future__ import annotations

import math
import struct
from dataclasses import dataclass
from typing import Sequence


INDEXED_TRIANGLE_MESH_MEDIA_TYPE = "application/vnd.wavenumber.geometer.indexed-triangle-mesh"
INDEXED_TRIANGLE_MESH_PACKET_FORMAT = "geometry.indexed_triangle_mesh.packet.a0"

_MAGIC = b"GMIMSH01"
_HEADER_BYTES = 64
_HAS_SOURCE_FACES = 1
_MAX_PACKET_BYTES = 268_435_456
_MAX_VERTICES = 2_000_000
_MAX_TRIANGLES = 2_000_000
_UNSPECIFIED_SOURCE_FACE = 0xFFFF_FFFF


class IndexedTriangleMeshPacketError(ValueError):
    """The indexed-mesh value or packet violates its A0 contract."""


@dataclass(frozen=True, slots=True)
class IndexedTriangleMeshA0:
    """Flat XYZ millimeter coordinates, triangle indices, and optional source faces."""

    positions: Sequence[float]
    indices: Sequence[int]
    source_faces: Sequence[int] | None = None


def encode_indexed_triangle_mesh_a0_packet(mesh: IndexedTriangleMeshA0) -> bytes:
    """Encode a canonical ``geometry.indexed_triangle_mesh.packet.a0`` packet."""

    vertex_count, triangle_count, positions, indices, source_faces = _validated_mesh(mesh)
    has_source_faces = source_faces is not None and any(value != _UNSPECIFIED_SOURCE_FACE for value in source_faces)
    positions_offset = _HEADER_BYTES
    triangles_offset = positions_offset + vertex_count * 24
    triangles_end = triangles_offset + triangle_count * 12
    source_faces_offset = _align_eight(triangles_end) if has_source_faces else 0
    payload_end = source_faces_offset + triangle_count * 4 if has_source_faces else triangles_end
    packet_bytes = _align_eight(payload_end)
    if packet_bytes > _MAX_PACKET_BYTES:
        _fail("indexed-mesh packet exceeds 268 MiB")

    output = bytearray(packet_bytes)
    struct.pack_into(
        "<8sHHIQIIQQQQ",
        output,
        0,
        _MAGIC,
        1,
        _HEADER_BYTES,
        _HAS_SOURCE_FACES if has_source_faces else 0,
        packet_bytes,
        vertex_count,
        triangle_count,
        positions_offset,
        triangles_offset,
        source_faces_offset,
        0,
    )
    struct.pack_into(f"<{len(positions)}d", output, positions_offset, *positions)
    struct.pack_into(f"<{len(indices)}I", output, triangles_offset, *indices)
    if has_source_faces and source_faces is not None:
        struct.pack_into(f"<{len(source_faces)}I", output, source_faces_offset, *source_faces)
    return bytes(output)


def _validated_mesh(
    mesh: IndexedTriangleMeshA0,
) -> tuple[int, int, tuple[float, ...], tuple[int, ...], tuple[int, ...] | None]:
    if len(mesh.positions) < 9 or len(mesh.positions) % 3:
        _fail("positions must contain at least three complete XYZ vertices")
    if len(mesh.indices) < 3 or len(mesh.indices) % 3:
        _fail("indices must contain at least one complete triangle")
    vertex_count = len(mesh.positions) // 3
    triangle_count = len(mesh.indices) // 3
    if vertex_count > _MAX_VERTICES or triangle_count > _MAX_TRIANGLES:
        _fail("indexed mesh exceeds its governed vertex or triangle limit")
    if mesh.source_faces is not None and len(mesh.source_faces) != triangle_count:
        _fail("source_faces must contain exactly one value per triangle")

    positions = tuple(float(value) for value in mesh.positions)
    if not all(math.isfinite(value) for value in positions):
        _fail("positions must contain only finite numbers")
    indices = tuple(_uint32(value, "triangle index") for value in mesh.indices)
    if any(value >= vertex_count for value in indices):
        _fail("triangle index is outside the vertex table")
    for offset in range(0, len(indices), 3):
        if len(set(indices[offset : offset + 3])) != 3:
            _fail("each triangle must reference three distinct vertex indices")

    source_faces = (
        None if mesh.source_faces is None else tuple(_uint32(value, "source face") for value in mesh.source_faces)
    )
    return vertex_count, triangle_count, positions, indices, source_faces


def decode_indexed_triangle_mesh_a0_packet(packet: bytes | bytearray | memoryview) -> IndexedTriangleMeshA0:
    """Strictly decode and validate an indexed-triangle-mesh A0 packet."""

    data = memoryview(packet).cast("B")
    if len(data) < _HEADER_BYTES or len(data) > _MAX_PACKET_BYTES:
        _fail("indexed-mesh packet length is outside its governed bounds")
    (
        magic,
        version,
        header_bytes,
        flags,
        packet_bytes,
        vertex_count,
        triangle_count,
        positions_offset,
        triangles_offset,
        source_faces_offset,
        reserved,
    ) = struct.unpack_from("<8sHHIQIIQQQQ", data)
    has_source_faces = bool(flags & _HAS_SOURCE_FACES)
    triangles_end = triangles_offset + triangle_count * 12
    payload_end = source_faces_offset + triangle_count * 4 if has_source_faces else triangles_end
    if (
        magic != _MAGIC
        or version != 1
        or header_bytes != _HEADER_BYTES
        or flags & ~_HAS_SOURCE_FACES
        or packet_bytes != len(data)
        or not 0 < vertex_count <= _MAX_VERTICES
        or not 0 < triangle_count <= _MAX_TRIANGLES
        or positions_offset != _HEADER_BYTES
        or triangles_offset != positions_offset + vertex_count * 24
        or source_faces_offset != (_align_eight(triangles_end) if has_source_faces else 0)
        or _align_eight(payload_end) != len(data)
        or reserved != 0
    ):
        _fail("indexed-mesh packet header or table layout is invalid")
    padding_begin = triangles_end
    padding_end = source_faces_offset if has_source_faces else len(data)
    if any(data[padding_begin:padding_end]) or any(data[payload_end:]):
        _fail("indexed-mesh alignment or reserved bytes must be zero")

    positions = struct.unpack_from(f"<{vertex_count * 3}d", data, positions_offset)
    if not all(math.isfinite(value) for value in positions):
        _fail("indexed-mesh position is non-finite")
    indices = struct.unpack_from(f"<{triangle_count * 3}I", data, triangles_offset)
    if any(value >= vertex_count for value in indices):
        _fail("triangle index is outside the vertex table")
    for offset in range(0, len(indices), 3):
        if len(set(indices[offset : offset + 3])) != 3:
            _fail("triangle contains repeated vertex indices")
    source_faces = struct.unpack_from(f"<{triangle_count}I", data, source_faces_offset) if has_source_faces else None
    return IndexedTriangleMeshA0(positions=positions, indices=indices, source_faces=source_faces)


def _align_eight(value: int) -> int:
    return (value + 7) & ~7


def _uint32(value: int, label: str) -> int:
    if isinstance(value, bool) or not isinstance(value, int) or not 0 <= value <= _UNSPECIFIED_SOURCE_FACE:
        _fail(f"{label} must be an unsigned 32-bit integer")
    return value


def _fail(message: str) -> None:
    raise IndexedTriangleMeshPacketError(message)
