#!/usr/bin/env python3
"""Create the immutable R2 tag alias for an already-stored candidate."""

from __future__ import annotations

import argparse
import hashlib
import re
from typing import Callable, Sequence

from plan_release_promotion import canonical_bytes
import r2_store
from validate_release_inventory import release_version


SCHEMA = "wn.geometer.release_tag_alias.a0"
SHA256 = re.compile(r"[0-9a-f]{64}")
REVISION = re.compile(r"[0-9a-f]{40}")
GetObject = Callable[[r2_store.R2Config, str], bytes | None]
CreateObject = Callable[[r2_store.R2Config, str, bytes, str], bool]


def publish_tag_alias(
    tag: str,
    source_revision: str,
    inventory_sha256: str,
    config: r2_store.R2Config,
    *,
    get: GetObject = r2_store.get_object,
    create: CreateObject = r2_store.create_or_verify,
) -> tuple[str, bool]:
    """Verify the candidate commit marker, then create its tag alias once."""

    version = release_version(tag)
    if REVISION.fullmatch(source_revision) is None:
        raise ValueError("source revision must be a full lowercase Git SHA")
    if SHA256.fullmatch(inventory_sha256) is None:
        raise ValueError("inventory digest must be a lowercase SHA-256")
    inventory_name = f"geometer-release-inventory-{tag}.json"
    prefix = f"releases/candidates/{source_revision}/{inventory_sha256}"
    inventory_key = f"{prefix}/{inventory_name}"
    inventory = get(config, inventory_key)
    if inventory is None or hashlib.sha256(inventory).hexdigest() != inventory_sha256:
        raise ValueError("R2 candidate inventory is absent or does not match its digest")
    alias = canonical_bytes(
        {
            "candidate_prefix": prefix,
            "inventory_key": inventory_key,
            "inventory_sha256": inventory_sha256,
            "release_tag": tag,
            "release_version": version,
            "schema": SCHEMA,
            "source_revision": source_revision,
        }
    )
    key = f"releases/tags/{tag}.json"
    return key, create(config, key, alias, "application/json")


def main(argv: Sequence[str] | None = None) -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--tag", required=True)
    parser.add_argument("--source-revision", required=True)
    parser.add_argument("--inventory-sha256", required=True)
    args = parser.parse_args(argv)
    config = r2_store.config_from_env()
    if config is None:
        parser.error("R2_BUCKET, R2_ENDPOINT_URL, R2_ACCESS_KEY_ID, and R2_SECRET_ACCESS_KEY are required")
    key, created = publish_tag_alias(
        args.tag,
        args.source_revision,
        args.inventory_sha256,
        config,
    )
    print(f"R2 release tag {'created' if created else 'already exact'}: {key}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
