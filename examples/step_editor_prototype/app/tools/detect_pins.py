"""Detect Pins: drag a rectangle over the pins in the orthographic top view.
Separate-solid pins are matched by their XY bounds; unibody parts use edge
flow until discontinuity (see app.pins). Detected pins are numbered
serpentine-style (or row-major for BGAs), honoring the pin-1 hint from the
Pin 1 Quadrant tool, and can be reordered manually."""

from __future__ import annotations

import numpy as np
from PySide6.QtCore import Qt
from PySide6.QtGui import QKeySequence, QShortcut
from PySide6.QtWidgets import (
    QCheckBox,
    QComboBox,
    QDoubleSpinBox,
    QHBoxLayout,
    QLabel,
    QLineEdit,
    QListWidget,
    QPushButton,
    QVBoxLayout,
    QWidget,
)

from ..ortho2d import Ortho2DCanvas
from ..pins import (
    Band,
    Pin,
    detect_pins_by_section,
    detect_pins_multibody,
    detect_pins_unibody,
    find_similar_regions,
    grow_smooth_region,
    join_mouth_pins,
    mesh_region_centroid,
    order_pins,
    parse_grid_name,
    predict_grid_names,
    predict_serpentine_order,
    section_segments,
)
from ..scene import BandSelector
from .base import ToolMode, make_apply_button

# Area cutoff for SMT/THR seed growth — matches Separate's default factor so
# the seed isolates one contact (not the whole housing) on connectors whose
# contacts meet the body tangent-continuously. See grow_smooth_region.
SEED_AREA_FACTOR = 4.0

# Detect EXACT tolerances: tight area + bbox match (vs find_similar_regions'
# loose 0.12/0.25 defaults) so it copies only the SAME tip shape, in any
# orientation — the basis for hand-building a reference file.
_EXACT_AREA_TOL = 0.06
_EXACT_DIM_TOL = 0.08


class DetectPinsTool(ToolMode):
    id = "detect_pins"
    title = "Detect and Name Pins"
    accent = "#2a7a2a"

    def __init__(self, ctx) -> None:
        super().__init__(ctx)
        self._band_selector: BandSelector | None = None
        self._last_band: Band | None = None
        self.pending: list[Pin] = []
        self._pending_bands: list[list[float]] = []

    # ------------------------------------------------------------------- UI

    def build_actions_widget(self) -> QWidget:
        widget = QWidget()
        layout = QVBoxLayout(widget)
        layout.setContentsMargins(8, 8, 8, 8)
        layout.addWidget(QLabel(
            "<b>Detect and Name Pins</b><br>"
            "Seed a tip and Detect Exact (SMT/THR or CON/HEAD), or use Context "
            "Plane / Auto. Click a pin's faces to select it; press Delete to "
            "remove it. Then Apply."
        ))

        # ---- SMT / THR group ----
        smt_label = QLabel("SMT / THR")
        smt_label.setStyleSheet("font-weight: 700; color: #c89d00; padding-top: 4px;")
        layout.addWidget(smt_label)
        smt_row = QHBoxLayout()
        self.context_button = QPushButton("Context Plane")
        self.context_button.setToolTip(
            "One click: slice the model 0.01 mm above the Z-sit seating plane "
            "(that work is already done) — every closed shape in the section "
            "is staged as a pin."
        )
        self.context_button.clicked.connect(self._run_context_plane)
        smt_row.addWidget(self.context_button)
        self.seed_button = QPushButton("Seed Tip")
        self.seed_button.setCheckable(True)
        self.seed_button.setToolTip(
            "Click a pin TIP (the end face of an SMT/THR pad — every pin has\n"
            "one, square/round/curved). Each click stages one pin; then press\n"
            "Detect Exact to find the rest of the row automatically."
        )
        self.seed_button.setStyleSheet(
            "QPushButton:checked {background-color: #c89d00; color: #000000; font-weight: 700;}"
        )
        self.seed_button.toggled.connect(self._on_seed_toggled)
        smt_row.addWidget(self.seed_button)
        smt_similar_button = QPushButton("Detect Exact")
        smt_similar_button.setToolTip(
            "Find every face of the SAME SHAPE as the last SMT/THR tip, in any\n"
            "orientation (exact area + bbox match) — the other pads — and stage\n"
            "them. Use it to copy one hand-marked tip and seed a reference."
        )
        smt_similar_button.clicked.connect(lambda: self._detect_similar("primary"))
        smt_row.addWidget(smt_similar_button)
        layout.addLayout(smt_row)

        self.exclude_check = QCheckBox("Exclude largest body (package)")
        self.exclude_check.setChecked(True)
        layout.addWidget(self.exclude_check)

        # ---- CON / HEAD group ----
        con_label = QLabel("CON / HEAD")
        con_label.setStyleSheet("font-weight: 700; color: #2a6fd4; padding-top: 4px;")
        layout.addWidget(con_label)
        mouth_row = QHBoxLayout()
        join_button = QPushButton("Join Pins")
        join_button.setToolTip(
            "Give each blue mouth pin the designator of the primary pin it\n"
            "lines up with along the connector row — same designator = same\n"
            "net, so their hitboxes link as one electrical node."
        )
        join_button.clicked.connect(self._join_mouth_pins)
        mouth_row.addWidget(join_button)
        self.mouth_seed_button = QPushButton("Seed Tip")
        self.mouth_seed_button.setCheckable(True)
        self.mouth_seed_button.setToolTip(
            "Click a CON/HEAD contact TIP up inside the connector mouth —\n"
            "each clicked face is staged as a BLUE mouth pin, exactly the tip\n"
            "you click. Then Detect Exact finds the other contacts."
        )
        self.mouth_seed_button.setStyleSheet(
            "QPushButton:checked {background-color: #2a6fd4; color: #ffffff; font-weight: 700;}"
        )
        self.mouth_seed_button.toggled.connect(self._on_mouth_seed_toggled)
        mouth_row.addWidget(self.mouth_seed_button)
        similar_button = QPushButton("Detect Exact")
        similar_button.setToolTip(
            "Find every face of the SAME SHAPE as the last CON/HEAD tip, in any\n"
            "orientation (exact area + bbox match) — the other mouth contacts — in blue."
        )
        similar_button.clicked.connect(lambda: self._detect_similar("mouth"))
        mouth_row.addWidget(similar_button)
        layout.addLayout(mouth_row)

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
        self.order_combo.addItems(["serpentine", "row-major", "bga-grid (A1..)"])
        order_row.addWidget(self.order_combo, 1)
        renumber_button = QPushButton("Renumber")
        renumber_button.clicked.connect(self.renumber)
        order_row.addWidget(renumber_button)
        layout.addLayout(order_row)

        name_row = QHBoxLayout()
        name_row.addWidget(QLabel("Name"))
        self.name_edit = QLineEdit()
        self.name_edit.setPlaceholderText("e.g. A1 or 5")
        self.name_edit.returnPressed.connect(self._set_selected_name)
        self.name_edit.textEdited.connect(self._autocapitalize)
        name_row.addWidget(self.name_edit, 1)
        self.autocap_button = QPushButton("aA")
        self.autocap_button.setCheckable(True)
        self.autocap_button.setChecked(True)
        self.autocap_button.setMaximumWidth(34)
        self.autocap_button.setToolTip(
            "Auto-capitalize typed pin names (uncheck to allow lowercase)"
        )
        name_row.addWidget(self.autocap_button)
        set_name_button = QPushButton("Set")
        set_name_button.setMinimumWidth(36)
        set_name_button.clicked.connect(self._set_selected_name)
        name_row.addWidget(set_name_button)
        predict_button = QPushButton("Predict")
        predict_button.setMinimumWidth(36)
        predict_button.setToolTip(
            "Name a few pins by hand (grid names like A1/C4, or plain numbers),\n"
            "then Predict propagates the scheme to every pin from their 3D\n"
            "positions. Blue mouth pins are joined to the tail they line up\n"
            "with automatically (same as Join to Pins)."
        )
        predict_button.clicked.connect(self.predict_names)
        name_row.addWidget(predict_button)
        layout.addLayout(name_row)

        self.pending_label = QLabel("")
        self.pending_label.setStyleSheet("color: #c97a1a; font-weight: 600;")
        layout.addWidget(self.pending_label)

        auto_button = QPushButton("Auto")
        auto_button.setToolTip(
            "Stage pins automatically: context-plane slice 0.01 mm above\n"
            "the seat, falling back to whole-model multibody detection.\n"
            "Everything stages orange — review, Delete strays, Apply."
        )
        auto_button.clicked.connect(self.run_auto)
        layout.addWidget(auto_button)

        self.apply_button = make_apply_button("Apply Detected Pins")
        self.apply_button.setEnabled(False)
        self.apply_button.clicked.connect(self.apply)
        layout.addWidget(self.apply_button)

        self.pin_list = QListWidget()
        self.pin_list.currentRowChanged.connect(self._on_list_selection)
        layout.addWidget(self.pin_list, 2)
        # DEL removes the selected pin (selecting a pin's faces in the 3D view
        # focuses this list) — fires only when the list, not the name box, has
        # focus, so typing in Name still edits text normally.
        delete_shortcut = QShortcut(QKeySequence(Qt.Key_Delete), self.pin_list)
        delete_shortcut.activated.connect(self._delete_selected)

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
            button.setMinimumWidth(36)  # let the panel compress to 1/3 width
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
        self.ctx.window.request_footprint(self._on_footprint)
        self._refresh_views()
        self.status(
            "Detect Pins: seed a tip + Detect Exact, or Context Plane / Auto; "
            "click a pin to select, Delete to remove"
        )

    def exit(self) -> None:
        self._disarm_band()
        self.ctx.scene.show_pin_labels([], [])
        self.ctx.scene.set_markers([], name="pending-pins")
        self.ctx.scene.set_markers([], name="pending-mouth")
        self.ctx.scene.remove_overlay("context-section")
        self.ctx.scene.clear_highlight()

    # ------------------------------------------------------------- band arm

    def _arm_band(self, armed: bool) -> None:
        if not armed:
            self._disarm_band()
            self.status("Detect Pins: drag-select off — left button orbits")
            return
        scene = self.ctx.scene
        self._band_selector = BandSelector(
            scene.plotter.interactor,
            self._on_band_qt,
            on_update=lambda origin, pos: scene.show_band_2d(
                origin.x(), origin.y(), pos.x(), pos.y()
            ),
            on_finish=scene.hide_band_2d,
            on_click=self._on_click_qt,
        )
        self._band_selector.attach()
        self.status("Detect Pins: drag a rectangle over the pins (top view)")

    def _disarm_band(self) -> None:
        if self._band_selector is not None:
            self._band_selector.detach()
            self._band_selector = None

    def _on_seed_toggled(self, checked: bool) -> None:
        if checked and self._actions_widget is not None:
            self.mouth_seed_button.setChecked(False)

    def _on_mouth_seed_toggled(self, checked: bool) -> None:
        if checked and self._actions_widget is not None:
            self.seed_button.setChecked(False)
            self.status(
                "Mouth Seed: click a mating-contact face inside the mouth — "
                "each click stages one blue mouth pin"
            )

    def on_pick(self, pick) -> None:
        """Plain click: seed a pin from the clicked face when armed,
        otherwise select that pin for quick rename."""
        if self._actions_widget is not None and self.mouth_seed_button.isChecked():
            self._seed_pin_from_pick(pick, role="mouth")
            return
        if self._actions_widget is not None and self.seed_button.isChecked():
            self._seed_pin_from_pick(pick)
            return
        self._select_pin_from_pick(pick)

    def _seed_pin_from_pick(self, pick, role: str = "primary") -> None:
        document = self.ctx.document
        if document is None:
            return
        registry = self.ctx.window.pins
        claimed = {
            face
            for pin in (*registry.pins, *self.pending)
            for body, face in pin.face_ids
            if body == pick.body_index
        }
        if pick.face_index in claimed:
            self.status("Seed Pin: that face already belongs to a pin")
            return
        # Tip-based seeding: the clicked face IS the pin tip — every pin has
        # one (square, round, or curved). No growth, so nothing can flood;
        # Detect Exact finds the rest by matching tip area+shape, and the
        # spine / Separate grows each tip into its full contact.
        region = {pick.face_index}
        mesh = document.bodies[pick.body_index].mesh
        centroid = mesh_region_centroid(mesh, sorted(region))
        if centroid is None:
            return
        pin = Pin(
            number=0,
            centroid=centroid,
            face_ids=[(pick.body_index, face) for face in sorted(region)],
            role=role,
        )
        self.pending.append(pin)
        self.ctx.journal.record(
            tool=self.id,
            params={"action": "seed_pin", "role": role, "grow": False},
            inputs={"body": pick.body_index, "face": pick.face_index,
                    "point": list(pick.world_point)},
            result={"faces": len(region), "centroid": list(centroid)},
        )
        self._refresh_pending()
        kind = "CON/HEAD" if role == "mouth" else "SMT/THR"
        self.status(
            f"{kind} Seed: staged the pin tip — {len(self.pending)} pending; "
            f"keep clicking tips or press Detect Exact to find the rest"
        )

    # ----------------------------------------------------------- mouth pins

    def _detect_similar(self, role: str = "mouth") -> None:
        """Stage a pin for every face region of the SAME SHAPE as the last
        seeded tip of `role`, in any orientation (the match is on area + the
        sorted bbox extents, both rotation/mirror invariant) — an EXACT shape
        match, not a loose one, so it copies one hand-marked tip across the
        whole part and the result can seed a reference file."""
        # QOL: Detect Exact ends the seeding gesture — disarm the Seed Tip
        # button for this role so plain clicks orbit again.
        if self._actions_widget is not None:
            seed_button = (self.mouth_seed_button if role == "mouth"
                           else self.seed_button)
            seed_button.setChecked(False)
        document = self.ctx.document
        if document is None:
            return
        registry = self.ctx.window.pins
        same = [p for p in self.pending if p.role == role] or [
            p for p in registry.pins if p.role == role
        ]
        if not same:
            kind = "CON/HEAD" if role == "mouth" else "SMT/THR"
            self.status(f"Detect Exact: stage a {kind} tip first")
            return
        template = same[-1]
        seed_body = template.face_ids[0][0]
        seed_region = {face for body, face in template.face_ids if body == seed_body}
        claimed = {
            (body, face)
            for pin in (*registry.pins, *self.pending)
            for body, face in pin.face_ids
        }
        matches = find_similar_regions(
            document, seed_body, seed_region,
            smooth_angle_deg=float(self.angle_spin.value()),
            claimed=claimed,
            area_tol=_EXACT_AREA_TOL, dim_tol=_EXACT_DIM_TOL,
        )
        added = []
        for region in matches:
            mesh = document.bodies[region[0][0]].mesh
            centroid = mesh_region_centroid(
                mesh, sorted(face for _body, face in region)
            )
            if centroid is None:
                continue
            added.append(Pin(number=0, centroid=centroid,
                             face_ids=region, role=role))
        self.pending.extend(added)
        self.ctx.journal.record(
            tool=self.id,
            params={"action": "detect_similar", "role": role,
                    "smooth_angle_deg": float(self.angle_spin.value()),
                    "area_tol": _EXACT_AREA_TOL, "dim_tol": _EXACT_DIM_TOL},
            inputs={"body": seed_body, "faces": sorted(seed_region)},
            result={"matches": len(added),
                    "centroids": [list(pin.centroid) for pin in added]},
        )
        self._refresh_pending()
        kind = "CON/HEAD" if role == "mouth" else "SMT/THR"
        self.status(
            f"Detect Exact ({kind}): {len(added)} same-shape tip(s) staged — "
            f"{len(self.pending)} pending; press Apply"
        )

    def _join_mouth_pins(self) -> str | None:
        """Pair each mouth pin with the primary pin it lines up with along
        the connector row and inherit its designator (same designator = same
        net, hitboxes linked). Returns the outcome line (also shown in the
        status bar) or None when there was nothing to join."""
        registry = self.ctx.window.pins
        primaries = registry.primaries()
        mouths = registry.mouths() + [p for p in self.pending if p.role == "mouth"]
        if not mouths:
            self.status("Join: no mouth pins — stage some with Mouth Seed first")
            return None
        if not primaries:
            self.status("Join: detect and Apply the primary (tail) pins first")
            return None
        assigned, conflicts = join_mouth_pins(primaries, mouths)
        anchors = sum(1 for pin in mouths if pin.name_source == "anchor")
        for index, primary in assigned.items():
            if mouths[index].name_source == "anchor":
                continue  # the user's green anchors are ground truth
            mouths[index].name = primary.name or str(primary.number)
            mouths[index].name_source = "joined"
        self.ctx.journal.record(
            tool=self.id,
            params={"action": "join_mouth"},
            inputs={},
            result={
                "centroids": [list(mouths[i].centroid) for i in sorted(assigned)],
                "names": [mouths[i].name for i in sorted(assigned)],
                "conflicts": len(conflicts),
            },
        )
        self._refresh_views()
        self._refresh_pending()
        outcome = f"{len(assigned)} mouth pin(s) inherited designators"
        if anchors:
            outcome += f" ({anchors} anchor(s) calibrated the pairing)"
        if conflicts:
            outcome += f" — {len(conflicts)} had no primary in line; check those"
        self.status(f"Join: {outcome}")
        return outcome

    def run_auto(self) -> None:
        """One press: detect, APPLY, number the tail row, and Join the mouth
        (mating) pins so each shares its tail's designator. Tip end-cap binning
        for unibody connectors (SMT/THR tail + CON/HEAD mouth), whole-body pins
        for multibody parts, context-plane section as the fallback. AUTO runs
        the trained Z-Sit in the background to get the seat plane, so it works
        on a naked, un-seated model in any orientation — the normal pipeline
        seats first, but this stays robust when a user runs it standalone."""
        document = self.ctx.document
        if document is None:
            return
        from ..auto import auto_detect_pins

        found, how = auto_detect_pins(document)
        if not found:
            self.status("Auto Detect: nothing found — seed a pin tip manually")
            return
        staged = self._stage_found(found)
        self._refresh_pending()
        if staged:
            self.apply()              # registry + renumber the primary tails
            self._join_mouth_pins()   # mouths inherit their tail's designator
        primaries = sum(1 for p in found if p.role != "mouth")
        mouths = sum(1 for p in found if p.role == "mouth")
        label = {"tips": "tip clusters", "multibody": "whole bodies",
                 "bga": "BGA ball grid", "radial": "radial pin (one body)",
                 "reference": "baked reference (exact)",
                 "context-plane": "context-plane section"}.get(how, how)
        self.status(
            f"Auto Detect ({label}): {primaries} tail pin(s) (SMT/THR) + "
            f"{mouths} mating (CON/HEAD), applied, numbered and joined"
            + self._hint_note(document, primaries)
        )

    def _hint_note(self, document, primaries: int) -> str:
        """Goal feedback from the filename's package hint: confirm the count or
        flag a mismatch, so a wrong detection is caught against the part name."""
        from ..package_hint import package_hint

        name = document.path.name if getattr(document, "path", None) else ""
        hint = package_hint(name)
        if not hint or not hint.get("leads"):
            return ""
        leads = hint["leads"]
        mark = "✓ matches" if primaries == leads else "⚠ name expects"
        return f"  [{mark} {leads} {hint['archetype']} leads]"

    def _stage_found(self, found) -> list:
        """Stage the detected pins whose faces don't overlap an existing or
        already-pending pin; returns the newly staged list."""
        existing = {
            key
            for pin in (*self.ctx.window.pins.pins, *self.pending)
            for key in pin.face_ids
        }
        staged = []
        for pin in found:
            if not any(key in existing for key in pin.face_ids):
                staged.append(pin)
                existing.update(pin.face_ids)
        self.pending.extend(staged)
        return staged

    # --------------------------------------------------------- context plane

    def _run_context_plane(self) -> None:
        """One click: slice 0.01 mm above the Z-sit seating plane and stage
        every closed shape as a pin — the user already defined that frame."""
        document = self.ctx.document
        if document is None:
            return
        bounds = document.bounds()
        point = [(bounds[0] + bounds[1]) / 2.0, (bounds[2] + bounds[3]) / 2.0, 0.01]
        normal = [0.0, 0.0, -1.0]

        segments = [
            section_segments(body.mesh, np.asarray(point), np.asarray(normal))
            for body in document.bodies
            if body.mesh is not None and len(body.mesh.tris)
        ]
        segments = [s for s in segments if len(s)]
        merged = np.concatenate(segments) if segments else np.empty((0, 2, 3))
        self.ctx.scene.show_section(merged)
        if not len(merged):
            self.status(
                "Context Plane: nothing crosses Z=0.01 — run Z-Sit first so "
                "the model sits on Z=0"
            )
            return

        found = detect_pins_by_section(document, point, normal)
        self.ctx.window.context_plane = (list(point), list(normal))
        registry = self.ctx.window.pins
        existing = {self._pin_key(pin) for pin in registry.pins}
        existing.update(self._pin_key(pin) for pin in self.pending)
        added = [pin for pin in found if self._pin_key(pin) not in existing]
        self.pending.extend(added)
        self.ctx.journal.record(
            tool=self.id,
            params={"action": "context_plane"},
            inputs={"plane_point": point, "plane_normal": normal},
            result={"closed_shapes": len(found), "added": len(added),
                    "centroids": [list(pin.centroid) for pin in added]},
        )
        self._refresh_pending()
        self.status(
            f"Context Plane: {len(found)} closed shapes at Z=0.01, "
            f"{len(added)} newly staged, {len(self.pending)} pending — press Apply"
        )

    def on_document_changed(self) -> None:
        # The registry itself survives edits (Separate Unibody remaps pins to
        # the new bodies); only staged-but-unapplied state goes stale. The
        # window clears the registry on file load.
        self._last_band = None
        self.pending = []
        self._pending_bands = []
        self.ctx.scene.remove_overlay("context-section")
        if self._actions_widget is not None:
            self.canvas.set_projection(None)
            self.canvas.set_band(None)
            self._refresh_pending()
            self._refresh_views()

    def _on_footprint(self, result) -> None:
        if self._actions_widget is not None and result is not None:
            self.canvas.set_projection(result, "top")

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
        """Stage the pins found in the band; Apply commits them."""
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
        existing.update(self._pin_key(pin) for pin in self.pending)
        added = [pin for pin in found if self._pin_key(pin) not in existing]
        self.pending.extend(added)
        self._pending_bands.append([band.x_min, band.y_min, band.x_max, band.y_max])
        self._refresh_pending()
        self.status(
            f"Detect Pins: {len(found)} in band, {len(added)} newly staged, "
            f"{len(self.pending)} pending — press Apply"
        )

    def _refresh_pending(self) -> None:
        self.ctx.scene.set_markers(
            [pin.centroid for pin in self.pending if pin.role != "mouth"],
            name="pending-pins",
            color="#ff9a2a",
            point_size=26.0,
        )
        self.ctx.scene.set_markers(
            [pin.centroid for pin in self.pending if pin.role == "mouth"],
            name="pending-mouth",
            color="#2a6fd4",
            point_size=26.0,
        )
        if self._actions_widget is not None:
            count = len(self.pending)
            self.pending_label.setText(
                f"{count} pin(s) staged" if count else ""
            )
            self.apply_button.setEnabled(count > 0)
        if self._last_band is not None and self._actions_widget is not None:
            self.canvas.set_band(
                (self._last_band.x_min, self._last_band.y_min,
                 self._last_band.x_max, self._last_band.y_max)
            )

    def apply(self) -> None:
        if not self.pending:
            self.status("Detect Pins: nothing staged — drag a band first")
            return
        registry = self.ctx.window.pins
        added = self.pending
        self.pending = []
        bands = self._pending_bands
        self._pending_bands = []
        registry.set_pins(registry.pins + added)
        self.renumber(record=False)
        self.ctx.journal.record(
            tool=self.id,
            params={
                "exclude_largest": self.exclude_check.isChecked(),
                "flow": self.flow_combo.currentText(),
                "smooth_angle_deg": float(self.angle_spin.value()),
                "ordering": self.order_combo.currentText(),
            },
            inputs={"bands": bands},
            result={
                "added": len(added),
                "total": len(registry.pins),
                "centroids": [list(pin.centroid) for pin in added],
            },
        )
        self._refresh_pending()
        self.status(f"Detect Pins: applied {len(added)} pins, {len(registry.pins)} total")

    @staticmethod
    def _pin_key(pin: Pin):
        return (tuple(pin.body_ids), tuple(map(tuple, pin.face_ids)))

    # ------------------------------------------------------------- ordering

    def _ordering_mode(self) -> str:
        return self.order_combo.currentText().split(" ")[0]

    def renumber(self, *, record: bool = True) -> None:
        registry = self.ctx.window.pins
        if not registry.pins:
            self._refresh_views()
            return
        mode = self._ordering_mode()
        hint = self.ctx.window.pin1_hint
        primaries = registry.primaries()
        order = order_pins(
            primaries,
            mode=mode,
            pin1_hint=(hint[0], hint[1]) if hint else None,
        )
        registry.set_pins([primaries[i] for i in order] + registry.mouths())
        if mode == "bga-grid":
            try:
                primaries = registry.primaries()
                names = predict_grid_names(primaries, {})
                for index, pin in enumerate(primaries):
                    pin.name = names[index]
                    pin.name_source = ""  # default scheme: plain white
                unknown = sum(1 for pin in primaries if pin.name.startswith("?"))
                if unknown:
                    self.status(
                        f"Detect Pins: {unknown} region(s) named '?' — likely not "
                        f"balls (index marks); select and Delete them"
                    )
            except ValueError as exc:
                self.status(f"Detect Pins: grid naming failed ({exc})")
        if record:
            self.ctx.journal.record(
                tool=self.id,
                params={"action": "renumber", "ordering": mode},
                inputs={"pin1_hint": list(hint) if hint else None},
                result={"order": [pin.number for pin in registry.pins],
                        "names": [pin.name for pin in registry.pins]},
            )
        self._refresh_views()

    def _on_click_qt(self, point) -> None:
        """Click while drag-select is armed: still selects a pin."""
        if self.ctx.document is None:
            return
        pick = self.ctx.scene.pick_at_qt(point.x(), point.y())
        if pick is not None:
            self._select_pin_from_pick(pick)

    def _select_pin_from_pick(self, pick) -> None:
        """Select the clicked pin in the list and focus the name box."""
        if self.ctx.document is None:
            return
        registry = self.ctx.window.pins
        row = -1
        for index, pin in enumerate(registry.pins):
            if pick.body_index in pin.body_ids or (
                (pick.body_index, pick.face_index) in pin.face_ids
            ):
                row = index
                break
        if row < 0 and registry.pins:
            # fall back to the nearest pin centroid to the click
            import numpy as np

            centroids = np.array([pin.centroid for pin in registry.pins])
            point3 = np.asarray(pick.world_point)
            distances = np.linalg.norm(centroids - point3, axis=1)
            nearest = int(np.argmin(distances))
            bounds = self.ctx.document.bounds()
            diag = np.linalg.norm(
                [bounds[1] - bounds[0], bounds[3] - bounds[2], bounds[5] - bounds[4]]
            )
            if distances[nearest] <= diag * 0.05:
                row = nearest
        if row < 0:
            return
        self.pin_list.setCurrentRow(row)
        self.pin_list.setFocus()  # so DEL removes this pin (not edit the name box)
        pin = registry.pins[row]
        self.status(
            f"Detect Pins: pin {pin.number}{f' ({pin.name})' if pin.name else ''} "
            f"selected — press Delete to remove, or type in the Name box to rename"
        )

    def _autocapitalize(self, text: str) -> None:
        if not self.autocap_button.isChecked():
            return
        upper = text.upper()
        if upper != text:
            position = self.name_edit.cursorPosition()
            self.name_edit.setText(upper)
            self.name_edit.setCursorPosition(position)

    # ---------------------------------------------------- naming / predict

    def _set_selected_name(self) -> None:
        registry = self.ctx.window.pins
        row = self.pin_list.currentRow()
        if not (0 <= row < len(registry.pins)):
            self.status("Detect Pins: select a pin in the list first")
            return
        pin = registry.pins[row]
        pin.name = self.name_edit.text().strip()
        pin.name_source = "anchor" if pin.name else ""
        self.ctx.journal.record(
            tool=self.id, params={"action": "rename"},
            inputs={"pin_number": pin.number, "centroid": list(pin.centroid)},
            result={"name": pin.name, "name_source": pin.name_source},
        )
        self._refresh_views()
        self.pin_list.setCurrentRow(row)
        self.status(
            f"Detect Pins: pin {registry.pins[row].number} named "
            f"'{registry.pins[row].name}' — name more or press Predict"
        )

    def predict_names(self) -> None:
        """Treat the manually named pins as ground truth and propagate the
        scheme to every other pin from the 3D positions."""
        registry = self.ctx.window.pins
        pins = registry.primaries()  # mouth designators come from the join below
        has_mouths = bool(
            registry.mouths() or any(p.role == "mouth" for p in self.pending)
        )
        anchors = {i: pin.name.strip() for i, pin in enumerate(pins) if pin.name.strip()}
        if not anchors:
            if has_mouths and pins:
                # nothing to predict on the primaries — Predict still joins
                # the mouth pins to the tails they line up with
                self._join_mouth_pins()
                return
            self.status("Detect Pins: name at least one pin first (e.g. A1)")
            return

        grid_anchors = {
            i: name for i, name in anchors.items() if parse_grid_name(name) is not None
        }
        numeric_anchors = {
            i: int(name) for i, name in anchors.items() if name.isdigit()
        }
        grid_like = bool(grid_anchors)
        numeric = not grid_like and bool(numeric_anchors)
        conflicting: list = []
        try:
            if grid_like:
                names = predict_grid_names(pins, grid_anchors, outliers=conflicting)
                # anchors (and any custom-named pins) keep what the user
                # typed; prediction fills in every non-anchored pin
                for index, pin in enumerate(pins):
                    if index not in anchors:
                        pin.name = names[index]
                        pin.name_source = "predicted"
                order = sorted(
                    range(len(pins)),
                    key=lambda i: parse_grid_name(pins[i].name) or (9999, i),
                )
                registry.set_pins([pins[i] for i in order] + registry.mouths())
                unknown = sum(1 for pin in pins if pin.name.startswith("?"))
                predicted = len(pins) - len(anchors)
                outcome = (
                    f"{predicted} non-anchored pin(s) predicted from "
                    f"{len(grid_anchors)} anchor(s)"
                )
                if conflicting:
                    outcome += (
                        f" — majority fit used; CHECK anchor(s) "
                        f"{', '.join(conflicting[:6])}"
                    )
                if unknown:
                    outcome += f" ({unknown} flagged '?' — check or delete those)"
            elif numeric:
                order = predict_serpentine_order(pins, numeric_anchors)
                if order is None:
                    self.status(
                        "Detect Pins: no serpentine numbering fits those pins — "
                        "check the numbers or use grid names"
                    )
                    return
                registry.set_pins([pins[i] for i in order] + registry.mouths())
                outcome = "serpentine numbering aligned to the named pins"
            else:
                self.status(
                    "Detect Pins: name at least one pin with a grid name (A1) "
                    "or a number first"
                )
                return
        except ValueError as exc:
            self.status(f"Detect Pins: predict failed — {exc}")
            return

        self.ctx.journal.record(
            tool=self.id,
            params={"action": "predict", "scheme": "grid" if grid_like else "numeric"},
            inputs={"anchors": {str(k): v for k, v in anchors.items()}},
            result={"names": [pin.name for pin in registry.pins],
                    "order": [pin.number for pin in registry.pins]},
        )
        self._refresh_views()
        if has_mouths:
            # the primaries just got their final designators — propagate them
            # into the mouth, exactly like running Join afterwards
            joined = self._join_mouth_pins()
            if joined:
                outcome += f"; {joined}"
        self.status(f"Detect Pins: {outcome}")

    def _journal_order(self) -> None:
        """Manual list edits change the numbering everything downstream keys
        on (hitboxes, functions, nets) — record the resulting order so replay
        reproduces it (matched by centroid, robust to renumbering)."""
        self.ctx.journal.record(
            tool=self.id, params={"action": "set_order"}, inputs={},
            result={"centroids": [list(p.centroid)
                                  for p in self.ctx.window.pins.pins]},
        )

    def _move_selected(self, delta: int) -> None:
        row = self.pin_list.currentRow()
        if row >= 0:
            new_row = self.ctx.window.pins.move(row, delta)
            self._journal_order()
            self._refresh_views()
            self.pin_list.setCurrentRow(new_row)

    def _reverse(self) -> None:
        self.ctx.window.pins.reverse()
        self._journal_order()
        self._refresh_views()

    def _make_pin1(self) -> None:
        row = self.pin_list.currentRow()
        if row >= 0:
            self.ctx.window.pins.make_pin1(row)
            self._journal_order()
            self._refresh_views()
            self.pin_list.setCurrentRow(0)

    def _delete_selected(self) -> None:
        row = self.pin_list.currentRow()
        registry = self.ctx.window.pins
        if 0 <= row < len(registry.pins):
            pin = registry.pins[row]
            self.ctx.journal.record(
                tool=self.id, params={"action": "delete"},
                inputs={"pin_number": pin.number,
                        "centroid": list(pin.centroid)},
                result={"remaining": len(registry.pins) - 1},
            )
            del registry.pins[row]
            registry.set_pins(registry.pins)
            self._refresh_views()

    def _clear(self) -> None:
        if self.ctx.window.pins.pins:
            self.ctx.journal.record(
                tool=self.id, params={"action": "clear"}, inputs={}, result={}
            )
        self.ctx.window.pins.clear()
        self.pending = []
        self._pending_bands = []
        self._last_band = None
        self._refresh_pending()
        self._refresh_views()

    # --------------------------------------------------------------- views

    def _on_list_selection(self, row: int) -> None:
        registry = self.ctx.window.pins
        if not (0 <= row < len(registry.pins)):
            self.ctx.scene.clear_highlight()
            return
        pin = registry.pins[row]
        self.name_edit.setText(pin.name)
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
            name = f" '{pin.name}'" if pin.name else ""
            mouth = " [CON/HEAD]" if pin.role == "mouth" else ""
            self.pin_list.addItem(
                f"Pin {pin.number}{name}{mouth}  "
                f"({pin.centroid[0]:+.2f}, {pin.centroid[1]:+.2f})  {kind}"
            )
        self._draw_pin_callouts(registry)
        self.canvas.set_pins(
            [(pin.centroid[0], pin.centroid[1], pin.name or str(pin.number))
             for pin in registry.pins]
        )
        if self._last_band is not None:
            self.canvas.set_band(
                (self._last_band.x_min, self._last_band.y_min,
                 self._last_band.x_max, self._last_band.y_max)
            )

    # Designation colours: orange THR/SMT tails, blue CON/HEAD mouths.
    _PRIMARY_COLOR = "#e07b00"
    _MOUTH_COLOR = "#2a6fd4"

    def _pin_normal(self, document, pin) -> np.ndarray:
        """Outward (area-weighted) normal of a pin's tip face(s) — the leader
        pops straight OUT along this, so it exits at the face and never runs
        through the body interior. +Z fallback for a degenerate face."""
        acc = np.zeros(3)
        for body_index, face_id in pin.face_ids:
            if not 0 <= body_index < len(document.bodies):
                continue
            mesh = document.bodies[body_index].mesh
            if mesh is None:
                continue
            tris = mesh.tris[mesh.tri_face_ids == face_id]
            if len(tris):
                corners = mesh.points[tris]
                acc += np.cross(corners[:, 1] - corners[:, 0],
                                corners[:, 2] - corners[:, 0]).sum(axis=0)
        norm = float(np.linalg.norm(acc))
        return acc / norm if norm > 1e-9 else np.array([0.0, 0.0, 1.0])

    def _draw_pin_callouts(self, registry) -> None:
        """Highlight every pin tip face (xray, on top) and draw a short leader
        straight OUT along the face normal to an off-the-face designator label
        — orange for THR/SMT tails, blue for CON/HEAD mouths — so leaders never
        run inside the body and the faces stay readable."""
        scene = self.ctx.scene
        pins = registry.pins
        document = self.ctx.document
        if not pins or document is None:
            scene.show_pin_faces({})
            scene.show_pin_axes([], [])
            scene.show_pin_labels([], [], [])
            return
        bounds = document.bounds()
        margin = 0.14 * float(np.linalg.norm([
            bounds[1] - bounds[0], bounds[3] - bounds[2], bounds[5] - bounds[4]]))
        segments, seg_colors, label_pts, label_colors, labels = [], [], [], [], []
        faces: dict[str, list] = {}
        for pin in pins:
            centroid = np.asarray(pin.centroid, dtype=float)
            label = centroid + self._pin_normal(document, pin) * margin
            color = (self._MOUTH_COLOR if pin.role == "mouth"
                     else self._PRIMARY_COLOR)
            segments.append([centroid, label])
            seg_colors.append(color)
            label_pts.append(label)
            label_colors.append(
                "#1f9d3a" if pin.name_source == "anchor" else color)
            labels.append(pin.name or str(pin.number))
            faces.setdefault(color, []).extend(pin.face_ids)
        scene.show_pin_faces(faces)
        scene.show_pin_axes(segments, seg_colors)
        scene.show_pin_labels(label_pts, labels, label_colors)

