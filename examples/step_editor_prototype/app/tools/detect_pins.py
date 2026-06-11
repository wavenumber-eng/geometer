"""Detect Pins: drag a rectangle over the pins in the orthographic top view.
Separate-solid pins are matched by their XY bounds; unibody parts use edge
flow until discontinuity (see app.pins). Detected pins are numbered
serpentine-style (or row-major for BGAs), honoring the pin-1 hint from the
Pin 1 Quadrant tool, and can be reordered manually."""

from __future__ import annotations

import numpy as np
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
            "<b>Detect Pins</b><br>"
            "Left button clicks and orbits as usual. Press <b>Drag Select</b>, "
            "then drag a see-through rectangle over the pins (top view). Each "
            "drag stages the pins it finds — press Apply to add them."
        ))

        select_row = QHBoxLayout()
        self.band_button = QPushButton("Drag Select (band)")
        self.band_button.setCheckable(True)
        self.band_button.setStyleSheet(
            "QPushButton:checked {background-color: #c89d00; color: #000000; font-weight: 700;}"
        )
        self.band_button.toggled.connect(self._arm_band)
        select_row.addWidget(self.band_button)
        self.context_button = QPushButton("Context Plane (Z+0.01)")
        self.context_button.setToolTip(
            "One click: slice the model 0.01 mm above the Z-sit seating plane "
            "(that work is already done) — every closed shape in the section "
            "is staged as a pin."
        )
        self.context_button.clicked.connect(self._run_context_plane)
        select_row.addWidget(self.context_button)
        self.seed_button = QPushButton("Seed Pin (click face)")
        self.seed_button.setCheckable(True)
        self.seed_button.setToolTip(
            "For parts the automatic detection can't solve: click a face on a "
            "pin tip/pad and it grows across smooth edges until a "
            "discontinuity — each click stages one pin."
        )
        self.seed_button.setStyleSheet(
            "QPushButton:checked {background-color: #c89d00; color: #000000; font-weight: 700;}"
        )
        self.seed_button.toggled.connect(self._on_seed_toggled)
        select_row.addWidget(self.seed_button)
        layout.addLayout(select_row)

        self.exclude_check = QCheckBox("Exclude largest body (package)")
        self.exclude_check.setChecked(True)
        layout.addWidget(self.exclude_check)

        mouth_row = QHBoxLayout()
        self.mouth_seed_button = QPushButton("Mouth Seed (click face)")
        self.mouth_seed_button.setCheckable(True)
        self.mouth_seed_button.setToolTip(
            "Second detector set for contacts that pop out inside the\n"
            "connector mouth: normal face picking — each clicked face is\n"
            "staged as a BLUE mouth pin, exactly the face you click."
        )
        self.mouth_seed_button.setStyleSheet(
            "QPushButton:checked {background-color: #2a6fd4; color: #ffffff; font-weight: 700;}"
        )
        self.mouth_seed_button.toggled.connect(self._on_mouth_seed_toggled)
        mouth_row.addWidget(self.mouth_seed_button)
        similar_button = QPushButton("Detect Similar")
        similar_button.setToolTip(
            "Find every face region that looks like the last mouth seed\n"
            "(same area and size) across the whole model — the other mouth\n"
            "contacts — and stage them in blue."
        )
        similar_button.clicked.connect(self._detect_similar)
        mouth_row.addWidget(similar_button)
        join_button = QPushButton("Join to Pins")
        join_button.setToolTip(
            "Give each blue mouth pin the designator of the primary pin it\n"
            "lines up with along the connector row — same designator = same\n"
            "net, so their hitboxes link as one electrical node."
        )
        join_button.clicked.connect(self._join_mouth_pins)
        mouth_row.addWidget(join_button)
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
            "then Predict propagates the scheme to every pin from their 3D positions."
        )
        predict_button.clicked.connect(self.predict_names)
        name_row.addWidget(predict_button)
        layout.addLayout(name_row)

        self.pending_label = QLabel("")
        self.pending_label.setStyleSheet("color: #c97a1a; font-weight: 600;")
        layout.addWidget(self.pending_label)
        self.apply_button = make_apply_button("Apply Detected Pins")
        self.apply_button.setEnabled(False)
        self.apply_button.clicked.connect(self.apply)
        layout.addWidget(self.apply_button)

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
            "Detect Pins: press Drag Select then sweep over the pins; "
            "plain clicks select pins for renaming"
        )

    def exit(self) -> None:
        self._disarm_band()
        if self._actions_widget is not None:
            self.band_button.setChecked(False)
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
            self.band_button.setChecked(False)
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
        if role == "mouth":
            # normal face picking: the clicked face IS the mouth pin —
            # no growth, the marker sits exactly where the user clicked
            region = {pick.face_index}
        else:
            region = grow_smooth_region(
                document,
                pick.body_index,
                pick.face_index,
                smooth_angle_deg=float(self.angle_spin.value()),
                claimed=claimed,
            )
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
            params={"action": "seed_pin", "role": role,
                    "smooth_angle_deg": float(self.angle_spin.value())},
            inputs={"body": pick.body_index, "face": pick.face_index,
                    "point": list(pick.world_point)},
            result={"faces": len(region), "centroid": list(centroid)},
        )
        self._refresh_pending()
        if role == "mouth":
            self.status(
                f"Mouth Seed: staged the clicked face — {len(self.pending)} "
                f"pending; keep clicking, then Detect Similar"
            )
        else:
            self.status(
                f"Seed Pin: grew {len(region)} face(s) from the click — "
                f"{len(self.pending)} pending; keep clicking or press Apply"
            )

    # ----------------------------------------------------------- mouth pins

    def _detect_similar(self) -> None:
        """Stage a blue mouth pin for every region that looks like the last
        mouth seed — the other contacts inside the mouth."""
        document = self.ctx.document
        if document is None:
            return
        registry = self.ctx.window.pins
        mouths = [p for p in self.pending if p.role == "mouth"] or registry.mouths()
        if not mouths:
            self.status("Detect Similar: stage a Mouth Seed first (blue button)")
            return
        template = mouths[-1]
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
                             face_ids=region, role="mouth"))
        self.pending.extend(added)
        self.ctx.journal.record(
            tool=self.id,
            params={"action": "detect_similar",
                    "smooth_angle_deg": float(self.angle_spin.value())},
            inputs={"body": seed_body,
                    "faces": sorted(seed_region)},
            result={"matches": len(added),
                    "centroids": [list(pin.centroid) for pin in added]},
        )
        self._refresh_pending()
        self.status(
            f"Detect Similar: {len(added)} matching region(s) staged in blue — "
            f"{len(self.pending)} pending; press Apply, then Join to Pins"
        )

    def _join_mouth_pins(self) -> None:
        """Pair each mouth pin with the primary pin it lines up with along
        the connector row and inherit its designator (same designator = same
        net, hitboxes linked)."""
        registry = self.ctx.window.pins
        primaries = registry.primaries()
        mouths = registry.mouths() + [p for p in self.pending if p.role == "mouth"]
        if not mouths:
            self.status("Join: no mouth pins — stage some with Mouth Seed first")
            return
        if not primaries:
            self.status("Join: detect and Apply the primary (tail) pins first")
            return
        assigned, conflicts = join_mouth_pins(primaries, mouths)
        for index, primary in assigned.items():
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
        outcome = f"Join: {len(assigned)} mouth pin(s) inherited designators"
        if conflicts:
            outcome += f" — {len(conflicts)} had no free primary in line; check those"
        self.status(outcome)

    # --------------------------------------------------------- context plane

    def _run_context_plane(self) -> None:
        """One click: slice 0.01 mm above the Z-sit seating plane and stage
        every closed shape as a pin — the user already defined that frame."""
        document = self.ctx.document
        if document is None:
            return
        self.band_button.setChecked(False)
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
        # one band per arm: hand the left button back to orbit/click
        if self._actions_widget is not None:
            self.band_button.setChecked(False)

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
        self.name_edit.setFocus()
        self.name_edit.selectAll()
        pin = registry.pins[row]
        self.status(
            f"Detect Pins: pin {pin.number}{f' ({pin.name})' if pin.name else ''} "
            f"selected — type a name and press Enter"
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
        pins = registry.primaries()  # mouth pins are named by Join, not Predict
        anchors = {i: pin.name.strip() for i, pin in enumerate(pins) if pin.name.strip()}
        if not anchors:
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
            mouth = " [mouth]" if pin.role == "mouth" else ""
            self.pin_list.addItem(
                f"Pin {pin.number}{name}{mouth}  "
                f"({pin.centroid[0]:+.2f}, {pin.centroid[1]:+.2f})  {kind}"
            )
        points = [pin.centroid for pin in registry.pins]
        labels = [pin.name or str(pin.number) for pin in registry.pins]
        label_colors = [
            "#2a6fd4" if pin.role == "mouth"
            else "#1f9d3a" if pin.name_source == "anchor"
            else "#e07b00" if pin.name_source == "predicted"
            else "#1a1a1a"
            for pin in registry.pins
        ]
        self.ctx.scene.show_pin_labels(points, labels, label_colors)
        self.canvas.set_pins(
            [(pin.centroid[0], pin.centroid[1], pin.name or str(pin.number))
             for pin in registry.pins]
        )
        if self._last_band is not None:
            self.canvas.set_band(
                (self._last_band.x_min, self._last_band.y_min,
                 self._last_band.x_max, self._last_band.y_max)
            )

