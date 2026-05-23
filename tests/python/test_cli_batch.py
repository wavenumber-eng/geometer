from __future__ import annotations

import json
import subprocess
import sys
from pathlib import Path


ROOT = Path(__file__).resolve().parents[2]
SOT23_STEP = ROOT / "tests" / "fixtures" / "step" / "embedded_models" / "SOT-23.STEP"


def _geometer_exe() -> Path:
    name = "geometer.exe" if sys.platform == "win32" else "geometer"
    return ROOT / "dist" / name


def test_cli_init_request_and_run_hlr_json(tmp_path: Path) -> None:
    request = tmp_path / "request.json"
    response = tmp_path / "response.json"
    projection = tmp_path / "projection.json"

    subprocess.run(
        [
            str(_geometer_exe()),
            "init-request",
            str(request),
            "--step",
            str(SOT23_STEP),
            "--operation",
            "step_hlr_projection_json",
            "--output",
            str(projection),
        ],
        check=True,
        cwd=ROOT,
    )

    payload = json.loads(request.read_text(encoding="utf-8"))
    assert payload["schema"] == "geometer.batch.request.a0"
    assert payload["jobs"][0]["operation"] == "step_hlr_projection_json"

    subprocess.run(
        [str(_geometer_exe()), "run", str(request), str(response)],
        check=True,
        cwd=ROOT,
    )

    result = json.loads(response.read_text(encoding="utf-8"))
    assert result["schema"] == "geometer.batch.response.a0"
    assert result["ok"] is True
    assert result["jobs"][0]["ok"] is True
    assert json.loads(projection.read_text(encoding="utf-8"))["schema"] == "geometry.projection.a0"


def test_cli_run_batch_hlr_svg_and_glb(tmp_path: Path) -> None:
    request = tmp_path / "request.json"
    response = tmp_path / "response.json"
    projection = tmp_path / "projection.json"
    svg = tmp_path / "projection.svg"
    glb = tmp_path / "model.glb"
    request.write_text(
        json.dumps(
            {
                "schema": "geometer.batch.request.a0",
                "jobs": [
                    {
                        "id": "projection",
                        "operation": "step_hlr_projection_json",
                        "step_path": str(SOT23_STEP),
                        "output_path": str(projection),
                        "options": {
                            "views": [
                                {"id": "top", "direction": [0, 0, 1], "up": [0, 1, 0]}
                            ],
                            "curve_mode": "polyline",
                        },
                    },
                    {
                        "id": "svg",
                        "operation": "step_hlr_projection_svg",
                        "step_path": str(SOT23_STEP),
                        "output_path": str(svg),
                        "mode": "simple",
                        "view": "top",
                        "options": {
                            "views": [
                                {"id": "top", "direction": [0, 0, 1], "up": [0, 1, 0]}
                            ],
                            "curve_mode": "polyline",
                        },
                    },
                    {
                        "id": "glb",
                        "operation": "step_to_glb",
                        "step_path": str(SOT23_STEP),
                        "output_path": str(glb),
                        "options": {"linear_deflection": 0.1, "angular_deflection": 0.5},
                    },
                ],
            }
        ),
        encoding="utf-8-sig",
    )

    subprocess.run(
        [str(_geometer_exe()), "run", str(request), str(response)],
        check=True,
        cwd=ROOT,
    )

    result = json.loads(response.read_text(encoding="utf-8"))
    assert result["ok"] is True
    assert [job["id"] for job in result["jobs"]] == ["projection", "svg", "glb"]
    assert all(job["ok"] for job in result["jobs"])
    assert projection.stat().st_size > 0
    assert svg.read_text(encoding="utf-8").lstrip().startswith("<svg")
    assert glb.read_bytes()[:4] == b"glTF"
