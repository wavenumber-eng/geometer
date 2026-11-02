"""Merge split edges before writing a STEP.

Separate cuts one solid into many, and the boolean leaves every edge it
crossed chopped into short collinear pieces. ADDR0136-K008C (577 solids) went
from 2.5 to 5.3 edges per face: the export was 69 MB instead of 33 and every
load took 40 s instead of 6, because OCC's STEP import repairs each face's
edge loop and that cost grows faster than the loop's length.

ShapeUpgrade_UnifySameDomain with edges only (faces untouched) merges the
pieces back. The writer keys face colors and pin faces by face INDEX, so a
merged solid is used only when every face keeps its index, area and centre;
otherwise the body is written as it is."""

from __future__ import annotations

from OCP.BRepGProp import BRepGProp
from OCP.GProp import GProp_GProps
from OCP.ShapeUpgrade import ShapeUpgrade_UnifySameDomain
from OCP.TopAbs import TopAbs_FACE
from OCP.TopExp import TopExp
from OCP.TopoDS import TopoDS_Shape
from OCP.TopTools import TopTools_IndexedMapOfShape

AREA_TOLERANCE = 1e-4   # relative: merging re-fits a face boundary by a few nanometres
CENTRE_TOLERANCE = 1e-4  # mm (0.1 um)


def _faces(shape: TopoDS_Shape) -> list[tuple[float, tuple[float, float, float]]]:
    """(area, centre) of every face, in the writer's index order."""
    face_map = TopTools_IndexedMapOfShape()
    TopExp.MapShapes_s(shape, TopAbs_FACE, face_map)
    out = []
    for index in range(1, face_map.Extent() + 1):
        props = GProp_GProps()
        BRepGProp.SurfaceProperties_s(face_map.FindKey(index), props)
        c = props.CentreOfMass()
        out.append((props.Mass(), (c.X(), c.Y(), c.Z())))
    return out


def _same_faces(before: list, after: list) -> bool:
    if len(before) != len(after):
        return False
    for (a0, c0), (a1, c1) in zip(before, after):
        if abs(a0 - a1) > AREA_TOLERANCE * max(abs(a0), 1e-9):
            return False
        if max(abs(p - q) for p, q in zip(c0, c1)) > CENTRE_TOLERANCE + 1e-9 * max(map(abs, c0)):
            return False
    return True


def merge_split_edges(solid: TopoDS_Shape) -> TopoDS_Shape:
    """The solid with collinear edge pieces merged, or the solid itself when
    merging would move, drop or reorder any face."""
    try:
        unify = ShapeUpgrade_UnifySameDomain(solid, True, False, False)
        unify.Build()
        merged = unify.Shape()
    except Exception:  # noqa: BLE001 - an unmergeable body is written as it is
        return solid
    if merged is None or merged.IsNull():
        return solid
    before, after = _faces(solid), _faces(merged)
    if _same_faces(before, after):
        return merged
    # Same faces, other order (the big housing of a separated part): rebuild
    # the shell with the merged faces in the original order.
    ordered = _in_original_order(merged, before, after)
    return ordered if ordered is not None else solid


def _face_order(before: list, after: list) -> list[int] | None:
    """For each original face, the index of the identical merged face (same
    area and centre); None unless that is a one-to-one match."""
    from scipy.spatial import cKDTree

    if len(before) != len(after) or not before:
        return None
    tree = cKDTree([c for _a, c in after])
    order = []
    for area, centre in before:
        candidates = tree.query_ball_point(centre, CENTRE_TOLERANCE + 1e-9 * max(map(abs, centre)))
        same = [j for j in candidates if abs(after[j][0] - area) <= AREA_TOLERANCE * max(abs(area), 1e-9)]
        if len(same) != 1:
            return None
        order.append(same[0])
    return order if len(set(order)) == len(order) else None


def _in_original_order(merged: TopoDS_Shape, before: list, after: list) -> TopoDS_Shape | None:
    """A single-shell solid rebuilt from the merged faces in the original
    face order, if every face matches one-to-one and the result is valid."""
    from OCP.BRep import BRep_Builder
    from OCP.BRepCheck import BRepCheck_Analyzer
    from OCP.TopAbs import TopAbs_SHELL
    from OCP.TopoDS import TopoDS_Shell, TopoDS_Solid

    order = _face_order(before, after)
    shells = TopTools_IndexedMapOfShape()
    TopExp.MapShapes_s(merged, TopAbs_SHELL, shells)
    if order is None or shells.Extent() != 1:
        return None
    face_map = TopTools_IndexedMapOfShape()
    TopExp.MapShapes_s(merged, TopAbs_FACE, face_map)
    builder, shell, rebuilt = BRep_Builder(), TopoDS_Shell(), TopoDS_Solid()
    builder.MakeShell(shell)
    for j in order:
        builder.Add(shell, face_map.FindKey(j + 1))
    builder.MakeSolid(rebuilt)
    builder.Add(rebuilt, shell)
    if not BRepCheck_Analyzer(rebuilt).IsValid() or not _same_faces(before, _faces(rebuilt)):
        return None
    return rebuilt


def lean_solids(bodies) -> list[TopoDS_Shape]:
    """merge_split_edges for every body's solid, in body order."""
    return [merge_split_edges(body.solid) for body in bodies]
