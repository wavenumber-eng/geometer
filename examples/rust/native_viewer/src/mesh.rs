//! GPU presentation conversion of the supported colored tessellation contract.
use crate::camera::{Bounds, Vec3, cross, dot, normalized, sub};
use geometer_client::contracts::MeshCollectionA0;

#[repr(C)]
#[derive(Clone, Copy, Debug, bytemuck::Pod, bytemuck::Zeroable)]
pub struct Vertex {
    pub position: [f32; 3],
    pub normal: [f32; 3],
    pub color: [f32; 4],
}

pub struct MeshData {
    pub vertices: Vec<Vertex>,
    pub bounds: Bounds,
}

fn point(values: &[f64], index: usize) -> Vec3 {
    [
        values[index * 3],
        values[index * 3 + 1],
        values[index * 3 + 2],
    ]
}

pub fn prepare(collection: &MeshCollectionA0) -> Result<MeshData, String> {
    let mut minimum = [f64::INFINITY; 3];
    let mut maximum = [f64::NEG_INFINITY; 3];
    for mesh in &collection.meshes {
        if mesh.matrix.is_some() {
            return Err("STEP tessellation unexpectedly returned an unbaked transform".into());
        }
        for p in mesh.positions.chunks_exact(3) {
            for axis in 0..3 {
                minimum[axis] = minimum[axis].min(p[axis]);
                maximum[axis] = maximum[axis].max(p[axis]);
            }
        }
    }
    let center = std::array::from_fn(|axis| (minimum[axis] + maximum[axis]) * 0.5);
    let half = sub(maximum, center);
    let radius = dot(half, half).sqrt().max(1e-6);
    if !radius.is_finite() || radius > f64::from(f32::MAX) / 16.0 {
        return Err("Model bounds are outside the GPU preview range".into());
    }
    let mut vertices = Vec::new();
    for mesh in &collection.meshes {
        let indices = mesh
            .indices
            .as_ref()
            .ok_or("Expected indexed STEP tessellation")?;
        for (triangle, indices) in indices.chunks_exact(3).enumerate() {
            let points: [Vec3; 3] =
                std::array::from_fn(|i| point(&mesh.positions, indices[i] as usize));
            let geometric = normalized(cross(sub(points[1], points[0]), sub(points[2], points[0])));
            let material_index = mesh
                .triangle_material_indices
                .as_ref()
                .map_or(0, |v| v[triangle] as usize);
            let material = &mesh.materials[material_index];
            for (corner, index) in indices.iter().enumerate() {
                let normal = mesh.normals.as_ref().map_or(geometric, |values| {
                    normalized(point(values, *index as usize))
                });
                vertices.push(Vertex {
                    position: sub(points[corner], center).map(|v| v as f32),
                    normal: normal.map(|v| v as f32),
                    color: [
                        material.color[0] as f32,
                        material.color[1] as f32,
                        material.color[2] as f32,
                        1.0,
                    ], // Opaque diagnostic preview; native SVG retains source opacity.
                });
            }
        }
    }
    if vertices.is_empty() {
        return Err("No triangles for GPU preview".into());
    }
    Ok(MeshData {
        vertices,
        bounds: Bounds { center, radius },
    })
}

#[cfg(test)]
mod tests {
    use super::*;
    use geometer_client::contracts::{MeshIllustrationMaterial, MeshIllustrationMesh};

    #[test]
    fn preview_is_opaque_without_changing_native_materials() {
        let collection = MeshCollectionA0 {
            schema: "geometry.mesh_collection.a0".into(),
            length_unit: "millimeter".into(),
            meshes: vec![MeshIllustrationMesh {
                id: "triangle".into(),
                positions: vec![0.0, 0.0, 0.0, 1.0, 0.0, 0.0, 0.0, 1.0, 0.0],
                indices: Some(vec![0, 1, 2]),
                normals: None,
                matrix: None,
                materials: vec![MeshIllustrationMaterial {
                    color: [0.2, 0.4, 0.6],
                    opacity: Some(0.0),
                    name: None,
                }],
                triangle_material_indices: None,
                double_sided: None,
            }],
        };
        let mesh = prepare(&collection).unwrap();
        assert_eq!(mesh.vertices.len(), 3);
        assert!(
            mesh.vertices
                .iter()
                .all(|vertex| vertex.color == [0.2, 0.4, 0.6, 1.0])
        );
        assert_eq!(collection.meshes[0].materials[0].opacity, Some(0.0));
        assert_eq!(mesh.bounds.center, [0.5, 0.5, 0.0]);
    }
}
