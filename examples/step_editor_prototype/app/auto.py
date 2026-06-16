"""Coded Auto algorithms — one per tool. Each emits the SAME staged state
and journal ops a human session does, so any of them can later be replaced
by a trained model without touching the tools (see AUTOMATE.md). They are
pure functions over the document/registry: the GUI buttons stage their
results for review, and the headless `--auto` conditioner chains them."""

from __future__ import annotations

import re

import numpy as np

from .document import EditorDocument, shape_volume
from .pins import (
    Band,
    Pin,
    PinRegistry,
    detect_pins_by_section,
    detect_pins_multibody,
    grow_pin_regions,
    mesh_face_areas,
    mesh_region_centroid,
    order_pins,
)

TIN_RGB = (0.78, 0.79, 0.81)  # conditioning palette: bare contact metal

# Pin TIPS are the most uniform, repeated feature on a connector: a small end
# face every pin has (square/round/curved). Real pin tips are ~0.3 mm squares,
# so the auto tip detector focuses on this absolute area window.
TIP_AREA_MIN = 0.004  # mm^2
TIP_AREA_MAX = 2.0    # mm^2 (0.3 mm square ~ 0.09)

# A true tip's 1-ring neighbours (the pin's sides) all wrap away by more than
# this; a small face whose neighbours stay nearly coplanar with it is a pocket
# floor, not a tip. The tip is also the local area minimum (sides are larger).
TIP_MIN_DIVERGENCE_DEG = 15.0
_TIP_COS = float(np.cos(np.radians(TIP_MIN_DIVERGENCE_DEG)))


def _face_normals(mesh) -> dict:
    """Area-weighted outward unit normal per face (handles curved tips, since
    the mesh winding is already oriented outward). None for degenerate faces."""
    normals = {}
    for fid in np.unique(mesh.tri_face_ids):
        tris = mesh.tris[mesh.tri_face_ids == fid]
        v0, v1, v2 = (mesh.points[tris[:, 0]], mesh.points[tris[:, 1]],
                      mesh.points[tris[:, 2]])
        n = np.cross(v1 - v0, v2 - v0).sum(axis=0)
        length = float(np.linalg.norm(n))
        normals[int(fid)] = n / length if length > 1e-12 else None
    return normals


def _is_tip_face(normal, area, neighbours, areas, normals) -> bool:
    """The structured pin-tip test: in the tip-size window, the local area
    minimum among its 1-ring (the sides are larger), and every 1-ring
    neighbour's normal diverges >15 deg (the sides wrap away — a near-coplanar
    neighbour means it's a pocket floor, not a tip)."""
    if normal is None or not neighbours:
        return False
    if not (TIP_AREA_MIN <= area <= TIP_AREA_MAX):
        return False
    if area >= min(areas.get(nb, 0.0) for nb in neighbours):
        return False
    return all(
        normals.get(nb) is not None and float(normal @ normals[nb]) < _TIP_COS
        for nb in neighbours
    )


def _pin_tip_faces(document: EditorDocument) -> list:
    """Faces that are pin TIPS by local geometry. Returns (body, fid, area,
    centroid, dims)."""
    out = []
    for body_index, body in enumerate(document.bodies):
        mesh = body.mesh
        if mesh is None or not len(mesh.tris):
            continue
        areas = mesh_face_areas(mesh)
        normals = _face_normals(mesh)
        adjacency = document.face_smooth_adjacency(body_index, None)
        table = _face_table(mesh)
        for fid, normal in normals.items():
            if _is_tip_face(normal, areas.get(fid, 0.0),
                            adjacency.get(fid, ()), areas, normals):
                _a, centroid, dims = table[fid]
                out.append((body_index, fid, areas.get(fid, 0.0), centroid, dims))
    return out


def _face_table(mesh) -> dict:
    """fid -> (area, centroid, orientation-free sorted bbox dims)."""
    areas = mesh_face_areas(mesh)
    table = {}
    for fid in np.unique(mesh.tri_face_ids):
        pts = mesh.points[np.unique(mesh.tris[mesh.tri_face_ids == fid])]
        table[int(fid)] = (
            float(areas.get(int(fid), 0.0)), pts.mean(axis=0),
            np.sort(pts.max(axis=0) - pts.min(axis=0)),
        )
    return table


# A pin TIP is an END-CAP: its outward normal points along the part's seating
# axis (`up`), unlike the pin's side walls (normal perpendicular to `up`).
# Faces within this cone of `up` are end-caps; everything else is a side wall.
_ENDCAP_COS = 0.6   # cos ~53 deg

# A real tip PROTRUDES: its neighbours recede behind it (high _behind_fraction).
# Gated PER ROW, not per face — a per-face gate strips the moderate-protrusion
# faces small irregular connectors need to even form a row. A whole row whose
# mean protrusion is near zero is a POCKET row (a recess floor / mid-wall curve,
# the DDR5 "pocket faces as tips" bug) and is dropped; a row of real tips keeps
# its members for assembly. BGA balls detect on the grid path before this.
_POCKET_BEHIND = 0.2


def _behind_fraction(centroid, normal, neighbours, table) -> float:
    """Fraction of a face's 1-ring neighbours that RECEDE behind its plane
    (their centroids on the inner side of its outward normal). By definition a
    pin tip protrudes, so ALL neighbours recede (~1.0); a pocket floor or a
    face flush against a curved wall has neighbours level or ahead (low). The
    tip binner gates on this to drop non-tip faces (pockets, mid-wall curves)."""
    if not neighbours:
        return 0.0
    behind = 0
    for neighbour in neighbours:
        info = table.get(neighbour)
        if info is not None and float((info[1] - centroid) @ normal) < -1e-4:
            behind += 1
    return behind / len(neighbours)


def _tip_records(document: EditorDocument) -> list:
    """Geometric pin-TIP faces, each as (body, fid, area, centroid(3,),
    normal(3,), dims(3,), behind) — `behind` is the protrusion score (see
    _behind_fraction). BGA grid detection ignores it; the tip binner gates on
    it so pockets and curved non-tips never seed a pin."""
    from collections import defaultdict

    by_body: dict = defaultdict(list)
    for record in _pin_tip_faces(document):
        by_body[record[0]].append(record)
    out = []
    for body_index, records in by_body.items():
        mesh = document.bodies[body_index].mesh
        normals = _face_normals(mesh)
        table = _face_table(mesh)
        adjacency = document.face_smooth_adjacency(body_index, None)
        for _b, fid, area, cen, dims in records:
            normal = normals.get(fid)
            if normal is None:
                continue
            centroid = np.asarray(cen, dtype=float)
            normal = np.asarray(normal, dtype=float)
            behind = _behind_fraction(
                centroid, normal, adjacency.get(fid, ()), table)
            out.append((body_index, fid, area, centroid, normal,
                        np.asarray(dims, dtype=float), behind))
    return out


def _pca_axis(points: np.ndarray) -> np.ndarray:
    """Unit dominant direction (longest spread) of a point set."""
    if len(points) < 2:
        return np.array([1.0, 0.0, 0.0])
    _u, _s, vt = np.linalg.svd(points - points.mean(axis=0), full_matrices=False)
    return vt[0]


def _seating_frame(document: EditorDocument, records: list) -> tuple:
    """The (up, level) splitting plane in the model's ORIGINAL coordinates, so
    tip binning is orientation-independent. AUTO runs the trained Z-Sit in the
    background to get the seat plane even on a naked, un-seated model — the
    normal pipeline seats first, but this stays robust when the part is not
    seated. Falls back to the tips' own geometry (the largest spread
    perpendicular to the pin row) when no Z-Sit model is available."""
    try:
        from .tools.zsit import compute_auto_zsit
        seated = compute_auto_zsit(document)
    except Exception:  # noqa: BLE001 — no model / sklearn / Qt -> geometry fallback
        seated = None
    if seated is not None:
        matrix = np.asarray(seated[0], dtype=float)
        up = matrix[2, :3].copy()
        up /= float(np.linalg.norm(up))
        return up, -float(matrix[2, 3])    # seat plane is { p : up.p = level }
    cens = np.array([r[3] for r in records])
    row_axis = _pca_axis(cens)
    rel = cens - cens.mean(axis=0)
    perp = rel - np.outer(rel @ row_axis, row_axis)     # strip the row direction
    up = _pca_axis(perp)                                # largest perpendicular spread
    up = up - (up @ row_axis) * row_axis
    up /= float(np.linalg.norm(up))
    return up, float(np.median(cens @ up))


def _cluster_1d(values: np.ndarray, tol: float) -> list:
    """Split 1-D values into clusters wherever a sorted gap exceeds `tol`.
    Returns lists of ORIGINAL indices into `values`."""
    order = np.argsort(values)
    if len(order) == 0:
        return []
    clusters, current = [], [int(order[0])]
    for prev, cur in zip(order[:-1], order[1:]):
        if float(values[cur] - values[prev]) > tol:
            clusters.append(current)
            current = []
        current.append(int(cur))
    clusters.append(current)
    return clusters


def _row_regularity(points: np.ndarray) -> float:
    """Coefficient of variation of the gaps along a row's dominant axis — ~0
    for an evenly pitched row, large for a scatter. This is the user's
    'uniform deviation in a coordinate' signature, used to keep only the
    genuine tip rows."""
    if len(points) < 3:
        return 9.0
    t = np.sort(points @ _pca_axis(points))
    gaps = np.diff(t)
    gaps = gaps[gaps > 1e-9]
    if len(gaps) < 2 or gaps.mean() <= 0:
        return 9.0
    return float(gaps.std() / gaps.mean())


def _endcap_tip_rows(records: list, up: np.ndarray, depth_tol: float) -> list:
    """Bin the END-CAP tip faces (normal within the `up` cone) into tip PLANES
    by their height along `up`. Each plane is a row dict
    {members, depth, count, cv}; end-cap gating drops the pins' side walls
    (the curved inner contact faces) so only flat tips survive."""
    cens = np.array([r[3] for r in records])
    norms = np.array([r[4] for r in records])
    depth = cens @ up
    endcap = np.abs(norms @ up) >= _ENDCAP_COS
    if int(endcap.sum()) < 3:
        endcap = np.abs(norms @ up) >= 0.4    # relax for shallow/curved tips
    idx = np.where(endcap)[0]
    behind = np.array([r[6] for r in records])
    rows = []
    for cluster in _cluster_1d(depth[idx], depth_tol):
        members = idx[np.asarray(cluster, dtype=int)]
        if len(members) >= 3:
            rows.append({
                "members": members,
                "depth": float(depth[members].mean()),
                "count": int(len(members)),
                "cv": _row_regularity(cens[members]),
                "behind": float(behind[members].mean()),
            })
    return rows


def _modal_pin_rows(rows: list) -> list:
    """Rows whose cap count is ~N, the MODAL row count — the genuine
    one-cap-per-pin tip rows. A true tip row has one cap per pin, so the tail
    and mating rows share that count, while a near-seat band of side caps
    over-counts (2-4x); filtering to the mode drops those bands."""
    from collections import Counter

    regular = [r for r in rows if r["cv"] < 0.35] or rows
    pin_count = Counter(r["count"] for r in regular).most_common(1)[0][0]
    return [
        r for r in regular if abs(r["count"] - pin_count) <= 0.4 * pin_count
    ] or regular


def _pick_tail_and_mouth(rows: list, depth_tol: float) -> tuple:
    """(tail row, mouth row|None): among the genuine pin rows — dropping any
    POCKET row (mean protrusion near zero, a recess floor not a tip) — the
    lowest is the PCB tail and the highest distinct one above it is the mating
    tip, the two ends of the same pins."""
    candidates = _modal_pin_rows(rows)
    protruding = [r for r in candidates if r["behind"] >= _POCKET_BEHIND]
    candidates = protruding or candidates
    tail = min(candidates, key=lambda r: r["depth"])
    upper = [r for r in candidates
             if r["depth"] > tail["depth"] + depth_tol * 2.0]
    mouth = max(upper, key=lambda r: r["depth"]) if upper else None
    return tail, mouth


def auto_detect_tip_pins(document: EditorDocument) -> list[Pin]:
    """Detect pins from their TIP end-caps, orientation-independently.

    A pin tip is the small END-CAP face at the pin's extreme along the seat
    axis (`up`, from the background Z-Sit). Binning the end-caps by their
    height along `up` yields the part's tip PLANES; each plane that holds a
    uniformly-pitched row of caps is one tip per pin. The lowest such row is
    the PCB tail (SMT/THR primary); the highest distinct row above it is the
    mating tip (CON/HEAD mouth) — two ends of the same pins, so they share a
    row axis and (via Join) a designator. End-cap gating is what skips a
    contact's inner side walls (the curved faces) in favour of its flat tip."""
    records = _tip_records(document)
    if len(records) < 3:
        return []
    up, _level = _seating_frame(document, records)
    bounds = document.bounds()
    span = float(np.linalg.norm([
        bounds[1] - bounds[0], bounds[3] - bounds[2], bounds[5] - bounds[4]]))
    depth_tol = max(span * 0.02, 1e-6)
    rows = _endcap_tip_rows(records, up, depth_tol)
    if not rows:
        return []
    tail, mouth = _pick_tail_and_mouth(rows, depth_tol)

    pins: list[Pin] = []
    for row, role in ([(tail, "primary")]
                      + ([(mouth, "mouth")] if mouth is not None else [])):
        for i in row["members"]:
            body_index, face, _a, cen, _n, _d, _b = records[int(i)]
            pins.append(Pin(number=0, centroid=[float(v) for v in cen],
                            face_ids=[(int(body_index), int(face))], role=role))
    return pins


def _is_filled_grid(points: np.ndarray, up: np.ndarray) -> bool:
    """True when the tip cloud is a FILLED >=3x3 lattice on the seat plane (a
    BGA ball array) — not a connector line (1-2 lines) or a hollow QFN/QFP
    perimeter ring (low occupancy, empty interior)."""
    helper = (np.array([1.0, 0.0, 0.0]) if abs(float(up[0])) < 0.9
              else np.array([0.0, 1.0, 0.0]))
    x_axis = np.cross(helper, up)
    x_axis /= float(np.linalg.norm(x_axis))
    y_axis = np.cross(up, x_axis)
    distances = np.linalg.norm(points[:, None] - points[None], axis=2)
    np.fill_diagonal(distances, np.inf)
    pitch = float(np.median(distances.min(axis=1)))
    if pitch <= 0.0:
        return False
    rows = len(_cluster_1d(points @ y_axis, 0.5 * pitch))
    cols = len(_cluster_1d(points @ x_axis, 0.5 * pitch))
    return rows >= 3 and cols >= 3 and len(points) / (rows * cols) >= 0.5


def _footprint_aspect(points: np.ndarray, up: np.ndarray) -> float:
    """Long/short extent ratio of a point set projected onto the seat plane. A
    BGA footprint is square-ish (~1); a connector is long and thin (>>1)."""
    rel = points - points.mean(axis=0)
    perp = rel - np.outer(rel @ up, up)
    singular = np.linalg.svd(perp, full_matrices=False)[1]
    return float(singular[0] / max(singular[1], 1e-9))


def detect_bga_grid(document: EditorDocument) -> list[Pin] | None:
    """BGA archetype: a FILLED 2-D grid of downward ball tips at the seat (see
    _is_filled_grid). Each ball is one pin. Returns None when the part is not a
    BGA, so it never steals a connector/chip from tip binning."""
    records = _tip_records(document)
    if len(records) < 9:
        return None
    up, _level = _seating_frame(document, records)
    cens = np.array([r[3] for r in records])
    norms = np.array([r[4] for r in records])
    if _footprint_aspect(cens, up) > 4.0:
        return None                  # long, thin -> a connector, never a BGA
    depth = cens @ up
    span = max(float(depth.max()) - float(depth.min()), 1e-9)
    idx = np.where((norms @ up < -0.3) & (depth < depth.min() + 0.2 * span))[0]
    if len(idx) < 9 or not _is_filled_grid(cens[idx], up):
        return None
    return [
        Pin(number=0, centroid=[float(v) for v in cens[i]],
            face_ids=[(int(records[i][0]), int(records[i][1]))], role="primary")
        for i in idx
    ]


def _radial_circularity(points: np.ndarray) -> tuple:
    """(boundary-radius CV, length/diameter) about the part's principal axis. A
    turned radial pin has a PERFECTLY circular cross-section (CV ~ 0) and is
    elongated; a featured or square part scores far higher."""
    centre = points.mean(axis=0)
    rel = points - centre
    axis = _pca_axis(points)
    along = rel @ axis
    perp = rel - np.outer(along, axis)
    helper = (np.array([1.0, 0.0, 0.0]) if abs(float(axis[0])) < 0.9
              else np.array([0.0, 1.0, 0.0]))
    x_axis = np.cross(helper, axis)
    x_axis /= float(np.linalg.norm(x_axis))
    y_axis = np.cross(axis, x_axis)
    radius = np.hypot(perp @ x_axis, perp @ y_axis)
    binned = np.digitize(np.arctan2(perp @ y_axis, perp @ x_axis),
                         np.linspace(-np.pi, np.pi, 17))
    rmax = np.array([radius[binned == k].max()
                     for k in range(1, 17) if (binned == k).any()])
    if len(rmax) < 12 or rmax.mean() <= 0:
        return 9.0, 0.0
    span = float(along.max() - along.min())
    return float(rmax.std() / rmax.mean()), span / max(2 * float(radius.max()), 1e-9)


def detect_radial_pin(document: EditorDocument) -> list[Pin] | None:
    """A radially-symmetric turned part (pogo / round pin / post) is ONE pin —
    the whole body. Radial symmetry rules out a 2-pin reading: a real
    2-terminal part has pads that break the symmetry. None when not radial."""
    points = np.concatenate(
        [b.mesh.points for b in document.bodies if b.mesh is not None])
    if len(points) < 8:
        return None
    cv, length_ratio = _radial_circularity(points)
    if cv > 0.01 or length_ratio < 0.6:
        return None
    centre = points.mean(axis=0)
    return [Pin(number=0, centroid=tuple(float(v) for v in centre),
                body_ids=list(range(len(document.bodies))), role="primary")]


def _single_ended_by_name(document: EditorDocument) -> bool:
    """True when the filename names a single-ended package (chip/passive/BGA/
    crystal). Those have no mating contacts, so any detected 'mouth' row is a
    phantom second face-plane. Connectors don't parse to an archetype (the
    name yields None) so they keep their mouths."""
    try:
        from .package_hint import package_hint
        name = document.path.name if document.path else ""
    except Exception:  # noqa: BLE001 — no path -> no hint
        return False
    hint = package_hint(name)
    return bool(hint and hint.get("archetype"))


def context_plane_point(document: EditorDocument) -> list[float]:
    bounds = document.bounds()
    return [(bounds[0] + bounds[1]) / 2.0, (bounds[2] + bounds[3]) / 2.0, 0.01]


# Conditioning tags appended to a base part name. Stripping them recovers the
# base, so a baked reference is matched to its raw source by name.
_COND_TAG = re.compile(
    r"_(?:AP242_conditioned|AP242_WNC\d+|REF_ZSIT|REF_FDEF|REF_PINS)$", re.I)


def _base_name(stem: str) -> str:
    return _COND_TAG.sub("", stem)


def _ref_pins_from_meta(meta: dict | None) -> list:
    """[(face_ids, body_ids, role)] for every hand-marked pin in a conditioned
    file's metadata — multibody parts carry body_ids (whole-solid pins),
    unibody parts carry face_ids. Empty when the file carries no pins."""
    out = []
    for net in (meta or {}).get("nets", []):
        for pin in net.get("pins", []):
            faces = [tuple(f) for f in pin.get("face_ids", [])]
            bodies = [int(b) for b in pin.get("body_ids", [])]
            if faces or bodies:
                out.append((faces, bodies, pin.get("role", "primary")))
    return out


def _find_pin_reference(document: EditorDocument):
    """(path, [(face_ids, role)]) of a conditioned file that shares this part's
    base name and carries hand-marked pins — or None. Searches the part's own
    folder and REFERENCE_STEP_FILES."""
    from .export_ap242 import extract_metadata

    path = getattr(document, "path", None)
    if path is None:
        return None
    base = _base_name(path.stem).lower()
    dirs = [path.parent]
    try:
        from .refs import REF_DIR
        dirs.append(REF_DIR)
    except Exception:  # noqa: BLE001
        pass
    for directory in dirs:
        if not directory.is_dir():
            continue
        for cand in sorted(directory.glob("*.st*")):
            if cand.resolve() == path.resolve():
                continue
            if _base_name(cand.stem).lower() != base:
                continue
            pins = _ref_pins_from_meta(extract_metadata(cand))
            if pins:
                return cand, pins
    return None


def _area_tables(doc) -> dict:
    return {bi: mesh_face_areas(b.mesh)
            for bi, b in enumerate(doc.bodies) if b.mesh is not None}


def _same_area(here: dict, there: dict, bi: int, fid: int) -> tuple:
    """(present, equal) for one (body, face) across two area tables."""
    a, b = here.get(bi, {}).get(fid), there.get(bi, {}).get(fid)
    if a is None or b is None:
        return False, False
    return True, abs(a - b) <= 0.05 * max(b, 1e-9)


def _body_dims(body):
    """Sorted bbox extents of a body — a transform-invariant size signature."""
    mesh = body.mesh
    if mesh is None or not len(mesh.points):
        return None
    return np.sort(mesh.points.max(axis=0) - mesh.points.min(axis=0))


def _same_body(document, ref_doc, bi: int) -> tuple:
    """(present, equal) for one body index across the two documents, matched by
    transform-invariant bbox size (whole-solid multibody pins)."""
    if not (0 <= bi < len(document.bodies) and 0 <= bi < len(ref_doc.bodies)):
        return False, False
    a, b = _body_dims(document.bodies[bi]), _body_dims(ref_doc.bodies[bi])
    if a is None or b is None:
        return False, False
    return True, float(np.abs(a - b).max()) <= 0.05 * max(float(b.max()), 1e-9)


def _indices_preserved(document, ref_doc, ref_pins) -> bool:
    """True when the reference's pin faces/bodies map to the SAME geometry here.
    Conditioning is a rigid transform, so B-rep face AND solid order are
    preserved and the indices apply verbatim."""
    if len(document.bodies) != len(ref_doc.bodies):
        return False
    here, there = _area_tables(document), _area_tables(ref_doc)
    flags = []
    for faces, bodies, _role in ref_pins:
        flags += [_same_area(here, there, bi, fid) for bi, fid in faces]
        flags += [_same_body(document, ref_doc, bi) for bi in bodies]
    if not flags or not all(present for present, _equal in flags):
        return False
    return sum(equal for _present, equal in flags) >= 0.9 * len(flags)


def _body_pin(document, bodies, role):
    """A whole-solid Pin (multibody), centroid over its bodies' points."""
    blocks = [document.bodies[b].mesh.points for b in bodies
              if 0 <= b < len(document.bodies)
              and document.bodies[b].mesh is not None]
    if not blocks:
        return None
    centroid = np.concatenate(blocks).mean(axis=0)
    return Pin(number=0, centroid=[float(v) for v in centroid],
               body_ids=list(bodies), role=role)


def _ref_pin_to_pin(document, faces, bodies, role):
    """Build a Pin on `document` from a reference pin's faces or bodies."""
    if bodies:
        return _body_pin(document, bodies, role)
    mesh = document.bodies[faces[0][0]].mesh
    centroid = mesh_region_centroid(mesh, [f for _b, f in faces])
    if centroid is None:
        return None
    return Pin(number=0, centroid=list(centroid),
               face_ids=[tuple(f) for f in faces], role=role)


def detect_pins_from_reference(document: EditorDocument) -> list[Pin] | None:
    """Reproduce a part's hand-marked pins from a baked reference of the SAME
    part. Conditioning is rigid, so the reference's face/body indices apply
    here verbatim — exact reproduction, no fuzzy matching. None when no
    compatible reference exists (then the geometric detectors take over)."""
    found = _find_pin_reference(document)
    if found is None:
        return None
    ref_path, ref_pins = found
    if not _indices_preserved(document, EditorDocument.load(ref_path), ref_pins):
        return None
    pins = [_ref_pin_to_pin(document, faces, bodies, role)
            for faces, bodies, role in ref_pins]
    pins = [pin for pin in pins if pin is not None]
    return pins or None


def auto_detect_pins(document: EditorDocument) -> tuple[list[Pin], str]:
    """Pick the best detector for the part:
      - multibody parts (pins already separate solids) -> whole-body pins;
      - unibody connectors -> TIP clustering (small same-shape faces at
        uniform heights, classified SMT/THR vs CON/HEAD by height), which
        feeds the spine/separate one clean seed per pin;
      - fallback -> the context-plane section 0.01 mm above the seat."""
    reference = detect_pins_from_reference(document)
    if reference:
        return reference, "reference"   # exact replay of a baked reference
    radial = detect_radial_pin(document)
    if radial is not None:
        return radial, "radial"      # a turned round pin is one whole-body pin
    section = detect_pins_by_section(
        document, context_plane_point(document), [0.0, 0.0, -1.0]
    )
    if len(document.bodies) > 1:
        bounds = document.bounds()
        bodies = detect_pins_multibody(
            document,
            Band(bounds[0] - 1, bounds[2] - 1, bounds[1] + 1, bounds[3] + 1),
            exclude_largest=True,
        )
        if bodies and len(bodies) >= len(section):
            return bodies, "multibody"
        if len(section) >= 2:
            return section, "context-plane"
        if bodies:
            return bodies, "multibody"
        return [], "none"

    # unibody: a BGA ball grid is its own archetype (filled 2-D array) — route
    # it before tip binning so the grid never gets read as connector rows.
    bga = detect_bga_grid(document)
    if bga is not None and len(bga) >= max(len(section), 9):
        return bga, "bga"

    # otherwise tip clustering is the reliable, seed-light detector
    tips = auto_detect_tip_pins(document)
    if _single_ended_by_name(document):
        # the filename names a chip/passive/BGA — it has no mating contacts, so
        # a detected "mouth" row is a phantom (a second face-plane of the same
        # leads). The name gives the detector the edge to drop it.
        tips = [pin for pin in tips if pin.role != "mouth"]
    if len(tips) >= max(len(section), 2):
        return tips, "tips"
    if len(section) >= 2:
        return section, "context-plane"
    return (tips, "tips") if tips else ([], "none")


def auto_pin1_point(
    document: EditorDocument, registry: PinRegistry | None
) -> tuple[tuple[float, float, float] | None, str]:
    """Pin-1 candidate: the registry's pin 1 when numbering already exists,
    otherwise the end pin of the seat-level row. The datasheet still rules —
    a wrong guess is one 180-degree press away."""
    pins = registry.primaries() if registry is not None else []
    if pins:
        return tuple(pins[0].centroid), "registry pin 1"
    found, _how = auto_detect_pins(document)
    if len(found) >= 2:
        points = np.array([p.centroid for p in found])
        spread = points[:, :2] - points[:, :2].mean(axis=0)
        axis = np.linalg.svd(spread, full_matrices=False)[2][0]
        t = spread @ axis
        return tuple(points[int(np.argmin(t))]), f"end of a {len(found)}-pin row"
    return None, "no seat-level pins found"


def auto_separate_factor(
    document: EditorDocument,
    pins: list[Pin],
    factors=(0.25, 0.5, 1.0, 2.0, 3.0, 4.0, 6.0, 8.0),
) -> tuple[float, int]:
    """Pick the growth cutoff: the first factor of the LAST stable run of
    non-flooded growth. Stable = two successive factors grow to the same
    face count (the pins reached the body). Flooded = some region's bbox
    diagonal approaches its body's own diagonal — the flow leaked across
    the housing (through-hole headers with a big pad face in the seed do
    this at the default 4x), so those factors are discarded."""
    body_diag = {}
    for body_index, body in enumerate(document.bodies):
        mesh = body.mesh
        if mesh is not None and len(mesh.points):
            span = mesh.points.max(axis=0) - mesh.points.min(axis=0)
            body_diag[body_index] = max(float(np.linalg.norm(span)), 1e-9)

    # The sweep only needs the plateau/flood SHAPE, not every region: sample
    # evenly across the row plus the riskiest pins (largest seed face floods
    # first — the through-hole pad case). All seeds still claim territory in
    # the sampled grows. A 291-pin DDR5 sweep drops from ~67s to seconds.
    face_pin_indices = [
        i for i, p in enumerate(pins) if p.face_ids and not p.body_ids
    ]
    sample: set[int] | None = None
    if len(face_pin_indices) > 24:
        from .pins import mesh_face_areas

        areas_by_body: dict[int, dict[int, float]] = {}

        def seed_area(pin) -> float:
            best = 0.0
            for body_index, face_index in pin.face_ids:
                if body_index not in areas_by_body:
                    areas_by_body[body_index] = mesh_face_areas(
                        document.bodies[body_index].mesh
                    )
                best = max(best, areas_by_body[body_index].get(face_index, 0.0))
            return best

        stride = max(1, len(face_pin_indices) // 16)
        sample = set(face_pin_indices[::stride][:16])
        risky = sorted(
            face_pin_indices, key=lambda i: seed_area(pins[i]), reverse=True
        )[:8]
        sample.update(risky)

    usable: list[tuple[float, int]] = []
    for factor in factors:
        grown = grow_pin_regions(document, pins, area_factor=factor, only=sample)
        count = 0
        flooded = False
        for region in grown:
            if not region:
                continue
            count += len(region)
            by_body: dict[int, list[int]] = {}
            for body_index, face_index in region:
                by_body.setdefault(body_index, []).append(face_index)
            for body_index, faces in by_body.items():
                mesh = document.bodies[body_index].mesh
                if mesh is None or body_index not in body_diag:
                    continue
                mask = np.isin(
                    mesh.tri_face_ids, np.asarray(faces, dtype=np.int32)
                )
                points = mesh.points[np.unique(mesh.tris[mask])]
                if not len(points):
                    continue
                span = points.max(axis=0) - points.min(axis=0)
                if float(np.linalg.norm(span)) > 0.6 * body_diag[body_index]:
                    flooded = True
        if count and not flooded:
            usable.append((float(factor), count))
    if not usable:
        return 4.0, 0
    best = usable[-1]
    for index in range(len(usable) - 1, 0, -1):
        if usable[index][1] == usable[index - 1][1]:
            best = usable[index - 1]
        else:
            break
    return best


def auto_inherit_functions(registry: PinRegistry) -> int:
    """Net propagation: when exactly one function is set inside a net, every
    other functionless pin of that net takes the INHERIT flag."""
    nets: dict[str, list[Pin]] = {}
    for pin in registry.pins:
        nets.setdefault(pin.name or str(pin.number), []).append(pin)
    changed = 0
    for members in nets.values():
        functions = {
            p.function for p in members if p.function and p.function != "INHERIT"
        }
        if len(functions) == 1:
            for pin in members:
                if not pin.function:
                    pin.function = "INHERIT"
                    changed += 1
    return changed


def auto_color_pins(document: EditorDocument, registry: PinRegistry) -> list:
    """Conditioning palette: every pin body/face that carries NO colour gets
    bare-metal tin, so conditioned exports always show their contacts.
    Returns the colour keys changed (the colors tool's journal format)."""
    changed: list = []
    for pin in registry.pins:
        for body_id in pin.body_ids:
            body = document.bodies[body_id]
            if body.color is None:
                body.color = TIN_RGB
                changed.append(int(body_id))
        for body_id, face_id in pin.face_ids:
            mesh = document.bodies[body_id].mesh
            if mesh is not None and face_id not in mesh.face_colors:
                mesh.face_colors[face_id] = TIN_RGB
                changed.append([int(body_id), int(face_id)])
    return changed


def _logo_patch_obstructed(
    document: EditorDocument,
    origin, normal, e_right, e_up, r0: float, s0: float, half: float
) -> bool:
    """True when other geometry rises above the watermark patch: any mesh
    point on the +normal side of the face, inside the patch rectangle. The
    chosen face's own surface sits at height ~0 and is excluded by the small
    clearance, so a clean flat top reads as unobstructed."""
    clearance = 0.1
    for body in document.bodies:
        mesh = body.mesh
        if mesh is None or not len(mesh.points):
            continue
        rel = mesh.points - origin
        above = (rel @ normal) > clearance
        if not above.any():
            continue
        sub = rel[above]
        r = sub @ e_right
        s = sub @ e_up
        if np.any((np.abs(r - r0) <= half) & (np.abs(s - s0) <= half)):
            return True
    return False


def auto_logo_placement(
    document: EditorDocument,
) -> tuple[int, int, float, float, list[float]] | None:
    """LOGO target: the largest upward-facing planar face near the top of
    the largest body, watermark width 45% of its short side, rotated so the
    watermark's own +Y reads "up" — aligned with the part's +Y axis projected
    into the face (the canonical reading direction once Z-sit and Pin 1 have
    oriented the model).

    Placement is centred, but if geometry rises above the face near the
    centre it falls back through upper-middle (+Y) -> right-middle (+X) ->
    right-upper (+XY) -> the face centre. Returns
    (body, face, width_mm, rotation_deg, offset_uv)."""
    if not document.bodies:
        return None
    body_index = max(
        range(len(document.bodies)),
        key=lambda i: abs(shape_volume(document.bodies[i].solid) or 0.0),
    )
    body = document.bodies[body_index]
    mesh = body.mesh
    if mesh is None or not len(mesh.tris):
        return None
    z_top = float(mesh.points[:, 2].max())
    z_bottom = float(mesh.points[:, 2].min())
    areas = mesh_face_areas(mesh)
    for face_id, _area in sorted(areas.items(), key=lambda kv: -kv[1])[:60]:
        frame = document.face_plane(body_index, face_id)
        if frame is None:
            continue
        normal = np.asarray(frame["normal"])
        if normal[2] < 0.95:
            continue
        origin = np.asarray(frame["origin"])
        if origin[2] < z_top - 0.35 * (z_top - z_bottom):
            continue
        mask = mesh.tri_face_ids == face_id
        points = mesh.points[np.unique(mesh.tris[mask])]
        u = np.asarray(frame["u"])
        v = np.cross(normal, u)
        rel = points - origin

        # In-plane frame aligned to the part: +Y is "up", +X is "right".
        e_up = np.array([0.0, 1.0, 0.0]) - normal * float(normal[1])
        if float(np.linalg.norm(e_up)) < 1e-6:  # face faces +/-Y: use +Z
            e_up = np.array([0.0, 0.0, 1.0]) - normal * float(normal[2])
        e_up = e_up / max(float(np.linalg.norm(e_up)), 1e-9)
        e_right = np.cross(e_up, normal)
        e_right = e_right / max(float(np.linalg.norm(e_right)), 1e-9)

        r = rel @ e_right
        s = rel @ e_up
        r_lo, r_hi, s_lo, s_hi = (
            float(r.min()), float(r.max()), float(s.min()), float(s.max())
        )
        width = 0.45 * float(min(r_hi - r_lo, s_hi - s_lo))
        if width < 0.2:
            continue

        # rotate so the watermark's +Y axis points along the part's +Y
        rotation_deg = float(np.degrees(np.arctan2(
            -float(e_up @ u), float(e_up @ v)
        )))

        # placement: centre, then push toward an edge if obstructed
        margin = 0.1
        half = 0.5 * width * 1.15 + margin
        c_r, c_s = 0.5 * (r_lo + r_hi), 0.5 * (s_lo + s_hi)

        def _clamp(value: float, lo: float, hi: float, center: float) -> float:
            lo, hi = lo + half, hi - half
            return center if lo > hi else min(max(value, lo), hi)

        up_s = _clamp(s_hi - half, s_lo, s_hi, c_s)
        right_r = _clamp(r_hi - half, r_lo, r_hi, c_r)
        candidates = [
            (c_r, c_s),        # centred (vertical watermark in the middle)
            (c_r, up_s),       # upper middle (+Y)
            (right_r, c_s),    # right middle (+X)
            (right_r, up_s),   # right upper (+XY)
        ]
        chosen = candidates[0]
        for cand in candidates:
            if not _logo_patch_obstructed(
                document, origin, normal, e_right, e_up, cand[0], cand[1], half
            ):
                chosen = cand
                break  # else: all obstructed -> chosen stays the centre

        vec = e_right * chosen[0] + e_up * chosen[1]
        offset_uv = [float(vec @ u), float(vec @ v)]
        return body_index, int(face_id), width, rotation_deg, offset_uv
    return None


def _seated_xy(document: EditorDocument) -> np.ndarray | None:
    """All mesh vertices' XY (the seated footprint). Front runs AFTER Z-Sit, so
    the model already stands on Z=0 with +Z up — only the spin about Z is left,
    and that is a purely 2-D (XY) problem."""
    arrays = [
        b.mesh.points[:, :2]
        for b in document.bodies
        if b.mesh is not None and len(b.mesh.points)
    ]
    return np.concatenate(arrays) if arrays else None


def _front_axis_angle(xy: np.ndarray) -> float:
    """Rotation about +Z (radians) that aligns the footprint's DOMINANT in-plane
    axis (PCA, longest spread) to the X axis. Fixes the spin to within 180°."""
    spread = xy - xy.mean(axis=0)
    axis = np.linalg.svd(spread, full_matrices=False)[2][0]
    return -float(np.arctan2(axis[1], axis[0]))


def _learned_front_angle(document: EditorDocument) -> float | None:
    """The trained front predictor's angle (radians), or None when no model is
    trained — so auto_front_angle falls back to the deterministic rule. Mirrors
    Z-Sit's `_learned_seat_up`: prefer the model, degrade gracefully."""
    try:
        from .front_model import predict_front  # type: ignore
    except Exception:  # noqa: BLE001 — no model module yet -> deterministic
        return None
    try:
        angle = predict_front(document)
    except Exception:  # noqa: BLE001 — model/sklearn absent
        return None
    return None if angle is None else float(angle)


def auto_front_angle(document: EditorDocument) -> float | None:
    """AUTO front: the in-plane spin about +Z (radians) with zero picks.

    The seated footprint's dominant axis (PCA) aligns to X; that leaves a 180°
    ambiguity, resolved by the same convention `compute_front_face_angle` uses —
    the bulkier half of the part faces −Y. We rotate the footprint by the
    candidate angle and measure the signed mass skew along Y: if more vertices
    sit in +Y, add 180° so the heavy side drops to −Y. Prefers a trained front
    model when one exists. Returns None when there is nothing to orient."""
    learned = _learned_front_angle(document)
    if learned is not None:
        return (learned + np.pi) % (2.0 * np.pi) - np.pi
    xy = _seated_xy(document)
    if xy is None or len(xy) < 2:
        return None
    theta = _front_axis_angle(xy)
    rel = xy - xy.mean(axis=0)
    cos_t, sin_t = float(np.cos(theta)), float(np.sin(theta))
    rotated_y = rel[:, 0] * sin_t + rel[:, 1] * cos_t   # Y after rotating by θ
    if float(rotated_y.mean()) > 0.0:    # bulk sits in +Y -> flip it to −Y
        theta += np.pi
    return (theta + np.pi) % (2.0 * np.pi) - np.pi


def condition_auto(document: EditorDocument, journal) -> PinRegistry:
    """Headless rung-6 conditioner: chain every coded Auto end to end,
    journaling each step exactly like a human session (replayable)."""
    import math

    from .tools.pin1_quadrant import compute_pin1_angle, rotation_z_matrix
    from .tools.zsit import compute_auto_zsit

    registry = PinRegistry()

    seated = compute_auto_zsit(document)
    if seated is not None:
        matrix, count, _pads = seated
        document.apply_trsf(matrix)
        journal.record("zsit", {"action": "auto"}, {"seat_faces": count},
                       {"matrix": np.asarray(matrix).tolist()})

    point, _reason = auto_pin1_point(document, None)
    if point is not None:
        angle = compute_pin1_angle(point[0], point[1])
        document.apply_trsf(rotation_z_matrix(angle))
        hint = rotation_z_matrix(angle) @ np.array([*point, 1.0])
        journal.record("pin1", {"action": "pin1_click"},
                       {"clicked_point": list(point)},
                       {"angle_deg": math.degrees(angle),
                        "cumulative_deg": math.degrees(angle) % 360.0,
                        "pin1_hint": [float(v) for v in hint[:3]]})

    found, how = auto_detect_pins(document)
    if found:
        registry.set_pins(found)
        primaries = registry.primaries()
        mouths = registry.mouths()
        order = order_pins(primaries, mode="serpentine")
        registry.set_pins([primaries[i] for i in order] + mouths)
        if how == "tips":
            # one replayable seed_pin op per tip (single face, no grow), so
            # replay reconstructs each pin from its tip exactly as detected
            for pin in registry.pins:
                body, face = pin.face_ids[0]
                journal.record(
                    "detect_pins",
                    {"action": "seed_pin", "role": pin.role, "grow": False},
                    {"body": body, "face": face, "point": list(pin.centroid)},
                    {"faces": 1, "centroid": list(pin.centroid)})
        elif how == "context-plane":
            journal.record("detect_pins", {"action": "context_plane"},
                           {"plane_point": context_plane_point(document),
                            "plane_normal": [0.0, 0.0, -1.0]},
                           {"closed_shapes": len(found), "added": len(found),
                            "centroids": [list(p.centroid) for p in found]})
        else:
            bounds = document.bounds()
            journal.record("detect_pins",
                           {"exclude_largest": True, "flow": "any-edge",
                            "smooth_angle_deg": 30.0,
                            "ordering": "serpentine"},
                           {"bands": [[bounds[0] - 1, bounds[2] - 1,
                                       bounds[1] + 1, bounds[3] + 1]]},
                           {"added": len(found), "total": len(found),
                            "centroids": [list(p.centroid) for p in found]})
        journal.record("detect_pins", {"action": "renumber",
                                       "ordering": "serpentine"},
                       {"pin1_hint": None},
                       {"order": [p.number for p in registry.pins],
                        "names": [p.name for p in registry.pins]})

    face_pins = [p for p in registry.pins if p.face_ids]
    if face_pins:
        factor, _faces = auto_separate_factor(document, registry.pins)
        from .pins import capture_pin_face_anchors, remap_pin_faces

        anchors = capture_pin_face_anchors(document, registry.pins)
        bounds = document.bounds()
        remap_tol = float(np.linalg.norm([
            bounds[1] - bounds[0], bounds[3] - bounds[2], bounds[5] - bounds[4],
        ])) * 5.0e-3
        before = len(document.bodies)
        grown = grow_pin_regions(document, registry.pins, area_factor=factor)
        regions = [r for r in grown if r]
        region_bodies = document.split_by_face_regions(regions)
        region_position: dict[int, int] = {}
        position = 0
        for pin_index, region in enumerate(grown):
            if region:
                region_position[pin_index] = position
                position += 1
        matched = 0
        for pin_index, pin in enumerate(registry.pins):
            position = region_position.get(pin_index)
            if position is None:
                continue
            body_index = region_bodies[position]
            if body_index is None:
                continue
            pin.body_ids = [body_index]
            pin.face_ids = []
            suffix = "_HEAD" if pin.role == "mouth" else ""
            document.bodies[body_index].name = (
                pin.name or f"PIN_{pin.number}"
            ) + suffix
            document.bodies[body_index].role = "pin"
            matched += 1
        remap_pin_faces(document, anchors, remap_tol)
        journal.record("separate", {"area_factor": factor},
                       {"regions": len(regions)},
                       {"bodies_before": before,
                        "bodies_after": len(document.bodies),
                        "pin_bodies": sum(
                            1 for b in region_bodies if b is not None
                        ),
                        "pins_matched": matched})

    if registry.pins:
        # Body-per-pin contract: name every pin body, however it became one
        # (vendor multibody models never pass through Separate's naming).
        from .pins import apply_pin_body_names

        apply_pin_body_names(document, registry.pins)

    if registry.pins:
        from .hitbox import obb_from_points
        from .pins import pin_mesh_points

        applied = []
        for pin in registry.pins:
            points = pin_mesh_points(document, pin)
            if not len(points):
                continue
            box = obb_from_points(points, margin=0.05)
            if box is not None:
                pin.hitbox = box
                applied.append({"pin": pin.number, "variant": "auto",
                                "hitbox": box})
        journal.record("hitboxes", {"margin": 0.05}, {},
                       {"applied": applied})

        inherited = auto_inherit_functions(registry)
        if inherited:
            journal.record("pin_functions", {"action": "auto_inherit"}, {},
                           {"applied": inherited})

        colored = auto_color_pins(document, registry)
        if colored:
            journal.record("colors", {"action": "auto_pins"},
                           {"keys": colored}, {"color": "#c7c9ce"})
    return registry
