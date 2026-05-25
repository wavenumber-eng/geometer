from __future__ import annotations

import subprocess
import sys
from pathlib import Path

from ._paths import executable_path


def main() -> int:
    try:
        exe = executable_path()
    except FileNotFoundError as exc:
        print(str(exc), file=sys.stderr)
        return 127

    if _is_self(exe):
        print(
            "Could not find the native Geometer executable. Build the native CLI "
            "or set GEOMETER_EXE to the executable path.",
            file=sys.stderr,
        )
        return 127

    try:
        completed = subprocess.run([str(exe), *sys.argv[1:]])
    except KeyboardInterrupt:
        return 130
    if completed.returncode < 0:
        return 128 - completed.returncode
    return completed.returncode


def _is_self(path: Path) -> bool:
    if not sys.argv:
        return False
    try:
        return path.resolve() == Path(sys.argv[0]).resolve()
    except OSError:
        return False


if __name__ == "__main__":
    raise SystemExit(main())
