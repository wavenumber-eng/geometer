"""Fail-closed validation for public Geometer release artifacts."""

from __future__ import annotations

import argparse
import glob
import re
import zipfile
from pathlib import Path


LICENSE_NAMES = {
    "WN_GEOMETER_LICENSE.txt",
    "THIRD_PARTY_NOTICES.md",
    "CLIPPER2_LICENSE.txt",
    "RAPIDJSON_LICENSE.txt",
    "OCCT_LICENSE_LGPL_21.txt",
    "OCCT_LGPL_EXCEPTION.txt",
}


def require_archive_licenses(names: set[str], prefix: str = "licenses/") -> None:
    missing = {prefix + name for name in LICENSE_NAMES} - names
    if missing:
        raise ValueError("release artifact is missing licenses: " + ", ".join(sorted(missing)))


def validate_native(path: Path) -> None:
    with zipfile.ZipFile(path) as archive:
        names = set(archive.namelist())
    require_archive_licenses(names)
    if not ({"geometer", "geometer.exe"} & names):
        raise ValueError("native archive is missing the Geometer executable")
    if "geometer.build-attestation.json" not in names:
        raise ValueError("native archive is missing its executable build attestation")
    native_libraries = {name for name in names if Path(name).suffix.lower() in {".a", ".lib"}}
    if native_libraries:
        raise ValueError(
            "native runtime archive contains build-only static libraries: "
            + ", ".join(sorted(native_libraries))
        )


def validate_wasm(path: Path) -> None:
    with zipfile.ZipFile(path) as archive:
        names = set(archive.namelist())
        package = archive.read("node-test/package.json")
    require_archive_licenses(names)
    required = {
        "browser/geometer.js",
        "browser/geometer.wasm",
        "node-test/geometer-node-test.js",
        "node-test/geometer-node-test.wasm",
        "node-test/package.json",
        "planar-browser/geometer-planar-browser.js",
        "planar-browser/geometer-planar-browser.wasm",
        "npm/geometer/package.json",
    }
    missing = required - names
    if missing:
        raise ValueError("WASM archive is missing runtime files: " + ", ".join(sorted(missing)))
    if any(name.startswith("demos/") for name in names):
        raise ValueError("WASM runtime archive must not contain deferred demo artifacts")
    if package != b'{"private":true,"type":"commonjs"}\n':
        raise ValueError("Node CLI package boundary is not canonical CommonJS JSON")


def validate_wheel(path: Path) -> None:
    with zipfile.ZipFile(path) as archive:
        names = set(archive.namelist())
    license_prefixes = {name.rsplit("/", 1)[0] + "/" for name in names if name.endswith("THIRD_PARTY_NOTICES.md")}
    if len(license_prefixes) != 1:
        raise ValueError("wheel must contain exactly one third-party notice bundle")
    require_archive_licenses(names, next(iter(license_prefixes)))
    if not any(name.endswith(("/geometer", "/geometer.exe")) and "/native/" in name for name in names):
        raise ValueError("wheel is missing the bundled native executable")
    if not any(name.endswith("/geometer.build-attestation.json") for name in names):
        raise ValueError("wheel is missing the native build attestation")
    if "linux" in path.name and re.search(r"-manylinux_2_35_(x86_64|aarch64)\.whl$", path.name) is None:
        raise ValueError("Linux wheel does not carry the governed manylinux_2_35 platform tag")


def main() -> None:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("kind", choices=("native", "wasm", "wheel"))
    parser.add_argument("artifacts", nargs="+")
    args = parser.parse_args()
    validator = {"native": validate_native, "wasm": validate_wasm, "wheel": validate_wheel}[args.kind]
    expanded = [Path(match) for pattern in args.artifacts for match in (glob.glob(pattern) or [pattern])]
    for artifact in expanded:
        validator(artifact.resolve())
        print(f"validated {args.kind} release artifact: {artifact}")


if __name__ == "__main__":
    main()
