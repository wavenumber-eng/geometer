"""Collection policy shared by local and CI Python validation."""

from __future__ import annotations

import os

import pytest


collect_ignore: list[str] = []

if os.environ.get("GEOMETER_TEST_PROFILE") == "production":
    collect_ignore.extend(
        [
            "test_analytic_packet_a0.py",
            "test_analytic_qualification.py",
            "test_build_boost.py",
            "test_contract_promotion_implementation.py",
            "test_contract_promotion_manifest.py",
            "test_matz_observation_replay.py",
            "test_topology_glb_raycast.py",
            "test_topology_worker_supervisor.py",
        ]
    )


EXPERIMENTAL_ITEM_PREFIXES = (
    "tests/python/test_generated_contracts.py::test_generated_python_exposes_analytic_logical_models",
    "tests/python/test_ipc_client.py::test_friendly_analytic_call_",
    "tests/python/test_ipc_client.py::test_persistent_client_repeats_nonempty_solve_",
    "tests/python/test_ipc_client.py::test_analytic_projection_or_packet_protocol_fault_",
    "tests/python/test_ipc_client.py::test_public_package_exports_client_and_analytic_construction_",
)


def pytest_collection_modifyitems(items: list[pytest.Item]) -> None:
    """Keep mixed production modules while omitting their analytic-solver cases."""
    if os.environ.get("GEOMETER_TEST_PROFILE") != "production":
        return
    marker = pytest.mark.skip(reason="experimental analytic-solver validation is opt-in")
    for item in items:
        normalized_node_id = item.nodeid.replace("\\", "/")
        if any(normalized_node_id.startswith(prefix) for prefix in EXPERIMENTAL_ITEM_PREFIXES):
            item.add_marker(marker)
