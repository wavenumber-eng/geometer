"""POSIX exec launcher for the internal topology worker supervisor."""

from __future__ import annotations

import os
import sys
from importlib import import_module
from typing import Any, cast


def main() -> int:
    if os.name != "posix" or len(sys.argv) < 3:
        return 2
    resource = cast(Any, import_module("resource"))
    memory_limit_bytes = int(sys.argv[1])
    resource.setrlimit(resource.RLIMIT_AS, (memory_limit_bytes, memory_limit_bytes))
    os.execv(sys.argv[2], sys.argv[2:])
    return 127


if __name__ == "__main__":
    raise SystemExit(main())
