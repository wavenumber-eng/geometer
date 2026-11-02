"""Shallow concave hitbox: a thin ring on a concave contact's curved face,
clear of a centre pin of another net (a barrel jack's sleeve and post)."""

from pathlib import Path

import numpy as np
from OCP.BRepAdaptor import BRepAdaptor_Surface
from OCP.BRepAlgoAPI import BRepAlgoAPI_Cut
from OCP.BRepPrimAPI import BRepPrimAPI_MakeBox, BRepPrimAPI_MakeCylinder
from OCP.GeomAbs import GeomAbs_Cylinder
from OCP.gp import gp_Ax2, gp_Dir, gp_Pnt

from app.document import BodyRecord, EditorDocument
from app.hitbox import shallow_concave_hitbox
from app.pins import Pin


def test_ring_lies_on_the_bore_and_clears_the_middle():
    block = BRepPrimAPI_MakeBox(gp_Pnt(-5, -5, 0), 10, 10, 6).Shape()
    bore = BRepPrimAPI_MakeCylinder(gp_Ax2(gp_Pnt(0, 0, 1), gp_Dir(0, 0, 1)), 3.0, 6.0).Shape()
    doc = EditorDocument(Path("jack.step"), [BodyRecord(solid=BRepAlgoAPI_Cut(block, bore).Shape(),
                                                        name="housing", color=None)], "TEST")
    doc.remesh_all()
    bore_face = next(f for f in range(1, doc.bodies[0].mesh.face_count + 1)
                     if BRepAdaptor_Surface(doc.face_of(0, f)).GetType() == GeomAbs_Cylinder)
    sleeve = Pin(number=2, centroid=(0, 0, 3), face_ids=[(0, bore_face)], role="mouth")
    hitbox = shallow_concave_hitbox(doc, sleeve, depth=0.3)
    assert hitbox["type"] == "shallow_concave"
    assert abs(hitbox["radius_out"] - 3.0) < 1e-6 and abs(hitbox["radius_in"] - 2.7) < 1e-6
    assert abs(hitbox["arc_sweep_deg"] - 360.0) < 1e-6
    radial = np.linalg.norm(np.asarray(hitbox["vertices"])[:, :2], axis=1)
    assert radial.min() > 1.0  # a 1 mm centre post would stay clear
