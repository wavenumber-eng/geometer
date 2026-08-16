from __future__ import annotations

import argparse
import hashlib
import subprocess
import tomllib
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
MANIFEST = ROOT / "docs" / "contracts" / "promotion-manifest.toml"
VECTOR_PREFIX = "ANALYTIC_FILTERED_OUTCOMES_VECTOR="


def _discover_native() -> Path:
    names = (
        "geometer_analytic_filtered_outcomes_test.exe",
        "geometer_analytic_filtered_outcomes_test",
    )
    for build in [ROOT / "build", *sorted(ROOT.glob("build-native-*"))]:
        for name in names:
            candidate = build / "tests" / "cpp" / name
            if candidate.is_file():
                return candidate
    raise FileNotFoundError("build the native filtered-outcomes test first")


def _vector(command: list[str]) -> bytes:
    completed = subprocess.run(
        [*command, "--emit-parity"],
        cwd=ROOT,
        check=True,
        capture_output=True,
        text=True,
    )
    for line in completed.stdout.splitlines():
        if line.startswith(VECTOR_PREFIX):
            return bytes.fromhex(line.removeprefix(VECTOR_PREFIX))
    raise RuntimeError("filtered-outcomes test omitted its parity vector")


def main() -> int:
    parser = argparse.ArgumentParser(description="Verify native/Emscripten filtered operand-outcome parity.")
    parser.add_argument("--native", type=Path)
    parser.add_argument(
        "--wasm",
        type=Path,
        default=(ROOT / "build-wasm" / "tests" / "cpp" / "geometer_analytic_filtered_outcomes_test.cjs"),
    )
    args = parser.parse_args()
    native = args.native.resolve() if args.native else _discover_native()
    wasm = args.wasm.resolve()
    if not wasm.is_file():
        raise FileNotFoundError("build the Emscripten filtered-outcomes test first")
    native_vector = _vector([str(native)])
    wasm_vector = _vector(["node", str(wasm)])
    if native_vector != wasm_vector:
        raise RuntimeError("native and Emscripten filtered-outcome vectors differ")
    digest = hashlib.sha256(native_vector).hexdigest()
    expected = tomllib.loads(MANIFEST.read_text(encoding="utf-8"))["analytic_filtered_solver"]
    if len(native_vector) != expected["outcomes_vector_bytes"]:
        raise RuntimeError("filtered-outcome vector byte length differs from manifest")
    if digest != expected["outcomes_vector_sha256"]:
        raise RuntimeError("filtered-outcome vector SHA-256 differs from manifest")
    print(f"analytic filtered outcomes parity: {len(native_vector)} bytes sha256={digest}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
