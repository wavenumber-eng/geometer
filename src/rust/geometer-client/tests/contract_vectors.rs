use std::fs;
use std::path::{Path, PathBuf};

use geometer_client::contracts;
use serde::Deserialize;

#[derive(Deserialize)]
struct Manifest {
    vectors: Vec<Vector>,
}

#[derive(Deserialize)]
struct Vector {
    id: String,
    contract_identity: String,
    file: String,
    expected: String,
    oracle: String,
}

#[test]
fn replays_all_governed_contract_vectors() {
    let root = repository_root();
    let vector_root = root.join("tests/contracts/vectors");
    let manifest: Manifest =
        serde_json::from_slice(&fs::read(vector_root.join("manifest.json")).unwrap()).unwrap();
    assert_eq!(manifest.vectors.len(), 115);
    for vector in manifest.vectors {
        let path = vector_root.join(&vector.file);
        let data = if path.extension().and_then(|value| value.to_str()) == Some("hex") {
            decode_hex(&fs::read_to_string(path).unwrap())
        } else {
            fs::read(path).unwrap()
        };
        let accepted = decode_contract_vector(&vector.contract_identity, &data, &vector.id);
        if vector.oracle.starts_with("step_topology_") {
            assert!(accepted, "{} must be structurally valid", vector.id);
        } else {
            assert_eq!(accepted, vector.expected == "accept", "{}", vector.id);
        }
    }
}

type ContractDecoder = fn(&str, &[u8], &str) -> Option<bool>;

fn decode_contract_vector(identity: &str, data: &[u8], vector_id: &str) -> bool {
    let decoders: [ContractDecoder; 4] = [
        decode_core_contract,
        decode_topology_mutation_contract,
        decode_topology_persistence_contract,
        decode_topology_interaction_contract,
    ];
    for decoder in decoders {
        if let Some(accepted) = decoder(identity, data, vector_id) {
            return accepted;
        }
    }
    panic!("unhandled contract identity {identity}")
}

fn decode_core_contract(identity: &str, data: &[u8], _vector_id: &str) -> Option<bool> {
    Some(match identity {
        "geometry.common.diagnostic.a0" => contracts::decode_diagnostic_a0_json(data).is_ok(),
        "geometry.model_bounds.options.a0" => {
            contracts::decode_model_bounds_options_a0_json(data).is_ok()
        }
        "geometry.model_bounds.a0" => contracts::decode_model_bounds_result_a0_json(data).is_ok(),
        "geometer.operation.outcome.a0" => {
            contracts::decode_operation_outcome_a0_json(data).is_ok()
        }
        "geometer.ipc.request.a0" => contracts::decode_ipc_request_a0_json(data).is_ok(),
        _ => return None,
    })
}

fn decode_topology_mutation_contract(
    identity: &str,
    data: &[u8],
    _vector_id: &str,
) -> Option<bool> {
    Some(match identity {
        "geometry.step_topology.apply_logical_groups.request.a0" => {
            contracts::decode_step_topology_apply_logical_groups_request_a0_json(data).is_ok()
        }
        "geometry.step_topology.apply_logical_groups.result.a0" => {
            contracts::decode_step_topology_apply_logical_groups_result_a0_json(data).is_ok()
        }
        "geometry.step_topology.apply_metadata_probes.request.a0" => {
            contracts::decode_step_topology_apply_metadata_probes_request_a0_json(data).is_ok()
        }
        "geometry.step_topology.apply_metadata_probes.result.a0" => {
            contracts::decode_step_topology_apply_metadata_probes_result_a0_json(data).is_ok()
        }
        "geometry.step_topology.checkpoint_edit_journal.request.a0" => {
            contracts::decode_step_topology_checkpoint_edit_journal_request_a0_json(data).is_ok()
        }
        "geometry.step_topology.checkpoint_edit_journal.result.a0" => {
            contracts::decode_step_topology_checkpoint_edit_journal_result_a0_json(data).is_ok()
        }
        _ => return None,
    })
}

fn decode_topology_persistence_contract(
    identity: &str,
    data: &[u8],
    _vector_id: &str,
) -> Option<bool> {
    Some(match identity {
        "geometry.step_topology.apply_hierarchy.request.a0" => {
            contracts::decode_step_topology_apply_hierarchy_request_a0_json(data).is_ok()
        }
        "geometry.step_topology.apply_hierarchy.result.a0" => {
            contracts::decode_step_topology_apply_hierarchy_result_a0_json(data).is_ok()
        }
        "geometry.step_topology.save.result.a0" => {
            contracts::decode_step_topology_save_result_a0_json(data).is_ok()
        }
        "geometry.step_topology.restore.request.a0" => {
            contracts::decode_step_topology_restore_request_a0_json(data).is_ok()
        }
        "geometry.step_topology.restore.result.a0" => {
            contracts::decode_step_topology_restore_result_a0_json(data).is_ok()
        }
        "geometry.step_topology.analyze_recovery.request.a0" => {
            contracts::decode_step_topology_analyze_recovery_request_a0_json(data).is_ok()
        }
        "geometry.step_topology.analyze_recovery.result.a0" => {
            contracts::decode_step_topology_analyze_recovery_result_a0_json(data).is_ok()
        }
        _ => return None,
    })
}

fn decode_topology_interaction_contract(
    identity: &str,
    data: &[u8],
    vector_id: &str,
) -> Option<bool> {
    Some(match identity {
        "geometry.step_topology.resolve_hit.request.a0" => {
            let value = match contracts::decode_step_topology_resolve_hit_request_a0_json(data) {
                Ok(value) => value,
                Err(_) => return Some(false),
            };
            let encoded =
                contracts::encode_step_topology_resolve_hit_request_a0_json(&value).unwrap();
            assert_eq!(encoded, trim_line_endings(data), "{vector_id}");
            true
        }
        "geometry.step_topology.resolve_hit.result.a0" => {
            contracts::decode_step_topology_resolve_hit_result_a0_json(data).is_ok()
        }
        "geometry.step_topology.render.result.a0" => {
            contracts::decode_step_topology_render_result_a0_json(data).is_ok()
        }
        "geometry.step_topology.inspect.result.a0" => {
            contracts::decode_step_topology_inspect_result_a0_json(data).is_ok()
        }
        _ => return None,
    })
}

fn trim_line_endings(data: &[u8]) -> &[u8] {
    let mut end = data.len();
    while end > 0 && matches!(data[end - 1], b'\r' | b'\n') {
        end -= 1;
    }
    &data[..end]
}

fn decode_hex(text: &str) -> Vec<u8> {
    let compact: String = text
        .chars()
        .filter(|value| !value.is_whitespace())
        .collect();
    (0..compact.len())
        .step_by(2)
        .map(|index| u8::from_str_radix(&compact[index..index + 2], 16).unwrap())
        .collect()
}

fn repository_root() -> PathBuf {
    Path::new(env!("CARGO_MANIFEST_DIR"))
        .join("../../..")
        .canonicalize()
        .unwrap()
}
