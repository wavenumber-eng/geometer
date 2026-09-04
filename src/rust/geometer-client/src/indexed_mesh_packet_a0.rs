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
    validate_mesh(mesh)?;
    let layout = PacketLayout::for_mesh(mesh);
    if layout.packet_bytes > MAX_PACKET_BYTES {
        return Err(IndexedMeshPacketError::PacketTooLarge);
    }

    let mut output = vec![0_u8; layout.packet_bytes];
    write_header(&mut output, mesh, layout);
    write_payload(&mut output, mesh, layout);
    Ok(output)
}

fn validate_mesh(mesh: &IndexedTriangleMeshA0) -> Result<(), IndexedMeshPacketError> {
    validate_counts(mesh)?;
    validate_positions(mesh)?;
    validate_triangles(mesh)?;
    validate_source_faces(mesh)
}

fn validate_counts(mesh: &IndexedTriangleMeshA0) -> Result<(), IndexedMeshPacketError> {
    if mesh.positions.is_empty()
        || mesh.triangles.is_empty()
        || mesh.positions.len() > MAX_VERTICES
        || mesh.triangles.len() > MAX_TRIANGLES
    {
        return Err(IndexedMeshPacketError::InvalidCount);
    }
    Ok(())
}

fn validate_positions(mesh: &IndexedTriangleMeshA0) -> Result<(), IndexedMeshPacketError> {
    if mesh
        .positions
        .iter()
        .flatten()
        .any(|value| !value.is_finite())
    {
        return Err(IndexedMeshPacketError::NonFinitePosition);
    }
    Ok(())
}

fn validate_triangles(mesh: &IndexedTriangleMeshA0) -> Result<(), IndexedMeshPacketError> {
    let vertex_count = mesh.positions.len();
    if mesh.triangles.iter().any(|triangle| {
        triangle.iter().any(|index| *index as usize >= vertex_count)
            || triangle[0] == triangle[1]
            || triangle[1] == triangle[2]
            || triangle[2] == triangle[0]
    }) {
        return Err(IndexedMeshPacketError::InvalidTriangle);
    }
    Ok(())
}

fn validate_source_faces(mesh: &IndexedTriangleMeshA0) -> Result<(), IndexedMeshPacketError> {
    if mesh
        .source_faces
        .as_ref()
        .is_some_and(|values| values.len() != mesh.triangles.len())
    {
        return Err(IndexedMeshPacketError::InvalidSourceFaces);
    }
    Ok(())
}

fn has_source_faces(mesh: &IndexedTriangleMeshA0) -> bool {
    mesh.source_faces
        .as_ref()
        .is_some_and(|values| values.iter().any(|value| *value != UNSPECIFIED_SOURCE_FACE))
}

#[derive(Clone, Copy)]
struct PacketLayout {
    positions_offset: usize,
    triangles_offset: usize,
    source_faces_offset: usize,
    packet_bytes: usize,
}

impl PacketLayout {
    fn for_mesh(mesh: &IndexedTriangleMeshA0) -> Self {
        let positions_offset = HEADER_BYTES;
        let triangles_offset = positions_offset + mesh.positions.len() * 24;
        let triangles_end = triangles_offset + mesh.triangles.len() * 12;
        let source_faces_offset = if has_source_faces(mesh) {
            align_eight(triangles_end)
        } else {
            0
        };
        let payload_end = if source_faces_offset == 0 {
            triangles_end
        } else {
            source_faces_offset + mesh.triangles.len() * 4
        };
        Self {
            positions_offset,
            triangles_offset,
            source_faces_offset,
            packet_bytes: align_eight(payload_end),
        }
    }
}

fn write_header(output: &mut [u8], mesh: &IndexedTriangleMeshA0, layout: PacketLayout) {
    output[..8].copy_from_slice(MAGIC);
    put_u16(output, 8, 1);
    put_u16(output, 10, HEADER_BYTES as u16);
    let flags = if layout.source_faces_offset == 0 {
        0
    } else {
        HAS_SOURCE_FACES
    };
    put_u32(output, 12, flags);
    put_u64(output, 16, layout.packet_bytes as u64);
    put_u32(output, 24, mesh.positions.len() as u32);
    put_u32(output, 28, mesh.triangles.len() as u32);
    put_u64(output, 32, layout.positions_offset as u64);
    put_u64(output, 40, layout.triangles_offset as u64);
    put_u64(output, 48, layout.source_faces_offset as u64);
}

fn write_payload(output: &mut [u8], mesh: &IndexedTriangleMeshA0, layout: PacketLayout) {
    for (index, position) in mesh.positions.iter().flatten().enumerate() {
        output[layout.positions_offset + index * 8..layout.positions_offset + index * 8 + 8]
            .copy_from_slice(&position.to_le_bytes());
    }
    for (triangle_index, triangle) in mesh.triangles.iter().enumerate() {
        for (vertex_index, vertex) in triangle.iter().enumerate() {
            put_u32(
                output,
                layout.triangles_offset + triangle_index * 12 + vertex_index * 4,
                *vertex,
            );
        }
        if layout.source_faces_offset != 0 {
            put_u32(
                output,
                layout.source_faces_offset + triangle_index * 4,
                mesh.source_faces.as_ref().expect("validated source faces")[triangle_index],
            );
        }
    }
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
