"""Publish validated OCCT candidate packages from a secret-free build handoff."""

from __future__ import annotations

import argparse
from pathlib import Path

import occt_lock
import occt_producer


ALL_PROFILES = "all"


def candidate_directories(root: Path, expected_profile: str) -> list[Path]:
    directories = sorted(path.parent for path in root.rglob(occt_producer.MANIFEST_NAME))
    actual = {path.name for path in directories}
    lock_ids = {profile["id"] for profile in occt_lock.load_lock()["profiles"]}
    expected = lock_ids if expected_profile == ALL_PROFILES else {expected_profile}
    if expected_profile != ALL_PROFILES and expected_profile not in lock_ids:
        raise RuntimeError(f"Unknown locked OCCT profile: {expected_profile}")
    if actual != expected or len(directories) != len(actual):
        raise RuntimeError(f"OCCT candidate handoff profiles mismatch: expected {sorted(expected)}, got {sorted(actual)}")
    return directories


def main() -> None:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--root", type=Path, required=True)
    parser.add_argument("--expected-profile", required=True)
    args = parser.parse_args()

    for directory in candidate_directories(args.root, args.expected_profile):
        occt_producer.publish_candidate(directory)


if __name__ == "__main__":
    main()
