from __future__ import annotations

import argparse
import hashlib
import subprocess
import sys
import tomllib
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
MANIFEST = ROOT / "docs" / "contracts" / "promotion-manifest.toml"
PREFIX = "EXACT_BOOLEAN_SEEDED_PROPERTY="


def _discover_native() -> Path:
    names = (
        "geometer_exact_boolean_seeded_property_test.exe",
        "geometer_exact_boolean_seeded_property_test",
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
    raise FileNotFoundError("build the native exact Boolean seeded-property test first")


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
    raise RuntimeError("exact Boolean seeded-property test omitted its governed output")


def main() -> int:
    parser = argparse.ArgumentParser(
        description="Verify identical native/Emscripten exact Boolean seeded properties."
    )
    parser.add_argument("--native", type=Path)
    parser.add_argument("--profile", choices=("pull-request", "nightly"), default="pull-request")
    parser.add_argument(
        "--wasm",
        type=Path,
        default=(
            ROOT
            / "build-wasm"
            / "tests"
            / "cpp"
            / "geometer_exact_boolean_seeded_property_test.cjs"
        ),
    )
    args = parser.parse_args()
    native = args.native.resolve() if args.native else _discover_native()
    wasm = args.wasm.resolve()
    if not wasm.is_file():
        raise FileNotFoundError("build the Emscripten exact Boolean seeded-property test first")

    profile_arguments = ["--nightly"] if args.profile == "nightly" else []
    native_signature = _signature([str(native), *profile_arguments])
    wasm_signature = _signature(["node", str(wasm), *profile_arguments])
    if native_signature != wasm_signature:
        raise RuntimeError("native and Emscripten exact Boolean seeded properties differ")
    digest = hashlib.sha256(native_signature.encode("ascii")).hexdigest()
    expected = tomllib.loads(MANIFEST.read_text(encoding="utf-8"))["analytic_exact_backend"]
    key_prefix = "nightly_seeded_property" if args.profile == "nightly" else "seeded_property"
    if digest != expected[f"{key_prefix}_sha256"]:
        raise RuntimeError("exact Boolean seeded-property SHA-256 differs from the manifest")
    profile_prefix = "profile:nightly," if args.profile == "nightly" else ""
    prefix = (
        f"{profile_prefix}seeds:{len(expected[f'{key_prefix}_seeds'])},"
        f"cases:{expected[f'{key_prefix}_cases']}|"
    )
    if not native_signature.startswith(prefix):
        raise RuntimeError("exact Boolean seeded-property inventory differs")
    if f"|{expected['seeded_property_reducer_sentinel']}|" not in native_signature:
        raise RuntimeError("exact Boolean seeded-property reducer sentinel differs")
    if not native_signature.endswith(expected["seeded_property_reducer_multistage_sentinel"]):
        raise RuntimeError("exact Boolean seeded-property multistage reducer sentinel differs")
    print(
        "exact Boolean seeded-property parity: "
        f"profile={args.profile} seeds={len(expected[f'{key_prefix}_seeds'])} "
        f"cases={expected[f'{key_prefix}_cases']} sha256={digest}"
    )
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
