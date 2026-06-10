"""Redefine Colors: click a body, assign it a colour. Useful once a chip has
been dissected and new bodies (pins, pads) deserve colours the original model
never defined. Colours persist into the AP242 export via XCAF."""

from __future__ import annotations

from PySide6.QtGui import QColor
from PySide6.QtWidgets import (
    QColorDialog,
    QGridLayout,
    QLabel,
    QPushButton,
    QVBoxLayout,
    QWidget,
)

from .base import ToolMode

SWATCHES = [
    ("#c0c0c0", "tin"),
    ("#d4af37", "gold"),
    ("#b87333", "copper"),
    ("#202428", "epoxy"),
    ("#f5f5f0", "ceramic"),
    ("#2e7d32", "soldermask"),
    ("#8a1c1c", "marker red"),
    ("#1f5fd6", "marker blue"),
]


class ColorsTool(ToolMode):
    id = "colors"
    title = "Redefine Colors"
    accent = "#8a2b8a"

    def __init__(self, ctx) -> None:
        super().__init__(ctx)
        self.selected_body: int | None = None

    def build_actions_widget(self) -> QWidget:
        widget = QWidget()
        layout = QVBoxLayout(widget)
        layout.setContentsMargins(8, 8, 8, 8)
        layout.addWidget(QLabel(
            "<b>Redefine Colors</b><br>"
            "Click a body in the 3D view, then pick a colour."
        ))
        self.selection_label = QLabel("No body selected.")
        self.selection_label.setStyleSheet("font-family: Consolas, monospace;")
        layout.addWidget(self.selection_label)

        grid = QGridLayout()
        for index, (hex_color, label) in enumerate(SWATCHES):
            button = QPushButton(label)
            button.setStyleSheet(
                f"background: {hex_color}; color: "
                f"{'#ffffff' if QColor(hex_color).lightness() < 128 else '#101010'};"
            )
            button.clicked.connect(lambda _c=False, h=hex_color: self.apply_color(h))
            grid.addWidget(button, index // 2, index % 2)
        layout.addLayout(grid)

        custom_button = QPushButton("Custom colour ...")
        custom_button.clicked.connect(self._pick_custom)
        layout.addWidget(custom_button)
        reset_button = QPushButton("Reset to original")
        reset_button.clicked.connect(self._reset_original)
        layout.addWidget(reset_button)
        layout.addStretch(1)
        return widget

    def enter(self) -> None:
        self.status("Colors: click a body to select it")

    def exit(self) -> None:
        self.ctx.scene.clear_highlight()
        self.selected_body = None

    def on_document_changed(self) -> None:
        self.selected_body = None
        if self._actions_widget is not None:
            self.selection_label.setText("No body selected.")

    def on_pick(self, pick) -> None:
        self.selected_body = pick.body_index
        body = self.ctx.document.bodies[pick.body_index]
        current = body.color
        text = f"body {pick.body_index} '{body.name}'"
        if current:
            text += f"\ncolour ({current[0]:.2f}, {current[1]:.2f}, {current[2]:.2f})"
        self.selection_label.setText(text)
        self.ctx.scene.highlight_body(pick.body_index)
        self.status(f"Colors: selected {text.splitlines()[0]}")

    def _pick_custom(self) -> None:
        if self.selected_body is None:
            self.status("Colors: click a body first")
            return
        color = QColorDialog.getColor(parent=self.actions_widget())
        if color.isValid():
            self.apply_color(color.name())

    def _reset_original(self) -> None:
        if self.selected_body is None or self.ctx.document is None:
            return
        body = self.ctx.document.bodies[self.selected_body]
        body.color = body.original_color
        if body.mesh is not None:
            body.mesh.face_colors = {}
        self.ctx.window.document_mutated()
        self.status(f"Colors: body {self.selected_body} reset to original")

    def apply_color(self, hex_color: str) -> None:
        if self.selected_body is None or self.ctx.document is None:
            self.status("Colors: click a body first")
            return
        qcolor = QColor(hex_color)
        rgb = (qcolor.redF(), qcolor.greenF(), qcolor.blueF())
        body = self.ctx.document.bodies[self.selected_body]
        body.color = rgb
        if body.mesh is not None:
            body.mesh.face_colors = {}  # body colour overrides per-face colours
        self.ctx.scene.clear_highlight()
        self.ctx.scene.set_body_color(self.selected_body, rgb)
        self.ctx.journal.record(
            tool=self.id,
            params={},
            inputs={"body_index": self.selected_body},
            result={"color": hex_color},
        )
        self.status(f"Colors: body {self.selected_body} '{body.name}' -> {hex_color}")
