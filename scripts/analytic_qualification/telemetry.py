"""Fail-closed discovery and execution of the native solver telemetry helper."""

from __future__ import annotations

import json
import os
import subprocess
import tempfile
from pathlib import Path
from typing import Any

from .corpus import QualificationError, file_sha256, identity_sha256


ROOT = Path(__file__).resolve().parents[2]
TELEMETRY_SCHEMA = "wn.geometer.analytic_solver_telemetry.a0"
TELEMETRY_HELPER_ENV = "GEOMETER_ANALYTIC_TELEMETRY_HELPER"
EXPECTED_IMPLEMENTATION = "decode_analytic_request_packet+build_analytic_filtered_batch"


def _candidate_paths() -> tuple[Path, ...]:
    suffix = ".exe" if os.name == "nt" else ""
    name = f"geometer_analytic_solver_telemetry_helper{suffix}"
    return (
        ROOT / "build" / "tests" / "cpp" / name,
        *(path / "tests" / "cpp" / name for path in sorted(ROOT.glob("build-native-*"))),
    )


def resolve_telemetry_helper(explicit: Path | None, *, required: bool) -> Path | None:
    """Resolve only an explicit, environment, or canonical current-workspace helper."""

    environment_value = os.environ.get(TELEMETRY_HELPER_ENV)
    if explicit is not None:
        candidate = explicit.resolve()
        authority = "--telemetry-helper"
    elif environment_value:
        candidate = Path(environment_value).expanduser().resolve()
        authority = TELEMETRY_HELPER_ENV
    else:
        candidate = next((path.resolve() for path in _candidate_paths() if path.is_file()), None)
        authority = "canonical build path"
    if candidate is None:
        if required:
            raise QualificationError(
                "solver telemetry is required but the native helper was not found; build target "
                "geometer_analytic_solver_telemetry_helper or set "
                f"{TELEMETRY_HELPER_ENV}"
            )
        return None
    if not candidate.is_file():
        raise QualificationError(f"telemetry helper selected by {authority} does not exist: {candidate}")
    return candidate


def _load_json(stdout: str, context: str) -> dict[str, Any]:
    try:
        value = json.loads(stdout)
    except json.JSONDecodeError as error:
        raise QualificationError(f"telemetry helper returned invalid {context} JSON: {error}") from error
    if not isinstance(value, dict):
        raise QualificationError(f"telemetry helper returned non-object {context} JSON")
    return value


def helper_profile(helper: Path) -> dict[str, Any]:
    build_root = helper.parent.parent.parent
    library_candidates = (
        build_root / "src" / "cpp" / "lib" / "geometer.lib",
        build_root / "src" / "cpp" / "lib" / "libgeometer.a",
    )
    library = next((path for path in library_candidates if path.is_file()), None)
    if library is None:
        raise QualificationError(
            "telemetry helper has no adjacent geometer_lib build artifact; rebuild the helper target"
        )
    library_sources = [
        ROOT / "CMakeLists.txt",
        ROOT / "src" / "cpp" / "lib" / "CMakeLists.txt",
        *(ROOT / "src" / "cpp" / "lib").rglob("*.cpp"),
        *(ROOT / "src" / "cpp" / "lib").rglob("*.h"),
    ]
    helper_sources = [
        ROOT / "tests" / "cpp" / "CMakeLists.txt",
        ROOT / "tests" / "cpp" / "analytic_solver_telemetry_helper.cpp",
    ]
    stale_library_sources = [
        path
        for path in library_sources
        if path.is_file() and path.stat().st_mtime_ns > library.stat().st_mtime_ns
    ]
    stale_helper_sources = [
        path
        for path in helper_sources
        if path.is_file() and path.stat().st_mtime_ns > helper.stat().st_mtime_ns
    ]
    if (
        stale_library_sources
        or stale_helper_sources
        or library.stat().st_mtime_ns > helper.stat().st_mtime_ns
    ):
        detail = (
            stale_library_sources[0]
            if stale_library_sources
            else stale_helper_sources[0]
            if stale_helper_sources
            else library
        )
        raise QualificationError(
            f"telemetry helper is stale relative to {detail}; rebuild target "
            "geometer_analytic_solver_telemetry_helper"
        )
    try:
        completed = subprocess.run(
            [str(helper), "--identity"],
            check=False,
            capture_output=True,
            text=True,
            timeout=10,
        )
    except (OSError, subprocess.SubprocessError) as error:
        raise QualificationError(f"could not execute telemetry helper identity: {error}") from error
    if completed.returncode != 0:
        raise QualificationError(
            f"telemetry helper identity failed with exit {completed.returncode}: {completed.stderr.strip()}"
        )
    identity = _load_json(completed.stdout, "identity")
    expected = {
        "implementation": EXPECTED_IMPLEMENTATION,
        "request_magic": "GMABRQ01",
        "schema": TELEMETRY_SCHEMA,
    }
    if identity != expected:
        raise QualificationError("telemetry helper identity is stale or incompatible")
    profile = {
        "executable_sha256": file_sha256(helper),
        "identity": identity,
        "selection_path": str(helper),
    }
    return {"profile": profile, "sha256": identity_sha256(profile)}


def _counter(value: Any, path: str) -> int:
    if not isinstance(value, int) or isinstance(value, bool) or value < 0:
        raise QualificationError(f"telemetry helper field {path} must be a nonnegative integer")
    return value


def validate_telemetry_document(value: dict[str, Any]) -> dict[str, Any]:
    if value.get("schema") != TELEMETRY_SCHEMA or value.get("status") != "ok":
        raise QualificationError("telemetry helper result is stale, incompatible, or unsuccessful")
    if set(value) != {"schema", "status", "batch", "jobs"}:
        raise QualificationError("telemetry helper result has an unexpected top-level shape")
    fields = {
        "candidate_pairs",
        "capsule_coalescences",
        "emitted_bytes",
        "failures",
        "fallback_count",
        "maximum_capsule_adjustment_nm",
        "peak_working_memory_bytes",
        "work_units",
    }
    batch = value["batch"]
    jobs = value["jobs"]
    if not isinstance(batch, dict) or set(batch) != fields or not isinstance(jobs, list):
        raise QualificationError("telemetry helper result has an unexpected counter shape")
    normalized_batch = {field: _counter(batch[field], f"batch.{field}") for field in sorted(fields)}
    normalized_jobs = []
    seen_ids: set[int] = set()
    for index, job in enumerate(jobs):
        if not isinstance(job, dict) or set(job) != fields | {"job_id"}:
            raise QualificationError(f"telemetry helper jobs[{index}] has an unexpected shape")
        job_id = _counter(job["job_id"], f"jobs[{index}].job_id")
        if job_id in seen_ids:
            raise QualificationError("telemetry helper returned duplicate job identifiers")
        seen_ids.add(job_id)
        normalized_jobs.append(
            {
                "job_id": job_id,
                **{field: _counter(job[field], f"jobs[{index}].{field}") for field in sorted(fields)},
            }
        )
    if normalized_batch["failures"] != sum(job["failures"] for job in normalized_jobs):
        raise QualificationError("telemetry helper batch/job failure counters disagree")
    if normalized_batch["fallback_count"] < sum(
        job["fallback_count"] for job in normalized_jobs
    ):
        raise QualificationError("telemetry helper batch fallback count is below its job total")
    if normalized_batch["candidate_pairs"] < sum(job["candidate_pairs"] for job in normalized_jobs):
        raise QualificationError("telemetry helper batch candidate count is below its job total")
    if normalized_batch["capsule_coalescences"] != sum(
        job["capsule_coalescences"] for job in normalized_jobs
    ):
        raise QualificationError("telemetry helper batch/job capsule coalescence counts disagree")
    if normalized_batch["maximum_capsule_adjustment_nm"] != max(
        (job["maximum_capsule_adjustment_nm"] for job in normalized_jobs), default=0
    ):
        raise QualificationError("telemetry helper batch/job capsule adjustment maxima disagree")
    if normalized_jobs and normalized_batch["peak_working_memory_bytes"] < max(
        job["peak_working_memory_bytes"] for job in normalized_jobs
    ):
        raise QualificationError("telemetry helper batch memory peak is below a job peak")
    return {"batch": normalized_batch, "jobs": normalized_jobs}


def execute_telemetry(helper: Path, packet: bytes, production_result: bytes, timeout: float) -> dict[str, Any]:
    with tempfile.TemporaryDirectory(prefix="geometer-analytic-telemetry-") as temporary:
        directory = Path(temporary)
        request_path = directory / "request.bin"
        result_path = directory / "result.bin"
        request_path.write_bytes(packet)
        try:
            completed = subprocess.run(
                [str(helper), str(request_path), str(result_path)],
                check=False,
                capture_output=True,
                text=True,
                timeout=timeout,
            )
        except (OSError, subprocess.SubprocessError) as error:
            raise QualificationError(f"telemetry helper execution failed: {error}") from error
        if completed.returncode != 0:
            raise QualificationError(
                f"telemetry helper failed with exit {completed.returncode}: "
                f"{completed.stdout.strip()} {completed.stderr.strip()}"
            )
        if not result_path.is_file():
            raise QualificationError("telemetry helper did not emit a result packet")
        helper_result = result_path.read_bytes()
        if helper_result != production_result:
            raise QualificationError(
                "telemetry helper result bytes differ from production executable IPC bytes; "
                "internal counters are rejected"
            )
        telemetry = validate_telemetry_document(_load_json(completed.stdout, "result"))
        if telemetry["batch"]["emitted_bytes"] != len(production_result):
            raise QualificationError(
                "telemetry helper emitted-byte counter differs from the matched result packet size"
            )
        return telemetry
