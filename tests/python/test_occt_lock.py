from __future__ import annotations

import copy
import hashlib
import json
import sys
import zipfile
from pathlib import Path
from typing import Any

import pytest


ROOT = Path(__file__).resolve().parents[2]
SCRIPTS = ROOT / "scripts"
if str(SCRIPTS) not in sys.path:
    sys.path.insert(0, str(SCRIPTS))

import dependency_versions  # noqa: E402
import occt_lock  # noqa: E402


EXPECTED_PROFILES = {
    ("native", "linux-arm64", "none"): "linux-arm64-gcc11-static",
    ("native", "linux-x64", "none"): "linux-x64-gcc11-static",
    ("native", "macos-arm64", "none"): "macos-arm64-appleclang17-static",
    ("native", "windows-x64", "dynamic"): "windows-x64-msvc-v143-md-static",
    ("native", "windows-x64", "static"): "windows-x64-msvc-v143-mt-static",
    ("wasm", "wasm-emscripten", "none"): "wasm-emscripten-3.1.56-static",
}


def test_checked_in_lock_is_canonical_and_has_explicit_supported_matrix() -> None:
    lock = occt_lock.load_lock()

    actual = {
        (
            profile["selector"]["kind"],
            profile["selector"]["platform"],
            profile["selector"]["msvc_runtime"],
        ): profile["id"]
        for profile in lock["profiles"]
    }
    assert actual == EXPECTED_PROFILES
    assert all(profile["archive"]["sha256"] in profile["archive"]["object_key"] for profile in lock["profiles"])
    assert b"recipe" not in occt_lock.LOCK_PATH.read_bytes().lower()
    assert b"alias" not in occt_lock.LOCK_PATH.read_bytes().lower()


def test_dependency_versions_has_one_occt_authority() -> None:
    lock = occt_lock.load_lock()

    assert dependency_versions.OCCT_REPO == lock["dependency"]["source"]["repository"]
    assert dependency_versions.OCCT_TAG == lock["dependency"]["source"]["tag"]
    assert dependency_versions.OCCT_VERSION == lock["dependency"]["version"]
    wasm = occt_lock.profile_for_selector(lock, kind="wasm", platform="wasm-emscripten")
    assert dependency_versions.EMSDK_VERSION == wasm["abi"]["compiler"].removeprefix("emscripten-")


@pytest.mark.parametrize(
    ("mutation", "message"),
    [
        (lambda lock: lock["profiles"][0]["archive"].update(size=0), "between 1"),
        (lambda lock: lock["profiles"][0]["archive"].update(sha256="0" * 64), "must contain"),
        (lambda lock: lock["profiles"][1].update(id=lock["profiles"][0]["id"]), "sorted and unique"),
        (
            lambda lock: lock["profiles"][1].update(
                selector=lock["profiles"][0]["selector"], abi=lock["profiles"][0]["abi"]
            ),
            "selectors must be unique",
        ),
    ],
)
def test_lock_validation_fails_closed(mutation: Any, message: str) -> None:
    lock = copy.deepcopy(occt_lock.load_lock())
    mutation(lock)

    with pytest.raises(occt_lock.LockError, match=message):
        occt_lock.validate_lock(lock)


def test_restore_uses_only_the_exact_locked_object(tmp_path: Path) -> None:
    lock_path, profile, artifact_root = _make_test_lock(tmp_path)
    install_dir = tmp_path / "install"

    occt_lock.restore_locked_install(
        profile,
        install_dir,
        lock_path=lock_path,
        base_url=artifact_root.as_uri(),
    )

    assert occt_lock.install_matches_lock(install_dir, occt_lock.load_lock(lock_path), profile)
    marker = json.loads((install_dir / occt_lock.INSTALL_MARKER).read_text(encoding="utf-8"))
    assert marker["profile_id"] == profile["id"]
    assert marker["archive_sha256"] == profile["archive"]["sha256"]

    version_path = install_dir / "lib" / "cmake" / "opencascade" / "OpenCASCADEConfigVersion.cmake"
    version_path.write_text('set(PACKAGE_VERSION "0.0.0")\n', encoding="utf-8")
    assert not occt_lock.install_matches_lock(install_dir, occt_lock.load_lock(lock_path), profile)


def test_restore_rejects_wrong_locked_bytes(tmp_path: Path) -> None:
    lock_path, profile, artifact_root = _make_test_lock(tmp_path)
    artifact = artifact_root / profile["archive"]["object_key"]
    artifact.write_bytes(artifact.read_bytes() + b"corruption")

    with pytest.raises(occt_lock.LockError, match="size mismatch"):
        occt_lock.restore_locked_install(
            profile,
            tmp_path / "install",
            lock_path=lock_path,
            base_url=artifact_root.as_uri(),
        )


def test_download_preflights_size_and_uses_bounded_network_timeout(
    monkeypatch: pytest.MonkeyPatch, tmp_path: Path
) -> None:
    requests: list[tuple[str, int]] = []

    class Response:
        headers = {"Content-Length": "4"}

        def __init__(self, body: bytes = b"") -> None:
            self.body = body

        def __enter__(self) -> Response:
            return self

        def __exit__(self, *_args: object) -> None:
            return None

        def read(self, _size: int = -1) -> bytes:
            body, self.body = self.body, b""
            return body

    def fake_urlopen(request: Any, timeout: int) -> Response:
        requests.append((request.get_method(), timeout))
        return Response(b"data" if request.get_method() == "GET" else b"")

    monkeypatch.setattr(occt_lock.urllib.request, "urlopen", fake_urlopen)
    output = tmp_path / "artifact.zip"
    occt_lock._download_verified("https://example.invalid/artifact", output, hashlib.sha256(b"data").hexdigest(), 4)

    assert output.read_bytes() == b"data"
    assert requests == [("HEAD", occt_lock.DOWNLOAD_TIMEOUT_SECONDS), ("GET", occt_lock.DOWNLOAD_TIMEOUT_SECONDS)]


def test_extractor_rejects_archive_traversal(tmp_path: Path) -> None:
    archive_path = tmp_path / "unsafe.zip"
    with zipfile.ZipFile(archive_path, "w") as archive:
        archive.writestr("../escape", b"bad")

    with pytest.raises(occt_lock.LockError, match="unsafe path"):
        occt_lock._extract_zip_safely(archive_path, tmp_path / "extract")
    assert not (tmp_path / "escape").exists()


def _make_test_lock(tmp_path: Path) -> tuple[Path, dict[str, Any], Path]:
    source_tree = tmp_path / "source-install"
    cmake_dir = source_tree / "lib" / "cmake" / "opencascade"
    cmake_dir.mkdir(parents=True)
    (cmake_dir / "OpenCASCADEConfig.cmake").write_text("# test\n", encoding="utf-8")
    (cmake_dir / "OpenCASCADEConfigVersion.cmake").write_text(
        'set(PACKAGE_VERSION "8.0.1")\n',
        encoding="utf-8",
    )
    marker = {
        "config": "Release",
        "emsdk_version": None,
        "kind": "native",
        "library_type": "Static",
        "macos_deployment_target": None,
        "occt_repo": "https://example.invalid/OCCT.git",
        "occt_tag": "V8_0_1",
        "platform_tag": "linux-x64",
        "recipe_hash": "5" * 64,
        "toolchain_abi": "gcc-11-libstdcxx-abi-default",
    }
    marker_bytes = occt_lock.canonical_json_bytes(marker)
    (source_tree / occt_lock.ARCHIVE_PROFILE_MARKER).write_bytes(marker_bytes)
    archive_path = tmp_path / "occt-install.zip"
    with zipfile.ZipFile(archive_path, "w", compression=zipfile.ZIP_STORED) as archive:
        for child in sorted(path for path in source_tree.rglob("*") if path.is_file()):
            archive.write(child, child.relative_to(source_tree).as_posix())
    archive_bytes = archive_path.read_bytes()
    archive_sha256 = hashlib.sha256(archive_bytes).hexdigest()
    source_sha256 = "1" * 64
    profile = {
        "abi": {
            "architecture": "x64",
            "compiler": "gcc-11",
            "cpp_runtime": "libstdc++",
            "library_type": "static",
            "operating_system": "linux",
        },
        "archive": {
            "object_key": f"dependencies/occt/test/{archive_sha256}/occt-install.zip",
            "profile_sha256": hashlib.sha256(marker_bytes).hexdigest(),
            "sha256": archive_sha256,
            "size": len(archive_bytes),
        },
        "id": "linux-x64-gcc11-static",
        "qualification": {
            "release_tag": "v2026-09-19",
            "source_revision": "2" * 40,
            "workflow": ".github/workflows/release.yml",
            "workflow_run_id": "1",
        },
        "selector": {"kind": "native", "msvc_runtime": "none", "platform": "linux-x64"},
    }
    lock = {
        "dependency": {
            "name": "occt",
            "source": {
                "archive": {
                    "license_files": ["LICENSE_LGPL_21.txt"],
                    "object_key": f"dependencies/occt/source/{'3' * 40}/{source_sha256}/occt-source.tar.gz",
                    "sha256": source_sha256,
                    "size": 1,
                },
                "commit": "3" * 40,
                "repository": "https://example.invalid/OCCT.git",
                "tag": "V8_0_1",
                "tag_object": "4" * 40,
            },
            "version": "8.0.1",
        },
        "profiles": [profile],
        "schema": occt_lock.LOCK_SCHEMA,
    }
    lock_path = tmp_path / "occt-lock.json"
    lock_path.write_bytes(occt_lock.canonical_json_bytes(lock))
    artifact_root = tmp_path / "artifacts"
    locked_archive = artifact_root / profile["archive"]["object_key"]
    locked_archive.parent.mkdir(parents=True)
    locked_archive.write_bytes(archive_bytes)
    return lock_path, profile, artifact_root
