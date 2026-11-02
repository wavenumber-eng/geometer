"""Prepare the editor's own executable (Windows) and print its path.

The editor's GPU process would otherwise be the generic pythonw.exe of the
base interpreter, which the NVIDIA driver treats as an unknown "game" (it may
let it drive a G-SYNC monitor's refresh rate; see app/nvidia_profile.py).
This makes .venv/wn3d/WN3D_Editor.exe, a copy of the base pythonw.exe with
its DLLs; Python finds the venv through .venv/pyvenv.cfg one folder up, so it
runs with the venv's packages. The copy is refreshed when the base
interpreter changes. Then the NVIDIA profile for WN3D_Editor.exe is written.

Usage: <venv python> editor_exe.py   ->  prints the executable to launch
(falls back to the venv's pythonw.exe when anything is missing)."""

from __future__ import annotations

import shutil
import sys
from pathlib import Path

HERE = Path(__file__).resolve().parent
VENV = HERE / ".venv"
TARGET = VENV / "wn3d"
EXE = "WN3D_Editor.exe"
FILES = {"pythonw.exe": EXE, "python3.dll": None, "python312.dll": None,
         "vcruntime140.dll": None, "vcruntime140_1.dll": None}


def _base_home() -> Path | None:
    cfg = VENV / "pyvenv.cfg"
    if not cfg.is_file():
        return None
    for line in cfg.read_text(encoding="utf-8").splitlines():
        key, _, value = line.partition("=")
        if key.strip() == "home":
            return Path(value.strip())
    return None


def _stale(source: Path, copy: Path) -> bool:
    if not copy.is_file():
        return True
    a, b = source.stat(), copy.stat()
    return a.st_size != b.st_size or a.st_mtime > b.st_mtime + 1


def ensure_exe() -> Path | None:
    """The editor executable, (re)made from the base interpreter as needed."""
    home = _base_home()
    if sys.platform != "win32" or home is None or not (home / "pythonw.exe").is_file():
        return None
    TARGET.mkdir(parents=True, exist_ok=True)
    for name, renamed in FILES.items():
        source, copy = home / name, TARGET / (renamed or name)
        if source.is_file() and _stale(source, copy):
            shutil.copy2(source, copy)
    exe = TARGET / EXE
    return exe if exe.is_file() else None


def main() -> int:
    fallback = VENV / "Scripts" / "pythonw.exe"
    try:
        exe = ensure_exe()
    except OSError as exc:  # a running editor holds the exe: reuse it as it is
        print(f"editor exe not refreshed: {exc}", file=sys.stderr)
        exe = TARGET / EXE if (TARGET / EXE).is_file() else None
    if exe is not None:
        sys.path.insert(0, str(HERE))
        from app.nvidia_profile import ensure_profile

        print(ensure_profile(), file=sys.stderr)
    print(exe or fallback)
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
