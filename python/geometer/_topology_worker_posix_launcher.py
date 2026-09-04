"""Linux exec launcher for the internal topology worker supervisor."""

from __future__ import annotations

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
    resource.setrlimit(resource.RLIMIT_AS, (effective_limit, effective_limit))
    os.execv(sys.argv[2], sys.argv[2:])
    return 127


if __name__ == "__main__":
    raise SystemExit(main())
