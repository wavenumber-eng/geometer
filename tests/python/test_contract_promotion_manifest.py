from __future__ import annotations

import hashlib
import json
import re
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


def test_manifest_promoted_and_candidate_surfaces_are_complete() -> None:
    manifest = _manifest()
    toolchain = manifest["toolchain"]
    typescript = manifest["typescript_projection"]
    assert typescript["status"] == "implemented_model_bounds_worker_pilot"
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
        "viz_migration_guide",
    ):
        assert (ROOT / typescript[key]).exists(), key
    package_json = json.loads((ROOT / typescript["package_root"] / "package.json").read_text(encoding="utf-8"))
    assert package_json["name"] == manifest["packages"]["typescript"]
    assert manifest["packages"]["typescript_module_format"] == "esm"
    assert package_json["type"] == "module"

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
        assert _sha256(ROOT / path) == evidence[key]

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
        "direct_lowering_broad_and_narrow_phase_implemented_not_dispatched"
    )
    assert candidate["request_contract"] == "geometry.analytic_planar_boolean_batch.request.a0"
    assert candidate["result_contract"] == "geometry.analytic_planar_boolean_batch.result.a0"
    assert candidate["request_contract"] in contract_ids
    assert candidate["result_contract"] in contract_ids
    assert candidate["input_attachments"] == ["analytic_planar_boolean_request"]
    assert candidate["output_attachments"] == ["analytic_planar_boolean_result"]
    assert candidate["deferred_projections"] == ["cpp", "typescript", "rust", "python"]
    assert candidate["packed_format"] == "separately_governed_frozen_a0"
    assert candidate["implementation_gate"] == "cpp_wasm_rust_python_implementation_slices_pending"
    assert candidate["transport"] == "generic_named_attachments"
    assert candidate["operation_specific_c_abi_symbol"] is False
    assert candidate["replaces_existing_operation"] is False
    assert candidate["browser_target"] == "full_browser"
    assert (ROOT / candidate["compatibility_snapshot"]).is_file()
    assert (ROOT / candidate["design"]).is_file()
    assert (ROOT / candidate["packet_spec"]).is_file()
    assert (ROOT / candidate["numeric_catalog"]).is_file()
    assert (ROOT / candidate["superseded_solver_adr"]).is_file()
    assert (ROOT / candidate["feasibility_test"]).is_file()
    assert (ROOT / candidate["portable_fixture"]).is_file()
    assert (ROOT / candidate["independent_design_review_log"]).is_file()
    assert candidate["independent_design_review_revision"] == "529c768e559b4c88874264748d4186e775c8a4dd"
    assert candidate["independent_design_review_head"] == "b86a065c5926c35f1eee23a9ba1cef890689c7d7"
    assert candidate["typespec_projection_review_revision"] == "f4b6a9b87bf16f57ef29dae22150b16f2a742b64"
    assert candidate["typespec_projection_review_packet"] == "reviewer-019ffd0d-fa76-74b6-ac3e-c1c2642ba0de"
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

    for demo in manifest["demos"]:
        assert (ROOT / demo["source"]).is_file()
        if "worker" in demo:
            assert (ROOT / demo["worker"]).is_file()
        if "entrypoint" in demo:
            assert (ROOT / demo["entrypoint"]).is_file()
        assert demo["owning_operation"] in operation_ids

    consumers = manifest["consumers"]
    consumer_ids = [item["id"] for item in consumers]
    _unique(consumer_ids, "consumer id")
    assert consumer_ids == ["appz.viz", "appz.data_models.pcb.matz"]
    assert all((ROOT / item["snapshot"]).is_file() for item in consumers)


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


def test_filtered_solver_foundation_is_bounded_non_algebraic_and_not_dispatched() -> None:
    solver = _manifest()["analytic_filtered_solver"]
    assert solver == {
        "status": "implemented_direct_lowering_broad_and_narrow_phase_not_dispatched",
        "design": "docs/adr/013_filtered_resolution_bounded_planar_boolean.md",
        "coordinate_grid_nm": 1,
        "topology_resolution_nm": 50,
        "topology_resolution_caller_programmable": False,
        "production_dispatch_allowed": False,
        "algebraic_fallback_hard_limit": 0,
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
        "narrow_phase_pair_logical_bytes": 256,
        "narrow_phase_parity_validator": "scripts/validate_analytic_filtered_core_parity.py",
        "narrow_phase_vector_bytes": 216,
        "narrow_phase_vector_sha256": ("140760f79dfb64aca3bb68c8f849659d81fa590ae49f9805f707bc3990b86144"),
        "interval_arithmetic_header": "src/cpp/lib/analytic_filtered_interval.h",
        "fixed_width_integer_header": "src/cpp/lib/analytic_wide_integer.h",
        "lowering_header": "src/cpp/lib/geometer/analytic_filtered_lowering.h",
        "lowering_source": "src/cpp/lib/analytic_filtered_lowering.cpp",
        "lowering_test": "tests/cpp/analytic_filtered_lowering_test.cpp",
        "lowering_policy": (
            "direct_job_local_integer_origin_outward_intervals_and_lowering_only_proof_tokens"
        ),
        "lowering_supported_geometry": "authored_regions_disks_annuli_capsules",
        "lowering_swept_path_policy": (
            "job_local_unsupported_until_filtered_indexed_piece_union"
        ),
        "lowering_bounds_policy": "finite_arc_endpoints_plus_contained_cardinal_extrema",
        "lowering_token_policy": (
            "fixed_capacity_open_addressed_exact_construction_keys_with_metered_probes"
        ),
        "lowering_logical_bytes_per_curve": 768,
        "lowering_parity_validator": "scripts/validate_analytic_filtered_lowering_parity.py",
        "lowering_vector_bytes": 3552,
        "lowering_vector_sha256": (
            "c3bdb4e5fa66fae4677b7a5088cb645cd81d8bf09a927cbe331d2d4c5015b059"
        ),
    }
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
        )
    )


def test_exact_algebraic_backend_paths_exist() -> None:
    backend = _manifest()["analytic_exact_backend"]
    for key in (
        "rational_source",
        "polynomial_source",
        "expression_source",
        "construction_source",
        "construction_builder_source",
        "artifact_source",
        "geometry_source",
        "normalization_source",
        "curve_domain_source",
        "arc_distance_source",
        "source_sets_source",
        "result_packet_layout_source",
        "result_packet_records_source",
        "result_packet_canonical_source",
        "result_packet_standalone_source",
        "result_packet_sha256_source",
        "result_packet_topology_source",
        "boolean_outcomes_source",
        "result_normalization_source",
        "boolean_provenance_source",
        "boolean_regions_source",
        "boolean_stages_source",
        "arrangement_source",
        "arrangement_classification_source",
        "arrangement_faces_source",
        "arrangement_order_source",
        "arrangement_result_source",
        "arrangement_validation_source",
        "rational_test",
        "polynomial_test",
        "expression_test",
        "construction_test",
        "artifact_test",
        "geometry_test",
        "geometry_parity_validator",
        "curve_domain_test",
        "curve_domain_parity_validator",
        "arrangement_test",
        "arrangement_parity_validator",
        "boolean_stages_test",
        "boolean_stages_parity_validator",
        "boolean_regions_test",
        "boolean_regions_parity_validator",
        "boolean_provenance_test",
        "boolean_provenance_parity_validator",
        "boolean_outcomes_test",
        "boolean_outcomes_parity_validator",
        "boolean_lineage_matrix_test",
        "boolean_lineage_matrix_parity_validator",
        "result_normalization_test",
        "result_normalization_parity_validator",
        "source_sets_test",
        "source_sets_parity_validator",
        "result_packet_layout_test",
        "result_packet_layout_parity_validator",
        "result_packet_records_test",
        "result_packet_records_parity_validator",
        "result_packet_topology_test",
        "result_packet_topology_parity_validator",
        "closed_form_invariants_test",
        "closed_form_invariants_parity_validator",
        "degeneracy_sweep_test",
        "degeneracy_sweep_parity_validator",
        "boolean_identity_test",
        "boolean_identity_parity_validator",
        "boolean_metamorphic_test",
        "boolean_metamorphic_parity_validator",
        "seeded_property_test",
        "seeded_property_parity_validator",
        "nightly_seeded_property_workflow",
        "rectangle_enumeration_test",
        "rectangle_enumeration_parity_validator",
    ):
        assert (ROOT / backend[key]).is_file()


def test_exact_geometry_contains_binary_allocation_failures() -> None:
    geometry = (ROOT / "src/cpp/lib/exact_geometry.cpp").read_text(encoding="utf-8")
    construction = (ROOT / "src/cpp/lib/exact_construction.cpp").read_text(encoding="utf-8")
    assert "make_sum({left, right})" not in geometry
    assert "make_product({left, right})" not in geometry
    assert geometry.count("catch (const std::exception&)") == 3
    assert "children.reserve(2);" in construction
    assert "return make_associative(kind, children);" in construction


def test_wasm_export_inventory_matches_cmake_exactly() -> None:
    manifest = _manifest()
    cmake = (ROOT / "src" / "cpp" / "lib" / "CMakeLists.txt").read_text(encoding="utf-8")
    export_lists = re.findall(r'-sEXPORTED_FUNCTIONS=\[(.*?)\]"', cmake)
    assert len(export_lists) == 2
    actual = [[item.strip("'") for item in value.split(",")] for value in export_lists]
    assert actual[0] == manifest["wasm"]["full_browser"]["exports"]
    assert actual[1] == manifest["wasm"]["planar_browser"]["exports"]


def test_packet_inventory_matches_implementation_constants() -> None:
    manifest = _manifest()
    for packet in manifest["binary_formats"]:
        source = (ROOT / packet["source"]).read_text(encoding="utf-8")
        request_chars = ", ".join(f"'{value}'" for value in packet["request_magic"])
        response_chars = ", ".join(f"'{value}'" for value in packet["response_magic"])
        assert "{" + request_chars + "}" in source
        assert "{" + response_chars + "}" in source
        assert f"FORMAT_VERSION = {packet['version']};" in source


def test_cli_compatibility_names_are_still_dispatched() -> None:
    manifest = _manifest()
    cli = (ROOT / "src" / "cpp" / "cli" / "main.cpp").read_text(encoding="utf-8")
    names = {
        name
        for operation in manifest["operations"]
        for key in ("cli_names", "compatibility_cli_names")
        for name in operation[key]
    }
    for name in names:
        assert f'"{name}"' in cli


def test_viz_2026_6_10_compatibility_snapshot_is_preserved() -> None:
    manifest = _manifest()
    viz = next(item for item in manifest["consumers"] if item["id"] == "appz.viz")
    snapshot_path = ROOT / viz["snapshot"]
    with snapshot_path.open("rb") as stream:
        snapshot = tomllib.load(stream)

    assert snapshot["consumer"] == "appz/viz"
    assert snapshot["geometer_release"] == "2026.6.10"
    assert snapshot["migration"]["target_package"] == manifest["packages"]["typescript"]

    expected_artifact_pairs = {
        "dist/wasm/browser/geometer.js": "geometer-browser.js",
        "dist/wasm/browser/geometer.wasm": "geometer-browser.wasm",
        "dist/wasm/planar-browser/geometer-planar-browser.js": ("geometer-planar-browser.js"),
        "dist/wasm/planar-browser/geometer-planar-browser.wasm": ("geometer-planar-browser.wasm"),
    }
    assert (
        dict(
            zip(
                snapshot["artifacts"]["source_paths"],
                snapshot["artifacts"]["vendored_names"],
                strict=True,
            )
        )
        == expected_artifact_pairs
    )

    mappings = snapshot["artifact_mappings"]
    assert {item["source"]: item["vendored"] for item in mappings} == (expected_artifact_pairs)
    assert {(item["target"], item["kind"]) for item in mappings} == {
        ("full_browser", "javascript"),
        ("full_browser", "wasm"),
        ("planar_browser", "javascript"),
        ("planar_browser", "wasm"),
    }
    for item in mappings:
        artifact = ROOT / item["source"]
        assert artifact.is_file()
        assert artifact.stat().st_size > 0

    required_symbols = set(snapshot["wasm"]["required_c_abi_symbols"])
    assert required_symbols <= set(manifest["c_abi"]["symbols"])

    cmake = (ROOT / "src" / "cpp" / "lib" / "CMakeLists.txt").read_text(encoding="utf-8")
    for factory in snapshot["artifacts"]["factory_names"]:
        assert f"-sEXPORT_NAME={factory}" in cmake

    javascript_mappings = [item for item in mappings if item["kind"] == "javascript"]
    assert [item["factory"] for item in javascript_mappings] == snapshot["artifacts"]["factory_names"]
    required_memory_views = snapshot["wasm"]["required_memory_views"]
    assert required_memory_views == ["HEAPU8", "HEAPU32"]
    for item in javascript_mappings:
        loader = (ROOT / item["source"]).read_text(encoding="utf-8")
        assert item["factory"] in loader
        for view in required_memory_views:
            assert view in loader

    runtime_lists = re.findall(r'-sEXPORTED_RUNTIME_METHODS=\[(.*?)\]"', cmake)
    assert len(runtime_lists) == 2
    runtime_methods = [{item.strip("'") for item in value.split(",")} for value in runtime_lists]
    required_methods = set(snapshot["wasm"]["required_runtime_methods"])
    assert all(required_methods <= methods for methods in runtime_methods)

    assert snapshot["wasm"]["manifest_schema"] == "wn.viz.vendor.geometer.browser.a0"
    capabilities = snapshot["wasm"]["manifest_capabilities"]
    _unique(capabilities, "Viz manifest capability")
    expected_capabilities = ["version"] + [
        symbol.removeprefix("geometer_")
        for symbol in snapshot["wasm"]["required_c_abi_symbols"]
        if symbol
        not in {
            "geometer_version_string",
            "geometer_abi_version",
            "geometer_free_string",
            "geometer_free_bytes",
        }
    ]
    assert capabilities == expected_capabilities
    assert {"geometer_version_string", "geometer_abi_version"} <= required_symbols

    versions = {item["id"]: item["version"] for item in manifest["binary_formats"]}
    assert snapshot["packed_formats"] == {
        "planar_batch_version": versions["geometry.planar_batch.packet"],
        "planar_triangulate_version": versions["geometry.planar_triangulate.packet"],
        "clipper2_boolean_version": versions["geometry.clipper2_boolean.packet"],
        "clipper2_inflate_open_version": versions["geometry.clipper2_inflate_open.packet"],
    }


def test_data_models_geom_a0_requirements_snapshot_is_frozen() -> None:
    manifest = _manifest()
    matz = next(item for item in manifest["consumers"] if item["id"] == "appz.data_models.pcb.matz")
    snapshot_path = ROOT / matz["snapshot"]
    with snapshot_path.open("rb") as stream:
        snapshot = tomllib.load(stream)

    assert snapshot["snapshot_status"] == "requirements_input"
    assert snapshot["consumer"] == "appz/data_models/pcb/matz"
    assert snapshot["source_repository"] == "wavenumber-eng/appz"
    assert snapshot["source_component"] == "data_models"
    assert snapshot["source_branch"] == "pcb-matz-viz-data-models"
    assert snapshot["source_revision_publication"] == "published_origin_branch"
    assert snapshot["source_revision"] == "fabbf70e1970adb7fa74f3be64c4ef45e2b89154"
    assert snapshot["runtime_sibling_dependency"] is False
    assert snapshot["geom_contract"]["schema_identity"] == ("urn:wavenumber:schema:geom_a0")
    geom_contract = snapshot["geom_contract"]
    assert geom_contract["mapping_status"] == "accepted"
    assert geom_contract["mapping_review_commit"] == "433bad5"
    assert geom_contract["mapping_review_packet"] == ("reviewer-019ffce8-ac66-76c0-877d-3fcb5c1aa6c5")
    assert geom_contract["consumer_confirmation"] == ("joint_semantic_and_fixture_review_complete_no_known_blocker")
    mapping_report = ROOT / geom_contract["mapping_report"]
    assert mapping_report.is_file()
    assert _sha256(mapping_report) == geom_contract["mapping_report_sha256"]
    mapping_text = mapping_report.read_text(encoding="utf-8")
    for required_boundary in (
        "PointNm",
        "packet-local uint64",
        "two-half-arc",
        "round caps and joins only",
        "minor arc or semicircle",
        "requested directed sweep",
        "length - width",
        "A^T A = s^2 I",
        "W' = s W",
        "one-nanometer grid",
        "fail closed",
        "PCB source semantics",
        "tagged Geometer release",
        geom_contract["mapping_review_packet"],
    ):
        assert required_boundary in mapping_text

    source_files = snapshot["source_files"]
    source_paths = [item["path"] for item in source_files]
    _unique(source_paths, "Geom A0 snapshot source path")
    assert source_paths == [
        "contracts/geom/geom_a0.schema.json",
        "docs/geom/adr/geom-adr-0003-topology-first-ring-path-geometry-contract.md",
        "docs/geom/adr/geom-adr-0004-analytic-circle-disk-annulus-geometry-contract.md",
        "docs/geom/adr/geom-adr-0005-topology-preservation-and-late-flattening-policy.md",
        "docs/pcb/plans/auxiliary/matz-geometer-analytic-planar-boolean-requirements-2026-08-12.md",
    ]
    assert {item["path"]: item["sha256"] for item in source_files} == {
        "contracts/geom/geom_a0.schema.json": ("acd688d81c5445445c6ee3f324000009e3976b4bbaa19895d6f2d52fc0d0ef3b"),
        "docs/geom/adr/geom-adr-0003-topology-first-ring-path-geometry-contract.md": (
            "8ccf5623941dc2ed21a58268021acf88bc5d90bedabb93ada31d6bb5a93e2482"
        ),
        "docs/geom/adr/geom-adr-0004-analytic-circle-disk-annulus-geometry-contract.md": (
            "95d6e23a8a5fd63ee8f17a14b65a44eac2cb8f02d980d00a0bf26720b97e5fb9"
        ),
        "docs/geom/adr/geom-adr-0005-topology-preservation-and-late-flattening-policy.md": (
            "b6c39d4485abffcab8d110365e17fe0510596aaf26b492f3d2b850aa4392e05b"
        ),
        "docs/pcb/plans/auxiliary/matz-geometer-analytic-planar-boolean-requirements-2026-08-12.md": (
            "cfc06ccd6fe9ddef7a590a02984f58235d260f4303de71cfea00bf8b654a44ec"
        ),
    }
    assert {item["id"] for item in source_files if item["role"] == "accepted_adr"} == {
        "geom-adr-0003",
        "geom-adr-0004",
        "geom-adr-0005",
    }

    operation = snapshot["operation_candidate"]
    candidate = manifest["candidate_operations"][0]
    assert operation["id"] == candidate["id"]
    assert operation["request_contract"] == "unfrozen"
    assert operation["result_contract"] == "unfrozen"
    assert operation["transport"] == "generic_named_attachments"
    assert operation["operation_specific_c_abi_symbol"] is False
    assert snapshot["solver_feasibility"] == {
        "initial_candidate": "occt",
        "clipper2_role": "sampled_non_authoritative_oracle_only",
        "status": "prototype_complete_joint_reviewed",
    }
    assert snapshot["adoption"]["production_switch_requires_tagged_release"] is True

    fixture_input = snapshot["portable_fixture_input"]
    assert fixture_input == {
        "status": "joint_reviewed_design_input",
        "source_revision": "4c688e46729015d21dc140dbe274e396e3717c18",
        "source_path": "tests/fixtures/pcb_materialization/geometer_analytic_planar_boolean_observations_a0.json",
        "sha256": "10a97f0eed4a4f6852917c4fb6abd35854142bf5d148b8320c60a44f765414c4",
        "vendored_path": "tests/fixtures/analytic_planar_boolean/matz_observations_a0.json",
        "vendored_sha256": "10a97f0eed4a4f6852917c4fb6abd35854142bf5d148b8320c60a44f765414c4",
        "portable_case_count": 10,
        "real_board_case_count": 2,
        "case_2_oracle": "success",
        "case_2_oracle_evidence": "native_wasm_occt_feasibility_signature",
        "case_2_geometer_design_revision": ("182f5f2163e4085200adec779f98b6d3cc7c0e13"),
        "case_2_oracle_signature_sha256": ("c21b03c1b42a6cb3212cec5b3051987f645e21062eddecc82d3e3b0e0fd6dfc7"),
        "case_2_canonical_fragment_count": 12,
        "normalization_policy": "certified_once_after_final_stage",
        "normalization_collapse_case": "job_local_failure",
        "normal_build_sibling_dependency": False,
    }
    vendored_fixture = ROOT / fixture_input["vendored_path"]
    assert candidate["portable_fixture"] == fixture_input["vendored_path"]
    assert _sha256(vendored_fixture) == fixture_input["vendored_sha256"] == fixture_input["sha256"]

    plan = (ROOT / "docs" / "plans" / "geometer-typespec-contracts" / "plan.md").read_text(encoding="utf-8")
    assert operation["id"] in plan
    assert matz["snapshot"] in plan or snapshot_path.name in plan


def test_vendored_matz_analytic_boolean_observations_are_structurally_closed() -> None:
    manifest = _manifest()
    candidate = manifest["candidate_operations"][0]
    fixture = json.loads((ROOT / candidate["portable_fixture"]).read_text(encoding="utf-8"))

    assert fixture["type"] == "matz.geometer.analytic_planar_boolean_observation_manifest"
    assert fixture["version"] == "a0"
    assert fixture["capability"] == candidate["id"]
    assert fixture["status"] == "consumer_observations_pre_typespec"
    assert fixture["coordinate_unit"] == "nm"
    assert fixture["angle_unit"] == "microdegree"
    assert fixture["id_policy"] == {
        "storage": "nonzero_uint64",
        "scope": "per_declared_id_space_per_batch",
        "typescript": "bigint_or_BigUint64Array_never_number",
        "stable_identity": False,
    }
    assert fixture["batch_failure_policy"] == {
        "untrusted_frame": "reject_batch",
        "isolated_geometry_or_normalization_failure": "fail_job_continue_batch",
        "dependent_relationship_query": "skipped_dependency_failed",
    }

    cases = fixture["portable_cases"]
    assert len(cases) == 10
    assert [item["fixture_id"] for item in cases] == [
        "line_add_subtract_add",
        "intersecting_arbitrary_angle_arcs",
        "analytic_primitive_family",
        "nested_holes_and_islands",
        "tangent_coincident_overlap_matrix",
        "normalization_collision",
        "many_to_many_disconnected_results",
        "conductive_domain_contact_queries",
        "successful_requested_empty",
        "mixed_batch_equivalence",
    ]

    all_jobs: list[dict[str, Any]] = []
    all_queries: list[dict[str, Any]] = []
    stage_ids: list[int] = []
    operand_ids: list[int] = []
    source_ids: dict[str, list[int]] = {
        "ring": [],
        "path": [],
        "segment": [],
        "curve": [],
        "feature": [],
        "vertex": [],
    }
    geometry_kinds: set[str] = set()
    stage_operations: set[str] = set()
    for case in cases:
        jobs = case.get("jobs", [case] if "job_id" in case else [])
        all_jobs.extend(jobs)
        all_queries.extend(case.get("relationship_queries", []))
        for job in jobs:
            _assert_nonzero_uint64(job["job_id"], "job id")
            for stage in job["stages"]:
                _assert_nonzero_uint64(stage["stage_id"], "stage id")
                stage_ids.append(stage["stage_id"])
                stage_operations.add(stage["operation"])
                assert stage["operation"] in {"union", "difference"}
                for operand in stage["operands"]:
                    _assert_nonzero_uint64(operand["operand_id"], "operand id")
                    operand_ids.append(operand["operand_id"])
                    geometry_kinds.add(operand["geometry"]["kind"])
                    if "source_topology" in operand:
                        _collect_explicit_source_ids(operand["source_topology"], source_ids)

    job_ids = [job["job_id"] for job in all_jobs]
    _unique(job_ids, "portable job id")
    _unique(stage_ids, "portable stage id")
    _unique(operand_ids, "portable operand id")
    for label, values in source_ids.items():
        _unique(values, f"portable authored {label} id")
        for value in values:
            _assert_nonzero_uint64(value, f"authored {label} id")
    assert {label: len(values) for label, values in source_ids.items()} == {
        "ring": 12,
        "path": 1,
        "segment": 28,
        "curve": 28,
        "feature": 0,
        "vertex": 0,
    }
    assert stage_operations == {"union", "difference"}
    assert geometry_kinds == {
        "rectangle",
        "region",
        "disk",
        "annulus",
        "capsule",
        "arc_sweep",
        "line_arc_swept_path",
    }

    query_ids = [query["query_id"] for query in all_queries]
    _unique(query_ids, "portable relationship query id")
    known_job_ids = set(job_ids)
    for query in all_queries:
        _assert_nonzero_uint64(query["query_id"], "relationship query id")
        _assert_nonzero_uint64(query["left_job_id"], "relationship left job reference")
        _assert_nonzero_uint64(query["right_job_id"], "relationship right job reference")
    assert all(query[side] in known_job_ids for query in all_queries for side in ("left_job_id", "right_job_id"))

    arc_case = next(item for item in cases if item["fixture_id"] == "intersecting_arbitrary_angle_arcs")
    arc_expected = arc_case["expected"]
    assert len(arc_expected["canonical_fragments"]) == 12
    assert arc_expected["native_wasm_feasibility_signature_sha256"] == (
        "c21b03c1b42a6cb3212cec5b3051987f645e21062eddecc82d3e3b0e0fd6dfc7"
    )
    signature = (ROOT / "tests/fixtures/analytic_planar_boolean/feasibility_signature_a0.txt").read_text(
        encoding="utf-8"
    )
    assert "matz_endpoint_fragments|" + "|".join(arc_expected["canonical_fragments"]) in signature

    real_board_cases = fixture["real_board_cases"]
    assert len(real_board_cases) == 2
    assert [item["fixture_id"] for item in real_board_cases] == [
        "rt_super_c1_pwr4",
        "loz_old_man_curved_copper",
    ]
    for item in real_board_cases:
        assert re.fullmatch(r"[0-9a-f]{64}", item["source_sha256"])
        assert item["source_bytes"] > 0
        assert set(item["applicable_geometry_preflight"]["forbidden_count"].values()) == {0}

    budget = fixture["performance_budget"]
    assert budget["correctness_precedence"] is True
    assert budget["design_target_wall_seconds"] == 5
    assert budget["design_target_peak_bytes"] == 1_073_741_824


def test_analytic_planar_boolean_numeric_catalog_is_closed() -> None:
    manifest = _manifest()
    candidate = manifest["candidate_operations"][0]
    assert (ROOT / candidate["solver_adr"]).is_file()
    with (ROOT / candidate["numeric_catalog"]).open("rb") as stream:
        catalog = tomllib.load(stream)

    assert catalog["catalog_version"] == 1
    assert catalog["status"] == "structural_frozen_numeric_reopened"
    assert catalog["operation_identity"] == candidate["id"]
    assert catalog["request_magic"] == "GMABRQ01"
    assert catalog["result_magic"] == "GMABRS01"

    assert catalog["table_kind"]["request"] == {
        "jobs": 1,
        "stages": 2,
        "operands": 3,
        "planar_regions": 4,
        "ring_references": 5,
        "rings": 6,
        "authored_vertices": 7,
        "authored_segments": 8,
        "disks": 9,
        "annuli": 10,
        "capsules": 11,
        "swept_paths": 12,
        "relationship_queries": 13,
    }
    assert catalog["table_kind"]["result"] == {
        "job_results": 101,
        "diagnostics": 102,
        "result_vertices": 103,
        "directed_fragments": 104,
        "result_rings": 105,
        "fragment_references": 106,
        "result_regions": 107,
        "ring_region_references": 108,
        "source_sets": 109,
        "source_references": 110,
        "operand_outcome_events": 111,
        "relationship_results": 112,
        "relationship_region_pairs": 113,
        "source_reference_indices": 114,
    }
    assert catalog["record_size"]["request"]["operands"] == 24
    assert catalog["record_size"]["result"]["directed_fragments"] == 48
    assert catalog["record_size"]["result"]["result_regions"] == 24
    assert catalog["record_size"]["result"]["source_sets"] == 8
    assert catalog["record_size"]["result"]["source_reference_indices"] == 4
    assert catalog["record_size"]["result"]["operand_outcome_events"] == 48
    assert catalog["required_table_kinds"] == {
        "request": list(range(1, 14)),
        "result": list(range(101, 115)),
    }
    assert catalog["enum"]["diagnostic_scope"] == {
        "underlying": "u8",
        "job": 1,
    }
    assert catalog["enum"]["source_kind"] == {
        "underlying": "u16",
        "authored_segment_curve": 1,
        "compact_feature_role": 2,
        "subtractive_operand_effect": 3,
    }
    assert catalog["enum"]["source_role"] == {
        "underlying": "u16",
        "none": 0,
        "authored_line": 1,
        "authored_circular_arc": 2,
        "primitive_outer_circle": 16,
        "primitive_inner_circle": 17,
        "capsule_left_line": 32,
        "capsule_end_cap": 33,
        "capsule_right_line": 34,
        "capsule_start_cap": 35,
        "swept_left_offset_line": 48,
        "swept_left_offset_arc": 49,
        "swept_right_offset_line": 50,
        "swept_right_offset_arc": 51,
        "swept_round_join": 52,
        "swept_start_cap": 53,
        "swept_end_cap": 54,
    }

    for flag_set in catalog["flags"].values():
        allowed_mask = flag_set["allowed_mask"]
        for name, value in flag_set.items():
            if name not in {"underlying", "allowed_mask"}:
                assert value != 0, name
                assert value & ~allowed_mask == 0, name

    operation_codes = catalog["operation_diagnostic"]
    _unique(list(operation_codes.values()), "analytic Boolean operation diagnostic")
    assert set(catalog["operation_diagnostic_identity"]) == set(operation_codes)
    assert all(
        identity.startswith("geometer.operation.analytic_planar_boolean.")
        for identity in catalog["operation_diagnostic_identity"].values()
    )
    assert "invalid_id" not in operation_codes
    assert "invalid_reference" not in operation_codes
    assert "normalization_ambiguous_tie" not in operation_codes
    assert catalog["reserved"]["operation_diagnostic"] == {
        "normalization_ambiguous_tie": 65542,
    }
    assert catalog["contract_diagnostic"]["invalid_id"].startswith("geometer.contract.")
    assert catalog["contract_diagnostic"]["invalid_reference"].startswith("geometer.contract.")
    assert catalog["source_reference_mapping"] == {
        "authored_segment_curve": {
            "primary_id": "authored_segment_id",
            "secondary_id": "authored_curve_id",
            "allowed_roles": [1, 2],
        },
        "compact_feature_role": {
            "primary_id": "compact_feature_id",
            "secondary_id": "boundary_occurrence_key",
            "allowed_roles": [16, 17, 32, 33, 34, 35, 48, 49, 50, 51, 52, 53, 54],
        },
        "subtractive_operand_effect": {
            "primary_id": "stage_id",
            "secondary_id": "zero",
            "allowed_roles": [0],
        },
    }
    assert catalog["operand_event_semantics"]["subtraction_effect_survives"] == {
        "required_for_unfilled_attributed_removal": True,
        "result_references": "all_attributed_final_boundary_ring_region_references",
        "empty_result_reference_case": ("unfilled_attributed_removal_without_final_material_boundary"),
    }
    assert catalog["path_token"]["none"] == 0
    assert sorted(catalog["path_token"].values()) == list(range(27))

    solver_limits = catalog["limit"]
    assert solver_limits["coordinate_grid_nm"] == 1
    assert solver_limits["topology_resolution_nm"] == 50
    assert solver_limits["filtered_algebraic_fallback_calls_per_job"] == 0
    assert solver_limits["vertex_squared_error_nm2_numerator"] == 2_500
    assert solver_limits["vertex_squared_error_nm2_denominator"] == 1
    assert solver_limits["radius_error_nm_numerator"] == 50
    assert solver_limits["radius_error_nm_denominator"] == 1
    assert solver_limits["arc_hausdorff_error_nm_numerator"] == 50
    assert solver_limits["arc_hausdorff_error_nm_denominator"] == 1
    assert solver_limits["algebraic_work_units_per_job"] == 1_000_000_000
    for name in (
        "examined_curve_pairs_per_job",
        "exact_intersections_per_job",
        "arrangement_half_edges_per_job",
        "algebraic_polynomial_degree",
        "algebraic_coefficient_bits",
        "algebraic_storage_bytes_per_job",
        "algebraic_work_units_per_job",
        "provenance_references_per_job",
        "source_reference_index_memberships_per_job",
        "exact_predicate_calls_per_job",
        "solver_working_memory_bytes_per_job",
    ):
        assert solver_limits[name] > 0

    packet_spec = (ROOT / candidate["packet_spec"]).read_text(encoding="utf-8")
    assert "| Fallback/oracle algebraic work units per job | 1,000,000,000 |" in packet_spec
    for magic in (catalog["request_magic"], catalog["result_magic"]):
        assert magic in packet_spec
    assert "normalized curves" not in packet_spec
    assert "content key" not in packet_spec
    assert "geometry.analytic_planar_boolean.invalid_topology" not in packet_spec
    assert "geometer.operation.analytic_planar_boolean.invalid_topology" in packet_spec


def test_analytic_planar_boolean_feasibility_signature_has_canonical_bytes() -> None:
    fixture = (ROOT / "tests" / "fixtures" / "analytic_planar_boolean" / "feasibility_signature_a0.txt").read_bytes()
    canonical = _canonical_feasibility_stdout(fixture)
    assert canonical.count(b"\n") == 4
    assert hashlib.sha256(canonical).hexdigest() == ("c21b03c1b42a6cb3212cec5b3051987f645e21062eddecc82d3e3b0e0fd6dfc7")
    assert _canonical_feasibility_stdout(canonical.replace(b"\n", b"\r\n") + b"\r\n") == canonical
