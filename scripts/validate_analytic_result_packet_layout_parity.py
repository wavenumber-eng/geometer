from __future__ import annotations

import argparse
import hashlib
import subprocess
import sys
import tomllib
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
MANIFEST = ROOT / "docs" / "contracts" / "promotion-manifest.toml"
EMPTY = "ANALYTIC_RESULT_PACKET_EMPTY="
VECTOR = "ANALYTIC_RESULT_PACKET_LAYOUT_VECTOR="


def _discover_native() -> Path:
    names = (
        "geometer_analytic_result_packet_layout_test.exe",
        "geometer_analytic_result_packet_layout_test",
    )
    platform_fragment = {"win32": "windows", "linux": "linux", "darwin": "macos"}.get(
        sys.platform, "*"
    )
    for build in sorted(ROOT.glob(f"build-native-{platform_fragment}-*")):
        for name in names:
            candidate = build / "tests" / "cpp" / name
            if candidate.is_file():
                return candidate
    raise FileNotFoundError("build the native analytic result-packet layout test first")


def _run(command: list[str]) -> dict[str, bytes]:
    completed = subprocess.run(
        command, cwd=ROOT, check=True, capture_output=True, text=True
    )
    fields: dict[str, bytes] = {}
    for line in completed.stdout.splitlines():
        for prefix in (EMPTY, VECTOR):
            if line.startswith(prefix):
                fields[prefix] = bytes.fromhex(line.removeprefix(prefix))
    return fields


def main() -> int:
    parser = argparse.ArgumentParser(
        description="Verify native/Emscripten analytic result-packet layout bytes."
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
            / "geometer_analytic_result_packet_layout_test.cjs"
        ),
    )
    args = parser.parse_args()
    native = args.native.resolve() if args.native else _discover_native()
    wasm = args.wasm.resolve()
    if not wasm.is_file():
        raise FileNotFoundError("build the Emscripten analytic result-packet layout test first")
    native_fields = _run([str(native)])
    wasm_fields = _run(["node", str(wasm)])
    if native_fields != wasm_fields or set(native_fields) != {EMPTY, VECTOR}:
        raise RuntimeError("native and Emscripten result-packet layout bytes differ")

    expected = tomllib.loads(MANIFEST.read_text(encoding="utf-8"))["analytic_exact_backend"]
    checks = (
        (EMPTY, "result_packet_empty_bytes", "result_packet_empty_sha256"),
        (VECTOR, "result_packet_layout_vector_bytes", "result_packet_layout_vector_sha256"),
    )
    for prefix, size_key, digest_key in checks:
        value = native_fields[prefix]
        if len(value) != expected[size_key]:
            raise RuntimeError(f"{prefix} byte count differs from the manifest")
        if hashlib.sha256(value).hexdigest() != expected[digest_key]:
            raise RuntimeError(f"{prefix} SHA-256 differs from the manifest")
    print(
        "analytic result-packet layout parity: "
        f"empty={expected['result_packet_empty_sha256']} "
        f"all_tables={expected['result_packet_layout_vector_sha256']}"
    )
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
