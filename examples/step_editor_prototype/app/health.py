"""Model health: one detector and one fix per kind of problem (step 1).

Each check returns its instances; each fix is a pure function of the document
and its recorded inputs, so the journal can replay it. STEP problems come
first; mesh-import problems (STL / OBJ / board exports) follow.

  far_from_origin   the model sits far away in its own space (fix: move its
                    geometric mean to the origin, so Z-Sit has it in view)
  units             a size that only makes sense in inches or metres (fix: scale)
  surfaces_only     faces with no solid (fix: sew into a closed solid)
  openings          open edge loops in a shell (fix: fill, flat / smooth / tangent)
  inside_out        a solid with negative volume (fix: flip it)
  invalid           topology that fails OpenCascade's check (fix: ShapeFix)
  double_vertices   distinct vertices closer than the merge distance (fix: sew at it)
  star_vertices     vertices fanning many edges (star N-gons) (fix: merge same-surface faces)
  tiny_edges        edges shorter than the tolerance, slivers (fix: ShapeFix small edges)
  z_fighting        coincident faces of two bodies (fix: push one body off along
                    its face normal; the arrow picks which body moves)
  disjoint          a body made of several separate solids, or a floating sliver
                    (fix: split into bodies, or delete slivers)
"""

from __future__ import annotations

import numpy as np
from OCP.BRep import BRep_Tool
from OCP.BRepAdaptor import BRepAdaptor_Surface
from OCP.BRepBuilderAPI import BRepBuilderAPI_Sewing, BRepBuilderAPI_Transform
from OCP.BRepCheck import BRepCheck_Analyzer
from OCP.GeomAbs import GeomAbs_Plane
from OCP.gp import gp_Pnt, gp_Trsf, gp_Vec
from OCP.ShapeAnalysis import ShapeAnalysis_FreeBounds
from OCP.ShapeFix import ShapeFix_Shape
from OCP.ShapeUpgrade import ShapeUpgrade_UnifySameDomain
from OCP.TopAbs import TopAbs_EDGE, TopAbs_FACE, TopAbs_REVERSED, TopAbs_SOLID, TopAbs_VERTEX, TopAbs_WIRE
from OCP.TopExp import TopExp
from OCP.TopoDS import TopoDS
from OCP.TopTools import TopTools_IndexedDataMapOfShapeListOfShape, TopTools_IndexedMapOfShape

STAR_EDGES = 10        # a vertex this many edges meet at is a star
SLIVER_SHARE = 0.001   # a separate solid under 0.1% of the body's volume is a sliver
NUDGE_MM = 0.005       # how far a Z-fighting body is pushed off
MAX_FACE_PAIRS = 2_000_000  # Z-fighting work cap on huge models
MAX_MARKS = 4000       # edges / vertices drawn per highlighted finding


def _shapes(shape, kind) -> list:
    found = TopTools_IndexedMapOfShape()
    TopExp.MapShapes_s(shape, kind, found)
    return [found.FindKey(i) for i in range(1, found.Extent() + 1)]


def _vertex_xyz(vertex) -> np.ndarray:
    p = BRep_Tool.Pnt_s(TopoDS.Vertex_s(vertex))
    return np.array([p.X(), p.Y(), p.Z()])


def _edge_polyline(edge, samples: int = 16) -> np.ndarray:
    from OCP.BRepAdaptor import BRepAdaptor_Curve

    curve = BRepAdaptor_Curve(TopoDS.Edge_s(edge))
    points = (curve.Value(float(u)) for u in np.linspace(curve.FirstParameter(), curve.LastParameter(), samples))
    return np.array([[p.X(), p.Y(), p.Z()] for p in points])


def shape_marks(shape) -> tuple[list, list]:
    """(edge polylines, vertex xyz) of a shape, for highlighting."""
    lines = []
    for edge in _shapes(shape, TopAbs_EDGE)[:MAX_MARKS]:
        if BRep_Tool.Degenerated_s(TopoDS.Edge_s(edge)):
            continue  # a pole / seam with no 3D curve
        try:
            lines.append(_edge_polyline(edge))
        except Exception:  # noqa: BLE001 - an edge without a curve just isn't drawn
            continue
    return lines, [_vertex_xyz(v) for v in _shapes(shape, TopAbs_VERTEX)[:MAX_MARKS]]


def item_marks(document, item: dict) -> tuple[list, list]:
    """What lights up for one finding: the edges / vertices its detector
    recorded, else every edge and vertex of the bodies it names."""
    if "lines" in item or "points" in item:
        return item.get("lines", []), item.get("points", [])
    lines, points = [], []
    for key in ("body", "body_a", "body_b"):
        index = item.get(key)
        if index is not None and 0 <= index < len(document.bodies):
            more_lines, more_points = shape_marks(document.bodies[index].solid)
            lines += more_lines
            points += more_points
    return lines, points


def _volume(shape) -> float:
    from OCP.BRepGProp import BRepGProp
    from OCP.GProp import GProp_GProps

    props = GProp_GProps()
    BRepGProp.VolumeProperties_s(shape, props)
    return props.Mass()


def _transform(shape, trsf):
    return BRepBuilderAPI_Transform(shape, trsf, True).Shape()


def model_size(document) -> float:
    b = document.bounds()
    return float(max(b[1] - b[0], b[3] - b[2], b[5] - b[4]))


# ------------------------------------------------------------------ detectors

def far_from_origin(document) -> list[dict]:
    b = document.bounds()
    centre = np.array([(b[0] + b[1]) / 2, (b[2] + b[3]) / 2, (b[4] + b[5]) / 2])
    size = max(model_size(document), 1e-6)
    distance = float(np.linalg.norm(centre))
    return [{"label": f"{distance:.1f} mm from the origin ({distance / size:.0f}x its size)"}] \
        if distance > 5 * size and distance > 10.0 else []


def units(document) -> list[dict]:
    size = model_size(document)
    if size < 0.05:
        return [{"label": f"only {size:.4f} mm across: probably metres", "factor": 1000.0}]
    if size > 2000.0:
        return [{"label": f"{size:.0f} mm across: probably micrometres or a wrong unit", "factor": 0.001}]
    if size < 0.5:
        return [{"label": f"{size:.3f} mm across: maybe inches", "factor": 25.4}]
    return []


def surfaces_only(document) -> list[dict]:
    return [{"body": i, "label": f"body {i}: faces with no solid"}
            for i, body in enumerate(document.bodies) if not _shapes(body.solid, TopAbs_SOLID)]


CRACK_SHARE = 0.05  # a loop whose sides are this close (share of its size) is a crack


def crack_gap(wire) -> float | None:
    """How far apart the two sides of a loop are, if the loop is a CRACK (an
    unsewn seam: every edge has a twin running alongside it) rather than a
    hole; None for a hole."""
    from scipy.spatial import cKDTree

    polys = [_edge_polyline(e, 8) for e in _shapes(wire, TopAbs_EDGE)
             if not BRep_Tool.Degenerated_s(TopoDS.Edge_s(e))]
    if len(polys) < 2:
        return None
    every = np.vstack(polys)
    size = float(np.linalg.norm(every.max(axis=0) - every.min(axis=0))) or 1.0
    widest = 0.0
    for k, poly in enumerate(polys):
        others = np.vstack([q for j, q in enumerate(polys) if j != k])
        widest = max(widest, float(cKDTree(others).query(poly)[0].max()))
    return widest if widest < CRACK_SHARE * size else None


def openings(document) -> list[dict]:
    """Open loops of free edges: a HOLE (fill it) or a CRACK (an unsewn seam,
    both sides within a hair of each other: sew it shut)."""
    out = []
    for i, body in enumerate(document.bodies):
        free = ShapeAnalysis_FreeBounds(body.solid, 1.0e-6, False, False)
        for k, wire in enumerate(_shapes(free.GetClosedWires(), TopAbs_WIRE)):
            lines, points = shape_marks(wire)
            gap = crack_gap(wire)
            kind = f"crack, sides {gap:.4f} mm apart" if gap is not None else "hole"
            out.append({"body": i, "wire": k, "lines": lines, "points": points, "gap": gap,
                        "label": f"body {i}: opening {k + 1}: {kind} ({len(_shapes(wire, TopAbs_EDGE))} edges)"})
    return out


def inside_out(document) -> list[dict]:
    """Bodies with a closed solid of negative volume (each solid on its own:
    an open shell in the same body has no meaningful volume)."""
    return [{"body": i, "label": f"body {i}: negative volume"}
            for i, body in enumerate(document.bodies)
            if any(_volume(solid) < 0 for solid in _shapes(body.solid, TopAbs_SOLID))]


def invalid(document) -> list[dict]:
    return [{"body": i, "label": f"body {i}: fails the topology check"}
            for i, body in enumerate(document.bodies) if not BRepCheck_Analyzer(body.solid).IsValid()]


def _void_shells(solid) -> tuple[list, list]:
    """(outer shells, void shells) of a solid: a shell that closes into a
    negative volume on its own faces inward, a hollow inside the solid."""
    from OCP.BRepBuilderAPI import BRepBuilderAPI_MakeSolid
    from OCP.TopAbs import TopAbs_SHELL

    from .document import shape_volume

    outers, voids = [], []
    for shell in _shapes(solid, TopAbs_SHELL):
        maker = BRepBuilderAPI_MakeSolid(TopoDS.Shell_s(shell))
        volume = shape_volume(maker.Solid()) if maker.IsDone() else None
        (voids if volume is not None and volume < 0 else outers).append((shell, abs(volume or 0.0)))
    return outers, voids


def internal_voids(document, max_share: float = 0.05) -> list[dict]:
    """Bodies with hollow shells sealed inside them (STEP BREP_WITH_VOIDS):
    usually CAD leftovers, a hairline gap enclosed when the vendor merged parts
    (JST JWPF: two 0.05 mm slivers in the housing wall). Other readers turn
    each void shell into a phantom solid. Only bodies whose voids together are
    at most `max_share` of the body's volume are listed: a big cavity may be
    meant and is never offered for filling."""
    from .document import shape_volume

    out = []
    for i, body in enumerate(document.bodies):
        outers, voids = _void_shells(body.solid)
        if len(outers) != 1 or not voids:
            continue
        total = sum(v for _s, v in voids)
        if total <= max_share * abs(shape_volume(body.solid) or total or 1.0):
            out.append({"body": i, "voids": len(voids), "volume_mm3": total,
                        "label": f"body {i}: {len(voids)} internal void(s), {total:.3g} mm3"})
    return out


def fill_voids(document, body_index: int) -> bool:
    """Fill a body's internal voids: rebuild it from its outer shell only.
    Face colours follow their faces. True when the body changed."""
    from .document import _without_voids

    changed = _without_voids(document.bodies[body_index])
    if changed:
        document._adjacency_cache = {}
    return changed


def _edge_joined(shape, vertex_map) -> set[tuple[int, int]]:
    """Index pairs (0-based, into vertex_map) of the two ends of every edge."""
    joined = set()
    for edge in _shapes(shape, TopAbs_EDGE):
        edge = TopoDS.Edge_s(edge)
        a = vertex_map.FindIndex(TopExp.FirstVertex_s(edge)) - 1
        b = vertex_map.FindIndex(TopExp.LastVertex_s(edge)) - 1
        joined.add((min(a, b), max(a, b)))
    return joined


def double_vertices(document, merge_mm: float = 0.001) -> list[dict]:
    """Two separate vertices at (almost) the same spot with NO edge between
    them: faces that meet there were never sewn. Two ends of one short edge
    are not doubles (that is a tiny edge, its own check)."""
    from scipy.spatial import cKDTree

    out = []
    for i, body in enumerate(document.bodies):
        vertex_map = TopTools_IndexedMapOfShape()
        TopExp.MapShapes_s(body.solid, TopAbs_VERTEX, vertex_map)
        if vertex_map.Extent() < 2:
            continue
        xyz = np.array([_vertex_xyz(vertex_map.FindKey(k)) for k in range(1, vertex_map.Extent() + 1)])
        pairs = cKDTree(xyz).query_pairs(merge_mm) - _edge_joined(body.solid, vertex_map)
        if pairs:
            close = sorted({k for pair in pairs for k in pair})[:MAX_MARKS]
            out.append({"body": i, "points": list(xyz[close]),
                        "label": f"body {i}: {len(pairs)} vertex pair(s) within {merge_mm} mm"})
    return out


def _hub_directions(hub_xyz: np.ndarray, edges) -> np.ndarray:
    """Unit direction each edge leaves the hub in (from its first sample step)."""
    directions = []
    for edge in edges:
        if BRep_Tool.Degenerated_s(TopoDS.Edge_s(edge)):
            continue
        line = _edge_polyline(edge, samples=8)
        step = line[1] - line[0] if np.linalg.norm(line[0] - hub_xyz) <= np.linalg.norm(line[-1] - hub_xyz)             else line[-2] - line[-1]
        length = np.linalg.norm(step)
        if length > 0:
            directions.append(step / length)
    return np.array(directions)


def _is_flat_fan(directions: np.ndarray, planar_deg: float) -> bool:
    """Every direction lies in one plane through the hub (within planar_deg)."""
    if len(directions) < 3:
        return False
    normal = np.linalg.svd(directions)[2][-1]
    return float(np.max(np.abs(directions @ normal))) <= np.sin(np.radians(planar_deg))


def _is_star(ancestors, k: int, min_edges: int, planar_deg: float) -> bool:
    edges = ancestors.FindFromIndex(k)
    return edges.Extent() >= min_edges and _is_flat_fan(
        _hub_directions(_vertex_xyz(ancestors.FindKey(k)), edges), planar_deg)


def star_vertices(document, min_edges: int = STAR_EDGES, planar_deg: float = 1.0) -> list[dict]:
    """Star (N-gon fan) vertices: at least min_edges edges meet AND they all
    leave the hub in one plane, as a fan triangulating a flat face does. A
    hub whose edges spread in 3D (a cone tip, a corner) is real shape."""
    out = []
    for i, body in enumerate(document.bodies):
        ancestors = TopTools_IndexedDataMapOfShapeListOfShape()
        TopExp.MapShapesAndAncestors_s(body.solid, TopAbs_VERTEX, TopAbs_EDGE, ancestors)
        hubs = [k for k in range(1, ancestors.Extent() + 1) if _is_star(ancestors, k, min_edges, planar_deg)]
        if hubs:
            lines = [_edge_polyline(edge) for k in hubs[:MAX_MARKS // max(min_edges, 1)]
                     for edge in ancestors.FindFromIndex(k)
                     if not BRep_Tool.Degenerated_s(TopoDS.Edge_s(edge))]
            out.append({"body": i, "lines": lines, "points": [_vertex_xyz(ancestors.FindKey(k)) for k in hubs],
                        "label": f"body {i}: {len(hubs)} star vertex(es)"})
    return out


def tiny_edges(document, tolerance_mm: float = 0.0001) -> list[dict]:
    """Edges under 0.1 um: real slivers. EDA watermark lettering is built
    from ~1 um edges on purpose, so the cutoff sits well below that."""
    from OCP.BRepGProp import BRepGProp
    from OCP.GProp import GProp_GProps

    out = []
    for i, body in enumerate(document.bodies):
        tiny = []
        for edge in _shapes(body.solid, TopAbs_EDGE):
            if BRep_Tool.Degenerated_s(TopoDS.Edge_s(edge)):
                continue  # a sphere's pole / seam: zero length by design
            props = GProp_GProps()
            BRepGProp.LinearProperties_s(edge, props)
            if props.Mass() < tolerance_mm:
                tiny.append(edge)
        if tiny:
            ends = [_vertex_xyz(v) for edge in tiny[:MAX_MARKS] for v in _shapes(edge, TopAbs_VERTEX)]
            out.append({"body": i, "lines": [_edge_polyline(e) for e in tiny[:MAX_MARKS]], "points": ends,
                        "label": f"body {i}: {len(tiny)} edge(s) under {tolerance_mm} mm"})
    return out


def _planar_faces(body):
    faces = []
    for face in _shapes(body.solid, TopAbs_FACE):
        surface = BRepAdaptor_Surface(TopoDS.Face_s(face))
        if surface.GetType() != GeomAbs_Plane:
            continue
        plane = surface.Plane()
        n = plane.Axis().Direction()
        normal = np.array([n.X(), n.Y(), n.Z()])
        if face.Orientation() == TopAbs_REVERSED:
            normal = -normal
        xyz = np.array([_vertex_xyz(v) for v in _shapes(face, TopAbs_VERTEX)])
        if len(xyz):
            faces.append((normal, xyz, face))
    return faces


def z_fighting(document, tolerance_mm: float = 1e-4) -> list[dict]:
    """Pairs of bodies with a coincident, overlapping planar face (a pin face
    lying exactly in the package's bottom face, say)."""
    from .document import shape_bounds

    boxes = [np.asarray(shape_bounds([body.solid])) for body in document.bodies]
    touching = [(a, b) for a in range(len(boxes)) for b in range(a + 1, len(boxes))
                if np.all(boxes[a][0::2] <= boxes[b][1::2] + tolerance_mm)
                and np.all(boxes[b][0::2] <= boxes[a][1::2] + tolerance_mm)]
    planar: dict[int, list] = {}
    out = []
    budget = MAX_FACE_PAIRS  # huge models: stop rather than stall (the count is then a floor)
    for a, b in touching:
        for k in (a, b):
            if k not in planar:
                planar[k] = _planar_faces(document.bodies[k])
        budget -= len(planar[a]) * len(planar[b])
        if budget < 0:
            break
        for na, va, face_a in planar[a]:
            hit = None
            for nb, vb, face_b in planar[b]:
                # same-facing AND coincident: two surfaces drawn on top of each
                # other. Opposite-facing coincident faces are only two solids
                # touching (normal after a split), not Z-fighting.
                if float(na @ nb) < 1.0 - 1e-6:
                    continue
                if abs(float((vb.mean(axis=0) - va.mean(axis=0)) @ na)) > tolerance_mm:
                    continue
                lo = np.maximum(va.min(axis=0), vb.min(axis=0))
                hi = np.minimum(va.max(axis=0), vb.max(axis=0))
                overlap = np.delete(hi - lo, int(np.argmax(np.abs(na))))
                if np.all(overlap > tolerance_mm):
                    hit = (na, nb, face_a, face_b)
                    break
            if hit is not None:
                volumes = [abs(_volume(document.bodies[k].solid)) for k in (a, b)]
                mover = 0 if volumes[0] <= volumes[1] else 1  # the smaller body (the pin) moves
                lines_a, points_a = shape_marks(hit[2])
                lines_b, points_b = shape_marks(hit[3])
                out.append({"body_a": a, "body_b": b, "normal_a": hit[0].tolist(),
                            "normal_b": hit[1].tolist(), "mover": mover,
                            "lines": lines_a + lines_b, "points": points_a + points_b,
                            "label": f"bodies {a} / {b}: coincident faces"})
                break
    return out


SLIVER_EXTENT = 0.10  # ...and under 10% of the model's size in EVERY direction


def _is_sliver(volume: float, volumes: list[float], extent: float, total: float, size: float,
               max_extent: float = SLIVER_EXTENT) -> bool:
    """Tiny in volume AND in extent, and one of a kind. A thin plate (a QFN's
    exposed pad, a label) is tiny in volume but wide: a part, not debris.
    Hundreds of identical tiny bodies are BGA balls or pins."""
    twins = sum(1 for v in volumes if abs(v - volume) <= 0.2 * volume)
    return 0.0 < volume < SLIVER_SHARE * total and extent < max_extent * size and twins < 3


def disjoint(document, sliver_extent: float = SLIVER_EXTENT) -> list[dict]:
    from .document import shape_bounds

    out = []
    for i, body in enumerate(document.bodies):
        solids = _shapes(body.solid, TopAbs_SOLID)
        if len(solids) > 1:
            out.append({"body": i, "label": f"body {i}: {len(solids)} separate solids"})
    volumes = [abs(_volume(b.solid)) if _shapes(b.solid, TopAbs_SOLID) else 0.0 for b in document.bodies]
    total = sum(volumes) or 1.0
    size = model_size(document)
    for i, volume in enumerate(volumes):
        box = shape_bounds([document.bodies[i].solid])
        extent = max(box[1] - box[0], box[3] - box[2], box[5] - box[4])
        if _is_sliver(volume, volumes, extent, total, size, sliver_extent) and not _touches_another(document, i):
            out.append({"body": i, "sliver": True, "label": f"body {i}: floating sliver"})
    return out


ATTACHED_MM = 0.01  # a tiny body this close to another sits ON it (a marking), it does not float


def _touches_another(document, index: int, tol: float = ATTACHED_MM) -> bool:
    """True when body `index` touches another body: a pin-1 dot or a logo
    decal on the package (BTN8982TA's 0.001 mm3 dot was deleted as a
    'floating sliver'). Only debris that floats free of everything is a sliver."""
    from OCP.BRepExtrema import BRepExtrema_DistShapeShape

    from .document import shape_bounds

    lo = np.array(shape_bounds([document.bodies[index].solid]))
    for j, other in enumerate(document.bodies):
        if j == index:
            continue
        box = np.array(shape_bounds([other.solid]))
        if (lo[0::2] > box[1::2] + tol).any() or (box[0::2] > lo[1::2] + tol).any():
            continue  # boxes apart: cannot touch
        dist = BRepExtrema_DistShapeShape(document.bodies[index].solid, other.solid)
        if dist.IsDone() and dist.Value() <= tol:
            return True
    return False


CHECKS = [  # (key, label, detector): STEP issues first, mesh / board imports after
    ("far_from_origin", "Far from origin", far_from_origin),
    ("units", "Units", units),
    ("surfaces_only", "Surfaces, no solid", surfaces_only),
    ("openings", "Openings", openings),
    ("inside_out", "Flipped normals (inside out)", inside_out),
    ("invalid", "Invalid topology", invalid),
    ("internal_voids", "Internal voids", internal_voids),
    ("double_vertices", "Double vertices", double_vertices),
    ("star_vertices", "Star vertices (N-gon fans)", star_vertices),
    ("tiny_edges", "Tiny edges / slivers", tiny_edges),
    ("z_fighting", "Z-fighting faces", z_fighting),
    ("disjoint", "Disjoint geometry", disjoint),
]


# ---------------------------------------------------------------------- fixes

def recentre(document, target: str = "mean") -> np.ndarray:
    """Move the model to the origin; returns the matrix. target: 'mean' (the
    mesh's geometric mean), 'box' (bounding-box centre) or 'seat' (box centre
    in X / Y, bottom at Z = 0)."""
    points = np.vstack([b.mesh.points for b in document.bodies if b.mesh is not None])
    lo, hi = points.min(axis=0), points.max(axis=0)
    centre = {"mean": points.mean(axis=0), "box": (lo + hi) / 2,
              "seat": np.array([(lo[0] + hi[0]) / 2, (lo[1] + hi[1]) / 2, lo[2]])}[target]
    matrix = np.eye(4)
    matrix[:3, 3] = -centre
    document.apply_trsf(matrix)
    return matrix


def scale(document, factor: float) -> None:
    trsf = gp_Trsf()
    trsf.SetScale(gp_Pnt(0, 0, 0), float(factor))
    for body in document.bodies:
        body.solid = _transform(body.solid, trsf)
    document.remesh_all()


def sew(document, body_index: int, tolerance_mm: float = 1e-4) -> None:
    from .tools.repair import repair_solid

    body = document.bodies[body_index]
    body.solid, _done = repair_solid(body.solid, tolerance_mm)
    document.remesh_body(body_index)


def _fill_patch(wire, mode: str):
    """The face that closes a hole: flat (planar loops only), smooth or tangent."""
    from OCP.BRepBuilderAPI import BRepBuilderAPI_MakeFace
    from OCP.BRepOffsetAPI import BRepOffsetAPI_MakeFilling
    from OCP.GeomAbs import GeomAbs_C0, GeomAbs_G1

    if mode == "flat":
        maker = BRepBuilderAPI_MakeFace(wire, True)
        return maker.Face() if maker.IsDone() else None  # not flat: choose smooth / tangent
    try:
        filler = BRepOffsetAPI_MakeFilling()
        for edge in _shapes(wire, TopAbs_EDGE):
            filler.Add(TopoDS.Edge_s(edge), GeomAbs_G1 if mode == "tangent" else GeomAbs_C0)
        filler.Build()
        return filler.Shape() if filler.IsDone() else None
    except Exception:  # noqa: BLE001 - OpenCascade refuses some loops outright
        return None


def _meshes(patch) -> bool:
    """A patch that cannot be triangulated would be invisible and wreck the
    model's bounds: refuse it."""
    from OCP.BRepMesh import BRepMesh_IncrementalMesh
    from OCP.TopLoc import TopLoc_Location

    BRepMesh_IncrementalMesh(patch, 0.05, False, 0.5, True)
    return all(BRep_Tool.Triangulation_s(TopoDS.Face_s(f), TopLoc_Location()) is not None
               for f in _shapes(patch, TopAbs_FACE))


def sew_shut(document, body_index: int, tolerance_mm: float) -> bool:
    """Sew a body's faces with a gap of tolerance_mm (closes cracks), then
    rebuild its solids."""
    from .tools.repair import repair_solid

    body = document.bodies[body_index]
    sewing = BRepBuilderAPI_Sewing(tolerance_mm)
    sewing.Add(body.solid)
    sewing.Perform()
    body.solid, _done = repair_solid(sewing.SewedShape(), tolerance_mm)
    document.remesh_body(body_index)
    return True


def _patch_fits(patch, wire) -> bool:
    """A fill patch must stay near its loop: one that balloons far outside it
    (a filling surface gone wild) would wreck the model's size."""
    from OCP.Bnd import Bnd_Box
    from OCP.BRepBndLib import BRepBndLib

    def box(shape):
        b = Bnd_Box()
        BRepBndLib.AddOptimal_s(shape, b, False, False)
        return np.array(b.Get())

    loop, fill = box(wire), box(patch)
    reach = max(float(np.max(loop[3:] - loop[:3])), 1e-6)
    return bool(np.all(fill[:3] >= loop[:3] - reach) and np.all(fill[3:] <= loop[3:] + reach))


def fill_opening(document, body_index: int, wire_index: int, mode: str = "flat",
                 tolerance_mm: float = 1e-4) -> bool:
    """Close one open loop: 'flat' = a planar face on the loop, 'smooth' = a
    surface interpolated through it, 'tangent' = that surface kept tangent to
    the faces around it. Then sew and close the shell."""
    from OCP.BRepBuilderAPI import BRepBuilderAPI_MakeFace
    from OCP.BRepOffsetAPI import BRepOffsetAPI_MakeFilling
    from OCP.GeomAbs import GeomAbs_C0, GeomAbs_G1

    body = document.bodies[body_index]
    free = ShapeAnalysis_FreeBounds(body.solid, 1.0e-6, False, False)
    wires = _shapes(free.GetClosedWires(), TopAbs_WIRE)
    if not (0 <= wire_index < len(wires)):
        return False
    wire = TopoDS.Wire_s(wires[wire_index])
    if mode == "sew":  # a crack: sew its two sides together, no new face
        return sew_shut(document, body_index, tolerance_mm)
    patch = _fill_patch(wire, mode)
    if patch is None or not _patch_fits(patch, wire) or not _meshes(patch):
        return False  # the mode chosen is the mode used: no silent fallback
    sewing = BRepBuilderAPI_Sewing(tolerance_mm)
    sewing.Add(body.solid)
    sewing.Add(patch)
    sewing.Perform()
    from .tools.repair import repair_solid

    body.solid, _done = repair_solid(sewing.SewedShape(), tolerance_mm)
    document.remesh_body(body_index)
    return True


def flip(document, body_index: int) -> None:
    """Turn each inside-out solid of the body the right way out."""
    from .tools.repair import _outward

    body = document.bodies[body_index]
    body.solid, _flipped = _outward(body.solid)
    document.remesh_body(body_index)


def fix_topology(document, body_index: int, tolerance_mm: float = 1e-4) -> None:
    body = document.bodies[body_index]
    fixer = ShapeFix_Shape(body.solid)
    fixer.SetPrecision(tolerance_mm)
    fixer.Perform()
    if not fixer.Shape().IsNull():
        body.solid = fixer.Shape()
        document.remesh_body(body_index)


def merge_vertices(document, body_index: int, merge_mm: float) -> None:
    sew(document, body_index, merge_mm)


def unify_faces(document, body_index: int, angle_deg: float = 0.0) -> None:
    """Merge faces that lie on one surface: removes the star fans. angle_deg
    > 0 also merges faces that meet at up to that angle (near-flat fans)."""
    body = document.bodies[body_index]
    unify = ShapeUpgrade_UnifySameDomain(body.solid, True, True, True)
    if angle_deg > 0:
        unify.SetAngularTolerance(np.radians(angle_deg))
    unify.Build()
    body.solid = unify.Shape()
    document.remesh_body(body_index)


def connected_pieces(document, body_index: int, keep_still: set[int]) -> list[int]:
    """The body plus every piece touching it, transitively (a pin split into
    a tail and a mouth moves as one), never the package (the largest body)
    nor the bodies in `keep_still`."""
    from .document import shape_bounds

    volumes = [abs(_volume(b.solid)) for b in document.bodies]
    still = set(keep_still) | {int(np.argmax(volumes))}
    boxes = [np.asarray(shape_bounds([b.solid])) for b in document.bodies]

    def touch(a, b):
        return np.all(boxes[a][0::2] <= boxes[b][1::2] + 1e-4) and np.all(boxes[b][0::2] <= boxes[a][1::2] + 1e-4)

    group, frontier = {body_index}, [body_index]
    while frontier:
        current = frontier.pop()
        for other in range(len(boxes)):
            if other not in group and other not in still and touch(current, other):
                group.add(other)
                frontier.append(other)
    return sorted(group)


def nudge(document, body_index: int, normal, distance: float = NUDGE_MM, partner: int | None = None) -> None:
    """Push a body, with the pieces it is made of, off a coincident face
    along `normal`."""
    trsf = gp_Trsf()
    trsf.SetTranslation(gp_Vec(*(np.asarray(normal, dtype=float) * distance)))
    keep = {partner} if partner is not None else set()
    for index in connected_pieces(document, body_index, keep):
        body = document.bodies[index]
        body.solid = _transform(body.solid, trsf)
        document.remesh_body(index)


def split_or_drop(document, body_index: int, sliver: bool) -> None:
    """A sliver body is deleted; a multi-solid body becomes one body per solid."""
    from copy import copy

    body = document.bodies[body_index]
    if sliver:
        del document.bodies[body_index]
        return
    pieces = []
    for solid in _shapes(body.solid, TopAbs_SOLID):
        piece = copy(body)
        piece.solid = solid
        pieces.append(piece)
    document.bodies[body_index:body_index + 1] = pieces
    document.remesh_all()


_FIXES = {  # action -> how one recorded fix replays (document, op)
    "far_from_origin": lambda d, op: d.apply_trsf(np.asarray(op["matrix"])),
    "units": lambda d, op: scale(d, op["factor"]),
    "surfaces_only": lambda d, op: sew(d, op["body"], op.get("tolerance", 1e-4)),
    "openings": lambda d, op: fill_opening(d, op["body"], op["wire"], op.get("mode", "flat"),
                                           op.get("tolerance", 1e-4)),
    "inside_out": lambda d, op: flip(d, op["body"]),
    "internal_voids": lambda d, op: fill_voids(d, op["body"]),
    "invalid": lambda d, op: fix_topology(d, op["body"], op.get("tolerance", 1e-4)),
    "tiny_edges": lambda d, op: fix_topology(d, op["body"], op.get("tolerance", 1e-4)),
    "double_vertices": lambda d, op: merge_vertices(d, op["body"], op.get("merge_mm", 0.001)),
    "star_vertices": lambda d, op: unify_faces(d, op["body"], op.get("angle_deg", 0.0)),
    "z_fighting": lambda d, op: nudge(d, op["body"], op["normal"], op.get("distance", NUDGE_MM), op.get("partner")),
    "disjoint": lambda d, op: split_or_drop(d, op["body"], op.get("sliver", False)),
}


def apply_fix(document, op: dict) -> None:
    """Replay one recorded fix ({'action': key, ...inputs}); unknown actions
    are ignored, as before."""
    fix = _FIXES.get(op["action"])
    if fix is not None:
        fix(document, op)
