"""Model Health: Internal voids (B04B-JWPF-SK-R, 2026-11-01). Hollow shells
sealed inside a body are listed and filled; a big cavity is never offered."""

from pathlib import Path

import pytest
from OCP.BRepAlgoAPI import BRepAlgoAPI_Cut
from OCP.BRepPrimAPI import BRepPrimAPI_MakeBox
from OCP.gp import gp_Pnt

from app import health
from app.document import BodyRecord, EditorDocument, shape_volume


def _hollow_cube(size: float) -> EditorDocument:
    """A 10 mm cube with a closed cubic hollow of `size` mm at its centre."""
    cube = BRepPrimAPI_MakeBox(gp_Pnt(0, 0, 0), 10, 10, 10).Shape()
    lo = 5 - size / 2
    hole = BRepPrimAPI_MakeBox(gp_Pnt(lo, lo, lo), size, size, size).Shape()
    solid = BRepAlgoAPI_Cut(cube, hole).Shape()
    from app.document import _iter_solids
    doc = EditorDocument(Path("void.step"), [BodyRecord(solid=next(_iter_solids(solid)), name="b", color=None)],
                         "TEST")
    doc.remesh_all()
    return doc


def test_small_void_is_found_and_filled():
    doc = _hollow_cube(0.5)
    found = health.internal_voids(doc)
    assert len(found) == 1 and found[0]["voids"] == 1
    assert found[0]["volume_mm3"] == pytest.approx(0.125, rel=1e-3)
    health.apply_fix(doc, {"action": "internal_voids", "body": 0})
    assert health.internal_voids(doc) == []
    assert shape_volume(doc.bodies[0].solid) == pytest.approx(1000.0, rel=1e-6)


def test_big_cavity_is_not_offered():
    doc = _hollow_cube(5.0)  # 125 mm3 = 14% of the body: may be meant
    assert health.internal_voids(doc) == []
    assert len(health.internal_voids(doc, max_share=0.2)) == 1


def test_solid_without_voids_is_clean():
    doc = EditorDocument(Path("c.step"), [BodyRecord(solid=BRepPrimAPI_MakeBox(1, 1, 1).Shape(), name="c",
                                                     color=None)], "TEST")
    doc.remesh_all()
    assert health.internal_voids(doc) == []
