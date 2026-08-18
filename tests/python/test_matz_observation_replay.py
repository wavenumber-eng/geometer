from __future__ import annotations

import json
from collections.abc import Iterator
from pathlib import Path
from typing import Any

import pytest

from geometer import GeometerClient
from geometer._generated.contracts.models import (
    AnalyticPlanarBooleanBatchRequestA0,
    AnalyticPlanarBooleanBatchResultA0,
    AnalyticPlanarBooleanJob,
    AnalyticPlanarBooleanStage,
    AnnulusOperand,
    ArcDirection,
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
from geometer._paths import executable_path


ROOT = Path(__file__).resolve().parents[2]
FIXTURE_PATH = ROOT / "tests/fixtures/analytic_planar_boolean/matz_observations_a0.json"


class MatzFixtureConversionError(ValueError):
    """A vendored observation lacks data required by the governed DTO."""


def _fixture() -> dict[str, Any]:
    return json.loads(FIXTURE_PATH.read_text(encoding="utf-8"))


def _case(fixture_id: str) -> dict[str, Any]:
    return next(item for item in _fixture()["portable_cases"] if item["fixture_id"] == fixture_id)


def _point(value: list[int]) -> PointNm:
    return PointNm(x=value[0], y=value[1])


def _compact_id(base: int, operand_id: int, stride: int, ordinal: int = 1) -> int:
    return base + operand_id * stride + ordinal


def _line_ring(operand_id: int, points: list[list[int]], ring_id: int | None = None) -> PlanarRing:
    vertices = tuple(
        AuthoredVertex(
            vertex_id=_compact_id(6_000_000_000_000, operand_id, 64, index),
            point=_point(point),
        )
        for index, point in enumerate(points, 1)
    )
    segments = tuple(
        AuthoredLineSegment(
            segment_id=_compact_id(3_000_000_000_000, operand_id, 64, index),
            curve_id=_compact_id(4_000_000_000_000, operand_id, 64, index),
            kind="line",
        )
        for index in range(1, len(points) + 1)
    )
    return PlanarRing(
        ring_id=ring_id or _compact_id(1_000_000_000_000, operand_id, 32),
        vertices=vertices,
        segments=segments,
    )


def _rectangle(operand_id: int, geometry: dict[str, Any]) -> PlanarRegionOperand:
    minimum = geometry["min"]
    maximum = geometry["max"]
    points = [
        [minimum[0], minimum[1]],
        [maximum[0], minimum[1]],
        [maximum[0], maximum[1]],
        [minimum[0], maximum[1]],
    ]
    return PlanarRegionOperand(
        operand_id=operand_id,
        kind="planar_region",
        region_id=_compact_id(5_000_000_000_000, operand_id, 32),
        outer=_line_ring(operand_id, points),
        holes=(),
    )


def _swept_path(operand: dict[str, Any]) -> SweptPathOperand:
    operand_id = operand["operand_id"]
    geometry = operand["geometry"]
    segments = geometry["segments"]
    if not segments or any("start" not in item or "end" not in item for item in segments):
        raise MatzFixtureConversionError("swept path requires authoritative integer start/end points")
    topology = operand.get("source_topology", {})
    authored = topology.get("path_segments", [])
    if authored and len(authored) != len(segments):
        raise MatzFixtureConversionError("source path segment inventory does not match geometry")
    points = [segments[0]["start"], *(item["end"] for item in segments)]
    vertices = tuple(
        AuthoredVertex(
            vertex_id=_compact_id(6_000_000_000_000, operand_id, 64, index),
            point=_point(point),
        )
        for index, point in enumerate(points, 1)
    )
    projected_segments = []
    for index, item in enumerate(segments, 1):
        source = authored[index - 1] if authored else {}
        segment_id = source.get("segment_id", _compact_id(3_000_000_000_000, operand_id, 64, index))
        curve_id = source.get("curve_id", _compact_id(4_000_000_000_000, operand_id, 64, index))
        if item["kind"] == "line":
            projected_segments.append(AuthoredLineSegment(segment_id=segment_id, curve_id=curve_id, kind="line"))
        elif item["kind"] == "arc":
            sweep = item["sweep_angle"]
            projected_segments.append(
                AuthoredCircularArcSegment(
                    segment_id=segment_id,
                    curve_id=curve_id,
                    kind="circular_arc",
                    center=_point(item["center"]),
                    direction=ArcDirection.CCW if sweep > 0 else ArcDirection.CW,
                    major_arc=abs(sweep) > 180_000_000,
                )
            )
        else:
            raise MatzFixtureConversionError(f"unsupported swept path segment kind {item['kind']}")
    return SweptPathOperand(
        operand_id=operand_id,
        kind="swept_path",
        feature_id=_compact_id(5_000_000_000_000, operand_id, 32),
        centerline=PlanarPath(
            path_id=topology.get("path_id", _compact_id(2_000_000_000_000, operand_id, 32)),
            vertices=vertices,
            segments=tuple(projected_segments),
        ),
        width_nm=geometry["width"],
        cap="round",
        join="round",
    )


def _operand(value: dict[str, Any]) -> Any:
    operand_id = value["operand_id"]
    geometry = value["geometry"]
    kind = geometry["kind"]
    feature_id = _compact_id(5_000_000_000_000, operand_id, 32)
    if kind == "rectangle":
        return _rectangle(operand_id, geometry)
    if kind == "region":
        return PlanarRegionOperand(
            operand_id=operand_id,
            kind="planar_region",
            region_id=feature_id,
            outer=_line_ring(operand_id, geometry["outer"]),
            holes=tuple(
                _line_ring(
                    operand_id,
                    hole,
                    _compact_id(1_000_000_000_000, operand_id, 32, index + 2),
                )
                for index, hole in enumerate(geometry.get("holes", []))
            ),
        )
    if kind == "disk":
        return DiskOperand(
            operand_id=operand_id,
            kind="disk",
            feature_id=feature_id,
            center=_point(geometry["center"]),
            radius_nm=geometry["radius"],
        )
    if kind == "annulus":
        return AnnulusOperand(
            operand_id=operand_id,
            kind="annulus",
            feature_id=feature_id,
            center=_point(geometry["center"]),
            inner_radius_nm=geometry["inner_radius"],
            outer_radius_nm=geometry["outer_radius"],
        )
    if kind == "capsule":
        return CapsuleOperand(
            operand_id=operand_id,
            kind="capsule",
            feature_id=feature_id,
            start=_point(geometry["start"]),
            end=_point(geometry["end"]),
            width_nm=geometry["width"],
        )
    if kind == "line_arc_swept_path":
        return _swept_path(value)
    if kind == "arc_sweep":
        raise MatzFixtureConversionError(
            "arc_sweep fixture lacks authoritative integer endpoints; test adapter will not synthesize trigonometry"
        )
    raise MatzFixtureConversionError(f"unsupported MATZ fixture geometry kind {kind}")


def _job(value: dict[str, Any]) -> AnalyticPlanarBooleanJob:
    return AnalyticPlanarBooleanJob(
        job_id=value["job_id"],
        stages=tuple(
            AnalyticPlanarBooleanStage(
                stage_id=stage["stage_id"],
                operation=StageOperation(stage["operation"]),
                operands=tuple(_operand(operand) for operand in stage["operands"]),
            )
            for stage in value["stages"]
        ),
    )


def _request(value: dict[str, Any]) -> AnalyticPlanarBooleanBatchRequestA0:
    jobs = value.get("jobs", [value] if "job_id" in value else [])
    return AnalyticPlanarBooleanBatchRequestA0(
        jobs=tuple(_job(job) for job in jobs),
        relationship_queries=tuple(
            PlanarRelationshipQuery(
                query_id=query["query_id"],
                left_job_id=query["left_job_id"],
                right_job_id=query["right_job_id"],
            )
            for query in value.get("relationship_queries", [])
        ),
    )


def _mixed_request() -> AnalyticPlanarBooleanBatchRequestA0:
    mixed = _case("mixed_batch_equivalence")
    members = [_case(fixture_id) for fixture_id in mixed["batch_members"]]
    return AnalyticPlanarBooleanBatchRequestA0(
        jobs=tuple(_job(member) for member in members),
        relationship_queries=_request(mixed).relationship_queries,
    )


@pytest.fixture(scope="module")
def client() -> Iterator[GeometerClient]:
    executable = executable_path()
    if not executable.is_file():
        pytest.skip("filtered production Geometer executable is unavailable")
    with GeometerClient(executable, client_name="python-matz-production-replay") as value:
        yield value


def _summary(result: AnalyticPlanarBooleanBatchResultA0) -> dict[str, Any]:
    jobs = []
    for job in result.job_results:
        if job.status == "success":
            assert isinstance(job, SuccessfulJobResult)
        else:
            assert isinstance(job, FailedJobResult)
        jobs.append(
            {
                "job_id": job.job_id,
                "status": job.status,
                "digest": job.digest_sha256,
                "diagnostics": [diagnostic.code.value for diagnostic in job.diagnostics],
                "regions": len(job.result_regions) if isinstance(job, SuccessfulJobResult) else None,
                "rings": len(job.rings) if isinstance(job, SuccessfulJobResult) else None,
                "fragments": len(job.directed_fragments) if isinstance(job, SuccessfulJobResult) else None,
            }
        )
    relationships = [
        {
            "query_id": item.query_id,
            "status": item.status.value,
            "dimension": item.aggregate_dimension.value,
            "pairs": len(item.pairs),
        }
        for item in result.relationship_results
    ]
    return {"jobs": jobs, "relationships": relationships}


def _job_summary(
    job_id: int,
    status: str,
    digest: str,
    diagnostics: list[str],
    regions: int | None,
    rings: int | None,
    fragments: int | None,
) -> dict[str, Any]:
    return {
        "job_id": job_id,
        "status": status,
        "digest": digest,
        "diagnostics": diagnostics,
        "regions": regions,
        "rings": rings,
        "fragments": fragments,
    }


CURRENT_OBSERVED: dict[str, dict[str, Any]] = {
    "line_add_subtract_add": {
        "jobs": [
            _job_summary(1, "success", "17482b0b28b103cbb9aade86d2220f86d6bc89ffc8bcc61d0762feef0108405a", [], 1, 1, 12)
        ],
        "relationships": [],
    },
    "analytic_primitive_family": {
        "jobs": [
            _job_summary(
                3,
                "success",
                "841a653ae0b0294d4458e29d18b9997f3f392f1211cfe69341faaa9e11444114",
                [],
                4,
                7,
                27,
            )
        ],
        "relationships": [],
    },
    "nested_holes_and_islands": {
        "jobs": [
            _job_summary(4, "success", "582ae02cb64d230269253d565d4a400b7a8f5a7294aceede892df945cd5cb073", [], 2, 4, 8)
        ],
        "relationships": [],
    },
    "tangent_coincident_overlap_matrix": {
        "jobs": [
            _job_summary(
                51, "success", "4b711690d27e40282aafd1c6c144bbfcc653469204c8b57f45ed3d2550565170", [], 2, 2, 4
            ),
            _job_summary(
                52, "success", "529e52ad6a3a42d7412ee11fc66df9139adb259b286c941d5c7a1ac5840848c8", [], 1, 1, 6
            ),
            _job_summary(
                53, "success", "143c8392f388f9c59864df716de02757d2a89e693c7497629e205e9767f8e62d", [], 1, 1, 4
            ),
        ],
        "relationships": [],
    },
    "normalization_collision": {
        "jobs": [
            _job_summary(
                6,
                "failure",
                "c75fadbfb7136dcdaa9ce9ee24f5c4878630072ab04e04b609572688a6ea02d6",
                ["geometer.operation.analytic_planar_boolean.normalization_topology_collapse"],
                None,
                None,
                None,
            )
        ],
        "relationships": [],
    },
    "many_to_many_disconnected_results": {
        "jobs": [
            _job_summary(
                7,
                "success",
                "5d14d5c654f4b37840bf646471361fd85855ca21f4f292db0ca78bf56ad15cea",
                [],
                2,
                2,
                22,
            )
        ],
        "relationships": [],
    },
    "conductive_domain_contact_queries": {
        "jobs": [
            _job_summary(
                81, "success", "874a4ad9d73641a17d05c1f9fdc3011b8c2775506ca91f5efaf913807e13f66c", [], 1, 1, 4
            ),
            _job_summary(
                82, "success", "47ed141ef7f1acc7bc7b1556d12bf2e348f706354d0c2193390853eac523f38c", [], 1, 1, 4
            ),
            _job_summary(
                83, "success", "ea6931dd802a4c4b599ce1fbea2caff018e649a8eba40c4c1d00337f81eccd0d", [], 1, 1, 4
            ),
            _job_summary(
                84, "success", "36eefdc8373adb2a6ec49e7428181db7ce45ce31ac70c6c1397ce5cebe5eb02e", [], 1, 1, 4
            ),
            _job_summary(
                85, "success", "2bac3d602de31f3fbe73f771ece2b0b7251262de2847473e5c84e3ea8e751255", [], 1, 1, 2
            ),
            _job_summary(
                86, "success", "ece82243493af0070d4f85577ece4dda1ff019b85a090316db48b71de3ba5b09", [], 1, 1, 2
            ),
        ],
        "relationships": [
            {"query_id": 8001, "status": "success", "dimension": "area", "pairs": 1},
            {"query_id": 8002, "status": "success", "dimension": "curve", "pairs": 1},
            {"query_id": 8003, "status": "success", "dimension": "point", "pairs": 1},
            {"query_id": 8004, "status": "success", "dimension": "area", "pairs": 1},
            {"query_id": 8005, "status": "success", "dimension": "disjoint", "pairs": 0},
        ],
    },
    "successful_requested_empty": {
        "jobs": [
            _job_summary(
                9,
                "success",
                "7fbea0f0cd7140a1445cc7dddbab895b8e02dee29f6fc40080468b32add061f0",
                [],
                0,
                0,
                0,
            )
        ],
        "relationships": [],
    },
}
CURRENT_OBSERVED["mixed_batch_equivalence"] = {
    "jobs": [
        CURRENT_OBSERVED["line_add_subtract_add"]["jobs"][0],
        CURRENT_OBSERVED["normalization_collision"]["jobs"][0],
        CURRENT_OBSERVED["successful_requested_empty"]["jobs"][0],
    ],
    "relationships": [
        {"query_id": 10001, "status": "success", "dimension": "disjoint", "pairs": 0},
        {"query_id": 10002, "status": "skipped_dependency_failed", "dimension": "disjoint", "pairs": 0},
    ],
}


@pytest.mark.parametrize(
    "fixture_id",
    [
        "line_add_subtract_add",
        "nested_holes_and_islands",
        "tangent_coincident_overlap_matrix",
        "normalization_collision",
        "conductive_domain_contact_queries",
        "many_to_many_disconnected_results",
        "successful_requested_empty",
    ],
)
def test_matching_portable_production_replay(client: GeometerClient, fixture_id: str) -> None:
    assert (
        _summary(client.analytic_planar_boolean_batch(_request(_case(fixture_id)), timeout=10))
        == CURRENT_OBSERVED[fixture_id]
    )


def test_primitive_family_exact_conversion_matches_successful_production_result(client: GeometerClient) -> None:
    result = client.analytic_planar_boolean_batch(_request(_case("analytic_primitive_family")), timeout=10)
    assert _summary(result) == CURRENT_OBSERVED["analytic_primitive_family"]
    assert _case("analytic_primitive_family")["expected"]["status"] == "success"
    job = result.job_results[0]
    assert isinstance(job, SuccessfulJobResult)
    assert job.diagnostics == ()


def test_arbitrary_angle_arc_case_requires_authoritative_integer_endpoints() -> None:
    with pytest.raises(MatzFixtureConversionError, match="authoritative integer endpoints"):
        _request(_case("intersecting_arbitrary_angle_arcs"))


def test_mixed_batch_replay_preserves_standalone_digests(client: GeometerClient) -> None:
    result = client.analytic_planar_boolean_batch(_mixed_request(), timeout=10)
    assert _summary(result) == CURRENT_OBSERVED["mixed_batch_equivalence"]
    fixture_expected = _case("mixed_batch_equivalence")["expected"]
    assert fixture_expected["job_statuses"]["9"] == "success"
    assert result.job_results[2].status == "success"
    fixture_query = _case("mixed_batch_equivalence")["relationship_queries"][0]
    assert fixture_query["expected_status"] == "success"
    assert result.relationship_results[0].status.value == "success"
    standalone = {
        fixture_id: client.analytic_planar_boolean_batch(_request(_case(fixture_id)), timeout=10).job_results[0]
        for fixture_id in ("line_add_subtract_add", "normalization_collision", "successful_requested_empty")
    }
    assert [item.digest_sha256 for item in result.job_results] == [
        standalone["line_add_subtract_add"].digest_sha256,
        standalone["normalization_collision"].digest_sha256,
        standalone["successful_requested_empty"].digest_sha256,
    ]
