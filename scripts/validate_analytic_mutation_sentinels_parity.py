from __future__ import annotations

import argparse
import subprocess
import tomllib
from pathlib import Path


ROOT = Path(__file__).resolve().parents[1]
MANIFEST = ROOT / "docs" / "contracts" / "promotion-manifest.toml"
PREFIX = "ANALYTIC_MUTATION_SENTINELS="


def _discover_native() -> Path:
    candidates = [
        ROOT
        / "build-native-windows-x64"
        / "tests"
        / "cpp"
        / "geometer_analytic_result_packet_topology_test.exe",
        ROOT / "build" / "tests" / "cpp" / "geometer_analytic_result_packet_topology_test",
    ]
    for candidate in candidates:
        if candidate.is_file():
            return candidate
    raise FileNotFoundError("build the native analytic topology test first")


def _sentinels(command: list[str]) -> list[str]:
    completed = subprocess.run(
        command, cwd=ROOT, check=True, capture_output=True, text=True
    )
    for line in completed.stdout.splitlines():
        if line.startswith(PREFIX):
            return line.removeprefix(PREFIX).split(",")
    raise RuntimeError("analytic topology test omitted its mutation inventory")


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
        / "geometer_analytic_result_packet_topology_test.cjs",
    )
    args = parser.parse_args()
    native = args.native.resolve() if args.native else _discover_native()
    wasm = args.wasm.resolve()
    if not wasm.is_file():
        raise FileNotFoundError("build the Emscripten analytic topology test first")
    native_values = _sentinels([str(native)])
    wasm_values = _sentinels(["node", str(wasm)])
    expected = tomllib.loads(MANIFEST.read_text(encoding="utf-8"))["analytic_exact_backend"][
        "mutation_sentinels"
    ]
    if native_values != wasm_values or native_values != expected:
        raise RuntimeError("native, Emscripten, and governed mutation inventories differ")
    print("analytic mutation sentinel parity: " + ",".join(native_values))
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
