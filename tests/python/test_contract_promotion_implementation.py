from __future__ import annotations

import hashlib
import json
import re
import sys
import tomllib
from pathlib import Path
from typing import Any

import test_contract_promotion_manifest as support

sys.path.insert(0, str(Path(__file__).resolve().parents[2] / "scripts"))
from standalone_html import _esbuild_command  # noqa: E402
from release_licenses import release_license_sources  # noqa: E402


ROOT = support.ROOT
_manifest = support._manifest
_unique = support._unique
_sha256 = support._sha256
_canonical_feasibility_stdout = support._canonical_feasibility_stdout
_assert_nonzero_uint64 = support._assert_nonzero_uint64
_collect_explicit_source_ids = support._collect_explicit_source_ids


def test_esbuild_launcher_matches_installed_platform_artifact() -> None:
    executable = ROOT / "node_modules" / "esbuild" / "bin" / "esbuild"
    assert _esbuild_command("node", executable, platform_name="nt") == (
        "node",
        str(executable),
    )
    assert _esbuild_command("node", executable, platform_name="posix") == (str(executable),)


def test_release_licenses_fall_back_to_the_cache_restored_occt_install(tmp_path: Path) -> None:
    install = tmp_path / ".deps" / "native" / "windows-x64" / "occt-install"
    install.mkdir(parents=True)
    for name in ("LICENSE_LGPL_21.txt", "OCCT_LGPL_EXCEPTION.txt"):
        (install / name).write_text(name, encoding="utf-8")
    sources = release_license_sources(tmp_path, "windows-x64")
    assert sources["OCCT_LICENSE_LGPL_21.txt"] == install / "LICENSE_LGPL_21.txt"
    assert sources["OCCT_LGPL_EXCEPTION.txt"] == install / "OCCT_LGPL_EXCEPTION.txt"


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
        "source_path": (
            "data_models/tests/fixtures/pcb_materialization/geometer_analytic_planar_boolean_observations_a0.json"
        ),
        "git_blob_sha1": "45c5a648fc0b167c8a887006cb14d3e62229e360",
        "content_sha256": "10a97f0eed4a4f6852917c4fb6abd35854142bf5d148b8320c60a44f765414c4",
        "vendored_path": "tests/fixtures/analytic_planar_boolean/matz_observations_a0.json",
        "vendored_content_sha256": "10a97f0eed4a4f6852917c4fb6abd35854142bf5d148b8320c60a44f765414c4",
        "portable_case_count": 10,
        "real_board_case_count": 2,
        "case_2_oracle": "historical_angle_form_feasibility_success",
        "case_2_oracle_evidence": "historical_nonproduction_native_wasm_occt_feasibility_signature",
        "case_2_geometer_design_revision": ("182f5f2163e4085200adec779f98b6d3cc7c0e13"),
        "case_2_oracle_signature_sha256": ("c21b03c1b42a6cb3212cec5b3051987f645e21062eddecc82d3e3b0e0fd6dfc7"),
        "case_2_canonical_fragment_count": 12,
        "case_2_production_status": "blocked_missing_authoritative_integer_endpoints",
        "case_2_handoff": "docs/contracts/matz-case-2-handoff-a0.json",
        "normalization_policy": "certified_once_after_final_stage",
        "normalization_collapse_case": "job_local_failure",
        "normal_build_sibling_dependency": False,
    }
    vendored_fixture = ROOT / fixture_input["vendored_path"]
    assert candidate["portable_fixture"] == fixture_input["vendored_path"]
    assert fixture_input["case_2_handoff"] == candidate["matz_case_2_handoff"]
    assert fixture_input["case_2_production_status"] == "blocked_missing_authoritative_integer_endpoints"
    assert candidate["python_production_replay_blocked_cases"] == ["2_missing_authoritative_integer_endpoints"]
    assert _sha256(vendored_fixture) == fixture_input["vendored_content_sha256"] == fixture_input["content_sha256"]

    review_record = ROOT / candidate["independent_design_review_record"]
    review_text = review_record.read_text(encoding="utf-8")
    assert operation["id"] in review_text
    assert _sha256(review_record) == candidate["independent_design_review_record_sha256"]


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


def _assert_numeric_packet_spec(candidate: dict[str, Any], catalog: dict[str, Any]) -> None:
    packet_spec = (ROOT / candidate["packet_spec"]).read_text(encoding="utf-8")
    assert "| Fallback/oracle algebraic work units per job | 1,000,000,000 |" in packet_spec
    assert "| Logical source-reference expansions per decoded batch | 1,048,576 |" in packet_spec
    for magic in (catalog["request_magic"], catalog["result_magic"]):
        assert magic in packet_spec
    assert "normalized curves" not in packet_spec
    assert "content key" not in packet_spec
    assert "geometry.analytic_planar_boolean.invalid_topology" not in packet_spec
    assert "geometer.operation.analytic_planar_boolean.invalid_topology" in packet_spec


def _assert_numeric_catalog_identity(candidate: dict[str, Any], catalog: dict[str, Any]) -> None:
    assert catalog["catalog_version"] == 1
    assert catalog["status"] == "pre_release_contract_evolution"
    assert catalog["operation_identity"] == candidate["id"]
    assert catalog["request_magic"] == "GMABRQ01"
    assert catalog["result_magic"] == "GMABRS01"


def test_analytic_planar_boolean_numeric_catalog_is_closed() -> None:
    manifest = _manifest()
    candidate = manifest["candidate_operations"][0]
    assert (ROOT / candidate["solver_adr"]).is_file()
    with (ROOT / candidate["numeric_catalog"]).open("rb") as stream:
        catalog = tomllib.load(stream)

    _assert_numeric_catalog_identity(candidate, catalog)

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
    authoritative_logical_expansion_limit = 1_048_576
    assert candidate["logical_source_reference_expansions_per_batch"] == authoritative_logical_expansion_limit
    assert solver_limits["logical_source_reference_expansions_per_batch"] == authoritative_logical_expansion_limit
    typespec = (ROOT / candidate["candidate_typespec_source"]).read_text(encoding="utf-8")
    assert '@extension("x-wn-max-logical-source-reference-expansions", 1048576)' in typespec
    result_schema = json.loads(
        (ROOT / "contracts/geometer/generated/schema/AnalyticPlanarBooleanBatchResultA0.json").read_text(
            encoding="utf-8"
        )
    )
    assert result_schema["x-wn-max-logical-source-reference-expansions"] == authoritative_logical_expansion_limit
    generated_catalog = json.loads(
        (ROOT / "contracts/geometer/generated/wn_geometer_contract_catalog.a0.json").read_text(encoding="utf-8")
    )
    result_declaration = next(
        declaration
        for declaration in generated_catalog["declarations"]
        if declaration["name"].endswith(".AnalyticPlanarBooleanBatchResultA0")
    )
    assert (
        result_declaration["annotations"]["x-wn-max-logical-source-reference-expansions"]
        == authoritative_logical_expansion_limit
    )
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

    _assert_numeric_packet_spec(candidate, catalog)


def test_analytic_planar_boolean_feasibility_signature_has_canonical_bytes() -> None:
    fixture = (ROOT / "tests" / "fixtures" / "analytic_planar_boolean" / "feasibility_signature_a0.txt").read_bytes()
    canonical = _canonical_feasibility_stdout(fixture)
    assert canonical.count(b"\n") == 4
    assert hashlib.sha256(canonical).hexdigest() == ("c21b03c1b42a6cb3212cec5b3051987f645e21062eddecc82d3e3b0e0fd6dfc7")
    assert _canonical_feasibility_stdout(canonical.replace(b"\n", b"\r\n") + b"\r\n") == canonical
