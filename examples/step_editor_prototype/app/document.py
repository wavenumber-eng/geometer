"""EditorDocument: every OpenCascade (OCP) call in the editor lives here.

The public API accepts and returns only plain Python / numpy data so the
document could later move behind a subprocess boundary (the wn3d_browser
pattern) without touching tool or UI code. All calls are expected to run on
the Qt main thread — never on a worker thread while VTK is rendering.
"""

from __future__ import annotations

import math
import re
from dataclasses import dataclass, field
from pathlib import Path

import numpy as np

from OCP.Bnd import Bnd_Box
from OCP.BRep import BRep_Tool
from OCP.BRepAdaptor import BRepAdaptor_Curve, BRepAdaptor_Surface
from OCP.BRepAlgoAPI import (
    BRepAlgoAPI_Common,
    BRepAlgoAPI_Cut,
    BRepAlgoAPI_Fuse,
    BRepAlgoAPI_Splitter,
)
from OCP.BRepPrimAPI import BRepPrimAPI_MakeBox, BRepPrimAPI_MakeHalfSpace
from OCP.GeomAbs import GeomAbs_Plane
from OCP.ShapeFix import ShapeFix_Shape
from OCP.BRepBndLib import BRepBndLib
from OCP.BRepBuilderAPI import BRepBuilderAPI_MakeFace, BRepBuilderAPI_Transform
from OCP.GeomAPI import GeomAPI_ProjectPointOnSurf
from OCP.GeomLProp import GeomLProp_SLProps
from OCP.gp import gp_Ax3, gp_Dir, gp_Pln, gp_Pnt, gp_Trsf
from OCP.TopTools import TopTools_ListOfShape
from OCP.BRepGProp import BRepGProp
from OCP.BRepMesh import BRepMesh_IncrementalMesh
from OCP.GProp import GProp_GProps
from OCP.IFSelect import IFSelect_RetDone
from OCP.Quantity import Quantity_Color
from OCP.STEPCAFControl import STEPCAFControl_Reader
from OCP.TCollection import TCollection_AsciiString, TCollection_ExtendedString
from OCP.TDataStd import TDataStd_Name
from OCP.TDF import TDF_LabelSequence
from OCP.TDocStd import TDocStd_Document
from OCP.TopAbs import (
    TopAbs_EDGE,
    TopAbs_FACE,
    TopAbs_REVERSED,
    TopAbs_SHELL,
    TopAbs_SOLID,
    TopAbs_VERTEX,
)
from OCP.TopExp import TopExp, TopExp_Explorer
from OCP.TopLoc import TopLoc_Location
from OCP.TopTools import (
    TopTools_IndexedDataMapOfShapeListOfShape,
    TopTools_IndexedMapOfShape,
)
from OCP.TopoDS import TopoDS, TopoDS_Shape
from OCP.XCAFApp import XCAFApp_Application
from OCP.XCAFDoc import XCAFDoc_ColorType, XCAFDoc_DocumentTool, XCAFDoc_ShapeTool


NEUTRAL_RGB = (0.62, 0.64, 0.66)


@dataclass
class BodyMesh:
    points: np.ndarray          # (N, 3) float64, world coordinates
    tris: np.ndarray            # (M, 3) int64 indices into points
    tri_face_ids: np.ndarray    # (M,) int32, 1-based face index within the body
    face_count: int
    face_colors: dict[int, tuple[float, float, float]] = field(default_factory=dict)


@dataclass
class BodyRecord:
    solid: TopoDS_Shape
    name: str
    color: tuple[float, float, float] | None
    role: str = "body"
    mesh: BodyMesh | None = None
    original_color: tuple[float, float, float] | None = None
    original_face_colors: dict[int, tuple[float, float, float]] = field(default_factory=dict)


@dataclass(frozen=True)
class DocumentInfo:
    path: Path
    schema: str
    body_count: int
    face_count: int
    edge_count: int
    vertex_count: int
    bounds: tuple[float, float, float, float, float, float]  # xmin,xmax,ymin,ymax,zmin,zmax


def _label_name(label) -> str | None:
    attr = TDataStd_Name()
    try:
        if label.FindAttribute(TDataStd_Name.GetID_s(), attr):
            ext = attr.Get()
            try:
                return TCollection_AsciiString(ext).ToCString()
            except Exception:
                return TCollection_AsciiString(ext, "?").ToCString()
    except Exception:
        pass
    return None


def _shape_rgb(color_tool, shape) -> tuple[float, float, float] | None:
    """Surface colour first, then generic colour. Guarded: GetColor raises on
    unlabelled shapes in some OCP builds."""
    if color_tool is None:
        return None
    col = Quantity_Color()
    for ctype in (XCAFDoc_ColorType.XCAFDoc_ColorSurf, XCAFDoc_ColorType.XCAFDoc_ColorGen):
        try:
            if color_tool.GetColor(shape, ctype, col):
                return (col.Red(), col.Green(), col.Blue())
        except Exception:
            continue
    return None


def _harvest_face_colors(color_tool, solid) -> dict[int, tuple[float, float, float]]:
    """Per-face colours keyed by face-map index. Must run BEFORE the solid's
    location is baked — the XCAF colour tool keys by exact shape identity."""
    result: dict[int, tuple[float, float, float]] = {}
    if color_tool is None:
        return result
    face_map = TopTools_IndexedMapOfShape()
    TopExp.MapShapes_s(solid, TopAbs_FACE, face_map)
    for index in range(1, face_map.Extent() + 1):
        rgb = _shape_rgb(color_tool, TopoDS.Face_s(face_map.FindKey(index)))
        if rgb is not None:
            result[index] = rgb
    return result


def _bake_location(solid):
    """Assembly instances carry a TopLoc_Location; bake it into the geometry
    so every body is a plain located-at-identity solid. Required for XCAF
    sub-shape (face colour) export — located shapes register as references,
    not simple shapes. (An identity-trsf transform is a no-op in OCCT, so
    strip the location and re-apply it as a real copying transform.)"""
    location = solid.Location()
    if location.IsIdentity():
        return solid
    unplaced = solid.Located(TopLoc_Location())
    return BRepBuilderAPI_Transform(unplaced, location.Transformation(), True).Shape()


def _solid_color(color_tool, solid) -> tuple[float, float, float] | None:
    rgb = _shape_rgb(color_tool, solid)
    if rgb is not None:
        return rgb
    explorer = TopExp_Explorer(solid, TopAbs_FACE)
    if explorer.More():
        return _shape_rgb(color_tool, TopoDS.Face_s(explorer.Current()))
    return None


def _iter_solids(shape):
    explorer = TopExp_Explorer(shape, TopAbs_SOLID)
    while explorer.More():
        yield TopoDS.Solid_s(explorer.Current())
        explorer.Next()


def _count(shape, kind) -> int:
    found = TopTools_IndexedMapOfShape()
    TopExp.MapShapes_s(shape, kind, found)
    return found.Extent()


def shape_bounds(shapes) -> tuple[float, float, float, float, float, float]:
    box = Bnd_Box()
    for shape in shapes:
        BRepBndLib.Add_s(shape, box)
    if box.IsVoid():
        return (-0.5, 0.5, -0.5, 0.5, -0.5, 0.5)
    xmin, ymin, zmin, xmax, ymax, zmax = box.Get()
    return (xmin, xmax, ymin, ymax, zmin, zmax)


def preview_deflection(bounds: tuple[float, float, float, float, float, float]) -> float:
    xmin, xmax, ymin, ymax, zmin, zmax = bounds
    diag = math.sqrt((xmax - xmin) ** 2 + (ymax - ymin) ** 2 + (zmax - zmin) ** 2)
    if diag <= 0.0:
        return 0.1
    return min(max(diag / 1000.0, 0.002), 10.0)


def shape_volume(shape) -> float | None:
    try:
        props = GProp_GProps()
        BRepGProp.VolumeProperties_s(shape, props)
        return float(props.Mass())
    except Exception:
        return None


def shape_area(shape) -> float | None:
    try:
        props = GProp_GProps()
        BRepGProp.SurfaceProperties_s(shape, props)
        return float(props.Mass())
    except Exception:
        return None


def solid_centroid(shape) -> np.ndarray:
    """Volume centre of mass; falls back to the bbox centre when the volume
    integrator can't measure the shape. Used to decide which split piece sits
    inside the cutting box."""
    try:
        props = GProp_GProps()
        BRepGProp.VolumeProperties_s(shape, props)
        if abs(props.Mass()) > 1e-12:
            point = props.CentreOfMass()
            return np.array([point.X(), point.Y(), point.Z()])
    except Exception:
        pass
    xmin, xmax, ymin, ymax, zmin, zmax = shape_bounds([shape])
    return np.array([(xmin + xmax) / 2, (ymin + ymax) / 2, (zmin + zmax) / 2])


def _edge_face_table(solid) -> list[tuple[object, frozenset]]:
    """Each edge of the solid with the face indices flanking it — computed
    once so per-region boundary scans are cheap set lookups."""
    face_map = TopTools_IndexedMapOfShape()
    TopExp.MapShapes_s(solid, TopAbs_FACE, face_map)
    edge_faces = TopTools_IndexedDataMapOfShapeListOfShape()
    TopExp.MapShapesAndAncestors_s(solid, TopAbs_EDGE, TopAbs_FACE, edge_faces)
    table = []
    for edge_index in range(1, edge_faces.Extent() + 1):
        indices = frozenset(
            index
            for shape in edge_faces.FindFromIndex(edge_index)
            if (index := face_map.FindIndex(shape)) > 0
        )
        table.append((TopoDS.Edge_s(edge_faces.FindKey(edge_index)), indices))
    return table


def _boundary_wires(table, inside: set):
    """Closed wires along the edges where `inside` faces meet the rest of the
    body. None when the set has no boundary (it is the whole shell)."""
    from OCP.ShapeAnalysis import ShapeAnalysis_FreeBounds
    from OCP.TopTools import TopTools_HSequenceOfShape

    boundary = [
        edge
        for edge, faces in table
        if not faces.isdisjoint(inside) and not faces <= inside
    ]
    if not boundary:
        return None
    edges = TopTools_HSequenceOfShape()
    for edge in boundary:
        edges.Append(edge)
    wires = TopTools_HSequenceOfShape()
    ShapeAnalysis_FreeBounds.ConnectEdgesToWires_s(edges, 1.0e-7, False, wires)
    return wires


def _region_anchor_points(mesh, region: set[int], max_points: int = 60):
    """Points just BELOW a face region's surface (inward along the triangle
    normals) — constraints that make a filled cap hug the region instead of
    billowing across a bent boundary loop like a soap film."""
    mask = np.isin(mesh.tri_face_ids, np.asarray(sorted(region), dtype=np.int32))
    tris = mesh.tris[mask]
    if not len(tris):
        return []
    corners = mesh.points[tris]
    centers = corners.mean(axis=1)
    normals = np.cross(corners[:, 1] - corners[:, 0], corners[:, 2] - corners[:, 0])
    lengths = np.linalg.norm(normals, axis=1)
    keep = lengths > 1e-12
    centers, normals, lengths = centers[keep], normals[keep], lengths[keep]
    if not len(centers):
        return []
    extents = centers.max(axis=0) - centers.min(axis=0)
    nonzero = extents[extents > 1e-6]
    depth = max(0.02, 0.12 * float(nonzero.min())) if len(nonzero) else 0.02
    # triangle winding is outward (tessellation flips REVERSED faces), so
    # inward = -normal
    points = centers - normals / lengths[:, None] * depth
    step = max(1, len(points) // max_points)
    return [tuple(float(v) for v in p) for p in points[::step]]


def _fill_wire(wire, constraints):
    """An n-sided filled patch bounded by the wire's own edges, optionally
    pinned through `constraints` points. None when filling fails."""
    from OCP.BRepOffsetAPI import BRepOffsetAPI_MakeFilling
    from OCP.GeomAbs import GeomAbs_C0
    from OCP.gp import gp_Pnt

    try:
        filling = BRepOffsetAPI_MakeFilling()
        explorer = TopExp_Explorer(wire, TopAbs_EDGE)
        edge_count = 0
        while explorer.More():
            filling.Add(TopoDS.Edge_s(explorer.Current()), GeomAbs_C0, True)
            edge_count += 1
            explorer.Next()
        if not edge_count:
            return None
        for point in constraints:
            filling.Add(gp_Pnt(*point))
        filling.Build()
        if filling.IsDone():
            return TopoDS.Face_s(filling.Shape())
    except Exception:
        return None
    return None


def _cap_face_for_wire(wire, anchor_points):
    """One cap face for a closed boundary wire: an exact planar face when the
    loop is planar, else a filled patch (anchored first, then unanchored)."""
    try:
        maker = BRepBuilderAPI_MakeFace(wire, True)
        if maker.IsDone():
            return maker.Face()
    except Exception:
        pass
    for constraints in ((anchor_points or []), []):
        face = _fill_wire(wire, constraints)
        if face is not None:
            return face
    return None


def _cap_faces_for_wires(wires, anchor_points=None) -> list:
    """One cap face per closed boundary wire. `anchor_points` pin filled
    patches to the region's own surface so non-planar loops cannot balloon."""
    caps = []
    for wire_index in range(1, wires.Length() + 1):
        wire = TopoDS.Wire_s(wires.Value(wire_index))
        face = _cap_face_for_wire(wire, anchor_points)
        if face is not None:
            caps.append(face)
    return caps


def _sample_wire_points(wire) -> list:
    """Sample a wire's edges into a chord polyline (8 samples/edge), honouring
    edge orientation so the loop stays consistently wound."""
    from OCP.BRepTools import BRepTools_WireExplorer
    from OCP.TopAbs import TopAbs_REVERSED

    points: list[np.ndarray] = []
    explorer = BRepTools_WireExplorer(wire)
    while explorer.More():
        edge = explorer.Current()
        curve = BRepAdaptor_Curve(edge)
        first, last = curve.FirstParameter(), curve.LastParameter()
        samples = 8
        params = [first + (last - first) * i / samples for i in range(samples)]
        if edge.Orientation() == TopAbs_REVERSED:
            params = [first + last - p for p in params]
        for parameter in params:
            value = curve.Value(parameter)
            points.append(np.array([value.X(), value.Y(), value.Z()]))
        explorer.Next()
    return points


def _fan_triangles(arr) -> list:
    """Triangulate a loop polyline as a fan from its centroid — degenerate and
    collinear slivers are skipped. Cannot leave the loop's convex hull."""
    from OCP.BRepBuilderAPI import BRepBuilderAPI_MakePolygon

    center = arr.mean(axis=0)
    faces = []
    for i in range(len(arr)):
        a, b = arr[i], arr[(i + 1) % len(arr)]
        if (np.linalg.norm(a - b) < 1e-9
                or np.linalg.norm(np.cross(a - center, b - center)) < 1e-12):
            continue
        try:
            polygon = BRepBuilderAPI_MakePolygon(
                gp_Pnt(*center), gp_Pnt(*a), gp_Pnt(*b), True
            )
            faces.append(BRepBuilderAPI_MakeFace(polygon.Wire()).Face())
        except Exception:
            continue
    return faces


def _fan_cap_faces(wires) -> list:
    """Last-resort caps: triangulate each boundary loop as a centroid fan —
    many small planar faces that cannot leave the loop's convex hull (smooth
    fillings balloon on L-shaped / slit loops; a fan cannot). The chord edges
    deviate from the exact curves by the sampling sag, so sewing needs a
    matching tolerance."""
    caps = []
    for wire_index in range(1, wires.Length() + 1):
        wire = TopoDS.Wire_s(wires.Value(wire_index))
        points = _sample_wire_points(wire)
        if len(points) < 3:
            continue
        caps.extend(_fan_triangles(np.asarray(points)))
    return caps


def _collect_shells(shape) -> list:
    from OCP.TopAbs import TopAbs_SHELL

    shells = []
    explorer = TopExp_Explorer(shape, TopAbs_SHELL)
    while explorer.More():
        shells.append(TopoDS.Shell_s(explorer.Current()))
        explorer.Next()
    return shells


def _sew_solid(faces: list, tolerance: float = 1.0e-6):
    """Sew faces into a closed shell and build a correctly oriented solid.
    None when they do not close up into exactly one solid."""
    from OCP.BRepBuilderAPI import BRepBuilderAPI_MakeSolid, BRepBuilderAPI_Sewing
    from OCP.ShapeFix import ShapeFix_Solid

    try:
        sewing = BRepBuilderAPI_Sewing(tolerance)
        for face in faces:
            sewing.Add(face)
        sewing.Perform()
        shells = _collect_shells(sewing.SewedShape())
        if not shells:
            return None
        maker = BRepBuilderAPI_MakeSolid()
        for shell in shells:
            maker.Add(shell)
        if not maker.IsDone():
            return None
        fix = ShapeFix_Solid(maker.Solid())
        fix.Perform()
        solids = list(_iter_solids(fix.Solid()))
        if len(solids) != 1:
            return None
        volume = shape_volume(solids[0])
        if volume is None or abs(volume) < 1.0e-12:
            return None
        return solids[0]
    except Exception:
        return None


# --------------------------------------------------- split_by_face_regions helpers

def _group_regions_by_body(regions, cuts) -> dict:
    """Group the previewed (body, face) regions by body, keeping each region's
    index so callers can link pins to the sealed solid by identity."""
    by_body: dict = {}
    for region_index, (region, cut) in enumerate(zip(regions, cuts)):
        if not region:
            continue
        groups: dict = {}
        for body_index, face_index in region:
            groups.setdefault(body_index, set()).add(face_index)
        for body_index, faces in groups.items():
            by_body.setdefault(body_index, []).append((region_index, faces, cut))
    return by_body


def _capture_colored_faces(body):
    """Coloured-face centroids + match tolerance: per-face colours can't follow
    a split directly (face ids change), so they are re-matched geometrically
    after remeshing. Returns (list of (centroid, rgb), tolerance)."""
    if body.mesh is None or not body.mesh.face_colors:
        return [], 0.0
    centers = body.mesh.points[body.mesh.tris].mean(axis=1)
    span = body.mesh.points.max(axis=0) - body.mesh.points.min(axis=0)
    tolerance = float(np.linalg.norm(span)) * 5.0e-3
    colored_faces = []
    for face_id, rgb in body.mesh.face_colors.items():
        mask = body.mesh.tri_face_ids == face_id
        if mask.any():
            colored_faces.append((centers[mask].mean(axis=0), rgb))
    return colored_faces, tolerance


def _region_area_bounds(mesh, region):
    """Mesh area and (min, max) corner of a face region — used to reject cap
    patches that balloon past the region they are supposed to close."""
    if mesh is None:
        return 0.0, None
    mask = np.isin(mesh.tri_face_ids, np.asarray(sorted(region), dtype=np.int32))
    corners = mesh.points[mesh.tris[mask]]
    if not len(corners):
        return 0.0, None
    area = float(0.5 * np.linalg.norm(np.cross(
        corners[:, 1] - corners[:, 0], corners[:, 2] - corners[:, 0],
    ), axis=1).sum())
    flat = corners.reshape(-1, 3)
    return area, (flat.min(axis=0), flat.max(axis=0))


def _solid_within_region_bounds(solid, region_bounds) -> bool:
    """A ballooned cap would corrupt the package too (both sides share it), so
    the sealed solid must stay near its region's bounds."""
    if region_bounds is None:
        return True
    low, high = region_bounds
    margin = 0.35 * float(np.linalg.norm(high - low)) + 1e-3
    sxmin, sxmax, symin, symax, szmin, szmax = shape_bounds([solid])
    return not (
        sxmin < low[0] - margin or sxmax > high[0] + margin
        or symin < low[1] - margin or symax > high[1] + margin
        or szmin < low[2] - margin or szmax > high[2] + margin
    )


def _cap_attempts(wires, anchors, fan_tolerance):
    """Cap strategies in order: exact filled patches first, then centroid-fan
    triangulation (cannot leave the loop hull) when those balloon past the
    guards. Each entry is (cap faces, sew tolerance)."""
    attempts = []
    filled = _cap_faces_for_wires(wires, anchor_points=anchors)
    if filled:
        attempts.append((filled, 1.0e-6))
    fan = _fan_cap_faces(wires)
    if fan:
        attempts.append((fan, fan_tolerance))
    return attempts


def _try_seal(faces, attempts, region_area, region_bounds):
    """First cap attempt that sews a solid staying near the region. Caps larger
    than the region (a billowing patch over a bent loop) are rejected."""
    for caps, sew_tolerance in attempts:
        cap_area = sum(shape_area(cap) or 0.0 for cap in caps)
        if region_area > 0.0 and cap_area > 1.2 * region_area:
            continue
        solid = _sew_solid(faces + caps, tolerance=sew_tolerance)
        if solid is None or not _solid_within_region_bounds(solid, region_bounds):
            continue
        return solid, caps, sew_tolerance
    return None


def _seal_one_region(body, region, face_map, edge_table, fan_tolerance):
    """Seal one previewed region into its own solid: its faces plus a cap over
    each boundary loop. Returns (solid, caps, sew_tolerance) or None."""
    wires = _boundary_wires(edge_table, region)
    if wires is None:
        return None
    region_area, region_bounds = _region_area_bounds(body.mesh, region)
    faces = [TopoDS.Face_s(face_map.FindKey(i)) for i in sorted(region)]
    anchors = _region_anchor_points(body.mesh, region) if body.mesh is not None else []
    attempts = _cap_attempts(wires, anchors, fan_tolerance)
    return _try_seal(faces, attempts, region_area, region_bounds)


def _seal_regions(body, region_sets, face_map, edge_table, progress):
    """Seal every region of one body; returns the sealed tuples that took."""
    fan_tolerance = max(1.0e-4, 2.0 * preview_deflection(shape_bounds([body.solid])))
    sealed = []
    for region_number, (region_index, region, _cut) in enumerate(region_sets):
        if progress is not None:
            progress("Sealing pin regions", region_number, len(region_sets))
        result = _seal_one_region(body, region, face_map, edge_table, fan_tolerance)
        if result is not None:
            solid, caps, tol = result
            sealed.append((region_index, region, solid, caps, tol))
    return sealed


def _volume_conserved(package, sealed, original_volume) -> bool:
    if original_volume <= 0.0:
        return True
    pieces = abs(shape_volume(package) or 0.0) + sum(
        abs(shape_volume(s) or 0.0) for _ri, _r, s, _c, _t in sealed
    )
    return abs(pieces - original_volume) <= 0.01 * original_volume


def _drop_largest_sealed(sealed) -> None:
    sealed.remove(max(sealed, key=lambda item: abs(shape_volume(item[2]) or 0.0)))


def _rebuild_package(sealed, face_map):
    """Sew the package from the unclaimed faces plus the SAME cap faces that
    sealed the pins. Returns (package|None, all_claimed): all_claimed flags the
    BGA case where one region swallowed the whole body, leaving no remainder."""
    claimed = set().union(*(region for _ri, region, _s, _c, _t in sealed))
    remainder = [i for i in range(1, face_map.Extent() + 1) if i not in claimed]
    if not remainder:
        return None, True
    package = _sew_solid(
        [TopoDS.Face_s(face_map.FindKey(i)) for i in remainder]
        + [cap for _ri, _r, _s, caps, _t in sealed for cap in caps],
        tolerance=max(tol for _ri, _r, _s, _c, tol in sealed),
    )
    return package, False


def _seal_package(body, sealed, face_map, progress):
    """Rebuild the package; the pieces must conserve the body's volume to 1%.
    When they don't (a false detection grown over the housing) the largest
    sealed region is dropped back into the package and we retry, rejecting junk
    regions one by one. Returns (package_solid|None, sealed) — sealed shrinks."""
    original_volume = abs(shape_volume(body.solid) or 0.0)
    package = None
    while sealed:
        if progress is not None:
            progress(f"Sealing {body.name} package", 0, 0)
        package, all_claimed = _rebuild_package(sealed, face_map)
        if all_claimed:
            _drop_largest_sealed(sealed)
            continue
        if package is not None and _volume_conserved(package, sealed, original_volume):
            break
        package = None
        _drop_largest_sealed(sealed)
    return package, sealed


def _emit_split_bodies(body, package, sealed, colored_faces, tolerance,
                       new_bodies, region_bodies, color_restores) -> None:
    """Append the package and each sealed pin as new BodyRecords, linking each
    region to its solid by identity and queueing colour re-attachment."""
    for index, solid in enumerate(
        [package] + [s for _ri, _region, s, _caps, _tol in sealed]
    ):
        record = BodyRecord(
            solid=solid, name=body.name, color=body.color,
            original_color=body.original_color,
        )
        if index != 0:
            region_bodies[sealed[index - 1][0]] = len(new_bodies)
        if colored_faces:
            color_restores.append((record, colored_faces, tolerance))
        new_bodies.append(record)


def _reattach_colors(color_restores) -> None:
    """Re-attach per-face colours: a new face whose centroid coincides with an
    old coloured face's centroid is that same face."""
    for record, colored_faces, tolerance in color_restores:
        mesh = record.mesh
        if mesh is None or not len(mesh.tris) or tolerance <= 0.0:
            continue
        centers = mesh.points[mesh.tris].mean(axis=1)
        anchors = np.array([c for c, _rgb in colored_faces])
        for face_id in np.unique(mesh.tri_face_ids):
            mask = mesh.tri_face_ids == face_id
            centroid = centers[mask].mean(axis=0)
            distances = np.linalg.norm(anchors - centroid, axis=1)
            nearest = int(np.argmin(distances))
            if distances[nearest] <= tolerance:
                mesh.face_colors[int(face_id)] = colored_faces[nearest][1]
        record.original_face_colors = dict(mesh.face_colors)


def _bodies_from_label(label, color_tool, fallback_name) -> list:
    """Build one BodyRecord per solid under a free-shape label (colours first,
    keyed by the original shape identity, then the location is baked in)."""
    shape = XCAFDoc_ShapeTool.GetShape_s(label)
    if shape is None or shape.IsNull():
        return []
    base_name = _label_name(label) or fallback_name
    # Open shells / loose faces: keep the shape as one body so it still renders.
    solids = list(_iter_solids(shape)) or [shape]
    bodies = []
    for index, solid in enumerate(solids):
        name = base_name if len(solids) == 1 else f"{base_name}.{index + 1}"
        color = _solid_color(color_tool, solid)
        face_colors = _harvest_face_colors(color_tool, solid)
        bodies.append(BodyRecord(
            solid=_bake_location(solid), name=name, color=color,
            original_color=color, original_face_colors=face_colors,
        ))
    return bodies


def _apply_original_face_colors(document) -> None:
    for body in document.bodies:
        if (body.mesh is not None and body.original_face_colors
                and body.mesh.face_count >= max(body.original_face_colors)):
            body.mesh.face_colors = dict(body.original_face_colors)


def _place_logo_tool(tool_shape, frame, offset_uv, raised, rotation_deg):
    """Place a logo tool (built at origin, extruded +Z) onto a face frame.
    Returns (placed_shape, origin, normal) — origin/normal feed colour reattach."""
    origin = np.asarray(frame["origin"], dtype=np.float64)
    n = np.asarray(frame["normal"], dtype=np.float64)
    u = np.asarray(frame["u"], dtype=np.float64)
    v = np.cross(n, u)
    origin = origin + u * offset_uv[0] + v * offset_uv[1]
    theta = math.radians(rotation_deg)
    u_eff = u * math.cos(theta) + v * math.sin(theta)
    target_z = n if raised else -n  # raised grows out, engrave sinks in
    ax3 = gp_Ax3(gp_Pnt(*origin), gp_Dir(*target_z), gp_Dir(*u_eff))
    trsf = gp_Trsf()
    trsf.SetTransformation(ax3, gp_Ax3())
    placed = BRepBuilderAPI_Transform(tool_shape, trsf, True).Shape()
    return placed, origin, n


def _capture_face_colors(body):
    """(face centroid array, [(centroid, rgb)]) so per-face colours can be
    re-matched after the boolean re-fids every face. ([]/None when uncoloured)."""
    if body.mesh is None:
        return None, []
    tri_centers = body.mesh.points[body.mesh.tris].mean(axis=1)
    face_ids = np.unique(body.mesh.tri_face_ids)
    old_centers = np.array([
        tri_centers[body.mesh.tri_face_ids == fid].mean(axis=0) for fid in face_ids
    ])
    old_colored = [
        (old_centers[i], body.mesh.face_colors[int(fid)])
        for i, fid in enumerate(face_ids)
        if int(fid) in body.mesh.face_colors
    ]
    return old_centers, old_colored


def _keep_meaningful_solids(solids, fallback):
    """Drop sub-tolerance debris slivers a shallow engrave fragments off; keep
    the meaningful pieces as one solid or a compound."""
    from OCP.BRep import BRep_Builder
    from OCP.TopoDS import TopoDS_Compound

    if len(solids) == 1:
        return solids[0]  # nothing to weigh against; skip the volume integral
    volumes = [abs(shape_volume(s) or 0.0) for s in solids]
    threshold = max(max(volumes) * 1.0e-5, 1.0e-9)
    keep = [s for s, vol in zip(solids, volumes) if vol > threshold]
    if len(keep) == 1:
        return keep[0]
    if not keep:
        return fallback
    builder = BRep_Builder()
    compound = TopoDS_Compound()
    builder.MakeCompound(compound)
    for solid in keep:
        builder.Add(compound, solid)
    return compound


def _fuzzy_boolean(operation, shape_a, shape_b, fuzzy: float):
    """Run a BRepAlgoAPI boolean with a fuzzy tolerance so near-coincident /
    coplanar faces are resolved instead of dropped."""
    args = TopTools_ListOfShape()
    args.Append(shape_a)
    tools = TopTools_ListOfShape()
    tools.Append(shape_b)
    operation.SetArguments(args)
    operation.SetTools(tools)
    if fuzzy > 0.0:
        operation.SetFuzzyValue(fuzzy)
    operation.Build()
    return operation.Shape()


def _collect_boxes(cuts) -> list:
    """The valid (non-degenerate) limit boxes of a list of cut dicts."""
    boxes = [tuple(float(v) for v in c["limits"]) for c in cuts]
    return [b for b in boxes if b[1] > b[0] and b[3] > b[2] and b[5] > b[4]]


def _boxes_union(boxes) -> tuple:
    """Axis-aligned bounding box (xmin,xmax,ymin,ymax,zmin,zmax) of all boxes."""
    return (min(b[0] for b in boxes), max(b[1] for b in boxes),
            min(b[2] for b in boxes), max(b[3] for b in boxes),
            min(b[4] for b in boxes), max(b[5] for b in boxes))


def _bbox_disjoint(bb, box) -> bool:
    """True when bounding box `bb` does not overlap `box` (both as
    xmin,xmax,ymin,ymax,zmin,zmax) — nothing to carve there."""
    return (bb[1] < box[0] or bb[0] > box[1] or bb[3] < box[2]
            or bb[2] > box[3] or bb[5] < box[4] or bb[4] > box[5])


def _bbox_in_any_box(sb, boxes, margin) -> bool:
    """True when bounding box `sb` (xmin,xmax,ymin,ymax,zmin,zmax) fits wholly
    inside any of the cut `boxes` (± margin) — i.e. the piece is a carved pin,
    not the package that pokes past a wall."""
    for xlo, xhi, ylo, yhi, zlo, zhi in boxes:
        if (sb[0] >= xlo - margin and sb[1] <= xhi + margin
                and sb[2] >= ylo - margin and sb[3] <= yhi + margin
                and sb[4] >= zlo - margin and sb[5] <= zhi + margin):
            return True
    return False


def _meaningful_solids(shape) -> list:
    """The non-debris solids of a boolean result, as a list (one body each)."""
    solids = list(_iter_solids(shape))
    if not solids:
        return []
    volumes = [abs(shape_volume(s) or 0.0) for s in solids]
    threshold = max(max(volumes) * 1.0e-5, 1.0e-9)
    return [s for s, vol in zip(solids, volumes) if vol > threshold]


def _logo_boolean_solid(body_solid, placed, raised):
    """Fuse (raised) or cut (engrave) the tool into the body; return the solid."""
    operation = BRepAlgoAPI_Fuse if raised else BRepAlgoAPI_Cut
    boolean = operation(body_solid, placed)
    boolean.Build()
    if not boolean.IsDone():
        raise RuntimeError("logo boolean failed")
    fixer = ShapeFix_Shape(boolean.Shape())
    fixer.Perform()
    result = fixer.Shape()
    solids = list(_iter_solids(result))
    if len(solids) <= 1:
        return solids[0] if solids else result
    return _keep_meaningful_solids(solids, result)


def _reattach_logo_colors(body, old_centers, old_colored, origin, n,
                          depth_mm, raised, logo_rgb) -> None:
    """Re-colour after the boolean re-fids faces. Logo faces (geometry created
    strictly off the surface plane) take logo_rgb; the cut-through original
    keeps its colour via a wider centroid re-match; everything else re-matches
    exactly. Caller guarantees old_centers and body.mesh are present."""
    span = body.mesh.points.max(axis=0) - body.mesh.points.min(axis=0)
    tolerance = float(np.linalg.norm(span)) * 2.0e-3
    side_tol = max(depth_mm * 0.05, 1e-6)
    direction = 1.0 if raised else -1.0
    colored_centers = (
        np.array([c for c, _rgb in old_colored]) if old_colored else None
    )

    def restore(fid, center, match_tol) -> None:
        if colored_centers is None:
            return
        distances = np.linalg.norm(colored_centers - center, axis=1)
        nearest = int(np.argmin(distances))
        if distances[nearest] <= match_tol:
            body.mesh.face_colors[int(fid)] = old_colored[nearest][1]

    tri_centers = body.mesh.points[body.mesh.tris].mean(axis=1)
    for fid in np.unique(body.mesh.tri_face_ids):
        center = tri_centers[body.mesh.tri_face_ids == fid].mean(axis=0)
        off_surface = float((center - origin) @ n) * direction > side_tol
        is_new = np.linalg.norm(old_centers - center, axis=1).min() > tolerance
        if is_new and off_surface:
            if logo_rgb is not None:
                body.mesh.face_colors[int(fid)] = tuple(logo_rgb)
        elif is_new:
            restore(fid, center, tolerance * 25.0)
        else:
            restore(fid, center, tolerance)


def _split_one_body_by_plane(body, tool_face, point, normal, eps) -> list:
    """Split one body by the plane; returns [(record, is_pin_side)]. A body
    that doesn't cross or doesn't split returns itself (not pin-side)."""
    mesh = body.mesh
    crosses = False
    if mesh is not None and len(mesh.points):
        distances = (mesh.points - point) @ normal
        crosses = distances.max() > eps and distances.min() < -eps
    if not crosses:
        return [(body, False)]
    splitter = BRepAlgoAPI_Splitter()
    arguments = TopTools_ListOfShape()
    arguments.Append(body.solid)
    tools = TopTools_ListOfShape()
    tools.Append(tool_face)
    splitter.SetArguments(arguments)
    splitter.SetTools(tools)
    splitter.Build()
    if not splitter.IsDone():
        return [(body, False)]
    solids = list(_iter_solids(splitter.Shape()))
    if len(solids) <= 1:
        return [(body, False)]
    out = []
    for solid in solids:
        box = Bnd_Box()
        BRepBndLib.Add_s(solid, box)
        bxmin, bymin, bzmin, bxmax, bymax, bzmax = box.Get()
        center = np.array(
            [(bxmin + bxmax) / 2, (bymin + bymax) / 2, (bzmin + bzmax) / 2]
        )
        pin_side = float((center - point) @ normal) > 0.0
        out.append((
            BodyRecord(solid=solid, name=body.name, color=body.color,
                       original_color=body.original_color),
            pin_side,
        ))
    return out


def _edge_face_pair(edge_faces, face_map, edge_index):
    """The two distinct body-face indices sharing an edge, or None."""
    faces = []
    for shape in edge_faces.FindFromIndex(edge_index):
        index = face_map.FindIndex(shape)
        if index > 0 and index not in faces:
            faces.append(index)
    return tuple(faces) if len(faces) == 2 else None


def _smooth_adjacency(solid, smooth_angle_deg) -> dict:
    """Build the face-adjacency graph: edges connect their two faces unless a
    threshold is set and the dihedral angle at the edge exceeds it."""
    face_map = TopTools_IndexedMapOfShape()
    TopExp.MapShapes_s(solid, TopAbs_FACE, face_map)
    edge_faces = TopTools_IndexedDataMapOfShapeListOfShape()
    TopExp.MapShapesAndAncestors_s(solid, TopAbs_EDGE, TopAbs_FACE, edge_faces)
    adjacency: dict[int, set[int]] = {
        i: set() for i in range(1, face_map.Extent() + 1)
    }
    for edge_index in range(1, edge_faces.Extent() + 1):
        pair = _edge_face_pair(edge_faces, face_map, edge_index)
        if pair is None:
            continue
        f1, f2 = pair
        if smooth_angle_deg is not None:
            angle = _edge_dihedral_deg(
                TopoDS.Edge_s(edge_faces.FindKey(edge_index)),
                TopoDS.Face_s(face_map.FindKey(f1)),
                TopoDS.Face_s(face_map.FindKey(f2)),
            )
            if angle is None or angle > smooth_angle_deg:
                continue
        adjacency[f1].add(f2)
        adjacency[f2].add(f1)
    return adjacency


def tessellate_body(
    solid: TopoDS_Shape,
    deflection: float,
    color_tool=None,
) -> BodyMesh:
    """Mesh one solid and keep the triangle → B-rep-face mapping.

    Face ids are 1-based indices into the body's TopTools face map, which is
    deterministic for an unmodified solid, so picks can be resolved back to
    the exact TopoDS_Face later.
    """
    BRepMesh_IncrementalMesh(solid, deflection, False, 0.5, True)
    face_map = TopTools_IndexedMapOfShape()
    TopExp.MapShapes_s(solid, TopAbs_FACE, face_map)

    point_blocks: list[np.ndarray] = []
    tri_blocks: list[np.ndarray] = []
    fid_blocks: list[np.ndarray] = []
    face_colors: dict[int, tuple[float, float, float]] = {}
    offset = 0

    for face_index in range(1, face_map.Extent() + 1):
        face = TopoDS.Face_s(face_map.FindKey(face_index))
        loc = TopLoc_Location()
        triangulation = BRep_Tool.Triangulation_s(face, loc)
        if triangulation is None:
            continue
        trsf = loc.Transformation()

        node_count = triangulation.NbNodes()
        points = np.empty((node_count, 3), dtype=np.float64)
        for i in range(1, node_count + 1):
            p = triangulation.Node(i).Transformed(trsf)
            points[i - 1] = (p.X(), p.Y(), p.Z())

        tri_count = triangulation.NbTriangles()
        reversed_face = face.Orientation() == TopAbs_REVERSED
        tris = np.empty((tri_count, 3), dtype=np.int64)
        for i in range(1, tri_count + 1):
            tri = triangulation.Triangle(i)
            i1, i2, i3 = tri.Value(1), tri.Value(2), tri.Value(3)
            if reversed_face:
                i2, i3 = i3, i2
            tris[i - 1] = (i1 - 1 + offset, i2 - 1 + offset, i3 - 1 + offset)

        point_blocks.append(points)
        tri_blocks.append(tris)
        fid_blocks.append(np.full(tri_count, face_index, dtype=np.int32))
        offset += node_count

        if color_tool is not None:
            rgb = _shape_rgb(color_tool, face)
            if rgb is not None:
                face_colors[face_index] = rgb

    if not point_blocks:
        return BodyMesh(
            points=np.empty((0, 3), dtype=np.float64),
            tris=np.empty((0, 3), dtype=np.int64),
            tri_face_ids=np.empty(0, dtype=np.int32),
            face_count=face_map.Extent(),
        )
    return BodyMesh(
        points=np.concatenate(point_blocks),
        tris=np.concatenate(tri_blocks),
        tri_face_ids=np.concatenate(fid_blocks),
        face_count=face_map.Extent(),
        face_colors=face_colors,
    )


def _edge_dihedral_deg(edge, face1, face2) -> float | None:
    """Angle between the two faces' outward normals at the edge midpoint.
    None means the normal could not be evaluated (treated as sharp)."""
    try:
        curve = BRepAdaptor_Curve(edge)
        t = 0.5 * (curve.FirstParameter() + curve.LastParameter())
        point = curve.Value(t)
        normals = []
        for face in (face1, face2):
            surface = BRep_Tool.Surface_s(face)
            projector = GeomAPI_ProjectPointOnSurf(point, surface)
            if projector.NbPoints() < 1:
                return None
            u, v = projector.LowerDistanceParameters()
            props = GeomLProp_SLProps(surface, u, v, 1, 1.0e-6)
            if not props.IsNormalDefined():
                return None
            direction = props.Normal()
            normal = np.array([direction.X(), direction.Y(), direction.Z()])
            if face.Orientation() == TopAbs_REVERSED:
                normal = -normal
            normals.append(normal)
        cosine = float(np.clip(np.dot(normals[0], normals[1]), -1.0, 1.0))
        return math.degrees(math.acos(cosine))
    except Exception:
        return None


def _carry_face_colors(previous: BodyMesh | None, current: BodyMesh) -> None:
    """Re-tessellation drops XCAF colour lookups (the source document is
    gone), but face ids are stable across rigid transforms, so reuse the
    colours harvested at load time."""
    if previous is None or current.face_colors:
        return
    if previous.face_count == current.face_count:
        current.face_colors = dict(previous.face_colors)


def _read_file_schema(path: Path) -> str:
    try:
        head = path.read_text(encoding="utf-8", errors="replace")[:4096]
        match = re.search(r"FILE_SCHEMA\s*\(\s*\(\s*'([^']+)'", head)
        if match:
            return match.group(1)
    except OSError:
        pass
    return "?"


class EditorDocument:
    """Holds the live B-rep state: a flat table of solids (the source of
    truth for all edits) plus the meshes used for rendering and picking."""

    def __init__(self, path: Path, bodies: list[BodyRecord], schema: str) -> None:
        self.path = path
        self.bodies = bodies
        self.schema = schema
        self._xcaf_doc = None  # kept alive: the color tool dangles if the doc is GC'd

    # ------------------------------------------------------------------ load

    @classmethod
    def load(cls, path: str | Path) -> "EditorDocument":
        path = Path(path)
        doc = TDocStd_Document(TCollection_ExtendedString("MDTV-XCAF"))
        XCAFApp_Application.GetApplication_s().InitDocument(doc)

        reader = STEPCAFControl_Reader()
        reader.SetColorMode(True)
        reader.SetNameMode(True)
        status = reader.ReadFile(str(path))
        if status != IFSelect_RetDone:
            raise RuntimeError(f"STEP read failed for {path}")
        if not reader.Transfer(doc):
            raise RuntimeError(f"STEP transfer failed for {path}")

        main = doc.Main()
        shape_tool = XCAFDoc_DocumentTool.ShapeTool_s(main)
        color_tool = XCAFDoc_DocumentTool.ColorTool_s(main)
        labels = TDF_LabelSequence()
        shape_tool.GetFreeShapes(labels)

        bodies: list[BodyRecord] = []
        for li in range(1, labels.Length() + 1):
            bodies.extend(_bodies_from_label(labels.Value(li), color_tool, path.stem))

        if not bodies:
            raise RuntimeError(f"No shapes found in {path}")

        document = cls(path=path, bodies=bodies, schema=_read_file_schema(path))
        document._xcaf_doc = doc
        document.remesh_all()
        _apply_original_face_colors(document)
        return document

    # ------------------------------------------------------------ tessellate

    def remesh_all(self, color_tool=None, progress=None) -> None:
        self._adjacency_cache = {}  # body table changed: adjacency is stale
        deflection = preview_deflection(self.bounds())
        total = len(self.bodies)
        for index, body in enumerate(self.bodies):
            if progress is not None:
                progress("Meshing bodies", index, total)
            previous = body.mesh
            body.mesh = tessellate_body(body.solid, deflection, color_tool)
            _carry_face_colors(previous, body.mesh)

    def remesh_body(self, body_index: int) -> None:
        deflection = preview_deflection(self.bounds())
        body = self.bodies[body_index]
        previous = body.mesh
        body.mesh = tessellate_body(body.solid, deflection)
        _carry_face_colors(previous, body.mesh)

    # ------------------------------------------------------------- mutation

    def apply_trsf(self, matrix) -> None:
        """Apply a rigid 4x4 transform (row-major, numpy or nested lists) to
        every body. Topology is preserved, so face ids — and therefore pin
        face references and face colours — stay valid."""
        m = np.asarray(matrix, dtype=np.float64)
        if m.shape != (4, 4):
            raise ValueError("apply_trsf expects a 4x4 matrix")
        trsf = gp_Trsf()
        trsf.SetValues(
            m[0, 0], m[0, 1], m[0, 2], m[0, 3],
            m[1, 0], m[1, 1], m[1, 2], m[1, 3],
            m[2, 0], m[2, 1], m[2, 2], m[2, 3],
        )
        for body in self.bodies:
            builder = BRepBuilderAPI_Transform(body.solid, trsf, True)
            body.solid = builder.Shape()
        self.remesh_all()

    # ----------------------------------------------------------------- query

    def bounds(self) -> tuple[float, float, float, float, float, float]:
        return shape_bounds(body.solid for body in self.bodies)

    def face_of(self, body_index: int, face_index: int) -> TopoDS_Shape:
        face_map = TopTools_IndexedMapOfShape()
        TopExp.MapShapes_s(self.bodies[body_index].solid, TopAbs_FACE, face_map)
        return TopoDS.Face_s(face_map.FindKey(face_index))

    def split_by_plane(self, point, normal) -> list[int]:
        """Split every body that crosses the plane into separate solids
        (BRepAlgoAPI_Splitter with a large planar face). Returns the indices
        of the NEW bodies on the positive-normal side of the plane (the pin
        side). Bodies that don't cross are kept as-is."""
        point = np.asarray(point, dtype=np.float64)
        normal = np.asarray(normal, dtype=np.float64)
        normal = normal / max(float(np.linalg.norm(normal)), 1.0e-12)
        xmin, xmax, ymin, ymax, zmin, zmax = self.bounds()
        size = max(
            float(np.linalg.norm([xmax - xmin, ymax - ymin, zmax - zmin])), 1.0
        ) * 2.0
        eps = size * 1.0e-9

        plane = gp_Pln(gp_Pnt(*point), gp_Dir(*normal))
        tool_face = BRepBuilderAPI_MakeFace(plane, -size, size, -size, size).Face()

        new_bodies: list[BodyRecord] = []
        pin_side_indices: list[int] = []
        for body in self.bodies:
            for record, pin_side in _split_one_body_by_plane(
                body, tool_face, point, normal, eps
            ):
                if pin_side:
                    pin_side_indices.append(len(new_bodies))
                new_bodies.append(record)

        self.bodies = new_bodies
        self.remesh_all()
        return pin_side_indices

    def _box_cut_tool(self, limits, normal, location, pad: float = 0.0):
        """The boolean tool solid for a box-bounded cut: the limits box
        (expanded outward by `pad` so its faces don't sit exactly on model
        faces — coplanar faces then fall clearly INSIDE the pin), optionally
        clipped to the +normal side of the plane through unit_normal*location."""
        xlo, xhi, ylo, yhi, zlo, zhi = (float(v) for v in limits)
        xlo -= pad; ylo -= pad; zlo -= pad
        xhi += pad; yhi += pad; zhi += pad
        tool = BRepPrimAPI_MakeBox(
            gp_Pnt(xlo, ylo, zlo), xhi - xlo, yhi - ylo, zhi - zlo
        ).Solid()
        if normal is None or location is None:
            return tool
        unit = np.asarray(normal, dtype=float)
        magnitude = float(np.linalg.norm(unit))
        if magnitude <= 1e-9:
            return tool
        unit = unit / magnitude
        point = unit * float(location)
        big = max(xhi - xlo, yhi - ylo, zhi - zlo) * 10.0 + 1.0
        plane = gp_Pln(gp_Pnt(*point), gp_Dir(*unit))
        face = BRepBuilderAPI_MakeFace(plane, -big, big, -big, big).Face()
        # MakeHalfSpace keeps material on the SAME side as the reference point
        # (verified empirically), so put the reference on the +normal side.
        half = BRepPrimAPI_MakeHalfSpace(face, gp_Pnt(*(point + unit * big))).Solid()
        clipped = BRepAlgoAPI_Common(tool, half).Shape()
        return clipped if list(_iter_solids(clipped)) else tool

    def split_by_box(self, limits, normal=None, location=None,
                     remesh: bool = True) -> list[int]:
        """Box-bounded boolean cut — the manual Cut Plane sub-mode. `limits` =
        (xlo, xhi, ylo, yhi, zlo, zhi); an optional plane (`normal`/`location`)
        clips the box to its +normal side. The carved piece = the part of a
        solid INSIDE the box (on the +normal side); everything else stays the
        package, untouched. Cuts straight THROUGH faces (unlike the face-region
        sealer), so a pin whose junction with the body runs mid-face still
        separates. Returns the indices, in the final body table, of the freshly
        carved pieces (callers link pins to them).

        The cut works purely on solids; meshes are only needed downstream. Pass
        `remesh=False` to skip the (expensive) re-tessellation when carving many
        boxes in a row — the caller MUST then call `remesh_all()` once before
        any mesh consumer (preview, pin re-linking) runs. Remeshing per call
        is O(bodies²) over a batch; deferring it makes the batch O(bodies)."""
        xlo, xhi, ylo, yhi, zlo, zhi = (float(v) for v in limits)
        if xhi <= xlo or yhi <= ylo or zhi <= zlo:
            return []
        bounds = self.bounds()
        diag = float(np.linalg.norm([bounds[1] - bounds[0], bounds[3] - bounds[2],
                                     bounds[5] - bounds[4]]))
        fuzzy = max(diag * 1.0e-5, 1.0e-7)  # tolerate near-coincident faces
        # The EXACT box (no outward pad): BRepAlgoAPI_Splitter divides the solid
        # by the box's faces in ONE pass, duplicating the shared cut plane onto
        # BOTH pieces. So a model face flush with a box wall ends up on whichever
        # piece its material backs — outward-normal faces stay with the pin,
        # inward-normal faces stay with the package — which is exactly the
        # in/out/both rule, resolved by the kernel instead of by a heuristic pad.
        tool = self._box_cut_tool(limits, normal, location, pad=0.0)
        unit = None
        if normal is not None and location is not None:
            vec = np.asarray(normal, dtype=float)
            magnitude = float(np.linalg.norm(vec))
            if magnitude > 1e-9:
                unit = vec / magnitude
        margin = max(diag * 1.0e-4, 1.0e-6)  # tolerance for "centroid in box"
        new_bodies: list[BodyRecord] = []
        carved: list[int] = []
        for body in self.bodies:
            bxmin, bxmax, bymin, bymax, bzmin, bzmax = shape_bounds([body.solid])
            disjoint = (bxmax < xlo or bxmin > xhi or bymax < ylo
                        or bymin > yhi or bzmax < zlo or bzmin > zhi)
            pieces = self._split_solid_by_tool(body.solid, tool, fuzzy) \
                if not disjoint else []
            pin_solids: list = []
            rem_solids: list = []
            for solid in pieces:
                # The splitter divides pieces EXACTLY along the box walls, so an
                # inside piece's whole bounding box lies within the box (± fuzzy
                # margin) and an outside piece pokes past a wall. A bbox test is
                # equivalent to the old volume-centroid test here but ~10x
                # cheaper — the centroid integral on the big package piece was
                # the batch's quadratic-feeling cost. The exact mass centroid is
                # only needed for the optional plane clip (box-cut mode never
                # sets one), so compute it lazily for that case alone.
                sxmin, sxmax, symin, symax, szmin, szmax = shape_bounds([solid])
                inside = (sxmin >= xlo - margin and sxmax <= xhi + margin
                          and symin >= ylo - margin and symax <= yhi + margin
                          and szmin >= zlo - margin and szmax <= zhi + margin)
                if inside and unit is not None:
                    centre = solid_centroid(solid)
                    inside = float(centre @ unit) >= float(location) - margin
                (pin_solids if inside else rem_solids).append(solid)
            if not pin_solids or not rem_solids:
                new_bodies.append(body)  # box missed it, or no clean split
                continue
            package = _keep_meaningful_solids(rem_solids, body.solid)
            new_bodies.append(BodyRecord(
                solid=package, name=body.name, color=body.color,
                original_color=body.original_color))
            for solid in pin_solids:
                carved.append(len(new_bodies))
                new_bodies.append(BodyRecord(
                    solid=solid, name=body.name, color=body.color,
                    original_color=body.original_color))
        self.bodies = new_bodies
        if remesh:
            self.remesh_all()
        return carved

    def split_by_boxes(self, cuts, remesh: bool = True) -> list[int]:
        """Carve MANY pure-box cuts in ONE boolean per body — the batch form of
        split_by_box. `cuts` is a list of {"limits": (xlo..zhi)} dicts (no plane
        clip). Carving the boxes one at a time re-splits the whole (growing)
        package on every cut, which is O(cuts²) in the package's face count and
        is what made a 288-pin connector hang for minutes; imprinting all box
        tools in a single Splitter.Build divides each solid once — O(cuts).
        Returns the carved-piece indices in the new body table, like
        split_by_box. Pass remesh=False to defer re-tessellation to the caller."""
        boxes = _collect_boxes(cuts)
        if not boxes:
            return []
        bounds = self.bounds()
        diag = float(np.linalg.norm([bounds[1] - bounds[0], bounds[3] - bounds[2],
                                     bounds[5] - bounds[4]]))
        fuzzy = max(diag * 1.0e-5, 1.0e-7)
        margin = max(diag * 1.0e-4, 1.0e-6)
        tools = [self._box_cut_tool(b, None, None, pad=0.0) for b in boxes]
        union = _boxes_union(boxes)
        new_bodies: list[BodyRecord] = []
        carved: list[int] = []
        for body in self.bodies:
            pieces = [] if _bbox_disjoint(shape_bounds([body.solid]), union) \
                else self._split_solid_by_tools(body.solid, tools, fuzzy)
            if len(pieces) < 2:
                new_bodies.append(body)  # box missed it, or no clean split
                continue
            self._emit_box_pieces(body, pieces, boxes, margin, new_bodies, carved)
        self.bodies = new_bodies
        if remesh:
            self.remesh_all()
        return carved

    def _emit_box_pieces(self, body, pieces, boxes, margin, new_bodies, carved):
        """Classify a split body's pieces (a piece whose whole bbox sits inside
        some box is a carved pin; the rest is package), then append the package
        and each pin to `new_bodies`, recording pin indices in `carved`."""
        pin_solids, rem_solids = [], []
        for solid in pieces:
            sb = shape_bounds([solid])
            (pin_solids if _bbox_in_any_box(sb, boxes, margin)
             else rem_solids).append(solid)
        if not pin_solids or not rem_solids:
            new_bodies.append(body)  # nothing cleanly carved
            return
        package = _keep_meaningful_solids(rem_solids, body.solid)
        new_bodies.append(BodyRecord(
            solid=package, name=body.name, color=body.color,
            original_color=body.original_color))
        for solid in pin_solids:
            carved.append(len(new_bodies))
            new_bodies.append(BodyRecord(
                solid=solid, name=body.name, color=body.color,
                original_color=body.original_color))

    def _split_solid_by_tools(self, solid, tools, fuzzy: float) -> list:
        """Split `solid` by SEVERAL tools in one Splitter.Build. Returns the
        meaningful solids of the result, or [] if it did not divide."""
        splitter = BRepAlgoAPI_Splitter()
        arguments = TopTools_ListOfShape()
        arguments.Append(solid)
        tool_list = TopTools_ListOfShape()
        for tool in tools:
            tool_list.Append(tool)
        splitter.SetArguments(arguments)
        splitter.SetTools(tool_list)
        if fuzzy > 0.0:
            splitter.SetFuzzyValue(fuzzy)
        splitter.Build()
        if not splitter.IsDone():
            return []
        return _meaningful_solids(splitter.Shape())

    def _split_solid_by_tool(self, solid, tool, fuzzy: float) -> list:
        """Split `solid` by `tool` with BRepAlgoAPI_Splitter (one boolean; the
        shared cut faces are duplicated onto both resulting pieces). Returns the
        meaningful solids of the result, or [] if it did not divide."""
        splitter = BRepAlgoAPI_Splitter()
        arguments = TopTools_ListOfShape()
        arguments.Append(solid)
        tools = TopTools_ListOfShape()
        tools.Append(tool)
        splitter.SetArguments(arguments)
        splitter.SetTools(tools)
        if fuzzy > 0.0:
            splitter.SetFuzzyValue(fuzzy)
        splitter.Build()
        if not splitter.IsDone():
            return []
        solids = _meaningful_solids(splitter.Shape())
        return solids if len(solids) >= 2 else []

    def split_by_face_regions(
        self, regions: list[list[tuple[int, int]]], cuts=None, progress=None
    ) -> list[int | None]:
        """Split bodies into EXACTLY the previewed pin regions: each region's
        own faces are closed with caps over its boundary loops and sewn into
        a solid, and the package is rebuilt the same way from the remaining
        faces plus the same boundary caps. No volume splitter is involved, so
        the result cannot diverge from the preview, and contacts that touch
        the housing along their whole length still sever. A body is left
        untouched unless the rebuilt pieces conserve its volume to 1%.
        Returns, PER INPUT REGION, the new body index of its sealed solid
        (None where the region did not separate) — callers link pins to
        pieces by identity, never by guessing from centroids."""
        cuts = cuts if cuts is not None else [None] * len(regions)
        region_bodies: list[int | None] = [None] * len(regions)
        by_body = _group_regions_by_body(regions, cuts)

        new_bodies: list[BodyRecord] = []
        color_restores: list[tuple[BodyRecord, list, float]] = []
        for body_index, body in enumerate(self.bodies):
            region_sets = by_body.get(body_index)
            if not region_sets:
                new_bodies.append(body)
                continue

            colored_faces, tolerance = _capture_colored_faces(body)
            face_map = TopTools_IndexedMapOfShape()
            TopExp.MapShapes_s(body.solid, TopAbs_FACE, face_map)
            edge_table = _edge_face_table(body.solid)

            sealed = _seal_regions(body, region_sets, face_map, edge_table, progress)
            if not sealed:
                new_bodies.append(body)
                continue
            package, sealed = _seal_package(body, sealed, face_map, progress)
            if package is None or not sealed:
                new_bodies.append(body)
                continue
            _emit_split_bodies(body, package, sealed, colored_faces, tolerance,
                               new_bodies, region_bodies, color_restores)

        self.bodies = new_bodies
        self.remesh_all(progress=progress)
        _reattach_colors(color_restores)
        return region_bodies

    def face_plane(self, body_index: int, face_index: int) -> dict | None:
        """Frame of a planar face: origin (face centre), outward normal, and
        in-plane u/v axes. None for non-planar faces."""
        face = self.face_of(body_index, face_index)
        adaptor = BRepAdaptor_Surface(face)
        if adaptor.GetType() != GeomAbs_Plane:
            return None
        plane = adaptor.Plane()
        normal = plane.Axis().Direction()
        n = np.array([normal.X(), normal.Y(), normal.Z()])
        if face.Orientation() == TopAbs_REVERSED:
            n = -n
        u_dir = plane.Position().XDirection()
        u = np.array([u_dir.X(), u_dir.Y(), u_dir.Z()])
        v = np.cross(n, u)
        mesh = self.bodies[body_index].mesh
        mask = mesh.tri_face_ids == face_index
        origin = mesh.points[np.unique(mesh.tris[mask])].mean(axis=0)
        return {"origin": origin, "normal": n, "u": u, "v": v}

    def nearest_linear_edge(self, body_index: int, point):
        """The straight (line) edge of a body nearest a clicked point, returned
        as (p1, p2) endpoints — lets a tool 'pick a line that exists on the
        model'. None when the body has no linear edge."""
        from OCP.BRepAdaptor import BRepAdaptor_Curve
        from OCP.GeomAbs import GeomAbs_Line
        from OCP.TopAbs import TopAbs_EDGE
        from OCP.TopoDS import TopoDS

        if not (0 <= body_index < len(self.bodies)):
            return None
        point = np.asarray(point, dtype=np.float64)
        best, best_distance = None, None
        explorer = TopExp_Explorer(self.bodies[body_index].solid, TopAbs_EDGE)
        while explorer.More():
            edge = TopoDS.Edge_s(explorer.Current())
            explorer.Next()
            curve = BRepAdaptor_Curve(edge)
            if curve.GetType() != GeomAbs_Line:
                continue
            start = curve.Value(curve.FirstParameter())
            end = curve.Value(curve.LastParameter())
            a = np.array([start.X(), start.Y(), start.Z()])
            b = np.array([end.X(), end.Y(), end.Z()])
            ab = b - a
            t = float(np.clip(np.dot(point - a, ab) / max(float(ab @ ab), 1e-12), 0.0, 1.0))
            distance = float(np.linalg.norm(point - (a + t * ab)))
            if best_distance is None or distance < best_distance:
                best_distance, best = distance, (tuple(a), tuple(b))
        return best

    def face_frame_at(self, body_index: int, face_index: int, point) -> dict | None:
        """Embossing frame at `point` on a face: the face's own plane when it
        is planar, else the LOCAL TANGENT plane at the nearest surface
        location — so drafted housings, cylinders, and B-spline faces take
        the logo as a projected stamp (deepest at the picked point, shallower
        where the surface curves away). None only when the surface has no
        defined normal there."""
        frame = self.face_plane(body_index, face_index)
        if frame is not None:
            return frame
        from OCP.BRep import BRep_Tool
        from OCP.GeomLProp import GeomLProp_SLProps
        from OCP.ShapeAnalysis import ShapeAnalysis_Surface

        face = self.face_of(body_index, face_index)
        surface = BRep_Tool.Surface_s(face)
        if surface is None:
            return None
        uv = ShapeAnalysis_Surface(surface).ValueOfUV(
            gp_Pnt(*np.asarray(point, dtype=np.float64)), 1.0e-6
        )
        props = GeomLProp_SLProps(surface, uv.X(), uv.Y(), 1, 1.0e-9)
        if not props.IsNormalDefined():
            return None
        normal = props.Normal()
        n = np.array([normal.X(), normal.Y(), normal.Z()])
        if face.Orientation() == TopAbs_REVERSED:
            n = -n
        value = props.Value()
        origin = np.array([value.X(), value.Y(), value.Z()])
        d1u = props.D1U()
        u = np.array([d1u.X(), d1u.Y(), d1u.Z()])
        u = u - (u @ n) * n
        norm = float(np.linalg.norm(u))
        if norm < 1.0e-9:
            u = np.cross(n, [0.0, 0.0, 1.0])
            if np.linalg.norm(u) < 1.0e-6:
                u = np.cross(n, [0.0, 1.0, 0.0])
            norm = float(np.linalg.norm(u))
        u = u / norm
        v = np.cross(n, u)
        return {"origin": origin, "normal": n, "u": u, "v": v}

    def emboss(
        self,
        body_index: int,
        tool_shape,
        frame: dict,
        *,
        depth_mm: float,
        offset_uv: tuple[float, float] = (0.0, 0.0),
        raised: bool = False,
        logo_rgb: tuple[float, float, float] | None = None,
        rotation_deg: float = 0.0,
    ) -> float:
        """Place a logo tool (built at origin, extruded +Z over [0, depth])
        onto the planar face frame and boolean it into the body. Engrave
        sinks the tool below the surface and cuts; raised fuses it on top.
        Returns the signed volume change."""
        placed, origin, n = _place_logo_tool(
            tool_shape, frame, offset_uv, raised, rotation_deg
        )
        body = self.bodies[body_index]
        before = shape_volume(body.solid) or 0.0
        old_centers, old_colored = _capture_face_colors(body)
        body.solid = _logo_boolean_solid(body.solid, placed, raised)
        self.remesh_body(body_index)
        after = shape_volume(body.solid) or 0.0

        if old_centers is not None and body.mesh is not None and len(old_centers):
            _reattach_logo_colors(body, old_centers, old_colored, origin, n,
                                  depth_mm, raised, logo_rgb)
        return after - before

    def face_smooth_adjacency(
        self, body_index: int, smooth_angle_deg: float | None = 30.0
    ) -> dict[int, set[int]]:
        """Face adjacency graph of one body. With a threshold, only edges
        whose dihedral angle (surface normals sampled at the shared edge
        midpoint) is below it are kept — region growing 'flows' across those
        and stops at discontinuities. With None, every shared edge connects.

        Cached per (body, angle) until the body table changes: building the
        graph costs ~8s on a DDR5-class solid and grow/detect call it
        repeatedly (the factor sweep alone used to pay it eight times)."""
        cache = getattr(self, "_adjacency_cache", None)
        if cache is None:
            cache = self._adjacency_cache = {}
        key = (body_index, smooth_angle_deg)
        cached = cache.get(key)
        if cached is not None:
            return cached
        adjacency = _smooth_adjacency(self.bodies[body_index].solid, smooth_angle_deg)
        cache[key] = adjacency
        return adjacency

    def info(self) -> DocumentInfo:
        faces = sum(_count(body.solid, TopAbs_FACE) for body in self.bodies)
        edges = sum(_count(body.solid, TopAbs_EDGE) for body in self.bodies)
        vertices = sum(_count(body.solid, TopAbs_VERTEX) for body in self.bodies)
        return DocumentInfo(
            path=self.path,
            schema=self.schema,
            body_count=len(self.bodies),
            face_count=faces,
            edge_count=edges,
            vertex_count=vertices,
            bounds=self.bounds(),
        )
