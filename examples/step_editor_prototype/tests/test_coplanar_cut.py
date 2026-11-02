"""Box cuts and coplanar faces: a pin's box whose bottom lies on the body's
top face must cut the pin out without taking a square of the body with it."""

from pathlib import Path

import pytest
from OCP.BRepAlgoAPI import BRepAlgoAPI_Fuse
from OCP.BRepPrimAPI import BRepPrimAPI_MakeBox
from OCP.gp import gp_Pnt

from app.document import BodyRecord, EditorDocument, respect_coplanar_faces, shape_volume


def cube_with_pin() -> EditorDocument:
    """A 10 mm cube with a 2 x 2 x 5 mm pin fused on top (one unibody)."""
    cube = BRepPrimAPI_MakeBox(gp_Pnt(0, 0, 0), 10, 10, 10).Shape()
    pin = BRepPrimAPI_MakeBox(gp_Pnt(4, 4, 10), 2, 2, 5).Shape()
    fused = BRepAlgoAPI_Fuse(cube, pin).Shape()
    return EditorDocument(Path("cube_with_pin.step"), [BodyRecord(solid=fused, name="unibody", color=None)], "TEST")


@pytest.mark.parametrize("bottom", [10.0, 9.98])  # exactly coplanar, and dragged a hair too deep
def test_box_around_pin_leaves_the_body_whole(bottom):
    doc = cube_with_pin()
    cut = {"type": "box", "limits": [3.5, 6.5, 3.5, 6.5, bottom, 15.5]}
    carved = doc.split_by_cuts([cut], remesh=False)
    assert len(carved) == 1
    volumes = sorted(shape_volume(b.solid) for b in doc.bodies)
    assert volumes[0] == pytest.approx(20.0, rel=1e-3)    # the pin: 2 x 2 x 5
    assert volumes[1] == pytest.approx(1000.0, rel=1e-3)  # the cube keeps every bit


def test_same_facing_coplanar_face_is_taken_in():
    """The pin's own top face lies on the box's top face and faces the same
    way (+Z): the box face moves outward to include it."""
    doc = cube_with_pin()
    adjusted = respect_coplanar_faces(doc.bodies, [{"type": "box", "limits": [3.5, 6.5, 3.5, 6.5, 10.0, 15.0]}])
    assert adjusted[0]["limits"][5] > 15.0       # top: same direction -> outward
    assert adjusted[0]["limits"][4] == pytest.approx(10.0)  # bottom: opposite -> exactly on the plane
