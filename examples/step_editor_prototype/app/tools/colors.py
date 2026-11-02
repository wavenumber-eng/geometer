"""Redefine Colors: pick a color, select bodies or faces (Shift+click to
build up a selection), then Apply — or switch on the paintbrush and every
click paints immediately. Body and face colors persist into the AP242
export via XCAF (faces as sub-shape colors)."""

from __future__ import annotations

import numpy as np
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

from .base import ToolMode, make_apply_button, shrink_to_grid

# (hex, short name, what it is): metals, packages, then LED / marking colors
SWATCHES = [
    ("#c0c0c0", "Sn", "tin (leads, terminations)"),
    ("#e1e1e1", "Ag", "silver"),
    ("#d4af37", "Au", "gold (contacts)"),
    ("#b87333", "Cu", "copper"),
    ("#9da1a6", "Ni", "nickel / steel"),
    ("#6e7378", "ZN", "zinc / dark metal"),
    ("#202428", "EPOXY", "black epoxy body"),
    ("#0b0b0b", "BLK", "black plastic"),
    ("#3c3c3c", "GRY", "grey plastic"),
    ("#f5f5f0", "CER", "ceramic"),
    ("#d2b48c", "TAN", "tan ceramic / capacitor body"),
    ("#6b4423", "BRN", "brown"),
    ("#2e7d32", "MASK", "green solder mask"),
    ("#1b4f9c", "MASKB", "blue solder mask"),
    ("#c9b56a", "FR4", "bare FR4"),
    ("#ffffff", "WHT", "white (marking, LED)"),
    ("#e8f4ff", "CLR", "clear lens"),
    ("#d81e1e", "RED", "red (LED, marking)"),
    ("#ff7a00", "ORG", "orange (LED)"),
    ("#ffb300", "AMB", "amber (LED)"),
    ("#f5d800", "YEL", "yellow (LED)"),
    ("#18b33a", "GRN", "green (LED)"),
    ("#1f5fd6", "BLU", "blue (LED)"),
    ("#7b2fbe", "PUR", "purple / UV (LED)"),
]
SWATCH_COLUMNS = 6


class ColorsTool(ToolMode):
    id = "colors"
    title = "10 Colors"
    accent = "#8a2b8a"

    def __init__(self, ctx) -> None:
        super().__init__(ctx)
        self.current_color = "#d4af37"
        self.selection: set = set()  # body indices, or (body, face) pairs
        self._split_first = None  # (body, face, point) of the first split click

    # ------------------------------------------------------------------- UI

    def build_actions_widget(self) -> QWidget:
        widget = QWidget()
        layout = QVBoxLayout(widget)
        layout.setContentsMargins(8, 8, 8, 8)
        layout.addWidget(QLabel(
            "<b>Redefine Colors</b><br>"
            "Pick a color, click to select (Shift+click adds more), then "
            "Apply — or tick Brush to paint on every click. Click the color chip for any color."
        ))

        target_row = QHBoxLayout()
        self.target_combo = QComboBox()
        self.target_combo.addItems(["Bodies", "Faces"])
        self.target_combo.setToolTip("Paint whole bodies, or single faces")
        self.target_combo.currentTextChanged.connect(lambda _t: self.clear_selection())
        target_row.addWidget(self.target_combo, 1)
        self.brush_check = QCheckBox("Brush")
        self.brush_check.setToolTip("Paintbrush: every click paints the current color at once")
        target_row.addWidget(self.brush_check)
        self.color_chip = QPushButton()
        self.color_chip.setFixedSize(40, 22)
        self.color_chip.setToolTip("The current color; click for a custom color")
        self.color_chip.clicked.connect(self._pick_custom)
        target_row.addWidget(self.color_chip)
        layout.addLayout(target_row)

        grid = QGridLayout()
        grid.setSpacing(2)
        for index, (hex_color, label, meaning) in enumerate(SWATCHES):
            button = QPushButton(label)
            button.setToolTip(f"{label}: {meaning} ({hex_color})")
            button.setFixedHeight(22)
            button.setMinimumWidth(30)
            button.setStyleSheet(
                f"QPushButton {{background: {hex_color}; font-size: 9px; font-weight: 700; padding: 0;"
                f" border: 1px solid #00000060; color: "
                f"{'#ffffff' if QColor(hex_color).lightness() < 128 else '#101010'};}}"
            )
            button.clicked.connect(lambda _c=False, h=hex_color: self.set_color(h))
            grid.addWidget(button, index // SWATCH_COLUMNS, index % SWATCH_COLUMNS)
        layout.addLayout(grid)
        layout.addLayout(self._button_grid(self._pick_buttons()))

        self.selection_label = QLabel("Nothing selected.")
        self.selection_label.setWordWrap(True)
        layout.addWidget(self.selection_label)
        layout.addLayout(self._button_grid(self._select_buttons()))

        self.apply_button = make_apply_button("Apply Color")
        self.apply_button.setEnabled(False)
        self.apply_button.clicked.connect(self.apply)
        layout.addWidget(self.apply_button)

        auto_button = QPushButton("Auto")  # hidden on the page: the frame's AUTO drives it
        auto_button.setToolTip(
            "Conditioning palette: every pin body/face that carries no\n"
            "color gets bare-metal tin, so exported contacts always show."
        )
        auto_button.clicked.connect(self.run_auto)
        layout.addWidget(auto_button)
        from .colors_photo import PhotoPanel

        self.photo_panel = PhotoPanel(self)
        layout.addWidget(self.photo_panel)
        layout.addStretch(1)

        self._update_color_chip()
        return widget

    @staticmethod
    def _button_grid(buttons, columns: int = 3) -> QGridLayout:
        """Small buttons packed three to a row (a grid never reflows into a
        column of full-width buttons the way a too-wide row does)."""
        grid = QGridLayout()
        grid.setSpacing(3)
        for index, button in enumerate(buttons):
            shrink_to_grid(button)
            grid.addWidget(button, index // columns, index % columns)
        return grid

    def _pick_buttons(self) -> list:
        """Ways to get a color other than a swatch, and the face split."""
        custom_button = QPushButton("Custom ...")
        custom_button.setToolTip("Pick any color")
        custom_button.clicked.connect(self._pick_custom)
        self.eyedropper_button = QPushButton("Eyedropper")
        self.eyedropper_button.setCheckable(True)
        self.eyedropper_button.setToolTip(
            "Click a face to take its color as the current color\n"
            "(reuse a vendor color the swatches don't have).")
        self.eyedropper_button.toggled.connect(self._arm_eyedropper)
        self.split_button = QPushButton("Split face")
        self.split_button.setCheckable(True)
        self.split_button.setToolTip(
            "Split face (2 clicks): click near two VERTICES of one face (clicks snap\n"
            "to the face's own vertices, never a free point): the face is cut in two\n"
            "along the line between them so each half can take its own color.")
        self.split_button.toggled.connect(self._arm_split)
        return [custom_button, self.eyedropper_button, self.split_button]

    def _select_buttons(self) -> list:
        """Selection helpers, then Clear and Reset."""
        specs = [
            ("Same color", "Select every face (or body) wearing the same color as the selection",
             self.select_same_color),
            ("Similar", "Select every face with the same shape as a selected face\n"
                        "(same surface type, area and direction): all matching pins at once",
             self.select_similar),
            ("+1 ring", "Grow the face selection by one ring: every face sharing an edge with a\n"
                        "selected face joins it. Shift+click afterwards to drop the ones you don't want.",
             self.propagate_selection),
            ("All pins", "Select every pin body / pin face", self.select_all_pins),
            ("Clear", "Clear the selection", self.clear_selection),
            ("Reset", "Reset every color to the file's original colors", self._reset_original),
        ]
        buttons = []
        for text, tip, slot in specs:
            button = QPushButton(text)
            button.setToolTip(tip)
            button.clicked.connect(lambda _c=False, f=slot: f())
            buttons.append(button)
        return buttons

    # ------------------------------------------------------------ lifecycle

    def enter(self) -> None:
        self.status("Colors: pick a color, then click bodies/faces (Shift adds)")

    def exit(self) -> None:
        self.ctx.scene.clear_highlight()
        self.selection = set()
        if self._actions_widget is not None:
            self.eyedropper_button.setChecked(False)
            self.split_button.setChecked(False)
            self.photo_panel.cancel()  # leaving the page never keeps an un-applied preview

    def on_document_changed(self) -> None:
        self.selection = set()
        if self._actions_widget is not None:
            self._refresh_selection_ui()
            self.photo_panel.document_changed()

    # -------------------------------------------------------------- picking

    def on_pick(self, pick) -> None:
        if self.ctx.document is None:
            return
        if self.eyedropper_button.isChecked():
            self._eyedrop(pick)
            return
        if self.split_button.isChecked():
            self._split_click(pick)
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
        self.status(f"Colors: current color {hex_color}")

    def _pick_custom(self) -> None:
        color = QColorDialog.getColor(parent=self.actions_widget())
        if color.isValid():
            self.set_color(color.name())

    def _arm_eyedropper(self, checked: bool) -> None:
        if checked:
            self.split_button.setChecked(False)
            self.status("Eyedropper: click a face to take its color")

    def _eyedrop(self, pick) -> None:
        body = self.ctx.document.bodies[pick.body_index]
        rgb = None
        if body.mesh is not None:
            rgb = body.mesh.face_colors.get(pick.face_index)
        rgb = rgb or body.color or (0.62, 0.64, 0.66)
        self.set_color(QColor.fromRgbF(*rgb).name())
        self.eyedropper_button.setChecked(False)
        self.status(f"Eyedropper: current color is now {self.current_color}")

    def _arm_split(self, checked: bool) -> None:
        self._split_first = None
        self.ctx.scene.set_markers([], name="split-points")
        self.ctx.scene.set_markers([], name="split-hover")
        # preview before clicking: the vertex a click here would snap to
        self.ctx.scene.set_hover_callback(self._split_hover if checked else None)
        if checked:
            self.eyedropper_button.setChecked(False)
            self.status("Split face: click near a VERTEX of the face to split (clicks snap to the face's vertices)")

    def _split_hover(self, pick) -> None:
        """Hovering in Split face: the clicked face's vertex the click would
        snap to lights up (on top), so the person sees it before clicking."""
        if pick is None or self.ctx.document is None:
            self.ctx.scene.set_markers([], name="split-hover")
            return
        first = self._split_first
        if first is not None and (pick.body_index, pick.face_index) != first[:2]:
            self.ctx.scene.set_markers([], name="split-hover")  # the second vertex must be on the same face
            return
        self.ctx.scene.set_markers([self._face_vertex(pick)], name="split-hover", color="#ffd24a",
                                   point_size=12.0, opacity=0.9, on_top=True)

    def _face_vertex(self, pick) -> list[float]:
        """The clicked face's B-rep vertex nearest the click: a split runs
        only between vertices that already exist."""
        from OCP.BRep import BRep_Tool
        from OCP.TopAbs import TopAbs_VERTEX
        from OCP.TopExp import TopExp_Explorer
        from OCP.TopoDS import TopoDS

        face = self.ctx.document.face_of(pick.body_index, pick.face_index)
        corners = []
        explorer = TopExp_Explorer(face, TopAbs_VERTEX)
        while explorer.More():
            p = BRep_Tool.Pnt_s(TopoDS.Vertex_s(explorer.Current()))
            corners.append([p.X(), p.Y(), p.Z()])
            explorer.Next()
        click = np.asarray(pick.world_point, dtype=float)
        return min(corners, key=lambda c: float(np.linalg.norm(np.asarray(c) - click)))

    def _split_click(self, pick) -> None:
        point = self._face_vertex(pick)  # snapped: never a free point on the face
        if self._split_first is not None and np.allclose(point, self._split_first[2]):
            self.status("Split face: that is the first vertex again — click another vertex of the face")
            return
        if self._split_first is None:
            self._split_first = (pick.body_index, pick.face_index, point)
            self.ctx.scene.set_markers([point], name="split-points", color="#ff4fd8",
                                       point_size=14.0, on_top=True)
            self.status("Split face: snapped to the face's vertex; now click near ANOTHER vertex of the same face")
            return
        body_index, face_index, first = self._split_first
        self._split_first = None
        self.ctx.scene.set_markers([], name="split-points")
        if (pick.body_index, pick.face_index) != (body_index, face_index):
            self.status("Split face: both points must be on the same face — start again")
            return
        from ..pins import capture_pin_face_anchors, remap_pin_faces

        document = self.ctx.document
        registry = self.ctx.window.pins
        anchors = capture_pin_face_anchors(document, registry.pins)
        try:
            new_faces = document.split_face(body_index, face_index, first, point)
        except Exception as exc:  # noqa: BLE001 - OCC failures surface to the user
            self.status(f"Split face failed: {exc}")
            return
        if not new_faces:
            self.status("Split face: the line between those two vertices doesn't cut the face "
                        "(they share an edge) — pick two vertices not on the same edge")
            return
        b = document.bounds()
        remap_pin_faces(document, anchors,
                        float(np.linalg.norm([b[1] - b[0], b[3] - b[2], b[5] - b[4]])) * 5e-3)
        self.ctx.journal.record(
            tool=self.id, params={"action": "split_face"},
            inputs={"body": body_index, "face": face_index, "p1": first, "p2": point},
            result={"new_faces": new_faces},
        )
        self.split_button.setChecked(False)
        self.ctx.window.document_mutated()
        self.target_combo.setCurrentText("Faces")
        self.selection = {(body_index, f) for f in new_faces[:1]}
        self._refresh_selection_ui()
        self.status(f"Split face: face {face_index} is now faces {new_faces} — the first "
                    "half is selected; pick a color and Apply (Shift+click the other)")

    def apply(self) -> None:
        if not self.selection:
            self.status("Colors: nothing selected")
            return
        self._paint_many(self.selection, self.current_color)
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

    def run_auto(self) -> None:
        """Tin every uncolored pin body/face (journaled like an apply)."""
        from ..auto import auto_color_pins

        document = self.ctx.document
        registry = self.ctx.window.pins
        if document is None or not registry.pins:
            self.status("Colors: no pins yet — run Detect Pins first")
            return
        changed = auto_color_pins(document, registry)
        if not changed:
            self.status("Auto: every pin already carries a color")
            return
        self.ctx.journal.record(
            tool=self.id,
            params={"action": "auto_pins"},
            inputs={"keys": changed},
            result={"color": "#c7c9ce"},
        )
        self.ctx.window.document_mutated()
        self.status(f"Auto: tinned {len(changed)} uncolored pin item(s)")

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

    def propagate_selection(self) -> None:
        """Grow the face selection by one adjacency ring."""
        document = self.ctx.document
        if document is None or not self.selection:
            self.status("Colors: select some faces first")
            return
        if self.target_combo.currentText() != "Faces":
            self.status("Colors: propagate works in Faces mode")
            return
        by_body: dict[int, set[int]] = {}
        for body_index, face_index in self.selection:
            by_body.setdefault(body_index, set()).add(face_index)
        added = 0
        for body_index, faces in by_body.items():
            adjacency = document.face_smooth_adjacency(body_index, None)
            for face_index in list(faces):
                for neighbor in adjacency.get(face_index, ()):
                    key = (body_index, int(neighbor))
                    if key not in self.selection:
                        self.selection.add(key)
                        added += 1
        self._refresh_selection_ui()
        self.status(
            f"Colors: propagated outwards — {added} face(s) joined "
            f"({len(self.selection)} selected); Shift+click to drop extras"
        )

    def on_miss(self) -> None:
        """A click on the background keeps the selection (right-click clears)."""

    def on_right_miss(self) -> None:
        self.clear_selection()

    def _face_color(self, body_index: int, face_index: int):
        body = self.ctx.document.bodies[body_index]
        rgb = body.mesh.face_colors.get(face_index) if body.mesh is not None else None
        rgb = rgb or body.color or (0.62, 0.64, 0.66)
        return tuple(round(float(c), 3) for c in rgb)

    def staged_rows(self) -> list[tuple[str, str, str]]:
        if not self.selection:
            return []
        kind = "bodies" if self.target_combo.currentText() == "Bodies" else "faces"
        name = next((label for hex_color, label, _m in SWATCHES if hex_color == self.current_color),
                    self.current_color)
        rows = [("paint", f"{len(self.selection)} {kind}", f"→ {name} ({self.current_color})")]
        for key in sorted(self.selection, key=str)[:40]:
            rows.append(("", self._key_text(key), ""))
        return rows

    def deny(self) -> None:
        self.clear_selection()

    def select_same_color(self) -> None:
        """Every face (or body) with a color the selection already wears."""
        document = self.ctx.document
        if document is None or not self.selection:
            self.status("Colors: select a face or body first")
            return
        if self.target_combo.currentText() == "Bodies":
            wanted = {tuple(round(float(c), 3) for c in (document.bodies[k].color or (0.62, 0.64, 0.66)))
                      for k in self.selection}
            self.selection = {i for i, b in enumerate(document.bodies)
                              if tuple(round(float(c), 3) for c in (b.color or (0.62, 0.64, 0.66))) in wanted}
        else:
            wanted = {self._face_color(b, f) for b, f in self.selection}
            found = set()
            for b, body in enumerate(document.bodies):
                if body.mesh is None:
                    continue
                for f in np.unique(body.mesh.tri_face_ids):
                    if self._face_color(b, int(f)) in wanted:
                        found.add((b, int(f)))
            self.selection = found
        self._refresh_selection_ui()
        self.status(f"Colors: {len(self.selection)} item(s) share that color")

    def select_similar(self) -> None:
        """Faces shaped like a selected face: same area (2%) and the same
        facing direction, on any body: every matching pin face in one go."""
        document = self.ctx.document
        if document is None or not self.selection or self.target_combo.currentText() != "Faces":
            self.status("Colors: select a face first (Faces mode)")
            return
        from ..pins import mesh_face_areas

        def signature(body_index, face_index):
            mesh = document.bodies[body_index].mesh
            mask = mesh.tri_face_ids == face_index
            tri = mesh.points[mesh.tris[mask]]
            normal = np.cross(tri[:, 1] - tri[:, 0], tri[:, 2] - tri[:, 0]).sum(axis=0)
            return np.linalg.norm(normal) and normal / np.linalg.norm(normal)

        targets = []
        for b, f in self.selection:
            areas = mesh_face_areas(document.bodies[b].mesh)
            targets.append((areas.get(f, 0.0), signature(b, f)))
        found = set(self.selection)
        for b, body in enumerate(document.bodies):
            if body.mesh is None:
                continue
            for f, area in mesh_face_areas(body.mesh).items():
                for t_area, t_normal in targets:
                    if t_area and abs(area - t_area) <= 0.02 * t_area \
                            and float(np.dot(signature(b, f), t_normal)) > 0.98:
                        found.add((b, int(f)))
                        break
        added = len(found) - len(self.selection)
        self.selection = found
        self._refresh_selection_ui()
        self.status(f"Colors: {added} similar face(s) joined ({len(found)} selected)")

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

    def _paint_many(self, keys, hex_color: str) -> None:
        """Paint many bodies / faces with ONE render per body (painting face
        by face re-rendered the whole scene each time)."""
        document = self.ctx.document
        qcolor = QColor(hex_color)
        rgb = (qcolor.redF(), qcolor.greenF(), qcolor.blueF())
        faces_by_body: dict[int, dict[int, tuple]] = {}
        for key in keys:
            if isinstance(key, int):
                self._paint(key, hex_color)
                continue
            body_index, face_index = key
            mesh = document.bodies[body_index].mesh
            if mesh is not None:
                mesh.face_colors[face_index] = rgb
            faces_by_body.setdefault(body_index, {})[face_index] = rgb
        for body_index, face_rgb in faces_by_body.items():
            self.ctx.scene.paint_faces(body_index, face_rgb)

    def _paint(self, key, hex_color: str) -> None:
        document = self.ctx.document
        qcolor = QColor(hex_color)
        rgb = (qcolor.redF(), qcolor.greenF(), qcolor.blueF())
        if isinstance(key, int):
            body = document.bodies[key]
            body.color = rgb
            if body.mesh is not None:
                body.mesh.face_colors = {}  # body color overrides face colors
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
            scene.highlight_faces(sorted(self.selection), hatched=True)
        shown = sorted(self.selection, key=str)
        names = ", ".join(self._key_text(k) for k in shown[:8])
        more = f" (+{len(shown) - 8} more)" if len(shown) > 8 else ""
        self.selection_label.setText(f"{len(self.selection)} selected: {names}{more}")
        self.apply_button.setEnabled(True)

    def _update_color_chip(self) -> None:
        self.color_chip.setStyleSheet(
            f"QPushButton {{background: {self.current_color}; border: 1px solid #555;}}"
        )
