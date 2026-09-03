//! Canonical encoder for `geometry.indexed_triangle_mesh.packet.a0`.

const MAGIC: &[u8; 8] = b"GMIMSH01";
const HEADER_BYTES: usize = 64;
const HAS_SOURCE_FACES: u32 = 1;
const MAX_PACKET_BYTES: usize = 268_435_456;
const MAX_VERTICES: usize = 2_000_000;
const MAX_TRIANGLES: usize = 2_000_000;
const UNSPECIFIED_SOURCE_FACE: u32 = u32::MAX;

pub const INDEXED_TRIANGLE_MESH_MEDIA_TYPE: &str =
    "application/vnd.wavenumber.geometer.indexed-triangle-mesh";
pub const INDEXED_TRIANGLE_MESH_PACKET_FORMAT: &str = "geometry.indexed_triangle_mesh.packet.a0";

#[derive(Clone, Debug, PartialEq)]
pub struct IndexedTriangleMeshA0 {
    pub positions: Vec<[f64; 3]>,
    pub triangles: Vec<[u32; 3]>,
    pub source_faces: Option<Vec<u32>>,
}

#[derive(Clone, Debug, PartialEq, Eq, thiserror::Error)]
pub enum IndexedMeshPacketError {
    #[error("indexed mesh is empty or exceeds its governed limits")]
    InvalidCount,
    #[error("indexed mesh contains a non-finite position")]
    NonFinitePosition,
    #[error("indexed mesh triangle contains an invalid vertex index")]
    InvalidTriangle,
    #[error("source_faces must contain exactly one value per triangle")]
    InvalidSourceFaces,
    #[error("indexed-mesh packet exceeds 268 MiB")]
    PacketTooLarge,
}

pub fn encode_indexed_triangle_mesh_a0_packet(
    mesh: &IndexedTriangleMeshA0,
) -> Result<Vec<u8>, IndexedMeshPacketError> {
    if mesh.positions.is_empty()
        || mesh.triangles.is_empty()
        || mesh.positions.len() > MAX_VERTICES
        || mesh.triangles.len() > MAX_TRIANGLES
    {
        return Err(IndexedMeshPacketError::InvalidCount);
    }
    if mesh
        .positions
        .iter()
        .flatten()
        .any(|value| !value.is_finite())
    {
        return Err(IndexedMeshPacketError::NonFinitePosition);
    }
    let vertex_count = mesh.positions.len();
    if mesh.triangles.iter().any(|triangle| {
        triangle.iter().any(|index| *index as usize >= vertex_count)
            || triangle[0] == triangle[1]
            || triangle[1] == triangle[2]
            || triangle[2] == triangle[0]
    }) {
        return Err(IndexedMeshPacketError::InvalidTriangle);
    }
    if mesh
        .source_faces
        .as_ref()
        .is_some_and(|values| values.len() != mesh.triangles.len())
    {
        return Err(IndexedMeshPacketError::InvalidSourceFaces);
    }
    let has_source_faces = mesh
        .source_faces
        .as_ref()
        .is_some_and(|values| values.iter().any(|value| *value != UNSPECIFIED_SOURCE_FACE));
    let positions_offset = HEADER_BYTES;
    let triangles_offset = positions_offset + mesh.positions.len() * 24;
    let triangles_end = triangles_offset + mesh.triangles.len() * 12;
    let source_faces_offset = if has_source_faces {
        align_eight(triangles_end)
    } else {
        0
    };
    let payload_end = if has_source_faces {
        source_faces_offset + mesh.triangles.len() * 4
    } else {
        triangles_end
    };
    let packet_bytes = align_eight(payload_end);
    if packet_bytes > MAX_PACKET_BYTES {
        return Err(IndexedMeshPacketError::PacketTooLarge);
    }

    let mut output = vec![0_u8; packet_bytes];
    output[..8].copy_from_slice(MAGIC);
    put_u16(&mut output, 8, 1);
    put_u16(&mut output, 10, HEADER_BYTES as u16);
    put_u32(
        &mut output,
        12,
        if has_source_faces {
            HAS_SOURCE_FACES
        } else {
            0
        },
    );
    put_u64(&mut output, 16, packet_bytes as u64);
    put_u32(&mut output, 24, mesh.positions.len() as u32);
    put_u32(&mut output, 28, mesh.triangles.len() as u32);
    put_u64(&mut output, 32, positions_offset as u64);
    put_u64(&mut output, 40, triangles_offset as u64);
    put_u64(&mut output, 48, source_faces_offset as u64);
    for (index, position) in mesh.positions.iter().flatten().enumerate() {
        output[positions_offset + index * 8..positions_offset + index * 8 + 8]
            .copy_from_slice(&position.to_le_bytes());
    }
    for (triangle_index, triangle) in mesh.triangles.iter().enumerate() {
        for (vertex_index, vertex) in triangle.iter().enumerate() {
            put_u32(
                &mut output,
                triangles_offset + triangle_index * 12 + vertex_index * 4,
                *vertex,
            );
        }
        if has_source_faces {
            put_u32(
                &mut output,
                source_faces_offset + triangle_index * 4,
                mesh.source_faces.as_ref().expect("validated source faces")[triangle_index],
            );
        }
    }
    Ok(output)
}

fn align_eight(value: usize) -> usize {
    (value + 7) & !7
}

fn put_u16(output: &mut [u8], offset: usize, value: u16) {
    output[offset..offset + 2].copy_from_slice(&value.to_le_bytes());
}

fn put_u32(output: &mut [u8], offset: usize, value: u32) {
    output[offset..offset + 4].copy_from_slice(&value.to_le_bytes());
}

fn put_u64(output: &mut [u8], offset: usize, value: u64) {
    output[offset..offset + 8].copy_from_slice(&value.to_le_bytes());
}

#[cfg(test)]
mod tests {
    use super::*;

    #[test]
    fn canonical_triangle_packet_has_expected_header() {
        let packet = encode_indexed_triangle_mesh_a0_packet(&IndexedTriangleMeshA0 {
            positions: vec![[0.0, 0.0, 0.0], [1.0, 0.0, 0.0], [0.0, 1.0, 0.0]],
            triangles: vec![[0, 1, 2]],
            source_faces: None,
        })
        .expect("valid packet");
        assert_eq!(&packet[..8], MAGIC);
        assert_eq!(packet.len(), 152);
        assert_eq!(&packet[16..24], &(152_u64).to_le_bytes());
    }
}
