from __future__ import annotations

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

    for demo in manifest["demos"]:
        assert (ROOT / demo["source"]).is_file()
        if "worker" in demo:
            assert (ROOT / demo["worker"]).is_file()
        assert demo["owning_operation"] in operation_ids


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

