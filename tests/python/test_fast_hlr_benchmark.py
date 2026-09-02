from __future__ import annotations

import importlib.util
from pathlib import Path

import pytest


ROOT = Path(__file__).resolve().parents[2]
SCRIPT = ROOT / "scripts/benchmark_fast_hlr.py"
SPEC = importlib.util.spec_from_file_location("benchmark_fast_hlr", SCRIPT)
if SPEC is None or SPEC.loader is None:
    raise RuntimeError("Could not load scripts/benchmark_fast_hlr.py")
benchmark = importlib.util.module_from_spec(SPEC)
SPEC.loader.exec_module(benchmark)


def test_percentile_uses_nearest_rank() -> None:
    values = [9.0, 1.0, 3.0, 7.0, 5.0]

    assert benchmark.percentile(values, 0.0) == 1.0
    assert benchmark.percentile(values, 0.5) == 5.0
    assert benchmark.percentile(values, 0.95) == 9.0
    assert benchmark.percentile(values, 1.0) == 9.0


def test_summary_reports_reproducible_statistics() -> None:
    summary = benchmark.summarize([1.0, 2.0, 8.0, 9.0])

    assert summary == {
        "count": 4,
        "min": 1.0,
        "mean": 5.0,
        "p50": 2.0,
        "p95": 9.0,
        "max": 9.0,
    }


def test_empty_statistics_are_rejected() -> None:
    with pytest.raises(ValueError, match="at least one"):
        benchmark.summarize([])
    with pytest.raises(ValueError, match="between zero and one"):
        benchmark.percentile([1.0], 1.1)


def test_default_fixture_corpus_exists() -> None:
    workloads = benchmark.load_workloads(benchmark.DEFAULT_MANIFEST, [])

    assert [workload["name"] for workload in workloads] == list(benchmark.DEFAULT_MODELS)
    assert all(workload["step_bytes"] > 0 for workload in workloads)
    assert all(workload["step_path"].is_file() for workload in workloads)
