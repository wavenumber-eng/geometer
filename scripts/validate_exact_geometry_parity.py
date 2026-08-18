from __future__ import annotations

import argparse
import hashlib
import subprocess
import tomllib
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
MANIFEST = ROOT / "docs" / "contracts" / "promotion-manifest.toml"
ARTIFACT_PREFIX = "GEXPA001_GEOMETRY_HEX="
WORK_PREFIX = "EXACT_GEOMETRY_WORK="


def _discover_native() -> Path:
    names = ("geometer_exact_geometry_test.exe", "geometer_exact_geometry_test")
    for build in sorted(ROOT.glob("build-native-*")):
        for name in names:
            candidate = build / "tests" / "cpp" / name
            if candidate.is_file():
                return candidate
    raise FileNotFoundError("build the native exact-geometry test before parity validation")


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
        description="Verify byte-identical native/Emscripten exact-geometry vectors."
    )
    parser.add_argument("--native", type=Path)
    parser.add_argument(
        "--wasm",
        type=Path,
        default=ROOT / "build-wasm" / "tests" / "cpp" / "geometer_exact_geometry_test.cjs",
    )
    args = parser.parse_args()
    native = args.native.resolve() if args.native else _discover_native()
    wasm = args.wasm.resolve()
    if not wasm.is_file():
        raise FileNotFoundError("build the Emscripten exact-geometry test before parity validation")

    native_fields = _run([str(native)])
    wasm_fields = _run(["node", str(wasm)])
    if native_fields != wasm_fields:
        raise RuntimeError("native and Emscripten exact-geometry outputs differ")
    if ARTIFACT_PREFIX not in native_fields or WORK_PREFIX not in native_fields:
        raise RuntimeError("exact-geometry test omitted a governed output field")

    artifact = bytes.fromhex(native_fields[ARTIFACT_PREFIX])
    digest = hashlib.sha256(artifact).hexdigest()
    work = int(native_fields[WORK_PREFIX])
    manifest = tomllib.loads(MANIFEST.read_text(encoding="utf-8"))
    expected = manifest["analytic_exact_backend"]
    if len(artifact) != expected["geometry_vector_artifact_bytes"]:
        raise RuntimeError("exact-geometry artifact byte length differs from the manifest")
    if digest != expected["geometry_vector_artifact_sha256"]:
        raise RuntimeError("exact-geometry artifact SHA-256 differs from the manifest")
    if work != expected["geometry_vector_success_work_units"]:
        raise RuntimeError("exact-geometry work boundary differs from the manifest")
    print(f"exact geometry parity: {len(artifact)} bytes sha256={digest} work={work}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
