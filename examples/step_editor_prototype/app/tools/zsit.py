"""Redefine Z-Sit plane: the user clicks 3 points to define a plane (the XYZ
origin lands on it) and a 4th point to choose which side is +Z, then the whole
model — assemblies included — is repositioned so that plane is Z=0, Z up.

The math is a pure function (`compute_zsit_matrix`) so the journal can replay
it headlessly; the GUI only collects the four picks and the parameters."""

from __future__ import annotations

import numpy as np
from PySide6.QtWidgets import (
    QCheckBox,
    QComboBox,
    QHBoxLayout,
    QLabel,
    QPushButton,
    QVBoxLayout,
    QWidget,
)

from .base import ToolMode, make_apply_button

ORIGIN_RULES = ("rect-center", "centroid", "first-pick", "hull-center")
_EPS = 1.0e-9


def _hull_center_uv(uv) -> np.ndarray:
    """Geometric centre of the 2D convex hull of `uv` (Nx2): the area
    centroid of the hull polygon. Used by the 'hull-center' origin rule to
    centre a model on its orthographic silhouette rather than on the picked
    seat — parts whose body overhangs a small rear pad field have no useful
    pick-centroid, but their full footprint does. Monotone-chain hull +
    shoelace centroid, so it stays dependency-free and headless-safe."""
    pts = np.asarray(uv, dtype=np.float64)
    if len(pts) < 3:
        return pts.mean(axis=0) if len(pts) else np.zeros(2)
    order = np.lexsort((pts[:, 1], pts[:, 0]))
    pts = pts[order]

    def _half(points):
        chain: list = []
        for p in points:
            while len(chain) >= 2:
                a, b = chain[-2], chain[-1]
                if (b[0] - a[0]) * (p[1] - a[1]) - (b[1] - a[1]) * (p[0] - a[0]) <= 0:
                    chain.pop()
                else:
                    break
            chain.append(p)
        return chain

    lower = _half(pts)
    upper = _half(pts[::-1])
    hull = np.asarray(lower[:-1] + upper[:-1], dtype=np.float64)
    if len(hull) < 3:
        return pts.mean(axis=0)
    x, y = hull[:, 0], hull[:, 1]
    x1, y1 = np.roll(x, -1), np.roll(y, -1)
    cross = x * y1 - x1 * y
    area = 0.5 * float(cross.sum())
    if abs(area) < _EPS:  # collinear hull -> fall back to the vertex mean
        return hull.mean(axis=0)
    cx = float(((x + x1) * cross).sum()) / (6.0 * area)
    cy = float(((y + y1) * cross).sum()) / (6.0 * area)
    return np.array([cx, cy])


def _fit_plane(points) -> tuple[np.ndarray, np.ndarray]:
    """Best-fit plane through 3+ points (SVD); exact for 3. Returns
    (centroid-on-plane, unit normal)."""
    pts = np.asarray(points, dtype=np.float64)
    centroid = pts.mean(axis=0)
    _u, s, vt = np.linalg.svd(pts - centroid)
    if len(pts) > 2 and s[1] < _EPS:
        raise ValueError("the plane picks are collinear — pick again")
    normal = vt[-1]
    length = float(np.linalg.norm(normal))
    if length < _EPS:
        raise ValueError("could not fit a plane through the picks")
    return centroid, normal / length


def _in_plane_axis(points, normal) -> np.ndarray:
    """Rectangle orientation from the GEOMETRY, not the pick order: try the
    direction of every pick pair and keep the one whose in-plane bounding
    rectangle has minimum area — with two parallel pick pairs (a pad field's
    corners) the rectangle aligns to those parallel lines."""
    pts = [np.asarray(p, dtype=np.float64) for p in points]
    candidates = []
    for i in range(len(pts)):
        for j in range(i + 1, len(pts)):
            direction = pts[j] - pts[i]
            in_plane = direction - normal * float(direction @ normal)
            length = float(np.linalg.norm(in_plane))
            if length > _EPS:
                candidates.append(in_plane / length)
    if not candidates:
        raise ValueError("cannot derive an in-plane axis from the picks")
    best, best_area = candidates[0], np.inf
    base = pts[0]
    for axis in candidates:
        v_axis = np.cross(normal, axis)
        uv = np.array([[(p - base) @ axis, (p - base) @ v_axis] for p in pts])
        span = uv.max(axis=0) - uv.min(axis=0)
        area = float(max(span[0], _EPS) * max(span[1], _EPS))
        if area < best_area - 1e-12:
            best, best_area = axis, area
    return best


def compute_zsit_frame(
    plane_points, above, *, origin_rule: str = "rect-center", flip_z: bool = False,
    model_points=None,
) -> tuple[np.ndarray, np.ndarray, np.ndarray, np.ndarray]:
    """Return (origin, x_dir, y_dir, z_dir) of the picked seating frame.
    plane_points is 3 OR 4 picks — with 4, the best-fit plane through all of
    them is used and the rectangle bounds every pick. z points toward
    `above` (flipped by flip_z).

    `model_points` (Nx3, the whole model's mesh vertices) is required only for
    origin_rule='hull-center', which sets the in-plane origin to the geometric
    centre of the model's orthographic convex hull projected onto the plane —
    so the model centres on its real footprint, not the picked seat."""
    pts = [np.asarray(p, dtype=np.float64) for p in plane_points]
    if len(pts) < 3:
        raise ValueError("need at least 3 plane picks")
    above = np.asarray(above, dtype=np.float64)
    centroid, normal = _fit_plane(pts)
    x_axis = _in_plane_axis(pts, normal)
    y_axis = np.cross(normal, x_axis)

    if origin_rule == "first-pick":
        offset = pts[0] - centroid
        origin = pts[0] - normal * float(offset @ normal)  # projected to plane
    elif origin_rule == "rect-center":
        uv = np.array([[(p - centroid) @ x_axis, (p - centroid) @ y_axis]
                       for p in pts])
        center_uv = (uv.min(axis=0) + uv.max(axis=0)) * 0.5
        origin = centroid + center_uv[0] * x_axis + center_uv[1] * y_axis
    elif origin_rule == "hull-center":
        if model_points is None or len(model_points) < 3:
            origin = centroid  # no geometry to hull — fall back to pick centroid
        else:
            mp = np.asarray(model_points, dtype=np.float64)
            uv = np.column_stack([mp @ x_axis, mp @ y_axis])
            cu, cv = _hull_center_uv(uv)
            # reconstruct a point with that in-plane (u, v), kept on the plane
            origin = cu * x_axis + cv * y_axis + float(centroid @ normal) * normal
    else:  # centroid
        origin = centroid

    if float((above - origin) @ normal) < 0.0:
        normal = -normal
    if flip_z:
        normal = -normal
    x_dir = _in_plane_axis(pts, normal)
    y_dir = np.cross(normal, x_dir)
    return origin, x_dir, y_dir, normal


def compute_zsit_matrix(
    plane_points, above, *, origin_rule: str = "rect-center", flip_z: bool = False,
    model_points=None,
) -> np.ndarray:
    """4x4 transform taking current model coordinates into the user-defined
    frame: picked plane -> Z=0 with the origin on it, `above` side -> +Z."""
    origin, x_dir, y_dir, z_dir = compute_zsit_frame(
        plane_points, above, origin_rule=origin_rule, flip_z=flip_z,
        model_points=model_points,
    )
    rotation = np.vstack([x_dir, y_dir, z_dir])  # frame axes as rows
    matrix = np.eye(4, dtype=np.float64)
    matrix[:3, :3] = rotation
    matrix[:3, 3] = -rotation @ origin
    return matrix


def _level_groups(face_z, face_area, tol):
    """Group flat-face levels (within tol), area-weighted level per group,
    sorted bottom-up. Returns [(level, total_area, member indices)]."""
    order = np.argsort(face_z)
    groups = [[int(order[0])]]
    for k in range(1, len(order)):
        if face_z[order[k]] - face_z[order[k - 1]] <= tol:
            groups[-1].append(int(order[k]))
        else:
            groups.append([int(order[k])])
    info = []
    for group in groups:
        area = float(face_area[group].sum())
        level = float((face_z[group] * face_area[group]).sum() / max(area, 1e-12))
        info.append((level, area, group))
    info.sort(key=lambda t: t[0])
    return info


def _lowest_substantial_level(face_z, face_area, face_center, tol, span,
                              frac=0.3, gap_frac=0.12):
    """Seat level. Walk the flat downward-face levels bottom-up and seat on the
    first one that ISN'T a THR lead tip — a level is skipped only when it is
    thin AND a long way (a lead's length) below the next level. So SMT pads
    (thin but right under the body) stay the seat, while THR lead tips drop out
    and the seat lands on the body-rest plane. Returns (count, area, level,
    pad centers)."""
    info = _level_groups(face_z, face_area, tol)
    max_area = max(t[1] for t in info)
    for i, (level, area, group) in enumerate(info):
        thin = area < frac * max_area
        far = i + 1 < len(info) and (info[i + 1][0] - level) > gap_frac * max(span, 1e-9)
        if thin and far:
            continue  # a long thin lead below the body — not the seat
        return len(group), area, level, [face_center[j] for j in group]
    level, area, group = info[0]
    return len(group), area, level, [face_center[j] for j in group]


def _learned_seat_up(document):
    """The trained seat model's up vector (normalized), or None when no model
    is trained yet — so compute_auto_zsit falls back to the deterministic rule."""
    try:
        from ..seat_model import predict_up
        up = predict_up(document)
    except Exception:  # noqa: BLE001 — model/sklearn absent -> deterministic
        return None
    if up is None:
        return None
    up = np.asarray(up, dtype=np.float64)
    return up / float(np.linalg.norm(up))


def _learned_seat_level(document, up):
    """The trained level model's seat z (along up), or None — so the seat falls
    back to the deterministic substantial-level rule."""
    try:
        from ..seat_model import predict_level
        return predict_level(document, up)
    except Exception:  # noqa: BLE001
        return None


def compute_auto_zsit(
    document, *, tol: float = 0.08, normal_cos: float = 0.85
) -> tuple[np.ndarray, int, list[tuple[float, float, float]]] | None:
    """Best seating orientation with zero picks. Try candidate 'down'
    directions (the six axes plus the model's dominant face normals) and
    score each by how many DISTINCT faces lie flat at the very bottom —
    pads and feet are many small coplanar faces, while an upside-down
    package rests on one big one, so the face COUNT picks the right way up.
    The in-plane axes align to the pads' principal direction and the origin
    lands at their center. Returns (matrix, pad_count, pad_centroids) or
    None when there is nothing to seat."""
    chunks = []  # (centers, unit normals, areas, body_index, face_ids)
    all_points = []
    for body_index, body in enumerate(document.bodies):
        mesh = body.mesh
        if mesh is None or not len(mesh.tris):
            continue
        corners = mesh.points[mesh.tris]
        cross = np.cross(
            corners[:, 1] - corners[:, 0], corners[:, 2] - corners[:, 0]
        )
        lengths = np.linalg.norm(cross, axis=1)
        keep = lengths > 1e-12
        chunks.append((
            corners.mean(axis=1)[keep],
            cross[keep] / lengths[keep, None],
            0.5 * lengths[keep],
            body_index,
            mesh.tri_face_ids[keep],
        ))
        all_points.append(mesh.points)
    if not chunks:
        return None
    points = np.concatenate(all_points)

    candidates = [
        np.array(v, dtype=np.float64)
        for v in ((1, 0, 0), (-1, 0, 0), (0, 1, 0),
                  (0, -1, 0), (0, 0, 1), (0, 0, -1))
    ]
    normals_all = np.concatenate([n for _c, n, _a, _b, _f in chunks])
    areas_all = np.concatenate([a for _c, _n, a, _b, _f in chunks])
    for index in np.argsort(-areas_all)[:2000]:
        normal = normals_all[index]
        if all(float(normal @ c) < 0.985 for c in candidates):
            candidates.append(normal.copy())
        if len(candidates) >= 14:
            break

    def seat_at(up: np.ndarray):
        """Seat when `up` is +Z: cluster downward flat faces by height and seat
        on the LOWEST substantial level (see _lowest_substantial_level) — SMT
        pads when they're the lowest face, the body-rest plane for THR (thin
        lead tips below are skipped). Returns (count, area, level, pads)."""
        face_z, face_area, face_center = [], [], []
        for centers, normals, areas, body_index, face_ids in chunks:
            mask = normals @ up < -normal_cos
            if not mask.any():
                continue
            cz = centers[mask] @ up
            sub_centers = centers[mask]
            sub_areas = areas[mask]
            sub_faces = face_ids[mask]
            for face in np.unique(sub_faces):
                sub = sub_faces == face
                weight = sub_areas[sub]
                total = max(float(weight.sum()), 1e-12)
                face_z.append(float((cz[sub] * weight).sum() / total))
                face_area.append(total)
                face_center.append(
                    (sub_centers[sub] * weight[:, None]).sum(axis=0) / total
                )
        if not face_z:
            return None
        z = points @ up
        return _lowest_substantial_level(
            np.asarray(face_z), np.asarray(face_area), face_center, tol,
            float(z.max() - z.min()),
        )

    up = _learned_seat_up(document)
    if up is None or seat_at(up) is None:
        # no trained model (or it gives no seat) -> deterministic face-count rule
        best = None
        for down in candidates:
            cand = -down / float(np.linalg.norm(down))
            seat = seat_at(cand)
            if seat is None:
                continue
            count, area, _level, _pads = seat
            if best is None or (count, area) > best[0]:
                best = ((count, area), cand)
        if best is None or best[0][0] == 0:
            return None
        up = best[1]

    seat = seat_at(up)
    count, _area, level, pads = seat
    # The trained level ranker decides where z=0 goes (SMT pad / THR body-rest /
    # BGA ball tip); fall back to the deterministic level when no model.
    learned_level = _learned_seat_level(document, up)
    if learned_level is not None:
        level = learned_level
    pad_points = np.array(pads)
    helper = (
        np.array([1.0, 0.0, 0.0])
        if abs(float(up[0])) < 0.9 else np.array([0.0, 1.0, 0.0])
    )
    x_axis = np.cross(helper, up)
    x_axis /= float(np.linalg.norm(x_axis))
    y_axis = np.cross(up, x_axis)
    if len(pad_points) > 2:
        uv = np.column_stack([pad_points @ x_axis, pad_points @ y_axis])
        spread = uv - uv.mean(axis=0)
        _u, s, vt = np.linalg.svd(spread, full_matrices=False)
        if s[0] > 1e-9:
            x_axis, y_axis = (
                vt[0][0] * x_axis + vt[0][1] * y_axis,
                None,
            )
            x_axis /= float(np.linalg.norm(x_axis))
            y_axis = np.cross(up, x_axis)

    # Origin: centre of the whole model's FOOTPRINT (its XY bounding box in the
    # seat plane), at the seat level — matches the user's "centre the part"
    # convention better than the seat-pad centre (pads can sit to one side).
    foot = np.column_stack([points @ x_axis, points @ y_axis])
    center_uv = (foot.min(axis=0) + foot.max(axis=0)) * 0.5
    origin = (
        center_uv[0] * x_axis + center_uv[1] * y_axis + level * up
    )
    rotation = np.vstack([x_axis, y_axis, up])
    matrix = np.eye(4, dtype=np.float64)
    matrix[:3, :3] = rotation
    matrix[:3, 3] = -rotation @ origin

    # Mesh sag correction: the tessellation can sit up to one deflection
    # above the true B-rep surface — measure the EXACT bounds under this
    # transform (cheap: located shapes, no copies) and zero the true
    # minimum, but ONLY when nothing hangs below the seat (board-lock pegs
    # legitimately reach below Z=0, like a manual seat over peg holes).
    from OCP.Bnd import Bnd_Box
    from OCP.BRepBndLib import BRepBndLib
    from OCP.TopLoc import TopLoc_Location
    from OCP.gp import gp_Trsf

    trsf = gp_Trsf()
    trsf.SetValues(*(float(v) for v in matrix[:3, :].ravel()))
    box = Bnd_Box()
    for body in document.bodies:
        BRepBndLib.Add_s(body.solid.Moved(TopLoc_Location(trsf)), box)
    if not box.IsVoid():
        exact_zmin = float(box.Get()[2])
        if abs(exact_zmin) <= tol:
            matrix[2, 3] -= exact_zmin
    return matrix, count, [tuple(float(v) for v in p) for p in pad_points]


PLANE_COLOR = "#e8443a"   # plane picks: RED
Z_COLOR = "#1f5fd6"       # +Z pick + Z arrows: BLUE
AUTO_COLOR = "#1f9d3a"    # auto-detected seating pads: GREEN


def rect_corners(plane_points) -> np.ndarray:
    """The seating rectangle: a true rectangle in the (best-fit) plane — the
    in-plane bounding box of ALL plane picks (3 or 4), sides aligned with
    the first pick pair. Raises ValueError for degenerate picks."""
    pts = [np.asarray(p, dtype=np.float64) for p in plane_points]
    centroid, normal = _fit_plane(pts)
    x_axis = _in_plane_axis(pts, normal)
    y_axis = np.cross(normal, x_axis)
    uv = np.array([[(p - centroid) @ x_axis, (p - centroid) @ y_axis] for p in pts])
    u_min, v_min = uv.min(axis=0)
    u_max, v_max = uv.max(axis=0)
    return np.array(
        [
            centroid + u * x_axis + v * y_axis
            for u, v in ((u_min, v_min), (u_max, v_min), (u_max, v_max), (u_min, v_max))
        ]
    )


class ZSitTool(ToolMode):
    id = "zsit"
    title = "Redefine Z-Sit Plane"
    accent = "#1f5fd6"

    def __init__(self, ctx) -> None:
        super().__init__(ctx)
        self.points: list[tuple[float, float, float]] = []
        self._auto: tuple | None = None  # staged (matrix, count, centroids)

    # ----------------------------------------------------------------- UI

    def build_actions_widget(self) -> QWidget:
        widget = QWidget()
        layout = QVBoxLayout(widget)
        layout.setContentsMargins(8, 8, 8, 8)
        layout.addWidget(QLabel(
            "<b>Redefine Z-Sit plane</b><br>"
            "Click 3 points on the surface the part should sit on, "
            "then a 4th point on the side that becomes +Z."
        ))

        rule_row = QHBoxLayout()
        rule_row.addWidget(QLabel("Plane picks"))
        self.count_combo = QComboBox()
        self.count_combo.addItems(["3 points", "4 points"])
        self.count_combo.setToolTip(
            "4 points: best-fit plane through all four picks, rectangle "
            "bounds them all — for parts where 3 picks can't frame the pads."
        )
        self.count_combo.currentTextChanged.connect(lambda _t: self.reset_points())
        rule_row.addWidget(self.count_combo)
        rule_row.addWidget(QLabel("Origin"))
        self.origin_combo = QComboBox()
        self.origin_combo.addItems(ORIGIN_RULES)
        self.origin_combo.setToolTip(
            "Where X/Y=0 lands in the seating plane:\n"
            "• rect-center — centre of the picks' bounding rectangle\n"
            "• centroid — average of the picks\n"
            "• first-pick — the first picked point\n"
            "• hull-center — geometric centre of the model's orthographic\n"
            "  silhouette (convex hull) projected onto this plane. Use it\n"
            "  when the seat pads are off to one side and the body overhangs,\n"
            "  so there's no good pick-based centre."
        )
        rule_row.addWidget(self.origin_combo, 1)
        layout.addLayout(rule_row)

        self.snap_check = QCheckBox("Snap picks to mesh vertices")
        self.snap_check.setChecked(True)
        layout.addWidget(self.snap_check)
        self.flip_check = QCheckBox("Flip Z")
        layout.addWidget(self.flip_check)

        self.points_label = QLabel("No points picked.")
        self.points_label.setWordWrap(True)
        layout.addWidget(self.points_label)

        button_row = QHBoxLayout()
        self.reset_button = QPushButton("Reset")
        self.reset_button.clicked.connect(self.reset_points)
        self.undo_button = QPushButton("Undo pick")
        self.undo_button.clicked.connect(self.undo_pick)
        button_row.addWidget(self.reset_button)
        button_row.addWidget(self.undo_button)
        layout.addLayout(button_row)

        apply_row = QHBoxLayout()
        self.auto_button = QPushButton("Auto")
        from ..seat_model import fidelity_summary

        self.auto_button.setToolTip(
            "AUTO Z-Sit — learned seating, no clicks. Stages:\n"
            "  1 · orientation — a RandomForest ranks candidate up-vectors "
            "(which way is up)\n"
            "  2 · seat level — a RandomForest ranks candidate planes "
            "(where Z=0 sits)\n"
            "  3 · centering — footprint centre at the seat level "
            "(deterministic)\n"
            "(falls back to a face-count rule when no model is trained)\n"
            "\n"
            f"Trained fidelity (leave-one-out): {fidelity_summary()}\n"
            "\n"
            "The result is STAGED — press Apply to accept it, or just\n"
            "start picking points to do it manually instead."
        )
        self.auto_button.clicked.connect(self.run_auto)
        apply_row.addWidget(self.auto_button)
        self.apply_button = make_apply_button("Apply Z-Sit")
        self.apply_button.setEnabled(False)
        self.apply_button.clicked.connect(self.apply)
        apply_row.addWidget(self.apply_button, 1)
        layout.addLayout(apply_row)
        layout.addStretch(1)

        self.origin_combo.currentTextChanged.connect(lambda _t: self._refresh_preview())
        self.flip_check.stateChanged.connect(lambda _s: self._refresh_preview())
        return widget

    # ------------------------------------------------------------ lifecycle

    def enter(self) -> None:
        self.reset_points()
        self.ctx.scene.set_hover_callback(self._on_hover)

    def exit(self) -> None:
        scene = self.ctx.scene
        scene.set_hover_callback(None)
        scene.set_markers([], name="zsit-picks-plane")
        scene.set_markers([], name="zsit-picks-z")
        scene.set_markers([], name="zsit-ghost")
        scene.set_markers([], name="zsit-auto-pads")
        scene.show_quad([], name="zsit-rect")
        scene.show_arrows([], (0, 0, 1), scale=1.0, name="zsit-z-arrows")
        scene.clear_triad()
        self._auto = None

    def on_document_changed(self) -> None:
        self.reset_points()

    # -------------------------------------------------------------- picking

    def _plane_count(self) -> int:
        return 4 if self.count_combo.currentText().startswith("4") else 3

    def on_pick(self, pick) -> None:
        if self.ctx.document is None or len(self.points) >= self._plane_count() + 1:
            return
        if self._auto is not None:
            # manual picking overrides the staged auto result silently —
            # the user chose to redo it by hand
            self._auto = None
            self.ctx.scene.set_markers([], name="zsit-auto-pads")
        point = np.asarray(pick.world_point, dtype=np.float64)
        if self.snap_check.isChecked():
            point = self._snap_to_vertex(pick.body_index, point)
        self.points.append(tuple(float(v) for v in point))
        self._refresh_preview()

    def _model_points(self):
        """All mesh vertices, for the hull-center origin rule. None when there
        is no document/mesh yet."""
        document = self.ctx.document
        if document is None:
            return None
        arrays = [
            b.mesh.points
            for b in document.bodies
            if b.mesh is not None and len(b.mesh.points)
        ]
        return np.concatenate(arrays) if arrays else None

    def _snap_to_vertex(self, body_index: int, point: np.ndarray) -> np.ndarray:
        mesh = self.ctx.document.bodies[body_index].mesh
        if mesh is None or len(mesh.points) == 0:
            return point
        distances = np.linalg.norm(mesh.points - point, axis=1)
        nearest = int(np.argmin(distances))
        bounds = self.ctx.document.bounds()
        diag = float(np.linalg.norm([bounds[1] - bounds[0], bounds[3] - bounds[2], bounds[5] - bounds[4]]))
        if distances[nearest] <= diag * 0.02:
            return mesh.points[nearest].copy()
        return point

    # ------------------------------------------------------------- actions

    def reset_points(self) -> None:
        self.points = []
        self._auto = None
        if self.ctx.scene is not None:
            self.ctx.scene.set_markers([], name="zsit-auto-pads")
        self._refresh_preview()

    def run_auto(self) -> None:
        """Stage the automatic seating orientation: the workflow is not
        interrupted — Apply accepts it, picking points discards it."""
        document = self.ctx.document
        if document is None:
            return
        self.ctx.window.progress("Auto Z-Sit: scoring orientations", 0, 0)
        try:
            result = compute_auto_zsit(document)
        finally:
            self.ctx.window.progress_done()
        if result is None:
            self.status("Auto Z-Sit: no seating faces found — pick manually")
            return
        matrix, count, centroids = result
        self.points = []
        self._auto = result
        self.ctx.scene.set_markers(
            centroids, name="zsit-auto-pads", color=AUTO_COLOR, point_size=16.0
        )
        self.apply_button.setEnabled(True)
        self.status(
            f"Auto Z-Sit: {count} face(s) seat flat at Z=0 (green) — "
            f"press Apply to accept, or pick points to redo manually"
        )

    def undo_pick(self) -> None:
        if self.points:
            self.points.pop()
        self._refresh_preview()

    def _on_hover(self, pick) -> None:
        """Ghost point: preview the nearest surface point under the cursor
        (snapped like a real pick would be), tinted to the next pick's color."""
        scene = self.ctx.scene
        needed = self._plane_count()
        if pick is None or self.ctx.document is None or len(self.points) >= needed + 1:
            scene.set_markers([], name="zsit-ghost")
            return
        point = np.asarray(pick.world_point, dtype=np.float64)
        if self.snap_check.isChecked():
            point = self._snap_to_vertex(pick.body_index, point)
        color = PLANE_COLOR if len(self.points) < needed else Z_COLOR
        scene.set_markers(
            [tuple(float(v) for v in point)],
            name="zsit-ghost",
            color=color,
            point_size=20.0,
            opacity=0.5,
        )

    def _refresh_preview(self) -> None:
        scene = self.ctx.scene
        needed = self._plane_count()
        scene.set_markers(self.points[:needed], name="zsit-picks-plane",
                          color=PLANE_COLOR)
        scene.set_markers(self.points[needed:needed + 1], name="zsit-picks-z",
                          color=Z_COLOR)
        if len(self.points) >= needed + 1:
            scene.set_markers([], name="zsit-ghost")
        self.apply_button.setEnabled(
            len(self.points) >= needed + 1 or self._auto is not None
        )
        lines = [
            f"p{i + 1}: ({p[0]:.3f}, {p[1]:.3f}, {p[2]:.3f})"
            for i, p in enumerate(self.points)
        ]
        self.points_label.setText("\n".join(lines) if lines else "No points picked.")
        if len(self.points) < needed:
            prompt = f"pick point {len(self.points) + 1} of {needed} on the seating plane"
        elif len(self.points) == needed:
            prompt = "pick one more point on the +Z side of the plane"
        else:
            prompt = "ready — press Apply (or Reset to start over)"
        self.status(f"Z-Sit: {prompt}")

        corners = []
        if len(self.points) >= needed:
            try:
                corners = rect_corners(self.points[:needed])
            except ValueError:
                self.status("Z-Sit: picks are collinear — spread them out")
        scene.show_quad(corners, name="zsit-rect", color=PLANE_COLOR, opacity=0.25)

        scene.clear_triad()
        arrow_starts: list = []
        z_dir = (0.0, 0.0, 1.0)
        diag = 1.0
        if len(self.points) >= needed + 1 and self.ctx.document is not None:
            try:
                rule = self.origin_combo.currentText()
                origin, x_dir, y_dir, z_dir = compute_zsit_frame(
                    self.points[:needed], self.points[needed],
                    origin_rule=rule,
                    flip_z=self.flip_check.isChecked(),
                    model_points=self._model_points() if rule == "hull-center" else None,
                )
            except ValueError as exc:
                scene.show_arrows([], z_dir, scale=1.0, name="zsit-z-arrows")
                self.status(f"Z-Sit: {exc}")
                return
            bounds = self.ctx.document.bounds()
            diag = float(np.linalg.norm([bounds[1] - bounds[0], bounds[3] - bounds[2], bounds[5] - bounds[4]]))
            scene.show_triad(origin, x_dir, y_dir, z_dir, scale=max(diag * 0.25, 1.0e-3))
            # Blue Z arrows on all four corners of the seating rectangle
            # (the three picks plus the inferred corner).
            arrow_starts = list(corners)
        scene.show_arrows(
            arrow_starts,
            z_dir,
            scale=max(diag * 0.15, 1.0e-3),
            name="zsit-z-arrows",
            color=Z_COLOR,
        )

    def apply(self) -> None:
        needed = self._plane_count()
        if self.ctx.document is None:
            return
        if len(self.points) >= needed + 1:
            origin_rule = self.origin_combo.currentText()
            flip_z = self.flip_check.isChecked()
            try:
                matrix = compute_zsit_matrix(
                    self.points[:needed], self.points[needed],
                    origin_rule=origin_rule, flip_z=flip_z,
                    model_points=(
                        self._model_points()
                        if origin_rule == "hull-center" else None
                    ),
                )
            except ValueError as exc:
                self.status(f"Z-Sit: {exc}")
                return
            params = {"origin_rule": origin_rule, "flip_z": flip_z,
                      "plane_points": needed}
            inputs = {"points": [list(p) for p in self.points[:needed + 1]]}
        elif self._auto is not None:
            matrix, count, _centroids = self._auto
            params = {"action": "auto"}
            inputs = {"seat_faces": count}
        else:
            return
        self.ctx.document.apply_trsf(matrix)
        self.ctx.journal.record(
            tool=self.id, params=params, inputs=inputs,
            result={"matrix": np.asarray(matrix).tolist()},
        )
        self.points = []
        self._auto = None
        self.ctx.scene.set_markers([], name="zsit-auto-pads")
        self.ctx.window.document_mutated()
        bounds = self.ctx.document.bounds()
        self.status(
            f"Z-Sit applied — model now sits at z=[{bounds[4]:.4f}, {bounds[5]:.4f}]"
        )
