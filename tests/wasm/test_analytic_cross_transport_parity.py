from __future__ import annotations

import hashlib
import json
import shutil
import subprocess
from pathlib import Path

import pytest

from geometer._analytic_packet_a0 import (
    decode_analytic_planar_boolean_batch_result_a0_packet,
    encode_analytic_planar_boolean_batch_request_a0_packet,
)
from geometer._generated.contracts.models import (
    AnalyticPlanarBooleanBatchRequestA0,
    AnalyticPlanarBooleanJob,
    AnalyticPlanarBooleanStage,
    ArcDirection,
    AuthoredCircularArcByRadiusSegment,
    AuthoredLineSegment,
    AuthoredVertex,
    OperationSuccessA0,
    PackedAttachmentProjectionA0,
    PackedAttachmentReferenceA0,
    PlanarRegionOperand,
    PlanarRing,
    PointNm,
    StageOperation,
    SuccessfulJobResult,
)
from geometer._ipc_a0 import Attachment
from geometer._ipc_client import (
    ANALYTIC_OPERATION,
    ANALYTIC_PACKET_FORMAT,
    ANALYTIC_REQUEST_ATTACHMENT,
    ANALYTIC_REQUEST_CONTRACT,
    ANALYTIC_REQUEST_MEDIA_TYPE,
    ANALYTIC_RESULT_ATTACHMENT,
    ANALYTIC_RESULT_CONTRACT,
    ANALYTIC_RESULT_MEDIA_TYPE,
    GeometerIpcClient,
)
from geometer._paths import executable_path


ROOT = Path(__file__).resolve().parents[2]
VECTOR_ROOT = ROOT / "tests/contracts/vectors/analytic"
MANIFEST_PATH = VECTOR_ROOT / "manifest.json"
WASM_RUNNER = ROOT / "tests/wasm/analytic_cross_transport_runner.js"


def _hex_bytes(path: Path) -> bytes:
    return bytes.fromhex(path.read_text(encoding="ascii"))


def _manifest() -> dict[str, object]:
    manifest = json.loads(MANIFEST_PATH.read_text(encoding="utf-8"))
    value = manifest.get("cross_transport_parity")
    assert isinstance(value, dict)
    return value


def _native_ipc_result(request_packet: bytes) -> bytes:
    projection = PackedAttachmentProjectionA0(
        schema=ANALYTIC_REQUEST_CONTRACT,
        packet=PackedAttachmentReferenceA0(
            attachment=ANALYTIC_REQUEST_ATTACHMENT,
            format=ANALYTIC_PACKET_FORMAT,
        ),
    )
    with GeometerIpcClient(
        executable_path(),
        client_name="analytic-cross-transport-parity",
        client_version="a0",
    ) as client:
        response = client.execute(
            ANALYTIC_OPERATION,
            projection,
            (
                Attachment(
                    name=ANALYTIC_REQUEST_ATTACHMENT,
                    media_type=ANALYTIC_REQUEST_MEDIA_TYPE,
                    data=request_packet,
                ),
            ),
            timeout=20,
        )
    assert isinstance(response.outcome, OperationSuccessA0)
    assert response.outcome.result == PackedAttachmentProjectionA0(
        schema=ANALYTIC_RESULT_CONTRACT,
        packet=PackedAttachmentReferenceA0(
            attachment=ANALYTIC_RESULT_ATTACHMENT,
            format=ANALYTIC_PACKET_FORMAT,
        ),
    )
    assert len(response.attachments) == 1
    assert response.attachments[0].name == ANALYTIC_RESULT_ATTACHMENT
    assert response.attachments[0].media_type == ANALYTIC_RESULT_MEDIA_TYPE
    return response.attachments[0].data


def test_governed_cross_transport_vector_integrity() -> None:
    manifest = _manifest()
    request_path = VECTOR_ROOT / str(manifest["request_file"])
    result_path = VECTOR_ROOT / str(manifest["result_file"])
    request_packet = _hex_bytes(request_path)
    result_packet = _hex_bytes(result_path)
    assert len(request_packet) == manifest["request_bytes"]
    assert hashlib.sha256(request_packet).hexdigest() == manifest["request_sha256"]
    assert len(result_packet) == manifest["result_bytes"]
    assert hashlib.sha256(result_packet).hexdigest() == manifest["result_sha256"]

    decoded = decode_analytic_planar_boolean_batch_result_a0_packet(result_packet)
    assert len(decoded.job_results) == 1
    job = decoded.job_results[0]
    assert isinstance(job, SuccessfulJobResult)
    assert job.job_id == manifest["job_id"]
    assert job.digest_sha256 == manifest["standalone_job_result_sha256"]
    assert len(job.result_regions) == 4
    assert len(job.rings) == 7
    assert len(job.directed_fragments) == 27
    assert len(decoded.relationship_results) == 1
    relationship = decoded.relationship_results[0]
    assert relationship.query_id == manifest["self_query_id"]
    assert relationship.status.value == "success"
    assert relationship.aggregate_dimension.value == "area"
    assert len(relationship.pairs) == 4
    assert all(
        pair.left_result_region_id == pair.right_result_region_id
        and pair.equality
        and pair.left_contains_right
        and pair.right_contains_left
        for pair in relationship.pairs
    )


def test_native_executable_ipc_and_browser_wasm_are_canonical_identical(tmp_path: Path) -> None:
    manifest = _manifest()
    request_path = VECTOR_ROOT / str(manifest["request_file"])
    expected = _hex_bytes(VECTOR_ROOT / str(manifest["result_file"]))
    request_packet = _hex_bytes(request_path)
    native = _native_ipc_result(request_packet)

    node = shutil.which("node")
    assert node is not None, "Node 24 is required for cross-transport parity."
    wasm_output = tmp_path / "browser-wasm-result.bin"
    completed = subprocess.run(
        [node, str(WASM_RUNNER), str(request_path), str(wasm_output)],
        cwd=ROOT,
        check=False,
        capture_output=True,
        text=True,
        timeout=60,
    )
    assert completed.returncode == 0, completed.stdout + completed.stderr
    wasm = wasm_output.read_bytes()

    assert native == expected
    assert wasm == expected
    assert native == wasm
    native_result = decode_analytic_planar_boolean_batch_result_a0_packet(native)
    wasm_result = decode_analytic_planar_boolean_batch_result_a0_packet(wasm)
    assert native_result.job_results[0].digest_sha256 == manifest["standalone_job_result_sha256"]
    assert wasm_result.job_results[0].digest_sha256 == manifest["standalone_job_result_sha256"]


@pytest.mark.parametrize("major_arc", [False, True], ids=["minor", "major"])
def test_endpoint_radius_irrational_center_is_native_wasm_identical(tmp_path: Path, *, major_arc: bool) -> None:
    request = AnalyticPlanarBooleanBatchRequestA0(
        jobs=(
            AnalyticPlanarBooleanJob(
                job_id=20,
                stages=(
                    AnalyticPlanarBooleanStage(
                        stage_id=112,
                        operation=StageOperation.UNION_STAGE,
                        operands=(
                            PlanarRegionOperand(
                                operand_id=1004,
                                kind="planar_region",
                                region_id=504,
                                outer=PlanarRing(
                                    ring_id=9020,
                                    vertices=(
                                        AuthoredVertex(
                                            vertex_id=6020,
                                            point=PointNm(x=0, y=0),
                                        ),
                                        AuthoredVertex(
                                            vertex_id=6021,
                                            point=PointNm(x=4000, y=0),
                                        ),
                                    ),
                                    segments=(
                                        AuthoredCircularArcByRadiusSegment(
                                            segment_id=7020,
                                            curve_id=8020,
                                            kind="circular_arc_by_radius",
                                            radius_nm=3000,
                                            direction=ArcDirection.CCW,
                                            major_arc=major_arc,
                                        ),
                                        AuthoredLineSegment(
                                            segment_id=7021,
                                            curve_id=8021,
                                            kind="line",
                                        ),
                                    ),
                                ),
                                holes=(),
                            ),
                        ),
                    ),
                ),
            ),
        ),
        relationship_queries=(),
    )
    request_packet = encode_analytic_planar_boolean_batch_request_a0_packet(request)
    branch = "major" if major_arc else "minor"
    request_path = tmp_path / f"endpoint-radius-{branch}-request.hex"
    request_path.write_text(request_packet.hex(), encoding="ascii")
    native = _native_ipc_result(request_packet)

    node = shutil.which("node")
    assert node is not None, "Node 24 is required for cross-transport parity."
    wasm_output = tmp_path / f"endpoint-radius-{branch}-wasm-result.bin"
    completed = subprocess.run(
        [node, str(WASM_RUNNER), str(request_path), str(wasm_output)],
        cwd=ROOT,
        check=False,
        capture_output=True,
        text=True,
        timeout=60,
    )
    assert completed.returncode == 0, completed.stdout + completed.stderr
    wasm = wasm_output.read_bytes()
    assert native == wasm

    decoded = decode_analytic_planar_boolean_batch_result_a0_packet(native)
    assert len(decoded.job_results) == 1
    job = decoded.job_results[0]
    assert isinstance(job, SuccessfulJobResult)
    assert job.job_id == 20
    assert len(job.result_regions) == 1
    assert len(job.directed_fragments) == (4 if major_arc else 2)
    arcs = tuple(fragment for fragment in job.directed_fragments if fragment.kind == "circular_arc")
    assert len(arcs) == (3 if major_arc else 1)
    assert all(arc.radius_nm == 3000 and not arc.major_arc for arc in arcs)
    for arc in arcs:
        sources = arc.coincident_positive_sources.sources
        assert len(sources) == 1
        assert sources[0].kind.value == "authored_segment_curve"
        assert sources[0].role.value == "authored_circular_arc"
        assert sources[0].operand_id == 1004
        assert sources[0].primary_id == 7020
        assert sources[0].secondary_id == 8020
