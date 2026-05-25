from __future__ import annotations

from pathlib import Path

import geometer
import pytest


ROOT = Path(__file__).resolve().parents[2]
SOT23_STEP = ROOT / "tests" / "fixtures" / "step" / "embedded_models" / "SOT-23.STEP"


def test_executable_path_finds_dist_cli() -> None:
    assert geometer.executable_path().name in {"geometer", "geometer.exe"}


def test_version_reports_geometer_abi() -> None:
    version = geometer.version()

    assert version.string == "2026.5.24.2"
    assert version.major == 2026
    assert version.minor == 5
    assert version.patch == 24
    assert version.abi == 20260524


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


def test_project_model_hlr_alias_returns_projection_result() -> None:
    result = geometer.project_model_hlr(
        SOT23_STEP,
        format="step",
        views=[geometer.ProjectionView.top()],
        options=geometer.HlrOptions.assembly_outline(),
    )

    assert result.schema == "geometry.projection.a0"
    assert result.geometry("top", "detail")


def test_hlr_projection_json_returns_json_text() -> None:
    text = geometer.hlr_projection_json(
        SOT23_STEP.read_bytes(),
        views=[geometer.ProjectionView.top()],
        options={"curve_mode": "polyline", "projection_algorithm": "poly"},
    )

    assert '"schema":"geometry.projection.a0"' in text


def test_model_bounds_returns_transformed_bounds() -> None:
    base = geometer.model_bounds(SOT23_STEP.read_bytes())
    result = geometer.model_bounds(
        SOT23_STEP.read_bytes(),
        model_transform=[
            [1.0, 0.0, 0.0, 1.0],
            [0.0, 1.0, 0.0, 2.0],
            [0.0, 0.0, 1.0, 3.0],
            [0.0, 0.0, 0.0, 1.0],
        ],
    )

    assert result.schema == "geometry.model_bounds.a0"
    assert result.units == "mm"
    assert result.source_format == "step"
    assert result.source_hash
    assert result.bounds["max"][0] > result.bounds["min"][0]
    assert result.bounds["max"][1] > result.bounds["min"][1]
    assert result.bounds["max"][2] > result.bounds["min"][2]
    assert result.bounds["min"][0] - base.bounds["min"][0] == pytest.approx(1.0)
    assert result.bounds["min"][1] - base.bounds["min"][1] == pytest.approx(2.0)
    assert result.bounds["min"][2] - base.bounds["min"][2] == pytest.approx(3.0)


def test_step_to_glb_returns_glb_bytes() -> None:
    glb = geometer.step_to_glb(SOT23_STEP)

    assert glb[:4] == b"glTF"


def test_model_to_glb_returns_glb_bytes() -> None:
    glb = geometer.model_to_glb(SOT23_STEP, format="step")

    assert glb[:4] == b"glTF"


def test_planar_step_returns_step_bytes() -> None:
    step = geometer.planar_step(
        {
            "schema": "geometry.planar_step.request.a0",
            "units": "mm",
            "name": "python_planar_step",
            "bodies": [
                {
                    "id": "copper",
                    "name": "copper",
                    "color": "#B87333",
                    "thickness_mm": 0.05,
                    "regions": [
                        {
                            "outer": {
                                "points": [[0, 0], [3, 0], [3, 2], [0, 2]],
                                "segments": [
                                    {"kind": "line"},
                                    {"kind": "line"},
                                    {"kind": "line"},
                                    {"kind": "line"},
                                ],
                            }
                        }
                    ],
                }
            ],
        }
    )

    assert step.startswith(b"ISO-10303-21;")
