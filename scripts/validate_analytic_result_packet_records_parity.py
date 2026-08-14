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


def _packets(command: list[str]) -> tuple[bytes, bytes]:
    completed = subprocess.run(
        command, cwd=ROOT, check=True, capture_output=True, text=True
    )
    record: bytes | None = None
    canonical: bytes | None = None
    for line in completed.stdout.splitlines():
        if line.startswith(RECORD_PREFIX):
            record = bytes.fromhex(line.removeprefix(RECORD_PREFIX))
        if line.startswith(CANONICAL_PREFIX):
            canonical = bytes.fromhex(line.removeprefix(CANONICAL_PREFIX))
    if record is None or canonical is None:
        raise RuntimeError("typed result-packet test omitted a governed vector")
    return record, canonical


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
    native_packet, native_canonical = _packets([str(native)])
    wasm_packet, wasm_canonical = _packets(["node", str(wasm)])
    if native_packet != wasm_packet:
        raise RuntimeError("native and Emscripten typed result-packet bytes differ")
    if native_canonical != wasm_canonical:
        raise RuntimeError("native and Emscripten canonical result-packet bytes differ")
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
    print(f"analytic result-packet record parity: bytes={len(native_packet)} sha256={digest}")
    print(
        "analytic result-packet canonical parity: "
        f"bytes={len(native_canonical)} sha256={canonical_digest}"
    )
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
