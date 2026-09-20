from __future__ import annotations

import hashlib
import json
from email.message import Message
from pathlib import Path
import urllib.error

import pytest

from candidate_root import candidate_root_sha256, create_candidate_root
from ci_release_metadata import package_version, release_date, release_tag
import fetch_release_candidate
import publish_release_candidate
import publish_release_tag
import r2_store
from validate_release_inventory import expected_asset_names
from verify_release_inventory import verify_release


TEST_VERSION = package_version()
TEST_TAG = release_tag(TEST_VERSION)
TEST_REVISION = "7" * 40


def _release(root: Path) -> str:
    assets = []
    for name in sorted(expected_asset_names(TEST_TAG)):
        payload = name.encode()
        (root / name).write_bytes(payload)
        assets.append(
            {
                "name": name,
                "sha256": hashlib.sha256(payload).hexdigest(),
                "size": len(payload),
            }
        )
    date_value = release_date(TEST_VERSION)
    candidate = create_candidate_root(
        source_revision=TEST_REVISION,
        release_version=TEST_VERSION,
        release_date=date_value,
        abi_generation=int(date_value.replace("-", "")),
        expected_tag=TEST_TAG,
        occt_lock_sha256="5" * 64,
    )
    inventory = {
        "assets": assets,
        "candidate_root": candidate,
        "candidate_root_sha256": candidate_root_sha256(candidate),
        "release_tag": TEST_TAG,
        "release_version": TEST_VERSION,
        "schema": "wn.geometer.release_inventory.b0",
    }
    inventory_path = root / f"geometer-release-inventory-{TEST_TAG}.json"
    inventory_path.write_text(
        json.dumps(inventory, indent=2, sort_keys=True) + "\n",
        encoding="utf-8",
        newline="\n",
    )
    return hashlib.sha256(inventory_path.read_bytes()).hexdigest()


def test_candidate_store_creates_assets_then_inventory(tmp_path: Path) -> None:
    inventory_sha256 = _release(tmp_path)
    observed: list[tuple[str, bytes, str]] = []

    def create(_config: r2_store.R2Config, key: str, body: bytes, content_type: str) -> bool:
        observed.append((key, body, content_type))
        return len(observed) % 2 == 1

    config = r2_store.R2Config("bucket", "https://example.invalid", "key", "secret")
    report = publish_release_candidate.publish_candidate(
        tmp_path,
        TEST_TAG,
        TEST_REVISION,
        config,
        create,
        verify_release,
    )

    prefix = f"releases/candidates/{TEST_REVISION}/{inventory_sha256}"
    assert report["prefix"] == prefix
    assert observed[-1][0] == f"{prefix}/geometer-release-inventory-{TEST_TAG}.json"
    assert len(observed) == len(expected_asset_names(TEST_TAG)) + 1
    assert set(report["created"]).isdisjoint(report["existing"])
    assert set(report["created"]) | set(report["existing"]) == {
        *expected_asset_names(TEST_TAG),
        f"geometer-release-inventory-{TEST_TAG}.json",
    }


def test_create_or_verify_accepts_only_exact_existing_bytes(monkeypatch: pytest.MonkeyPatch) -> None:
    config = r2_store.R2Config("bucket", "https://example.invalid", "key", "secret")
    error = urllib.error.HTTPError("https://example.invalid", 412, "occupied", Message(), None)
    monkeypatch.setattr(r2_store, "put_object_if_absent", lambda *_args: (_ for _ in ()).throw(error))
    monkeypatch.setattr(r2_store, "get_object", lambda *_args: b"same")
    assert not r2_store.create_or_verify(config, "key", b"same", "application/octet-stream")

    monkeypatch.setattr(r2_store, "get_object", lambda *_args: b"different")
    with pytest.raises(RuntimeError, match="occupied by different bytes"):
        r2_store.create_or_verify(config, "key", b"same", "application/octet-stream")


def test_r2_config_uses_one_standard_environment(monkeypatch: pytest.MonkeyPatch) -> None:
    monkeypatch.setenv("R2_BUCKET", "bucket")
    monkeypatch.setenv("R2_ENDPOINT_URL", "https://account.invalid/bucket")
    monkeypatch.setenv("R2_ACCESS_KEY_ID", "key")
    monkeypatch.setenv("R2_SECRET_ACCESS_KEY", "secret")
    config = r2_store.config_from_env()
    assert config == r2_store.R2Config("bucket", "https://account.invalid", "key", "secret")


def test_candidate_store_round_trip_and_tag_alias(tmp_path: Path) -> None:
    source = tmp_path / "source"
    source.mkdir()
    inventory_sha256 = _release(source)
    config = r2_store.R2Config("bucket", "https://example.invalid", "key", "secret")
    objects: dict[str, bytes] = {}

    def create(_config: r2_store.R2Config, key: str, body: bytes, _content_type: str) -> bool:
        previous = objects.setdefault(key, body)
        if previous != body:
            raise RuntimeError("different bytes")
        return previous is body

    def get(_config: r2_store.R2Config, key: str) -> bytes | None:
        return objects.get(key)

    publish_release_candidate.publish_candidate(
        source,
        TEST_TAG,
        TEST_REVISION,
        config,
        create,
        verify_release,
    )
    output = tmp_path / "downloaded"
    inventory = fetch_release_candidate.fetch_candidate(
        output,
        TEST_TAG,
        TEST_REVISION,
        inventory_sha256,
        config,
        get,
    )
    assert inventory["candidate_root"]["source"]["revision"] == TEST_REVISION
    assert {path.name for path in output.iterdir()} == {
        *expected_asset_names(TEST_TAG),
        f"geometer-release-inventory-{TEST_TAG}.json",
    }

    key, created = publish_release_tag.publish_tag_alias(
        TEST_TAG,
        TEST_REVISION,
        inventory_sha256,
        config,
        get=get,
        create=create,
    )
    assert created
    assert key == f"releases/tags/{TEST_TAG}.json"
    alias = json.loads(objects[key])
    assert alias["inventory_sha256"] == inventory_sha256
    assert alias["source_revision"] == TEST_REVISION


def test_candidate_fetch_fails_without_leaving_partial_output(tmp_path: Path) -> None:
    source = tmp_path / "source"
    source.mkdir()
    inventory_sha256 = _release(source)
    inventory_name = f"geometer-release-inventory-{TEST_TAG}.json"
    prefix = f"releases/candidates/{TEST_REVISION}/{inventory_sha256}"
    objects = {f"{prefix}/{inventory_name}": (source / inventory_name).read_bytes()}
    config = r2_store.R2Config("bucket", "https://example.invalid", "key", "secret")

    with pytest.raises(ValueError, match="asset is absent"):
        fetch_release_candidate.fetch_candidate(
            tmp_path / "downloaded",
            TEST_TAG,
            TEST_REVISION,
            inventory_sha256,
            config,
            lambda _config, key: objects.get(key),
        )
    assert not (tmp_path / "downloaded").exists()


def test_publisher_cli_writes_canonical_candidate_reference(
    tmp_path: Path, monkeypatch: pytest.MonkeyPatch
) -> None:
    report = tmp_path / "reference.json"
    monkeypatch.setattr(publish_release_candidate.r2_store, "config_from_env", lambda: object())
    monkeypatch.setattr(
        publish_release_candidate,
        "publish_candidate",
        lambda *_args: {
            "created": ["asset.zip"],
            "existing": [],
            "inventory_sha256": "a" * 64,
            "prefix": f"releases/candidates/{TEST_REVISION}/{'a' * 64}",
        },
    )
    assert (
        publish_release_candidate.main(
            [
                str(tmp_path),
                "--tag",
                TEST_TAG,
                "--source-revision",
                TEST_REVISION,
                "--report",
                str(report),
            ]
        )
        == 0
    )
    value = json.loads(report.read_bytes())
    assert report.read_text(encoding="utf-8") == json.dumps(value, indent=2, sort_keys=True) + "\n"
    assert value["inventory_sha256"] == "a" * 64
    assert value["source_revision"] == TEST_REVISION
