from __future__ import annotations

from dataclasses import replace
from pathlib import Path

import pytest

import geometer


ROOT = Path(__file__).resolve().parents[2]


def _analytic_source() -> geometer.AnalyticIllustrationSourceA0:
    line = geometer.IllustrationProfileLineA0(kind="line")
    ring = geometer.IllustrationProfileRingA0(
        points_mm=((0.0, 0.0), (8.0, 0.0), (8.0, 5.0), (0.0, 5.0)),
        segments=(line, line, line, line),
    )
    blue = geometer.MeshIllustrationMaterial(color=(0.2, 0.65, 0.85))
    definition = geometer.AnalyticDefinitionA0(
        id="package",
        primitives=(
            geometer.AnalyticExtrusionA0(
                kind="extrusion",
                id="body",
                regions=(geometer.IllustrationProfileRegionA0(outer=ring),),
                z_min_mm=0.0,
                z_max_mm=2.0,
                material=blue,
            ),
            geometer.AnalyticCylinderA0(
                kind="cylinder",
                id="pin",
                center_mm=(2.0, 2.0),
                radius_mm=0.7,
                z_min_mm=2.0,
                z_max_mm=3.0,
                material=geometer.MeshIllustrationMaterial(color=(0.8, 0.8, 0.8)),
            ),
            geometer.AnalyticSphereA0(
                kind="sphere",
                id="marker",
                center_mm=(6.0, 2.5, 2.4),
                radius_mm=0.6,
                material=geometer.MeshIllustrationMaterial(color=(0.9, 0.2, 0.2)),
            ),
        ),
    )
    source = geometer.AnalyticIllustrationSourceA0(
        kind="analytic",
        scene=geometer.AnalyticSceneA0(
            definitions=(definition,),
            occurrences=(
                geometer.AnalyticOccurrenceA0(id="U1", definition_id="package"),
                geometer.AnalyticOccurrenceA0(
                    id="U2",
                    definition_id="package",
                    transform=(1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1, 0, 10, 0, 0, 1),
                ),
            ),
        ),
    )
    return source


def _analytic_request() -> geometer.ModelIllustrationRequestA0:
    return geometer.ModelIllustrationRequestA0(
        schema="geometry.model_illustration.request.a0",
        source=_analytic_source(),
        view=geometer.MeshIllustrationView(direction=(0, 0, -1), up=(0, 1, 0)),
        style=geometer.MeshIllustrationStyleA0(show_hlr_detail=True),
    )


def _analytic_geometry_request() -> geometer.ModelIllustrationGeometryRequestA0:
    return geometer.ModelIllustrationGeometryRequestA0(
        schema="geometry.model_illustration_geometry.request.a0",
        source=_analytic_source(),
        view=geometer.MeshIllustrationView(direction=(0, 0, -1), up=(0, 1, 0)),
        style=geometer.MeshIllustrationStyleA0(show_hlr_detail=True),
    )


def _executable() -> Path:
    executable = geometer.executable_path()
    if not executable.is_file():
        pytest.skip("native Geometer executable is unavailable")
    return executable


def test_analytic_model_illustration_reuses_definitions_for_svg_and_geometry() -> None:
    with geometer.GeometerClient(_executable(), client_name="analytic-illustration-test") as client:
        result = client.model_illustration(_analytic_request())
        response = client.model_illustration_geometry(_analytic_geometry_request())
        geometry = response.geometry

    assert "<svg" in result.svg
    assert isinstance(result.source, geometer.AnalyticSourceSummaryA0)
    assert result.source.definitions == 1
    assert result.source.occurrences == 2
    assert result.source.primitives == 3
    assert result.source.triangles > 0
    assert result.stats.commands == len(geometry.lines) + geometry.stats.surface_draws
    assert result.stats == geometry.stats
    assert response.metadata.source == result.source
    assert response.metadata.stats == geometry.stats


def test_model_illustration_enforces_hidden_line_and_command_limits() -> None:
    request = _analytic_request()
    hidden = replace(
        request,
        linework=geometer.ModelIllustrationLineworkOptionsA0(fast=geometer.FastHlrOptionsA0(include_hidden=True)),
    )
    limited = replace(
        request,
        work_limits=geometer.ModelIllustrationWorkLimitsA0(max_drawing_commands=1),
    )
    with geometer.GeometerClient(_executable(), client_name="illustration-limit-test") as client:
        with pytest.raises(geometer.GeometerOperationError) as hidden_error:
            client.model_illustration(hidden)
        with pytest.raises(geometer.GeometerOperationError) as limit_error:
            client.model_illustration(limited)
    assert hidden_error.value.diagnostics[0].code == "geometer.contract.unsupported_linework_option"
    assert limit_error.value.diagnostics[0].code == "geometer.operation.resource_limit_exceeded"


def test_step_model_illustration_uses_one_model_attachment() -> None:
    step = (ROOT / "tests/fixtures/step/embedded_models/SOT-23.STEP").read_bytes()
    request = geometer.ModelIllustrationRequestA0(
        schema="geometry.model_illustration.request.a0",
        source=geometer.ModelAttachmentIllustrationSourceA0(kind="model", attachment="model"),
        view=geometer.MeshIllustrationView(direction=(0.4, 0.7, 1), up=(0, 1, 0)),
        style=geometer.MeshIllustrationStyleA0(show_hlr_detail=True),
    )
    with geometer.GeometerClient(_executable(), client_name="step-illustration-test") as client:
        result = client.model_illustration(request, step)
    assert "<svg" in result.svg
    assert isinstance(result.source, geometer.ModelAttachmentSourceSummaryA0)
    assert result.source.source_sha256
    assert result.source.meshes > 0
    assert result.source.triangles > 0
