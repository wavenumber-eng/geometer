from __future__ import annotations

import argparse
import hashlib
import subprocess
import sys
import tomllib
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
MANIFEST = ROOT / "docs" / "contracts" / "promotion-manifest.toml"
RECORD_PREFIX = "ANALYTIC_RESULT_PACKET_RECORD_VECTOR="
CANONICAL_PREFIX = "ANALYTIC_RESULT_PACKET_CANONICAL_VECTOR="
STANDALONE_PREFIX = "ANALYTIC_RESULT_PACKET_STANDALONE_VECTOR="
STANDALONE_DIGEST_PREFIX = "ANALYTIC_RESULT_PACKET_STANDALONE_DIGEST="
FAILED_STANDALONE_PREFIX = "ANALYTIC_RESULT_PACKET_FAILED_STANDALONE_VECTOR="
FAILED_STANDALONE_DIGEST_PREFIX = "ANALYTIC_RESULT_PACKET_FAILED_STANDALONE_DIGEST="


def _discover_native() -> Path:
    names = (
        "geometer_analytic_result_packet_records_test.exe",
        "geometer_analytic_result_packet_records_test",
    )
    for name in names:
        candidate = ROOT / "build" / "tests" / "cpp" / name
        if candidate.is_file():
            return candidate
    platform_fragment = {"win32": "windows", "linux": "linux", "darwin": "macos"}.get(
        sys.platform, "*"
    )
    for build in sorted(ROOT.glob(f"build-native-{platform_fragment}-*")):
        for name in names:
            candidate = build / "tests" / "cpp" / name
            if candidate.is_file():
                return candidate
    raise FileNotFoundError("build the native analytic result-packet record test first")


def _packets(command: list[str]) -> tuple[bytes, bytes, bytes, str, bytes, str]:
    completed = subprocess.run(
        command, cwd=ROOT, check=True, capture_output=True, text=True
    )
    record: bytes | None = None
    canonical: bytes | None = None
    standalone: bytes | None = None
    standalone_digest: str | None = None
    failed_standalone: bytes | None = None
    failed_standalone_digest: str | None = None
    for line in completed.stdout.splitlines():
        if line.startswith(RECORD_PREFIX):
            record = bytes.fromhex(line.removeprefix(RECORD_PREFIX))
        if line.startswith(CANONICAL_PREFIX):
            canonical = bytes.fromhex(line.removeprefix(CANONICAL_PREFIX))
        if line.startswith(STANDALONE_PREFIX):
            standalone = bytes.fromhex(line.removeprefix(STANDALONE_PREFIX))
        if line.startswith(STANDALONE_DIGEST_PREFIX):
            standalone_digest = line.removeprefix(STANDALONE_DIGEST_PREFIX)
        if line.startswith(FAILED_STANDALONE_PREFIX):
            failed_standalone = bytes.fromhex(line.removeprefix(FAILED_STANDALONE_PREFIX))
        if line.startswith(FAILED_STANDALONE_DIGEST_PREFIX):
            failed_standalone_digest = line.removeprefix(FAILED_STANDALONE_DIGEST_PREFIX)
    if (
        record is None
        or canonical is None
        or standalone is None
        or standalone_digest is None
        or failed_standalone is None
        or failed_standalone_digest is None
    ):
        raise RuntimeError("typed result-packet test omitted a governed vector")
    return (
        record,
        canonical,
        standalone,
        standalone_digest,
        failed_standalone,
        failed_standalone_digest,
    )


def main() -> int:
    parser = argparse.ArgumentParser(
        description="Verify native/Emscripten typed analytic result-packet bytes."
    )
    parser.add_argument("--native", type=Path)
    parser.add_argument(
        "--wasm",
        type=Path,
        default=(
            ROOT
            / "build-wasm"
            / "tests"
            / "cpp"
            / "geometer_analytic_result_packet_records_test.cjs"
        ),
    )
    args = parser.parse_args()
    native = args.native.resolve() if args.native else _discover_native()
    wasm = args.wasm.resolve()
    if not wasm.is_file():
        raise FileNotFoundError("build the Emscripten typed result-packet test first")
    native = _packets([str(native)])
    wasm = _packets(["node", str(wasm)])
    if native != wasm:
        raise RuntimeError("native and Emscripten result-packet vectors differ")
    (
        native_packet,
        native_canonical,
        native_standalone,
        native_standalone_digest,
        native_failed,
        native_failed_digest,
    ) = native
    expected = tomllib.loads(MANIFEST.read_text(encoding="utf-8"))["analytic_exact_backend"]
    digest = hashlib.sha256(native_packet).hexdigest()
    if len(native_packet) != expected["result_packet_record_vector_bytes"]:
        raise RuntimeError("typed result-packet byte count differs from the manifest")
    if digest != expected["result_packet_record_vector_sha256"]:
        raise RuntimeError("typed result-packet SHA-256 differs from the manifest")
    canonical_digest = hashlib.sha256(native_canonical).hexdigest()
    if len(native_canonical) != expected["result_packet_canonical_vector_bytes"]:
        raise RuntimeError("canonical result-packet byte count differs from the manifest")
    if canonical_digest != expected["result_packet_canonical_vector_sha256"]:
        raise RuntimeError("canonical result-packet SHA-256 differs from the manifest")
    standalone_digest = hashlib.sha256(native_standalone).hexdigest()
    failed_digest = hashlib.sha256(native_failed).hexdigest()
    if len(native_standalone) != expected["result_packet_standalone_vector_bytes"]:
        raise RuntimeError("standalone result-packet byte count differs from the manifest")
    if standalone_digest != expected["result_packet_standalone_vector_sha256"]:
        raise RuntimeError("standalone result-packet SHA-256 differs from the manifest")
    if native_standalone_digest != standalone_digest:
        raise RuntimeError("derived standalone result-packet digest is incorrect")
    if len(native_failed) != expected["result_packet_failed_standalone_vector_bytes"]:
        raise RuntimeError("failed standalone result-packet byte count differs from the manifest")
    if failed_digest != expected["result_packet_failed_standalone_vector_sha256"]:
        raise RuntimeError("failed standalone result-packet SHA-256 differs from the manifest")
    if native_failed_digest != failed_digest:
        raise RuntimeError("derived failed standalone result-packet digest is incorrect")
    print(f"analytic result-packet record parity: bytes={len(native_packet)} sha256={digest}")
    print(
        "analytic result-packet canonical parity: "
        f"bytes={len(native_canonical)} sha256={canonical_digest}"
    )
    print(
        "analytic standalone result-packet parity: "
        f"bytes={len(native_standalone)} sha256={standalone_digest}"
    )
    print(
        "analytic failed standalone result-packet parity: "
        f"bytes={len(native_failed)} sha256={failed_digest}"
    )
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
