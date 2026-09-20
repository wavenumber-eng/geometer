#!/usr/bin/env python3
"""Download one exact, immutable release candidate from R2."""

from __future__ import annotations

import argparse
import hashlib
import json
import os
from pathlib import Path
import re
import shutil
import tempfile
from typing import Any, Callable, Sequence

from plan_release_promotion import canonical_bytes, validate_release_inventory
import r2_store
from validate_release_inventory import expected_asset_names


SHA256 = re.compile(r"[0-9a-f]{64}")
REVISION = re.compile(r"[0-9a-f]{40}")
MAX_INVENTORY_BYTES = 1024 * 1024
MAX_ASSET_BYTES = 2 * 1024 * 1024 * 1024
MAX_CANDIDATE_BYTES = 8 * 1024 * 1024 * 1024

GetObject = Callable[[r2_store.R2Config, str], bytes | None]


def _digest(payload: bytes) -> str:
    return hashlib.sha256(payload).hexdigest()


def _identity(tag: str, source_revision: str, inventory_sha256: str) -> tuple[str, str]:
    if REVISION.fullmatch(source_revision) is None:
        raise ValueError("source revision must be a full lowercase Git SHA")
    if SHA256.fullmatch(inventory_sha256) is None:
        raise ValueError("inventory digest must be a lowercase SHA-256")
    inventory_name = f"geometer-release-inventory-{tag}.json"
    prefix = f"releases/candidates/{source_revision}/{inventory_sha256}"
    return prefix, inventory_name


def fetch_candidate(
    output: Path,
    tag: str,
    source_revision: str,
    inventory_sha256: str,
    config: r2_store.R2Config,
    get: GetObject = r2_store.get_object,
) -> dict[str, Any]:
    """Fetch exact candidate bytes into a new directory and verify every digest."""

    prefix, inventory_name = _identity(tag, source_revision, inventory_sha256)
    if output.exists() or output.is_symlink():
        raise ValueError(f"candidate output already exists: {output}")
    inventory_bytes = get(config, f"{prefix}/{inventory_name}")
    if inventory_bytes is None:
        raise ValueError("candidate inventory is absent from R2")
    if len(inventory_bytes) > MAX_INVENTORY_BYTES:
        raise ValueError("candidate inventory exceeds the size limit")
    if _digest(inventory_bytes) != inventory_sha256:
        raise ValueError("candidate inventory digest does not match its R2 identity")
    try:
        decoded = json.loads(inventory_bytes)
    except (UnicodeDecodeError, json.JSONDecodeError) as error:
        raise ValueError("candidate inventory is not valid JSON") from error
    if inventory_bytes != canonical_bytes(decoded):
        raise ValueError("candidate inventory is not canonical JSON")
    inventory = validate_release_inventory(decoded)
    if inventory["release_tag"] != tag:
        raise ValueError("candidate inventory release tag mismatch")
    if inventory["candidate_root"]["source"]["revision"] != source_revision:
        raise ValueError("candidate inventory source revision mismatch")
    expected = expected_asset_names(tag)
    recorded = {entry["name"]: entry for entry in inventory["assets"]}
    if set(recorded) != expected:
        raise ValueError("candidate inventory contains an unexpected release asset set")
    total = sum(entry["size"] for entry in recorded.values())
    if any(entry["size"] > MAX_ASSET_BYTES for entry in recorded.values()) or total > MAX_CANDIDATE_BYTES:
        raise ValueError("candidate assets exceed the download size limits")

    output.parent.mkdir(parents=True, exist_ok=True)
    temporary = Path(tempfile.mkdtemp(prefix=f".{output.name}.", dir=output.parent))
    try:
        for name in sorted(recorded):
            payload = get(config, f"{prefix}/{name}")
            if payload is None:
                raise ValueError(f"candidate asset is absent from R2: {name}")
            entry = recorded[name]
            if len(payload) != entry["size"] or _digest(payload) != entry["sha256"]:
                raise ValueError(f"candidate asset identity mismatch: {name}")
            (temporary / name).write_bytes(payload)
        (temporary / inventory_name).write_bytes(inventory_bytes)
        os.replace(temporary, output)
    except BaseException:
        shutil.rmtree(temporary, ignore_errors=True)
        raise
    return inventory


def main(argv: Sequence[str] | None = None) -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("output", type=Path)
    parser.add_argument("--tag", required=True)
    parser.add_argument("--source-revision", required=True)
    parser.add_argument("--inventory-sha256", required=True)
    args = parser.parse_args(argv)
    config = r2_store.config_from_env()
    if config is None:
        parser.error("R2_BUCKET, R2_ENDPOINT_URL, R2_ACCESS_KEY_ID, and R2_SECRET_ACCESS_KEY are required")
    inventory = fetch_candidate(
        args.output.resolve(),
        args.tag,
        args.source_revision,
        args.inventory_sha256,
        config,
    )
    print(f"fetched {len(inventory['assets'])} candidate assets: {args.output.resolve()}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
