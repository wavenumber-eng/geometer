from __future__ import annotations

import hashlib
import re
import tomllib
from pathlib import Path
from typing import Any


ROOT = Path(__file__).resolve().parents[2]
MANIFEST_PATH = ROOT / "docs" / "contracts" / "promotion-manifest.toml"


def _manifest() -> dict[str, Any]:
    with MANIFEST_PATH.open("rb") as stream:
        return tomllib.load(stream)


def _unique(values: list[str], label: str) -> None:
    assert len(values) == len(set(values)), f"duplicate {label}: {values}"


def _sha256(path: Path) -> str:
    return hashlib.sha256(path.read_bytes()).hexdigest()


def _canonical_feasibility_stdout(raw: bytes) -> bytes:
    text = raw.decode("utf-8", errors="strict").replace("\r\n", "\n")
    assert "\r" not in text
    assert text.endswith("\n")
    assert not text.endswith("\n\n")
    return text[:-1].encode("utf-8")


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
        assert design_review["review_head"] == (
            "b86a065c5926c35f1eee23a9ba1cef890689c7d7"
        )
        assert design_review["reviewed_adr_sha256"] == (
            "d4905bda88727fadeb221a3b6c5bfb392f5e062bb6033b0cded29f59fb492de0"
        )
        assert "## Status\n\nAccepted." in adr
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
            assert asset["license_evidence"] == (
                "docs/design/assets/fonts/Cousine/OFL.txt"
            )
            assert asset["status"] == license_asset["status"]
            if asset["status"] == "vendored":
                assert font_redistribution["status"] == "approved_open_license"
                assert (ROOT / asset["license_evidence"]).is_file()
        elif asset["role"] == "license":
            assert font_redistribution["source_revision"] in asset["source"]
            assert asset["source"].endswith("/OFL.txt")

    contracts = manifest["contracts"]
    contract_ids = [item["id"] for item in contracts]
    _unique(contract_ids, "contract id")
    assert "geometry.model_bounds.options.a0" in contract_ids
    assert "geometry.model_bounds.a0" in contract_ids
    assert all((ROOT / item["source"]).is_file() for item in contracts if item["source"] != "none")

    operations = manifest["operations"]
    operation_ids = [item["id"] for item in operations]
    _unique(operation_ids, "operation id")
    assert {item["id"] for item in operations if item["status"] == "pilot_candidate"} == {
        "geometry.model_bounds.a0"
    }

    candidates = manifest["candidate_operations"]
    candidate_ids = [item["id"] for item in candidates]
    _unique(candidate_ids, "candidate operation id")
    assert not set(candidate_ids) & set(operation_ids)
    assert candidate_ids == ["geometry.analytic_planar_boolean_batch.a0"]
    candidate = candidates[0]
    assert candidate["status"] == "design_required"
    assert candidate["request_contract"] == "unfrozen"
    assert candidate["result_contract"] == "unfrozen"
    assert candidate["transport"] == "generic_named_attachments"
    assert candidate["operation_specific_c_abi_symbol"] is False
    assert candidate["replaces_existing_operation"] is False
    assert candidate["browser_target"] == "full_browser"
    assert (ROOT / candidate["compatibility_snapshot"]).is_file()
    assert (ROOT / candidate["design"]).is_file()
    assert (ROOT / candidate["packet_spec"]).is_file()
    assert (ROOT / candidate["numeric_catalog"]).is_file()
    assert (ROOT / candidate["feasibility_test"]).is_file()

    for demo in manifest["demos"]:
        assert (ROOT / demo["source"]).is_file()
        if "worker" in demo:
            assert (ROOT / demo["worker"]).is_file()
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
        "dist/wasm/planar-browser/geometer-planar-browser.js": (
            "geometer-planar-browser.js"
        ),
        "dist/wasm/planar-browser/geometer-planar-browser.wasm": (
            "geometer-planar-browser.wasm"
        ),
    }
    assert dict(
        zip(
            snapshot["artifacts"]["source_paths"],
            snapshot["artifacts"]["vendored_names"],
            strict=True,
        )
    ) == expected_artifact_pairs

    mappings = snapshot["artifact_mappings"]
    assert {item["source"]: item["vendored"] for item in mappings} == (
        expected_artifact_pairs
    )
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
    assert [item["factory"] for item in javascript_mappings] == snapshot["artifacts"][
        "factory_names"
    ]
    required_memory_views = snapshot["wasm"]["required_memory_views"]
    assert required_memory_views == ["HEAPU8", "HEAPU32"]
    for item in javascript_mappings:
        loader = (ROOT / item["source"]).read_text(encoding="utf-8")
        assert item["factory"] in loader
        for view in required_memory_views:
            assert view in loader

    runtime_lists = re.findall(r'-sEXPORTED_RUNTIME_METHODS=\[(.*?)\]"', cmake)
    assert len(runtime_lists) == 2
    runtime_methods = [
        {item.strip("'") for item in value.split(",")} for value in runtime_lists
    ]
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
    matz = next(
        item
        for item in manifest["consumers"]
        if item["id"] == "appz.data_models.pcb.matz"
    )
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
    assert snapshot["geom_contract"]["schema_identity"] == (
        "urn:wavenumber:schema:geom_a0"
    )

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
        "contracts/geom/geom_a0.schema.json": (
            "acd688d81c5445445c6ee3f324000009e3976b4bbaa19895d6f2d52fc0d0ef3b"
        ),
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
        "portable_case_count": 10,
        "real_board_case_count": 2,
        "case_2_oracle": "success",
        "case_2_oracle_evidence": "native_wasm_occt_feasibility_signature",
        "case_2_geometer_design_revision": (
            "182f5f2163e4085200adec779f98b6d3cc7c0e13"
        ),
        "case_2_oracle_signature_sha256": (
            "c21b03c1b42a6cb3212cec5b3051987f645e21062eddecc82d3e3b0e0fd6dfc7"
        ),
        "case_2_canonical_fragment_count": 12,
        "normalization_policy": "certified_once_after_final_stage",
        "normalization_collapse_case": "job_local_failure",
        "normal_build_sibling_dependency": False,
    }

    plan = (
        ROOT / "docs" / "plans" / "geometer-typespec-contracts" / "plan.md"
    ).read_text(encoding="utf-8")
    assert operation["id"] in plan
    assert matz["snapshot"] in plan or snapshot_path.name in plan


def test_analytic_planar_boolean_numeric_catalog_is_closed() -> None:
    manifest = _manifest()
    candidate = manifest["candidate_operations"][0]
    assert (ROOT / candidate["solver_adr"]).is_file()
    with (ROOT / candidate["numeric_catalog"]).open("rb") as stream:
        catalog = tomllib.load(stream)

    assert catalog["catalog_version"] == 1
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
    assert catalog["contract_diagnostic"]["invalid_id"].startswith(
        "geometer.contract."
    )
    assert catalog["contract_diagnostic"]["invalid_reference"].startswith(
        "geometer.contract."
    )
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

    solver_limits = catalog["limit"]
    for name in (
        "examined_curve_pairs_per_job",
        "exact_intersections_per_job",
        "arrangement_half_edges_per_job",
        "algebraic_polynomial_degree",
        "algebraic_coefficient_bits",
        "algebraic_storage_bytes_per_job",
        "provenance_references_per_job",
        "source_reference_index_memberships_per_job",
        "exact_predicate_calls_per_job",
        "solver_working_memory_bytes_per_job",
    ):
        assert solver_limits[name] > 0

    packet_spec = (ROOT / candidate["packet_spec"]).read_text(encoding="utf-8")
    for magic in (catalog["request_magic"], catalog["result_magic"]):
        assert magic in packet_spec
    assert "normalized curves" not in packet_spec
    assert "content key" not in packet_spec
    assert "geometry.analytic_planar_boolean.invalid_topology" not in packet_spec
    assert "geometer.operation.analytic_planar_boolean.invalid_topology" in packet_spec


def test_analytic_planar_boolean_feasibility_signature_has_canonical_bytes() -> None:
    fixture = (
        ROOT
        / "tests"
        / "fixtures"
        / "analytic_planar_boolean"
        / "feasibility_signature_a0.txt"
    ).read_bytes()
    canonical = _canonical_feasibility_stdout(fixture)
    assert canonical.count(b"\n") == 4
    assert hashlib.sha256(canonical).hexdigest() == (
        "c21b03c1b42a6cb3212cec5b3051987f645e21062eddecc82d3e3b0e0fd6dfc7"
    )
    assert _canonical_feasibility_stdout(
        canonical.replace(b"\n", b"\r\n") + b"\r\n"
    ) == canonical
