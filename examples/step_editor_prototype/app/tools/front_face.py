"""Redefine Front (step 5): fix the in-plane rotation about +Z so the part
faces a canonical front. Z-Sit already stood it up, centred, and seated it; the
only freedom left is the spin about Z. Two ways to give it:

  FRONT + LINE   a FRONT point that must end up in the −Y half-plane, then a
                 LINE that must become parallel to the X axis.
  PIN 1 + LINE   the IPC way: click pin 1 (it goes to the upper left, IPC-7351
                 zero orientation), then the LINE along X.

The line fixes the rotation to within 180°; the first point picks which of the
two flips. The line snaps to the nearest straight, roughly horizontal edge on
any body; a part with none takes two clicked points instead. The model then
rotates about its centre (the Z-Sit origin). These hand orientations are the ground truth for the per-package-type
front model (SISO pins −X→+X, SOT pins on ±X sides, QFN pin-1 indicator in
+X+Y — see DESIGN_INTENT.md).
"""

from __future__ import annotations

import math

import numpy as np
from PySide6.QtWidgets import (
    QDoubleSpinBox,
    QHBoxLayout,
    QLabel,
    QPushButton,
    QVBoxLayout,
    QWidget,
)

from .base import ToolMode, make_apply_button
from .pin1_quadrant import rotation_z_matrix

# Axis-consistent colors (X=red, Y=green, Z=blue): the front point relates to
# the Y axis (it goes to −Y) so it's GREEN; the line relates to the X axis (it
# aligns to X) so it's RED — same scheme as Z-Sit (red plane, blue Z).
FRONT_COLOR = "#1f9d3a"   # the front point (→ −Y, the Y axis): GREEN
LINE_COLOR = "#e8443a"    # the alignment line (→ X axis): RED


def compute_pin1_angle(line_p1, line_p2, pin1) -> float:
    """Rotation about +Z (radians) that makes the line parallel to X and puts
    pin 1 at the upper left (IPC zero orientation: left of centre, top half;
    for a 2-pin part simply the -X end)."""
    dx, dy = line_p2[0] - line_p1[0], line_p2[1] - line_p1[1]
    if math.hypot(dx, dy) < 1.0e-12:
        return 0.0
    base = -math.atan2(dy, dx)
    px, py = float(pin1[0]), float(pin1[1])

    def score(theta):
        x = px * math.cos(theta) - py * math.sin(theta)
        y = px * math.sin(theta) + py * math.cos(theta)
        return -x + y

    theta = max((base, base + math.pi), key=score)
    return (theta + math.pi) % (2.0 * math.pi) - math.pi


def compute_front_face_angle(line_p1, line_p2, front_point) -> float:
    """Rotation about +Z (radians) that makes the line p1→p2 parallel to the X
    axis AND puts the front point in the −Y half-plane. Rotating a point (x, y)
    about the origin by θ sends y → x·sinθ + y·cosθ."""
    dx = line_p2[0] - line_p1[0]
    dy = line_p2[1] - line_p1[1]
    if math.hypot(dx, dy) < 1.0e-12:
        return 0.0
    theta = -math.atan2(dy, dx)            # bring the line onto +X
    fx, fy = float(front_point[0]), float(front_point[1])
    for cand in (theta, theta + math.pi):  # both align the line to the X axis
        if fx * math.sin(cand) + fy * math.cos(cand) < -1.0e-9:
            theta = cand                    # this flip drops the front into −Y
            break
    # normalise to (−π, π]
    return (theta + math.pi) % (2.0 * math.pi) - math.pi


class FrontFaceTool(ToolMode):
    id = "front"
    title = "5 Redefine Front"
    accent = "#b05c12"

    def __init__(self, ctx) -> None:
        super().__init__(ctx)
        self.cumulative_deg = 0.0
        self._front_point: tuple[float, float, float] | None = None
        self._line_pts: list[tuple[float, float, float]] = []
        self.mode = "front"  # "front" (front point + line) | "pin1" (IPC pin 1 + line)

    def build_actions_widget(self) -> QWidget:
        widget = QWidget()
        layout = QVBoxLayout(widget)
        layout.setContentsMargins(8, 8, 8, 8)
        mode_row = QHBoxLayout()
        toggle_style = ("QPushButton {padding: 6px; font-weight: 700;}"
                        "QPushButton:checked {background-color: #b05c12; color: #ffffff;}")
        self.front_mode_button = QPushButton("Front point + X line")
        self.front_mode_button.setToolTip("Click the point that faces front (ends up toward -Y), "
                                          "then a straight edge that should run along X")
        self.pin1_mode_button = QPushButton("IPC pin 1 + X line")
        self.pin1_mode_button.setToolTip("Click pin 1 (it goes to the upper left, IPC-7351 zero "
                                         "orientation), then a straight edge that should run along X")
        for button, mode in ((self.front_mode_button, "front"), (self.pin1_mode_button, "pin1")):
            button.setCheckable(True)
            button.setStyleSheet(toggle_style)
            button.clicked.connect(lambda _c=False, m=mode: self.set_mode(m))
            mode_row.addWidget(button)
        self.front_mode_button.setChecked(True)
        layout.addLayout(mode_row)

        self.pick_label = QLabel("Click the FRONT point first.")
        self.pick_label.setWordWrap(True)
        layout.addWidget(self.pick_label)

        rotate_row = QHBoxLayout()
        rotate_row.addWidget(QLabel("Rotate by"))
        self.rotation_spin = QDoubleSpinBox()
        self.rotation_spin.setRange(-360.0, 360.0)
        self.rotation_spin.setDecimals(2)
        self.rotation_spin.setSuffix(" °")
        self.rotation_spin.setToolTip("A custom spin about Z, applied with Rotate")
        rotate_row.addWidget(self.rotation_spin, 1)
        rotate_button = QPushButton("Rotate")
        rotate_button.setToolTip("Spin the model about Z by the angle on the left")
        rotate_button.clicked.connect(lambda: self.rotate_manual(self.rotation_spin.value()))
        rotate_row.addWidget(rotate_button)
        layout.addLayout(rotate_row)

        self.reset_button = QPushButton("Reset rotation")
        self.reset_button.setToolTip("Undo every rotation this tool applied")
        self.reset_button.clicked.connect(self.reset_rotation)
        layout.addWidget(self.reset_button)

        self.auto_button = QPushButton("Auto Front")
        self.auto_button.setToolTip("AUTO Front: compute and apply the in-plane spin about Z with no picks.")
        self.auto_button.clicked.connect(self.run_auto)
        layout.addWidget(self.auto_button)
        self.apply_button = make_apply_button("Apply Front")
        self.apply_button.setEnabled(False)
        self.apply_button.clicked.connect(self.apply)
        layout.addWidget(self.apply_button)

        self.result_label = QLabel("No rotation applied.")
        self.result_label.setWordWrap(True)
        layout.addWidget(self.result_label)
        layout.addStretch(1)
        return widget

    # ------------------------------------------------------------- lifecycle

    def enter(self) -> None:
        self.status("Redefine Front: click the FRONT point (→ −Y), then a "
                    "straight edge to align to X")
        self.ctx.scene.set_hover_callback(self._on_hover)

    def exit(self) -> None:
        self.ctx.scene.set_hover_callback(None)
        self._clear_markers()

    def on_document_changed(self) -> None:
        self.cumulative_deg = 0.0
        self.clear_picks()

    # ------------------------------------------------------------- picking

    def _clear_markers(self) -> None:
        self.ctx.scene.set_markers([], name="front-point")
        self.ctx.scene.set_markers([], name="front-line")
        self.ctx.scene.set_markers([], name="front-ghost")

    def _on_hover(self, pick) -> None:
        """Ghost preview of the next pick at the snapped cursor vertex, tinted
        to that pick's color (green FRONT, then red LINE) — so the user sees
        exactly where a point will land before clicking."""
        scene = self.ctx.scene
        done = self._front_point is not None and len(self._line_pts) >= 2
        if pick is None or self.ctx.document is None or done:
            scene.set_markers([], name="front-ghost")
            return
        placing_front = self._front_point is None
        if placing_front:
            point = self._snap_to_vertex(
                pick.body_index, np.asarray(pick.world_point, dtype=float))
            scene.set_markers(
                [tuple(float(v) for v in point)], name="front-ghost",
                color=FRONT_COLOR, point_size=18.0, opacity=0.5, on_top=True)
            return
        # placing the line: ghost the straight EDGE nearest the cursor as a line
        edge = self._line_near(np.asarray(pick.world_point, dtype=float))
        if edge is None:
            scene.show_segment(None, None, name="front-ghost")
        else:
            scene.show_segment(edge[0], edge[1], name="front-ghost",
                               color=LINE_COLOR, width=5.0, opacity=0.6)

    def set_mode(self, mode: str) -> None:
        self.mode = mode
        self.front_mode_button.setChecked(mode == "front")
        self.pin1_mode_button.setChecked(mode == "pin1")
        self.clear_picks()

    def _first_name(self) -> str:
        return "PIN 1" if self.mode == "pin1" else "FRONT point"

    def _line_near(self, point):
        """The straight edge to align to X: the nearest one on ANY body whose
        run in XY is real (vertical edges say nothing about X)."""
        best, best_d = None, None
        for index in range(len(self.ctx.document.bodies)):
            edge = self.ctx.document.nearest_linear_edge(index, point)
            if edge is None:
                continue
            a, b = np.asarray(edge[0]), np.asarray(edge[1])
            if np.linalg.norm((b - a)[:2]) < 0.3 * max(np.linalg.norm(b - a), 1e-12):
                continue
            ab = b - a
            t = float(np.clip(np.dot(point - a, ab) / max(float(ab @ ab), 1e-12), 0.0, 1.0))
            d = float(np.linalg.norm(point - (a + t * ab)))
            if best_d is None or d < best_d:
                best, best_d = edge, d
        return best

    def staged_rows(self) -> list[tuple[str, str, str]]:
        def xyz(p):
            return "(" + ", ".join(f"{float(c):.3f}" for c in p) + ")"

        rows = []
        if self._front_point is not None:
            rows.append((self._first_name(), xyz(self._front_point), ""))
        for k, point in enumerate(self._line_pts):
            rows.append((f"X line {k + 1}", xyz(point), ""))
        if self._front_point is not None and len(self._line_pts) >= 2:
            rows.append(("spin about Z", f"{math.degrees(self._pending_angle()):+.2f}°", ""))
        return rows

    def deny(self) -> None:
        self.clear_picks()

    def _pending_angle(self) -> float:
        compute = compute_pin1_angle if self.mode == "pin1" else compute_front_face_angle
        return compute(self._line_pts[0], self._line_pts[1], self._front_point)

    def clear_picks(self) -> None:
        self._front_point = None
        self._line_pts = []
        self._clear_markers()
        if self._actions_widget is not None:
            self.apply_button.setEnabled(False)
            self.pick_label.setText(f"Click the {self._first_name()} first.")

    def _snap_to_vertex(self, body_index, point):
        """Snap a click to the nearest mesh vertex of the picked body, so the
        front point and line endpoints land on real geometry (a line through
        two vertices gives an exact, repeatable direction)."""
        doc = self.ctx.document
        if (doc is None or body_index is None or body_index < 0
                or body_index >= len(doc.bodies)):
            return point
        mesh = doc.bodies[body_index].mesh
        if mesh is None or not len(mesh.points):
            return point
        distances = np.linalg.norm(mesh.points - point, axis=1)
        return mesh.points[int(np.argmin(distances))]

    def on_pick(self, pick) -> None:
        if self.ctx.document is None:
            return
        snapped = self._snap_to_vertex(
            pick.body_index, np.asarray(pick.world_point, dtype=float))
        point = tuple(float(v) for v in snapped)
        if self._front_point is None:
            self._front_point = point
            self.ctx.scene.set_markers([point], color=FRONT_COLOR,
                                       point_size=20.0, name="front-point",
                                       on_top=True)
            self.pick_label.setText(f"{self._first_name()} set. Now click a straight edge for the X line.")
            self.status("Redefine Front: click an existing straight edge to align to X")
        elif len(self._line_pts) < 2:
            edge = self._line_near(np.asarray(pick.world_point, dtype=float)) if not self._line_pts else None
            if edge is None:
                # no usable straight edge: the line is two clicked points
                self._line_pts.append(point)
                if len(self._line_pts) < 2:
                    self.ctx.scene.set_markers(self._line_pts, color=LINE_COLOR, point_size=16.0,
                                               name="front-line", on_top=True)
                    self.pick_label.setText("No straight edge there: click a second point for the X line.")
                    return
                edge = (self._line_pts[0], self._line_pts[1])
            self._line_pts = [edge[0], edge[1]]
            self.ctx.scene.show_segment(edge[0], edge[1], name="front-line",
                                        color=LINE_COLOR, width=7.0)
            self._preview()
        else:
            self.clear_picks()
            self.on_pick(pick)   # extra click restarts

    def _preview(self) -> None:
        angle = math.degrees(self._pending_angle())
        self.apply_button.setEnabled(True)
        self.pick_label.setText(f"Ready: pending spin {angle:+.2f}° about Z. CONFIRM, or DENY.")
        self.status(f"Redefine Front: pending spin {angle:+.2f}°, press CONFIRM")

    # --------------------------------------------------------------- apply

    def apply(self) -> None:
        if (self.ctx.document is None or self._front_point is None
                or len(self._line_pts) < 2):
            return
        angle = self._pending_angle()
        inputs = {
            "mode": self.mode,
            "front_point": list(self._front_point),
            "line_p1": list(self._line_pts[0]),
            "line_p2": list(self._line_pts[1]),
        }
        self.clear_picks()
        self._apply(angle, action="front_constraints", inputs=inputs)

    def run_auto(self) -> None:
        """AUTO front: compute the in-plane spin with no picks and apply it
        immediately, recording the same replayable "front" op as a manual
        apply. Pin picks (if any) are cleared first, like a manual apply."""
        if self.ctx.document is None:
            return
        from ..auto import auto_front_angle

        angle = auto_front_angle(self.ctx.document)
        if angle is None:
            self.status("Auto Front: nothing to orient — pick the front manually")
            return
        self.clear_picks()
        self._apply(angle, action="auto", inputs={"source": "auto_front_angle"})
        self.status(
            f"Auto Front: spun {math.degrees(angle):+.2f}° about Z — "
            f"rate it, or rotate by 180° to flip the front"
        )

    def rotate_manual(self, degrees: float) -> None:
        if self.ctx.document is None or abs(degrees) < 1.0e-12:
            return
        self._apply(math.radians(degrees), action="manual",
                    inputs={"degrees": degrees})

    def reset_rotation(self) -> None:
        if self.ctx.document is None:
            return
        if abs(self.cumulative_deg) < 1.0e-12:
            self.status("Redefine Front: nothing to reset")
            return
        self._apply(math.radians(-self.cumulative_deg), action="reset",
                    inputs={"undone_deg": self.cumulative_deg})
        self.cumulative_deg = 0.0
        self.result_label.setText("Rotation reset (back to 0°).")

    def _apply(self, angle_rad: float, *, action: str, inputs: dict) -> None:
        matrix = rotation_z_matrix(angle_rad)
        self.ctx.document.apply_trsf(matrix)
        from ..pins import transform_pins
        transform_pins(self.ctx.window.pins.pins, matrix)  # pins spin with the model
        degrees = math.degrees(angle_rad)
        self.cumulative_deg = (self.cumulative_deg + degrees) % 360.0

        window = self.ctx.window
        if window.pin1_hint is not None:   # keep any pin-1 hint synced
            rotated = matrix @ np.array([*window.pin1_hint, 1.0])
            window.pin1_hint = tuple(float(v) for v in rotated[:3])

        self.ctx.journal.record(
            tool=self.id,
            params={"action": action},
            inputs=inputs,
            result={"angle_deg": degrees, "cumulative_deg": self.cumulative_deg},
        )
        window.document_mutated()
        text = (f"spin {degrees:+.2f}° ({action})\n"
                f"cumulative {self.cumulative_deg:.2f}°")
        self.result_label.setText(text)
        self.status(f"Redefine Front: {text.splitlines()[0]}")
