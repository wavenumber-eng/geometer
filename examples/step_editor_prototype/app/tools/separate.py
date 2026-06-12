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
from PySide6.QtWidgets import (
    QComboBox,
    QDoubleSpinBox,
    QHBoxLayout,
    QLabel,
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


def pastel(index: int) -> tuple[float, float, float]:
    """Golden-ratio pastel palette (the wn3d Bodies-view look)."""
    hue = (index * 0.61803398875) % 1.0
    return colorsys.hsv_to_rgb(hue, 0.45, 0.92)


# CON/HEAD pins not yet joined to an SMT/THR tail (no designator to share a
# colour with) paint in this neutral blue — the mouth-detector accent.
UNJOINED_MOUTH_RGB = (0.35, 0.55, 0.95)


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

    # ------------------------------------------------------------------- UI

    def build_actions_widget(self) -> QWidget:
        widget = QWidget()
        layout = QVBoxLayout(widget)
        layout.setContentsMargins(8, 8, 8, 8)
        layout.addWidget(QLabel(
            "<b>Separate Unibody</b><br>"
            "Each SMT/THR pin grows by edge flow from its seed faces until "
            "the flow reaches the BODY; the preview shows the full pin shapes "
            "in body colours. CON/HEAD (mouth) pins show in their net's "
            "colour but are never cut. Apply splits the solid at the "
            "pin/body junctions — every SMT/THR pin becomes its own body."
        ))

        self.info_label = QLabel("")
        self.info_label.setWordWrap(True)
        layout.addWidget(self.info_label)

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
        self.factor_spin.valueChanged.connect(lambda _v: self._regrow())
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
        layout.addLayout(factor_row)

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
        layout.addLayout(paint_row)

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
        # whole model at 0.5 alpha so the pin regions show through the body
        self.ctx.scene.set_model_opacity(0.5)
        self.status(
            "Separate Unibody: check the grown pin shapes (model at 0.5 alpha), "
            "then Apply"
        )

    def exit(self) -> None:
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
        if self._actions_widget is not None:
            self.paint_button.setChecked(False)
            self._update_split_preview()

    # --------------------------------------------------------- paint regions

    def _arm_paint(self, checked: bool) -> None:
        if not checked:
            return  # bundles stay active until Regrow/Apply discards them
        document = self.ctx.document
        registry = self.ctx.window.pins
        if document is None or not registry.pins:
            self.paint_button.setChecked(False)
            return
        if self._grown is None:
            self._grown = grow_pin_regions(
                document, registry.pins,
                area_factor=float(self.factor_spin.value()),
            )
        if self._manual is None:
            # bundles start from what the preview shows: the grown region,
            # or the pin's own faces for passive CON/HEAD anchors
            self._manual = {}
            for index, (pin, region) in enumerate(
                zip(registry.pins, self._grown)
            ):
                if pin.body_ids:
                    continue
                faces = region if region else pin.face_ids
                if faces:
                    self._manual[index] = {tuple(key) for key in faces}
        self._bundle_pins = sorted(self._manual)
        self.bundle_combo.clear()
        for index in self._bundle_pins:
            pin = registry.pins[index]
            tag = " [CON/HEAD]" if pin.role == "mouth" else ""
            name = f" '{pin.name}'" if pin.name else ""
            self.bundle_combo.addItem(f"Pin {pin.number}{name}{tag}")
        self._paint_manual()
        self.status(
            "Paint Regions: pick the bundle in the dropdown, click faces to "
            "add them (click again to remove) — Apply splits exactly these"
        )

    def on_pick(self, pick) -> None:
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

    def _propagate_bundle(self) -> None:
        """Grow the active bundle by one adjacency ring (any shared edge)."""
        document = self.ctx.document
        if document is None or self._manual is None or not self._bundle_pins:
            self.status("Paint Regions: arm Paint Regions first")
            return
        active = self._bundle_pins[max(0, self.bundle_combo.currentIndex())]
        bundle = self._manual.setdefault(active, set())
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
                adjacency[body_index] = document.face_smooth_adjacency(
                    body_index, None
                )
            for neighbor in adjacency[body_index].get(face_index, ()):
                key = (body_index, neighbor)
                if key not in bundle and key not in claimed:
                    added.add(key)
        bundle |= added
        self._paint_manual()
        registry = self.ctx.window.pins
        self.status(
            f"Paint Regions: +{len(added)} face(s) on pin "
            f"{registry.pins[active].number} ({len(bundle)} total)"
        )

    def _paint_manual(self) -> None:
        """Preview EXACTLY the manual bundles."""
        document = self.ctx.document
        registry = self.ctx.window.pins
        if document is None or self._manual is None:
            return
        if self._preview_painted:
            self.ctx.scene.rebuild(document)
        net_pastels = designator_pastels(registry.pins)
        by_body: dict[int, dict[int, tuple[float, float, float]]] = {}
        total = 0
        for index, bundle in self._manual.items():
            pin = registry.pins[index]
            rgb = (
                net_pastels.get(pin.name, UNJOINED_MOUTH_RGB)
                if pin.role == "mouth" else pastel(index)
            )
            total += len(bundle)
            for body_index, face_index in bundle:
                by_body.setdefault(body_index, {})[face_index] = rgb
        for body_index, face_rgb in by_body.items():
            self.ctx.scene.paint_faces(body_index, face_rgb)
        self._preview_painted = True
        self.ctx.scene.set_model_opacity(0.5)
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

    def _update_split_preview(self) -> None:
        """The little 3D pane in the actions panel: render the POST-SPLIT
        picture — every future body as its own solid pastel (the Bodies-view
        look), package remainder in neutral slate — so it's always visible
        where each split body is going."""
        plotter = getattr(self, "preview_plotter", None)
        if plotter is None:
            return
        plotter.clear()
        document = self.ctx.document
        registry = self.ctx.window.pins
        if document is None or not registry.pins:
            plotter.render()
            return

        regions = self._preview_regions()
        net_pastels = designator_pastels(registry.pins)
        # (body -> face -> rgb) for face-region pins; whole-body pins colour
        # their entire solid.
        face_rgb: dict[int, dict[int, tuple]] = {}
        body_rgb: dict[int, tuple] = {}
        for index, (pin, region) in enumerate(zip(registry.pins, regions)):
            if pin.role == "mouth":
                rgb = net_pastels.get(pin.name, UNJOINED_MOUTH_RGB)
            else:
                rgb = pastel(index)
            if region:
                for body_index, f_index in region:
                    face_rgb.setdefault(body_index, {})[f_index] = rgb
            elif pin.body_ids:
                for body_index in pin.body_ids:
                    body_rgb[body_index] = rgb

        package_rgb = (0.42, 0.44, 0.50)  # neutral slate: the body remainder

        def add_tris(points, tris, rgb):
            if not len(tris):
                return
            cells = np.column_stack(
                [np.full(len(tris), 3, dtype=np.int64), tris.astype(np.int64)]
            ).ravel()
            plotter.add_mesh(
                pv.PolyData(points, cells), color=rgb,
                smooth_shading=False, show_edges=False,
            )

        for body_index, body in enumerate(document.bodies):
            mesh = body.mesh
            if mesh is None or not len(mesh.tris):
                continue
            if body_index in body_rgb:
                add_tris(mesh.points, mesh.tris, body_rgb[body_index])
                continue
            owned = face_rgb.get(body_index)
            if not owned:
                add_tris(mesh.points, mesh.tris, package_rgb)
                continue
            colors = sorted({rgb for rgb in owned.values()})
            claimed_faces = np.asarray(sorted(owned), dtype=np.int32)
            for rgb in colors:
                faces = np.asarray(
                    [f for f, c in owned.items() if c == rgb], dtype=np.int32
                )
                add_tris(mesh.points,
                         mesh.tris[np.isin(mesh.tri_face_ids, faces)], rgb)
            add_tris(mesh.points,
                     mesh.tris[~np.isin(mesh.tri_face_ids, claimed_faces)],
                     package_rgb)

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
            document, registry.pins, area_factor=float(self.factor_spin.value())
        )
        if self._preview_painted:
            self.ctx.scene.rebuild(document)

        by_body: dict[int, dict[int, tuple[float, float, float]]] = {}
        grown_faces = 0
        seed_faces = 0
        body_pins = 0
        mouth_pins = 0
        net_pastels = designator_pastels(registry.pins)
        mouth_grown = 0
        for index, (pin, region) in enumerate(zip(registry.pins, self._grown)):
            if pin.role == "mouth":
                # CON/HEAD pins paint in their net's (tail's) colour. On a
                # unibody they grow and split like tails; on an already-split
                # contact they stay passive anchors.
                rgb = net_pastels.get(pin.name, UNJOINED_MOUTH_RGB)
                faces = region if region else pin.face_ids
                for body_index, face_index in faces:
                    by_body.setdefault(body_index, {})[face_index] = rgb
                mouth_pins += 1
                if region:
                    mouth_grown += 1
                    grown_faces += len(region)
                    seed_faces += len(pin.face_ids)
                continue
            rgb = pastel(index)
            if region is None:
                for body_index in pin.body_ids:
                    self.ctx.scene.set_body_color(body_index, rgb)
                body_pins += 1
                continue
            seed_faces += len(pin.face_ids)
            grown_faces += len(region)
            for body_index, face_index in region:
                by_body.setdefault(body_index, {})[face_index] = rgb
        for body_index, face_rgb in by_body.items():
            self.ctx.scene.paint_faces(body_index, face_rgb)
        self._preview_painted = True

        # keep the alpha inspection view through live cutoff edits (the
        # rebuild above resets opacity)
        self.ctx.scene.set_model_opacity(0.5)

        parts = []
        if grown_faces:
            parts.append(
                f"{len(registry.pins) - body_pins - mouth_pins} SMT/THR pin(s): "
                f"{seed_faces} seed faces grown to {grown_faces} by edge flow"
            )
        if body_pins:
            parts.append(f"{body_pins} SMT/THR pin(s) already separate bodies")
        if mouth_pins:
            joined = sum(
                1 for p in registry.pins if p.role == "mouth" and p.name
            )
            parts.append(
                f"{mouth_pins} CON/HEAD pin(s) in their net's colour "
                f"({joined} joined, {mouth_grown} will split, "
                f"{mouth_pins - mouth_grown} on already-split contacts)"
            )
        self.info_label.setText("; ".join(parts) if parts else "Nothing to separate.")
        self._update_split_preview()

    # --------------------------------------------------------------- apply

    def apply(self) -> None:
        document = self.ctx.document
        registry = self.ctx.window.pins
        if document is None or not registry.pins:
            return
        painted_bundles = None
        if self._manual is not None:
            # split EXACTLY the painted bundles
            self._grown = [
                sorted(self._manual.get(index))
                if self._manual.get(index) else None
                for index in range(len(registry.pins))
            ]
            painted_bundles = {
                str(registry.pins[index].number): [list(key) for key in sorted(bundle)]
                for index, bundle in self._manual.items()
                if bundle
            }
            self._manual = None
            if self._actions_widget is not None:
                self.paint_button.setChecked(False)
        elif self._grown is None:
            self._regrow()
        regions = [region for region in self._grown if region]
        if not regions:
            self.status("Separate Unibody: nothing to split — pins are already bodies")
            return

        before = len(document.bodies)
        window = self.ctx.window

        # The split re-tessellates with new face ids, so the original per-face
        # colours can't survive as faces — carry them as BODY colours instead:
        # each pin body takes its region's dominant colour, the package the
        # dominant of what remains.
        grown_list = list(self._grown)
        pin_region_colors: list = []
        for pin, region in zip(registry.pins, grown_list):
            color = None
            if region:
                by_body: dict[int, list[int]] = {}
                for body_index, face_index in region:
                    by_body.setdefault(body_index, []).append(face_index)
                for body_index, faces in by_body.items():
                    color = dominant_face_color(
                        document.bodies[body_index].mesh, faces
                    )
                    if color:
                        break
            pin_region_colors.append(color)
        all_region_faces: dict[int, set[int]] = {}
        for region in regions:
            for body_index, face_index in region:
                all_region_faces.setdefault(body_index, set()).add(face_index)
        remainder_colors = {
            body_index: dominant_face_color(
                document.bodies[body_index].mesh,
                [f for f in range(1, document.bodies[body_index].mesh.face_count + 1)
                 if f not in faces],
            )
            for body_index, faces in all_region_faces.items()
        }
        old_records = {id(body) for body in document.bodies}

        # Reversible: the split only reorganises in-memory geometry, so a
        # snapshot of the body table + pin links restores everything.
        self._undo_state = (
            list(document.bodies),
            [(pin, list(pin.body_ids), list(pin.face_ids)) for pin in registry.pins],
        )

        # (body, face) references die with the old body table — capture every
        # face-region pin's geometry now so mouth pins and unsplit primaries
        # can be re-pointed at the new faces after the split.
        face_anchors = capture_pin_face_anchors(document, registry.pins)
        b = document.bounds()
        remap_tol = float(np.linalg.norm(
            [b[1] - b[0], b[3] - b[2], b[5] - b[4]]
        )) * 5.0e-3

        try:
            # Cut EXACTLY what the preview shows: the grown regions' junction
            # loops. (find_pin_cut cross-section planes are benched until the
            # detector is tuned — they could land cuts the preview never
            # showed, and preview/apply must never disagree.)
            region_bodies = document.split_by_face_regions(
                regions, progress=window.progress
            )
            self._preview_painted = False
            self._grown = None
            pin_indices = [b for b in region_bodies if b is not None]
            if not pin_indices:
                self.status(
                    "Separate Unibody: split produced nothing — junction loops may "
                    "not be planar; adjust the cutoff and retry"
                )
                return

            # Exact linking: regions was built as [r for r in grown if r], so
            # each pin with a grown region owns one slot of region_bodies —
            # by IDENTITY, no centroid guessing (a pin grown through the
            # whole body has its centroid far from the seed; guessing fails).
            region_position: dict[int, int] = {}
            position = 0
            for pin_index, region in enumerate(grown_list):
                if region:
                    region_position[pin_index] = position
                    position += 1
            matched = 0
            unsplit = 0
            for pin_index, pin in enumerate(registry.pins):
                position = region_position.get(pin_index)
                if position is None:
                    continue  # already a body, or a passive anchor
                body_index = region_bodies[position]
                if body_index is None:
                    unsplit += 1  # stays a face-region pin (still valid metadata)
                    continue
                pin.body_ids = [body_index]
                pin.face_ids = []
                suffix = "_HEAD" if pin.role == "mouth" else ""
                document.bodies[body_index].name = (
                    pin.name or f"PIN_{pin.number}"
                ) + suffix
                document.bodies[body_index].role = "pin"
                region_color = (
                    pin_region_colors[pin_index]
                    if pin_index < len(pin_region_colors) else None
                )
                if region_color is not None:
                    document.bodies[body_index].color = region_color
                matched += 1

            remapped = remap_pin_faces(document, face_anchors, remap_tol)

            # Package pieces (newly created, not pins) inherit the dominant
            # colour of the faces that were NOT part of any pin region.
            fallback = next(
                (c for c in remainder_colors.values() if c is not None), None
            )
            for body_index, body in enumerate(document.bodies):
                if id(body) in old_records or body_index in set(pin_indices):
                    continue
                if body.color is None and fallback is not None:
                    body.color = fallback

            inputs: dict = {"regions": len(regions)}
            if painted_bundles is not None:
                inputs["bundles"] = painted_bundles
            self.ctx.journal.record(
                tool=self.id,
                params={"area_factor": float(self.factor_spin.value()),
                        "painted": painted_bundles is not None},
                inputs=inputs,
                result={
                    "bodies_before": before,
                    "bodies_after": len(document.bodies),
                    "pin_bodies": len(pin_indices),
                    "pins_matched": matched,
                    "face_pins_remapped": remapped,
                },
            )
            window.progress("Rendering bodies", 0, 0)
            window.document_mutated()
        finally:
            window.progress_done()
        # pastel-preview the NEW pin bodies (display only — exported colours
        # stay the real ones) so the separation result is verifiable at a
        # glance, still translucent. CON/HEAD pins keep their face regions
        # and paint in their net's (tail's) colour.
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
        self._preview_painted = True  # exit() rebuilds to restore real colours
        self.undo_button.setEnabled(True)
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
        self._update_split_preview()  # now shows the real post-split bodies
        self.status(
            f"Separate Unibody: {len(pin_indices)} whole-pin bodies created — "
            f"{len(document.bodies)} bodies total (Undo available)"
        )

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