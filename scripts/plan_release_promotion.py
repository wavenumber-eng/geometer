"""Plan an idempotent release promotion without modifying a release channel."""

from __future__ import annotations

import argparse
import hashlib
import json
import re
from pathlib import Path
from typing import Any

from validate_release_inventory import release_version


RELEASE_INVENTORY_SCHEMA = "wn.geometer.release_inventory.a0"
CHANNEL_INVENTORY_SCHEMA = "wn.geometer.release_channel_inventory.a0"
PROMOTION_PLAN_SCHEMA = "wn.geometer.release_promotion_plan.a0"
ASSET_FIELDS = {"name", "sha256", "size"}
SAFE_ASSET_NAME = re.compile(r"[A-Za-z0-9][A-Za-z0-9._+-]{0,254}")
SHA256 = re.compile(r"[0-9a-f]{64}")


def canonical_bytes(value: Any) -> bytes:
    """Return the repository's canonical, newline-terminated JSON encoding."""

    return (json.dumps(value, indent=2, sort_keys=True) + "\n").encode("utf-8")


def _validate_asset(entry: Any, *, inventory_label: str) -> dict[str, Any]:
    if not isinstance(entry, dict) or set(entry) != ASSET_FIELDS:
        raise ValueError(f"{inventory_label} contains a malformed asset entry")

    name = entry["name"]
    if not isinstance(name, str) or SAFE_ASSET_NAME.fullmatch(name) is None:
        raise ValueError(f"{inventory_label} contains an unsafe asset name: {name!r}")

    size = entry["size"]
    if type(size) is not int or size < 0:
        raise ValueError(f"{inventory_label} contains an invalid size for {name}")

    digest = entry["sha256"]
    if not isinstance(digest, str) or SHA256.fullmatch(digest) is None:
        raise ValueError(f"{inventory_label} contains an invalid SHA-256 for {name}")

    return {"name": name, "sha256": digest, "size": size}


def _index_assets(
    entries: Any,
    *,
    inventory_label: str,
    require_canonical_order: bool,
) -> dict[str, dict[str, Any]]:
    if not isinstance(entries, list):
        raise ValueError(f"{inventory_label} assets must be a list")

    indexed: dict[str, dict[str, Any]] = {}
    names: list[str] = []
    for raw_entry in entries:
        entry = _validate_asset(raw_entry, inventory_label=inventory_label)
        name = entry["name"]
        if name in indexed:
            raise ValueError(f"duplicate asset name in {inventory_label}: {name}")
        indexed[name] = entry
        names.append(name)

    if require_canonical_order and names != sorted(names):
        raise ValueError(f"{inventory_label} assets are not in canonical name order")
    return indexed


def validate_release_inventory(inventory: Any) -> dict[str, Any]:
    """Validate and normalize a canonical release inventory."""

    if not isinstance(inventory, dict):
        raise ValueError("expected release inventory root must be an object")
    if set(inventory) != {"assets", "release_tag", "release_version", "schema"}:
        raise ValueError("expected release inventory has unexpected root fields")
    if inventory["schema"] != RELEASE_INVENTORY_SCHEMA:
        raise ValueError(f"unsupported release inventory schema: {inventory['schema']!r}")

    tag = inventory["release_tag"]
    if not isinstance(tag, str):
        raise ValueError("expected release inventory tag must be a string")
    version = release_version(tag)
    if inventory["release_version"] != version:
        raise ValueError(f"expected release inventory version does not match {tag}")

    assets = _index_assets(
        inventory["assets"],
        inventory_label="expected release inventory",
        require_canonical_order=True,
    )
    return {
        "assets": [assets[name] for name in sorted(assets)],
        "release_tag": tag,
        "release_version": version,
        "schema": RELEASE_INVENTORY_SCHEMA,
    }


def validate_channel_inventory(inventory: Any) -> dict[str, Any]:
    """Validate and normalize a read-only snapshot of one publication channel."""

    if not isinstance(inventory, dict):
        raise ValueError("observed channel inventory root must be an object")
    if set(inventory) != {"assets", "channel", "schema"}:
        raise ValueError("observed channel inventory has unexpected root fields")
    if inventory["schema"] != CHANNEL_INVENTORY_SCHEMA:
        raise ValueError(f"unsupported channel inventory schema: {inventory['schema']!r}")

    channel = inventory["channel"]
    if not isinstance(channel, str) or re.fullmatch(r"[a-z][a-z0-9_-]{0,63}", channel) is None:
        raise ValueError(f"invalid release channel name: {channel!r}")
    assets = _index_assets(
        inventory["assets"],
        inventory_label="observed channel inventory",
        require_canonical_order=False,
    )
    return {
        "assets": [assets[name] for name in sorted(assets)],
        "channel": channel,
        "schema": CHANNEL_INVENTORY_SCHEMA,
    }


def plan_release_promotion(expected: Any, observed: Any) -> dict[str, Any]:
    """Classify exact existing assets and missing uploads, or reject the channel state."""

    release = validate_release_inventory(expected)
    channel = validate_channel_inventory(observed)
    expected_assets = {entry["name"]: entry for entry in release["assets"]}
    observed_assets = {entry["name"]: entry for entry in channel["assets"]}

    extras = sorted(set(observed_assets) - set(expected_assets))
    if extras:
        raise ValueError("observed channel contains unexpected assets: " + ", ".join(extras))

    existing: list[dict[str, Any]] = []
    uploads: list[dict[str, Any]] = []
    for name, expected_entry in expected_assets.items():
        observed_entry = observed_assets.get(name)
        if observed_entry is None:
            uploads.append(expected_entry)
            continue
        if observed_entry["size"] != expected_entry["size"]:
            raise ValueError(f"observed channel asset size mismatch: {name}")
        if observed_entry["sha256"] != expected_entry["sha256"]:
            raise ValueError(f"observed channel asset SHA-256 mismatch: {name}")
        existing.append(expected_entry)

    return {
        "channel": channel["channel"],
        "existing": existing,
        "expected_inventory_sha256": hashlib.sha256(canonical_bytes(release)).hexdigest(),
        "observed_inventory_sha256": hashlib.sha256(canonical_bytes(channel)).hexdigest(),
        "release_tag": release["release_tag"],
        "release_version": release["release_version"],
        "schema": PROMOTION_PLAN_SCHEMA,
        "uploads": uploads,
    }


def load_canonical_json(path: Path, *, label: str) -> Any:
    raw = path.read_bytes()
    try:
        value = json.loads(raw)
    except (UnicodeDecodeError, json.JSONDecodeError) as error:
        raise ValueError(f"invalid {label} JSON: {path}") from error
    if raw != canonical_bytes(value):
        raise ValueError(f"{label} is not canonical JSON: {path}")
    return value


def write_plan(path: Path, plan: dict[str, Any]) -> None:
    path.parent.mkdir(parents=True, exist_ok=True)
    path.write_bytes(canonical_bytes(plan))


def main() -> None:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("expected_inventory", type=Path)
    parser.add_argument("observed_inventory", type=Path)
    parser.add_argument("--output", type=Path, required=True)
    args = parser.parse_args()

    expected = load_canonical_json(args.expected_inventory.resolve(), label="expected release inventory")
    observed = load_canonical_json(args.observed_inventory.resolve(), label="observed channel inventory")
    plan = plan_release_promotion(expected, observed)
    output = args.output.resolve()
    write_plan(output, plan)
    print(
        f"planned {len(plan['uploads'])} uploads and {len(plan['existing'])} exact existing assets "
        f"for {plan['channel']}: {output}"
    )


if __name__ == "__main__":
    main()
