from __future__ import annotations

import ctypes
from dataclasses import dataclass
from pathlib import Path

from ._errors import GeometerError
from ._paths import load_native_library
from ._types import Version


@dataclass(frozen=True)
class NativeLibrary:
    library: ctypes.CDLL
    path: Path | None

    def version(self) -> Version:
        text = self.library.geometer_version_string().decode("utf-8")
        return Version(
            major=int(self.library.geometer_version_major()),
            minor=int(self.library.geometer_version_minor()),
            patch=int(self.library.geometer_version_patch()),
            abi=int(self.library.geometer_abi_version()),
            string=text,
        )

    def projection_json(self, step_bytes: bytes, options_json: bytes | None) -> str:
        value = ctypes.c_void_p()
        error = ctypes.c_void_p()
        input_buffer, input_ptr = _byte_input(step_bytes)
        code = self.library.geometer_step_hlr_projection_json_bytes(
            input_ptr,
            len(step_bytes),
            options_json,
            ctypes.byref(value),
            ctypes.byref(error),
        )
        _keep_alive(input_buffer)
        if code != 0:
            raise self._error(code, error, "geometer_step_hlr_projection_json_bytes")
        try:
            return ctypes.string_at(value).decode("utf-8")
        finally:
            self.library.geometer_free_string(value)

    def step_to_glb(self, step_bytes: bytes, options_json: bytes | None) -> bytes:
        value = ctypes.c_void_p()
        value_size = ctypes.c_size_t()
        error = ctypes.c_void_p()
        input_buffer, input_ptr = _byte_input(step_bytes)
        code = self.library.geometer_step_to_glb_bytes(
            input_ptr,
            len(step_bytes),
            options_json,
            ctypes.byref(value),
            ctypes.byref(value_size),
            ctypes.byref(error),
        )
        _keep_alive(input_buffer)
        if code != 0:
            raise self._error(code, error, "geometer_step_to_glb_bytes")
        try:
            return ctypes.string_at(value, value_size.value)
        finally:
            self.library.geometer_free_bytes(value)

    def _error(self, code: int, error_ptr: ctypes.c_void_p, function: str) -> GeometerError:
        message = _take_error_message(self.library, error_ptr)
        version = self.version()
        return GeometerError(
            code=code,
            message=message,
            function=function,
            version=version.string,
            abi=version.abi,
        )


_NATIVE: NativeLibrary | None = None


def native() -> NativeLibrary:
    global _NATIVE
    if _NATIVE is None:
        library, path = load_native_library()
        _configure_signatures(library)
        _NATIVE = NativeLibrary(library=library, path=path)
    return _NATIVE


def _configure_signatures(library: ctypes.CDLL) -> None:
    library.geometer_version_string.argtypes = []
    library.geometer_version_string.restype = ctypes.c_char_p
    library.geometer_version_major.argtypes = []
    library.geometer_version_major.restype = ctypes.c_int
    library.geometer_version_minor.argtypes = []
    library.geometer_version_minor.restype = ctypes.c_int
    library.geometer_version_patch.argtypes = []
    library.geometer_version_patch.restype = ctypes.c_int
    library.geometer_abi_version.argtypes = []
    library.geometer_abi_version.restype = ctypes.c_int

    library.geometer_step_hlr_projection_json_bytes.argtypes = [
        ctypes.c_void_p,
        ctypes.c_size_t,
        ctypes.c_char_p,
        ctypes.POINTER(ctypes.c_void_p),
        ctypes.POINTER(ctypes.c_void_p),
    ]
    library.geometer_step_hlr_projection_json_bytes.restype = ctypes.c_int

    library.geometer_step_to_glb_bytes.argtypes = [
        ctypes.c_void_p,
        ctypes.c_size_t,
        ctypes.c_char_p,
        ctypes.POINTER(ctypes.c_void_p),
        ctypes.POINTER(ctypes.c_size_t),
        ctypes.POINTER(ctypes.c_void_p),
    ]
    library.geometer_step_to_glb_bytes.restype = ctypes.c_int

    library.geometer_free_string.argtypes = [ctypes.c_void_p]
    library.geometer_free_string.restype = None
    library.geometer_free_bytes.argtypes = [ctypes.c_void_p]
    library.geometer_free_bytes.restype = None


def _byte_input(data: bytes) -> tuple[ctypes.Array[ctypes.c_ubyte] | None, ctypes.c_void_p | None]:
    if not data:
        return None, None
    buffer = (ctypes.c_ubyte * len(data)).from_buffer_copy(data)
    return buffer, ctypes.cast(buffer, ctypes.c_void_p)


def _take_error_message(library: ctypes.CDLL, error_ptr: ctypes.c_void_p) -> str:
    if not error_ptr:
        return "native call failed without an error message"
    try:
        return ctypes.string_at(error_ptr).decode("utf-8", errors="replace")
    finally:
        library.geometer_free_string(error_ptr)


def _keep_alive(_value: object) -> None:
    return None
