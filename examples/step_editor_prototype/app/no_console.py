"""Keep helper programs from flashing console windows on Windows.

The editor runs under pythonw.exe (no console), so every console program it
starts (geometer.exe for the top-view projection, kicad-cli, ...) gets a new,
visible console window that takes the foreground. Each of those focus
switches makes the NVIDIA driver hand the monitor's variable refresh rate
back and forth between the editor and the desktop, and one such handoff left
the whole desktop stuck at ~11 Hz (2026-10-31). Child processes are started
with CREATE_NO_WINDOW unless the caller chose its own creation flags."""

from __future__ import annotations

import subprocess
import sys

_installed = False


def install() -> None:
    """Default every subprocess.Popen to CREATE_NO_WINDOW (Windows only)."""
    global _installed
    if _installed or sys.platform != "win32":
        return
    original = subprocess.Popen.__init__

    def quiet_init(self, *args, **kwargs):
        if not kwargs.get("creationflags"):
            kwargs["creationflags"] = subprocess.CREATE_NO_WINDOW
        original(self, *args, **kwargs)

    subprocess.Popen.__init__ = quiet_init
    _installed = True
