"""Small SigV4 client for immutable objects in the Wavenumber R2 bucket."""

from __future__ import annotations

from dataclasses import dataclass
from datetime import datetime, timezone
import hashlib
import hmac
import os
import re
import urllib.error
import urllib.parse
import urllib.request


DEFAULT_REGION = "auto"


@dataclass(frozen=True)
class R2Config:
    bucket: str
    endpoint_url: str
    access_key_id: str
    secret_access_key: str
    region: str = DEFAULT_REGION


def config_from_env() -> R2Config | None:
    values = {
        "bucket": os.environ.get("R2_BUCKET"),
        "endpoint_url": os.environ.get("R2_ENDPOINT_URL"),
        "access_key_id": os.environ.get("R2_ACCESS_KEY_ID"),
        "secret_access_key": os.environ.get("R2_SECRET_ACCESS_KEY"),
    }
    if not all(values.values()):
        return None
    bucket = str(values["bucket"])
    return R2Config(
        bucket=bucket,
        endpoint_url=normalize_endpoint_url(str(values["endpoint_url"]), bucket),
        access_key_id=str(values["access_key_id"]),
        secret_access_key=str(values["secret_access_key"]),
        region=os.environ.get("AWS_DEFAULT_REGION") or DEFAULT_REGION,
    )


def normalize_endpoint_url(endpoint_url: str, bucket: str) -> str:
    parsed = urllib.parse.urlparse(endpoint_url.rstrip("/"))
    path_parts = [part for part in parsed.path.split("/") if part]
    if path_parts and path_parts[-1] == bucket:
        path_parts.pop()
    normalized_path = "/" + "/".join(path_parts) if path_parts else ""
    return urllib.parse.urlunparse((parsed.scheme, parsed.netloc, normalized_path, "", "", ""))


def get_object(config: R2Config, key: str) -> bytes | None:
    try:
        return request(config, "GET", key)
    except urllib.error.HTTPError as error:
        if error.code in {403, 404}:
            return None
        raise RuntimeError(f"R2 GET {key} returned HTTP {error.code}") from error


def put_object_if_absent(config: R2Config, key: str, body: bytes, content_type: str) -> None:
    request(
        config,
        "PUT",
        key,
        body=body,
        content_type=content_type,
        extra_headers={"if-none-match": "*"},
    )


def create_or_verify(config: R2Config, key: str, body: bytes, content_type: str) -> bool:
    """Create one immutable object, or verify exact bytes already occupy its key."""

    try:
        put_object_if_absent(config, key, body, content_type)
        return True
    except urllib.error.HTTPError as error:
        if error.code != 412:
            raise RuntimeError(f"R2 conditional create {key} returned HTTP {error.code}") from error
        existing = get_object(config, key)
        if existing != body:
            raise RuntimeError(f"immutable R2 key is occupied by different bytes: {key}") from error
        return False


def request(
    config: R2Config,
    method: str,
    key: str,
    *,
    body: bytes = b"",
    content_type: str | None = None,
    extra_headers: dict[str, str] | None = None,
) -> bytes:
    parsed = urllib.parse.urlparse(config.endpoint_url)
    if not parsed.scheme or not parsed.netloc:
        raise RuntimeError(f"invalid R2 endpoint URL: {config.endpoint_url}")
    now = datetime.now(timezone.utc)
    date_stamp = now.strftime("%Y%m%d")
    amz_date = now.strftime("%Y%m%dT%H%M%SZ")
    object_path = f"{parsed.path.rstrip('/')}/{config.bucket}/{key}"
    canonical_uri = urllib.parse.quote(object_path, safe="/-_.~")
    url = urllib.parse.urlunparse((parsed.scheme, parsed.netloc, canonical_uri, "", "", ""))
    payload_hash = hashlib.sha256(body).hexdigest()
    headers = {"host": parsed.netloc, "x-amz-content-sha256": payload_hash, "x-amz-date": amz_date}
    if content_type is not None:
        headers["content-type"] = content_type
    for name, value in (extra_headers or {}).items():
        normalized = name.strip().lower()
        if not re.fullmatch(r"[a-z0-9-]+", normalized) or normalized in headers or normalized == "authorization":
            raise ValueError(f"invalid, duplicate, or reserved R2 request header: {name}")
        headers[normalized] = value
    signed_header_names = sorted(headers)
    canonical_headers = "".join(f"{name}:{headers[name].strip()}\n" for name in signed_header_names)
    signed_headers = ";".join(signed_header_names)
    canonical_request = "\n".join([method, canonical_uri, "", canonical_headers, signed_headers, payload_hash])
    credential_scope = f"{date_stamp}/{config.region}/s3/aws4_request"
    string_to_sign = "\n".join(
        [
            "AWS4-HMAC-SHA256",
            amz_date,
            credential_scope,
            hashlib.sha256(canonical_request.encode()).hexdigest(),
        ]
    )
    signing_key = aws_v4_signing_key(config.secret_access_key, date_stamp, config.region, "s3")
    signature = hmac.new(signing_key, string_to_sign.encode(), hashlib.sha256).hexdigest()
    headers["authorization"] = (
        "AWS4-HMAC-SHA256 "
        f"Credential={config.access_key_id}/{credential_scope}, "
        f"SignedHeaders={signed_headers}, Signature={signature}"
    )
    http_request = urllib.request.Request(
        url,
        data=body if method in {"PUT", "POST"} else None,
        headers={name.title(): value for name, value in headers.items() if name != "host"},
        method=method,
    )
    with urllib.request.urlopen(http_request, timeout=120) as response:
        return response.read()


def aws_v4_signing_key(secret_key: str, date_stamp: str, region: str, service: str) -> bytes:
    key_date = hmac.new(("AWS4" + secret_key).encode(), date_stamp.encode(), hashlib.sha256).digest()
    key_region = hmac.new(key_date, region.encode(), hashlib.sha256).digest()
    key_service = hmac.new(key_region, service.encode(), hashlib.sha256).digest()
    return hmac.new(key_service, b"aws4_request", hashlib.sha256).digest()
