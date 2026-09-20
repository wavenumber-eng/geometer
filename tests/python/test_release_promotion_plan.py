from __future__ import annotations

import copy
import hashlib
import json
from pathlib import Path
import subprocess
import sys
from typing import Any

import pytest

from ci_release_metadata import package_version, release_tag
from plan_release_promotion import (
    CHANNEL_INVENTORY_SCHEMA,
    PROMOTION_PLAN_SCHEMA,
    canonical_bytes,
    plan_release_promotion,
)


TEST_VERSION = package_version()
TEST_TAG = release_tag(TEST_VERSION)


def asset(name: str, payload: bytes) -> dict[str, Any]:
    return {"name": name, "sha256": hashlib.sha256(payload).hexdigest(), "size": len(payload)}


def expected_inventory() -> dict[str, Any]:
    return {
        "assets": [asset("native-windows-x64.zip", b"native"), asset("wasm-dist.zip", b"wasm")],
        "release_tag": TEST_TAG,
        "release_version": TEST_VERSION,
        "schema": "wn.geometer.release_inventory.a0",
    }


def observed_inventory(*entries: dict[str, Any]) -> dict[str, Any]:
    return {
        "assets": list(entries),
        "channel": "github",
        "schema": CHANNEL_INVENTORY_SCHEMA,
    }


def test_plan_classifies_exact_existing_assets_and_missing_uploads() -> None:
    expected = expected_inventory()
    first = expected["assets"][0]
    plan = plan_release_promotion(expected, observed_inventory(first))

    assert plan["schema"] == PROMOTION_PLAN_SCHEMA
    assert plan["release_tag"] == TEST_TAG
    assert plan["channel"] == "github"
    assert plan["existing"] == [first]
    assert plan["uploads"] == [expected["assets"][1]]
    assert len(plan["expected_inventory_sha256"]) == 64
    assert len(plan["observed_inventory_sha256"]) == 64


def test_plan_is_deterministic_for_observed_channel_order() -> None:
    expected = expected_inventory()
    entries = expected["assets"]
    forward = plan_release_promotion(expected, observed_inventory(*entries))
    reverse = plan_release_promotion(expected, observed_inventory(*reversed(entries)))
    assert forward == reverse
    assert forward["uploads"] == []


def test_plan_rejects_unexpected_channel_assets() -> None:
    with pytest.raises(ValueError, match="unexpected assets: extra.zip"):
        plan_release_promotion(expected_inventory(), observed_inventory(asset("extra.zip", b"extra")))


@pytest.mark.parametrize(
    ("field", "replacement", "message"),
    [("size", 99, "size mismatch"), ("sha256", "0" * 64, "SHA-256 mismatch")],
)
def test_plan_rejects_occupied_name_with_different_identity(field: str, replacement: object, message: str) -> None:
    expected = expected_inventory()
    changed = copy.deepcopy(expected["assets"][0])
    changed[field] = replacement
    with pytest.raises(ValueError, match=message):
        plan_release_promotion(expected, observed_inventory(changed))


@pytest.mark.parametrize("which", ["expected", "observed"])
def test_plan_rejects_duplicate_asset_names(which: str) -> None:
    expected = expected_inventory()
    duplicate = copy.deepcopy(expected["assets"][0])
    if which == "expected":
        expected["assets"].append(duplicate)
        observed = observed_inventory()
    else:
        observed = observed_inventory(duplicate, duplicate)
    with pytest.raises(ValueError, match="duplicate asset name"):
        plan_release_promotion(expected, observed)


@pytest.mark.parametrize("name", ["../asset.zip", "dir/asset.zip", r"dir\asset.zip", ".env", "asset zip"])
def test_plan_rejects_unsafe_asset_names(name: str) -> None:
    expected = expected_inventory()
    expected["assets"][0]["name"] = name
    with pytest.raises(ValueError, match="unsafe asset name"):
        plan_release_promotion(expected, observed_inventory())


@pytest.mark.parametrize(
    ("field", "replacement", "message"),
    [("size", True, "invalid size"), ("size", -1, "invalid size"), ("sha256", "A" * 64, "invalid SHA-256")],
)
def test_plan_rejects_malformed_asset_identity(field: str, replacement: object, message: str) -> None:
    expected = expected_inventory()
    expected["assets"][0][field] = replacement
    with pytest.raises(ValueError, match=message):
        plan_release_promotion(expected, observed_inventory())


def test_cli_writes_canonical_json_without_channel_access(tmp_path: Path) -> None:
    expected_path = tmp_path / "expected.json"
    observed_path = tmp_path / "observed.json"
    output_path = tmp_path / "nested" / "plan.json"
    expected_path.write_bytes(canonical_bytes(expected_inventory()))
    observed_path.write_bytes(canonical_bytes(observed_inventory()))

    subprocess.run(
        [
            sys.executable,
            "scripts/plan_release_promotion.py",
            str(expected_path),
            str(observed_path),
            "--output",
            str(output_path),
        ],
        check=True,
        capture_output=True,
        text=True,
    )
    raw = output_path.read_bytes()
    value = json.loads(raw)
    assert raw == canonical_bytes(value)
    assert [entry["name"] for entry in value["uploads"]] == [
        "native-windows-x64.zip",
        "wasm-dist.zip",
    ]


def test_cli_rejects_noncanonical_input(tmp_path: Path) -> None:
    expected_path = tmp_path / "expected.json"
    observed_path = tmp_path / "observed.json"
    expected_path.write_text(json.dumps(expected_inventory()), encoding="utf-8")
    observed_path.write_bytes(canonical_bytes(observed_inventory()))

    result = subprocess.run(
        [
            sys.executable,
            "scripts/plan_release_promotion.py",
            str(expected_path),
            str(observed_path),
            "--output",
            str(tmp_path / "plan.json"),
        ],
        capture_output=True,
        text=True,
    )
    assert result.returncode != 0
    assert "not canonical JSON" in result.stderr
