#!/usr/bin/env python3
"""Create a canonical digest inventory for already-published channel files."""

from __future__ import annotations

import argparse
import hashlib
from pathlib import Path
from typing import Any, Sequence

from plan_release_promotion import CHANNEL_INVENTORY_SCHEMA, canonical_bytes, validate_channel_inventory


def snapshot(directory: Path, channel: str, excluded: set[str] | None = None) -> dict[str, Any]:
    excluded = excluded or set()
    if not directory.is_dir():
        raise ValueError(f"channel directory does not exist: {directory}")
    nested = [path for path in directory.rglob("*") if path.is_file() and path.parent != directory]
    if nested:
        raise ValueError("channel snapshot accepts only a flat asset directory")
    assets = []
    for path in sorted(directory.iterdir(), key=lambda item: item.name):
        if not path.is_file() or path.name in excluded:
            continue
        payload = path.read_bytes()
        assets.append({"name": path.name, "sha256": hashlib.sha256(payload).hexdigest(), "size": len(payload)})
    return validate_channel_inventory(
        {"assets": assets, "channel": channel, "schema": CHANNEL_INVENTORY_SCHEMA}
    )


def main(argv: Sequence[str] | None = None) -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("directory", type=Path)
    parser.add_argument("--channel", required=True)
    parser.add_argument("--exclude", action="append", default=[])
    parser.add_argument("--output", type=Path, required=True)
    args = parser.parse_args(argv)
    value = snapshot(args.directory.resolve(), args.channel, set(args.exclude))
    args.output.parent.mkdir(parents=True, exist_ok=True)
    args.output.write_bytes(canonical_bytes(value))
    print(f"snapshotted {len(value['assets'])} {args.channel} assets: {args.output}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
