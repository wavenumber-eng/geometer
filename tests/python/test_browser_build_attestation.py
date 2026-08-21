from __future__ import annotations

import copy
import hashlib
from pathlib import Path
import sys
from typing import Any, Callable

import pytest


ROOT = Path(__file__).resolve().parents[2]
sys.path.insert(0, str(ROOT / "scripts"))
import browser_build_attestation as attestation  # noqa: E402


def _artifact_closure() -> dict[str, dict[str, object]]:
    return {
        "dist/wasm/browser/geometer.js": {
            "bytes": 3,
            "sha256": hashlib.sha256(b"abc").hexdigest(),
        },
        "dist/wasm/browser/geometer.wasm": {
            "bytes": 3,
            "sha256": hashlib.sha256(b"def").hexdigest(),
        },
        "dist/wasm/planar-browser/geometer-planar-browser.js": {
            "bytes": 3,
            "sha256": hashlib.sha256(b"ghi").hexdigest(),
        },
        "dist/wasm/planar-browser/geometer-planar-browser.wasm": {
            "bytes": 3,
            "sha256": hashlib.sha256(b"jkl").hexdigest(),
        },
        "dist/wasm/npm/geometer/package.json": {
            "bytes": 3,
            "sha256": hashlib.sha256(b"mno").hexdigest(),
        },
    }


def _value() -> dict[str, Any]:
    return {
        "artifacts": _artifact_closure(),
        "build": {
            "contract_check": "npm run check:contracts",
            "contract_check_status": "passed",
            "source": {
                "authority": attestation.SOURCE_AUTHORITY,
                "revision": "1" * 40,
                "tree_state": "clean",
            },
            "source_content_sha256": "2" * 64,
            "toolchain": {
                "emscripten": {
                    "executable": ".deps/emsdk/upstream/emscripten/emcc.bat",
                    "executable_sha256": "4" * 64,
                    "version": attestation.EMSDK_VERSION,
                    "version_text": "emcc 4.0.0",
                },
                "node": "v24.0.0",
                "npm": "11.0.0",
                "occt": {
                    "profile": ".deps/occt-wasm-install/.geometer-occt-profile.json",
                    "profile_sha256": "5" * 64,
                    "recipe_hash": "6" * 64,
                    "repo": attestation.OCCT_REPO,
                    "tag": attestation.OCCT_TAG,
                },
            },
        },
        "producer": {
            "identity": attestation.PRODUCER_IDENTITY,
            "source": attestation.PRODUCER_SOURCE,
            "source_sha256": "3" * 64,
        },
        "schema": attestation.SCHEMA,
    }


def test_browser_attestation_shape_closes_contract_and_artifacts() -> None:
    value = _value()
    attestation._validate_shape(value)
    assert value["build"]["contract_check_status"] == "passed"
    assert set(value["artifacts"]) == set(_artifact_closure())


@pytest.mark.parametrize(
    ("mutation", "match"),
    [
        (lambda value: value.update(schema="wrong"), "schema"),
        (
            lambda value: value["build"].update(contract_check_status="skipped"),
            "freshness",
        ),
        (
            lambda value: value["producer"].update(identity="forged"),
            "producer",
        ),
        (
            lambda value: value["artifacts"]["dist/wasm/browser/geometer.js"].update(bytes=-1),
            "byte count",
        ),
    ],
)
def test_browser_attestation_shape_rejects_mutations(mutation: Callable[[dict[str, Any]], None], match: str) -> None:
    value = copy.deepcopy(_value())
    mutation(value)
    with pytest.raises(attestation.BrowserBuildAttestationError, match=match):
        attestation._validate_shape(value)


def test_browser_attestation_load_rejects_stale_artifacts(tmp_path: Path, monkeypatch: pytest.MonkeyPatch) -> None:
    value = _value()
    value["producer"]["source_sha256"] = attestation.file_sha256(ROOT / attestation.PRODUCER_SOURCE)
    path = tmp_path / "browser.json"
    path.write_bytes(attestation.canonical_json(value))
    monkeypatch.setattr(attestation, "source_content_sha256", lambda: "2" * 64)
    monkeypatch.setattr(
        attestation,
        "wasm_toolchain_identity",
        lambda: value["build"]["toolchain"],
    )
    monkeypatch.setattr(attestation, "artifact_closure", lambda: {})
    with pytest.raises(attestation.BrowserBuildAttestationError, match="artifact closure"):
        attestation.load_and_validate_attestation(path)


def test_browser_attestation_build_uses_platform_aware_npm(
    monkeypatch: pytest.MonkeyPatch,
) -> None:
    monkeypatch.setattr(attestation, "artifact_closure", _artifact_closure)
    monkeypatch.setattr(attestation, "source_content_sha256", lambda: "2" * 64)
    monkeypatch.setattr(
        attestation,
        "source_identity",
        lambda: {
            "authority": attestation.SOURCE_AUTHORITY,
            "revision": "1" * 40,
            "tree_state": "clean",
        },
    )
    monkeypatch.setattr(
        attestation,
        "wasm_toolchain_identity",
        lambda: _value()["build"]["toolchain"],
    )
    value = attestation.build_attestation()
    assert value["schema"] == attestation.SCHEMA
    expected_npm = "npm.cmd" if attestation.os.name == "nt" else "npm"
    assert attestation._npm_executable() == expected_npm


def test_build_wasm_checks_contracts_before_browser_build_and_attests() -> None:
    source = (ROOT / "scripts" / "build_wasm.py").read_text(encoding="utf-8")
    check = 'run([npm, "run", "check:contracts"], cwd=ROOT)'
    build = 'print("Configuring geometer for WASM ...")'
    attest = 'str(ROOT / "scripts" / "browser_build_attestation.py")'
    assert check in source and attest in source
    assert source.index(check) < source.index(build) < source.index(attest)
