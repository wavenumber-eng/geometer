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
    name: str = ""
    name_source: str = ""  # "" (default) | "anchor" (user-typed) | "predicted"
    function: str = ""
    hitbox: dict | None = None


class PinRegistry:
    def __init__(self) -> None:
        self.pins: list[Pin] = []

    def clear(self) -> None:
        self.pins = []

    def set_pins(self, pins: list[Pin]) -> None:
        self.pins = pins
        self._renumber()

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


def detect_pins_multibody(
    document: EditorDocument,
    band: Band,
    *,
    exclude_largest: bool = True,
) -> list[Pin]:
    """Easy case: pins are separate solids. A body is a pin candidate when its
    XY bounding box lies inside the band; the largest-volume body in the whole
    document (the package) is excluded by default."""
    from .document import shape_volume

    largest_index = -1
    if exclude_largest and len(document.bodies) > 1:
        volumes = [shape_volume(b.solid) or 0.0 for b in document.bodies]
        largest_index = int(np.argmax(volumes))

    pins: list[Pin] = []
    for index, body in enumerate(document.bodies):
        if index == largest_index or body.mesh is None or len(body.mesh.points) == 0:
            continue
        xy = body.mesh.points[:, :2]
        if (
            band.contains_xy(*xy.min(axis=0))
            and band.contains_xy(*xy.max(axis=0))
        ):
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


def grow_pin_regions(
    document: EditorDocument,
    pins: list[Pin],
    *,
    area_factor: float = 4.0,
) -> list[list[tuple[int, int]] | None]:
    """Grow each detected pin from its seed faces by edge flow: expand across
    shared edges for as long as the faces stay pin-scaled, stopping when the
    flow would enter a BODY face (area beyond `area_factor` x the largest
    seed face) or another pin's territory. Returns the grown (body, face)
    region per pin; None for pins that already are whole bodies."""
    claimed: dict[tuple[int, int], int] = {}
    for pin_index, pin in enumerate(pins):
        for key in pin.face_ids:
            claimed[key] = pin_index

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

    grown: list[list[tuple[int, int]] | None] = []
    for pin_index, pin in enumerate(pins):
        if pin.body_ids or not pin.face_ids:
            grown.append(None)  # already its own body (or nothing to grow)
            continue
        region = set(pin.face_ids)
        by_body: dict[int, set[int]] = {}
        for body_index, face_index in pin.face_ids:
            by_body.setdefault(body_index, set()).add(face_index)
        for body_index, seeds in by_body.items():
            areas = body_areas(body_index)
            adjacency = body_adjacency(body_index)
            limit = max(areas.get(face, 0.0) for face in seeds) * area_factor
            frontier = list(seeds)
            while frontier:
                face = frontier.pop()
                for neighbor in adjacency.get(face, ()):
                    key = (body_index, neighbor)
                    if key in claimed:
                        continue
                    if areas.get(neighbor, np.inf) > limit:
                        continue  # reached the BODY — flow stops here
                    claimed[key] = pin_index
                    region.add(key)
                    frontier.append(neighbor)
        grown.append(sorted(region))
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

def _serpentine_variant(centroids: np.ndarray, flip_x: bool, start_top: bool) -> list[int]:
    y_mid = (centroids[:, 1].min() + centroids[:, 1].max()) * 0.5
    bottom = [i for i in range(len(centroids)) if centroids[i, 1] < y_mid]
    top = [i for i in range(len(centroids)) if centroids[i, 1] >= y_mid]
    first, second = (top, bottom) if start_top else (bottom, top)
    first = sorted(first, key=lambda i: centroids[i, 0], reverse=flip_x)
    second = sorted(second, key=lambda i: centroids[i, 0], reverse=not flip_x)
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
    best, best_distance = None, np.inf
    for flip_x in (False, True):
        for start_top in (False, True):
            order = _serpentine_variant(centroids, flip_x, start_top)
            if not order:
                continue
            distance = float(np.linalg.norm(centroids[order[0], :2] - hint))
            if distance < best_distance:
                best, best_distance = order, distance
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
) -> tuple[int, int]:
    """Fit target = sign*cluster + offset from anchors; without anchors (or
    with ambiguous ones) fall back to the default orientation. anchor_pairs
    items are (cluster, target, label) — labels make conflicts reportable."""
    if not anchor_pairs:
        offset = base if default_sign > 0 else base + count - 1
        return default_sign, offset
    solutions = []
    best_outliers: list[str] | None = None
    for sign in (1, -1):
        offsets: dict[int, list[str]] = {}
        for cluster, target, label in anchor_pairs:
            offsets.setdefault(target - sign * cluster, []).append(label)
        if len(offsets) == 1:
            solutions.append((sign, next(iter(offsets))))
        else:
            majority = max(offsets.values(), key=len)
            outliers = [
                label
                for group in offsets.values()
                if group is not majority
                for label in group
            ]
            if best_outliers is None or len(outliers) < len(best_outliers):
                best_outliers = outliers
    if not solutions:
        suspects = ", ".join((best_outliers or [])[:6]) or "?"
        raise ValueError(
            f"{axis} anchors disagree — no single orientation fits; "
            f"check pin(s) named {suspects}"
        )

    def lowest_ordinal(solution) -> int:
        sign, offset = solution
        return min(sign * cluster + offset for cluster in range(count))

    # A single anchor satisfies both directions; prefer the one whose grid
    # starts at A / 1 (chips almost always do), then the default orientation.
    solutions.sort(
        key=lambda solution: (lowest_ordinal(solution) != base,
                              solution[0] != default_sign)
    )
    return solutions[0]


def predict_grid_names(pins: list[Pin], anchors: dict[int, str]) -> dict[int, str]:
    """BGA grid naming (A1, A2, ... B1, ...) from ball positions. `anchors`
    (pin index -> name) are ground truth: they fix the row-letter and
    column-number directions — and even which world axis carries the letters:
    if the default (letters along Y) cannot satisfy the anchors, the swapped
    assignment (letters along X) is tried before failing."""
    errors: list[ValueError] = []
    for row_axis, col_axis in ((1, 0), (0, 1)):
        try:
            return _predict_grid_with_axes(pins, anchors, row_axis, col_axis)
        except ValueError as exc:
            errors.append(exc)
    raise errors[0]


def _predict_grid_with_axes(
    pins: list[Pin], anchors: dict[int, str], row_axis: int, col_axis: int
) -> dict[int, str]:
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

    row_sign, row_offset = _solve_axis(
        row_anchors, default_sign=-1, base=0, count=int(rows.max()) + 1, axis="row-letter"
    )
    col_sign, col_offset = _solve_axis(
        col_anchors, default_sign=1, base=1, count=int(cols.max()) + 1, axis="column"
    )

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
    return names


def predict_serpentine_order(pins: list[Pin], anchors: dict[int, int]) -> list[int] | None:
    """Find the serpentine variant whose numbering matches every manually
    numbered pin (pin index -> number). None if no variant fits."""
    centroids = np.array([pin.centroid for pin in pins], dtype=np.float64)
    for flip_x in (False, True):
        for start_top in (False, True):
            order = _serpentine_variant(centroids, flip_x, start_top)
            if all(
                order.index(index) + 1 == number for index, number in anchors.items()
            ):
                return order
    return None


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
