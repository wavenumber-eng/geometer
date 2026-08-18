from __future__ import annotations

import hashlib
import json
import os
import subprocess
import sys
import time
from pathlib import Path

import pytest

from geometer._paths import executable_path


ROOT = Path(__file__).resolve().parents[2]
sys.path.insert(0, str(ROOT / "scripts"))

from analytic_qualification import corpus, environment, runner, telemetry  # noqa: E402
from qualify_analytic_planar_boolean import _parser  # noqa: E402


REAL_BOARD_VECTOR = ROOT / "tests/contracts/vectors/analytic/real-board/rt_super_c1_pwr4"


def test_qualification_cli_exposes_promotion_attestation_gate() -> None:
    args = _parser().parse_args(["--require-promotion-attested"])
    assert args.require_promotion_attested is True


def test_governed_default_case_is_explicitly_synthetic_and_canonically_accounted() -> None:
    [case] = corpus.load_corpus(None)
    assert case.case_id == "cross_transport_primitive_family_self_query"
    assert case.classification == "synthetic"
    packet = corpus.load_hex_packet(case.request_path)
    assert corpus.request_accounting(packet) == {
        "job_count": 1,
        "stage_count": 2,
        "operand_count": 6,
        "planar_region_count": 0,
        "ring_count": 1,
        "input_segment_count": 2,
        "disk_count": 2,
        "annulus_count": 1,
        "capsule_count": 2,
        "swept_path_count": 1,
        "relationship_query_count": 1,
    }


def test_raw_gmabrq01_request_is_exact_and_canonically_accounted(
    tmp_path: Path,
) -> None:
    expected = corpus.load_hex_packet(corpus.DEFAULT_REQUEST)
    request = tmp_path / "request.gmabrq01"
    request.write_bytes(expected)

    assert corpus.load_raw_packet(request) == expected
    assert corpus.load_request_packet(request) == expected
    assert corpus.request_accounting(expected)["job_count"] == 1


@pytest.mark.parametrize(
    ("name", "payload", "message"),
    [
        ("bad-magic.gmabrq01", b"BADMAGIC", "does not begin with GMABRQ01"),
        ("truncated.gmabrq01", b"GMABRQ01", "complete GMABRQ01 header"),
        (
            "trailing.gmabrq01",
            corpus.load_hex_packet(corpus.DEFAULT_REQUEST) + b"\0",
            "header is not canonical A0",
        ),
        (
            "hex-as-raw.gmabrq01",
            corpus.DEFAULT_REQUEST.read_bytes(),
            "does not begin with GMABRQ01",
        ),
    ],
)
def test_raw_gmabrq01_request_rejects_malformed_bytes(
    tmp_path: Path,
    name: str,
    payload: bytes,
    message: str,
) -> None:
    request = tmp_path / name
    request.write_bytes(payload)
    with pytest.raises(corpus.QualificationError, match=message):
        corpus.load_request_packet(request)


def test_request_loaders_reject_wrong_suffix(tmp_path: Path) -> None:
    packet = corpus.load_hex_packet(corpus.DEFAULT_REQUEST)
    raw = tmp_path / "request.bin"
    raw.write_bytes(packet)
    with pytest.raises(corpus.QualificationError, match="canonical .hex or raw .gmabrq01"):
        corpus.load_request_packet(raw)
    with pytest.raises(corpus.QualificationError, match="must use .hex"):
        corpus.load_hex_packet(tmp_path / "request.gmabrq01")
    with pytest.raises(corpus.QualificationError, match="must use .gmabrq01"):
        corpus.load_raw_packet(tmp_path / "request.hex")


@pytest.mark.parametrize("request_name", ["../escape.gmabrq01", "request.bin"])
def test_corpus_request_path_and_suffix_fail_closed(
    tmp_path: Path,
    request_name: str,
) -> None:
    if request_name == "request.bin":
        (tmp_path / request_name).write_bytes(b"GMABRQ01")
    corpus_path = tmp_path / "corpus.json"
    corpus_path.write_text(
        json.dumps(
            {
                "schema": corpus.CORPUS_SCHEMA,
                "cases": [
                    {
                        "id": "bad-request",
                        "request": request_name,
                        "classification": "synthetic",
                    }
                ],
            }
        ),
        encoding="utf-8",
    )
    with pytest.raises(corpus.QualificationError, match="escapes|canonical .hex"):
        corpus.load_corpus(corpus_path)


def test_corpus_absolute_request_path_fails_closed(tmp_path: Path) -> None:
    request = tmp_path / "request.gmabrq01"
    request.write_bytes(corpus.load_hex_packet(corpus.DEFAULT_REQUEST))
    corpus_path = tmp_path / "corpus.json"
    corpus_path.write_text(
        json.dumps(
            {
                "schema": corpus.CORPUS_SCHEMA,
                "cases": [
                    {
                        "id": "absolute-request",
                        "request": str(request),
                        "classification": "synthetic",
                    }
                ],
            }
        ),
        encoding="utf-8",
    )
    with pytest.raises(corpus.QualificationError, match="relative to its corpus"):
        corpus.load_corpus(corpus_path)


def test_vendored_rt_real_board_evidence_is_exact_and_nonpromotional() -> None:
    packet_path = REAL_BOARD_VECTOR / "rt_super_c1_pwr4.gmabrq01"
    manifest_path = REAL_BOARD_VECTOR / "rt_super_c1_pwr4.exporter-manifest.json"
    corpus_path = REAL_BOARD_VECTOR / "corpus.json"
    report_path = REAL_BOARD_VECTOR / "qualification.local-5950x-dirty-build.json"

    assert packet_path.stat().st_size == 377_160
    assert hashlib.sha256(packet_path.read_bytes()).hexdigest() == (
        "bb52c537d109e782e4f24337dd57caf6a70bfae469a99a3e7ae58db1c4532413"
    )
    packet = corpus.load_request_packet(packet_path)
    assert corpus.request_accounting(packet) == {
        "job_count": 1,
        "stage_count": 3,
        "operand_count": 632,
        "planar_region_count": 292,
        "ring_count": 454,
        "input_segment_count": 5079,
        "disk_count": 290,
        "annulus_count": 0,
        "capsule_count": 16,
        "swept_path_count": 34,
        "relationship_query_count": 0,
    }

    manifest_bytes = manifest_path.read_bytes()
    assert len(manifest_bytes) == 20_411
    assert hashlib.sha256(manifest_bytes).hexdigest() == (
        "0f8ad9bcdc19d2644ad4e12f525b69af7c8ead11ba188139a81f9378eb2f57df"
    )
    manifest = json.loads(manifest_bytes)
    assert manifest_bytes == (
        json.dumps(manifest, sort_keys=True, separators=(",", ":"), ensure_ascii=True).encode() + b"\n"
    )
    assert manifest["packet"] == {
        "filename": "rt_super_c1_pwr4.gmabrq01",
        "format_magic": "GMABRQ01",
        "max_size_bytes": 16_777_216,
        "sha256": hashlib.sha256(packet).hexdigest(),
        "size_bytes": len(packet),
    }
    source = manifest["case"]["source"]
    assert source["sha256"] == "90545409b6ce16e0b636fce3176c90b73be301d03f0a9e32c4ab21a42ad14aca"
    assert source["source_checkout"]["clean"] is True
    assert source["source_checkout"]["head_revision"] == ("e4e58aec01fe76b9a4e9d699eea9aea18734fc0f")
    producer = manifest["producer"]
    assert producer["appz_checkout"]["clean"] is False
    assert producer["appz_checkout"]["head_revision"] == ("edfac72b6db4625a1c52ed667589d02298fd01d3")
    assert producer["appz_checkout"]["worktree_content_sha256"] == (
        "dbfe508907b67e5a0c25f824fe745f70dabdd1e4e27571ef948a8d53de823f42"
    )
    assert all(
        "/" in str(component.get("path", component.get("source_path", ""))) for component in producer["components"]
    )
    assert len(manifest["expectations"]["jobs"][0]["reconstructed_operand_groups"]) == 34
    assert len(manifest["expectations"]["junction_resolutions"]) == 1
    assert not any(
        token in manifest_bytes for token in (b"record_key", b"source_ref", b"job_id", b"operand_id", b"transient")
    )

    corpus_bytes = corpus_path.read_bytes()
    assert hashlib.sha256(corpus_bytes).hexdigest() == (
        "0636dd1feedcd746417bced237f6677d81a07876a623b7193862eacd811d0388"
    )
    corpus_payload = json.loads(corpus_bytes)
    assert corpus_bytes == corpus.canonical_bytes(corpus_payload) + b"\n"
    [case] = corpus.load_corpus(corpus_path)
    assert case.request_path == packet_path
    assert case.expected_result_sha256 == ("17477a9d1b7005a9bc8a097687fe1a0bf0453f1d8230bf260a8628330af997ad")
    assert case.source_provenance is not None
    assert case.source_provenance["exporter_revision"].find("manifest-sha256:0f8ad9bcdc19d264") >= 0

    report_bytes = report_path.read_bytes()
    assert hashlib.sha256(report_bytes).hexdigest() == (
        "3abb84f0b083fdcb6530d01a2c1e5ae4097b6cfe3ff37fc331610f5036e52de6"
    )
    report = json.loads(report_bytes)
    assert report_bytes.decode("utf-8").replace("\r\n", "\n") == (json.dumps(report, indent=2, sort_keys=True) + "\n")
    assert report["identity_sha256"] == corpus.identity_sha256(report["identity"])
    environment_evidence = report["environment"]
    for field in ("machine", "toolchain", "telemetry_helper"):
        evidence = environment_evidence[field]
        assert evidence["sha256"] == corpus.identity_sha256(evidence["profile"])
    [result] = report["cases"]
    assert result["request_sha256"] == hashlib.sha256(packet).hexdigest()
    assert {run["result_sha256"] for run in result["runs"]} == {case.expected_result_sha256}
    assert {run["result_region_count"] for run in result["runs"]} == {265}
    assert {run["result_segment_count"] for run in result["runs"]} == {5601}
    assert {run["failed_job_count"] for run in result["runs"]} == {0}
    assert result["solver_telemetry"]["counters"]["batch"]["failures"] == 0
    assert result["solver_telemetry"]["counters"]["batch"]["fallback_count"] == 0
    assert result["solver_telemetry"]["production_result_packet_match"] is True
    assert {run["solver_telemetry_sha256"] for run in result["runs"]} == {
        corpus.identity_sha256(result["solver_telemetry"]["counters"])
    }
    assert report["qualification"]["status"] == "pass"
    assert report["qualification"]["real_board_promotion_gate"] == "incomplete"
    assert report["qualification"]["reference_machine_gate"] == "fail"
    assert report["qualification"]["build_provenance_gate"] == "fail"


def test_external_real_board_manifest_requires_digest_provenance(tmp_path: Path) -> None:
    request = tmp_path / "request.hex"
    request.write_text(corpus.DEFAULT_REQUEST.read_text(encoding="ascii"), encoding="ascii")
    corpus_path = tmp_path / "corpus.json"
    corpus_path.write_text(
        json.dumps(
            {
                "schema": corpus.CORPUS_SCHEMA,
                "cases": [
                    {
                        "id": "board-a",
                        "request": "request.hex",
                        "classification": "external_real_board",
                    }
                ],
            }
        ),
        encoding="utf-8",
    )
    with pytest.raises(corpus.QualificationError, match="exact source/exporter/authorization"):
        corpus.load_corpus(corpus_path)

    value = json.loads(corpus_path.read_text(encoding="utf-8"))
    value["cases"][0]["source_provenance"] = {
        "source_identity": "owner:board-a",
        "source_sha256": "0" * 64,
        "exporter_identity": "fixture-exporter",
        "exporter_revision": "abc123",
        "redistribution_authorization": "authorized_for_qualification",
        "license_scope": "internal qualification redistribution",
    }
    corpus_path.write_text(json.dumps(value), encoding="utf-8")
    [case] = corpus.load_corpus(corpus_path)
    assert case.classification == "external_real_board"
    assert case.source_provenance is not None
    assert case.source_provenance["exporter_revision"] == "abc123"


def test_peak_rss_sampler_tracks_maximum_and_reset() -> None:
    readings = iter((10, 20, 15, 20, 20, 20))

    def reader(_pid: int) -> int | None:
        return next(readings, 8)

    sampler = environment.PeakRssSampler(123, reader, lambda _pid: 7)
    sampler.sample_once()
    sampler.sample_once()
    sampler.sample_once()
    assert sampler.peak() == 20
    assert sampler.reset() == 7
    assert sampler.peak() == 20


def test_identity_hash_excludes_runtime_observations() -> None:
    identity = {"request_sha256": "a" * 64, "repeat_count": 3}
    first = {"identity": identity, "runs": [{"wall_time_ns": 1}]}
    second = {"identity": identity, "runs": [{"wall_time_ns": 999}]}
    assert corpus.identity_sha256(first["identity"]) == corpus.identity_sha256(second["identity"])


def _case_report(*, ceiling: bool = True, target: bool = True, expected: bool = True) -> dict[str, object]:
    return {
        "summary": {
            "result_observation_count": 2,
            "process_envelope_hard_ceiling_met": ceiling,
            "process_envelope_target_observation_met": target,
            "deterministic_result": True,
            "expected_result_match": expected,
            "no_job_failures": True,
        },
        "solver_telemetry": {
            "observation_count": 0,
            "available": False,
            "production_result_packet_match": False,
            "deterministic": True,
            "target_observation_met": False,
            "hard_ceiling_met": False,
        },
    }


def test_ceiling_is_hard_while_process_target_is_opt_in() -> None:
    cases = corpus.load_corpus(None)
    assert runner._qualification(cases, [_case_report(target=False)], require_target=False)["status"] == "pass"
    target_required = runner._qualification(cases, [_case_report(target=False)], require_target=True)
    assert target_required["status"] == "fail"
    assert target_required["process_envelope_hard_ceiling_gate"] == "pass"
    ceiling_failed = runner._qualification(cases, [_case_report(ceiling=False)], require_target=False)
    assert ceiling_failed["status"] == "fail"
    assert ceiling_failed["process_envelope_hard_ceiling_gate"] == "fail"


def test_expected_result_mismatch_fails_correctness_gate() -> None:
    result = runner._qualification(corpus.load_corpus(None), [_case_report(expected=False)], require_target=False)
    assert result["status"] == "fail"
    assert result["correctness_gate"] == "fail"


def test_required_solver_telemetry_gate_fails_closed() -> None:
    cases = corpus.load_corpus(None)
    result = runner._qualification(cases, [_case_report()], require_target=False, require_solver_telemetry=True)
    assert result["status"] == "fail"
    assert result["solver_telemetry_gate"] == "not_run_or_failed"


def test_real_board_promotion_is_portable_and_requires_attested_build() -> None:
    report = _case_report()
    report["solver_telemetry"] = {
        "observation_count": 2,
        "available": True,
        "production_result_packet_match": True,
        "deterministic": True,
        "target_observation_met": True,
        "hard_ceiling_met": True,
    }
    cases = [corpus.QualificationCase("board", Path("board-request.hex"), "external_real_board", None, None)]
    comparative = runner._qualification(cases, [report], require_target=False, require_solver_telemetry=True)
    assert comparative["status"] == "pass"
    assert comparative["real_board_promotion_gate"] == "incomplete"
    assert comparative["reference_machine_observation"] == "different_or_unavailable"
    assert comparative["build_provenance_gate"] == "fail"
    promoted = runner._qualification(
        cases,
        [report],
        require_target=False,
        require_solver_telemetry=True,
        build_provenance_attested=True,
    )
    assert promoted["real_board_promotion_gate"] == "incomplete"
    assert promoted["real_board_expected_result_authority_gate"] == "fail"
    governed_cases = [
        corpus.QualificationCase("board", Path("board-request.hex"), "external_real_board", "0" * 64, None)
    ]
    promoted = runner._qualification(
        governed_cases,
        [report],
        require_target=False,
        require_solver_telemetry=True,
        build_provenance_attested=True,
    )
    assert promoted["real_board_expected_result_authority_gate"] == "pass"
    assert promoted["real_board_promotion_gate"] == "pass"
    assert promoted["reference_machine_observation"] == "different_or_unavailable"
    report_summary = report["summary"]
    report_telemetry = report["solver_telemetry"]
    assert isinstance(report_summary, dict) and isinstance(report_telemetry, dict)
    report_summary["result_observation_count"] = 1
    report_telemetry["observation_count"] = 1
    singleton = runner._qualification(
        governed_cases,
        [report],
        require_target=False,
        require_solver_telemetry=True,
        build_provenance_attested=True,
    )
    assert singleton["promotion_repeat_evidence_gate"] == "fail"
    assert singleton["real_board_promotion_gate"] == "incomplete"


def test_telemetry_document_requires_exact_nonnegative_counter_shape() -> None:
    value = {
        "schema": telemetry.TELEMETRY_SCHEMA,
        "status": "ok",
        "batch": {
            "candidate_pairs": 3,
            "emitted_bytes": 100,
            "failures": 0,
            "fallback_count": 0,
            "peak_working_memory_bytes": 200,
            "work_units": 400,
        },
        "jobs": [
            {
                "candidate_pairs": 3,
                "emitted_bytes": 80,
                "failures": 0,
                "fallback_count": 0,
                "job_id": 9,
                "peak_working_memory_bytes": 150,
                "work_units": 300,
            }
        ],
    }
    assert telemetry.validate_telemetry_document(value)["jobs"][0]["job_id"] == 9
    value["batch"]["work_units"] = -1
    with pytest.raises(corpus.QualificationError, match="nonnegative integer"):
        telemetry.validate_telemetry_document(value)


def test_required_telemetry_helper_discovery_names_remediation(monkeypatch: pytest.MonkeyPatch) -> None:
    monkeypatch.delenv(telemetry.TELEMETRY_HELPER_ENV, raising=False)
    monkeypatch.setattr(telemetry, "_candidate_paths", lambda: ())
    with pytest.raises(corpus.QualificationError, match=telemetry.TELEMETRY_HELPER_ENV):
        telemetry.resolve_telemetry_helper(None, required=True)


def test_stale_telemetry_helper_is_rejected_before_identity(monkeypatch: pytest.MonkeyPatch, tmp_path: Path) -> None:
    root = tmp_path / "workspace"
    helper = root / "build/tests/cpp/geometer_analytic_solver_telemetry_helper.exe"
    library = root / "build/src/cpp/lib/geometer.lib"
    helper_source = root / "tests/cpp/analytic_solver_telemetry_helper.cpp"
    for path in (helper, library, helper_source):
        path.parent.mkdir(parents=True, exist_ok=True)
        path.write_bytes(b"fixture")
    now = time.time_ns()
    os.utime(library, ns=(now - 2_000_000_000, now - 2_000_000_000))
    os.utime(helper, ns=(now - 1_000_000_000, now - 1_000_000_000))
    os.utime(helper_source, ns=(now, now))
    monkeypatch.setattr(telemetry, "ROOT", root)
    with pytest.raises(corpus.QualificationError, match="is stale"):
        telemetry.helper_profile(helper)


def test_telemetry_is_rejected_when_helper_result_differs_from_production(
    monkeypatch: pytest.MonkeyPatch, tmp_path: Path
) -> None:
    document = {
        "schema": telemetry.TELEMETRY_SCHEMA,
        "status": "ok",
        "batch": {
            "candidate_pairs": 0,
            "emitted_bytes": 1,
            "failures": 0,
            "fallback_count": 0,
            "peak_working_memory_bytes": 0,
            "work_units": 0,
        },
        "jobs": [],
    }

    def fake_run(arguments: list[str], **_kwargs: object) -> subprocess.CompletedProcess[str]:
        Path(arguments[2]).write_bytes(b"different")
        return subprocess.CompletedProcess(arguments, 0, json.dumps(document), "")

    monkeypatch.setattr(telemetry.subprocess, "run", fake_run)
    with pytest.raises(corpus.QualificationError, match="differ from production"):
        telemetry.execute_telemetry(tmp_path / "helper", b"request", b"production", 1.0)


def test_governed_packet_replays_through_production_executable_ipc() -> None:
    try:
        executable = executable_path()
    except FileNotFoundError:
        if os.environ.get("GEOMETER_REQUIRE_NATIVE_TEST_SERVERS") == "1":
            pytest.fail("native qualification requires the production Geometer executable")
        pytest.skip("native Geometer executable is unavailable")
    helper = telemetry.resolve_telemetry_helper(None, required=False)
    report = runner.qualify(
        corpus.load_corpus(None),
        executable,
        warmup_count=0,
        repeat_count=1,
        power_mode="test-unrecorded",
        require_target=False,
        telemetry_helper=helper,
        require_solver_telemetry=helper is not None,
    )
    case = report["cases"][0]
    assert report["qualification"]["status"] == "pass"
    assert report["qualification"]["real_board_evidence_present"] is False
    assert report["qualification"]["real_board_promotion_gate"] == "incomplete"
    assert report["identity"]["process_rss_measurement"]["transient_peak_can_be_missed"] is False
    assert len(report["identity"]["machine_profile_sha256"]) == 64
    assert len(report["identity"]["toolchain_profile_sha256"]) == 64
    toolchain = report["environment"]["toolchain"]["profile"]
    if toolchain.get("build_attestation_schema") is None:
        assert toolchain["hint_authority"] == "current_workspace_not_embedded_executable_provenance"
        assert toolchain["build_provenance_attested"] is False
    else:
        assert toolchain["build_attestation_schema"] == "wn.geometer.native_build_attestation.a1"
        source = toolchain["build"]["source"]
        assert not toolchain["build_provenance_attested"] or source["tree_state"] == "clean"
        assert toolchain["hint_authority"].startswith("validated_executable_bound_")
    assert len(toolchain["geometer_executable_sha256"]) == 64
    assert case["summary"]["expected_result_match"] is True
    assert case["summary"]["deterministic_result"] is True
    assert case["solver_telemetry"]["available"] is (helper is not None)
    if helper is not None:
        counters = case["solver_telemetry"]["counters"]
        assert counters["batch"]["emitted_bytes"] == case["runs"][0]["result_bytes"]
        assert counters["batch"]["fallback_count"] == 0
        assert counters["batch"]["failures"] == 0
        assert report["qualification"]["solver_telemetry_gate"] == "pass"
    assert case["runs"][0]["result_sha256"] == corpus.DEFAULT_RESULT_SHA256
    assert case["runs"][0]["process_peak_rss_bytes"] is not None
