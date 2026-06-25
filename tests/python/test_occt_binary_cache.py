from __future__ import annotations

import importlib.util
import json
import sys
from pathlib import Path
from typing import Any

import pytest
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
    (cmake_dir / "OpenCASCADEConfigVersion.cmake").write_text('set(PACKAGE_VERSION "7.8.1")\n', encoding="utf-8")
    (root / "lib").mkdir(exist_ok=True)
    (root / "lib" / "libTKTest.a").write_bytes(b"test")


def clear_cache_env(monkeypatch: MonkeyPatch) -> None:
    for name in (
        "GEOMETER_OCCT_BINARY",
        "GEOMETER_OCCT_BINARY_CACHE",
        "GEOMETER_OCCT_PUBLIC_CACHE",
        "GEOMETER_OCCT_CACHE_PUBLIC",
        "GEOMETER_OCCT_CACHE_PUBLIC_BASE_URL",
        "GEOMETER_OCCT_PUBLIC_BASE_URL",
        "WN_ARTIFACTS_BASE_URL",
        "GEOMETER_OCCT_CACHE_PREFIX",
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


def test_public_config_defaults_to_artifacts_domain(monkeypatch: MonkeyPatch) -> None:
    clear_cache_env(monkeypatch)

    config = occt_binary_cache.public_config_from_env()

    assert config == occt_binary_cache.PublicCacheConfig(
        base_url="https://artifacts.wavenumber.net",
        prefix="deps/v1/geometer/occt",
    )
    assert occt_binary_cache._public_object_url(config, "deps/v1/geometer/occt/key/manifest.json") == (
        "https://artifacts.wavenumber.net/deps/v1/geometer/occt/key/manifest.json"
    )


def test_public_config_can_be_disabled(monkeypatch: MonkeyPatch) -> None:
    clear_cache_env(monkeypatch)
    monkeypatch.setenv("GEOMETER_OCCT_PUBLIC_CACHE", "off")

    assert occt_binary_cache.public_config_from_env() is None


def test_signed_r2_config_strips_bucket_path_from_endpoint(monkeypatch: MonkeyPatch) -> None:
    clear_cache_env(monkeypatch)
    monkeypatch.setenv("R2_BUCKET", "wn-build-deps")
    monkeypatch.setenv("R2_ENDPOINT_URL", "https://example.r2.cloudflarestorage.com/wn-build-deps/")
    monkeypatch.setenv("R2_ACCESS_KEY_ID", "key")
    monkeypatch.setenv("R2_SECRET_ACCESS_KEY", "secret")

    config = occt_binary_cache.config_from_env()

    assert config is not None
    assert config.endpoint_url == "https://example.r2.cloudflarestorage.com"


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
    assert manifest["project"] == "geometer"
    assert manifest["dependency"]["name"] == "occt"
    assert manifest["dependency"]["version"] == "V7_8_1"
    assert manifest["archive"]["sha256"] == archive_sha
    occt_binary_cache.validate_manifest(json.loads(json.dumps(manifest)), profile)

    restored = tmp_path / "restored-install"
    occt_binary_cache.extract_install_archive(archive_path, restored)

    assert occt_binary_cache.install_ready(restored)
    assert (restored / "lib" / "libTKTest.a").read_bytes() == b"test"


def test_install_matches_profile_requires_occt_version(tmp_path: Path) -> None:
    install_dir = tmp_path / "occt-install"
    make_install_tree(install_dir)
    profile_7 = occt_binary_cache.OcctCacheProfile(
        kind="native",
        platform_tag="windows-x64",
        config="Release",
        library_type="Static",
        occt_repo="https://example.invalid/OCCT.git",
        occt_tag="V7_8_1",
        recipe_hash="b" * 64,
    )
    profile_8 = occt_binary_cache.OcctCacheProfile(
        kind="native",
        platform_tag="windows-x64",
        config="Release",
        library_type="Static",
        occt_repo="https://example.invalid/OCCT.git",
        occt_tag="V8_0_0",
        recipe_hash="b" * 64,
    )

    assert occt_binary_cache.install_matches_profile(install_dir, profile_7)
    assert not occt_binary_cache.install_matches_profile(install_dir, profile_8)


def test_object_prefix_uses_wavenumber_layout_and_legacy_fallback() -> None:
    config = occt_binary_cache.CacheConfig(
        bucket="cache",
        endpoint_url="https://example.invalid",
        access_key_id="id",
        secret_access_key="secret",
        region="auto",
        prefix="deps/v1/geometer/occt",
    )
    profile = occt_binary_cache.OcctCacheProfile(
        kind="wasm",
        platform_tag="wasm-emscripten",
        config="Release",
        library_type="Static",
        occt_repo="https://example.invalid/OCCT.git",
        occt_tag="V8_0_0",
        recipe_hash="d" * 64,
        emsdk_version="3.1.56",
    )

    assert occt_binary_cache.object_prefix(config, profile) == (
        "deps/v1/geometer/occt/V8_0_0/wasm/wasm-emscripten/"
        "occt-wasm-v8-0-0-wasm-emscripten-release-static-emsdk-3.1.56-recipe-dddddddddddddddd"
    )
    assert occt_binary_cache.object_prefix_candidates(config, profile)[-1] == (
        "geometer/occt/wasm/wasm-emscripten/"
        "occt-wasm-v8-0-0-wasm-emscripten-release-static-emsdk-3.1.56-recipe-dddddddddddddddd"
    )


def test_restore_auto_falls_back_when_public_cache_misses_without_r2(
    monkeypatch: MonkeyPatch,
    tmp_path: Path,
) -> None:
    clear_cache_env(monkeypatch)
    monkeypatch.setattr(occt_binary_cache, "_public_get_object", lambda _config, _key: None)
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


def test_restore_prefers_public_cache_before_r2(monkeypatch: MonkeyPatch, tmp_path: Path) -> None:
    clear_cache_env(monkeypatch)
    install_dir = tmp_path / "occt-install"
    make_install_tree(install_dir)
    archive_path = tmp_path / occt_binary_cache.ARCHIVE_NAME
    profile = occt_binary_cache.OcctCacheProfile(
        kind="native",
        platform_tag="windows-x64",
        config="Release",
        library_type="Static",
        occt_repo="https://example.invalid/OCCT.git",
        occt_tag="V7_8_1",
        recipe_hash="e" * 64,
    )
    occt_binary_cache.package_install_archive(install_dir, archive_path)
    archive_sha = occt_binary_cache.sha256_file(archive_path)
    manifest = occt_binary_cache.build_manifest(profile, archive_path, archive_sha)
    public_config = occt_binary_cache.PublicCacheConfig(
        base_url="https://artifacts.example.invalid",
        prefix="deps/v1/geometer/occt",
    )
    public_prefix = occt_binary_cache.object_prefix(public_config, profile)
    requested_keys: list[str] = []

    def fake_public_get(_config: Any, key: str) -> bytes | None:
        requested_keys.append(key)
        if key == f"{public_prefix}/{occt_binary_cache.MANIFEST_NAME}":
            return json.dumps(manifest).encode("utf-8")
        if key == f"{public_prefix}/{occt_binary_cache.ARCHIVE_NAME}":
            return archive_path.read_bytes()
        return None

    monkeypatch.setattr(occt_binary_cache, "public_config_from_env", lambda: public_config)
    monkeypatch.setattr(
        occt_binary_cache,
        "config_from_env",
        lambda: occt_binary_cache.CacheConfig(
            bucket="wn-build-deps",
            endpoint_url="https://example.invalid",
            access_key_id="key",
            secret_access_key="secret",
            region="auto",
            prefix="deps/v1/geometer/occt",
        ),
    )
    monkeypatch.setattr(occt_binary_cache, "_public_get_object", fake_public_get)
    monkeypatch.setattr(
        occt_binary_cache,
        "_r2_get_object",
        lambda _config, _key: pytest.fail("signed R2 should not be used when public cache hits"),
    )

    restored = tmp_path / "restored-install"
    assert occt_binary_cache.restore_prebuilt_install(profile, restored, mode="only")
    assert occt_binary_cache.install_ready(restored)
    assert requested_keys == [
        f"{public_prefix}/{occt_binary_cache.MANIFEST_NAME}",
        f"{public_prefix}/{occt_binary_cache.ARCHIVE_NAME}",
    ]


def test_restore_auto_falls_back_when_cache_read_fails(monkeypatch: MonkeyPatch, tmp_path: Path) -> None:
    clear_cache_env(monkeypatch)
    monkeypatch.setattr(occt_binary_cache, "public_config_from_env", lambda: None)
    monkeypatch.setattr(
        occt_binary_cache,
        "config_from_env",
        lambda: occt_binary_cache.CacheConfig(
            bucket="wn-build-deps",
            endpoint_url="https://example.invalid",
            access_key_id="key",
            secret_access_key="secret",
            region="auto",
            prefix="deps/v1/geometer/occt",
        ),
    )
    monkeypatch.setattr(
        occt_binary_cache,
        "_r2_get_object",
        lambda _config, _key: (_ for _ in ()).throw(occt_binary_cache.CacheReadError("HTTP 500")),
    )
    profile = occt_binary_cache.OcctCacheProfile(
        kind="native",
        platform_tag="windows-x64",
        config="Release",
        library_type="Static",
        occt_repo="https://example.invalid/OCCT.git",
        occt_tag="V8_0_0",
        recipe_hash="c" * 64,
    )

    assert not occt_binary_cache.restore_prebuilt_install(profile, tmp_path / "install", mode="auto")


def test_restore_only_fails_when_cache_read_fails(monkeypatch: MonkeyPatch, tmp_path: Path) -> None:
    clear_cache_env(monkeypatch)
    monkeypatch.setattr(occt_binary_cache, "public_config_from_env", lambda: None)
    monkeypatch.setattr(
        occt_binary_cache,
        "config_from_env",
        lambda: occt_binary_cache.CacheConfig(
            bucket="wn-build-deps",
            endpoint_url="https://example.invalid",
            access_key_id="key",
            secret_access_key="secret",
            region="auto",
            prefix="deps/v1/geometer/occt",
        ),
    )
    monkeypatch.setattr(
        occt_binary_cache,
        "_r2_get_object",
        lambda _config, _key: (_ for _ in ()).throw(occt_binary_cache.CacheReadError("HTTP 500")),
    )
    profile = occt_binary_cache.OcctCacheProfile(
        kind="native",
        platform_tag="windows-x64",
        config="Release",
        library_type="Static",
        occt_repo="https://example.invalid/OCCT.git",
        occt_tag="V8_0_0",
        recipe_hash="c" * 64,
    )

    with pytest.raises(RuntimeError, match="OCCT binary cache read failed"):
        occt_binary_cache.restore_prebuilt_install(profile, tmp_path / "install", mode="only")
