"""Altium-side container laws, exercised through altium_bake.py in
toolz/altium_monkey's venv (skipped wholesale when that toolchain or the
fixture STEP isn't available — the laws still gate on machines that have it)."""

from __future__ import annotations

import json
import subprocess
from pathlib import Path

import geometer
import pytest

from app.vendor_files import ALTIUM_MONKEY, BAKER, altium_available
from conftest import PROTO

FIXTURE = PROTO.parents[1] / "tests" / "fixtures" / "step" / "embedded_models" / "SOIC-20-300.STEP"

pytestmark = pytest.mark.skipif(
    altium_available() is not None or not FIXTURE.exists(),
    reason="altium_monkey toolchain or fixture STEP not available",
)


def run_baker(*args: str) -> str:
    proc = subprocess.run(
        ["uv", "run", "--project", str(ALTIUM_MONKEY), "python", str(BAKER), *args],
        capture_output=True, text=True, cwd=str(ALTIUM_MONKEY), timeout=600,
    )
    assert proc.returncode == 0, proc.stdout + proc.stderr
    return proc.stdout


@pytest.fixture(scope="module")
def built_lib(tmp_path_factory) -> Path:
    """A fresh single-part PcbLib scaffolded from the fixture STEP."""
    td = tmp_path_factory.mktemp("altium_lib")
    bake_in = td / "in"
    bake_in.mkdir()
    step = bake_in / "fixture_part.step"
    step.write_bytes(FIXTURE.read_bytes())
    bounds = geometer.model_bounds(str(step)).bounds
    (bake_in / "bounds_mm.json").write_text(
        json.dumps({"fixture_part": {"min": bounds["min"], "max": bounds["max"]}})
    )
    out = run_baker(str(bake_in), str(td / "out"))
    assert "IDENTICAL" in out  # the baker self-verifies re-extraction
    return td / "out" / "fixture_part.PcbLib"


class TestAltiumLaws:
    def test_list_shape(self, built_lib):
        rows = json.loads(run_baker("--list", str(built_lib)))
        assert len(rows) == 1
        assert rows[0]["footprint"] == "fixture_part"
        models = rows[0]["models"]
        assert len(models) == 1
        assert models[0]["kind"] == "step"  # sniffed from content
        assert models[0]["bytes"] == FIXTURE.stat().st_size

    def test_extract_identity(self, built_lib, tmp_path):
        out_step = tmp_path / "x.step"
        info = json.loads(run_baker("--extract", str(built_lib), str(out_step)).strip().splitlines()[-1])
        assert out_step.read_bytes() == FIXTURE.read_bytes()
        assert info["model_name"] == "fixture_part.step"

    def test_replace_self_verifies_and_preserves_source(self, built_lib, tmp_path):
        before = built_lib.read_bytes()
        new_payload = tmp_path / "new.step"
        new_payload.write_bytes(b"ISO-10303-21;\nreplacement payload\n" * 100)
        out = tmp_path / "replaced.PcbLib"
        stdout = run_baker(
            "--replace", str(built_lib), "fixture_part.step",
            str(new_payload), str(out),
        )
        assert "payload identity OK" in stdout
        assert "deterministic OK" in stdout
        assert built_lib.read_bytes() == before  # opened sources never modified
        extracted = tmp_path / "back.step"
        run_baker("--extract", str(out), str(extracted))
        assert extracted.read_bytes() == new_payload.read_bytes()

    def test_split_single_part_from_corpus(self, tmp_path):
        timm = PROTO / "TEST_ALTIUM_PCB" / "TiMM.PcbLib"
        if not timm.exists():
            pytest.skip("TEST_ALTIUM_PCB corpus not present")
        split = tmp_path / "SOIC-8.PcbLib"
        run_baker("--split", str(timm), "SOIC-8", str(split))
        rows = json.loads(run_baker("--list", str(split)))
        assert [r["footprint"] for r in rows] == ["SOIC-8"]
        assert len(rows[0]["models"]) == 1  # only the part's own model came along
