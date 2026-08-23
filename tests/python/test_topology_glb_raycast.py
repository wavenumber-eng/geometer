from __future__ import annotations

import os
import shutil
import subprocess
from pathlib import Path

ROOT = Path(__file__).resolve().parents[2]
FIXTURE_ROOT = ROOT / "tests" / "fixtures" / "step"
FIXTURES = (
    FIXTURE_ROOT / "embedded_models" / "miniature_test_point.stp",
    FIXTURE_ROOT / "embedded_models" / "SOT-23.STEP",
    FIXTURE_ROOT / "embedded_models" / "RESC1608X06L.step",
    FIXTURE_ROOT / "embedded_models" / "SOIC-20-300.STEP",
    FIXTURE_ROOT / "embedded_models" / "ABM3B.STEP",
    FIXTURE_ROOT / "generated_topology" / "generated_repeated_occurrences.step",
    FIXTURE_ROOT / "generated_topology" / "generated_fused_slab.step",
    FIXTURE_ROOT / "generated_topology" / "generated_flat_multi_solid.step",
)


def _fixture_emitter() -> Path:
    suffix = ".exe" if os.name == "nt" else ""
    from geometer._paths import _platform_tag

    name = f"geometer_step_topology_glb_fixture_emitter{suffix}"
    candidates = (
        ROOT / f"build-native-{_platform_tag()}" / "tests" / "cpp" / name,
        ROOT / "build" / "tests" / "cpp" / name,
    )
    for candidate in candidates:
        if candidate.is_file():
            return candidate
    raise AssertionError("native topology GLB fixture emitter is unavailable")


def _run_raycast_case(tmp_path: Path, fixture: Path, *, reflect_x: bool = False) -> str:
    node = shutil.which("node")
    if node is None:
        raise AssertionError("Node.js is unavailable")
    stem = f"{fixture.stem}{'-reflected' if reflect_x else ''}"
    glb_path = tmp_path / f"{stem}.glb"
    expected_path = tmp_path / f"{stem}.expected.json"
    emitter_command: tuple[object, ...] = (
        _fixture_emitter(),
        fixture,
        glb_path,
        expected_path,
    )
    if reflect_x:
        emitter_command += ("--reflect-x",)
    subprocess.run(
        emitter_command,
        cwd=ROOT,
        check=True,
        capture_output=True,
        timeout=30,
    )
    raycast_command: tuple[object, ...] = (
        node,
        ROOT / "tests" / "js" / "step_topology_glb_raycast.mjs",
        glb_path,
        expected_path,
    )
    if reflect_x:
        raycast_command += ("--expect-reflection",)
    completed = subprocess.run(
        raycast_command,
        cwd=ROOT,
        check=True,
        capture_output=True,
        text=True,
        timeout=30,
    )
    return completed.stdout


def test_threejs_raycast_resolves_every_corpus_occurrence_and_face(tmp_path: Path) -> None:
    for fixture in FIXTURES:
        output = _run_raycast_case(tmp_path, fixture)
        assert output.startswith("validated ")
        assert "occurrence/face raycast bindings" in output


def test_threejs_raycast_preserves_reflected_instance_binding(tmp_path: Path) -> None:
    output = _run_raycast_case(
        tmp_path,
        FIXTURE_ROOT / "generated_topology" / "generated_repeated_occurrences.step",
        reflect_x=True,
    )
    assert output.startswith("validated ")
