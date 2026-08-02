"""Vendor-container laws (Phase 2): containers open by extraction, exports
are single per-part files, payloads are sniffed by content, and the opened
source is NEVER modified."""

from __future__ import annotations

from pathlib import Path

import pytest

from app.kicad_embed import decode_payload, find_embedded_files, scaffold_kicad_mod
from app.vendor_files import (
    VendorContainer,
    _safe_filename,
    altium_available,
    bake_conditioned,
    is_vendor_file,
    open_vendor_file,
)
from conftest import make_synthetic_payload


def write_scaffold(path: Path, payload: bytes, name: str = "part.step") -> None:
    with open(path, "w", encoding="utf-8", newline="") as fh:
        fh.write(scaffold_kicad_mod(path.stem, name, payload))


class TestRouting:
    @pytest.mark.parametrize("name,expected", [
        ("a.kicad_mod", True),
        ("a.KICAD_MOD", True),
        ("a.PcbLib", True),
        ("a.pcblib", True),
        ("a.step", False),
        ("a.stp", False),
        ("a.PcbDoc", False),  # user decision: PcbDoc is not a vehicle
    ])
    def test_is_vendor_file(self, name, expected):
        assert is_vendor_file(Path(name)) is expected

    def test_safe_filename_strips_reserved(self):
        assert _safe_filename('TZ-SB-0001-PCB-[A] (Main)') == "TZ-SB-0001-PCB-[A] (Main)"
        assert _safe_filename('bad<>:"/\\|?*name') == "bad_________name"
        assert _safe_filename("   ") == "part"


class TestOutputNamingLaws:
    def _container(self, tmp_path, kind="kicad"):
        return VendorContainer(
            kind=kind, source=tmp_path / "src" / "My Part.kicad_mod",
            part_name="My Part", model_name="My Part.step",
            work_lib=None, step_path=tmp_path / "t.step",
        )

    def test_container_output_next_to_source(self, tmp_path):
        c = self._container(tmp_path)
        out = c.conditioned_container_path()
        assert out.parent == c.source.parent
        assert out.name == "My Part_conditioned.kicad_mod"

    def test_step_output_next_to_source(self, tmp_path):
        c = self._container(tmp_path)
        out = c.conditioned_step_path()
        assert out.parent == c.source.parent
        assert out.name == "My Part_AP242_conditioned.step"

    def test_altium_suffix(self, tmp_path):
        c = self._container(tmp_path, kind="altium")
        assert c.conditioned_container_path().suffix == ".PcbLib"


class TestKicadOpen:
    def test_happy_path_extraction_identity(self, tmp_path):
        payload = make_synthetic_payload(b"OPEN")
        src = tmp_path / "part.kicad_mod"
        write_scaffold(src, payload)
        container = open_vendor_file(src, tmp_path)
        assert container.kind == "kicad"
        assert container.part_name == "part"
        assert container.model_name == "part.step"
        assert container.step_path.read_bytes() == payload

    def test_no_embedded_model_is_an_error(self, tmp_path):
        src = tmp_path / "empty.kicad_mod"
        src.write_text('(footprint "empty" (version 20241229))', encoding="utf-8")
        with pytest.raises(RuntimeError, match="no embedded 3D model"):
            open_vendor_file(src, tmp_path)

    def test_non_step_payload_is_refused_by_content(self, tmp_path):
        # Corpus lesson: Altium/KiCad embed whatever the librarian attached.
        # A parasolid payload behind a .step name must be refused.
        src = tmp_path / "sneaky.kicad_mod"
        write_scaffold(src, b"**ABCDEF parasolid transmit file **", name="fake.step")
        with pytest.raises(RuntimeError, match="not STEP"):
            open_vendor_file(src, tmp_path)


class TestSourceNeverModified:
    def test_open_and_bake_leave_source_untouched(self, tmp_path):
        payload = make_synthetic_payload(b"SRC")
        src = tmp_path / "part.kicad_mod"
        write_scaffold(src, payload)
        before = src.read_bytes()

        container = open_vendor_file(src, tmp_path)
        conditioned = tmp_path / "cond.step"
        conditioned.write_bytes(make_synthetic_payload(b"NEW"))
        out = bake_conditioned(container, conditioned, tmp_path / "out.kicad_mod")

        assert src.read_bytes() == before
        with open(out, encoding="utf-8", newline="") as fh:
            text = fh.read()
        raw = decode_payload(find_embedded_files(text)[0].data_base64)
        assert raw == conditioned.read_bytes()

    def test_default_bake_target_is_conditioned_container_path(self, tmp_path):
        payload = make_synthetic_payload(b"SRC")
        src = tmp_path / "part.kicad_mod"
        write_scaffold(src, payload)
        container = open_vendor_file(src, tmp_path)
        conditioned = tmp_path / "cond.step"
        conditioned.write_bytes(make_synthetic_payload(b"NEW"))
        out = bake_conditioned(container, conditioned)
        assert out == container.conditioned_container_path()
        assert out.exists()


class TestAltiumAvailability:
    def test_reports_none_or_reason(self):
        reason = altium_available()
        assert reason is None or isinstance(reason, str)
