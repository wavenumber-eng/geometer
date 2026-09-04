from __future__ import annotations

import struct

import pytest

from geometer import (
    IndexedTriangleMeshA0,
    IndexedTriangleMeshPacketError,
    decode_indexed_triangle_mesh_a0_packet,
    encode_indexed_triangle_mesh_a0_packet,
)


def test_indexed_mesh_packet_round_trips_canonical_triangle() -> None:
    mesh = IndexedTriangleMeshA0(
        positions=(0.0, 0.0, 0.0, 1.0, 0.0, 0.0, 0.0, 1.0, 0.0),
        indices=(0, 1, 2),
    )

    packet = encode_indexed_triangle_mesh_a0_packet(mesh)

    assert packet[:8] == b"GMIMSH01"
    assert len(packet) == 152
    assert struct.unpack_from("<Q", packet, 16) == (152,)
    assert decode_indexed_triangle_mesh_a0_packet(packet) == mesh


def test_indexed_mesh_packet_preserves_source_faces_and_rejects_noncanonical_padding() -> None:
    packet = bytearray(
        encode_indexed_triangle_mesh_a0_packet(
            IndexedTriangleMeshA0(
                positions=(0.0, 0.0, 0.0, 1.0, 0.0, 0.0, 0.0, 1.0, 0.0),
                indices=(0, 1, 2),
                source_faces=(7,),
            )
        )
    )
    decoded = decode_indexed_triangle_mesh_a0_packet(packet)
    assert decoded.source_faces == (7,)

    packet[-1] = 1
    with pytest.raises(IndexedTriangleMeshPacketError, match="must be zero"):
        decode_indexed_triangle_mesh_a0_packet(packet)


@pytest.mark.parametrize(
    "mesh",
    [
        IndexedTriangleMeshA0(positions=(0.0,) * 8, indices=(0, 1, 2)),
        IndexedTriangleMeshA0(positions=(0.0,) * 9, indices=(0, 0, 2)),
        IndexedTriangleMeshA0(positions=(0.0,) * 9, indices=(0, 1, 3)),
    ],
)
def test_indexed_mesh_packet_rejects_invalid_meshes(mesh: IndexedTriangleMeshA0) -> None:
    with pytest.raises(IndexedTriangleMeshPacketError):
        encode_indexed_triangle_mesh_a0_packet(mesh)
