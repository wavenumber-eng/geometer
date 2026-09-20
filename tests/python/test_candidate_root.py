from __future__ import annotations

import copy
import hashlib
import json
import os
from pathlib import Path
import subprocess
import sys
from typing import Any, Callable

import pytest


ROOT = Path(__file__).resolve().parents[2]
sys.path.insert(0, str(ROOT / "scripts"))
import candidate_root  # noqa: E402


def _sha(byte: bytes) -> str:
    return hashlib.sha256(byte).hexdigest()


def _value() -> dict[str, Any]:
    return {
        "occt_lock_sha256": "5" * 64,
        "release": {
            "abi_generation": 20260920,
            "date": "2026-09-20",
            "expected_tag": "v2026-09-20",
            "version": "2026.9.20",
        },
        "schema": candidate_root.SCHEMA,
        "source": {"revision": "7" * 40},
    }


def test_candidate_root_is_strict_canonical_and_deterministic(tmp_path: Path) -> None:
    value = _value()
    path = tmp_path / "candidate-root.json"
    candidate_root.write_candidate_root(path, value)

    assert candidate_root.load_candidate_root(path) == value
    assert path.read_bytes() == candidate_root.canonical_json(value)
    assert candidate_root.candidate_root_sha256(value) == _sha(path.read_bytes())
    assert candidate_root.canonical_json(dict(reversed(list(value.items())))) == path.read_bytes()


@pytest.mark.parametrize(
    ("mutation", "match"),
    [
        (lambda value: value.update(extra=True), "fields differ"),
        (lambda value: value.pop("occt_lock_sha256"), "missing=.*occt_lock_sha256"),
        (lambda value: value["source"].update(extra=True), "source fields differ"),
        (lambda value: value["source"].update(revision="A" * 40), "lowercase 40-character"),
        (lambda value: value["source"].update(revision="0" * 40), "40-character Git revision"),
        (lambda value: value.update(occt_lock_sha256="f" * 63), "occt_lock_sha256"),
    ],
)
def test_candidate_root_rejects_malformed_or_open_shapes(
    mutation: Callable[[dict[str, Any]], None], match: str
) -> None:
    value = copy.deepcopy(_value())
    mutation(value)
    with pytest.raises(candidate_root.CandidateRootError, match=match):
        candidate_root.validate_candidate_root(value)


@pytest.mark.parametrize(
    ("field", "replacement", "match"),
    [
        ("version", "2026.02.20", "canonical"),
        ("version", "2026.2.30", "invalid calendar"),
        ("date", "2026-09-21", "must be 2026-09-20"),
        ("abi_generation", 20260921, "must be 20260920"),
        ("abi_generation", True, "must be 20260920"),
        ("expected_tag", "v2026-09-21", "must be v2026-09-20"),
    ],
)
def test_candidate_root_rejects_inconsistent_release_identity(field: str, replacement: Any, match: str) -> None:
    value = _value()
    value["release"][field] = replacement
    with pytest.raises(candidate_root.CandidateRootError, match=match):
        candidate_root.validate_candidate_root(value)


def test_candidate_root_supports_consistent_release_serial() -> None:
    value = _value()
    value["release"].update(version="2026.9.20.2", expected_tag="v2026-09-20-2")
    candidate_root.validate_candidate_root(value)


def test_create_candidate_root_contains_only_build_inputs() -> None:
    value = _value()
    created = candidate_root.create_candidate_root(
        source_revision=value["source"]["revision"],
        release_version=value["release"]["version"],
        release_date=value["release"]["date"],
        abi_generation=value["release"]["abi_generation"],
        expected_tag=value["release"]["expected_tag"],
        occt_lock_sha256=value["occt_lock_sha256"],
    )
    assert created == value


def test_checkout_candidate_root_uses_exact_commit_and_lock() -> None:
    created = candidate_root.create_candidate_root_from_checkout()
    revision = subprocess.run(
        ["git", "rev-parse", "--verify", "HEAD"],
        cwd=ROOT,
        check=True,
        capture_output=True,
        text=True,
    ).stdout.strip()
    assert created["source"] == {"revision": revision}
    assert created["occt_lock_sha256"] == candidate_root.file_sha256(ROOT / "dependencies" / "occt-lock.json")
    assert created["release"]["expected_tag"] == candidate_root.release_tag(candidate_root.package_version())


def test_load_rejects_valid_but_noncanonical_json(tmp_path: Path) -> None:
    path = tmp_path / "candidate-root.json"
    path.write_text(json.dumps(_value()), encoding="utf-8")
    with pytest.raises(candidate_root.CandidateRootError, match="not canonical"):
        candidate_root.load_candidate_root(path)


def test_write_refuses_to_replace_existing_root(tmp_path: Path) -> None:
    path = tmp_path / "candidate-root.json"
    candidate_root.write_candidate_root(path, _value())
    with pytest.raises(candidate_root.CandidateRootError, match="refusing to overwrite"):
        candidate_root.write_candidate_root(path, _value())


def test_write_does_not_replace_a_path_created_during_commit(tmp_path: Path, monkeypatch: pytest.MonkeyPatch) -> None:
    path = tmp_path / "candidate-root.json"
    original_link = os.link

    def create_racing_path(source: os.PathLike[str], destination: os.PathLike[str]) -> None:
        Path(destination).write_bytes(b"other writer")
        original_link(source, destination)

    monkeypatch.setattr(candidate_root.os, "link", create_racing_path)
    with pytest.raises(candidate_root.CandidateRootError, match="refusing to overwrite"):
        candidate_root.write_candidate_root(path, _value())
    assert path.read_bytes() == b"other writer"
    assert not list(tmp_path.glob(".candidate-root.json.*.tmp"))


def test_cli_hashes_occt_lock_and_validates(tmp_path: Path, capsys: pytest.CaptureFixture[str]) -> None:
    occt_lock = tmp_path / "occt-lock.json"
    occt_lock.write_bytes(b"occt lock")
    output = tmp_path / "candidate-root.json"

    result = candidate_root.main(
        [
            "create",
            str(output),
            "--source-revision",
            "7" * 40,
            "--release-version",
            "2026.9.20",
            "--release-date",
            "2026-09-20",
            "--abi-generation",
            "20260920",
            "--expected-tag",
            "v2026-09-20",
            "--occt-lock-file",
            str(occt_lock),
        ]
    )
    value = candidate_root.load_candidate_root(output)
    assert result == 0
    assert value["occt_lock_sha256"] == _sha(b"occt lock")
    assert "candidate root valid" in capsys.readouterr().out
    assert candidate_root.main(["validate", str(output)]) == 0
