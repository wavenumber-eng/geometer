"""Pads from geometry: a pin modelled as a pad thinner than the 0.01 mm
section still gives its full contact (its bottom faces at the seat), and an
exposed pad keeps its own size in the IPC land pattern."""

from pathlib import Path

from OCP.BRepPrimAPI import BRepPrimAPI_MakeBox
from OCP.gp import gp_Pnt

from app.document import BodyRecord, EditorDocument
from app.pins import Pin
from app.tools.footprint_embed import geometry_footprint, ipc_land_pattern


def _doc():
    package = BRepPrimAPI_MakeBox(gp_Pnt(-3, -3, 0.0), 6, 6, 0.9).Shape()
    lead = BRepPrimAPI_MakeBox(gp_Pnt(-3, -0.12, 0.0), 0.6, 0.24, 0.005).Shape()   # 5 um thick
    pad = BRepPrimAPI_MakeBox(gp_Pnt(-1.85, -1.85, -0.001), 3.7, 3.7, 0.001).Shape()  # 1 um thick
    bodies = [BodyRecord(solid=s, name=n, color=None) for s, n in ((package, "pkg"), (lead, "pin"), (pad, "ep"))]
    doc = EditorDocument(Path("qfn.step"), bodies, "TEST")
    doc.remesh_all()
    pins = [Pin(number=1, centroid=(-2.7, 0.0, 0.0025), body_ids=[1]),
            Pin(number=2, centroid=(0.0, 0.0, -0.0005), body_ids=[2])]
    return doc, pins


def test_thin_pads_come_from_their_bottom_faces():
    doc, pins = _doc()
    sizes = {pad["pin"]: pad["size_xy"] for pad in geometry_footprint(doc, pins)["pads"]}
    assert sizes == {"1": [0.6, 0.24], "2": [3.7, 3.7]}


def test_exposed_pad_keeps_its_size():
    doc, pins = _doc()
    lands = {pad["number"]: pad for pad in ipc_land_pattern(geometry_footprint(doc, pins), doc)}
    assert (lands["2"]["w"], lands["2"]["h"], lands["2"]["shape"]) == (3.7, 3.7, "rect")
    assert lands["1"]["w"] > 0.6  # the lead's land gets its IPC fillets
