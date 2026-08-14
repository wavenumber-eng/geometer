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
MUTATION_PREFIX = "EXACT_RESULT_NORMALIZATION_MUTATIONS="
COLLAPSE_PREFIX = "EXACT_RESULT_NORMALIZATION_COLLAPSE="


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


def _outputs(command: list[str]) -> tuple[str, list[str], str]:
    completed = subprocess.run(
        command, cwd=ROOT, check=True, capture_output=True, text=True
    )
    vector: str | None = None
    mutations: list[str] | None = None
    collapse: str | None = None
    for line in completed.stdout.splitlines():
        if line.startswith(VECTOR_PREFIX):
            vector = line.removeprefix(VECTOR_PREFIX)
        elif line.startswith(MUTATION_PREFIX):
            mutations = line.removeprefix(MUTATION_PREFIX).split(",")
        elif line.startswith(COLLAPSE_PREFIX):
            collapse = line.removeprefix(COLLAPSE_PREFIX)
    if vector is None or mutations is None or collapse is None:
        raise RuntimeError("exact result-normalization test omitted governed output")
    return vector, mutations, collapse


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
    native_vector, native_mutations, native_collapse = _outputs([str(native)])
    wasm_vector, wasm_mutations, wasm_collapse = _outputs(["node", str(wasm)])
    if native_vector != wasm_vector:
        raise RuntimeError("native and Emscripten result-normalization vectors differ")
    digest = hashlib.sha256(native_vector.encode("ascii")).hexdigest()
    expected = tomllib.loads(MANIFEST.read_text(encoding="utf-8"))["analytic_exact_backend"]
    if digest != expected["result_normalization_vector_sha256"]:
        raise RuntimeError("exact result-normalization SHA-256 differs from the manifest")
    if native_mutations != wasm_mutations or native_mutations != expected[
        "result_normalization_mutation_sentinels"
    ]:
        raise RuntimeError("result-normalization mutation inventories differ")
    if native_collapse != wasm_collapse:
        raise RuntimeError("native and Emscripten normalization-collapse sweeps differ")
    collapse_digest = hashlib.sha256(native_collapse.encode("ascii")).hexdigest()
    if collapse_digest != expected["normalization_collapse_sha256"]:
        raise RuntimeError("normalization-collapse SHA-256 differs from the manifest")
    cursor = 0
    for case in expected["normalization_collapse_cases"]:
        position = native_collapse.find(f"{case}:", cursor)
        if position < cursor:
            raise RuntimeError("normalization-collapse case inventory or order differs")
        cursor = position + len(case) + 1
    print(
        f"exact result normalization parity: sha256={digest} "
        f"mutations={','.join(native_mutations)} collapse_sha256={collapse_digest}"
    )
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
