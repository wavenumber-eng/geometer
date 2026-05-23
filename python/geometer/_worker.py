from __future__ import annotations

import base64
import ctypes
import json
import os
import sys

from ._errors import GeometerError
from ._native import native


def main() -> None:
    try:
        request = json.loads(sys.stdin.read())
        operation = request.get("operation")
        step_bytes = base64.b64decode(request.get("step_b64", ""))
        options_text = request.get("options_json")
        options_json = options_text.encode("utf-8") if isinstance(options_text, str) else None

        if operation == "projection_json":
            text = native().projection_json(step_bytes, options_json)
            _write({"ok": True, "text": text})
        elif operation == "step_to_glb":
            glb_bytes = native().step_to_glb(step_bytes, options_json)
            _write({"ok": True, "bytes_b64": base64.b64encode(glb_bytes).decode("ascii")})
        else:
            _write(
                {
                    "ok": False,
                    "code": -1,
                    "message": f"unknown worker operation: {operation}",
                    "function": "geometer._worker",
                }
            )
    except GeometerError as exc:
        _write(
            {
                "ok": False,
                "code": exc.code,
                "message": exc.message,
                "function": exc.function,
            }
        )
    except Exception as exc:
        _write(
            {
                "ok": False,
                "code": -1,
                "message": str(exc),
                "function": "geometer._worker",
            }
        )
    finally:
        sys.stdout.flush()
        sys.stderr.flush()
        _hard_exit(0)


def _write(payload: dict[str, object]) -> None:
    sys.stdout.write(json.dumps(payload, separators=(",", ":")))


def _hard_exit(code: int) -> None:
    if sys.platform == "win32":
        kernel32 = ctypes.WinDLL("kernel32", use_last_error=True)
        kernel32.GetCurrentProcess.argtypes = []
        kernel32.GetCurrentProcess.restype = ctypes.c_void_p
        kernel32.TerminateProcess.argtypes = [ctypes.c_void_p, ctypes.c_uint]
        kernel32.TerminateProcess.restype = ctypes.c_int
        kernel32.TerminateProcess(kernel32.GetCurrentProcess(), code)
    os._exit(code)


if __name__ == "__main__":
    main()
