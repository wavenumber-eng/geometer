"""Generate and validate deterministic browser/TypeScript build attestations."""

from __future__ import annotations

import argparse
import hashlib
import json
import os
from pathlib import Path
import re
import subprocess
from typing import Any

from native_build_attestation import (
    REVISION_RE,
    SOURCE_AUTHORITY,
    canonical_json,
    file_sha256,
    source_identity,
)
from dependency_versions import EMSDK_VERSION, OCCT_REPO, OCCT_TAG
from occt_binary_cache import INSTALL_PROFILE_NAME


ROOT = Path(__file__).resolve().parent.parent
SCHEMA = "wn.geometer.browser_build_attestation.a0"
PRODUCER_IDENTITY = "wn.geometer.browser_build_attestation_generator.a0"
PRODUCER_SOURCE = "scripts/browser_build_attestation.py"
SIDECAR = ROOT / "dist" / "wasm" / "geometer.browser-build-attestation.json"
SHA256_RE = re.compile(r"[0-9a-f]{64}")
ARTIFACT_ROOTS = (
    Path("dist/wasm/browser/geometer.js"),
    Path("dist/wasm/browser/geometer.wasm"),
    Path("dist/wasm/planar-browser/geometer-planar-browser.js"),
    Path("dist/wasm/planar-browser/geometer-planar-browser.wasm"),
)
PACKAGE_ROOT = Path("dist/wasm/npm/geometer")
SOURCE_EXCLUDES = ("dist/", "build/", "build-", ".deps/", "out/")
OCCT_PROFILE = ROOT / ".deps" / "occt-wasm-install" / INSTALL_PROFILE_NAME
EMCC = ROOT / ".deps" / "emsdk" / "upstream" / "emscripten" / ("emcc.bat" if os.name == "nt" else "emcc")


class BrowserBuildAttestationError(RuntimeError):
    """Raised when browser build provenance is absent or inconsistent."""


def _command_text(command: list[str]) -> str:
    try:
        return (
            subprocess.check_output(
                command,
                cwd=ROOT,
                text=True,
                stderr=subprocess.STDOUT,
                timeout=20,
            )
            .strip()
            .splitlines()[0]
        )
    except (OSError, subprocess.SubprocessError, IndexError) as error:
        raise BrowserBuildAttestationError(f"could not query browser build toolchain: {' '.join(command)}") from error


def _npm_executable() -> str:
    return "npm.cmd" if os.name == "nt" else "npm"


def wasm_toolchain_identity() -> dict[str, object]:
    if not OCCT_PROFILE.is_file():
        raise BrowserBuildAttestationError("verified WASM OCCT profile is unavailable")
    try:
        raw_profile = OCCT_PROFILE.read_bytes()
        profile = json.loads(raw_profile)
    except (OSError, UnicodeDecodeError, json.JSONDecodeError) as error:
        raise BrowserBuildAttestationError("WASM OCCT profile is malformed") from error
    if not isinstance(profile, dict):
        raise BrowserBuildAttestationError("WASM OCCT profile must be an object")
    if (
        profile.get("kind") != "wasm"
        or profile.get("platform_tag") != "wasm-emscripten"
        or profile.get("occt_repo") != OCCT_REPO
        or profile.get("occt_tag") != OCCT_TAG
        or not isinstance(profile.get("recipe_hash"), str)
        or SHA256_RE.fullmatch(profile["recipe_hash"]) is None
    ):
        raise BrowserBuildAttestationError("WASM OCCT profile does not match governed dependency authority")
    if not EMCC.is_file():
        raise BrowserBuildAttestationError("governed Emscripten executable is unavailable")
    return {
        "emscripten": {
            "executable": EMCC.relative_to(ROOT).as_posix(),
            "executable_sha256": file_sha256(EMCC),
            "version": EMSDK_VERSION,
            "version_text": _command_text([str(EMCC), "--version"]),
        },
        "node": _command_text(["node", "--version"]),
        "npm": _command_text([_npm_executable(), "--version"]),
        "occt": {
            "profile": OCCT_PROFILE.relative_to(ROOT).as_posix(),
            "profile_sha256": hashlib.sha256(raw_profile).hexdigest(),
            "recipe_hash": profile["recipe_hash"],
            "repo": OCCT_REPO,
            "tag": OCCT_TAG,
        },
    }


def source_content_sha256() -> str:
    try:
        raw_paths = subprocess.check_output(["git", "ls-files", "-z"], cwd=ROOT, timeout=20)
    except (OSError, subprocess.SubprocessError) as error:
        raise BrowserBuildAttestationError("could not enumerate tracked browser build inputs") from error
    digest = hashlib.sha256()
    for raw_path in sorted(path for path in raw_paths.split(b"\0") if path):
        relative = raw_path.decode("utf-8")
        normalized = relative.replace("\\", "/")
        if normalized.startswith(SOURCE_EXCLUDES):
            continue
        path = ROOT / relative
        if not path.is_file():
            raise BrowserBuildAttestationError(f"tracked browser build input is unavailable: {relative}")
        digest.update(len(raw_path).to_bytes(4, "little"))
        digest.update(raw_path)
        content = path.read_bytes()
        digest.update(len(content).to_bytes(8, "little"))
        digest.update(content)
    return digest.hexdigest()


def artifact_closure() -> dict[str, dict[str, object]]:
    paths = list(ARTIFACT_ROOTS)
    if not (ROOT / PACKAGE_ROOT).is_dir():
        raise BrowserBuildAttestationError("generated TypeScript package is unavailable")
    paths.extend(sorted(path.relative_to(ROOT) for path in (ROOT / PACKAGE_ROOT).rglob("*") if path.is_file()))
    closure: dict[str, dict[str, object]] = {}
    for relative in paths:
        path = ROOT / relative
        if not path.is_file():
            raise BrowserBuildAttestationError(f"browser build artifact is unavailable: {relative.as_posix()}")
        closure[relative.as_posix()] = {
            "bytes": path.stat().st_size,
            "sha256": file_sha256(path),
        }
    return closure


def build_attestation() -> dict[str, Any]:
    value: dict[str, Any] = {
        "artifacts": artifact_closure(),
        "build": {
            "contract_check": "npm run check:contracts",
            "contract_check_status": "passed",
            "source": source_identity(),
            "source_content_sha256": source_content_sha256(),
            "toolchain": wasm_toolchain_identity(),
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


def write_attestation(path: Path = SIDECAR) -> dict[str, Any]:
    value = build_attestation()
    path.parent.mkdir(parents=True, exist_ok=True)
    temporary = path.with_name(f"{path.name}.tmp")
    temporary.write_bytes(canonical_json(value))
    temporary.replace(path)
    return value


def load_and_validate_attestation(path: Path = SIDECAR) -> dict[str, Any]:
    if not path.is_file():
        raise BrowserBuildAttestationError("browser build attestation is missing")
    try:
        raw = path.read_bytes()
        value = json.loads(raw)
    except (OSError, UnicodeDecodeError, json.JSONDecodeError) as error:
        raise BrowserBuildAttestationError(f"invalid browser build attestation JSON: {error}") from error
    if not isinstance(value, dict):
        raise BrowserBuildAttestationError("browser build attestation root must be an object")
    _validate_shape(value)
    if raw != canonical_json(value):
        raise BrowserBuildAttestationError("browser build attestation is not canonical deterministic JSON")
    producer_source = ROOT / value["producer"]["source"]
    if not producer_source.is_file() or value["producer"]["source_sha256"] != file_sha256(producer_source):
        raise BrowserBuildAttestationError("browser build attestation producer source is stale")
    if value["build"]["source_content_sha256"] != source_content_sha256():
        raise BrowserBuildAttestationError("browser build attestation source-content closure is stale")
    if value["artifacts"] != artifact_closure():
        raise BrowserBuildAttestationError("browser build attestation artifact closure is stale")
    if value["build"]["toolchain"] != wasm_toolchain_identity():
        raise BrowserBuildAttestationError("browser build attestation toolchain authority is stale")
    current_source = source_identity()
    source = value["build"]["source"]
    value["build_provenance_attested"] = bool(
        source.get("authority") == SOURCE_AUTHORITY
        and REVISION_RE.fullmatch(str(source.get("revision")))
        and source.get("tree_state") == "clean"
        and source == current_source
    )
    value["sidecar_sha256"] = hashlib.sha256(raw).hexdigest()
    return value


def _require_keys(value: dict[str, Any], expected: set[str], label: str) -> None:
    if set(value) != expected:
        raise BrowserBuildAttestationError(f"{label} fields differ from schema")


def _require_sha(value: object, label: str) -> None:
    if not isinstance(value, str) or SHA256_RE.fullmatch(value) is None:
        raise BrowserBuildAttestationError(f"{label} must be a SHA-256 digest")


def _validate_shape(value: dict[str, Any]) -> None:
    _require_keys(value, {"artifacts", "build", "producer", "schema"}, "attestation")
    if value["schema"] != SCHEMA:
        raise BrowserBuildAttestationError("browser build attestation schema is unsupported")
    artifacts = value["artifacts"]
    build = value["build"]
    producer = value["producer"]
    if not isinstance(artifacts, dict) or not artifacts:
        raise BrowserBuildAttestationError("artifact closure must be non-empty")
    for name, artifact in artifacts.items():
        if not isinstance(name, str) or not name.startswith("dist/wasm/") or not isinstance(artifact, dict):
            raise BrowserBuildAttestationError("artifact closure entry is malformed")
        _require_keys(artifact, {"bytes", "sha256"}, f"artifact {name}")
        if not isinstance(artifact["bytes"], int) or isinstance(artifact["bytes"], bool) or artifact["bytes"] < 0:
            raise BrowserBuildAttestationError(f"artifact {name} byte count is invalid")
        _require_sha(artifact["sha256"], f"artifact {name} digest")
    if not isinstance(build, dict) or not isinstance(producer, dict):
        raise BrowserBuildAttestationError("attestation sections must be objects")
    _require_keys(
        build,
        {
            "contract_check",
            "contract_check_status",
            "source",
            "source_content_sha256",
            "toolchain",
        },
        "build",
    )
    if build["contract_check"] != "npm run check:contracts" or build["contract_check_status"] != "passed":
        raise BrowserBuildAttestationError("contract freshness check is not attested")
    _require_sha(build["source_content_sha256"], "source content digest")
    source = build["source"]
    toolchain = build["toolchain"]
    if not isinstance(source, dict) or not isinstance(toolchain, dict):
        raise BrowserBuildAttestationError("source/toolchain sections must be objects")
    _require_keys(source, {"authority", "revision", "tree_state"}, "source")
    _require_keys(toolchain, {"emscripten", "node", "npm", "occt"}, "toolchain")
    if not all(isinstance(toolchain[key], str) and toolchain[key] for key in ("node", "npm")):
        raise BrowserBuildAttestationError("toolchain identities must be text")
    emscripten = toolchain["emscripten"]
    occt = toolchain["occt"]
    if not isinstance(emscripten, dict) or not isinstance(occt, dict):
        raise BrowserBuildAttestationError("Emscripten and OCCT identities must be objects")
    _require_keys(
        emscripten,
        {"executable", "executable_sha256", "version", "version_text"},
        "emscripten",
    )
    _require_sha(emscripten["executable_sha256"], "Emscripten executable digest")
    if emscripten["version"] != EMSDK_VERSION:
        raise BrowserBuildAttestationError("Emscripten version is unsupported")
    _require_keys(
        occt,
        {"profile", "profile_sha256", "recipe_hash", "repo", "tag"},
        "occt",
    )
    _require_sha(occt["profile_sha256"], "OCCT profile digest")
    _require_sha(occt["recipe_hash"], "OCCT recipe digest")
    if occt["repo"] != OCCT_REPO or occt["tag"] != OCCT_TAG:
        raise BrowserBuildAttestationError("OCCT authority is unsupported")
    _require_keys(producer, {"identity", "source", "source_sha256"}, "producer")
    if producer["identity"] != PRODUCER_IDENTITY or producer["source"] != PRODUCER_SOURCE:
        raise BrowserBuildAttestationError("attestation producer is unsupported")
    _require_sha(producer["source_sha256"], "producer source digest")


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    subparsers = parser.add_subparsers(dest="command", required=True)
    write = subparsers.add_parser("write")
    write.add_argument("--output", type=Path, default=SIDECAR)
    validate = subparsers.add_parser("validate")
    validate.add_argument("--attestation", type=Path, default=SIDECAR)
    validate.add_argument("--require-promotion", action="store_true")
    args = parser.parse_args()
    if args.command == "write":
        write_attestation(args.output)
        return 0
    value = load_and_validate_attestation(args.attestation)
    if args.require_promotion and value["build_provenance_attested"] is not True:
        raise BrowserBuildAttestationError("browser build attestation is not promotion-eligible")
    print(value["sidecar_sha256"])
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
