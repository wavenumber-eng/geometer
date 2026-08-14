from __future__ import annotations

import argparse
import hashlib
import subprocess
import sys
import tomllib
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
MANIFEST = ROOT / "docs" / "contracts" / "promotion-manifest.toml"
PREFIX = "EXACT_BOOLEAN_METAMORPHIC="


def _discover_native() -> Path:
    names = (
        "geometer_exact_boolean_metamorphic_test.exe",
        "geometer_exact_boolean_metamorphic_test",
    )
    platform_fragment = {
        "win32": "windows",
        "linux": "linux",
        "darwin": "macos",
    }.get(sys.platform, "*")
    for build in sorted(ROOT.glob(f"build-native-{platform_fragment}-*")):
        for name in names:
            candidate = build / "tests" / "cpp" / name
            if candidate.is_file():
                return candidate
    raise FileNotFoundError("build the native exact Boolean metamorphic test first")


def _signature(command: list[str]) -> str:
    completed = subprocess.run(
        command,
        cwd=ROOT,
        check=True,
        capture_output=True,
        text=True,
    )
    for line in completed.stdout.splitlines():
        if line.startswith(PREFIX):
            return line.removeprefix(PREFIX)
    raise RuntimeError("exact Boolean metamorphic test omitted its governed output")


def main() -> int:
    parser = argparse.ArgumentParser(
        description="Verify identical native/Emscripten exact Boolean metamorphic results."
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
            / "geometer_exact_boolean_metamorphic_test.cjs"
        ),
    )
    args = parser.parse_args()
    native = args.native.resolve() if args.native else _discover_native()
    wasm = args.wasm.resolve()
    if not wasm.is_file():
        raise FileNotFoundError("build the Emscripten exact Boolean metamorphic test first")

    native_signature = _signature([str(native)])
    wasm_signature = _signature(["node", str(wasm)])
    if native_signature != wasm_signature:
        raise RuntimeError("native and Emscripten exact Boolean metamorphic results differ")
    digest = hashlib.sha256(native_signature.encode("ascii")).hexdigest()
    expected = tomllib.loads(MANIFEST.read_text(encoding="utf-8"))["analytic_exact_backend"]
    if digest != expected["boolean_metamorphic_sha256"]:
        raise RuntimeError("exact Boolean metamorphic SHA-256 differs from the manifest")
    union_prefix = f"union_permutations:{expected['boolean_union_permutations']}:"
    difference_prefix = f"|difference_permutations:{expected['boolean_difference_permutations']}:"
    if not native_signature.startswith(union_prefix) or difference_prefix not in native_signature:
        raise RuntimeError("exact Boolean metamorphic permutation inventory differs")
    for sentinel in expected["boolean_metamorphic_sentinels"]:
        if sentinel not in native_signature:
            raise RuntimeError(f"missing exact Boolean metamorphic sentinel: {sentinel}")
    print(
        "exact Boolean metamorphic parity: "
        f"union={expected['boolean_union_permutations']} "
        f"difference={expected['boolean_difference_permutations']} sha256={digest}"
    )
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
