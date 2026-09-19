"""Validate the exact cross-platform release payload and write its digest inventory."""

from __future__ import annotations

import argparse
import hashlib
import json
import re
import zipfile
from pathlib import Path
from typing import Any

from validate_release_artifacts import validate_native, validate_sdk, validate_wasm, validate_wheel


PLATFORMS = ("windows-x64", "linux-x64", "linux-arm64", "macos-arm64")
WHEEL_SUFFIXES = {
    "windows-x64": "win_amd64.whl",
    "linux-x64": "manylinux_2_35_x86_64.whl",
    "linux-arm64": "manylinux_2_35_aarch64.whl",
    "macos-arm64": "macosx_11_0_arm64.whl",
}


def release_version(tag: str) -> str:
    match = re.fullmatch(r"v(\d{4})-(\d{2})-(\d{2})", tag)
    if match is None:
        raise ValueError(f"invalid date-version release tag: {tag}")
    return ".".join(str(int(part)) for part in match.groups())


def expected_asset_names(tag: str) -> set[str]:
    version = release_version(tag)
    names = {"wasm-dist.zip"}
    for target in PLATFORMS:
        sdk = f"geometer-sdk-{version}-{target}.zip"
        names.update(
            {
                f"native-{target}.zip",
                f"wn_geometer-{version}-py3-none-{WHEEL_SUFFIXES[target]}",
                sdk,
                f"{sdk}.sha256",
                f"{sdk}.provenance.json",
            }
        )
    return names


def sha256_file(path: Path) -> str:
    digest = hashlib.sha256()
    with path.open("rb") as stream:
        for chunk in iter(lambda: stream.read(1024 * 1024), b""):
            digest.update(chunk)
    return digest.hexdigest()


def collect_assets(root: Path) -> dict[str, Path]:
    assets: dict[str, Path] = {}
    for path in sorted(item for item in root.rglob("*") if item.is_file()):
        if path.name in assets:
            raise ValueError(f"duplicate release asset basename: {path.name}")
        assets[path.name] = path
    return assets


def archive_member(archive: zipfile.ZipFile, suffixes: tuple[str, ...]) -> bytes:
    matches = [name for name in archive.namelist() if name.endswith(suffixes)]
    if len(matches) != 1:
        raise ValueError(f"expected exactly one archive member ending in {suffixes}, found {len(matches)}")
    return archive.read(matches[0])


def validate_native_wheel_pair(native_path: Path, wheel_path: Path) -> None:
    with zipfile.ZipFile(native_path) as native, zipfile.ZipFile(wheel_path) as wheel:
        if archive_member(native, ("geometer", "geometer.exe")) != archive_member(
            wheel, ("/geometer", "/geometer.exe")
        ):
            raise ValueError(f"wheel runtime does not match its native archive: {wheel_path.name}")
        if archive_member(native, ("geometer.build-attestation.json",)) != archive_member(
            wheel, ("/geometer.build-attestation.json",)
        ):
            raise ValueError(f"wheel attestation does not match its native archive: {wheel_path.name}")


def validate_inventory(root: Path, tag: str) -> dict[str, Any]:
    expected = expected_asset_names(tag)
    assets = collect_assets(root)
    missing = expected - set(assets)
    extra = set(assets) - expected
    if missing or extra:
        messages = []
        if missing:
            messages.append("missing: " + ", ".join(sorted(missing)))
        if extra:
            messages.append("unexpected: " + ", ".join(sorted(extra)))
        raise ValueError("release asset inventory mismatch; " + "; ".join(messages))

    for platform_name in PLATFORMS:
        native_path = assets[f"native-{platform_name}.zip"]
        wheel_path = assets[f"wn_geometer-{release_version(tag)}-py3-none-{WHEEL_SUFFIXES[platform_name]}"]
        validate_native(native_path)
        validate_wheel(wheel_path)
        validate_native_wheel_pair(native_path, wheel_path)
        validate_sdk(assets[f"geometer-sdk-{release_version(tag)}-{platform_name}.zip"])
    validate_wasm(assets["wasm-dist.zip"])

    return {
        "schema": "wn.geometer.release_inventory.a0",
        "release_tag": tag,
        "release_version": release_version(tag),
        "assets": [
            {"name": name, "sha256": sha256_file(path), "size": path.stat().st_size}
            for name, path in sorted(assets.items())
        ],
    }


def main() -> None:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("root", type=Path)
    parser.add_argument("--tag", required=True)
    parser.add_argument("--output", type=Path, required=True)
    args = parser.parse_args()
    inventory = validate_inventory(args.root.resolve(), args.tag)
    output = args.output.resolve()
    output.parent.mkdir(parents=True, exist_ok=True)
    output.write_text(json.dumps(inventory, indent=2, sort_keys=True) + "\n", encoding="utf-8", newline="\n")
    print(f"validated {len(inventory['assets'])} immutable release assets: {output}")


if __name__ == "__main__":
    main()
