from __future__ import annotations

import re
import sys
import tomllib
from pathlib import Path

import test_contract_promotion_manifest as support

sys.path.insert(0, str(Path(__file__).resolve().parents[2] / "scripts"))
from standalone_html import _esbuild_command  # noqa: E402
from release_licenses import release_license_sources  # noqa: E402


ROOT = support.ROOT
_manifest = support._manifest
_unique = support._unique
_sha256 = support._sha256


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


def test_release_licenses_find_cache_restored_unix_documentation(tmp_path: Path) -> None:
    install = (
        tmp_path
        / ".deps"
        / "native"
        / "macos-arm64"
        / "occt-install"
        / "share"
        / "doc"
        / "opencascade"
    )
    install.mkdir(parents=True)
    for name in ("LICENSE_LGPL_21.txt", "OCCT_LGPL_EXCEPTION.txt"):
        (install / name).write_text(name, encoding="utf-8")
    sources = release_license_sources(tmp_path, "macos-arm64")
    assert sources["OCCT_LICENSE_LGPL_21.txt"] == install / "LICENSE_LGPL_21.txt"
    assert sources["OCCT_LGPL_EXCEPTION.txt"] == install / "OCCT_LGPL_EXCEPTION.txt"


def test_release_licenses_find_isolated_static_crt_install(tmp_path: Path) -> None:
    install = tmp_path / ".deps" / "native" / "windows-x64" / "occt-static-crt-install"
    install.mkdir(parents=True)
    for name in ("LICENSE_LGPL_21.txt", "OCCT_LGPL_EXCEPTION.txt"):
        (install / name).write_text(name, encoding="utf-8")
    sources = release_license_sources(tmp_path, "windows-x64")
    assert sources["OCCT_LICENSE_LGPL_21.txt"] == install / "LICENSE_LGPL_21.txt"
    assert sources["OCCT_LGPL_EXCEPTION.txt"] == install / "OCCT_LGPL_EXCEPTION.txt"


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
        assert "{" + request_chars + "}" in source
        if response_magic := packet.get("response_magic"):
            response_chars = ", ".join(f"'{value}'" for value in response_magic)
            assert "{" + response_chars + "}" in source
        version = packet["version"]
        assert f"FORMAT_VERSION = {version};" in source or f"kVersion = {version};" in source


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
