"""Desktop / Start-menu launcher: make sure WN3D_Editor.exe and its NVIDIA
profile are in place (editor_exe.py), start the editor as WN3D_Editor.exe
with any file passed on the command line, and exit. Run it with the venv's
pythonw.exe so no console appears. Falls back to running the editor in this
process when the own executable cannot be made (non-Windows, no venv)."""

from __future__ import annotations

import subprocess
import sys
from pathlib import Path

HERE = Path(__file__).resolve().parent


def main() -> int:
    sys.path.insert(0, str(HERE))
    import editor_exe

    try:
        exe = editor_exe.ensure_exe()
    except OSError:  # an editor already running holds the exe: use it as it is
        exe = editor_exe.TARGET / editor_exe.EXE
    if exe is None or not exe.is_file():
        import runpy

        sys.argv = [str(HERE / "step_editor.py"), *sys.argv[1:]]
        runpy.run_path(str(HERE / "step_editor.py"), run_name="__main__")
        return 0
    from app.nvidia_profile import ensure_profile

    ensure_profile()
    flags = subprocess.DETACHED_PROCESS | subprocess.CREATE_NEW_PROCESS_GROUP
    subprocess.Popen([str(exe), str(HERE / "step_editor.py"), *sys.argv[1:]], cwd=str(HERE),
                     creationflags=flags, close_fds=True)
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
