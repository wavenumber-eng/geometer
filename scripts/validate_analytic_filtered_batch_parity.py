from __future__ import annotations

import argparse
import hashlib
import subprocess
import tomllib
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
MANIFEST = ROOT / "docs" / "contracts" / "promotion-manifest.toml"
BATCH_VECTOR_PREFIX = "ANALYTIC_FILTERED_BATCH_VECTOR="
RELATIONSHIP_VECTOR_PREFIX = "ANALYTIC_FILTERED_RELATIONSHIP_VECTOR="


def _discover_native() -> Path:
    names = ("geometer_analytic_filtered_batch_test.exe", "geometer_analytic_filtered_batch_test")
    for build in [ROOT / "build", *sorted(ROOT.glob("build-native-*"))]:
        for name in names:
            candidate = build / "tests" / "cpp" / name
            if candidate.is_file():
                return candidate
    raise FileNotFoundError("build the native filtered-batch test first")


def _vectors(command: list[str]) -> tuple[bytes, bytes]:
    completed = subprocess.run([*command, "--emit-parity"], cwd=ROOT, check=True, capture_output=True, text=True)
    vectors: dict[str, bytes] = {}
    for line in completed.stdout.splitlines():
        for name, prefix in (
            ("batch", BATCH_VECTOR_PREFIX),
            ("relationship", RELATIONSHIP_VECTOR_PREFIX),
        ):
            if line.startswith(prefix):
                vectors[name] = bytes.fromhex(line.removeprefix(prefix))
    if set(vectors) != {"batch", "relationship"}:
        raise RuntimeError("filtered-batch test omitted a governed parity vector")
    return vectors["batch"], vectors["relationship"]


def main() -> int:
    parser = argparse.ArgumentParser(description="Verify native/Emscripten filtered batch parity.")
    parser.add_argument("--native", type=Path)
    parser.add_argument(
        "--wasm",
        type=Path,
        default=ROOT / "build-wasm" / "tests" / "cpp" / "geometer_analytic_filtered_batch_test.cjs",
    )
    args = parser.parse_args()
    native = args.native.resolve() if args.native else _discover_native()
    wasm = args.wasm.resolve()
    if not wasm.is_file():
        raise FileNotFoundError("build the Emscripten filtered-batch test first")
    native_batch, native_relationship = _vectors([str(native)])
    wasm_batch, wasm_relationship = _vectors(["node", str(wasm)])
    if native_batch != wasm_batch:
        raise RuntimeError("native and Emscripten filtered-batch vectors differ")
    if native_relationship != wasm_relationship:
        raise RuntimeError("native and Emscripten filtered-relationship vectors differ")
    batch_digest = hashlib.sha256(native_batch).hexdigest()
    relationship_digest = hashlib.sha256(native_relationship).hexdigest()
    expected = tomllib.loads(MANIFEST.read_text(encoding="utf-8"))["analytic_filtered_solver"]
    if len(native_batch) != expected["batch_vector_bytes"]:
        raise RuntimeError("filtered-batch vector byte length differs from manifest")
    if batch_digest != expected["batch_vector_sha256"]:
        raise RuntimeError("filtered-batch SHA-256 differs from manifest")
    if len(native_relationship) != expected["batch_relationship_vector_bytes"]:
        raise RuntimeError("filtered-relationship vector byte length differs from manifest")
    if relationship_digest != expected["batch_relationship_vector_sha256"]:
        raise RuntimeError("filtered-relationship SHA-256 differs from manifest")
    print(f"analytic filtered batch parity: {len(native_batch)} bytes sha256={batch_digest}")
    print(f"analytic filtered relationship parity: {len(native_relationship)} bytes sha256={relationship_digest}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
