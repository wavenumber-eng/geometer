use super::*;

fn response(collection: &MeshCollectionA0) -> OperationResponse {
    let data = contracts::encode_mesh_collection_a0_json(collection).unwrap();
    let metadata = ModelTessellationResultA0 {
        schema: "geometry.model_tessellation.result.a0".to_owned(),
        mesh_collection: contracts::MeshCollectionAttachment {
            attachment: "mesh_collection".to_owned(),
            schema: "geometry.mesh_collection.a0".to_owned(),
            byte_length: data.len() as u32,
            sha256: format!("{:x}", Sha256::digest(&data)),
        },
        source_sha256: "f".repeat(64),
        meshes: collection.meshes.len() as u32,
        triangles: 1,
        warnings: Vec::new(),
    };
    OperationResponse {
        outcome: OperationOutcomeA0::Success(contracts::OperationSuccessA0 {
            operation: "geometry.model_tessellation.a0".to_owned(),
            ok: true,
            result: OperationResultValueA0::ModelTessellation(metadata),
        }),
        attachments: vec![Attachment {
            name: "mesh_collection".to_owned(),
            media_type: "application/vnd.wavenumber.geometer.mesh-collection+json".to_owned(),
            data,
        }],
    }
}

fn triangle() -> MeshCollectionA0 {
    contracts::decode_mesh_collection_a0_json(br#"{"schema":"geometry.mesh_collection.a0","length_unit":"millimeter","meshes":[{"id":"triangle","positions":[0,0,0,1,0,0,0,1,0],"indices":[0,1,2],"materials":[{"color":[1,0,0]}]}]}"#).unwrap()
}

#[test]
fn attachment_integrity_and_requested_budget_are_checked() {
    let mesh = triangle();
    assert!(decode_response(response(&mesh), &"f".repeat(64), 1).is_ok());
    assert!(decode_response(response(&mesh), &"0".repeat(64), 1).is_err());
    assert!(decode_response(response(&mesh), &"f".repeat(64), 0).is_err());
    let mut corrupted = response(&mesh);
    corrupted.attachments[0].data[0] = b'!';
    assert!(decode_response(corrupted, &"f".repeat(64), 1).is_err());
}

#[test]
fn mesh_semantic_layout_is_checked_after_structural_decode() {
    let mut mesh = triangle();
    mesh.meshes[0].indices = Some(vec![0, 1, 3]);
    assert!(decode_response(response(&mesh), &"f".repeat(64), 1).is_err());
    mesh = triangle();
    mesh.meshes[0].normals = Some(vec![0.0, 0.0, 1.0]);
    assert!(decode_response(response(&mesh), &"f".repeat(64), 1).is_err());
    mesh = triangle();
    mesh.meshes[0].triangle_material_indices = Some(vec![1]);
    assert!(decode_response(response(&mesh), &"f".repeat(64), 1).is_err());
    mesh = triangle();
    mesh.meshes.push(mesh.meshes[0].clone());
    assert!(decode_response(response(&mesh), &"f".repeat(64), 1).is_err());
}
