from __future__ import annotations

import hashlib
import json
from pathlib import Path
import zipfile

import pytest

from ci_release_metadata import package_version, release_tag
from validate_release_inventory import (
    collect_assets,
    expected_asset_names,
    release_version,
    validate_native_wheel_pair,
)
from verify_release_inventory import verify_release


TEST_VERSION = package_version()
TEST_TAG = release_tag(TEST_VERSION)


def write_downloaded_release(root: Path, tag: str) -> tuple[Path, dict[str, object]]:
    entries = []
    for name in sorted(expected_asset_names(tag)):
        payload = name.encode()
        (root / name).write_bytes(payload)
        entries.append(
            {
                "name": name,
                "sha256": hashlib.sha256(payload).hexdigest(),
                "size": len(payload),
            }
        )
    inventory: dict[str, object] = {
        "assets": entries,
        "release_tag": tag,
        "release_version": release_version(tag),
        "schema": "wn.geometer.release_inventory.a0",
    }
    inventory_path = root / f"geometer-release-inventory-{tag}.json"
    inventory_path.write_text(
        json.dumps(inventory, indent=2, sort_keys=True) + "\n",
        encoding="utf-8",
        newline="\n",
    )
    return inventory_path, inventory


def test_release_inventory_has_every_platform_product() -> None:
    names = expected_asset_names(TEST_TAG)
    assert len(names) == 21
    assert f"wn_geometer-{TEST_VERSION}-py3-none-win_amd64.whl" in names
    assert f"wn_geometer-{TEST_VERSION}-py3-none-manylinux_2_35_x86_64.whl" in names
    assert f"wn_geometer-{TEST_VERSION}-py3-none-manylinux_2_35_aarch64.whl" in names
    assert f"wn_geometer-{TEST_VERSION}-py3-none-macosx_11_0_arm64.whl" in names
    for platform_name in ("windows-x64", "linux-x64", "linux-arm64", "macos-arm64"):
        archive = f"geometer-sdk-{TEST_VERSION}-{platform_name}.zip"
        assert {archive, f"{archive}.sha256", f"{archive}.provenance.json"}.issubset(names)
    assert not any("illustration-demo" in name for name in names)


def test_release_inventory_normalizes_date_version() -> None:
    assert release_version("v2001-02-03") == "2001.2.3"
    assert release_version("v2001-02-03-2") == "2001.2.3.2"
    with pytest.raises(ValueError, match="date-version"):
        release_version("2001.2.3")
    with pytest.raises(ValueError, match="date-version"):
        release_version("v2001-02-03-2٢")


def test_release_inventory_rejects_duplicate_basenames(tmp_path: Path) -> None:
    for directory in (tmp_path / "one", tmp_path / "two"):
        directory.mkdir()
        (directory / "same.zip").write_bytes(b"payload")
    with pytest.raises(ValueError, match="duplicate"):
        collect_assets(tmp_path)


def test_release_inventory_requires_wheel_native_byte_identity(tmp_path: Path) -> None:
    native_path = tmp_path / "native.zip"
    wheel_path = tmp_path / "wheel.whl"
    with zipfile.ZipFile(native_path, "w") as archive:
        archive.writestr("geometer.exe", b"native")
        archive.writestr("geometer.build-attestation.json", b"attestation")
    with zipfile.ZipFile(wheel_path, "w") as archive:
        archive.writestr("geometer/native/windows-x64/geometer.exe", b"native")
        archive.writestr("geometer/native/windows-x64/geometer.build-attestation.json", b"attestation")
    validate_native_wheel_pair(native_path, wheel_path)

    with zipfile.ZipFile(wheel_path, "w") as archive:
        archive.writestr("geometer/native/windows-x64/geometer.exe", b"different")
        archive.writestr("geometer/native/windows-x64/geometer.build-attestation.json", b"attestation")
    with pytest.raises(ValueError, match="runtime does not match"):
        validate_native_wheel_pair(native_path, wheel_path)


def test_downloaded_release_requires_exact_inventory_bytes(tmp_path: Path) -> None:
    tag = TEST_TAG
    _, inventory = write_downloaded_release(tmp_path, tag)
    verify_release(tmp_path, tag)

    entries = inventory["assets"]
    assert isinstance(entries, list)
    (tmp_path / entries[0]["name"]).write_bytes(b"changed")
    with pytest.raises(ValueError, match="mismatch"):
        verify_release(tmp_path, tag)


def test_downloaded_release_rejects_inconsistent_metadata(tmp_path: Path) -> None:
    tag = TEST_TAG
    inventory_path, inventory = write_downloaded_release(tmp_path, tag)
    inventory["release_version"] = "not-the-current-version"
    inventory_path.write_text(
        json.dumps(inventory, indent=2, sort_keys=True) + "\n",
        encoding="utf-8",
        newline="\n",
    )
    with pytest.raises(ValueError, match="version does not match"):
        verify_release(tmp_path, tag)

    inventory["release_version"] = release_version(tag)
    entries = inventory["assets"]
    assert isinstance(entries, list)
    entries[0]["size"] = True
    inventory_path.write_text(
        json.dumps(inventory, indent=2, sort_keys=True) + "\n",
        encoding="utf-8",
        newline="\n",
    )
    with pytest.raises(ValueError, match="invalid recorded size"):
        verify_release(tmp_path, tag)

    entries[0]["size"] = len(entries[0]["name"])
    entries[0]["sha256"] = "G" * 64
    inventory_path.write_text(
        json.dumps(inventory, indent=2, sort_keys=True) + "\n",
        encoding="utf-8",
        newline="\n",
    )
    with pytest.raises(ValueError, match="invalid recorded SHA-256"):
        verify_release(tmp_path, tag)


def test_downloaded_release_requires_canonical_asset_order(tmp_path: Path) -> None:
    tag = TEST_TAG
    inventory_path, inventory = write_downloaded_release(tmp_path, tag)
    entries = inventory["assets"]
    assert isinstance(entries, list)
    entries.reverse()
    inventory_path.write_text(
        json.dumps(inventory, indent=2, sort_keys=True) + "\n",
        encoding="utf-8",
        newline="\n",
    )
    with pytest.raises(ValueError, match="canonical name order"):
        verify_release(tmp_path, tag)
