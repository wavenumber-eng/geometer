"""Star vertices are flat fans: many edges leaving the hub in ONE plane (an
N-gon triangulated from its centre). A cone tip or a 3D corner is not."""

from pathlib import Path

import numpy as np
from OCP.BRepBuilderAPI import BRepBuilderAPI_MakePolygon, BRepBuilderAPI_MakeFace, BRepBuilderAPI_Sewing
from OCP.gp import gp_Pnt

from app import health
from app.document import BodyRecord, EditorDocument


def _fan(hub_height: float, spokes: int = 12) -> EditorDocument:
    sewing = BRepBuilderAPI_Sewing(1e-6)
    hub = gp_Pnt(0, 0, hub_height)
    rim = [gp_Pnt(np.cos(a), np.sin(a), 0) for a in np.linspace(0, 2 * np.pi, spokes, endpoint=False)]
    for k in range(spokes):
        poly = BRepBuilderAPI_MakePolygon(hub, rim[k], rim[(k + 1) % spokes], True)
        sewing.Add(BRepBuilderAPI_MakeFace(poly.Wire(), True).Face())
    sewing.Perform()
    return EditorDocument(Path("fan.step"), [BodyRecord(solid=sewing.SewedShape(), name="fan", color=None)], "TEST")


def test_flat_fan_is_a_star():
    assert [len(i["points"]) for i in health.star_vertices(_fan(0.0))] == [1]


def test_cone_tip_is_not_a_star():
    assert health.star_vertices(_fan(0.5)) == []
