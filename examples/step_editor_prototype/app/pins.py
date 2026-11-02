"""Pin registry and detection math. Everything here is pure (document +
numbers in, pins out) so Detect Pins can run headlessly from the journal and
the selftests."""

from __future__ import annotations

import re
from dataclasses import dataclass, field

import numpy as np

from .document import BodyMesh, EditorDocument

# JEDEC ball-row letters: I, O, Q, S, X, Z are skipped.
JEDEC_ALPHABET = "ABCDEFGHJKLMNPRTUVWY"


@dataclass
class Pin:
    number: int
    centroid: tuple[float, float, float]
    body_ids: list[int] = field(default_factory=list)            # pins that are whole solids
    face_ids: list[tuple[int, int]] = field(default_factory=list)  # (body, face) regions
    kind: str = "pin"
    role: str = "primary"  # "primary" (tail at the PCB) | "mouth" (mating contact)
    name: str = ""
    name_source: str = ""  # "" | "anchor" (user-typed) | "predicted" | "joined"
    function: str = ""
    signal: str = ""  # what the schematic calls the pin (VDDIO2, nRESET ...)
    hitbox: dict | None = None


# Specialty pin designators. ANCHOR: a casing / mechanical pin soldered into a
# plated hole (it is on a net and gets a hitbox, so it splits from the body
# like any pin). GUIDE: a plastic locating peg through a non-plated hole (no
# net, no hitbox): it stays part of the body. Neither is numbered; their
# footprint shape is the outline they cut at the Z-Sit plane.
SPECIALTY_NAMES = {"ANCHOR": "anchor", "GUIDE": "guide"}


def specialty(pin: "Pin") -> str:
    """'anchor' / 'guide' for specialty pins, '' for ordinary numbered pins."""
    if pin.kind in ("anchor", "guide"):
        return pin.kind
    return SPECIALTY_NAMES.get(pin.name.strip().upper(), "")


def stays_with_body(pin: "Pin") -> bool:
    """A GUIDE peg is the housing's own plastic: never grown or split. ANCHOR
    pins are metal on a net and separate like ordinary pins."""
    return specialty(pin) == "guide"


def set_specialty(pin: "Pin", kind: str) -> None:
    """Mark a pin ANCHOR / GUIDE (kind '' turns it back into a plain pin)."""
    if kind:
        pin.kind = kind
        pin.name = kind.upper()
        pin.name_source = "anchor"
    else:
        pin.kind = "pin"
        if pin.name.strip().upper() in SPECIALTY_NAMES:
            pin.name = ""
            pin.name_source = ""


def predict_remaining_numbers(pins: list["Pin"], *, pin1_hint=None, pattern: str | None = None) -> dict[int, int]:
    """Number only the still-unnamed ordinary pins (index -> number).

    The pool is every non-specialty pin; pins already carrying a number keep
    it, and the free numbers 1..len(pool) go to the rest in serpentine order,
    fitted to the hand-numbered anchors when there are any. ANCHOR / GUIDE /
    custom-named pins are never touched."""
    pool = [i for i, p in enumerate(pins) if not specialty(p)]
    defined = {i: int(pins[i].name) for i in pool if pins[i].name.strip().isdigit()}
    remaining = [i for i in pool if not pins[i].name.strip()]
    if not remaining:
        return {}
    sub = [pins[i] for i in pool]
    local_anchor = {pool.index(i): n for i, n in defined.items()}
    if local_anchor:
        order = predict_serpentine_order(sub, local_anchor, pattern) or list(range(len(sub)))
    else:
        order = order_pins(sub, pin1_hint=pin1_hint, pattern=pattern)
    used = set(defined.values())
    free = (n for n in range(1, len(pool) + len(used) + 1) if n not in used)
    rank = [pool[k] for k in order]
    return {i: next(free) for i in rank if i in remaining}


class PinRegistry:
    def __init__(self) -> None:
        self.pins: list[Pin] = []

    def clear(self) -> None:
        self.pins = []

    def set_pins(self, pins: list[Pin]) -> None:
        # Primaries first, mouth pins after: numbering/ordering/prediction all
        # run over the primary prefix, mouth pins ride along at the tail.
        self.pins = [p for p in pins if p.role != "mouth"] + [
            p for p in pins if p.role == "mouth"
        ]
        self._renumber()

    def primaries(self) -> list[Pin]:
        return [p for p in self.pins if p.role != "mouth"]

    def mouths(self) -> list[Pin]:
        return [p for p in self.pins if p.role == "mouth"]

    def _renumber(self) -> None:
        for index, pin in enumerate(self.pins):
            pin.number = index + 1

    def reorder(self, order: list[int]) -> None:
        self.pins = [self.pins[i] for i in order]
        self._renumber()

    def make_pin1(self, index: int) -> None:
        if 0 <= index < len(self.pins):
            self.pins = self.pins[index:] + self.pins[:index]
            self._renumber()

    def reverse(self) -> None:
        self.pins.reverse()
        self._renumber()

    def move(self, index: int, delta: int) -> int:
        target = index + delta
        if 0 <= index < len(self.pins) and 0 <= target < len(self.pins):
            self.pins[index], self.pins[target] = self.pins[target], self.pins[index]
            self._renumber()
            return target
        return index


def pin_mesh_points(document: EditorDocument, pin: Pin) -> np.ndarray:
    """All mesh vertices belonging to a pin (whole bodies or face regions)."""
    blocks: list[np.ndarray] = []
    for body_id in pin.body_ids:
        mesh = document.bodies[body_id].mesh
        if mesh is not None and len(mesh.points):
            blocks.append(mesh.points)
    by_body: dict[int, list[int]] = {}
    for body_id, face_id in pin.face_ids:
        by_body.setdefault(body_id, []).append(face_id)
    for body_id, faces in by_body.items():
        mesh = document.bodies[body_id].mesh
        if mesh is None:
            continue
        mask = np.isin(mesh.tri_face_ids, np.asarray(faces, dtype=np.int32))
        indices = np.unique(mesh.tris[mask])
        if len(indices):
            blocks.append(mesh.points[indices])
    if not blocks:
        return np.empty((0, 3), dtype=np.float64)
    return np.concatenate(blocks)


# ---------------------------------------------------------------- centroids

def mesh_region_centroid(mesh: BodyMesh, face_ids: list[int] | None = None):
    """Area-weighted centroid of a body mesh, optionally restricted to a set
    of B-rep face ids."""
    tris = mesh.tris
    if face_ids is not None:
        mask = np.isin(mesh.tri_face_ids, np.asarray(face_ids, dtype=np.int32))
        tris = tris[mask]
    if len(tris) == 0:
        return None
    corners = mesh.points[tris]                      # (M, 3, 3)
    centers = corners.mean(axis=1)                   # (M, 3)
    areas = 0.5 * np.linalg.norm(
        np.cross(corners[:, 1] - corners[:, 0], corners[:, 2] - corners[:, 0]), axis=1
    )
    total = float(areas.sum())
    if total <= 0.0:
        return tuple(float(v) for v in centers.mean(axis=0))
    return tuple(float(v) for v in (centers * areas[:, None]).sum(axis=0) / total)


def face_centroids(mesh: BodyMesh) -> dict[int, np.ndarray]:
    """Area-weighted centroid per B-rep face id."""
    corners = mesh.points[mesh.tris]
    centers = corners.mean(axis=1)
    areas = 0.5 * np.linalg.norm(
        np.cross(corners[:, 1] - corners[:, 0], corners[:, 2] - corners[:, 0]), axis=1
    )
    result: dict[int, np.ndarray] = {}
    for face_id in np.unique(mesh.tri_face_ids):
        mask = mesh.tri_face_ids == face_id
        weight = float(areas[mask].sum())
        if weight > 0.0:
            result[int(face_id)] = (centers[mask] * areas[mask, None]).sum(axis=0) / weight
        else:
            result[int(face_id)] = centers[mask].mean(axis=0)
    return result


# ---------------------------------------------------------------- detection

@dataclass(frozen=True)
class Band:
    """Axis-aligned XY rectangle in world coordinates (top view)."""
    x_min: float
    y_min: float
    x_max: float
    y_max: float

    def contains_xy(self, x: float, y: float) -> bool:
        return self.x_min <= x <= self.x_max and self.y_min <= y <= self.y_max


def _degenerate_floor(volumes: list, largest_index: int) -> float:
    """Volume below which a multibody solid is an artifact, not a pin — 10% of
    the median real (non-package) body volume. A vendor multibody chip has one
    solid per real pin (all similar size), plus the occasional much-smaller
    sliver or top-surface feature that must not be counted (e.g. the AK5558VN
    body 0, a thin marking plate ~4% of a lead's volume)."""
    real = [v for i, v in enumerate(volumes)
            if i != largest_index and v > 1e-12]
    return 0.10 * float(np.median(real)) if real else 0.0


def _body_is_pin(body, index, largest_index, volume, vol_floor, band) -> bool:
    """A body is a pin: not the package, has real geometry above the
    degenerate floor, and its XY bbox lies inside the band."""
    if index == largest_index or body.mesh is None or not len(body.mesh.points):
        return False
    if volume <= vol_floor:
        return False
    xy = body.mesh.points[:, :2]
    return (band.contains_xy(*xy.min(axis=0))
            and band.contains_xy(*xy.max(axis=0)))


def detect_pins_multibody(
    document: EditorDocument,
    band: Band,
    *,
    exclude_largest: bool = True,
) -> list[Pin]:
    """Easy case: pins are separate solids — one per real pin. A body is a pin
    candidate when its XY bbox lies inside the band; the largest-volume body
    (the package) is excluded by default, and zero-volume artifact bodies are
    dropped so the body count equals the pin count."""
    from .document import shape_volume

    volumes = [shape_volume(b.solid) or 0.0 for b in document.bodies]
    largest_index = -1
    if exclude_largest and len(document.bodies) > 1:
        largest_index = int(np.argmax(volumes))
    vol_floor = _degenerate_floor(volumes, largest_index)

    pins: list[Pin] = []
    for index, body in enumerate(document.bodies):
        if _body_is_pin(body, index, largest_index, volumes[index],
                        vol_floor, band):
            centroid = mesh_region_centroid(body.mesh)
            if centroid is not None:
                pins.append(Pin(number=0, centroid=centroid, body_ids=[index]))
    return pins


def faces_fully_in_band(mesh: BodyMesh, band: Band) -> set[int]:
    """Face ids whose every mesh vertex projects inside the band (XY)."""
    points = mesh.points
    in_band = (
        (points[:, 0] >= band.x_min)
        & (points[:, 0] <= band.x_max)
        & (points[:, 1] >= band.y_min)
        & (points[:, 1] <= band.y_max)
    )
    tri_ok = in_band[mesh.tris].all(axis=1)
    result: set[int] = set()
    for face_id in np.unique(mesh.tri_face_ids):
        mask = mesh.tri_face_ids == face_id
        if tri_ok[mask].all():
            result.add(int(face_id))
    return result


def detect_pins_unibody(
    document: EditorDocument,
    body_index: int,
    band: Band,
    *,
    smooth_angle_deg: float = 30.0,
    flow: str = "any-edge",
) -> list[Pin]:
    """Hard case: the part is one solid. Candidates are faces FULLY inside the
    band; regions grow by edge flow until a discontinuity. With the default
    "any-edge" flow the band boundary is the discontinuity — whole leads
    coalesce while package faces (never fully banded) stay out. The
    "smooth-only" variant stops at sharp dihedrals too, yielding the raw
    smooth chains for nuanced manual work."""
    body = document.bodies[body_index]
    if body.mesh is None:
        return []
    candidates = faces_fully_in_band(body.mesh, band)
    if not candidates:
        return []
    threshold = smooth_angle_deg if flow == "smooth-only" else None
    adjacency = document.face_smooth_adjacency(body_index, threshold)
    return _grow_regions(body, body_index, candidates, adjacency)


def _connected_components(candidates: set[int], adjacency) -> list[set[int]]:
    components: list[set[int]] = []
    remaining = set(candidates)
    while remaining:
        seed = remaining.pop()
        region = {seed}
        frontier = [seed]
        while frontier:
            face = frontier.pop()
            for neighbor in adjacency.get(face, ()):
                if neighbor in remaining:
                    remaining.discard(neighbor)
                    region.add(neighbor)
                    frontier.append(neighbor)
        components.append(region)
    return components


def _grow_regions(body, body_index: int, candidates: set[int], adjacency) -> list[Pin]:
    regions = _connected_components(candidates, adjacency)
    regions = _merge_regions_by_xy(body.mesh, regions)

    pins: list[Pin] = []
    for region in regions:
        centroid = mesh_region_centroid(body.mesh, sorted(region))
        if centroid is not None:
            pins.append(
                Pin(
                    number=0,
                    centroid=centroid,
                    face_ids=[(body_index, fid) for fid in sorted(region)],
                )
            )
    return pins


def _merge_regions_by_xy(mesh: BodyMesh, regions: list[set[int]], factor: float = 0.5):
    """Regions stacked at the same XY spot are one pin — a BGA ball and its
    collar pad, or the segments of one lead. Merge regions whose centroids
    are closer than `factor` x the median nearest-neighbour pitch."""
    if len(regions) <= 1:
        return regions
    centroids = np.array(
        [mesh_region_centroid(mesh, sorted(region))[:2] for region in regions]
    )
    distances = np.linalg.norm(centroids[:, None, :] - centroids[None, :, :], axis=2)
    np.fill_diagonal(distances, np.inf)
    pitch = float(np.median(distances.min(axis=1)))
    if not np.isfinite(pitch) or pitch <= 0.0:
        return regions
    threshold = pitch * factor

    parent = list(range(len(regions)))

    def find(i: int) -> int:
        while parent[i] != i:
            parent[i] = parent[parent[i]]
            i = parent[i]
        return i

    pairs = np.argwhere(distances < threshold)
    for i, j in pairs:
        ri, rj = find(int(i)), find(int(j))
        if ri != rj:
            parent[rj] = ri

    merged: dict[int, set[int]] = {}
    for index, region in enumerate(regions):
        merged.setdefault(find(index), set()).update(region)
    return list(merged.values())


def mesh_face_areas(mesh: BodyMesh) -> dict[int, float]:
    corners = mesh.points[mesh.tris]
    areas = 0.5 * np.linalg.norm(
        np.cross(corners[:, 1] - corners[:, 0], corners[:, 2] - corners[:, 0]), axis=1
    )
    result: dict[int, float] = {}
    for face_id in np.unique(mesh.tri_face_ids):
        result[int(face_id)] = float(areas[mesh.tri_face_ids == face_id].sum())
    return result


def dominant_face_color(mesh: BodyMesh, face_ids=None):
    """Area-weighted dominant color among the given faces (all faces when
    None). None if none of them carry a color."""
    if mesh is None or not mesh.face_colors:
        return None
    areas = mesh_face_areas(mesh)
    targets = face_ids if face_ids is not None else list(areas)
    weights: dict[tuple, float] = {}
    for face_id in targets:
        rgb = mesh.face_colors.get(face_id)
        if rgb is not None:
            weights[tuple(rgb)] = weights.get(tuple(rgb), 0.0) + areas.get(face_id, 0.0)
    if not weights:
        return None
    return max(weights, key=weights.get)


def grow_smooth_region(
    document: EditorDocument,
    body_index: int,
    face_index: int,
    *,
    smooth_angle_deg: float = 30.0,
    claimed: set | None = None,
    area_factor: float | None = None,
) -> set[int]:
    """Manual pin seeding: from one clicked face, flow across smooth shared
    edges until every path hits a discontinuity (sharp dihedral) or a face
    already claimed by another pin. Returns the face ids of the region.

    When `area_factor` is set, the flow ALSO stops before crossing onto a face
    larger than area_factor x the seed face. Dihedral gating alone can't catch
    a contact that meets the housing tangent-continuously (no sharp edge) — on
    such connectors a plain smooth grow floods the whole body; the area gate
    keeps the seed to one contact, matching what Separate will carve."""
    adjacency = document.face_smooth_adjacency(body_index, smooth_angle_deg)
    blocked = claimed or set()
    region = {face_index}
    frontier = [face_index]
    areas = limit = None
    if area_factor is not None:
        areas = mesh_face_areas(document.bodies[body_index].mesh)
        limit = areas.get(face_index, 0.0) * area_factor
    while frontier:
        face = frontier.pop()
        for neighbor in adjacency.get(face, ()):
            if neighbor in region or neighbor in blocked:
                continue
            if limit is not None and areas.get(neighbor, np.inf) > limit:
                continue  # body-scale face — flow would flood here
            region.add(neighbor)
            frontier.append(neighbor)
    return region


def _region_dims(mesh: BodyMesh, region: set[int]) -> np.ndarray | None:
    """Sorted bbox extents of a face region — its orientation-free size."""
    mask = np.isin(mesh.tri_face_ids, np.asarray(sorted(region), dtype=np.int32))
    indices = np.unique(mesh.tris[mask])
    if not len(indices):
        return None
    points = mesh.points[indices]
    return np.sort(points.max(axis=0) - points.min(axis=0))


def find_similar_regions(
    document: EditorDocument,
    body_index: int,
    seed_region: set[int],
    *,
    smooth_angle_deg: float = 30.0,
    claimed: set[tuple[int, int]] | None = None,
    area_tol: float = 0.12,
    dim_tol: float = 0.25,
) -> list[list[tuple[int, int]]]:
    """Find every disjoint face region that looks like the seed: repeated
    contacts inside a connector mouth are copies of one another, so anything
    with a matching area and bbox size is another instance. A single-face
    seed (normal face picking) is matched against individual faces; a grown
    seed is matched against regions grown the same way. Searches all bodies
    (multibody connectors keep each contact as its own solid). Returns the
    matches as (body, face) regions, seed excluded."""
    seed_mesh = document.bodies[body_index].mesh
    if seed_mesh is None or not seed_region:
        return []
    seed_areas = mesh_face_areas(seed_mesh)
    seed_total = sum(seed_areas.get(f, 0.0) for f in seed_region)
    seed_dims = _region_dims(seed_mesh, seed_region)
    if seed_total <= 0.0 or seed_dims is None:
        return []
    template_face = max(seed_region, key=lambda f: seed_areas.get(f, 0.0))
    template_area = seed_areas[template_face]
    dim_floor = max(float(seed_dims.max()), 1e-9)
    single_face = len(seed_region) == 1

    claimed = claimed or set()
    matches: list[list[tuple[int, int]]] = []
    taken: set[tuple[int, int]] = {(body_index, f) for f in seed_region} | set(claimed)
    for b, body in enumerate(document.bodies):
        mesh = body.mesh
        if mesh is None or not len(mesh.tris):
            continue
        areas = mesh_face_areas(mesh)
        blocked_faces = {face for (bb, face) in taken if bb == b}
        candidates = [
            face
            for face, area in areas.items()
            if abs(area - template_area) <= area_tol * template_area
            and face not in blocked_faces
        ]
        for face in sorted(candidates):
            if face in blocked_faces:
                continue
            if single_face:
                region = {face}
            else:
                region = grow_smooth_region(
                    document, b, face,
                    smooth_angle_deg=smooth_angle_deg,
                    claimed=blocked_faces,
                )
            total = sum(areas.get(f, 0.0) for f in region)
            dims = _region_dims(mesh, region)
            if (
                dims is not None
                and abs(total - seed_total) <= area_tol * seed_total
                and float(np.abs(dims - seed_dims).max()) <= dim_tol * dim_floor
            ):
                matches.append([(b, f) for f in sorted(region)])
                blocked_faces |= region
                taken |= {(b, f) for f in region}
    return matches


def capture_pin_face_anchors(
    document: EditorDocument, pins: list[Pin]
) -> list[tuple[Pin, list[tuple[float, float, float]]]]:
    """Geometric anchors (per-face centroids) for every face-region pin,
    captured BEFORE a split rebuilds the body table and renumbers faces —
    (body, face) references do not survive, centroids do."""
    anchors = []
    for pin in pins:
        if not pin.face_ids:
            continue
        points = []
        for body_index, face_index in pin.face_ids:
            if not (0 <= body_index < len(document.bodies)):
                continue
            mesh = document.bodies[body_index].mesh
            if mesh is None:
                continue
            centroid = mesh_region_centroid(mesh, [face_index])
            if centroid is not None:
                points.append(centroid)
        if points:
            anchors.append((pin, points))
    return anchors


def remap_pin_faces(
    document: EditorDocument,
    anchors: list[tuple[Pin, list[tuple[float, float, float]]]],
    tolerance: float,
) -> int:
    """Re-point face-region pins at the rebuilt body table: a new face whose
    centroid coincides with the pin's old face centroid is that same (uncut)
    face. Mouth pins and unsplit primaries stay valid through Separate this
    way. Returns how many pins were remapped."""
    table: list[tuple[int, int]] = []
    points = []
    for b, body in enumerate(document.bodies):
        mesh = body.mesh
        if mesh is None or not len(mesh.tris):
            continue
        for face, centroid in face_centroids(mesh).items():
            table.append((b, int(face)))
            points.append(centroid)
    if not table:
        return 0
    points = np.asarray(points)
    remapped = 0
    for pin, old_points in anchors:
        if pin.body_ids or not pin.face_ids:
            continue  # linked to its own body — nothing left to remap
        new_faces = []
        for centroid in old_points:
            distances = np.linalg.norm(points - np.asarray(centroid), axis=1)
            nearest = int(np.argmin(distances))
            if float(distances[nearest]) <= tolerance:
                new_faces.append(table[nearest])
        if new_faces:
            pin.face_ids = sorted(set(new_faces))
            remapped += 1
    return remapped


def fix_keeping_pins(document, pins: list[Pin], change) -> None:
    """Run `change()` (a Model Health fix) and keep every pin pointing at the
    same geometry: bodies by identity (a deleted body before a pin's shifts
    every index after it: BTN8982TA's pin-1 dot), faces by position (a
    rebuilt solid renumbers its faces). A body that was split into pieces is
    dropped from body_ids; its faces remap and link_own_solids relinks it."""
    if not pins:
        change()
        return
    before = list(document.bodies)
    anchors = capture_pin_face_anchors(document, pins)
    change()
    _relink_bodies(document, pins, before)
    _remap_every_face(document, anchors)
    link_own_solids(document, pins)


def _relink_bodies(document, pins: list[Pin], before: list) -> None:
    """Point body_ids at the same body records in the new table."""
    index = {id(body): i for i, body in enumerate(document.bodies)}
    for pin in pins:
        pin.body_ids = [index[id(before[b])] for b in pin.body_ids
                        if 0 <= b < len(before) and id(before[b]) in index]


def _remap_every_face(document, anchors) -> None:
    """remap_pin_faces for linked pins too (it skips them by design)."""
    linked = [(pin, pin.body_ids) for pin, _points in anchors]
    for pin, _ids in linked:
        pin.body_ids = []
    remap_pin_faces(document, anchors, _remap_tolerance(document))
    for pin, ids in linked:
        pin.body_ids = ids


def _remap_tolerance(document) -> float:
    """5/1000 of the model's diagonal (as Separate uses)."""
    b = document.bounds()
    return float(np.linalg.norm([b[1] - b[0], b[3] - b[2], b[5] - b[4]])) * 5.0e-3


def _row_pitch(t: np.ndarray) -> float:
    """Along-row pitch: the median gap between pin positions, ignoring gaps
    far below the mean spacing. Two-sided connectors put a tail on each side
    at the same position, and those pairs differ only by modelling noise (a
    micron on a DDR4 socket): counted as gaps they made the pitch 0.001 mm and
    the join tolerance (half a pitch) left most contacts unjoined."""
    ordered = np.sort(np.asarray(t, dtype=np.float64))
    if len(ordered) < 2:
        return np.inf
    gaps = np.diff(ordered)
    mean_spacing = float(ordered[-1] - ordered[0]) / (len(ordered) - 1)
    gaps = gaps[gaps > 0.1 * mean_spacing]
    return float(np.median(gaps)) if len(gaps) else np.inf


def contact_row_order(contacts: list["Pin"], flip: bool = False) -> list[int]:
    """Indices of `contacts` from J1 to Jn: along the row's own axis, J1 at
    the +X end (+Y when the row runs along Y); `flip` starts at the other end."""
    xy = np.array([c.centroid[:2] for c in contacts], dtype=float)
    if len(xy) < 2:
        return list(range(len(xy)))
    _u, _s, vt = np.linalg.svd(xy - xy.mean(axis=0), full_matrices=False)
    axis = vt[0]
    sign = np.sign(axis[0]) if abs(axis[0]) > 1e-9 else np.sign(axis[1])
    t = (xy - xy.mean(axis=0)) @ (axis * (sign or 1.0))
    order = list(np.argsort(-t))  # J1 at the +X (or +Y) end
    return order[::-1] if flip else order


def join_mouth_pins(
    primaries: list[Pin], mouths: list[Pin]
) -> tuple[dict[int, Pin], list[int]]:
    """Give each mouth pin the designator of the primary (tail) pin it lines
    up with along the connector row: contacts run perpendicular to the row,
    so projecting centroids onto the primaries' dominant XY axis pairs them
    naturally. Mouth pins the user named by hand (green anchors) calibrate
    the pairing — they reveal the offset and direction between the mouth
    row and the tail row, so shifted or mirrored rows still pair right —
    and are never reassigned. Several mouth faces may join the same primary
    (one contact can expose more than one face in the mouth). Returns
    ({mouth list-index: primary}, [conflicted indices]) where conflicts are
    mouth pins farther than half the primary pitch from any (calibrated)
    primary position — misaligned, left unjoined for the user to check."""
    if not primaries or not mouths:
        return {}, list(range(len(mouths)))
    points = np.array([p.centroid for p in primaries], dtype=np.float64)[:, :2]
    center = points.mean(axis=0)
    spread = points - center
    if len(primaries) > 1:
        _u, _s, vt = np.linalg.svd(spread, full_matrices=False)
        axis = vt[0]
    else:
        axis = np.array([1.0, 0.0])
    perp = np.array([-axis[1], axis[0]])
    t_primary = spread @ axis
    s_primary = spread @ perp
    mouth_xy = (
        np.array([p.centroid for p in mouths], dtype=np.float64)[:, :2] - center
    )
    t_mouth = mouth_xy @ axis
    s_mouth = mouth_xy @ perp

    # Along-row pitch: the yardstick for both row clustering and the join
    # tolerance.
    pitch = _row_pitch(t_primary)

    def cluster_rows(s_values: np.ndarray) -> list[np.ndarray]:
        """Split indices into SIDES along the perpendicular axis, at the
        single largest gap when it exceeds half the along-row pitch.
        Two-sided connectors (DIMM sockets, dual-row headers) otherwise
        collapse onto one line and the 1-D pairing scrambles designators
        between the sides — the DDR5 regression. One split only: staggered
        sub-rows WITHIN a side (common on DIMM tails) must stay together."""
        order = np.argsort(s_values)
        if len(order) <= 1 or not np.isfinite(pitch):
            return [order]
        sorted_s = s_values[order]
        gaps = np.diff(sorted_s)
        widest = int(np.argmax(gaps))
        if float(gaps[widest]) <= 0.5 * pitch:
            return [order]
        return [order[: widest + 1], order[widest + 1 :]]

    primary_rows = cluster_rows(s_primary)
    mouth_rows = cluster_rows(s_mouth)

    by_designator = {
        (p.name or str(p.number)): i for i, p in enumerate(primaries)
    }
    anchor_pairs = [
        (m, by_designator[pin.name.strip()])
        for m, pin in enumerate(mouths)
        if pin.name_source == "anchor" and pin.name.strip() in by_designator
    ]
    anchored = dict(anchor_pairs)

    # Pair each mouth row with a primary row: anchors vote first (they reveal
    # the true row mapping, including swapped sides); rows without an anchor
    # pair by perpendicular proximity of the row centres.
    primary_row_of = {}
    for row_index, row in enumerate(primary_rows):
        for p in row:
            primary_row_of[int(p)] = row_index
    row_match: dict[int, int] = {}
    for mouth_row_index, mouth_row in enumerate(mouth_rows):
        votes = [
            primary_row_of[anchored[int(m)]]
            for m in mouth_row if int(m) in anchored
        ]
        if votes:
            row_match[mouth_row_index] = max(set(votes), key=votes.count)
    primary_centres = [float(np.mean(s_primary[row])) for row in primary_rows]
    for mouth_row_index, mouth_row in enumerate(mouth_rows):
        if mouth_row_index in row_match:
            continue
        centre = float(np.mean(s_mouth[mouth_row]))
        row_match[mouth_row_index] = int(np.argmin(
            [abs(centre - pc) for pc in primary_centres]
        ))

    tolerance = pitch * 0.5 if np.isfinite(pitch) else np.inf

    assigned: dict[int, Pin] = {}
    conflicts: list[int] = []
    for mouth_row_index, mouth_row in enumerate(mouth_rows):
        primary_row = primary_rows[row_match[mouth_row_index]]
        tp_row = t_primary[primary_row]

        # Anchored mouth pins: t_mouth ~= sign * t_primary + offset. Fit from
        # the anchors in THIS row pair, falling back to all anchors (2+ can
        # also detect a mirrored row).
        row_anchor_pairs = [
            (int(m), anchored[int(m)]) for m in mouth_row if int(m) in anchored
        ] or anchor_pairs
        sign, offset = 1.0, 0.0
        if len(row_anchor_pairs) >= 2:
            tm = np.array([t_mouth[m] for m, _p in row_anchor_pairs])
            tp = np.array([t_primary[p] for _m, p in row_anchor_pairs])
            best_fit = None
            for a in (1.0, -1.0):
                b = float(np.mean(tm - a * tp))
                residual = float(np.abs(tm - (a * tp + b)).max())
                if best_fit is None or residual < best_fit[0]:
                    best_fit = (residual, a, b)
            _residual, sign, offset = best_fit
        elif len(row_anchor_pairs) == 1:
            m, p = row_anchor_pairs[0]
            offset = float(t_mouth[m] - t_primary[p])

        expected = sign * tp_row + offset  # where each primary's mouth sits
        for m in mouth_row:
            m = int(m)
            if m in anchored:
                assigned[m] = primaries[anchored[m]]
                continue
            distances = np.abs(expected - t_mouth[m])
            nearest = int(np.argmin(distances))
            if float(distances[nearest]) <= tolerance:
                assigned[m] = primaries[int(primary_row[nearest])]
            else:
                conflicts.append(m)
    return assigned, sorted(conflicts)


def transform_pins(pins: list["Pin"], matrix) -> None:
    """Move pins with the model: a geometry transform (Z-Sit, Front, Pin 1)
    must carry each pin's centroid and hitbox along, or callouts, leaders and
    the 2-D views are left where the pins used to be. A turn about Z keeps an
    OBB an OBB (its yaw changes); any tilt turns it into the convex hull of its
    eight transformed corners."""
    import math

    m = np.asarray(matrix, dtype=np.float64)
    rot, shift = m[:3, :3], m[:3, 3]
    move = lambda p: (rot @ np.asarray(p, dtype=np.float64) + shift).tolist()  # noqa: E731
    z_only = abs(rot[2, 2] - 1.0) < 1.0e-9
    yaw = math.degrees(math.atan2(rot[1, 0], rot[0, 0]))
    for pin in pins:
        pin.centroid = tuple(move(pin.centroid))
        box = pin.hitbox
        if not box:
            continue
        if box.get("type") in ("convex_hull", "shrink_wrap"):
            box["vertices"] = [move(v) for v in box.get("vertices", [])]
        elif box.get("type") == "obb":
            if z_only:
                box["center"] = move(box["center"])
                box["rotation_z_deg"] = float(box.get("rotation_z_deg", 0.0)) + yaw
            else:
                c = np.asarray(box["center"], dtype=np.float64)
                h = np.asarray(box["half_extents"], dtype=np.float64)
                a = math.radians(float(box.get("rotation_z_deg", 0.0)))
                local = np.array([[math.cos(a), -math.sin(a), 0], [math.sin(a), math.cos(a), 0], [0, 0, 1]])
                corners = [c + local @ (h * np.array(s)) for s in
                           [(sx, sy, sz) for sx in (-1, 1) for sy in (-1, 1) for sz in (-1, 1)]]
                pin.hitbox = {"type": "convex_hull", "vertices": [move(p) for p in corners]}


MECHANICAL_PAD = re.compile(r"^(MP|SH|MH|MNT|S|M|H|NC)\d*$", re.I)  # plus a blank number


def pad_role(pad: dict) -> str:
    """What the footprint says a pad is: 'guide' (non-plated hole: a plastic
    peg), 'anchor' (copper with no electrical number: a soldered mechanical /
    shield pin, plated hole or SMD fixing tab alike), or 'pin' (a numbered
    electrical pad)."""
    if pad.get("type") == "np_thru_hole":
        return "guide"
    number = str(pad.get("number") or "").strip()
    if not number or MECHANICAL_PAD.match(number):
        return "anchor"
    return "pin"


def _pin_label(pin) -> tuple[str, str]:
    kind = specialty(pin)
    return (kind or "pin", "" if kind else (pin.name or str(pin.number)))


def _drill_size(drill) -> float:
    """A pad's drill as one size: a round hole's diameter, a slot's length."""
    if not drill:
        return 0.0
    return float(max(drill)) if isinstance(drill, (list, tuple)) else float(drill)


def match_pins_to_pads(pins: list["Pin"], pads: list[dict]) -> dict:
    """Cross-validate the board-side pins against the footprint, the official
    decider. Every board-side pin (SMT foot / THT lead / ANCHOR / GUIDE; not
    mating contacts) is paired one-to-one with a pad by XY after aligning the
    two sets (four 90-degree turns, translation refined from the matched
    pairs). Each pin then gets the pad's verdict: its number, or ANCHOR
    (plated, unnumbered pad) or GUIDE (non-plated hole).

    Returns {"assign": {pin index: (role, number)}, "rotation", "rms",
    "changed": [(pin, old, new)], "pins_without_pad": [...],
    "pads_without_pin": [...], "ambiguous": bool, "alternatives": [...]}.
    Symmetric layouts that fit turned are 'ambiguous'."""
    import math

    from scipy.optimize import linear_sum_assignment

    board = [i for i, p in enumerate(pins) if p.role != "mouth"]
    pads = [p for p in pads if p.get("x") is not None]
    empty = {"assign": {}, "rotation": 0, "rms": None, "changed": [], "pins_without_pad": [],
             "pads_without_pin": [], "ambiguous": False, "alternatives": []}
    if not board or not pads:
        return empty
    pin_xy = np.array([pins[i].centroid[:2] for i in board], dtype=float)
    pad_xy = np.array([[p["x"], p["y"]] for p in pads], dtype=float)
    reach = np.array([max(p.get("w") or 0.5, p.get("h") or 0.5, _drill_size(p.get("drill"))) for p in pads])
    reach = np.maximum(reach, 0.5)  # a pin lands on a pad within about its size

    def fit(k: int):
        a = math.pi / 2 * k
        rot = np.array([[math.cos(a), -math.sin(a)], [math.sin(a), math.cos(a)]])
        turned = pad_xy @ rot.T
        shift = pin_xy.mean(axis=0) - turned.mean(axis=0)
        for _ in range(3):  # refine the shift from the matched pairs (copes with missing pins)
            cost = np.linalg.norm(pin_xy[:, None, :] - (turned + shift)[None, :, :], axis=2)
            rows, cols = linear_sum_assignment(cost)
            close = cost[rows, cols] <= reach[cols] * 1.5
            if close.any():
                shift = (pin_xy[rows[close]] - turned[cols[close]]).mean(axis=0)
        cost = np.linalg.norm(pin_xy[:, None, :] - (turned + shift)[None, :, :], axis=2)
        rows, cols = linear_sum_assignment(cost)
        ok = cost[rows, cols] <= reach[cols] * 1.5
        pairs = {board[r]: c for r, c, good in zip(rows, cols, ok) if good}
        rms = float(np.sqrt(np.mean(cost[rows[ok], cols[ok]] ** 2))) if ok.any() else float("inf")
        # fewer matched pins is worse than a slightly larger residual
        return (-len(pairs), rms, 90 * k, pairs, shift)

    fits = sorted((fit(k) for k in range(4)), key=lambda f: (f[0], f[1]))
    _neg, rms, rotation, pairs, best_shift = fits[0]

    def verdicts(pairs_):
        return {i: (pad_role(pads[c]), "" if pad_role(pads[c]) != "pin" else str(pads[c]["number"]))
                for i, c in pairs_.items()}

    assign = verdicts(pairs)
    pitch = float(np.min([np.linalg.norm(a - b) for i, a in enumerate(pad_xy) for b in pad_xy[i + 1:]]
                         or [1.0]))
    alternatives = [{"rotation": f[2], "rms": f[1], "assign": verdicts(f[3])} for f in fits[1:]
                    if f[0] == fits[0][0] and f[1] <= rms + 0.1 * pitch and verdicts(f[3]) != assign]
    changed = [(pins[i].name or str(pins[i].number), _pin_label(pins[i]), new)
               for i, new in assign.items() if _pin_label(pins[i]) != new]
    used = set(pairs.values())
    return {"assign": assign, "rotation": rotation, "rms": rms, "changed": changed,
            # model pins = R(rotation) @ pads + shift
            "shift": [float(v) for v in best_shift],
            "pins_without_pad": [pins[i].name or str(pins[i].number) for i in board if i not in pairs],
            "pads_without_pin": [str(pads[c].get("number") or pad_role(pads[c]))
                                 for c in range(len(pads)) if c not in used],
            "ambiguous": bool(alternatives), "alternatives": alternatives}


def current_assignment(pins: list["Pin"], indices) -> dict:
    return {i: _pin_label(pins[i]) for i in indices}


def apply_pad_numbers(pins: list["Pin"], assign: dict) -> int:
    """Apply the footprint's verdict: numbers for electrical pins, ANCHOR /
    GUIDE for mechanical ones. A mating (mouth) pin that carried its tail's old
    designator follows it to the new one."""
    old_to_new = {}
    for index, (role, number) in assign.items():
        pin = pins[index]
        old = pin.name or str(pin.number)
        if role == "pin":
            set_specialty(pin, "")
            pin.name = number
            pin.name_source = "footprint"
            old_to_new[old] = number
        else:
            set_specialty(pin, role)
    for pin in pins:
        if pin.role == "mouth" and pin.name in old_to_new:
            pin.name = old_to_new[pin.name]
    return len(assign)


def footprint_numbering(pins: list["Pin"], pads: list[dict]) -> dict:
    """Decide and apply the official numbering. Returns the match plus
    "status": verified (already agreed), consistent (symmetric part; the
    user's numbering is one of the equally good fits, kept), corrected
    (renumbered / re-roled to the footprint), or no-footprint."""
    if not pads:
        return {"status": "assumed (no footprint)"}
    match = match_pins_to_pads(pins, pads)
    if not match["assign"]:
        return {**match, "status": "assumed (nothing matched)"}
    current = current_assignment(pins, match["assign"])
    if not match["changed"]:
        match["status"] = "verified"
    elif match["ambiguous"] and any(a["assign"] == current for a in match["alternatives"]):
        match["status"] = "consistent (symmetric; kept)"
    else:
        apply_pad_numbers(pins, match["assign"])
        match["status"] = "corrected"
    return match


def link_pin_pieces(document, pins: list["Pin"]) -> int:
    """A face-region pin whose faces now sit on a split-off piece (not the
    package, the largest body) IS that piece: link it, so hitboxes, colors and
    naming treat the whole piece as the pin. Separate can cut one lead into a
    tail piece and a mouth piece; each end links to its own. Returns links made."""
    from .document import shape_volume

    if len(document.bodies) < 2:
        return 0
    volumes = [shape_volume(body.solid) or 0.0 for body in document.bodies]
    package = max(range(len(volumes)), key=volumes.__getitem__)
    linked = 0
    for pin in pins:
        if stays_with_body(pin):
            continue  # a GUIDE peg belongs to the body
        for body_index, _face in pin.face_ids:
            if body_index != package and 0 <= body_index < len(document.bodies)                     and body_index not in pin.body_ids:
                pin.body_ids.append(body_index)
                linked += 1
    return linked


def link_own_solids(document, pins: list["Pin"]) -> int:
    """A pin known by a face on a solid that is ONLY its own (a vendor model
    that already ships every lead as its own body: BTN8982TA, TI WQFN) IS that
    solid: link it, so the footprint, hitboxes, colours and the export treat
    the whole lead as the pin even when Separate is skipped (nothing to split).
    Same rule as hitbox.pin_solid_ids: not the package, used by no other pin,
    under a quarter of the package's volume. Returns pins linked."""
    from .hitbox import pin_solid_ids

    linked = 0
    for pin in pins:
        if pin.body_ids or stays_with_body(pin):
            continue
        own = pin_solid_ids(document, pin, pins)
        if own:
            pin.body_ids = list(own)
            linked += 1
    return linked


def apply_pin_body_names(document, pins: list["Pin"]) -> int:
    """The body-per-pin association contract: every body that IS a pin gets
    named for it (PIN_<n>, or the pin's designator) and role "pin" — however
    it became one (vendor multibody, Separate, or paint). Bodies shared by
    several pins (bridged contacts) name all of them. Returns bodies named."""
    body_pins: dict[int, list[Pin]] = {}
    for pin in pins:
        for body_id in pin.body_ids:
            body_pins.setdefault(body_id, []).append(pin)
    named = 0
    for body_id, owners in body_pins.items():
        if body_id >= len(document.bodies):
            continue
        if len(owners) == 1:
            pin = owners[0]
            label = pin.name or f"PIN_{pin.number}"
            if pin.role == "mouth":
                label += "_HEAD"
        else:
            label = "PINS_" + "_".join(
                str(p.number) for p in sorted(owners, key=lambda p: p.number)
            )
        body = document.bodies[body_id]
        body.name = label
        body.role = "pin"
        named += 1
    return named


def find_pin_cut(
    document: EditorDocument,
    region: list[tuple[int, int]],
    *,
    jump_ratio: float = 3.0,
    samples: int = 80,
):
    """Find where a pin ENDS by geometry, not by existing face boundaries:
    start at the pin tip, march a section plane back along the tip's normal,
    and watch the local cross-section — where it grows by `jump_ratio` the
    body has started. Returns (cut_point, axis, half_size) for a bounded
    cutting plane (which may split model faces mid-face), or None if no
    clear jump is found."""
    if not region:
        return None
    body_index = region[0][0]
    faces = sorted({face for body, face in region if body == body_index})
    mesh = document.bodies[body_index].mesh
    if mesh is None or not len(mesh.tris):
        return None

    region_mask = np.isin(mesh.tri_face_ids, np.asarray(faces, dtype=np.int32))
    region_points = mesh.points[np.unique(mesh.tris[region_mask])]
    if not len(region_points):
        return None
    region_centroid = region_points.mean(axis=0)
    body_centroid = mesh.points.mean(axis=0)
    outward = region_centroid - body_centroid
    norm = float(np.linalg.norm(outward))
    outward = outward / norm if norm > 1e-9 else np.array([0.0, 0.0, -1.0])

    # tip face: the region face whose centroid lies farthest outward; the
    # march axis is its (outward) normal flipped to point INTO the body.
    tri_corners = mesh.points[mesh.tris]
    tri_centers = tri_corners.mean(axis=1)
    tri_normals = np.cross(
        tri_corners[:, 1] - tri_corners[:, 0], tri_corners[:, 2] - tri_corners[:, 0]
    )
    best_face, best_proj = faces[0], -np.inf
    for face in faces:
        mask = mesh.tri_face_ids == face
        proj = float(tri_centers[mask].mean(axis=0) @ outward)
        if proj > best_proj:
            best_face, best_proj = face, proj
    tip_mask = mesh.tri_face_ids == best_face
    axis = tri_normals[tip_mask].sum(axis=0)
    norm = float(np.linalg.norm(axis))
    if norm < 1e-12:
        return None
    axis /= norm
    if float(axis @ outward) > 0.0:
        axis = -axis  # march into the body

    # start at the region's outermost point along the axis
    start_t = float((region_points @ axis).min())
    tip_point = region_centroid + axis * (start_t - float(region_centroid @ axis))

    span = mesh.points @ axis
    length = float(span.max() - (tip_point @ axis))
    if length <= 1e-9:
        return None
    step = length / samples

    u = np.cross(axis, [0.0, 0.0, 1.0])
    if np.linalg.norm(u) < 1e-6:
        u = np.cross(axis, [0.0, 1.0, 0.0])
    u /= np.linalg.norm(u)
    v = np.cross(axis, u)

    baseline = None
    radius = None
    for i in range(1, samples):
        t = i * step
        plane_point = tip_point + axis * t
        segments = section_segments(mesh, plane_point, axis)
        if not len(segments):
            continue
        pts = segments.reshape(-1, 3)
        offsets = pts - plane_point
        if radius is not None:
            radial = offsets - np.outer(offsets @ axis, axis)
            keep = np.linalg.norm(radial, axis=1) <= radius
            if not keep.any():
                continue
            pts, offsets = pts[keep], offsets[keep]
        pu, pv = offsets @ u, offsets @ v
        area = float((pu.max() - pu.min()) * (pv.max() - pv.min()))
        if area <= 1e-12:
            continue
        if baseline is None:
            baseline = area
            radial = offsets - np.outer(offsets @ axis, axis)
            radius = float(np.linalg.norm(radial, axis=1).max()) * 2.0 + step
            continue
        if area > jump_ratio * baseline:
            cut_point = tip_point + axis * (t - step * 0.5)
            half_size = radius * 1.25
            return (
                tuple(float(x) for x in cut_point),
                tuple(float(x) for x in axis),
                half_size,
            )
        baseline = max(baseline, area)
    return None


def _claim_primary_seeds(pins) -> dict:
    """Primaries claim their seed faces first: a tail flows through the WHOLE
    contact, swallowing any CON/HEAD seed faces on the way."""
    claimed: dict[tuple[int, int], int] = {}
    for pin_index, pin in enumerate(pins):
        if pin.role != "mouth":
            for key in pin.face_ids:
                claimed[key] = pin_index
    return claimed


def _primary_body_ids(pins) -> set:
    """Bodies already linked to an SMT/THR pin are whole contacts."""
    return {
        body_id for pin in pins if pin.role != "mouth" for body_id in pin.body_ids
    }


def _growable_count(pins, only) -> int:
    return sum(
        1
        for i, p in enumerate(pins)
        if not p.body_ids and p.face_ids and not stays_with_body(p)
        and (only is None or i in only)
    )


def _face_color(body, face) -> tuple:
    rgb = body.mesh.face_colors.get(face, body.color or (0.62, 0.64, 0.66))
    return tuple(round(float(c), 3) for c in rgb)


def _color_components(document: EditorDocument, body_index: int) -> tuple[dict, list]:
    """Connected same-colour face regions of a body: (face -> region id,
    [(faces, area)])."""
    body = document.bodies[body_index]
    adjacency = document.face_smooth_adjacency(body_index, None)
    areas = mesh_face_areas(body.mesh)
    owner: dict[int, int] = {}
    regions: list[tuple[set, float]] = []
    for start in areas:
        if start in owner:
            continue
        colour, faces, frontier = _face_color(body, start), {start}, [start]
        owner[start] = len(regions)
        while frontier:
            for neighbor in adjacency.get(frontier.pop(), ()):
                if neighbor not in owner and _face_color(body, neighbor) == colour:
                    owner[neighbor] = len(regions)
                    faces.add(neighbor)
                    frontier.append(neighbor)
        regions.append((faces, sum(areas.get(f, 0.0) for f in faces)))
    return owner, regions


def _face_centres(mesh, faces) -> dict:
    return {f: mesh.points[mesh.tris[mesh.tri_face_ids == f]].reshape(-1, 3).mean(axis=0) for f in faces}


def _open_neighbors(adjacency, face: int, faces: set, owner: dict) -> list:
    return [n for n in adjacency.get(face, ()) if n in faces and n not in owner]


def _nearest_seed_owner(document, body_index: int, faces: set, seeds: dict[int, set]) -> dict[int, int]:
    """face -> the pin whose seed is nearest walking over the surface
    (Dijkstra on face centroids, inside `faces`)."""
    import heapq

    adjacency = document.face_smooth_adjacency(body_index, None)
    centre = _face_centres(document.bodies[body_index].mesh, faces)
    owner: dict[int, int] = {}
    heap = [(0.0, f, pin) for pin, own in seeds.items() for f in own & faces]
    heapq.heapify(heap)
    while heap:
        dist, face, pin = heapq.heappop(heap)
        if face in owner:
            continue
        owner[face] = pin
        for neighbor in _open_neighbors(adjacency, face, faces, owner):
            heapq.heappush(heap, (dist + float(np.linalg.norm(centre[neighbor] - centre[face])), neighbor, pin))
    return owner


def _split_by_nearest_seed(document, body_index: int, faces: set, seeds: dict[int, set]) -> dict[int, set]:
    """Share a colour region between several pins: each face goes to the pin
    whose seed is nearest over the surface, so one region joining several
    contacts parts between them."""
    out: dict[int, set] = {}
    for face, pin in _nearest_seed_owner(document, body_index, faces, seeds).items():
        out.setdefault(pin, set()).add((body_index, face))
    return out


def color_region_bundles(document: EditorDocument, pins: list[Pin]) -> dict[int, set]:
    """Separation driven by colour. Each connected same-colour region that is
    not the body's largest (the housing / shell) is contact metal:
    - one SMT/THR pin in it (plus any CON/HEAD mouth pins): the whole region
      is that contact, tail and mouth together; its mouth pins ride along;
    - several SMT/THR pins in it: the region is shared between them, each
      face to the pin whose seed is nearest over the surface;
    - only mouth pins: the region goes to the first of them.
    Returns pin index -> {(body, face)}; GUIDE pegs never get one."""
    by_region: dict[tuple[int, int], dict[int, set]] = {}
    components: dict[int, tuple[dict, list]] = {}
    for index, pin in enumerate(pins):
        if stays_with_body(pin) or pin.body_ids or not pin.face_ids:
            continue
        for body_index, seed in pin.face_ids:
            if document.bodies[body_index].mesh is None:
                continue
            if body_index not in components:
                components[body_index] = _color_components(document, body_index)
            owner, regions = components[body_index]
            region = owner.get(seed)
            largest = max(range(len(regions)), key=lambda r: regions[r][1])
            if region is None or region == largest:
                continue  # the housing's own colour region: colour can't tell this pin apart
            by_region.setdefault((body_index, region), {}).setdefault(index, set()).add(seed)
    bundles: dict[int, set] = {}
    for (body_index, region), seeds in by_region.items():
        faces = components[body_index][1][region][0]
        primaries = {i: s for i, s in seeds.items() if pins[i].role != "mouth"}
        if len(primaries) > 1:
            for pin_index, share in _split_by_nearest_seed(document, body_index, faces, primaries).items():
                bundles.setdefault(pin_index, set()).update(share)
        else:
            owner_pin = next(iter(primaries)) if primaries else min(seeds)
            bundles.setdefault(owner_pin, set()).update((body_index, f) for f in faces)
    return bundles


def grow_pin_regions(
    document: EditorDocument,
    pins: list[Pin],
    *,
    area_factor: float = 4.0,
    only: set[int] | None = None,
    progress=None,
) -> list[list[tuple[int, int]] | None]:
    """Grow each detected pin from its seed faces by edge flow: expand across
    shared edges for as long as the faces stay pin-scaled, stopping when the
    flow would enter a BODY face (area beyond `area_factor` x the largest
    seed face) or another pin's territory. Returns the grown (body, face)
    region per pin; None for pins that already are whole bodies.

    `only` grows just those pin indices (every pin's SEEDS still claim
    territory, so flows stop where they would in a full run) — the factor
    sweep samples representative pins this way instead of growing all 291
    pins of a DDR5 eight times over.

    `progress(done, total)` is an optional plain callable (no Qt — keeps this
    headless-safe) fired once per grown pin so the GUI can drive a loading
    bar on big unibody connectors, where this is the tool-switch bottleneck."""
    claimed = _claim_primary_seeds(pins)
    primary_bodies = _primary_body_ids(pins)
    areas_cache: dict[int, dict[int, float]] = {}
    adjacency_cache: dict[int, dict] = {}

    def body_areas(body_index: int) -> dict[int, float]:
        if body_index not in areas_cache:
            areas_cache[body_index] = mesh_face_areas(document.bodies[body_index].mesh)
        return areas_cache[body_index]

    def body_adjacency(body_index: int):
        if body_index not in adjacency_cache:
            adjacency_cache[body_index] = document.face_smooth_adjacency(body_index, None)
        return adjacency_cache[body_index]

    def mouth_is_passive(pin) -> bool:
        # A CON/HEAD pin on an already-split contact, or swallowed by a tail's
        # region, is the same metal — keep it a passive anchor (no extra cut).
        if any(body_id in primary_bodies for body_id, _face in pin.face_ids):
            return True
        owners = {claimed.get(key) for key in pin.face_ids if key in claimed}
        return any(
            owner is not None and pins[owner].role != "mouth" for owner in owners
        )

    colour_cache: dict[int, tuple] = {}

    def colour_gate(body_index: int, seeds) -> tuple[set, bool] | None:
        """On a body with more than one colour: (faces of the seed's colour
        region, whether that region is contact metal, i.e. not the body's
        largest = the housing / shell). None on a one-colour body."""
        if body_index not in colour_cache:
            owner, regions = _color_components(document, body_index)
            colours = {_face_color(document.bodies[body_index], f) for f in owner}
            colour_cache[body_index] = (owner, regions) if len(colours) > 1 else None
        cached = colour_cache[body_index]
        if cached is None:
            return None
        owner, regions = cached
        region = owner.get(next(iter(seeds)))
        if region is None:
            return None
        largest = max(range(len(regions)), key=lambda r: regions[r][1])
        return regions[region][0], region != largest

    def flow(body_index, seeds, limit, same_colour, pin_index) -> set:
        """Edge flow from the seeds, stopping at body-scale faces, claimed
        faces and (on a coloured body) colour changes."""
        areas, adjacency = body_areas(body_index), body_adjacency(body_index)
        grown, frontier = set(), list(seeds)
        while frontier:
            for neighbor in adjacency.get(frontier.pop(), ()):
                key = (body_index, neighbor)
                if key in claimed or areas.get(neighbor, np.inf) > limit:
                    continue  # claimed, or reached the BODY — flow stops
                if same_colour is not None and neighbor not in same_colour:
                    continue  # a colour change is a boundary too
                claimed[key] = pin_index
                grown.add(key)
                frontier.append(neighbor)
        return grown

    def grow_body(body_index, seeds, pin_index, mouth_phase) -> set:
        # CON/HEAD seeds are single small mouth faces; the region must
        # reach the whole exposed beam or its boundary is a slit no cap
        # can fill — give them 6x the tail headroom.
        factor = area_factor * (6.0 if mouth_phase else 1.0)
        limit = max(body_areas(body_index).get(face, 0.0) for face in seeds) * factor
        gate = colour_gate(body_index, seeds)
        if gate is not None and gate[1]:  # the seed's own colour region (not the housing's): that whole contact
            return {(body_index, f) for f in gate[0] if (body_index, f) not in claimed}
        return flow(body_index, seeds, limit, gate[0] if gate else None, pin_index)

    def grow_one(pin, pin_index, mouth_phase):
        region = set(pin.face_ids)
        by_body: dict[int, set[int]] = {}
        for body_index, face_index in pin.face_ids:
            by_body.setdefault(body_index, set()).add(face_index)
        for body_index, seeds in by_body.items():
            region |= grow_body(body_index, seeds, pin_index, mouth_phase)
        for key in region:
            claimed.setdefault(key, pin_index)
        return sorted(region)

    def process_pin(pin, pin_index, mouth_phase):
        """Grow this pin, or None when it should be skipped this phase."""
        if (pin.role == "mouth") != mouth_phase:
            return None
        if stays_with_body(pin):
            return None  # a GUIDE peg stays part of the body
        if only is not None and pin_index not in only:
            return None
        if pin.body_ids or not pin.face_ids:
            return None  # already its own body (or nothing to grow)
        if mouth_phase:
            if mouth_is_passive(pin):
                return None
            for key in pin.face_ids:
                claimed[key] = pin_index
        return grow_one(pin, pin_index, mouth_phase)

    total = _growable_count(pins, only)
    done = 0
    if progress is not None:
        progress(0, total)

    grown: list[list[tuple[int, int]] | None] = [None] * len(pins)
    for mouth_phase in (False, True):
        for pin_index, pin in enumerate(pins):
            region = process_pin(pin, pin_index, mouth_phase)
            if region is None:
                continue
            grown[pin_index] = region
            done += 1
            if progress is not None:
                progress(min(done, total), total)
    merge_touching_mouths(document, pins, grown)
    return grown


def _touching_primary(document, pins, grown, region) -> int | None:
    """The SMT/THR pin whose grown region shares an edge with `region`."""
    owner = {key: i for i, r in enumerate(grown) if r and pins[i].role != "mouth" for key in r}
    adjacency_of: dict[int, dict] = {}
    for body_index, face in region:
        if body_index not in adjacency_of:
            adjacency_of[body_index] = document.face_smooth_adjacency(body_index, None)
        for neighbor in adjacency_of[body_index].get(face, ()):
            hit = owner.get((body_index, neighbor))
            if hit is not None:
                return hit
    return None


def _mouth_tail(document, pins, grown, index: int, owner: dict) -> int | None:
    """The tail a mouth belongs to: the tail region its own region touches,
    or (a passive mouth, never grown) the tail region holding its faces."""
    if grown[index]:
        return _touching_primary(document, pins, grown, grown[index])
    return next((owner[tuple(k)] for k in pins[index].face_ids if tuple(k) in owner), None)


def _join_mouth(pins, grown, mouth: int, tail: int) -> None:
    grown[tail] = sorted(set(grown[tail]) | set(grown[mouth] or []))
    grown[mouth] = None
    if pins[tail].name and pins[mouth].name != pins[tail].name:
        pins[mouth].name = pins[tail].name  # the metal decides the pairing, not an earlier join
        pins[mouth].name_source = "joined"


def merge_touching_mouths(document, pins, grown) -> dict[int, int]:
    """A CON/HEAD mouth whose region touches a tail's region (or lies in it)
    is the same contact, running from the tail out to the mouth: its faces
    join the tail's region so the split cuts one whole contact, not a stub
    and a stranded beam, and it takes the tail's designator (the touching
    metal is the pairing, whatever an earlier join said). Returns {mouth: tail}."""
    owner = {key: i for i, r in enumerate(grown) if r and pins[i].role != "mouth" for key in r}
    joined: dict[int, int] = {}
    for index in [i for i, pin in enumerate(pins) if pin.role == "mouth"]:
        tail = _mouth_tail(document, pins, grown, index, owner)
        if tail is not None:
            _join_mouth(pins, grown, index, tail)
            joined[index] = tail
    return joined


# ----------------------------------------------------------- context plane

def section_segments(mesh: BodyMesh, origin, normal) -> np.ndarray:
    """Mesh/plane intersection segments, (S, 2, 3) — the visible section
    curve while dragging the context plane."""
    origin = np.asarray(origin, dtype=np.float64)
    normal = np.asarray(normal, dtype=np.float64)
    distances = (mesh.points - origin) @ normal
    tri_d = distances[mesh.tris]
    crossing = (tri_d.max(axis=1) > 0.0) & (tri_d.min(axis=1) < 0.0)
    segments = []
    for tri, d in zip(mesh.tris[crossing], tri_d[crossing]):
        points = []
        for i in range(3):
            j = (i + 1) % 3
            if (d[i] > 0.0) != (d[j] > 0.0):
                t = d[i] / (d[i] - d[j])
                a, b = mesh.points[tri[i]], mesh.points[tri[j]]
                points.append(a + t * (b - a))
        if len(points) == 2:
            segments.append(points)
    if not segments:
        return np.empty((0, 2, 3), dtype=np.float64)
    return np.asarray(segments, dtype=np.float64)


def detect_pins_by_section(
    document: EditorDocument,
    origin,
    normal,
    *,
    count_only: bool = False,
) -> list[Pin] | int:
    """CONTEXT-plane detection: slice the model with a plane; every closed
    shape in that plane is a pin. A pin region = the faces on the positive
    (outward) side of the plane connected to a face the plane crosses —
    i.e. each ball / lead foot the plane cuts through, never the package
    body above it. With count_only, just return the number of closed shapes
    (for live feedback while dragging)."""
    origin = np.asarray(origin, dtype=np.float64)
    normal = np.asarray(normal, dtype=np.float64)
    normal = normal / max(float(np.linalg.norm(normal)), 1.0e-12)
    xmin, xmax, ymin, ymax, zmin, zmax = document.bounds()
    eps = max(float(np.linalg.norm([xmax - xmin, ymax - ymin, zmax - zmin])), 1.0) * 1.0e-9

    count = 0
    pins: list[Pin] = []
    for body_index, body in enumerate(document.bodies):
        mesh = body.mesh
        if mesh is None or len(mesh.tris) == 0:
            continue
        distances = (mesh.points - origin) @ normal
        tri_d = distances[mesh.tris]
        crossing = (tri_d.max(axis=1) > eps) & (tri_d.min(axis=1) < -eps)
        crossed_faces = set(int(f) for f in np.unique(mesh.tri_face_ids[crossing]))
        if not crossed_faces:
            continue
        positive = tri_d.max(axis=1) > eps
        candidates = set(int(f) for f in np.unique(mesh.tri_face_ids[positive]))
        adjacency = document.face_smooth_adjacency(body_index, None)
        components = [
            component
            for component in _connected_components(candidates, adjacency)
            if component & crossed_faces
        ]
        components = _merge_regions_by_xy(mesh, components)
        count += len(components)
        if count_only:
            continue
        for region in components:
            centroid = mesh_region_centroid(mesh, sorted(region))
            if centroid is not None:
                pins.append(
                    Pin(
                        number=0,
                        centroid=centroid,
                        face_ids=[(body_index, fid) for fid in sorted(region)],
                    )
                )
    return count if count_only else pins


# ----------------------------------------------------------------- ordering

def _path_length(centroids: np.ndarray, order: list[int]) -> float:
    """Total walk length through pins in `order` — the serpentine that snakes
    cleanly along the row/column is the SHORTEST; a variant that orders a
    constant-X column by its (degenerate) X makes big jumps and scores worse.
    Used to break ties when an anchor/hint under-constrains the variant."""
    if len(order) < 2:
        return 0.0
    pts = centroids[order]
    return float(np.linalg.norm(np.diff(pts, axis=0), axis=1).sum())


def _serpentine_variant(
    centroids: np.ndarray, flip_x: bool, start_top: bool, axis: int = 1
) -> list[int]:
    """axis=1: rows split along Y, ordered by X (DIP/SOIC). axis=0: rows are
    COLUMNS split along X, ordered by Y (edge connectors)."""
    other = 0 if axis == 1 else 1
    mid = (centroids[:, axis].min() + centroids[:, axis].max()) * 0.5
    low = [i for i in range(len(centroids)) if centroids[i, axis] < mid]
    high = [i for i in range(len(centroids)) if centroids[i, axis] >= mid]
    first, second = (high, low) if start_top else (low, high)
    first = sorted(first, key=lambda i: centroids[i, other], reverse=flip_x)
    second = sorted(second, key=lambda i: centroids[i, other], reverse=not flip_x)
    return first + second


def _single_line_axis(centroids: np.ndarray) -> int | None:
    """The long-axis index (0/1) when the pins form ONE straight line — the
    perpendicular spread is far smaller than the along-axis pitch. A
    micro-staggered single row (sub-micron Y noise from mesh centroids) reads
    as one line here, so it is numbered monotonically, not snaked as two
    spurious sub-rows. None when there are genuinely separated rows."""
    if len(centroids) < 3:
        return 0
    span = centroids[:, :2].max(axis=0) - centroids[:, :2].min(axis=0)
    long_axis = int(np.argmax(span))
    along = np.sort(centroids[:, long_axis])
    gaps = np.diff(along)
    gaps = gaps[gaps > 1e-9]
    pitch = float(np.median(gaps)) if len(gaps) else 0.0
    if pitch > 0.0 and span[1 - long_axis] < 0.5 * pitch:
        return long_axis
    return None


def _line_order(centroids: np.ndarray, axis: int, pin1_hint) -> list[int]:
    """Order pins monotonically along `axis`, flipped so pin 1 is the end
    nearest the pin-1 hint (if any)."""
    order = sorted(range(len(centroids)), key=lambda i: centroids[i, axis])
    if pin1_hint is not None and order:
        hint = np.asarray(pin1_hint[:2], dtype=np.float64)
        if (np.linalg.norm(centroids[order[-1], :2] - hint)
                < np.linalg.norm(centroids[order[0], :2] - hint)):
            order = order[::-1]
    return order


def _is_ring(centroids: np.ndarray) -> bool:
    """A QFN/QFP perimeter ring (pins on ~4 sides) vs an elongated 1-2 row part:
    a ring has comparable spread on both in-plane axes."""
    rel = centroids[:, :2] - centroids[:, :2].mean(axis=0)
    singular = np.linalg.svd(rel, full_matrices=False)[1]
    return len(singular) >= 2 and singular[1] > 0.45 * singular[0]


def _rows(centroids: np.ndarray) -> list[list[int]]:
    """Pins clustered into rows of (nearly) equal Y."""
    order = sorted(range(len(centroids)), key=lambda i: -centroids[i, 1])
    tolerance = max(float(np.ptp(centroids[:, 1])) * 0.02, 1.0e-6)
    rows: list[list[int]] = []
    for index in order:
        if rows and abs(centroids[rows[-1][-1], 1] - centroids[index, 1]) <= tolerance:
            rows[-1].append(index)
        else:
            rows.append([index])
    return rows


def _is_grid(centroids: np.ndarray) -> bool:
    """A filled array (BGA / LGA): most inner rows hold more than the two
    side pins (+ a centre pad) a QFN ring has."""
    rows = _rows(centroids)
    inner = rows[1:-1]
    return len(rows) >= 3 and bool(inner) and sum(len(r) > 3 for r in inner) >= 0.5 * len(inner)


def _two_rows(centroids: np.ndarray) -> bool:
    """Exactly two rows (or two columns) of pins: SOT / SOIC / DIP / dual header."""
    columns = np.column_stack([centroids[:, 1], centroids[:, 0]])
    return len(_rows(centroids)) == 2 or len(_rows(columns)) == 2


def _four_sided(centroids: np.ndarray) -> bool:
    """Pins along all four edges (a QFN / QFP ring), at least two per edge."""
    lo, hi = centroids[:, :2].min(axis=0), centroids[:, :2].max(axis=0)
    band = 0.15 * np.maximum(hi - lo, 1e-9)
    near = [centroids[:, axis] <= lo[axis] + band[axis] for axis in (0, 1)]
    near += [centroids[:, axis] >= hi[axis] - band[axis] for axis in (0, 1)]
    return all(int(side.sum()) >= 2 for side in near)


PIN_PATTERNS = {  # key -> what the Predict dropdown shows
    "ring": "RING (QFN / QFP)",
    "rows": "TWO ROW (SOT / SOIC / DIP)",
    "line": "LINE (single-row header)",
    "grid": "GRID (BGA / LGA)",
}


def pin_pattern(centroids) -> str:
    """Which numbering a pin layout calls for: "line" (one row), "grid"
    (BGA / LGA array), "rows" (two rows: SOT / SOIC / DIP serpentine) or
    "ring" (QFN / QFP perimeter, centre pad last)."""
    centroids = np.asarray(centroids, dtype=np.float64)
    if len(centroids) < 3 or _single_line_axis(centroids) is not None:
        return "line"
    if _is_grid(centroids):
        return "grid"
    if _two_rows(centroids):
        return "rows"
    if len(centroids) >= 8 and _four_sided(centroids):
        return "ring"
    return "rows"


def _ring_split(centroids: np.ndarray) -> tuple[list[int], list[int]]:
    """(perimeter pins counter-clockwise seen from +Z, centre pads)."""
    rel = centroids[:, :2] - centroids[:, :2].mean(axis=0)
    radius = np.linalg.norm(rel, axis=1)
    rmax = max(float(radius.max()), 1e-9)
    perimeter = [i for i in range(len(radius)) if radius[i] > 0.35 * rmax]
    centre = [i for i in range(len(radius)) if radius[i] <= 0.35 * rmax]
    perimeter.sort(key=lambda i: float(np.arctan2(rel[i, 1], rel[i, 0])))
    return perimeter, centre


def _ring_order(centroids: np.ndarray) -> list[int]:
    """Walk a perimeter ring by angle around its centre — a continuous, unique
    traversal (consecutive pins are spatially adjacent). Any centre pad(s) sit
    near the centre and come last."""
    perimeter, centre = _ring_split(centroids)
    return perimeter + centre


def _ring_candidates(centroids: np.ndarray, anchors: dict[int, int]) -> list[list[int]]:
    """The ring walk started so each anchored pin lands on its number, both
    ways round (counter-clockwise first), centre pads last."""
    perimeter, centre = _ring_split(centroids)
    out = []
    for walk in (perimeter, perimeter[::-1]):
        starts = [0] + [(walk.index(i) - (n - 1)) % len(walk) for i, n in anchors.items() if i in walk]
        for start in dict.fromkeys(starts):
            out.append(walk[start:] + walk[:start] + centre)
    return out


def _hint_anchor(centroids: np.ndarray, pin1_hint) -> dict[int, int]:
    """{perimeter pin nearest the pin-1 hint: 1}, or {} without a hint."""
    if pin1_hint is None:
        return {}
    perimeter, _centre = _ring_split(centroids)
    hint = np.asarray(pin1_hint[:2], dtype=np.float64)
    return {min(perimeter, key=lambda i: float(np.linalg.norm(centroids[i, :2] - hint))): 1}


def _predict_ring(centroids: np.ndarray, anchors: dict[int, int]) -> list[int]:
    """QFN / QFP: the ring walk that agrees with most anchors; then the one
    where pin 2 is pin 1's neighbour along its own side (the corner gap lies
    behind pin 1, as on every QFN); then counter-clockwise (seen from the top)."""
    candidates = _ring_candidates(centroids, anchors)

    def fit(k: int) -> tuple:
        order = candidates[k]
        hits = sum(1 for i, n in anchors.items() if 1 <= n <= len(order) and order[n - 1] == i)
        step = float(np.linalg.norm(centroids[order[0], :2] - centroids[order[1], :2])) if len(order) > 1 else 0.0
        return hits, -round(step, 6), -k

    return candidates[max(range(len(candidates)), key=fit)]


def serpentine_order(
    centroids: np.ndarray,
    *,
    pin1_hint: tuple[float, float] | None = None,
    pattern: str | None = None,
) -> list[int]:
    """Geometry-adaptive numbering that is always UNIQUE and a continuous
    traversal (so a footprint can remap it later): a single (even
    micro-staggered) row -> monotonic along its long axis; a QFN/QFP perimeter
    ring -> an angular walk with any centre pad last; otherwise the two-row
    serpentine (one row ascending X, the other descending), pin-1 hint picking
    the start."""
    centroids = np.asarray(centroids, dtype=np.float64)
    pattern = pattern or pin_pattern(centroids)
    if pattern == "line" and _single_line_axis(centroids) is not None:
        return _line_order(centroids, _single_line_axis(centroids), pin1_hint)
    if pattern == "grid":
        return row_major_order(centroids)
    if pattern == "ring":
        return _predict_ring(centroids, _hint_anchor(centroids, pin1_hint))
    if pin1_hint is None:
        return _serpentine_variant(centroids, False, False)
    return _serpentine_with_hint(centroids, pin1_hint)


def _serpentine_with_hint(centroids: np.ndarray, pin1_hint) -> list[int]:
    """The serpentine variant whose first pin lies nearest the pin-1 hint, then
    the cleanest (shortest) snake — so a column is numbered monotonically along
    its varying axis, not scrambled by a constant one."""
    hint = np.asarray(pin1_hint[:2], dtype=np.float64)
    best, best_key = None, None
    for axis in (1, 0):
        for flip_x in (False, True):
            for start_top in (False, True):
                order = _serpentine_variant(centroids, flip_x, start_top, axis)
                if not order:
                    continue
                key = (round(float(np.linalg.norm(centroids[order[0], :2] - hint)), 3),
                       _path_length(centroids, order))
                if best_key is None or key < best_key:
                    best, best_key = order, key
    return best or _serpentine_variant(centroids, False, False)


def row_letter(ordinal: int) -> str:
    """JEDEC row letter for a 0-based row ordinal (A..Y, then AA, AB, ...)."""
    if ordinal < len(JEDEC_ALPHABET):
        return JEDEC_ALPHABET[ordinal]
    first = ordinal // len(JEDEC_ALPHABET) - 1
    second = ordinal % len(JEDEC_ALPHABET)
    return JEDEC_ALPHABET[first] + JEDEC_ALPHABET[second]


def parse_grid_name(name: str) -> tuple[int, int] | None:
    """'B3' -> (row_ordinal 1, column 3). None if not a JEDEC grid name."""
    match = re.fullmatch(r"([A-Za-z]+)(\d+)", name.strip())
    if not match:
        return None
    ordinal = 0
    for char in match.group(1).upper():
        if char not in JEDEC_ALPHABET:
            return None
        ordinal = ordinal * len(JEDEC_ALPHABET) + JEDEC_ALPHABET.index(char) + 1
    return ordinal - 1, int(match.group(2))


def cluster_ordinals(values) -> np.ndarray:
    """0-based grid ordinal (ascending value) for 1D positions. The pitch is
    the median significant gap; larger gaps advance the ordinal by the number
    of pitches they span, so depopulated rows/columns (e.g. the DDR ball
    channel) keep their JEDEC numbering."""
    values = np.asarray(values, dtype=np.float64)
    ordinals = np.zeros(len(values), dtype=int)
    if len(values) <= 1:
        return ordinals
    order = np.argsort(values)
    sorted_values = values[order]
    spread = float(sorted_values[-1] - sorted_values[0])
    if spread <= 1.0e-9:
        return ordinals
    diffs = np.diff(sorted_values)
    noise = max(spread * 0.01, 1.0e-9)
    significant = diffs[diffs > noise]
    if len(significant) == 0:
        return ordinals
    pitch = float(np.median(significant))
    threshold = max(pitch * 0.5, noise)
    current = 0
    for i in range(1, len(sorted_values)):
        gap = float(diffs[i - 1])
        if gap > threshold:
            current += max(1, int(round(gap / pitch)))
        ordinals[order[i]] = current
    return ordinals


def _solve_axis(
    anchor_pairs, *, default_sign: int, base: int, count: int, axis: str = "axis"
) -> tuple[int, int, list[str]]:
    """Fit target = sign*cluster + offset from anchors by MAJORITY: the
    orientation most anchors agree on wins; disagreeing anchors are returned
    as outlier labels instead of failing the whole prediction. Without
    anchors, the default orientation is used."""
    if not anchor_pairs:
        offset = base if default_sign > 0 else base + count - 1
        return default_sign, offset, []
    candidates = []
    for sign in (1, -1):
        offsets: dict[int, list[str]] = {}
        for cluster, target, label in anchor_pairs:
            offsets.setdefault(target - sign * cluster, []).append(label)
        best_offset = max(offsets, key=lambda off: len(offsets[off]))
        outliers = [
            label
            for off, labels in offsets.items()
            if off != best_offset
            for label in labels
        ]
        candidates.append((sign, best_offset, outliers))

    def lowest_ordinal(candidate) -> int:
        sign, offset, _ = candidate
        return min(sign * cluster + offset for cluster in range(count))

    # Most agreeing anchors first; then grids starting at A/1; then default.
    candidates.sort(
        key=lambda c: (len(c[2]), lowest_ordinal(c) != base, c[0] != default_sign)
    )
    return candidates[0]


def marker_corner(document, pins: list["Pin"]) -> tuple[float, float] | None:
    """The XY corner of the ball field that carries the package's pin-1 mark
    (a dot, chamfer or notch): the one corner of the non-pin bodies with more
    faces near it, on the upper half of the package, than any other corner.
    None when no corner stands out (no mark, or a symmetric one)."""
    if not pins:
        return None
    xy = np.array([p.centroid[:2] for p in pins], dtype=float)
    owned = {b for p in pins for b in p.body_ids} | {b for p in pins for b, _f in p.face_ids}
    centres = _package_face_centres(document, owned)
    return _standout_corner(xy.min(axis=0), xy.max(axis=0), centres) if len(centres) else None


def _standout_corner(lo, hi, centres: np.ndarray) -> tuple[float, float] | None:
    """The corner with strictly the most face centres near it, or None."""
    span = float(min(hi - lo))
    radius = 0.12 * span if span > 0 else 1.0
    corners = [(float(x), float(y)) for x in (lo[0], hi[0]) for y in (lo[1], hi[1])]
    counts = [int(np.sum(np.linalg.norm(centres - np.array(c), axis=1) < radius)) for c in corners]
    best = int(np.argmax(counts))
    return corners[best] if counts.count(counts[best]) == 1 and counts[best] > 0 else None


def _package_face_centres(document, owned: set[int]) -> np.ndarray:
    """XY centres of the faces on the upper half of the non-pin bodies."""
    centres = []
    for index, body in enumerate(document.bodies):
        mesh = body.mesh
        if index in owned or mesh is None or not len(mesh.tris):
            continue
        tri = mesh.points[mesh.tris].mean(axis=1)
        mid = 0.5 * (mesh.points[:, 2].min() + mesh.points[:, 2].max())
        for face in np.unique(mesh.tri_face_ids):
            c = tri[mesh.tri_face_ids == face].mean(axis=0)
            if c[2] > mid:
                centres.append(c[:2])
    return np.array(centres, dtype=float).reshape(-1, 2)


def a1_anchor(pins: list["Pin"], corner: tuple[float, float] | None) -> dict[int, str]:
    """{index: 'A1'} for the ball nearest `corner`; without a corner, JEDEC's
    top-view default: the top-left ball (smallest X, largest Y)."""
    xy = np.array([p.centroid[:2] for p in pins], dtype=float)
    if corner is None:
        corner = (float(xy[:, 0].min()), float(xy[:, 1].max()))
    return {int(np.argmin(np.linalg.norm(xy - np.array(corner), axis=1))): "A1"}


def predict_grid_names(
    pins: list[Pin], anchors: dict[int, str], outliers: list | None = None
) -> dict[int, str]:
    """BGA grid naming (A1, A2, ... B1, ...) from ball positions. `anchors`
    (pin index -> name) are ground truth by MAJORITY: the axis assignment and
    orientation most anchors agree on wins (letters along Y is tried first,
    then along X), and any anchors that contradict the majority are appended
    to `outliers` (when given) so the user can fix those names."""
    attempts = []
    for row_axis, col_axis in ((1, 0), (0, 1)):
        try:
            attempts.append(
                _predict_grid_with_axes(pins, anchors, row_axis, col_axis)
            )
        except ValueError:
            continue
    if not attempts:
        raise ValueError("no grid orientation fits these pins")
    attempts.sort(key=lambda a: len(a[1]))  # fewest disagreeing anchors wins
    names, bad = attempts[0]
    if outliers is not None:
        outliers.extend(bad)
    return names


def _predict_grid_with_axes(
    pins: list[Pin], anchors: dict[int, str], row_axis: int, col_axis: int
) -> tuple[dict[int, str], list[str]]:
    centroids = np.array([pin.centroid for pin in pins], dtype=np.float64)
    rows = cluster_ordinals(centroids[:, row_axis])
    cols = cluster_ordinals(centroids[:, col_axis])

    row_anchors, col_anchors = [], []
    for index, name in anchors.items():
        parsed = parse_grid_name(name)
        if parsed is None:
            raise ValueError(f"'{name}' is not a grid name like B3")
        letter_ordinal, column = parsed
        row_anchors.append((int(rows[index]), letter_ordinal, name))
        col_anchors.append((int(cols[index]), column, name))

    row_sign, row_offset, row_outliers = _solve_axis(
        row_anchors, default_sign=-1, base=0, count=int(rows.max()) + 1, axis="row-letter"
    )
    col_sign, col_offset, col_outliers = _solve_axis(
        col_anchors, default_sign=1, base=1, count=int(cols.max()) + 1, axis="column"
    )
    outliers = sorted(set(row_outliers) | set(col_outliers))

    names: dict[int, str] = {}
    for index in range(len(pins)):
        letter_ordinal = row_sign * int(rows[index]) + row_offset
        column = col_sign * int(cols[index]) + col_offset
        if letter_ordinal < 0 or column < 1:
            names[index] = f"?{index + 1}"  # off-grid: not a real pin, or bad anchor
        else:
            names[index] = f"{row_letter(letter_ordinal)}{column}"
    # Two pins landing on the same cell means one of them isn't a ball
    # (index marks etc.) — flag rather than fail, so the user can delete it.
    seen: dict[str, int] = {}
    for index in sorted(names):
        name = names[index]
        if name.startswith("?"):
            continue
        if name in seen:
            names[index] = f"?{index + 1}"
        else:
            seen[name] = index
    return names, outliers


def _order_candidates(centroids: np.ndarray) -> list[list[int]]:
    """Geometric numbering candidates for a pin set: single-axis rows/columns
    in both directions (a straight row/column, even when micro-staggered) PLUS
    the 2-row/col serpentine variants (snaked dual rows). The anchor-fit picks
    among them, so straight, staggered, and snaked layouts are all covered
    without committing to one rigid pattern up front."""
    count = len(centroids)
    candidates: list[list[int]] = []
    for axis in (0, 1):
        straight = sorted(range(count), key=lambda i: centroids[i, axis])
        candidates.append(straight)
        candidates.append(straight[::-1])
    ring = _ring_order(centroids)            # QFN/QFP perimeter walk (+ reverse)
    candidates.append(ring)
    candidates.append(ring[::-1])
    for axis in (1, 0):
        for flip_x in (False, True):
            for start_top in (False, True):
                candidates.append(
                    _serpentine_variant(centroids, flip_x, start_top, axis))
    return candidates


def predict_serpentine_order(pins: list[Pin], anchors: dict[int, int],
                             pattern: str | None = None) -> list[int] | None:
    """Geometry-adaptive numbering: order the pins along whichever geometric
    traversal best AGREES with the manually numbered anchors (pin index ->
    number). Robust to micro-staggered rows and to a stray/partial anchor set —
    it maximises anchor agreement instead of demanding one exact variant, and
    always returns an ordering (the cleanest geometric one if nothing fits)."""
    if not pins:
        return None
    centroids = np.array([pin.centroid for pin in pins], dtype=np.float64)
    pattern = pattern or pin_pattern(centroids)
    if pattern == "ring":
        return _predict_ring(centroids, anchors)
    if pattern == "grid":
        return row_major_order(centroids)

    def fit(order: list[int]) -> tuple:
        hits = sum(1 for index, number in anchors.items()
                   if 1 <= number <= len(order) and order[number - 1] == index)
        return hits, -_path_length(centroids, order)

    return max(_order_candidates(centroids), key=fit)


def row_major_order(centroids: np.ndarray) -> list[int]:
    """BGA-style A1 ordering: rows from +Y down, ascending X within a row.
    Rows are clustered by Y with a pitch-relative tolerance."""
    centroids = np.asarray(centroids, dtype=np.float64)
    order = sorted(range(len(centroids)), key=lambda i: -centroids[i, 1])
    if not order:
        return []
    ys = centroids[:, 1]
    spread = float(ys.max() - ys.min())
    tolerance = max(spread * 0.02, 1.0e-6)
    rows: list[list[int]] = []
    for index in order:
        if rows and abs(centroids[rows[-1][-1], 1] - centroids[index, 1]) <= tolerance:
            rows[-1].append(index)
        else:
            rows.append([index])
    result: list[int] = []
    for row in rows:
        result.extend(sorted(row, key=lambda i: centroids[i, 0]))
    return result


def order_pins(
    pins: list[Pin],
    *,
    mode: str = "serpentine",
    pin1_hint: tuple[float, float] | None = None,
    pattern: str | None = None,
) -> list[int]:
    if not pins:
        return []
    centroids = np.array([pin.centroid for pin in pins], dtype=np.float64)
    if mode in ("row-major", "bga-grid"):
        return row_major_order(centroids)
    return serpentine_order(centroids, pin1_hint=pin1_hint, pattern=pattern)
