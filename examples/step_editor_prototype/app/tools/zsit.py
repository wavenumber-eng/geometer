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

ORIGIN_RULES = ("centroid", "first-pick", "rect-center")
_EPS = 1.0e-9


def compute_zsit_frame(
    p1, p2, p3, p4, *, origin_rule: str = "centroid", flip_z: bool = False
) -> tuple[np.ndarray, np.ndarray, np.ndarray, np.ndarray]:
    """Return (origin, x_dir, y_dir, z_dir) of the picked seating frame, in
    current model coordinates. z points toward p4 (flipped by flip_z)."""
    a, b, c, above = (np.asarray(p, dtype=np.float64) for p in (p1, p2, p3, p4))

    normal = np.cross(b - a, c - a)
    length = float(np.linalg.norm(normal))
    if length < _EPS:
        raise ValueError("the three plane picks are collinear — pick again")
    normal /= length

    if origin_rule == "first-pick":
        origin = a
    elif origin_rule == "rect-center":
        # In-plane bounding-rectangle centre of the three picks.
        x_axis = _in_plane_axis(a, b, c, normal)
        y_axis = np.cross(normal, x_axis)
        uv = np.array([[(p - a) @ x_axis, (p - a) @ y_axis] for p in (a, b, c)])
        center_uv = (uv.min(axis=0) + uv.max(axis=0)) * 0.5
        origin = a + center_uv[0] * x_axis + center_uv[1] * y_axis
    else:  # centroid (default)
        origin = (a + b + c) / 3.0

    if float((above - origin) @ normal) < 0.0:
        normal = -normal
    if flip_z:
        normal = -normal

    x_dir = _in_plane_axis(a, b, c, normal)
    y_dir = np.cross(normal, x_dir)
    return origin, x_dir, y_dir, normal


def _in_plane_axis(a, b, c, normal) -> np.ndarray:
    for candidate in (b - a, c - a):
        in_plane = candidate - normal * float(candidate @ normal)
        length = float(np.linalg.norm(in_plane))
        if length > _EPS:
            return in_plane / length
    raise ValueError("cannot derive an in-plane axis from the picks")


def compute_zsit_matrix(
    p1, p2, p3, p4, *, origin_rule: str = "centroid", flip_z: bool = False
) -> np.ndarray:
    """4x4 transform taking current model coordinates into the user-defined
    frame: picked plane -> Z=0 with the origin on it, 4th pick side -> +Z."""
    origin, x_dir, y_dir, z_dir = compute_zsit_frame(
        p1, p2, p3, p4, origin_rule=origin_rule, flip_z=flip_z
    )
    rotation = np.vstack([x_dir, y_dir, z_dir])  # frame axes as rows
    matrix = np.eye(4, dtype=np.float64)
    matrix[:3, :3] = rotation
    matrix[:3, 3] = -rotation @ origin
    return matrix


PICK_PROMPTS = [
    "pick point 1 of 3 on the seating plane",
    "pick point 2 of 3 on the seating plane",
    "pick point 3 of 3 on the seating plane",
    "pick a 4th point on the +Z side of the plane",
    "ready — press Apply (or Reset to start over)",
]

PLANE_COLOR = "#e8443a"   # first three picks: RED
Z_COLOR = "#1f5fd6"       # 4th pick + Z arrows: BLUE


def rect_corners(p1, p2, p3) -> np.ndarray:
    """The seating rectangle: a true rectangle in the picked plane — the
    in-plane bounding box of the three picks, sides aligned with the p1->p2
    direction. Raises ValueError for collinear picks."""
    a, b, c = (np.asarray(p, dtype=np.float64) for p in (p1, p2, p3))
    normal = np.cross(b - a, c - a)
    length = float(np.linalg.norm(normal))
    if length < _EPS:
        raise ValueError("the three plane picks are collinear")
    normal /= length
    x_axis = _in_plane_axis(a, b, c, normal)
    y_axis = np.cross(normal, x_axis)
    uv = np.array([[(p - a) @ x_axis, (p - a) @ y_axis] for p in (a, b, c)])
    u_min, v_min = uv.min(axis=0)
    u_max, v_max = uv.max(axis=0)
    return np.array(
        [
            a + u * x_axis + v * y_axis
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
        rule_row.addWidget(QLabel("Origin rule"))
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

    def on_pick(self, pick) -> None:
        if self.ctx.document is None or len(self.points) >= 4:
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
        if pick is None or self.ctx.document is None or len(self.points) >= 4:
            scene.set_markers([], name="zsit-ghost")
            return
        point = np.asarray(pick.world_point, dtype=np.float64)
        if self.snap_check.isChecked():
            point = self._snap_to_vertex(pick.body_index, point)
        color = PLANE_COLOR if len(self.points) < 3 else Z_COLOR
        scene.set_markers(
            [tuple(float(v) for v in point)],
            name="zsit-ghost",
            color=color,
            point_size=20.0,
            opacity=0.5,
        )

    def _refresh_preview(self) -> None:
        scene = self.ctx.scene
        scene.set_markers(self.points[:3], name="zsit-picks-plane", color=PLANE_COLOR)
        scene.set_markers(self.points[3:4], name="zsit-picks-z", color=Z_COLOR)
        if len(self.points) >= 4:
            scene.set_markers([], name="zsit-ghost")
        self.apply_button.setEnabled(len(self.points) >= 4)
        lines = [
            f"p{i + 1}: ({p[0]:.3f}, {p[1]:.3f}, {p[2]:.3f})"
            for i, p in enumerate(self.points)
        ]
        self.points_label.setText("\n".join(lines) if lines else "No points picked.")
        self.status(f"Z-Sit: {PICK_PROMPTS[min(len(self.points), 4)]}")

        corners = []
        if len(self.points) >= 3:
            try:
                corners = rect_corners(*self.points[:3])
            except ValueError:
                self.status("Z-Sit: picks are collinear — pick a wider triangle")
        scene.show_quad(corners, name="zsit-rect", color=PLANE_COLOR, opacity=0.25)

        scene.clear_triad()
        arrow_starts: list = []
        z_dir = (0.0, 0.0, 1.0)
        diag = 1.0
        if len(self.points) >= 4 and self.ctx.document is not None:
            try:
                origin, x_dir, y_dir, z_dir = compute_zsit_frame(
                    *self.points[:4],
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
        if self.ctx.document is None or len(self.points) < 4:
            return
        origin_rule = self.origin_combo.currentText()
        flip_z = self.flip_check.isChecked()
        try:
            matrix = compute_zsit_matrix(
                *self.points[:4], origin_rule=origin_rule, flip_z=flip_z
            )
        except ValueError as exc:
            self.status(f"Z-Sit: {exc}")
            return
        self.ctx.document.apply_trsf(matrix)
        self.ctx.journal.record(
            tool=self.id,
            params={"origin_rule": origin_rule, "flip_z": flip_z},
            inputs={"points": [list(p) for p in self.points[:4]]},
            result={"matrix": matrix.tolist()},
        )
        self.points = []
        self.ctx.window.document_mutated()
        bounds = self.ctx.document.bounds()
        self.status(
            f"Z-Sit applied — model now sits at z=[{bounds[4]:.4f}, {bounds[5]:.4f}]"
        )
