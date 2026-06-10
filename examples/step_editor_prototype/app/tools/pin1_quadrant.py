"""Redefine Pin 1 Quadrant: click pin 1 and the whole model swings about Z so
that pin lands in the +X +Y quadrant — always in exact 90-degree increments
so the model stays aligned with the principal axes. Arbitrary angles are a
separate, explicit manual rotation; Reset undoes everything this tool did.
The clicked point is kept as the pin-1 hint that Detect Pins uses to
propagate numbering."""

from __future__ import annotations

import math

import numpy as np
from PySide6.QtCore import Qt
from PySide6.QtWidgets import (
    QDoubleSpinBox,
    QHBoxLayout,
    QLabel,
    QPushButton,
    QVBoxLayout,
    QWidget,
)

from .base import ToolMode, make_apply_button


def compute_pin1_angle(x: float, y: float) -> float:
    """Rotation about +Z (radians, multiple of 90 deg) that brings (x, y)
    into the +X +Y quadrant."""
    if math.hypot(x, y) < 1.0e-12:
        return 0.0
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

    def __init__(self, ctx) -> None:
        super().__init__(ctx)
        self.cumulative_deg = 0.0
        self.pending_click: tuple[float, float, float] | None = None

    def build_actions_widget(self) -> QWidget:
        widget = QWidget()
        layout = QVBoxLayout(widget)
        layout.setContentsMargins(8, 8, 8, 8)
        layout.addWidget(QLabel(
            "<b>Redefine Pin 1 Quadrant</b><br>"
            "Click pin 1, then press Apply — the model swings in exact "
            "90&deg; steps about the Z axis through the origin defined by "
            "Z-Sit, so the clicked pin lands in the +X +Y quadrant."
        ))

        quick_row = QHBoxLayout()
        quick_row.addWidget(QLabel("Quick"))
        for label, degrees in (("+90°", 90.0), ("-90°", -90.0), ("180°", 180.0)):
            button = QPushButton(label)
            button.clicked.connect(
                lambda _c=False, d=degrees: self.rotate_manual(d)
            )
            quick_row.addWidget(button)
        layout.addLayout(quick_row)

        manual_row = QHBoxLayout()
        manual_row.addWidget(QLabel("Manual"))
        self.angle_spin = QDoubleSpinBox()
        self.angle_spin.setRange(-360.0, 360.0)
        self.angle_spin.setDecimals(2)
        self.angle_spin.setSuffix(" °")
        self.angle_spin.setSingleStep(5.0)
        self.angle_spin.setToolTip(
            "Arbitrary rotation about Z (CCW positive). Use only when the "
            "model genuinely needs a non-90° swing."
        )
        manual_row.addWidget(self.angle_spin, 1)
        rotate_button = QPushButton("Rotate")
        rotate_button.clicked.connect(
            lambda: self.rotate_manual(float(self.angle_spin.value()))
        )
        manual_row.addWidget(rotate_button)
        layout.addLayout(manual_row)

        self.reset_button = QPushButton("Reset rotation")
        self.reset_button.setToolTip("Undo every rotation this tool applied")
        self.reset_button.clicked.connect(self.reset_rotation)
        layout.addWidget(self.reset_button)

        self.apply_button = make_apply_button("Apply Pin 1 Swing")
        self.apply_button.setEnabled(False)
        self.apply_button.clicked.connect(self.apply)
        layout.addWidget(self.apply_button)

        self.result_label = QLabel("No rotation applied.")
        self.result_label.setWordWrap(True)
        layout.addWidget(self.result_label)
        layout.addStretch(1)
        return widget

    def enter(self) -> None:
        self.status("Pin 1 Quadrant: click pin 1, then press Apply (90° swings)")

    def exit(self) -> None:
        self.pending_click = None
        self.ctx.scene.set_markers([])

    def on_document_changed(self) -> None:
        self.cumulative_deg = 0.0
        self.pending_click = None
        if self._actions_widget is not None:
            self.apply_button.setEnabled(False)
            self.result_label.setText("No rotation applied.")

    # -------------------------------------------------------------- actions

    def on_pick(self, pick) -> None:
        if self.ctx.document is None:
            return
        self.pending_click = pick.world_point
        x, y, _z = pick.world_point
        angle = math.degrees(compute_pin1_angle(x, y))
        self.ctx.scene.set_markers([pick.world_point], color="#ffb000", point_size=20.0)
        self.apply_button.setEnabled(True)
        self.result_label.setText(
            f"pin 1 candidate @ ({x:.3f}, {y:.3f})\npending swing {angle:+.0f}° — press Apply"
        )
        self.status(f"Pin 1 Quadrant: pending swing {angle:+.0f}° — press Apply")

    def apply(self) -> None:
        if self.ctx.document is None or self.pending_click is None:
            return
        x, y, z = self.pending_click
        self.pending_click = None
        self.apply_button.setEnabled(False)
        angle = compute_pin1_angle(x, y)
        hint = rotation_z_matrix(angle) @ np.array([x, y, z, 1.0])
        hint_point = tuple(float(v) for v in hint[:3])
        self._apply(
            angle,
            action="pin1_click",
            inputs={"clicked_point": [x, y, z]},
            pin1_hint=hint_point,
        )
        self.ctx.scene.set_markers([hint_point], color="#ffb000", point_size=20.0)

    def rotate_manual(self, degrees: float) -> None:
        if self.ctx.document is None or abs(degrees) < 1.0e-12:
            return
        self._apply(math.radians(degrees), action="manual", inputs={"degrees": degrees})

    def reset_rotation(self) -> None:
        if self.ctx.document is None:
            return
        if abs(self.cumulative_deg) < 1.0e-12:
            self.status("Pin 1 Quadrant: nothing to reset")
            return
        self._apply(
            math.radians(-self.cumulative_deg),
            action="reset",
            inputs={"undone_deg": self.cumulative_deg},
        )
        self.cumulative_deg = 0.0
        self.result_label.setText("Rotation reset (back to 0°).")

    def _apply(self, angle_rad: float, *, action: str, inputs: dict, pin1_hint=None) -> None:
        matrix = rotation_z_matrix(angle_rad)
        self.ctx.document.apply_trsf(matrix)
        degrees = math.degrees(angle_rad)
        self.cumulative_deg = (self.cumulative_deg + degrees) % 360.0

        # Keep the pin-1 hint in sync with the model.
        window = self.ctx.window
        if pin1_hint is not None:
            window.pin1_hint = pin1_hint
        elif window.pin1_hint is not None:
            rotated = matrix @ np.array([*window.pin1_hint, 1.0])
            window.pin1_hint = tuple(float(v) for v in rotated[:3])

        self.ctx.journal.record(
            tool=self.id,
            params={"action": action},
            inputs=inputs,
            result={
                "angle_deg": degrees,
                "cumulative_deg": self.cumulative_deg,
                "pin1_hint": list(window.pin1_hint) if window.pin1_hint else None,
            },
        )
        window.document_mutated()
        text = f"swing {degrees:+.2f}° ({action})\ncumulative {self.cumulative_deg:.2f}°"
        if window.pin1_hint:
            hx, hy, hz = window.pin1_hint
            text += f"\npin 1 @ ({hx:.3f}, {hy:.3f}, {hz:.3f})"
        self.result_label.setText(text)
        self.status(f"Pin 1 Quadrant: {text.splitlines()[0]}")
