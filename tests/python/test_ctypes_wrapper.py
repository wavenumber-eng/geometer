from __future__ import annotations

from pathlib import Path

import geometer


ROOT = Path(__file__).resolve().parents[2]
SOT23_STEP = ROOT / "tests" / "fixtures" / "step" / "embedded_models" / "SOT-23.STEP"


def test_version_reports_native_abi() -> None:
    version = geometer.version()

    assert version.string
    assert version.major >= 1
    assert version.abi >= 1


def test_project_step_hlr_returns_projection_result() -> None:
    result = geometer.project_step_hlr(
        SOT23_STEP,
        views=[geometer.ProjectionView.top()],
        model_transform=[
            [1.0, 0.0, 0.0, 1.0],
            [0.0, 1.0, 0.0, 2.0],
            [0.0, 0.0, 1.0, 0.0],
            [0.0, 0.0, 0.0, 1.0],
        ],
        options=geometer.HlrOptions.assembly_outline(),
    )

    assert result.schema == "geometry.projection.a0"
    assert result.units == "mm"
    assert result.source_hash

    detail = result.geometry("top", "detail")
    simple = result.geometry("top", "simple")
    assert len(detail["segments"]) + len(detail["arcs"]) > 0
    assert "segments" in simple


def test_hlr_projection_json_returns_json_text() -> None:
    text = geometer.hlr_projection_json(
        SOT23_STEP.read_bytes(),
        views=[geometer.ProjectionView.top()],
        options={"curve_mode": "polyline", "projection_algorithm": "poly"},
    )

    assert '"schema":"geometry.projection.a0"' in text


def test_step_to_glb_returns_glb_bytes() -> None:
    glb = geometer.step_to_glb(SOT23_STEP)

    assert glb[:4] == b"glTF"
