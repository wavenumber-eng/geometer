"""Assign Pin Hitboxes: bound each pin with a metadata-only volume that other
programs can intersection-test against (3D footprint). Variants per the
design intent: male box (3 clicks), auto-box, female interior, BGA cube, pad
flat prism, convex hull."""

from __future__ import annotations

from PySide6.QtWidgets import (
    QComboBox,
    QDoubleSpinBox,
    QHBoxLayout,
    QLabel,
    QListWidget,
    QPushButton,
    QVBoxLayout,
    QWidget,
)

from ..hitbox import (
    convex_hull_hitbox,
    cube_from_point,
    obb_from_points,
    obb_from_three_clicks,
    obb_xy_cross_section,
)
from ..ortho2d import FootprintPadsCanvas, Ortho2DCanvas
from ..pins import pin_mesh_points
from .base import ToolMode, make_apply_button

VARIANTS = (
    "male box (3 clicks)",
    "auto-box from pin",
    "female interior box",
    "BGA cube (1 click)",
    "pad flat prism",
    "convex hull",
)

VARIANT_KINDS = {
    "male box (3 clicks)": "male",
    "auto-box from pin": "male",
    "female interior box": "female",
    "BGA cube (1 click)": "bga",
    "pad flat prism": "pad",
    "convex hull": "curved",
}


class HitboxTool(ToolMode):
    id = "hitboxes"
    title = "Assign Pin Hitboxes"
    accent = "#0e8a8a"

    def __init__(self, ctx) -> None:
        super().__init__(ctx)
        self.clicks: list[tuple[float, float, float]] = []
        self.pending: dict[int, tuple[dict, str]] = {}  # pin number -> (hitbox, variant)

    # ------------------------------------------------------------------- UI

    def build_actions_widget(self) -> QWidget:
        widget = QWidget()
        layout = QVBoxLayout(widget)
        layout.setContentsMargins(8, 8, 8, 8)
        layout.addWidget(QLabel(
            "<b>Assign Pin Hitboxes</b><br>"
            "Select a pin, choose a variant, then click in the 3D view "
            "(3 clicks: base edge p1-p2, then opposite corner). Hitboxes are "
            "metadata, not geometry."
        ))

        variant_row = QHBoxLayout()
        variant_row.addWidget(QLabel("Variant"))
        self.variant_combo = QComboBox()
        self.variant_combo.addItems(VARIANTS)
        variant_row.addWidget(self.variant_combo, 1)
        layout.addLayout(variant_row)

        param_row = QHBoxLayout()
        param_row.addWidget(QLabel("Margin"))
        self.margin_spin = QDoubleSpinBox()
        self.margin_spin.setRange(0.0, 10.0)
        self.margin_spin.setDecimals(3)
        self.margin_spin.setSingleStep(0.01)
        self.margin_spin.setValue(0.05)
        param_row.addWidget(self.margin_spin)
        param_row.addWidget(QLabel("Size/Depth"))
        self.size_spin = QDoubleSpinBox()
        self.size_spin.setRange(0.001, 50.0)
        self.size_spin.setDecimals(3)
        self.size_spin.setValue(0.5)
        self.size_spin.setToolTip("BGA cube size / female depth / pad prism height")
        param_row.addWidget(self.size_spin)
        layout.addLayout(param_row)

        self.pin_list = QListWidget()
        self.pin_list.currentRowChanged.connect(lambda _row: self._refresh_views())
        layout.addWidget(self.pin_list, 2)

        action_row = QHBoxLayout()
        auto_button = QPushButton("Auto selected")
        auto_button.clicked.connect(self._auto_selected)
        all_button = QPushButton("Auto ALL pins")
        all_button.clicked.connect(self._auto_all)
        clear_button = QPushButton("Clear hitbox")
        clear_button.clicked.connect(self._clear_selected)
        action_row.addWidget(auto_button)
        action_row.addWidget(all_button)
        action_row.addWidget(clear_button)
        layout.addLayout(action_row)

        self.click_label = QLabel("")
        layout.addWidget(self.click_label)

        self.pending_label = QLabel("")
        self.pending_label.setStyleSheet("color: #c97a1a; font-weight: 600;")
        layout.addWidget(self.pending_label)
        self.apply_button = make_apply_button("Apply Hitboxes")
        self.apply_button.setEnabled(False)
        self.apply_button.clicked.connect(self.apply)
        layout.addWidget(self.apply_button)

        views_row = QHBoxLayout()
        hlr_column = QVBoxLayout()
        hlr_column.addWidget(QLabel("Cross-sections (geometer HLR)"))
        self.canvas = Ortho2DCanvas()
        hlr_column.addWidget(self.canvas, 1)
        pads_column = QVBoxLayout()
        pads_column.addWidget(QLabel("PCB footprint (hitboxes only)"))
        self.pads_canvas = FootprintPadsCanvas()
        pads_column.addWidget(self.pads_canvas, 1)
        views_row.addLayout(hlr_column, 1)
        views_row.addLayout(pads_column, 1)
        layout.addLayout(views_row, 3)
        return widget

    # ------------------------------------------------------------ lifecycle

    def enter(self) -> None:
        self.clicks = []
        self.ctx.window.request_footprint(self._on_footprint)
        self._refresh_views()
        registry = self.ctx.window.pins
        if registry.pins:
            self.status("Hitboxes: select a pin, place its hitbox, press Apply")
        else:
            self.status("Hitboxes: no pins yet — run Detect Pins first")

    def exit(self) -> None:
        self.ctx.scene.set_markers([])
        self.ctx.scene.clear_highlight()

    def on_document_changed(self) -> None:
        self.clicks = []
        self.pending = {}
        if self._actions_widget is not None:
            self._refresh_views()

    def _on_footprint(self, result) -> None:
        if self._actions_widget is not None and result is not None:
            self.canvas.set_projection(result, "top")

    # -------------------------------------------------------------- picking

    def on_pick(self, pick) -> None:
        registry = self.ctx.window.pins
        row = self.pin_list.currentRow()
        if not (0 <= row < len(registry.pins)):
            self.status("Hitboxes: select a pin in the list first")
            return
        pin = registry.pins[row]
        variant = self.variant_combo.currentText()
        margin = float(self.margin_spin.value())

        if variant == "BGA cube (1 click)":
            hitbox = cube_from_point(pick.world_point, float(self.size_spin.value()))
            self._stage(pin, hitbox, variant)
            return

        if variant != "male box (3 clicks)":
            self.status("Hitboxes: this variant uses the Auto buttons, not clicks")
            return

        self.clicks.append(pick.world_point)
        self.ctx.scene.set_markers(self.clicks, color="#0e8a8a")
        self.click_label.setText(f"clicks: {len(self.clicks)}/3 (p1-p2 base edge, p3 corner)")
        if len(self.clicks) < 3:
            return
        points = pin_mesh_points(self.ctx.document, pin)
        if len(points):
            z_range = (float(points[:, 2].min()), float(points[:, 2].max()))
        else:
            zs = [c[2] for c in self.clicks]
            z_range = (min(zs), max(zs))
        try:
            hitbox = obb_from_three_clicks(
                *self.clicks[:3], z_range=z_range, margin=margin
            )
        except ValueError as exc:
            self.status(f"Hitboxes: {exc}")
            self.clicks = []
            return
        self.clicks = []
        self.ctx.scene.set_markers([])
        self.click_label.setText("")
        self._stage(pin, hitbox, variant)

    # -------------------------------------------------------------- actions

    def _auto_selected(self) -> None:
        row = self.pin_list.currentRow()
        registry = self.ctx.window.pins
        if 0 <= row < len(registry.pins):
            self._auto_one(registry.pins[row])

    def _auto_all(self) -> None:
        for pin in self.ctx.window.pins.pins:
            self._auto_one(pin)

    def _auto_one(self, pin) -> None:
        points = pin_mesh_points(self.ctx.document, pin)
        if not len(points):
            self.status(f"Hitboxes: pin {pin.number} has no mesh points")
            return
        variant = self.variant_combo.currentText()
        margin = float(self.margin_spin.value())
        size = float(self.size_spin.value())
        if variant == "convex hull":
            hitbox = convex_hull_hitbox(points, margin=margin)
        elif variant == "pad flat prism":
            hitbox = obb_from_points(points, margin=margin, flat_height=size)
        elif variant == "female interior box":
            hitbox = obb_from_points(points, margin=-margin, height_override=size)
        elif variant == "BGA cube (1 click)":
            hitbox = cube_from_point(points.mean(axis=0), size)
        else:
            hitbox = obb_from_points(points, margin=margin)
        self._stage(pin, hitbox, variant)

    def _clear_selected(self) -> None:
        row = self.pin_list.currentRow()
        registry = self.ctx.window.pins
        if 0 <= row < len(registry.pins):
            pin = registry.pins[row]
            pin.hitbox = None
            self.pending.pop(pin.number, None)
            self._refresh_views()

    def _stage(self, pin, hitbox: dict, variant: str) -> None:
        """Hitboxes preview in orange until the green Apply commits them."""
        self.pending[pin.number] = (hitbox, variant)
        self._refresh_views()
        self.status(
            f"Hitboxes: pin {pin.number} staged ({variant}) — "
            f"{len(self.pending)} pending, press Apply"
        )

    def apply(self) -> None:
        if not self.pending:
            self.status("Hitboxes: nothing staged")
            return
        registry = self.ctx.window.pins
        by_number = {pin.number: pin for pin in registry.pins}
        applied = []
        for number, (hitbox, variant) in self.pending.items():
            pin = by_number.get(number)
            if pin is None:
                continue
            pin.hitbox = hitbox
            pin.kind = VARIANT_KINDS.get(variant, pin.kind)
            applied.append({"pin": number, "variant": variant, "hitbox": hitbox})
        self.pending = {}
        self.ctx.journal.record(
            tool=self.id,
            params={
                "margin": float(self.margin_spin.value()),
                "size": float(self.size_spin.value()),
            },
            inputs={},
            result={"applied": applied},
        )
        self._refresh_views()
        self.status(f"Hitboxes: applied {len(applied)} hitboxes")

    # ---------------------------------------------------------------- views

    def _refresh_views(self) -> None:
        if self._actions_widget is None:
            return
        registry = self.ctx.window.pins
        row = self.pin_list.currentRow()
        self.pin_list.blockSignals(True)
        self.pin_list.clear()
        for pin in registry.pins:
            if pin.number in self.pending:
                state = f"{self.pending[pin.number][0]['type']} (pending)"
            elif pin.hitbox:
                state = pin.hitbox["type"]
            else:
                state = "—"
            self.pin_list.addItem(f"Pin {pin.number} [{pin.kind}] hitbox: {state}")
        if 0 <= row < self.pin_list.count():
            self.pin_list.setCurrentRow(row)
        self.pin_list.blockSignals(False)

        self.pending_label.setText(
            f"{len(self.pending)} hitbox(es) staged" if self.pending else ""
        )
        self.apply_button.setEnabled(bool(self.pending))

        # committed = cyan, pending = orange, selected = red
        hitboxes: list[dict] = []
        colors: list[str] = []
        selected_index = -1
        selected_pin = registry.pins[row] if 0 <= row < len(registry.pins) else None
        for pin in registry.pins:
            pending = self.pending.get(pin.number)
            hitbox = pending[0] if pending else pin.hitbox
            if hitbox is None:
                continue
            if selected_pin is not None and pin.number == selected_pin.number:
                selected_index = len(hitboxes)
            hitboxes.append(hitbox)
            colors.append("#ff9a2a" if pending else "#27b0d6")
        self.ctx.scene.show_hitboxes(hitboxes, selected=selected_index, colors=colors)

        if selected_pin is not None:
            if selected_pin.body_ids:
                self.ctx.scene.highlight_body(selected_pin.body_ids[0])
            elif selected_pin.face_ids:
                self.ctx.scene.highlight_face(*selected_pin.face_ids[0])

        pin_labels = [(p.centroid[0], p.centroid[1], str(p.number)) for p in registry.pins]
        rects = [
            obb_xy_cross_section(hitbox)
            for hitbox in hitboxes
            if hitbox.get("type") == "obb"
        ]
        self.canvas.set_pins(pin_labels)
        self.canvas.set_rects(rects)
        self.pads_canvas.set_pins(pin_labels)
        self.pads_canvas.set_rects(rects)
