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
    hitbox: dict | None = None


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
    """Area-weighted dominant colour among the given faces (all faces when
    None). None if none of them carry a colour."""
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
    gaps = np.diff(np.sort(t_primary))
    gaps = gaps[gaps > 1e-9]
    pitch = float(np.median(gaps)) if len(gaps) else np.inf

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
        if not p.body_ids and p.face_ids and (only is None or i in only)
    )


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

    def grow_one(pin, pin_index, mouth_phase):
        region = set(pin.face_ids)
        by_body: dict[int, set[int]] = {}
        for body_index, face_index in pin.face_ids:
            by_body.setdefault(body_index, set()).add(face_index)
        for body_index, seeds in by_body.items():
            areas = body_areas(body_index)
            adjacency = body_adjacency(body_index)
            # CON/HEAD seeds are single small mouth faces; the region must
            # reach the whole exposed beam or its boundary is a slit no cap
            # can fill — give them 6x the tail headroom.
            factor = area_factor * (6.0 if mouth_phase else 1.0)
            limit = max(areas.get(face, 0.0) for face in seeds) * factor
            frontier = list(seeds)
            while frontier:
                face = frontier.pop()
                for neighbor in adjacency.get(face, ()):
                    key = (body_index, neighbor)
                    if key in claimed or areas.get(neighbor, np.inf) > limit:
                        continue  # claimed, or reached the BODY — flow stops
                    claimed[key] = pin_index
                    region.add(key)
                    frontier.append(neighbor)
        return sorted(region)

    def process_pin(pin, pin_index, mouth_phase):
        """Grow this pin, or None when it should be skipped this phase."""
        if (pin.role == "mouth") != mouth_phase:
            return None
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
    return grown


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


def serpentine_order(
    centroids: np.ndarray,
    *,
    pin1_hint: tuple[float, float] | None = None,
) -> list[int]:
    """Two-row serpentine numbering per the design intent's example: one row
    ascending X, the opposite row descending X. With a pin-1 hint, the variant
    whose first pin lies nearest the hint wins."""
    centroids = np.asarray(centroids, dtype=np.float64)
    if pin1_hint is None:
        return _serpentine_variant(centroids, False, False)

    hint = np.asarray(pin1_hint[:2], dtype=np.float64)
    best, best_key = None, None
    for axis in (1, 0):
        for flip_x in (False, True):
            for start_top in (False, True):
                order = _serpentine_variant(centroids, flip_x, start_top, axis)
                if not order:
                    continue
                # anchor pin 1 at the hint first, then prefer the cleanest
                # (shortest) snake — so a column is numbered monotonically
                # along its varying axis, not scrambled by a constant one.
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


def predict_serpentine_order(pins: list[Pin], anchors: dict[int, int]) -> list[int] | None:
    """Find the serpentine variant whose numbering matches every manually
    numbered pin (pin index -> number). None if no variant fits."""
    centroids = np.array([pin.centroid for pin in pins], dtype=np.float64)
    matching = []
    for axis in (1, 0):  # rows along Y (chips) OR columns along X (connectors)
        for flip_x in (False, True):
            for start_top in (False, True):
                order = _serpentine_variant(centroids, flip_x, start_top, axis)
                if all(
                    order.index(index) + 1 == number
                    for index, number in anchors.items()
                ):
                    matching.append(order)
    if not matching:
        return None
    # several variants can satisfy a single anchor — take the cleanest snake
    return min(matching, key=lambda o: _path_length(centroids, o))


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
) -> list[int]:
    if not pins:
        return []
    centroids = np.array([pin.centroid for pin in pins], dtype=np.float64)
    if mode in ("row-major", "bga-grid"):
        return row_major_order(centroids)
    return serpentine_order(centroids, pin1_hint=pin1_hint)
