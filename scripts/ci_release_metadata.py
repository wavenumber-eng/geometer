#!/usr/bin/env python3
"""Validate release metadata from GitHub Actions."""

from __future__ import annotations

import argparse
import tomllib
from pathlib import Path


ROOT = Path(__file__).resolve().parents[1]


def package_version() -> str:
    with (ROOT / "pyproject.toml").open("rb") as handle:
        return tomllib.load(handle)["project"]["version"]


def release_date(version: str) -> str:
    year, month, day, *_ = (int(part) for part in version.split("."))
    return f"{year:04d}-{month:02d}-{day:02d}"


def check_tag(tag: str) -> None:
    version = package_version()
    expected = f"v{release_date(version)}"
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


def main() -> None:
    parser = argparse.ArgumentParser(description=__doc__)
    subparsers = parser.add_subparsers(dest="command", required=True)
    subparsers.add_parser("version")
    subparsers.add_parser("date")
    tag_parser = subparsers.add_parser("check-tag")
    tag_parser.add_argument("tag")
    subparsers.add_parser("check-notes")
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
    else:
        raise AssertionError(args.command)


if __name__ == "__main__":
    main()
