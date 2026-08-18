from __future__ import annotations

import argparse
import hashlib
import subprocess
import sys
import tomllib
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
MANIFEST = ROOT / "docs" / "contracts" / "promotion-manifest.toml"
PREFIX = "EXACT_BOOLEAN_LINEAGE_MATRIX="


def _discover_native() -> Path:
    names = (
        "geometer_exact_boolean_lineage_matrix_test.exe",
        "geometer_exact_boolean_lineage_matrix_test",
    )
    platform_fragment = {"win32": "windows", "linux": "linux", "darwin": "macos"}.get(sys.platform, "*")
    for build in sorted(ROOT.glob(f"build-native-{platform_fragment}-*")):
        for name in names:
            candidate = build / "tests" / "cpp" / name
            if candidate.is_file():
                return candidate
    fallback = ROOT / "build" / "tests" / "cpp" / names[0]
    if fallback.is_file():
        return fallback
    raise FileNotFoundError("build the native exact Boolean lineage-matrix test first")


def _run(command: list[str]) -> str:
    completed = subprocess.run(command, cwd=ROOT, check=True, capture_output=True, text=True)
    for line in completed.stdout.splitlines():
        if line.startswith(PREFIX):
            return line.removeprefix(PREFIX)
    raise RuntimeError("exact Boolean lineage-matrix test omitted its governed output")


def main() -> int:
    parser = argparse.ArgumentParser(description="Verify native/Emscripten exact Boolean lineage-matrix parity.")
    parser.add_argument("--native", type=Path)
    parser.add_argument(
        "--wasm",
        type=Path,
        default=(ROOT / "build-wasm" / "tests" / "cpp" / "geometer_exact_boolean_lineage_matrix_test.cjs"),
    )
    args = parser.parse_args()
    native = args.native.resolve() if args.native else _discover_native()
    wasm = args.wasm.resolve()
    if not wasm.is_file():
        raise FileNotFoundError("build the Emscripten exact Boolean lineage-matrix test first")

    native_vector = _run([str(native)])
    wasm_vector = _run(["node", str(wasm)])
    if native_vector != wasm_vector:
        raise RuntimeError("native and Emscripten exact Boolean lineage matrices differ")
    digest = hashlib.sha256(native_vector.encode("ascii")).hexdigest()
    expected = tomllib.loads(MANIFEST.read_text(encoding="utf-8"))["analytic_exact_backend"]
    if digest != expected["boolean_lineage_matrix_sha256"]:
        raise RuntimeError("exact Boolean lineage-matrix SHA-256 differs from the manifest")
    print(f"exact Boolean lineage-matrix parity: sha256={digest}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
