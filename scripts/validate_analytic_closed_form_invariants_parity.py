from __future__ import annotations

import argparse
import hashlib
import subprocess
import tomllib
from pathlib import Path


ROOT = Path(__file__).resolve().parents[1]
MANIFEST = ROOT / "docs" / "contracts" / "promotion-manifest.toml"
PREFIX = "ANALYTIC_CLOSED_FORM_INVARIANTS="


def _discover_native() -> Path:
    candidates = [
        ROOT
        / "build-native-windows-x64"
        / "tests"
        / "cpp"
        / "geometer_analytic_closed_form_invariants_test.exe",
        ROOT / "build" / "tests" / "cpp" / "geometer_analytic_closed_form_invariants_test",
    ]
    for candidate in candidates:
        if candidate.is_file():
            return candidate
    raise FileNotFoundError("build the native analytic closed-form test first")


def _signature(command: list[str]) -> str:
    completed = subprocess.run(
        command, cwd=ROOT, check=True, capture_output=True, text=True
    )
    for line in completed.stdout.splitlines():
        if line.startswith(PREFIX):
            return line.removeprefix(PREFIX)
    raise RuntimeError("analytic closed-form test omitted its invariant signature")


def main() -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument("--native", type=Path, default=None)
    parser.add_argument(
        "--wasm",
        type=Path,
        default=ROOT
        / "build-wasm"
        / "tests"
        / "cpp"
        / "geometer_analytic_closed_form_invariants_test.cjs",
    )
    args = parser.parse_args()
    native = args.native.resolve() if args.native else _discover_native()
    wasm = args.wasm.resolve()
    if not wasm.is_file():
        raise FileNotFoundError("build the Emscripten analytic closed-form test first")
    native_signature = _signature([str(native)])
    wasm_signature = _signature(["node", str(wasm)])
    if native_signature != wasm_signature:
        raise RuntimeError("native and Emscripten closed-form invariant signatures differ")
    digest = hashlib.sha256(native_signature.encode("utf-8")).hexdigest()
    expected = tomllib.loads(MANIFEST.read_text(encoding="utf-8"))["analytic_exact_backend"][
        "closed_form_invariants_sha256"
    ]
    if digest != expected:
        raise RuntimeError(f"closed-form invariant digest changed: {digest} != {expected}")
    print(f"analytic closed-form invariant parity: sha256={digest}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
