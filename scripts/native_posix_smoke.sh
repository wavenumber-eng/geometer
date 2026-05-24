#!/usr/bin/env bash
set -euo pipefail

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
cd "$ROOT"

platform_tag="$(
python3 - <<'PY'
import platform
import sys

if sys.platform == "darwin":
    os_name = "macos"
elif sys.platform.startswith("linux"):
    os_name = "linux"
else:
    os_name = sys.platform.replace("_", "-").replace(".", "-")

machine = platform.machine().strip().lower()
if machine in {"amd64", "x86_64"}:
    arch = "x64"
elif machine in {"aarch64", "arm64"}:
    arch = "arm64"
elif machine in {"i386", "i686", "x86"}:
    arch = "x86"
else:
    arch = machine or "unknown"

print(f"{os_name}-{arch}")
PY
)"

echo "Geometer POSIX smoke"
echo "root: $ROOT"
echo "platform: $platform_tag"

cmake --preset default
cmake --build build --config Release

exe="$ROOT/dist/native/$platform_tag/geometer"
if [[ ! -x "$exe" ]]; then
    echo "Missing executable: $exe" >&2
    exit 1
fi

"$exe" --version

mkdir -p "$ROOT/out/posix-smoke"
"$exe" step-project-hlr \
    "$ROOT/tests/fixtures/step/embedded_models/SOT-23.STEP" \
    "$ROOT/out/posix-smoke/SOT-23.projection.json"

GEOMETER_EXE="$exe" PYTHONPATH="$ROOT/python" python3 - <<'PY'
from pathlib import Path
import geometer

root = Path.cwd()
version = geometer.version()
projection = geometer.project_step_hlr(
    root / "tests/fixtures/step/embedded_models/SOT-23.STEP",
    views=[geometer.ProjectionView.top()],
)
print(f"python geometer {version.string} abi {version.abi}")
print(f"projection {projection.schema} views={len(projection.views)}")
PY

if [[ "${GEOMETER_SKIP_CTEST:-0}" != "1" ]]; then
    ctest --test-dir build -C Release --output-on-failure
fi

echo "POSIX smoke complete"
