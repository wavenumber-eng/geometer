from __future__ import annotations

import argparse
import hashlib
import subprocess
import sys
import tomllib
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
MANIFEST = ROOT / "docs" / "contracts" / "promotion-manifest.toml"
VECTOR_PREFIX = "EXACT_CURVE_DOMAIN_VECTOR="
WORK_PREFIX = "EXACT_CURVE_DOMAIN_WORK="


def _discover_native() -> Path:
    names = ("geometer_exact_curve_domain_test.exe", "geometer_exact_curve_domain_test")
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
    raise FileNotFoundError("build the native exact curve-domain test before parity validation")


def _run(command: list[str]) -> dict[str, str]:
    completed = subprocess.run(
        command,
        cwd=ROOT,
        check=True,
        capture_output=True,
        text=True,
    )
    fields: dict[str, str] = {}
    for line in completed.stdout.splitlines():
        if "=" in line:
            key, value = line.split("=", 1)
            fields[key + "="] = value
    return fields


def main() -> int:
    parser = argparse.ArgumentParser(
        description="Verify identical native/Emscripten exact curve-domain vectors."
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
            / "geometer_exact_curve_domain_test.cjs"
        ),
    )
    args = parser.parse_args()
    native = args.native.resolve() if args.native else _discover_native()
    wasm = args.wasm.resolve()
    if not wasm.is_file():
        raise FileNotFoundError(
            "build the Emscripten exact curve-domain test before parity validation"
        )

    native_fields = _run([str(native)])
    wasm_fields = _run(["node", str(wasm)])
    if native_fields != wasm_fields:
        raise RuntimeError("native and Emscripten exact curve-domain outputs differ")
    if VECTOR_PREFIX not in native_fields or WORK_PREFIX not in native_fields:
        raise RuntimeError("exact curve-domain test omitted a governed output field")

    vector = native_fields[VECTOR_PREFIX].encode("ascii")
    digest = hashlib.sha256(vector).hexdigest()
    work = int(native_fields[WORK_PREFIX])
    manifest = tomllib.loads(MANIFEST.read_text(encoding="utf-8"))
    expected = manifest["analytic_exact_backend"]
    if digest != expected["curve_domain_vector_sha256"]:
        raise RuntimeError("exact curve-domain vector SHA-256 differs from the manifest")
    if work != expected["curve_domain_vector_success_work_units"]:
        raise RuntimeError("exact curve-domain work boundary differs from the manifest")
    print(f"exact curve domain parity: sha256={digest} work={work}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
