from __future__ import annotations

import importlib.util
import threading
import time
from pathlib import Path
from types import SimpleNamespace

import pytest


ROOT = Path(__file__).resolve().parents[2]
SCRIPT = ROOT / "scripts/benchmark_operation_transport.py"
SPEC = importlib.util.spec_from_file_location("benchmark_operation_transport", SCRIPT)
if SPEC is None or SPEC.loader is None:
    raise RuntimeError("Could not load scripts/benchmark_operation_transport.py")
benchmark = importlib.util.module_from_spec(SPEC)
SPEC.loader.exec_module(benchmark)


def test_percentile_uses_nearest_rank() -> None:
    values = [9.0, 1.0, 3.0, 7.0, 5.0]

    assert benchmark.percentile(values, 0.0) == 1.0
    assert benchmark.percentile(values, 0.5) == 5.0
    assert benchmark.percentile(values, 0.95) == 9.0
    assert benchmark.percentile(values, 1.0) == 9.0


def test_summary_preserves_population_extremes() -> None:
    assert benchmark.summary_ms([1.0, 2.0, 8.0, 9.0]) == {
        "minimum": 1.0,
        "median": 5.0,
        "p95": 9.0,
        "maximum": 9.0,
    }


def test_invalid_percentile_is_rejected() -> None:
    with pytest.raises(ValueError, match="requires values"):
        benchmark.percentile([], 0.5)
    with pytest.raises(ValueError, match="requires values"):
        benchmark.percentile([1.0], 1.1)


def test_request_uses_the_governed_native_operation() -> None:
    operation_request = benchmark.request()

    assert operation_request.schema == "geometry.model_illustration_geometry.request.a0"
    assert operation_request.source.kind == "model"
    assert operation_request.source.attachment == "model"
    assert operation_request.style.show_hlr_detail is True


class FakeClient:
    next_pid = 1000
    instances: list[FakeClient] = []
    corrupt_call: tuple[int, int] | None = None

    def __init__(self, _executable: Path, *, client_name: str) -> None:
        self.client_name = client_name
        self.process_id = FakeClient.next_pid
        FakeClient.next_pid += 1
        self.calls = 0
        self.active = 0
        self.max_active = 0
        self.thread_ids: set[int] = set()
        self.closed = False
        FakeClient.instances.append(self)

    def model_illustration_geometry(self, _request: object, _model: bytes, *, timeout: float) -> SimpleNamespace:
        assert timeout == 180.0
        self.active += 1
        self.max_active = max(self.max_active, self.active)
        self.thread_ids.add(threading.get_ident())
        self.calls += 1
        time.sleep(0.002)
        digest = "stable"
        instance_index = FakeClient.instances.index(self)
        if FakeClient.corrupt_call == (instance_index, self.calls):
            digest = "corrupt"
        self.active -= 1
        geometry = SimpleNamespace(byte_length=7, sha256=digest)
        return SimpleNamespace(metadata=SimpleNamespace(geometry=geometry))

    def close(self) -> None:
        self.closed = True


@pytest.fixture
def fake_runtime(monkeypatch: pytest.MonkeyPatch) -> None:
    FakeClient.next_pid = 1000
    FakeClient.instances = []
    FakeClient.corrupt_call = None
    monkeypatch.setattr(benchmark.geometer, "GeometerClient", FakeClient)
    monkeypatch.setattr(
        benchmark,
        "resident_sets_bytes",
        lambda process_ids: {process_id: 1024 for process_id in process_ids},
    )
    monkeypatch.setattr(benchmark, "windows_working_set_bytes", lambda _process_id: None)


def test_single_worker_counts_first_warmup_and_samples(fake_runtime: None) -> None:
    result = benchmark.benchmark_single_worker(Path("geometer"), object(), b"model", 2, 3)

    client = FakeClient.instances[0]
    assert client.calls == 6
    assert len(result["warmup_samples"]) == 2
    assert len(result["warm_samples"]) == 3
    assert result["sampled_memory"]["max_simultaneous_resident_set_bytes"] == 1024
    assert client.closed is True


def test_balanced_pool_uses_one_serial_thread_per_client(fake_runtime: None) -> None:
    result = benchmark.benchmark_throughput(
        Path("geometer"), object(), b"model", workers=4, minimum_jobs=8, minimum_seconds=0.0
    )

    assert result["jobs"] == 8
    assert result["jobs_per_worker"] == 2
    assert len(result["warmup_samples"]) == 4
    assert len(result["samples"]) == 8
    assert result["throughput_jobs_per_second"] > 0.0
    assert result["sampled_memory"]["max_simultaneous_resident_set_bytes"] == 4096
    assert [client.calls for client in FakeClient.instances] == [3, 3, 3, 3]
    assert all(client.max_active == 1 for client in FakeClient.instances)
    assert all(client.thread_ids for client in FakeClient.instances)
    assert all(client.closed for client in FakeClient.instances)


def test_pool_rejects_digest_drift_in_warmup(fake_runtime: None) -> None:
    FakeClient.corrupt_call = (0, 1)

    with pytest.raises(RuntimeError, match="identity changed"):
        benchmark.benchmark_throughput(
            Path("geometer"), object(), b"model", workers=2, minimum_jobs=2, minimum_seconds=0.0
        )

    assert all(client.closed for client in FakeClient.instances)


def test_throughput_batch_meets_minimum_duration(fake_runtime: None) -> None:
    result = benchmark.benchmark_throughput(
        Path("geometer"), object(), b"model", workers=1, minimum_jobs=1, minimum_seconds=0.01
    )

    assert result["batch_seconds"] >= 0.01
    assert result["jobs"] >= 1
    assert result["calibration_attempts"][-1]["batch_seconds"] >= 0.01


def test_report_rejects_process_stable_but_process_dependent_output() -> None:
    first = {"output_bytes": 7, "output_sha256": "first"}
    second = {"output_bytes": 7, "output_sha256": "second"}
    runs = [
        {
            "single_worker": {"output_identity": first},
            "single_lane_throughput": {"output_identity": second},
            "process_pool": {"output_identity": first},
        }
    ]

    with pytest.raises(RuntimeError, match="report-wide output identity changed"):
        benchmark.report_output_identity(runs)


def test_resident_sampler_rejects_partial_process_observations(
    monkeypatch: pytest.MonkeyPatch,
) -> None:
    monkeypatch.setattr(benchmark, "resident_sets_bytes", lambda _process_ids: {1000: 1024})
    sampler = benchmark.ResidentSetSampler([1000, 1001], interval_seconds=10.0)

    sampler.start()
    with pytest.raises(RuntimeError, match="telemetry was incomplete"):
        sampler.stop()


def test_resident_sampler_rejects_missing_observations(monkeypatch: pytest.MonkeyPatch) -> None:
    monkeypatch.setattr(benchmark, "resident_sets_bytes", lambda _process_ids: {})
    sampler = benchmark.ResidentSetSampler([1000], interval_seconds=10.0)

    sampler.start()
    with pytest.raises(RuntimeError, match="complete=0, partial=0, failed=2"):
        sampler.stop()
