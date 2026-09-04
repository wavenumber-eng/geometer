from __future__ import annotations

import os
import subprocess
import sys
import tempfile
import threading
import time
from pathlib import Path

import pytest

from geometer._topology_worker_supervisor import (
    TopologyWorkerCancelled,
    TopologyWorkerDeadlineExceeded,
    TopologyWorkerProcessError,
    TopologyWorkerSupervisor,
)


ROOT = Path(__file__).resolve().parents[2]
FIXTURE = ROOT / "tests" / "fixtures" / "step" / "generated_topology" / "generated_repeated_occurrences.step"


def _occt_worker_memory_limit() -> int:
    # A forked OCCT process tree reserves more virtual address space on macOS.
    # The dedicated allocation test below retains its intentionally small cap.
    return (2 * 1024 * 1024 * 1024) if sys.platform == "darwin" else (512 * 1024 * 1024)


def _worker_executable() -> Path:
    suffix = ".exe" if os.name == "nt" else ""
    from geometer._paths import _platform_tag

    name = f"geometer_step_topology_worker_test_server{suffix}"
    candidates = (
        ROOT / f"build-native-{_platform_tag()}" / "tests" / "cpp" / name,
        ROOT / "build" / "tests" / "cpp" / name,
    )
    for candidate in candidates:
        if candidate.is_file():
            return candidate
    pytest.skip("native topology containment test worker is unavailable")


def _session_handle(output: bytes) -> str:
    handle = output.decode("ascii").strip()
    assert handle.startswith("gts_") and len(handle) == 68
    return handle


def test_deadline_kills_real_occt_session_and_descendant_then_allows_replacement() -> None:
    supervisor = TopologyWorkerSupervisor(_worker_executable(), memory_limit_bytes=_occt_worker_memory_limit())
    with pytest.raises(TopologyWorkerDeadlineExceeded) as caught:
        supervisor.run(("open-hold-tree", FIXTURE), timeout=0.5)
    old_handle = _session_handle(caught.value.stdout)
    assert not caught.value.temporary_directory.exists()
    assert supervisor.active_process_id is None

    replacement = supervisor.run(("open-once", FIXTURE), timeout=5)
    assert replacement.generation == caught.value.generation + 1
    assert _session_handle(replacement.stdout) != old_handle
    assert not replacement.temporary_directory.exists()


def test_explicit_cancel_kills_active_generation_and_cleans_private_temp() -> None:
    supervisor = TopologyWorkerSupervisor(_worker_executable(), memory_limit_bytes=_occt_worker_memory_limit())
    failure: list[BaseException] = []

    def run_worker() -> None:
        try:
            supervisor.run(("open-hold", FIXTURE), timeout=10)
        except BaseException as error:
            failure.append(error)

    thread = threading.Thread(target=run_worker)
    thread.start()
    deadline = time.monotonic() + 5
    while supervisor.active_process_id is None and time.monotonic() < deadline:
        time.sleep(0.01)
    assert supervisor.cancel()
    thread.join(timeout=5)
    assert not thread.is_alive()
    assert len(failure) == 1 and isinstance(failure[0], TopologyWorkerCancelled)
    cancellation = failure[0]
    assert isinstance(cancellation, TopologyWorkerCancelled)
    assert not cancellation.outcome.temporary_directory.exists()
    assert supervisor.active_process_id is None


def test_os_memory_ceiling_rejects_worker_allocation_and_cleans_temp() -> None:
    supervisor = TopologyWorkerSupervisor(_worker_executable(), memory_limit_bytes=64 * 1024 * 1024)
    with pytest.raises(TopologyWorkerProcessError) as caught:
        supervisor.run(("allocate", str(256 * 1024 * 1024)), timeout=10)
    assert caught.value.outcome.return_code != 0
    assert not caught.value.outcome.temporary_directory.exists()


def test_communicate_failure_still_kills_reaps_and_cleans_worker(
    monkeypatch: pytest.MonkeyPatch,
) -> None:
    supervisor = TopologyWorkerSupervisor(_worker_executable(), memory_limit_bytes=512 * 1024 * 1024)
    existing_temp = set(Path(tempfile.gettempdir()).glob("geometer-topology-worker-*"))
    original_communicate = subprocess.Popen.communicate
    workers: list[subprocess.Popen[bytes]] = []
    calls = 0

    def fail_first_communicate(
        process: subprocess.Popen[bytes],
        input: bytes | None = None,
        timeout: float | None = None,
    ) -> tuple[bytes, bytes]:
        nonlocal calls
        calls += 1
        workers.append(process)
        if calls == 1:
            raise OSError("injected supervisor communicate failure")
        return original_communicate(process, input=input, timeout=timeout)

    monkeypatch.setattr(subprocess.Popen, "communicate", fail_first_communicate)
    with pytest.raises(OSError, match="injected supervisor communicate failure"):
        supervisor.run(("open-hold", FIXTURE), timeout=5)

    assert workers and workers[0].poll() is not None
    assert supervisor.active_process_id is None
    assert set(Path(tempfile.gettempdir()).glob("geometer-topology-worker-*")) == existing_temp
