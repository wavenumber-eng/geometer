#!/usr/bin/env python3
"""Create and validate canonical release-candidate root identities."""

from __future__ import annotations

import argparse
from datetime import date
import hashlib
import json
import os
from pathlib import Path, PurePosixPath
import re
import tempfile
from typing import Any, Sequence


SCHEMA = "wn.geometer.release_candidate_root.a0"
SHA256_RE = re.compile(r"[0-9a-f]{64}")
REVISION_RE = re.compile(r"[0-9a-f]{40}")
VERSION_RE = re.compile(
    r"(?P<year>[1-9][0-9]{3})\."
    r"(?P<month>0|[1-9][0-9]*)\."
    r"(?P<day>0|[1-9][0-9]*)"
    r"(?:\.(?P<serial>0|[1-9][0-9]*))?"
)
REPOSITORY_RE = re.compile(r"[A-Za-z0-9_.-]+/[A-Za-z0-9_.-]+")
LANE_ID_RE = re.compile(r"[a-z0-9][a-z0-9-]*")


class CandidateRootError(ValueError):
    """Raised when a release-candidate root is malformed or inconsistent."""


def canonical_json(value: dict[str, Any]) -> bytes:
    """Return the sole accepted JSON encoding for a candidate root."""
    return (json.dumps(value, indent=2, sort_keys=True, ensure_ascii=True) + "\n").encode("utf-8")


def file_sha256(path: Path) -> str:
    """Hash exact file bytes without interpreting their contents."""
    digest = hashlib.sha256()
    with path.open("rb") as stream:
        for chunk in iter(lambda: stream.read(1024 * 1024), b""):
            digest.update(chunk)
    return digest.hexdigest()


def candidate_root_sha256(value: dict[str, Any]) -> str:
    """Hash the canonical candidate-root encoding."""
    validate_candidate_root(value)
    return hashlib.sha256(canonical_json(value)).hexdigest()


def _object(value: Any, label: str) -> dict[str, Any]:
    if not isinstance(value, dict):
        raise CandidateRootError(f"{label} must be an object")
    return value


def _keys(value: dict[str, Any], expected: set[str], label: str) -> None:
    actual = set(value)
    if actual == expected:
        return
    missing = sorted(expected - actual)
    extra = sorted(actual - expected)
    raise CandidateRootError(f"{label} fields differ from schema; missing={missing}; extra={extra}")


def _text(value: Any, label: str) -> str:
    if not isinstance(value, str) or not value or "\r" in value or "\n" in value:
        raise CandidateRootError(f"{label} must be non-empty single-line text")
    return value


def _sha256(value: Any, label: str) -> str:
    text = _text(value, label)
    if SHA256_RE.fullmatch(text) is None:
        raise CandidateRootError(f"{label} must be a lowercase SHA-256 digest")
    return text


def _revision(value: Any, label: str) -> str:
    text = _text(value, label)
    if REVISION_RE.fullmatch(text) is None or set(text) == {"0"}:
        raise CandidateRootError(f"{label} must be a full lowercase 40-character Git revision")
    return text


def _repository(value: Any, label: str) -> str:
    text = _text(value, label)
    if REPOSITORY_RE.fullmatch(text) is None or text.endswith(".git"):
        raise CandidateRootError(f"{label} must be an owner/repository slug")
    return text


def _workflow_path(value: Any) -> str:
    text = _text(value, "workflow.path")
    path = PurePosixPath(text)
    if (
        path.is_absolute()
        or path.as_posix() != text
        or ".." in path.parts
        or path.parts[:2] != (".github", "workflows")
        or len(path.parts) != 3
        or path.suffix not in {".yml", ".yaml"}
    ):
        raise CandidateRootError("workflow.path must be a normalized .github/workflows YAML path")
    return text


def _release_identity(version: Any) -> tuple[str, str, int, str]:
    text = _text(version, "release.version")
    match = VERSION_RE.fullmatch(text)
    if match is None:
        raise CandidateRootError("release.version must be canonical YYYY.M.D or YYYY.M.D.N")
    year = int(match.group("year"))
    month = int(match.group("month"))
    day = int(match.group("day"))
    try:
        release_day = date(year, month, day)
    except ValueError as error:
        raise CandidateRootError(f"release.version contains an invalid calendar date: {text}") from error
    release_date = release_day.isoformat()
    abi_generation = int(release_day.strftime("%Y%m%d"))
    expected_tag = f"v{release_date}"
    serial = match.group("serial")
    if serial is not None:
        expected_tag = f"{expected_tag}-{serial}"
    return text, release_date, abi_generation, expected_tag


def validate_candidate_root(value: Any) -> dict[str, Any]:
    """Validate a candidate-root value without consulting a checkout or network."""
    root = _object(value, "candidate root")
    _keys(
        root,
        {"lanes", "occt_lock_sha256", "policy_sha256", "release", "schema", "source", "workflow"},
        "candidate root",
    )
    if root["schema"] != SCHEMA:
        raise CandidateRootError(f"unsupported candidate-root schema: {root['schema']!r}")

    source = _object(root["source"], "source")
    _keys(source, {"revision"}, "source")
    _revision(source["revision"], "source.revision")

    release = _object(root["release"], "release")
    _keys(release, {"abi_generation", "date", "expected_tag", "version"}, "release")
    _, expected_date, expected_abi, expected_tag = _release_identity(release["version"])
    if release["date"] != expected_date:
        raise CandidateRootError(f"release.date must be {expected_date} for release.version")
    if type(release["abi_generation"]) is not int or release["abi_generation"] != expected_abi:
        raise CandidateRootError(f"release.abi_generation must be {expected_abi} for release.version")
    if release["expected_tag"] != expected_tag:
        raise CandidateRootError(f"release.expected_tag must be {expected_tag} for release.version")

    workflow = _object(root["workflow"], "workflow")
    _keys(workflow, {"file_sha256", "path", "repository", "revision"}, "workflow")
    _repository(workflow["repository"], "workflow.repository")
    _workflow_path(workflow["path"])
    _revision(workflow["revision"], "workflow.revision")
    _sha256(workflow["file_sha256"], "workflow.file_sha256")
    _sha256(root["policy_sha256"], "policy_sha256")
    _sha256(root["occt_lock_sha256"], "occt_lock_sha256")

    lanes = root["lanes"]
    if not isinstance(lanes, list) or not lanes:
        raise CandidateRootError("lanes must be a non-empty array")
    lane_ids: list[str] = []
    for index, raw_lane in enumerate(lanes):
        label = f"lanes[{index}]"
        lane = _object(raw_lane, label)
        _keys(lane, {"id", "recipe_sha256", "toolchain_sha256"}, label)
        lane_id = _text(lane["id"], f"{label}.id")
        if LANE_ID_RE.fullmatch(lane_id) is None:
            raise CandidateRootError(f"{label}.id must be a lowercase lane identifier")
        _sha256(lane["recipe_sha256"], f"{label}.recipe_sha256")
        _sha256(lane["toolchain_sha256"], f"{label}.toolchain_sha256")
        lane_ids.append(lane_id)
    if len(set(lane_ids)) != len(lane_ids):
        raise CandidateRootError("lane identifiers must be unique")
    if lane_ids != sorted(lane_ids):
        raise CandidateRootError("lanes must be ordered lexicographically by id")
    return root


def create_candidate_root(
    *,
    source_revision: str,
    release_version: str,
    release_date: str,
    abi_generation: int,
    expected_tag: str,
    workflow_repository: str,
    workflow_path: str,
    workflow_revision: str,
    workflow_file_sha256: str,
    policy_sha256: str,
    occt_lock_sha256: str,
    lanes: list[dict[str, str]],
) -> dict[str, Any]:
    """Construct and validate a candidate root, normalizing lane order."""
    value: dict[str, Any] = {
        "lanes": sorted(lanes, key=lambda lane: lane.get("id", "")),
        "occt_lock_sha256": occt_lock_sha256,
        "policy_sha256": policy_sha256,
        "release": {
            "abi_generation": abi_generation,
            "date": release_date,
            "expected_tag": expected_tag,
            "version": release_version,
        },
        "schema": SCHEMA,
        "source": {"revision": source_revision},
        "workflow": {
            "file_sha256": workflow_file_sha256,
            "path": workflow_path,
            "repository": workflow_repository,
            "revision": workflow_revision,
        },
    }
    return validate_candidate_root(value)


def load_candidate_root(path: Path) -> dict[str, Any]:
    """Load a candidate root and require its exact canonical encoding."""
    try:
        raw = path.read_bytes()
        value = json.loads(raw)
    except OSError as error:
        raise CandidateRootError(f"could not read candidate root {path}: {error}") from error
    except (UnicodeDecodeError, json.JSONDecodeError) as error:
        raise CandidateRootError(f"invalid candidate-root JSON: {path}") from error
    root = validate_candidate_root(value)
    if raw != canonical_json(root):
        raise CandidateRootError("candidate root is not canonical deterministic JSON")
    return root


def write_candidate_root(path: Path, value: dict[str, Any]) -> None:
    """Atomically create a validated candidate root without replacing any path."""
    validate_candidate_root(value)
    if path.exists() or path.is_symlink():
        raise CandidateRootError(f"refusing to overwrite existing candidate root: {path}")
    path.parent.mkdir(parents=True, exist_ok=True)
    temporary: Path | None = None
    try:
        with tempfile.NamedTemporaryFile(
            mode="wb", prefix=f".{path.name}.", suffix=".tmp", dir=path.parent, delete=False
        ) as stream:
            temporary = Path(stream.name)
            stream.write(canonical_json(value))
            stream.flush()
            os.fsync(stream.fileno())
        os.link(temporary, path)
    except FileExistsError as error:
        raise CandidateRootError(f"refusing to overwrite existing candidate root: {path}") from error
    except OSError as error:
        raise CandidateRootError(f"could not atomically create candidate root {path}: {error}") from error
    finally:
        if temporary is not None:
            temporary.unlink(missing_ok=True)


def _digest(value: str | None, path: Path | None, label: str) -> str:
    if (value is None) == (path is None):
        raise CandidateRootError(f"provide exactly one of --{label}-sha256 or --{label}-file")
    if value is not None:
        return _sha256(value, label)
    if path is None:
        raise AssertionError("digest path must be present")
    try:
        return file_sha256(path)
    except OSError as error:
        raise CandidateRootError(f"could not hash {label} file {path}: {error}") from error


def _lane(value: str) -> dict[str, str]:
    parts = value.split(":")
    if len(parts) != 3:
        raise argparse.ArgumentTypeError("lane must be ID:RECIPE_SHA256:TOOLCHAIN_SHA256")
    return {"id": parts[0], "recipe_sha256": parts[1], "toolchain_sha256": parts[2]}


def _add_digest_options(parser: argparse.ArgumentParser, label: str) -> None:
    group = parser.add_mutually_exclusive_group(required=True)
    group.add_argument(f"--{label}-sha256")
    group.add_argument(f"--{label}-file", type=Path)


def _parser() -> argparse.ArgumentParser:
    parser = argparse.ArgumentParser(description=__doc__)
    commands = parser.add_subparsers(dest="command", required=True)
    create = commands.add_parser("create", help="create a canonical candidate-root document")
    create.add_argument("output", type=Path)
    create.add_argument("--source-revision", required=True)
    create.add_argument("--release-version", required=True)
    create.add_argument("--release-date", required=True)
    create.add_argument("--abi-generation", required=True, type=int)
    create.add_argument("--expected-tag", required=True)
    create.add_argument("--workflow-repository", required=True)
    create.add_argument("--workflow-path", required=True)
    create.add_argument("--workflow-revision", required=True)
    _add_digest_options(create, "workflow")
    _add_digest_options(create, "policy")
    _add_digest_options(create, "occt-lock")
    create.add_argument(
        "--lane", action="append", required=True, type=_lane, metavar="ID:RECIPE_SHA256:TOOLCHAIN_SHA256"
    )
    validate = commands.add_parser("validate", help="validate a canonical candidate-root document")
    validate.add_argument("input", type=Path)
    return parser


def main(argv: Sequence[str] | None = None) -> int:
    args = _parser().parse_args(argv)
    try:
        if args.command == "create":
            value = create_candidate_root(
                source_revision=args.source_revision,
                release_version=args.release_version,
                release_date=args.release_date,
                abi_generation=args.abi_generation,
                expected_tag=args.expected_tag,
                workflow_repository=args.workflow_repository,
                workflow_path=args.workflow_path,
                workflow_revision=args.workflow_revision,
                workflow_file_sha256=_digest(args.workflow_sha256, args.workflow_file, "workflow"),
                policy_sha256=_digest(args.policy_sha256, args.policy_file, "policy"),
                occt_lock_sha256=_digest(args.occt_lock_sha256, args.occt_lock_file, "occt-lock"),
                lanes=args.lane,
            )
            write_candidate_root(args.output, value)
            path = args.output
        else:
            value = load_candidate_root(args.input)
            path = args.input
    except CandidateRootError as error:
        raise SystemExit(str(error)) from error
    print(f"candidate root valid: {path} sha256={candidate_root_sha256(value)}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
