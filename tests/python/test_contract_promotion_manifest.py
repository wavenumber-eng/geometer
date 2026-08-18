from __future__ import annotations

import hashlib
import json
import re
import subprocess
import tomllib
from pathlib import Path
from typing import Any


ROOT = Path(__file__).resolve().parents[2]
MANIFEST_PATH = ROOT / "docs" / "contracts" / "promotion-manifest.toml"
_RECTANGLE_ENUMERATION_GATE: dict[str, Any] = {
    "rectangle_enumeration_test": "tests/cpp/exact_rectangle_enumeration_test.cpp",
    "rectangle_enumeration_parity_validator": "scripts/validate_exact_rectangle_enumeration_parity.py",
    "rectangle_enumeration_grid": "2x2_integer_unit_cells",
    "rectangle_enumeration_cases": 558,
    "rectangle_enumeration_sha256": "db0ebca47a53585d7d52312b128a1c77fd345c282481b5415b061401cd5deb74",
    "rectangle_differential_engine": "clipper2_existing_planar_solve",
    "rectangle_differential_policy": "warning_only_non_authoritative",
    "rectangle_differential_schedule": "monthly_and_manual_release",
    "rectangle_differential_eligible_cases": 117,
    "rectangle_differential_baseline_warnings": 2,
    "rectangle_differential_baseline_sha256": ("4d5bb7b6302424bbf6621fc4a0a611d8fb17981710c95b617edd3b5fe4762a80"),
    "rectangle_differential_warning_identities": [
        "case_433_point_contact_component_merge",
        "case_465_point_contact_component_merge",
    ],
}

_NORMALIZATION_COLLAPSE_GATE: dict[str, Any] = {
    "normalization_collapse_cases": [
        "hole_x_below_tie",
        "hole_x_above_tie",
        "hole_y_below_tie",
        "hole_y_above_tie",
        "notch_below_tie",
        "notch_above_tie",
    ],
    "normalization_collapse_sha256": "761350eccf99d53336aa8c99903b62cc5906eab29ebdd2b0b3a1c3bd17654ede",
}

_BOOLEAN_METAMORPHIC_GATE: dict[str, Any] = {
    "boolean_metamorphic_test": "tests/cpp/exact_boolean_metamorphic_test.cpp",
    "boolean_metamorphic_parity_validator": "scripts/validate_exact_boolean_metamorphic_parity.py",
    "boolean_union_permutations": 6,
    "boolean_difference_permutations": 2,
    "boolean_metamorphic_sentinels": [
        "split:union,difference",
        "mutation:ordered_stage_swap",
    ],
    "boolean_metamorphic_sha256": "3572a4fcd40cc212a542d5dc8a166ce372404e058b01c819baa1b102e818bb6f",
}

_SEEDED_PROPERTY_GATE: dict[str, Any] = {
    "seeded_property_test": "tests/cpp/exact_boolean_seeded_property_test.cpp",
    "seeded_property_parity_validator": "scripts/validate_exact_boolean_seeded_property_parity.py",
    "seeded_property_generator": "splitmix64",
    "seeded_property_grid": "4x4_integer_unit_cells",
    "seeded_property_seeds": [
        "0000000000000001",
        "0123456789abcdef",
        "9e3779b97f4a7c15",
        "ffffffffffffffff",
    ],
    "seeded_property_cases_per_seed": 8,
    "seeded_property_cases": 32,
    "seeded_property_reducer": "greedy_one_minimal_stage_operand_rectangle_extent",
    "seeded_property_reducer_sentinel": "reducer:seed=0x0,case=0,stages=U[0,0,1,1;]",
    "seeded_property_reducer_multistage_sentinel": ("reducer_multistage:seed=0x0,case=1,stages=D[]U[0,0,1,1;]"),
    "seeded_property_sha256": "60945d8695701d82457c8aad4f47d225b0b66a2759dd4bac43f4dacf23d0b01e",
    "nightly_seeded_property_profile": "nightly",
    "nightly_seeded_property_schedule": "monthly_and_manual_release",
    "nightly_seeded_property_workflow": ".github/workflows/wasm.yml",
    "nightly_seeded_property_seeds": [
        "0000000000000000",
        "0000000000000001",
        "0123456789abcdef",
        "3141592653589793",
        "9e3779b97f4a7c15",
        "c0ffeec0ffeec0ff",
        "deadbeefcafebabe",
        "ffffffffffffffff",
    ],
    "nightly_seeded_property_cases_per_seed": 16,
    "nightly_seeded_property_cases": 128,
    "nightly_seeded_property_sha256": ("0cc922aca1bd2718b330cf1d0d080882181aed861fb5edf6601ee5c5d96f56a5"),
}

_LINEAGE_MATRIX_GATE: dict[str, Any] = {
    "boolean_lineage_matrix_test": "tests/cpp/exact_boolean_lineage_matrix_test.cpp",
    "boolean_lineage_matrix_parity_validator": ("scripts/validate_exact_boolean_lineage_matrix_parity.py"),
    "boolean_lineage_matrix_cases": [
        "two_contributors_two_disconnected_results",
        "same_stage_permutation",
        "omitted_association_mutation",
    ],
    "boolean_lineage_matrix_sha256": ("6f084826be2bf3b34ce13e4b392dd0f81d3de40ed97b8245ab05d18ae4fa6ae4"),
}

_CLOSED_FORM_GATE: dict[str, Any] = {
    "closed_form_invariants_test": "tests/cpp/analytic_closed_form_invariants_test.cpp",
    "closed_form_invariants_parity_validator": ("scripts/validate_analytic_closed_form_invariants_parity.py"),
    "closed_form_invariants": [
        "rectangle",
        "circle",
        "annulus",
        "capsule",
        "nested_island",
        "swept_l_path",
    ],
    "closed_form_arc_classes": ["semicircle", "quarter_circle"],
    "closed_form_invariants_sha256": ("0f9ec222d05e077277e2356e4a4f1b247946d0a6b7f9e2d6b2f11d0113bf33f6"),
    "metamorphic_invariants": [
        "translation",
        "rotation_90",
        "reflection",
        "integer_scaling",
        "source_id_renaming",
    ],
    "metamorphic_invariants_sha256": ("0907fe0bcc8b6d14989dceefa3cd8029405a09a493d91ea7434180b856ab80ea"),
}

_SYNTHETIC_CORRECTNESS_GATE: dict[str, Any] = {
    **_CLOSED_FORM_GATE,
    **_BOOLEAN_METAMORPHIC_GATE,
    **_LINEAGE_MATRIX_GATE,
    **_NORMALIZATION_COLLAPSE_GATE,
    **_RECTANGLE_ENUMERATION_GATE,
    **_SEEDED_PROPERTY_GATE,
}


def _manifest() -> dict[str, Any]:
    with MANIFEST_PATH.open("rb") as stream:
        return tomllib.load(stream)


def _unique(values: list[Any], label: str) -> None:
    assert len(values) == len(set(values)), f"duplicate {label}: {values}"


def _assert_documentation_manifest(manifest: dict[str, Any]) -> None:
    documentation = manifest["documentation"]
    assert documentation["runtime_sibling_dependency"] is False
    assert (ROOT / documentation["design"]).is_file()
    assert re.fullmatch(r"[0-9a-f]{40}", documentation["source_revision"])
    font_redistribution = documentation["font_redistribution"]
    assert font_redistribution["completion_gate"] is True
    assert font_redistribution["status"] == "approved_open_license"
    assert font_redistribution["selected_font"] == "Cousine"
    assert font_redistribution["license_spdx"] == "OFL-1.1"
    assert re.fullmatch(r"[0-9a-f]{40}", font_redistribution["source_revision"])
    assert font_redistribution["license_url"].startswith("https://")

    assets = documentation["assets"]
    asset_ids = [item["id"] for item in assets]
    _unique(asset_ids, "documentation asset id")
    assert asset_ids == [
        "stylesheet",
        "cousine_regular",
        "cousine_bold",
        "cousine_license",
        "wavenumber_light_watermark",
    ]
    design = (ROOT / documentation["design"]).read_text(encoding="utf-8")
    assert documentation["source_revision"] in design
    license_asset = next(item for item in assets if item["role"] == "license")
    assert {item["status"] for item in assets} == {"vendored"}
    for asset in assets:
        assert asset["role"] in {"stylesheet", "font", "license", "watermark"}
        assert re.fullmatch(r"[0-9a-f]{64}", asset["sha256"])
        assert asset["source"] in design or Path(asset["source"]).name in design
        assert asset["sha256"] in design
        assert asset["status"] in {"planned", "vendored"}
        destination = ROOT / asset["destination"]
        if asset["status"] == "vendored":
            assert destination.is_file()
            assert _sha256(destination) == asset["sha256"]
        else:
            assert not destination.exists()

        if asset["role"] == "font":
            assert font_redistribution["source_revision"] in asset["source"]
            assert asset["redistribution_status"] == "approved_open_license"
            assert asset["license_evidence"] == "docs/design/assets/fonts/Cousine/OFL.txt"
            assert asset["status"] == license_asset["status"]
            if asset["status"] == "vendored":
                assert font_redistribution["status"] == "approved_open_license"
                assert (ROOT / asset["license_evidence"]).is_file()
        elif asset["role"] == "license":
            assert font_redistribution["source_revision"] in asset["source"]
            assert asset["source"].endswith("/OFL.txt")

    stylesheet = next(item for item in assets if item["role"] == "stylesheet")
    assert stylesheet["source_sha256"] == ("b0452e403db12c3fca581866b0953dbca45d751bcc83c137f0da16674859d151")
    assert "Cousine" in stylesheet["adaptation"]
    stylesheet_text = (ROOT / stylesheet["destination"]).read_text(encoding="utf-8")
    assert 'font-family: "Cousine"' in stylesheet_text
    assert "Berkeley Mono" not in stylesheet_text


def _sha256(path: Path) -> str:
    return hashlib.sha256(path.read_bytes()).hexdigest()


def _tracked_bytes(path: Path) -> bytes:
    relative = path.relative_to(ROOT).as_posix()
    completed = subprocess.run(
        ["git", "show", f"HEAD:{relative}"],
        cwd=ROOT,
        check=False,
        capture_output=True,
    )
    return completed.stdout if completed.returncode == 0 else path.read_bytes()


def _tracked_sha256(path: Path) -> str:
    return hashlib.sha256(_tracked_bytes(path)).hexdigest()


def _lf_sha256(path: Path) -> str:
    return hashlib.sha256(path.read_bytes().replace(b"\r\n", b"\n")).hexdigest()


def _canonical_feasibility_stdout(raw: bytes) -> bytes:
    text = raw.decode("utf-8", errors="strict").replace("\r\n", "\n")
    assert "\r" not in text
    assert text.endswith("\n")
    assert not text.endswith("\n\n")
    return text[:-1].encode("utf-8")


def _assert_nonzero_uint64(value: Any, label: str) -> None:
    assert isinstance(value, int) and not isinstance(value, bool), label
    assert 0 < value <= 0xFFFF_FFFF_FFFF_FFFF, label


def _collect_explicit_source_ids(value: Any, spaces: dict[str, list[int]]) -> None:
    key_to_space = {
        "ring_id": "ring",
        "boundary_ring_id": "ring",
        "path_id": "path",
        "segment_id": "segment",
        "curve_id": "curve",
        "feature_id": "feature",
        "vertex_id": "vertex",
    }
    if isinstance(value, dict):
        for key, child in value.items():
            if key in key_to_space:
                _assert_nonzero_uint64(child, key)
                spaces[key_to_space[key]].append(child)
            else:
                assert not key.endswith("_id"), f"unclassified source-topology id: {key}"
                _collect_explicit_source_ids(child, spaces)
    elif isinstance(value, list):
        for child in value:
            _collect_explicit_source_ids(child, spaces)


def test_manifest_sources_and_identities_are_complete() -> None:
    manifest = _manifest()
    assert manifest["manifest_version"] == 1
    assert manifest["tracking_issue"].endswith("/issues/18")
    assert manifest["policy"]["required_projections"] == [
        "json_schema",
        "cpp",
        "typescript",
        "rust",
        "python",
    ]
    toolchain = manifest["toolchain"]
    assert toolchain == {
        "status": "implemented",
        "design": "docs/design/typespec-toolchain.md",
        "runtime_dependency": False,
        "node_major": 24,
        "package_manager": "npm@11.16.0",
        "npm_provision_command": "npm install --global npm@11.16.0",
        "npm_version_check_command": "npm --version",
        "typespec_compiler": "1.14.0",
        "typespec_json_schema": "1.14.0",
        "typescript": "5.9.3",
        "biome": "2.5.7",
        "source_root": "src/tsp/geometer",
        "emitter_root": "src/ts/wn-geometer-contract-emitter",
        "catalog_schema": "contracts/geometer/catalog-schema.a0.json",
        "catalog": "contracts/geometer/generated/wn_geometer_contract_catalog.a0.json",
        "schema_root": "contracts/geometer/generated/schema",
        "catalog_identity": "wn.geometer.contract_catalog",
        "catalog_generation": "a0",
    }
    assert (ROOT / toolchain["design"]).is_file()
    for implemented_path in (
        "source_root",
        "emitter_root",
        "catalog_schema",
        "catalog",
        "schema_root",
    ):
        assert (ROOT / toolchain[implemented_path]).exists()
    transports = manifest["transports"]
    assert (ROOT / transports["generic_c_abi_spec"]).is_file()
    assert (ROOT / transports["executable_ipc_spec"]).is_file()
    assert (ROOT / transports["transport_adr"]).is_file()
    design_review = transports["design_review"]
    review_sources = {
        "packet_sha256": design_review["packet"],
        "adr_sha256": transports["transport_adr"],
        "generic_c_abi_sha256": transports["generic_c_abi_spec"],
        "executable_ipc_sha256": transports["executable_ipc_spec"],
    }
    for digest_key, source in review_sources.items():
        assert _sha256(ROOT / source) == design_review[digest_key]
    adr = (ROOT / transports["transport_adr"]).read_text(encoding="utf-8")
    assert re.fullmatch(r"[0-9a-f]{40}", design_review["requested_revision"])
    assert re.fullmatch(r"[0-9a-f]{40}", design_review["review_head"])
    assert design_review["status"] in {"pending", "approved"}
    if design_review["status"] == "pending":
        assert transports["implementation_allowed"] is False
        assert design_review["reviewer"] == "none"
        assert design_review["review_date"] == "none"
        assert design_review["reviewed_revision"] == "none"
        assert "Proposed." in adr
    else:
        assert transports["implementation_allowed"] is True
        assert design_review["reviewer"] != "none"
        assert re.fullmatch(r"\d{4}-\d{2}-\d{2}", design_review["review_date"])
        assert design_review["reviewed_revision"] == design_review["requested_revision"]
        assert design_review["review_head"] == ("b86a065c5926c35f1eee23a9ba1cef890689c7d7")
        assert design_review["reviewed_adr_sha256"] == (
            "d4905bda88727fadeb221a3b6c5bfb392f5e062bb6033b0cded29f59fb492de0"
        )
        assert "## Status\n\nAccepted." in adr
    _assert_documentation_manifest(manifest)


def _assert_projection_surfaces(manifest: dict[str, Any]) -> None:
    typescript = manifest["typescript_projection"]
    assert typescript["status"] == "implemented_model_bounds_and_analytic_worker_pilot"
    assert typescript["worker_protocol"] == "wn.geometer.wasm_worker.a0"
    assert typescript["runtime_dependency"] is False
    for key in (
        "design",
        "source_root",
        "generated_root",
        "package_root",
        "worker_client_source",
        "worker_host_source",
        "example_source",
        "example_worker_source",
        "example_page",
        "example_artifact",
        "example_worker_artifact",
        "analytic_example_source",
        "analytic_example_worker_source",
        "analytic_example_page",
        "analytic_example_artifact",
        "analytic_example_worker_artifact",
    ):
        assert (ROOT / typescript[key]).exists(), key
    package_json = json.loads((ROOT / typescript["package_root"] / "package.json").read_text(encoding="utf-8"))
    assert package_json["name"] == manifest["packages"]["typescript"]
    assert manifest["packages"]["typescript_module_format"] == "esm"
    assert package_json["type"] == "module"

    python_projection = manifest["python_projection"]
    assert python_projection["status"] == ("implemented_model_bounds_compatible_boundary_and_analytic_ipc_pilot")
    assert python_projection["live_operation"] == "geometry.model_bounds.a0"
    assert python_projection["analytic_live_operation"] == ("geometry.analytic_planar_boolean_batch.a0")
    for key in (
        "design",
        "generated_root",
        "runtime",
        "generator",
        "analytic_packet_codec",
        "ipc_client",
    ):
        assert (ROOT / python_projection[key]).exists(), key
    assert python_projection["runtime_dependency"] is False


def _assert_model_bounds_promotion(manifest: dict[str, Any]) -> None:
    toolchain = manifest["toolchain"]
    evidence = manifest["promotion_evidence"]["model_bounds"]
    assert evidence["status"] == "accepted_promoted"
    assert evidence["independent_review"] == "accepted"
    assert re.fullmatch(r"[0-9a-f]{40}", evidence["candidate_revision"])
    assert evidence["hosted_workflow_run"] == 31741067434
    assert evidence["hosted_native_conclusion"] == "success"
    assert evidence["hosted_platforms"] == [
        "windows-x64",
        "macos-arm64",
        "linux-x64",
        "linux-arm64",
    ]
    assert len(evidence["hosted_native_jobs"]) == 4
    assert evidence["standards_expected_failure"] == "active_temporary_plan_only"
    assert evidence["governed_vector_count"] == 22
    assert evidence["structural_vector_count"] == 20
    assert evidence["operation_vector_count"] == 2
    assert evidence["local_ctest_count"] == 10
    assert evidence["local_rack_passed"] == 66
    assert evidence["historical_reviewed_browser_js_sha256"] == (
        "5c9f0594465cdb5732911777e2ca38cc2814c1ef9cb52b7672022df6a50bf937"
    )
    assert evidence["browser_js_lock_remediation"] == (
        "historical_worktree_bytes_unretained_relocked_to_committed_distributable"
    )
    for key, path in (
        ("catalog_sha256", toolchain["catalog"]),
        ("vector_manifest_sha256", "tests/contracts/vectors/manifest.json"),
        ("browser_js_sha256", "dist/wasm/browser/geometer.js"),
        ("browser_wasm_sha256", "dist/wasm/browser/geometer.wasm"),
    ):
        digest = _tracked_sha256(ROOT / path) if key.startswith("browser_") else _sha256(ROOT / path)
        assert digest == evidence[key]


def _assert_contract_and_operation_inventory(manifest: dict[str, Any]) -> None:
    contracts = manifest["contracts"]
    contract_ids = [item["id"] for item in contracts]
    _unique(contract_ids, "contract id")
    assert "geometry.model_bounds.options.a0" in contract_ids
    assert "geometry.model_bounds.a0" in contract_ids
    promoted_contracts = {item["id"] for item in contracts if item["status"] == "promoted"}
    assert promoted_contracts == {
        "geometry.common.diagnostic.a0",
        "geometry.model_bounds.options.a0",
        "geometry.model_bounds.a0",
        "geometer.operation.outcome.a0",
    }
    assert {item["id"] for item in contracts if item["status"] == "candidate_frozen"} == {
        "geometry.analytic_planar_boolean_batch.request.a0",
        "geometry.analytic_planar_boolean_batch.result.a0",
    }
    assert all(
        item["current_authority"] == "typespec_normalized_catalog"
        for item in contracts
        if item["status"] in {"promoted", "candidate_frozen"}
    )
    assert all((ROOT / item["source"]).is_file() for item in contracts if item["source"] != "none")

    operations = manifest["operations"]
    operation_ids = [item["id"] for item in operations]
    _unique(operation_ids, "operation id")
    assert {item["id"] for item in operations if item["status"] == "pilot_candidate"} == set()
    assert {item["id"] for item in operations if item["status"] == "promoted"} == {"geometry.model_bounds.a0"}

    candidates = manifest["candidate_operations"]
    candidate_ids = [item["id"] for item in candidates]
    _unique(candidate_ids, "candidate operation id")
    assert not set(candidate_ids) & set(operation_ids)
    assert candidate_ids == ["geometry.analytic_planar_boolean_batch.a0"]
    candidate = candidates[0]
    assert candidate["status"] == "contract_frozen"
    assert candidate["solver_numeric_status"] == "reopened_filtered_50nm"
    assert candidate["filtered_solver_foundation"] == (
        "direct_lowering_broad_narrow_overlay_arrangement_face_selection_material_regions_"
        "lineage_operand_outcomes_normalization_packet_assembly_batch_merge_strict_published_"
        "geometry_policy_and_relationship_evaluation_implemented_packed_dispatch"
    )
    assert candidate["request_contract"] == "geometry.analytic_planar_boolean_batch.request.a0"
    assert candidate["result_contract"] == "geometry.analytic_planar_boolean_batch.result.a0"
    assert candidate["request_contract"] in contract_ids
    assert candidate["result_contract"] in contract_ids
    assert candidate["input_attachments"] == ["analytic_planar_boolean_request"]
    assert candidate["output_attachments"] == ["analytic_planar_boolean_result"]
    assert candidate["deferred_projections"] == []
    assert candidate["logical_dto_projections"] == ["cpp", "typescript", "rust", "python"]
    assert candidate["packed_runtime_projections"] == ["cpp", "typescript", "rust", "python"]
    assert candidate["logical_source_reference_expansions_per_batch"] == 1_048_576


def _assert_candidate_packet_vectors(manifest: dict[str, Any]) -> None:
    candidate = manifest["candidate_operations"][0]
    vector_manifest_path = ROOT / candidate["packet_vector_manifest"]
    vector_manifest = json.loads(vector_manifest_path.read_text(encoding="utf-8"))
    assert hashlib.sha256(vector_manifest_path.read_bytes()).hexdigest() == candidate["packet_vector_manifest_sha256"]
    assert vector_manifest["manifest_identity"] == candidate["packet_vector_manifest_identity"]
    assert vector_manifest["generation"] == candidate["packet_vector_generation"]
    assert len(vector_manifest["vectors"]) == candidate["packet_vector_count"] == 6
    for vector in vector_manifest["vectors"]:
        encoded = (vector_manifest_path.parent / vector["file"]).read_text(encoding="ascii").strip()
        data = bytes.fromhex(encoded)
        assert len(data) == vector["bytes"]
        assert hashlib.sha256(data).hexdigest() == vector["sha256"]
    vector_writer_path = ROOT / candidate["packet_vector_writer"]
    vector_writer = vector_writer_path.read_text(encoding="utf-8")
    assert hashlib.sha256(vector_writer_path.read_bytes()).hexdigest() == candidate["packet_vector_writer_sha256"]
    assert candidate["packet_vector_producer_targets"] == [
        "geometer_analytic_request_packet_test",
        "geometer_analytic_result_packet_records_test",
    ]
    assert candidate["packet_vector_producer_sources"] == [
        "tests/cpp/analytic_request_packet_test.cpp",
        "tests/cpp/analytic_result_packet_records_test.cpp",
    ]
    assert candidate["packet_vector_producer_source_sha256"] == [
        "7c2c6a960182271688a87f64747b52e0b63391295c27c68247829878293f6966",
        "886473dec4add537f99a15465a0cd78474a4f19f2cecf78a090c798360e7af71",
    ]
    for source, digest in zip(
        candidate["packet_vector_producer_sources"],
        candidate["packet_vector_producer_source_sha256"],
        strict=True,
    ):
        source_path = ROOT / source
        assert source_path.is_file()
        assert hashlib.sha256(source_path.read_bytes()).hexdigest() == digest
    assert all(target in vector_writer for target in candidate["packet_vector_producer_targets"])


def _assert_candidate_projection_surfaces(manifest: dict[str, Any]) -> None:
    candidate = manifest["candidate_operations"][0]
    assert candidate["production_transport_status"] == ("native_c_abi_browser_wasm_executable_ipc_dispatched")
    assert candidate["typescript_packed_projection"] == (
        "bigint_logical_dtos_strict_packet_codec_direct_and_worker_clients"
    )
    assert candidate["rust_packed_projection"] == (
        "integer_logical_dtos_strict_packet_codec_and_friendly_persistent_client"
    )
    assert candidate["python_packed_projection"] == (
        "generated_integer_logical_dtos_strict_packet_codec_and_friendly_bounded_persistent_client"
    )
    assert candidate["python_production_replay_test"] == "tests/python/test_matz_observation_replay.py"
    assert (ROOT / candidate["python_production_replay_test"]).is_file()
    assert candidate["python_production_replay_matching_cases"] == [
        "1",
        "3",
        "4",
        "5",
        "6",
        "7",
        "8",
        "9",
        "10",
    ]
    assert candidate["python_production_replay_blocked_cases"] == ["2_missing_authoritative_integer_endpoints"]
    assert candidate["python_production_replay_mismatched_cases"] == []
    assert candidate["packed_format"] == "separately_governed_frozen_a0"
    assert candidate["implementation_gate"] == (
        "packed_runtime_typescript_rust_python_projections_and_cpp_rust_python_logical_dtos_"
        "implemented_python_production_replay_matching_subset_1_3_4_5_6_7_8_9_10_cross_transport_"
        "exact_parity_case_2_"
        "blocked_missing_authoritative_integer_endpoints"
    )
    assert candidate["transport"] == "generic_named_attachments"
    assert candidate["operation_specific_c_abi_symbol"] is False
    assert candidate["replaces_existing_operation"] is False
    assert candidate["browser_target"] == "full_browser"
    cpp_contract_header = (ROOT / "src/cpp/lib/geometer/generated/contracts/contracts.h").read_text(encoding="utf-8")
    cpp_contract_json = (ROOT / "src/cpp/lib/geometer/generated/contracts/contracts_json.cpp").read_text(
        encoding="utf-8"
    )
    cpp_operation_catalog = (ROOT / "src/cpp/lib/geometer/generated/contracts/operation_catalog.cpp").read_text(
        encoding="utf-8"
    )
    assert "struct AnalyticPlanarBooleanBatchRequestA0" in cpp_contract_header
    assert "struct AnalyticPlanarBooleanBatchResultA0" in cpp_contract_header
    assert "decode_AnalyticPlanarBoolean" not in cpp_contract_json
    assert "write_AnalyticPlanarBoolean" not in cpp_contract_json
    assert (
        "using OperationResultValueA0 = std::variant<ModelBoundsResultA0, PackedAttachmentProjectionA0>;"
    ) in cpp_contract_header
    assert "holds_alternative<contracts::AnalyticPlanarBoolean" not in cpp_operation_catalog
    rust_contracts = (ROOT / "src/rust/geometer-client/src/generated/contracts.rs").read_text(encoding="utf-8")
    first_analytic_type = rust_contracts.index("pub struct JobId")
    analytic_rust_start = rust_contracts.rindex("#[derive", 0, first_analytic_type)
    first_json_enum = rust_contracts.index("pub enum DiagnosticCategory")
    analytic_rust_end = rust_contracts.rindex("#[derive", analytic_rust_start, first_json_enum)
    analytic_rust = rust_contracts[analytic_rust_start:analytic_rust_end]
    assert "pub struct JobId" in analytic_rust
    assert "pub struct AnalyticPlanarBooleanBatchResultA0" in analytic_rust
    assert "#[serde" not in analytic_rust
    assert "Deserialize" not in analytic_rust
    assert "Serialize" not in analytic_rust
    assert "decode_analytic_planar_boolean" not in rust_contracts
    assert "encode_analytic_planar_boolean" not in rust_contracts
    assert (ROOT / candidate["compatibility_snapshot"]).is_file()
    assert (ROOT / candidate["design"]).is_file()
    assert (ROOT / candidate["packet_spec"]).is_file()
    assert (ROOT / candidate["numeric_catalog"]).is_file()
    assert (ROOT / candidate["superseded_solver_adr"]).is_file()
    assert (ROOT / candidate["feasibility_test"]).is_file()
    assert (ROOT / candidate["portable_fixture"]).is_file()
    review_record = ROOT / candidate["independent_design_review_record"]
    assert review_record.is_file()
    assert _sha256(review_record) == candidate["independent_design_review_record_sha256"]
    assert candidate["independent_design_review_revision"] == "529c768e559b4c88874264748d4186e775c8a4dd"
    assert candidate["independent_design_review_head"] == "b86a065c5926c35f1eee23a9ba1cef890689c7d7"
    assert candidate["typespec_projection_review_revision"] == "f4b6a9b87bf16f57ef29dae22150b16f2a742b64"
    assert candidate["typespec_projection_review_packet"] == "reviewer-019ffd0d-fa76-74b6-ac3e-c1c2642ba0de"


def _assert_matz_case_2_handoff(manifest: dict[str, Any]) -> None:
    candidate = manifest["candidate_operations"][0]
    handoff_path = ROOT / candidate["matz_case_2_handoff"]
    handoff = json.loads(handoff_path.read_text(encoding="utf-8"))
    assert _sha256(handoff_path) == candidate["matz_case_2_handoff_sha256"]
    assert handoff["identity"] == "wn.geometer.matz_case_2_handoff"
    assert handoff["generation"] == "a0"
    assert (
        handoff["status"]
        == candidate["matz_case_2_handoff_status"]
        == ("blocked_awaiting_authoritative_integer_endpoints")
    )

    upstream = handoff["upstream_fixture"]
    assert upstream == {
        "repository": "wavenumber-eng/appz",
        "ref": "4c688e46729015d21dc140dbe274e396e3717c18",
        "path": "data_models/tests/fixtures/pcb_materialization/geometer_analytic_planar_boolean_observations_a0.json",
        "content_sha256": "10a97f0eed4a4f6852917c4fb6abd35854142bf5d148b8320c60a44f765414c4",
        "git_blob_sha1": "45c5a648fc0b167c8a887006cb14d3e62229e360",
        "vendored_path": candidate["portable_fixture"],
        "fixture_id": "intersecting_arbitrary_angle_arcs",
        "job_id": 2,
        "stage_id": 201,
    }
    vendored_bytes = (ROOT / upstream["vendored_path"]).read_bytes()
    assert hashlib.sha256(vendored_bytes).hexdigest() == upstream["content_sha256"]
    git_blob = f"blob {len(vendored_bytes)}\0".encode() + vendored_bytes
    assert hashlib.sha1(git_blob).hexdigest() == upstream["git_blob_sha1"]

    fixture = json.loads((ROOT / upstream["vendored_path"]).read_text(encoding="utf-8"))
    arc_case = next(item for item in fixture["portable_cases"] if item["fixture_id"] == upstream["fixture_id"])
    operands = arc_case["stages"][0]["operands"]
    updates = handoff["required_operand_updates"]
    assert [item["operand_id"] for item in operands] == [item["operand_id"] for item in updates] == [2001, 2002]
    required_fields = {"start", "end", "direction", "major_arc"}
    for operand, update in zip(operands, updates, strict=True):
        assert set(update["required_fields"]) == required_fields
        assert required_fields.isdisjoint(operand["geometry"])
        assert update["required_fields"]["start"] == {
            "type": "tuple_int64_int64",
            "unit": "nm",
            "authority": "matz",
        }
        assert update["required_fields"]["end"] == update["required_fields"]["start"]
        assert update["required_fields"]["direction"] == {
            "type": "enum",
            "allowed": ["ccw", "cw"],
            "authority": "matz",
        }
        assert update["required_fields"]["major_arc"] == {"type": "boolean", "authority": "matz"}

    projection = handoff["a0_projection"]
    assert projection["operand_kind"] == "swept_path"
    assert projection["centerline_vertices_per_operand"] == 2
    assert projection["centerline_segments_per_operand"] == 1
    assert projection["centerline_segment_kind"] == "circular_arc"
    assert projection["derived_from_angles"] == []
    assert (
        "the exact integer squared distances center-to-start and center-to-end are equal and nonzero"
        in (projection["validation_constraints"])
    )
    assert (
        "no angle, trigonometric value, or Geometer-selected rounding enters the A0 request"
        in (projection["validation_constraints"])
    )

    evidence = handoff["regeneration"]["required_evidence"]
    assert evidence == [
        "revised_upstream_ref",
        "revised_upstream_git_blob_sha1",
        "revised_upstream_content_sha256",
        "vendored_fixture_content_sha256",
        "canonical_a0_request_sha256",
        "native_result_packet_sha256",
        "browser_wasm_result_packet_sha256",
        "standalone_job_2_result_sha256",
        "canonical_fragment_oracle_sha256",
        "independent_review_identity",
    ]
    design = (ROOT / candidate["design"]).read_text(encoding="utf-8")
    assert "historical angle-form case-2 feasibility oracle" in design
    assert "[machine-actionable handoff](../contracts/matz-case-2-handoff-a0.json)" in design
    assert "historical fragment list must not be copied forward" in design
    assert "The case-2 success oracle" not in design


def _assert_candidate_reviewed_paths(manifest: dict[str, Any]) -> None:
    candidate = manifest["candidate_operations"][0]
    for path_key in (
        "design",
        "packet_spec",
        "numeric_catalog",
        "solver_adr",
        "candidate_typespec_entrypoint",
        "candidate_typespec_source",
        "candidate_operation_source",
        "candidate_check",
    ):
        assert _sha256(ROOT / candidate[path_key]) == candidate[f"{path_key}_sha256"]
    for key in (
        "candidate_typespec_entrypoint",
        "candidate_typespec_source",
        "candidate_operation_source",
        "candidate_check",
    ):
        assert (ROOT / candidate[key]).is_file()


def _assert_demo_inventory(manifest: dict[str, Any]) -> None:
    operation_ids = [item["id"] for item in manifest["operations"]]
    candidate_ids = [item["id"] for item in manifest["candidate_operations"]]
    for demo in manifest["demos"]:
        assert (ROOT / demo["source"]).is_file()
        if "worker" in demo:
            assert (ROOT / demo["worker"]).is_file()
        if "entrypoint" in demo:
            assert (ROOT / demo["entrypoint"]).is_file()
        for key in (
            "fixture",
            "theme",
            "font_regular",
            "font_bold",
            "font_license",
            "distribution",
            "asset_manifest",
            "headers",
            "standalone_distribution",
        ):
            if key in demo:
                assert (ROOT / demo[key]).is_file()
        if "distribution_sha256" in demo:
            static_manifest = json.loads(_tracked_bytes(ROOT / demo["asset_manifest"]))
            assert demo["distribution_sha256"] == static_manifest["sha256"]
        if "standalone_sha256" in demo:
            assert _tracked_sha256(ROOT / demo["standalone_distribution"]) == demo["standalone_sha256"]
        for key in ("font_regular", "font_bold", "font_license"):
            if key in demo:
                asset = ROOT / demo[key]
                digest = (
                    hashlib.sha256(asset.read_text(encoding="utf-8").replace("\r\n", "\n").encode("utf-8")).hexdigest()
                    if key == "font_license"
                    else _sha256(asset)
                )
                assert digest == demo[f"{key}_sha256"]
        assert demo["owning_operation"] in operation_ids + candidate_ids


def _assert_consumer_inventory(manifest: dict[str, Any]) -> None:
    consumers = manifest["consumers"]
    consumer_ids = [item["id"] for item in consumers]
    _unique(consumer_ids, "consumer id")
    assert consumer_ids == ["appz.viz", "appz.data_models.pcb.matz"]
    assert all((ROOT / item["snapshot"]).is_file() for item in consumers)


def test_manifest_promoted_and_candidate_surfaces_are_complete() -> None:
    manifest = _manifest()
    _assert_projection_surfaces(manifest)
    _assert_model_bounds_promotion(manifest)
    _assert_contract_and_operation_inventory(manifest)
    _assert_candidate_packet_vectors(manifest)
    _assert_candidate_projection_surfaces(manifest)
    _assert_matz_case_2_handoff(manifest)
    _assert_candidate_reviewed_paths(manifest)
    _assert_demo_inventory(manifest)
    _assert_consumer_inventory(manifest)


def test_analytic_cross_transport_parity_pair_is_governed_separately_from_codec_vectors() -> None:
    candidate = _manifest()["candidate_operations"][0]
    vector_manifest_path = ROOT / candidate["packet_vector_manifest"]
    vector_manifest = json.loads(vector_manifest_path.read_text(encoding="utf-8"))
    cross_transport = vector_manifest["cross_transport_parity"]
    assert len(vector_manifest["vectors"]) == candidate["packet_vector_count"] == 6
    assert cross_transport["id"] == candidate["cross_transport_parity_fixture_id"]
    assert cross_transport["vector_count"] == candidate["cross_transport_parity_vector_count"] == 2
    assert cross_transport["comparison"] == "exact_bytes"
    assert cross_transport["runtimes"] == ["native_executable_ipc", "browser_wasm_generic_c_abi"]
    for vector_key, candidate_path_key, candidate_digest_key in (
        ("request_file", "cross_transport_parity_request", "cross_transport_parity_request_sha256"),
        ("result_file", "cross_transport_parity_result", "cross_transport_parity_result_sha256"),
    ):
        path = vector_manifest_path.parent / cross_transport[vector_key]
        data = bytes.fromhex(path.read_text(encoding="ascii"))
        assert path == ROOT / candidate[candidate_path_key]
        assert hashlib.sha256(data).hexdigest() == candidate[candidate_digest_key]
    assert (
        cross_transport["standalone_job_result_sha256"] == (candidate["cross_transport_parity_standalone_job_sha256"])
    )
    assert cross_transport["validator"] == candidate["cross_transport_parity_validator"]
    assert cross_transport["wasm_runner"] == candidate["cross_transport_parity_wasm_runner"]
    for path_key, digest_key in (
        ("cross_transport_parity_validator", "cross_transport_parity_validator_sha256"),
        ("cross_transport_parity_wasm_runner", "cross_transport_parity_wasm_runner_sha256"),
    ):
        path = ROOT / candidate[path_key]
        assert path.is_file()
        assert hashlib.sha256(path.read_bytes()).hexdigest() == candidate[digest_key]
    assert candidate["cross_transport_parity_status"] == (
        "exact_native_executable_ipc_browser_wasm_result_bytes_and_standalone_job_digest_governed"
    )


def test_analytic_performance_qualification_harness_is_governed_without_overclaiming() -> None:
    candidate = _manifest()["candidate_operations"][0]
    assert candidate["performance_qualification_default_fixture_classification"] == "synthetic"
    assert candidate["performance_qualification_default_request"] == candidate["cross_transport_parity_request"]
    assert candidate["performance_qualification_target_wall_seconds"] == 1
    assert candidate["performance_qualification_target_solver_peak_working_memory_bytes"] == 536_870_912
    assert candidate["performance_qualification_hard_ceiling_wall_seconds"] == 5
    assert candidate["performance_qualification_hard_ceiling_solver_peak_working_memory_bytes"] == 1_073_741_824
    assert candidate["performance_qualification_status"] == (
        "production_ipc_replay_and_byte_matched_internal_solver_telemetry_and_portable_hard_ceiling_"
        "passed_for_rt_local_"
        "candidate_clean_build_attestation_pending_loz_pending"
    )
    assert candidate["performance_qualification_reference_machine_policy"] == (
        "optional_comparative_observation_not_release_gate"
    )
    assert candidate["performance_qualification_target_policy"] == ("one_second_512_mib_observation_not_release_gate")
    assert candidate["performance_qualification_release_ceiling_policy"] == (
        "five_seconds_1_gib_process_and_solver_hard_gate"
    )
    assert candidate["performance_qualification_build_attestation_schema"] == (
        "wn.geometer.native_build_attestation.a1"
    )
    assert candidate["performance_qualification_build_attestation_generator_identity"] == (
        "wn.geometer.native_build_attestation_generator.a1"
    )
    assert candidate["performance_qualification_build_attestation_source_policy"] == (
        "authoritative_clean_git_source_only_dirty_or_unavailable_is_diagnostic_unattested"
    )
    assert candidate["performance_qualification_build_attestation_sidecar"] == (
        "dist/native/<platform>/geometer.build-attestation.json"
    )
    governed = [
        (candidate["performance_qualification_harness"], candidate["performance_qualification_harness_sha256"]),
        (candidate["performance_qualification_test"], candidate["performance_qualification_test_sha256"]),
        (
            candidate["performance_qualification_native_helper"],
            candidate["performance_qualification_native_helper_sha256"],
        ),
        (
            candidate["performance_qualification_build_attestation_generator"],
            candidate["performance_qualification_build_attestation_generator_sha256"],
        ),
        (
            candidate["performance_qualification_build_attestation_test"],
            candidate["performance_qualification_build_attestation_test_sha256"],
        ),
        *zip(
            candidate["performance_qualification_modules"],
            candidate["performance_qualification_module_sha256"],
            strict=True,
        ),
    ]
    for relative, expected_sha256 in governed:
        path = ROOT / relative
        assert path.is_file()
        assert _sha256(path) == expected_sha256


def test_real_board_packet_provenance_records_rt_candidate_without_promoting() -> None:
    candidate = _manifest()["candidate_operations"][0]
    provenance_path = ROOT / candidate["real_board_packet_provenance"]
    assert _sha256(provenance_path) == candidate["real_board_packet_provenance_sha256"]
    assert candidate["real_board_packet_provenance_test"] == "tests/python/test_contract_promotion_manifest.py"
    assert (
        _sha256(ROOT / candidate["real_board_packet_provenance_test"])
        == (candidate["real_board_packet_provenance_test_sha256"])
    )
    with provenance_path.open("rb") as stream:
        provenance = tomllib.load(stream)

    assert (
        candidate["real_board_packet_provenance_status"]
        == provenance["record_status"]
        == ("public_mit_redistribution_authorized_rt_generated_local_qualified_loz_pending")
    )
    assert provenance["artifact_scope"].startswith("Geometry-only GMABRQ01 request packets")
    assert provenance["packet_format_magic"] == "GMABRQ01"
    assert provenance["public_redistribution_authorization"] == "authorized_for_public_redistribution"
    assert provenance["qualification_redistribution_authorization"] == "authorized_for_qualification"
    assert candidate["real_board_packet_license_spdx"] == provenance["license_spdx"] == "MIT"
    assert provenance["license_file"] == "LICENSE"
    assert (
        candidate["real_board_packet_required_attribution"]
        == provenance["required_attribution"]
        == ("Copyright (c) Wavenumber LLC")
    )
    assert provenance["required_attribution"] in (ROOT / provenance["license_file"]).read_text(encoding="utf-8")
    assert candidate["real_board_source_cad_redistribution"] == provenance["source_cad_redistribution"] == ("excluded")
    assert provenance["source_cad_repository_status"] == "not_committed"
    assert provenance["packet_generation_status"] == "rt_generated_loz_pending"
    assert provenance["exporter_provenance_status"] == "rt_complete_loz_pending"
    assert provenance["expected_result_authority_status"] == "rt_governed_loz_pending"
    assert provenance["promotion_status"] == ("rt_local_candidate_clean_build_attestation_pending")

    fixture = json.loads((ROOT / candidate["portable_fixture"]).read_text(encoding="utf-8"))
    audited_cases = {item["fixture_id"]: item for item in fixture["real_board_cases"]}
    cases = provenance["cases"]
    assert [item["fixture_id"] for item in cases] == candidate["real_board_packet_case_ids"]
    assert [item["source_sha256"] for item in cases] == candidate["real_board_packet_source_sha256"]
    for item in cases:
        audit = audited_cases[item["fixture_id"]]
        assert item["source_audit_locator"] == audit["source_path"]
        assert item["source_sha256"] == audit["source_sha256"]
        assert item["source_bytes"] == audit["source_bytes"]
        assert not (ROOT / item["source_audit_locator"]).exists()

    rt_case, loz_case = cases
    assert rt_case["packet_status"] == (
        "generated_publicly_redistributable_deterministic_production_qualified_local_candidate"
    )
    assert rt_case["source_checkout_clean"] is True
    assert rt_case["producer_clean"] is False
    assert rt_case["expected_result_sha256"] == ("17477a9d1b7005a9bc8a097687fe1a0bf0453f1d8230bf260a8628330af997ad")
    assert rt_case["result_regions"] == 265
    assert rt_case["result_segments"] == 5601
    assert rt_case["failed_jobs"] == rt_case["fallback_count"] == 0
    assert rt_case["promotion_status"] == "incomplete_clean_build_attestation_pending"
    for path_key, digest_key in (
        ("packet_path", "packet_sha256"),
        ("exporter_manifest_path", "exporter_manifest_sha256"),
        ("corpus_path", "corpus_sha256"),
        ("qualification_report_path", "qualification_report_sha256"),
    ):
        artifact = ROOT / rt_case[path_key]
        assert artifact.is_file()
        digest = _lf_sha256(artifact) if path_key == "qualification_report_path" else _sha256(artifact)
        assert digest == rt_case[digest_key]

    vector_directory = (ROOT / rt_case["packet_path"]).parent
    assert {path.name for path in vector_directory.iterdir()} == {
        "README.md",
        "corpus.json",
        "qualification.local-5950x-dirty-build.json",
        "rt_super_c1_pwr4.exporter-manifest.json",
        "rt_super_c1_pwr4.gmabrq01",
    }
    assert all(path.is_file() for path in vector_directory.iterdir())
    assert not tuple(vector_directory.rglob("*.hex"))
    assert not any(path.suffix.lower() in {".pcbdoc", ".gmabrs01"} for path in vector_directory.rglob("*"))

    assert candidate["real_board_rt_packet"] == rt_case["packet_path"]
    assert candidate["real_board_rt_packet_bytes"] == rt_case["packet_bytes"] == 377_160
    assert candidate["real_board_rt_packet_sha256"] == rt_case["packet_sha256"]
    assert candidate["real_board_rt_exporter_manifest"] == rt_case["exporter_manifest_path"]
    assert candidate["real_board_rt_exporter_manifest_bytes"] == (rt_case["exporter_manifest_bytes"])
    assert candidate["real_board_rt_exporter_manifest_sha256"] == (rt_case["exporter_manifest_sha256"])
    assert candidate["real_board_rt_corpus"] == rt_case["corpus_path"]
    assert candidate["real_board_rt_corpus_sha256"] == rt_case["corpus_sha256"]
    assert candidate["real_board_rt_local_qualification_report"] == (rt_case["qualification_report_path"])
    assert candidate["real_board_rt_local_qualification_report_sha256"] == (rt_case["qualification_report_sha256"])
    assert candidate["real_board_rt_expected_result_sha256"] == (rt_case["expected_result_sha256"])
    assert candidate["real_board_rt_result_bytes"] == rt_case["result_bytes"]
    assert candidate["real_board_rt_result_regions"] == rt_case["result_regions"]
    assert candidate["real_board_rt_result_segments"] == rt_case["result_segments"]
    assert candidate["real_board_rt_solver_telemetry_sha256"] == (rt_case["solver_telemetry_sha256"])
    assert candidate["real_board_rt_failed_jobs"] == rt_case["failed_jobs"] == 0
    assert candidate["real_board_rt_fallback_count"] == rt_case["fallback_count"] == 0
    assert candidate["real_board_rt_status"] == rt_case["packet_status"]
    assert candidate["real_board_rt_promotion_status"] == rt_case["promotion_status"]

    assert loz_case["packet_status"] == "pending_blocked"
    assert loz_case["packet_blocker"] == ("governed_exact_copper_selector_and_exporter_case_not_complete")
    assert loz_case["exporter_identity_status"] == "pending"
    assert loz_case["exporter_revision_status"] == "pending"
    assert candidate["real_board_loz_status"] == ("pending_blocked_governed_exact_copper_selector_and_exporter_case")
    assert not any(
        key in loz_case
        for key in (
            "packet_path",
            "packet_sha256",
            "exporter_identity",
            "producer_revision",
        )
    )

    assert candidate["python_production_replay_blocked_cases"] == ["2_missing_authoritative_integer_endpoints"]


def test_c_abi_manifest_matches_header_exactly() -> None:
    manifest = _manifest()
    c_abi = manifest["c_abi"]
    header = (ROOT / c_abi["source"]).read_text(encoding="utf-8")
    declared = re.findall(r"GEOMETER_C_API\s+[\w\s*]+?\b(geometer_\w+)\s*\(", header)
    _unique(declared, "C ABI declaration")
    assert declared == c_abi["symbols"]


def test_exact_algebraic_backend_is_governed_and_non_primary() -> None:
    backend = _manifest()["analytic_exact_backend"]
    assert backend == {
        "status": "implemented_non_primary_oracle",
        "design": "docs/design/exact-real-algebraic-a0.md",
        "design_sha256": "71139fbe41e98fd1c4bab70916fb4fc3f9e720f948d2629815a61bba1433fb8e",
        "conformance_identity": "geometry.exact_real_algebraic.feasibility.a0",
        "magic": "GEXPA001",
        "generation": 1,
        "implementation_allowed": True,
        "boost_version": "1.92.0",
        "boost_archive_url": "https://archives.boost.io/release/1.92.0/source/boost_1_92_0.tar.gz",
        "boost_archive_sha256": "c4a3b310ddd2472416e091067166b0713be97c63f38c212c484ada022fd296ce",
        "boost_upstream_commit": "afdfa32505af73e3d208144b3f623f0096cb62b6",
        "generated_dependency_root": ".deps/boost_1_92_0",
        "production_solver_allowed": False,
        "normal_production_path_allowed": False,
        "production_role": ("conformance_oracle_offline_diagnostics_optional_bounded_fallback"),
        "design_review_revision": "a8c9604de280e2a67018e1106fd1b430b34fcf50",
        "design_review_packet": "reviewer-019ffd1f-3c67-7001-87a5-200b6cda10d8",
        "implemented_surface": "budgeted_exact_intersections_nm_normalization_curve_domain_half_edge_face_classification_stage_lineage_result_region_provenance_operand_outcome_certified_arc_result_normalization_canonical_source_set_result_packet_layout_typed_record_graph_semantic_canonical_projection_standalone_closure_digest_decoder_enforcement_exact_topology_replay_mutation_sentinels_closed_form_invariants_degeneracy_sweeps_boolean_identities_metamorphic_relations_bounded_enumeration_seeded_property_many_to_many_lineage_monthly_extended_seed_quarter_arc_swept_path_clipper_warning_and_swept_area_self_overlap_feasibility",
        "arc_distance_source": "src/cpp/lib/exact_arc_distance.cpp",
        "result_normalization_source": "src/cpp/lib/exact_result_normalization.cpp",
        "source_sets_source": "src/cpp/lib/exact_source_sets.cpp",
        "boolean_outcomes_source": "src/cpp/lib/exact_boolean_outcomes.cpp",
        "boolean_provenance_source": "src/cpp/lib/exact_boolean_provenance.cpp",
        "boolean_regions_source": "src/cpp/lib/exact_boolean_regions.cpp",
        "boolean_stages_source": "src/cpp/lib/exact_boolean_stages.cpp",
        "rational_source": "src/cpp/lib/exact_rational.cpp",
        "polynomial_source": "src/cpp/lib/exact_polynomial.cpp",
        "resultant_source": "src/cpp/lib/exact_resultant.cpp",
        "factorization_source": "src/cpp/lib/exact_factorization.cpp",
        "value_source": "src/cpp/lib/exact_value.cpp",
        "value_codec_source": "src/cpp/lib/exact_value_codec.cpp",
        "expression_source": "src/cpp/lib/exact_expression.cpp",
        "construction_source": "src/cpp/lib/exact_construction.cpp",
        "construction_builder_source": "src/cpp/lib/exact_construction_builder.cpp",
        "artifact_source": "src/cpp/lib/exact_artifact.cpp",
        "geometry_source": "src/cpp/lib/exact_geometry.cpp",
        "normalization_source": "src/cpp/lib/exact_normalization.cpp",
        "curve_domain_source": "src/cpp/lib/exact_curve_domain.cpp",
        "arrangement_source": "src/cpp/lib/exact_arrangement.cpp",
        "arrangement_classification_source": "src/cpp/lib/exact_arrangement_classification.cpp",
        "arrangement_faces_source": "src/cpp/lib/exact_arrangement_faces.cpp",
        "arrangement_order_source": "src/cpp/lib/exact_arrangement_order.cpp",
        "arrangement_result_source": "src/cpp/lib/exact_arrangement_result.cpp",
        "arrangement_validation_source": "src/cpp/lib/exact_arrangement_validation.cpp",
        "rational_test": "tests/cpp/exact_rational_test.cpp",
        "polynomial_test": "tests/cpp/exact_polynomial_test.cpp",
        "resultant_test": "tests/cpp/exact_resultant_test.cpp",
        "factorization_test": "tests/cpp/exact_factorization_test.cpp",
        "value_test": "tests/cpp/exact_value_test.cpp",
        "value_codec_test": "tests/cpp/exact_value_codec_test.cpp",
        "expression_test": "tests/cpp/exact_expression_test.cpp",
        "construction_test": "tests/cpp/exact_construction_test.cpp",
        "artifact_test": "tests/cpp/exact_artifact_test.cpp",
        "geometry_test": "tests/cpp/exact_geometry_test.cpp",
        "geometry_parity_validator": "scripts/validate_exact_geometry_parity.py",
        "geometry_vector_artifact_bytes": 1248,
        "geometry_vector_artifact_sha256": "0af1965a293c1b7a90ec633d35ee98f2d53098350e86e922e8c18751ea15a5e0",
        "geometry_vector_success_work_units": 41182568,
        "curve_domain_test": "tests/cpp/exact_curve_domain_test.cpp",
        "curve_domain_parity_validator": "scripts/validate_exact_curve_domain_parity.py",
        "curve_domain_vector_sha256": "56a531c6731e01d518f85bfdb046becc44d42a52dd81c0e1aab1f1aab917fcd8",
        "curve_domain_vector_success_work_units": 710842,
        "arrangement_test": "tests/cpp/exact_arrangement_test.cpp",
        "arrangement_parity_validator": "scripts/validate_exact_arrangement_parity.py",
        "arrangement_vector_sha256": "b134a245f7d3daa6d4f8e03db1871ec8e99565a500e87ba4c452090951046c23",
        "arrangement_vector_success_work_units": 3051260,
        "arrangement_vector_storage_bytes": 8608,
        "boolean_stages_test": "tests/cpp/exact_boolean_stages_test.cpp",
        "boolean_stages_parity_validator": "scripts/validate_exact_boolean_stages_parity.py",
        "boolean_stages_vector_sha256": "2fb0a89e6226f01bddde8cdb920ec3e2c22f9f3f09b3681fe0f884d64c33a8a5",
        "boolean_stages_vector_success_work_units": 416,
        "boolean_stages_vector_storage_bytes": 4560,
        "boolean_regions_test": "tests/cpp/exact_boolean_regions_test.cpp",
        "boolean_regions_parity_validator": "scripts/validate_exact_boolean_regions_parity.py",
        "boolean_regions_vector_sha256": "b33cc58943c8346a8bc54232fbe7cc4a55ca35d10f9e4038be9be3e88cbacc2f",
        "boolean_regions_vector_success_work_units": 904,
        "boolean_regions_vector_storage_bytes": 5760,
        "boolean_provenance_test": "tests/cpp/exact_boolean_provenance_test.cpp",
        "boolean_provenance_parity_validator": "scripts/validate_exact_boolean_provenance_parity.py",
        "boolean_provenance_vector_sha256": "3e996f29bfeca09f584dc6412670302d92edbd4f3282bc921829b23dd79108e5",
        "boolean_provenance_vector_success_work_units": 4208,
        "boolean_provenance_vector_storage_bytes": 26528,
        "boolean_outcomes_test": "tests/cpp/exact_boolean_outcomes_test.cpp",
        "boolean_outcomes_parity_validator": "scripts/validate_exact_boolean_outcomes_parity.py",
        "boolean_outcomes_vector_sha256": "e8cfe16cf62b1f074537415eb9a8ddfe2d9d7d6bdebb70abe152d8b3d82744af",
        "boolean_outcomes_coexistence_sha256": "08684e05441cfc3af24d33f26747a34b913ee8a7d85b2a92abf2dbb4f637564f",
        "boolean_outcomes_vector_success_work_units": 1600,
        "boolean_outcomes_vector_storage_bytes": 11584,
        "result_normalization_test": "tests/cpp/exact_result_normalization_test.cpp",
        "result_normalization_parity_validator": "scripts/validate_exact_result_normalization_parity.py",
        "result_normalization_vector_sha256": "db3be0a6e7bfc88bde2435bc603569d9dac18d8d320a32e2d0e48a8e00e469ea",
        "result_normalization_mutation_sentinels": ["between_stage_normalization"],
        "source_sets_test": "tests/cpp/exact_source_sets_test.cpp",
        "source_sets_parity_validator": "scripts/validate_exact_source_sets_parity.py",
        "source_sets_vector_sha256": "dc19f5249a91d159f0ed1dff4ff78617315c157050f1df09fe47d834986daffc",
        "source_sets_vector_success_work_units": 5760,
        "source_sets_vector_storage_bytes": 6272,
        "result_packet_layout_source": "src/cpp/lib/analytic_result_packet_layout.cpp",
        "result_packet_layout_test": "tests/cpp/analytic_result_packet_layout_test.cpp",
        "result_packet_layout_parity_validator": "scripts/validate_analytic_result_packet_layout_parity.py",
        "result_packet_empty_bytes": 512,
        "result_packet_empty_sha256": "6098488722c00c2959df6e16d1381cbdcacf79c5bd4d8b9ae2cebdd5a0de8638",
        "result_packet_layout_vector_bytes": 924,
        "result_packet_layout_vector_sha256": "4bce719a8da4fe334ea1bc758a7d726a77deec90976aab62b81e205e4a3f493c",
        "result_packet_records_source": "src/cpp/lib/analytic_result_packet_records.cpp",
        "result_packet_records_test": "tests/cpp/analytic_result_packet_records_test.cpp",
        "result_packet_records_parity_validator": "scripts/validate_analytic_result_packet_records_parity.py",
        "result_packet_record_vector_bytes": 1220,
        "result_packet_record_vector_sha256": "07019a4c2f94f8066db90b6f57bb3e5ea7801bb6b8bda63d8c7268dcad5bcb72",
        "result_packet_canonical_source": "src/cpp/lib/analytic_result_packet_canonical.cpp",
        "result_packet_canonical_vector_bytes": 1220,
        "result_packet_canonical_vector_sha256": "07019a4c2f94f8066db90b6f57bb3e5ea7801bb6b8bda63d8c7268dcad5bcb72",
        "result_packet_batch_owner_key": "owner_job_id_then_existing_complete_semantic_key",
        "result_packet_batch_owner_key_review": "reviewer-01a000c8-8fb8-73b2-93b0-505315c71f09",
        "result_packet_standalone_source": "src/cpp/lib/analytic_result_packet_standalone.cpp",
        "result_packet_sha256_source": "src/cpp/lib/sha256.cpp",
        "result_packet_standalone_vector_bytes": 996,
        "result_packet_standalone_vector_sha256": "a934acefb39e7b397516b4ba948e3ded16f2d30c989a43271ee90883191f4f63",
        "result_packet_failed_standalone_vector_bytes": 616,
        "result_packet_failed_standalone_vector_sha256": "87f193938f0c1d60d8d196eb256adf0fa423f56175be63a61f0ef6cc8bebd46e",
        "result_packet_topology_source": "src/cpp/lib/analytic_result_packet_topology.cpp",
        "result_packet_topology_test": "tests/cpp/analytic_result_packet_topology_test.cpp",
        "result_packet_topology_parity_validator": "scripts/validate_analytic_mutation_sentinels_parity.py",
        "mutation_sentinels": [
            "reversed_line",
            "reversed_arc",
            "non_containing_parent",
            "point_tangent_merge",
            "nested_island_ownership",
            "omitted_lineage",
            "ties_to_even",
            "deep_hierarchy_bound",
            "many_empty_jobs",
        ],
        "degeneracy_sweep_test": "tests/cpp/exact_degeneracy_sweep_test.cpp",
        "degeneracy_sweep_parity_validator": "scripts/validate_exact_degeneracy_sweep_parity.py",
        "degeneracy_sweep_cases": [
            "external_tangency",
            "internal_tangency",
            "concentric_coincidence",
            "near_collinearity",
            "half_grid_ties",
            "arc_0_360_boundary",
            "arc_180_boundary",
            "closed_arc_rejection",
            "permitted_swept_area_self_overlap",
        ],
        "degeneracy_swept_path_width_nm": 10,
        "degeneracy_swept_path_leg_length_nm": 12,
        "degeneracy_swept_path_separations_nm": [9, 10, 11],
        "degeneracy_swept_path_classifications": [
            "overlap_allowed",
            "contact_allowed",
            "disjoint_allowed",
        ],
        "degeneracy_swept_path_end_cap_relations": ["two_points", "point", "disjoint"],
        "degeneracy_swept_path_signed_witness_areas_nm2": [12, 0, -12],
        "degeneracy_sweep_sha256": "f9a5f27116e098ef333f676f2420cca9dd217b022ef2fc7af19dd58e23a07540",
        "boolean_identity_test": "tests/cpp/exact_boolean_identities_test.cpp",
        "boolean_identity_parity_validator": "scripts/validate_exact_boolean_identities_parity.py",
        "boolean_identity_cases": [
            "union_self",
            "difference_self",
            "union_empty",
            "difference_empty",
        ],
        "boolean_identity_sha256": "bf0c289c99026044bfcb6b8d990fbc33306ec61a50763cab1158d312f656027e",
        **_SYNTHETIC_CORRECTNESS_GATE,
    }
    assert (ROOT / backend["design"]).is_file()
    assert _sha256(ROOT / backend["design"]) == backend["design_sha256"]


_EXPECTED_FILTERED_SOLVER: dict[str, Any] = {
    "status": "implemented_filtered_batch_relationships_packed_dispatch",
    "design": "docs/geometer/adr/geometer-adr-013-filtered_resolution_bounded_planar_boolean.md",
    "coordinate_grid_nm": 1,
    "topology_resolution_nm": 50,
    "topology_resolution_caller_programmable": False,
    "production_dispatch_allowed": True,
    "algebraic_fallback_hard_limit": 0,
    "strict_policy_header": "src/cpp/lib/analytic_filtered_execution_policy.h",
    "strict_policy": (
        "private_published_geometry_mode_disables_50nm_topology_repair_and_fails_closed_on_unresolved_equality"
    ),
    "limits_header": "src/cpp/lib/geometer/analytic_solver_limits.h",
    "limits_source": "src/cpp/lib/analytic_solver_limits.cpp",
    "numeric_filter_header": "src/cpp/lib/geometer/analytic_numeric_filter.h",
    "numeric_filter_source": "src/cpp/lib/analytic_numeric_filter.cpp",
    "broad_phase_header": "src/cpp/lib/geometer/analytic_curve_broad_phase.h",
    "broad_phase_source": "src/cpp/lib/analytic_curve_broad_phase.cpp",
    "interval_index_header": "src/cpp/lib/analytic_interval_index.h",
    "interval_index_source": "src/cpp/lib/analytic_interval_index.cpp",
    "foundation_test": "tests/cpp/analytic_filtered_core_test.cpp",
    "broad_phase_policy": "deterministic_sparser_axis_sweep_with_secondary_interval_index",
    "narrow_phase_header": "src/cpp/lib/geometer/analytic_curve_narrow_phase.h",
    "narrow_phase_source": "src/cpp/lib/analytic_curve_narrow_phase.cpp",
    "narrow_phase_policy": "canonical_candidate_pairs_only_constant_work_per_pair",
    "narrow_phase_input": "job_local_filtered_nm_line_and_arc_carriers_with_validated_integer_and_construction_certificates",
    "narrow_phase_uncertain_policy": "job_local_resource_limit_exceeded",
    "narrow_phase_resolution_policy": ("pair_witness_at_or_below_50nm_no_independent_tolerance_composition"),
    "narrow_phase_pair_logical_bytes": 256,
    "narrow_phase_parity_validator": "scripts/validate_analytic_filtered_core_parity.py",
    "narrow_phase_vector_bytes": 304,
    "narrow_phase_vector_sha256": ("6cb876823564bb996984851003e0160306365f4ae2700e5566d7bbb112177824"),
    "interval_arithmetic_header": "src/cpp/lib/analytic_filtered_interval.h",
    "fixed_width_integer_header": "src/cpp/lib/analytic_wide_integer.h",
    "lowering_header": "src/cpp/lib/geometer/analytic_filtered_lowering.h",
    "lowering_source": "src/cpp/lib/analytic_filtered_lowering.cpp",
    "lowering_test": "tests/cpp/analytic_filtered_lowering_test.cpp",
    "lowering_policy": ("direct_job_local_integer_origin_outward_intervals_and_lowering_only_proof_tokens"),
    "lowering_supported_geometry": "authored_regions_disks_annuli_capsules_constant_width_open_swept_paths",
    "lowering_swept_path_policy": ("filtered_indexed_piece_union_zero_algebraic_fallback"),
    "lowering_bounds_policy": "finite_arc_endpoints_plus_contained_cardinal_extrema",
    "lowering_token_policy": (
        "fixed_capacity_open_addressed_exact_construction_keys_and_vertical_column_identities_with_metered_probes"
    ),
    "lowering_logical_bytes_per_curve": 768,
    "lowering_parity_validator": "scripts/validate_analytic_filtered_lowering_parity.py",
    "lowering_vector_bytes": 3952,
    "lowering_vector_sha256": ("9c3dada9b6df8c7e219620a31e5effb363ef0c3468f5db7ae1555852b94ce02d"),
    "overlay_header": "src/cpp/lib/geometer/analytic_filtered_overlay.h",
    "overlay_source": "src/cpp/lib/analytic_filtered_overlay.cpp",
    "overlay_test": "tests/cpp/analytic_filtered_overlay_test.cpp",
    "overlay_input": "canonical_broad_phase_pairs_with_internal_narrow_phase_execution",
    "overlay_policy": "carrier_grouped_sorted_events_indexed_active_memberships",
    "overlay_resolution_policy": (
        "complete_same_carrier_clusters_at_or_below_50nm_without_independent_circle_seam_snaps"
    ),
    "overlay_budget_policy": (
        "allocation_free_combined_minimum_preflight_then_fully_metered_complete_cluster_"
        "narrow_and_overlay_work_and_live_logical_memory"
    ),
    "overlay_parity_validator": "scripts/validate_analytic_filtered_overlay_parity.py",
    "overlay_vector_bytes": 1048,
    "overlay_vector_sha256": ("8c1162b7e14b6a58a2dd8f50c4c4b87767a223632c6393a3a8ace3d0244ac352"),
    "arrangement_header": "src/cpp/lib/geometer/analytic_filtered_arrangement.h",
    "arrangement_source": "src/cpp/lib/analytic_filtered_arrangement.cpp",
    "arrangement_test": "tests/cpp/analytic_filtered_arrangement_test.cpp",
    "arrangement_input": ("filtered_geometry_and_canonical_broad_pairs_with_internal_narrow_overlay"),
    "arrangement_policy": ("indexed_complete_diameter_vertex_clusters_and_fixed_width_certified_half_edge_germs"),
    "arrangement_resolution_policy": ("inclusive_50nm_nontransitive_global_vertex_reconciliation"),
    "arrangement_budget_policy": (
        "single_validated_bulk_metered_allocation_free_downstream_minimum_preflight_then_"
        "phase_accurate_target_independent_memory_and_shared_upstream_work"
    ),
    "arrangement_parity_validator": ("scripts/validate_analytic_filtered_arrangement_parity.py"),
    "arrangement_vector_bytes": 13032,
    "arrangement_vector_sha256": ("8cc7fc278c73aefcd9f480b52e1ab4445e37c972538a1672a23a4d422e8bd1df"),
    "boolean_selection_header": ("src/cpp/lib/geometer/analytic_filtered_boolean_selection.h"),
    "boolean_selection_source": "src/cpp/lib/analytic_filtered_boolean_selection.cpp",
    "boolean_selection_admission_source": ("src/cpp/lib/analytic_filtered_boolean_selection_admission.cpp"),
    "boolean_selection_support_header": ("src/cpp/lib/analytic_filtered_boolean_selection_support.h"),
    "boolean_selection_test": ("tests/cpp/analytic_filtered_boolean_selection_test.cpp"),
    "boolean_selection_input": ("ordered_request_records_and_trusted_filtered_geometry_with_canonical_broad_pairs"),
    "boolean_selection_policy": (
        "certified_vertical_slab_face_ownership_canonical_persistent_coverage_and_incremental_ordered_stage_state"
    ),
    "boolean_selection_complexity_policy": (
        "indexed_sweep_and_sparse_edge_transitions_without_cycle_pair_containment_or_face_by_operand_copies"
    ),
    "boolean_selection_budget_policy": (
        "metered_multi_pass_admission_fixed_capacity_distinct_phase_memory_o1_membership_"
        "ordinals_and_candidate_bounded_split_coverage_reservation"
    ),
    "boolean_selection_parity_validator": ("scripts/validate_analytic_filtered_boolean_selection_parity.py"),
    "boolean_selection_vector_bytes": 12744,
    "boolean_selection_vector_sha256": ("4590aa7d11918f8db25f657d6e5027ffed1c1c0737aff40a3a8cb92ebd252e27"),
    "regions_header": "src/cpp/lib/geometer/analytic_filtered_regions.h",
    "regions_source": "src/cpp/lib/analytic_filtered_regions.cpp",
    "regions_test": "tests/cpp/analytic_filtered_regions_test.cpp",
    "regions_input": ("ordered_request_records_and_trusted_filtered_geometry_with_canonical_broad_pairs"),
    "regions_policy": ("rotation_indexed_selected_boundary_successors_and_component_graph_material_rings"),
    "regions_complexity_policy": (
        "linear_boundary_tracing_and_component_graph_without_seam_walk_or_cycle_pair_containment"
    ),
    "regions_budget_policy": (
        "candidate_bounded_pre_arrangement_region_phase_reservation_then_fixed_capacity_metered_traversals"
    ),
    "regions_parity_validator": "scripts/validate_analytic_filtered_regions_parity.py",
    "regions_vector_bytes": 1160,
    "regions_vector_sha256": ("835631e3af611513e05423e0b29705778e3a87fe2b53a56c4f37c3ca6bacbd9e"),
    "lineage_header": "src/cpp/lib/geometer/analytic_filtered_lineage.h",
    "lineage_source": "src/cpp/lib/analytic_filtered_lineage.cpp",
    "lineage_test": "tests/cpp/analytic_filtered_lineage_test.cpp",
    "lineage_input": "owned_filtered_regions_with_stage_order_coverage_roots",
    "lineage_policy": (
        "coordinate_free_transition_driven_region_boundary_vertex_and_empty_face_subtraction_projection"
    ),
    "lineage_complexity_policy": (
        "indexed_material_component_transitions_and_actual_membership_incidence_without_"
        "dense_root_union_or_face_by_operand_scans"
    ),
    "lineage_budget_policy": (
        "candidate_bounded_pre_arrangement_structural_reservation_then_allocation_free_"
        "exact_source_count_and_fixed_capacity_publication"
    ),
    "lineage_parity_validator": "scripts/validate_analytic_filtered_lineage_parity.py",
    "lineage_vector_bytes": 3672,
    "lineage_vector_sha256": ("6cab24e409b59bb6b27ec4142ab2cd6d2cb6f765561a915d7c01ecbf1bb4b74b"),
    "outcomes_header": "src/cpp/lib/geometer/analytic_filtered_outcomes.h",
    "outcomes_source": "src/cpp/lib/analytic_filtered_outcomes.cpp",
    "outcomes_tracker_header": "src/cpp/lib/analytic_filtered_outcome_tracker.h",
    "outcomes_tracker_source": "src/cpp/lib/analytic_filtered_outcome_tracker.cpp",
    "outcomes_test": "tests/cpp/analytic_filtered_outcomes_test.cpp",
    "outcomes_input": ("owned_filtered_lineage_with_selection_integrated_sparse_positive_area_history"),
    "outcomes_policy": (
        "coordinate_free_nonexclusive_operand_events_with_complete_original_sources_and_"
        "tagged_pre_normalization_topology_handles"
    ),
    "outcomes_complexity_policy": (
        "active_unseen_stage_reporters_and_incidence_projection_without_face_by_operand_replay"
    ),
    "outcomes_budget_policy": (
        "candidate_bounded_pre_arrangement_history_reservation_then_exact_count_fixed_"
        "capacity_reference_and_event_publication"
    ),
    "outcomes_parity_validator": "scripts/validate_analytic_filtered_outcomes_parity.py",
    "outcomes_vector_bytes": 1480,
    "outcomes_vector_sha256": ("98a322779493361e0a9fd1175017ecff7473aa230d3542c63f70692a93081604"),
    "normalization_header": "src/cpp/lib/geometer/analytic_filtered_normalization.h",
    "normalization_source": "src/cpp/lib/analytic_filtered_normalization.cpp",
    "normalization_replay_header": ("src/cpp/lib/analytic_filtered_normalization_replay.h"),
    "normalization_replay_source": ("src/cpp/lib/analytic_filtered_normalization_replay.cpp"),
    "normalization_reconstruction_header": ("src/cpp/lib/analytic_endpoint_arc_reconstruction.h"),
    "normalization_test": "tests/cpp/analytic_filtered_normalization_test.cpp",
    "normalization_input": ("owned_filtered_outcomes_with_retained_certified_arrangement_and_topology"),
    "normalization_policy": (
        "one_time_global_1nm_endpoint_authoritative_publication_with_filtered_whole_arc_"
        "hausdorff_and_strict_zero_repair_topology_replay"
    ),
    "normalization_complexity_policy": (
        "indexed_replay_candidates_and_boundary_to_ring_mapping_without_fragment_pair_or_ring_pair_scans"
    ),
    "normalization_budget_policy": (
        "candidate_bounded_pre_outcomes_reservation_then_exact_fixed_capacity_normalization_and_replay_phases"
    ),
    "normalization_parity_validator": ("scripts/validate_analytic_filtered_normalization_parity.py"),
    "normalization_vector_bytes": 2360,
    "normalization_vector_sha256": ("ea874451d9f86f3c30eae894562a68510f0b99e5fac194c856399d137d437252"),
    "packet_header": "src/cpp/lib/geometer/analytic_filtered_packet.h",
    "packet_source": "src/cpp/lib/analytic_filtered_packet.cpp",
    "packet_sequences_header": "src/cpp/lib/analytic_filtered_packet_sequences.h",
    "packet_sequences_source": "src/cpp/lib/analytic_filtered_packet_sequences.cpp",
    "source_reference_header": "src/cpp/lib/geometer/analytic_source_reference.h",
    "packet_test": "tests/cpp/analytic_filtered_packet_test.cpp",
    "packet_input": ("owned_filtered_normalization_with_lineage_outcomes_and_explicit_topology_maps"),
    "packet_policy": ("coordinate_preserving_canonical_source_set_topology_event_and_standalone_packet_assembly"),
    "packet_complexity_policy": (
        "fixed_width_sort_keys_and_exact_prefix_trie_without_variable_length_comparators_or_face_by_operand_replay"
    ),
    "packet_budget_policy": (
        "candidate_bounded_pre_normalization_reservation_then_exact_fixed_capacity_source_and_packet_publication"
    ),
    "packet_parity_validator": "scripts/validate_analytic_filtered_packet_parity.py",
    "packet_vector_bytes": 4288,
    "packet_vector_sha256": ("a47f82ad1ab38c9e38ea55f38b44849ebd477de31c465726cbd17f5c4047ccfb"),
    "batch_header": "src/cpp/lib/geometer/analytic_filtered_batch.h",
    "batch_source": "src/cpp/lib/analytic_filtered_batch.cpp",
    "batch_test": "tests/cpp/analytic_filtered_batch_test.cpp",
    "batch_input": ("validated_canonical_request_records_with_owned_lowering_broad_phase_and_job_packet_publication"),
    "batch_policy": ("sequential_job_isolation_specialized_job_major_canonical_merge_and_single_batch_encode"),
    "batch_complexity_policy": (
        "deterministic_sorted_id_validation_global_source_sort_and_fixed_capacity_sequence_"
        "interning_without_job_pair_or_variable_prefix_scans"
    ),
    "batch_budget_policy": (
        "separate_per_job_and_batch_live_limits_capacity_based_retained_records_and_"
        "precharged_validation_merge_and_encoding_phases"
    ),
    "batch_relationship_header": "src/cpp/lib/analytic_filtered_relationships.h",
    "batch_relationship_source": "src/cpp/lib/analytic_filtered_relationships.cpp",
    "batch_relationship_policy": (
        "strict_published_geometry_face_coverage_and_incidence_with_fail_closed_unresolved_predicates"
    ),
    "batch_relationship_complexity_policy": (
        "cached_unordered_job_pairs_with_two_color_indexed_broad_phase_and_output_sensitive_region_pair_classification"
    ),
    "batch_relationship_budget_policy": (
        "precharged_target_independent_work_memory_candidate_cache_and_remaining_packet_byte_limits"
    ),
    "batch_parity_validator": "scripts/validate_analytic_filtered_batch_parity.py",
    "batch_vector_bytes": 1636,
    "batch_vector_sha256": ("a4b6a8c4a82f77c5de4e232e0a2e1520a57e7370422ddc7e4059951d192a05d9"),
    "batch_relationship_vector_bytes": 5448,
    "batch_relationship_vector_sha256": ("bddccaac7ac1f8b141e007574e08d282a03221b85cb684e9addaf49fa89be073"),
    "production_exact_source_policy": "exact_oracle_sources_excluded_from_geometer_lib",
    "exact_oracle_target": "geometer_exact_feasibility",
}


def test_filtered_solver_foundation_is_bounded_non_algebraic_and_packed_dispatched() -> None:
    solver = _manifest()["analytic_filtered_solver"]
    assert solver == _EXPECTED_FILTERED_SOLVER
    implementation_paths = [
        solver[key]
        for key in (
            "limits_header",
            "limits_source",
            "numeric_filter_header",
            "numeric_filter_source",
            "broad_phase_header",
            "broad_phase_source",
            "interval_index_header",
            "interval_index_source",
            "narrow_phase_header",
            "narrow_phase_source",
            "interval_arithmetic_header",
            "fixed_width_integer_header",
            "lowering_header",
            "lowering_source",
            "overlay_header",
            "overlay_source",
            "arrangement_header",
            "arrangement_source",
            "boolean_selection_header",
            "boolean_selection_source",
            "boolean_selection_admission_source",
            "boolean_selection_support_header",
            "regions_header",
            "regions_source",
            "lineage_header",
            "lineage_source",
            "outcomes_header",
            "outcomes_source",
            "outcomes_tracker_header",
            "outcomes_tracker_source",
            "normalization_header",
            "normalization_source",
            "normalization_replay_header",
            "normalization_replay_source",
            "normalization_reconstruction_header",
            "packet_header",
            "packet_source",
            "packet_sequences_header",
            "packet_sequences_source",
            "batch_header",
            "batch_source",
            "source_reference_header",
        )
    ]
    for relative_path in implementation_paths:
        source = (ROOT / relative_path).read_text(encoding="utf-8")
        assert "boost::" not in source
        assert "multiprecision" not in source
        assert "geometer/exact" not in source
    assert all(
        (ROOT / solver[key]).is_file()
        for key in (
            "design",
            "foundation_test",
            "narrow_phase_parity_validator",
            "lowering_test",
            "lowering_parity_validator",
            "overlay_test",
            "overlay_parity_validator",
            "arrangement_test",
            "arrangement_parity_validator",
            "boolean_selection_test",
            "boolean_selection_parity_validator",
            "regions_test",
            "regions_parity_validator",
            "lineage_test",
            "lineage_parity_validator",
            "outcomes_test",
            "outcomes_parity_validator",
            "normalization_test",
            "normalization_parity_validator",
            "normalization_reconstruction_header",
            "packet_test",
            "packet_parity_validator",
            "batch_test",
            "batch_parity_validator",
            "batch_relationship_header",
            "batch_relationship_source",
        )
    )
