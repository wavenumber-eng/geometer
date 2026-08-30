from __future__ import annotations

import zipfile
from pathlib import Path

import pytest

from package_release_artifacts import is_native_runtime_file
from validate_release_artifacts import LICENSE_NAMES, validate_native


ROOT = Path(__file__).resolve().parents[2]


def _write_native_archive(path: Path, extra_names: tuple[str, ...] = ()) -> None:
    names = {
        "geometer.exe",
        "geometer.build-attestation.json",
        *(f"licenses/{name}" for name in LICENSE_NAMES),
        *extra_names,
    }
    with zipfile.ZipFile(path, "w") as archive:
        for name in sorted(names):
            archive.writestr(name, b"governed test payload")


def test_native_runtime_file_filter_excludes_static_libraries(tmp_path: Path) -> None:
    executable = tmp_path / "geometer.exe"
    static_library = tmp_path / "geometer.lib"
    unix_archive = tmp_path / "libgeometer.a"
    executable.write_bytes(b"exe")
    static_library.write_bytes(b"lib")
    unix_archive.write_bytes(b"archive")

    assert is_native_runtime_file(executable)
    assert not is_native_runtime_file(static_library)
    assert not is_native_runtime_file(unix_archive)


def test_native_runtime_validator_accepts_archive_without_static_library(tmp_path: Path) -> None:
    archive = tmp_path / "native-windows-x64.zip"
    _write_native_archive(archive)

    validate_native(archive)


@pytest.mark.parametrize("library_name", ["geometer.lib", "libgeometer.a"])
def test_native_runtime_validator_rejects_static_library(tmp_path: Path, library_name: str) -> None:
    archive = tmp_path / "native-windows-x64.zip"
    _write_native_archive(archive, (library_name,))

    with pytest.raises(ValueError, match="build-only static libraries"):
        validate_native(archive)


def test_native_build_does_not_copy_static_library_to_dist() -> None:
    cmake = (ROOT / "src/cpp/lib/CMakeLists.txt").read_text(encoding="utf-8")

    assert "Copying geometer_lib to dist/native" not in cmake
    assert '"${GEOMETER_NATIVE_DIST_DIR}/libgeometer.a"' not in cmake


def test_release_workflow_uses_filtered_native_runtime_packager() -> None:
    workflow = (ROOT / ".github/workflows/release.yml").read_text(encoding="utf-8")

    assert "scripts/package_release_artifacts.py native" in workflow
    assert "make_archive('out/native-" not in workflow
