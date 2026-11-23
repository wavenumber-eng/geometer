"""Collection policy shared by local and CI Python validation."""

from __future__ import annotations

import os

collect_ignore: list[str] = []

if os.environ.get("GEOMETER_TEST_PROFILE") == "production":
    collect_ignore.extend(
        [
            "test_contract_promotion_implementation.py",
            "test_contract_promotion_manifest.py",
            "test_topology_glb_raycast.py",
            "test_topology_worker_supervisor.py",
        ]
    )
