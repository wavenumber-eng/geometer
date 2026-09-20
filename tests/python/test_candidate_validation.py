from __future__ import annotations

import copy
import hashlib
import json
from pathlib import Path

import pytest

from candidate_root import create_candidate_root
from candidate_validation import (
    LANES,
    build_candidate_validation,
    canonical_bytes,
    required_tasks,
    validate_candidate_validation,
    validate_candidate_validation_file,
)
from ci_execution_ledger import canonical_json as ledger_bytes
from ci_release_metadata import package_version, release_date, release_tag
from release_asset_names import product_asset_names


TEST_VERSION = package_version()
TEST_TAG = release_tag(TEST_VERSION)


def _candidate() -> dict[str, object]:
    date_value = release_date(TEST_VERSION)
    return create_candidate_root(
        source_revision="7" * 40,
        release_version=TEST_VERSION,
        release_date=date_value,
        abi_generation=int(date_value.replace("-", "")),
        expected_tag=TEST_TAG,
        occt_lock_sha256="5" * 64,
    )


def _fixture(root: Path) -> tuple[Path, Path]:
    artifacts = root / "artifacts"
    ledgers = root / "ledgers"
    artifacts.mkdir()
    ledgers.mkdir()
    for name in product_asset_names(TEST_TAG):
        (artifacts / name).write_bytes(name.encode())
    for lane in LANES:
        environment = {
            "CARGO_BUILD_JOBS": "1",
            "CMAKE_BUILD_PARALLEL_LEVEL": "2",
            "GEOMETER_REQUIRE_NATIVE_TEST_SERVERS": "1" if lane in {"windows-x64", "linux-x64", "linux-arm64", "macos-arm64"} else None,
            "GEOMETER_TEST_PROFILE": None,
            "GEOMETER_TYPESCRIPT_SCOPE": None,
        }
        entries = []
        for kind, name in required_tasks(lane):
            task_environment = dict(environment)
            if name in {"Python client stratum", "Rust client stratum", "TypeScript host client stratum"}:
                task_environment["GEOMETER_TEST_PROFILE"] = "production"
            if name == "TypeScript host client stratum":
                task_environment["GEOMETER_TYPESCRIPT_SCOPE"] = "host"
            if name == "TypeScript WASM client stratum":
                task_environment["GEOMETER_TEST_PROFILE"] = "production"
                task_environment["GEOMETER_TYPESCRIPT_SCOPE"] = "wasm"
            entries.append(
                {
                    "command": ["candidate-tool", lane, name],
                    "duration_seconds": 1.25,
                    "environment": task_environment,
                    "exit_code": 0,
                    "kind": kind,
                    "lane": lane,
                    "name": name,
                    "outcome": "success",
                }
            )
        ledger = {
            "entries": entries,
            "schema": "wn.geometer.ci_execution_ledger.a0",
        }
        (ledgers / f"ci-ledger-{lane}.json").write_bytes(ledger_bytes(ledger))
    return artifacts, ledgers


def test_candidate_validation_binds_every_lane_and_product(tmp_path: Path) -> None:
    artifacts, ledgers = _fixture(tmp_path)
    candidate = _candidate()
    value = build_candidate_validation(
        artifacts,
        ledgers,
        candidate,
        TEST_TAG,
        "https://github.example/actions/runs/1",
    )
    output = artifacts / f"geometer-release-validation-{TEST_TAG}.json"
    output.write_bytes(canonical_bytes(value))

    assert [lane["lane"] for lane in value["lanes"]] == list(LANES)
    assert sum(len(lane["artifacts"]) for lane in value["lanes"]) == len(product_asset_names(TEST_TAG))
    assert validate_candidate_validation_file(output, artifacts, candidate, TEST_TAG) == value


def test_candidate_validation_rejects_artifact_drift(tmp_path: Path) -> None:
    artifacts, ledgers = _fixture(tmp_path)
    candidate = _candidate()
    value = build_candidate_validation(artifacts, ledgers, candidate, TEST_TAG, "local:test")
    (artifacts / "wasm-dist.zip").write_bytes(b"changed")

    with pytest.raises(ValueError, match="artifact identity mismatch"):
        validate_candidate_validation(value, artifacts, candidate, TEST_TAG)


def test_candidate_validation_rejects_duplicate_task_identity(tmp_path: Path) -> None:
    artifacts, ledgers = _fixture(tmp_path)
    candidate = _candidate()
    value = build_candidate_validation(artifacts, ledgers, candidate, TEST_TAG, "local:test")
    changed = copy.deepcopy(value)
    changed["lanes"][0]["entries"].append(copy.deepcopy(changed["lanes"][0]["entries"][0]))
    embedded = {
        "entries": changed["lanes"][0]["entries"],
        "schema": "wn.geometer.ci_execution_ledger.a0",
    }
    changed["lanes"][0]["ledger_sha256"] = hashlib.sha256(ledger_bytes(embedded)).hexdigest()
    changed["lanes"][0]["duration_seconds"] += 1.25

    with pytest.raises(ValueError, match="task order or identity mismatch"):
        validate_candidate_validation(changed, artifacts, candidate, TEST_TAG)


def test_candidate_validation_requires_canonical_file(tmp_path: Path) -> None:
    artifacts, ledgers = _fixture(tmp_path)
    candidate = _candidate()
    value = build_candidate_validation(artifacts, ledgers, candidate, TEST_TAG, "local:test")
    output = artifacts / f"geometer-release-validation-{TEST_TAG}.json"
    output.write_text(json.dumps(value), encoding="utf-8")

    with pytest.raises(ValueError, match="not canonical"):
        validate_candidate_validation_file(output, artifacts, candidate, TEST_TAG)
