"""Pins on leads that are already their own solids (BTN8982TA, 2026-11-01).

The vendor ships every lead as its own body, so Separate has nothing to split
and was skipped; the pins (clicked on each lead's tip face) were never linked
to their leads. Step 9 then found no contact for any lead (a tip face is
vertical) and the footprint came out with the tab only. Now the pins link to
their own solids when Detect Pins is left, on replay and on reopen, and the
footprint measures a face pin on its own lead as the whole lead; a foot seated
a fraction of a degree off flat still counts as on the board."""

from pathlib import Path

import numpy as np
from OCP.BRepBuilderAPI import BRepBuilderAPI_Transform
from OCP.BRepPrimAPI import BRepPrimAPI_MakeBox
from OCP.gp import gp_Ax1, gp_Dir, gp_Pnt, gp_Trsf

from app.document import BodyRecord, EditorDocument
from app.pins import Pin, apply_pin_body_names, link_own_solids
from app.tools.footprint_embed import geometry_footprint


def _box(lo, size):
    return BRepPrimAPI_MakeBox(gp_Pnt(*lo), *size).Shape()


def _tilted(shape, angle_rad):
    trsf = gp_Trsf()
    trsf.SetRotation(gp_Ax1(gp_Pnt(0, 0, 0), gp_Dir(1, 0, 0)), angle_rad)
    return BRepBuilderAPI_Transform(shape, trsf, True).Shape()


def _part(tilt=0.0):
    """A package with two separate gull-wing-ish feet (flat boxes) in front."""
    shapes = [_box((-3, -3, 0.2), (6, 6, 2.0)), _box((-1.5, -5.0, 0.0), (0.8, 2.2, 0.3)),
              _box((0.7, -5.0, 0.0), (0.8, 2.2, 0.3))]
    if tilt:
        shapes = [_tilted(s, tilt) for s in shapes]
    doc = EditorDocument(Path("p.step"), [BodyRecord(solid=s, name=f"b{i}", color=None)
                                          for i, s in enumerate(shapes)], "TEST")
    if tilt:  # re-seat: lowest point at Z = 0
        zmin = min(b.mesh.points[:, 2].min() for b in (doc.remesh_all() or doc).bodies)
        doc.apply_trsf(np.array([[1, 0, 0, 0], [0, 1, 0, 0], [0, 0, 1, -zmin], [0, 0, 0, 1]], float))
    doc.remesh_all()
    return doc


def _tip_face(doc, body):
    """The lead's front (tip) face: the face whose triangles reach furthest out in -Y."""
    mesh = doc.bodies[body].mesh
    centres = mesh.points[mesh.tris].mean(axis=1)
    return int(mesh.tri_face_ids[int(np.argmin(centres[:, 1]))])


def _pins(doc):
    return [Pin(i + 1, (x, -5.0, 0.15), face_ids=[(b, _tip_face(doc, b))], name=str(i + 1))
            for i, (x, b) in enumerate(((-1.1, 1), (1.1, 2)))]


def test_pins_link_to_their_own_solids():
    doc = _part()
    pins = _pins(doc)
    assert link_own_solids(doc, pins) == 2
    assert [p.body_ids for p in pins] == [[1], [2]]
    apply_pin_body_names(doc, pins)
    assert [b.role for b in doc.bodies] == ["body", "pin", "pin"]


def test_footprint_finds_leads_clicked_on_their_tip_face():
    doc = _part()
    geo = geometry_footprint(doc, _pins(doc))  # not linked: the footprint resolves the solids itself
    assert geo["unresolved"] == []
    sizes = sorted(tuple(np.round(p["size_xy"], 2)) for p in geo["pads"])
    assert sizes == [(0.8, 2.2), (0.8, 2.2)]


def test_slightly_tilted_feet_still_touch_the_board():
    doc = _part(tilt=0.0035)  # 0.2 deg: a 2.2 mm foot rises ~0.008 mm along its length
    geo = geometry_footprint(doc, _pins(doc))
    assert geo["unresolved"] == []
    assert all(p["size_xy"][1] > 2.0 for p in geo["pads"])  # the whole foot, not a sliver of it


def test_repair_deleting_a_body_keeps_pins_on_their_leads():
    """A body removed BEFORE the pins' bodies (BTN8982TA: a speck at index 0)
    shifted every index after it while the pins kept the old numbers."""
    from app import health
    from app.pins import fix_keeping_pins

    speck = _box((9.0, 9.0, 0.0), (0.05, 0.05, 0.05))  # floating debris, body 0
    shapes = [speck, _box((-3, -3, 0.2), (6, 6, 2.0)), _box((-1.5, -5.0, 0.0), (0.8, 2.2, 0.3)),
              _box((0.7, -5.0, 0.0), (0.8, 2.2, 0.3))]
    doc = EditorDocument(Path("p.step"), [BodyRecord(solid=s, name=f"b{i}", color=None)
                                          for i, s in enumerate(shapes)], "TEST")
    doc.remesh_all()
    pins = [Pin(1, (-1.1, -5.0, 0.15), body_ids=[2], face_ids=[(2, _tip_face(doc, 2))], name="1"),
            Pin(2, (1.1, -5.0, 0.15), body_ids=[3], face_ids=[(3, _tip_face(doc, 3))], name="2")]
    leads = [doc.bodies[2], doc.bodies[3]]
    assert [i["body"] for i in health.disjoint(doc) if i.get("sliver")] == [0]
    fix_keeping_pins(doc, pins, lambda: health.apply_fix(doc, {"action": "disjoint", "body": 0, "sliver": True}))
    assert len(doc.bodies) == 3
    assert [doc.bodies[p.body_ids[0]] for p in pins] == leads
    assert [p.face_ids[0][0] for p in pins] == [1, 2]
