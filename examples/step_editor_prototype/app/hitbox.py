"""Hitbox math: metadata-only volumes that declare where a pin electrically
meets the world (the design intent's 'IPC footprint in 3D'). Two shapes:

  obb         {type, center[3], half_extents[3], rotation_z_deg}
  convex_hull {type, vertices[[x,y,z], ...]}

All pure functions — the GUI only collects clicks and parameters."""

from __future__ import annotations

import math

from pathlib import Path

import numpy as np


def obb_from_three_clicks(
    p1,
    p2,
    p3,
    *,
    z_range: tuple[float, float],
    margin: float = 0.0,
) -> dict:
    """p1 -> p2 is the base edge (length + Z rotation), p3 the opposite
    corner (width). Height comes from z_range (usually the pin's Z extent)."""
    a = np.asarray(p1, dtype=np.float64)
    b = np.asarray(p2, dtype=np.float64)
    c = np.asarray(p3, dtype=np.float64)

    u = b[:2] - a[:2]
    length = float(np.linalg.norm(u))
    if length < 1.0e-9:
        raise ValueError("first two clicks coincide in XY")
    u /= length
    perp = np.array([-u[1], u[0]])
    width = float((c[:2] - a[:2]) @ perp)

    center_xy = a[:2] + u * (length * 0.5) + perp * (width * 0.5)
    z_min, z_max = min(z_range), max(z_range)
    return {
        "type": "obb",
        "center": [float(center_xy[0]), float(center_xy[1]), (z_min + z_max) * 0.5],
        "half_extents": [
            length * 0.5 + margin,
            abs(width) * 0.5 + margin,
            (z_max - z_min) * 0.5 + margin,
        ],
        "rotation_z_deg": math.degrees(math.atan2(u[1], u[0])),
    }


def obb_from_points(
    points: np.ndarray,
    *,
    margin: float = 0.0,
    height_override: float | None = None,
    flat_height: float | None = None,
) -> dict:
    """Axis-aligned box around a point set (auto-box / BGA cube / pad prism).
    flat_height clamps the box to a thin prism rising from the point minimum
    (ground pads); height_override forces a symmetric height about center."""
    points = np.asarray(points, dtype=np.float64)
    low = points.min(axis=0)
    high = points.max(axis=0)
    center = (low + high) * 0.5
    half = (high - low) * 0.5 + margin

    if flat_height is not None:
        center[2] = low[2] + flat_height * 0.5
        half[2] = flat_height * 0.5
    elif height_override is not None:
        half[2] = height_override * 0.5

    return {
        "type": "obb",
        "center": [float(v) for v in center],
        "half_extents": [float(v) for v in half],
        "rotation_z_deg": 0.0,
    }


def cube_from_point(center, size: float) -> dict:
    """BGA ball cube centered on a click."""
    center = np.asarray(center, dtype=np.float64)
    half = max(size, 1.0e-9) * 0.5
    return {
        "type": "obb",
        "center": [float(v) for v in center],
        "half_extents": [half, half, half],
        "rotation_z_deg": 0.0,
    }


def _hull_vertices(points: np.ndarray) -> np.ndarray:
    """Convex-hull vertices via scipy's Qhull (no networkx needed, unlike
    trimesh's hull, whose normal fixing imports it). 'QJ' joggles flat or
    degenerate point sets (a pad face) instead of failing."""
    from scipy.spatial import ConvexHull

    try:
        return points[ConvexHull(points).vertices]
    except Exception:  # noqa: BLE001 - Qhull rejects coplanar input
        return points[ConvexHull(points, qhull_options="QJ").vertices]


def convex_hull_hitbox(points: np.ndarray, *, margin: float = 0.0, max_vertices: int = 64) -> dict:
    """Convex hull of the pin's mesh vertices, dilated by margin from the
    hull centroid (curved pins)."""
    points = np.unique(np.asarray(points, dtype=np.float64), axis=0)
    vertices = _hull_vertices(points)
    if margin > 0.0:
        centroid = vertices.mean(axis=0)
        offsets = vertices - centroid
        norms = np.linalg.norm(offsets, axis=1, keepdims=True)
        norms[norms < 1.0e-12] = 1.0
        vertices = _hull_vertices(vertices + offsets / norms * margin)
    if len(vertices) > max_vertices:
        # Keep the hull's extreme points: farthest-point sampling.
        chosen = [int(np.argmax(np.linalg.norm(vertices - vertices.mean(axis=0), axis=1)))]
        distance = np.linalg.norm(vertices - vertices[chosen[0]], axis=1)
        while len(chosen) < max_vertices:
            chosen.append(int(np.argmax(distance)))
            distance = np.minimum(distance, np.linalg.norm(vertices - vertices[chosen[-1]], axis=1))
        vertices = _hull_vertices(vertices[chosen])
    return {"type": "convex_hull", "vertices": [[float(v) for v in row] for row in vertices]}


def pin_solid_ids(document, pin, pins) -> list[int]:
    """The bodies that ARE this pin: its own `body_ids`, else any body its seed
    faces sit on that is a separate piece used by no other pin and is not the
    package (the largest body). A pin found by its tip face on a separate lead
    therefore gets the whole lead, not just the tip."""
    if pin.body_ids:
        return list(pin.body_ids)
    from .document import shape_volume

    volumes = [shape_volume(b.solid) or 0.0 for b in document.bodies]
    if len(volumes) < 2:
        return []
    package = int(np.argmax(volumes))
    users: dict[int, set[int]] = {}
    for other in pins:
        for body_id, _face in other.face_ids:
            users.setdefault(body_id, set()).add(id(other))
    return sorted({body_id for body_id, _face in pin.face_ids
                   if body_id != package and users.get(body_id) == {id(pin)}
                   and volumes[body_id] < 0.25 * volumes[package]})


def thicken_flat(points: np.ndarray, size: float, toward=None) -> np.ndarray:
    """Give a flat point set (a pin known only by one face) a thickness so its
    hull is a volume. House convention by where the face lies:
    - horizontal, off the board (a THT tip below, a lead foot above): run
      to the Z-Sit plane, Z = 0
    - horizontal, on the board: `size` upward (never under the board)
    - vertical (a side contact): `size` along the face normal, into the
      part (toward `toward`, e.g. the package centre)"""
    points = np.asarray(points, dtype=np.float64)
    centre = points.mean(axis=0)
    _u, s, vt = np.linalg.svd(points - centre, full_matrices=False)
    if len(s) < 3 or s[-1] > 1.0e-3 * max(np.sqrt(len(points)), 1.0):
        return points  # already a volume
    normal = vt[-1] / np.linalg.norm(vt[-1])
    lifted = points.copy()
    if abs(normal[2]) > 0.7:
        z = float(centre[2])
        lifted[:, 2] = 0.0 if abs(z) > 0.05 else z + max(size, 0.2)
    else:
        if toward is not None and np.dot(np.asarray(toward, dtype=float) - centre, normal) < 0:
            normal = -normal
        lifted = points + normal * max(size, 0.2)
    return np.vstack([points, lifted])


SEAT_TOLERANCE = 0.02  # mm: a tip this far below Z = 0 is under the board


def leg_below_seat(document, pin) -> np.ndarray | None:
    """Points of the metal a face-marked pin hangs from, from its tip up to the
    Z-Sit plane: the pin's body cut at Z = 0, keeping the connected piece below
    the board that holds the tip faces (an ANCHOR shield tab, a THT lead on a
    unibody). Without this the hitbox only covered the seeded tip face. None
    when the pin has no faces, sits on or above the board, or nothing is
    found below it."""
    from .pins import mesh_region_centroid

    if not pin.face_ids or pin.body_ids:
        return None
    body = pin.face_ids[0][0]
    mesh = document.bodies[body].mesh if 0 <= body < len(document.bodies) else None
    tip = mesh_region_centroid(mesh, sorted(f for b, f in pin.face_ids if b == body)) if mesh is not None else None
    if tip is None or float(tip[2]) > -SEAT_TOLERANCE:
        return None
    return _connected_below(mesh, np.asarray(tip, dtype=np.float64))


def _connected_below(mesh, tip: np.ndarray) -> np.ndarray | None:
    import pyvista as pv

    surface = pv.PolyData(mesh.points, np.column_stack([np.full(len(mesh.tris), 3), mesh.tris]).ravel()).clean()
    below = surface.clip(normal=(0.0, 0.0, 1.0), origin=(0.0, 0.0, 0.0), invert=True)
    if below.n_points == 0:
        return None
    leg = below.connectivity("closest", closest_point=tip)
    return np.asarray(leg.points, dtype=np.float64) if leg.n_points >= 4 else None


def _face_pin_points(document, pin, size: float) -> np.ndarray:
    """Points for a pin known by faces: below the board, the metal it hangs
    from up to Z = 0 (an ANCHOR tab); else its faces thickened the house way."""
    from .pins import pin_mesh_points

    leg = leg_below_seat(document, pin)
    if leg is not None:
        return leg
    points = pin_mesh_points(document, pin)
    if not len(points):
        return points
    return thicken_flat(points, size, _package_centre(document))


def _package_centre(document):
    """The centre of the package (the largest body): where a vertical side
    contact is thickened toward."""
    from .document import shape_volume

    volumes = [shape_volume(b.solid) or 0.0 for b in document.bodies]
    package = document.bodies[int(np.argmax(volumes))].mesh if volumes else None
    return package.points.mean(axis=0) if package is not None else None


BENT_HULL_RATIO = 1.5  # hull volume / metal volume above this ...
BENT_ELONGATION = 2.5  # ... AND longest side >= this x the thinnest: a long bent lead (gull-wing, J, DIP leg; QFN / DFN terminals are 2.0)
BALL_PACKAGES = ("BGA", "CSP")  # model-name families whose round pins are balls (BGA, DSBGA, WLCSP ...)


def pin_hitbox(document, pin, pins, *, margin: float = 0.05, size: float = 0.2) -> dict | None:
    """The rule-based hitbox for a pin:
    - a BGA ball: a box around the ball;
    - a pin with its own solid(s): the convex hull of that metal, or a shrink
      wrap when the lead is bent (its hull is much bigger than the metal, so a
      hull would fill the bend with air); a paper-thin solid (TI draws a QFN's
      exposed pad as a 1 um plate) is thickened like a face first;
    - a face pin: its faces thickened by `thicken_flat` (or its leg up to Z=0).
    A surface pin's hitbox never reaches under the board (Z = 0)."""
    solids = pin_solid_ids(document, pin, pins)
    points, thin = _pin_points(document, pin, solids, size)
    if len(points) < 4:
        return None
    if is_ball(document, pin, points):
        return obb_from_points(points, margin=margin)
    if solids and not thin and is_bent_lead(document, solids, points):
        hitbox = metal_wrap_hitbox(document, solids, margin=margin) or shrink_wrap_hitbox(points, margin=margin)
    else:
        hitbox = convex_hull_hitbox(points, margin=margin)
    return keep_above_board(hitbox, points)


def _pin_points(document, pin, solids, size: float):
    """(the pin's points, True when they came from a paper-thin solid)."""
    if not solids:
        return _face_pin_points(document, pin, size), False
    blocks = [document.bodies[b].mesh.points for b in solids if document.bodies[b].mesh is not None]
    points = np.concatenate(blocks) if blocks else np.empty((0, 3))
    if len(points) < 3:
        return points, False
    thickened = thicken_flat(points, size, _package_centre(document))
    return thickened, len(thickened) > len(points)


SURFACE_TOLERANCE = 0.01  # mm: metal starting this close to Z = 0 sits ON the board


def keep_above_board(hitbox: dict | None, points: np.ndarray) -> dict | None:
    """A surface pin (its metal starts at the board) keeps its hitbox at or
    above Z = 0: the margin must not dig into the board. Pins that go through
    the board (metal below Z = 0) are left alone."""
    if hitbox is None or not len(points) or float(np.min(points[:, 2])) < -SURFACE_TOLERANCE:
        return hitbox
    vertices = np.asarray(hitbox.get("vertices") or [], dtype=float)
    if not len(vertices) or vertices[:, 2].min() >= 0.0:
        return hitbox
    vertices[:, 2] = np.maximum(vertices[:, 2], 0.0)
    return {**hitbox, "vertices": vertices.tolist()}


def _welded_surface(meshes):
    """The pin's meshes as one welded triangle surface (None if too small)."""
    import pyvista as pv

    parts = [pv.PolyData(m.points, np.column_stack([np.full(len(m.tris), 3), m.tris]).ravel())
             for m in meshes if m is not None]
    if not parts:
        return None
    surface = (parts[0] if len(parts) == 1 else parts[0].merge(parts[1:])).clean(tolerance=1e-6).triangulate()
    return surface if surface.n_cells >= 4 else None


def metal_wrap_hitbox(document, solids, *, margin: float = 0.05) -> dict | None:
    """A shrink wrap that IS the lead: the pin solid's own closed surface,
    welded and pushed out by `margin` along its normals. Watertight, follows
    every bend, and holds all of the metal (the point-cloud alpha shape tore
    holes in thin bent leads and left up to 79% of the metal outside)."""
    surface = _welded_surface([document.bodies[b].mesh for b in solids])
    if surface is None:
        return None
    surface = surface.compute_normals(point_normals=True, cell_normals=False, split_vertices=False,
                                      consistent_normals=True, auto_orient_normals=True)
    if margin > 0.0:
        surface.points = surface.points + surface.point_data["Normals"] * margin
    faces = surface.faces.reshape(-1, 4)[:, 1:]
    return {"type": "shrink_wrap",
            "vertices": [[float(v) for v in row] for row in surface.points],
            "faces": [[int(i) for i in tri] for tri in faces]}


def is_ball(document, pin, points: np.ndarray) -> bool:
    """A BGA / chip-scale ball: tagged so, or a roughly cube-shaped pin on a
    model of a ball-grid family."""
    if getattr(pin, "kind", "") == "bga":
        return True
    name = Path(getattr(document, "path", "") or "").stem.upper()
    if not any(tag in name for tag in BALL_PACKAGES):
        return False
    extent = np.ptp(np.asarray(points, dtype=np.float64), axis=0)
    return float(extent.max()) <= 1.5 * max(float(extent.min()), 1e-9)


def is_bent_lead(document, solids, points: np.ndarray) -> bool:
    """A long lead whose metal fills well under 1/BENT_HULL_RATIO of its
    convex hull: a hull would fill the bend with air. Short L-shaped QFN / DFN
    terminals stay hulls."""
    extent = np.ptp(np.asarray(points, dtype=np.float64), axis=0)
    if float(extent.max()) < BENT_ELONGATION * max(float(extent.min()), 1e-9):
        return False
    from scipy.spatial import ConvexHull

    from .document import shape_volume

    metal = sum(shape_volume(document.bodies[b].solid) or 0.0 for b in solids)
    try:
        hull = ConvexHull(np.unique(np.asarray(points, dtype=np.float64), axis=0)).volume
    except Exception:  # noqa: BLE001 - flat / degenerate: not a bent lead
        return False
    return metal > 0 and hull > BENT_HULL_RATIO * metal


def shrink_wrap_hitbox(points: np.ndarray, *, margin: float = 0.0, detail: float = 0.25) -> dict:
    """A concave 'shrink wrap' around the pin: the alpha shape of its points
    (alpha = `detail` x the pin's size), pushed out by `margin` along the
    surface normals. Hugs L-shaped and stepped leads a convex hull would
    bridge (e.g. PowerPAK / QFN pads with notches). Falls back to the convex
    hull when the alpha shape comes out empty."""
    import pyvista as pv

    points = np.unique(np.asarray(points, dtype=np.float64), axis=0)
    size = float(np.linalg.norm(points.max(axis=0) - points.min(axis=0))) or 1.0
    try:
        surface = pv.PolyData(points).delaunay_3d(alpha=detail * size).extract_surface(algorithm="dataset_surface")
        surface = surface.triangulate().clean()
    except Exception:  # noqa: BLE001 - degenerate input: use the convex hull
        surface = None
    if surface is None or surface.n_cells < 4:
        return convex_hull_hitbox(points, margin=margin)
    if margin > 0.0:
        surface = surface.compute_normals(auto_orient_normals=True, point_normals=True, cell_normals=False)
        surface.points = surface.points + surface.point_data["Normals"] * margin
    faces = surface.faces.reshape(-1, 4)[:, 1:]
    return {"type": "shrink_wrap",
            "vertices": [[float(v) for v in row] for row in surface.points],
            "faces": [[int(i) for i in tri] for tri in faces]}


def resize_hitbox(hitbox: dict, new_bounds, step: float = 0.01) -> dict:
    """Stretch a hitbox to new axis-aligned bounds (xmin, xmax, ymin, ymax,
    zmin, zmax), snapped to `step` mm: every vertex keeps its relative place
    inside the box. OBBs become a convex hull of their stretched corners."""
    bounds = np.round(np.asarray(new_bounds, dtype=np.float64) / step) * step
    if hitbox.get("type") == "obb":
        hx, hy, hz = hitbox["half_extents"]
        corners = np.array([[sx * hx, sy * hy, sz * hz] for sx in (-1, 1) for sy in (-1, 1) for sz in (-1, 1)])
        angle = np.radians(float(hitbox.get("rotation_z_deg", 0.0)))
        rot = np.array([[np.cos(angle), -np.sin(angle), 0], [np.sin(angle), np.cos(angle), 0], [0, 0, 1]])
        vertices = corners @ rot.T + np.asarray(hitbox["center"], dtype=np.float64)
        hitbox = {"type": "convex_hull", "vertices": vertices.tolist()}
    vertices = np.asarray(hitbox["vertices"], dtype=np.float64)
    lo, hi = vertices.min(axis=0), vertices.max(axis=0)
    span = np.where(hi - lo > 1e-12, hi - lo, 1.0)
    new_lo, new_hi = bounds[0::2], bounds[1::2]
    stretched = new_lo + (vertices - lo) / span * (new_hi - new_lo)
    out = dict(hitbox)
    out["vertices"] = [[float(v) for v in row] for row in stretched]
    return out


def obb_contains(hitbox: dict, points: np.ndarray, *, tolerance: float = 1.0e-9) -> np.ndarray:
    """Boolean mask: which points lie inside the OBB."""
    points = np.asarray(points, dtype=np.float64)
    center = np.asarray(hitbox["center"], dtype=np.float64)
    half = np.asarray(hitbox["half_extents"], dtype=np.float64)
    angle = math.radians(-float(hitbox.get("rotation_z_deg", 0.0)))
    cos_a, sin_a = math.cos(angle), math.sin(angle)
    local = points - center
    x = local[:, 0] * cos_a - local[:, 1] * sin_a
    y = local[:, 0] * sin_a + local[:, 1] * cos_a
    z = local[:, 2]
    return (
        (np.abs(x) <= half[0] + tolerance)
        & (np.abs(y) <= half[1] + tolerance)
        & (np.abs(z) <= half[2] + tolerance)
    )


def obb_xy_cross_section(hitbox: dict) -> tuple[float, float, float, float, float]:
    """(cx, cy, half_w, half_h, rotation_deg) for 2D footprint drawing."""
    center = hitbox["center"]
    half = hitbox["half_extents"]
    return (
        float(center[0]),
        float(center[1]),
        float(half[0]),
        float(half[1]),
        float(hitbox.get("rotation_z_deg", 0.0)),
    )


# ------------------------------------------------------------------ cavities

def _face_tris(mesh, face: int):
    corners = mesh.points[mesh.tris[mesh.tri_face_ids == face]]
    if not len(corners):
        return None, None, None
    centres = corners.mean(axis=1)
    normals = np.cross(corners[:, 1] - corners[:, 0], corners[:, 2] - corners[:, 0])
    normals /= np.maximum(np.linalg.norm(normals, axis=1, keepdims=True), 1e-12)
    return corners, centres, normals


def edge_is_concave(mesh, face_a: int, face_b: int, tol: float) -> bool | None:
    """Is the edge shared by two faces CONCAVE (the material wraps around the
    open space, like the inside corner of a socket) rather than convex (a rim
    where the walls turn outward)? Voted over sample points along the shared
    edge: from each, step into face A (its nearest triangle) and ask whether
    that step goes to face B's outward side. None when the faces don't touch."""
    ca, ta, na = _face_tris(mesh, face_a)
    cb, tb, nb = _face_tris(mesh, face_b)
    if ca is None or cb is None:
        return None
    pa = ca.reshape(-1, 3)
    pb = cb.reshape(-1, 3)
    from scipy.spatial import cKDTree

    dist, _ = cKDTree(pb).query(pa)
    shared = np.unique(np.round(pa[dist <= tol], 6), axis=0)
    if not len(shared):
        return None
    if len(shared) > 12:
        shared = shared[np.linspace(0, len(shared) - 1, 12).astype(int)]
    votes = 0
    for point in shared:
        into_a = ta[np.argmin(np.linalg.norm(ta - point, axis=1))] - point
        normal_b = nb[np.argmin(np.linalg.norm(tb - point, axis=1))]
        votes += 1 if float(into_a @ normal_b) > 1e-9 else -1
    return votes > 0


def cavity_region(document, body_index: int, seed_faces, *, max_faces: int = 400) -> set:
    """The walls of the cavity a contact face sits in: grow from the seed
    across CONCAVE edges only, so growth stops at the first convex edge, the
    rim where the cavity opens out."""
    mesh = document.bodies[body_index].mesh
    adjacency = document.face_smooth_adjacency(body_index, None)
    b = document.bounds()
    tol = max(float(np.linalg.norm([b[1] - b[0], b[3] - b[2], b[5] - b[4]])) * 1e-6, 1e-6)
    region = set(int(f) for f in seed_faces)
    frontier = list(region)
    while frontier and len(region) < max_faces:
        face = frontier.pop()
        for neighbour in adjacency.get(face, ()):
            neighbour = int(neighbour)
            if neighbour in region:
                continue
            if edge_is_concave(mesh, face, neighbour, tol):
                region.add(neighbour)
                frontier.append(neighbour)
    return region


def cavity_hitbox(document, pin, *, margin: float = 0.0) -> dict | None:
    """A hitbox that FILLS the cavity a female contact sits in, up to the rim:
    the convex hull of the cavity's walls (found by cavity_region), shrunk
    by `margin` toward its centre so it stays clear of the walls. None when
    the pin has no faces or no cavity was found around them."""
    by_body: dict[int, list[int]] = {}
    for body_index, face in pin.face_ids:
        by_body.setdefault(body_index, []).append(face)
    points = []
    grew = False
    for body_index, faces in by_body.items():
        region = cavity_region(document, body_index, faces)
        grew = grew or len(region) > len(faces)
        mesh = document.bodies[body_index].mesh
        mask = np.isin(mesh.tri_face_ids, list(region))
        points.append(mesh.points[np.unique(mesh.tris[mask])])
    if not points or not grew:
        return None
    pts = np.unique(np.vstack(points), axis=0)
    if len(pts) < 4 or (pts.max(axis=0) - pts.min(axis=0)).min() < 1e-6:
        return None
    if margin > 0.0:
        centre = pts.mean(axis=0)
        pts = centre + (pts - centre) * np.maximum(
            1.0 - margin / np.maximum(np.linalg.norm(pts - centre, axis=1, keepdims=True), 1e-9), 0.0)
    return convex_hull_hitbox(pts)


def _cylinder_faces(document, pin) -> list:
    """(radius, axis point, axis direction, face mesh points) for every
    cylindrical face of the pin (its own solid's faces, or its face region)."""
    from OCP.BRepAdaptor import BRepAdaptor_Surface
    from OCP.GeomAbs import GeomAbs_Cylinder

    keys = list(pin.face_ids) or [(b, f) for b in pin.body_ids
                                  for f in range(1, document.bodies[b].mesh.face_count + 1)]
    out = []
    for body_index, face_index in keys:
        surface = BRepAdaptor_Surface(document.face_of(body_index, face_index))
        if surface.GetType() != GeomAbs_Cylinder:
            continue
        cylinder = surface.Cylinder()
        location, direction = cylinder.Location(), cylinder.Axis().Direction()
        mesh = document.bodies[body_index].mesh
        points = mesh.points[mesh.tris[mesh.tri_face_ids == face_index]].reshape(-1, 3)
        out.append((cylinder.Radius(), np.array([location.X(), location.Y(), location.Z()]),
                    np.array([direction.X(), direction.Y(), direction.Z()]), points))
    return out


def _arc_span(angles: np.ndarray) -> tuple[float, float]:
    """(start, sweep) of the smallest arc holding every angle (radians); a
    near-full ring comes back as the whole circle."""
    a = np.sort(np.mod(angles, 2 * np.pi))
    gaps = np.diff(np.concatenate([a, [a[0] + 2 * np.pi]]))
    k = int(np.argmax(gaps))
    if gaps[k] < np.radians(40):
        return 0.0, 2 * np.pi
    start = a[(k + 1) % len(a)]
    return float(start), float(2 * np.pi - gaps[k])


def _frame(axis: np.ndarray) -> tuple[np.ndarray, np.ndarray]:
    """Two unit vectors square to `axis` and to each other."""
    helper = np.array([1.0, 0.0, 0.0]) if abs(axis[0]) < 0.9 else np.array([0.0, 1.0, 0.0])
    u = np.cross(axis, helper)
    u /= np.linalg.norm(u)
    return u, np.cross(axis, u)


def _band_faces(steps: int, count: int, full: bool) -> list:
    """Triangles of an annular sector sampled at `count` angles, 2 radii x 2
    ends per angle (vertex (i, r, s) = (i * 2 + r) * 2 + s)."""
    def idx(i, ri, si):
        return ((i % count) * 2 + ri) * 2 + si

    faces = []
    for i in range(steps):
        j = i + 1
        for ri in (0, 1):  # inner and outer walls
            faces += [[idx(i, ri, 0), idx(j, ri, 0), idx(j, ri, 1)], [idx(i, ri, 0), idx(j, ri, 1), idx(i, ri, 1)]]
        for si in (0, 1):  # the two end rings
            faces += [[idx(i, 0, si), idx(j, 0, si), idx(j, 1, si)], [idx(i, 0, si), idx(j, 1, si), idx(i, 1, si)]]
    ends = () if full else (0, count - 1)
    for i in ends:  # close the two cut ends of the arc
        faces += [[idx(i, 0, 0), idx(i, 1, 0), idx(i, 1, 1)], [idx(i, 0, 0), idx(i, 1, 1), idx(i, 0, 1)]]
    return faces


def _annular_band(centre, axis, radius_in, radius_out, start, sweep, s0, s1, steps=48) -> tuple[list, list]:
    """Closed triangle mesh of an annular sector: radii in..out, angles
    start..start+sweep about `axis` through `centre`, from s0 to s1 along it."""
    u, v = _frame(axis)
    full = sweep >= 2 * np.pi - 1e-9
    count = steps if full else steps + 1
    angles = start + sweep * np.arange(count) / steps
    vertices = [centre + axis * s + (np.cos(t) * u + np.sin(t) * v) * r
                for t in angles for r in (radius_in, radius_out) for s in (s0, s1)]
    return [list(map(float, p)) for p in vertices], _band_faces(steps, count, full)


def shallow_concave_hitbox(document, pin, *, depth: float = 0.3, margin: float = 0.0) -> dict | None:
    """For a concave contact with a pin of another net in its middle (a
    barrel jack's sleeve around its centre post): a shallow band lying on the
    contact's curved face, from its radius inward by `depth`, over the arc
    and length the face covers. It never reaches the middle, so it cannot
    swallow the other net's pin the way a cavity fill does. None when the pin
    has no cylindrical face."""
    cylinders = _cylinder_faces(document, pin)
    if not cylinders:
        return None
    radius, origin, axis, _pts = max(cylinders, key=lambda c: len(c[3]) * c[0])
    axis = axis / np.linalg.norm(axis)
    points = np.vstack([c[3] for c in cylinders if abs(abs(c[2] @ axis) - 1) < 1e-3 and abs(c[0] - radius) < 0.05 * radius])
    along = (points - origin) @ axis
    radial = points - origin - np.outer(along, axis)
    u, v = _frame(axis)
    start, sweep = _arc_span(np.arctan2(radial @ v, radial @ u))
    outer = radius + margin
    inner = max(radius - depth, 1e-3)
    vertices, faces = _annular_band(origin, axis, inner, outer, start, sweep,
                                    float(along.min()) - margin, float(along.max()) + margin)
    return {"type": "shallow_concave", "vertices": vertices, "faces": faces,
            "axis_point": origin.tolist(), "axis": axis.tolist(), "radius_in": inner, "radius_out": outer,
            "arc_start_deg": float(np.degrees(start)), "arc_sweep_deg": float(np.degrees(sweep))}
