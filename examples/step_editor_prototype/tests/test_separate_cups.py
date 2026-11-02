"""Separate Unibody must never invent material (1984714, 2026-11-01).

A painted pin region on a unibody can include metal that only shows through
a hole: the wire clamp seen through a terminal block's wire entry. Sealing
those faces made a solid of the AIR in the pocket, put a lid on the housing
over the opening, and (sewn with the leg into one solid) cancelled the leg's
volume, so the housing and pin overlapped (z-fighting) and the hitbox built
from that pin covered the pocket. Now only faces the sealed solid really sits
behind become the pin; the rest stays on the housing, uncapped."""

from pathlib import Path
from types import SimpleNamespace

import numpy as np
import pytest
from OCP.BRepAlgoAPI import BRepAlgoAPI_Cut, BRepAlgoAPI_Fuse
from OCP.BRepPrimAPI import BRepPrimAPI_MakeBox
from OCP.GProp import GProp_GProps
from OCP.BRepGProp import BRepGProp
from OCP.TopAbs import TopAbs_FACE
from OCP.TopExp import TopExp
from OCP.TopTools import TopTools_IndexedMapOfShape
from OCP.gp import gp_Pnt

from app.document import BodyRecord, EditorDocument, shape_volume

POCKET = 2.0 * 2.0 * 2.0
PIN = 2.0 * 2.0 * 5.0


def _housing():
    """A 10 mm cube, a 2x2x5 pin fused on top, a 2x2x2 pocket cut in a side."""
    cube = BRepPrimAPI_MakeBox(gp_Pnt(0, 0, 0), 10, 10, 10).Shape()
    pin = BRepPrimAPI_MakeBox(gp_Pnt(4, 4, 10), 2, 2, 5).Shape()
    pocket = BRepPrimAPI_MakeBox(gp_Pnt(-1, 4, 4), 3, 2, 2).Shape()  # x 0..2 inside
    solid = BRepAlgoAPI_Cut(BRepAlgoAPI_Fuse(cube, pin).Shape(), pocket).Shape()
    doc = EditorDocument(Path("pocket.step"), [BodyRecord(solid=solid, name="unibody", color=None)], "TEST")
    doc.remesh_all()
    return doc


def _faces(doc, test):
    """Face indices (1-based, the editor's numbering) whose centroid passes."""
    face_map = TopTools_IndexedMapOfShape()
    TopExp.MapShapes_s(doc.bodies[0].solid, TopAbs_FACE, face_map)
    out = []
    for i in range(1, face_map.Extent() + 1):
        props = GProp_GProps()
        BRepGProp.SurfaceProperties_s(face_map.FindKey(i), props)
        if test(np.array(props.CentreOfMass().Coord())):
            out.append(i)
    return out


def _pin_faces(doc):
    return _faces(doc, lambda c: c[2] > 10.0 + 1e-6)


def _pocket_faces(doc):
    return _faces(doc, lambda c: 0.0 < c[0] <= 2.0 + 1e-6 and 4 - 1e-6 <= c[1] <= 6 + 1e-6
                  and 4 - 1e-6 <= c[2] <= 6 + 1e-6)


def test_fixture_faces():
    doc = _housing()
    assert len(_pin_faces(doc)) == 5 and len(_pocket_faces(doc)) == 5


def test_pin_plus_pocket_region_takes_only_the_pin():
    doc = _housing()
    total = shape_volume(doc.bodies[0].solid)
    region = [(0, f) for f in _pin_faces(doc) + _pocket_faces(doc)]
    bodies = doc.split_by_face_regions([region])
    assert bodies[0] is not None and len(doc.bodies) == 2
    pin = doc.bodies[bodies[0]]
    package = doc.bodies[1 - bodies[0]]
    assert shape_volume(pin.solid) == pytest.approx(PIN, rel=1e-3)
    # the pocket is still open: the housing did not get a lid over it
    assert shape_volume(package.solid) == pytest.approx(total - PIN, rel=1e-3)
    assert shape_volume(package.solid) == pytest.approx(1000.0 - POCKET, rel=1e-3)


def test_pocket_only_region_leaves_the_body_alone():
    doc = _housing()
    before = shape_volume(doc.bodies[0].solid)
    bodies = doc.split_by_face_regions([[(0, f) for f in _pocket_faces(doc)]])
    assert bodies == [None] and len(doc.bodies) == 1
    assert shape_volume(doc.bodies[0].solid) == pytest.approx(before, rel=1e-6)


# ------------------------------------------------- one solid, one hitbox

def _pin(number, role="primary", bodies=(), faces=(), kind="pin", name=""):
    return SimpleNamespace(number=number, role=role, body_ids=list(bodies), face_ids=list(faces),
                           kind=kind, name=name or str(number), hitbox=None)


def _bodies_doc(sizes):
    return SimpleNamespace(bodies=[SimpleNamespace(mesh=SimpleNamespace(tris=np.zeros((n, 3)))) for n in sizes])


def test_one_hitbox_per_solid():
    from app.tools.hitboxes import entity_owner
    doc = _bodies_doc([500, 20, 20])  # housing, a contact, a second contact
    tail = _pin(1, bodies=[1])
    mouth = _pin(13, role="mouth", faces=[(1, 4)])  # the same contact's other end
    second = _pin(2, bodies=[2])
    on_housing = _pin(14, role="mouth", faces=[(0, 9)])  # a contact face on the housing
    pins = [mouth, tail, second, on_housing]
    assert entity_owner(doc, pins, mouth) is tail      # mouth shares the tail's hitbox
    assert entity_owner(doc, pins, tail) is None       # the tail owns it
    assert entity_owner(doc, pins, second) is None     # another solid, its own
    assert entity_owner(doc, pins, on_housing) is None  # the housing is not an entity


def test_two_pins_on_one_anchor_solid_share():
    from app.tools.hitboxes import entity_owner
    doc = _bodies_doc([500, 30])
    a = _pin(5, bodies=[1], kind="anchor", name="ANCHOR")
    b = _pin(6, faces=[(1, 2)], kind="anchor", name="ANCHOR")
    assert entity_owner(doc, [a, b], b) is a and entity_owner(doc, [a, b], a) is None


def test_guide_never_shares_or_owns():
    from app.tools.hitboxes import entity_owner
    doc = _bodies_doc([500, 30])
    guide = _pin(7, bodies=[1], kind="guide", name="GUIDE")
    pin = _pin(8, faces=[(1, 3)])
    assert entity_owner(doc, [guide, pin], guide) is None
    assert entity_owner(doc, [guide, pin], pin) is None


def test_coplanar_housing_face_in_the_region_is_not_doubled():
    """1984714 pin 8: the region took a strip of housing lying in the plane
    where the leg is capped; the cap covered the strip twice (a zero-volume
    fin that z-fights and stretches the hitbox). The strip stays on the housing."""
    doc = _housing()
    top = _faces(doc, lambda c: abs(c[2] - 10.0) < 1e-6)  # the cube's top, around the pin
    assert len(top) == 1
    region = [(0, f) for f in _pin_faces(doc) + top]
    bodies = doc.split_by_face_regions([region])
    assert bodies[0] is not None
    pin = doc.bodies[bodies[0]]
    assert shape_volume(pin.solid) == pytest.approx(PIN, rel=1e-3)
    xmin, xmax, ymin, ymax, zmin, zmax = __import__("app.document", fromlist=["shape_bounds"]).shape_bounds([pin.solid])
    assert (xmin, ymin, xmax, ymax) == pytest.approx((4, 4, 6, 6), abs=1e-3)  # no fin out to the cube's edge
