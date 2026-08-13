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
}

#[test]
fn replays_all_governed_contract_vectors() {
    let root = repository_root();
    let vector_root = root.join("tests/contracts/vectors");
    let manifest: Manifest =
        serde_json::from_slice(&fs::read(vector_root.join("manifest.json")).unwrap()).unwrap();
    assert_eq!(manifest.vectors.len(), 20);
    for vector in manifest.vectors {
        let path = vector_root.join(&vector.file);
        let data = if path.extension().and_then(|value| value.to_str()) == Some("hex") {
            decode_hex(&fs::read_to_string(path).unwrap())
        } else {
            fs::read(path).unwrap()
        };
        let accepted = match vector.contract_identity.as_str() {
            "geometry.common.diagnostic.a0" => contracts::decode_diagnostic_a0_json(&data).is_ok(),
            "geometry.model_bounds.options.a0" => {
                contracts::decode_model_bounds_options_a0_json(&data).is_ok()
            }
            "geometry.model_bounds.a0" => {
                contracts::decode_model_bounds_result_a0_json(&data).is_ok()
            }
            "geometer.operation.outcome.a0" => {
                contracts::decode_operation_outcome_a0_json(&data).is_ok()
            }
            identity => panic!("unhandled contract identity {identity}"),
        };
        assert_eq!(accepted, vector.expected == "accept", "{}", vector.id);
    }
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
