"""POSIX exec launcher for the internal topology worker supervisor."""

from __future__ import annotations

import ctypes
import os
import sys
from importlib import import_module
from typing import Any, cast


def _effective_memory_limit(
    requested: int,
    inherited_soft_limit: int,
    inherited_hard_limit: int,
    infinity: int,
) -> int:
    finite_limits = [
        limit for limit in (inherited_soft_limit, inherited_hard_limit) if limit != infinity
    ]
    return min([requested, *finite_limits])


class _DarwinRLimit(ctypes.Structure):
    _fields_ = [("current", ctypes.c_uint64), ("maximum", ctypes.c_uint64)]


def _set_memory_limit(resource: Any, effective_limit: int) -> None:
    if sys.platform != "darwin":
        resource.setrlimit(resource.RLIMIT_AS, (effective_limit, effective_limit))
        return
    libc = ctypes.CDLL(None, use_errno=True)
    setrlimit = libc.setrlimit
    setrlimit.argtypes = (ctypes.c_int, ctypes.POINTER(_DarwinRLimit))
    setrlimit.restype = ctypes.c_int
    limits = _DarwinRLimit(effective_limit, effective_limit)
    if setrlimit(resource.RLIMIT_AS, ctypes.byref(limits)) != 0:
        error_number = ctypes.get_errno()
        raise OSError(error_number, os.strerror(error_number))


def main() -> int:
    if os.name != "posix" or len(sys.argv) < 3:
        return 2
    resource = cast(Any, import_module("resource"))
    memory_limit_bytes = int(sys.argv[1])
    inherited_soft_limit, inherited_hard_limit = resource.getrlimit(resource.RLIMIT_AS)
    effective_limit = _effective_memory_limit(
        memory_limit_bytes,
        inherited_soft_limit,
        inherited_hard_limit,
        resource.RLIM_INFINITY,
    )
    _set_memory_limit(resource, effective_limit)
    os.execv(sys.argv[2], sys.argv[2:])
    return 127


if __name__ == "__main__":
    raise SystemExit(main())
