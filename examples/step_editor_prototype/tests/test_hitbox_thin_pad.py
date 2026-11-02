"""Hitboxes for paper-thin pin solids and the board rule (16-WQFN-4x4,
2026-11-01). TI draws a QFN's exposed pad as a 1 um plate: its hull was flat
(no thickness), and the 0.05 mm margin pushed every surface pin's hitbox under
the board. A thin solid is thickened like a face; a surface pin's hitbox stays
at Z >= 0; a through-hole leg still reaches below the board."""

from pathlib import Path

import numpy as np
import pytest
from OCP.BRepPrimAPI import BRepPrimAPI_MakeBox
from OCP.gp import gp_Pnt

from app.document import BodyRecord, EditorDocument
from app.hitbox import keep_above_board, pin_hitbox
from app.pins import Pin


def _doc(*boxes) -> EditorDocument:
    bodies = [BodyRecord(solid=BRepPrimAPI_MakeBox(gp_Pnt(*lo), *size).Shape(), name=f"b{i}", color=None)
              for i, (lo, size) in enumerate(boxes)]
    doc = EditorDocument(Path("qfn.step"), bodies, "TEST")
    doc.remesh_all()
    return doc


def test_paper_thin_exposed_pad_gets_a_real_hitbox():
    doc = _doc(((-2, -2, 0.001), (4, 4, 0.8)),          # package
               ((-1.3, -1.3, 0.0), (2.6, 2.6, 0.001)))   # 1 um exposed-pad plate
    pin = Pin(17, (0.0, 0.0, 0.0), body_ids=[1], name="THERM")
    hitbox = pin_hitbox(doc, pin, [pin], margin=0.05, size=0.5)
    v = np.array(hitbox["vertices"])
    assert v[:, 2].min() == pytest.approx(0.0, abs=1e-9)        # on the board, not under it
    assert v[:, 2].max() - v[:, 2].min() >= 0.5                 # a volume, not a sheet
    assert all(2.6 < e <= 2.71 for e in (v.max(0) - v.min(0))[:2])  # the pad plus about its margin


def test_surface_lead_margin_stays_above_the_board():
    doc = _doc(((-2, -2, 0.0), (4, 4, 0.8)), ((-2.05, 0.6, 0.0), (0.4, 0.3, 0.2)))
    pin = Pin(1, (-1.85, 0.75, 0.0), body_ids=[1], name="1")
    v = np.array(pin_hitbox(doc, pin, [pin], margin=0.05)["vertices"])
    assert v[:, 2].min() == pytest.approx(0.0, abs=1e-9) and v[:, 2].max() > 0.2


def test_through_hole_leg_still_reaches_below_the_board():
    leg = np.array([[0, 0, -3.0], [0.6, 0, -3.0], [0, 0.6, -3.0], [0, 0, 1.0], [0.6, 0.6, 1.0]])
    box = {"type": "convex_hull", "vertices": (leg + [[0, 0, -0.05]] * 5).tolist()}
    assert keep_above_board(box, leg) is box
