"""Separate Unibody: automatically detect the full PIN shapes by edge flow.
The pins found by Detect Pins tell us where each pin STARTS; from those seed
faces the flow expands across shared edges for as long as the faces stay
pin-scaled and stops when it reaches the BODY. The grown shapes preview in
body colours (the browse-3d 'Bodies' look); Apply caps each pin/body junction
loop and splits the solid there — every pin becomes its own whole body, which
makes the hitbox stage (and the exported AP242) far more useful."""

from __future__ import annotations

import colorsys

import numpy as np
import pyvista as pv
from PySide6.QtCore import Signal
from PySide6.QtWidgets import (
    QComboBox,
    QDoubleSpinBox,
    QGridLayout,
    QHBoxLayout,
    QLabel,
    QLineEdit,
    QPushButton,
    QVBoxLayout,
    QWidget,
)
from pyvistaqt import QtInteractor

from ..pins import (
    capture_pin_face_anchors,
    dominant_face_color,
    grow_pin_regions,
    mesh_region_centroid,
    remap_pin_faces,
)
from .base import ToolMode, make_apply_button


class _AxisLineEdit(QLineEdit):
    """A limit field that announces focus so the tool can light up the plane
    whose bounds are being edited."""

    focusedIn = Signal(str)
    focusedOut = Signal()

    def __init__(self, key: str, parent=None) -> None:
        super().__init__(parent)
        self._key = key

    def focusInEvent(self, event) -> None:  # noqa: N802 (Qt override)
        super().focusInEvent(event)
        self.focusedIn.emit(self._key)

    def focusOutEvent(self, event) -> None:  # noqa: N802 (Qt override)
        super().focusOutEvent(event)
        self.focusedOut.emit()


def pastel(index: int) -> tuple[float, float, float]:
    """Golden-ratio pastel palette (the wn3d Bodies-view look)."""
    hue = (index * 0.61803398875) % 1.0
    return colorsys.hsv_to_rgb(hue, 0.45, 0.92)


# CON/HEAD pins not yet joined to an SMT/THR tail (no designator to share a
# colour with) paint in this neutral blue — the mouth-detector accent.
UNJOINED_MOUTH_RGB = (0.35, 0.55, 0.95)

# Pick-marker colours: axis picks use the X/Y/Z basis colours; the box-corner
# pick uses two yellow points.
AXIS_MARKER_COLORS = ("#e0524d", "#4dd158", "#5a86ff")  # X red, Y green, Z blue
BOX_MARKER_COLOR = "#ffd23f"  # yellow


def designator_pastels(pins) -> dict[str, tuple[float, float, float]]:
    """Pastel per designator, keyed off the SMT/THR (primary) pins — a joined
    CON/HEAD pin paints in its tail's colour so the pair reads as one net."""
    return {
        (pin.name or str(pin.number)): pastel(index)
        for index, pin in enumerate(pins)
        if pin.role != "mouth"
    }


class SeparateUnibodyTool(ToolMode):
    id = "separate"
    title = "Separate Unibody"
    accent = "#5a3e9e"

    def __init__(self, ctx) -> None:
        super().__init__(ctx)
        self._preview_painted = False
        self._grown: list | None = None
        # manual face bundles: {pin index -> {(body, face), ...}} — when set,
        # the preview shows EXACTLY these and Apply splits EXACTLY these
        self._manual: dict[int, set[tuple[int, int]]] | None = None
        self._bundle_pins: list[int] = []
        # manual box-bounded cuts (the universal split fallback): banked
        # [{"limits": [xlo,xhi,ylo,yhi,zlo,zhi], "normal": [..], "location": f}]
        # sliced by the boolean cutter on Apply. Pastel-previewed live.
        self._cuts: list[dict] = []
        self._cut_actor_names: list[str] = []
        self._axis_hl_names: list[str] = []  # focus-highlight plane actors
        # per-axis vertex pick: (axis_index, [collected vertices]) while picking
        self._axis_pick: tuple[int, list] | None = None
        # box-corner pick: [collected vertex points] while picking two corners
        self._box_pick: list | None = None

    # ------------------------------------------------------------------- UI

    def build_actions_widget(self) -> QWidget:
        widget = QWidget()
        layout = QVBoxLayout(widget)
        layout.setContentsMargins(8, 8, 8, 8)
        header = QLabel(
            "<b>Separate Unibody</b> — cut a single solid into one body per pin. "
            "Two ways (toggle below):<br>"
            "• <b>Body-Face Cutoff</b> — grows each pin from its seed faces by "
            "edge flow (best for clean protrusions).<br>"
            "• <b>Box Cut</b> — box a pin and slice it out with a boolean cut "
            "(works when growth can't, e.g. mid-face junctions)."
        )
        header.setWordWrap(True)
        layout.addWidget(header)

        self.info_label = QLabel("")
        self.info_label.setWordWrap(True)
        layout.addWidget(self.info_label)

        # Sub-mode toggle (flathead vs phillips): Body-Face Cutoff <-> Box Cut.
        self.cut_button = QPushButton("Box Cut mode")
        self.cut_button.setCheckable(True)
        self.cut_button.setToolTip(
            "Switch this tool's application: OFF = Body-Face Cutoff (edge-flow\n"
            "growth); ON = Box Cut (box a pin and carve that chunk out with a\n"
            "boolean cut). The box cuts THROUGH faces, so pins whose junction\n"
            "with the body runs mid-face still separate. Same tool, two ways to\n"
            "drive it; the other mode's controls hide while active."
        )
        self.cut_button.setStyleSheet(
            "QPushButton:checked {background-color: #5a3e9e; color: #ffffff; "
            "font-weight: 700;}"
        )
        self.cut_button.toggled.connect(self._arm_cut)
        layout.addWidget(self.cut_button)

        # ===== Body-Face Cutoff sub-mode (hidden while Box Cut is active) =====
        self.cutoff_group = QWidget()
        cutoff_layout = QVBoxLayout(self.cutoff_group)
        cutoff_layout.setContentsMargins(0, 0, 0, 0)
        layout.addWidget(self.cutoff_group)

        factor_row = QHBoxLayout()
        factor_row.addWidget(QLabel("Body face cutoff"))
        self.factor_spin = QDoubleSpinBox()
        self.factor_spin.setRange(0.1, 1000.0)
        self.factor_spin.setDecimals(2)
        self.factor_spin.setValue(4.0)
        self.factor_spin.setSingleStep(0.5)
        self.factor_spin.setToolTip(
            "Growth flows from the seed right up to the BODY: it stops at the "
            "first face larger than this multiple of the pin's largest seed "
            "face. LOWER = stops sooner (tighter pins, 0.1 ~ seeds only); "
            "HIGHER = grows farther (risk of leaking into the body)."
        )
        # A regrow at DDR5 scale takes seconds — coalesce rapid spinbox ticks
        # into one regrow instead of one per tick.
        from PySide6.QtCore import QTimer

        self._regrow_timer = QTimer(widget)
        self._regrow_timer.setSingleShot(True)
        self._regrow_timer.setInterval(300)
        self._regrow_timer.timeout.connect(self._regrow)
        self.factor_spin.valueChanged.connect(
            lambda _v: self._regrow_timer.start()
        )
        factor_row.addWidget(self.factor_spin, 1)
        repaint_button = QPushButton("Regrow preview")
        repaint_button.clicked.connect(self._regrow)
        factor_row.addWidget(repaint_button)
        auto_button = QPushButton("Auto")
        auto_button.setToolTip(
            "Pick the cutoff automatically: the smallest factor where the\n"
            "edge-flow growth plateaus (pins reached the body). Sets the\n"
            "spinbox and refreshes the preview — review, then Apply."
        )
        auto_button.clicked.connect(self.run_auto)
        factor_row.addWidget(auto_button)
        cutoff_layout.addLayout(factor_row)

        paint_row = QHBoxLayout()
        self.paint_button = QPushButton("Paint Regions")
        self.paint_button.setCheckable(True)
        self.paint_button.setToolTip(
            "Take over when the grown preview is wrong: bundles start from\n"
            "the current preview, then every click adds the face to the\n"
            "selected pin's bundle (click again to remove). Apply splits\n"
            "EXACTLY the painted bundles."
        )
        self.paint_button.setStyleSheet(
            "QPushButton:checked {background-color: #5a3e9e; color: #ffffff; "
            "font-weight: 700;}"
        )
        self.paint_button.toggled.connect(self._arm_paint)
        paint_row.addWidget(self.paint_button)
        self.bundle_combo = QComboBox()
        self.bundle_combo.setToolTip("The pin whose bundle your clicks edit")
        paint_row.addWidget(self.bundle_combo, 1)
        propagate_button = QPushButton("+1 ring")
        propagate_button.setToolTip(
            "Grow the selected bundle by one ring: every face sharing an\n"
            "edge with the bundle joins it (same as the Colors tool's\n"
            "propagate button). Faces owned by other bundles stay put."
        )
        propagate_button.clicked.connect(self._propagate_bundle)
        paint_row.addWidget(propagate_button)
        cutoff_layout.addLayout(paint_row)

        # ===== Box Cut sub-mode (hidden until armed) =====
        self.cut_group = QWidget()
        cut_layout = QVBoxLayout(self.cut_group)
        cut_layout.setContentsMargins(0, 0, 0, 0)
        cut_guide = QLabel("Box a pin → Add Cut → Apply.")
        cut_guide.setStyleSheet("color: #8a8f98;")
        cut_layout.addWidget(cut_guide)

        box_pick_button = QPushButton("⊙ Box from 2 points")
        box_pick_button.setToolTip(
            "Click two model vertices (opposite corners); their axis-aligned "
            "box becomes the X/Y/Z limits. The box carves out that chunk as the pin."
        )
        box_pick_button.clicked.connect(self._arm_box_pick)
        cut_layout.addWidget(box_pick_button)

        grid = QGridLayout()
        self._cut_fields: dict[str, QLineEdit] = {}

        def _field(key: str, default: str = "") -> QLineEdit:
            edit = _AxisLineEdit(key)
            edit.setText(default)
            edit.setMaximumWidth(70)
            edit.editingFinished.connect(self._update_cut_preview)
            edit.focusedIn.connect(self._highlight_axis_plane)
            edit.focusedOut.connect(self._clear_axis_highlight)
            self._cut_fields[key] = edit
            return edit

        def _pick_btn(axis: int) -> QPushButton:
            button = QPushButton("⊙ 2 pts")
            button.setMaximumWidth(64)
            button.setToolTip(
                "Click two model vertices; their coordinates set this axis's "
                "min/max (clicks snap to the nearest vertex)."
            )
            button.clicked.connect(lambda: self._arm_axis_pick(axis))
            return button

        grid.addWidget(QLabel("X min/max"), 0, 0)
        grid.addWidget(_field("xlo"), 0, 1)
        grid.addWidget(_field("xhi"), 0, 2)
        grid.addWidget(_pick_btn(0), 0, 3)
        grid.addWidget(QLabel("Y min/max"), 1, 0)
        grid.addWidget(_field("ylo"), 1, 1)
        grid.addWidget(_field("yhi"), 1, 2)
        grid.addWidget(_pick_btn(1), 1, 3)
        grid.addWidget(QLabel("Z min/max"), 2, 0)
        grid.addWidget(_field("zlo"), 2, 1)
        grid.addWidget(_field("zhi"), 2, 2)
        grid.addWidget(_pick_btn(2), 2, 3)
        cut_layout.addLayout(grid)

        cut_buttons = QHBoxLayout()
        fit_button = QPushButton("Box = model")
        fit_button.setToolTip("Reset the X/Y/Z limits to the whole model bounds.")
        fit_button.clicked.connect(self._fit_box_to_model)
        cut_buttons.addWidget(fit_button)
        add_cut_button = QPushButton("Add Cut")
        add_cut_button.setToolTip("Bank the current box as one cut (stack several to carve more pins).")
        add_cut_button.clicked.connect(self._add_cut)
        cut_buttons.addWidget(add_cut_button)
        match_button = QPushButton("Find matching")
        match_button.setToolTip(
            "Detect the feature inside the current box and stage a matching box "
            "cut on every identical feature in the model (rotation/mirror-"
            "invariant). Review the pastel preview, then Apply."
        )
        match_button.clicked.connect(self._find_matching)
        cut_buttons.addWidget(match_button)
        clear_cut_button = QPushButton("Clear Cuts")
        clear_cut_button.clicked.connect(self._clear_cuts)
        cut_buttons.addWidget(clear_cut_button)
        cut_layout.addLayout(cut_buttons)
        self.cut_label = QLabel("")
        self.cut_label.setWordWrap(True)
        self.cut_label.setStyleSheet("color: #8a8f98;")
        cut_layout.addWidget(self.cut_label)
        layout.addWidget(self.cut_group)
        self.cut_group.setVisible(False)

        preview_header = QLabel("SPLIT PREVIEW — each future body in its own colour")
        preview_header.setStyleSheet(
            "font-weight: 700; color: #8a8f98; padding-top: 6px;"
        )
        layout.addWidget(preview_header)
        self.preview_plotter = QtInteractor(widget)
        self.preview_plotter.interactor.setMinimumHeight(190)
        self.preview_plotter.set_background("#15171c")
        layout.addWidget(self.preview_plotter.interactor, 1)

        self.apply_button = make_apply_button("Apply Separate")
        self.apply_button.clicked.connect(self.apply)
        layout.addWidget(self.apply_button)
        self.undo_button = QPushButton("Undo Separate")
        self.undo_button.setToolTip(
            "Separation only reorganises in-memory geometry — restore the "
            "pre-split bodies and pin links."
        )
        self.undo_button.setEnabled(False)
        self.undo_button.clicked.connect(self.undo)
        layout.addWidget(self.undo_button)
        return widget

    # ------------------------------------------------------------ lifecycle

    def enter(self) -> None:
        document = self.ctx.document
        registry = self.ctx.window.pins
        if document is None:
            return
        if not registry.pins:
            self.info_label.setText("No pins detected yet — run Detect Pins first.")
            self.status("Separate Unibody: run Detect Pins first")
            return
        self._regrow()
        self._draw_spines(registry)
        # whole model at 0.5 alpha so the pin regions show through the body
        self.ctx.scene.set_model_opacity(0.5)
        self.status(
            "Separate Unibody: check the grown pin shapes (model at 0.5 alpha), "
            "then Apply"
        )

    def _draw_spines(self, registry) -> None:
        """Bridge each pin's tail and mouth FACES with a dotted yellow spine —
        the pin's central axis the split carves along. Tail<->mouth are paired
        by their shared designator (set by Join), so each physical pin gets one
        spine joining its two ends through the body."""
        by_designator = {
            (p.name or str(p.number)): p for p in registry.primaries()
        }
        segments = []
        for mouth in registry.mouths():
            primary = by_designator.get(mouth.name or str(mouth.number))
            if primary is not None:
                segments.append([primary.centroid, mouth.centroid])
        self.ctx.scene.show_pin_spines(segments)

    def exit(self) -> None:
        self.ctx.scene.show_pin_spines([])
        self.ctx.scene.set_hover_callback(None)
        self._axis_pick = None
        self._box_pick = None
        self._clear_cut_visuals()
        if self._actions_widget is not None:
            self.cut_button.blockSignals(True)
            self.cut_button.setChecked(False)
            self.cut_button.blockSignals(False)
            self.cut_group.setVisible(False)
            self.cutoff_group.setVisible(True)
        if self.ctx.document is not None:
            if self._preview_painted:
                self.ctx.scene.rebuild(self.ctx.document)  # restore real colours
                self._preview_painted = False
            else:
                self.ctx.scene.set_model_opacity(1.0)

    def on_document_changed(self) -> None:
        self._preview_painted = False
        self._grown = None
        self._manual = None
        self._cuts = []
        self._axis_pick = None
        self._box_pick = None
        self._clear_cut_visuals()
        if self._actions_widget is not None:
            self.paint_button.setChecked(False)
            self.cut_button.blockSignals(True)  # don't trigger a scene repaint here
            self.cut_button.setChecked(False)
            self.cut_button.blockSignals(False)
            self.cut_group.setVisible(False)
            self.cutoff_group.setVisible(True)
            self._update_cut_label()
            self._update_split_preview()

    # --------------------------------------------------------- paint regions

    def _init_paint_bundles(self, document, registry) -> None:
        """Seed the manual bundles from the preview: each pin's grown region,
        or its own faces for passive CON/HEAD anchors (whole-body pins skip)."""
        if self._grown is None:
            self._grown = grow_pin_regions(
                document, registry.pins,
                area_factor=float(self.factor_spin.value()),
                progress=self._grow_progress(),
            )
            if hasattr(self.ctx.window, "progress_done"):
                self.ctx.window.progress_done()
        if self._manual is None:
            self._manual = {}
            for index, (pin, region) in enumerate(zip(registry.pins, self._grown)):
                if pin.body_ids:
                    continue
                faces = region if region else pin.face_ids
                if faces:
                    self._manual[index] = {tuple(key) for key in faces}

    def _populate_bundle_combo(self, registry) -> None:
        self._bundle_pins = sorted(self._manual)
        self.bundle_combo.clear()
        for index in self._bundle_pins:
            pin = registry.pins[index]
            tag = " [CON/HEAD]" if pin.role == "mouth" else ""
            name = f" '{pin.name}'" if pin.name else ""
            self.bundle_combo.addItem(f"Pin {pin.number}{name}{tag}")

    def _arm_paint(self, checked: bool) -> None:
        if not checked:
            return  # bundles stay active until Regrow/Apply discards them
        document = self.ctx.document
        registry = self.ctx.window.pins
        if document is None or not registry.pins:
            self.paint_button.setChecked(False)
            return
        self._init_paint_bundles(document, registry)
        self._populate_bundle_combo(registry)
        self._paint_manual()
        self.status(
            "Paint Regions: pick the bundle in the dropdown, click faces to "
            "add them (click again to remove) — Apply splits exactly these"
        )

    def on_pick(self, pick) -> None:
        if self._actions_widget is not None and self.cut_button.isChecked():
            if self._box_pick is not None:
                self._pick_box_corner(pick)
            elif self._axis_pick is not None:
                self._pick_axis_vertex(pick)
            return
        if (
            self._actions_widget is None
            or not self.paint_button.isChecked()
            or self._manual is None
            or not self._bundle_pins
        ):
            return
        active = self._bundle_pins[max(0, self.bundle_combo.currentIndex())]
        key = (pick.body_index, pick.face_index)
        if key in self._manual.get(active, set()):
            self._manual[active].discard(key)
            action = "removed from"
        else:
            for bundle in self._manual.values():
                bundle.discard(key)  # a face belongs to one bundle only
            self._manual.setdefault(active, set()).add(key)
            action = "added to"
        self._paint_manual()
        registry = self.ctx.window.pins
        self.status(
            f"Paint Regions: face {pick.face_index} {action} pin "
            f"{registry.pins[active].number}'s bundle "
            f"({len(self._manual[active])} faces)"
        )

    def _arm_axis_pick(self, axis: int) -> None:
        if not self.cut_button.isChecked():
            self.status("Box Cut: enable Box Cut mode first")
            return
        self._axis_pick = (axis, [])
        self._box_pick = None
        self._show_axis_guide(axis)
        self.status(
            f"Box Cut: click two model vertices to set {'XYZ'[axis]} min/max "
            "(snaps to the nearest vertex)"
        )

    def _show_axis_guide(self, axis: int) -> None:
        """A bold, drawn-on-front coloured tube along the axis being picked
        (X red / Y green / Z blue) so the active axis stands out — same
        always-visible style as the 3D browser's reference axes."""
        document = self.ctx.document
        if document is None:
            return
        self._hide_axis_guide()
        b = document.bounds()
        center = [(b[0] + b[1]) / 2, (b[2] + b[3]) / 2, (b[4] + b[5]) / 2]
        span = max(b[1] - b[0], b[3] - b[2], b[5] - b[4]) * 0.65 + 1e-3
        start, end = list(center), list(center)
        start[axis] = center[axis] - span
        end[axis] = center[axis] + span
        color = ((0.95, 0.25, 0.25), (0.30, 0.92, 0.35), (0.35, 0.55, 1.0))[axis]
        actor = self.ctx.scene.plotter.add_mesh(
            pv.Line(start, end), name="sep-axis-guide", color=color,
            line_width=6, render_lines_as_tubes=True, pickable=False)
        self.ctx.scene._push_on_top(actor)  # draw on front, through the solid
        self.ctx.scene.plotter.render()

    def _hide_axis_guide(self) -> None:
        self.ctx.scene.plotter.remove_actor("sep-axis-guide", render=False)

    def _highlight_axis_plane(self, key: str) -> None:
        """Light up (alpha 0.5) the two box faces perpendicular to the axis
        whose limit field just took focus — so the user sees which plane's
        bounds they are editing."""
        axis = {"xlo": 0, "xhi": 0, "ylo": 1, "yhi": 1, "zlo": 2, "zhi": 2}.get(key)
        self._clear_axis_highlight()
        if axis is None or not self.cut_button.isChecked():
            return
        live = self._read_cut_fields()
        if live is None:
            return
        limits = live[0]
        center = [(limits[0] + limits[1]) / 2, (limits[2] + limits[3]) / 2,
                  (limits[4] + limits[5]) / 2]
        sizes = [limits[1] - limits[0], limits[3] - limits[2], limits[5] - limits[4]]
        others = [i for i in (0, 1, 2) if i != axis]
        i_size = max(sizes[others[0]], 1e-3)
        j_size = max(sizes[others[1]], 1e-3)
        normal = [0.0, 0.0, 0.0]
        normal[axis] = 1.0
        plotter = self.ctx.scene.plotter
        for index, value in enumerate((limits[axis * 2], limits[axis * 2 + 1])):
            corner = list(center)
            corner[axis] = value
            name = f"sep-axis-hl-{index}"
            plotter.add_mesh(
                pv.Plane(center=corner, direction=normal, i_size=i_size, j_size=j_size),
                name=name, color=AXIS_MARKER_COLORS[axis], opacity=0.5, pickable=False)
            self._axis_hl_names.append(name)
        plotter.render()

    def _clear_axis_highlight(self) -> None:
        plotter = self.ctx.scene.plotter
        for name in self._axis_hl_names:
            plotter.remove_actor(name, render=False)
        self._axis_hl_names = []
        plotter.render()

    def _clear_cut_visuals(self) -> None:
        """Remove every Box-Cut overlay: banked/live regions, the axis guide,
        the focus-highlight planes, and pick markers/ghost."""
        self._hide_axis_guide()
        self._clear_axis_highlight()
        self.ctx.scene.set_markers([], name="sep-pick")
        self.ctx.scene.set_markers([], name="sep-pick-ghost")
        self._erase_banked_cuts()

    def _pick_axis_vertex(self, pick) -> None:
        """Collect a vertex-snapped coordinate for the active axis; two picks
        set that axis's min/max."""
        document = self.ctx.document
        if document is None or self._axis_pick is None:
            return
        axis, collected = self._axis_pick
        vertex = self._snap_vertex(pick)
        if vertex is None:
            return
        collected.append(np.asarray(vertex, dtype=float))
        self.ctx.scene.set_markers(collected, name="sep-pick",
                                   color=AXIS_MARKER_COLORS[axis],
                                   point_size=18, on_top=True)
        axis_name = "XYZ"[axis]
        if len(collected) < 2:
            self.status(
                f"Box Cut: {axis_name}={float(vertex[axis]):.3f} — click the second vertex"
            )
            return
        low, high = sorted(float(v[axis]) for v in collected)
        self._cut_fields[("xlo", "ylo", "zlo")[axis]].setText(f"{low:.3f}")
        self._cut_fields[("xhi", "yhi", "zhi")[axis]].setText(f"{high:.3f}")
        self._axis_pick = None
        self._hide_axis_guide()
        self.ctx.scene.set_markers([], name="sep-pick")
        self._update_cut_preview()
        self.status(f"Box Cut: {axis_name} limits set to [{low:.3f}, {high:.3f}]")

    def _snap_vertex(self, pick):
        """The picked body's mesh vertex nearest the click, or None."""
        document = self.ctx.document
        if document is None:
            return None
        mesh = document.bodies[pick.body_index].mesh
        if mesh is None or not len(mesh.points):
            return None
        world = np.asarray(pick.world_point, dtype=float)
        return mesh.points[int(np.argmin(np.linalg.norm(mesh.points - world, axis=1)))]

    def _arm_box_pick(self) -> None:
        if not self.cut_button.isChecked():
            self.status("Box Cut: enable Box Cut mode first")
            return
        self._box_pick = []
        self._axis_pick = None
        self._hide_axis_guide()
        self.status("Box Cut: click two model vertices for the box's opposite corners")

    def _pick_box_corner(self, pick) -> None:
        """Two vertex-snapped corners → the whole axis-aligned limits box."""
        if self._box_pick is None:
            return
        vertex = self._snap_vertex(pick)
        if vertex is None:
            return
        self._box_pick.append(np.asarray(vertex, dtype=float))
        self.ctx.scene.set_markers(self._box_pick, name="sep-pick",
                                   color=BOX_MARKER_COLOR, point_size=18, on_top=True)
        if len(self._box_pick) < 2:
            self.status(
                f"Box Cut: corner 1 at "
                f"({vertex[0]:.2f}, {vertex[1]:.2f}, {vertex[2]:.2f}) — click the opposite corner"
            )
            return
        low = np.minimum(self._box_pick[0], self._box_pick[1])
        high = np.maximum(self._box_pick[0], self._box_pick[1])
        current = self._read_cut_fields()
        normal = current[1] if current else [1.0, 0.0, 0.0]
        location = current[2] if current else 0.0
        self._write_cut_fields(
            [low[0], high[0], low[1], high[1], low[2], high[2]], normal, location)
        self._box_pick = None
        self.ctx.scene.set_markers([], name="sep-pick")
        self._update_cut_preview()
        self.status("Box Cut: box set from the two picked corners")

    def _faces_in_box(self, limits, normal, location):
        """(body, {face ids}) for the body with the most faces inside the box."""
        document = self.ctx.document
        best_body, best_faces = None, set()
        for body_index, body in enumerate(document.bodies):
            mesh = body.mesh
            if mesh is None or not len(mesh.tris):
                continue
            mask = self._contained_mask(mesh, limits, normal, location)
            faces = {int(f) for f in np.unique(mesh.tri_face_ids[mask])}
            if len(faces) > len(best_faces):
                best_body, best_faces = body_index, faces
        return best_body, best_faces

    def _box_face_set(self, mesh, limits):
        """Faces of `mesh` whose triangles fall inside the axis-aligned box."""
        mask = self._contained_mask(mesh, limits, None, None)
        return {int(f) for f in np.unique(mesh.tri_face_ids[mask])}

    @staticmethod
    def _signature(face_set, areas):
        """A box's feature signature: its face areas, largest first."""
        return sorted((areas.get(f, 0.0) for f in face_set), reverse=True)

    @staticmethod
    def _signatures_match(seed_sig, other_sig, area_tol=0.2, min_frac=0.8):
        """True when two boxes hold the SAME feature. Compares the face-area
        lists as a TOLERANCE MULTISET (each face must find a close counterpart),
        not positionally — a box edge catching one extra/fewer tiny face would
        shift a sorted list and break a positional compare, but the same pin's
        faces still pair up. A count guard stops a tiny stray matching a big
        feature. This tells a real duplicate pin from a stray same-area face."""
        if not seed_sig or not other_sig:
            return False
        longer = max(len(seed_sig), len(other_sig))
        if abs(len(seed_sig) - len(other_sig)) > max(2, round(0.4 * longer)):
            return False
        small, big = sorted((seed_sig, other_sig), key=len)
        pool = list(big)
        matched = 0
        for area in small:
            cands = [(abs(area - b), i) for i, b in enumerate(pool)
                     if abs(area - b) <= area_tol * max(area, 1e-9)]
            if cands:
                pool.pop(min(cands)[1])
                matched += 1
        return matched >= min_frac * len(small)

    def _find_matching(self) -> None:
        """Stamp the box on every OTHER occurrence of the boxed pin. A
        representative face finds candidate locations cheaply; each candidate is
        then VALIDATED by stamping the box there and requiring its contained
        face-area signature to match the seed's — so repeated housing features
        that merely share one face area don't get boxed (the over-detection)."""
        from ..pins import find_similar_regions, mesh_face_areas

        document = self.ctx.document
        if document is None:
            return
        live = self._read_cut_fields()
        if live is None:
            self.status("Box Cut: set a box around one pin first, then Find matching")
            return
        limits = live[0]
        seed_body, seed_faces = self._faces_in_box(limits, [0.0, 0.0, 0.0], 0.0)
        if seed_body is None or not seed_faces:
            self.status("Box Cut: nothing inside the box to match — box a pin first")
            return
        mesh = document.bodies[seed_body].mesh
        areas = mesh_face_areas(mesh)
        seed_sig = self._signature(seed_faces, areas)
        seed_center = np.asarray(mesh_region_centroid(mesh, list(seed_faces)), dtype=float)
        rep_face = max(seed_faces, key=lambda f: areas.get(f, 0.0))
        rep_centroid = np.asarray(mesh_region_centroid(mesh, [rep_face]), dtype=float)
        matches = find_similar_regions(
            document, seed_body, {rep_face}, area_tol=0.12, dim_tol=0.18)
        if not matches:
            self.status(
                "Box Cut: no identical features found — box ONE pin tightly and retry")
            return
        box_dims = np.array([limits[1] - limits[0], limits[3] - limits[2],
                             limits[5] - limits[4]])
        box_center = np.array([(limits[0] + limits[1]) / 2, (limits[2] + limits[3]) / 2,
                               (limits[4] + limits[5]) / 2])
        merge_tol = 0.5 * float(min(box_dims))
        # find_similar returns several same-area faces PER pin (e.g. a pin's
        # front + back face), so cluster the matched faces into pins first —
        # otherwise per-face placement lands the box half-on a pin. Faces within
        # ~one box span (per axis) are the same pin; pins sit a pitch apart.
        rep_area = areas.get(rep_face, 0.0)
        seed_big = [f for f in seed_faces
                    if abs(areas.get(f, 0.0) - rep_area) <= 0.12 * max(rep_area, 1e-9)]
        seed_pin_center = np.asarray(
            mesh_region_centroid(mesh, seed_big or list(seed_faces)), dtype=float)
        radius = box_dims * 1.1
        clusters: list[dict] = []
        for region in matches:
            body_index, face_index = region[0]
            if body_index != seed_body:
                continue
            centroid = np.asarray(mesh_region_centroid(mesh, [face_index]), dtype=float)
            for cluster in clusters:
                if np.all(np.abs(centroid - cluster["center"]) <= radius):
                    cluster["pts"].append(centroid)
                    cluster["center"] = np.mean(cluster["pts"], axis=0)
                    break
            else:
                clusters.append({"pts": [centroid], "center": centroid})

        self._cuts = [{"limits": list(limits), "normal": [0.0, 0.0, 0.0],
                       "location": 0.0}]
        centers = [box_center]
        rejected = 0
        for cluster in clusters:
            delta = cluster["center"] - seed_pin_center      # box -> this pin
            new_center = box_center + delta
            if any(float(np.linalg.norm(new_center - c)) < merge_tol for c in centers):
                continue  # the seed pin itself, or already boxed
            shifted = [limits[0] + delta[0], limits[1] + delta[0],
                       limits[2] + delta[1], limits[3] + delta[1],
                       limits[4] + delta[2], limits[5] + delta[2]]
            # validate: the box must hold the SAME feature signature as the seed
            here_sig = self._signature(self._box_face_set(mesh, shifted), areas)
            if not self._signatures_match(seed_sig, here_sig):
                rejected += 1
                continue
            centers.append(new_center)
            self._cuts.append({"limits": shifted, "normal": [0.0, 0.0, 0.0],
                               "location": 0.0})
        self._update_cut_preview()
        note = f" ({rejected} stray match(es) filtered)" if rejected else ""
        self.status(
            f"Box Cut: {len(self._cuts)} pins staged — review the pastels, then Apply"
            + note)

    def _adjacency_ring(self, document, active, bundle):
        """The faces one shared-edge ring out from `bundle`, minus the faces
        already claimed by other bundles."""
        claimed = {
            key
            for index, other in self._manual.items()
            if index != active
            for key in other
        }
        adjacency: dict[int, dict] = {}
        added: set[tuple[int, int]] = set()
        for body_index, face_index in list(bundle):
            if body_index not in adjacency:
                adjacency[body_index] = document.face_smooth_adjacency(body_index, None)
            for neighbor in adjacency[body_index].get(face_index, ()):
                key = (body_index, neighbor)
                if key not in bundle and key not in claimed:
                    added.add(key)
        return added

    def _propagate_bundle(self) -> None:
        """Grow the active bundle by one adjacency ring (any shared edge)."""
        document = self.ctx.document
        if document is None or self._manual is None or not self._bundle_pins:
            self.status("Paint Regions: arm Paint Regions first")
            return
        active = self._bundle_pins[max(0, self.bundle_combo.currentIndex())]
        bundle = self._manual.setdefault(active, set())
        added = self._adjacency_ring(document, active, bundle)
        bundle |= added
        self._paint_manual()
        registry = self.ctx.window.pins
        self.status(
            f"Paint Regions: +{len(added)} face(s) on pin "
            f"{registry.pins[active].number} ({len(bundle)} total)"
        )

    # -------------------------------------------- manual cut planes (box mode)

    def _arm_cut(self, checked: bool) -> None:
        """Switch sub-mode: Box Cut (box a pin, carve it out) <-> Body-Face
        Cutoff. The other mode's controls hide; the model goes solid so the
        pin and its vertices are easy to see/click."""
        document = self.ctx.document
        if document is None:
            self.cut_button.setChecked(False)
            return
        self.cut_group.setVisible(checked)
        self.cutoff_group.setVisible(not checked)
        if not checked:
            self._cuts = []
            self._axis_pick = None
            self._box_pick = None
            self._clear_cut_visuals()
            self.ctx.scene.set_hover_callback(None)
            self.status("Box Cut mode: off")
            self._regrow()  # back to the edge-flow growth preview
            return
        if self.paint_button.isChecked():
            self.paint_button.setChecked(False)
        if self._preview_painted:
            self.ctx.scene.rebuild(document)
            self._preview_painted = False
        self.ctx.scene.set_model_opacity(1.0)  # solid: easy to see the pin
        self.ctx.scene.set_hover_callback(self._on_cut_hover)
        if self._read_cut_fields() is None:
            self._fit_box_to_model()
        else:
            self._update_cut_preview()
        self.status(
            "Box Cut mode: box a pin (⊙ Box / ⊙ 2 pts / type), Add Cut, then Apply"
        )

    def _on_cut_hover(self, pick) -> None:
        """Preview dot at the vertex a pick will snap to, coloured for the
        active pick (yellow box-corner / X-red Y-green Z-blue axis)."""
        scene = self.ctx.scene
        armed = self._box_pick is not None or self._axis_pick is not None
        if (pick is None or self.ctx.document is None
                or not self.cut_button.isChecked() or not armed):
            scene.set_markers([], name="sep-pick-ghost")
            return
        vertex = self._snap_vertex(pick)
        if vertex is None:
            scene.set_markers([], name="sep-pick-ghost")
            return
        color = (AXIS_MARKER_COLORS[self._axis_pick[0]] if self._axis_pick is not None
                 else BOX_MARKER_COLOR)
        scene.set_markers([tuple(float(v) for v in vertex)], name="sep-pick-ghost",
                          color=color, point_size=16.0, opacity=0.5, on_top=True)

    def _read_cut_fields(self):
        """(limits, normal, location) from the X/Y/Z fields, or None if invalid.
        Box Cut has no plane, so normal/location are always 'undefined' (a pure
        box) — kept in the tuple for the shared preview/cut helpers."""
        try:
            field = self._cut_fields
            limits = [float(field["xlo"].text()), float(field["xhi"].text()),
                      float(field["ylo"].text()), float(field["yhi"].text()),
                      float(field["zlo"].text()), float(field["zhi"].text())]
        except (ValueError, KeyError):
            return None
        if limits[1] <= limits[0] or limits[3] <= limits[2] or limits[5] <= limits[4]:
            return None
        return limits, [0.0, 0.0, 0.0], 0.0

    def _write_cut_fields(self, limits, normal=None, location=None) -> None:
        field = self._cut_fields
        for key, value in zip(("xlo", "xhi", "ylo", "yhi", "zlo", "zhi"), limits):
            field[key].setText(f"{value:.3f}")

    def _fit_box_to_model(self) -> None:
        document = self.ctx.document
        if document is None:
            return
        self._write_cut_fields(list(document.bounds()))
        self._update_cut_preview()

    def _contained_mask(self, mesh, limits, normal, location):
        """Boolean mask over a mesh's triangles: centroid inside the box AND on
        the +normal side (the part that becomes the carved body).

        Cake-slice rule: a box cut closes BOTH shells, so a face that sits flush
        on a box wall belongs to the carved pin only if its (outward) normal
        points OUT of the box. A flush face whose normal points INTO the box is
        the package remainder's stripped cap — its material is on the far side
        of the wall, so it must not be classified as part of the pin."""
        tri_pts = mesh.points[mesh.tris]
        centroids = tri_pts.mean(axis=1)
        xlo, xhi, ylo, yhi, zlo, zhi = limits
        mask = ((centroids[:, 0] >= xlo) & (centroids[:, 0] <= xhi)
                & (centroids[:, 1] >= ylo) & (centroids[:, 1] <= yhi)
                & (centroids[:, 2] >= zlo) & (centroids[:, 2] <= zhi))
        if normal is not None:
            unit = np.asarray(normal, dtype=float)
            magnitude = float(np.linalg.norm(unit))
            if magnitude > 1e-9:
                mask = mask & ((centroids @ (unit / magnitude)) >= float(location))

        # Drop flush-but-inward-facing triangles (the package's stripped cap):
        # on a box wall, but the outward normal points into the box.
        face_n = np.cross(tri_pts[:, 1] - tri_pts[:, 0],
                          tri_pts[:, 2] - tri_pts[:, 0])
        face_n = face_n / np.maximum(np.linalg.norm(face_n, axis=1, keepdims=True), 1e-12)
        span = np.array([max(xhi - xlo, 1e-9), max(yhi - ylo, 1e-9),
                         max(zhi - zlo, 1e-9)])
        flush = np.maximum(span * 0.03, 1e-4)  # "on the wall" tolerance per axis
        hit = 0.5  # |normal . axis| above this => face is parallel to that wall
        inward = (
            ((np.abs(centroids[:, 0] - xlo) <= flush[0]) & (face_n[:, 0] >= hit))
            | ((np.abs(centroids[:, 0] - xhi) <= flush[0]) & (face_n[:, 0] <= -hit))
            | ((np.abs(centroids[:, 1] - ylo) <= flush[1]) & (face_n[:, 1] >= hit))
            | ((np.abs(centroids[:, 1] - yhi) <= flush[1]) & (face_n[:, 1] <= -hit))
            | ((np.abs(centroids[:, 2] - zlo) <= flush[2]) & (face_n[:, 2] >= hit))
            | ((np.abs(centroids[:, 2] - zhi) <= flush[2]) & (face_n[:, 2] <= -hit))
        )
        return mask & ~inward

    def _region_polydata(self, mesh, mask):
        tris = mesh.tris[mask]
        if not len(tris):
            return None
        faces = np.hstack(
            [np.full((len(tris), 1), 3, dtype=np.int64), tris]
        ).ravel()
        return pv.PolyData(mesh.points, faces)

    def _update_cut_preview(self) -> None:
        """Pastel-preview every body the banked cuts produce, plus the live
        (field/seeded) cut in red — Apply reproduces exactly this."""
        if self._actions_widget is None or not self.cut_button.isChecked():
            return
        document = self.ctx.document
        if document is None:
            return
        self._erase_banked_cuts()
        # (cut, colour, name): banked cuts pastel, the live box red
        overlays = [(cut, pastel(index), f"sep-cut-{index}")
                    for index, cut in enumerate(self._cuts)]
        live = self._read_cut_fields()
        if live is not None:
            overlays.append(({"limits": live[0], "normal": live[1],
                              "location": live[2]}, (0.88, 0.30, 0.28), "sep-cut-live"))
        plotter = self.ctx.scene.plotter
        for cut, rgb, name in overlays:
            box_name = f"{name}-box"
            plotter.add_mesh(pv.Box(bounds=tuple(cut["limits"])), name=box_name,
                             color=rgb, style="wireframe", line_width=3,
                             pickable=False)
            self._cut_actor_names.append(box_name)
            # the contained region (what actually becomes the carved body)
            for body_index, body in enumerate(document.bodies):
                if body.mesh is None or not len(body.mesh.tris):
                    continue
                mask = self._contained_mask(
                    body.mesh, cut["limits"], cut.get("normal"), cut.get("location"))
                poly = self._region_polydata(body.mesh, mask)
                if poly is None:
                    continue
                actor_name = f"{name}-{body_index}"
                plotter.add_mesh(poly, name=actor_name, color=rgb, pickable=False)
                self._cut_actor_names.append(actor_name)
        plotter.render()
        self._update_cut_label()
        self._update_split_preview()

    def _add_cut(self) -> None:
        live = self._read_cut_fields()
        if live is None:
            self.status("Box Cut: set a valid X/Y/Z box first")
            return
        limits, normal, location = live
        self._cuts.append({"limits": list(limits), "normal": list(normal),
                           "location": float(location)})
        self._update_cut_preview()
        self.status(
            f"Box Cut: banked cut #{len(self._cuts)} — keep adding, then Apply Separate"
        )

    def _clear_cuts(self) -> None:
        self._cuts = []
        self._update_cut_preview()
        self.status("Box Cut: banked cuts cleared")

    def _update_cut_label(self) -> None:
        if self._actions_widget is None:
            return
        count = len(self._cuts)
        if count:
            self.cut_label.setText(
                f"{count} banked box(es) (pastel) + live box (red). "
                "Apply carves exactly the previewed bodies.")
        elif self.cut_button.isChecked():
            self.cut_label.setText("Box a pin (⊙ Box / ⊙ 2 pts / type), then Add Cut.")
        else:
            self.cut_label.setText("")

    def _erase_banked_cuts(self) -> None:
        plotter = self.ctx.scene.plotter
        for name in self._cut_actor_names:
            plotter.remove_actor(name, render=False)
        self._cut_actor_names = []
        plotter.render()

    def _link_cut_pins(self, document, registry, carved, diag) -> int:
        """Link each unclaimed pin to its carved piece — PRIMARY and MOUTH pins
        alike (a connector's mating contacts are separated bodies too, and were
        skipped before, which is why back-row CON pins never got a shell).

        Identity first: remap_pin_faces has already re-pointed each pin's own
        detected faces onto the new body table, and a pin's faces land on its
        carved shell, so the carved body hosting the most of the pin's faces IS
        its body. Proximity (nearest carved-piece centroid) is only the fallback
        for pins whose faces didn't survive the cut."""
        carved_set = set(carved)
        tol = 0.2 * diag + 1e-6
        centroids = {}
        for body_index in carved:
            mesh = document.bodies[body_index].mesh
            if mesh is not None and len(mesh.points):
                centroids[body_index] = mesh.points.mean(axis=0)
        used: set[int] = set()
        matched = 0
        for pin in registry.pins:
            if pin.body_ids:
                continue
            best = None
            # identity: the carved body hosting the most of this pin's faces
            if pin.face_ids:
                counts: dict[int, int] = {}
                for body_index, _face in pin.face_ids:
                    if body_index in carved_set and body_index not in used:
                        counts[body_index] = counts.get(body_index, 0) + 1
                if counts:
                    best = max(counts, key=counts.get)
            # fallback: nearest unused carved piece by centroid
            if best is None:
                pin_centroid = np.asarray(pin.centroid, dtype=float)
                best_distance = tol
                for body_index, centroid in centroids.items():
                    if body_index in used:
                        continue
                    distance = float(np.linalg.norm(centroid - pin_centroid))
                    if distance < best_distance:
                        best_distance, best = distance, body_index
            if best is not None:
                used.add(best)
                pin.body_ids = [best]
                pin.face_ids = []
                body = document.bodies[best]
                body.name = pin.name or f"PIN_{pin.number}"
                body.role = "pin"
                matched += 1
        return matched

    def _journal_cuts(self, document, before, cuts, matched, remapped) -> None:
        self.ctx.journal.record(
            tool=self.id,
            params={"cut": True},
            inputs={"cuts": cuts},
            result={"bodies_before": before, "bodies_after": len(document.bodies),
                    "pins_matched": matched, "face_pins_remapped": remapped},
        )

    def _apply_cuts(self) -> None:
        """Carve every banked box-bounded cut with the boolean cutter, then
        link each carved piece to the nearest pin."""
        document = self.ctx.document
        registry = self.ctx.window.pins
        window = self.ctx.window
        before = len(document.bodies)
        self._undo_state = (
            list(document.bodies),
            [(pin, list(pin.body_ids), list(pin.face_ids)) for pin in registry.pins],
        )
        face_anchors = capture_pin_face_anchors(document, registry.pins)
        # split_by_box REBUILDS + REORDERS the body table on every call, so a
        # pin already linked in an earlier Apply has stale integer body_ids
        # after a later cut displaces its body. Remember the actual BodyRecord
        # OBJECTS now (unchanged bodies keep their identity through a split) and
        # re-resolve those pins' indices once all cuts are done.
        prelinked = [(pin, [document.bodies[i] for i in pin.body_ids
                            if 0 <= i < len(document.bodies)])
                     for pin in registry.pins if pin.body_ids]
        b = document.bounds()
        diag = float(np.linalg.norm([b[1] - b[0], b[3] - b[2], b[5] - b[4]]))
        cuts = [{"limits": list(c["limits"]), "normal": list(c["normal"]),
                 "location": float(c["location"])} for c in self._cuts]
        try:
            window.progress("Carving banked cuts", 0, 0)
            # Capture each cut's carved PIN objects from split_by_box's own
            # return (the pin-side pieces). An object-diff would be wrong: the
            # package remainder is a fresh object after every cut and would be
            # mistaken for a carved pin. Pins survive later (disjoint) cuts, so
            # map the captured objects to their final indices at the end.
            carved_objs: list = []
            for cut in cuts:
                pin_indices = document.split_by_box(
                    cut["limits"], cut["normal"], cut["location"])
                carved_objs.extend(document.bodies[i] for i in pin_indices)
            present = {id(body): index for index, body in enumerate(document.bodies)}
            carved = [present[id(o)] for o in carved_objs if id(o) in present]
            # Re-resolve already-linked pins to their bodies' NEW indices.
            for pin, objs in prelinked:
                new_ids = [present[id(o)] for o in objs if id(o) in present]
                if new_ids:
                    pin.body_ids = new_ids
            # Re-point each remaining pin's faces onto the new body table FIRST,
            # so the linker can claim each pin by identity (the carved body its
            # own faces landed on) before falling back to proximity.
            remapped = remap_pin_faces(document, face_anchors, diag * 5.0e-3)
            matched = self._link_cut_pins(document, registry, carved, diag)
            self._journal_cuts(document, before, cuts, matched, remapped)
            window.progress("Rendering bodies", 0, 0)
            window.document_mutated()
        finally:
            window.progress_done()
        self._cuts = []
        self._clear_cut_visuals()
        self._preview_painted = False
        self._grown = None
        self.undo_button.setEnabled(True)
        # Stay in Box Cut mode (model solid, box/live region re-shown) so the
        # user can cut again — one pass rarely gets every pin.
        self.ctx.scene.set_model_opacity(1.0)
        self._update_cut_preview()
        self._update_split_preview()
        self._update_cut_label()
        self.info_label.setText(
            f"Cut applied: {before} body(ies) -> {len(document.bodies)} "
            f"({len(carved)} carved piece(s), {matched} linked to pins). "
            "Cut again for more pins, or switch off Box Cut mode."
        )
        self.status(
            f"Separate Unibody: carved into {len(document.bodies)} bodies "
            "— cut again, or Undo"
        )

    def _manual_color_map(self, registry):
        """(body -> {face: rgb}, total faces) for the painted bundles."""
        net_pastels = designator_pastels(registry.pins)
        by_body: dict[int, dict[int, tuple[float, float, float]]] = {}
        total = 0
        for index, bundle in self._manual.items():
            pin = registry.pins[index]
            rgb = (net_pastels.get(pin.name, UNJOINED_MOUTH_RGB)
                   if pin.role == "mouth" else pastel(index))
            total += len(bundle)
            for body_index, face_index in bundle:
                by_body.setdefault(body_index, {})[face_index] = rgb
        return by_body, total

    def _paint_manual(self) -> None:
        """Preview EXACTLY the manual bundles."""
        document = self.ctx.document
        registry = self.ctx.window.pins
        if document is None or self._manual is None:
            return
        if self._preview_painted:
            self.ctx.scene.rebuild(document)
        by_body, total = self._manual_color_map(registry)
        for body_index, face_rgb in by_body.items():
            self.ctx.scene.paint_faces(body_index, face_rgb)
        self._preview_painted = True
        # Paint Regions: model stays OPAQUE so faces are easy to see and click
        # (translucency is for the grow preview, not hand painting).
        self.ctx.scene.set_model_opacity(1.0)
        self.info_label.setText(
            f"PAINTED bundles: {sum(1 for b in self._manual.values() if b)} "
            f"bundle(s), {total} faces — Apply splits exactly these."
        )
        self._update_split_preview()

    def run_auto(self) -> None:
        """Choose the growth cutoff automatically and refresh the preview."""
        from ..auto import auto_separate_factor

        document = self.ctx.document
        registry = self.ctx.window.pins
        if document is None or not registry.pins:
            self.status("Separate Unibody: run Detect Pins first")
            return
        self.ctx.window.progress("Auto cutoff: probing growth plateau", 0, 0)
        try:
            factor, faces = auto_separate_factor(document, registry.pins)
        finally:
            self.ctx.window.progress_done()
        self.factor_spin.setValue(factor)  # triggers _regrow via the signal
        self.status(
            f"Auto cutoff: growth plateaus at factor {factor:g} "
            f"({faces} faces) — review the preview, then Apply"
        )

    # -------------------------------------------------------------- preview

    def _split_preview_color_maps(self, registry, regions):
        """(face_rgb, body_rgb): face-region pins colour faces; whole-body pins
        colour their entire solid."""
        net_pastels = designator_pastels(registry.pins)
        face_rgb: dict[int, dict[int, tuple]] = {}
        body_rgb: dict[int, tuple] = {}
        for index, (pin, region) in enumerate(zip(registry.pins, regions)):
            rgb = (net_pastels.get(pin.name, UNJOINED_MOUTH_RGB)
                   if pin.role == "mouth" else pastel(index))
            if region:
                for body_index, f_index in region:
                    face_rgb.setdefault(body_index, {})[f_index] = rgb
            elif pin.body_ids:
                for body_index in pin.body_ids:
                    body_rgb[body_index] = rgb
        return face_rgb, body_rgb

    def _body_preview_colors(self, mesh, body_index, face_rgb, body_rgb, package_rgb):
        """Per-triangle colours for one body in the post-split preview."""
        colors = np.empty((len(mesh.tris), 3), dtype=np.uint8)
        if body_index in body_rgb:
            colors[:] = [int(c * 255) for c in body_rgb[body_index]]
            return colors
        colors[:] = [int(c * 255) for c in package_rgb]
        owned = face_rgb.get(body_index)
        if owned:
            by_color: dict[tuple, list[int]] = {}
            for f_index, rgb in owned.items():
                by_color.setdefault(rgb, []).append(f_index)
            for rgb, faces in by_color.items():
                mask = np.isin(mesh.tri_face_ids, np.asarray(faces, dtype=np.int32))
                colors[mask] = [int(c * 255) for c in rgb]
        return colors

    def _build_merged_preview(self, document, face_rgb, body_rgb, package_rgb):
        """ONE merged, per-cell-coloured mesh for the whole preview (per-region
        actors crawl at DDR5 scale — 577 future bodies would mean 577 actors)."""
        point_blocks, cell_blocks, color_blocks = [], [], []
        offset = 0
        for body_index, body in enumerate(document.bodies):
            mesh = body.mesh
            if mesh is None or not len(mesh.tris):
                continue
            color_blocks.append(self._body_preview_colors(
                mesh, body_index, face_rgb, body_rgb, package_rgb
            ))
            point_blocks.append(mesh.points)
            cell_blocks.append(mesh.tris.astype(np.int64) + offset)
            offset += len(mesh.points)
        if not point_blocks:
            return None
        tris = np.vstack(cell_blocks)
        cells = np.column_stack([np.full(len(tris), 3, dtype=np.int64), tris]).ravel()
        merged = pv.PolyData(np.vstack(point_blocks), cells)
        merged.cell_data["rgb"] = np.vstack(color_blocks)
        return merged

    def _build_cut_preview_mesh(self, document):
        """Per-triangle pastel preview. With cuts banked: each cut's region in
        its pastel (+ the live cut red) on a slate model — the cut decomposition
        to confirm before Apply. With NO cuts (e.g. just after Apply): each
        BODY in its own pastel, so the already-separated bodies are all visible."""
        cuts = list(self._cuts)
        # the live (being-positioned) cut only matters while banking cuts; after
        # Apply the fields still hold the last box, but we want the bodies shown.
        live = self._read_cut_fields() if cuts else None
        live_cut = ({"limits": live[0], "normal": live[1], "location": live[2]}
                    if live is not None else None)
        neutral = np.array([0.42, 0.44, 0.50])
        point_blocks, tri_blocks, color_blocks, offset = [], [], [], 0
        for body_index, body in enumerate(document.bodies):
            mesh = body.mesh
            if mesh is None or not len(mesh.tris):
                continue
            base = neutral if cuts else np.array(pastel(body_index))
            colors = np.tile(base, (len(mesh.tris), 1))
            for index, cut in enumerate(cuts):
                mask = self._contained_mask(
                    mesh, cut["limits"], cut.get("normal"), cut.get("location"))
                colors[mask] = pastel(index)
            if live_cut is not None:
                mask = self._contained_mask(
                    mesh, live_cut["limits"], live_cut["normal"], live_cut["location"])
                colors[mask] = np.array([0.88, 0.30, 0.28])
            point_blocks.append(mesh.points)
            tri_blocks.append(mesh.tris + offset)
            color_blocks.append(colors)
            offset += len(mesh.points)
        if not point_blocks:
            return None
        tris = np.vstack(tri_blocks)
        cells = np.column_stack([np.full(len(tris), 3, dtype=np.int64), tris]).ravel()
        merged = pv.PolyData(np.vstack(point_blocks), cells)
        merged.cell_data["rgb"] = np.vstack(color_blocks)
        return merged

    def _render_split_bodies(self, plotter, document) -> None:
        """Post-split bodies in the preview pane: pin bodies (role=pin) opaque
        and drawn ON TOP, the package translucent — so pins buried inside a
        connector housing (USB CON/HEAD contacts) still show through."""
        bodies = document.bodies
        has_pins = any(getattr(b, "role", "") == "pin" for b in bodies)
        for index, body in enumerate(bodies):
            mesh = body.mesh
            if mesh is None or not len(mesh.tris):
                continue
            cells = np.column_stack(
                [np.full(len(mesh.tris), 3, dtype=np.int64), mesh.tris]).ravel()
            poly = pv.PolyData(mesh.points, cells)
            is_pin = getattr(body, "role", "") == "pin"
            if has_pins and not is_pin:        # the package: translucent backdrop
                plotter.add_mesh(poly, color=(0.42, 0.44, 0.50), opacity=0.18,
                                 smooth_shading=False, show_edges=False)
                continue
            actor = plotter.add_mesh(poly, color=pastel(index), opacity=1.0,
                                     smooth_shading=False, show_edges=False)
            if has_pins:                       # pin: pull on top so it's never hidden
                try:
                    mapper = actor.GetMapper()
                    mapper.SetResolveCoincidentTopologyToPolygonOffset()
                    mapper.SetRelativeCoincidentTopologyPolygonOffsetParameters(
                        -66000.0, -66000.0)
                except Exception:  # noqa: BLE001
                    pass

    def _update_split_preview(self) -> None:
        """The little 3D pane in the actions panel: the POST-SPLIT picture —
        every future body its own solid pastel, package remainder slate."""
        plotter = getattr(self, "preview_plotter", None)
        if plotter is None:
            return
        plotter.clear()
        document = self.ctx.document
        if document is None:
            plotter.render()
            return
        if self._actions_widget is not None and self.cut_button.isChecked():
            if self._cuts:
                merged = self._build_cut_preview_mesh(document)
                if merged is not None:
                    plotter.add_mesh(merged, scalars="rgb", rgb=True,
                                     smooth_shading=False, show_edges=False)
            else:
                self._render_split_bodies(plotter, document)
            plotter.view_isometric()
            plotter.reset_camera()
            plotter.render()
            return
        registry = self.ctx.window.pins
        if not registry.pins:
            plotter.render()
            return
        regions = self._preview_regions()
        face_rgb, body_rgb = self._split_preview_color_maps(registry, regions)
        merged = self._build_merged_preview(
            document, face_rgb, body_rgb, (0.42, 0.44, 0.50)
        )
        if merged is not None:
            plotter.add_mesh(merged, scalars="rgb", rgb=True,
                             smooth_shading=False, show_edges=False)
        plotter.view_isometric()
        plotter.reset_camera()
        plotter.render()

    def _preview_regions(self) -> list:
        """Regions the preview (and Apply) will use: painted bundles when the
        user took over, else the grown regions."""
        registry = self.ctx.window.pins
        if self._manual is not None:
            return [self._manual.get(i) for i in range(len(registry.pins))]
        return self._grown or [None] * len(registry.pins)

    def _grow_progress(self):
        """A loading-bar callback for grow_pin_regions, or None when there is
        no real window (headless). Growing a big unibody connector is the
        slow part of entering this tool — drive the status-bar bar through it
        so a 291-pin DDR5 switch never looks hung."""
        window = self.ctx.window
        if not hasattr(window, "progress"):
            return None

        def tick(done: int, total: int) -> None:
            window.progress("Growing pin regions", done, max(total, 1))

        return tick

    def _paint_mouth(self, pin, region, by_body, net_pastels, stats) -> None:
        """A CON/HEAD pin paints in its net's (tail's) colour. On a unibody it
        grows and splits like a tail; on an already-split contact it stays a
        passive anchor."""
        rgb = net_pastels.get(pin.name, UNJOINED_MOUTH_RGB)
        faces = region if region else pin.face_ids
        for body_index, face_index in faces:
            by_body.setdefault(body_index, {})[face_index] = rgb
        stats["mouth_pins"] += 1
        if region:
            stats["mouth_grown"] += 1
            stats["grown_faces"] += len(region)
            stats["seed_faces"] += len(pin.face_ids)

    def _paint_grown(self, registry):
        """Colour the grown preview; whole-body pins paint directly. Returns
        (face-colour map for face-region pins, stats dict)."""
        net_pastels = designator_pastels(registry.pins)
        by_body: dict[int, dict[int, tuple[float, float, float]]] = {}
        stats = {"grown_faces": 0, "seed_faces": 0, "body_pins": 0,
                 "mouth_pins": 0, "mouth_grown": 0}
        for index, (pin, region) in enumerate(zip(registry.pins, self._grown)):
            if pin.role == "mouth":
                self._paint_mouth(pin, region, by_body, net_pastels, stats)
            elif region is None:
                for body_index in pin.body_ids:
                    self.ctx.scene.set_body_color(body_index, pastel(index))
                stats["body_pins"] += 1
            else:
                stats["seed_faces"] += len(pin.face_ids)
                stats["grown_faces"] += len(region)
                for body_index, face_index in region:
                    by_body.setdefault(body_index, {})[face_index] = pastel(index)
        return by_body, stats

    def _grown_preview_message(self, registry, stats) -> str:
        parts = []
        if stats["grown_faces"]:
            smt = len(registry.pins) - stats["body_pins"] - stats["mouth_pins"]
            parts.append(f"{smt} SMT/THR pin(s): {stats['seed_faces']} seed faces "
                         f"grown to {stats['grown_faces']} by edge flow")
        if stats["body_pins"]:
            parts.append(f"{stats['body_pins']} SMT/THR pin(s) already separate bodies")
        if stats["mouth_pins"]:
            joined = sum(1 for p in registry.pins if p.role == "mouth" and p.name)
            parts.append(
                f"{stats['mouth_pins']} CON/HEAD pin(s) in their net's colour "
                f"({joined} joined, {stats['mouth_grown']} will split, "
                f"{stats['mouth_pins'] - stats['mouth_grown']} on already-split contacts)"
            )
        return "; ".join(parts) if parts else "Nothing to separate."

    def _regrow(self) -> None:
        """Edge-flow growth from the detected pin seeds + pastel preview."""
        document = self.ctx.document
        registry = self.ctx.window.pins
        if document is None or not registry.pins:
            return
        if self._manual is not None:
            self._manual = None  # regrowing replaces the painted bundles
            if self._actions_widget is not None:
                self.paint_button.setChecked(False)
        self._grown = grow_pin_regions(
            document, registry.pins, area_factor=float(self.factor_spin.value()),
            progress=self._grow_progress(),
        )
        if hasattr(self.ctx.window, "progress_done"):
            self.ctx.window.progress_done()
        if self._preview_painted:
            self.ctx.scene.rebuild(document)

        by_body, stats = self._paint_grown(registry)
        for body_index, face_rgb in by_body.items():
            self.ctx.scene.paint_faces(body_index, face_rgb)
        self._preview_painted = True
        # keep the alpha inspection view through live cutoff edits
        self.ctx.scene.set_model_opacity(0.5)
        self.info_label.setText(self._grown_preview_message(registry, stats))
        self._update_split_preview()

    # --------------------------------------------------------------- apply

    def _take_painted_bundles(self, registry):
        """Convert the painted bundles into grown regions + a journal payload,
        then disarm Paint Regions."""
        self._grown = [
            sorted(self._manual.get(index)) if self._manual.get(index) else None
            for index in range(len(registry.pins))
        ]
        bundles = {
            str(registry.pins[index].number): [list(key) for key in sorted(bundle)]
            for index, bundle in self._manual.items()
            if bundle
        }
        self._manual = None
        if self._actions_widget is not None:
            self.paint_button.setChecked(False)
        return bundles

    def _regions_for_apply(self, registry):
        """What to split: painted bundles (exactly) or the grown regions.
        Returns (non-empty regions list, painted_bundles|None)."""
        painted_bundles = None
        if self._manual is not None:
            painted_bundles = self._take_painted_bundles(registry)
        elif self._grown is None:
            self._regrow()
        return [region for region in self._grown if region], painted_bundles

    def _pin_region_colors(self, document, registry, grown_list):
        """Each pin body's display colour = its region's dominant face colour
        (face ids die in the split, so colours move to the BODY)."""
        colors = []
        for pin, region in zip(registry.pins, grown_list):
            color = None
            if region:
                by_body: dict[int, list[int]] = {}
                for body_index, face_index in region:
                    by_body.setdefault(body_index, []).append(face_index)
                for body_index, faces in by_body.items():
                    color = dominant_face_color(document.bodies[body_index].mesh, faces)
                    if color:
                        break
            colors.append(color)
        return colors

    def _remainder_colors(self, document, regions):
        """Dominant colour of each body's faces that no pin region claimed."""
        all_region_faces: dict[int, set[int]] = {}
        for region in regions:
            for body_index, face_index in region:
                all_region_faces.setdefault(body_index, set()).add(face_index)
        return {
            body_index: dominant_face_color(
                document.bodies[body_index].mesh,
                [f for f in range(1, document.bodies[body_index].mesh.face_count + 1)
                 if f not in faces],
            )
            for body_index, faces in all_region_faces.items()
        }

    def _name_pin_body(self, document, pin, body_index, pin_index, pin_region_colors):
        pin.body_ids = [body_index]
        pin.face_ids = []
        suffix = "_HEAD" if pin.role == "mouth" else ""
        body = document.bodies[body_index]
        body.name = (pin.name or f"PIN_{pin.number}") + suffix
        body.role = "pin"
        region_color = (
            pin_region_colors[pin_index] if pin_index < len(pin_region_colors) else None
        )
        if region_color is not None:
            body.color = region_color

    def _link_split_pins(self, document, registry, grown_list, region_bodies,
                         pin_region_colors):
        """Link each grown pin to its sealed solid by region IDENTITY (regions
        was built as [r for r in grown if r], so each grown pin owns one slot —
        no centroid guessing). Returns (matched, unsplit)."""
        region_position: dict[int, int] = {}
        position = 0
        for pin_index, region in enumerate(grown_list):
            if region:
                region_position[pin_index] = position
                position += 1
        matched = unsplit = 0
        for pin_index, pin in enumerate(registry.pins):
            position = region_position.get(pin_index)
            if position is None:
                continue  # already a body, or a passive anchor
            body_index = region_bodies[position]
            if body_index is None:
                unsplit += 1  # stays a face-region pin (still valid metadata)
                continue
            self._name_pin_body(document, pin, body_index, pin_index, pin_region_colors)
            matched += 1
        return matched, unsplit

    def _color_package_pieces(self, document, old_records, pin_indices, remainder_colors):
        """New package pieces inherit the dominant colour of the non-pin faces."""
        fallback = next((c for c in remainder_colors.values() if c is not None), None)
        pin_set = set(pin_indices)
        for body_index, body in enumerate(document.bodies):
            if id(body) in old_records or body_index in pin_set:
                continue
            if body.color is None and fallback is not None:
                body.color = fallback

    def _journal_split(self, document, before, regions, painted_bundles,
                       pin_indices, matched, remapped):
        inputs: dict = {"regions": len(regions)}
        if painted_bundles is not None:
            inputs["bundles"] = painted_bundles
        self.ctx.journal.record(
            tool=self.id,
            params={"area_factor": float(self.factor_spin.value()),
                    "painted": painted_bundles is not None},
            inputs=inputs,
            result={"bodies_before": before, "bodies_after": len(document.bodies),
                    "pin_bodies": len(pin_indices), "pins_matched": matched,
                    "face_pins_remapped": remapped},
        )

    def _pastel_preview_split(self, registry):
        """Pastel-preview the new pin bodies (display only — exported colours
        stay real), still translucent. CON/HEAD pins paint in their net colour."""
        net_pastels = designator_pastels(registry.pins)
        mouth_paint: dict[int, dict[int, tuple[float, float, float]]] = {}
        for pin_index, pin in enumerate(registry.pins):
            if pin.role == "mouth":
                rgb = net_pastels.get(pin.name, UNJOINED_MOUTH_RGB)
                for body_id in pin.body_ids:  # split out as its own sliver
                    self.ctx.scene.set_body_color(body_id, rgb)
                for body_id, face_id in pin.face_ids:  # still a face region
                    mouth_paint.setdefault(body_id, {})[face_id] = rgb
                continue
            for body_id in pin.body_ids:
                self.ctx.scene.set_body_color(body_id, pastel(pin_index))
        for body_id, face_rgb in mouth_paint.items():
            self.ctx.scene.paint_faces(body_id, face_rgb)
        self.ctx.scene.set_model_opacity(0.5)

    def _report_split(self, before, document, pin_indices, matched, unsplit):
        message = (
            f"Split done: {before} body(ies) -> {len(document.bodies)} "
            f"({len(pin_indices)} pin bodies, {matched} pins re-linked; "
            f"cut along the previewed region boundaries)."
        )
        if unsplit:
            message += (
                f" WARNING: {unsplit} pin(s) did not separate (no body "
                f"appeared at their location) — they remain face-region pins."
            )
        self.info_label.setText(message)
        self.status(
            f"Separate Unibody: {len(pin_indices)} whole-pin bodies created — "
            f"{len(document.bodies)} bodies total (Undo available)"
        )

    def apply(self) -> None:
        document = self.ctx.document
        registry = self.ctx.window.pins
        if document is None or not registry.pins:
            return
        # Either/or: in Box Cut mode, Apply is ALWAYS a cut, never the
        # edge-flow growth split. Auto-bank the live cut if not Added yet.
        if self._actions_widget is not None and self.cut_button.isChecked():
            if not self._cuts:
                live = self._read_cut_fields()
                if live is not None:
                    self._cuts.append({"limits": list(live[0]),
                                       "normal": list(live[1]),
                                       "location": float(live[2])})
            if not self._cuts:
                self.status("Box Cut: define a cut (click a face / type values) first")
                return
            self._apply_cuts()
            return
        regions, painted_bundles = self._regions_for_apply(registry)
        if not regions:
            self.status("Separate Unibody: nothing to split — pins are already bodies")
            return

        before = len(document.bodies)
        window = self.ctx.window
        grown_list = list(self._grown)
        pin_region_colors = self._pin_region_colors(document, registry, grown_list)
        remainder_colors = self._remainder_colors(document, regions)
        old_records = {id(body) for body in document.bodies}

        # Reversible: the split only reorganises in-memory geometry, so a
        # snapshot of the body table + pin links restores everything.
        self._undo_state = (
            list(document.bodies),
            [(pin, list(pin.body_ids), list(pin.face_ids)) for pin in registry.pins],
        )
        # (body, face) references die with the old body table — capture every
        # face-region pin's geometry so it can be re-pointed after the split.
        face_anchors = capture_pin_face_anchors(document, registry.pins)
        b = document.bounds()
        remap_tol = float(np.linalg.norm([b[1] - b[0], b[3] - b[2], b[5] - b[4]])) * 5.0e-3

        try:
            # Cut EXACTLY what the preview shows: the grown regions' junction
            # loops (preview/apply must never disagree).
            region_bodies = document.split_by_face_regions(
                regions, progress=window.progress
            )
            self._preview_painted = False
            self._grown = None
            pin_indices = [bi for bi in region_bodies if bi is not None]
            if not pin_indices:
                self.status(
                    "Separate Unibody: split produced nothing — junction loops may "
                    "not be planar; adjust the cutoff and retry"
                )
                return
            matched, unsplit = self._link_split_pins(
                document, registry, grown_list, region_bodies, pin_region_colors
            )
            remapped = remap_pin_faces(document, face_anchors, remap_tol)
            self._color_package_pieces(document, old_records, pin_indices, remainder_colors)
            self._journal_split(document, before, regions, painted_bundles,
                                pin_indices, matched, remapped)
            window.progress("Rendering bodies", 0, 0)
            window.document_mutated()
        finally:
            window.progress_done()

        self._pastel_preview_split(registry)
        self._preview_painted = True  # exit() rebuilds to restore real colours
        self.undo_button.setEnabled(True)
        self._report_split(before, document, pin_indices, matched, unsplit)
        self._update_split_preview()  # now shows the real post-split bodies

    def undo(self) -> None:
        """Restore the pre-split body table and pin links — separation only
        reorganises in-memory geometry, so it is fully reversible."""
        document = self.ctx.document
        if document is None or not getattr(self, "_undo_state", None):
            return
        bodies, pin_links = self._undo_state
        self._undo_state = None
        document.bodies = list(bodies)
        for pin, body_ids, face_ids in pin_links:
            pin.body_ids = body_ids
            pin.face_ids = face_ids
        self._grown = None
        self._preview_painted = False
        self.undo_button.setEnabled(False)
        self.ctx.journal.record(
            tool=self.id, params={"action": "undo"}, inputs={}, result={}
        )
        self.ctx.window.document_mutated()
        self.ctx.scene.set_model_opacity(0.5)
        self._update_split_preview()
        self.info_label.setText("Separation undone — pre-split bodies restored.")
        self.status("Separate Unibody: undone")