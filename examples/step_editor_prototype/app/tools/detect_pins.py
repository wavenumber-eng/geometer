"""Detect Pins: drag a rectangle over the pins in the orthographic top view.
Separate-solid pins are matched by their XY bounds; unibody parts use edge
flow until discontinuity (see app.pins). Detected pins are numbered
serpentine-style (or row-major for BGAs), honoring the pin-1 hint from the
Pin 1 Quadrant tool, and can be reordered manually."""

from __future__ import annotations

import geometer
import numpy as np
from PySide6.QtWidgets import (
    QCheckBox,
    QComboBox,
    QDoubleSpinBox,
    QHBoxLayout,
    QLabel,
    QListWidget,
    QPushButton,
    QVBoxLayout,
    QWidget,
)

from ..export_ap242 import document_step_bytes
from ..ortho2d import Ortho2DCanvas
from ..pins import (
    Band,
    Pin,
    detect_pins_multibody,
    detect_pins_unibody,
    order_pins,
)
from ..scene import BandSelector
from .base import ToolMode


class DetectPinsTool(ToolMode):
    id = "detect_pins"
    title = "Detect Pins"
    accent = "#2a7a2a"

    def __init__(self, ctx) -> None:
        super().__init__(ctx)
        self._band_selector: BandSelector | None = None
        self._last_band: Band | None = None

    # ------------------------------------------------------------------- UI

    def build_actions_widget(self) -> QWidget:
        widget = QWidget()
        layout = QVBoxLayout(widget)
        layout.setContentsMargins(8, 8, 8, 8)
        layout.addWidget(QLabel(
            "<b>Detect Pins</b><br>"
            "Drag a rectangle over the pins (top view, left button). "
            "Each drag adds the pins it finds; orbit with right/middle button."
        ))

        self.exclude_check = QCheckBox("Exclude largest body (package)")
        self.exclude_check.setChecked(True)
        layout.addWidget(self.exclude_check)

        flow_row = QHBoxLayout()
        flow_row.addWidget(QLabel("Unibody flow"))
        self.flow_combo = QComboBox()
        self.flow_combo.addItems(["any-edge", "smooth-only"])
        self.flow_combo.setToolTip(
            "any-edge: whole leads (band boundary is the discontinuity)\n"
            "smooth-only: raw smooth chains, stops at sharp dihedrals"
        )
        flow_row.addWidget(self.flow_combo, 1)
        flow_row.addWidget(QLabel("angle"))
        self.angle_spin = QDoubleSpinBox()
        self.angle_spin.setRange(1.0, 89.0)
        self.angle_spin.setValue(30.0)
        self.angle_spin.setSuffix(" deg")
        flow_row.addWidget(self.angle_spin)
        layout.addLayout(flow_row)

        order_row = QHBoxLayout()
        order_row.addWidget(QLabel("Ordering"))
        self.order_combo = QComboBox()
        self.order_combo.addItems(["serpentine", "row-major"])
        order_row.addWidget(self.order_combo, 1)
        renumber_button = QPushButton("Renumber")
        renumber_button.clicked.connect(self.renumber)
        order_row.addWidget(renumber_button)
        layout.addLayout(order_row)

        self.pin_list = QListWidget()
        self.pin_list.currentRowChanged.connect(self._on_list_selection)
        layout.addWidget(self.pin_list, 2)

        edit_row = QHBoxLayout()
        for label, handler in (
            ("Up", lambda: self._move_selected(-1)),
            ("Down", lambda: self._move_selected(1)),
            ("Reverse", self._reverse),
            ("Pin 1", self._make_pin1),
            ("Delete", self._delete_selected),
            ("Clear", self._clear),
        ):
            button = QPushButton(label)
            button.clicked.connect(handler)
            edit_row.addWidget(button)
        layout.addLayout(edit_row)

        layout.addWidget(QLabel("Footprint (geometer HLR, top view)"))
        self.canvas = Ortho2DCanvas()
        layout.addWidget(self.canvas, 3)
        return widget

    # ------------------------------------------------------------ lifecycle

    def enter(self) -> None:
        if self.ctx.document is not None:
            self.ctx.window.snap_camera("top")
        widget = self.ctx.scene.plotter.interactor
        self._band_selector = BandSelector(widget, self._on_band_qt)
        self._band_selector.attach()
        self._refresh_footprint()
        self._refresh_views()
        self.status("Detect Pins: drag a rectangle over the pins (top view)")

    def exit(self) -> None:
        if self._band_selector is not None:
            self._band_selector.detach()
            self._band_selector = None
        self.ctx.scene.show_pin_labels([], [])
        self.ctx.scene.clear_highlight()

    def on_document_changed(self) -> None:
        registry = self.ctx.window.pins
        registry.clear()
        self._last_band = None
        if self._actions_widget is not None:
            self.pin_list.clear()
            self.canvas.set_projection(None)
            self.canvas.set_pins([])
            self.canvas.set_band(None)

    # ------------------------------------------------------------ detection

    def _on_band_qt(self, origin, end) -> None:
        if self.ctx.document is None:
            return
        scene = self.ctx.scene
        x0, y0, _ = scene.display_to_world(origin.x(), origin.y())
        x1, y1, _ = scene.display_to_world(end.x(), end.y())
        band = Band(min(x0, x1), min(y0, y1), max(x0, x1), max(y0, y1))
        self._last_band = band
        self.detect_in_band(band)

    def detect_in_band(self, band: Band) -> None:
        document = self.ctx.document
        multibody = len(document.bodies) > 1
        if multibody:
            found = detect_pins_multibody(
                document, band, exclude_largest=self.exclude_check.isChecked()
            )
        else:
            found = detect_pins_unibody(
                document,
                0,
                band,
                smooth_angle_deg=float(self.angle_spin.value()),
                flow=self.flow_combo.currentText(),
            )

        registry = self.ctx.window.pins
        existing = {self._pin_key(pin) for pin in registry.pins}
        added = [pin for pin in found if self._pin_key(pin) not in existing]
        registry.set_pins(registry.pins + added)
        self.renumber(record=False)

        self.ctx.journal.record(
            tool=self.id,
            params={
                "mode": "multibody" if multibody else "unibody",
                "exclude_largest": self.exclude_check.isChecked(),
                "flow": self.flow_combo.currentText(),
                "smooth_angle_deg": float(self.angle_spin.value()),
                "ordering": self.order_combo.currentText(),
            },
            inputs={"band": [band.x_min, band.y_min, band.x_max, band.y_max]},
            result={
                "found": len(found),
                "added": len(added),
                "total": len(registry.pins),
                "centroids": [list(pin.centroid) for pin in added],
            },
        )
        self.status(
            f"Detect Pins: {len(found)} in band, {len(added)} new, "
            f"{len(registry.pins)} total"
        )

    @staticmethod
    def _pin_key(pin: Pin):
        return (tuple(pin.body_ids), tuple(map(tuple, pin.face_ids)))

    # ------------------------------------------------------------- ordering

    def renumber(self, *, record: bool = True) -> None:
        registry = self.ctx.window.pins
        if not registry.pins:
            self._refresh_views()
            return
        hint = self.ctx.window.pin1_hint
        order = order_pins(
            registry.pins,
            mode=self.order_combo.currentText(),
            pin1_hint=(hint[0], hint[1]) if hint else None,
        )
        registry.reorder(order)
        if record:
            self.ctx.journal.record(
                tool=self.id,
                params={"action": "renumber", "ordering": self.order_combo.currentText()},
                inputs={"pin1_hint": list(hint) if hint else None},
                result={"order": [pin.number for pin in registry.pins]},
            )
        self._refresh_views()

    def _move_selected(self, delta: int) -> None:
        row = self.pin_list.currentRow()
        if row >= 0:
            new_row = self.ctx.window.pins.move(row, delta)
            self._refresh_views()
            self.pin_list.setCurrentRow(new_row)

    def _reverse(self) -> None:
        self.ctx.window.pins.reverse()
        self._refresh_views()

    def _make_pin1(self) -> None:
        row = self.pin_list.currentRow()
        if row >= 0:
            self.ctx.window.pins.make_pin1(row)
            self._refresh_views()
            self.pin_list.setCurrentRow(0)

    def _delete_selected(self) -> None:
        row = self.pin_list.currentRow()
        registry = self.ctx.window.pins
        if 0 <= row < len(registry.pins):
            del registry.pins[row]
            registry.set_pins(registry.pins)
            self._refresh_views()

    def _clear(self) -> None:
        self.ctx.window.pins.clear()
        self._last_band = None
        self._refresh_views()

    # --------------------------------------------------------------- views

    def _on_list_selection(self, row: int) -> None:
        registry = self.ctx.window.pins
        if not (0 <= row < len(registry.pins)):
            self.ctx.scene.clear_highlight()
            return
        pin = registry.pins[row]
        if pin.body_ids:
            self.ctx.scene.highlight_body(pin.body_ids[0])
        elif pin.face_ids:
            self.ctx.scene.highlight_face(*pin.face_ids[0])

    def _refresh_views(self) -> None:
        registry = self.ctx.window.pins
        if self._actions_widget is None:
            return
        self.pin_list.clear()
        for pin in registry.pins:
            kind = f"bodies {pin.body_ids}" if pin.body_ids else f"{len(pin.face_ids)} faces"
            self.pin_list.addItem(
                f"Pin {pin.number}  ({pin.centroid[0]:+.2f}, {pin.centroid[1]:+.2f})  {kind}"
            )
        points = [pin.centroid for pin in registry.pins]
        labels = [str(pin.number) for pin in registry.pins]
        self.ctx.scene.show_pin_labels(points, labels)
        self.canvas.set_pins(
            [(pin.centroid[0], pin.centroid[1], str(pin.number)) for pin in registry.pins]
        )
        if self._last_band is not None:
            self.canvas.set_band(
                (self._last_band.x_min, self._last_band.y_min,
                 self._last_band.x_max, self._last_band.y_max)
            )

    def _refresh_footprint(self) -> None:
        if self.ctx.document is None:
            return
        try:
            step_bytes = document_step_bytes(self.ctx.document)
            result = geometer.project_step_hlr(
                step_bytes,
                views=[geometer.ProjectionView.top()],
                options=geometer.HlrOptions.visible_detail(),
            )
            self.canvas.set_projection(result, "top")
        except Exception as exc:
            self.status(f"Detect Pins: footprint projection failed ({exc})")
