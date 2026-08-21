"""Strict binary codec for the pre-release analytic planar Boolean A0 packets."""

from __future__ import annotations

import hashlib
import struct
from dataclasses import dataclass, field as dataclass_field
from typing import Any, NoReturn

from ._generated.contracts import models as m


REQUEST_MAGIC, RESULT_MAGIC = b"GMABRQ01", b"GMABRS01"
HEADER_BYTES, DIRECTORY_ENTRY_BYTES = 64, 32
MAX_PACKET_BYTES = 256 * 1024 * 1024
U32_NONE = (1 << 32) - 1
U64_MAX = (1 << 64) - 1
I64_MIN, I64_MAX = -(1 << 63), (1 << 63) - 1
MAX_LENGTH_NM = 1_000_000_000_000
MAX_JOBS, MAX_STAGES, MAX_OPERANDS, MAX_QUERIES = 65_535, 1_048_576, 4_194_304, 1_048_576
MAX_RING_SEGMENTS = 131_072
MAX_LOGICAL_SOURCE_REFERENCE_EXPANSIONS = 1_048_576
REQUEST_TABLES = (24, 32, 24, 32, 4, 32, 24, 40, 32, 40, 48, 32, 24)
RESULT_TABLES = (48, 56, 32, 48, 32, 4, 24, 8, 8, 32, 48, 32, 32, 4)
_REQUEST_OPERAND_TYPES = (
    m.PlanarRegionOperand,
    m.DiskOperand,
    m.AnnulusOperand,
    m.CapsuleOperand,
    m.SweptPathOperand,
)


class AnalyticPacketError(ValueError):
    """The logical value or packed attachment is not valid canonical A0."""


def _fail(message: str) -> NoReturn:
    raise AnalyticPacketError(message)


def _exact_int(value: Any, label: str) -> int:
    if type(value) is not int:
        _fail(f"{label} must be an integer.")
    return value


def _u64(value: Any, label: str, *, nonzero: bool = False) -> int:
    value = _exact_int(value, label)
    if value < (1 if nonzero else 0) or value > U64_MAX:
        _fail(f"{label} must be a{' nonzero' if nonzero else ''} uint64 integer.")
    return value


def _i64(value: Any, label: str) -> int:
    value = _exact_int(value, label)
    if value < I64_MIN or value > I64_MAX:
        _fail(f"{label} must be an int64 integer.")
    return value


def _length(value: Any, label: str) -> int:
    value = _u64(value, label, nonzero=True)
    if value > MAX_LENGTH_NM:
        _fail(f"{label} exceeds {MAX_LENGTH_NM} nanometers.")
    return value


def _tuple(value: Any, label: str) -> tuple[Any, ...]:
    if type(value) is not tuple:
        _fail(f"{label} must be a tuple.")
    return value


def _record(size: int, fmt: str = "", *values: Any) -> bytearray:
    out = bytearray(size)
    if fmt:
        struct.pack_into("<" + fmt, out, 0, *values)
    return out


def _put(out: bytearray, offset: int, fmt: str, *values: Any) -> None:
    struct.pack_into("<" + fmt, out, offset, *values)


def _enum_value(value: Any, enum_type: type[Any], label: str) -> str:
    if not isinstance(value, enum_type):
        _fail(f"{label} must be {enum_type.__name__}.")
    return str(value.value)


@dataclass(slots=True)
class _RequestContext:
    tables: list[list[bytearray]] = dataclass_field(default_factory=lambda: [[] for _ in REQUEST_TABLES])
    ids: dict[str, set[int]] = dataclass_field(default_factory=dict)


def _unique_request_id(context: _RequestContext, space: str, value: Any) -> int:
    number = _u64(value, f"{space} id", nonzero=True)
    values = context.ids.setdefault(space, set())
    if number in values:
        _fail(f"Duplicate {space} id {number}.")
    values.add(number)
    return number


def _add_request_ring(context: _RequestContext, ring: Any, *, open_path: bool) -> int:
    expected = m.PlanarPath if open_path else m.PlanarRing
    if type(ring) is not expected:
        _fail("Ring/path ownership does not match its geometry kind.")
    identity = _unique_request_id(
        context,
        "path" if open_path else "ring",
        getattr(ring, "path_id" if open_path else "ring_id"),
    )
    vertices = _tuple(ring.vertices, "ring/path vertices")
    segments = _tuple(ring.segments, "ring/path segments")
    if (
        len(segments) > MAX_RING_SEGMENTS
        or (open_path and (len(segments) < 1 or len(vertices) > MAX_RING_SEGMENTS + 1))
        or (not open_path and len(segments) < 2)
    ):
        _fail("Ring/path record count is outside its governed bounds.")
    if len(vertices) != len(segments) + (1 if open_path else 0):
        _fail(
            "A path must have one more vertex than segment."
            if open_path
            else "A ring must have equal vertex and segment counts."
        )
    ring_index = len(context.tables[5])
    context.tables[5].append(bytearray(32))
    vertex_begin = len(context.tables[6])
    for vertex in vertices:
        if type(vertex) is not m.AuthoredVertex or type(vertex.point) is not m.PointNm:
            _fail("Authored vertices must use generated models.")
        context.tables[6].append(
            _record(
                24,
                "Qqq",
                _unique_request_id(context, "vertex", vertex.vertex_id),
                _i64(vertex.point.x, "vertex x"),
                _i64(vertex.point.y, "vertex y"),
            )
        )
    segment_begin = len(context.tables[7])
    for segment in segments:
        if open_path and type(segment) is m.AuthoredCircularArcByRadiusSegment:
            _fail("Endpoint/radius arcs are not supported in swept paths.")
        context.tables[7].append(_encode_request_segment(context, segment))
    row = _record(32)
    _put(row, 0, "QIIIII", identity, vertex_begin, len(vertices), segment_begin, len(segments), 1 if open_path else 0)
    context.tables[5][ring_index] = row
    return ring_index


def _encode_request_segment(context: _RequestContext, segment: Any) -> bytearray:
    row = _record(40)
    if type(segment) is m.AuthoredLineSegment:
        if segment.kind != "line":
            _fail("Unknown authored segment kind.")
        _put(
            row,
            0,
            "QQB",
            _unique_request_id(context, "segment", segment.segment_id),
            _u64(segment.curve_id, "curve id", nonzero=True),
            1,
        )
        return row
    if type(segment) is m.AuthoredCircularArcByRadiusSegment:
        if segment.kind != "circular_arc_by_radius" or type(segment.major_arc) is not bool:
            _fail("Invalid authored endpoint/radius circular arc.")
        direction = _enum_value(segment.direction, m.ArcDirection, "arc direction")
        segment_id = _unique_request_id(context, "segment", segment.segment_id)
        curve_id = _u64(segment.curve_id, "curve id", nonzero=True)
        _put(row, 0, "QQ", segment_id, curve_id)
        row[16:19] = bytes((3, 1 if direction == "ccw" else 2, int(segment.major_arc)))
        _put(row, 24, "Q", _length(segment.radius_nm, "arc radius"))
        return row
    if type(segment) is not m.AuthoredCircularArcSegment:
        _fail("Unknown authored segment kind.")
    if segment.kind != "circular_arc" or type(segment.center) is not m.PointNm or type(segment.major_arc) is not bool:
        _fail("Invalid authored circular arc.")
    direction = _enum_value(segment.direction, m.ArcDirection, "arc direction")
    _put(
        row,
        0,
        "QQBBB",
        _unique_request_id(context, "segment", segment.segment_id),
        _u64(segment.curve_id, "curve id", nonzero=True),
        2,
        1 if direction == "ccw" else 2,
        1 if segment.major_arc else 0,
    )
    _put(row, 24, "qq", _i64(segment.center.x, "arc center x"), _i64(segment.center.y, "arc center y"))
    return row


def _add_request_operand(context: _RequestContext, operand: Any) -> None:
    if type(operand) not in _REQUEST_OPERAND_TYPES:
        _fail("Unknown analytic operand geometry kind.")
    operand_id = _unique_request_id(context, "operand", operand.operand_id)
    kind, geometry_index = _add_request_operand_geometry(context, operand)
    row = _record(24)
    _put(row, 0, "QH", operand_id, kind)
    _put(row, 12, "I", geometry_index)
    context.tables[2].append(row)


def _add_request_operand_geometry(context: _RequestContext, operand: Any) -> tuple[int, int]:
    if type(operand) is m.PlanarRegionOperand:
        return _add_request_region(context, operand)
    if type(operand) is m.DiskOperand:
        return _add_request_disk(context, operand)
    if type(operand) is m.AnnulusOperand:
        return _add_request_annulus(context, operand)
    if type(operand) is m.CapsuleOperand:
        return _add_request_capsule(context, operand)
    return _add_request_swept_path(context, operand)


def _add_request_region(context: _RequestContext, operand: Any) -> tuple[int, int]:
    if operand.kind != "planar_region":
        _fail("Unknown analytic operand geometry kind.")
    region_id = _unique_request_id(context, "region", operand.region_id)
    holes = _tuple(operand.holes, "planar-region holes")
    if len(holes) > MAX_RING_SEGMENTS - 1:
        _fail("Planar-region hole count exceeds its governed bound.")
    geometry_index = len(context.tables[3])
    context.tables[3].append(bytearray(32))
    outer = _add_request_ring(context, operand.outer, open_path=False)
    hole_begin = len(context.tables[4])
    for hole in holes:
        context.tables[4].append(_record(4, "I", _add_request_ring(context, hole, open_path=False)))
    row = _record(32)
    _put(row, 0, "QIII", region_id, outer, hole_begin, len(holes))
    context.tables[3][geometry_index] = row
    return 1, geometry_index


def _add_request_disk(context: _RequestContext, operand: Any) -> tuple[int, int]:
    if operand.kind != "disk" or type(operand.center) is not m.PointNm:
        _fail("Invalid disk operand.")
    geometry_index = len(context.tables[8])
    context.tables[8].append(
        _record(
            32,
            "QqqQ",
            _unique_request_id(context, "feature", operand.feature_id),
            _i64(operand.center.x, "disk center x"),
            _i64(operand.center.y, "disk center y"),
            _length(operand.radius_nm, "disk radius"),
        )
    )
    return 2, geometry_index


def _add_request_annulus(context: _RequestContext, operand: Any) -> tuple[int, int]:
    if operand.kind != "annulus" or type(operand.center) is not m.PointNm:
        _fail("Invalid annulus operand.")
    inner = _length(operand.inner_radius_nm, "annulus inner radius")
    outer = _length(operand.outer_radius_nm, "annulus outer radius")
    if inner >= outer:
        _fail("Annulus inner radius must be smaller than its outer radius.")
    geometry_index = len(context.tables[9])
    context.tables[9].append(
        _record(
            40,
            "QqqQQ",
            _unique_request_id(context, "feature", operand.feature_id),
            _i64(operand.center.x, "annulus center x"),
            _i64(operand.center.y, "annulus center y"),
            inner,
            outer,
        )
    )
    return 3, geometry_index


def _add_request_capsule(context: _RequestContext, operand: Any) -> tuple[int, int]:
    if operand.kind != "capsule" or type(operand.start) is not m.PointNm or type(operand.end) is not m.PointNm:
        _fail("Invalid capsule operand.")
    geometry_index = len(context.tables[10])
    context.tables[10].append(
        _record(
            48,
            "QqqqqQ",
            _unique_request_id(context, "feature", operand.feature_id),
            _i64(operand.start.x, "capsule start x"),
            _i64(operand.start.y, "capsule start y"),
            _i64(operand.end.x, "capsule end x"),
            _i64(operand.end.y, "capsule end y"),
            _length(operand.width_nm, "capsule width"),
        )
    )
    return 4, geometry_index


def _add_request_swept_path(context: _RequestContext, operand: Any) -> tuple[int, int]:
    if operand.kind != "swept_path" or operand.cap != "round" or operand.join != "round":
        _fail("Swept-path cap and join must both be round in A0.")
    geometry_index = len(context.tables[11])
    feature_id = _unique_request_id(context, "feature", operand.feature_id)
    path = _add_request_ring(context, operand.centerline, open_path=True)
    row = _record(32)
    _put(row, 0, "QI", feature_id, path)
    _put(row, 16, "Q", _length(operand.width_nm, "swept width"))
    context.tables[11].append(row)
    return 5, geometry_index


def _add_request_job(context: _RequestContext, job: Any) -> None:
    if type(job) is not m.AnalyticPlanarBooleanJob:
        _fail("Jobs must use AnalyticPlanarBooleanJob.")
    job_id = _unique_request_id(context, "job", job.job_id)
    stages = _tuple(job.stages, "job stages")
    if len(context.tables[1]) + len(stages) > MAX_STAGES:
        _fail("Analytic request exceeds its stage limit.")
    stage_begin = len(context.tables[1])
    for stage in stages:
        _add_request_stage(context, stage)
    context.tables[0].append(_record(24, "QII", job_id, stage_begin, len(stages)))


def _add_request_stage(context: _RequestContext, stage: Any) -> None:
    if type(stage) is not m.AnalyticPlanarBooleanStage:
        _fail("Stages must use AnalyticPlanarBooleanStage.")
    stage_id = _unique_request_id(context, "stage", stage.stage_id)
    operation = _enum_value(stage.operation, m.StageOperation, "stage operation")
    operands = _tuple(stage.operands, "stage operands")
    if len(context.tables[2]) + len(operands) > MAX_OPERANDS:
        _fail("Analytic request exceeds its operand limit.")
    if any(type(operand) not in _REQUEST_OPERAND_TYPES for operand in operands):
        _fail("Unknown analytic operand geometry kind.")
    operand_begin = len(context.tables[2])
    for operand in sorted(operands, key=lambda value: _u64(value.operand_id, "operand id", nonzero=True)):
        _add_request_operand(context, operand)
    row = _record(32)
    _put(row, 0, "QB", stage_id, 1 if operation == "union" else 2)
    _put(row, 16, "II", operand_begin, len(operands))
    context.tables[1].append(row)


def _add_request_query(context: _RequestContext, query: Any) -> None:
    if type(query) is not m.PlanarRelationshipQuery:
        _fail("Queries must use PlanarRelationshipQuery.")
    query_id = _unique_request_id(context, "query", query.query_id)
    left = _u64(query.left_job_id, "left job id", nonzero=True)
    right = _u64(query.right_job_id, "right job id", nonzero=True)
    job_ids = context.ids.get("job", set())
    if left not in job_ids or right not in job_ids:
        _fail("Relationship query references an unknown job.")
    context.tables[12].append(_record(24, "QQQ", query_id, left, right))


def encode_analytic_planar_boolean_batch_request_a0_packet(
    request: m.AnalyticPlanarBooleanBatchRequestA0,
) -> bytes:
    """Encode a generated logical request as the unique canonical GMABRQ01 packet."""
    if type(request) is not m.AnalyticPlanarBooleanBatchRequestA0:
        _fail("request must be AnalyticPlanarBooleanBatchRequestA0.")
    jobs = _tuple(request.jobs, "request jobs")
    queries = _tuple(request.relationship_queries, "relationship queries")
    if len(jobs) > MAX_JOBS or len(queries) > MAX_QUERIES:
        _fail("Analytic request exceeds its job or relationship-query limit.")
    if any(type(job) is not m.AnalyticPlanarBooleanJob for job in jobs):
        _fail("Jobs must use AnalyticPlanarBooleanJob.")
    if any(type(query) is not m.PlanarRelationshipQuery for query in queries):
        _fail("Queries must use PlanarRelationshipQuery.")
    context = _RequestContext()
    for job in sorted(jobs, key=lambda value: _u64(value.job_id, "job id", nonzero=True)):
        _add_request_job(context, job)
    for query in sorted(queries, key=lambda value: _u64(value.query_id, "query id", nonzero=True)):
        _add_request_query(context, query)
    return _encode_tables(REQUEST_MAGIC, context.tables, REQUEST_TABLES, len(jobs), len(queries), 1)


def _align8(value: int) -> int:
    return (value + 7) & ~7


def _encode_tables(
    magic: bytes, tables: list[list[bytearray]], sizes: tuple[int, ...], jobs: int, relationships: int, first_kind: int
) -> bytes:
    cursor = _align8(HEADER_BYTES + len(tables) * DIRECTORY_ENTRY_BYTES)
    offsets: list[int] = []
    payload = 0
    for records, size in zip(tables, sizes, strict=True):
        offsets.append(cursor)
        count = len(records) * size
        payload += count
        cursor = cursor + count
        if len(offsets) != len(tables):
            cursor = _align8(cursor)
    if cursor > MAX_PACKET_BYTES:
        _fail("Packet exceeds the A0 attachment bound.")
    out = bytearray(cursor)
    out[:8] = magic
    _put(out, 8, "HHIQQIIIIQQ", 1, HEADER_BYTES, 0, cursor, 64, len(tables), jobs, relationships, 0, payload, 0)
    for index, (records, size, offset) in enumerate(zip(tables, sizes, offsets, strict=True)):
        entry = HEADER_BYTES + index * DIRECTORY_ENTRY_BYTES
        _put(out, entry, "HHIQQQ", first_kind + index, 1, size, offset, len(records) * size, len(records))
        for record_index, value in enumerate(records):
            out[offset + record_index * size : offset + (record_index + 1) * size] = value
    return bytes(out)


def _input_view(data: bytes | bytearray | memoryview) -> memoryview:
    try:
        view = memoryview(data)
    except TypeError:
        _fail("Packet must be bytes, bytearray, or memoryview.")
    if view.ndim != 1 or not view.contiguous:
        _fail("Packet buffer must be one-dimensional and contiguous.")
    try:
        return view.cast("B")
    except TypeError:
        _fail("Packet buffer must have a byte-compatible format.")


def _decode_directory(
    view: memoryview, magic: bytes, first_kind: int, sizes: tuple[int, ...]
) -> list[tuple[int, int, int]]:
    directory_end = HEADER_BYTES + len(sizes) * DIRECTORY_ENTRY_BYTES
    if len(view) < directory_end or len(view) > MAX_PACKET_BYTES:
        _fail("Packet size is outside the A0 bounds.")
    if bytes(view[:8]) != magic:
        _fail("Invalid packet header.")
    generation, header_bytes, flags, total, directory, entries, jobs, relationships, reserved, payload, reserved64 = (
        struct.unpack_from("<HHIQQIIIIQQ", view, 8)
    )
    if (generation, header_bytes, flags, total, directory, entries, reserved, reserved64) != (
        1,
        64,
        0,
        len(view),
        64,
        len(sizes),
        0,
        0,
    ):
        _fail("Invalid packet header.")
    cursor = _align8(HEADER_BYTES + len(sizes) * DIRECTORY_ENTRY_BYTES)
    payload_actual = 0
    tables: list[tuple[int, int, int]] = []
    for index, size in enumerate(sizes):
        entry = HEADER_BYTES + index * DIRECTORY_ENTRY_BYTES
        kind, version, record_bytes, offset, byte_length, count = struct.unpack_from("<HHIQQQ", view, entry)
        if (
            kind != first_kind + index
            or version != 1
            or record_bytes != size
            or byte_length != count * size
            or offset != cursor
            or offset + byte_length > len(view)
        ):
            _fail("Invalid packet table directory.")
        tables.append((offset, count, size))
        payload_actual += byte_length
        end = offset + byte_length
        cursor = end if index + 1 == len(sizes) else _align8(end)
        if any(view[end:cursor]):
            _fail("Packet alignment padding is nonzero.")
    if cursor != len(view) or payload != payload_actual:
        _fail("Packet payload accounting is invalid.")
    return tables


def _rows(view: memoryview, table: tuple[int, int, int], decoder: Any) -> list[dict[str, Any]]:
    offset, count, size = table
    return [decoder(view, offset + index * size) for index in range(count)]


def _slice(values: list[Any], begin: int, count: int, label: str) -> list[Any]:
    if begin < 0 or count < 0 or begin + count > len(values) or (count == 0 and begin != 0):
        _fail(f"Invalid {label} range.")
    return values[begin : begin + count]


def _partition(ranges: list[tuple[int, int]], total: int, label: str) -> None:
    cursor = 0
    for begin, count in ranges:
        if count == 0:
            if begin != 0:
                _fail(f"Empty {label} range must begin at zero.")
        elif begin != cursor or begin + count > total:
            _fail(f"{label} ranges are not gapless.")
        else:
            cursor += count
    if cursor != total:
        _fail(f"{label} ranges do not own their complete table.")


def _reserved(view: memoryview, table: tuple[int, int, int], index: int, ranges: tuple[tuple[int, int], ...]) -> None:
    base = table[0] + index * table[2]
    for offset, count in ranges:
        if any(view[base + offset : base + offset + count]):
            _fail("Nonzero reserved byte in result table.")


def _strict(values: list[Any], label: str) -> None:
    if any(left >= right for left, right in zip(values, values[1:], strict=False)):
        _fail(f"{label} are not strictly canonical.")


def _one_based(values: list[int], label: str) -> None:
    if any(value != index + 1 for index, value in enumerate(values)):
        _fail(f"{label} ids are not canonical one-based ordinals.")


def _parse_result(view: memoryview, tables: list[tuple[int, int, int]]) -> dict[str, list[Any]]:
    return {
        "jobs": _rows(
            view,
            tables[0],
            lambda v, o: dict(
                zip(
                    ("job_id", "status", "db", "dc", "rb", "rc", "eb", "ec"),
                    struct.unpack_from("<QB7xIIIIII8x", v, o),
                    strict=True,
                )
            ),
        ),
        "diagnostics": _rows(
            view,
            tables[1],
            lambda v, o: dict(
                zip(
                    (
                        "code",
                        "severity",
                        "version",
                        "presence",
                        "job_id",
                        "stage_id",
                        "operand_id",
                        "geometry_id",
                        "path",
                    ),
                    struct.unpack_from("<IBBHQQQQI12x", v, o),
                    strict=True,
                )
            ),
        ),
        "vertices": _rows(
            view,
            tables[2],
            lambda v, o: dict(zip(("id", "x", "y", "set", "flags"), struct.unpack_from("<QqqII", v, o), strict=True)),
        ),
        "fragments": _rows(
            view,
            tables[3],
            lambda v, o: dict(
                zip(
                    ("id", "start", "end", "kind", "direction", "major", "radius", "positive", "subtraction"),
                    struct.unpack_from("<QIIBBB5xQII8x", v, o),
                    strict=True,
                )
            ),
        ),
        "rings": _rows(
            view,
            tables[4],
            lambda v, o: dict(
                zip(
                    ("id", "begin", "count", "parent", "depth", "flags"),
                    struct.unpack_from("<QIIIII4x", v, o),
                    strict=True,
                )
            ),
        ),
        "fragment_refs": _rows(view, tables[5], lambda v, o: struct.unpack_from("<I", v, o)[0]),
        "regions": _rows(
            view,
            tables[6],
            lambda v, o: dict(zip(("id", "outer", "positive"), struct.unpack_from("<QII8x", v, o), strict=True)),
        ),
        "result_refs": _rows(view, tables[7], lambda v, o: struct.unpack_from("<Q", v, o)[0]),
        "sets": _rows(
            view, tables[8], lambda v, o: dict(zip(("begin", "count"), struct.unpack_from("<II", v, o), strict=True))
        ),
        "sources": _rows(
            view,
            tables[9],
            lambda v, o: dict(
                zip(
                    ("kind", "role", "operand", "primary", "secondary"),
                    struct.unpack_from("<HH4xQQQ", v, o),
                    strict=True,
                )
            ),
        ),
        "events": _rows(
            view,
            tables[10],
            lambda v, o: dict(
                zip(("operand", "kind", "begin", "count", "set"), struct.unpack_from("<QH2xIII24x", v, o), strict=True)
            ),
        ),
        "relationships": _rows(
            view,
            tables[11],
            lambda v, o: dict(
                zip(
                    ("query", "status", "dimension", "begin", "count"),
                    struct.unpack_from("<QBB2xII12x", v, o),
                    strict=True,
                )
            ),
        ),
        "pairs": _rows(
            view,
            tables[12],
            lambda v, o: dict(
                zip(
                    ("left", "right", "dimension", "equality", "left_contains", "right_contains"),
                    struct.unpack_from("<QQBBBB12x", v, o),
                    strict=True,
                )
            ),
        ),
        "source_indices": _rows(view, tables[13], lambda v, o: struct.unpack_from("<I", v, o)[0]),
    }


def _preflight_logical_source_reference_expansions(view: memoryview, tables: list[tuple[int, int, int]]) -> None:
    vertices, fragments, regions = tables[2], tables[3], tables[6]
    source_sets, events = tables[8], tables[10]
    total = 0

    def charge(handle: int) -> None:
        nonlocal total
        if handle == 0:
            return
        if handle > source_sets[1]:
            _fail("Logical source-set handle is out of range.")
        source_set = source_sets[0] + (handle - 1) * source_sets[2]
        count = struct.unpack_from("<I", view, source_set + 4)[0]
        if count > MAX_LOGICAL_SOURCE_REFERENCE_EXPANSIONS - total:
            _fail("Logical source-reference expansion limit exceeded.")
        total += count

    for index in range(vertices[1]):
        charge(struct.unpack_from("<I", view, vertices[0] + index * vertices[2] + 24)[0])
    for index in range(fragments[1]):
        record = fragments[0] + index * fragments[2]
        charge(struct.unpack_from("<I", view, record + 32)[0])
        charge(struct.unpack_from("<I", view, record + 36)[0])
    for index in range(regions[1]):
        charge(struct.unpack_from("<I", view, regions[0] + index * regions[2] + 12)[0])
    for index in range(events[1]):
        charge(struct.unpack_from("<I", view, events[0] + index * events[2] + 20)[0])


def _validate_result_jobs(records: dict[str, list[Any]], view: memoryview, tables: list[tuple[int, int, int]]) -> None:
    jobs, diagnostics, regions, events, relationships = (
        records[name] for name in ("jobs", "diagnostics", "regions", "events", "relationships")
    )
    if len(jobs) > MAX_JOBS or len(relationships) > MAX_QUERIES:
        _fail("Analytic result exceeds its job or relationship-result limit.")
    for index, job in enumerate(jobs):
        _reserved(view, tables[0], index, ((9, 7), (40, 8)))
        if job["job_id"] == 0 or job["status"] > 1:
            _fail("Invalid job-result identity or status.")
        owned_diagnostics = _slice(diagnostics, job["db"], job["dc"], "job diagnostics")
        _slice(regions, job["rb"], job["rc"], "job regions")
        _slice(events, job["eb"], job["ec"], "job events")
        if job["status"] == 1 and (job["rc"] != 0 or job["dc"] == 0):
            _fail("Failed job owns invalid result ranges.")
        if any(value["job_id"] != job["job_id"] for value in owned_diagnostics):
            _fail("Job diagnostic identity does not match its owner.")
        has_error = any(value["severity"] == 1 for value in owned_diagnostics)
        if (job["status"] == 1) != has_error:
            _fail("Job status does not match its diagnostic severity closure.")
    _strict([value["job_id"] for value in jobs], "job-result ids")
    _partition([(value["db"], value["dc"]) for value in jobs], len(diagnostics), "job diagnostic")
    _partition([(value["rb"], value["rc"]) for value in jobs], len(regions), "job region")
    _partition([(value["eb"], value["ec"]) for value in jobs], len(events), "job event")


def _validate_result_diagnostics(
    records: dict[str, list[Any]], view: memoryview, tables: list[tuple[int, int, int]]
) -> None:
    diagnostic_keys: list[tuple[Any, ...]] = []
    valid_codes = {65539, 65540, 65541, 65543, 65544, 65545, 65546, 65547}
    for index, value in enumerate(records["diagnostics"]):
        _reserved(view, tables[1], index, ((44, 12),))
        presence = value["presence"]
        matches = (
            (presence & 1 != 0) == (value["job_id"] != 0)
            and (presence & 2 != 0) == (value["stage_id"] != 0)
            and (presence & 4 != 0) == (value["operand_id"] != 0)
            and (presence & 8 != 0) == (value["geometry_id"] != 0)
        )
        if (
            value["version"] != 1
            or value["code"] not in valid_codes
            or value["severity"] not in (1, 2)
            or presence > 15
            or not (presence & 1)
            or not matches
            or value["path"] > 26
        ):
            _fail("Invalid diagnostic presence flags.")
        diagnostic_keys.append(
            (
                value["job_id"],
                value["severity"],
                value["code"],
                presence,
                value["stage_id"],
                value["operand_id"],
                value["geometry_id"],
                value["path"],
            )
        )
    _strict(diagnostic_keys, "diagnostic records")


def _validate_result_vertices_and_fragments(
    records: dict[str, list[Any]], view: memoryview, tables: list[tuple[int, int, int]]
) -> None:
    vertices, fragments, sets = (records[name] for name in ("vertices", "fragments", "sets"))
    for value in vertices:
        if (
            value["id"] == 0
            or value["set"] > len(sets)
            or value["flags"] > 1
            or value["flags"] != (1 if value["set"] else 0)
        ):
            _fail("Invalid result vertex.")
    _one_based([value["id"] for value in vertices], "result vertex")
    for index, value in enumerate(fragments):
        _reserved(view, tables[3], index, ((19, 5), (40, 8)))
        if (
            value["id"] == 0
            or value["start"] >= len(vertices)
            or value["end"] >= len(vertices)
            or value["start"] == value["end"]
            or value["major"] > 1
            or value["kind"] not in (1, 2)
            or value["direction"] > 2
            or value["positive"] > len(sets)
            or value["subtraction"] > len(sets)
        ):
            _fail("Invalid directed fragment.")
        if value["kind"] == 1 and (value["direction"] != 0 or value["major"] or value["radius"] != 0):
            _fail("Invalid directed fragment.")
        if value["kind"] == 2:
            _validate_result_arc(value, vertices)
    _one_based([value["id"] for value in fragments], "result fragment")


def _validate_result_arc(value: dict[str, Any], vertices: list[Any]) -> None:
    if value["direction"] not in (1, 2) or not 0 < value["radius"] <= MAX_LENGTH_NM:
        _fail("Invalid directed fragment.")
    start, end = vertices[value["start"]], vertices[value["end"]]
    chord = (end["x"] - start["x"]) ** 2 + (end["y"] - start["y"]) ** 2
    diameter = 4 * value["radius"] ** 2
    if chord > diameter or (chord == diameter and value["major"]):
        _fail("Circular-arc radius and branch are incoherent with its endpoints.")


def _validate_result_rings_and_regions(
    records: dict[str, list[Any]], view: memoryview, tables: list[tuple[int, int, int]]
) -> None:
    fragments, rings, fragment_refs, regions, sets = (
        records[name] for name in ("fragments", "rings", "fragment_refs", "regions", "sets")
    )
    used_fragment_references: set[int] = set()
    for index, value in enumerate(rings):
        _reserved(view, tables[4], index, ((28, 4),))
        refs = _slice(fragment_refs, value["begin"], value["count"], "ring fragments")
        parent = value["parent"]
        if (
            value["id"] == 0
            or (parent != U32_NONE and (parent >= index or rings[parent]["depth"] + 1 != value["depth"]))
            or (parent == U32_NONE and value["depth"] != 0)
            or value["flags"] & ~1
            or bool(value["flags"] & 1) != (value["depth"] % 2 == 1)
        ):
            _fail("Invalid result ring.")
        _validate_ring_references(refs, fragments, used_fragment_references)
    _one_based([value["id"] for value in rings], "result ring")
    _partition([(value["begin"], value["count"]) for value in rings], len(fragment_refs), "ring fragment-reference")
    if used_fragment_references != set(range(len(fragments))):
        _fail("A directed fragment is unreferenced.")
    _validate_result_regions(regions, rings, sets, view, tables)


def _validate_ring_references(refs: list[Any], fragments: list[Any], used: set[int]) -> None:
    if len(refs) < 2 or any(ref >= len(fragments) for ref in refs):
        _fail("Invalid fragment reference.")
    for ref in refs:
        if ref in used:
            _fail("A directed fragment is referenced more than once.")
        used.add(ref)
    for at, ref in enumerate(refs):
        if fragments[ref]["end"] != fragments[refs[(at + 1) % len(refs)]]["start"]:
            _fail("Result ring fragment topology is disconnected.")


def _validate_result_regions(
    regions: list[Any], rings: list[Any], sets: list[Any], view: memoryview, tables: list[tuple[int, int, int]]
) -> None:
    for index, value in enumerate(regions):
        _reserved(view, tables[6], index, ((16, 8),))
        if (
            value["id"] == 0
            or value["outer"] >= len(rings)
            or rings[value["outer"]]["depth"] % 2
            or value["positive"] == 0
            or value["positive"] > len(sets)
        ):
            _fail("Invalid result region.")
    _one_based([value["id"] for value in regions], "result region")
    outers = [value["outer"] for value in regions]
    if len(set(outers)) != len(outers):
        _fail("A result ring is owned by more than one region.")
    outer_set = set(outers)
    if any((ring["depth"] % 2 == 0) != (index in outer_set) for index, ring in enumerate(rings)):
        _fail("Even-depth result rings and result regions are not one-to-one.")


def _validate_result_sources(
    records: dict[str, list[Any]], view: memoryview, tables: list[tuple[int, int, int]]
) -> None:
    sets, sources, source_indices = (records[name] for name in ("sets", "sources", "source_indices"))
    for value in sets:
        _slice(source_indices, value["begin"], value["count"], "source indices")
        if value["count"] == 0:
            _fail("Empty source-set record.")
    _partition([(value["begin"], value["count"]) for value in sets], len(source_indices), "source-set index")
    if any(index >= len(sources) for index in source_indices):
        _fail("Invalid source index.")
    for index, value in enumerate(sources):
        _reserved(view, tables[9], index, ((4, 4),))
        if not _valid_source_reference(value):
            _fail("Invalid source reference.")
    _strict(
        [(value["kind"], value["role"], value["operand"], value["primary"], value["secondary"]) for value in sources],
        "source-reference table",
    )
    member_sequences = [_slice(source_indices, value["begin"], value["count"], "source set") for value in sets]
    if any(
        any(left >= right for left, right in zip(sequence, sequence[1:], strict=False)) for sequence in member_sequences
    ):
        _fail("Source-set members are not strictly ordered.")
    _strict([tuple(sequence) for sequence in member_sequences], "source-set table")


def _valid_source_reference(value: dict[str, Any]) -> bool:
    high, low = value["secondary"] >> 32, value["secondary"] & U32_NONE
    allowed = value["kind"] == 1 and value["secondary"] != 0 and value["role"] in (1, 2)
    if value["kind"] == 2:
        allowed = (
            (value["role"] in (16, 17, 32, 33, 34, 35) and value["secondary"] == 0)
            or (value["role"] in (48, 49, 50, 51, 54) and high != 0 and low == 0)
            or (value["role"] == 52 and high != 0 and low != 0)
            or (value["role"] == 53 and high == 1 and low == 0)
        )
    if value["kind"] == 3:
        allowed = value["role"] == 0 and value["secondary"] == 0
    return value["operand"] != 0 and value["primary"] != 0 and allowed


def _validate_result_events_and_relationships(
    records: dict[str, list[Any]], view: memoryview, tables: list[tuple[int, int, int]]
) -> None:
    events, result_refs, rings, regions, sets = (
        records[name] for name in ("events", "result_refs", "rings", "regions", "sets")
    )
    for index, value in enumerate(events):
        _reserved(view, tables[10], index, ((10, 2), (24, 24)))
        refs = _slice(result_refs, value["begin"], value["count"], "event references")
        if value["operand"] == 0 or value["kind"] not in range(1, 8) or value["set"] > len(sets):
            _fail("Invalid operand event.")
        _strict(refs, "operand-event references")
        for ref in refs:
            kind, target = ref >> 32, ref & U32_NONE
            if (kind == 1 and target >= len(rings)) or (kind == 2 and target >= len(regions)) or kind not in (1, 2):
                _fail("Invalid operand-event result reference.")
    _partition(
        [(value["begin"], value["count"]) for value in events], len(result_refs), "operand-event result-reference"
    )
    _validate_result_relationships(records, view, tables)


def _validate_result_relationships(
    records: dict[str, list[Any]], view: memoryview, tables: list[tuple[int, int, int]]
) -> None:
    relationships, pairs, regions = (records[name] for name in ("relationships", "pairs", "regions"))
    for index, value in enumerate(relationships):
        _reserved(view, tables[11], index, ((10, 2), (20, 12)))
        owned = _slice(pairs, value["begin"], value["count"], "relationship pairs")
        if (
            value["query"] == 0
            or value["status"] > 1
            or value["dimension"] > 3
            or (value["status"] == 1 and (value["dimension"] != 0 or value["begin"] != 0 or value["count"] != 0))
        ):
            _fail("Invalid relationship result.")
        keys = [
            (
                item["left"],
                item["right"],
                item["dimension"],
                item["equality"],
                item["left_contains"],
                item["right_contains"],
            )
            for item in owned
        ]
        _strict(keys, "relationship pairs")
        if value["status"] == 0 and value["dimension"] != max((item["dimension"] for item in owned), default=0):
            _fail("Relationship aggregate dimension does not match its pairs.")
    _strict([value["query"] for value in relationships], "relationship query ids")
    _partition([(value["begin"], value["count"]) for value in relationships], len(pairs), "relationship pair")
    for index, value in enumerate(pairs):
        _reserved(view, tables[12], index, ((20, 12),))
        _validate_relationship_pair(value, len(regions))


def _validate_relationship_pair(value: dict[str, Any], region_count: int) -> None:
    if (
        value["dimension"] > 3
        or not 1 <= value["left"] <= region_count
        or not 1 <= value["right"] <= region_count
        or any(value[key] > 1 for key in ("equality", "left_contains", "right_contains"))
    ):
        _fail("Invalid relationship pair.")
    has_containment = value["equality"] or value["left_contains"] or value["right_contains"]
    if (has_containment and value["dimension"] != 3) or (
        value["equality"] and not (value["left_contains"] and value["right_contains"])
    ):
        _fail("Relationship pair flags are inconsistent with its dimension.")


def _validate_result_content_usage(records: dict[str, list[Any]]) -> None:
    vertices, fragments, regions, events, sets, source_indices, sources = (
        records[name] for name in ("vertices", "fragments", "regions", "events", "sets", "source_indices", "sources")
    )
    used_sets = (
        {value["set"] - 1 for value in vertices if value["set"]}
        | {value[key] - 1 for value in fragments for key in ("positive", "subtraction") if value[key]}
        | {value["positive"] - 1 for value in regions if value["positive"]}
        | {value["set"] - 1 for value in events if value["set"]}
    )
    if used_sets != set(range(len(sets))) or set(source_indices) != set(range(len(sources))):
        _fail("Result packet contains unused source content.")


def _build_result_owners(records: dict[str, list[Any]]) -> dict[str, list[int]]:
    jobs, vertices, fragments, rings, fragment_refs, regions, events = (
        records[name] for name in ("jobs", "vertices", "fragments", "rings", "fragment_refs", "regions", "events")
    )
    owners = {name: [-1] * len(records[name]) for name in ("vertices", "fragments", "rings", "regions", "events")}
    for owner, job in enumerate(jobs):
        for index in range(job["rb"], job["rb"] + job["rc"]):
            owners["regions"][index] = owner
        for index in range(job["eb"], job["eb"] + job["ec"]):
            owners["events"][index] = owner
    outer_regions = [-1] * len(rings)
    for index, region in enumerate(regions):
        outer_regions[region["outer"]] = index
    for index, ring in enumerate(rings):
        owner = (
            owners["regions"][outer_regions[index]] if ring["parent"] == U32_NONE else owners["rings"][ring["parent"]]
        )
        if owner < 0:
            _fail("Result ring has no owning job.")
        owners["rings"][index] = owner
        for ref in _slice(fragment_refs, ring["begin"], ring["count"], "ring fragments"):
            if owners["fragments"][ref] not in (-1, owner):
                _fail("A mutable result record is shared by jobs.")
            owners["fragments"][ref] = owner
    for index, region in enumerate(regions):
        if owners["rings"][region["outer"]] != owners["regions"][index]:
            _fail("Result region and its outer ring belong to different jobs.")
    for index, fragment in enumerate(fragments):
        owner = owners["fragments"][index]
        for vertex in (fragment["start"], fragment["end"]):
            if owners["vertices"][vertex] not in (-1, owner):
                _fail("A mutable result record is shared by jobs.")
            owners["vertices"][vertex] = owner
    if any(-1 in values for values in owners.values()):
        _fail("Result packet contains an unowned result record.")
    _validate_owner_spans_and_event_refs(records, owners)
    return owners


def _validate_owner_spans_and_event_refs(records: dict[str, list[Any]], owners: dict[str, list[int]]) -> None:
    jobs, vertices, events, result_refs = (records[name] for name in ("jobs", "vertices", "events", "result_refs"))
    for owner in range(len(jobs)):
        points = [
            (value["x"], value["y"]) for index, value in enumerate(vertices) if owners["vertices"][index] == owner
        ]
        if points and (
            max(x for x, _ in points) - min(x for x, _ in points) > MAX_LENGTH_NM
            or max(y for _, y in points) - min(y for _, y in points) > MAX_LENGTH_NM
        ):
            _fail("A job result exceeds the governed coordinate span.")
    for index, event in enumerate(events):
        owner = owners["events"][index]
        for ref in _slice(result_refs, event["begin"], event["count"], "event references"):
            kind, target = ref >> 32, ref & U32_NONE
            if owners["rings" if kind == 1 else "regions"][target] != owner:
                _fail("Operand event references a different job closure.")


def _validate_result(
    records: dict[str, list[Any]], view: memoryview, tables: list[tuple[int, int, int]]
) -> tuple[list[dict[str, list[int]]], dict[str, list[int]]]:
    _validate_result_jobs(records, view, tables)
    _validate_result_diagnostics(records, view, tables)
    _validate_result_vertices_and_fragments(records, view, tables)
    _validate_result_rings_and_regions(records, view, tables)
    _validate_result_sources(records, view, tables)
    _validate_result_events_and_relationships(records, view, tables)
    _validate_result_content_usage(records)
    owners = _build_result_owners(records)
    _validate_canonical(records, owners)
    selections = _build_selections(records, owners)
    return selections, owners


def _least_rotation(values: list[int]) -> int:
    if not values:
        return 0
    return min(range(len(values)), key=lambda index: tuple(values[index:] + values[:index]))


def _validate_canonical(records: dict[str, list[Any]], owners: dict[str, list[int]]) -> None:
    jobs, vertices, fragments, rings, refs, regions, events = (
        records[name] for name in ("jobs", "vertices", "fragments", "rings", "fragment_refs", "regions", "events")
    )
    incidents: list[list[tuple[Any, ...]]] = [[] for _ in vertices]
    for fragment in fragments:
        start, end = vertices[fragment["start"]], vertices[fragment["end"]]
        incidents[fragment["start"]].append(
            (0, end["x"], end["y"], fragment["kind"], fragment["direction"], fragment["major"], fragment["radius"])
        )
        incidents[fragment["end"]].append(
            (1, start["x"], start["y"], fragment["kind"], fragment["direction"], fragment["major"], fragment["radius"])
        )
    vertex_keys = [
        (jobs[owners["vertices"][i]]["job_id"], value["x"], value["y"], tuple(sorted(incidents[i])), value["set"])
        for i, value in enumerate(vertices)
    ]
    _strict(vertex_keys, "result vertices")
    _strict(
        [
            (
                jobs[owners["fragments"][i]]["job_id"],
                value["start"],
                value["end"],
                value["kind"],
                value["direction"],
                value["major"],
                value["radius"],
                value["positive"],
                value["subtraction"],
            )
            for i, value in enumerate(fragments)
        ],
        "directed fragments",
    )
    ring_keys = []
    for index, ring in enumerate(rings):
        sequence = _slice(refs, ring["begin"], ring["count"], "ring references")
        if _least_rotation(sequence) != 0:
            _fail("Result ring does not use its least canonical fragment rotation.")
        ring_keys.append((jobs[owners["rings"][index]]["job_id"], ring["depth"], tuple(sequence), ring["parent"]))
    _strict(ring_keys, "result rings")
    _strict(
        [(jobs[owners["regions"][i]]["job_id"], value["outer"], value["positive"]) for i, value in enumerate(regions)],
        "result regions",
    )
    for job in jobs:
        keys = []
        for event in _slice(events, job["eb"], job["ec"], "job events"):
            keys.append(
                (
                    event["operand"],
                    event["kind"],
                    tuple(_slice(records["result_refs"], event["begin"], event["count"], "event references")),
                    event["set"],
                )
            )
        _strict(keys, "operand events")


def _build_selections(records: dict[str, list[Any]], owners: dict[str, list[int]]) -> list[dict[str, list[int]]]:
    selections = [
        {name: [] for name in ("vertices", "fragments", "rings", "regions", "events", "sets", "sources")}
        for _ in records["jobs"]
    ]
    for name in ("vertices", "fragments", "rings", "regions", "events"):
        for index, owner in enumerate(owners[name]):
            selections[owner][name].append(index)
    sets_by_job = [set() for _ in records["jobs"]]
    for name, fields in (
        ("vertices", ("set",)),
        ("fragments", ("positive", "subtraction")),
        ("regions", ("positive",)),
        ("events", ("set",)),
    ):
        for index, value in enumerate(records[name]):
            owner = owners[name][index]
            for field in fields:
                if value[field]:
                    sets_by_job[owner].add(value[field] - 1)
    for index, selection in enumerate(selections):
        selection["sets"] = sorted(sets_by_job[index])
        source_set: set[int] = set()
        for set_index in selection["sets"]:
            value = records["sets"][set_index]
            source_set.update(_slice(records["source_indices"], value["begin"], value["count"], "source set"))
        selection["sources"] = sorted(source_set)
    return selections


def _clone(value: dict[str, Any], **updates: Any) -> dict[str, Any]:
    return value | updates


def _encode_standalone(records: dict[str, list[Any]], job_index: int, selection: dict[str, list[int]]) -> bytes:
    maps = {
        name: {old: new for new, old in enumerate(selection[name])}
        for name in ("vertices", "fragments", "rings", "regions", "sets", "sources")
    }

    def remap_handle(value: int) -> int:
        return 0 if value == 0 else maps["sets"][value - 1] + 1

    job = records["jobs"][job_index]
    output: dict[str, list[Any]] = {name: [] for name in records}
    output["diagnostics"] = list(_slice(records["diagnostics"], job["db"], job["dc"], "diagnostics"))
    output["sources"] = [records["sources"][index] for index in selection["sources"]]
    for index in selection["vertices"]:
        value = records["vertices"][index]
        output["vertices"].append(_clone(value, id=len(output["vertices"]) + 1, set=remap_handle(value["set"])))
    for index in selection["fragments"]:
        value = records["fragments"][index]
        output["fragments"].append(
            _clone(
                value,
                id=len(output["fragments"]) + 1,
                start=maps["vertices"][value["start"]],
                end=maps["vertices"][value["end"]],
                positive=remap_handle(value["positive"]),
                subtraction=remap_handle(value["subtraction"]),
            )
        )
    for index in selection["rings"]:
        value = records["rings"][index]
        begin = len(output["fragment_refs"])
        output["fragment_refs"].extend(
            maps["fragments"][ref]
            for ref in _slice(records["fragment_refs"], value["begin"], value["count"], "ring references")
        )
        output["rings"].append(
            _clone(
                value,
                id=len(output["rings"]) + 1,
                begin=begin,
                parent=U32_NONE if value["parent"] == U32_NONE else maps["rings"][value["parent"]],
            )
        )
    for index in selection["regions"]:
        value = records["regions"][index]
        output["regions"].append(
            _clone(
                value,
                id=len(output["regions"]) + 1,
                outer=maps["rings"][value["outer"]],
                positive=remap_handle(value["positive"]),
            )
        )
    for index in selection["sets"]:
        value = records["sets"][index]
        begin = len(output["source_indices"])
        output["source_indices"].extend(
            maps["sources"][ref]
            for ref in _slice(records["source_indices"], value["begin"], value["count"], "source indices")
        )
        output["sets"].append({"begin": begin, "count": value["count"]})
    for index in selection["events"]:
        value = records["events"][index]
        begin = 0 if value["count"] == 0 else len(output["result_refs"])
        for ref in _slice(records["result_refs"], value["begin"], value["count"], "event references"):
            kind, target = ref >> 32, ref & U32_NONE
            output["result_refs"].append((kind << 32) | maps["rings" if kind == 1 else "regions"][target])
        output["events"].append(_clone(value, begin=begin, set=remap_handle(value["set"])))
    output["jobs"] = [_clone(job, db=0, rb=0, eb=0)]
    return _encode_result_records(output)


def _encode_result_records(records: dict[str, list[Any]]) -> bytes:
    tables: list[list[bytearray]] = [[] for _ in RESULT_TABLES]

    def add(index: int, fmt: str, *values: Any) -> None:
        tables[index].append(_record(RESULT_TABLES[index], fmt, *values))

    for v in records["jobs"]:
        add(0, "QB7xIIIIII8x", v["job_id"], v["status"], v["db"], v["dc"], v["rb"], v["rc"], v["eb"], v["ec"])
    for v in records["diagnostics"]:
        add(
            1,
            "IBBHQQQQI12x",
            v["code"],
            v["severity"],
            1,
            v["presence"],
            v["job_id"],
            v["stage_id"],
            v["operand_id"],
            v["geometry_id"],
            v["path"],
        )
    for v in records["vertices"]:
        add(2, "QqqII", v["id"], v["x"], v["y"], v["set"], v["flags"])
    for v in records["fragments"]:
        add(
            3,
            "QIIBBB5xQII8x",
            v["id"],
            v["start"],
            v["end"],
            v["kind"],
            v["direction"],
            v["major"],
            v["radius"],
            v["positive"],
            v["subtraction"],
        )
    for v in records["rings"]:
        add(4, "QIIIII4x", v["id"], v["begin"], v["count"], v["parent"], v["depth"], v["flags"])
    for v in records["fragment_refs"]:
        add(5, "I", v)
    for v in records["regions"]:
        add(6, "QII8x", v["id"], v["outer"], v["positive"])
    for v in records["result_refs"]:
        add(7, "Q", v)
    for v in records["sets"]:
        add(8, "II", v["begin"], v["count"])
    for v in records["sources"]:
        add(9, "HH4xQQQ", v["kind"], v["role"], v["operand"], v["primary"], v["secondary"])
    for v in records["events"]:
        add(10, "QH2xIII24x", v["operand"], v["kind"], v["begin"], v["count"], v["set"])
    for v in records["relationships"]:
        add(11, "QBB2xII12x", v["query"], v["status"], v["dimension"], v["begin"], v["count"])
    for v in records["pairs"]:
        add(
            12,
            "QQBBBB12x",
            v["left"],
            v["right"],
            v["dimension"],
            v["equality"],
            v["left_contains"],
            v["right_contains"],
        )
    for v in records["source_indices"]:
        add(13, "I", v)
    return _encode_tables(RESULT_MAGIC, tables, RESULT_TABLES, len(records["jobs"]), len(records["relationships"]), 101)


_DIAGNOSTIC_CODES = {
    65539: m.JobDiagnosticCode.INVALID_TOPOLOGY,
    65540: m.JobDiagnosticCode.INVALID_ARC,
    65541: m.JobDiagnosticCode.UNSUPPORTED_GEOMETRY,
    65543: m.JobDiagnosticCode.NORMALIZATION_ERROR_EXCEEDED,
    65544: m.JobDiagnosticCode.NORMALIZATION_TOPOLOGY_COLLAPSE,
    65545: m.JobDiagnosticCode.NONANALYTIC_RESULT,
    65546: m.JobDiagnosticCode.SOLVER_FAILED,
    65547: m.JobDiagnosticCode.RESOURCE_LIMIT_EXCEEDED,
}
_PATHS: tuple[m.JobDiagnosticPath | None, ...] = (
    None,
    m.JobDiagnosticPath.REQUEST_JOBS,
    m.JobDiagnosticPath.JOB_ID,
    m.JobDiagnosticPath.JOB_STAGES,
    m.JobDiagnosticPath.STAGE_ID,
    m.JobDiagnosticPath.STAGE_OPERATION,
    m.JobDiagnosticPath.STAGE_OPERANDS,
    m.JobDiagnosticPath.OPERAND_ID,
    m.JobDiagnosticPath.OPERAND_GEOMETRY,
    m.JobDiagnosticPath.REGION_OUTER,
    m.JobDiagnosticPath.REGION_HOLES,
    m.JobDiagnosticPath.RING_VERTICES,
    m.JobDiagnosticPath.RING_SEGMENTS,
    m.JobDiagnosticPath.PATH_VERTICES,
    m.JobDiagnosticPath.PATH_SEGMENTS,
    m.JobDiagnosticPath.SEGMENT_CURVE,
    m.JobDiagnosticPath.DISK_RADIUS,
    m.JobDiagnosticPath.ANNULUS_INNER_RADIUS,
    m.JobDiagnosticPath.ANNULUS_OUTER_RADIUS,
    m.JobDiagnosticPath.CAPSULE_START,
    m.JobDiagnosticPath.CAPSULE_END,
    m.JobDiagnosticPath.CAPSULE_WIDTH,
    m.JobDiagnosticPath.SWEPT_PATH_CENTERLINE,
    m.JobDiagnosticPath.SWEPT_PATH_WIDTH,
    m.JobDiagnosticPath.RELATIONSHIP_QUERIES,
    m.JobDiagnosticPath.RELATIONSHIP_LEFT_JOB_ID,
    m.JobDiagnosticPath.RELATIONSHIP_RIGHT_JOB_ID,
)
_SOURCE_ROLES = {
    0: m.SourceRole.NONE,
    1: m.SourceRole.AUTHORED_LINE,
    2: m.SourceRole.AUTHORED_CIRCULAR_ARC,
    16: m.SourceRole.PRIMITIVE_OUTER_CIRCLE,
    17: m.SourceRole.PRIMITIVE_INNER_CIRCLE,
    32: m.SourceRole.CAPSULE_LEFT_LINE,
    33: m.SourceRole.CAPSULE_END_CAP,
    34: m.SourceRole.CAPSULE_RIGHT_LINE,
    35: m.SourceRole.CAPSULE_START_CAP,
    48: m.SourceRole.SWEPT_LEFT_OFFSET_LINE,
    49: m.SourceRole.SWEPT_LEFT_OFFSET_ARC,
    50: m.SourceRole.SWEPT_RIGHT_OFFSET_LINE,
    51: m.SourceRole.SWEPT_RIGHT_OFFSET_ARC,
    52: m.SourceRole.SWEPT_ROUND_JOIN,
    53: m.SourceRole.SWEPT_START_CAP,
    54: m.SourceRole.SWEPT_END_CAP,
}
_OUTCOMES = tuple(m.OperandOutcomeKind)
_DIMENSIONS = tuple(m.IntersectionDimension)


def _project_source_set(records: dict[str, list[Any]], handle: int) -> m.SourceSet:
    if handle == 0:
        return m.SourceSet(sources=())
    value = records["sets"][handle - 1]
    projected = []
    for index in _slice(records["source_indices"], value["begin"], value["count"], "source-set indices"):
        source = records["sources"][index]
        kinds = {
            1: m.SourceKind.AUTHORED_SEGMENT_CURVE,
            2: m.SourceKind.COMPACT_FEATURE_ROLE,
            3: m.SourceKind.SUBTRACTIVE_OPERAND_EFFECT,
        }
        projected.append(
            m.SourceReference(
                kind=kinds[source["kind"]],
                role=_SOURCE_ROLES[source["role"]],
                operand_id=source["operand"],
                primary_id=source["primary"],
                secondary_id=source["secondary"],
            )
        )
    return m.SourceSet(sources=tuple(projected))


def _project_diagnostic(value: dict[str, Any]) -> m.JobDiagnostic:
    return m.JobDiagnostic(
        code=_DIAGNOSTIC_CODES[value["code"]],
        severity=m.DiagnosticSeverity.ERROR if value["severity"] == 1 else m.DiagnosticSeverity.WARNING,
        job_id=value["job_id"],
        stage_id=value["stage_id"] if value["presence"] & 2 else None,
        operand_id=value["operand_id"] if value["presence"] & 4 else None,
        geometry_id=value["geometry_id"] if value["presence"] & 8 else None,
        path_identity=_PATHS[value["path"]],
    )


def _project_job(
    records: dict[str, list[Any]], index: int, selection: dict[str, list[int]], digest: str
) -> m.AnalyticPlanarBooleanJobResult:
    job = records["jobs"][index]
    diagnostics = tuple(
        _project_diagnostic(value) for value in _slice(records["diagnostics"], job["db"], job["dc"], "job diagnostics")
    )
    if job["status"] == 1:
        return m.FailedJobResult(job_id=job["job_id"], status="failure", diagnostics=diagnostics, digest_sha256=digest)
    vertices = tuple(
        m.ResultVertex(
            vertex_id=value["id"],
            point=m.PointNm(x=value["x"], y=value["y"]),
            intersection_sources=_project_source_set(records, value["set"]),
        )
        for value in (records["vertices"][i] for i in selection["vertices"])
    )
    fragments = []
    for value in (records["fragments"][i] for i in selection["fragments"]):
        common = dict(
            fragment_id=value["id"],
            start_vertex_id=records["vertices"][value["start"]]["id"],
            end_vertex_id=records["vertices"][value["end"]]["id"],
            coincident_positive_sources=_project_source_set(records, value["positive"]),
            surviving_subtraction_sources=_project_source_set(records, value["subtraction"]),
        )
        if value["kind"] == 1:
            fragments.append(m.ResultLineFragment(kind="line", **common))
        else:
            fragments.append(
                m.ResultCircularArcFragment(
                    kind="circular_arc",
                    radius_nm=value["radius"],
                    direction=m.ArcDirection.CCW if value["direction"] == 1 else m.ArcDirection.CW,
                    major_arc=bool(value["major"]),
                    **common,
                )
            )
    rings = []
    for value in (records["rings"][i] for i in selection["rings"]):
        fragment_ids = tuple(
            records["fragments"][ref]["id"]
            for ref in _slice(records["fragment_refs"], value["begin"], value["count"], "ring fragments")
        )
        rings.append(
            m.ResultRing(
                ring_id=value["id"],
                fragment_ids=fragment_ids,
                parent_ring_id=None if value["parent"] == U32_NONE else records["rings"][value["parent"]]["id"],
                depth=value["depth"],
                hole=bool(value["flags"] & 1),
            )
        )
    regions = tuple(
        m.ResultRegion(
            result_region_id=value["id"],
            outer_ring_id=records["rings"][value["outer"]]["id"],
            positive_contributors=_project_source_set(records, value["positive"]),
        )
        for value in (records["regions"][i] for i in selection["regions"])
    )
    outcomes = []
    for value in (records["events"][i] for i in selection["events"]):
        ring_ids, region_ids = [], []
        for ref in _slice(records["result_refs"], value["begin"], value["count"], "outcome references"):
            kind, target = ref >> 32, ref & U32_NONE
            (ring_ids if kind == 1 else region_ids).append(records["rings" if kind == 1 else "regions"][target]["id"])
        outcomes.append(
            m.OperandOutcomeEvent(
                operand_id=value["operand"],
                kind=_OUTCOMES[value["kind"] - 1],
                result_ring_ids=tuple(ring_ids),
                result_region_ids=tuple(region_ids),
                sources=_project_source_set(records, value["set"]),
            )
        )
    return m.SuccessfulJobResult(
        job_id=job["job_id"],
        status="success",
        diagnostics=diagnostics,
        vertices=vertices,
        directed_fragments=tuple(fragments),
        rings=tuple(rings),
        result_regions=regions,
        operand_outcomes=tuple(outcomes),
        digest_sha256=digest,
    )


def decode_analytic_planar_boolean_batch_result_a0_packet(
    data: bytes | bytearray | memoryview,
) -> m.AnalyticPlanarBooleanBatchResultA0:
    """Strictly decode canonical GMABRS01 into generated logical result DTOs."""
    view = _input_view(data)
    tables = _decode_directory(view, RESULT_MAGIC, 101, RESULT_TABLES)
    _preflight_logical_source_reference_expansions(view, tables)
    records = _parse_result(view, tables)
    selections, _ = _validate_result(records, view, tables)
    if memoryview(_encode_result_records(records)) != view:
        _fail("Result packet is not canonically encoded.")
    digests = [
        hashlib.sha256(_encode_standalone(records, index, selection)).hexdigest()
        for index, selection in enumerate(selections)
    ]
    jobs = tuple(
        _project_job(records, index, selections[index], digests[index]) for index in range(len(records["jobs"]))
    )
    relationships = []
    for value in records["relationships"]:
        projected_pairs = tuple(
            m.RelationshipRegionPair(
                left_result_region_id=pair["left"],
                right_result_region_id=pair["right"],
                dimension=_DIMENSIONS[pair["dimension"]],
                equality=bool(pair["equality"]),
                left_contains_right=bool(pair["left_contains"]),
                right_contains_left=bool(pair["right_contains"]),
            )
            for pair in _slice(records["pairs"], value["begin"], value["count"], "relationship pairs")
        )
        relationships.append(
            m.PlanarRelationshipResult(
                query_id=value["query"],
                status=m.RelationshipStatus.SUCCESS
                if value["status"] == 0
                else m.RelationshipStatus.SKIPPED_DEPENDENCY_FAILED,
                aggregate_dimension=_DIMENSIONS[value["dimension"]],
                pairs=projected_pairs,
            )
        )
    header_jobs, header_relationships = struct.unpack_from("<II", view, 36)
    if header_jobs != len(jobs) or header_relationships != len(relationships):
        _fail("Result header counts do not match their tables.")
    return m.AnalyticPlanarBooleanBatchResultA0(job_results=jobs, relationship_results=tuple(relationships))
