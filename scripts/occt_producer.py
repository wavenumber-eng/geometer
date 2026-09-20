from __future__ import annotations

import dataclasses
import hashlib
import json
import os
import posixpath
import re
import shutil
import subprocess
import tempfile
import urllib.error
import urllib.parse
import urllib.request
from datetime import datetime, timezone
from pathlib import Path

import r2_store


CANDIDATE_SCHEMA = "wn.geometer.occt_candidate.a0"
DEFAULT_REGION = "auto"
ARCHIVE_NAME = "occt-install.zip"
MANIFEST_NAME = "manifest.json"
SHA256_NAME = "occt-install.zip.sha256"
INSTALL_PROFILE_NAME = ".geometer-occt-profile.json"
VALID_MODES = {"auto", "off", "only"}


CacheConfig = r2_store.R2Config


@dataclasses.dataclass(frozen=True)
class CMakeDefinition:
    """One CMake definition and its path-independent producer-evidence value."""

    name: str
    value: str
    include_in_recipe: bool = True
    recipe_value: str | None = None

    @property
    def semantic_value(self) -> str:
        return self.recipe_value if self.recipe_value is not None else self.value


@dataclasses.dataclass(frozen=True)
class OcctBuildProfile:
    """Producer evidence; never used to select a consumer archive."""

    kind: str
    platform_tag: str
    config: str
    library_type: str
    occt_repo: str
    occt_tag: str
    source_commit: str
    source_tag_object: str
    recipe_hash: str
    toolchain_abi: str | None = None
    macos_deployment_target: str | None = None
    emsdk_version: str | None = None

    @property
    def evidence_id(self) -> str:
        parts = [self.kind, self.platform_tag, self.occt_tag, self.recipe_hash[:16]]
        return "-".join(_slug(part) for part in parts)


def load_dotenv(root: Path) -> None:
    env_path = root / ".env"
    if not env_path.exists():
        return
    for raw_line in env_path.read_text(encoding="utf-8").splitlines():
        line = raw_line.strip()
        if not line or line.startswith("#") or "=" not in line:
            continue
        name, value = line.split("=", 1)
        name = name.strip()
        value = value.strip()
        if not name or name in os.environ:
            continue
        if (value.startswith('"') and value.endswith('"')) or (value.startswith("'") and value.endswith("'")):
            value = value[1:-1]
        os.environ[name] = value


def mode_from_value(value: str | None) -> str:
    mode = (
        (value or os.environ.get("GEOMETER_OCCT_BINARY") or os.environ.get("GEOMETER_OCCT_BINARY_CACHE") or "auto")
        .strip()
        .lower()
    )
    if mode not in VALID_MODES:
        raise ValueError(f"Unsupported OCCT mode {mode!r}; expected one of {sorted(VALID_MODES)}")
    return mode


def config_from_env() -> CacheConfig | None:
    bucket = _env_value("GEOMETER_OCCT_CACHE_BUCKET", "R2_BUCKET")
    endpoint_url = _env_value("GEOMETER_OCCT_CACHE_ENDPOINT_URL", "R2_ENDPOINT_URL")
    access_key_id = _env_value("GEOMETER_OCCT_CACHE_ACCESS_KEY_ID", "R2_ACCESS_KEY_ID")
    secret_access_key = _env_value("GEOMETER_OCCT_CACHE_SECRET_ACCESS_KEY", "R2_SECRET_ACCESS_KEY")
    if not (bucket and endpoint_url and access_key_id and secret_access_key):
        return None
    return CacheConfig(
        bucket=bucket,
        endpoint_url=_normalize_r2_endpoint_url(endpoint_url, bucket),
        access_key_id=access_key_id,
        secret_access_key=secret_access_key,
        region=_env_value("GEOMETER_OCCT_CACHE_REGION", "AWS_DEFAULT_REGION") or DEFAULT_REGION,
    )


def cmake_definition_args(definitions: tuple[CMakeDefinition, ...]) -> list[str]:
    _validated_cmake_definitions(definitions)
    return [f"-D{definition.name}={definition.value}" for definition in definitions]


def semantic_recipe_hash(
    recipe_schema: str,
    definitions: tuple[CMakeDefinition, ...],
    inputs: dict[str, str],
) -> str:
    """Record byte-relevant source-build choices as producer evidence."""

    _validated_cmake_definitions(definitions)
    payload = {
        "schema": "geometer-occt-producer-recipe-a0",
        "recipe_schema": recipe_schema,
        "cmake_definitions": dict(
            sorted(
                (definition.name, definition.semantic_value)
                for definition in definitions
                if definition.include_in_recipe
            )
        ),
        "inputs": dict(sorted(inputs.items())),
    }
    return hashlib.sha256((json.dumps(payload, separators=(",", ":"), sort_keys=True) + "\n").encode()).hexdigest()


def directory_content_hash(path: Path) -> str:
    if not path.is_dir():
        raise RuntimeError(f"Producer input directory is missing: {path}")
    digest = hashlib.sha256(b"geometer-producer-directory-content-a0\n")
    for child in sorted(
        (candidate for candidate in path.rglob("*") if candidate.is_file()), key=lambda p: p.as_posix()
    ):
        relative = child.relative_to(path).as_posix().encode()
        digest.update(len(relative).to_bytes(8, "big"))
        digest.update(relative)
        digest.update(child.stat().st_size.to_bytes(8, "big"))
        with child.open("rb") as handle:
            for chunk in iter(lambda: handle.read(1024 * 1024), b""):
                digest.update(chunk)
    return digest.hexdigest()


def source_identity(checkout: Path, expected_tag: str) -> tuple[str, str]:
    """Return the exact tag object and peeled commit for a checked-out release tag."""

    if not (checkout / ".git").is_dir():
        raise RuntimeError(f"OCCT source checkout is missing Git metadata: {checkout}")
    try:
        tag_object = subprocess.check_output(
            ["git", "-C", str(checkout), "rev-parse", f"refs/tags/{expected_tag}"],
            text=True,
            stderr=subprocess.STDOUT,
        ).strip()
        commit = subprocess.check_output(
            ["git", "-C", str(checkout), "rev-parse", f"refs/tags/{expected_tag}^{{commit}}"],
            text=True,
            stderr=subprocess.STDOUT,
        ).strip()
        head = subprocess.check_output(
            ["git", "-C", str(checkout), "rev-parse", "HEAD"],
            text=True,
            stderr=subprocess.STDOUT,
        ).strip()
    except (OSError, subprocess.CalledProcessError) as exc:
        raise RuntimeError(f"Could not resolve exact OCCT source identity at {checkout}") from exc
    if not all(re.fullmatch(r"[0-9a-f]{40}", value) for value in (tag_object, commit, head)):
        raise RuntimeError(f"OCCT source identity at {checkout} is not canonical SHA-1")
    if head != commit:
        raise RuntimeError(f"OCCT source HEAD {head} does not match {expected_tag} commit {commit}")
    return tag_object, commit


def require_locked_source(profile: OcctBuildProfile, lock: dict[str, object]) -> None:
    """Reject publication unless producer source evidence matches the checked-in lock."""

    dependency = lock["dependency"]
    if not isinstance(dependency, dict) or not isinstance(dependency.get("source"), dict):
        raise RuntimeError("OCCT lock dependency source is invalid")
    source = dependency["source"]
    actual = {
        "repository": profile.occt_repo,
        "tag": profile.occt_tag,
        "tag_object": profile.source_tag_object,
        "commit": profile.source_commit,
    }
    expected = {name: source[name] for name in actual}
    if actual != expected:
        raise RuntimeError(f"OCCT producer source does not match the checked-in lock: {actual!r}")


def require_locked_profile(profile: OcctBuildProfile, locked_profile: dict[str, object]) -> None:
    """Bind producer bytes to the explicitly selected lock profile."""

    selector = locked_profile["selector"]
    abi = locked_profile["abi"]
    if not isinstance(selector, dict) or not isinstance(abi, dict):
        raise RuntimeError("Selected OCCT lock profile is invalid")
    compiler = abi["compiler"]
    if selector["kind"] == "wasm":
        expected_toolchain = None
        expected_emsdk = str(compiler).removeprefix("emscripten-")
    elif compiler == "msvc-v143":
        expected_toolchain = str(compiler) + ("-crt-static" if selector["msvc_runtime"] == "static" else "")
        expected_emsdk = None
    else:
        runtime = str(abi["cpp_runtime"]).replace("+", "x")
        expected_toolchain = f"{compiler}-{runtime}-abi-default"
        expected_emsdk = None
    actual = {
        "config": profile.config,
        "emsdk_version": profile.emsdk_version,
        "kind": profile.kind,
        "library_type": profile.library_type,
        "macos_deployment_target": profile.macos_deployment_target,
        "platform_tag": profile.platform_tag,
        "toolchain_abi": profile.toolchain_abi,
    }
    expected = {
        "config": "Release",
        "emsdk_version": expected_emsdk,
        "kind": selector["kind"],
        "library_type": "Static",
        "macos_deployment_target": abi.get("deployment_target"),
        "platform_tag": selector["platform"],
        "toolchain_abi": expected_toolchain,
    }
    if actual != expected:
        raise RuntimeError(f"OCCT producer does not match selected lock profile: {actual!r}")


def install_ready(install_dir: Path) -> bool:
    return (install_dir / "lib" / "cmake" / "opencascade" / "OpenCASCADEConfig.cmake").is_file() or (
        install_dir / "cmake" / "OpenCASCADEConfig.cmake"
    ).is_file()


def occt_version_from_tag(tag: str) -> str:
    return tag.removeprefix("V").replace("_", ".")


def installed_occt_version(install_dir: Path) -> str | None:
    for version_file in (
        install_dir / "lib" / "cmake" / "opencascade" / "OpenCASCADEConfigVersion.cmake",
        install_dir / "cmake" / "OpenCASCADEConfigVersion.cmake",
    ):
        if not version_file.exists():
            continue
        match = re.search(
            r'set\s*\(\s*PACKAGE_VERSION\s+"([^"]+)"\s*\)',
            version_file.read_text(encoding="utf-8", errors="replace"),
        )
        if match:
            return match.group(1)
    return None


def install_profile_identity(profile: OcctBuildProfile) -> dict[str, str | None]:
    return dataclasses.asdict(profile)


def write_install_profile(install_dir: Path, profile: OcctBuildProfile) -> None:
    if not install_ready(install_dir):
        raise RuntimeError(f"OCCT install is not ready for producer evidence: {install_dir}")
    (install_dir / INSTALL_PROFILE_NAME).write_bytes(
        (json.dumps(install_profile_identity(profile), indent=2, sort_keys=True) + "\n").encode("utf-8")
    )


def install_matches_profile(install_dir: Path, profile: OcctBuildProfile) -> bool:
    if not install_ready(install_dir) or installed_occt_version(install_dir) != occt_version_from_tag(profile.occt_tag):
        return False
    try:
        marker = json.loads((install_dir / INSTALL_PROFILE_NAME).read_text(encoding="utf-8"))
    except (OSError, json.JSONDecodeError):
        return False
    return marker == install_profile_identity(profile)


def upload_prebuilt_install(
    profile: OcctBuildProfile,
    install_dir: Path,
    *,
    out_dir: Path,
    locked_profile_id: str,
) -> Path:
    """Package and publish one source-built candidate (local operator convenience)."""

    package_dir = package_prebuilt_install(
        profile,
        install_dir,
        out_dir=out_dir,
        locked_profile_id=locked_profile_id,
    )
    publish_candidate(package_dir)
    return package_dir


def package_prebuilt_install(
    profile: OcctBuildProfile,
    install_dir: Path,
    *,
    out_dir: Path,
    locked_profile_id: str,
) -> Path:
    """Create a closed candidate package without loading publication credentials."""

    if not install_matches_profile(install_dir, profile):
        raise RuntimeError(f"OCCT install does not match its producer profile: {install_dir}")
    # Imported lazily so this producer-only module remains independent of consumer selection.
    import occt_lock

    lock = occt_lock.load_lock()
    require_locked_source(profile, lock)
    require_locked_profile(profile, occt_lock.profile_by_id(lock, locked_profile_id))
    if not re.fullmatch(r"[a-z0-9][a-z0-9.-]*", locked_profile_id):
        raise ValueError(f"Invalid locked OCCT profile ID: {locked_profile_id!r}")
    package_dir = out_dir / locked_profile_id
    package_dir.mkdir(parents=True, exist_ok=True)
    archive_path = package_dir / ARCHIVE_NAME
    package_install_archive(install_dir, archive_path)
    archive_sha256 = sha256_file(archive_path)
    archive_key = posixpath.join("dependencies", "occt", locked_profile_id, archive_sha256, ARCHIVE_NAME)
    (package_dir / SHA256_NAME).write_bytes(f"{archive_sha256}  {ARCHIVE_NAME}\n".encode("ascii"))
    manifest = {
        "archive": {
            "name": ARCHIVE_NAME,
            "object_key": archive_key,
            "sha256": archive_sha256,
            "size": archive_path.stat().st_size,
        },
        "build": install_profile_identity(profile),
        "profile_id": locked_profile_id,
        "profile_sha256": sha256_file(install_dir / INSTALL_PROFILE_NAME),
        "producer": {
            "created_utc": datetime.now(timezone.utc).isoformat().replace("+00:00", "Z"),
            "github_repository": os.environ.get("GITHUB_REPOSITORY"),
            "github_run_id": os.environ.get("GITHUB_RUN_ID"),
            "github_sha": os.environ.get("GITHUB_SHA"),
        },
        "schema": CANDIDATE_SCHEMA,
    }
    (package_dir / MANIFEST_NAME).write_bytes((json.dumps(manifest, indent=2, sort_keys=True) + "\n").encode("utf-8"))

    print(f"Packaged OCCT candidate {locked_profile_id} ({archive_sha256})")
    return package_dir


def _validated_candidate(package_dir: Path) -> tuple[Path, str, str, str]:
    """Validate an untrusted handoff before publication credentials are read."""

    manifest_path = package_dir / MANIFEST_NAME
    archive_path = package_dir / ARCHIVE_NAME
    if not manifest_path.is_file() or manifest_path.stat().st_size > 1024 * 1024:
        raise RuntimeError(f"OCCT candidate manifest is missing or too large: {manifest_path}")
    try:
        raw_manifest = manifest_path.read_bytes()
        manifest = json.loads(raw_manifest, object_pairs_hook=_reject_duplicate_keys)
    except (OSError, UnicodeDecodeError, json.JSONDecodeError, ValueError) as exc:
        raise RuntimeError(f"Invalid OCCT candidate manifest: {manifest_path}") from exc
    if not isinstance(manifest, dict) or set(manifest) != {
        "archive",
        "build",
        "producer",
        "profile_id",
        "profile_sha256",
        "schema",
    }:
        raise RuntimeError(f"OCCT candidate manifest has an invalid shape: {manifest_path}")
    if manifest["schema"] != CANDIDATE_SCHEMA or not isinstance(manifest["build"], dict):
        raise RuntimeError(f"OCCT candidate manifest has an unsupported schema: {manifest_path}")
    if raw_manifest != (json.dumps(manifest, indent=2, sort_keys=True) + "\n").encode("utf-8"):
        raise RuntimeError(f"OCCT candidate manifest is not canonical JSON: {manifest_path}")
    try:
        profile = OcctBuildProfile(**manifest["build"])
    except TypeError as exc:
        raise RuntimeError(f"OCCT candidate build evidence is invalid: {manifest_path}") from exc
    _validate_build_profile(profile)
    _validate_producer_identity(manifest["producer"])
    profile_id = manifest["profile_id"]
    if not isinstance(profile_id, str) or not re.fullmatch(r"[a-z0-9][a-z0-9.-]*", profile_id):
        raise RuntimeError(f"OCCT candidate profile ID is invalid: {manifest_path}")

    import occt_lock

    lock = occt_lock.load_lock()
    require_locked_source(profile, lock)
    require_locked_profile(profile, occt_lock.profile_by_id(lock, profile_id))
    if not archive_path.is_file():
        raise RuntimeError(f"OCCT candidate archive is missing: {archive_path}")
    archive_size = archive_path.stat().st_size
    if not 0 < archive_size <= occt_lock.MAX_ARCHIVE_BYTES:
        raise RuntimeError(f"OCCT candidate archive exceeds the size limit: {archive_size}")
    archive_sha256 = sha256_file(archive_path)
    expected_key = posixpath.join("dependencies", "occt", profile_id, archive_sha256, ARCHIVE_NAME)
    archive = manifest["archive"]
    expected_archive = {
        "name": ARCHIVE_NAME,
        "object_key": expected_key,
        "sha256": archive_sha256,
        "size": archive_size,
    }
    if archive != expected_archive:
        raise RuntimeError(f"OCCT candidate archive evidence does not match its bytes: {manifest_path}")
    checksum_path = package_dir / SHA256_NAME
    expected_checksum = f"{archive_sha256}  {ARCHIVE_NAME}\n".encode("ascii")
    try:
        actual_checksum = checksum_path.read_bytes()
    except OSError as exc:
        raise RuntimeError(f"OCCT candidate checksum evidence is missing: {checksum_path}") from exc
    if actual_checksum != expected_checksum:
        raise RuntimeError(f"OCCT candidate checksum evidence is invalid: {checksum_path}")
    marker_bytes = _archive_profile_bytes(archive_path)
    marker_sha256 = hashlib.sha256(marker_bytes).hexdigest()
    if manifest["profile_sha256"] != marker_sha256:
        raise RuntimeError(f"OCCT candidate profile evidence does not match its archive: {manifest_path}")
    expected_marker = (json.dumps(install_profile_identity(profile), indent=2, sort_keys=True) + "\n").encode("utf-8")
    if marker_bytes != expected_marker:
        raise RuntimeError(f"OCCT candidate profile marker does not match build evidence: {manifest_path}")
    with tempfile.TemporaryDirectory(prefix="geometer-occt-publish-") as temp_name:
        extracted = Path(temp_name) / "install"
        occt_lock._extract_zip_safely(archive_path, extracted)
        if not install_matches_profile(extracted, profile):
            raise RuntimeError("OCCT candidate archive does not contain the reported install/profile/version")

    return archive_path, expected_key, profile_id, archive_sha256


def publish_candidate(package_dir: Path) -> None:
    """Validate an untrusted candidate package and conditionally create its R2 object."""

    archive_path, expected_key, profile_id, archive_sha256 = _validated_candidate(package_dir)
    config = config_from_env()
    if config is None:
        raise RuntimeError("OCCT candidate publication requires R2 credentials in the environment")

    archive_bytes = archive_path.read_bytes()
    print(f"Creating immutable OCCT candidate s3://{config.bucket}/{expected_key}")
    try:
        _r2_put_object_if_absent(config, expected_key, archive_bytes, "application/zip")
    except urllib.error.HTTPError as exc:
        if exc.code != 412:
            raise RuntimeError(f"R2 conditional create {expected_key} returned HTTP {exc.code}") from exc
        existing = _r2_get_object(config, expected_key)
        if existing != archive_bytes:
            raise RuntimeError(f"Immutable OCCT candidate key is occupied by different bytes: {expected_key}") from exc
        print(f"Immutable OCCT candidate already exists with identical bytes: {expected_key}")
    print(f"Published OCCT candidate {profile_id} ({archive_sha256})")


def package_install_archive(install_dir: Path, archive_path: Path) -> None:
    archive_path.parent.mkdir(parents=True, exist_ok=True)
    if archive_path.exists():
        archive_path.unlink()
    produced = Path(shutil.make_archive(str(archive_path.with_suffix("")), "zip", root_dir=install_dir))
    if produced != archive_path:
        produced.replace(archive_path)


def _archive_profile_bytes(archive_path: Path) -> bytes:
    import zipfile

    with zipfile.ZipFile(archive_path) as archive:
        matches = [item for item in archive.infolist() if item.filename == INSTALL_PROFILE_NAME]
        if len(matches) != 1 or matches[0].file_size > 64 * 1024:
            raise RuntimeError("OCCT candidate archive has no unique bounded profile marker")
        return archive.read(matches[0])


def _validate_build_profile(profile: OcctBuildProfile) -> None:
    required_strings = (
        profile.kind,
        profile.platform_tag,
        profile.config,
        profile.library_type,
        profile.occt_repo,
        profile.occt_tag,
        profile.source_commit,
        profile.source_tag_object,
        profile.recipe_hash,
    )
    if not all(isinstance(value, str) and value for value in required_strings):
        raise RuntimeError("OCCT candidate build profile fields must be non-empty strings")
    optional_strings = (profile.toolchain_abi, profile.macos_deployment_target, profile.emsdk_version)
    if not all(value is None or isinstance(value, str) for value in optional_strings):
        raise RuntimeError("OCCT candidate optional build profile fields must be strings or null")
    if not re.fullmatch(r"[0-9a-f]{64}", profile.recipe_hash):
        raise RuntimeError("OCCT candidate recipe hash is invalid")
    if not re.fullmatch(r"[0-9a-f]{40}", profile.source_commit) or not re.fullmatch(
        r"[0-9a-f]{40}", profile.source_tag_object
    ):
        raise RuntimeError("OCCT candidate source identity is invalid")


def _validate_producer_identity(producer: object) -> None:
    if not isinstance(producer, dict) or set(producer) != {
        "created_utc",
        "github_repository",
        "github_run_id",
        "github_sha",
    }:
        raise RuntimeError("OCCT candidate producer identity has an invalid shape")
    expected = {
        "github_repository": os.environ.get("GITHUB_REPOSITORY"),
        "github_run_id": os.environ.get("GITHUB_RUN_ID"),
        "github_sha": os.environ.get("GITHUB_SHA"),
    }
    if not all(isinstance(value, str) and value for value in expected.values()):
        raise RuntimeError("OCCT candidate publication requires GitHub run identity")
    if any(producer[name] != value for name, value in expected.items()):
        raise RuntimeError("OCCT candidate producer identity does not match the publishing run")
    created = producer["created_utc"]
    if not isinstance(created, str):
        raise RuntimeError("OCCT candidate producer timestamp is invalid")
    try:
        datetime.fromisoformat(created.replace("Z", "+00:00"))
    except ValueError as exc:
        raise RuntimeError("OCCT candidate producer timestamp is invalid") from exc


def _reject_duplicate_keys(pairs: list[tuple[str, object]]) -> dict[str, object]:
    result: dict[str, object] = {}
    for key, value in pairs:
        if key in result:
            raise ValueError(f"duplicate key {key!r}")
        result[key] = value
    return result


def sha256_file(path: Path) -> str:
    digest = hashlib.sha256()
    with path.open("rb") as handle:
        for chunk in iter(lambda: handle.read(1024 * 1024), b""):
            digest.update(chunk)
    return digest.hexdigest()


def _validated_cmake_definitions(definitions: tuple[CMakeDefinition, ...]) -> None:
    names = [definition.name for definition in definitions]
    if any(not name or "=" in name for name in names):
        raise ValueError("CMake definition names must be non-empty and cannot contain '='")
    if len(names) != len(set(names)):
        raise ValueError("CMake definitions must have unique names")
    if any(not definition.include_in_recipe and definition.recipe_value is not None for definition in definitions):
        raise ValueError("Excluded CMake definitions cannot provide a recipe value")


def _env_value(*names: str) -> str | None:
    return next((value for name in names if (value := os.environ.get(name))), None)


def _slug(value: str) -> str:
    return re.sub(r"[^a-z0-9.-]+", "-", value.strip().lower().replace("_", "-")).strip("-") or "none"


def _normalize_r2_endpoint_url(endpoint_url: str, bucket: str) -> str:
    return r2_store.normalize_endpoint_url(endpoint_url, bucket)


def _r2_get_object(config: CacheConfig, key: str) -> bytes | None:
    return r2_store.get_object(config, key)


def _r2_put_object_if_absent(config: CacheConfig, key: str, body: bytes, content_type: str) -> None:
    r2_store.put_object_if_absent(config, key, body, content_type)


def _r2_request(
    config: CacheConfig,
    method: str,
    key: str,
    *,
    body: bytes = b"",
    content_type: str | None = None,
    extra_headers: dict[str, str] | None = None,
) -> bytes:
    return r2_store.request(
        config,
        method,
        key,
        body=body,
        content_type=content_type,
        extra_headers=extra_headers,
    )


def _aws_v4_signing_key(secret_key: str, date_stamp: str, region: str, service: str) -> bytes:
    return r2_store.aws_v4_signing_key(secret_key, date_stamp, region, service)
