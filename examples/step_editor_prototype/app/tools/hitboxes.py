"""Assign Pin Hitboxes: bound each pin with a metadata-only volume that other
programs can intersection-test against (3D footprint). Variants per the
design intent: male box (3 clicks), auto-box, female interior, BGA cube, pad
flat prism, convex hull."""

from __future__ import annotations

import numpy as np

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
    keep_above_board,
    cavity_hitbox,
    leg_below_seat,
    convex_hull_hitbox,
    shallow_concave_hitbox,
    cube_from_point,
    obb_from_points,
    obb_from_three_clicks,
    obb_xy_cross_section,
    pin_hitbox,
    pin_solid_ids,
    resize_hitbox,
    shrink_wrap_hitbox,
    thicken_flat,
)
from ..ortho2d import FootprintPadsCanvas, Ortho2DCanvas
from ..pins import pin_mesh_points, specialty
from .base import ToolMode, make_apply_button

VARIANTS = (
    "male box (3 clicks)",
    "auto-box from pin",
    "cavity (fills to rim)",
    "shallow concave (ring on the face)",
    "BGA cube (1 click)",
    "pad flat prism",
    "convex hull",
    "shrink wrap",
)

VARIANT_KINDS = {
    "male box (3 clicks)": "male",
    "auto-box from pin": "male",
    "cavity (fills to rim)": "female",
    "BGA cube (1 click)": "bga",
    "pad flat prism": "pad",
    "convex hull": "curved",
    "shrink wrap": "curved",
}


def package_index(document) -> int | None:
    """The housing: the body with the most surface (pins are always smaller).
    Face pins on it are not their own entity."""
    sizes = [len(b.mesh.tris) if b.mesh is not None else 0 for b in document.bodies]
    return max(range(len(sizes)), key=sizes.__getitem__) if sizes else None


def pin_entity(document, pin) -> frozenset:
    """The solids this pin is: its own bodies, else the bodies its faces lie
    on, minus the housing (a contact face on the housing is not an entity)."""
    bodies = set(pin.body_ids) or {b for b, _f in pin.face_ids}
    return frozenset(bodies - {package_index(document)})


def entity_owner(document, pins, pin):
    """Hitboxes are per ENTITY (solid), not per pin face: when several pins sit
    on one solid (a contact's tail and its mouth, an anchor's two faces) only
    one of them carries the hitbox. The owner is the first non-mouth pin on the
    solid by pin number (a mouth only owns a solid no tail is on). Returns the
    owner when it is another pin, else None."""
    entity = pin_entity(document, pin)
    if not entity or specialty(pin) == "guide":
        return None
    sharing = [p for p in pins if specialty(p) != "guide" and entity & pin_entity(document, p)]
    owner = min(sharing, key=lambda p: (p.role == "mouth", str(p.number).isdigit() is False,
                                        int(p.number) if str(p.number).isdigit() else 0))
    return None if owner is pin else owner


def extrude_flat_to_seat(points: np.ndarray, size: float, toward=None) -> np.ndarray:
    """A pin known only by its flat tip face (a THT lead end, an SMT foot) has
    a zero-height hull, so its hitbox would be invisible: see `thicken_flat`
    (to Z = 0 off the board, upward on it, into the part for side faces)."""
    return thicken_flat(points, size, toward)


class HitboxTool(ToolMode):
    id = "hitboxes"
    title = "7 Pin Hitboxes"
    accent = "#0e8a8a"
    auto_method = "_auto_all"

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
        # Default to convex hull: once pins are separated into their own shells,
        # Auto ALL wraps each whole shell in a tight hull (the right hitbox for
        # arbitrary pin shapes, incl. buried/curved connector contacts).
        self.variant_combo.setCurrentText("convex hull")
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
        auto_button.setToolTip(
            "Fit the chosen variant's hitbox to the selected pin's mesh "
            "points (OBB/hull/prism/female box/cube) — staged, then Apply."
        )
        auto_button.clicked.connect(self._auto_selected)
        all_button = QPushButton("Auto ALL pins")
        all_button.setToolTip(
            "Fit the chosen variant's hitbox to every pin's mesh points "
            "at once — all staged, then Apply."
        )
        all_button.clicked.connect(self._auto_all)
        clear_button = QPushButton("Clear hitbox")
        clear_button.clicked.connect(self._clear_selected)
        clear_all_button = QPushButton("Clear ALL")
        clear_all_button.setToolTip("Remove every pin's hitbox (applied and staged)")
        clear_all_button.clicked.connect(self._clear_all)
        for button in (auto_button, all_button, clear_button, clear_all_button):
            button.setMinimumWidth(36)  # let the panel compress to 1/3 width
            action_row.addWidget(button)
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

        # the two previews side by side
        views_row = QHBoxLayout()
        hlr_column = QVBoxLayout()
        hlr_column.addWidget(QLabel("Cross-sections (geometer HLR)"))
        self.canvas = Ortho2DCanvas()
        hlr_column.addWidget(self.canvas, 1)
        pads_column = QVBoxLayout()
        pads_column.addWidget(QLabel("PCB footprint (hitboxes only)"))
        self.pads_canvas = FootprintPadsCanvas()
        pads_column.addWidget(self.pads_canvas, 1)
        for canvas in (self.canvas, self.pads_canvas):
            canvas.setMinimumSize(140, 140)  # side by side within the panel budget
        views_row.addLayout(hlr_column, 1)
        views_row.addLayout(pads_column, 1)
        layout.addLayout(views_row, 3)
        return widget

    # ------------------------------------------------------------ lifecycle

    def enter(self) -> None:
        self.clicks = []
        self.canvas.on_request = lambda: self.ctx.window.request_footprint(self._on_footprint, force=True)
        self.canvas.waiting = not self.ctx.window.request_footprint(self._on_footprint)
        self.canvas.update()
        self._refresh_views()
        registry = self.ctx.window.pins
        if registry.pins:
            self.status("Hitboxes: select a pin, place its hitbox, press Apply")
        else:
            self.status("Hitboxes: no pins yet — run Detect Pins first")

    def exit(self) -> None:
        self._hide_handles()
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
            self._select_pin_at(pick)
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

    # ---- handles: resize the selected pin's hitbox -------------------------
    def _select_pin_at(self, pick) -> None:
        """A click on a pin selects it and puts resize handles on its hitbox."""
        registry = self.ctx.window.pins
        for row, pin in enumerate(registry.pins):
            if pick.body_index in pin.body_ids or (pick.body_index, pick.face_index) in pin.face_ids:
                self.pin_list.setCurrentRow(row)
                self._show_handles(pin)
                self.status(f"Hitboxes: pin {pin.number}: drag the handles to resize (0.01 mm steps)")
                return
        self.status("Hitboxes: click a pin to resize its hitbox")

    def _hide_handles(self) -> None:
        try:
            self.ctx.scene.plotter.clear_box_widgets()
        except Exception:  # noqa: BLE001 - no widget to clear
            pass

    def _show_handles(self, pin) -> None:
        self._hide_handles()
        staged = self.pending.get(pin.number)
        hitbox = staged[0] if staged else pin.hitbox
        if not hitbox:
            return
        if hitbox.get("type") == "obb":
            hitbox = resize_hitbox(hitbox, [c for pair in zip(
                np.asarray(hitbox["center"]) - np.asarray(hitbox["half_extents"]),
                np.asarray(hitbox["center"]) + np.asarray(hitbox["half_extents"])) for c in pair])
        vertices = np.asarray(hitbox["vertices"], dtype=np.float64)
        lo, hi = vertices.min(axis=0), vertices.max(axis=0)
        bounds = [lo[0], hi[0], lo[1], hi[1], lo[2], hi[2]]

        placing = [True]  # the widget calls back once as it is created: not a resize

        def on_resize(box) -> None:
            if placing[0]:
                return
            new = resize_hitbox(hitbox, box.bounds)
            self._stage(pin, new, "resized")

        self.ctx.scene.plotter.add_box_widget(
            on_resize, bounds=bounds, factor=1.0, rotation_enabled=False,
            color="#ffd24a", interaction_event="end")
        placing[0] = False

    def _auto_selected(self) -> None:
        row = self.pin_list.currentRow()
        registry = self.ctx.window.pins
        if not 0 <= row < len(registry.pins):
            return
        owner = self._body_owner(registry.pins[row])
        if owner is not None:
            self.status(f"Hitboxes: pin {registry.pins[row].number} is on the same solid as pin "
                        f"{owner.number}: one solid, one hitbox (pin {owner.number}'s)")
            return
        self._auto_one(registry.pins[row])

    def _auto_all(self) -> None:
        # Stage everything first, then refresh ONCE — refreshing per pin
        # rebuilds every hitbox actor each time (O(n^2) on big BGAs).
        cavities = shared = 0
        for pin in self.ctx.window.pins.pins:
            if specialty(pin) == "guide":
                continue  # a GUIDE peg is not on a net: no hitbox
            if self._body_owner(pin) is not None:
                shared += 1
                continue  # one solid, one hitbox: its owner's covers it
            if pin.role == "mouth" and pin.face_ids and not pin.body_ids:
                # a mating contact inside an opening: fill the cavity to its rim
                hitbox = cavity_hitbox(self.ctx.document, pin)
                if hitbox is not None:
                    self._stage(pin, hitbox, "cavity (fills to rim)", refresh=False)
                    cavities += 1
                    continue
            self._auto_one(pin, refresh=False)
        self._refresh_views()
        self.status(
            f"Hitboxes: {len(self.pending)} staged ({cavities} filling their cavity"
            + (f"; {shared} pin(s) share another pin's solid and its hitbox" if shared else "")
            + ") — press Apply"
        )

    def _body_owner(self, pin):
        """The pin whose hitbox covers this pin's solid, or None when this pin
        owns its own hitbox (see entity_owner)."""
        return entity_owner(self.ctx.document, self.ctx.window.pins.pins, pin)

    def _auto_one(self, pin, *, refresh: bool = True) -> None:
        variant = self.variant_combo.currentText()
        margin = float(self.margin_spin.value())
        size = float(self.size_spin.value())
        pins = self.ctx.window.pins.pins
        if variant == "convex hull":
            # the rule: hull of the pin's own solid (a separate lead: the whole
            # lead), else its faces thickened the right way
            hitbox = pin_hitbox(self.ctx.document, pin, pins, margin=margin, size=size)
            if hitbox is None:
                self.status(f"Hitboxes: pin {pin.number} has no mesh points")
                return
            self._stage(pin, hitbox, variant, refresh=refresh)
            return
        solids = pin_solid_ids(self.ctx.document, pin, pins)
        if solids:
            points = np.concatenate([self.ctx.document.bodies[b].mesh.points for b in solids])
        else:  # a face-marked pin below the board: follow its metal up to Z = 0
            leg = leg_below_seat(self.ctx.document, pin)
            points = leg if leg is not None else pin_mesh_points(self.ctx.document, pin)
        if not len(points):
            self.status(f"Hitboxes: pin {pin.number} has no mesh points")
            return
        points = extrude_flat_to_seat(points, size)
        hitbox = keep_above_board(self._variant_hitbox(variant, pin, points, margin, size), points)
        if hitbox is None:
            self.status(f"Hitboxes: pin {pin.number} has no curved face for a shallow concave ring")
            return
        self._stage(pin, hitbox, variant, refresh=refresh)

    def _variant_hitbox(self, variant, pin, points, margin, size):
        """The chosen variant's hitbox from the pin's points (None: not possible)."""
        makers = {
            "convex hull": lambda: convex_hull_hitbox(points, margin=margin),
            "shrink wrap": lambda: shrink_wrap_hitbox(points, margin=margin),
            "pad flat prism": lambda: obb_from_points(points, margin=margin, flat_height=size),
            "cavity (fills to rim)": lambda: self._cavity_or_box(pin, points, margin, size),
            "shallow concave (ring on the face)": lambda: shallow_concave_hitbox(
                self.ctx.document, pin, depth=size, margin=margin),
            "BGA cube (1 click)": lambda: cube_from_point(points.mean(axis=0), size),
        }
        return makers.get(variant, lambda: obb_from_points(points, margin=margin))()

    def _cavity_or_box(self, pin, points, margin, size):
        """Fill the cavity the contact sits in (walls grown across concave
        edges, stopping at the convex rim); the old interior box if none."""
        hitbox = cavity_hitbox(self.ctx.document, pin, margin=margin)
        if hitbox is None:
            self.status(f"Hitboxes: pin {pin.number}: no cavity found around it, using a box")
            return obb_from_points(points, margin=-margin, height_override=size)
        return hitbox

    def _clear_selected(self) -> None:
        row = self.pin_list.currentRow()
        registry = self.ctx.window.pins
        if 0 <= row < len(registry.pins):
            pin = registry.pins[row]
            pin.hitbox = None
            self.pending.pop(pin.number, None)
            self.ctx.journal.record(tool=self.id, params={"action": "clear"},
                                    inputs={"pin_number": pin.number}, result={})
            self._refresh_views()

    def _clear_all(self) -> None:
        registry = self.ctx.window.pins
        cleared = sum(1 for pin in registry.pins if pin.hitbox) + len(self.pending)
        for pin in registry.pins:
            pin.hitbox = None
        self.pending = {}
        self.ctx.journal.record(tool=self.id, params={"action": "clear_all"}, inputs={}, result={})
        self._refresh_views()
        self.status(f"Hitboxes: cleared all ({cleared})")

    def _stage(self, pin, hitbox: dict, variant: str, *, refresh: bool = True) -> None:
        """Hitboxes preview in orange until the green Apply commits them."""
        self.pending[pin.number] = (hitbox, variant)
        if not refresh:
            return
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
            self._clear_sharers(pin)
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

    def _clear_sharers(self, owner) -> None:
        """One solid, one hitbox: a hitbox applied to a solid's owner replaces
        any older hitbox another pin on the same solid still carried."""
        document, pins = self.ctx.document, self.ctx.window.pins.pins
        for other in pins:
            if other is not owner and other.hitbox and entity_owner(document, pins, other) is owner:
                other.hitbox = None

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
