"""Redefine Colors: pick a colour, select bodies or faces (Shift+click to
build up a selection), then Apply — or switch on the paintbrush and every
click paints immediately. Body and face colours persist into the AP242
export via XCAF (faces as sub-shape colours)."""

from __future__ import annotations

from PySide6.QtGui import QColor
from PySide6.QtWidgets import (
    QCheckBox,
    QColorDialog,
    QComboBox,
    QGridLayout,
    QHBoxLayout,
    QLabel,
    QPushButton,
    QVBoxLayout,
    QWidget,
)

from .base import ToolMode, make_apply_button

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
        self.current_color = "#d4af37"
        self.selection: set = set()  # body indices, or (body, face) pairs

    # ------------------------------------------------------------------- UI

    def build_actions_widget(self) -> QWidget:
        widget = QWidget()
        layout = QVBoxLayout(widget)
        layout.setContentsMargins(8, 8, 8, 8)
        layout.addWidget(QLabel(
            "<b>Redefine Colors</b><br>"
            "Pick a colour, click to select (Shift+click adds more), then "
            "Apply — or enable the paintbrush to paint on every click."
        ))

        target_row = QHBoxLayout()
        target_row.addWidget(QLabel("Target"))
        self.target_combo = QComboBox()
        self.target_combo.addItems(["Bodies", "Faces"])
        self.target_combo.currentTextChanged.connect(lambda _t: self.clear_selection())
        target_row.addWidget(self.target_combo, 1)
        target_row.addWidget(QLabel("Color"))
        self.color_chip = QLabel()
        self.color_chip.setFixedSize(40, 20)
        target_row.addWidget(self.color_chip)
        layout.addLayout(target_row)

        self.brush_check = QCheckBox("Paintbrush — clicking paints immediately")
        layout.addWidget(self.brush_check)

        grid = QGridLayout()
        for index, (hex_color, label) in enumerate(SWATCHES):
            button = QPushButton(label)
            button.setStyleSheet(
                f"background: {hex_color}; color: "
                f"{'#ffffff' if QColor(hex_color).lightness() < 128 else '#101010'};"
            )
            button.clicked.connect(lambda _c=False, h=hex_color: self.set_color(h))
            grid.addWidget(button, index // 2, index % 2)
        layout.addLayout(grid)

        custom_button = QPushButton("Custom colour ...")
        custom_button.clicked.connect(self._pick_custom)
        layout.addWidget(custom_button)

        self.selection_label = QLabel("Nothing selected.")
        self.selection_label.setWordWrap(True)
        layout.addWidget(self.selection_label)

        self.apply_button = make_apply_button("Apply Color")
        self.apply_button.setEnabled(False)
        self.apply_button.clicked.connect(self.apply)
        layout.addWidget(self.apply_button)

        misc_row = QHBoxLayout()
        pins_button = QPushButton("Select all pins")
        pins_button.clicked.connect(self.select_all_pins)
        clear_button = QPushButton("Clear selection")
        clear_button.clicked.connect(self.clear_selection)
        reset_button = QPushButton("Reset to original")
        reset_button.clicked.connect(self._reset_original)
        misc_row.addWidget(pins_button)
        misc_row.addWidget(clear_button)
        misc_row.addWidget(reset_button)
        layout.addLayout(misc_row)
        layout.addStretch(1)

        self._update_color_chip()
        return widget

    # ------------------------------------------------------------ lifecycle

    def enter(self) -> None:
        self.status("Colors: pick a colour, then click bodies/faces (Shift adds)")

    def exit(self) -> None:
        self.ctx.scene.clear_highlight()
        self.selection = set()

    def on_document_changed(self) -> None:
        self.selection = set()
        if self._actions_widget is not None:
            self._refresh_selection_ui()

    # -------------------------------------------------------------- picking

    def on_pick(self, pick) -> None:
        if self.ctx.document is None:
            return
        key = (
            pick.body_index
            if self.target_combo.currentText() == "Bodies"
            else (pick.body_index, pick.face_index)
        )
        if self.brush_check.isChecked():
            self._paint(key, self.current_color)
            self.ctx.journal.record(
                tool=self.id,
                params={"action": "paint", "target": self.target_combo.currentText()},
                inputs={"key": list(key) if isinstance(key, tuple) else key},
                result={"color": self.current_color},
            )
            self.status(f"Colors: painted {self._key_text(key)}")
            return
        if pick.shift:
            if key in self.selection:
                self.selection.discard(key)
            else:
                self.selection.add(key)
        else:
            self.selection = {key}
        self._refresh_selection_ui()

    # -------------------------------------------------------------- actions

    def set_color(self, hex_color: str) -> None:
        self.current_color = hex_color
        self._update_color_chip()
        self.status(f"Colors: current colour {hex_color}")

    def _pick_custom(self) -> None:
        color = QColorDialog.getColor(parent=self.actions_widget())
        if color.isValid():
            self.set_color(color.name())

    def apply(self) -> None:
        if not self.selection:
            self.status("Colors: nothing selected")
            return
        for key in sorted(self.selection, key=str):
            self._paint(key, self.current_color)
        self.ctx.journal.record(
            tool=self.id,
            params={"action": "apply", "target": self.target_combo.currentText()},
            inputs={
                "keys": [list(k) if isinstance(k, tuple) else k for k in self.selection]
            },
            result={"color": self.current_color},
        )
        self.status(
            f"Colors: applied {self.current_color} to {len(self.selection)} item(s)"
        )

    def select_all_pins(self) -> None:
        """Select every detected pin — pin bodies (after Separate Unibody) in
        Bodies mode, pin face regions in Faces mode."""
        document = self.ctx.document
        registry = self.ctx.window.pins
        if document is None:
            return
        if self.target_combo.currentText() == "Bodies":
            keys = {
                index for index, body in enumerate(document.bodies)
                if body.role == "pin"
            }
            for pin in registry.pins:
                keys.update(pin.body_ids)
        else:
            keys = {
                (body_index, face_index)
                for pin in registry.pins
                for body_index, face_index in pin.face_ids
            }
        if not keys:
            self.status("Colors: no pins detected yet — run Detect Pins first")
            return
        self.selection = keys
        self._refresh_selection_ui()
        self.status(f"Colors: selected {len(keys)} pin item(s)")

    def on_miss(self) -> None:
        """Click into empty space deselects."""
        self.clear_selection()

    def clear_selection(self) -> None:
        self.selection = set()
        if self._actions_widget is not None:
            self._refresh_selection_ui()

    def _reset_original(self) -> None:
        document = self.ctx.document
        if document is None or not self.selection:
            self.status("Colors: select bodies/faces to reset first")
            return
        bodies = {
            key if isinstance(key, int) else key[0] for key in self.selection
        }
        for body_index in bodies:
            body = document.bodies[body_index]
            body.color = body.original_color
            if body.mesh is not None:
                body.mesh.face_colors = dict(body.original_face_colors)
        self.selection = set()
        self.ctx.window.document_mutated()
        self.status(f"Colors: reset {len(bodies)} body(ies) to original")

    # ---------------------------------------------------------------- impl

    def _paint(self, key, hex_color: str) -> None:
        document = self.ctx.document
        qcolor = QColor(hex_color)
        rgb = (qcolor.redF(), qcolor.greenF(), qcolor.blueF())
        if isinstance(key, int):
            body = document.bodies[key]
            body.color = rgb
            if body.mesh is not None:
                body.mesh.face_colors = {}  # body colour overrides face colours
            self.ctx.scene.set_body_color(key, rgb)
        else:
            body_index, face_index = key
            body = document.bodies[body_index]
            if body.mesh is not None:
                body.mesh.face_colors[face_index] = rgb
            self.ctx.scene.set_face_color(body_index, face_index, rgb)

    @staticmethod
    def _key_text(key) -> str:
        return f"body {key}" if isinstance(key, int) else f"body {key[0]} face {key[1]}"

    def _refresh_selection_ui(self) -> None:
        scene = self.ctx.scene
        if not self.selection:
            scene.clear_highlight()
            scene.plotter.render()
            self.selection_label.setText("Nothing selected.")
            self.apply_button.setEnabled(False)
            return
        if self.target_combo.currentText() == "Bodies":
            scene.highlight_bodies(sorted(self.selection))
        else:
            scene.highlight_faces(sorted(self.selection))
        names = ", ".join(self._key_text(k) for k in sorted(self.selection, key=str))
        self.selection_label.setText(f"{len(self.selection)} selected: {names}")
        self.apply_button.setEnabled(True)

    def _update_color_chip(self) -> None:
        self.color_chip.setStyleSheet(
            f"background: {self.current_color}; border: 1px solid #555;"
        )
