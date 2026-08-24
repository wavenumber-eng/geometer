from __future__ import annotations

import io
import os
import struct
import time
from dataclasses import replace
from pathlib import Path

import pytest

import geometer
from geometer._generated.contracts.codecs import encode_ipc_shutdown_ack_a0_json, encode_operation_outcome_a0_json
from geometer._generated.contracts.models import (
    AnalyticPlanarBooleanJob,
    AnalyticPlanarBooleanBatchRequestA0,
    AnalyticPlanarBooleanStage,
    DiskOperand,
    DiagnosticA0,
    DiagnosticCategory,
    OperationFailureA0,
    OperationSuccessA0,
    PackedAttachmentProjectionA0,
    PackedAttachmentReferenceA0,
    PointNm,
    StageOperation,
    IpcShutdownAckA0,
)
from geometer._ipc_a0 import Attachment, Frame, FrameKind, IpcFrameError, read_frame, write_frame
from geometer._ipc_client import (
    ANALYTIC_OPERATION,
    ANALYTIC_PACKET_FORMAT,
    ANALYTIC_REQUEST_ATTACHMENT,
    ANALYTIC_REQUEST_CONTRACT,
    ANALYTIC_RESULT_ATTACHMENT,
    ANALYTIC_RESULT_CONTRACT,
    GeometerIpcClient,
    GeometerIpcError,
    GeometerIpcProcessError,
    GeometerIpcProtocolError,
    GeometerIpcTimeoutError,
    GeometerOperationError,
    NORMALIZED_CATALOG_SHA256,
    _ReadFailure,
    _validate_welcome,
)
from geometer._generated.contracts.operations import expected_operation_catalog
from geometer._paths import executable_path


ROOT = Path(__file__).resolve().parents[2]
REQUIRE_NATIVE_TEST_SERVERS_ENV = "GEOMETER_REQUIRE_NATIVE_TEST_SERVERS"


def test_shutdown_frame_matches_the_governed_exact_bytes() -> None:
    stream = io.BytesIO()
    write_frame(stream, Frame(kind=FrameKind.SHUTDOWN, request_id=0, json=b"{}"))
    assert stream.getvalue() == bytes.fromhex(
        "47 4d 49 50 43 41 30 31 30 00 00 00 08 00 00 00"
        " 00 00 00 00 00 00 00 00 02 00 00 00 00 00 00 00"
        " 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00"
        " 7b 7d"
    )


def test_binary_attachment_frame_round_trips() -> None:
    expected = Frame(
        kind=FrameKind.REQUEST,
        request_id=(1 << 64) - 1,
        json=b'{"operation":"fixture"}',
        attachments=(
            Attachment(
                name="model",
                media_type="application/step",
                data=b"\x00\xffbinary\x00",
            ),
        ),
    )
    stream = io.BytesIO()
    write_frame(stream, expected)
    stream.seek(0)
    assert read_frame(stream) == expected
    assert read_frame(stream) is None


def test_frame_codec_rejects_ambiguous_or_malformed_sections() -> None:
    duplicate = Frame(
        kind=FrameKind.REQUEST,
        request_id=1,
        json=b"{}",
        attachments=(
            Attachment(name="packet", media_type="application/x.packet", data=b"a"),
            Attachment(name="packet", media_type="application/x.packet", data=b"b"),
        ),
    )
    with pytest.raises(IpcFrameError, match="duplicated"):
        write_frame(io.BytesIO(), duplicate)

    with pytest.raises(IpcFrameError, match="control frames"):
        write_frame(
            io.BytesIO(),
            Frame(
                kind=FrameKind.SHUTDOWN,
                request_id=0,
                json=b"{}",
                attachments=(Attachment(name="x", media_type="application/x", data=b"x"),),
            ),
        )

    truncated = io.BytesIO(b"GMIPCA01")
    with pytest.raises(IpcFrameError, match="unexpected EOF"):
        read_frame(truncated)

    reserved_header = bytearray(48)
    struct.pack_into("<8sHHHHQIIQII", reserved_header, 0, b"GMIPCA01", 48, 0, 8, 1, 0, 2, 0, 0, 0, 0)
    with pytest.raises(IpcFrameError, match="reserved"):
        read_frame(io.BytesIO(reserved_header + b"{}"))

    oversized_for_negotiated_limit = io.BytesIO()
    write_frame(oversized_for_negotiated_limit, Frame(kind=FrameKind.RESPONSE, request_id=1, json=b"{}"))
    oversized_for_negotiated_limit.seek(0)
    with pytest.raises(IpcFrameError, match="JSON section"):
        read_frame(oversized_for_negotiated_limit, max_json_bytes=1)


def test_persistent_client_validates_welcome_and_gracefully_shuts_down() -> None:
    executable = executable_path()
    if not executable.is_file():
        pytest.skip("native Geometer executable is unavailable")
    with GeometerIpcClient(executable, client_name="python-ipc-test", client_version="a0") as client:
        assert client.process_id > 0
        assert client.welcome.ipc == "a0"
        assert client.welcome.catalog_sha256 == NORMALIZED_CATALOG_SHA256
        expected_catalog = expected_operation_catalog(
            client.welcome.release_version,
            client.welcome.c_abi_generation,
        )
        assert client.welcome.operation_catalog == expected_catalog
        assert client.stderr_text == ""


def test_generated_catalog_authority_rejects_mutated_welcome_order() -> None:
    executable = executable_path()
    if not executable.is_file():
        pytest.skip("native Geometer executable is unavailable")
    with GeometerIpcClient(executable, client_name="python-catalog-test") as client:
        welcome = client.welcome
    mutated_catalog = replace(
        welcome.operation_catalog,
        operations=tuple(reversed(welcome.operation_catalog.operations)),
    )
    with pytest.raises(GeometerIpcProtocolError, match="generated authority"):
        _validate_welcome(replace(welcome, operation_catalog=mutated_catalog))


def test_persistent_client_rejects_missing_named_attachment_before_write() -> None:
    executable = executable_path()
    if not executable.is_file():
        pytest.skip("native Geometer executable is unavailable")
    projection = PackedAttachmentProjectionA0(
        schema=ANALYTIC_REQUEST_CONTRACT,
        packet=PackedAttachmentReferenceA0(
            attachment=ANALYTIC_REQUEST_ATTACHMENT,
            format=ANALYTIC_PACKET_FORMAT,
        ),
    )
    with GeometerIpcClient(executable, client_name="python-ipc-test", client_version="a0") as client:
        with pytest.raises(GeometerIpcProtocolError, match="missing required attachments"):
            client.execute(ANALYTIC_OPERATION, projection)


@pytest.mark.parametrize("timeout", [0, -1, float("inf"), float("nan")])
def test_client_rejects_invalid_local_deadline(timeout: float) -> None:
    with pytest.raises(ValueError, match="finite positive"):
        GeometerIpcClient(startup_timeout=timeout)


def test_friendly_analytic_call_round_trips_empty_batch() -> None:
    executable = executable_path()
    if not executable.is_file():
        pytest.skip("native Geometer executable is unavailable")
    request = AnalyticPlanarBooleanBatchRequestA0(jobs=(), relationship_queries=())
    with GeometerIpcClient(executable, client_name="python-ipc-test", client_version="a0") as client:
        result = client.analytic_planar_boolean_batch(request)
    assert result.job_results == ()
    assert result.relationship_results == ()


def _nonempty_request() -> AnalyticPlanarBooleanBatchRequestA0:
    return AnalyticPlanarBooleanBatchRequestA0(
        jobs=(
            AnalyticPlanarBooleanJob(
                job_id=1,
                stages=(
                    AnalyticPlanarBooleanStage(
                        stage_id=1,
                        operation=StageOperation.UNION_STAGE,
                        operands=(
                            DiskOperand(
                                operand_id=1,
                                kind="disk",
                                feature_id=1,
                                center=PointNm(x=0, y=0),
                                radius_nm=1_000_000,
                            ),
                        ),
                    ),
                ),
            ),
        ),
        relationship_queries=(),
    )


def _native_test_server(
    name: str,
    *,
    root: Path = ROOT,
    platform_tag: str | None = None,
) -> Path:
    suffix = ".exe" if os.name == "nt" else ""
    if platform_tag is None:
        from geometer._paths import _platform_tag

        platform_tag = _platform_tag()
    candidates = (
        root / f"build-native-{platform_tag}" / "tests" / "cpp" / f"{name}{suffix}",
        root / "build" / "tests" / "cpp" / f"{name}{suffix}",
    )
    for candidate in candidates:
        if candidate.is_file():
            return candidate
    if os.environ.get(REQUIRE_NATIVE_TEST_SERVERS_ENV) == "1":
        pytest.fail(
            f"native qualification requires {name}; probed: "
            + ", ".join(str(candidate) for candidate in candidates)
        )
    return candidates[0]


def test_native_test_server_prefers_validate_native_platform_tree(tmp_path: Path) -> None:
    name = "fixture_server"
    suffix = ".exe" if os.name == "nt" else ""
    fallback = tmp_path / "build" / "tests" / "cpp" / f"{name}{suffix}"
    current = tmp_path / "build-native-fixture-x64" / "tests" / "cpp" / f"{name}{suffix}"
    fallback.parent.mkdir(parents=True)
    current.parent.mkdir(parents=True)
    fallback.touch()
    current.touch()
    assert _native_test_server(name, root=tmp_path, platform_tag="fixture-x64") == current


def test_native_qualification_requires_fake_servers(tmp_path: Path, monkeypatch: pytest.MonkeyPatch) -> None:
    monkeypatch.setenv(REQUIRE_NATIVE_TEST_SERVERS_ENV, "1")
    with pytest.raises(pytest.fail.Exception, match="native qualification requires missing_server"):
        _native_test_server("missing_server", root=tmp_path, platform_tag="fixture-x64")


def test_persistent_client_repeats_nonempty_solve_on_one_connection() -> None:
    executable = executable_path()
    if not executable.is_file():
        pytest.skip("native Geometer executable is unavailable")
    with GeometerIpcClient(executable, client_name="python-ipc-test", client_version="a0") as client:
        first = client.analytic_planar_boolean_batch(_nonempty_request(), timeout=5)
        second = client.analytic_planar_boolean_batch(_nonempty_request(), timeout=5)
    assert first == second


def test_timeout_is_typed_and_prevents_reuse_while_native_fake_server_drains() -> None:
    server = _native_test_server("geometer_ipc_a0_deadline_test_server")
    if not server.is_file():
        pytest.skip("native deadline fake server is unavailable")
    client = GeometerIpcClient(server, client_name="python-timeout-test", shutdown_timeout=0.25)
    with pytest.raises(GeometerIpcTimeoutError, match="does not prove interruption") as caught:
        client.analytic_planar_boolean_batch(_nonempty_request(), timeout=0.05)
    assert caught.value.request_id == 1
    with pytest.raises(GeometerIpcError, match="still draining"):
        client.analytic_planar_boolean_batch(_nonempty_request(), timeout=0.05)
    started = time.monotonic()
    with pytest.raises(GeometerIpcError):
        client.close()
    assert time.monotonic() - started < 2


def test_process_crash_poison_connection_with_native_fake_server() -> None:
    server = _native_test_server("geometer_ipc_a0_unexpected_exit_test_server")
    if not server.is_file():
        pytest.skip("native unexpected-exit fake server is unavailable")
    client = GeometerIpcClient(server, client_name="python-crash-test")
    with pytest.raises(GeometerIpcProcessError, match="closed stdout"):
        client.analytic_planar_boolean_batch(_nonempty_request(), timeout=2)
    with pytest.raises(GeometerIpcProcessError, match="closed"):
        client.analytic_planar_boolean_batch(_nonempty_request(), timeout=1)


@pytest.mark.parametrize(
    ("injected", "message"),
    [
        (Frame(kind=FrameKind.RESPONSE, request_id=2, json=b"{}"), "expected response"),
        (Frame(kind=FrameKind.CANCELLED, request_id=1, json=b'{"status":"cancelled"}'), "expected response"),
        (Frame(kind=FrameKind.RESPONSE, request_id=1, json=b"{"), "invalid generated outcome"),
        (_ReadFailure(IpcFrameError("fixture malformed header")), "malformed IPC framing"),
    ],
)
def test_post_write_protocol_faults_are_typed_and_poison_connection(
    injected: Frame | _ReadFailure, message: str
) -> None:
    executable = executable_path()
    if not executable.is_file():
        pytest.skip("native Geometer executable is unavailable")
    client = GeometerIpcClient(executable, client_name="python-protocol-test")
    client._stdout_queue.put(injected)
    with pytest.raises(GeometerIpcProtocolError, match=message):
        client.analytic_planar_boolean_batch(_nonempty_request(), timeout=2)
    with pytest.raises(GeometerIpcProcessError, match="closed"):
        client.analytic_planar_boolean_batch(_nonempty_request(), timeout=1)


def test_friendly_analytic_call_raises_typed_operation_failure() -> None:
    executable = executable_path()
    if not executable.is_file():
        pytest.skip("native Geometer executable is unavailable")
    failure = OperationFailureA0(
        operation=ANALYTIC_OPERATION,
        ok=False,
        diagnostics=(
            DiagnosticA0(
                code="fixture.operation.failed",
                category=DiagnosticCategory.OPERATION,
                message="fixture",
                retryable=False,
                operation=ANALYTIC_OPERATION,
            ),
        ),
    )
    client = GeometerIpcClient(executable, client_name="python-failure-test")
    client._stdout_queue.put(
        Frame(kind=FrameKind.RESPONSE, request_id=1, json=encode_operation_outcome_a0_json(failure))
    )
    with pytest.raises(GeometerOperationError) as caught:
        client.analytic_planar_boolean_batch(_nonempty_request(), timeout=2)
    assert caught.value.diagnostics == failure.diagnostics
    client._terminate()


@pytest.mark.parametrize(
    "acknowledgment",
    [
        IpcShutdownAckA0(
            status="complete",
            active_request_completed=True,
            rejected_queued_request_count=0,
        ),
        IpcShutdownAckA0(
            status="complete",
            active_request_completed=False,
            rejected_queued_request_count=1,
        ),
    ],
)
def test_shutdown_ack_must_reconcile_with_no_pending_request(acknowledgment: IpcShutdownAckA0) -> None:
    executable = executable_path()
    if not executable.is_file():
        pytest.skip("native Geometer executable is unavailable")
    client = GeometerIpcClient(executable, client_name="python-shutdown-test")
    client._stdout_queue.put(
        Frame(
            kind=FrameKind.SHUTDOWN_ACK,
            request_id=0,
            json=encode_ipc_shutdown_ack_a0_json(acknowledgment),
        )
    )
    with pytest.raises(GeometerIpcProtocolError, match="contradicts observed"):
        client.close()


@pytest.mark.parametrize("invalid_packet", [False, True])
def test_analytic_projection_or_packet_protocol_fault_poisons_connection(invalid_packet: bool) -> None:
    executable = executable_path()
    if not executable.is_file():
        pytest.skip("native Geometer executable is unavailable")
    projection = PackedAttachmentProjectionA0(
        schema=ANALYTIC_RESULT_CONTRACT if invalid_packet else "fixture.wrong.result",
        packet=PackedAttachmentReferenceA0(
            attachment=ANALYTIC_RESULT_ATTACHMENT,
            format=ANALYTIC_PACKET_FORMAT,
        ),
    )
    success = OperationSuccessA0(operation=ANALYTIC_OPERATION, ok=True, result=projection)
    client = GeometerIpcClient(executable, client_name="python-analytic-fault-test")
    client._stdout_queue.put(
        Frame(
            kind=FrameKind.RESPONSE,
            request_id=1,
            json=encode_operation_outcome_a0_json(success),
            attachments=(
                Attachment(
                    name=ANALYTIC_RESULT_ATTACHMENT,
                    media_type="application/vnd.wavenumber.geometer.analytic-planar-boolean-result",
                    data=b"invalid packet",
                ),
            ),
        )
    )
    with pytest.raises(GeometerIpcProtocolError):
        client.analytic_planar_boolean_batch(_nonempty_request(), timeout=2)
    with pytest.raises(GeometerIpcProcessError, match="closed"):
        client.analytic_planar_boolean_batch(_nonempty_request(), timeout=1)


def test_public_package_exports_client_and_analytic_construction_surface() -> None:
    assert geometer.GeometerClient is GeometerIpcClient
    assert geometer.GeometerIpcTimeoutError is GeometerIpcTimeoutError
    assert callable(geometer.encode_analytic_planar_boolean_batch_request_a0_packet)
    assert callable(geometer.decode_analytic_planar_boolean_batch_result_a0_packet)
    assert geometer.AnalyticPlanarBooleanBatchRequestA0 is AnalyticPlanarBooleanBatchRequestA0
    assert geometer.AnalyticPointNm(x=1, y=-1).x == 1
    assert geometer.StageOperation.UNION_STAGE.value == "union"
    assert (
        geometer.DiskOperand(
            operand_id=1,
            kind="disk",
            feature_id=1,
            center=geometer.AnalyticPointNm(x=0, y=0),
            radius_nm=1,
        ).kind
        == "disk"
    )
