"""Verify downloaded release assets against their canonical digest inventory."""

from __future__ import annotations

import argparse
import hashlib
import json
from pathlib import Path
import re
from typing import Any

from candidate_root import candidate_root_sha256, validate_candidate_root
from validate_release_inventory import collect_assets, expected_asset_names, release_version


SCHEMA = "wn.geometer.release_inventory.b0"


def sha256_file(path: Path) -> str:
    digest = hashlib.sha256()
    with path.open("rb") as stream:
        for chunk in iter(lambda: stream.read(1024 * 1024), b""):
            digest.update(chunk)
    return digest.hexdigest()


def load_inventory(path: Path) -> dict[str, Any]:
    raw = path.read_bytes()
    try:
        value = json.loads(raw)
    except (UnicodeDecodeError, json.JSONDecodeError) as error:
        raise ValueError(f"invalid release inventory JSON: {path}") from error
    if not isinstance(value, dict):
        raise ValueError("release inventory root must be an object")
    canonical = (json.dumps(value, indent=2, sort_keys=True) + "\n").encode()
    if raw != canonical:
        raise ValueError("release inventory is not canonical JSON")
    return value


def validate_candidate_binding(inventory: dict[str, Any], tag: str, source_revision: str | None) -> None:
    candidate = validate_candidate_root(inventory["candidate_root"])
    if candidate_root_sha256(candidate) != inventory["candidate_root_sha256"]:
        raise ValueError("release inventory candidate-root digest mismatch")
    if candidate["release"]["expected_tag"] != tag:
        raise ValueError("release inventory candidate-root tag mismatch")
    if candidate["release"]["version"] != inventory["release_version"]:
        raise ValueError("release inventory candidate-root version mismatch")
    if source_revision is not None and candidate["source"]["revision"] != source_revision:
        raise ValueError("release inventory candidate-root source revision mismatch")


def verify_release(root: Path, tag: str, source_revision: str | None = None) -> dict[str, Any]:
    inventory_name = f"geometer-release-inventory-{tag}.json"
    assets = collect_assets(root)
    if inventory_name not in assets:
        raise ValueError(f"release inventory is missing: {inventory_name}")

    inventory = load_inventory(assets[inventory_name])
    if set(inventory) != {
        "assets",
        "candidate_root",
        "candidate_root_sha256",
        "release_tag",
        "release_version",
        "schema",
    }:
        raise ValueError("release inventory has unexpected root fields")
    if inventory["schema"] != SCHEMA:
        raise ValueError(f"unsupported release inventory schema: {inventory['schema']!r}")
    if inventory["release_tag"] != tag:
        raise ValueError(f"release inventory tag does not match {tag}")
    if inventory["release_version"] != release_version(tag):
        raise ValueError(f"release inventory version does not match {tag}")
    validate_candidate_binding(inventory, tag, source_revision)

    entries = inventory["assets"]
    if not isinstance(entries, list):
        raise ValueError("release inventory assets must be a list")
    recorded: dict[str, dict[str, Any]] = {}
    for entry in entries:
        if not isinstance(entry, dict) or set(entry) != {"name", "sha256", "size"}:
            raise ValueError("release inventory contains a malformed asset entry")
        name = entry["name"]
        if not isinstance(name, str) or not name or Path(name).name != name:
            raise ValueError(f"invalid release asset name: {name!r}")
        if name in recorded:
            raise ValueError(f"duplicate release inventory entry: {name}")
        recorded[name] = entry
    if list(recorded) != sorted(recorded):
        raise ValueError("release inventory assets are not in canonical name order")

    expected = expected_asset_names(tag)
    if set(recorded) != expected:
        raise ValueError("release inventory records an unexpected asset set")
    downloaded = set(assets) - {inventory_name}
    if downloaded != expected:
        missing = expected - downloaded
        extra = downloaded - expected
        raise ValueError(
            "downloaded release asset set does not match inventory; "
            f"missing={sorted(missing)}; unexpected={sorted(extra)}"
        )

    for name, entry in recorded.items():
        path = assets[name]
        if type(entry["size"]) is not int or entry["size"] < 0:
            raise ValueError(f"invalid recorded size for {name}")
        if path.stat().st_size != entry["size"]:
            raise ValueError(f"release asset size mismatch: {name}")
        digest = entry["sha256"]
        if not isinstance(digest, str) or re.fullmatch(r"[0-9a-f]{64}", digest) is None:
            raise ValueError(f"invalid recorded SHA-256 for {name}")
        if sha256_file(path) != digest:
            raise ValueError(f"release asset SHA-256 mismatch: {name}")
    return inventory


def main() -> None:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("root", type=Path)
    parser.add_argument("--tag", required=True)
    parser.add_argument("--source-revision")
    args = parser.parse_args()
    inventory = verify_release(args.root.resolve(), args.tag, args.source_revision)
    print(f"verified {len(inventory['assets'])} downloaded release assets for {args.tag}")


if __name__ == "__main__":
    main()
