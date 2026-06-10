"""Pin registry and detection math. Everything here is pure (document +
numbers in, pins out) so Detect Pins can run headlessly from the journal and
the selftests."""

from __future__ import annotations

from dataclasses import dataclass, field

import numpy as np

from .document import BodyMesh, EditorDocument


@dataclass
class Pin:
    number: int
    centroid: tuple[float, float, float]
    body_ids: list[int] = field(default_factory=list)            # pins that are whole solids
    face_ids: list[tuple[int, int]] = field(default_factory=list)  # (body, face) regions
    kind: str = "pin"
    name: str = ""
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

    pins: list[Pin] = []
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


# ----------------------------------------------------------------- ordering

def serpentine_order(
    centroids: np.ndarray,
    *,
    pin1_hint: tuple[float, float] | None = None,
) -> list[int]:
    """Two-row serpentine numbering per the design intent's example: one row
    ascending X, the opposite row descending X. With a pin-1 hint, the variant
    whose first pin lies nearest the hint wins."""
    centroids = np.asarray(centroids, dtype=np.float64)
    y_mid = (centroids[:, 1].min() + centroids[:, 1].max()) * 0.5
    bottom = [i for i in range(len(centroids)) if centroids[i, 1] < y_mid]
    top = [i for i in range(len(centroids)) if centroids[i, 1] >= y_mid]

    def variant(flip_x: bool, start_top: bool) -> list[int]:
        first, second = (top, bottom) if start_top else (bottom, top)
        first = sorted(first, key=lambda i: centroids[i, 0], reverse=flip_x)
        second = sorted(second, key=lambda i: centroids[i, 0], reverse=not flip_x)
        return first + second

    if pin1_hint is None:
        return variant(False, False)

    hint = np.asarray(pin1_hint[:2], dtype=np.float64)
    best, best_distance = None, np.inf
    for flip_x in (False, True):
        for start_top in (False, True):
            order = variant(flip_x, start_top)
            if not order:
                continue
            distance = float(np.linalg.norm(centroids[order[0], :2] - hint))
            if distance < best_distance:
                best, best_distance = order, distance
    return best or variant(False, False)


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
    if mode == "row-major":
        return row_major_order(centroids)
    return serpentine_order(centroids, pin1_hint=pin1_hint)
