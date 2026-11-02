"""Double vertices: unsewn coincident vertices count; the two ends of one
short edge do not (that is a tiny edge)."""

from pathlib import Path

from OCP.BRep import BRep_Builder
from OCP.BRepPrimAPI import BRepPrimAPI_MakeBox
from OCP.gp import gp_Pnt
from OCP.TopoDS import TopoDS_Compound

from app import health
from app.document import BodyRecord, EditorDocument


def _doc(*shapes) -> EditorDocument:
    builder, compound = BRep_Builder(), TopoDS_Compound()
    builder.MakeCompound(compound)
    for shape in shapes:
        builder.Add(compound, shape)
    return EditorDocument(Path("t.step"), [BodyRecord(solid=compound, name="t", color=None)], "TEST")


def test_unsewn_neighbours_are_doubles():
    doc = _doc(BRepPrimAPI_MakeBox(gp_Pnt(0, 0, 0), 1, 1, 1).Shape(),
               BRepPrimAPI_MakeBox(gp_Pnt(1, 0, 0), 1, 1, 1).Shape())
    assert [len(item["points"]) for item in health.double_vertices(doc)] == [8]


def test_ends_of_a_short_edge_are_not_doubles():
    doc = _doc(BRepPrimAPI_MakeBox(gp_Pnt(0, 0, 0), 0.00005, 1, 1).Shape())  # 0.05 um edges
    assert health.double_vertices(doc) == []
    assert health.tiny_edges(doc)  # it is a tiny-edge finding instead


def test_thin_plate_is_not_a_sliver():
    """A QFN's exposed pad: 1 um thick (tiny volume) but wide. Not debris."""
    package = BRepPrimAPI_MakeBox(gp_Pnt(-5, -5, 0), 10, 10, 0.9).Shape()
    pad = BRepPrimAPI_MakeBox(gp_Pnt(-1.7, -1.7, -0.001), 3.4, 3.4, 0.001).Shape()
    speck = BRepPrimAPI_MakeBox(gp_Pnt(6, 6, 0), 0.05, 0.05, 0.05).Shape()
    doc = EditorDocument(Path("t.step"), [BodyRecord(solid=s, name=n, color=None)
                                          for s, n in ((package, "pkg"), (pad, "pad"), (speck, "speck"))], "TEST")
    slivers = [item["body"] for item in health.disjoint(doc) if item.get("sliver")]
    assert slivers == [2]  # the speck goes, the pad stays


def test_pin1_dot_on_the_package_is_not_a_sliver():
    """BTN8982TA: a tiny pin-1 dot sitting ON the package top is a marking,
    not floating debris; the same dot 1 mm above the package is debris."""
    package = BRepPrimAPI_MakeBox(gp_Pnt(-5, -5, 0), 10, 10, 4.4).Shape()
    dot = BRepPrimAPI_MakeBox(gp_Pnt(-4, -3, 4.4), 0.5, 0.5, 0.003).Shape()
    loose = BRepPrimAPI_MakeBox(gp_Pnt(2, 2, 5.4), 0.5, 0.5, 0.003).Shape()
    doc = EditorDocument(Path("t.step"), [BodyRecord(solid=s, name=n, color=None)
                                          for s, n in ((package, "pkg"), (dot, "dot"), (loose, "loose"))], "TEST")
    slivers = [item["body"] for item in health.disjoint(doc) if item.get("sliver")]
    assert slivers == [2]
