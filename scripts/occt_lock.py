from __future__ import annotations

import hashlib
import json
import os
import re
import shutil
import tempfile
import urllib.error
import urllib.parse
import urllib.request
import zipfile
from pathlib import Path
from typing import Any


ROOT = Path(__file__).resolve().parent.parent
LOCK_PATH = ROOT / "dependencies" / "occt-lock.json"
LOCK_SCHEMA = "wn.geometer.occt_lock.a0"
INSTALL_MARKER = ".geometer-occt-lock.json"
ARCHIVE_PROFILE_MARKER = ".geometer-occt-profile.json"
DEFAULT_BASE_URL = "https://artifacts.wavenumber.net"
DOWNLOAD_USER_AGENT = "wn-geometer-occt-lock/1.0"
MAX_ARCHIVE_BYTES = 512 * 1024 * 1024
MAX_ARCHIVE_ENTRIES = 20_000
MAX_EXPANDED_BYTES = 2 * 1024 * 1024 * 1024
MAX_ENTRY_BYTES = 1024 * 1024 * 1024
DOWNLOAD_TIMEOUT_SECONDS = 30
SHA256_PATTERN = re.compile(r"[0-9a-f]{64}")
GIT_SHA_PATTERN = re.compile(r"[0-9a-f]{40}")


class LockError(RuntimeError):
    """Raised when the immutable OCCT lock or its selected artifact is invalid."""


def canonical_json_bytes(value: Any) -> bytes:
    return (json.dumps(value, indent=2, sort_keys=True) + "\n").encode("utf-8")


def sha256_file(path: Path) -> str:
    digest = hashlib.sha256()
    with path.open("rb") as handle:
        for chunk in iter(lambda: handle.read(1024 * 1024), b""):
            digest.update(chunk)
    return digest.hexdigest()


def load_lock(path: Path = LOCK_PATH) -> dict[str, Any]:
    raw = path.read_bytes()
    try:
        lock = json.loads(raw, object_pairs_hook=_reject_duplicate_keys)
    except (json.JSONDecodeError, ValueError) as exc:
        raise LockError(f"Invalid OCCT lock JSON: {exc}") from exc
    validate_lock(lock)
    if raw != canonical_json_bytes(lock):
        raise LockError(f"OCCT lock is not canonical JSON: {path}")
    return lock


def lock_sha256(path: Path = LOCK_PATH) -> str:
    load_lock(path)
    return sha256_file(path)


def profile_by_id(lock: dict[str, Any], profile_id: str) -> dict[str, Any]:
    matches = [profile for profile in lock["profiles"] if profile["id"] == profile_id]
    if len(matches) != 1:
        raise LockError(f"OCCT lock has no unique profile {profile_id!r}")
    return matches[0]


def profile_for_selector(
    lock: dict[str, Any], *, kind: str, platform: str, msvc_runtime: str = "none"
) -> dict[str, Any]:
    selector = {"kind": kind, "msvc_runtime": msvc_runtime.lower(), "platform": platform}
    matches = [profile for profile in lock["profiles"] if profile["selector"] == selector]
    if len(matches) != 1:
        raise LockError(f"OCCT lock has no unique profile for selector {selector}")
    return matches[0]


def install_marker(lock: dict[str, Any], profile: dict[str, Any]) -> dict[str, str]:
    return {
        "archive_sha256": profile["archive"]["sha256"],
        "occt_version": lock["dependency"]["version"],
        "profile_id": profile["id"],
        "schema": LOCK_SCHEMA,
    }


def install_matches_lock(install_dir: Path, lock: dict[str, Any], profile: dict[str, Any]) -> bool:
    if not _install_ready(install_dir):
        return False
    marker_path = install_dir / INSTALL_MARKER
    try:
        marker = json.loads(marker_path.read_text(encoding="utf-8"), object_pairs_hook=_reject_duplicate_keys)
    except (OSError, json.JSONDecodeError, ValueError):
        return False
    profile_marker = install_dir / ARCHIVE_PROFILE_MARKER
    if not (
        marker == install_marker(lock, profile)
        and _installed_version(install_dir) == lock["dependency"]["version"]
        and profile_marker.is_file()
        and sha256_file(profile_marker) == profile["archive"]["profile_sha256"]
    ):
        return False
    try:
        _validate_profile_marker(_read_json_object(profile_marker), lock, profile)
    except LockError:
        return False
    return True


def restore_locked_install(
    profile: dict[str, Any],
    install_dir: Path,
    *,
    lock_path: Path = LOCK_PATH,
    base_url: str | None = None,
) -> None:
    """Restore exactly one locked archive, or fail without a source-build fallback."""

    lock = load_lock(lock_path)
    locked_profile = profile_by_id(lock, profile["id"])
    if profile != locked_profile:
        raise LockError(f"OCCT profile {profile['id']!r} does not match {lock_path}")
    if install_matches_lock(install_dir, lock, profile):
        print(f"OCCT locked install already present at {install_dir}")
        return

    archive = profile["archive"]
    resolved_base = (base_url or os.environ.get("WN_ARTIFACTS_BASE_URL") or DEFAULT_BASE_URL).rstrip("/")
    quoted_key = urllib.parse.quote(archive["object_key"].lstrip("/"), safe="/-_.~")
    url = f"{resolved_base}/{quoted_key}"
    print(f"Restoring locked OCCT profile {profile['id']} from {url}")
    install_dir.parent.mkdir(parents=True, exist_ok=True)
    with tempfile.TemporaryDirectory(prefix=".geometer-occt-lock-", dir=install_dir.parent) as temp_name:
        archive_path = Path(temp_name) / "occt-install.zip"
        _download_verified(url, archive_path, archive["sha256"], archive["size"])
        extracted = Path(temp_name) / "install"
        _extract_zip_safely(archive_path, extracted)
        _verify_extracted_install(extracted, lock, profile)
        (extracted / INSTALL_MARKER).write_bytes(canonical_json_bytes(install_marker(lock, profile)))
        _replace_install(extracted, install_dir)
    print(f"Restored locked OCCT profile {profile['id']} to {install_dir}")


def validate_lock(lock: Any) -> None:
    _require_object(lock, {"dependency", "profiles", "schema"}, "lock")
    if lock["schema"] != LOCK_SCHEMA:
        raise LockError(f"Unsupported OCCT lock schema: {lock['schema']!r}")
    dependency = lock["dependency"]
    _require_object(dependency, {"name", "source", "version"}, "dependency")
    if dependency["name"] != "occt" or not _nonempty(dependency["version"]):
        raise LockError("OCCT lock dependency identity is invalid")
    source = dependency["source"]
    _require_object(source, {"archive", "commit", "repository", "tag", "tag_object"}, "dependency.source")
    if not all(_nonempty(source[field]) for field in ("repository", "tag")):
        raise LockError("OCCT source repository and tag must be non-empty")
    if not GIT_SHA_PATTERN.fullmatch(source["commit"]) or not GIT_SHA_PATTERN.fullmatch(source["tag_object"]):
        raise LockError("OCCT source commit and tag object must be lowercase Git SHA-1 values")
    _validate_archive(source["archive"], "dependency.source.archive", require_profile=False)
    license_files = source["archive"].get("license_files")
    if (
        not isinstance(license_files, list)
        or not license_files
        or not all(_safe_relative_path(v) for v in license_files)
    ):
        raise LockError("OCCT source archive license_files must be safe relative paths")

    profiles = lock["profiles"]
    if not isinstance(profiles, list) or not profiles:
        raise LockError("OCCT lock profiles must be a non-empty array")
    ids: list[str] = []
    selectors: list[str] = []
    for index, profile in enumerate(profiles):
        location = f"profiles[{index}]"
        _require_object(profile, {"abi", "archive", "id", "qualification", "selector"}, location)
        if not _nonempty(profile["id"]) or not re.fullmatch(r"[a-z0-9][a-z0-9.-]*", profile["id"]):
            raise LockError(f"{location}.id is invalid")
        ids.append(profile["id"])
        selector = profile["selector"]
        _require_object(selector, {"kind", "msvc_runtime", "platform"}, f"{location}.selector")
        if not all(_nonempty(value) for value in selector.values()):
            raise LockError(f"{location}.selector values must be non-empty strings")
        if selector["kind"] not in {"native", "wasm"}:
            raise LockError(f"{location}.selector.kind is unsupported")
        if selector["msvc_runtime"] not in {"none", "dynamic", "static"}:
            raise LockError(f"{location}.selector.msvc_runtime is unsupported")
        _validate_abi(profile["abi"], selector, f"{location}.abi")
        selectors.append(json.dumps(selector, sort_keys=True))
        _validate_archive(profile["archive"], f"{location}.archive", require_profile=True)
        qualification = profile["qualification"]
        _require_object(
            qualification,
            {"release_tag", "source_revision", "workflow", "workflow_run_id"},
            f"{location}.qualification",
        )
        if not GIT_SHA_PATTERN.fullmatch(qualification["source_revision"]):
            raise LockError(f"{location}.qualification.source_revision is invalid")
        if not all(_nonempty(qualification[field]) for field in ("release_tag", "workflow", "workflow_run_id")):
            raise LockError(f"{location}.qualification fields must be non-empty strings")
    if ids != sorted(ids) or len(ids) != len(set(ids)):
        raise LockError("OCCT profile IDs must be sorted and unique")
    if len(selectors) != len(set(selectors)):
        raise LockError("OCCT profile selectors must be unique")


def _validate_archive(archive: Any, location: str, *, require_profile: bool) -> None:
    required = {"object_key", "sha256", "size"}
    if require_profile:
        required.add("profile_sha256")
    if not isinstance(archive, dict) or not required.issubset(archive) or set(archive) - (required | {"license_files"}):
        raise LockError(f"{location} has unexpected or missing fields")
    if not _safe_relative_path(archive["object_key"]):
        raise LockError(f"{location}.object_key must be a safe relative object key")
    if not SHA256_PATTERN.fullmatch(archive["sha256"]):
        raise LockError(f"{location}.sha256 is invalid")
    if archive["sha256"] not in archive["object_key"].split("/"):
        raise LockError(f"{location}.object_key must contain its full archive SHA-256 as a path component")
    if (
        not isinstance(archive["size"], int)
        or isinstance(archive["size"], bool)
        or not 0 < archive["size"] <= MAX_ARCHIVE_BYTES
    ):
        raise LockError(f"{location}.size must be between 1 and {MAX_ARCHIVE_BYTES} bytes")
    if require_profile and not SHA256_PATTERN.fullmatch(archive["profile_sha256"]):
        raise LockError(f"{location}.profile_sha256 is invalid")


def _download_verified(url: str, path: Path, expected_sha256: str, expected_size: int) -> None:
    head = urllib.request.Request(url, headers={"User-Agent": DOWNLOAD_USER_AGENT}, method="HEAD")
    try:
        with urllib.request.urlopen(head, timeout=DOWNLOAD_TIMEOUT_SECONDS) as response:
            content_length = response.headers.get("Content-Length")
    except urllib.error.HTTPError as exc:
        raise LockError(f"Locked OCCT object is unavailable ({exc.code}): {url}") from exc
    except (urllib.error.URLError, TimeoutError) as exc:
        reason = getattr(exc, "reason", exc)
        raise LockError(f"Locked OCCT object metadata request failed: {reason}") from exc
    if content_length is None:
        raise LockError(f"Locked OCCT object did not report Content-Length: {url}")
    try:
        remote_size = int(content_length)
    except ValueError as exc:
        raise LockError(f"Locked OCCT object reported invalid Content-Length: {content_length!r}") from exc
    if remote_size != expected_size:
        raise LockError(f"Locked OCCT object size mismatch: expected {expected_size}, got {remote_size}")

    request = urllib.request.Request(url, headers={"User-Agent": DOWNLOAD_USER_AGENT})
    digest = hashlib.sha256()
    size = 0
    try:
        with urllib.request.urlopen(request, timeout=DOWNLOAD_TIMEOUT_SECONDS) as response, path.open("wb") as output:
            while chunk := response.read(1024 * 1024):
                output.write(chunk)
                digest.update(chunk)
                size += len(chunk)
                if size > expected_size:
                    raise LockError(f"Locked OCCT object exceeds expected size {expected_size}: {url}")
    except urllib.error.HTTPError as exc:
        raise LockError(f"Locked OCCT object is unavailable ({exc.code}): {url}") from exc
    except (urllib.error.URLError, TimeoutError) as exc:
        reason = getattr(exc, "reason", exc)
        raise LockError(f"Locked OCCT object download failed: {reason}") from exc
    if size != expected_size:
        raise LockError(f"Locked OCCT object size mismatch: expected {expected_size}, got {size}")
    actual_sha256 = digest.hexdigest()
    if actual_sha256 != expected_sha256:
        raise LockError(f"Locked OCCT object SHA-256 mismatch: expected {expected_sha256}, got {actual_sha256}")


def _extract_zip_safely(archive_path: Path, destination: Path) -> None:
    destination.mkdir(parents=True)
    root = destination.resolve()
    with zipfile.ZipFile(archive_path) as archive:
        members = archive.infolist()
        if len(members) > MAX_ARCHIVE_ENTRIES:
            raise LockError(f"Locked OCCT archive has too many entries: {len(members)}")
        if sum(member.file_size for member in members) > MAX_EXPANDED_BYTES:
            raise LockError("Locked OCCT archive exceeds the expanded-size limit")
        names: set[str] = set()
        for member in members:
            member_path = Path(member.filename.replace("\\", "/"))
            if member.filename in names:
                raise LockError(f"Locked OCCT archive contains duplicate path {member.filename!r}")
            names.add(member.filename)
            if member_path.is_absolute() or ".." in member_path.parts:
                raise LockError(f"Locked OCCT archive contains unsafe path {member.filename!r}")
            if member.file_size > MAX_ENTRY_BYTES:
                raise LockError(f"Locked OCCT archive entry is too large: {member.filename!r}")
            if member.flag_bits & 0x1:
                raise LockError(f"Locked OCCT archive entry is encrypted: {member.filename!r}")
            if member.create_system == 3 and (member.external_attr >> 16) & 0o170000 == 0o120000:
                raise LockError(f"Locked OCCT archive entry is a symbolic link: {member.filename!r}")
            resolved = (destination / member_path).resolve()
            if resolved != root and root not in resolved.parents:
                raise LockError(f"Locked OCCT archive escapes its destination: {member.filename!r}")
        archive.extractall(destination)


def _verify_extracted_install(extracted: Path, lock: dict[str, Any], profile: dict[str, Any]) -> None:
    if not _install_ready(extracted):
        raise LockError("Locked OCCT archive does not contain an install tree")
    marker_path = extracted / ARCHIVE_PROFILE_MARKER
    if not marker_path.is_file():
        raise LockError("Locked OCCT archive is missing its internal profile marker")
    actual_profile_sha256 = sha256_file(marker_path)
    expected_profile_sha256 = profile["archive"]["profile_sha256"]
    if actual_profile_sha256 != expected_profile_sha256:
        raise LockError(
            "Locked OCCT internal profile marker mismatch: "
            f"expected {expected_profile_sha256}, got {actual_profile_sha256}"
        )
    _validate_profile_marker(_read_json_object(marker_path), lock, profile)
    expected_version = lock["dependency"]["version"]
    if _installed_version(extracted) != expected_version:
        raise LockError(f"Locked OCCT install does not report version {expected_version}")


def _replace_install(extracted: Path, install_dir: Path) -> None:
    install_dir.parent.mkdir(parents=True, exist_ok=True)
    if install_dir.exists():
        shutil.rmtree(install_dir)
    extracted.replace(install_dir)


def _installed_version(install_dir: Path) -> str | None:
    for version_path in (
        install_dir / "lib" / "cmake" / "opencascade" / "OpenCASCADEConfigVersion.cmake",
        install_dir / "cmake" / "OpenCASCADEConfigVersion.cmake",
    ):
        if version_path.is_file():
            match = re.search(
                r'set\s*\(\s*PACKAGE_VERSION\s+"([^"]+)"\s*\)',
                version_path.read_text(encoding="utf-8", errors="replace"),
            )
            if match:
                return match.group(1)
    return None


def _install_ready(install_dir: Path) -> bool:
    return (install_dir / "lib" / "cmake" / "opencascade" / "OpenCASCADEConfig.cmake").is_file() or (
        install_dir / "cmake" / "OpenCASCADEConfig.cmake"
    ).is_file()


def _require_object(value: Any, keys: set[str], location: str) -> None:
    if not isinstance(value, dict) or set(value) != keys:
        raise LockError(f"{location} must contain exactly {sorted(keys)}")


def _require_string_object(value: Any, location: str) -> None:
    if not isinstance(value, dict) or not value or not all(_nonempty(item) for item in value.values()):
        raise LockError(f"{location} must be a non-empty object of non-empty strings")


def _validate_abi(abi: Any, selector: dict[str, str], location: str) -> None:
    platform_name = selector["platform"]
    operating_system, _, architecture = platform_name.partition("-")
    common = {"architecture", "compiler", "library_type", "operating_system"}
    expected_keys = set(common)
    expected_values: dict[str, str] = {
        "architecture": "wasm32" if selector["kind"] == "wasm" else architecture,
        "library_type": "static",
        "operating_system": "wasm" if selector["kind"] == "wasm" else operating_system,
    }
    if operating_system in {"linux", "macos", "windows"}:
        expected_keys.add("cpp_runtime")
    if operating_system == "macos":
        expected_keys.add("deployment_target")
    if operating_system == "windows":
        expected_keys.add("runtime")
        expected_values["runtime"] = selector["msvc_runtime"]
    if not isinstance(abi, dict) or set(abi) != expected_keys:
        raise LockError(f"{location} must contain exactly {sorted(expected_keys)}")
    if not all(_nonempty(item) for item in abi.values()):
        raise LockError(f"{location} values must be non-empty strings")
    for name, expected in expected_values.items():
        if abi[name] != expected:
            raise LockError(f"{location}.{name} must be {expected!r}")
    allowed_compilers = {
        "linux": {"gcc-11"},
        "macos": {"apple-clang-17"},
        "windows": {"msvc-v143"},
        "wasm": {"emscripten-3.1.56"},
    }
    compiler_choices = allowed_compilers.get(expected_values["operating_system"])
    if compiler_choices is None or abi["compiler"] not in compiler_choices:
        raise LockError(f"{location}.compiler is unsupported")
    expected_runtime = {"linux": "libstdc++", "macos": "libc++", "windows": "msvc"}
    if operating_system in expected_runtime and abi["cpp_runtime"] != expected_runtime[operating_system]:
        raise LockError(f"{location}.cpp_runtime is invalid")
    if operating_system == "macos" and not re.fullmatch(r"[0-9]+\.[0-9]+", abi["deployment_target"]):
        raise LockError(f"{location}.deployment_target is invalid")


def _read_json_object(path: Path) -> dict[str, Any]:
    try:
        value = json.loads(path.read_text(encoding="utf-8"), object_pairs_hook=_reject_duplicate_keys)
    except (OSError, json.JSONDecodeError, ValueError) as exc:
        raise LockError(f"Invalid OCCT profile marker: {path}") from exc
    if not isinstance(value, dict):
        raise LockError(f"OCCT profile marker must be an object: {path}")
    return value


def _validate_profile_marker(marker: dict[str, Any], lock: dict[str, Any], profile: dict[str, Any]) -> None:
    base_keys = {
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
    source_keys = {"source_commit", "source_tag_object"}
    if frozenset(marker) not in {frozenset(base_keys), frozenset(base_keys | source_keys)}:
        raise LockError("Locked OCCT internal profile marker has unexpected fields")
    source = lock["dependency"]["source"]
    selector = profile["selector"]
    abi = profile["abi"]
    expected = {
        "config": "Release",
        "kind": selector["kind"],
        "library_type": "Static",
        "occt_repo": source["repository"],
        "occt_tag": source["tag"],
        "platform_tag": selector["platform"],
    }
    for name, value in expected.items():
        if marker[name] != value:
            raise LockError(f"Locked OCCT internal profile marker {name} mismatch")
    if not isinstance(marker["recipe_hash"], str) or not SHA256_PATTERN.fullmatch(marker["recipe_hash"]):
        raise LockError("Locked OCCT internal profile marker recipe_hash is invalid")
    expected_macos = abi.get("deployment_target")
    expected_emsdk = abi["compiler"].removeprefix("emscripten-") if selector["kind"] == "wasm" else None
    if marker["macos_deployment_target"] != expected_macos or marker["emsdk_version"] != expected_emsdk:
        raise LockError("Locked OCCT internal profile marker target metadata mismatch")
    compiler = abi["compiler"]
    if selector["kind"] == "wasm":
        expected_toolchain = None
    elif compiler == "msvc-v143":
        expected_toolchain = compiler + ("-crt-static" if selector["msvc_runtime"] == "static" else "")
    else:
        runtime = abi["cpp_runtime"].replace("+", "x")
        expected_toolchain = f"{compiler}-{runtime}-abi-default"
    if marker["toolchain_abi"] != expected_toolchain:
        raise LockError("Locked OCCT internal profile marker toolchain metadata mismatch")
    if source_keys.issubset(marker):
        if marker["source_commit"] != source["commit"] or marker["source_tag_object"] != source["tag_object"]:
            raise LockError("Locked OCCT internal profile marker source identity mismatch")


def _nonempty(value: Any) -> bool:
    return isinstance(value, str) and bool(value.strip())


def _safe_relative_path(value: Any) -> bool:
    if not _nonempty(value) or "\\" in value:
        return False
    path = Path(value)
    return not path.is_absolute() and ".." not in path.parts


def _reject_duplicate_keys(pairs: list[tuple[str, Any]]) -> dict[str, Any]:
    result: dict[str, Any] = {}
    for key, value in pairs:
        if key in result:
            raise ValueError(f"duplicate key {key!r}")
        result[key] = value
    return result
