"""Redefine Pin 1 Quadrant: click pin 1 and the whole model swings about Z so
that pin lands in the +X +Y quadrant.

Default swing is 90-degree increments (keeps axis-aligned packages pristine);
the "exact-bisector" variant rotates the click onto the 45-degree line for
non-rectangular parts. The clicked point is kept as the pin-1 hint that Detect
Pins uses to propagate numbering."""

from __future__ import annotations

import math

import numpy as np
from PySide6.QtWidgets import QComboBox, QHBoxLayout, QLabel, QVBoxLayout, QWidget

from .base import ToolMode

SWING_MODES = ("quarter", "exact-bisector")


def compute_pin1_angle(x: float, y: float, *, mode: str = "quarter") -> float:
    """Rotation about +Z (radians) that brings (x, y) into the +X +Y quadrant."""
    if math.hypot(x, y) < 1.0e-12:
        return 0.0
    if mode == "exact-bisector":
        return math.radians(45.0) - math.atan2(y, x)
    # quarter: swing by the multiple of 90 deg that maps the quadrant to Q1
    if x >= 0.0 and y >= 0.0:
        return 0.0
    if x < 0.0 and y >= 0.0:
        return -0.5 * math.pi
    if x < 0.0 and y < 0.0:
        return math.pi
    return 0.5 * math.pi


def rotation_z_matrix(angle_rad: float) -> np.ndarray:
    c, s = math.cos(angle_rad), math.sin(angle_rad)
    matrix = np.eye(4, dtype=np.float64)
    matrix[:2, :2] = [[c, -s], [s, c]]
    return matrix


class Pin1QuadrantTool(ToolMode):
    id = "pin1"
    title = "Redefine Pin 1 Quadrant"
    accent = "#b05c12"

    def build_actions_widget(self) -> QWidget:
        widget = QWidget()
        layout = QVBoxLayout(widget)
        layout.setContentsMargins(8, 8, 8, 8)
        layout.addWidget(QLabel(
            "<b>Redefine Pin 1 Quadrant</b><br>"
            "Click pin 1 — the model swings about Z so that pin lands in "
            "the +X +Y quadrant. Run Z-Sit first so the origin is meaningful."
        ))
        mode_row = QHBoxLayout()
        mode_row.addWidget(QLabel("Swing"))
        self.mode_combo = QComboBox()
        self.mode_combo.addItems(SWING_MODES)
        self.mode_combo.setToolTip(
            "quarter: 90-degree increments (keeps packages axis-aligned)\n"
            "exact-bisector: rotate the click onto the 45-degree line"
        )
        mode_row.addWidget(self.mode_combo, 1)
        layout.addLayout(mode_row)

        self.result_label = QLabel("No pin clicked yet.")
        self.result_label.setStyleSheet("font-family: Consolas, monospace;")
        self.result_label.setWordWrap(True)
        layout.addWidget(self.result_label)
        layout.addStretch(1)
        return widget

    def enter(self) -> None:
        self.status("Pin 1 Quadrant: click pin 1 on the model")

    def exit(self) -> None:
        self.ctx.scene.set_markers([])

    def on_pick(self, pick) -> None:
        if self.ctx.document is None:
            return
        x, y, z = pick.world_point
        mode = self.mode_combo.currentText()
        angle = compute_pin1_angle(x, y, mode=mode)
        matrix = rotation_z_matrix(angle)
        self.ctx.document.apply_trsf(matrix)

        hint = matrix @ np.array([x, y, z, 1.0])
        hint_point = tuple(float(v) for v in hint[:3])
        self.ctx.window.pin1_hint = hint_point

        self.ctx.journal.record(
            tool=self.id,
            params={"mode": mode},
            inputs={"clicked_point": [x, y, z]},
            result={"angle_deg": math.degrees(angle), "pin1_hint": list(hint_point)},
        )
        self.ctx.window.document_mutated()
        self.ctx.scene.set_markers([hint_point], color="#ffb000", point_size=20.0)
        text = (
            f"swing {math.degrees(angle):+.1f} deg ({mode})\n"
            f"pin 1 now @ ({hint_point[0]:.3f}, {hint_point[1]:.3f}, {hint_point[2]:.3f})"
        )
        self.result_label.setText(text)
        self.status(f"Pin 1 Quadrant: {text.splitlines()[0]} — pin 1 hint stored")
