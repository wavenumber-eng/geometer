"""Generate and validate deterministic native Geometer build attestations."""

from __future__ import annotations

import argparse
import hashlib
import json
import re
import subprocess
from pathlib import Path
from typing import Any

from dependency_versions import OCCT_REPO, OCCT_TAG, OCCT_VERSION


ROOT = Path(__file__).resolve().parent.parent
SCHEMA = "wn.geometer.native_build_attestation.a1"
PRODUCER_IDENTITY = "wn.geometer.native_build_attestation_generator.a1"
PRODUCER_SOURCE = "scripts/native_build_attestation.py"
SIDECAR_NAME = "geometer.build-attestation.json"
SOURCE_AUTHORITY = "git_head_and_source_worktree_excluding_dist_build_deps_out"
UNAVAILABLE_AUTHORITY = "source_control_unavailable"
OCCT_AUTHORITY = "geometer_occt_profile_verified"
OCCT_UNVERIFIED_AUTHORITY = "occt_profile_unverified"
SHA256_RE = re.compile(r"[0-9a-f]{64}")
REVISION_RE = re.compile(r"[0-9a-f]{40}")
VERSION_RE = re.compile(r"[0-9]+\.[0-9]+\.[0-9]+")
TOKEN_RE = re.compile(r"[A-Za-z0-9][A-Za-z0-9._+-]*")
PLATFORM_RE = re.compile(r"[a-z0-9][a-z0-9-]*")


class BuildAttestationError(RuntimeError):
    """Raised when an attestation is absent from its claimed authority."""


def file_sha256(path: Path) -> str:
    digest = hashlib.sha256()
    with path.open("rb") as handle:
        for chunk in iter(lambda: handle.read(1024 * 1024), b""):
            digest.update(chunk)
    return digest.hexdigest()


def canonical_json(value: dict[str, Any]) -> bytes:
    return (json.dumps(value, indent=2, sort_keys=True, ensure_ascii=True) + "\n").encode("utf-8")


def sidecar_path(executable: Path) -> Path:
    return executable.with_name(SIDECAR_NAME)


def _git_output(arguments: list[str]) -> str | None:
    try:
        return subprocess.check_output(
            ["git", *arguments],
            cwd=ROOT,
            text=True,
            stderr=subprocess.DEVNULL,
            timeout=10,
        ).strip()
    except (OSError, subprocess.SubprocessError):
        return None


def source_identity() -> dict[str, str]:
    revision = _git_output(["rev-parse", "HEAD"])
    if revision is None or REVISION_RE.fullmatch(revision) is None:
        return {
            "authority": UNAVAILABLE_AUTHORITY,
            "revision": "unavailable",
            "tree_state": "unavailable",
        }
    status = _git_output(
        [
            "status",
            "--porcelain",
            "--untracked-files=normal",
            "--",
            ".",
            ":(exclude)dist/**",
            ":(exclude)build/**",
            ":(exclude)build-*/**",
            ":(exclude).deps/**",
            ":(exclude)out/**",
        ]
    )
    if status is None:
        return {
            "authority": UNAVAILABLE_AUTHORITY,
            "revision": "unavailable",
            "tree_state": "unavailable",
        }
    return {
        "authority": SOURCE_AUTHORITY,
        "revision": revision,
        "tree_state": "dirty" if status else "clean",
    }


def occt_identity(
    profile_path: Path,
    *,
    version: str,
    platform_name: str,
    arch: str,
    build_type: str,
) -> dict[str, str]:
    identity = {
        "authority": OCCT_UNVERIFIED_AUTHORITY,
        "profile_sha256": "unavailable",
        "repo": OCCT_REPO,
        "tag": OCCT_TAG,
        "version": version,
    }
    if not profile_path.is_file():
        return identity
    try:
        raw = profile_path.read_bytes()
        profile = json.loads(raw)
    except (OSError, UnicodeDecodeError, json.JSONDecodeError):
        return identity
    identity["profile_sha256"] = hashlib.sha256(raw).hexdigest()
    if not isinstance(profile, dict):
        return identity
    expected_keys = {
        "config",
        "emsdk_version",
        "kind",
        "library_type",
        "macos_deployment_target",
        "occt_repo",
        "occt_tag",
        "platform_tag",
        "recipe_hash",
        "toolchain_abi",
    }
    valid = all(
        (
            set(profile) == expected_keys,
            raw.replace(b"\r\n", b"\n") == canonical_json(profile),
            profile.get("kind") == "native",
            profile.get("platform_tag") == f"{platform_name}-{arch}",
            profile.get("config") == build_type,
            profile.get("library_type") in {"Static", "Shared"},
            profile.get("occt_repo") == OCCT_REPO,
            profile.get("occt_tag") == OCCT_TAG,
            version == OCCT_VERSION,
            isinstance(profile.get("recipe_hash"), str),
            SHA256_RE.fullmatch(str(profile.get("recipe_hash"))) is not None,
        )
    )
    if valid:
        identity["authority"] = OCCT_AUTHORITY
    return identity


def build_attestation(
    executable: Path,
    *,
    geometer_version: str,
    c_abi_version: int,
    compiler_id: str,
    compiler_version: str,
    platform_name: str,
    arch: str,
    build_type: str,
    occt: dict[str, str],
    generator: str,
) -> dict[str, Any]:
    executable = executable.resolve()
    if not executable.is_file():
        raise BuildAttestationError(f"native executable does not exist: {executable}")
    value = {
        "artifact": {"name": executable.name, "sha256": file_sha256(executable)},
        "build": {
            "arch": arch,
            "build_type": build_type,
            "c_abi_version": c_abi_version,
            "compiler": {
                "authority": "cmake_compiler_id_and_version",
                "id": compiler_id,
                "version": compiler_version,
            },
            "generator": generator,
            "geometer_version": geometer_version,
            "occt": occt,
            "platform": platform_name,
            "source": source_identity(),
        },
        "producer": {
            "identity": PRODUCER_IDENTITY,
            "source": PRODUCER_SOURCE,
            "source_sha256": file_sha256(Path(__file__).resolve()),
        },
        "schema": SCHEMA,
    }
    _validate_shape(value)
    return value


def write_attestation(executable: Path, output: Path, **metadata: Any) -> dict[str, Any]:
    value = build_attestation(executable, **metadata)
    output.parent.mkdir(parents=True, exist_ok=True)
    temporary = output.with_name(f"{output.name}.tmp")
    temporary.write_bytes(canonical_json(value))
    temporary.replace(output)
    return value


def load_and_validate_attestation(executable: Path, path: Path | None = None) -> dict[str, Any] | None:
    executable = executable.resolve()
    attestation = sidecar_path(executable) if path is None else path.resolve()
    if not attestation.is_file():
        return None
    try:
        raw = attestation.read_bytes()
        value = json.loads(raw)
    except (OSError, UnicodeDecodeError, json.JSONDecodeError) as error:
        raise BuildAttestationError(f"invalid native build attestation JSON: {error}") from error
    if not isinstance(value, dict):
        raise BuildAttestationError("native build attestation root must be an object")
    _validate_shape(value)
    if raw != canonical_json(value):
        raise BuildAttestationError("native build attestation is not canonical deterministic JSON")
    producer_source = ROOT / value["producer"]["source"]
    if not producer_source.is_file():
        raise BuildAttestationError("native build attestation producer source is unavailable")
    if value["producer"]["source_sha256"] != file_sha256(producer_source):
        raise BuildAttestationError("native build attestation producer source SHA-256 is stale")
    if value["artifact"]["name"] != executable.name:
        raise BuildAttestationError("native build attestation artifact name does not match executable")
    actual_sha256 = file_sha256(executable)
    if value["artifact"]["sha256"] != actual_sha256:
        raise BuildAttestationError("native build attestation executable SHA-256 is stale or mismatched")
    version = _executable_version(executable)
    expected = (
        f"geometer {value['build']['geometer_version']} "
        f"(abi {value['build']['c_abi_version']})"
    )
    if version != expected:
        raise BuildAttestationError("native build attestation version/C ABI does not match executable")
    source = value["build"]["source"]
    current_source = source_identity()
    value["build_provenance_attested"] = bool(
        REVISION_RE.fullmatch(source["revision"])
        and source["tree_state"] == "clean"
        and source["authority"] == SOURCE_AUTHORITY
        and source == current_source
        and value["build"]["occt"]["authority"] == OCCT_AUTHORITY
    )
    value["sidecar_sha256"] = hashlib.sha256(raw).hexdigest()
    return value


def _executable_version(executable: Path) -> str:
    try:
        return subprocess.check_output(
            [str(executable), "--version"], text=True, stderr=subprocess.STDOUT, timeout=10
        ).strip()
    except (OSError, subprocess.SubprocessError) as error:
        raise BuildAttestationError(f"could not query attested executable version: {error}") from error


def _keys(value: dict[str, Any], expected: set[str], label: str) -> None:
    if set(value) != expected:
        raise BuildAttestationError(f"{label} fields differ from schema")


def _text(value: Any, label: str, pattern: re.Pattern[str] | None = None) -> str:
    if not isinstance(value, str) or not value or "\n" in value or "\r" in value:
        raise BuildAttestationError(f"{label} must be non-empty single-line text")
    if pattern is not None and pattern.fullmatch(value) is None:
        raise BuildAttestationError(f"{label} has an invalid value")
    return value


def _validate_occt(occt: dict[str, Any]) -> None:
    _keys(occt, {"authority", "profile_sha256", "repo", "tag", "version"}, "occt")
    authority = _text(occt["authority"], "occt.authority", TOKEN_RE)
    if authority not in {OCCT_AUTHORITY, OCCT_UNVERIFIED_AUTHORITY}:
        raise BuildAttestationError("occt.authority is unsupported")
    profile_sha256 = _text(occt["profile_sha256"], "occt.profile_sha256")
    if profile_sha256 != "unavailable" and SHA256_RE.fullmatch(profile_sha256) is None:
        raise BuildAttestationError("occt.profile_sha256 has an invalid value")
    if authority == OCCT_AUTHORITY and SHA256_RE.fullmatch(profile_sha256) is None:
        raise BuildAttestationError("verified OCCT authority requires a profile SHA-256")
    if occt["repo"] != OCCT_REPO:
        raise BuildAttestationError("occt.repo does not match the governed dependency source")
    tag = _text(occt["tag"], "occt.tag", TOKEN_RE)
    version = _text(occt["version"], "occt.version", VERSION_RE)
    if tag != OCCT_TAG or version != OCCT_VERSION:
        raise BuildAttestationError("OCCT tag/version does not match the governed dependency pin")


def _validate_shape(value: dict[str, Any]) -> None:
    _keys(value, {"artifact", "build", "producer", "schema"}, "attestation")
    if value["schema"] != SCHEMA:
        raise BuildAttestationError("native build attestation schema is unsupported")
    artifact = value["artifact"]
    build = value["build"]
    producer = value["producer"]
    if not isinstance(artifact, dict) or not isinstance(build, dict) or not isinstance(producer, dict):
        raise BuildAttestationError("native build attestation sections must be objects")
    _keys(artifact, {"name", "sha256"}, "artifact")
    _keys(
        build,
        {
            "arch",
            "build_type",
            "c_abi_version",
            "compiler",
            "generator",
            "geometer_version",
            "occt",
            "platform",
            "source",
        },
        "build",
    )
    artifact_name = _text(artifact["name"], "artifact.name")
    if Path(artifact_name).name != artifact_name or "/" in artifact_name or "\\" in artifact_name:
        raise BuildAttestationError("artifact.name must not contain a path")
    _text(artifact["sha256"], "artifact.sha256", SHA256_RE)
    _text(build["arch"], "build.arch", PLATFORM_RE)
    _text(build["build_type"], "build.build_type", TOKEN_RE)
    if not isinstance(build["c_abi_version"], int) or isinstance(build["c_abi_version"], bool) or build["c_abi_version"] <= 0:
        raise BuildAttestationError("build.c_abi_version must be a positive integer")
    generator = _text(build["generator"], "build.generator")
    if "/" in generator or "\\" in generator:
        raise BuildAttestationError("build.generator must not contain a path")
    _text(build["geometer_version"], "build.geometer_version", VERSION_RE)
    _text(build["platform"], "build.platform", PLATFORM_RE)
    compiler = build["compiler"]
    occt = build["occt"]
    source = build["source"]
    if not isinstance(compiler, dict) or not isinstance(occt, dict) or not isinstance(source, dict):
        raise BuildAttestationError("compiler, OCCT, and source must be objects")
    _keys(compiler, {"authority", "id", "version"}, "compiler")
    if compiler["authority"] != "cmake_compiler_id_and_version":
        raise BuildAttestationError("compiler.authority is unsupported")
    _text(compiler["id"], "compiler.id", TOKEN_RE)
    _text(compiler["version"], "compiler.version", TOKEN_RE)
    _validate_occt(occt)
    _keys(source, {"authority", "revision", "tree_state"}, "source")
    authority = _text(source["authority"], "source.authority", TOKEN_RE)
    revision = _text(source["revision"], "source.revision")
    tree_state = _text(source["tree_state"], "source.tree_state", TOKEN_RE)
    if authority == SOURCE_AUTHORITY:
        if REVISION_RE.fullmatch(revision) is None or tree_state not in {"clean", "dirty"}:
            raise BuildAttestationError("git source authority requires a revision and clean/dirty state")
    elif authority == UNAVAILABLE_AUTHORITY:
        if revision != "unavailable" or tree_state != "unavailable":
            raise BuildAttestationError("unavailable source authority has inconsistent fields")
    else:
        raise BuildAttestationError("source.authority is unsupported")
    _keys(producer, {"identity", "source", "source_sha256"}, "producer")
    if producer["identity"] != PRODUCER_IDENTITY or producer["source"] != PRODUCER_SOURCE:
        raise BuildAttestationError("attestation producer identity/source is unsupported")
    _text(producer["source_sha256"], "producer.source_sha256", SHA256_RE)


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    subparsers = parser.add_subparsers(dest="command", required=True)
    write = subparsers.add_parser("write")
    write.add_argument("--executable", required=True, type=Path)
    write.add_argument("--output", required=True, type=Path)
    write.add_argument("--geometer-version", required=True)
    write.add_argument("--c-abi-version", required=True, type=int)
    write.add_argument("--compiler-id", required=True)
    write.add_argument("--compiler-version", required=True)
    write.add_argument("--platform", required=True)
    write.add_argument("--arch", required=True)
    write.add_argument("--build-type", required=True)
    write.add_argument("--occt-profile", required=True, type=Path)
    write.add_argument("--occt-version", required=True)
    write.add_argument("--generator", required=True)
    validate = subparsers.add_parser("validate")
    validate.add_argument("--executable", required=True, type=Path)
    validate.add_argument("--attestation", type=Path)
    args = parser.parse_args()
    if args.command == "validate":
        value = load_and_validate_attestation(args.executable, args.attestation)
        if value is None:
            raise BuildAttestationError("native build attestation is missing")
        print(value["sidecar_sha256"])
        return 0
    occt = occt_identity(
        args.occt_profile,
        version=args.occt_version,
        platform_name=args.platform,
        arch=args.arch,
        build_type=args.build_type,
    )
    write_attestation(
        args.executable,
        args.output,
        geometer_version=args.geometer_version,
        c_abi_version=args.c_abi_version,
        compiler_id=args.compiler_id,
        compiler_version=args.compiler_version,
        platform_name=args.platform,
        arch=args.arch,
        build_type=args.build_type,
        occt=occt,
        generator=args.generator,
    )
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
