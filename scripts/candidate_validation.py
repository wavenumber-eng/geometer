#!/usr/bin/env python3
"""Build and verify the candidate's source-bound build/test evidence record."""

from __future__ import annotations

import argparse
import hashlib
import json
from pathlib import Path
from typing import Any, Sequence

from candidate_root import candidate_root_sha256, load_candidate_root, validate_candidate_root
from ci_execution_ledger import canonical_json as ledger_canonical_json
from ci_execution_ledger import load_ledger, validate_ledger
from release_asset_names import PLATFORMS, WHEEL_SUFFIXES, product_asset_names, release_version


SCHEMA = "wn.geometer.release_candidate_validation.a0"
LANES = ("release-verify", *PLATFORMS, "wasm")
RELEASE_VERIFY_TASKS = (
    ("test", "release orchestration preflight"),
    ("test", "generated contract drift"),
    ("test", "release metadata tag"),
    ("test", "candidate source identity"),
    ("test", "candidate main ancestry"),
    ("test", "release notes version"),
    ("test", "L99 release gate"),
    ("test", "repository standards"),
    ("test", "release standards audit"),
)
NATIVE_TASKS = (
    ("build", "static C ABI SDK"),
    ("test", "relocated static SDK consumer"),
    ("build-test", "production native validation"),
)
LINUX_X64_CLIENT_TASKS = (
    ("test", "Python client stratum"),
    ("test", "Rust client stratum"),
    ("prepare", "pinned Node dependencies"),
    ("test", "Node toolchain version"),
    ("test", "TypeScript host client stratum"),
)
NATIVE_PACKAGE_TASKS = (
    ("build-test", "Python package validation"),
    ("test", "wheel metadata"),
    ("package", "native distribution"),
)
WASM_TASKS = (
    ("prepare", "pinned Node dependencies"),
    ("test", "Node toolchain version"),
    ("build", "WASM artifacts"),
    ("test", "TypeScript check"),
    ("build", "HLR browser site"),
    ("build", "illustration browser site"),
    ("build", "standalone HLR demo"),
    ("build", "standalone illustration demo"),
    ("test", "browser site validation"),
    ("test", "TypeScript WASM client stratum"),
    ("test", "WASM planar batch validation"),
    ("test", "WASM STEP to GLB validation"),
    ("package", "WASM distribution"),
)


def required_tasks(lane: str) -> tuple[tuple[str, str], ...]:
    if lane == "release-verify":
        return RELEASE_VERIFY_TASKS
    if lane == "wasm":
        return WASM_TASKS
    if lane not in PLATFORMS:
        raise ValueError(f"unknown candidate lane: {lane}")
    clients = LINUX_X64_CLIENT_TASKS if lane == "linux-x64" else ()
    return (*NATIVE_TASKS, *clients, *NATIVE_PACKAGE_TASKS)


def _validate_required_tasks(lane: str, entries: list[dict[str, Any]]) -> None:
    observed = tuple((entry["kind"], entry["name"]) for entry in entries)
    expected = required_tasks(lane)
    if observed != expected:
        raise ValueError(f"candidate lane task order or identity mismatch: {lane}")
    for entry in entries:
        environment = entry["environment"]
        if environment["CARGO_BUILD_JOBS"] is None or environment["CMAKE_BUILD_PARALLEL_LEVEL"] is None:
            raise ValueError(f"candidate lane omitted governed parallelism: {lane}/{entry['name']}")
        if lane in PLATFORMS and environment["GEOMETER_REQUIRE_NATIVE_TEST_SERVERS"] != "1":
            raise ValueError(f"native candidate lane did not require native test servers: {lane}/{entry['name']}")
        if entry["name"] in {"Python client stratum", "Rust client stratum", "TypeScript host client stratum"}:
            if environment["GEOMETER_TEST_PROFILE"] != "production":
                raise ValueError(f"client candidate task did not use the production test profile: {entry['name']}")
        if entry["name"] == "TypeScript host client stratum" and environment["GEOMETER_TYPESCRIPT_SCOPE"] != "host":
            raise ValueError("TypeScript host candidate task did not use the host scope")
        if entry["name"] == "TypeScript WASM client stratum":
            if environment["GEOMETER_TEST_PROFILE"] != "production" or environment["GEOMETER_TYPESCRIPT_SCOPE"] != "wasm":
                raise ValueError("TypeScript WASM candidate task did not use the production WASM scope")


def canonical_bytes(value: dict[str, Any]) -> bytes:
    return (json.dumps(value, indent=2, sort_keys=True) + "\n").encode("utf-8")


def sha256_file(path: Path) -> str:
    digest = hashlib.sha256()
    with path.open("rb") as stream:
        for chunk in iter(lambda: stream.read(1024 * 1024), b""):
            digest.update(chunk)
    return digest.hexdigest()


def lane_asset_names(tag: str) -> dict[str, tuple[str, ...]]:
    version = release_version(tag)
    lanes: dict[str, tuple[str, ...]] = {"release-verify": (), "wasm": ("wasm-dist.zip",)}
    for platform in PLATFORMS:
        sdk = f"geometer-sdk-{version}-{platform}.zip"
        lanes[platform] = tuple(
            sorted(
                {
                    f"native-{platform}.zip",
                    f"wn_geometer-{version}-py3-none-{WHEEL_SUFFIXES[platform]}",
                    sdk,
                    f"{sdk}.sha256",
                    f"{sdk}.provenance.json",
                }
            )
        )
    return lanes


def _assets(root: Path) -> dict[str, Path]:
    result: dict[str, Path] = {}
    for path in root.rglob("*"):
        if not path.is_file():
            continue
        if path.name in result:
            raise ValueError(f"duplicate candidate artifact basename: {path.name}")
        result[path.name] = path
    return result


def build_candidate_validation(
    artifact_root: Path,
    ledger_root: Path,
    candidate_root: dict[str, Any],
    tag: str,
    log_reference: str,
) -> dict[str, Any]:
    candidate = validate_candidate_root(candidate_root)
    if candidate["release"]["expected_tag"] != tag:
        raise ValueError("candidate validation tag disagrees with candidate root")
    if not log_reference or any(character in log_reference for character in "\r\n\0"):
        raise ValueError("candidate validation log reference must be non-empty single-line text")
    artifacts = _assets(artifact_root)
    expected_products = product_asset_names(tag)
    if set(artifacts) != expected_products:
        raise ValueError("candidate validation artifact set is incomplete or unexpected")

    mapping = lane_asset_names(tag)
    if set().union(*map(set, mapping.values())) != expected_products:
        raise AssertionError("candidate validation lane mapping does not cover the product set")
    lanes: list[dict[str, Any]] = []
    observed_task_identities: set[tuple[str, str, str]] = set()
    for lane in LANES:
        ledger_path = ledger_root / f"ci-ledger-{lane}.json"
        ledger = load_ledger(ledger_path)
        entries = ledger["entries"]
        _validate_required_tasks(lane, entries)
        for entry in entries:
            if entry["lane"] != lane:
                raise ValueError(f"candidate ledger entry is assigned to the wrong lane: {lane}")
            if entry["outcome"] != "success":
                raise ValueError(f"candidate ledger contains a failed command: {lane}/{entry['name']}")
            identity = (lane, entry["kind"], entry["name"])
            if identity in observed_task_identities:
                raise ValueError(f"candidate ledger repeats a task identity: {identity}")
            observed_task_identities.add(identity)
        lane_artifacts = []
        for name in mapping[lane]:
            path = artifacts[name]
            lane_artifacts.append({"name": name, "sha256": sha256_file(path), "size": path.stat().st_size})
        lanes.append(
            {
                "artifacts": lane_artifacts,
                "duration_seconds": round(sum(float(entry["duration_seconds"]) for entry in entries), 6),
                "entries": entries,
                "lane": lane,
                "ledger_sha256": sha256_file(ledger_path),
            }
        )
    return {
        "candidate_root_sha256": candidate_root_sha256(candidate),
        "lanes": lanes,
        "log_reference": log_reference,
        "release_tag": tag,
        "schema": SCHEMA,
        "source_revision": candidate["source"]["revision"],
    }


def validate_candidate_validation(
    value: Any,
    artifact_root: Path,
    candidate_root: dict[str, Any],
    tag: str,
) -> dict[str, Any]:
    if not isinstance(value, dict):
        raise ValueError("candidate validation root must be an object")
    expected_fields = {
        "candidate_root_sha256",
        "lanes",
        "log_reference",
        "release_tag",
        "schema",
        "source_revision",
    }
    if set(value) != expected_fields or value["schema"] != SCHEMA:
        raise ValueError("candidate validation root fields or schema are invalid")
    candidate = validate_candidate_root(candidate_root)
    if value["candidate_root_sha256"] != candidate_root_sha256(candidate):
        raise ValueError("candidate validation root digest mismatch")
    if value["source_revision"] != candidate["source"]["revision"] or value["release_tag"] != tag:
        raise ValueError("candidate validation source or tag mismatch")
    if not isinstance(value["log_reference"], str) or not value["log_reference"]:
        raise ValueError("candidate validation log reference is invalid")
    lanes = value["lanes"]
    if not isinstance(lanes, list) or [lane.get("lane") for lane in lanes if isinstance(lane, dict)] != list(LANES):
        raise ValueError("candidate validation lanes are missing or not canonical")
    artifacts = _assets(artifact_root)
    validation_name = f"geometer-release-validation-{tag}.json"
    artifacts.pop(validation_name, None)
    if set(artifacts) != product_asset_names(tag):
        raise ValueError("candidate validation artifact set is incomplete or unexpected")
    mapping = lane_asset_names(tag)
    task_identities: set[tuple[str, str, str]] = set()
    for lane_record in lanes:
        if not isinstance(lane_record, dict) or set(lane_record) != {
            "artifacts",
            "duration_seconds",
            "entries",
            "lane",
            "ledger_sha256",
        }:
            raise ValueError("candidate validation lane record is malformed")
        lane = lane_record["lane"]
        ledger = validate_ledger({"entries": lane_record["entries"], "schema": "wn.geometer.ci_execution_ledger.a0"})
        _validate_required_tasks(lane, ledger["entries"])
        if any(entry["lane"] != lane or entry["outcome"] != "success" for entry in ledger["entries"]):
            raise ValueError(f"candidate validation lane contains invalid entries: {lane}")
        for entry in ledger["entries"]:
            identity = (lane, entry["kind"], entry["name"])
            if identity in task_identities:
                raise ValueError(f"candidate validation repeats a task identity: {identity}")
            task_identities.add(identity)
        if lane_record["ledger_sha256"] != hashlib.sha256(ledger_canonical_json(ledger)).hexdigest():
            raise ValueError(f"candidate validation ledger digest mismatch: {lane}")
        expected_artifacts = [
            {"name": name, "sha256": sha256_file(artifacts[name]), "size": artifacts[name].stat().st_size}
            for name in mapping[lane]
        ]
        if lane_record["artifacts"] != expected_artifacts:
            raise ValueError(f"candidate validation artifact identity mismatch: {lane}")
        duration = round(sum(float(entry["duration_seconds"]) for entry in ledger["entries"]), 6)
        if lane_record["duration_seconds"] != duration:
            raise ValueError(f"candidate validation duration mismatch: {lane}")
    return value


def validate_candidate_validation_file(
    path: Path,
    artifact_root: Path,
    candidate_root: dict[str, Any],
    tag: str,
) -> dict[str, Any]:
    try:
        raw = path.read_bytes()
        value = json.loads(raw)
    except (OSError, UnicodeDecodeError, json.JSONDecodeError) as error:
        raise ValueError(f"invalid candidate validation record: {path}") from error
    validated = validate_candidate_validation(value, artifact_root, candidate_root, tag)
    if raw != canonical_bytes(validated):
        raise ValueError("candidate validation record is not canonical JSON")
    return validated


def main(argv: Sequence[str] | None = None) -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("artifact_root", type=Path)
    parser.add_argument("ledger_root", type=Path)
    parser.add_argument("--tag", required=True)
    parser.add_argument("--candidate-root", type=Path, required=True)
    parser.add_argument("--log-reference", required=True)
    parser.add_argument("--output", type=Path, required=True)
    args = parser.parse_args(argv)
    value = build_candidate_validation(
        args.artifact_root.resolve(),
        args.ledger_root.resolve(),
        load_candidate_root(args.candidate_root.resolve()),
        args.tag,
        args.log_reference,
    )
    if args.output.exists() or args.output.is_symlink():
        parser.error(f"candidate validation output already exists: {args.output}")
    args.output.parent.mkdir(parents=True, exist_ok=True)
    args.output.write_bytes(canonical_bytes(value))
    validate_candidate_validation_file(
        args.output,
        args.artifact_root.resolve(),
        load_candidate_root(args.candidate_root.resolve()),
        args.tag,
    )
    print(f"validated {len(value['lanes'])} candidate lanes: {args.output}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
