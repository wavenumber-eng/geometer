from __future__ import annotations

import dataclasses
import hashlib
import hmac
import json
import os
import posixpath
import re
import shutil
import tempfile
import urllib.error
import urllib.parse
import urllib.request
from datetime import datetime, timezone
from pathlib import Path
from typing import Any


SCHEMA = "wavenumber.dependency_cache_manifest.a1"
LEGACY_SCHEMA = "geometry.occt_binary_cache_manifest.a0"
DEFAULT_PREFIX = "deps/v1/geometer/occt"
LEGACY_DEFAULT_PREFIX = "geometer/occt"
DEFAULT_PUBLIC_BASE_URL = "https://artifacts.wavenumber.net"
DEFAULT_REGION = "auto"
ARCHIVE_NAME = "occt-install.zip"
MANIFEST_NAME = "manifest.json"
SHA256_NAME = "occt-install.zip.sha256"
VALID_MODES = {"auto", "off", "only"}
PUBLIC_CACHE_DISABLE_VALUES = {"0", "false", "no", "off"}
DOWNLOAD_USER_AGENT = "wn-geometer-cache/1.0"


class CacheReadError(RuntimeError):
    """Raised when the remote cache cannot be checked reliably."""


@dataclasses.dataclass(frozen=True)
class CacheConfig:
    bucket: str
    endpoint_url: str
    access_key_id: str
    secret_access_key: str
    region: str
    prefix: str


@dataclasses.dataclass(frozen=True)
class PublicCacheConfig:
    base_url: str
    prefix: str


@dataclasses.dataclass(frozen=True)
class OcctCacheProfile:
    kind: str
    platform_tag: str
    config: str
    library_type: str
    occt_repo: str
    occt_tag: str
    recipe_hash: str
    macos_deployment_target: str | None = None
    emsdk_version: str | None = None

    @property
    def cache_key(self) -> str:
        parts = [
            "occt",
            self.kind,
            self.occt_tag,
            self.platform_tag,
            self.config,
            self.library_type,
        ]
        if self.macos_deployment_target:
            parts.append(f"macos-{self.macos_deployment_target}")
        if self.emsdk_version:
            parts.append(f"emsdk-{self.emsdk_version}")
        parts.append(f"recipe-{self.recipe_hash[:16]}")
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
    mode = (value or os.environ.get("GEOMETER_OCCT_BINARY") or os.environ.get("GEOMETER_OCCT_BINARY_CACHE") or "auto").strip().lower()
    if mode not in VALID_MODES:
        raise ValueError(f"Unsupported OCCT binary cache mode {mode!r}; expected one of {sorted(VALID_MODES)}")
    return mode


def config_from_env() -> CacheConfig | None:
    bucket = _env_value("GEOMETER_OCCT_CACHE_BUCKET", "R2_BUCKET")
    endpoint_url = _env_value("GEOMETER_OCCT_CACHE_ENDPOINT_URL", "R2_ENDPOINT_URL")
    access_key_id = _env_value("GEOMETER_OCCT_CACHE_ACCESS_KEY_ID", "R2_ACCESS_KEY_ID")
    secret_access_key = _env_value("GEOMETER_OCCT_CACHE_SECRET_ACCESS_KEY", "R2_SECRET_ACCESS_KEY")
    if not (bucket and endpoint_url and access_key_id and secret_access_key):
        return None
    region = _env_value("GEOMETER_OCCT_CACHE_REGION", "AWS_DEFAULT_REGION") or DEFAULT_REGION
    prefix = (_env_value("GEOMETER_OCCT_CACHE_PREFIX") or DEFAULT_PREFIX).strip("/")
    return CacheConfig(
        bucket=bucket,
        endpoint_url=_normalize_r2_endpoint_url(endpoint_url, bucket),
        access_key_id=access_key_id,
        secret_access_key=secret_access_key,
        region=region,
        prefix=prefix,
    )


def public_config_from_env() -> PublicCacheConfig | None:
    enabled = os.environ.get("GEOMETER_OCCT_PUBLIC_CACHE") or os.environ.get("GEOMETER_OCCT_CACHE_PUBLIC")
    if enabled and enabled.strip().lower() in PUBLIC_CACHE_DISABLE_VALUES:
        return None

    base_url = (
        _env_value(
            "GEOMETER_OCCT_CACHE_PUBLIC_BASE_URL",
            "GEOMETER_OCCT_PUBLIC_BASE_URL",
            "WN_ARTIFACTS_BASE_URL",
        )
        or DEFAULT_PUBLIC_BASE_URL
    )
    prefix = (_env_value("GEOMETER_OCCT_CACHE_PREFIX") or DEFAULT_PREFIX).strip("/")
    return PublicCacheConfig(base_url=base_url.rstrip("/"), prefix=prefix)


def recipe_hash(paths: list[Path], extra: dict[str, str]) -> str:
    digest = hashlib.sha256()
    digest.update(b"geometer-occt-cache-recipe-a0\n")
    for key, value in sorted(extra.items()):
        digest.update(f"extra:{key}={value}\n".encode("utf-8"))
    for path in sorted(paths, key=lambda item: item.as_posix()):
        if path.is_dir():
            for child in sorted((p for p in path.rglob("*") if p.is_file()), key=lambda item: item.as_posix()):
                _hash_file(digest, child, path.parent)
        elif path.exists():
            _hash_file(digest, path, path.parent)
        else:
            digest.update(f"missing:{path.as_posix()}\n".encode("utf-8"))
    return digest.hexdigest()


def install_ready(install_dir: Path) -> bool:
    return (
        (install_dir / "lib" / "cmake" / "opencascade" / "OpenCASCADEConfig.cmake").exists()
        or (install_dir / "cmake" / "OpenCASCADEConfig.cmake").exists()
    )


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


def install_matches_profile(install_dir: Path, profile: OcctCacheProfile) -> bool:
    if not install_ready(install_dir):
        return False
    expected = occt_version_from_tag(profile.occt_tag)
    actual = installed_occt_version(install_dir)
    return actual == expected


def restore_prebuilt_install(profile: OcctCacheProfile, install_dir: Path, *, mode: str | None = None) -> bool:
    selected_mode = mode_from_value(mode)
    if selected_mode == "off":
        print("OCCT binary cache disabled; building from source.")
        return False

    configs: list[PublicCacheConfig | CacheConfig] = []
    public_config = public_config_from_env()
    if public_config is not None:
        configs.append(public_config)
    r2_config = config_from_env()
    if r2_config is not None:
        configs.append(r2_config)

    if not configs:
        message = "OCCT binary cache is not configured; building from source."
        if selected_mode == "only":
            raise RuntimeError(message)
        print(message)
        return False

    prefix = ""
    selected_config: PublicCacheConfig | CacheConfig | None = None
    manifest_bytes = None
    read_errors: list[str] = []
    for config in configs:
        for candidate_prefix in object_prefix_candidates(config, profile):
            manifest_key = f"{candidate_prefix}/{MANIFEST_NAME}"
            print(f"Checking OCCT binary cache: {_cache_location(config, manifest_key)}")
            try:
                candidate_manifest = _get_cache_object(config, manifest_key)
            except CacheReadError as exc:
                read_errors.append(f"{_cache_kind(config)}: {exc}")
                break
            if candidate_manifest is not None:
                prefix = candidate_prefix
                selected_config = config
                manifest_bytes = candidate_manifest
                break
        if manifest_bytes is not None:
            break

    if manifest_bytes is None:
        message = f"OCCT binary cache miss for {profile.cache_key}"
        if read_errors:
            message = f"OCCT binary cache read failed for {profile.cache_key}: {'; '.join(read_errors)}"
        if selected_mode == "only":
            raise RuntimeError(message)
        print(f"{message}; building from source.")
        return False

    assert selected_config is not None
    manifest = json.loads(manifest_bytes.decode("utf-8"))
    validate_manifest(manifest, profile)
    archive_key = f"{prefix}/{ARCHIVE_NAME}"
    archive_bytes = _get_cache_object(selected_config, archive_key)
    if archive_bytes is None:
        raise RuntimeError(
            "OCCT binary cache manifest exists but archive is missing: "
            f"{_cache_location(selected_config, archive_key)}"
        )

    archive_sha256 = hashlib.sha256(archive_bytes).hexdigest()
    if archive_sha256 != manifest["archive"]["sha256"]:
        raise RuntimeError(
            "OCCT binary cache archive checksum mismatch: "
            f"expected {manifest['archive']['sha256']}, got {archive_sha256}"
        )

    with tempfile.TemporaryDirectory(prefix="geometer-occt-cache-") as tmp_name:
        archive_path = Path(tmp_name) / ARCHIVE_NAME
        archive_path.write_bytes(archive_bytes)
        extract_install_archive(archive_path, install_dir)

    print(f"Restored OCCT binary cache {profile.cache_key} to {install_dir}")
    return True


def upload_prebuilt_install(profile: OcctCacheProfile, install_dir: Path, *, out_dir: Path) -> Path:
    if not install_ready(install_dir):
        raise RuntimeError(f"OCCT install is not ready and cannot be cached: {install_dir}")
    config = config_from_env()
    if config is None:
        raise RuntimeError("OCCT binary cache upload requires R2/GEOMETER_OCCT_CACHE credentials in the environment.")

    package_dir = out_dir / profile.cache_key
    package_dir.mkdir(parents=True, exist_ok=True)
    archive_path = package_dir / ARCHIVE_NAME
    package_install_archive(install_dir, archive_path)

    archive_sha256 = sha256_file(archive_path)
    sha_path = package_dir / SHA256_NAME
    sha_path.write_text(f"{archive_sha256}  {ARCHIVE_NAME}\n", encoding="utf-8")
    manifest = build_manifest(profile, archive_path, archive_sha256)
    manifest_path = package_dir / MANIFEST_NAME
    manifest_path.write_text(json.dumps(manifest, indent=2, sort_keys=True) + "\n", encoding="utf-8")

    prefix = object_prefix(config, profile)
    print(f"Uploading OCCT binary cache to s3://{config.bucket}/{prefix}/")
    _r2_put_object(config, f"{prefix}/{ARCHIVE_NAME}", archive_path.read_bytes(), "application/zip")
    _r2_put_object(config, f"{prefix}/{SHA256_NAME}", sha_path.read_bytes(), "text/plain")
    _r2_put_object(config, f"{prefix}/{MANIFEST_NAME}", manifest_path.read_bytes(), "application/json")
    print(f"Uploaded OCCT binary cache {profile.cache_key}")
    return package_dir


def package_install_archive(install_dir: Path, archive_path: Path) -> None:
    archive_path.parent.mkdir(parents=True, exist_ok=True)
    if archive_path.exists():
        archive_path.unlink()
    base_name = str(archive_path.with_suffix(""))
    produced = Path(shutil.make_archive(base_name, "zip", root_dir=install_dir))
    if produced != archive_path:
        if archive_path.exists():
            archive_path.unlink()
        produced.replace(archive_path)


def extract_install_archive(archive_path: Path, install_dir: Path) -> None:
    parent = install_dir.parent
    parent.mkdir(parents=True, exist_ok=True)
    tmp_dir = parent / f".{install_dir.name}-extracting"
    if tmp_dir.exists():
        shutil.rmtree(tmp_dir)
    shutil.unpack_archive(str(archive_path), str(tmp_dir))
    if not install_ready(tmp_dir):
        shutil.rmtree(tmp_dir)
        raise RuntimeError(f"OCCT cache archive did not contain a valid install tree: {archive_path}")
    if install_dir.exists():
        shutil.rmtree(install_dir)
    tmp_dir.replace(install_dir)


def build_manifest(profile: OcctCacheProfile, archive_path: Path, archive_sha256: str) -> dict[str, Any]:
    return {
        "schema": SCHEMA,
        "project": "geometer",
        "dependency": {
            "name": "occt",
            "version": profile.occt_tag,
            "source_repo": profile.occt_repo,
        },
        "target": {
            "kind": profile.kind,
            "platform_tag": profile.platform_tag,
            "toolchain": "emscripten" if profile.kind == "wasm" else None,
        },
        "build": {
            "config": profile.config,
            "library_type": profile.library_type,
            "macos_deployment_target": profile.macos_deployment_target,
            "emsdk_version": profile.emsdk_version,
        },
        "cache_key": profile.cache_key,
        "kind": profile.kind,
        "platform_tag": profile.platform_tag,
        "config": profile.config,
        "library_type": profile.library_type,
        "occt": {
            "repo": profile.occt_repo,
            "tag": profile.occt_tag,
        },
        "recipe_hash": profile.recipe_hash,
        "macos_deployment_target": profile.macos_deployment_target,
        "emsdk_version": profile.emsdk_version,
        "archive": {
            "name": ARCHIVE_NAME,
            "sha256": archive_sha256,
            "size": archive_path.stat().st_size,
        },
        "producer": {
            "created_utc": datetime.now(timezone.utc).isoformat().replace("+00:00", "Z"),
            "github_repository": os.environ.get("GITHUB_REPOSITORY"),
            "github_sha": os.environ.get("GITHUB_SHA"),
            "github_run_id": os.environ.get("GITHUB_RUN_ID"),
        },
    }


def validate_manifest(manifest: dict[str, Any], profile: OcctCacheProfile) -> None:
    if manifest.get("schema") not in {SCHEMA, LEGACY_SCHEMA}:
        raise RuntimeError("OCCT binary cache manifest field 'schema' mismatch.")
    expected = {
        "cache_key": profile.cache_key,
        "kind": profile.kind,
        "platform_tag": profile.platform_tag,
        "config": profile.config,
        "library_type": profile.library_type,
        "recipe_hash": profile.recipe_hash,
    }
    for key, value in expected.items():
        if manifest.get(key) != value:
            raise RuntimeError(f"OCCT binary cache manifest field {key!r} mismatch.")
    archive = manifest.get("archive")
    if not isinstance(archive, dict) or archive.get("name") != ARCHIVE_NAME or not archive.get("sha256"):
        raise RuntimeError("OCCT binary cache manifest has an invalid archive block.")
    dependency = manifest.get("dependency")
    if isinstance(dependency, dict):
        if dependency.get("name") not in {None, "occt"}:
            raise RuntimeError("OCCT binary cache manifest dependency name mismatch.")
        if dependency.get("version") not in {None, profile.occt_tag}:
            raise RuntimeError("OCCT binary cache manifest dependency version mismatch.")


def object_prefix(config: CacheConfig | PublicCacheConfig, profile: OcctCacheProfile) -> str:
    return posixpath.join(config.prefix, profile.occt_tag, profile.kind, profile.platform_tag, profile.cache_key)


def object_prefix_candidates(config: CacheConfig | PublicCacheConfig, profile: OcctCacheProfile) -> list[str]:
    candidates = [
        object_prefix(config, profile),
        posixpath.join(config.prefix, profile.kind, profile.platform_tag, profile.cache_key),
    ]
    legacy_default = posixpath.join(LEGACY_DEFAULT_PREFIX, profile.kind, profile.platform_tag, profile.cache_key)
    if legacy_default not in candidates:
        candidates.append(legacy_default)
    return candidates


def sha256_file(path: Path) -> str:
    digest = hashlib.sha256()
    with path.open("rb") as handle:
        for chunk in iter(lambda: handle.read(1024 * 1024), b""):
            digest.update(chunk)
    return digest.hexdigest()


def _hash_file(digest: Any, path: Path, relative_to: Path) -> None:
    digest.update(f"file:{path.relative_to(relative_to).as_posix()}\n".encode("utf-8"))
    with path.open("rb") as handle:
        for chunk in iter(lambda: handle.read(1024 * 1024), b""):
            digest.update(chunk)
    digest.update(b"\n")


def _env_value(*names: str) -> str | None:
    for name in names:
        value = os.environ.get(name)
        if value:
            return value
    return None


def _slug(value: str) -> str:
    slug = []
    for ch in value.strip().lower():
        if ch.isalnum():
            slug.append(ch)
        elif ch in {"-", "_", "."}:
            slug.append(ch.replace("_", "-"))
        else:
            slug.append("-")
    return "".join(slug).strip("-") or "none"


def _cache_kind(config: CacheConfig | PublicCacheConfig) -> str:
    return "public" if isinstance(config, PublicCacheConfig) else "r2"


def _cache_location(config: CacheConfig | PublicCacheConfig, key: str) -> str:
    if isinstance(config, PublicCacheConfig):
        return _public_object_url(config, key)
    return f"s3://{config.bucket}/{key}"


def _get_cache_object(config: CacheConfig | PublicCacheConfig, key: str) -> bytes | None:
    if isinstance(config, PublicCacheConfig):
        return _public_get_object(config, key)
    return _r2_get_object(config, key)


def _public_object_url(config: PublicCacheConfig, key: str) -> str:
    quoted_key = urllib.parse.quote(key.lstrip("/"), safe="/-_.~")
    return f"{config.base_url}/{quoted_key}"


def _public_get_object(config: PublicCacheConfig, key: str) -> bytes | None:
    url = _public_object_url(config, key)
    request = urllib.request.Request(url, headers={"User-Agent": DOWNLOAD_USER_AGENT})
    try:
        with urllib.request.urlopen(request, timeout=120) as response:
            return response.read()
    except urllib.error.HTTPError as exc:
        if exc.code == 404:
            return None
        raise CacheReadError(f"GET {url} returned HTTP {exc.code}") from exc
    except urllib.error.URLError as exc:
        raise CacheReadError(f"GET {url} failed: {exc.reason}") from exc


def _normalize_r2_endpoint_url(endpoint_url: str, bucket: str) -> str:
    parsed = urllib.parse.urlparse(endpoint_url.rstrip("/"))
    path_parts = [part for part in parsed.path.split("/") if part]
    if path_parts and path_parts[-1] == bucket:
        path_parts = path_parts[:-1]
    normalized_path = "/" + "/".join(path_parts) if path_parts else ""
    return urllib.parse.urlunparse((parsed.scheme, parsed.netloc, normalized_path, "", "", ""))


def _r2_get_object(config: CacheConfig, key: str) -> bytes | None:
    try:
        return _r2_request(config, "GET", key)
    except urllib.error.HTTPError as exc:
        if exc.code in {403, 404}:
            return None
        raise CacheReadError(f"GET {key} returned HTTP {exc.code}") from exc
    except urllib.error.URLError as exc:
        raise CacheReadError(f"GET {key} failed: {exc.reason}") from exc


def _r2_put_object(config: CacheConfig, key: str, body: bytes, content_type: str) -> None:
    _r2_request(config, "PUT", key, body=body, content_type=content_type)


def _r2_request(config: CacheConfig, method: str, key: str, *, body: bytes = b"", content_type: str | None = None) -> bytes:
    parsed = urllib.parse.urlparse(config.endpoint_url)
    if not parsed.scheme or not parsed.netloc:
        raise RuntimeError(f"Invalid R2 endpoint URL: {config.endpoint_url}")

    now = datetime.now(timezone.utc)
    date_stamp = now.strftime("%Y%m%d")
    amz_date = now.strftime("%Y%m%dT%H%M%SZ")
    object_path = f"{parsed.path.rstrip('/')}/{config.bucket}/{key}"
    canonical_uri = urllib.parse.quote(object_path, safe="/-_.~")
    url = urllib.parse.urlunparse((parsed.scheme, parsed.netloc, canonical_uri, "", "", ""))
    payload_hash = hashlib.sha256(body).hexdigest()

    headers = {
        "host": parsed.netloc,
        "x-amz-content-sha256": payload_hash,
        "x-amz-date": amz_date,
    }
    if content_type is not None:
        headers["content-type"] = content_type

    signed_header_names = sorted(headers)
    canonical_headers = "".join(f"{name}:{headers[name].strip()}\n" for name in signed_header_names)
    signed_headers = ";".join(signed_header_names)
    canonical_request = "\n".join(
        [
            method,
            canonical_uri,
            "",
            canonical_headers,
            signed_headers,
            payload_hash,
        ]
    )
    credential_scope = f"{date_stamp}/{config.region}/s3/aws4_request"
    string_to_sign = "\n".join(
        [
            "AWS4-HMAC-SHA256",
            amz_date,
            credential_scope,
            hashlib.sha256(canonical_request.encode("utf-8")).hexdigest(),
        ]
    )
    signing_key = _aws_v4_signing_key(config.secret_access_key, date_stamp, config.region, "s3")
    signature = hmac.new(signing_key, string_to_sign.encode("utf-8"), hashlib.sha256).hexdigest()
    headers["authorization"] = (
        "AWS4-HMAC-SHA256 "
        f"Credential={config.access_key_id}/{credential_scope}, "
        f"SignedHeaders={signed_headers}, "
        f"Signature={signature}"
    )

    request_headers = {name.title(): value for name, value in headers.items() if name != "host"}
    request = urllib.request.Request(
        url,
        data=body if method in {"PUT", "POST"} else None,
        headers=request_headers,
        method=method,
    )
    with urllib.request.urlopen(request, timeout=120) as response:
        return response.read()


def _aws_v4_signing_key(secret_key: str, date_stamp: str, region: str, service: str) -> bytes:
    key_date = hmac.new(("AWS4" + secret_key).encode("utf-8"), date_stamp.encode("utf-8"), hashlib.sha256).digest()
    key_region = hmac.new(key_date, region.encode("utf-8"), hashlib.sha256).digest()
    key_service = hmac.new(key_region, service.encode("utf-8"), hashlib.sha256).digest()
    return hmac.new(key_service, b"aws4_request", hashlib.sha256).digest()


def main() -> int:
    print("This module is imported by build_occt.py and build_wasm.py.")
    print("It does not expose a standalone command-line interface yet.")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
