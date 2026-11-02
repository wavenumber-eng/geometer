"""An NVIDIA driver profile that keeps the editor off variable refresh.

On a G-SYNC / G-SYNC Compatible monitor the NVIDIA driver lets a windowed
OpenGL "game" drive the panel's refresh rate. The editor's 3D view counts as
one, and on 2026-10-31 the whole desktop got stuck at the editor's ~11 Hz
(below the panel's 50 Hz floor) until a graphics-driver reset. This module
writes one application profile, "WN3D STEP Editor", for the editor's own
executable (WN3D_Editor.exe, see editor_exe.py) with G-SYNC off and a fixed
refresh rate, through NVIDIA's documented NVAPI driver-settings (DRS) calls.
It never touches the global profile or any other application, and it is a
no-op on machines without an NVIDIA driver."""

from __future__ import annotations

import ctypes
import sys

PROFILE_NAME = "WN3D STEP Editor"
APP_NAME = "wn3d_editor.exe"
# NvApiDriverSettings.h
VRR_APP_OVERRIDE_ID, VRR_APP_OVERRIDE_FIXED_REFRESH = 0x10A879CF, 4
VRR_MODE_ID, VRR_MODE_DISABLED = 0x1194F158, 0
SETTINGS = {VRR_APP_OVERRIDE_ID: VRR_APP_OVERRIDE_FIXED_REFRESH, VRR_MODE_ID: VRR_MODE_DISABLED}

# nvapi_QueryInterface ids (nvapi.h)
_IDS = {"Initialize": 0x0150E828, "CreateSession": 0x0694D52E, "LoadSettings": 0x375DBD6B,
        "FindProfileByName": 0x7E4A9A0B, "CreateProfile": 0xCC176068, "FindApplicationByName": 0xEEE566B2,
        "CreateApplication": 0x4347A9DE, "SetSetting": 0x577DD202, "GetSetting": 0x73BF8338,
        "SaveSettings": 0xFCBC7E14, "DestroySession": 0xDAD9CFF8}
NVAPI_OK, PROFILE_NOT_FOUND, EXECUTABLE_NOT_FOUND = 0, -163, -166
_STR = ctypes.c_wchar * 2048  # NvAPI_UnicodeString


def _ustr(text: str):
    value = _STR()
    value.value = text
    return value


class _Binary(ctypes.Structure):
    _fields_ = [("valueLength", ctypes.c_uint32), ("valueData", ctypes.c_ubyte * 4096)]


class _Value(ctypes.Union):
    _fields_ = [("u32", ctypes.c_uint32), ("binary", _Binary), ("wsz", _STR)]


class _Setting(ctypes.Structure):  # NVDRS_SETTING_V1
    _fields_ = [("version", ctypes.c_uint32), ("settingName", _STR), ("settingId", ctypes.c_uint32),
                ("settingType", ctypes.c_uint32), ("settingLocation", ctypes.c_uint32),
                ("isCurrentPredefined", ctypes.c_uint32), ("isPredefinedValid", ctypes.c_uint32),
                ("predefined", _Value), ("current", _Value)]


class _Profile(ctypes.Structure):  # NVDRS_PROFILE_V1
    _fields_ = [("version", ctypes.c_uint32), ("profileName", _STR), ("gpuSupport", ctypes.c_uint32),
                ("isPredefined", ctypes.c_uint32), ("numOfApps", ctypes.c_uint32), ("numOfSettings", ctypes.c_uint32)]


class _Application(ctypes.Structure):  # NVDRS_APPLICATION_V1
    _fields_ = [("version", ctypes.c_uint32), ("isPredefined", ctypes.c_uint32), ("appName", _STR),
                ("userFriendlyName", _STR), ("launcher", _STR)]


def _versioned(struct_type, version: int = 1):
    item = struct_type()
    item.version = ctypes.sizeof(struct_type) | (version << 16)
    return item


class _NvApi:
    def __init__(self) -> None:
        dll = ctypes.WinDLL("nvapi64.dll")
        query = dll.nvapi_QueryInterface
        query.restype, query.argtypes = ctypes.c_void_p, [ctypes.c_uint32]
        self._fn = {}
        for name, fid in _IDS.items():
            address = query(fid)
            if not address:
                raise OSError(f"NVAPI has no {name}")
            fn = ctypes.CFUNCTYPE(ctypes.c_int)(address)
            fn.argtypes = None  # calls take 0-4 arguments of mixed types
            self._fn[name] = fn

    def __call__(self, name: str, *args) -> int:
        return self._fn[name](*args)


def _check(status: int, what: str) -> None:
    if status != NVAPI_OK:
        raise OSError(f"NVAPI {what} failed ({status})")


def _profile_handle(api: _NvApi, session) -> ctypes.c_void_p:
    """Our profile, created when missing."""
    handle = ctypes.c_void_p()
    status = api("FindProfileByName", session, _ustr(PROFILE_NAME), ctypes.byref(handle))
    if status == PROFILE_NOT_FOUND:
        profile = _versioned(_Profile)
        profile.profileName = PROFILE_NAME
        status = api("CreateProfile", session, ctypes.byref(profile), ctypes.byref(handle))
    _check(status, "find/create profile")
    return handle


def _attach_app(api: _NvApi, session, profile) -> None:
    """Put WN3D_Editor.exe in our profile unless it already is in one."""
    owner, app = ctypes.c_void_p(), _versioned(_Application)
    status = api("FindApplicationByName", session, _ustr(APP_NAME), ctypes.byref(owner), ctypes.byref(app))
    if status == NVAPI_OK:
        return
    app = _versioned(_Application)
    app.appName, app.userFriendlyName = APP_NAME, PROFILE_NAME
    _check(api("CreateApplication", session, profile, ctypes.byref(app)), "add application")


def _set_dword(api: _NvApi, session, profile, setting_id: int, value: int) -> None:
    setting = _versioned(_Setting)
    setting.settingId, setting.settingType = setting_id, 0  # NVDRS_DWORD_TYPE
    setting.current.u32 = value
    _check(api("SetSetting", session, profile, ctypes.byref(setting)), f"set 0x{setting_id:08X}")


def _read_dword(api: _NvApi, session, profile, setting_id: int) -> int | None:
    setting = _versioned(_Setting)
    status = api("GetSetting", session, profile, ctypes.c_uint32(setting_id), ctypes.byref(setting))
    return setting.current.u32 if status == NVAPI_OK else None


def ensure_profile() -> str:
    """Create / refresh the profile; returns a one-line status (never raises)."""
    if sys.platform != "win32":
        return "not Windows: no NVIDIA profile needed"
    try:
        api = _NvApi()
    except OSError as exc:
        return f"no NVIDIA driver ({exc})"
    session = ctypes.c_void_p()
    try:
        _check(api("Initialize"), "initialize")
        _check(api("CreateSession", ctypes.byref(session)), "create session")
        _check(api("LoadSettings", session), "load settings")
        profile = _profile_handle(api, session)
        _attach_app(api, session, profile)
        for setting_id, value in SETTINGS.items():
            _set_dword(api, session, profile, setting_id, value)
        _check(api("SaveSettings", session), "save settings")
        read = {f"0x{k:08X}": _read_dword(api, session, profile, k) for k in SETTINGS}
        return f"NVIDIA profile '{PROFILE_NAME}' for {APP_NAME}: {read}"
    except OSError as exc:
        return str(exc)
    finally:
        if session:
            api("DestroySession", session)
