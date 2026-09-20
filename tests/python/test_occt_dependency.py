from __future__ import annotations

import dataclasses
from email.message import Message
import json
import subprocess
import sys
import urllib.error
import zipfile
from pathlib import Path
from typing import Any

import pytest
from pytest import MonkeyPatch


ROOT = Path(__file__).resolve().parents[2]
SCRIPTS = ROOT / "scripts"
if str(SCRIPTS) not in sys.path:
    sys.path.insert(0, str(SCRIPTS))

import build_occt  # noqa: E402
import build_wasm  # noqa: E402
import compare_occt_qualification  # noqa: E402
import occt_lock  # noqa: E402
import occt_producer  # noqa: E402
import publish_occt_candidates  # noqa: E402


def _make_install_tree(root: Path, version: str = "8.0.1") -> None:
    cmake_dir = root / "lib" / "cmake" / "opencascade"
    cmake_dir.mkdir(parents=True)
    (cmake_dir / "OpenCASCADEConfig.cmake").write_text("# test config\n", encoding="utf-8")
    (cmake_dir / "OpenCASCADEConfigVersion.cmake").write_text(
        f'set(PACKAGE_VERSION "{version}")\n', encoding="utf-8"
    )
    (root / "lib" / "libTKTest.a").write_bytes(b"test")


def _profile() -> occt_producer.OcctBuildProfile:
    source = occt_lock.load_lock()["dependency"]["source"]
    return occt_producer.OcctBuildProfile(
        kind="native",
        platform_tag="windows-x64",
        config="Release",
        library_type="Static",
        occt_repo=source["repository"],
        occt_tag=source["tag"],
        source_commit=source["commit"],
        source_tag_object=source["tag_object"],
        recipe_hash="a" * 64,
        toolchain_abi="msvc-v143",
    )


def _set_github_identity(monkeypatch: MonkeyPatch) -> None:
    monkeypatch.setenv("GITHUB_REPOSITORY", "wavenumber-eng/geometer")
    monkeypatch.setenv("GITHUB_RUN_ID", "12345")
    monkeypatch.setenv("GITHUB_SHA", "f" * 40)


def test_producer_recipe_is_path_independent_but_tracks_byte_inputs() -> None:
    definition = occt_producer.CMakeDefinition
    first = (
        definition("CMAKE_INSTALL_PREFIX", "C:/first/install", include_in_recipe=False),
        definition("CMAKE_BUILD_TYPE", "Release"),
        definition("3RDPARTY_RAPIDJSON_DIR", "C:/first/rapidjson", recipe_value="vendored-rapidjson"),
    )
    second = (
        definition("3RDPARTY_RAPIDJSON_DIR", "D:/other/rapidjson", recipe_value="vendored-rapidjson"),
        definition("CMAKE_BUILD_TYPE", "Release"),
        definition("CMAKE_INSTALL_PREFIX", "D:/other/install", include_in_recipe=False),
    )

    assert occt_producer.semantic_recipe_hash("test-a0", first, {"tag": "V8_0_1"}) == (
        occt_producer.semantic_recipe_hash("test-a0", second, {"tag": "V8_0_1"})
    )
    assert occt_producer.semantic_recipe_hash("test-a0", first, {"tag": "V8_0_1"}) != (
        occt_producer.semantic_recipe_hash("test-a0", first, {"tag": "V8_0_2"})
    )
    with pytest.raises(ValueError, match="unique"):
        occt_producer.semantic_recipe_hash(
            "test-a0",
            (definition("USE_TBB", "OFF"), definition("USE_TBB", "ON")),
            {},
        )


def test_directory_content_hash_tracks_vendored_bytes(tmp_path: Path) -> None:
    source = tmp_path / "rapidjson"
    source.mkdir()
    header = source / "document.h"
    header.write_bytes(b"first\n")
    first = occt_producer.directory_content_hash(source)
    header.write_bytes(b"second\n")
    assert occt_producer.directory_content_hash(source) != first


def test_source_identity_resolves_annotated_tag_and_peeled_commit(tmp_path: Path) -> None:
    checkout = tmp_path / "occt"
    checkout.mkdir()
    subprocess.run(["git", "init", "-q"], cwd=checkout, check=True)
    subprocess.run(["git", "config", "user.name", "Geometer Test"], cwd=checkout, check=True)
    subprocess.run(["git", "config", "user.email", "test@example.invalid"], cwd=checkout, check=True)
    (checkout / "CMakeLists.txt").write_text("# test\n", encoding="utf-8")
    subprocess.run(["git", "add", "CMakeLists.txt"], cwd=checkout, check=True)
    subprocess.run(["git", "commit", "-qm", "fixture"], cwd=checkout, check=True)
    subprocess.run(["git", "tag", "-am", "fixture tag", "V1_0_0"], cwd=checkout, check=True)

    tag_object, commit = occt_producer.source_identity(checkout, "V1_0_0")

    assert tag_object == subprocess.check_output(
        ["git", "rev-parse", "refs/tags/V1_0_0"], cwd=checkout, text=True
    ).strip()
    assert commit == subprocess.check_output(["git", "rev-parse", "HEAD"], cwd=checkout, text=True).strip()


@pytest.mark.parametrize("attribute", ["OCCT_WASM_INSTALL_RULE_ORIGINAL", "OCCT_WASM_INSTALL_RULE_PATCHED"])
def test_wasm_producer_evidence_tracks_install_patch(monkeypatch: MonkeyPatch, attribute: str) -> None:
    source = occt_lock.load_lock()["dependency"]["source"]
    arguments = (source["tag_object"], source["commit"])
    baseline = build_wasm.occt_wasm_build_profile(*arguments).recipe_hash
    monkeypatch.setattr(build_wasm, attribute, getattr(build_wasm, attribute) + " ")
    assert build_wasm.occt_wasm_build_profile(*arguments).recipe_hash != baseline


@pytest.mark.parametrize(
    ("version_output", "expected"),
    [
        ("g++ (Ubuntu 13.2.0) 13.2.0\n", "gcc-13-libstdcxx-abi-default"),
        ("Ubuntu clang version 18.1.3\n", "clang-18-libstdcxx-abi-default"),
    ],
)
def test_producer_records_compiler_family_and_major(
    monkeypatch: MonkeyPatch, version_output: str, expected: str
) -> None:
    monkeypatch.setenv("CXX", "/toolchain/c++")
    monkeypatch.delenv("CXXFLAGS", raising=False)
    monkeypatch.setattr(build_occt.subprocess, "check_output", lambda *_args, **_kwargs: version_output)
    assert build_occt.nonwindows_toolchain_abi("linux-x64") == expected


def test_publish_candidate_uses_content_addressed_conditional_create(
    monkeypatch: MonkeyPatch, tmp_path: Path
) -> None:
    install = tmp_path / "install"
    _set_github_identity(monkeypatch)
    _make_install_tree(install)
    profile = _profile()
    occt_producer.write_install_profile(install, profile)
    config = occt_producer.CacheConfig("bucket", "https://example.invalid", "key", "secret", "auto")
    uploads: list[tuple[str, bytes, str]] = []
    monkeypatch.setattr(occt_producer, "config_from_env", lambda: config)
    monkeypatch.setattr(occt_producer, "_r2_get_object", lambda _config, _key: None)
    monkeypatch.setattr(
        occt_producer,
        "_r2_put_object_if_absent",
        lambda _config, key, body, content_type: uploads.append((key, body, content_type)),
    )

    result = occt_producer.upload_prebuilt_install(
        profile,
        install,
        out_dir=tmp_path / "out",
        locked_profile_id="windows-x64-msvc-v143-md-static",
    )

    manifest = json.loads((result / occt_producer.MANIFEST_NAME).read_text(encoding="utf-8"))
    assert manifest["schema"] == occt_producer.CANDIDATE_SCHEMA
    assert manifest["archive"]["sha256"] in manifest["archive"]["object_key"].split("/")
    assert uploads == [
        (
            manifest["archive"]["object_key"],
            (result / occt_producer.ARCHIVE_NAME).read_bytes(),
            "application/zip",
        )
    ]


def test_secret_free_package_handoff_is_closed_and_tamper_evident(
    monkeypatch: MonkeyPatch, tmp_path: Path
) -> None:
    install = tmp_path / "install"
    _set_github_identity(monkeypatch)
    _make_install_tree(install)
    profile = _profile()
    occt_producer.write_install_profile(install, profile)
    monkeypatch.setattr(
        occt_producer,
        "config_from_env",
        lambda: pytest.fail("packaging must not load publication credentials"),
    )
    package = occt_producer.package_prebuilt_install(
        profile,
        install,
        out_dir=tmp_path / "handoff",
        locked_profile_id="windows-x64-msvc-v143-md-static",
    )
    assert publish_occt_candidates.candidate_directories(tmp_path / "handoff", package.name) == [package]

    archive = package / occt_producer.ARCHIVE_NAME
    archive.write_bytes(archive.read_bytes() + b"tamper")
    with pytest.raises(RuntimeError, match="does not match its bytes"):
        occt_producer.publish_candidate(package)


def test_concurrent_identical_candidate_publication_is_idempotent(
    monkeypatch: MonkeyPatch, tmp_path: Path
) -> None:
    install = tmp_path / "install"
    _set_github_identity(monkeypatch)
    _make_install_tree(install)
    profile = _profile()
    occt_producer.write_install_profile(install, profile)
    package = occt_producer.package_prebuilt_install(
        profile,
        install,
        out_dir=tmp_path / "handoff",
        locked_profile_id="windows-x64-msvc-v143-md-static",
    )
    archive_bytes = (package / occt_producer.ARCHIVE_NAME).read_bytes()
    monkeypatch.setattr(
        occt_producer,
        "config_from_env",
        lambda: occt_producer.CacheConfig("bucket", "https://example.invalid", "key", "secret", "auto"),
    )
    monkeypatch.setattr(
        occt_producer,
        "_r2_put_object_if_absent",
        lambda *_args: (_ for _ in ()).throw(
            urllib.error.HTTPError("url", 412, "exists", Message(), None)
        ),
    )
    monkeypatch.setattr(occt_producer, "_r2_get_object", lambda *_args: archive_bytes)

    occt_producer.publish_candidate(package)


def test_publisher_rejects_forged_run_identity_and_checksum(
    monkeypatch: MonkeyPatch, tmp_path: Path
) -> None:
    _set_github_identity(monkeypatch)
    install = tmp_path / "install"
    _make_install_tree(install)
    profile = _profile()
    occt_producer.write_install_profile(install, profile)
    package = occt_producer.package_prebuilt_install(
        profile,
        install,
        out_dir=tmp_path / "handoff",
        locked_profile_id="windows-x64-msvc-v143-md-static",
    )
    manifest_path = package / occt_producer.MANIFEST_NAME
    manifest = json.loads(manifest_path.read_text(encoding="utf-8"))
    manifest["producer"]["github_sha"] = "0" * 40
    manifest_path.write_bytes((json.dumps(manifest, indent=2, sort_keys=True) + "\n").encode())
    with pytest.raises(RuntimeError, match="publishing run"):
        occt_producer.publish_candidate(package)

    manifest["producer"]["github_sha"] = "f" * 40
    manifest_path.write_bytes((json.dumps(manifest, indent=2, sort_keys=True) + "\n").encode())
    (package / occt_producer.SHA256_NAME).write_bytes(b"forged\n")
    with pytest.raises(RuntimeError, match="checksum evidence"):
        occt_producer.publish_candidate(package)


def test_publisher_rejects_rehashed_unsafe_archive(
    monkeypatch: MonkeyPatch, tmp_path: Path
) -> None:
    _set_github_identity(monkeypatch)
    install = tmp_path / "install"
    _make_install_tree(install)
    profile = _profile()
    occt_producer.write_install_profile(install, profile)
    package = occt_producer.package_prebuilt_install(
        profile,
        install,
        out_dir=tmp_path / "handoff",
        locked_profile_id="windows-x64-msvc-v143-md-static",
    )
    archive_path = package / occt_producer.ARCHIVE_NAME
    with zipfile.ZipFile(archive_path, "a") as archive:
        archive.writestr("../escape", b"unsafe")
    archive_sha = occt_producer.sha256_file(archive_path)
    manifest_path = package / occt_producer.MANIFEST_NAME
    manifest = json.loads(manifest_path.read_text(encoding="utf-8"))
    manifest["archive"] = {
        "name": occt_producer.ARCHIVE_NAME,
        "object_key": (
            f"dependencies/occt/{package.name}/{archive_sha}/{occt_producer.ARCHIVE_NAME}"
        ),
        "sha256": archive_sha,
        "size": archive_path.stat().st_size,
    }
    manifest_path.write_bytes((json.dumps(manifest, indent=2, sort_keys=True) + "\n").encode())
    (package / occt_producer.SHA256_NAME).write_bytes(
        f"{archive_sha}  {occt_producer.ARCHIVE_NAME}\n".encode("ascii")
    )
    monkeypatch.setattr(
        occt_producer,
        "config_from_env",
        lambda: pytest.fail("unsafe archive must fail before credential access"),
    )

    with pytest.raises(occt_lock.LockError, match="unsafe path"):
        occt_producer.publish_candidate(package)


def test_publisher_bounds_archive_before_reading_it(monkeypatch: MonkeyPatch, tmp_path: Path) -> None:
    _set_github_identity(monkeypatch)
    install = tmp_path / "install"
    _make_install_tree(install)
    profile = _profile()
    occt_producer.write_install_profile(install, profile)
    package = occt_producer.package_prebuilt_install(
        profile,
        install,
        out_dir=tmp_path / "handoff",
        locked_profile_id="windows-x64-msvc-v143-md-static",
    )
    lock = occt_lock.load_lock()
    monkeypatch.setattr(occt_lock, "MAX_ARCHIVE_BYTES", 1)
    monkeypatch.setattr(occt_lock, "load_lock", lambda: lock)

    with pytest.raises(RuntimeError, match="size limit"):
        occt_producer.publish_candidate(package)


def test_producer_profile_requires_exact_version(tmp_path: Path) -> None:
    install = tmp_path / "install"
    _make_install_tree(install, version="7.8.1")
    profile = _profile()
    occt_producer.write_install_profile(install, profile)
    assert not occt_producer.install_matches_profile(install, profile)


def test_candidate_publication_rejects_source_identity_outside_lock(
    monkeypatch: MonkeyPatch, tmp_path: Path
) -> None:
    install = tmp_path / "install"
    _make_install_tree(install)
    profile = dataclasses.replace(_profile(), source_commit="0" * 40)
    occt_producer.write_install_profile(install, profile)
    monkeypatch.setattr(
        occt_producer,
        "config_from_env",
        lambda: occt_producer.CacheConfig("bucket", "https://example.invalid", "key", "secret", "auto"),
    )

    with pytest.raises(RuntimeError, match="does not match the checked-in lock"):
        occt_producer.upload_prebuilt_install(
            profile,
            install,
            out_dir=tmp_path / "out",
            locked_profile_id="windows-x64-msvc-v143-md-static",
        )


def test_qualification_defaults_to_source_builds() -> None:
    source = (ROOT / "scripts" / "qualify_occt.py").read_text(encoding="utf-8")
    argument = source[source.index('"--binary-cache"') : source.index("--prepare-only")]
    assert 'default="off"' in argument


def test_exact_tag_qualification_prints_locked_objects_without_creating_state() -> None:
    lock = occt_lock.load_lock()
    tag = lock["dependency"]["source"]["tag"]
    state_root = f".deps/occt-qualification/{tag}"
    existed_before = (ROOT / state_root).exists()
    native = subprocess.run(
        [
            sys.executable,
            "scripts/build_occt.py",
            "--platform-tag",
            "windows-x64",
            "--occt-tag",
            tag,
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
            tag,
            "--occt-state-root",
            state_root,
            "--print-occt-binary-cache-key",
        ],
        cwd=ROOT,
        check=True,
        capture_output=True,
        text=True,
    ).stdout.strip()
    native_profile = occt_lock.profile_for_selector(
        lock,
        kind="native",
        platform="windows-x64",
        msvc_runtime="dynamic",
    )
    wasm_profile = occt_lock.profile_for_selector(lock, kind="wasm", platform="wasm-emscripten")
    assert native == native_profile["archive"]["object_key"]
    assert wasm == wasm_profile["archive"]["object_key"]
    assert (ROOT / state_root).exists() == existed_before


def test_qualification_rejects_moving_tag_and_state_outside_generated_root() -> None:
    tag = occt_lock.load_lock()["dependency"]["source"]["tag"]
    for arguments in (
        ["--occt-tag", "master", "--occt-state-root", ".deps/occt-qualification/master"],
        ["--occt-tag", tag, "--occt-state-root", f"out/occt-qualification/{tag}"],
    ):
        completed = subprocess.run(
            [sys.executable, "scripts/build_occt.py", *arguments, "--print-binary-cache-key"],
            cwd=ROOT,
            check=False,
            capture_output=True,
            text=True,
        )
        assert completed.returncode != 0


def test_qualification_comparison_normalizes_only_runtime_fields(tmp_path: Path) -> None:
    first = tmp_path / "first.json"
    second = tmp_path / "second.json"
    first.write_text('{"views":[{"id":"top"}],"timings":{"hlr_ms":1}}', encoding="utf-8")
    second.write_text('{"timings":{"hlr_ms":99},"views":[{"id":"top"}]}', encoding="utf-8")
    assert compare_occt_qualification.normalized_projection(first) == (
        compare_occt_qualification.normalized_projection(second)
    )
    second.write_text('{"views":[{"id":"front"}],"timings":{"hlr_ms":1}}', encoding="utf-8")
    assert compare_occt_qualification.normalized_projection(first) != (
        compare_occt_qualification.normalized_projection(second)
    )


def test_signed_r2_config_strips_bucket_from_endpoint(monkeypatch: MonkeyPatch) -> None:
    monkeypatch.setenv("R2_BUCKET", "bucket")
    monkeypatch.setenv("R2_ENDPOINT_URL", "https://account.r2.cloudflarestorage.com/bucket")
    monkeypatch.setenv("R2_ACCESS_KEY_ID", "key")
    monkeypatch.setenv("R2_SECRET_ACCESS_KEY", "secret")
    config = occt_producer.config_from_env()
    assert config is not None
    assert config.endpoint_url == "https://account.r2.cloudflarestorage.com"


def test_source_build_recipes_use_structured_definitions() -> None:
    for path, function_name in (
        (ROOT / "scripts" / "build_occt.py", "def occt_build_profile("),
        (ROOT / "scripts" / "build_wasm.py", "def occt_wasm_build_profile("),
    ):
        source = path.read_text(encoding="utf-8")
        recipe = source[source.index(function_name) : source.index("def ", source.index(function_name) + 4)]
        assert "semantic_recipe_hash" in recipe
        assert "build_parallel_jobs" not in recipe


def test_no_consumer_discovery_or_alias_api_remains() -> None:
    source = (ROOT / "scripts" / "occt_producer.py").read_text(encoding="utf-8")
    forbidden = (
        "restore_prebuilt_install",
        "ACCEPTED_CACHE_ALIASES",
        "LOCAL_INSTALL_MIGRATIONS",
        "object_prefix_candidates",
        "public_config_from_env",
    )
    assert all(name not in source for name in forbidden)
    for build_script in ("build_occt.py", "build_wasm.py"):
        text = (ROOT / "scripts" / build_script).read_text(encoding="utf-8")
        assert text.index("load_dotenv(ROOT)") > text.index("if args.upload")


def test_profile_type_is_producer_evidence_only() -> None:
    assert "never used to select a consumer archive" in (occt_producer.OcctBuildProfile.__doc__ or "")
    assert isinstance(_profile().evidence_id, str)


def test_r2_conditional_create_signs_if_none_match(monkeypatch: MonkeyPatch) -> None:
    captured: dict[str, Any] = {}

    class Response:
        def __enter__(self) -> Response:
            return self

        def __exit__(self, *_args: object) -> None:
            return None

        def read(self) -> bytes:
            return b""

    def fake_urlopen(request: Any, timeout: int) -> Response:
        captured["headers"] = {key.lower(): value for key, value in request.header_items()}
        captured["timeout"] = timeout
        return Response()

    monkeypatch.setattr(occt_producer.urllib.request, "urlopen", fake_urlopen)
    config = occt_producer.CacheConfig("bucket", "https://example.invalid", "key", "secret", "auto")
    occt_producer._r2_put_object_if_absent(config, "path/archive.zip", b"data", "application/zip")
    assert captured["headers"]["if-none-match"] == "*"
    assert "if-none-match" in captured["headers"]["authorization"]
