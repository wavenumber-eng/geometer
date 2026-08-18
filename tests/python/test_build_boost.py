from __future__ import annotations

import importlib.util
import json
import sys
from pathlib import Path
from typing import Any

import pytest
from pytest import MonkeyPatch


ROOT = Path(__file__).resolve().parents[2]
SCRIPTS = ROOT / "scripts"
sys.path.insert(0, str(SCRIPTS))
SPEC = importlib.util.spec_from_file_location("build_boost", SCRIPTS / "build_boost.py")
if SPEC is None or SPEC.loader is None:
    raise RuntimeError("Could not load scripts/build_boost.py")
build_boost: Any = importlib.util.module_from_spec(SPEC)
SPEC.loader.exec_module(build_boost)


def configure_paths(monkeypatch: MonkeyPatch, tmp_path: Path) -> tuple[Path, Path]:
    boost_dir = tmp_path / "boost_1_92_0"
    sentinel = boost_dir / ".geometer-source.json"
    archive = tmp_path / "boost_1_92_0.tar.gz"
    monkeypatch.setattr(build_boost, "BOOST_DIR", boost_dir)
    monkeypatch.setattr(build_boost, "SENTINEL_PATH", sentinel)
    monkeypatch.setattr(build_boost, "ARCHIVE_PATH", archive)
    return sentinel, archive


def write_ready_tree(sentinel: Path) -> Path:
    version_header = sentinel.parent / "boost" / "version.hpp"
    version_header.parent.mkdir(parents=True, exist_ok=True)
    version_header.write_text(
        '#define BOOST_VERSION 109200\n#define BOOST_LIB_VERSION "1_92"\n',
        encoding="utf-8",
    )
    sentinel.write_text(json.dumps(build_boost._expected_sentinel()), encoding="utf-8")
    return version_header


def test_ready_tree_requires_exact_sentinel_and_version(
    monkeypatch: MonkeyPatch, tmp_path: Path
) -> None:
    sentinel, _ = configure_paths(monkeypatch, tmp_path)
    version_header = write_ready_tree(sentinel)
    assert build_boost._is_ready()

    version_header.write_text(
        '#define BOOST_VERSION 109100\n#define BOOST_LIB_VERSION "1_91"\n',
        encoding="utf-8",
    )
    assert not build_boost._is_ready()

    write_ready_tree(sentinel)
    sentinel.write_text('{}\n', encoding="utf-8")
    assert not build_boost._is_ready()


def test_verify_rejects_missing_archive(monkeypatch: MonkeyPatch, tmp_path: Path) -> None:
    sentinel, archive = configure_paths(monkeypatch, tmp_path)
    write_ready_tree(sentinel)

    with pytest.raises(RuntimeError, match="archive cache is missing or invalid"):
        build_boost.verify()

    archive.write_bytes(b"tampered archive")
    with pytest.raises(RuntimeError, match="archive cache is missing or invalid"):
        build_boost.verify()
