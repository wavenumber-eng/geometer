from __future__ import annotations

import hashlib
import json
import re
import tomllib
from pathlib import Path
from typing import Any


ROOT = Path(__file__).resolve().parents[2]
MANIFEST_PATH = ROOT / "docs" / "contracts" / "promotion-manifest.toml"


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
    assert all(
        item["current_authority"] == "typespec_normalized_catalog"
        for item in contracts
        if item["status"] == "promoted"
    )
    assert all((ROOT / item["source"]).is_file() for item in contracts if item["source"] != "none")

    operations = manifest["operations"]
    operation_ids = [item["id"] for item in operations]
    _unique(operation_ids, "operation id")
    assert {item["id"] for item in operations if item["status"] == "pilot_candidate"} == set()
    assert {item["id"] for item in operations if item["status"] == "promoted"} == {
        "geometry.model_bounds.a0"
    }

    candidates = manifest["candidate_operations"]
    candidate_ids = [item["id"] for item in candidates]
    _unique(candidate_ids, "candidate operation id")
    assert not set(candidate_ids) & set(operation_ids)
    assert candidate_ids == ["geometry.analytic_planar_boolean_batch.a0"]
    candidate = candidates[0]
    assert candidate["status"] == "design_frozen"
    assert candidate["request_contract"] == "typespec_candidate_frozen_a0"
    assert candidate["result_contract"] == "typespec_candidate_frozen_a0"
    assert candidate["packed_format"] == "separately_governed_frozen_a0"
    assert candidate["implementation_gate"] == "exact_backend_feasibility_and_occt_qualification_pending"
    assert candidate["transport"] == "generic_named_attachments"
    assert candidate["operation_specific_c_abi_symbol"] is False
    assert candidate["replaces_existing_operation"] is False
    assert candidate["browser_target"] == "full_browser"
    assert (ROOT / candidate["compatibility_snapshot"]).is_file()
    assert (ROOT / candidate["design"]).is_file()
    assert (ROOT / candidate["packet_spec"]).is_file()
    assert (ROOT / candidate["numeric_catalog"]).is_file()
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


def test_exact_algebraic_backend_design_gate_is_closed() -> None:
    backend = _manifest()["analytic_exact_backend"]
    assert backend == {
        "status": "design_accepted_implementation_in_progress",
        "design": "docs/design/exact-real-algebraic-a0.md",
        "design_sha256": "39576b283f162c87503f18f2c2bec23a83e394ec9b042449550e8b8ab59823bb",
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
        "design_review_revision": "a8c9604de280e2a67018e1106fd1b430b34fcf50",
        "design_review_packet": "reviewer-019ffd1f-3c67-7001-87a5-200b6cda10d8",
        "implemented_surface": "budgeted_rational_polynomial_root_thom_resultant_factor_and_root_selection_feasibility",
        "rational_source": "src/cpp/lib/exact_rational.cpp",
        "polynomial_source": "src/cpp/lib/exact_polynomial.cpp",
        "resultant_source": "src/cpp/lib/exact_resultant.cpp",
        "factorization_source": "src/cpp/lib/exact_factorization.cpp",
        "rational_test": "tests/cpp/exact_rational_test.cpp",
        "polynomial_test": "tests/cpp/exact_polynomial_test.cpp",
        "resultant_test": "tests/cpp/exact_resultant_test.cpp",
        "factorization_test": "tests/cpp/exact_factorization_test.cpp",
    }
    assert (ROOT / backend["design"]).is_file()
    assert _sha256(ROOT / backend["design"]) == backend["design_sha256"]
    for key in ("rational_source", "polynomial_source", "rational_test", "polynomial_test"):
        assert (ROOT / backend[key]).is_file()


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
    assert geom_contract["mapping_review_packet"] == (
        "reviewer-019ffce8-ac66-76c0-877d-3fcb5c1aa6c5"
    )
    assert geom_contract["consumer_confirmation"] == (
        "joint_semantic_and_fixture_review_complete_no_known_blocker"
    )
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
    assert all(
        query[side] in known_job_ids
        for query in all_queries
        for side in ("left_job_id", "right_job_id")
    )

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
    assert catalog["status"] == "frozen_a0"
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
    assert catalog["path_token"]["none"] == 0
    assert sorted(catalog["path_token"].values()) == list(range(27))

    solver_limits = catalog["limit"]
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
    assert "| Algebraic work units per job | 1,000,000,000 |" in packet_spec
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
