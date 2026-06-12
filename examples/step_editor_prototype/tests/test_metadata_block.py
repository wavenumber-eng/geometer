"""Conditioning-metadata invariants: the wn3d.step_conditioning.a0 payload,
its AP242 entity injection (always at the END of the DATA section, flush
against ENDSEC), and the pure-text structural audit.

The negative tests are the design-intent teeth: each one tampers with a
single invariant and asserts the auditor catches it, so a future regression
in the injector cannot pass silently.
"""

from __future__ import annotations

import json
import re
from types import SimpleNamespace

import pytest

from app.export_ap242 import extract_metadata, inject_metadata, verify_metadata_text
from app.metadata import SCHEMA, build_metadata
from conftest import make_fake_step


def _doc(name="part.step", bodies=()):
    return SimpleNamespace(
        path=SimpleNamespace(name=name),
        bodies=list(bodies),
    )


def _pin(number, name="", function="", hitbox=None):
    return SimpleNamespace(
        number=number, name=name, function=function, kind="pin", role="primary",
        centroid=(float(number), 0.0, 0.0), body_ids=[number],
        face_ids=[(0, number)], hitbox=hitbox,
    )


class TestNetGrouping:
    """DESIGN_INTENT: pins sharing a designator are the SAME NET — their
    hitboxes form one electrical node."""

    def _nets(self, pins):
        registry = SimpleNamespace(pins=pins)
        return build_metadata(_doc(), pins=registry)["nets"]

    def test_shared_designator_is_one_net(self):
        nets = self._nets([_pin(1, name="SW_A"), _pin(2, name="SW_A")])
        assert len(nets) == 1
        assert nets[0]["designator"] == "SW_A"
        assert [p["number"] for p in nets[0]["pins"]] == [1, 2]

    def test_unnamed_pins_net_by_number(self):
        nets = self._nets([_pin(1), _pin(2)])
        assert {n["designator"] for n in nets} == {"1", "2"}

    def test_inherit_never_becomes_net_function(self):
        nets = self._nets([
            _pin(1, name="SW_A", function="GND"),
            _pin(2, name="SW_A", function="INHERIT"),
        ])
        assert nets[0]["functions"] == ["GND"]
        assert nets[0]["pins"][1]["function"] == "INHERIT"  # stays on the pin

    def test_net_functions_deduplicate(self):
        nets = self._nets([
            _pin(1, name="A", function="GND"), _pin(2, name="A", function="GND"),
        ])
        assert nets[0]["functions"] == ["GND"]

    def test_hitboxes_ride_on_pins(self):
        box = {"kind": "obb", "center": [0, 0, 0]}
        nets = self._nets([_pin(1, name="A", hitbox=box)])
        assert nets[0]["pins"][0]["hitbox"] == box

    def test_schema_constant(self):
        payload = build_metadata(_doc())
        assert payload["schema"] == SCHEMA == "wn3d.step_conditioning.a0"
        assert payload["units"] == "mm"


class TestInjectExtract:
    def test_roundtrip(self, fake_step_path):
        payload = {"schema": SCHEMA, "nets": [{"designator": "A'B"}],
                   "note": "quotes ' and unicode µ"}
        inject_metadata(fake_step_path, payload)
        assert extract_metadata(fake_step_path) == payload

    def test_block_lands_flush_against_endsec(self, fake_step_path):
        inject_metadata(fake_step_path, {"schema": SCHEMA})
        text = fake_step_path.read_text(encoding="utf-8")
        report = verify_metadata_text(text)
        assert report["ok"], report["errors"]
        assert report["tail_gap"] <= 2  # nothing between the block and ENDSEC

    def test_large_payload_chunks_under_limit(self, fake_step_path):
        # OCCT rejects string literals past ~16k — the injector chunks at 6000.
        payload = {"schema": SCHEMA, "blob": "x" * 100_000}
        inject_metadata(fake_step_path, payload)
        text = fake_step_path.read_text(encoding="utf-8")
        report = verify_metadata_text(text)
        assert report["ok"], report["errors"]
        assert report["chunks"] > 1
        assert report["chunk_max"] <= 6000
        assert extract_metadata(fake_step_path) == payload

    def test_entity_ids_do_not_collide(self, fake_step_path):
        inject_metadata(fake_step_path, {"schema": SCHEMA})
        text = fake_step_path.read_text(encoding="utf-8")
        ids = [int(m) for m in re.findall(r"#(\d+)\s*=", text)]
        assert len(ids) == len(set(ids))

    def test_no_metadata_returns_none(self, fake_step_path):
        assert extract_metadata(fake_step_path) is None


class TestAuditCatchesTampering:
    """Each tamper breaks exactly one invariant; the audit must fail."""

    @pytest.fixture
    def injected(self, fake_step_path):
        inject_metadata(fake_step_path, {"schema": SCHEMA, "blob": "y" * 20_000})
        return fake_step_path.read_text(encoding="utf-8")

    def _first_item_id(self, text: str) -> int:
        return int(re.search(
            r"#(\d+)=DESCRIPTIVE_REPRESENTATION_ITEM\('WN3D_CONDITIONING'", text
        ).group(1))

    def test_clean_text_passes(self, injected):
        assert verify_metadata_text(injected)["ok"]

    def test_missing_block(self):
        report = verify_metadata_text(make_fake_step())
        assert not report["ok"]

    def test_foreign_entity_inside_block(self, injected):
        marker = "=REPRESENTATION('WN3D_CONDITIONING'"
        at = injected.index(marker)
        line_start = injected.rfind("#", 0, at)
        tampered = (injected[:line_start]
                    + "#99990=CARTESIAN_POINT('',(0.,0.,0.));\n"
                    + injected[line_start:])
        assert not verify_metadata_text(tampered)["ok"]

    def test_block_not_flush_against_endsec(self, injected):
        endsec = injected.rfind("ENDSEC;")
        tampered = (injected[:endsec]
                    + "#99991=CARTESIAN_POINT('',(0.,0.,0.));\n"
                    + injected[endsec:])
        assert not verify_metadata_text(tampered)["ok"]

    def test_duplicate_entity_id(self, injected):
        first = self._first_item_id(injected)
        tampered = injected.replace(
            "ENDSEC;\nEND-ISO", f"#{first}=CARTESIAN_POINT('',(0.,0.,0.));\nENDSEC;\nEND-ISO", 1
        )
        # appended duplicate id after the block -> flushness AND id checks fire
        assert not verify_metadata_text(tampered)["ok"]

    def test_representation_refs_mismatch(self, injected):
        first = self._first_item_id(injected)
        tampered = re.sub(
            r"(=REPRESENTATION\('WN3D_CONDITIONING',\()#%d," % first,
            r"\g<1>#99992,", injected,
        )
        assert not verify_metadata_text(tampered)["ok"]

    def test_missing_pdr(self, injected):
        tampered = re.sub(
            r"#\d+=PROPERTY_DEFINITION_REPRESENTATION\(#\d+,#\d+\);\n", "", injected
        )
        assert not verify_metadata_text(tampered)["ok"]

    def test_corrupt_json(self, injected):
        first = self._first_item_id(injected)
        tampered = injected.replace(
            f"#{first}=DESCRIPTIVE_REPRESENTATION_ITEM('WN3D_CONDITIONING','",
            f"#{first}=DESCRIPTIVE_REPRESENTATION_ITEM('WN3D_CONDITIONING','%%%",
            1,
        )
        assert not verify_metadata_text(tampered)["ok"]

    def test_audit_reports_facts(self, injected):
        report = verify_metadata_text(injected)
        assert report["schema"] == SCHEMA
        assert report["chunks"] >= 4  # 20k payload at 6000/chunk
        assert json.dumps(report["errors"]) == "[]"
