from __future__ import annotations

import importlib.util
import json
import subprocess
import sys
from pathlib import Path
from typing import Any

import pytest
from pytest import MonkeyPatch


ROOT = Path(__file__).resolve().parents[2]
SCRIPTS = ROOT / "scripts"
if str(SCRIPTS) not in sys.path:
    sys.path.insert(0, str(SCRIPTS))
SPEC = importlib.util.spec_from_file_location("occt_binary_cache", ROOT / "scripts" / "occt_binary_cache.py")
if SPEC is None or SPEC.loader is None:
    raise RuntimeError("Could not load scripts/occt_binary_cache.py")
occt_binary_cache: Any = importlib.util.module_from_spec(SPEC)
sys.modules[SPEC.name] = occt_binary_cache
SPEC.loader.exec_module(occt_binary_cache)

BUILD_OCCT_SPEC = importlib.util.spec_from_file_location("build_occt_cache_test", SCRIPTS / "build_occt.py")
if BUILD_OCCT_SPEC is None or BUILD_OCCT_SPEC.loader is None:
    raise RuntimeError("Could not load scripts/build_occt.py")
build_occt: Any = importlib.util.module_from_spec(BUILD_OCCT_SPEC)
BUILD_OCCT_SPEC.loader.exec_module(build_occt)

BUILD_WASM_SPEC = importlib.util.spec_from_file_location("build_wasm_cache_test", SCRIPTS / "build_wasm.py")
if BUILD_WASM_SPEC is None or BUILD_WASM_SPEC.loader is None:
    raise RuntimeError("Could not load scripts/build_wasm.py")
build_wasm: Any = importlib.util.module_from_spec(BUILD_WASM_SPEC)
BUILD_WASM_SPEC.loader.exec_module(build_wasm)

COMPARE_SPEC = importlib.util.spec_from_file_location(
    "compare_occt_qualification", ROOT / "scripts" / "compare_occt_qualification.py"
)
if COMPARE_SPEC is None or COMPARE_SPEC.loader is None:
    raise RuntimeError("Could not load scripts/compare_occt_qualification.py")
compare_occt_qualification: Any = importlib.util.module_from_spec(COMPARE_SPEC)
sys.modules[COMPARE_SPEC.name] = compare_occt_qualification
COMPARE_SPEC.loader.exec_module(compare_occt_qualification)


def make_install_tree(root: Path, version: str = "7.8.1") -> None:
    cmake_dir = root / "lib" / "cmake" / "opencascade"
    cmake_dir.mkdir(parents=True)
    (cmake_dir / "OpenCASCADEConfig.cmake").write_text("# test config\n", encoding="utf-8")
    (cmake_dir / "OpenCASCADEConfigVersion.cmake").write_text(f'set(PACKAGE_VERSION "{version}")\n', encoding="utf-8")
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


def test_cache_key_includes_platform_recipe_and_abi() -> None:
    profile = occt_binary_cache.OcctCacheProfile(
        kind="native",
        platform_tag="linux-arm64",
        config="Release",
        library_type="Static",
        occt_repo="https://example.invalid/OCCT.git",
        occt_tag="V7_8_1",
        recipe_hash="a" * 64,
        toolchain_abi="gnu",
    )

    assert profile.cache_key == ("occt-native-v7-8-1-linux-arm64-release-static-abi-gnu-recipe-aaaaaaaaaaaaaaaa")


def test_semantic_recipe_is_invariant_to_definition_order_and_local_paths() -> None:
    definition = occt_binary_cache.CMakeDefinition
    first = (
        definition("CMAKE_INSTALL_PREFIX", "C:/first/occt-install", include_in_recipe=False),
        definition("CMAKE_BUILD_TYPE", "Release"),
        definition("3RDPARTY_RAPIDJSON_DIR", "C:/first/rapidjson", recipe_value="vendored-rapidjson"),
    )
    second = (
        definition("3RDPARTY_RAPIDJSON_DIR", "D:/other/rapidjson", recipe_value="vendored-rapidjson"),
        definition("CMAKE_BUILD_TYPE", "Release"),
        definition("CMAKE_INSTALL_PREFIX", "D:/other/occt-install", include_in_recipe=False),
    )

    assert occt_binary_cache.semantic_recipe_hash("test-a0", first, {"occt_tag": "V8_0_1"}) == (
        occt_binary_cache.semantic_recipe_hash("test-a0", second, {"occt_tag": "V8_0_1"})
    )
    assert occt_binary_cache.cmake_definition_args(first) == [
        "-DCMAKE_INSTALL_PREFIX=C:/first/occt-install",
        "-DCMAKE_BUILD_TYPE=Release",
        "-D3RDPARTY_RAPIDJSON_DIR=C:/first/rapidjson",
    ]


def test_semantic_recipe_invalidates_on_byte_relevant_configure_change() -> None:
    definition = occt_binary_cache.CMakeDefinition
    disabled = (definition("BUILD_MODULE_Draw", "OFF"),)
    enabled = (definition("BUILD_MODULE_Draw", "ON"),)

    assert occt_binary_cache.semantic_recipe_hash("test-a0", disabled, {}) != (
        occt_binary_cache.semantic_recipe_hash("test-a0", enabled, {})
    )


def test_directory_content_hash_invalidates_on_vendored_byte_change(tmp_path: Path) -> None:
    source = tmp_path / "rapidjson"
    (source / "include").mkdir(parents=True)
    header = source / "include" / "document.h"
    header.write_bytes(b"first bytes\n")
    first = occt_binary_cache.directory_content_hash(source)

    header.write_bytes(b"second bytes\n")
    second = occt_binary_cache.directory_content_hash(source)

    assert first != second


@pytest.mark.parametrize("attribute", ["OCCT_WASM_INSTALL_RULE_ORIGINAL", "OCCT_WASM_INSTALL_RULE_PATCHED"])
def test_wasm_recipe_hashes_exact_install_patch_before_and_after(
    monkeypatch: MonkeyPatch,
    attribute: str,
) -> None:
    baseline = build_wasm.occt_wasm_cache_profile().recipe_hash
    monkeypatch.setattr(build_wasm, attribute, getattr(build_wasm, attribute) + " ")

    assert build_wasm.occt_wasm_cache_profile().recipe_hash != baseline


@pytest.mark.parametrize(
    ("version_output", "expected"),
    [
        ("g++ (Ubuntu 13.2.0-4ubuntu3) 13.2.0\n", "gcc-13-libstdcxx-abi-default"),
        ("g++ (Ubuntu 14.1.1) 14.1.1\n", "gcc-14-libstdcxx-abi-default"),
        ("Ubuntu clang version 18.1.3\n", "clang-18-libstdcxx-abi-default"),
    ],
)
def test_nonwindows_compiler_family_and_major_rotate_cache_abi(
    monkeypatch: MonkeyPatch,
    version_output: str,
    expected: str,
) -> None:
    monkeypatch.setenv("CXX", "/toolchain/c++")
    monkeypatch.delenv("CXXFLAGS", raising=False)
    monkeypatch.setattr(build_occt.subprocess, "check_output", lambda *_args, **_kwargs: version_output)

    assert build_occt.nonwindows_toolchain_abi("linux-x64") == expected


def test_nonwindows_compiler_patch_version_and_parallelism_do_not_rotate_cache_abi(
    monkeypatch: MonkeyPatch,
) -> None:
    output = {"value": "Ubuntu clang version 18.1.3\n"}
    monkeypatch.setenv("CXX", "/toolchain/clang++")
    monkeypatch.setenv("CMAKE_BUILD_PARALLEL_LEVEL", "1")
    monkeypatch.setattr(build_occt.subprocess, "check_output", lambda *_args, **_kwargs: output["value"])
    first = build_occt.nonwindows_toolchain_abi("linux-x64")

    output["value"] = "Ubuntu clang version 18.1.8\n"
    monkeypatch.setenv("CMAKE_BUILD_PARALLEL_LEVEL", "64")

    assert build_occt.nonwindows_toolchain_abi("linux-x64") == first


def test_nonwindows_compiler_runtime_and_abi_flag_rotate_cache_abi(monkeypatch: MonkeyPatch) -> None:
    monkeypatch.setenv("CXX", "/toolchain/clang++")
    monkeypatch.setattr(
        build_occt.subprocess,
        "check_output",
        lambda *_args, **_kwargs: "Ubuntu clang version 18.1.3\n",
    )
    default = build_occt.nonwindows_toolchain_abi("linux-x64")
    monkeypatch.setenv("CXXFLAGS", "-stdlib=libc++ -D_LIBCPP_ABI_VERSION=2")

    assert build_occt.nonwindows_toolchain_abi("linux-x64") != default
    assert build_occt.nonwindows_toolchain_abi("linux-x64") == "clang-18-libcxx-abi-2"


def test_cmake_definition_names_must_be_unique() -> None:
    definition = occt_binary_cache.CMakeDefinition
    duplicates = (
        definition("USE_TBB", "OFF"),
        definition("USE_TBB", "ON"),
    )

    with pytest.raises(ValueError, match="unique"):
        occt_binary_cache.semantic_recipe_hash("test-a0", duplicates, {})


def test_accepted_v801_native_cache_alias_is_exactly_pinned() -> None:
    profile = occt_binary_cache.OcctCacheProfile(
        kind="native",
        platform_tag="windows-x64",
        config="Release",
        library_type="Static",
        occt_repo="https://github.com/Open-Cascade-SAS/OCCT.git",
        occt_tag="V8_0_1",
        recipe_hash="afddffde43bb0288a99269de68220cc973eb3212551045a0f779e964a912231b",
        toolchain_abi="msvc-v143",
    )

    assert occt_binary_cache.accepted_cache_aliases(profile) == (
        occt_binary_cache.AcceptedCacheAlias(
            kind="native",
            platform_tag="windows-x64",
            config="Release",
            library_type="Static",
            occt_repo="https://github.com/Open-Cascade-SAS/OCCT.git",
            occt_tag="V8_0_1",
            recipe_hash="02d3ac07fe672579f1d5d97249964248a36e4b3982b3195c4fe7bd0d1dece46d",
            archive_sha256="255ad723184c62ef4e6dc82c20c1acd5b0aa43407cbefc6e26e484eb74a05df9",
            compatible_recipe_hashes=("afddffde43bb0288a99269de68220cc973eb3212551045a0f779e964a912231b",),
            requested_toolchain_abi="msvc-v143",
        ),
    )

    changed_recipe = occt_binary_cache.dataclasses.replace(profile, recipe_hash="f" * 64)
    changed_abi = occt_binary_cache.dataclasses.replace(profile, toolchain_abi="mingw")
    assert occt_binary_cache.accepted_cache_aliases(changed_recipe) == ()
    assert occt_binary_cache.accepted_cache_aliases(changed_abi) == ()


def test_accepted_v801_wasm_cache_alias_targets_canonical_recipe() -> None:
    profile = occt_binary_cache.OcctCacheProfile(
        kind="wasm",
        platform_tag="wasm-emscripten",
        config="Release",
        library_type="Static",
        occt_repo="https://github.com/Open-Cascade-SAS/OCCT.git",
        occt_tag="V8_0_1",
        recipe_hash="c48157a47af466d1793739f4022457f0f53f88f5c63ddc1ea86cf7149fe9542b",
        emsdk_version="3.1.56",
    )

    aliases = occt_binary_cache.accepted_cache_aliases(profile)

    assert len(aliases) == 1
    assert aliases[0].recipe_hash == "a15818c33b508d24f66702e3834be2d25fce89031a00a58f6391c0d702bb95f4"
    assert aliases[0].archive_sha256 == "44fe6d6294c7a26ac77cfa17e1fd4a312578638a5669b7032c227750d032614e"


@pytest.mark.parametrize(
    ("current_profile", "predecessor_hash"),
    [
        (
            occt_binary_cache.OcctCacheProfile(
                kind="native",
                platform_tag="windows-x64",
                config="Release",
                library_type="Static",
                occt_repo="https://github.com/Open-Cascade-SAS/OCCT.git",
                occt_tag="V8_0_1",
                recipe_hash="afddffde43bb0288a99269de68220cc973eb3212551045a0f779e964a912231b",
                toolchain_abi="msvc-v143",
            ),
            "45aee960e6bad68c5f26c67335b82b6cbd539922aaefcf96003f20d7193485d9",
        ),
        (
            occt_binary_cache.OcctCacheProfile(
                kind="wasm",
                platform_tag="wasm-emscripten",
                config="Release",
                library_type="Static",
                occt_repo="https://github.com/Open-Cascade-SAS/OCCT.git",
                occt_tag="V8_0_1",
                recipe_hash="c48157a47af466d1793739f4022457f0f53f88f5c63ddc1ea86cf7149fe9542b",
                emsdk_version="3.1.56",
            ),
            "be4fe9173cdb53703d60a169b0833ac2c9141ec480046ba2d040a9e02f610c89",
        ),
        (
            occt_binary_cache.OcctCacheProfile(
                kind="wasm",
                platform_tag="wasm-emscripten",
                config="Release",
                library_type="Static",
                occt_repo="https://github.com/Open-Cascade-SAS/OCCT.git",
                occt_tag="V8_0_1",
                recipe_hash="c48157a47af466d1793739f4022457f0f53f88f5c63ddc1ea86cf7149fe9542b",
                emsdk_version="3.1.56",
            ),
            "d71a27bcf279d9cabdd3d9f012867a0d419b0ffcfeee7036d5ed6fdc269e70fc",
        ),
    ],
)
def test_exact_local_predecessor_migration_is_nondestructive_and_idempotent(
    monkeypatch: MonkeyPatch,
    tmp_path: Path,
    current_profile: Any,
    predecessor_hash: str,
) -> None:
    install_dir = tmp_path / "occt-install"
    make_install_tree(install_dir, "8.0.1")
    retained_build_output = install_dir / "lib" / "retained-build-output.a"
    retained_build_output.write_bytes(b"must survive marker migration")
    predecessor = occt_binary_cache.dataclasses.replace(current_profile, recipe_hash=predecessor_hash)
    occt_binary_cache.write_install_profile(install_dir, predecessor)

    assert occt_binary_cache.install_matches_or_migrates_profile(install_dir, current_profile)
    assert occt_binary_cache.install_matches_profile(install_dir, current_profile)
    assert retained_build_output.read_bytes() == b"must survive marker migration"

    monkeypatch.setattr(
        occt_binary_cache,
        "write_install_profile",
        lambda *_args, **_kwargs: pytest.fail("an exact current marker must not be rewritten"),
    )
    assert occt_binary_cache.install_matches_or_migrates_profile(install_dir, current_profile)


def test_local_predecessor_migration_requires_every_nonrecipe_field_and_version(tmp_path: Path) -> None:
    install_dir = tmp_path / "occt-install"
    make_install_tree(install_dir, "8.0.1")
    current = occt_binary_cache.OcctCacheProfile(
        kind="native",
        platform_tag="windows-x64",
        config="Release",
        library_type="Static",
        occt_repo="https://github.com/Open-Cascade-SAS/OCCT.git",
        occt_tag="V8_0_1",
        recipe_hash="afddffde43bb0288a99269de68220cc973eb3212551045a0f779e964a912231b",
        toolchain_abi="msvc-v143",
    )
    predecessor = occt_binary_cache.dataclasses.replace(
        current,
        recipe_hash="45aee960e6bad68c5f26c67335b82b6cbd539922aaefcf96003f20d7193485d9",
        toolchain_abi="mingw",
    )
    occt_binary_cache.write_install_profile(install_dir, predecessor)

    assert not occt_binary_cache.install_matches_or_migrates_profile(install_dir, current)
    assert not occt_binary_cache.install_matches_profile(install_dir, current)

    version_file = install_dir / "lib" / "cmake" / "opencascade" / "OpenCASCADEConfigVersion.cmake"
    version_file.write_text('set(PACKAGE_VERSION "8.0.0")\n', encoding="utf-8")
    exact_predecessor = occt_binary_cache.dataclasses.replace(predecessor, toolchain_abi=current.toolchain_abi)
    occt_binary_cache.write_install_profile(install_dir, exact_predecessor)
    assert not occt_binary_cache.install_matches_or_migrates_profile(install_dir, current)


def test_local_recipe_migration_is_one_way(tmp_path: Path) -> None:
    install_dir = tmp_path / "occt-install"
    make_install_tree(install_dir, "8.0.1")
    current = occt_binary_cache.OcctCacheProfile(
        kind="native",
        platform_tag="windows-x64",
        config="Release",
        library_type="Static",
        occt_repo="https://github.com/Open-Cascade-SAS/OCCT.git",
        occt_tag="V8_0_1",
        recipe_hash="afddffde43bb0288a99269de68220cc973eb3212551045a0f779e964a912231b",
        toolchain_abi="msvc-v143",
    )
    predecessor = occt_binary_cache.dataclasses.replace(
        current,
        recipe_hash="45aee960e6bad68c5f26c67335b82b6cbd539922aaefcf96003f20d7193485d9",
    )
    occt_binary_cache.write_install_profile(install_dir, current)

    assert not occt_binary_cache.install_matches_or_migrates_profile(install_dir, predecessor)
    assert occt_binary_cache.install_matches_profile(install_dir, current)


def test_restore_uses_only_an_exact_pinned_compatible_alias(
    monkeypatch: MonkeyPatch,
    tmp_path: Path,
) -> None:
    clear_cache_env(monkeypatch)
    install_dir = tmp_path / "occt-install"
    make_install_tree(install_dir, "8.0.1")
    archive_path = tmp_path / occt_binary_cache.ARCHIVE_NAME
    occt_binary_cache.package_install_archive(install_dir, archive_path)
    archive_sha = occt_binary_cache.sha256_file(archive_path)
    current_profile = occt_binary_cache.OcctCacheProfile(
        kind="native",
        platform_tag="windows-x64",
        config="Release",
        library_type="Static",
        occt_repo="https://github.com/Open-Cascade-SAS/OCCT.git",
        occt_tag="V8_0_1",
        recipe_hash="c" * 64,
    )
    accepted_alias = occt_binary_cache.AcceptedCacheAlias(
        kind=current_profile.kind,
        platform_tag=current_profile.platform_tag,
        config=current_profile.config,
        library_type=current_profile.library_type,
        occt_repo=current_profile.occt_repo,
        occt_tag=current_profile.occt_tag,
        recipe_hash="a" * 64,
        archive_sha256=archive_sha,
        compatible_recipe_hashes=(current_profile.recipe_hash,),
    )
    accepted_profile = occt_binary_cache.dataclasses.replace(current_profile, recipe_hash=accepted_alias.recipe_hash)
    manifest = occt_binary_cache.build_manifest(accepted_profile, archive_path, archive_sha)
    public_config = occt_binary_cache.PublicCacheConfig(
        base_url="https://artifacts.example.invalid",
        prefix="deps/v1/geometer/occt",
    )
    accepted_prefix = occt_binary_cache.object_prefix(public_config, accepted_profile)
    requested_keys: list[str] = []

    def fake_public_get(_config: Any, key: str) -> bytes | None:
        requested_keys.append(key)
        if key == f"{accepted_prefix}/{occt_binary_cache.MANIFEST_NAME}":
            return json.dumps(manifest).encode("utf-8")
        if key == f"{accepted_prefix}/{occt_binary_cache.ARCHIVE_NAME}":
            return archive_path.read_bytes()
        return None

    monkeypatch.setattr(occt_binary_cache, "ACCEPTED_CACHE_ALIASES", (accepted_alias,))
    monkeypatch.setattr(occt_binary_cache, "public_config_from_env", lambda: public_config)
    monkeypatch.setattr(occt_binary_cache, "config_from_env", lambda: None)
    monkeypatch.setattr(occt_binary_cache, "_public_get_object", fake_public_get)

    restored = tmp_path / "restored-install"
    assert occt_binary_cache.restore_prebuilt_install(current_profile, restored, mode="only")
    assert occt_binary_cache.installed_occt_version(restored) == "8.0.1"
    assert occt_binary_cache.install_matches_profile(restored, current_profile)
    assert f"{accepted_prefix}/{occt_binary_cache.MANIFEST_NAME}" in requested_keys


def test_current_candidate_preserves_an_accepted_pinned_sha(
    monkeypatch: MonkeyPatch,
    tmp_path: Path,
) -> None:
    clear_cache_env(monkeypatch)
    install_dir = tmp_path / "occt-install"
    make_install_tree(install_dir, "8.0.1")
    archive_path = tmp_path / occt_binary_cache.ARCHIVE_NAME
    occt_binary_cache.package_install_archive(install_dir, archive_path)
    archive_sha = occt_binary_cache.sha256_file(archive_path)
    profile = occt_binary_cache.OcctCacheProfile(
        kind="native",
        platform_tag="windows-x64",
        config="Release",
        library_type="Static",
        occt_repo="https://github.com/Open-Cascade-SAS/OCCT.git",
        occt_tag="V8_0_1",
        recipe_hash="c" * 64,
        toolchain_abi="msvc-v143",
    )
    alias = occt_binary_cache.AcceptedCacheAlias(
        kind=profile.kind,
        platform_tag=profile.platform_tag,
        config=profile.config,
        library_type=profile.library_type,
        occt_repo=profile.occt_repo,
        occt_tag=profile.occt_tag,
        recipe_hash=profile.recipe_hash,
        archive_sha256="0" * 64,
        compatible_recipe_hashes=(profile.recipe_hash,),
        requested_toolchain_abi=profile.toolchain_abi,
        source_toolchain_abi=profile.toolchain_abi,
    )
    manifest = occt_binary_cache.build_manifest(profile, archive_path, archive_sha)
    public_config = occt_binary_cache.PublicCacheConfig(
        base_url="https://artifacts.example.invalid",
        prefix="deps/v1/geometer/occt",
    )
    prefix = occt_binary_cache.object_prefix(public_config, profile)

    def fake_public_get(_config: Any, key: str) -> bytes | None:
        if key == f"{prefix}/{occt_binary_cache.MANIFEST_NAME}":
            return json.dumps(manifest).encode("utf-8")
        if key == f"{prefix}/{occt_binary_cache.ARCHIVE_NAME}":
            return archive_path.read_bytes()
        return None

    monkeypatch.setattr(occt_binary_cache, "ACCEPTED_CACHE_ALIASES", (alias,))
    monkeypatch.setattr(occt_binary_cache, "public_config_from_env", lambda: public_config)
    monkeypatch.setattr(occt_binary_cache, "config_from_env", lambda: None)
    monkeypatch.setattr(occt_binary_cache, "_public_get_object", fake_public_get)

    with pytest.raises(RuntimeError, match="Accepted OCCT binary cache archive checksum mismatch"):
        occt_binary_cache.restore_prebuilt_install(profile, tmp_path / "restored", mode="only")


def test_exact_tag_qualification_uses_isolated_native_and_wasm_profiles() -> None:
    state_root = ".deps/occt-qualification/V8_0_1"
    existed_before = (ROOT / state_root).exists()
    native = subprocess.run(
        [
            sys.executable,
            "scripts/build_occt.py",
            "--platform-tag",
            "windows-x64",
            "--occt-tag",
            "V8_0_1",
            "--occt-state-root",
            state_root,
            "--print-binary-cache-key",
        ],
        cwd=ROOT,
        check=True,
        capture_output=True,
        text=True,
    ).stdout.strip()
    wasm = subprocess.run(
        [
            sys.executable,
            "scripts/build_wasm.py",
            "--occt-tag",
            "V8_0_1",
            "--occt-state-root",
            state_root,
            "--print-occt-binary-cache-key",
        ],
        cwd=ROOT,
        check=True,
        capture_output=True,
        text=True,
    ).stdout.strip()

    assert native == (
        "occt-native-v8-0-1-windows-x64-release-static-abi-msvc-v143-"
        "recipe-afddffde43bb0288"
    )
    assert wasm == (
        "occt-wasm-v8-0-1-wasm-emscripten-release-static-emsdk-3.1.56-"
        "recipe-c48157a47af466d1"
    )
    assert (ROOT / state_root).exists() == existed_before


def test_occt_recipe_keys_use_structured_semantic_definitions() -> None:
    native_source = (ROOT / "scripts" / "build_occt.py").read_text(encoding="utf-8")
    wasm_source = (ROOT / "scripts" / "build_wasm.py").read_text(encoding="utf-8")

    native_recipe = native_source[native_source.index("def occt_cache_profile(") :]
    native_recipe = native_recipe[: native_recipe.index("def linux_glibc_baseline(")]
    wasm_recipe = wasm_source[wasm_source.index("def occt_wasm_cache_profile(") :]
    wasm_recipe = wasm_recipe[: wasm_recipe.index("def build_occt_wasm(")]
    for recipe in (native_recipe, wasm_recipe):
        assert "Path(__file__)" not in recipe
        assert "build_parallel_jobs" not in recipe
        assert "semantic_recipe_hash" in recipe
    assert '"native-install-a2"' in native_recipe
    assert '"wasm-install-a2"' in wasm_recipe

    native_configure = native_source[native_source.index("def configure_occt(") :]
    native_configure = native_configure[: native_configure.index("def build_occt(")]
    wasm_configure = wasm_source[wasm_source.index("def build_occt_wasm(") :]
    wasm_configure = wasm_configure[: wasm_configure.index("def build_geometer_wasm(")]
    for configure in (native_configure, wasm_configure):
        assert "cmake_definition_args" in configure
        assert '"-D' not in configure


def test_windows_cached_occt_abi_is_guarded_at_configure_time() -> None:
    source = (ROOT / "CMakeLists.txt").read_text(encoding="utf-8")

    assert 'EXISTS "${OpenCASCADE_INSTALL_PREFIX}/win64/vc14/lib" AND NOT MSVC' in source
    assert "The restored Windows OCCT install uses the MSVC ABI" in source


def test_qualification_rejects_master_and_state_outside_generated_root() -> None:
    for arguments in (
        ["--occt-tag", "master", "--occt-state-root", ".deps/occt-qualification/master"],
        ["--occt-tag", "V8_0_1", "--occt-state-root", "out/occt-qualification/V8_0_1"],
    ):
        completed = subprocess.run(
            [sys.executable, "scripts/build_occt.py", *arguments, "--print-binary-cache-key"],
            cwd=ROOT,
            check=False,
            capture_output=True,
            text=True,
        )
        assert completed.returncode != 0


def test_qualification_dist_root_is_isolated_from_committed_artifacts() -> None:
    source = (ROOT / "CMakeLists.txt").read_text(encoding="utf-8")

    assert 'set(GEOMETER_DIST_ROOT "${CMAKE_SOURCE_DIR}/dist"' in source
    assert '"${GEOMETER_DIST_ROOT}/native/${GEOMETER_NATIVE_DIST_PLATFORM}"' in source
    assert '"${GEOMETER_DIST_ROOT}/wasm/browser"' in source


def test_qualification_comparison_normalizes_only_runtime_fields(tmp_path: Path) -> None:
    first_projection = tmp_path / "first.json"
    second_projection = tmp_path / "second.json"
    first_projection.write_text(
        '{"schema":"geometry.projection.b0","views":[{"id":"top"}],"timings":{"hlr_ms":1}}',
        encoding="utf-8",
    )
    second_projection.write_text(
        '{"timings":{"hlr_ms":99},"views":[{"id":"top"}],"schema":"geometry.projection.b0"}',
        encoding="utf-8",
    )
    assert compare_occt_qualification.normalized_projection(
        first_projection
    ) == compare_occt_qualification.normalized_projection(second_projection)
    second_projection.write_text(
        '{"schema":"geometry.projection.b0","views":[{"id":"front"}],"timings":{"hlr_ms":1}}',
        encoding="utf-8",
    )
    assert compare_occt_qualification.normalized_projection(
        first_projection
    ) != compare_occt_qualification.normalized_projection(second_projection)

    first_step = tmp_path / "first.step"
    second_step = tmp_path / "second.step"
    first_step.write_text(
        "FILE_NAME('Open CASCADE Shape Model','2026-08-14T01:02:03',('Author'));\n#1=POINT();\n",
        encoding="utf-8",
    )
    second_step.write_text(
        "FILE_NAME('Open CASCADE Shape Model','2026-08-15T04:05:06',('Author'));\n#1=POINT();\n",
        encoding="utf-8",
    )
    assert compare_occt_qualification.normalized_step(first_step) == compare_occt_qualification.normalized_step(
        second_step
    )
    second_step.write_text(
        "FILE_NAME('Open CASCADE Shape Model','2026-08-15T04:05:06',('Author'));\n#1=LINE();\n",
        encoding="utf-8",
    )
    assert compare_occt_qualification.normalized_step(first_step) != compare_occt_qualification.normalized_step(
        second_step
    )


def test_consumer_validators_accept_isolated_qualification_artifacts() -> None:
    validate_native = (ROOT / "scripts/validate_native.py").read_text(encoding="utf-8")
    assert 'parser.add_argument("--dist-root"' in validate_native
    assert 'parser.add_argument("--occt-dir"' in validate_native
    assert 'parser.add_argument("--validation-out"' in validate_native

    for path in (
        "tests/wasm/operation_contract_validation.js",
        "tests/wasm/step_to_glb_bytes_validation.js",
        "tests/wasm/planar_batch_solve_bytes_validation.js",
        "tests/typescript/wasm_client_validation.mjs",
        "tests/typescript/worker_client_validation.mjs",
        "tests/typescript/geometer_worker_entry.mjs",
    ):
        assert "GEOMETER_WASM_BROWSER_DIST" in (ROOT / path).read_text(encoding="utf-8")
    browser_html = (ROOT / "tests/wasm/browser_hlr_validation.html").read_text(encoding="utf-8")
    assert 'queryParam("browserDist", "/dist/wasm/browser")' in browser_html


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

    assert not occt_binary_cache.install_matches_profile(install_dir, profile_7)
    occt_binary_cache.write_install_profile(install_dir, profile_7)
    assert occt_binary_cache.install_matches_profile(install_dir, profile_7)
    assert not occt_binary_cache.install_matches_profile(install_dir, profile_8)
    assert not occt_binary_cache.install_matches_profile(
        install_dir,
        occt_binary_cache.dataclasses.replace(profile_7, toolchain_abi="different-abi"),
    )
    assert not occt_binary_cache.install_matches_profile(
        install_dir,
        occt_binary_cache.dataclasses.replace(profile_7, recipe_hash="c" * 64),
    )


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
