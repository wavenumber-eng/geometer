from __future__ import annotations

from dataclasses import replace
import hashlib
import json
from pathlib import Path
import struct
from typing import Any

import pytest

from geometer import (
    AnalyticPacketError,
    decode_analytic_planar_boolean_batch_result_a0_packet,
    encode_analytic_planar_boolean_batch_request_a0_packet,
)
from geometer._analytic_packet_a0 import (
    MAX_LOGICAL_SOURCE_REFERENCE_EXPANSIONS,
    RESULT_TABLES,
    _preflight_logical_source_reference_expansions,
)
from geometer._generated.contracts.models import (
    AnalyticPlanarBooleanBatchRequestA0,
    AnalyticPlanarBooleanJob,
    AnalyticPlanarBooleanStage,
    AnnulusOperand,
    ArcDirection,
    AuthoredCircularArcByRadiusSegment,
    AuthoredCircularArcSegment,
    AuthoredLineSegment,
    AuthoredVertex,
    CapsuleOperand,
    DiskOperand,
    FailedJobResult,
    PlanarPath,
    PlanarRegionOperand,
    PlanarRelationshipQuery,
    PlanarRing,
    PointNm,
    StageOperation,
    SuccessfulJobResult,
    SweptPathOperand,
)


ROOT = Path(__file__).resolve().parents[2]
VECTOR_ROOT = ROOT / "tests" / "contracts" / "vectors" / "analytic"


def _compact_expansion_tables(
    count: int, uses: tuple[tuple[int, int, int, int], ...] = ((2, 0, 24, 1),)
) -> tuple[memoryview, list[tuple[int, int, int]]]:
    table_storage = [bytearray() for _ in RESULT_TABLES]
    for table_index, record_index, field_offset, handle in uses:
        required_bytes = (record_index + 1) * RESULT_TABLES[table_index]
        table_storage[table_index].extend(b"\0" * (required_bytes - len(table_storage[table_index])))
        struct.pack_into(
            "<I",
            table_storage[table_index],
            record_index * RESULT_TABLES[table_index] + field_offset,
            handle,
        )
    table_storage[8] = bytearray(RESULT_TABLES[8])
    struct.pack_into("<I", table_storage[8], 4, count)
    storage = bytearray()
    tables = [(0, 0, size) for size in RESULT_TABLES]
    for index, value in enumerate(table_storage):
        offset = len(storage)
        storage.extend(value)
        tables[index] = (offset, len(value) // RESULT_TABLES[index], RESULT_TABLES[index])
    return memoryview(storage), tables


def _compact_expansion_packet(count: int, uses: tuple[tuple[int, int, int, int], ...]) -> bytes:
    _, compact_tables = _compact_expansion_tables(count, uses)
    table_bytes = [bytearray(table_count * record_bytes) for _, table_count, record_bytes in compact_tables]
    for table_index, record_index, field_offset, handle in uses:
        struct.pack_into(
            "<I", table_bytes[table_index], record_index * RESULT_TABLES[table_index] + field_offset, handle
        )
    struct.pack_into("<I", table_bytes[8], 4, count)

    cursor = 64 + 32 * len(RESULT_TABLES)
    offsets: list[int] = []
    for index, value in enumerate(table_bytes):
        offsets.append(cursor)
        cursor += len(value)
        if index + 1 != len(table_bytes):
            cursor = (cursor + 7) & ~7
    packet = bytearray(cursor)
    packet[:8] = b"GMABRS01"
    struct.pack_into("<HHIQQIII", packet, 8, 1, 64, 0, cursor, 64, len(RESULT_TABLES), 0, 0)
    struct.pack_into("<Q", packet, 48, sum(map(len, table_bytes)))
    for index, value in enumerate(table_bytes):
        struct.pack_into(
            "<HHIQQQ",
            packet,
            64 + index * 32,
            101 + index,
            1,
            RESULT_TABLES[index],
            offsets[index],
            len(value),
            len(value) // RESULT_TABLES[index],
        )
        packet[offsets[index] : offsets[index] + len(value)] = value
    return bytes(packet)


def _cpp_exemplar_request() -> AnalyticPlanarBooleanBatchRequestA0:
    def vertex(identity: int, x: int, y: int) -> AuthoredVertex:
        return AuthoredVertex(vertex_id=identity, point=PointNm(x=x, y=y))

    outer = PlanarRing(
        ring_id=600,
        vertices=(vertex(700, 0, 0), vertex(701, 10_000, 0), vertex(702, 10_000, 10_000), vertex(703, 0, 10_000)),
        segments=tuple(
            AuthoredLineSegment(segment_id=800 + index, curve_id=900 + index, kind="line") for index in range(4)
        ),
    )
    hole = PlanarRing(
        ring_id=601,
        vertices=(vertex(704, 4_000, 5_000), vertex(705, 6_000, 5_000)),
        segments=tuple(
            AuthoredCircularArcSegment(
                segment_id=804 + index,
                curve_id=904,
                kind="circular_arc",
                center=PointNm(x=5_000, y=5_000),
                direction=ArcDirection.CCW,
                major_arc=False,
            )
            for index in range(2)
        ),
    )
    path = PlanarPath(
        path_id=602,
        vertices=(vertex(706, 20_000, 20_000), vertex(707, 30_000, 20_000)),
        segments=(AuthoredLineSegment(segment_id=806, curve_id=906, kind="line"),),
    )
    stages = (
        AnalyticPlanarBooleanStage(
            stage_id=100,
            operation=StageOperation.UNION_STAGE,
            operands=(
                PlanarRegionOperand(operand_id=1000, kind="planar_region", region_id=500, outer=outer, holes=(hole,)),
                DiskOperand(operand_id=1001, kind="disk", feature_id=300, center=PointNm(x=1, y=2), radius_nm=250),
                AnnulusOperand(
                    operand_id=1002,
                    kind="annulus",
                    feature_id=310,
                    center=PointNm(x=0, y=0),
                    inner_radius_nm=100,
                    outer_radius_nm=200,
                ),
            ),
        ),
        AnalyticPlanarBooleanStage(
            stage_id=101,
            operation=StageOperation.DIFFERENCE,
            operands=(
                CapsuleOperand(
                    operand_id=1003,
                    kind="capsule",
                    feature_id=320,
                    start=PointNm(x=0, y=0),
                    end=PointNm(x=1_000, y=0),
                    width_nm=50,
                ),
                SweptPathOperand(
                    operand_id=1004,
                    kind="swept_path",
                    feature_id=330,
                    centerline=path,
                    width_nm=40,
                    cap="round",
                    join="round",
                ),
            ),
        ),
    )
    return AnalyticPlanarBooleanBatchRequestA0(
        jobs=(
            AnalyticPlanarBooleanJob(job_id=10, stages=stages),
            AnalyticPlanarBooleanJob(
                job_id=20,
                stages=(
                    AnalyticPlanarBooleanStage(
                        stage_id=102,
                        operation=StageOperation.UNION_STAGE,
                        operands=(
                            DiskOperand(
                                operand_id=1005,
                                kind="disk",
                                feature_id=301,
                                center=PointNm(x=-5, y=-6),
                                radius_nm=1_000_000_000_000,
                            ),
                        ),
                    ),
                ),
            ),
        ),
        relationship_queries=(
            PlanarRelationshipQuery(query_id=5000, left_job_id=10, right_job_id=20),
            PlanarRelationshipQuery(query_id=5001, left_job_id=20, right_job_id=10),
        ),
    )


def _request() -> AnalyticPlanarBooleanBatchRequestA0:
    maximum = (1 << 64) - 1
    return AnalyticPlanarBooleanBatchRequestA0(
        jobs=(
            AnalyticPlanarBooleanJob(
                job_id=maximum,
                stages=(
                    AnalyticPlanarBooleanStage(
                        stage_id=maximum,
                        operation=StageOperation.UNION_STAGE,
                        operands=(
                            DiskOperand(
                                operand_id=maximum,
                                kind="disk",
                                feature_id=maximum,
                                center=PointNm(x=-(1 << 63), y=(1 << 63) - 1),
                                radius_nm=1,
                            ),
                        ),
                    ),
                ),
            ),
        ),
        relationship_queries=(PlanarRelationshipQuery(query_id=maximum, left_job_id=maximum, right_job_id=maximum),),
    )


def _corpus() -> tuple[dict[str, Any], dict[str, bytes]]:
    manifest = json.loads((VECTOR_ROOT / "manifest.json").read_text(encoding="utf-8"))
    vectors: dict[str, bytes] = {}
    for entry in manifest["vectors"]:
        data = bytes.fromhex((VECTOR_ROOT / entry["file"]).read_text(encoding="ascii"))
        assert len(data) == entry["bytes"]
        assert hashlib.sha256(data).hexdigest() == entry["sha256"]
        vectors[entry["id"]] = data
    return manifest, vectors


def test_request_encoder_emits_exact_empty_and_rich_canonical_packets() -> None:
    _, vectors = _corpus()
    empty = encode_analytic_planar_boolean_batch_request_a0_packet(
        AnalyticPlanarBooleanBatchRequestA0(jobs=(), relationship_queries=())
    )
    assert empty == vectors["request.empty"]
    assert len(empty) == 480
    assert empty[:8] == b"GMABRQ01"
    assert struct.unpack_from("<Q", empty, 16)[0] == 480
    assert struct.unpack_from("<I", empty, 32)[0] == 13

    assert (
        encode_analytic_planar_boolean_batch_request_a0_packet(_cpp_exemplar_request()) == vectors["request.exemplar"]
    )
    encoded = encode_analytic_planar_boolean_batch_request_a0_packet(_request())
    assert struct.unpack_from("<Q", encoded, struct.unpack_from("<Q", encoded, 72)[0])[0] == (1 << 64) - 1
    assert struct.unpack_from("<I", encoded, 40)[0] == 1


def test_request_encoder_preserves_center_arc_bytes_and_adds_exact_radius_variant() -> None:
    ring = PlanarRing(
        ring_id=1,
        vertices=(
            AuthoredVertex(vertex_id=1, point=PointNm(x=0, y=0)),
            AuthoredVertex(vertex_id=2, point=PointNm(x=6, y=0)),
        ),
        segments=(
            AuthoredCircularArcByRadiusSegment(
                segment_id=1,
                curve_id=1,
                kind="circular_arc_by_radius",
                radius_nm=5,
                direction=ArcDirection.CCW,
                major_arc=False,
            ),
            AuthoredCircularArcByRadiusSegment(
                segment_id=2,
                curve_id=2,
                kind="circular_arc_by_radius",
                radius_nm=5,
                direction=ArcDirection.CCW,
                major_arc=True,
            ),
        ),
    )
    request = AnalyticPlanarBooleanBatchRequestA0(
        jobs=(
            AnalyticPlanarBooleanJob(
                job_id=1,
                stages=(
                    AnalyticPlanarBooleanStage(
                        stage_id=1,
                        operation=StageOperation.UNION_STAGE,
                        operands=(
                            PlanarRegionOperand(
                                operand_id=1,
                                kind="planar_region",
                                region_id=1,
                                outer=ring,
                                holes=(),
                            ),
                        ),
                    ),
                ),
            ),
        ),
        relationship_queries=(),
    )
    packet = encode_analytic_planar_boolean_batch_request_a0_packet(request)
    segment_table_offset = struct.unpack_from("<Q", packet, 64 + 7 * 32 + 8)[0]
    assert packet[segment_table_offset + 16] == 3
    assert packet[segment_table_offset + 17] == 1
    assert packet[segment_table_offset + 18] == 0
    assert struct.unpack_from("<Q", packet, segment_table_offset + 24)[0] == 5
    assert packet[segment_table_offset + 32 : segment_table_offset + 40] == b"\0" * 8
    assert packet[segment_table_offset + 40 + 16] == 3
    assert packet[segment_table_offset + 40 + 18] == 1

    # Existing center-form canonical vectors remain byte-for-byte unchanged.
    assert (
        encode_analytic_planar_boolean_batch_request_a0_packet(_cpp_exemplar_request())
        == _corpus()[1]["request.exemplar"]
    )


def test_request_encoder_rejects_endpoint_radius_arc_in_swept_path() -> None:
    request = _cpp_exemplar_request()
    stage = request.jobs[0].stages[1]
    swept = stage.operands[1]
    assert isinstance(swept, SweptPathOperand)
    invalid_path = replace(
        swept.centerline,
        segments=(
            AuthoredCircularArcByRadiusSegment(
                segment_id=806,
                curve_id=906,
                kind="circular_arc_by_radius",
                radius_nm=10_000,
                direction=ArcDirection.CCW,
                major_arc=False,
            ),
        ),
    )
    invalid_swept = replace(swept, centerline=invalid_path)  # type: ignore[arg-type]
    invalid_stage = replace(stage, operands=(stage.operands[0], invalid_swept))
    invalid_job = replace(request.jobs[0], stages=(request.jobs[0].stages[0], invalid_stage))
    with pytest.raises(AnalyticPacketError, match="not supported in swept paths"):
        encode_analytic_planar_boolean_batch_request_a0_packet(replace(request, jobs=(invalid_job, request.jobs[1])))


def test_request_encoder_rejects_wrong_runtime_shapes_ids_and_references() -> None:
    request = _request()
    invalid_request: object = {"jobs": (), "relationship_queries": ()}
    with pytest.raises(AnalyticPacketError):
        encode_analytic_planar_boolean_batch_request_a0_packet(invalid_request)  # type: ignore[arg-type]
    with pytest.raises(AnalyticPacketError):
        encode_analytic_planar_boolean_batch_request_a0_packet(
            replace(request, jobs=list(request.jobs))  # type: ignore[arg-type]
        )
    with pytest.raises(AnalyticPacketError):
        encode_analytic_planar_boolean_batch_request_a0_packet(
            replace(request, jobs=(object(),))  # type: ignore[arg-type]
        )
    with pytest.raises(AnalyticPacketError):
        encode_analytic_planar_boolean_batch_request_a0_packet(
            replace(request, relationship_queries=(object(),))  # type: ignore[arg-type]
        )
    with pytest.raises(AnalyticPacketError):
        encode_analytic_planar_boolean_batch_request_a0_packet(
            replace(
                request,
                jobs=(
                    replace(
                        request.jobs[0],
                        stages=(
                            replace(request.jobs[0].stages[0], operands=(object(),)),  # type: ignore[arg-type]
                        ),
                    ),
                ),
            )
        )
    with pytest.raises(AnalyticPacketError):
        encode_analytic_planar_boolean_batch_request_a0_packet(
            replace(request, jobs=(replace(request.jobs[0], job_id=True),))
        )
    with pytest.raises(AnalyticPacketError):
        encode_analytic_planar_boolean_batch_request_a0_packet(
            replace(
                request,
                relationship_queries=(replace(request.relationship_queries[0], right_job_id=7),),
            )
        )
    disk = request.jobs[0].stages[0].operands[0]
    assert isinstance(disk, DiskOperand)
    with pytest.raises(AnalyticPacketError):
        encode_analytic_planar_boolean_batch_request_a0_packet(
            replace(
                request,
                jobs=(
                    replace(
                        request.jobs[0],
                        stages=(
                            replace(
                                request.jobs[0].stages[0],
                                operands=(replace(disk, radius_nm=1_000_000_000_001),),
                            ),
                        ),
                    ),
                ),
            )
        )


def test_result_decoder_matches_cpp_canonical_mixed_and_standalone_vectors() -> None:
    manifest, vectors = _corpus()
    digests = manifest["job_digests"]
    mixed = decode_analytic_planar_boolean_batch_result_a0_packet(vectors["result.canonical-mixed"])
    assert len(mixed.job_results) == 2
    assert isinstance(mixed.job_results[0], SuccessfulJobResult)
    assert isinstance(mixed.job_results[1], FailedJobResult)
    assert mixed.job_results[0].digest_sha256 == digests["success_in_mixed_batch"]
    assert mixed.job_results[1].digest_sha256 == digests["failure_standalone"]
    assert mixed.job_results[0].vertices[0].point == PointNm(x=-10, y=0)
    assert mixed.relationship_results[0].query_id == (1 << 64) - 1

    standalone = decode_analytic_planar_boolean_batch_result_a0_packet(vectors["result.success-standalone"])
    assert standalone.job_results[0].digest_sha256 == digests["success_standalone"]
    sliced = b"prefix!" + vectors["result.success-standalone"]
    assert decode_analytic_planar_boolean_batch_result_a0_packet(memoryview(sliced)[7:]) == standalone


def test_result_decoder_rejects_header_directory_reserved_padding_and_canonical_mutations() -> None:
    packet = _corpus()[1]["result.canonical-mixed"]
    mutations: list[bytearray] = []
    for offset in (0, 12, 44, 56):
        value = bytearray(packet)
        value[offset] ^= 1
        mutations.append(value)
    wrong_kind = bytearray(packet)
    struct.pack_into("<H", wrong_kind, 64, 102)
    mutations.append(wrong_kind)
    job_offset = struct.unpack_from("<Q", packet, 72)[0]
    reserved = bytearray(packet)
    reserved[job_offset + 9] = 1
    mutations.append(reserved)
    vertex_offset = struct.unpack_from("<Q", packet, 64 + 2 * 32 + 8)[0]
    noncanonical = bytearray(packet)
    struct.pack_into("<Q", noncanonical, vertex_offset, 2)
    mutations.append(noncanonical)
    fragment_reference_offset = struct.unpack_from("<Q", packet, 64 + 5 * 32 + 8)[0]
    duplicate_fragment_reference = bytearray(packet)
    struct.pack_into(
        "<I",
        duplicate_fragment_reference,
        fragment_reference_offset + 4,
        struct.unpack_from("<I", packet, fragment_reference_offset)[0],
    )
    mutations.append(duplicate_fragment_reference)
    for value in mutations:
        with pytest.raises(AnalyticPacketError):
            decode_analytic_planar_boolean_batch_result_a0_packet(value)


@pytest.mark.parametrize("length", [64, 95])
def test_result_decoder_reports_truncated_directory_as_typed_error(length: int) -> None:
    with pytest.raises(AnalyticPacketError, match="outside the A0 bounds"):
        decode_analytic_planar_boolean_batch_result_a0_packet(bytes(length))


def test_compact_source_expansion_preflight_accepts_limit_and_rejects_limit_plus_one() -> None:
    authoritative_limit = 1_048_576
    assert MAX_LOGICAL_SOURCE_REFERENCE_EXPANSIONS == authoritative_limit
    use_slots = (
        (2, 0, 24, 1),
        (3, 0, 32, 1),
        (3, 0, 36, 1),
        (6, 0, 12, 1),
        (10, 0, 20, 1),
    )
    for use in use_slots:
        view, tables = _compact_expansion_tables(authoritative_limit, (use,))
        _preflight_logical_source_reference_expansions(view, tables)
        view, tables = _compact_expansion_tables(authoritative_limit + 1, (use,))
        with pytest.raises(AnalyticPacketError, match="expansion limit exceeded"):
            _preflight_logical_source_reference_expansions(view, tables)

    repeated = ((2, 0, 24, 1), (10, 0, 20, 1))
    view, tables = _compact_expansion_tables(authoritative_limit // 2, repeated)
    _preflight_logical_source_reference_expansions(view, tables)
    view, tables = _compact_expansion_tables(authoritative_limit // 2 + 1, repeated)
    with pytest.raises(AnalyticPacketError, match="expansion limit exceeded"):
        _preflight_logical_source_reference_expansions(view, tables)

    view, tables = _compact_expansion_tables(1, ((2, 0, 24, 2),))
    with pytest.raises(AnalyticPacketError, match="handle is out of range"):
        _preflight_logical_source_reference_expansions(view, tables)


def test_public_result_decoder_runs_source_expansion_preflight_before_materialization() -> None:
    authoritative_limit = 1_048_576
    uses = ((2, 0, 24, 1),)
    with pytest.raises(AnalyticPacketError) as exact:
        decode_analytic_planar_boolean_batch_result_a0_packet(_compact_expansion_packet(authoritative_limit, uses))
    assert "expansion limit" not in str(exact.value)

    with pytest.raises(AnalyticPacketError, match="expansion limit exceeded"):
        decode_analytic_planar_boolean_batch_result_a0_packet(_compact_expansion_packet(authoritative_limit + 1, uses))
    with pytest.raises(AnalyticPacketError, match="handle is out of range"):
        decode_analytic_planar_boolean_batch_result_a0_packet(_compact_expansion_packet(1, ((2, 0, 24, 2),)))
