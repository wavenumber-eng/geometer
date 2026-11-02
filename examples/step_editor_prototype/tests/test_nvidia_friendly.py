"""The editor stays friendly to the NVIDIA driver on Windows: helper programs
start without console windows (each focus switch re-triggers the driver's
refresh-rate handoff), and the DRS structs match nvapi.h exactly (a wrong
size would make the driver reject, or misread, the profile)."""

import ctypes
import subprocess
import sys

import pytest

from app import no_console, nvidia_profile


def test_drs_struct_sizes_match_nvapi_headers():
    assert ctypes.sizeof(nvidia_profile._Setting) == 12320
    assert ctypes.sizeof(nvidia_profile._Profile) == 4116
    assert ctypes.sizeof(nvidia_profile._Application) == 12296
    assert nvidia_profile._versioned(nvidia_profile._Setting).version == 12320 | (1 << 16)


def test_profile_turns_variable_refresh_off():
    assert nvidia_profile.SETTINGS[nvidia_profile.VRR_MODE_ID] == nvidia_profile.VRR_MODE_DISABLED
    assert nvidia_profile.SETTINGS[nvidia_profile.VRR_APP_OVERRIDE_ID] == nvidia_profile.VRR_APP_OVERRIDE_FIXED_REFRESH


@pytest.mark.skipif(sys.platform != "win32", reason="console windows are a Windows matter")
def test_children_start_without_a_console(monkeypatch):
    seen = {}
    original = subprocess.Popen.__init__

    def spy(self, *args, **kwargs):
        seen.update(kwargs)
        raise RuntimeError("stop before spawning")

    monkeypatch.setattr(subprocess.Popen, "__init__", spy)
    monkeypatch.setattr(no_console, "_installed", False)
    no_console.install()
    with pytest.raises(RuntimeError):
        subprocess.Popen(["cmd", "/c", "exit"])
    assert seen.get("creationflags") == subprocess.CREATE_NO_WINDOW
    subprocess.Popen.__init__ = original  # monkeypatch restores too; be explicit
