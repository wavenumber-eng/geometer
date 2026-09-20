"""Canonical public release asset names and date-version parsing."""

from __future__ import annotations

import re


PLATFORMS = ("windows-x64", "linux-x64", "linux-arm64", "macos-arm64")
WHEEL_SUFFIXES = {
    "windows-x64": "win_amd64.whl",
    "linux-x64": "manylinux_2_35_x86_64.whl",
    "linux-arm64": "manylinux_2_35_aarch64.whl",
    "macos-arm64": "macosx_11_0_arm64.whl",
}


def release_version(tag: str) -> str:
    match = re.fullmatch(r"v([0-9]{4})-([0-9]{2})-([0-9]{2})(?:-(0|[1-9][0-9]*))?", tag)
    if match is None:
        raise ValueError(f"invalid date-version release tag: {tag}")
    return ".".join(str(int(part)) for part in match.groups() if part is not None)


def product_asset_names(tag: str) -> set[str]:
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


def validation_asset_name(tag: str) -> str:
    release_version(tag)
    return f"geometer-release-validation-{tag}.json"


def expected_asset_names(tag: str) -> set[str]:
    return product_asset_names(tag) | {validation_asset_name(tag)}
