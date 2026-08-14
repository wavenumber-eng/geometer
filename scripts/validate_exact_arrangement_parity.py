from __future__ import annotations

import argparse
import hashlib
import subprocess
import sys
import tomllib
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
MANIFEST = ROOT / "docs" / "contracts" / "promotion-manifest.toml"
VECTOR_PREFIX = "EXACT_ARRANGEMENT_VECTOR="
WORK_PREFIX = "EXACT_ARRANGEMENT_WORK="
STORAGE_PREFIX = "EXACT_ARRANGEMENT_STORAGE="


def _discover_native() -> Path:
    names = ("geometer_exact_arrangement_test.exe", "geometer_exact_arrangement_test")
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
    raise FileNotFoundError("build the native exact arrangement test before parity validation")


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
        description="Verify identical native/Emscripten exact arrangement vectors."
    )
    parser.add_argument("--native", type=Path)
    parser.add_argument(
        "--wasm",
        type=Path,
        default=(
            ROOT / "build-wasm" / "tests" / "cpp" / "geometer_exact_arrangement_test.cjs"
        ),
    )
    args = parser.parse_args()
    native = args.native.resolve() if args.native else _discover_native()
    wasm = args.wasm.resolve()
    if not wasm.is_file():
        raise FileNotFoundError("build the Emscripten exact arrangement test first")

    native_fields = _run([str(native)])
    wasm_fields = _run(["node", str(wasm)])
    if native_fields != wasm_fields:
        raise RuntimeError("native and Emscripten exact arrangement outputs differ")
    required = (VECTOR_PREFIX, WORK_PREFIX, STORAGE_PREFIX)
    if any(field not in native_fields for field in required):
        raise RuntimeError("exact arrangement test omitted a governed output field")

    vector = native_fields[VECTOR_PREFIX].encode("ascii")
    digest = hashlib.sha256(vector).hexdigest()
    work = int(native_fields[WORK_PREFIX])
    storage = int(native_fields[STORAGE_PREFIX])
    expected = tomllib.loads(MANIFEST.read_text(encoding="utf-8"))["analytic_exact_backend"]
    if digest != expected["arrangement_vector_sha256"]:
        raise RuntimeError("exact arrangement vector SHA-256 differs from the manifest")
    if work != expected["arrangement_vector_success_work_units"]:
        raise RuntimeError("exact arrangement work boundary differs from the manifest")
    if storage != expected["arrangement_vector_storage_bytes"]:
        raise RuntimeError("exact arrangement storage boundary differs from the manifest")
    print(f"exact arrangement parity: sha256={digest} work={work} storage={storage}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
