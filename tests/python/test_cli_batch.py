from __future__ import annotations

import json
import subprocess
from pathlib import Path

import geometer


ROOT = Path(__file__).resolve().parents[2]
SOT23_STEP = ROOT / "tests" / "fixtures" / "step" / "embedded_models" / "SOT-23.STEP"


def _geometer_exe() -> Path:
    return geometer.executable_path()


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


def test_cli_run_batch_uses_request_options_with_job_overrides(tmp_path: Path) -> None:
    request = tmp_path / "request.json"
    response = tmp_path / "response.json"
    root_projection = tmp_path / "root_projection.json"
    job_projection = tmp_path / "job_projection.json"
    request.write_text(
        json.dumps(
            {
                "schema": "geometer.batch.request.a0",
                "options": {
                    "views": [
                        {"id": "root-view", "direction": [0, 0, 1], "up": [0, 1, 0]}
                    ],
                    "curve_mode": "polyline",
                    "mesh_linear_deflection": 0.2,
                },
                "jobs": [
                    {
                        "id": "root-options",
                        "operation": "step_hlr_projection_json",
                        "step_path": str(SOT23_STEP),
                        "output_path": str(root_projection),
                    },
                    {
                        "id": "job-override",
                        "operation": "step_hlr_projection_json",
                        "step_path": str(SOT23_STEP),
                        "output_path": str(job_projection),
                        "options": {
                            "views": [
                                {"id": "job-view", "direction": [0, 0, 1], "up": [0, 1, 0]}
                            ],
                            "meshLinearDeflection": 0.1,
                        },
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
    assert [job["id"] for job in result["jobs"]] == ["root-options", "job-override"]
    root_payload = json.loads(root_projection.read_text(encoding="utf-8"))
    job_payload = json.loads(job_projection.read_text(encoding="utf-8"))
    assert root_payload["views"][0]["id"] == "root-view"
    assert job_payload["views"][0]["id"] == "job-view"


def test_python_run_batch_wrapper(tmp_path: Path) -> None:
    projection = tmp_path / "projection.json"

    result = geometer.run_batch(
        [
            {
                "id": "projection",
                "operation": "step_hlr_projection_json",
                "step_path": str(SOT23_STEP),
                "output_path": str(projection),
            }
        ],
        options={
            "views": [{"id": "python-top", "direction": [0, 0, 1], "up": [0, 1, 0]}],
            "curve_mode": "polyline",
        },
        work_dir=tmp_path,
    )

    assert result["ok"] is True
    assert result["jobs"][0]["id"] == "projection"
    payload = json.loads(projection.read_text(encoding="utf-8"))
    assert payload["views"][0]["id"] == "python-top"


def test_python_batch_runner_chunks_jobs(tmp_path: Path) -> None:
    outputs = [tmp_path / f"projection_{index}.json" for index in range(7)]
    jobs = [
        {
            "id": f"projection-{index}",
            "operation": "step_hlr_projection_json",
            "step_path": str(SOT23_STEP),
            "output_path": str(output_path),
        }
        for index, output_path in enumerate(outputs)
    ]

    runner = geometer.GeometerBatchRunner(
        max_workers=2,
        chunk_size=3,
        work_dir=tmp_path / "batches",
    )
    assert runner.version().string == "2026.5.23"
    result = runner.run(
        jobs,
        options={
            "views": [{"id": "chunked-top", "direction": [0, 0, 1], "up": [0, 1, 0]}],
            "curve_mode": "polyline",
        },
    )

    assert result.ok is True
    assert [job["id"] for job in result.jobs] == [f"projection-{index}" for index in range(7)]
    assert [batch["job_count"] for batch in result.batches] == [3, 3, 1]
    assert result.work_dir == tmp_path / "batches"
    assert result.version == "2026.5.23"
    assert result.abi == 20260523
    for output_path in outputs:
        payload = json.loads(output_path.read_text(encoding="utf-8"))
        assert payload["views"][0]["id"] == "chunked-top"
