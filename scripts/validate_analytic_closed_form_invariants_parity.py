from __future__ import annotations

import argparse
import hashlib
import subprocess
import tomllib
from pathlib import Path


ROOT = Path(__file__).resolve().parents[1]
MANIFEST = ROOT / "docs" / "contracts" / "promotion-manifest.toml"
PREFIX = "ANALYTIC_CLOSED_FORM_INVARIANTS="
METAMORPHIC_PREFIX = "ANALYTIC_METAMORPHIC_INVARIANTS="


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


def _signatures(command: list[str]) -> tuple[str, str]:
    completed = subprocess.run(
        command, cwd=ROOT, check=True, capture_output=True, text=True
    )
    closed_form: str | None = None
    metamorphic: str | None = None
    for line in completed.stdout.splitlines():
        if line.startswith(PREFIX):
            closed_form = line.removeprefix(PREFIX)
        elif line.startswith(METAMORPHIC_PREFIX):
            metamorphic = line.removeprefix(METAMORPHIC_PREFIX)
    if closed_form is None or metamorphic is None:
        raise RuntimeError("analytic invariant test omitted a governed signature")
    return closed_form, metamorphic


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
    native_closed_form, native_metamorphic = _signatures([str(native)])
    wasm_closed_form, wasm_metamorphic = _signatures(["node", str(wasm)])
    if native_closed_form != wasm_closed_form or native_metamorphic != wasm_metamorphic:
        raise RuntimeError("native and Emscripten analytic invariant signatures differ")
    digest = hashlib.sha256(native_closed_form.encode("utf-8")).hexdigest()
    metamorphic_digest = hashlib.sha256(native_metamorphic.encode("utf-8")).hexdigest()
    expected = tomllib.loads(MANIFEST.read_text(encoding="utf-8"))["analytic_exact_backend"]
    if digest != expected["closed_form_invariants_sha256"]:
        raise RuntimeError("closed-form invariant digest changed")
    if metamorphic_digest != expected["metamorphic_invariants_sha256"]:
        raise RuntimeError("metamorphic invariant digest changed")
    print(
        f"analytic invariant parity: closed_form_sha256={digest} "
        f"metamorphic_sha256={metamorphic_digest}"
    )
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
