from __future__ import annotations

from pathlib import Path
import zipfile

import pytest

from validate_release_inventory import (
    collect_assets,
    expected_asset_names,
    release_version,
    validate_native_wheel_pair,
)


def test_release_inventory_has_every_platform_product() -> None:
    names = expected_asset_names("v2026-09-13")
    assert len(names) == 21
    assert "wn_geometer-2026.9.13-py3-none-win_amd64.whl" in names
    assert "wn_geometer-2026.9.13-py3-none-manylinux_2_35_x86_64.whl" in names
    assert "wn_geometer-2026.9.13-py3-none-manylinux_2_35_aarch64.whl" in names
    assert "wn_geometer-2026.9.13-py3-none-macosx_11_0_arm64.whl" in names
    for platform_name in ("windows-x64", "linux-x64", "linux-arm64", "macos-arm64"):
        archive = f"geometer-sdk-2026.9.13-{platform_name}.zip"
        assert {archive, f"{archive}.sha256", f"{archive}.provenance.json"}.issubset(names)


def test_release_inventory_normalizes_date_version() -> None:
    assert release_version("v2026-09-07") == "2026.9.7"
    with pytest.raises(ValueError, match="date-version"):
        release_version("2026.9.7")


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
