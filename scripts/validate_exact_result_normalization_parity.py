from __future__ import annotations

import argparse
import hashlib
import subprocess
import sys
import tomllib
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
MANIFEST = ROOT / "docs" / "contracts" / "promotion-manifest.toml"
VECTOR_PREFIX = "EXACT_RESULT_NORMALIZATION_VECTOR="


def _discover_native() -> Path:
    names = (
        "geometer_exact_result_normalization_test.exe",
        "geometer_exact_result_normalization_test",
    )
    platform_fragment = {"win32": "windows", "linux": "linux", "darwin": "macos"}.get(
        sys.platform, "*"
    )
    for build in sorted(ROOT.glob(f"build-native-{platform_fragment}-*")):
        for name in names:
            candidate = build / "tests" / "cpp" / name
            if candidate.is_file():
                return candidate
    raise FileNotFoundError("build the native exact result-normalization test first")


def _vector(command: list[str]) -> str:
    completed = subprocess.run(
        command, cwd=ROOT, check=True, capture_output=True, text=True
    )
    for line in completed.stdout.splitlines():
        if line.startswith(VECTOR_PREFIX):
            return line.removeprefix(VECTOR_PREFIX)
    raise RuntimeError("exact result-normalization test omitted its governed vector")


def main() -> int:
    parser = argparse.ArgumentParser(
        description="Verify identical native/Emscripten exact result normalization."
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
            / "geometer_exact_result_normalization_test.cjs"
        ),
    )
    args = parser.parse_args()
    native = args.native.resolve() if args.native else _discover_native()
    wasm = args.wasm.resolve()
    if not wasm.is_file():
        raise FileNotFoundError("build the Emscripten exact result-normalization test first")
    native_vector = _vector([str(native)])
    wasm_vector = _vector(["node", str(wasm)])
    if native_vector != wasm_vector:
        raise RuntimeError("native and Emscripten result-normalization vectors differ")
    digest = hashlib.sha256(native_vector.encode("ascii")).hexdigest()
    expected = tomllib.loads(MANIFEST.read_text(encoding="utf-8"))["analytic_exact_backend"]
    if digest != expected["result_normalization_vector_sha256"]:
        raise RuntimeError("exact result-normalization SHA-256 differs from the manifest")
    print(f"exact result normalization parity: sha256={digest}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
