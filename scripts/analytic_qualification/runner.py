"""Qualification orchestration and deterministic report structure."""

from __future__ import annotations

import hashlib
import time
from pathlib import Path
from statistics import median
from typing import Any

from geometer._ipc_client import ANALYTIC_OPERATION, GeometerIpcClient

from .corpus import QualificationCase, QualificationError, identity_sha256, load_request_packet, request_accounting
from .environment import (
    REFERENCE_INSTALLED_RAM_BYTES,
    REFERENCE_PROCESSOR,
    REFERENCE_SYSTEM,
    PeakRssSampler,
    machine_profile,
    process_rss_measurement_method,
    reference_machine_match,
    toolchain_profile,
)
from .replay import execute_packet
from .telemetry import execute_telemetry, helper_profile


REPORT_SCHEMA = "wn.geometer.analytic_planar_boolean_qualification.a2"
TARGET_WALL_SECONDS = 1.0
TARGET_PEAK_BYTES = 512 * 1024 * 1024
CEILING_WALL_SECONDS = 5.0
CEILING_PEAK_BYTES = 1024 * 1024 * 1024


def _identity(
    loaded: list[tuple[QualificationCase, bytes, dict[str, int]]],
    machine: dict[str, Any],
    toolchain: dict[str, Any],
    warmup_count: int,
    repeat_count: int,
    power_mode: str,
    telemetry_profile: dict[str, Any] | None,
    require_solver_telemetry: bool,
) -> dict[str, Any]:
    cases = [
        {
            "id": case.case_id,
            "classification": case.classification,
            "request_bytes": len(packet),
            "request_sha256": hashlib.sha256(packet).hexdigest(),
            "expected_result_sha256": case.expected_result_sha256,
            "source_provenance": case.source_provenance,
        }
        for case, packet, _ in loaded
    ]
    return {
        "operation": ANALYTIC_OPERATION,
        "transport": "native_executable_ipc_a0",
        "cases": cases,
        "warmup_count": warmup_count,
        "repeat_count": repeat_count,
        "power_mode": power_mode,
        "solver_telemetry_required": require_solver_telemetry,
        "telemetry_helper_sha256": (
            None if telemetry_profile is None else telemetry_profile["profile"]["executable_sha256"]
        ),
        "telemetry_schema": (None if telemetry_profile is None else telemetry_profile["profile"]["identity"]["schema"]),
        "target": {"wall_seconds": TARGET_WALL_SECONDS, "solver_peak_working_memory_bytes": TARGET_PEAK_BYTES},
        "hard_ceiling": {
            "wall_seconds": CEILING_WALL_SECONDS,
            "solver_peak_working_memory_bytes": CEILING_PEAK_BYTES,
        },
        "machine_profile_sha256": machine["sha256"],
        "toolchain_profile_sha256": toolchain["sha256"],
        "process_rss_measurement": process_rss_measurement_method(),
    }


def _run_case(
    client: GeometerIpcClient,
    sampler: PeakRssSampler,
    case: QualificationCase,
    packet: bytes,
    accounting: dict[str, int],
    warmup_count: int,
    repeat_count: int,
    telemetry_helper: Path | None,
) -> dict[str, Any]:
    warmup_hashes = []
    for _ in range(warmup_count):
        warmup_packet, _ = execute_packet(client, packet, CEILING_WALL_SECONDS)
        warmup_hashes.append(hashlib.sha256(warmup_packet).hexdigest())
    runs = []
    for _ in range(repeat_count):
        runs.append(_run_observation(client, sampler, packet, telemetry_helper))
    result_hashes = {*warmup_hashes, *(run["result_sha256"] for run in runs)}
    rss_values = [run["process_peak_rss_bytes"] for run in runs]
    peak_rss = None if any(value is None for value in rss_values) else max(rss_values)
    median_wall_ns = int(median(run["wall_time_ns"] for run in runs))
    telemetry_values = [run.pop("_solver_telemetry") for run in runs if "_solver_telemetry" in run]
    telemetry_deterministic = len({identity_sha256(value) for value in telemetry_values}) <= 1
    telemetry_value = telemetry_values[0] if telemetry_values and telemetry_deterministic else None
    return {
        "id": case.case_id,
        "classification": case.classification,
        "request": accounting,
        "request_sha256": hashlib.sha256(packet).hexdigest(),
        "runs": runs,
        "summary": _case_summary(case, runs, warmup_hashes, result_hashes, median_wall_ns, peak_rss),
        "solver_telemetry": _telemetry_summary(telemetry_values, telemetry_value, telemetry_deterministic),
    }


def _run_observation(
    client: GeometerIpcClient,
    sampler: PeakRssSampler,
    packet: bytes,
    telemetry_helper: Path | None,
) -> dict[str, Any]:
    baseline = sampler.reset()
    started = time.perf_counter_ns()
    result_packet, result_summary = execute_packet(client, packet, CEILING_WALL_SECONDS)
    elapsed_ns = time.perf_counter_ns() - started
    internal = (
        None
        if telemetry_helper is None
        else execute_telemetry(telemetry_helper, packet, result_packet, CEILING_WALL_SECONDS)
    )
    if internal is not None and [job["job_id"] for job in internal["jobs"]] != [
        job["job_id"] for job in result_summary["job_digests"]
    ]:
        raise QualificationError("telemetry helper job identifiers differ from the production result packet")
    observation = {
        "wall_time_ns": elapsed_ns,
        "process_baseline_rss_bytes": baseline,
        "process_peak_rss_bytes": sampler.peak(),
        "result_bytes": len(result_packet),
        "result_sha256": hashlib.sha256(result_packet).hexdigest(),
        "solver_telemetry_sha256": None if internal is None else identity_sha256(internal),
        **result_summary,
    }
    if internal is not None:
        observation["_solver_telemetry"] = internal
    return observation


def _case_summary(
    case: QualificationCase,
    runs: list[dict[str, Any]],
    warmup_hashes: list[str],
    result_hashes: set[str],
    median_wall_ns: int,
    peak_rss: int | None,
) -> dict[str, Any]:
    target_met = median_wall_ns <= int(TARGET_WALL_SECONDS * 1e9) and (
        peak_rss is not None and peak_rss <= TARGET_PEAK_BYTES
    )
    ceiling_met = all(run["wall_time_ns"] <= int(CEILING_WALL_SECONDS * 1e9) for run in runs) and (
        peak_rss is not None and peak_rss <= CEILING_PEAK_BYTES
    )
    return {
        "result_observation_count": len(warmup_hashes) + len(runs),
        "median_wall_time_ns": median_wall_ns,
        "maximum_process_peak_rss_bytes": peak_rss,
        "process_envelope_target_observation_met": target_met,
        "process_envelope_hard_ceiling_met": ceiling_met,
        "deterministic_result": len(result_hashes) == 1,
        "expected_result_match": case.expected_result_sha256 is None or result_hashes == {case.expected_result_sha256},
        "no_job_failures": all(run["failed_job_count"] == 0 for run in runs),
    }


def _telemetry_summary(
    values: list[dict[str, Any]], value: dict[str, Any] | None, deterministic: bool
) -> dict[str, Any]:
    target_observation_met = bool(
        value is not None
        and value["batch"]["peak_working_memory_bytes"] <= TARGET_PEAK_BYTES
        and value["batch"]["fallback_count"] == 0
        and value["batch"]["failures"] == 0
    )
    hard_ceiling_met = bool(
        value is not None
        and value["batch"]["peak_working_memory_bytes"] <= CEILING_PEAK_BYTES
        and value["batch"]["fallback_count"] == 0
        and value["batch"]["failures"] == 0
    )
    return {
        "observation_count": len(values),
        "available": value is not None,
        "production_result_packet_match": value is not None,
        "deterministic": deterministic,
        "target_observation_met": target_observation_met,
        "hard_ceiling_met": hard_ceiling_met,
        "counters": value,
        "reason": None if value is not None else "native qualification helper was not selected",
    }


def _qualification(
    cases: list[QualificationCase],
    reports: list[dict[str, Any]],
    require_target: bool,
    require_solver_telemetry: bool = False,
    reference_machine: bool = False,
    build_provenance_attested: bool = False,
) -> dict[str, Any]:
    all_ceiling = all(case["summary"]["process_envelope_hard_ceiling_met"] for case in reports)
    all_correct = all(_correct_result(case) for case in reports)
    all_process_targets = all(case["summary"]["process_envelope_target_observation_met"] for case in reports)
    has_real_board = any(case.classification == "external_real_board" for case in cases)
    real_board_expected_results_governed = all(
        case.classification != "external_real_board" or case.expected_result_sha256 is not None for case in cases
    )
    all_solver_telemetry = all(_valid_solver_telemetry(case) for case in reports)
    repeated_promotion_evidence = all(
        case["summary"]["result_observation_count"] >= 2 and case["solver_telemetry"]["observation_count"] >= 2
        for case in reports
    )
    real_board_gate = all(
        (
            has_real_board,
            all_correct,
            all_ceiling,
            all_solver_telemetry,
            build_provenance_attested,
            real_board_expected_results_governed,
            repeated_promotion_evidence,
        )
    )
    status = all(
        (
            all_ceiling,
            all_correct,
            all_process_targets or not require_target,
            all_solver_telemetry or not require_solver_telemetry,
        )
    )
    return {
        "status": "pass" if status else "fail",
        "process_envelope_hard_ceiling_gate": "pass" if all_ceiling else "fail",
        "correctness_gate": "pass" if all_correct else "fail",
        "process_envelope_target_observation": "met" if all_process_targets else "not_met",
        "process_envelope_target_required": require_target,
        "solver_telemetry_gate": "pass" if all_solver_telemetry else "not_run_or_failed",
        "solver_telemetry_required": require_solver_telemetry,
        "reference_machine_observation": "match" if reference_machine else "different_or_unavailable",
        "build_provenance_gate": "pass" if build_provenance_attested else "fail",
        "real_board_expected_result_authority_gate": ("pass" if real_board_expected_results_governed else "fail"),
        "promotion_repeat_evidence_gate": "pass" if repeated_promotion_evidence else "fail",
        "real_board_evidence_present": has_real_board,
        "real_board_promotion_gate": "pass" if real_board_gate else "incomplete",
        "real_board_promotion_gate_reason": (
            "one or more expected-result authority, repeat-evidence, correctness, hard process-envelope, solver, or clean-build provenance gates failed"
            if has_real_board and not real_board_gate
            else "portable real-board correctness, hard process-envelope, solver, and clean-build gates passed"
            if real_board_gate
            else "the corpus contains no externally supplied real-board packet"
        ),
    }


def _correct_result(case: dict[str, Any]) -> bool:
    summary = case["summary"]
    return all((summary["deterministic_result"], summary["expected_result_match"], summary["no_job_failures"]))


def _valid_solver_telemetry(case: dict[str, Any]) -> bool:
    telemetry = case["solver_telemetry"]
    return all(
        (
            telemetry["available"],
            telemetry["production_result_packet_match"],
            telemetry["deterministic"],
            telemetry["hard_ceiling_met"],
        )
    )


def qualify(
    cases: list[QualificationCase],
    executable: Path,
    *,
    warmup_count: int,
    repeat_count: int,
    power_mode: str,
    require_target: bool,
    telemetry_helper: Path | None = None,
    require_solver_telemetry: bool = False,
) -> dict[str, Any]:
    if warmup_count < 0 or repeat_count < 1:
        raise QualificationError("warmup count must be nonnegative and repeat count must be positive")
    machine = machine_profile()
    toolchain = toolchain_profile(executable)
    telemetry_profile = None if telemetry_helper is None else helper_profile(telemetry_helper)
    if require_solver_telemetry and telemetry_profile is None:
        raise QualificationError("solver telemetry is required but no helper was selected")
    loaded = [(case, packet := load_request_packet(case.request_path), request_accounting(packet)) for case in cases]
    identity = _identity(
        loaded,
        machine,
        toolchain,
        warmup_count,
        repeat_count,
        power_mode,
        telemetry_profile,
        require_solver_telemetry,
    )
    report: dict[str, Any] = {
        "schema": REPORT_SCHEMA,
        "identity": identity,
        "identity_sha256": identity_sha256(identity),
        "environment": {
            "machine": machine,
            "toolchain": toolchain,
            "telemetry_helper": telemetry_profile,
            "reference_machine": {
                "system": REFERENCE_SYSTEM,
                "processor": REFERENCE_PROCESSOR,
                "installed_ram_bytes": REFERENCE_INSTALLED_RAM_BYTES,
                "match": reference_machine_match(machine),
            },
        },
        "cases": [],
    }
    with GeometerIpcClient(executable, client_name="analytic-qualification", client_version="a0") as client:
        sampler = PeakRssSampler(client.process_id)
        sampler.start()
        try:
            report["cases"] = [
                _run_case(
                    client,
                    sampler,
                    case,
                    packet,
                    accounting,
                    warmup_count,
                    repeat_count,
                    telemetry_helper,
                )
                for case, packet, accounting in loaded
            ]
        finally:
            sampler.close()
    report["qualification"] = _qualification(
        cases,
        report["cases"],
        require_target,
        require_solver_telemetry,
        reference_machine_match(machine),
        bool(toolchain["profile"]["build_provenance_attested"]),
    )
    return report
