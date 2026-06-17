"""Redefine Front (Step 2): fix the in-plane rotation about +Z so the part
faces a canonical front. Z-Sit already stood it up, centred, and seated it; the
only freedom left is the spin about Z. The user gives two constraints:

  1. a FRONT point that must end up in the −Y half-plane, and
  2. a LINE that must become parallel to the X axis.

The line fixes the rotation to within 180°; the front point picks which of the
two flips. The model then rotates about its centre (the Z-Sit origin) to satisfy
both. These hand orientations are the ground truth for the per-package-type
front model (SISO pins −X→+X, SOT pins on ±X sides, QFN pin-1 indicator in
+X+Y — see DESIGN_INTENT.md).
"""

from __future__ import annotations

import math

import numpy as np
from PySide6.QtWidgets import (
    QHBoxLayout,
    QLabel,
    QPushButton,
    QVBoxLayout,
    QWidget,
)

from .base import ToolMode, make_apply_button
from .pin1_quadrant import rotation_z_matrix

# Axis-consistent colours (X=red, Y=green, Z=blue): the front point relates to
# the Y axis (it goes to −Y) so it's GREEN; the line relates to the X axis (it
# aligns to X) so it's RED — same scheme as Z-Sit (red plane, blue Z).
FRONT_COLOR = "#1f9d3a"   # the front point (→ −Y, the Y axis): GREEN
LINE_COLOR = "#e8443a"    # the alignment line (→ X axis): RED


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
    title = "Redefine Front"
    accent = "#b05c12"

    def __init__(self, ctx) -> None:
        super().__init__(ctx)
        self.cumulative_deg = 0.0
        self._front_point: tuple[float, float, float] | None = None
        self._line_pts: list[tuple[float, float, float]] = []

    def build_actions_widget(self) -> QWidget:
        widget = QWidget()
        layout = QVBoxLayout(widget)
        layout.setContentsMargins(8, 8, 8, 8)
        layout.addWidget(QLabel(
            "<b>Redefine Front</b><br>"
            "1. Click the <span style='color:#1f9d3a'><b>FRONT</b></span> point "
            "(green = Y) — it ends up in the −Y half-plane.<br>"
            "2. Click an existing straight <span style='color:#e8443a'><b>EDGE</b></span> "
            "(red = X) — that line aligns to the X axis.<br>"
            "Then Apply — the model spins about its centre so the line lies "
            "along X and the front faces −Y."
        ))

        self.pick_label = QLabel("Click the FRONT point first.")
        self.pick_label.setWordWrap(True)
        layout.addWidget(self.pick_label)

        btn_row = QHBoxLayout()
        clear_button = QPushButton("Clear Picks")
        clear_button.setToolTip("Start the front/line picks over")
        clear_button.clicked.connect(self.clear_picks)
        btn_row.addWidget(clear_button)
        for label, degrees in (("+90°", 90.0), ("-90°", -90.0), ("180°", 180.0)):
            b = QPushButton(label)
            b.clicked.connect(lambda _c=False, d=degrees: self.rotate_manual(d))
            btn_row.addWidget(b)
        layout.addLayout(btn_row)

        self.reset_button = QPushButton("Reset rotation")
        self.reset_button.setToolTip("Undo every rotation this tool applied")
        self.reset_button.clicked.connect(self.reset_rotation)
        layout.addWidget(self.reset_button)

        apply_row = QHBoxLayout()
        self.auto_button = QPushButton("Auto Front")
        self.auto_button.setToolTip(
            "AUTO Front — compute and apply the in-plane spin about Z with no "
            "picks. Aligns the part's dominant footprint axis to X and drops "
            "the bulkier half to −Y (the same front convention as a manual "
            "pick). Prefers a trained front model when one exists.\n"
            "Rate the result — a wrong flip is one 180° press away."
        )
        self.auto_button.clicked.connect(self.run_auto)
        apply_row.addWidget(self.auto_button)
        self.apply_button = make_apply_button("Apply Front")
        self.apply_button.setEnabled(False)
        self.apply_button.clicked.connect(self.apply)
        apply_row.addWidget(self.apply_button, 1)
        layout.addLayout(apply_row)

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
        to that pick's colour (green FRONT, then red LINE) — so the user sees
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
        edge = self.ctx.document.nearest_linear_edge(
            pick.body_index, np.asarray(pick.world_point, dtype=float))
        if edge is None:
            scene.show_segment(None, None, name="front-ghost")
        else:
            scene.show_segment(edge[0], edge[1], name="front-ghost",
                               color=LINE_COLOR, width=5.0, opacity=0.6)

    def clear_picks(self) -> None:
        self._front_point = None
        self._line_pts = []
        self._clear_markers()
        if self._actions_widget is not None:
            self.apply_button.setEnabled(False)
            self.pick_label.setText("Click the FRONT point first.")

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
            self.pick_label.setText("FRONT set. Now click a straight EDGE for the line.")
            self.status("Redefine Front: click an existing straight edge to align to X")
        elif len(self._line_pts) < 2:
            edge = self.ctx.document.nearest_linear_edge(
                pick.body_index, np.asarray(pick.world_point, dtype=float))
            if edge is None:
                self.status("Redefine Front: no straight edge here — click on a straight edge")
                return
            self._line_pts = [edge[0], edge[1]]
            self.ctx.scene.show_segment(edge[0], edge[1], name="front-line",
                                        color=LINE_COLOR, width=7.0)
            self._preview()
        else:
            self.clear_picks()
            self.on_pick(pick)   # extra click restarts

    def _preview(self) -> None:
        angle = math.degrees(
            compute_front_face_angle(self._line_pts[0], self._line_pts[1],
                                     self._front_point)
        )
        self.apply_button.setEnabled(True)
        self.pick_label.setText(
            f"Ready — pending spin {angle:+.2f}° about Z. Apply, or Clear Picks."
        )
        self.status(f"Redefine Front: pending spin {angle:+.2f}° — press Apply")

    # --------------------------------------------------------------- apply

    def apply(self) -> None:
        if (self.ctx.document is None or self._front_point is None
                or len(self._line_pts) < 2):
            return
        angle = compute_front_face_angle(self._line_pts[0], self._line_pts[1],
                                         self._front_point)
        inputs = {
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
            f"rate it, or press 180° to flip the front"
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
