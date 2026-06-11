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

ORIGIN_RULES = ("rect-center", "centroid", "first-pick")
_EPS = 1.0e-9


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
    plane_points, above, *, origin_rule: str = "rect-center", flip_z: bool = False
) -> tuple[np.ndarray, np.ndarray, np.ndarray, np.ndarray]:
    """Return (origin, x_dir, y_dir, z_dir) of the picked seating frame.
    plane_points is 3 OR 4 picks — with 4, the best-fit plane through all of
    them is used and the rectangle bounds every pick. z points toward
    `above` (flipped by flip_z)."""
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
    plane_points, above, *, origin_rule: str = "rect-center", flip_z: bool = False
) -> np.ndarray:
    """4x4 transform taking current model coordinates into the user-defined
    frame: picked plane -> Z=0 with the origin on it, `above` side -> +Z."""
    origin, x_dir, y_dir, z_dir = compute_zsit_frame(
        plane_points, above, origin_rule=origin_rule, flip_z=flip_z
    )
    rotation = np.vstack([x_dir, y_dir, z_dir])  # frame axes as rows
    matrix = np.eye(4, dtype=np.float64)
    matrix[:3, :3] = rotation
    matrix[:3, 3] = -rotation @ origin
    return matrix


PLANE_COLOR = "#e8443a"   # plane picks: RED
Z_COLOR = "#1f5fd6"       # +Z pick + Z arrows: BLUE


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

        self.apply_button = make_apply_button("Apply Z-Sit")
        self.apply_button.setEnabled(False)
        self.apply_button.clicked.connect(self.apply)
        layout.addWidget(self.apply_button)
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
        scene.show_quad([], name="zsit-rect")
        scene.show_arrows([], (0, 0, 1), scale=1.0, name="zsit-z-arrows")
        scene.clear_triad()

    def on_document_changed(self) -> None:
        self.reset_points()

    # -------------------------------------------------------------- picking

    def _plane_count(self) -> int:
        return 4 if self.count_combo.currentText().startswith("4") else 3

    def on_pick(self, pick) -> None:
        if self.ctx.document is None or len(self.points) >= self._plane_count() + 1:
            return
        point = np.asarray(pick.world_point, dtype=np.float64)
        if self.snap_check.isChecked():
            point = self._snap_to_vertex(pick.body_index, point)
        self.points.append(tuple(float(v) for v in point))
        self._refresh_preview()

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
        self._refresh_preview()

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
        self.apply_button.setEnabled(len(self.points) >= needed + 1)
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
                origin, x_dir, y_dir, z_dir = compute_zsit_frame(
                    self.points[:needed], self.points[needed],
                    origin_rule=self.origin_combo.currentText(),
                    flip_z=self.flip_check.isChecked(),
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
        if self.ctx.document is None or len(self.points) < needed + 1:
            return
        origin_rule = self.origin_combo.currentText()
        flip_z = self.flip_check.isChecked()
        try:
            matrix = compute_zsit_matrix(
                self.points[:needed], self.points[needed],
                origin_rule=origin_rule, flip_z=flip_z,
            )
        except ValueError as exc:
            self.status(f"Z-Sit: {exc}")
            return
        self.ctx.document.apply_trsf(matrix)
        self.ctx.journal.record(
            tool=self.id,
            params={"origin_rule": origin_rule, "flip_z": flip_z,
                    "plane_points": needed},
            inputs={"points": [list(p) for p in self.points[:needed + 1]]},
            result={"matrix": matrix.tolist()},
        )
        self.points = []
        self.ctx.window.document_mutated()
        bounds = self.ctx.document.bounds()
        self.status(
            f"Z-Sit applied — model now sits at z=[{bounds[4]:.4f}, {bounds[5]:.4f}]"
        )
