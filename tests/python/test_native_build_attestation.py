from __future__ import annotations

import json
import sys
from pathlib import Path
from typing import Any

import pytest


ROOT = Path(__file__).resolve().parents[2]
sys.path.insert(0, str(ROOT / "scripts"))

import native_build_attestation as attestation  # noqa: E402
from analytic_qualification import environment  # noqa: E402


def metadata() -> dict[str, Any]:
    return {
        "geometer_version": "2026.6.23",
        "c_abi_version": 20260623,
        "compiler_id": "MSVC",
        "compiler_version": "19.44.35207.1",
        "platform_name": "windows",
        "arch": "x64",
        "build_type": "Release",
        "occt": {
            "authority": attestation.OCCT_AUTHORITY,
            "profile_sha256": "f" * 64,
            "repo": attestation.OCCT_REPO,
            "tag": "V8_0_1",
            "version": "8.0.1",
        },
        "generator": "Ninja",
    }


def clean_source() -> dict[str, str]:
    return {
        "authority": attestation.SOURCE_AUTHORITY,
        "revision": "a" * 40,
        "tree_state": "clean",
    }


def write_sidecar(
    tmp_path: Path, monkeypatch: pytest.MonkeyPatch, source: dict[str, str] | None = None
) -> tuple[Path, Path]:
    executable = tmp_path / "geometer.exe"
    executable.write_bytes(b"native-geometer")
    monkeypatch.setattr(attestation, "source_identity", lambda: source or clean_source())
    monkeypatch.setattr(
        attestation,
        "_executable_version",
        lambda _path: "geometer 2026.6.23 (abi 20260623)",
    )
    sidecar = attestation.sidecar_path(executable)
    attestation.write_attestation(executable, sidecar, **metadata())
    return executable, sidecar


def test_canonical_sidecar_binds_exact_executable_and_clean_source(
    tmp_path: Path, monkeypatch: pytest.MonkeyPatch
) -> None:
    executable, sidecar = write_sidecar(tmp_path, monkeypatch)
    first = sidecar.read_bytes()
    attestation.write_attestation(executable, sidecar, **metadata())
    assert sidecar.read_bytes() == first
    value = attestation.load_and_validate_attestation(executable)
    assert value is not None
    assert value["build_provenance_attested"] is True
    assert value["artifact"]["sha256"] == attestation.file_sha256(executable)
    assert value["producer"]["identity"] == attestation.PRODUCER_IDENTITY
    assert value["producer"]["source"] == "scripts/native_build_attestation.py"
    assert len(value["producer"]["source_sha256"]) == 64
    raw = first.decode("utf-8")
    assert str(ROOT) not in raw
    assert "timestamp" not in raw


def test_same_day_runtime_serial_is_valid(
    tmp_path: Path, monkeypatch: pytest.MonkeyPatch
) -> None:
    executable = tmp_path / "geometer.exe"
    executable.write_bytes(b"native-geometer")
    values = metadata()
    values["geometer_version"] = "2026.6.23.1"
    monkeypatch.setattr(attestation, "source_identity", clean_source)
    monkeypatch.setattr(
        attestation,
        "_executable_version",
        lambda _path: "geometer 2026.6.23.1 (abi 20260623)",
    )
    sidecar = attestation.sidecar_path(executable)
    attestation.write_attestation(executable, sidecar, **values)
    loaded = attestation.load_and_validate_attestation(executable)
    assert loaded is not None
    assert loaded["build"]["geometer_version"] == "2026.6.23.1"


@pytest.mark.parametrize("tree_state", ["dirty", "unavailable"])
def test_nonclean_source_sidecar_is_valid_but_never_promotion_attested(
    tmp_path: Path, monkeypatch: pytest.MonkeyPatch, tree_state: str
) -> None:
    source = (
        {
            "authority": attestation.SOURCE_AUTHORITY,
            "revision": "b" * 40,
            "tree_state": "dirty",
        }
        if tree_state == "dirty"
        else {
            "authority": attestation.UNAVAILABLE_AUTHORITY,
            "revision": "unavailable",
            "tree_state": "unavailable",
        }
    )
    executable, _ = write_sidecar(tmp_path, monkeypatch, source)
    value = attestation.load_and_validate_attestation(executable)
    assert value is not None
    assert value["build_provenance_attested"] is False


def test_clean_sidecar_from_a_different_current_revision_is_not_attested(
    tmp_path: Path, monkeypatch: pytest.MonkeyPatch
) -> None:
    executable, _ = write_sidecar(tmp_path, monkeypatch)
    monkeypatch.setattr(
        attestation,
        "source_identity",
        lambda: {**clean_source(), "revision": "b" * 40},
    )
    value = attestation.load_and_validate_attestation(executable)
    assert value is not None
    assert value["build_provenance_attested"] is False


def test_unverified_occt_profile_is_never_promotion_attested(
    tmp_path: Path, monkeypatch: pytest.MonkeyPatch
) -> None:
    values = metadata()
    values["occt"] = {
        "authority": attestation.OCCT_UNVERIFIED_AUTHORITY,
        "profile_sha256": "unavailable",
        "repo": attestation.OCCT_REPO,
        "tag": "V8_0_1",
        "version": "8.0.1",
    }
    executable = tmp_path / "geometer.exe"
    executable.write_bytes(b"native-geometer")
    monkeypatch.setattr(attestation, "source_identity", clean_source)
    monkeypatch.setattr(
        attestation,
        "_executable_version",
        lambda _path: "geometer 2026.6.23 (abi 20260623)",
    )
    attestation.write_attestation(executable, attestation.sidecar_path(executable), **values)
    result = attestation.load_and_validate_attestation(executable)
    assert result is not None
    assert result["build_provenance_attested"] is False


def test_occt_identity_requires_exact_canonical_resolved_profile(
    tmp_path: Path,
) -> None:
    profile = tmp_path / ".geometer-occt-profile.json"
    value = {
        "config": "Release",
        "emsdk_version": None,
        "kind": "native",
        "library_type": "Static",
        "macos_deployment_target": None,
        "occt_repo": "https://github.com/Open-Cascade-SAS/OCCT.git",
        "occt_tag": "V8_0_1",
        "platform_tag": "windows-x64",
        "recipe_hash": "1" * 64,
        "toolchain_abi": "msvc-v143",
    }
    profile.write_bytes(attestation.canonical_json(value))
    verified = attestation.occt_identity(
        profile,
        version="8.0.1",
        platform_name="windows",
        arch="x64",
        build_type="Release",
    )
    assert verified["authority"] == attestation.OCCT_AUTHORITY
    assert verified["profile_sha256"] == attestation.file_sha256(profile)
    assert verified["repo"] == attestation.OCCT_REPO
    value["occt_repo"] = "https://example.invalid/Mutable-OCCT-Fork.git"
    profile.write_bytes(attestation.canonical_json(value))
    mismatch = attestation.occt_identity(
        profile,
        version="8.0.1",
        platform_name="windows",
        arch="x64",
        build_type="Release",
    )
    assert mismatch["authority"] == attestation.OCCT_UNVERIFIED_AUTHORITY


def test_sidecar_rejects_ungoverned_occt_repository(
    tmp_path: Path, monkeypatch: pytest.MonkeyPatch
) -> None:
    executable = tmp_path / "geometer.exe"
    executable.write_bytes(b"native-geometer")
    monkeypatch.setattr(attestation, "source_identity", clean_source)
    values = metadata()
    values["occt"] = {
        **values["occt"],
        "repo": "https://example.invalid/Mutable-OCCT-Fork.git",
    }
    with pytest.raises(attestation.BuildAttestationError, match="governed dependency source"):
        attestation.build_attestation(executable, **values)


def test_absent_sidecar_is_unattested(tmp_path: Path) -> None:
    executable = tmp_path / "geometer"
    executable.write_bytes(b"native-geometer")
    assert attestation.load_and_validate_attestation(executable) is None


def test_mutated_executable_rejects_stale_sidecar(
    tmp_path: Path, monkeypatch: pytest.MonkeyPatch
) -> None:
    executable, _ = write_sidecar(tmp_path, monkeypatch)
    executable.write_bytes(b"different-native-geometer")
    with pytest.raises(attestation.BuildAttestationError, match="stale or mismatched"):
        attestation.load_and_validate_attestation(executable)


def test_noncanonical_or_unknown_sidecar_fields_fail_closed(
    tmp_path: Path, monkeypatch: pytest.MonkeyPatch
) -> None:
    executable, sidecar = write_sidecar(tmp_path, monkeypatch)
    value = json.loads(sidecar.read_text(encoding="utf-8"))
    sidecar.write_text(json.dumps(value), encoding="utf-8")
    with pytest.raises(attestation.BuildAttestationError, match="not canonical"):
        attestation.load_and_validate_attestation(executable)
    value["unexpected"] = True
    sidecar.write_bytes(attestation.canonical_json(value))
    with pytest.raises(attestation.BuildAttestationError, match="fields differ"):
        attestation.load_and_validate_attestation(executable)


def test_version_or_c_abi_mismatch_fails_closed(
    tmp_path: Path, monkeypatch: pytest.MonkeyPatch
) -> None:
    executable, _ = write_sidecar(tmp_path, monkeypatch)
    monkeypatch.setattr(
        attestation,
        "_executable_version",
        lambda _path: "geometer 2026.6.23 (abi 19990101)",
    )
    with pytest.raises(attestation.BuildAttestationError, match="version/C ABI"):
        attestation.load_and_validate_attestation(executable)


def test_stale_attestation_generator_source_fails_closed(
    tmp_path: Path, monkeypatch: pytest.MonkeyPatch
) -> None:
    executable, sidecar = write_sidecar(tmp_path, monkeypatch)
    value = json.loads(sidecar.read_text(encoding="utf-8"))
    value["producer"]["source_sha256"] = "0" * 64
    sidecar.write_bytes(attestation.canonical_json(value))
    with pytest.raises(attestation.BuildAttestationError, match="producer source SHA-256 is stale"):
        attestation.load_and_validate_attestation(executable)


@pytest.mark.parametrize(
    ("field", "value", "message"),
    [
        ("compiler_id", "C:\\toolchain\\cl.exe", "compiler.id has an invalid value"),
        ("generator", "C:\\cmake\\Ninja", "build.generator must not contain a path"),
    ],
)
def test_toolchain_metadata_rejects_absolute_paths(
    tmp_path: Path,
    monkeypatch: pytest.MonkeyPatch,
    field: str,
    value: str,
    message: str,
) -> None:
    executable = tmp_path / "geometer.exe"
    executable.write_bytes(b"native-geometer")
    monkeypatch.setattr(attestation, "source_identity", clean_source)
    values = metadata()
    values[field] = value
    with pytest.raises(attestation.BuildAttestationError, match=message):
        attestation.build_attestation(executable, **values)


def test_toolchain_profile_accepts_only_validated_clean_attestation(
    tmp_path: Path, monkeypatch: pytest.MonkeyPatch
) -> None:
    executable, _ = write_sidecar(tmp_path, monkeypatch)
    monkeypatch.setattr(
        environment.subprocess,
        "check_output",
        lambda *_args, **_kwargs: "geometer 2026.6.23 (abi 20260623)\n",
    )
    profile = environment.toolchain_profile(executable)["profile"]
    assert profile["build_provenance_attested"] is True
    assert profile["hint_authority"] == "validated_executable_bound_clean_source_attestation"
    assert profile["build"]["compiler"] == {
        "authority": "cmake_compiler_id_and_version",
        "id": "MSVC",
        "version": "19.44.35207.1",
    }


def test_toolchain_profile_fails_closed_on_invalid_adjacent_sidecar(
    tmp_path: Path, monkeypatch: pytest.MonkeyPatch
) -> None:
    executable = tmp_path / "geometer"
    executable.write_bytes(b"native-geometer")
    attestation.sidecar_path(executable).write_text("{}\n", encoding="utf-8")
    monkeypatch.setattr(
        environment.subprocess,
        "check_output",
        lambda *_args, **_kwargs: "geometer 2026.6.23 (abi 20260623)\n",
    )
    with pytest.raises(environment.QualificationError, match="attestation validation failed"):
        environment.toolchain_profile(executable)


def test_cmake_generates_and_copies_fixed_sidecar() -> None:
    cmake = (ROOT / "src" / "cpp" / "cli" / "CMakeLists.txt").read_text(encoding="utf-8")
    assert "scripts/native_build_attestation.py\" write" in cmake
    assert "--executable \"$<TARGET_FILE:geometer>\"" in cmake
    assert "--compiler-id \"${CMAKE_CXX_COMPILER_ID}\"" in cmake
    assert "--compiler-version \"${CMAKE_CXX_COMPILER_VERSION}\"" in cmake
    assert "add_custom_target(geometer_native_distribution ALL" in cmake
    assert "DEPENDS geometer \"${CMAKE_SOURCE_DIR}/scripts/native_build_attestation.py\"" in cmake
    assert '--occt-profile "${OpenCASCADE_INSTALL_PREFIX}/.geometer-occt-profile.json"' in cmake
    assert '--occt-version "${OpenCASCADE_VERSION}"' in cmake
    assert "geometer.build-attestation.json" in cmake
    assert cmake.index("native_build_attestation.py\" write") < cmake.index(
        '"$<TARGET_FILE:geometer>"\n            "${GEOMETER_NATIVE_DIST_DIR}/"'
    )


def test_python_wheel_requires_and_copies_fixed_sidecar() -> None:
    setup = (ROOT / "setup.py").read_text(encoding="utf-8")
    assert "sidecar = _source_attestation(executable)" in setup
    assert "self.copy_file(str(sidecar), str(target_dir / sidecar.name))" in setup
    assert 'executable.with_name("geometer.build-attestation.json")' in setup
    assert "load_and_validate_attestation(executable, path)" in setup
