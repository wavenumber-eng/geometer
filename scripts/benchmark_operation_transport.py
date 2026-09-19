"""Benchmark persistent executable IPC before and after transport changes."""

from __future__ import annotations

import argparse
import concurrent.futures
import ctypes
import hashlib
import json
import math
import os
import platform
import statistics
import subprocess
import threading
import time
from pathlib import Path
from typing import Any

import geometer


ROOT = Path(__file__).resolve().parents[1]
DEFAULT_FIXTURE = ROOT / "tests/fixtures/step/embedded_models/SOT-23.STEP"
DEFAULT_OUTPUT = ROOT / "out/operation-transport-performance/sibling-process-baseline.json"


def percentile(values: list[float], quantile: float) -> float:
    """Return the nearest-rank percentile used by the qualification report."""
    if not values or not 0.0 <= quantile <= 1.0:
        raise ValueError("percentile requires values and a quantile in [0, 1]")
    ordered = sorted(values)
    index = max(0, math.ceil(quantile * len(ordered)) - 1)
    return ordered[index]


def summary_ms(values: list[float]) -> dict[str, float]:
    """Summarize millisecond samples without hiding their population."""
    return {
        "minimum": min(values),
        "median": statistics.median(values),
        "p95": percentile(values, 0.95),
        "maximum": max(values),
    }


def windows_working_set_bytes(process_id: int) -> tuple[int, int] | None:
    """Read current and OS-recorded peak working sets without a dependency."""
    if os.name != "nt":
        return None

    class ProcessMemoryCounters(ctypes.Structure):
        _fields_ = [
            ("cb", ctypes.c_ulong),
            ("page_fault_count", ctypes.c_ulong),
            ("peak_working_set_size", ctypes.c_size_t),
            ("working_set_size", ctypes.c_size_t),
            ("quota_peak_paged_pool_usage", ctypes.c_size_t),
            ("quota_paged_pool_usage", ctypes.c_size_t),
            ("quota_peak_non_paged_pool_usage", ctypes.c_size_t),
            ("quota_non_paged_pool_usage", ctypes.c_size_t),
            ("pagefile_usage", ctypes.c_size_t),
            ("peak_pagefile_usage", ctypes.c_size_t),
        ]

    kernel32 = ctypes.WinDLL("kernel32", use_last_error=True)
    psapi = ctypes.WinDLL("psapi", use_last_error=True)
    open_process = kernel32.OpenProcess
    open_process.argtypes = (ctypes.c_ulong, ctypes.c_int, ctypes.c_ulong)
    open_process.restype = ctypes.c_void_p
    close_handle = kernel32.CloseHandle
    close_handle.argtypes = (ctypes.c_void_p,)
    get_memory = psapi.GetProcessMemoryInfo
    get_memory.argtypes = (
        ctypes.c_void_p,
        ctypes.POINTER(ProcessMemoryCounters),
        ctypes.c_ulong,
    )
    get_memory.restype = ctypes.c_int

    process_query_information = 0x0400
    process_vm_read = 0x0010
    handle = open_process(process_query_information | process_vm_read, 0, process_id)
    if not handle:
        return None
    try:
        counters = ProcessMemoryCounters()
        counters.cb = ctypes.sizeof(counters)
        if not get_memory(handle, ctypes.byref(counters), counters.cb):
            return None
        return int(counters.working_set_size), int(counters.peak_working_set_size)
    finally:
        close_handle(handle)


def resident_sets_bytes(process_ids: list[int]) -> dict[int, int]:
    """Read one current-RSS observation for all live child processes."""
    if os.name == "nt":
        result: dict[int, int] = {}
        for process_id in process_ids:
            observation = windows_working_set_bytes(process_id)
            if observation is not None:
                result[process_id] = observation[0]
        return result

    completed = subprocess.run(
        ["ps", "-o", "pid=,rss=", "-p", ",".join(str(value) for value in process_ids)],
        capture_output=True,
        check=False,
        text=True,
    )
    if completed.returncode != 0:
        return {}
    result = {}
    for line in completed.stdout.splitlines():
        fields = line.split()
        if len(fields) == 2:
            result[int(fields[0])] = int(fields[1]) * 1024
    return result


class ResidentSetSampler:
    """Sample per-process RSS and the simultaneous pool total."""

    def __init__(self, process_ids: list[int], interval_seconds: float = 0.05) -> None:
        self._process_ids = process_ids
        self._interval_seconds = interval_seconds
        self._stop = threading.Event()
        self._thread: threading.Thread | None = None
        self._max_by_process = {process_id: 0 for process_id in process_ids}
        self._max_simultaneous = 0
        self._complete_samples = 0
        self._partial_samples = 0
        self._failed_samples = 0

    def _sample(self) -> None:
        observations = resident_sets_bytes(self._process_ids)
        if not observations:
            self._failed_samples += 1
            return
        expected = set(self._process_ids)
        if set(observations) != expected or any(value <= 0 for value in observations.values()):
            self._partial_samples += 1
            return
        self._complete_samples += 1
        self._max_simultaneous = max(self._max_simultaneous, sum(observations.values()))
        for process_id, value in observations.items():
            self._max_by_process[process_id] = max(self._max_by_process[process_id], value)

    def _run(self) -> None:
        while not self._stop.wait(self._interval_seconds):
            self._sample()

    def start(self) -> None:
        self._sample()
        self._thread = threading.Thread(target=self._run, name="geometer-rss-sampler", daemon=True)
        self._thread.start()

    def stop(self) -> dict[str, Any]:
        self._stop.set()
        if self._thread is not None:
            self._thread.join()
        self._sample()
        if self._complete_samples < 2 or any(value <= 0 for value in self._max_by_process.values()):
            raise RuntimeError(
                "resident-set telemetry was incomplete: "
                f"complete={self._complete_samples}, partial={self._partial_samples}, "
                f"failed={self._failed_samples}"
            )
        return {
            "sample_interval_seconds": self._interval_seconds,
            "complete_sample_count": self._complete_samples,
            "partial_sample_count": self._partial_samples,
            "failed_sample_count": self._failed_samples,
            "per_process_peak_resident_set_bytes": [
                self._max_by_process[process_id] for process_id in self._process_ids
            ],
            "max_simultaneous_resident_set_bytes": self._max_simultaneous or None,
        }


def request() -> geometer.ModelIllustrationGeometryRequestB0:
    """Create the representative STEP-to-drawing operation request."""
    return geometer.ModelIllustrationGeometryRequestB0(
        schema="geometry.model_illustration_geometry.request.b0",
        source=geometer.ModelAttachmentIllustrationSourceA0(kind="model", attachment="model"),
        view=geometer.MeshIllustrationView(direction=(0.4, 0.7, 1.0), up=(0.0, 1.0, 0.0)),
        style=geometer.MeshIllustrationStyleA0(show_hlr_detail=True),
    )


def execute_once(
    client: geometer.GeometerClient,
    operation_request: geometer.ModelIllustrationGeometryRequestB0,
    model: bytes,
) -> dict[str, Any]:
    """Execute and time one governed operation."""
    started = time.perf_counter_ns()
    result = client.model_illustration_geometry(operation_request, model, timeout=180.0)
    elapsed_ms = (time.perf_counter_ns() - started) / 1_000_000.0
    return {
        "elapsed_ms": elapsed_ms,
        "output_bytes": result.metadata.geometry.byte_length,
        "output_sha256": result.metadata.geometry.sha256,
    }


def require_output_identity(samples: list[dict[str, Any]], context: str) -> dict[str, Any]:
    """Fail closed unless byte length and digest agree for every output."""
    identities = {(sample["output_bytes"], sample["output_sha256"]) for sample in samples}
    if len(identities) != 1:
        raise RuntimeError(f"{context} output identity changed between operations")
    output_bytes, output_sha256 = identities.pop()
    return {"output_bytes": output_bytes, "output_sha256": output_sha256}


def benchmark_single_worker(
    executable: Path,
    operation_request: geometer.ModelIllustrationGeometryRequestB0,
    model: bytes,
    warmup: int,
    repeat: int,
) -> dict[str, Any]:
    """Measure construction, first call, and warm persistent calls."""
    started = time.perf_counter_ns()
    client = geometer.GeometerClient(executable, client_name="operation-transport-benchmark")
    startup_ms = (time.perf_counter_ns() - started) / 1_000_000.0
    sampler = ResidentSetSampler([client.process_id])
    sampler.start()
    try:
        first = execute_once(client, operation_request, model)
        warmups = [execute_once(client, operation_request, model) for _ in range(warmup)]
        samples = [execute_once(client, operation_request, model) for _ in range(repeat)]
        output_identity = require_output_identity([first, *warmups, *samples], "single-worker")
        windows_memory = windows_working_set_bytes(client.process_id)
        return {
            "startup_ms": startup_ms,
            "first_operation": first,
            "warmup_samples": warmups,
            "warm_samples": samples,
            "warm_summary_ms": summary_ms([sample["elapsed_ms"] for sample in samples]),
            "output_identity": output_identity,
            "sampled_memory": sampler.stop(),
            "windows_current_working_set_bytes": windows_memory[0] if windows_memory else None,
            "windows_os_peak_working_set_bytes": windows_memory[1] if windows_memory else None,
        }
    finally:
        if sampler._thread is not None and sampler._thread.is_alive():
            sampler.stop()
        client.close()


def execute_lane(
    client: geometer.GeometerClient,
    operation_request: geometer.ModelIllustrationGeometryRequestB0,
    model: bytes,
    jobs: int,
    barrier: threading.Barrier,
) -> list[dict[str, Any]]:
    """Run one serial lane; exactly one host thread owns each client."""
    barrier.wait()
    return [execute_once(client, operation_request, model) for _ in range(jobs)]


def execute_balanced_batch(
    executor: concurrent.futures.ThreadPoolExecutor,
    clients: list[geometer.GeometerClient],
    operation_request: geometer.ModelIllustrationGeometryRequestB0,
    model: bytes,
    jobs_per_worker: int,
) -> tuple[list[dict[str, Any]], float]:
    """Start balanced serial client loops at one synchronization barrier."""
    barrier = threading.Barrier(len(clients) + 1)
    futures = [
        executor.submit(execute_lane, client, operation_request, model, jobs_per_worker, barrier) for client in clients
    ]
    barrier.wait()
    started = time.perf_counter_ns()
    lane_samples = [future.result() for future in futures]
    elapsed_seconds = (time.perf_counter_ns() - started) / 1_000_000_000.0
    return [sample for lane in lane_samples for sample in lane], elapsed_seconds


def benchmark_throughput(
    executable: Path,
    operation_request: geometer.ModelIllustrationGeometryRequestB0,
    model: bytes,
    workers: int,
    minimum_jobs: int,
    minimum_seconds: float,
) -> dict[str, Any]:
    """Measure sustained throughput with one serial loop per child process."""
    started = time.perf_counter_ns()
    clients = [
        geometer.GeometerClient(executable, client_name=f"operation-transport-pool-{index}") for index in range(workers)
    ]
    startup_ms = (time.perf_counter_ns() - started) / 1_000_000.0
    sampler = ResidentSetSampler([client.process_id for client in clients])
    sampler.start()
    try:
        with concurrent.futures.ThreadPoolExecutor(max_workers=workers) as executor:
            warmups, _ = execute_balanced_batch(executor, clients, operation_request, model, 1)
            fastest_warmup_ms = min(sample["elapsed_ms"] for sample in warmups)
            jobs_per_worker = max(
                math.ceil(minimum_jobs / workers),
                math.ceil((minimum_seconds * 1000.0) / fastest_warmup_ms),
            )
            attempts = []
            all_samples = [*warmups]
            while True:
                samples, batch_seconds = execute_balanced_batch(
                    executor, clients, operation_request, model, jobs_per_worker
                )
                attempts.append(
                    {
                        "jobs_per_worker": jobs_per_worker,
                        "jobs": jobs_per_worker * workers,
                        "batch_seconds": batch_seconds,
                    }
                )
                all_samples.extend(samples)
                if batch_seconds >= minimum_seconds or minimum_seconds == 0.0:
                    break
                jobs_per_worker = math.ceil(jobs_per_worker * minimum_seconds / max(batch_seconds, 0.001) * 1.10)
        output_identity = require_output_identity(all_samples, "process-pool")
        windows_memory = [windows_working_set_bytes(client.process_id) for client in clients]
        final_jobs = jobs_per_worker * workers
        return {
            "workers": workers,
            "jobs": final_jobs,
            "jobs_per_worker": jobs_per_worker,
            "startup_ms": startup_ms,
            "batch_seconds": batch_seconds,
            "minimum_batch_seconds": minimum_seconds,
            "throughput_jobs_per_second": final_jobs / batch_seconds,
            "calibration_attempts": attempts,
            "executed_jobs_including_calibration": len(all_samples),
            "warmup_samples": warmups,
            "samples": samples,
            "output_identity": output_identity,
            "latency_summary_ms": summary_ms([sample["elapsed_ms"] for sample in samples]),
            "sampled_memory": sampler.stop(),
            "windows_os_peak_working_set_bytes": [
                observation[1] if observation else None for observation in windows_memory
            ],
        }
    finally:
        if sampler._thread is not None and sampler._thread.is_alive():
            sampler.stop()
        for client in clients:
            client.close()


def git_value(*args: str) -> str:
    """Return a diagnostic Git value without making the benchmark depend on Git."""
    completed = subprocess.run(
        ["git", *args],
        cwd=ROOT,
        capture_output=True,
        check=False,
        text=True,
    )
    return completed.stdout.strip() if completed.returncode == 0 else "unavailable"


def workflow_url() -> str | None:
    """Return the current GitHub Actions run URL when available."""
    server = os.environ.get("GITHUB_SERVER_URL")
    repository = os.environ.get("GITHUB_REPOSITORY")
    run_id = os.environ.get("GITHUB_RUN_ID")
    if server and repository and run_id:
        return f"{server}/{repository}/actions/runs/{run_id}"
    return None


def executable_attestation(executable: Path) -> dict[str, Any] | None:
    """Bind a colocated native build attestation when the distribution has one."""
    attestation = executable.with_name("geometer.build-attestation.json")
    if not attestation.is_file():
        return None
    data = attestation.read_bytes()
    return {
        "path": str(attestation),
        "bytes": len(data),
        "sha256": hashlib.sha256(data).hexdigest(),
        "document": json.loads(data),
    }


def report_output_identity(runs: list[dict[str, Any]]) -> dict[str, Any]:
    """Require one byte identity across all runs, clients, and modes."""
    identities = []
    for run in runs:
        identities.extend(
            [
                run["single_worker"]["output_identity"],
                run["single_lane_throughput"]["output_identity"],
                run["process_pool"]["output_identity"],
            ]
        )
    return require_output_identity(identities, "report-wide")


def main() -> None:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--executable", type=Path, required=True)
    parser.add_argument("--fixture", type=Path, default=DEFAULT_FIXTURE)
    parser.add_argument("--runs", type=int, default=3)
    parser.add_argument("--warmup", type=int, default=1)
    parser.add_argument("--repeat", type=int, default=7)
    parser.add_argument("--workers", type=int, default=4)
    parser.add_argument("--pool-jobs", type=int, default=12)
    parser.add_argument("--minimum-batch-seconds", type=float, default=5.0)
    parser.add_argument("--source-revision")
    parser.add_argument("--workflow-url")
    parser.add_argument("--output", type=Path, default=DEFAULT_OUTPUT)
    args = parser.parse_args()
    if (
        args.runs < 1
        or args.warmup < 0
        or args.repeat < 1
        or args.workers < 1
        or args.pool_jobs < args.workers
        or args.pool_jobs % args.workers != 0
        or args.minimum_batch_seconds < 0.0
    ):
        parser.error(
            "runs/repeat/workers must be positive, pool jobs a worker multiple, and minimum batch seconds nonnegative"
        )
    executable = args.executable.resolve(strict=True)
    fixture = args.fixture.resolve(strict=True)
    model = fixture.read_bytes()
    operation_request = request()
    runs = []
    for index in range(args.runs):
        print(f"benchmark run {index + 1}/{args.runs}", flush=True)
        runs.append(
            {
                "single_worker": benchmark_single_worker(
                    executable, operation_request, model, args.warmup, args.repeat
                ),
                "single_lane_throughput": benchmark_throughput(
                    executable,
                    operation_request,
                    model,
                    1,
                    max(1, args.pool_jobs // args.workers),
                    args.minimum_batch_seconds,
                ),
                "process_pool": benchmark_throughput(
                    executable,
                    operation_request,
                    model,
                    args.workers,
                    args.pool_jobs,
                    args.minimum_batch_seconds,
                ),
            }
        )

    report = {
        "schema": "geometer.operation_transport_benchmark.a0",
        "captured_at": time.strftime("%Y-%m-%dT%H:%M:%SZ", time.gmtime()),
        "platform": platform.platform(),
        "python": platform.python_version(),
        "git_head": git_value("rev-parse", "HEAD"),
        "source_revision": args.source_revision or git_value("rev-parse", "HEAD"),
        "git_status_after_build": git_value("status", "--short"),
        "workflow_url": args.workflow_url or workflow_url(),
        "executable": str(executable),
        "executable_bytes": executable.stat().st_size,
        "executable_sha256": hashlib.sha256(executable.read_bytes()).hexdigest(),
        "executable_attestation": executable_attestation(executable),
        "fixture": str(fixture),
        "input_bytes": len(model),
        "input_sha256": hashlib.sha256(model).hexdigest(),
        "configuration": {
            "runs": args.runs,
            "warmup": args.warmup,
            "repeat": args.repeat,
            "workers": args.workers,
            "pool_jobs": args.pool_jobs,
            "minimum_batch_seconds": args.minimum_batch_seconds,
        },
        "runs": runs,
        "aggregate": {
            "output_identity": report_output_identity(runs),
            "startup_ms": summary_ms([run["single_worker"]["startup_ms"] for run in runs]),
            "first_operation_ms": summary_ms([run["single_worker"]["first_operation"]["elapsed_ms"] for run in runs]),
            "warm_operation_ms": summary_ms(
                [sample["elapsed_ms"] for run in runs for sample in run["single_worker"]["warm_samples"]]
            ),
            "pool_throughput_jobs_per_second": summary_ms(
                [run["process_pool"]["throughput_jobs_per_second"] for run in runs]
            ),
            "single_lane_throughput_jobs_per_second": summary_ms(
                [run["single_lane_throughput"]["throughput_jobs_per_second"] for run in runs]
            ),
            "four_over_one_throughput_scaling": summary_ms(
                [
                    run["process_pool"]["throughput_jobs_per_second"]
                    / run["single_lane_throughput"]["throughput_jobs_per_second"]
                    for run in runs
                ]
            ),
        },
    }
    args.output.parent.mkdir(parents=True, exist_ok=True)
    args.output.write_text(json.dumps(report, indent=2) + "\n", encoding="utf-8")
    print(json.dumps(report["aggregate"], indent=2))


if __name__ == "__main__":
    main()
