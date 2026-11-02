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
from OCP.BRepPrimAPI import BRepPrimAPI_MakeBox, BRepPrimAPI_MakeCylinder, BRepPrimAPI_MakeHalfSpace
from OCP.GeomAbs import GeomAbs_Plane
from OCP.ShapeFix import ShapeFix_Shape
from OCP.BRepBndLib import BRepBndLib
from OCP.BRepBuilderAPI import BRepBuilderAPI_MakeFace, BRepBuilderAPI_Transform
from OCP.GeomAPI import GeomAPI_ProjectPointOnSurf
from OCP.GeomLProp import GeomLProp_SLProps
from OCP.gp import gp_Ax2, gp_Ax3, gp_Dir, gp_Pln, gp_Pnt, gp_Trsf
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
    TopAbs_IN,
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
    """Surface color first, then generic color. Guarded: GetColor raises on
    unlabelled shapes in some OCP builds."""
    if color_tool is None:
        return None
    col = Quantity_Color()
    # An assembly's placed copy carries a location, while XCAF keys the color
    # by the part's own (unlocated) shape: OCC's IsSame compares locations too,
    # so a vendor model built as an assembly read as all default grey. Retry
    # with the location stripped.
    candidates = [shape]
    if not shape.Location().IsIdentity():
        candidates.append(shape.Located(TopLoc_Location()))
    for candidate in candidates:
        for ctype in (XCAFDoc_ColorType.XCAFDoc_ColorSurf, XCAFDoc_ColorType.XCAFDoc_ColorGen):
            try:
                if color_tool.GetColor(candidate, ctype, col):
                    return (col.Red(), col.Green(), col.Blue())
            except Exception:
                continue
    return None


def _harvest_face_colors(color_tool, solid) -> dict[int, tuple[float, float, float]]:
    """Per-face colors keyed by face-map index. Must run BEFORE the solid's
    location is baked — the XCAF color tool keys by exact shape identity."""
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
    sub-shape (face color) export — located shapes register as references,
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


# Per-shape bounding boxes, keyed by hash(shape) (which tracks the shape and
# its location, so a moved body gets a fresh box). A 574-body connector paid
# ~4 s per document.bounds() call before this; most calls now cost a lookup.
_BOUNDS_CACHE: dict[int, tuple] = {}
_BOUNDS_CACHE_SIZE = 8192


def clear_bounds_cache() -> None:
    """Meshing (BRepMesh) edits a shape in place without changing its hash,
    and BRepBndLib reads the mesh, so cached boxes go stale: call after it."""
    _BOUNDS_CACHE.clear()


def _one_shape_bounds(shape):
    """((xmin, ymin, zmin, xmax, ymax, zmax) without the tolerance gap, gap)
    of one shape, or None if void."""
    key = hash(shape)
    hit = _BOUNDS_CACHE.get(key)
    if hit is not None and hit[0].IsEqual(shape):
        return hit[1]
    box = Bnd_Box()
    BRepBndLib.Add_s(shape, box)
    found = None
    if not box.IsVoid():
        gap = box.GetGap()
        xmin, ymin, zmin, xmax, ymax, zmax = box.Get()
        found = ((xmin + gap, ymin + gap, zmin + gap, xmax - gap, ymax - gap, zmax - gap), gap)
    if len(_BOUNDS_CACHE) >= _BOUNDS_CACHE_SIZE:
        _BOUNDS_CACHE.clear()
    _BOUNDS_CACHE[key] = (shape, found)
    return found


def shape_bounds(shapes) -> tuple[float, float, float, float, float, float]:
    """Same box as one Bnd_Box over all shapes: the union, widened by the
    largest tolerance gap (Bnd_Box keeps one gap, the max)."""
    found = [f for f in (_one_shape_bounds(shape) for shape in shapes) if f is not None]
    if not found:
        return (-0.5, 0.5, -0.5, 0.5, -0.5, 0.5)
    corners = np.array([f[0] for f in found], dtype=float)
    gap = max(f[1] for f in found)
    xmin, ymin, zmin = corners[:, :3].min(axis=0) - gap
    xmax, ymax, zmax = corners[:, 3:].max(axis=0) + gap
    return (float(xmin), float(xmax), float(ymin), float(ymax), float(zmin), float(zmax))


_UNIT_PREFIX = {"MILLI": "mm", "CENTI": "cm", "MICRO": "um", "DECI": "dm", "KILO": "km"}


def step_length_units(path) -> str | None:
    """The STEP file's length unit read straight from its text ("mm", "in",
    ...), or None. Replaces a geometer subprocess that re-read the whole
    model (50 s on a 574-body connector) just to learn this."""
    import re

    try:
        text = Path(path).read_bytes().decode("latin-1")
    except OSError:
        return None
    for entity in re.findall(r"=\s*\(([^;]*?LENGTH_UNIT[^;]*?)\)\s*;", text):
        named = re.search(r"CONVERSION_BASED_UNIT\s*\(\s*'([A-Za-z]+)'", entity)
        if named:
            name = named.group(1).upper()
            return {"INCH": "in", "FOOT": "ft", "MIL": "mil", "THOU": "mil",
                    "MILLIMETRE": "mm", "MILLIMETER": "mm"}.get(name, name.lower())
        si = re.search(r"SI_UNIT\s*\(\s*(?:\.(\w+)\.|\$|\*)\s*,\s*\.METRE\.", entity)
        if si:
            return _UNIT_PREFIX.get(si.group(1), si.group(1).lower()) if si.group(1) else "m"
    return None


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


def _polygon_face(points):
    """A planar face on a closed polygon of points, or None."""
    from OCP.BRepBuilderAPI import BRepBuilderAPI_MakePolygon

    try:
        polygon = BRepBuilderAPI_MakePolygon()
        for point in points:
            polygon.Add(gp_Pnt(*[float(v) for v in point]))
        polygon.Close()
        maker = BRepBuilderAPI_MakeFace(polygon.Wire(), True)
        return maker.Face() if maker.IsDone() else None
    except Exception:  # noqa: BLE001 - a degenerate polygon is just no face
        return None


def _plane_fit(points):
    """(centroid, unit normal, worst distance) of the best plane through points."""
    pts = np.asarray(points, dtype=float)
    centre = pts.mean(axis=0)
    _u, _s, vt = np.linalg.svd(pts - centre)
    normal = vt[-1]
    return centre, normal, float(np.abs((pts - centre) @ normal).max())


def _loop_corners(wire, samples: int = 8) -> list[int]:
    """Indices (into _sample_wire_points) where the wire's edges meet: the
    only places an opening can bend from one plane into the next."""
    count = len(_sample_wire_points(wire))
    return list(range(0, count, samples))


def _arc(points, start: int, end: int):
    n = len(points)
    return [points[(start + k) % n] for k in range((end - start) % n + 1)]


def _planar_patches_for_wire(wire):
    """Seal one opening with up to three flat patches (no star N-gons):
    one exact plane when the loop is flat; else the loop is split at its edge
    corners into 2 arcs that each lie in a plane (sharing the bend line as a
    chord), or 3 arcs whose planes meet at one corner point (an open box
    corner). None when no such split exists."""
    try:
        maker = BRepBuilderAPI_MakeFace(wire, True)
        if maker.IsDone():
            return [maker.Face()]
    except Exception:  # noqa: BLE001
        pass
    points = [np.asarray(p) for p in _sample_wire_points(wire)]
    if len(points) < 6:
        return None
    size = float(np.linalg.norm(np.ptp(np.asarray(points), axis=0))) or 1.0
    tol = 0.02 * size
    corners = _loop_corners(wire)
    for i in range(len(corners)):  # two planes sharing the chord between two corners
        for j in range(i + 1, len(corners)):
            a, b = corners[i], corners[j]
            arcs = [_arc(points, a, b), _arc(points, b, a)]
            fits = [_plane_fit(arc) for arc in arcs]
            if all(len(arc) >= 3 and fit[2] <= tol for arc, fit in zip(arcs, fits)) \
                    and abs(float(fits[0][1] @ fits[1][1])) < 0.999:
                faces = [_polygon_face(arc) for arc in arcs]
                if all(f is not None for f in faces):
                    return faces
    for i in range(len(corners)):  # three planes meeting at one corner point
        for j in range(i + 1, len(corners)):
            for k in range(j + 1, len(corners)):
                a, b, c = corners[i], corners[j], corners[k]
                arcs = [_arc(points, a, b), _arc(points, b, c), _arc(points, c, a)]
                fits = [_plane_fit(arc) for arc in arcs]
                if not all(len(arc) >= 2 and fit[2] <= tol for arc, fit in zip(arcs, fits)):
                    continue
                normals = np.array([fit[1] for fit in fits])
                if abs(np.linalg.det(normals)) < 1e-3:
                    continue
                offsets = np.array([fit[1] @ fit[0] for fit in fits])
                corner = np.linalg.solve(normals, offsets)
                low, high = np.min(points, axis=0) - tol, np.max(points, axis=0) + tol
                if np.any(corner < low) or np.any(corner > high):
                    continue
                faces = [_polygon_face(arc + [corner]) for arc in arcs]
                if all(f is not None for f in faces):
                    return faces
    return None


def _planar_patch_caps(wires) -> list | None:
    """Every boundary loop sealed with 1-3 flat patches, or None."""
    caps = []
    for wire_index in range(1, wires.Length() + 1):
        faces = _planar_patches_for_wire(TopoDS.Wire_s(wires.Value(wire_index)))
        if faces is None:
            return None
        caps.extend(faces)
    return caps


def _ear_clip_triangles(arr) -> list:
    """Triangulate a loop polygon by ear clipping in its best-fit plane:
    triangles between the loop's own vertices only, so no centre vertex and
    no star N-gon."""
    pts = np.asarray(arr, dtype=float)
    centre, normal, _worst = _plane_fit(pts)
    u = np.cross(normal, [1.0, 0.0, 0.0])
    if np.linalg.norm(u) < 1e-6:
        u = np.cross(normal, [0.0, 1.0, 0.0])
    u /= np.linalg.norm(u)
    v = np.cross(normal, u)
    flat = np.column_stack([(pts - centre) @ u, (pts - centre) @ v])
    order = list(range(len(pts)))
    area = 0.5 * sum(flat[i][0] * flat[(i + 1) % len(flat)][1] - flat[(i + 1) % len(flat)][0] * flat[i][1]
                     for i in range(len(flat)))
    if area < 0:
        order.reverse()

    def cross(o, a, b):
        return (flat[a][0] - flat[o][0]) * (flat[b][1] - flat[o][1]) - (flat[a][1] - flat[o][1]) * (flat[b][0] - flat[o][0])

    def inside(p, a, b, c):
        return cross(a, b, p) >= 0 and cross(b, c, p) >= 0 and cross(c, a, p) >= 0

    faces, guard = [], 0
    while len(order) > 3 and guard < 10 * len(pts):
        guard += 1
        for k in range(len(order)):
            a, b, c = order[k - 1], order[k], order[(k + 1) % len(order)]
            if cross(a, b, c) <= 1e-12:
                continue  # reflex or flat corner: not an ear
            if any(inside(p, a, b, c) for p in order if p not in (a, b, c)):
                continue
            face = _polygon_face([pts[a], pts[b], pts[c]])
            if face is not None:
                faces.append(face)
            order.pop(k)
            break
        else:
            break
    if len(order) == 3:
        face = _polygon_face([pts[i] for i in order])
        if face is not None:
            faces.append(face)
    return faces


def _ear_cap_faces(wires) -> list:
    """Last-resort caps: each boundary loop ear-clipped into triangles of its
    own vertices (the old centroid fan made star N-gons). The chords deviate
    from the exact curves by the sampling sag, so sewing needs a matching
    tolerance."""
    caps = []
    for wire_index in range(1, wires.Length() + 1):
        points = _sample_wire_points(TopoDS.Wire_s(wires.Value(wire_index)))
        if len(points) >= 3:
            caps.extend(_ear_clip_triangles(points))
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
    """Colored-face centroids + match tolerance: per-face colors can't follow
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
    """Cap strategies in order, cleanest topology first: flat patches (one
    exact plane, or 2-3 planes meeting along the bend / at a corner), then
    smooth filled patches, then ear-clipped triangles of the loop's own
    vertices (never a centroid fan, so never a star N-gon). Each entry is
    (cap faces, sew tolerance)."""
    attempts = []
    planar = _planar_patch_caps(wires)
    if planar:
        exact = len(planar) == wires.Length()  # one exact plane per loop
        attempts.append((planar, 1.0e-6 if exact else fan_tolerance))
    filled = _cap_faces_for_wires(wires, anchor_points=anchors)
    if filled:
        attempts.append((filled, 1.0e-6))
    ears = _ear_cap_faces(wires)
    if ears:
        attempts.append((ears, fan_tolerance))
    return attempts


def _sealed_attempt(faces, caps, sew_tolerance, region_area, region_bounds):
    """One cap attempt: the sewn solid, or None when the caps are larger than
    the region (a billowing patch over a bent loop) or the solid balloons."""
    cap_area = sum(shape_area(cap) or 0.0 for cap in caps)
    if region_area > 0.0 and cap_area > 1.2 * region_area:
        return None
    solid = _sew_solid(faces + caps, tolerance=sew_tolerance)
    if solid is None or not _solid_within_region_bounds(solid, region_bounds):
        return None
    if (shape_volume(solid) or 0.0) < 0.0:  # inside-out shell: material is the other side
        solid = TopoDS.Solid_s(solid.Reversed())
    return solid


def _region_components(edge_table, region: set) -> list[set]:
    """The region split into face patches that share an edge, largest first.
    A painted pin can be several patches (a THT leg below the board AND the
    wire clamp seen through the wire-entry hole) that touch nowhere."""
    parent = {face: face for face in region}

    def root(face):
        while parent[face] != face:
            parent[face] = parent[parent[face]]
            face = parent[face]
        return face

    for _edge, faces in edge_table:
        inside = [f for f in faces if f in parent]
        for other in inside[1:]:
            parent[root(other)] = root(inside[0])
    groups: dict = {}
    for face in region:
        groups.setdefault(root(face), set()).add(face)
    return sorted(groups.values(), key=lambda g: (-len(g), min(g)))


def _faces_outside(solid, mesh, patch: set) -> set:
    """Faces of the patch that the sealed solid does not sit behind: points a
    hair beneath the face (into the body, against its outward normal) must be
    inside the solid. A cup (a socket, a wire-entry pocket) or a strip of metal
    that only shows through a slot in the housing seals into the AIR in front
    of it or into a sliver cut through the housing; those faces fail, because
    separating them would invent material that is not in the model."""
    from OCP.BRepClass3d import BRepClass3d_SolidClassifier

    if mesh is None:
        return set()
    mask = np.isin(mesh.tri_face_ids, np.asarray(sorted(patch), dtype=np.int32))
    span = mesh.points[mesh.tris[mask]].reshape(-1, 3)
    depth = min(0.02, 0.002 * float(np.linalg.norm(span.max(axis=0) - span.min(axis=0)))) or 1e-4
    classifier = BRepClass3d_SolidClassifier(solid)
    outside = set()
    for face in patch:
        behind = _face_probe_points(mesh, face, depth)
        inside = sum(_state(classifier, p) == TopAbs_IN for p in behind)
        if behind and inside < 0.5 * len(behind):
            outside.add(face)
    return outside


def _state(classifier, point):
    """Where a point lies against the classifier's solid. (A self-crossing
    sliver can answer IN behind every face; _volume_consistent catches it.)"""
    classifier.Perform(gp_Pnt(*point), 1.0e-7)
    return classifier.State()


def _face_probe_points(mesh, face: int, depth: float, count: int = 5) -> list:
    """Up to `count` points `depth` beneath a face (inward along its triangle
    normals; tessellation winds triangles outward)."""
    corners = mesh.points[mesh.tris[mesh.tri_face_ids == face]]
    normals = np.cross(corners[:, 1] - corners[:, 0], corners[:, 2] - corners[:, 0])
    lengths = np.linalg.norm(normals, axis=1)
    keep = lengths > 1e-14
    centers = corners.mean(axis=1)[keep] - normals[keep] / lengths[keep, None] * depth
    step = max(1, len(centers) // count)
    return [tuple(float(v) for v in c) for c in centers[::step][:count]]


def _seal_patch(body, patch, face_map, edge_table, fan_tolerance):
    """Seal one connected patch. Returns ((solid, caps, sew_tolerance) or None,
    the faces that failed the material check on the best failed attempt)."""
    wires = _boundary_wires(edge_table, patch)
    if wires is None:
        return None, set()
    region_area, region_bounds = _region_area_bounds(body.mesh, patch)
    faces = [TopoDS.Face_s(face_map.FindKey(i)) for i in sorted(patch)]
    anchors = _region_anchor_points(body.mesh, patch) if body.mesh is not None else []
    attempts = _cap_attempts(wires, anchors, fan_tolerance)
    return _best_seal(faces, attempts, (region_area, region_bounds), body.mesh, patch)


def _best_seal(faces, attempts, region, mesh, patch):
    """The first cap attempt whose solid backs every face; else (None, the
    smallest set of unbacked faces any attempt left)."""
    worst = None
    for caps, tol in attempts:
        solid = _sealed_attempt(faces, caps, tol, *region)
        outside = None if solid is None else _unbacked_faces(solid, mesh, patch, faces, caps)
        if outside is None:
            continue
        if not outside:
            return (solid, caps, tol), set()
        worst = outside if worst is None or len(outside) < len(worst) else worst
    return None, worst or set()


def _unbacked_faces(solid, mesh, patch: set, faces=(), caps=()) -> set:
    """Faces the sealed solid does not really sit behind (empty: a good pin)."""
    outside = _faces_outside(solid, mesh, patch) | _faces_under_caps(sorted(patch), faces, caps)
    if not outside and not _volume_consistent(solid):
        outside = _recessed_faces(mesh, patch)  # a self-crossing sliver
    return outside


def _faces_under_caps(face_ids, faces, caps) -> set:
    """Patch faces that a cap lies ON (coplanar and overlapping): a strip of
    housing in the plane where a leg is capped gets covered twice, a zero-
    thickness fin that keeps the volume right but z-fights and stretches the
    pin's hitbox. Those faces belong to the housing."""
    covered = set()
    for cap in caps:
        plane = _plane_of(cap)
        if plane is None:
            continue
        for face_id, face in zip(face_ids, faces):
            if _coplanar(plane, _plane_of(face)) and _overlap_area(cap, face) > 1.0e-9:
                covered.add(face_id)
    return covered


def _plane_of(face):
    """(unit normal, offset) of a planar face, None for curved faces."""
    adaptor = BRepAdaptor_Surface(face)
    if adaptor.GetType() != GeomAbs_Plane:
        return None
    axis = adaptor.Plane().Axis()
    normal = np.array(axis.Direction().Coord())
    return normal, float(normal @ np.array(axis.Location().Coord()))


def _coplanar(a, b, tolerance: float = 1.0e-6) -> bool:
    if a is None or b is None:
        return False
    (na, da), (nb, db) = a, b
    sign = 1.0 if na @ nb > 0 else -1.0
    return abs(abs(na @ nb) - 1.0) < 1.0e-9 and abs(da - sign * db) < tolerance


def _overlap_area(face_a, face_b) -> float:
    try:
        return shape_area(BRepAlgoAPI_Common(face_a, face_b).Shape()) or 0.0
    except Exception:  # noqa: BLE001 - a failed boolean is no evidence of overlap
        return 0.0


def _volume_consistent(solid, samples: int = 800) -> bool:
    """A sealed solid whose shell crosses itself (caps slicing through the
    housing) reports a volume that disagrees with what point classification
    sees inside it. Monte-Carlo the box: fraction inside x box volume must match
    the computed volume (a valid solid always does, to sampling noise)."""
    from OCP.BRepClass3d import BRepClass3d_SolidClassifier

    xmin, xmax, ymin, ymax, zmin, zmax = shape_bounds([solid])
    low, high = np.array([xmin, ymin, zmin]), np.array([xmax, ymax, zmax])
    box = float(np.prod(np.maximum(high - low, 1e-9)))
    points = np.random.default_rng(0).uniform(low, high, size=(samples, 3))
    classifier = BRepClass3d_SolidClassifier(solid)
    inside = sum(_state(classifier, p) == TopAbs_IN for p in points)
    estimate = inside / samples * box
    volume = abs(shape_volume(solid) or 0.0)
    return abs(estimate - volume) <= 0.35 * max(estimate, volume) + 0.03 * box


RAY_DIRECTIONS = np.array([d for d in np.ndindex(3, 3, 3) if d != (1, 1, 1)], float) - 1.0
RAY_DIRECTIONS /= np.linalg.norm(RAY_DIRECTIONS, axis=1)[:, None]


def _recessed_faces(mesh, patch: set) -> set:
    """Faces of the patch that sit in a recess of the part: from a point just
    in front of the face, most rays hit the rest of the body. Those are metal
    that only shows through a hole or slot (a wire clamp, a strip in a
    channel); a leg or a lead in open space sees out."""
    if mesh is None:
        return set()
    others = ~np.isin(mesh.tri_face_ids, np.asarray(sorted(patch), dtype=np.int32))
    triangles = mesh.points[mesh.tris[others]]
    span = float(np.linalg.norm(mesh.points.max(axis=0) - mesh.points.min(axis=0)))
    recessed = set()
    for face in patch:
        probes = _face_probe_points(mesh, face, -2.0e-3 * span, count=3)
        hits = [_ray_hits(triangles, np.array(p)).mean() for p in probes]
        if hits and float(np.mean(hits)) >= 0.7:
            recessed.add(face)
    return recessed


def _ray_hits(triangles: np.ndarray, origin: np.ndarray) -> np.ndarray:
    """Per RAY_DIRECTIONS ray from origin: does it hit any triangle
    (Moller-Trumbore, vectorised over the triangles)?"""
    v0, e1, e2 = triangles[:, 0], triangles[:, 1] - triangles[:, 0], triangles[:, 2] - triangles[:, 0]
    out = np.zeros(len(RAY_DIRECTIONS), bool)
    for i, d in enumerate(RAY_DIRECTIONS):
        p = np.cross(d, e2)
        det = np.einsum("ij,ij->i", e1, p)
        ok = np.abs(det) > 1e-12
        inv = np.where(ok, 1.0 / np.where(ok, det, 1.0), 0.0)
        t = origin - v0
        u = np.einsum("ij,ij->i", t, p) * inv
        q = np.cross(t, e1)
        v = (q @ d) * inv
        dist = np.einsum("ij,ij->i", e2, q) * inv
        out[i] = bool((ok & (u >= 0) & (v >= 0) & (u + v <= 1) & (dist > 1e-9)).any())
    return out


def _refine_patch(body, patch, face_map, edge_table, fan_tolerance, rounds: int = 4):
    """Seal a patch; when some faces are not backed by the sealed solid, drop
    them (they stay on the package, untouched and uncapped) and try each
    connected rest, largest first. Returns (solid, caps, sew_tolerance, patch)
    or None."""
    result, outside = _seal_patch(body, patch, face_map, edge_table, fan_tolerance)
    if result is not None:
        return (*result, patch)
    rest = patch - outside
    if rounds <= 1 or not outside or not rest:
        return None
    for piece in _region_components(edge_table, rest):
        found = _refine_patch(body, piece, face_map, edge_table, fan_tolerance, rounds - 1)
        if found is not None:
            return found
    return None


def _seal_one_region(body, region, face_map, edge_table, fan_tolerance):
    """Seal one previewed region into its own solid. Each connected patch is
    sealed on its own (sewing disjoint patches into ONE solid makes one shell
    a void of the other) and only faces the solid really sits behind are kept;
    the largest patch that seals becomes the pin, everything else stays on the
    package. Returns (solid, caps, sew_tolerance, patch) or None."""
    for patch in _region_components(edge_table, region):
        found = _refine_patch(body, patch, face_map, edge_table, fan_tolerance)
        if found is not None:
            return found
    return None


def _seal_regions(body, region_sets, face_map, edge_table, progress):
    """Seal every region of one body; returns the sealed tuples that took."""
    fan_tolerance = max(1.0e-4, 2.0 * preview_deflection(shape_bounds([body.solid])))
    sealed = []
    for region_number, (region_index, region, _cut) in enumerate(region_sets):
        if progress is not None:
            progress("Sealing pin regions", region_number, len(region_sets))
        result = _seal_one_region(body, region, face_map, edge_table, fan_tolerance)
        if result is not None:
            solid, caps, tol, patch = result
            sealed.append((region_index, patch, solid, caps, tol))
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
    region to its solid by identity and queueing color re-attachment."""
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
    """Re-attach per-face colors: a new face whose centroid coincides with an
    old colored face's centroid is that same face."""
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


def _without_voids(body) -> bool:
    """Drop the void shells a split leaves in a package (cavities where its
    pins ran through). STEP writes them as BREP_WITH_VOIDS but OCC reads each
    void back as a separate pin-shaped solid, and vendor models put pins
    THROUGH their housing anyway. Face colors follow their faces. Returns
    True when the body changed."""
    from OCP.BRepBuilderAPI import BRepBuilderAPI_MakeSolid

    if body.solid.ShapeType() != TopAbs_SOLID or _count(body.solid, TopAbs_SHELL) < 2:
        return False
    outers = []
    explorer = TopExp_Explorer(body.solid, TopAbs_SHELL)
    while explorer.More():
        shell = TopoDS.Shell_s(explorer.Current())
        probe = BRepBuilderAPI_MakeSolid(shell)
        volume = shape_volume(probe.Solid()) if probe.IsDone() else None
        if volume is None or volume > 0:
            outers.append(shell)
        explorer.Next()
    if len(outers) != 1:
        return False  # several outer pieces: not a cavity case, leave it
    maker = BRepBuilderAPI_MakeSolid(outers[0])
    if not maker.IsDone():
        return False
    old_map = TopTools_IndexedMapOfShape()
    TopExp.MapShapes_s(body.solid, TopAbs_FACE, old_map)
    old_colors = dict(body.mesh.face_colors) if body.mesh is not None else {}
    body.solid = maker.Solid()
    new_map = TopTools_IndexedMapOfShape()
    TopExp.MapShapes_s(body.solid, TopAbs_FACE, new_map)
    body.mesh = tessellate_body(body.solid, preview_deflection(shape_bounds([body.solid])))
    body.mesh.face_colors = {
        index: old_colors[old_map.FindIndex(new_map.FindKey(index))]
        for index in range(1, new_map.Extent() + 1)
        if old_map.FindIndex(new_map.FindKey(index)) in old_colors
    }
    return True


def _bodies_from_label(label, color_tool, fallback_name) -> list:
    """Build one BodyRecord per solid under a free-shape label (colors first,
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
    Returns (placed_shape, origin, normal) — origin/normal feed color reattach."""
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
    """(face centroid array, [(centroid, rgb)]) so per-face colors can be
    re-matched after the boolean re-fids every face. ([]/None when uncolored)."""
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


def _connected_solids(shape) -> list:
    """The separate solids inside a carved piece (a compound of several
    disconnected solids becomes a list; a single solid stays as it is)."""
    found = TopTools_IndexedMapOfShape()
    TopExp.MapShapes_s(shape, TopAbs_SOLID, found)
    solids = [found.FindKey(i) for i in range(1, found.Extent() + 1)]
    return solids if len(solids) > 1 else [shape]


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


def cut_kind(cut: dict) -> str:
    """'box' (axis-aligned limits) or 'cylinder' (p0, p1, radius)."""
    return cut.get("type") or ("box" if "limits" in cut else "cylinder")


def cylinder_frame(cut: dict):
    """(p0, unit axis, length, radius) of a cylinder cut, or None if degenerate."""
    p0 = np.asarray(cut["p0"], dtype=float)
    p1 = np.asarray(cut["p1"], dtype=float)
    axis = p1 - p0
    length = float(np.linalg.norm(axis))
    radius = float(cut.get("radius", 0.0))
    if length < 1e-9 or radius <= 0.0:
        return None
    return p0, axis / length, length, radius


def points_in_cut(points: np.ndarray, cut: dict, margin: float = 0.0) -> np.ndarray:
    """Boolean mask: which points lie inside a cut volume (box or cylinder)."""
    points = np.asarray(points, dtype=float).reshape(-1, 3)
    if cut_kind(cut) == "box":
        xlo, xhi, ylo, yhi, zlo, zhi = (float(v) for v in cut["limits"])
        return ((points[:, 0] >= xlo - margin) & (points[:, 0] <= xhi + margin)
                & (points[:, 1] >= ylo - margin) & (points[:, 1] <= yhi + margin)
                & (points[:, 2] >= zlo - margin) & (points[:, 2] <= zhi + margin))
    frame = cylinder_frame(cut)
    if frame is None:
        return np.zeros(len(points), dtype=bool)
    p0, axis, length, radius = frame
    rel = points - p0
    t = rel @ axis
    radial = np.linalg.norm(rel - np.outer(t, axis), axis=1)
    return (t >= -margin) & (t <= length + margin) & (radial <= radius + margin)


def _solid_vertices(solid) -> np.ndarray:
    pts = []
    explorer = TopExp_Explorer(solid, TopAbs_VERTEX)
    while explorer.More():
        p = BRep_Tool.Pnt_s(TopoDS.Vertex_s(explorer.Current()))
        pts.append((p.X(), p.Y(), p.Z()))
        explorer.Next()
    return np.asarray(pts, dtype=float).reshape(-1, 3)


def _planar_faces_xyz(solid) -> list:
    """(outward unit normal, vertex coordinates) of every flat face of a solid."""
    from OCP.BRepAdaptor import BRepAdaptor_Surface
    from OCP.GeomAbs import GeomAbs_Plane
    from OCP.TopAbs import TopAbs_REVERSED

    faces = []
    explorer = TopExp_Explorer(solid, TopAbs_FACE)
    while explorer.More():
        face = TopoDS.Face_s(explorer.Current())
        explorer.Next()
        surface = BRepAdaptor_Surface(face)
        if surface.GetType() != GeomAbs_Plane:
            continue
        d = surface.Plane().Axis().Direction()
        normal = np.array([d.X(), d.Y(), d.Z()])
        if face.Orientation() == TopAbs_REVERSED:
            normal = -normal
        xyz = _solid_vertices(face)
        if len(xyz):
            faces.append((normal, xyz))
    return faces


def respect_coplanar_faces(bodies, cuts, tolerance: float = 0.05) -> list:
    """Box cuts, with each box face that lies on (or within `tolerance` of) a
    flat model face it overlaps adjusted by the faces' directions:
    - every such model face points the SAME way as the box face (outward
      from the cut): that geometry belongs to the new piece, so the box face
      moves a hair outward to take it in;
    - otherwise (it faces the other way, as the body's surface under a pin
      does, or both kinds share the plane) the box face snaps exactly onto the
      plane: the cut stops there and takes no material from the old model.
    Cylinders pass through unchanged."""
    out = []
    for cut in cuts:
        if cut_kind(cut) != "box":
            out.append(cut)
            continue
        limits = [float(v) for v in cut["limits"]]
        lo, hi = np.array(limits[0::2]), np.array(limits[1::2])
        size = float(np.max(hi - lo)) or 1.0
        tol = min(tolerance, 0.1 * float(np.min(hi - lo)) or tolerance)
        nudge = max(1e-4 * size, 1e-5)
        faces = []
        for body in bodies:
            box = shape_bounds([body.solid])
            if not _bbox_disjoint(box, tuple(limits)):
                faces.extend(_planar_faces_xyz(body.solid))
        for axis in range(3):
            others = [a for a in range(3) if a != axis]
            for side, index in ((-1, 2 * axis), (1, 2 * axis + 1)):
                plane = limits[index]
                directions = set()
                for normal, xyz in faces:
                    if abs(abs(normal[axis]) - 1.0) > 1e-6:
                        continue  # not parallel to this box face
                    level = float(xyz[:, axis].mean())
                    if abs(level - plane) > tol:
                        continue
                    flo, fhi = xyz.min(axis=0), xyz.max(axis=0)
                    if np.any(fhi[others] <= lo[others]) or np.any(flo[others] >= hi[others]):
                        continue  # coplanar but beside the box: not this cut's business
                    directions.add(int(np.sign(normal[axis])) == side)
                    snapped = level
                if not directions:
                    continue
                limits[index] = snapped + (side * nudge if directions == {True} else 0.0)
        adjusted = dict(cut)
        adjusted["limits"] = limits
        out.append(adjusted)
    return out


def _collect_boxes(cuts) -> list:
    """The valid (non-degenerate) limit boxes of a list of cut dicts."""
    boxes = [tuple(float(v) for v in c["limits"]) for c in cuts if cut_kind(c) == "box"]
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
    """Re-color after the boolean re-fids faces. Logo faces (geometry created
    strictly off the surface plane) take logo_rgb; the cut-through original
    keeps its color via a wider centroid re-match; everything else re-matches
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
    clear_bounds_cache()
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


def _face_signatures(mesh: BodyMesh) -> tuple[np.ndarray, np.ndarray]:
    """(area-weighted centroid, area) per 1-based face id, from the mesh."""
    tri = mesh.points[mesh.tris]
    area = 0.5 * np.linalg.norm(np.cross(tri[:, 1] - tri[:, 0], tri[:, 2] - tri[:, 0]), axis=1)
    centre = tri.mean(axis=1)
    n = mesh.face_count + 1
    areas = np.bincount(mesh.tri_face_ids, weights=area, minlength=n)
    sums = np.stack([np.bincount(mesh.tri_face_ids, weights=area * centre[:, k], minlength=n) for k in range(3)], 1)
    return sums / np.maximum(areas, 1e-30)[:, None], areas


def _face_triangles(mesh: BodyMesh, faces) -> tuple[np.ndarray, np.ndarray]:
    """(points, tris) of just these faces of a mesh."""
    keep = mesh.tris[np.isin(mesh.tri_face_ids, list(faces))]
    used, tris = np.unique(keep, return_inverse=True)
    return mesh.points[used], tris.reshape(-1, 3)


def changed_geometry(before: list, after: list, size: float) -> list:
    """Pieces of mesh that a fix added, moved, reshaped or removed, as
    (points, tris): faces of `after` with no face of `before` in the same
    place at the same area, and the other way round. before / after are the
    bodies' BodyMesh lists (None for an unmeshed body)."""
    from scipy.spatial import cKDTree

    def signatures(meshes):
        rows = []
        for mesh in meshes:
            if mesh is None or not len(mesh.tris):
                continue
            centre, area = _face_signatures(mesh)
            rows += [(mesh, f, centre[f], area[f]) for f in np.nonzero(area > 0)[0]]
        return rows

    def unmatched(rows, other):
        if not other:
            return rows
        tree = cKDTree(np.array([r[2] for r in other]))
        areas = np.array([r[3] for r in other])
        out = []
        for row in rows:
            near = tree.query_ball_point(row[2], 1e-4 * size + 1e-7)
            if not any(abs(areas[k] - row[3]) <= 0.01 * row[3] for k in near):
                out.append(row)
        return out

    old, new = signatures(before), signatures(after)
    pieces = []
    for rows in (unmatched(new, old), unmatched(old, new)):
        by_mesh: dict[int, tuple] = {}
        for mesh, face, _c, _a in rows:
            by_mesh.setdefault(id(mesh), (mesh, []))[1].append(int(face))
        pieces += [_face_triangles(mesh, faces) for mesh, faces in by_mesh.values()]
    return pieces


def _match_faces(previous: BodyMesh, current: BodyMesh) -> dict[int, int]:
    """new face id -> old face id after a topology change (sew, fill, fix):
    the old face in the same place with about the same area, else (a new
    fill patch) the nearest old face."""
    from scipy.spatial import cKDTree

    old_centre, old_area = _face_signatures(previous)
    new_centre, new_area = _face_signatures(current)
    old_ids = np.nonzero(old_area > 0)[0]
    if not len(old_ids):
        return {}
    k = min(4, len(old_ids))
    _dist, near = cKDTree(old_centre[old_ids]).query(new_centre, k=k)
    near = np.asarray(near).reshape(len(new_centre), k)
    mapping = {}
    for face in np.nonzero(new_area > 0)[0]:
        candidates = old_ids[near[face]]
        ratio = np.abs(np.log(np.maximum(old_area[candidates], 1e-30) / new_area[face]))
        same = candidates[ratio < 0.1]
        mapping[int(face)] = int(same[0] if len(same) else candidates[0])
    return mapping


def _same_faces(previous: BodyMesh, current: BodyMesh) -> bool:
    """Same face ids for the same faces (a rigid move keeps both)."""
    return previous.face_count == current.face_count and np.allclose(
        _face_signatures(previous)[1], _face_signatures(current)[1], rtol=0.05, atol=1e-9)


def _remap(colors: dict, mapping: dict) -> dict:
    return {new: colors[old] for new, old in mapping.items() if old in colors}


def _carry_face_colors(previous: BodyMesh | None, current: BodyMesh, body=None) -> None:
    """Re-tessellation drops XCAF color lookups (the source document is
    gone). Face ids survive rigid transforms, so colors carry by id; after
    a topology change (a repair renumbers every face) they carry by where
    each face is. The body's original colors (for Colors' reset) follow."""
    if previous is None or current.face_colors:
        return
    if _same_faces(previous, current):
        current.face_colors = dict(previous.face_colors)
        return
    mapping = _match_faces(previous, current)
    current.face_colors = _remap(previous.face_colors, mapping)
    if body is not None and body.original_face_colors:
        body.original_face_colors = _remap(body.original_face_colors, mapping)


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
    def load(cls, path: str | Path, mesh: bool = True) -> "EditorDocument":
        """Read a STEP. mesh=False skips tessellation and face colours (a
        structural re-read, e.g. export validation counting bodies)."""
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
        if mesh:
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
            _carry_face_colors(previous, body.mesh, body)

    def remesh_body(self, body_index: int) -> None:
        deflection = preview_deflection(self.bounds())
        body = self.bodies[body_index]
        previous = body.mesh
        body.mesh = tessellate_body(body.solid, deflection)
        _carry_face_colors(previous, body.mesh, body)

    # ------------------------------------------------------------- mutation

    def apply_trsf(self, matrix) -> None:
        """Apply a rigid 4x4 transform (row-major, numpy or nested lists) to
        every body. Topology is preserved, so face ids — and therefore pin
        face references and face colors — stay valid."""
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
        boxes = _collect_boxes(respect_coplanar_faces(self.bodies, cuts))
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

    def split_by_cuts(self, cuts, remesh: bool = True) -> list[int]:
        """Carve boxes AND cylinders in one multi-tool boolean per body (the
        general form of split_by_boxes). A piece is carved (a pin) when it lies
        wholly inside one cut volume; the rest stays the package. Returns the
        carved pieces' indices in the new body table."""
        if all(cut_kind(c) == "box" for c in cuts):
            return self.split_by_boxes(cuts, remesh=remesh)
        cuts = respect_coplanar_faces(self.bodies, cuts)
        bounds = self.bounds()
        diag = float(np.linalg.norm([bounds[1] - bounds[0], bounds[3] - bounds[2],
                                     bounds[5] - bounds[4]]))
        fuzzy = max(diag * 1.0e-5, 1.0e-7)
        margin = max(diag * 1.0e-3, 1.0e-5)
        tools, volumes = [], []
        for cut in cuts:
            if cut_kind(cut) == "box":
                b = tuple(float(v) for v in cut["limits"])
                if b[1] > b[0] and b[3] > b[2] and b[5] > b[4]:
                    tools.append(self._box_cut_tool(b, None, None, pad=0.0))
                    volumes.append(cut)
            else:
                frame = cylinder_frame(cut)
                if frame is None:
                    continue
                p0, axis, length, radius = frame
                ax2 = gp_Ax2(gp_Pnt(*p0), gp_Dir(*axis))
                tools.append(BRepPrimAPI_MakeCylinder(ax2, radius, length).Solid())
                volumes.append(cut)
        if not tools:
            return []
        new_bodies: list[BodyRecord] = []
        carved: list[int] = []
        for body in self.bodies:
            pieces = self._split_solid_by_tools(body.solid, tools, fuzzy)
            if len(pieces) < 2:
                new_bodies.append(body)
                continue
            pin_solids, rem_solids = [], []
            for solid in pieces:
                verts = _solid_vertices(solid)
                inside = len(verts) and any(points_in_cut(verts, cut, margin).all() for cut in volumes)
                if inside:
                    # one long cut over several separate shells: each becomes its
                    # own piece, never one floating multi-shell body
                    pin_solids.extend(_connected_solids(solid))
                else:
                    rem_solids.append(solid)
            if not pin_solids or not rem_solids:
                new_bodies.append(body)
                continue
            new_bodies.append(BodyRecord(solid=_keep_meaningful_solids(rem_solids, body.solid),
                                         name=body.name, color=body.color,
                                         original_color=body.original_color))
            for solid in pin_solids:
                carved.append(len(new_bodies))
                new_bodies.append(BodyRecord(solid=solid, name=body.name, color=body.color,
                                             original_color=body.original_color))
        self.bodies = new_bodies
        self._adjacency_cache = {}
        if remesh:
            self.remesh_all()
        return carved

    def circle_near(self, body_index: int, face_index: int, point):
        """The circular edge of a face nearest `point`: (centre, unit axis,
        radius), or None when the face has no circular edge. A cylindrical
        face itself works too (its nearest end circle)."""
        from OCP.GeomAbs import GeomAbs_Circle

        face = self.face_of(body_index, face_index)
        point = np.asarray(point, dtype=float)
        best = None
        explorer = TopExp_Explorer(face, TopAbs_EDGE)
        while explorer.More():
            curve = BRepAdaptor_Curve(TopoDS.Edge_s(explorer.Current()))
            if curve.GetType() == GeomAbs_Circle:
                circle = curve.Circle()
                c = circle.Location()
                centre = np.array([c.X(), c.Y(), c.Z()])
                d = circle.Axis().Direction()
                axis = np.array([d.X(), d.Y(), d.Z()])
                radius = circle.Radius()
                rel = point - centre
                along = rel @ axis
                ring = abs(np.linalg.norm(rel - along * axis) - radius) + abs(along)
                if best is None or ring < best[0]:
                    best = (ring, centre, axis, radius)
            explorer.Next()
        return None if best is None else (best[1], best[2], best[3])

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
        if any([_without_voids(body) for body in self.bodies]):
            self._adjacency_cache = {}
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

    def split_face(self, body_index: int, face_index: int, p1, p2) -> list[int]:
        """Split one face in two along the line p1 -> p2 (both on the face):
        the cut is the face's section with the plane through that line and the
        face normal. Only that face changes; other faces keep their colors,
        both halves inherit the old face's color. Returns the new face ids
        (empty when the line doesn't cross the face)."""
        from OCP.BRepAlgoAPI import BRepAlgoAPI_Section
        from OCP.BRepFeat import BRepFeat_SplitShape

        body = self.bodies[body_index]
        face = self.face_of(body_index, face_index)
        p1, p2 = np.asarray(p1, dtype=float), np.asarray(p2, dtype=float)
        mesh = body.mesh
        mask = mesh.tri_face_ids == face_index
        tris = mesh.tris[mask]
        corners = mesh.points[tris]
        centers = corners.mean(axis=1)
        nearest = int(np.argmin(np.linalg.norm(centers - (p1 + p2) / 2, axis=1)))
        normal = np.cross(corners[nearest, 1] - corners[nearest, 0],
                          corners[nearest, 2] - corners[nearest, 0])
        cut_normal = np.cross(p2 - p1, normal)
        length = float(np.linalg.norm(cut_normal))
        if length < 1e-12:
            return []
        cut_normal /= length
        plane = gp_Pln(gp_Pnt(*p1), gp_Dir(*cut_normal))
        section = BRepAlgoAPI_Section(face, plane, False)
        section.ComputePCurveOn1(True)
        section.Approximation(True)
        section.Build()
        if not section.IsDone():
            return []
        splitter = BRepFeat_SplitShape(body.solid)
        explorer = TopExp_Explorer(section.Shape(), TopAbs_EDGE)
        edges = 0
        while explorer.More():
            splitter.Add(TopoDS.Edge_s(explorer.Current()), TopoDS.Face_s(face))
            edges += 1
            explorer.Next()
        if not edges:
            return []
        splitter.Build()
        if not splitter.IsDone():
            return []
        solids = list(_iter_solids(splitter.Shape()))
        if len(solids) != 1:
            return []

        old_map = TopTools_IndexedMapOfShape()
        TopExp.MapShapes_s(body.solid, TopAbs_FACE, old_map)
        old_colors = dict(mesh.face_colors)
        halves = splitter.Modified(face)
        body.solid = solids[0]
        new_map = TopTools_IndexedMapOfShape()
        TopExp.MapShapes_s(body.solid, TopAbs_FACE, new_map)
        new_ids, colors = [], {}
        for index in range(1, new_map.Extent() + 1):
            shape = new_map.FindKey(index)
            if any(shape.IsSame(half) for half in halves):
                new_ids.append(index)
                if face_index in old_colors:
                    colors[index] = old_colors[face_index]
                continue
            old_index = old_map.FindIndex(shape)
            if old_index > 0 and old_index in old_colors:
                colors[index] = old_colors[old_index]
        self._adjacency_cache = {}
        body.mesh = tessellate_body(body.solid, preview_deflection(self.bounds()))
        body.mesh.face_colors = colors
        return new_ids

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
