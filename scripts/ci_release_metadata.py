#!/usr/bin/env python3
"""Validate release metadata from GitHub Actions."""

from __future__ import annotations

import argparse
import json
import re
import tomllib
from pathlib import Path


ROOT = Path(__file__).resolve().parents[1]


def package_version() -> str:
    with (ROOT / "pyproject.toml").open("rb") as handle:
        return tomllib.load(handle)["project"]["version"]


def release_date(version: str) -> str:
    year, month, day, *_ = (int(part) for part in version.split("."))
    return f"{year:04d}-{month:02d}-{day:02d}"


def release_tag(version: str) -> str:
    parts = version.split(".")
    tag = f"v{release_date(version)}"
    if len(parts) == 4:
        tag = f"{tag}-{int(parts[3])}"
    return tag


def client_package_version(version: str) -> str:
    """Return the SemVer-compatible client version for a runtime release."""
    return ".".join(version.split(".")[:3])


def check_tag(tag: str) -> None:
    version = package_version()
    expected = release_tag(version)
    if tag != expected:
        raise SystemExit(f"expected release tag {expected}, got {tag}")


def check_notes() -> None:
    version = package_version()
    date = release_date(version)
    changelog = (ROOT / "CHANGELOG.md").read_text(encoding="utf-8")
    expected_heading = f"## [{version}] - {date}"
    if expected_heading not in changelog:
        raise SystemExit(f"missing changelog heading: {expected_heading}")

    release_doc = ROOT / "docs" / "releases" / f"{date}.md"
    if not release_doc.exists():
        raise SystemExit(f"missing release doc: {release_doc.relative_to(ROOT)}")
    if f"`{version}`" not in release_doc.read_text(encoding="utf-8"):
        raise SystemExit(f"release doc does not mention `{version}`")


def check_surfaces() -> None:
    version = package_version()
    date = release_date(version)
    abi = date.replace("-", "")
    cmake = (ROOT / "CMakeLists.txt").read_text(encoding="utf-8")
    expected_cmake = {
        "GEOMETER_RELEASE_DATE": date,
        "GEOMETER_RELEASE_VERSION": version,
        "GEOMETER_ABI_VERSION": abi,
    }
    for name, expected in expected_cmake.items():
        match = re.search(rf'^set\({name} "([^"]+)"', cmake, re.MULTILINE)
        actual = None if match is None else match.group(1)
        if actual != expected:
            raise SystemExit(f"{name} mismatch: expected {expected}, got {actual}")

    library_cmake = (ROOT / "src" / "cpp" / "lib" / "CMakeLists.txt").read_text(
        encoding="utf-8"
    )
    for name in expected_cmake:
        if re.search(rf"set\(\s*{name}\b", library_cmake):
            raise SystemExit(
                f"{name} must be defined only by the top-level Geometer configuration"
            )

    with (ROOT / "scripts" / "pyproject.toml").open("rb") as handle:
        script_version = tomllib.load(handle)["project"]["version"]
    ts_version = json.loads((ROOT / "src" / "ts" / "geometer" / "package.json").read_text(encoding="utf-8"))[
        "version"
    ]
    with (ROOT / "src" / "rust" / "geometer-client" / "Cargo.toml").open("rb") as handle:
        rust_version = tomllib.load(handle)["package"]["version"]
    with (ROOT / "src" / "rust" / "geometer-client" / "Cargo.lock").open("rb") as handle:
        lock = tomllib.load(handle)
    rust_lock_version = next(
        package["version"] for package in lock["package"] if package["name"] == "geometer-client"
    )
    with (ROOT / "docs" / "contracts" / "promotion-manifest.toml").open("rb") as handle:
        manifest_abi = str(tomllib.load(handle)["c_abi"]["generation"])
    observed = {
        "scripts/pyproject.toml": script_version,
    }
    expected_client_version = client_package_version(version)
    client_observed = {
        "src/ts/geometer/package.json": ts_version,
        "src/rust/geometer-client/Cargo.toml": rust_version,
        "src/rust/geometer-client/Cargo.lock": rust_lock_version,
    }
    mismatches = [f"{path}={actual}" for path, actual in observed.items() if actual != version]
    mismatches.extend(
        f"{path}={actual}"
        for path, actual in client_observed.items()
        if actual != expected_client_version
    )
    if manifest_abi != abi:
        mismatches.append(f"docs/contracts/promotion-manifest.toml c_abi={manifest_abi}")
    if mismatches:
        raise SystemExit("release version surfaces disagree: " + ", ".join(mismatches))


def main() -> None:
    parser = argparse.ArgumentParser(description=__doc__)
    subparsers = parser.add_subparsers(dest="command", required=True)
    subparsers.add_parser("version")
    subparsers.add_parser("date")
    tag_parser = subparsers.add_parser("check-tag")
    tag_parser.add_argument("tag")
    subparsers.add_parser("check-notes")
    subparsers.add_parser("check-surfaces")
    args = parser.parse_args()

    version = package_version()
    if args.command == "version":
        print(version)
    elif args.command == "date":
        print(release_date(version))
    elif args.command == "check-tag":
        check_tag(args.tag)
    elif args.command == "check-notes":
        check_notes()
    elif args.command == "check-surfaces":
        check_surfaces()
    else:
        raise AssertionError(args.command)


if __name__ == "__main__":
    main()
