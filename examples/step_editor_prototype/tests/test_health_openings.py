"""Openings: a missing face is a HOLE (filled flat); the fill must not lose
geometry or blow up the model's size."""

from pathlib import Path

from OCP.BRep import BRep_Builder
from OCP.BRepPrimAPI import BRepPrimAPI_MakeBox
from OCP.TopAbs import TopAbs_FACE
from OCP.TopoDS import TopoDS_Shell

from app import health
from app.document import BodyRecord, EditorDocument


def _open_box() -> EditorDocument:
    builder, shell = BRep_Builder(), TopoDS_Shell()
    builder.MakeShell(shell)
    for face in health._shapes(BRepPrimAPI_MakeBox(10, 10, 10).Shape(), TopAbs_FACE)[:5]:  # one face missing
        builder.Add(shell, face)
    return EditorDocument(Path("open.step"), [BodyRecord(solid=shell, name="box", color=None)], "TEST")


def test_missing_face_is_a_hole_and_fills_flat():
    doc = _open_box()
    doc.remesh_all()
    found = health.openings(doc)
    assert len(found) == 1 and found[0]["gap"] is None and "hole" in found[0]["label"]
    assert health.fill_opening(doc, 0, 0, "flat")
    assert health.openings(doc) == []
    assert len(health._shapes(doc.bodies[0].solid, TopAbs_FACE)) == 6
    xmin, xmax, *_ = doc.bounds()
    assert xmax - xmin < 10.1  # no runaway patch
