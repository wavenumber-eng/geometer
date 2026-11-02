"""Export merges the collinear edge pieces Separate leaves behind, without
moving, dropping or reordering a single face (face colours and pin faces are
stored by face index)."""

from OCP.BRepAlgoAPI import BRepAlgoAPI_Fuse
from OCP.BRepPrimAPI import BRepPrimAPI_MakeBox
from OCP.gp import gp_Pnt
from OCP.TopAbs import TopAbs_EDGE
from OCP.TopTools import TopTools_IndexedMapOfShape
from OCP.TopExp import TopExp

from app.lean_brep import _face_order, _faces, _same_faces, merge_split_edges


def _edges(shape) -> int:
    edges = TopTools_IndexedMapOfShape()
    TopExp.MapShapes_s(shape, TopAbs_EDGE, edges)
    return edges.Extent()


def _split_box():
    """Two boxes fused side by side: the long edges come out in pieces."""
    a = BRepPrimAPI_MakeBox(gp_Pnt(0, 0, 0), 2, 1, 1).Shape()
    b = BRepPrimAPI_MakeBox(gp_Pnt(2, 0, 0), 2, 1, 1).Shape()
    return BRepAlgoAPI_Fuse(a, b).Shape()


def test_merge_keeps_every_face_in_place_and_drops_no_geometry():
    solid = _split_box()
    lean = merge_split_edges(solid)
    assert _same_faces(_faces(solid), _faces(lean))
    assert _edges(lean) <= _edges(solid)


def test_face_order_recovers_a_permutation_and_rejects_a_change():
    before = [(1.0, (0.0, 0.0, 0.0)), (2.0, (1.0, 0.0, 0.0)), (3.0, (0.0, 1.0, 0.0))]
    after = [before[2], before[0], before[1]]
    assert _face_order(before, after) == [1, 2, 0]
    moved = [before[2], before[0], (2.0, (1.0, 0.5, 0.0))]
    assert _face_order(before, moved) is None
