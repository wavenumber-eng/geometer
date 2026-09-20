#!/usr/bin/env python3
"""Conditionally create an immutable, inventory-complete release candidate in R2."""

from __future__ import annotations

import argparse
import hashlib
import json
from pathlib import Path
from typing import Any, Callable, Sequence, TypedDict

import r2_store
from validate_release_inventory import collect_assets, validate_inventory
from verify_release_inventory import verify_release


CreateObject = Callable[[r2_store.R2Config, str, bytes, str], bool]
CandidateValidator = Callable[[Path, str, str | None], dict[str, Any]]


class PublishReport(TypedDict):
    created: list[str]
    existing: list[str]
    inventory_sha256: str
    prefix: str


def _sha256(path: Path) -> str:
    digest = hashlib.sha256()
    with path.open("rb") as stream:
        for chunk in iter(lambda: stream.read(1024 * 1024), b""):
            digest.update(chunk)
    return digest.hexdigest()


def _content_type(name: str) -> str:
    if name.endswith(".json"):
        return "application/json"
    if name.endswith(".whl") or name.endswith(".zip"):
        return "application/zip"
    return "text/plain; charset=utf-8"


def validate_candidate(root: Path, tag: str, source_revision: str | None) -> dict[str, Any]:
    if source_revision is None:
        raise ValueError("release candidate publication requires a source revision")
    inventory = verify_release(root, tag, source_revision)
    rebuilt = validate_inventory(root, tag, inventory["candidate_root"])
    if rebuilt != inventory:
        raise ValueError("release candidate inventory does not match fresh artifact validation")
    return inventory


def publish_candidate(
    root: Path,
    tag: str,
    source_revision: str,
    config: r2_store.R2Config,
    create: CreateObject = r2_store.create_or_verify,
    validator: CandidateValidator = validate_candidate,
) -> PublishReport:
    """Publish assets first and their canonical inventory last."""

    inventory = validator(root, tag, source_revision)
    inventory_name = f"geometer-release-inventory-{tag}.json"
    files = collect_assets(root)
    inventory_path = files[inventory_name]
    inventory_sha256 = _sha256(inventory_path)
    prefix = f"releases/candidates/{source_revision}/{inventory_sha256}"
    created: list[str] = []
    existing: list[str] = []
    ordered_names = [entry["name"] for entry in inventory["assets"]] + [inventory_name]
    for name in ordered_names:
        key = f"{prefix}/{name}"
        was_created = create(config, key, files[name].read_bytes(), _content_type(name))
        (created if was_created else existing).append(name)
    return {
        "created": created,
        "existing": existing,
        "inventory_sha256": inventory_sha256,
        "prefix": prefix,
    }


def main(argv: Sequence[str] | None = None) -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("root", type=Path)
    parser.add_argument("--tag", required=True)
    parser.add_argument("--source-revision", required=True)
    parser.add_argument("--report", type=Path)
    args = parser.parse_args(argv)
    config = r2_store.config_from_env()
    if config is None:
        parser.error("R2_BUCKET, R2_ENDPOINT_URL, R2_ACCESS_KEY_ID, and R2_SECRET_ACCESS_KEY are required")
    report = publish_candidate(args.root.resolve(), args.tag, args.source_revision, config)
    if args.report is not None:
        publication = {
            "created": report["created"],
            "existing": report["existing"],
            "inventory_sha256": report["inventory_sha256"],
            "prefix": report["prefix"],
            "release_tag": args.tag,
            "schema": "wn.geometer.release_candidate_publication.a0",
            "source_revision": args.source_revision,
        }
        args.report.parent.mkdir(parents=True, exist_ok=True)
        args.report.write_text(
            json.dumps(publication, indent=2, sort_keys=True) + "\n",
            encoding="utf-8",
            newline="\n",
        )
    print(
        f"candidate stored at {report['prefix']}; created={len(report['created'])}; existing={len(report['existing'])}"
    )
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
