from __future__ import annotations

import hashlib
import json
import re
import subprocess
import tomllib
from pathlib import Path
from typing import Any

from contract_promotion_topology_inventory import assert_step_topology_inventory


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
        "design": "docs/contracts/typespec-toolchain.md",
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
    amendment = transports["step_media_amendment"]
    assert amendment["status"] == "accepted"
    assert amendment["review_date"] == "2026-10-14"
    assert amendment["adr"] == "docs/geometer/adr/geometer-adr-020-standard-step-media-types.md"
    assert 'status = "accepted"' in (ROOT / amendment["adr"]).read_text(encoding="utf-8")
    adr = (ROOT / transports["transport_adr"]).read_text(encoding="utf-8")
    assert "## Status\n\nAccepted" in adr
    _assert_documentation_manifest(manifest)


def _assert_projection_surfaces(manifest: dict[str, Any]) -> None:
    typescript = manifest["typescript_projection"]
    assert typescript["status"] == "implemented_model_bounds_and_hlr_worker_pilots"
    assert typescript["worker_protocol"] == "wn.geometer.wasm_worker.a0"
    assert typescript["model_hlr_live_operation"] == "geometry.model_hlr_projection.a0"
    assert typescript["mesh_hlr_live_operation"] == "geometry.mesh_hlr_projection.a0"
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
    ):
        assert (ROOT / typescript[key]).exists(), key
    package_json = json.loads((ROOT / typescript["package_root"] / "package.json").read_text(encoding="utf-8"))
    assert package_json["name"] == manifest["packages"]["typescript"]
    assert manifest["packages"]["typescript_module_format"] == "esm"
    assert package_json["type"] == "module"
    rust_projection = manifest["rust_projection"]
    assert rust_projection["status"] == "implemented_model_bounds_and_hlr_ipc_pilots"
    assert rust_projection["live_operation"] == "geometry.model_bounds.a0"
    assert rust_projection["model_hlr_live_operation"] == "geometry.model_hlr_projection.a0"
    assert rust_projection["mesh_hlr_live_operation"] == "geometry.mesh_hlr_projection.a0"
    for key in ("design", "source_root", "generated_root", "generator"):
        assert (ROOT / rust_projection[key]).exists(), key
    assert rust_projection["runtime_dependency"] is False

    python_projection = manifest["python_projection"]
    assert python_projection["status"] == ("implemented_model_bounds_compatible_boundary_and_hlr_ipc_pilots")
    assert python_projection["live_operation"] == "geometry.model_bounds.a0"
    assert python_projection["model_hlr_live_operation"] == "geometry.model_hlr_projection.a0"
    assert python_projection["mesh_hlr_live_operation"] == "geometry.mesh_hlr_projection.a0"
    for key in (
        "design",
        "generated_root",
        "runtime",
        "generator",
        "ipc_client",
    ):
        assert (ROOT / python_projection[key]).exists(), key
    assert python_projection["runtime_dependency"] is False


def _assert_model_bounds_promotion(manifest: dict[str, Any]) -> None:
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
    assert evidence["catalog_sha256"] == "c93e41a3aa0d64ab4dab905cea82aaeb3b3792155d1f5b2850673567a699d59c"
    assert evidence["vector_manifest_sha256"] == ("30f697bf63e6ca1ba28e0b9617a061b59a90115ea4ef06a06761d09ea68301a5")


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
    assert all(
        item["current_authority"] == "typespec_normalized_catalog"
        for item in contracts
        if item["status"] in {"promoted", "candidate_frozen"}
    )
    assert all((ROOT / item["source"]).is_file() for item in contracts if item["source"] != "none")
    operations = manifest["operations"]
    operation_ids = [item["id"] for item in operations]
    _unique(operation_ids, "operation id")
    assert {item["id"] for item in operations if item["status"] == "pilot_candidate"} == {
        "geometry.mesh_illustration.a0",
        "geometry.mesh_illustration_geometry.a0",
        "geometry.model_hlr_projection.a0",
        "geometry.mesh_hlr_projection.a0",
        "geometry.model_tessellation.a0",
        "geometry.model_illustration.a0",
        "geometry.model_illustration_geometry.a0",
        "geometry.mesh_hlr_projection.b0",
        "geometry.mesh_illustration.b0",
        "geometry.mesh_illustration_geometry.b0",
        "geometry.model_illustration.b0",
        "geometry.model_illustration_geometry.b0",
    }
    assert {item["id"] for item in operations if item["status"] == "promoted"} == {"geometry.model_bounds.a0"}
    assert_step_topology_inventory(manifest, contracts, operations)
def _assert_demo_inventory(manifest: dict[str, Any]) -> None:
    operation_ids = [item["id"] for item in manifest["operations"]]
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
        assert demo["owning_operation"] in operation_ids


def _assert_consumer_inventory(manifest: dict[str, Any]) -> None:
    consumers = manifest["consumers"]
    consumer_ids = [item["id"] for item in consumers]
    _unique(consumer_ids, "consumer id")
    assert consumer_ids == ["appz.viz"]
    assert all((ROOT / item["snapshot"]).is_file() for item in consumers)


def test_manifest_promoted_and_candidate_surfaces_are_complete() -> None:
    manifest = _manifest()
    _assert_projection_surfaces(manifest)
    _assert_model_bounds_promotion(manifest)
    _assert_contract_and_operation_inventory(manifest)
    _assert_demo_inventory(manifest)
    _assert_consumer_inventory(manifest)


def test_c_abi_manifest_matches_header_exactly() -> None:
    manifest = _manifest()
    c_abi = manifest["c_abi"]
    header = (ROOT / c_abi["source"]).read_text(encoding="utf-8")
    declared = re.findall(r"GEOMETER_C_API\s+[\w\s*]+?\b(geometer_\w+)\s*\(", header)
    _unique(declared, "C ABI declaration")
    assert declared == c_abi["symbols"]
