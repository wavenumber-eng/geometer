"""Run executable-backed Python HLR/GLB calls in fresh subprocesses.

This is a packaging smoke test for the public Python API. The API should use
the platform native `geometer` executable, not direct ctypes loading.
"""

from __future__ import annotations

import argparse
import os
import subprocess
import sys
from pathlib import Path


ROOT = Path(__file__).resolve().parent.parent
DEFAULT_STEP = ROOT / "tests" / "fixtures" / "step" / "embedded_models" / "SOT-23.STEP"


CHILD_CODE = r"""
import geometer

step = r"__GEOMETER_STEP_PATH__"
version = geometer.version()
projection = geometer.hlr_projection_json(
    step,
    views=[geometer.ProjectionView.top()],
    options={"curve_mode": "polyline"},
)
glb = geometer.step_to_glb(step)
print(
    f"version={version.string} abi={version.abi} "
    f"projection_bytes={len(projection)} glb_bytes={len(glb)}"
)
"""


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--runs", type=int, default=3, help="Subprocess runs to execute")
    parser.add_argument("--step", type=Path, default=DEFAULT_STEP, help="STEP file to smoke")
    parser.add_argument(
        "--exe",
        type=Path,
        default=None,
        help="Optional geometer executable path for GEOMETER_EXE",
    )
    args = parser.parse_args()

    if args.runs < 1:
        parser.error("--runs must be at least 1")
    step = args.step.resolve()
    if not step.exists():
        raise FileNotFoundError(step)

    env = os.environ.copy()
    env["PYTHONPATH"] = _prepend_pythonpath(ROOT / "python", env.get("PYTHONPATH"))
    env["GEOMETER_BACKEND"] = "exe"
    if args.exe is not None:
        env["GEOMETER_EXE"] = str(args.exe.resolve())

    child_code = CHILD_CODE.replace("__GEOMETER_STEP_PATH__", str(step))
    for index in range(1, args.runs + 1):
        completed = subprocess.run(
            [sys.executable, "-c", child_code],
            cwd=ROOT,
            env=env,
            capture_output=True,
            text=True,
            check=False,
        )
        print(f"[{index}/{args.runs}] exit={completed.returncode}")
        if completed.stdout:
            print(completed.stdout, end="")
        if completed.stderr:
            print(completed.stderr, end="", file=sys.stderr)
        if completed.returncode != 0:
            return completed.returncode
    return 0


def _prepend_pythonpath(path: Path, current: str | None) -> str:
    if not current:
        return str(path)
    return str(path) + os.pathsep + current


if __name__ == "__main__":
    raise SystemExit(main())
