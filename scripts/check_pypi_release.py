#!/usr/bin/env python3
"""Classify PyPI release files as exact, missing, or conflicting."""

from __future__ import annotations

import argparse
import json
import os
from pathlib import Path
from typing import Any, Sequence, TypedDict
import urllib.error
import urllib.request

from plan_release_promotion import load_canonical_json, validate_release_inventory


PROJECT = "wn-geometer"


class Classification(TypedDict):
    exact: list[str]
    missing: list[str]
    needs_upload: bool


def classify(inventory_value: Any, project_value: Any | None) -> Classification:
    inventory = validate_release_inventory(inventory_value)
    expected = {entry["name"]: entry for entry in inventory["assets"] if entry["name"].endswith(".whl")}
    if project_value is None:
        observed_entries: list[Any] = []
    elif not isinstance(project_value, dict) or not isinstance(project_value.get("releases"), dict):
        raise ValueError("PyPI response does not contain a releases object")
    else:
        observed_entries = project_value["releases"].get(inventory["release_version"], [])
        if not isinstance(observed_entries, list):
            raise ValueError("PyPI release entry is not a list")

    observed: dict[str, dict[str, object]] = {}
    for raw in observed_entries:
        if not isinstance(raw, dict) or not isinstance(raw.get("filename"), str):
            raise ValueError("PyPI release contains a malformed file entry")
        name = raw["filename"]
        if not name.endswith(".whl"):
            raise ValueError(f"PyPI release contains an unexpected non-wheel file: {name}")
        digests = raw.get("digests")
        if not isinstance(digests, dict) or not isinstance(digests.get("sha256"), str):
            raise ValueError(f"PyPI release omits a SHA-256 digest: {name}")
        if type(raw.get("size")) is not int:
            raise ValueError(f"PyPI release omits a valid size: {name}")
        if name in observed:
            raise ValueError(f"PyPI release repeats a filename: {name}")
        observed[name] = {"sha256": digests["sha256"], "size": raw["size"]}

    extras = sorted(set(observed) - set(expected))
    if extras:
        raise ValueError("PyPI release contains unexpected files: " + ", ".join(extras))
    for name in sorted(set(observed) & set(expected)):
        if observed[name] != {"sha256": expected[name]["sha256"], "size": expected[name]["size"]}:
            raise ValueError(f"PyPI file identity conflicts with the candidate: {name}")
    missing = sorted(set(expected) - set(observed))
    return {"exact": sorted(observed), "missing": missing, "needs_upload": bool(missing)}


def fetch_project(project: str = PROJECT) -> Any | None:
    request = urllib.request.Request(
        f"https://pypi.org/pypi/{project}/json",
        headers={"Accept": "application/json", "User-Agent": "wn-geometer-release-verifier"},
    )
    try:
        with urllib.request.urlopen(request, timeout=30) as response:
            return json.load(response)
    except urllib.error.HTTPError as error:
        if error.code == 404:
            return None
        raise


def main(argv: Sequence[str] | None = None) -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("inventory", type=Path)
    parser.add_argument("--github-output", type=Path)
    args = parser.parse_args(argv)
    inventory = load_canonical_json(args.inventory.resolve(), label="release inventory")
    result = classify(inventory, fetch_project())
    if args.github_output is not None:
        with args.github_output.open("a", encoding="utf-8", newline="\n") as stream:
            stream.write(f"needs_upload={'true' if result['needs_upload'] else 'false'}\n")
            stream.write(f"missing_count={len(result['missing'])}\n")
    elif os.environ.get("GITHUB_OUTPUT"):
        with Path(os.environ["GITHUB_OUTPUT"]).open("a", encoding="utf-8", newline="\n") as stream:
            stream.write(f"needs_upload={'true' if result['needs_upload'] else 'false'}\n")
            stream.write(f"missing_count={len(result['missing'])}\n")
    print(json.dumps(result, sort_keys=True))
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
