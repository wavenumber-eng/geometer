"""Detect Pins: drag a rectangle over the pins in the orthographic top view.
Separate-solid pins are matched by their XY bounds; unibody parts use edge
flow until discontinuity (see app.pins). Detected pins are numbered
serpentine-style (or row-major for BGAs), honoring the pin-1 hint from the
Pin 1 Quadrant tool, and can be reordered manually."""

from __future__ import annotations

import numpy as np
from PySide6.QtCore import QEvent, QObject, Qt
from PySide6.QtGui import QKeySequence, QShortcut
from PySide6.QtGui import QAction
from PySide6.QtWidgets import (
    QCheckBox,
    QComboBox,
    QDoubleSpinBox,
    QHBoxLayout,
    QLabel,
    QLineEdit,
    QListWidget,
    QMenu,
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
    join_mouth_pins,
    mesh_region_centroid,
    PIN_PATTERNS,
    order_pins,
    a1_anchor,
    contact_row_order,
    marker_corner,
    parse_grid_name,
    pin_pattern,
    predict_grid_names,
    predict_remaining_numbers,
    section_segments,
    set_specialty,
    specialty,
)
from ..scene import BandSelector
from .base import ToolMode

# Area cutoff for SMT/THR seed growth — matches Separate's default factor so
# the seed isolates one contact (not the whole housing) on connectors whose
# contacts meet the body tangent-continuously. See grow_smooth_region.
SEED_AREA_FACTOR = 4.0

# Detect EXACT tolerances: tight area + bbox match (vs find_similar_regions'
# loose 0.12/0.25 defaults) so it copies only the SAME tip shape, in any
# orientation — the basis for hand-building a reference file.
_EXACT_AREA_TOL = 0.06
_EXACT_DIM_TOL = 0.08


class _NameBoxKeys(QObject):
    """Delete in the Name box removes the selected pin when the box is empty
    or fully selected (clicking a pin focuses the Name box for typing)."""

    def __init__(self, tool) -> None:
        super().__init__()
        self.tool = tool

    def eventFilter(self, obj, event) -> bool:  # noqa: N802 (Qt override)
        if event.type() == QEvent.Type.KeyPress and event.key() == Qt.Key.Key_Delete:
            text = obj.text()
            if not text or obj.selectedText() == text:
                self.tool._delete_selected()
                return True
        return False


def _magnetics_hint(contacts: int, primaries: list) -> str:
    """A note when contacts and tails can't pair one to one: on a MagJack the
    contacts go through transformers and are on no PCB net."""
    tails = sum(1 for p in primaries if not specialty(p))
    if contacts == tails:
        return ""
    return (f" — {contacts} contacts vs {tails} tails: if the part has magnetics (MagJack), "
            f"the contacts are isolated: use 'Isolated contacts' instead")


def _typed(pin) -> bool:
    """A name the user typed (or took from a footprint), not one the editor
    predicted or renumbered: only these steer prediction."""
    return pin.name_source in ("anchor", "footprint") and bool(pin.name.strip())


def _row_kind(role: str) -> str:
    """The panel row a pin role belongs to."""
    return "CON/HEAD" if role == "mouth" else "SMT/THR"


class DetectPinsTool(ToolMode):
    id = "detect_pins"
    title = "4 Detect + Designate Pins"
    accent = "#2a7a2a"

    def __init__(self, ctx) -> None:
        super().__init__(ctx)
        self._band_selector: BandSelector | None = None
        self._last_band: Band | None = None
        self.pending: list[Pin] = []
        self._pending_bands: list[list[float]] = []
        self._clicked: set[int] = set()  # id() of pins staged by a tip click
        self._last_seed: Pin | None = None  # the tip just clicked: Enter names it
        self._groups: dict[str, list[list[Pin]]] = {}  # Select Group: candidate groups per role

    # ------------------------------------------------------------------- UI

    def build_actions_widget(self) -> QWidget:
        widget = QWidget()
        layout = QVBoxLayout(widget)
        layout.setContentsMargins(8, 8, 8, 8)
        # ---- SMT / THR: Seed Tip | Detect Exact | Select Group ----
        smt_label = QLabel("SMT / THR")
        smt_label.setStyleSheet("font-weight: 700; color: #c89d00; padding-top: 4px;")
        layout.addWidget(smt_label)
        smt_row = QHBoxLayout()
        self.seed_button = QPushButton("Seed Tip")
        self.seed_button.setCheckable(True)
        self.seed_button.setToolTip(
            "Click a pin TIP (the end face of an SMT/THR pad: every pin has\n"
            "one, square/round/curved). Each click stages one pin; then press\n"
            "Detect Exact or Select Group to find the rest.")
        self.seed_button.setStyleSheet(
            "QPushButton:checked {background-color: #c89d00; color: #000000; font-weight: 700;}")
        self.seed_button.toggled.connect(self._on_seed_toggled)
        smt_row.addWidget(self.seed_button)
        smt_similar_button = QPushButton("Detect Exact")
        smt_similar_button.setToolTip(
            "Stage every face of the SAME SHAPE as the last tip, in any\n"
            "orientation (exact area + bbox match): the other pads.")
        smt_similar_button.clicked.connect(lambda: self._detect_similar("primary"))
        smt_row.addWidget(smt_similar_button)
        self.group_button = self._group_button("primary")
        smt_row.addWidget(self.group_button)
        layout.addLayout(smt_row)

        # ---- CON / HEAD: Seed Tip | Detect Exact | Join Pins (same layout) ----
        con_label = QLabel("CON / HEAD")
        con_label.setStyleSheet("font-weight: 700; color: #2a6fd4; padding-top: 4px;")
        layout.addWidget(con_label)
        mouth_row = QHBoxLayout()
        self.mouth_seed_button = QPushButton("Seed Tip")
        self.mouth_seed_button.setCheckable(True)
        self.mouth_seed_button.setToolTip(
            "Click a CON/HEAD contact TIP inside the connector mouth: each\n"
            "clicked face is staged as a BLUE mouth pin.")
        self.mouth_seed_button.setStyleSheet(
            "QPushButton:checked {background-color: #2a6fd4; color: #ffffff; font-weight: 700;}")
        self.mouth_seed_button.toggled.connect(self._on_mouth_seed_toggled)
        mouth_row.addWidget(self.mouth_seed_button)
        similar_button = QPushButton("Detect Exact")
        similar_button.setToolTip("Stage every face of the SAME SHAPE as the last mouth tip (blue).")
        similar_button.clicked.connect(lambda: self._detect_similar("mouth"))
        mouth_row.addWidget(similar_button)
        self.mouth_group_button = self._group_button("mouth")
        mouth_row.addWidget(self.mouth_group_button)
        layout.addLayout(mouth_row)
        join_button = QPushButton("Join Pins")
        join_button.setToolTip(
            "Give each blue mouth pin the designator of the tail pin it lines\n"
            "up with (same designator = same net, hitboxes linked).")
        join_button.setMinimumHeight(34)
        join_button.setStyleSheet("font-weight: 700;")
        join_button.clicked.connect(self._join_mouth_pins)
        layout.addWidget(join_button)
        isolated_row = QHBoxLayout()
        isolated_button = QPushButton("Isolated contacts (magnetics): J1…Jn")
        isolated_button.setToolTip(
            "For jacks with built-in magnetics (MagJack, RJ45 with transformers):\n"
            "the contacts in the opening are NOT wired to any PCB pin, so they must\n"
            "not be joined. Numbers them J1…Jn along their row, each on its own net.\n"
            "J1 goes at the +X end (right when looking into a tab-down jack opening\n"
            "toward -Y); tick 'J1 at the other end' to flip.")
        isolated_button.clicked.connect(self._isolated_contacts)
        isolated_row.addWidget(isolated_button, 1)
        self.isolated_flip = QCheckBox("J1 at the other end")
        isolated_row.addWidget(self.isolated_flip)
        layout.addLayout(isolated_row)

        # detection parameters: Settings > Tool Parameter Defaults
        self.exclude_check = QCheckBox("Exclude largest body (package)")
        self.exclude_check.setChecked(True)
        self.flow_combo = QComboBox()
        self.flow_combo.addItems(["any-edge", "smooth-only"])
        self.angle_spin = QDoubleSpinBox()
        self.angle_spin.setRange(1.0, 89.0)
        self.angle_spin.setValue(30.0)
        self.order_combo = QComboBox()
        self.order_combo.addItems(["serpentine", "row-major", "bga-grid (A1..)"])
        self.autocap_button = QPushButton("aA")
        self.autocap_button.setCheckable(True)
        self.autocap_button.setChecked(True)
        for hidden in (self.exclude_check, self.flow_combo, self.angle_spin, self.order_combo,
                       self.autocap_button):
            hidden.setVisible(False)
            layout.addWidget(hidden)

        # ---- designate: Name | Set | Predict (with the ordering choices) ----
        name_row = QHBoxLayout()
        name_row.addWidget(QLabel("Name"))
        self.name_edit = QLineEdit()
        self.name_edit.setPlaceholderText("e.g. A1 or 5")
        self.name_edit.setMinimumWidth(80)
        self.name_edit.returnPressed.connect(self._set_selected_name)
        self.name_edit.textEdited.connect(self._autocapitalize)
        self._name_keys = _NameBoxKeys(self)
        self.name_edit.installEventFilter(self._name_keys)
        name_row.addWidget(self.name_edit, 1)
        set_name_button = QPushButton("Set")
        set_name_button.setToolTip("Give the selected pin this name (Enter)")
        set_name_button.clicked.connect(self._set_selected_name)
        name_row.addWidget(set_name_button)
        predict_button = QPushButton("Predict")
        predict_button.setToolTip(
            "Predict: number the pins that have no name yet, continuing from\n"
            "the ones you typed. The menu also renumbers everything in a fixed\n"
            "order when prediction keeps failing.")
        predict_menu = QMenu(predict_button)
        predict_action = QAction("Predict the unnamed pins", predict_menu)
        predict_action.triggered.connect(self.predict_names)
        predict_menu.addAction(predict_action)
        predict_menu.addSeparator()
        for label, order in (("Renumber: serpentine", "serpentine"), ("Renumber: row by row", "row-major"),
                             ("Renumber: BGA grid (A1, A2 ...)", "bga-grid (A1..)")):
            action = QAction(label, predict_menu)
            action.triggered.connect(lambda _c=False, o=order: self._renumber_as(o))
            predict_menu.addAction(action)
        predict_button.setMenu(predict_menu)
        name_row.addWidget(predict_button)
        self.pattern_combo = QComboBox()
        for key, text in PIN_PATTERNS.items():
            self.pattern_combo.addItem(text, key)
        self.pattern_combo.setToolTip(
            "How the pins are numbered, by their layout:\n"
            "RING: QFN / QFP, around the package from pin 1, centre pad last\n"
            "TWO ROW: SOT / SOIC / DIP, down one row and back up the other\n"
            "LINE: one row, end to end\n"
            "GRID: BGA / LGA, row by row (A1, A2 ...)\n"
            "Set from the pins' XY layout when they are detected; change it if it is wrong.")
        self._pattern_for: tuple | None = None  # the pin set the pattern was detected on
        name_row.addWidget(self.pattern_combo)
        self.live_check = QCheckBox("Live")
        self.live_check.setToolTip(
            "Live predict: every number you lock in re-predicts the pins you have not\n"
            "typed (your typed numbers are never changed). Saved in Settings.")
        self.live_check.setChecked(bool(self.ctx.window.settings.get("live_predict", True)))
        self.live_check.toggled.connect(self._save_live)
        name_row.addWidget(self.live_check)
        layout.addLayout(name_row)

        special_row = QHBoxLayout()
        special_row.addWidget(QLabel("Mark"))
        for label, kind, key, tip in (
            ("A", "anchor", "A", "ANCHOR: casing / mechanical pin soldered into a PLATED hole\n"
                                 "(on a net, gets a hitbox, stays with the body, never numbered)."),
            ("G", "guide", "G", "GUIDE: plastic locating peg through a NON-PLATED hole\n"
                                "(no net, no hitbox, stays with the body)."),
            ("T", "therm", "T", "THERMAL: the exposed thermal pad (named THERM, THERM_2 ...).\n"
                                "The footprint check confirms it sits on the big pad."),
            ("Unclassified", "", "N", "Back to a plain numbered pin."),
        ):
            button = QPushButton(label)
            button.setMinimumWidth(28)
            button.setToolTip(tip + f"\nHotkey {key} with a pin selected in the list.")
            button.clicked.connect(lambda _c=False, k=kind: self._mark_selected(k))
            special_row.addWidget(button)
        layout.addLayout(special_row)

        footprint_button = QPushButton("Number from footprint")
        footprint_button.setToolTip(
            "The footprint's pads define the real board-side pins: pair each\n"
            "SMT/THT pin with the pad under it and take that pad's number;\n"
            "mating contacts follow their tail. Uses the embedded footprint\n"
            "(step 9), the opened .kicad_mod, or one you pick.")
        footprint_button.clicked.connect(self.number_from_footprint)
        layout.addWidget(footprint_button)

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

        # adds the staged pins and stays in the tool; CONFIRM finishes the tool
        self.apply_button = QPushButton("Add Staged Pins")
        self.apply_button.setToolTip("Add the staged pins to the pin list (you stay in Detect Pins;\n"
                                     "CONFIRM finishes the tool and moves on)")
        self.apply_button.setEnabled(False)
        self.apply_button.clicked.connect(self.add_staged)
        layout.addWidget(self.apply_button)

        self.pin_list = QListWidget()
        self.pin_list.currentRowChanged.connect(self._on_list_selection)
        self.pin_list.itemClicked.connect(lambda _item: setattr(self, "_last_seed", None))  # a picked pin wins
        layout.addWidget(self.pin_list, 2)
        # DEL removes the selected pin (selecting a pin's faces in the 3D view
        # focuses this list) — fires only when the list, not the name box, has
        # focus, so typing in Name still edits text normally.
        delete_shortcut = QShortcut(QKeySequence(Qt.Key_Delete), self.pin_list)
        delete_shortcut.activated.connect(self._delete_selected)
        for key, kind in (("A", "anchor"), ("G", "guide"), ("T", "therm"), ("N", "")):
            QShortcut(QKeySequence(key), self.pin_list).activated.connect(
                lambda k=kind: self._mark_selected(k))

        edit_row = QHBoxLayout()
        for label, handler in (
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
        self.canvas.on_request = lambda: self.ctx.window.request_footprint(self._on_footprint, force=True)
        self.canvas.waiting = not self.ctx.window.request_footprint(self._on_footprint)
        self.canvas.update()
        self._refresh_views()
        self.status(
            "Detect Pins: Seed Tip, then Detect Exact or Select Group (or AUTO); click a pin "
            "to select it, Delete removes it, Shift+click drops a staged pin"
        )

    def exit(self) -> None:
        self._link_own_solids()
        self._disarm_band()
        self.ctx.scene.show_pin_faces({})
        self.ctx.scene.show_pin_axes([], [])
        self.ctx.scene.show_pin_labels([], [])
        self.ctx.scene.set_markers([], name="pending-pins")
        self.ctx.scene.set_markers([], name="pending-mouth")
        self.ctx.scene.remove_overlay("context-section")
        self.ctx.scene.clear_highlight()

    def _link_own_solids(self) -> None:
        """Leaving the step: pins on a lead that is already its own solid are
        that solid (Separate has nothing to split on such a model)."""
        from ..pins import apply_pin_body_names, link_own_solids

        document, registry = self.ctx.document, self.ctx.window.pins
        if document is not None and link_own_solids(document, registry.pins):
            apply_pin_body_names(document, registry.pins)
            self.ctx.window.document_mutated()

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
        otherwise select that pin for quick rename. Shift+click on a staged
        pin's face drops it from the staged set."""
        if pick.shift:
            key = (pick.body_index, pick.face_index)
            hit = [p for p in self.pending if key in p.face_ids]
            if hit:
                self.pending = [p for p in self.pending if p not in hit]
                self.ctx.journal.record(
                    tool=self.id, params={"action": "select_group", "on": False},
                    inputs={"regions": [[list(pair) for pair in pin.face_ids] for pin in hit]},
                    result={"pending": len(self.pending)})
                self._refresh_pending()
                self.status(f"Detect Pins: dropped {len(hit)} staged pin(s); {len(self.pending)} pending")
                return
        if self._actions_widget is not None and self.mouth_seed_button.isChecked():
            self._seed_pin_from_pick(pick, role="mouth")
            return
        if self._actions_widget is not None and self.seed_button.isChecked():
            self._seed_pin_from_pick(pick)
            return
        self._select_pin_from_pick(pick)

    def _is_known_pin(self, pick) -> bool:
        """The clicked face belongs to a pin already listed or staged (its
        face region, or its own solid)."""
        pins = (*self.ctx.window.pins.pins, *self.pending)
        return any((pick.body_index, pick.face_index) in pin.face_ids or pick.body_index in pin.body_ids
                   for pin in pins)

    def _seed_pin_from_pick(self, pick, role: str = "primary") -> None:
        document = self.ctx.document
        if document is None:
            return
        if self._is_known_pin(pick):
            # A defined pin (its faces, or its own solid): select it for renaming right away.
            self._select_pin_from_pick(pick)
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
        self._clicked.add(id(pin))
        self.ctx.journal.record(
            tool=self.id,
            params={"action": "seed_pin", "role": role, "grow": False},
            inputs={"body": pick.body_index, "face": pick.face_index,
                    "point": list(pick.world_point)},
            result={"faces": len(region), "centroid": list(centroid)},
        )
        self._refresh_pending()
        self._last_seed = pin
        self._focus_name()
        kind = "CON/HEAD" if role == "mouth" else "SMT/THR"
        self.status(
            f"{kind} Seed: staged the pin tip — {len(self.pending)} pending; type its number + Enter, "
            f"keep clicking tips, or press Detect Exact to find the rest"
        )

    # ----------------------------------------------------------- mouth pins

    def _tip_templates(self, role: str) -> list:
        """Tips seeded from a face, staged first then listed (whole-solid
        pins have no faces to match on)."""
        def tips(pins):
            return [p for p in pins if p.role == role and p.face_ids]
        return tips(self.pending) or tips(self.ctx.window.pins.pins)

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
        same = self._tip_templates(role)
        if not same:
            kind = "CON/HEAD" if role == "mouth" else "SMT/THR"
            self.status(f"Detect Exact: stage a {kind} tip first (Seed Tip, click one tip face)")
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

    def _isolated_contacts(self) -> None:
        """Name the mouth contacts J1…Jn along their row, on their own nets."""
        registry = self.ctx.window.pins
        mouths = registry.mouths() + [p for p in self.pending if p.role == "mouth"]
        if not mouths:
            self.status("Isolated contacts: stage the CON/HEAD contacts first (Seed Tip, Detect Exact)")
            return
        flip = self.isolated_flip.isChecked() if self._actions_widget is not None else False
        order = contact_row_order(mouths, flip=flip)
        for position, index in enumerate(order, start=1):
            mouths[index].name = f"J{position}"
            mouths[index].name_source = "isolated"
        self.ctx.journal.record(
            tool=self.id, params={"action": "isolated_contacts", "flip": flip}, inputs={},
            result={"centroids": [list(mouths[i].centroid) for i in order],
                    "names": [mouths[i].name for i in order]})
        self._refresh_views()
        self._refresh_pending()
        self.status(f"Isolated contacts: J1…J{len(mouths)} named (own nets, not joined to any tail)")

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
        outcome += _magnetics_hint(len(mouths), primaries)
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
        self._clicked = set()
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
        # Staged pins show as their actual faces, drawn through the model
        # (whole-body pins, which have no faces, fall back to a marker).
        self.ctx.scene.set_markers(
            [pin.centroid for pin in self.pending if not pin.face_ids],
            name="pending-pins", color="#ff9a2a", point_size=18.0,
        )
        self.ctx.scene.set_markers([], name="pending-mouth")
        self._draw_pin_callouts(self.ctx.window.pins)
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
        """CONFIRM: finish Detect Pins (any still-staged pins are added first)."""
        if self.pending:
            self.add_staged()

    def add_staged(self) -> None:
        if not self.pending:
            self.status("Detect Pins: nothing staged — drag a band first")
            return
        registry = self.ctx.window.pins
        added = self._click_then_geometric(self.pending)
        self.pending = []
        self._clicked = set()
        bands = self._pending_bands
        self._pending_bands = []
        # Existing pins keep their numbers; new ones follow in the order they
        # were clicked (Renumber re-sorts everything geometrically on demand).
        registry.set_pins(registry.pins + added)
        self._refresh_views()
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
        self._journal_order()  # replay keeps the click order too
        self._refresh_pending()
        self.status(f"Detect Pins: applied {len(added)} pins, {len(registry.pins)} total")

    def _click_then_geometric(self, pins: list[Pin]) -> list[Pin]:
        """Clicked pins in click order, then everything detected in bulk in
        serpentine order (that set has no click order to honour)."""
        clicked = [p for p in pins if id(p) in self._clicked]
        bulk = [p for p in pins if id(p) not in self._clicked]
        if len(bulk) > 1:
            hint = self.ctx.window.pin1_hint
            primaries = [p for p in bulk if p.role != "mouth"]
            order = order_pins(primaries, mode=self._ordering_mode(),
                               pin1_hint=(hint[0], hint[1]) if hint else None)
            bulk = [primaries[i] for i in order] + [p for p in bulk if p.role == "mouth"]
        return clicked + bulk

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
            pattern=self._pattern(),
        )
        registry.set_pins([primaries[i] for i in order] + registry.mouths())
        if mode == "bga-grid":
            try:
                primaries = registry.primaries()
                names = predict_grid_names(primaries, self._grid_anchors(primaries))
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
        self._last_seed = None  # a picked pin wins over an earlier tip click
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
        # Ready to type: the cursor goes straight into the Name box (Delete on
        # an empty / fully selected box still removes the pin).
        self._focus_name()
        pin = registry.pins[row]
        self.status(
            f"Detect Pins: pin {pin.number}{f' ({pin.name})' if pin.name else ''} "
            f"selected — type a name + Enter, A / G buttons for ANCHOR / GUIDE, "
            f"Delete to remove"
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

    def _focus_name(self) -> None:
        """Put the cursor in the Name box AFTER the click finishes: the 3D
        view takes focus back on mouse release, which left typed numbers and
        Enter going nowhere."""
        from PySide6.QtCore import QTimer

        def focus() -> None:
            self.name_edit.setFocus()
            self.name_edit.selectAll()

        focus()
        QTimer.singleShot(0, focus)

    def apply_staged(self) -> None:
        """Enter: a number typed for the tip just clicked names it (= Set);
        otherwise add the staged pins and stay in Detect Pins."""
        if self._last_seed is not None and self.name_edit.text().strip():
            self._set_selected_name()
        elif self.pending:
            self.add_staged()
        else:
            self.status("Detect Pins: nothing staged (drag a band); CONFIRM finishes the tool")

    def _save_live(self, on: bool) -> None:
        from .. import settings_store

        self.ctx.window.settings["live_predict"] = bool(on)
        settings_store.save(self.ctx.window.settings)

    def _select_last_seed(self) -> None:
        """A tip was just clicked: add the staged pins and select that pin,
        so the typed name lands on it."""
        seed, self._last_seed = self._last_seed, None
        if seed is None or seed not in self.pending or not self.name_edit.text().strip():
            return
        typed = self.name_edit.text()
        self.add_staged()
        registry = self.ctx.window.pins
        if seed in registry.pins:
            self.pin_list.setCurrentRow(registry.pins.index(seed))
        self.name_edit.setText(typed)  # selecting the new pin filled in its (empty) name

    def _set_selected_name(self) -> None:
        self._select_last_seed()
        registry = self.ctx.window.pins
        row = self.pin_list.currentRow()
        if not (0 <= row < len(registry.pins)):
            self.status("Detect Pins: select a pin in the list first")
            return
        pin = registry.pins[row]
        pin.name = self.name_edit.text().strip()
        pin.name_source = "anchor" if pin.name else ""
        set_specialty(pin, specialty(Pin(0, (0, 0, 0), name=pin.name)))
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
        if pin.name.isdigit() and self.live_check.isChecked():
            self._live_predict()

    def _live_predict(self) -> None:
        """Re-predict every pin the person has not typed, from all the typed
        numbers so far (earlier predictions are redone, typed ones kept)."""
        for pin in self.ctx.window.pins.primaries():
            if pin.name_source == "predicted":
                pin.name = ""
        self.predict_names()

    def predict_names(self) -> None:
        """Treat the manually named pins as ground truth and propagate the
        scheme to every other pin from the 3D positions."""
        registry = self.ctx.window.pins
        pins = registry.primaries()  # mouth designators come from the join below
        has_mouths = bool(
            registry.mouths() or any(p.role == "mouth" for p in self.pending)
        )
        anchors = {i: pin.name.strip() for i, pin in enumerate(pins) if pin.name.strip()}
        grid_anchors = self._grid_anchors(pins)
        grid_like = bool(grid_anchors)
        numeric = not grid_like
        if numeric and not any(not p.name.strip() for p in pins):
            if has_mouths and pins:
                # nothing left to number — Predict still joins the mouth pins
                self._join_mouth_pins()
                return
            self.status("Detect Pins: every pin already has a name — nothing to predict")
            return
        conflicting: list = []
        try:
            if grid_like:
                names = predict_grid_names(pins, grid_anchors, outliers=conflicting)
                # names the user TYPED stay; every other pin (including the
                # editor's own earlier guesses) gets the grid name
                for index, pin in enumerate(pins):
                    if not _typed(pin):  # an automatic A1 anchor is renamed too
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
            else:
                hint = self.ctx.window.pin1_hint
                numbers = predict_remaining_numbers(
                    pins, pin1_hint=(hint[0], hint[1]) if hint else None, pattern=self._pattern())
                for index, number in numbers.items():
                    pins[index].name = str(number)
                    pins[index].name_source = "predicted"

                def sort_key(i):
                    name = pins[i].name.strip()
                    if specialty(pins[i]):
                        return (2, 0, i)  # ANCHOR / GUIDE after the numbered pins
                    return (0, int(name), i) if name.isdigit() else (1, 0, i)

                order = sorted(range(len(pins)), key=sort_key)
                registry.set_pins([pins[i] for i in order] + registry.mouths())
                kept = sum(1 for i in anchors if not specialty(pins[i]))
                outcome = (f"numbered {len(numbers)} remaining pin(s) after "
                           f"{kept} you defined (ANCHOR / GUIDE untouched)")
        except ValueError as exc:
            self.status(f"Detect Pins: predict failed — {exc}")
            return

        self.ctx.journal.record(
            tool=self.id,
            params={"action": "predict", "scheme": "grid" if grid_like else "numeric",
                    "pattern": self._pattern()},
            inputs={"anchors": {str(k): v for k, v in anchors.items()}},
            result={"names": [pin.name for pin in registry.pins],
                    "order": [pin.number for pin in registry.pins],
                    "centroids": [list(pin.centroid) for pin in registry.pins]},
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

    def _footprint_pads(self) -> list[dict] | None:
        """Pads from the embedded footprint, the opened .kicad_mod, or a file."""
        from pathlib import Path

        from PySide6.QtWidgets import QFileDialog

        from .footprint_embed import parse_kicad_mod

        document = self.ctx.document
        fp = getattr(document, "footprint", None) or {}
        if fp.get("pads"):
            return fp["pads"]
        vendor = getattr(self.ctx.window, "vendor", None)
        if vendor is not None and Path(vendor.source).suffix.lower() == ".kicad_mod":
            return parse_kicad_mod(Path(vendor.source).read_text(encoding="utf-8", errors="replace"))["pads"]
        path, _ = QFileDialog.getOpenFileName(self._actions_widget, "Footprint for pin numbers",
                                              str(document.path.parent), "KiCad footprint (*.kicad_mod)")
        if not path:
            return None
        return parse_kicad_mod(Path(path).read_text(encoding="utf-8", errors="replace"))["pads"]

    def _journal_numbering(self, registry, pad_count: int, result: dict, problems: list) -> None:
        """Record the footprint numbering; names go with their pins' places so
        replay puts each on the right pin whatever order it detects them in."""
        pins = registry.pins
        self.ctx.journal.record(
            tool=self.id, params={"action": "number_from_footprint"},
            inputs={"pads": pad_count},
            result={"status": result["status"], "rotation": result["rotation"], "rms": result["rms"],
                    "names": [p.name for p in pins], "order": [p.number for p in pins],
                    "centroids": [list(p.centroid) for p in pins], "problems": problems},
        )

    def number_from_footprint(self) -> None:
        """The footprint is the official decider: each board-side pin takes its
        pad's number, or ANCHOR / GUIDE for a mechanical pad / non-plated hole."""
        from ..pins import footprint_numbering

        registry = self.ctx.window.pins
        if self.ctx.document is None or not registry.pins:
            self.status("Number from footprint: detect the pins first")
            return
        pads = self._footprint_pads()
        if not pads:
            self.status("Number from footprint: no footprint pads available")
            return
        result = footprint_numbering(registry.pins, pads)
        if not result.get("assign"):
            self.status(f"Number from footprint: {result['status']}")
            return
        if result["status"] == "corrected":
            ordered = sorted(registry.primaries(), key=lambda p: (
                bool(specialty(p)), not p.name.isdigit(), int(p.name) if p.name.isdigit() else 0, p.name))
            registry.set_pins(ordered + registry.mouths())
        problems = []
        if result["pins_without_pad"]:
            problems.append(f"pins with no pad: {', '.join(result['pins_without_pad'][:6])}")
        if result["pads_without_pin"]:
            problems.append(f"pads with no pin: {', '.join(result['pads_without_pin'][:6])}")
        self._journal_numbering(registry, len(pads), result, problems)
        self._refresh_views()
        changed = ", ".join(f"{old}→{new[1] or new[0].upper()}" for old, _cur, new in result["changed"][:8])
        self.status(f"Number from footprint: {result['status']}"
                    + (f" ({changed})" if result["status"] == "corrected" and changed else "")
                    + (" — " + "; ".join(problems) if problems else ""))

    def _mark_selected(self, kind: str) -> None:
        """Mark the selected pin ANCHOR / GUIDE (or back to a plain pin)."""
        registry = self.ctx.window.pins
        row = self.pin_list.currentRow()
        if not (0 <= row < len(registry.pins)):
            self.status("Detect Pins: select a pin first (click its face)")
            return
        pin = registry.pins[row]
        if kind == "therm":  # a thermal pad: a real (netted) pin named THERM, THERM_2 ...
            set_specialty(pin, "")
            taken = {p.name for p in registry.pins if p is not pin}
            pin.name = "THERM" if "THERM" not in taken else next(
                f"THERM_{k}" for k in range(2, 99) if f"THERM_{k}" not in taken)
            pin.name_source = "anchor"
        else:
            set_specialty(pin, kind)
        self.ctx.journal.record(
            tool=self.id, params={"action": "rename"},
            inputs={"pin_number": pin.number, "centroid": list(pin.centroid)},
            result={"name": pin.name, "name_source": pin.name_source, "kind": pin.kind},
        )
        self._refresh_views()
        self.pin_list.setCurrentRow(row)
        self.status(f"Detect Pins: pin {pin.number} is now "
                    f"{pin.name if kind else 'a plain numbered pin'}")

    def _renumber_as(self, order: str) -> None:
        self.order_combo.setCurrentText(order)
        self.renumber()

    # ---- Select Group -------------------------------------------------
    def _group_button(self, role: str) -> QPushButton:
        """Select Group for one row of the panel: SMT/THR tails (primary) or
        CON/HEAD mouth contacts (mouth)."""
        button = QPushButton("Select Group")
        kind = "CON/HEAD" if role == "mouth" else "SMT/THR"
        button.setToolTip(
            "Like Detect Exact but relaxed: every face that looks like the last\n"
            f"{kind} tip, sorted into groups by facing direction and row. Tick the\n"
            "groups to stage (the hovered group lights up in the view).")
        menu = QMenu(button)
        menu.aboutToShow.connect(lambda: self._fill_group_menu(role, menu))
        menu.hovered.connect(lambda action: self._focus_group(role, action))
        menu.aboutToHide.connect(self.ctx.scene.end_flash)
        button.setMenu(menu)
        return button

    def _group_templates(self, role: str) -> list[Pin]:
        """Seeded tips of this row: staged first, else the listed pins."""
        def ours(pins):
            return [p for p in pins if (p.role == "mouth") == (role == "mouth") and p.face_ids]
        return ours(self.pending) or ours(self.ctx.window.pins.pins)

    def _candidate_groups(self, role: str = "primary") -> list[list[Pin]]:
        """Relaxed look-alikes of the last staged tip of `role`, grouped by
        the way they face and the row they sit in."""
        document = self.ctx.document
        registry = self.ctx.window.pins
        same = self._group_templates(role)
        if document is None or not same:
            return []
        template = same[-1]
        body = template.face_ids[0][0]
        region = {face for b, face in template.face_ids if b == body}
        claimed = {(b, f) for pin in registry.pins for b, f in pin.face_ids}
        matches = find_similar_regions(document, body, region,
                                       smooth_angle_deg=float(self.angle_spin.value()), claimed=claimed)
        groups: dict[tuple, list[Pin]] = {}
        size = max(np.ptp(np.array(document.bounds()).reshape(3, 2), axis=1).max(), 1e-6)
        for found in [[(body, f) for f in sorted(region)]] + matches:
            mesh = document.bodies[found[0][0]].mesh
            faces = sorted(f for _b, f in found)
            centroid = mesh_region_centroid(mesh, faces)
            if centroid is None:
                continue
            mask = np.isin(mesh.tri_face_ids, faces)
            tri = mesh.points[mesh.tris[mask]]
            normal = np.cross(tri[:, 1] - tri[:, 0], tri[:, 2] - tri[:, 0]).sum(axis=0)
            normal = normal / (np.linalg.norm(normal) or 1.0)
            # the row: position along the normal (which side) and height
            key = (tuple(np.round(normal, 1)), round(float(np.dot(centroid, normal)) / (0.05 * size)),
                   round(float(centroid[2]) / (0.05 * size)))
            groups.setdefault(key, []).append(Pin(number=0, centroid=centroid, face_ids=found,
                                                  role="mouth" if role == "mouth" else "primary"))
        return sorted(groups.values(), key=len, reverse=True)

    def _fill_group_menu(self, role: str, menu: QMenu) -> None:
        menu.clear()
        self._groups[role] = groups = self._candidate_groups(role)
        if not groups:
            kind = "CON/HEAD" if role == "mouth" else "SMT/THR"
            menu.addAction(f"Seed a {kind} tip first").setEnabled(False)
            return
        staged = {tuple(p.face_ids) for p in self.pending}
        for index, group in enumerate(groups):
            action = QAction(f"Group {index + 1}: {len(group)} pin(s)", menu)
            action.setCheckable(True)
            action.setChecked(all(tuple(p.face_ids) in staged for p in group))
            action.setData(index)
            action.toggled.connect(lambda on, i=index: self._toggle_group(role, i, on))
            menu.addAction(action)

    def _focus_group(self, role: str, action) -> None:
        """The hovered group lights up green on top of the model, so every
        pin of it shows, even the ones behind the body."""
        from ..document import _face_triangles

        index, groups = action.data(), self._groups.get(role, [])
        if index is None or not (0 <= index < len(groups)):
            self.ctx.scene.end_flash()
            return
        by_body: dict[int, list[int]] = {}
        for pin in groups[index]:
            for body_index, face in pin.face_ids:
                by_body.setdefault(body_index, []).append(face)
        bodies = self.ctx.document.bodies
        self.ctx.scene.flash_geometry(
            [_face_triangles(bodies[b].mesh, faces) for b, faces in by_body.items() if bodies[b].mesh is not None],
            seconds=None)

    def _toggle_group(self, role: str, index: int, on: bool) -> None:
        group = self._groups[role][index]
        keys = {tuple(p.face_ids) for p in group}
        if on:
            staged = {tuple(p.face_ids) for p in self.pending}
            self.pending.extend(p for p in group if tuple(p.face_ids) not in staged)
        else:
            self.pending = [p for p in self.pending if tuple(p.face_ids) not in keys]
        self.ctx.journal.record(
            tool=self.id, params={"action": "select_group", "on": on, "role": role},
            inputs={"regions": [[list(pair) for pair in pin.face_ids] for pin in group]},
            result={"pending": len(self.pending)})
        self.ctx.scene.clear_highlight()
        self._refresh_pending()
        self.status(f"Select Group ({_row_kind(role)}): group {index + 1} {'staged' if on else 'dropped'}; "
                    f"{len(self.pending)} pending")

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

    def _grid_anchors(self, pins: list) -> dict[int, str]:
        """Grid names the user typed (A1, B7 ...): the ground truth for BGA
        naming. None typed but the pattern is GRID: an A1 at the pin-1 hint,
        else at the package's pin-1 mark, else JEDEC's top-left default."""
        typed = {i: p.name.strip() for i, p in enumerate(pins)
                 if _typed(p) and not specialty(p) and parse_grid_name(p.name.strip()) is not None}
        if typed or not pins or self._pattern() != "grid":
            return typed
        return a1_anchor(pins, self._a1_corner(pins))

    def _a1_corner(self, pins: list) -> tuple[float, float] | None:
        hint = self.ctx.window.pin1_hint
        if hint:
            return float(hint[0]), float(hint[1])
        return marker_corner(self.ctx.document, pins) if self.ctx.document is not None else None

    def _pattern(self) -> str | None:
        return self.pattern_combo.currentData() if self._actions_widget is not None else None

    def _sync_pattern(self) -> None:
        """When the pin set changes (detected, added, deleted), set the
        pattern from the pins' XY layout; a choice made by hand stays until
        the pins change again."""
        pins = [p for p in self.ctx.window.pins.primaries() if not specialty(p)]
        key = tuple(sorted(tuple(round(float(c), 3) for c in p.centroid[:2]) for p in pins))
        if not pins or key == self._pattern_for:
            return
        self._pattern_for = key
        found = pin_pattern(np.array([p.centroid for p in pins], dtype=np.float64))
        self.pattern_combo.setCurrentIndex(max(self.pattern_combo.findData(found), 0))

    def _refresh_views(self) -> None:
        registry = self.ctx.window.pins
        if self._actions_widget is None:
            return
        self._sync_pattern()
        self.pin_list.clear()
        for pin in registry.pins:
            kind = f"bodies {pin.body_ids}" if pin.body_ids else f"{len(pin.face_ids)} faces"
            name = f" '{pin.name}'" if pin.name else ""
            mouth = " [CON/HEAD]" if pin.role == "mouth" else ""
            if specialty(pin):
                mouth += f" [{specialty(pin).upper()}]"
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

    # Designation colors: orange THR/SMT tails, blue CON/HEAD mouths, grey
    # ANCHOR / GUIDE; staged (not yet applied) faces are lighter.
    _PRIMARY_COLOR = "#e07b00"
    _MOUTH_COLOR = "#2a6fd4"
    _SPECIAL_COLOR = "#9a9a9a"
    _PENDING_COLOR = "#ffc46b"
    _PENDING_MOUTH_COLOR = "#7fb2ff"

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
        faces: dict[str, list] = {}
        for pin in self.pending:
            color = self._PENDING_MOUTH_COLOR if pin.role == "mouth" else self._PENDING_COLOR
            faces.setdefault(color, []).extend(pin.face_ids)
        if not pins or document is None:
            scene.show_pin_faces(faces if document is not None else {})
            scene.show_pin_axes([], [])
            scene.show_pin_labels([], [], [])
            return
        bounds = document.bounds()
        margin = 0.14 * float(np.linalg.norm([
            bounds[1] - bounds[0], bounds[3] - bounds[2], bounds[5] - bounds[4]]))
        segments, seg_colors, label_pts, label_colors, labels = [], [], [], [], []
        for pin in pins:
            centroid = np.asarray(pin.centroid, dtype=float)
            label = centroid + self._pin_normal(document, pin) * margin
            color = (self._SPECIAL_COLOR if specialty(pin)
                     else self._MOUTH_COLOR if pin.role == "mouth"
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

