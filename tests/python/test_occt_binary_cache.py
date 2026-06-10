from __future__ import annotations

import importlib.util
import json
import sys
from pathlib import Path
from typing import Any

from pytest import MonkeyPatch


ROOT = Path(__file__).resolve().parents[2]
SPEC = importlib.util.spec_from_file_location("occt_binary_cache", ROOT / "scripts" / "occt_binary_cache.py")
if SPEC is None or SPEC.loader is None:
    raise RuntimeError("Could not load scripts/occt_binary_cache.py")
occt_binary_cache: Any = importlib.util.module_from_spec(SPEC)
sys.modules[SPEC.name] = occt_binary_cache
SPEC.loader.exec_module(occt_binary_cache)


def make_install_tree(root: Path) -> None:
    cmake_dir = root / "lib" / "cmake" / "opencascade"
    cmake_dir.mkdir(parents=True)
    (cmake_dir / "OpenCASCADEConfig.cmake").write_text("# test config\n", encoding="utf-8")
    (root / "lib").mkdir(exist_ok=True)
    (root / "lib" / "libTKTest.a").write_bytes(b"test")


def test_cache_key_includes_platform_and_recipe() -> None:
    profile = occt_binary_cache.OcctCacheProfile(
        kind="native",
        platform_tag="linux-arm64",
        config="Release",
        library_type="Static",
        occt_repo="https://example.invalid/OCCT.git",
        occt_tag="V7_8_1",
        recipe_hash="a" * 64,
    )

    assert profile.cache_key == "occt-native-v7-8-1-linux-arm64-release-static-recipe-aaaaaaaaaaaaaaaa"


def test_package_extract_and_manifest_round_trip(tmp_path: Path) -> None:
    install_dir = tmp_path / "occt-install"
    make_install_tree(install_dir)
    archive_path = tmp_path / "cache" / occt_binary_cache.ARCHIVE_NAME
    profile = occt_binary_cache.OcctCacheProfile(
        kind="native",
        platform_tag="windows-x64",
        config="Release",
        library_type="Static",
        occt_repo="https://example.invalid/OCCT.git",
        occt_tag="V7_8_1",
        recipe_hash="b" * 64,
    )

    occt_binary_cache.package_install_archive(install_dir, archive_path)
    archive_sha = occt_binary_cache.sha256_file(archive_path)
    manifest = occt_binary_cache.build_manifest(profile, archive_path, archive_sha)

    assert manifest["schema"] == occt_binary_cache.SCHEMA
    assert manifest["archive"]["sha256"] == archive_sha
    occt_binary_cache.validate_manifest(json.loads(json.dumps(manifest)), profile)

    restored = tmp_path / "restored-install"
    occt_binary_cache.extract_install_archive(archive_path, restored)

    assert occt_binary_cache.install_ready(restored)
    assert (restored / "lib" / "libTKTest.a").read_bytes() == b"test"


def test_restore_auto_skips_when_cache_is_unconfigured(monkeypatch: MonkeyPatch, tmp_path: Path) -> None:
    for name in (
        "GEOMETER_OCCT_CACHE_BUCKET",
        "R2_BUCKET",
        "GEOMETER_OCCT_CACHE_ENDPOINT_URL",
        "R2_ENDPOINT_URL",
        "GEOMETER_OCCT_CACHE_ACCESS_KEY_ID",
        "R2_ACCESS_KEY_ID",
        "GEOMETER_OCCT_CACHE_SECRET_ACCESS_KEY",
        "R2_SECRET_ACCESS_KEY",
    ):
        monkeypatch.delenv(name, raising=False)
    profile = occt_binary_cache.OcctCacheProfile(
        kind="native",
        platform_tag="linux-x64",
        config="Release",
        library_type="Static",
        occt_repo="https://example.invalid/OCCT.git",
        occt_tag="V7_8_1",
        recipe_hash="c" * 64,
    )

    assert not occt_binary_cache.restore_prebuilt_install(profile, tmp_path / "install", mode="auto")
