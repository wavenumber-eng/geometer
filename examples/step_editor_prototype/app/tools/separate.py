"""Separate Unibody: when the model is one solid, preview the detected pin
regions as a colourful per-body view (the browse-3d 'Bodies' look) and then
actually split the solid — each pin becomes its own body, which makes the
hitbox stage (and the exported AP242) far more useful.

The split uses the context plane from Detect Pins when one was confirmed,
or a horizontal plane whose height is derived from the detected pins and
adjustable with a live section preview."""

from __future__ import annotations

import colorsys

import numpy as np
from PySide6.QtWidgets import (
    QDoubleSpinBox,
    QHBoxLayout,
    QLabel,
    QPushButton,
    QVBoxLayout,
    QWidget,
)

from ..pins import section_segments
from .base import ToolMode, make_apply_button


def pastel(index: int) -> tuple[float, float, float]:
    """Golden-ratio pastel palette (the wn3d Bodies-view look)."""
    hue = (index * 0.61803398875) % 1.0
    return colorsys.hsv_to_rgb(hue, 0.45, 0.92)


class SeparateUnibodyTool(ToolMode):
    id = "separate"
    title = "Separate Unibody"
    accent = "#5a3e9e"

    def __init__(self, ctx) -> None:
        super().__init__(ctx)
        self._preview_painted = False

    # ------------------------------------------------------------------- UI

    def build_actions_widget(self) -> QWidget:
        widget = QWidget()
        layout = QVBoxLayout(widget)
        layout.setContentsMargins(8, 8, 8, 8)
        layout.addWidget(QLabel(
            "<b>Separate Unibody</b><br>"
            "Detected pin regions are previewed in body colours. Apply splits "
            "the solid with the plane below — every pin becomes its own body."
        ))

        self.info_label = QLabel("")
        self.info_label.setWordWrap(True)
        layout.addWidget(self.info_label)

        plane_row = QHBoxLayout()
        plane_row.addWidget(QLabel("Plane Z"))
        self.z_spin = QDoubleSpinBox()
        self.z_spin.setDecimals(4)
        self.z_spin.setRange(-1.0e6, 1.0e6)
        self.z_spin.setSingleStep(0.01)
        self.z_spin.valueChanged.connect(lambda _v: self._update_plane_preview())
        plane_row.addWidget(self.z_spin, 1)
        from_context = QPushButton("From context plane")
        from_context.setToolTip("Reuse the plane confirmed in Detect Pins")
        from_context.clicked.connect(self._use_context_plane)
        plane_row.addWidget(from_context)
        layout.addLayout(plane_row)

        repaint_button = QPushButton("Repaint preview")
        repaint_button.clicked.connect(self._paint_preview)
        layout.addWidget(repaint_button)

        self.apply_button = make_apply_button("Apply Separate")
        self.apply_button.clicked.connect(self.apply)
        layout.addWidget(self.apply_button)
        layout.addStretch(1)
        return widget

    # ------------------------------------------------------------ lifecycle

    def enter(self) -> None:
        document = self.ctx.document
        registry = self.ctx.window.pins
        if document is None:
            return
        if len(document.bodies) > 1:
            self.info_label.setText(
                f"Model already has {len(document.bodies)} bodies — separation "
                f"targets unibody models, but crossing bodies will still split."
            )
        elif not registry.pins:
            self.info_label.setText("No pins detected yet — run Detect Pins first.")
        else:
            self.info_label.setText(
                f"{len(registry.pins)} pin region(s) will become separate bodies."
            )
        self._seed_plane_z()
        self._paint_preview()
        self._update_plane_preview()
        self.status("Separate Unibody: check the preview, adjust the plane, Apply")

    def exit(self) -> None:
        scene = self.ctx.scene
        scene.remove_overlay("separate-section")
        scene.plotter.render()
        if self._preview_painted and self.ctx.document is not None:
            # restore real colours
            self.ctx.scene.rebuild(self.ctx.document)
            self._preview_painted = False

    def on_document_changed(self) -> None:
        self._preview_painted = False

    # -------------------------------------------------------------- preview

    def _paint_preview(self) -> None:
        """Colour each detected pin region with a pastel — the Bodies view of
        what the split will produce."""
        document = self.ctx.document
        registry = self.ctx.window.pins
        if document is None or not registry.pins:
            return
        by_body: dict[int, dict[int, tuple[float, float, float]]] = {}
        for index, pin in enumerate(registry.pins):
            rgb = pastel(index)
            for body_index, face_index in pin.face_ids:
                by_body.setdefault(body_index, {})[face_index] = rgb
            for body_index in pin.body_ids:
                self.ctx.scene.set_body_color(body_index, rgb)
        for body_index, face_rgb in by_body.items():
            self.ctx.scene.paint_faces(body_index, face_rgb)
        self._preview_painted = True

    def _seed_plane_z(self) -> None:
        document = self.ctx.document
        registry = self.ctx.window.pins
        if document is None:
            return
        context = self.ctx.window.context_plane
        if context is not None:
            point, _normal = context
            self.z_spin.blockSignals(True)
            self.z_spin.setValue(float(point[2]))
            self.z_spin.blockSignals(False)
            return
        bounds = document.bounds()
        if registry.pins:
            tops = [pin.centroid[2] for pin in registry.pins]
            seed = float(np.median(tops) + (np.max(tops) - bounds[4]) * 0.5)
        else:
            seed = bounds[4] + (bounds[5] - bounds[4]) * 0.05
        self.z_spin.blockSignals(True)
        self.z_spin.setValue(seed)
        self.z_spin.blockSignals(False)

    def _use_context_plane(self) -> None:
        if self.ctx.window.context_plane is None:
            self.status("Separate Unibody: no context plane confirmed yet")
            return
        self._seed_plane_z()
        self._update_plane_preview()

    def _plane(self) -> tuple[list, list]:
        context = self.ctx.window.context_plane
        if context is not None and abs(float(context[0][2]) - self.z_spin.value()) < 1e-9:
            return context
        # horizontal plane, normal pointing DOWN (pins below)
        return ([0.0, 0.0, float(self.z_spin.value())], [0.0, 0.0, -1.0])

    def _update_plane_preview(self) -> None:
        document = self.ctx.document
        if document is None:
            return
        point, normal = self._plane()
        segments = [
            section_segments(body.mesh, np.asarray(point), np.asarray(normal))
            for body in document.bodies
            if body.mesh is not None and len(body.mesh.tris)
        ]
        segments = [s for s in segments if len(s)]
        merged = np.concatenate(segments) if segments else np.empty((0, 2, 3))
        self.ctx.scene.show_section(merged, name="separate-section")

    # --------------------------------------------------------------- apply

    def apply(self) -> None:
        document = self.ctx.document
        registry = self.ctx.window.pins
        if document is None:
            return
        point, normal = self._plane()
        before = len(document.bodies)
        pin_indices = document.split_by_plane(point, normal)
        if not pin_indices:
            self.status("Separate Unibody: the plane does not split anything")
            return
        self._preview_painted = False

        # Re-associate detected pins with the new bodies by centroid, and
        # name the new pin bodies.
        from ..pins import mesh_region_centroid

        new_centroids = []
        for body_index in pin_indices:
            mesh = document.bodies[body_index].mesh
            centroid = mesh_region_centroid(mesh) if mesh is not None else None
            new_centroids.append(centroid or (0.0, 0.0, 0.0))
        new_centroids = np.asarray(new_centroids)

        matched = 0
        for pin in registry.pins:
            if not len(new_centroids):
                break
            distances = np.linalg.norm(
                new_centroids[:, :2] - np.asarray(pin.centroid[:2]), axis=1
            )
            nearest = int(np.argmin(distances))
            body_index = pin_indices[nearest]
            pin.body_ids = [body_index]
            pin.face_ids = []
            document.bodies[body_index].name = pin.name or f"PIN_{pin.number}"
            document.bodies[body_index].role = "pin"
            matched += 1

        self.ctx.journal.record(
            tool=self.id,
            params={},
            inputs={"plane_point": list(point), "plane_normal": list(normal)},
            result={
                "bodies_before": before,
                "bodies_after": len(document.bodies),
                "pin_bodies": len(pin_indices),
                "pins_matched": matched,
            },
        )
        self.ctx.window.document_mutated()
        self.info_label.setText(
            f"Split done: {before} body(ies) -> {len(document.bodies)} "
            f"({len(pin_indices)} pin bodies, {matched} pins re-linked)."
        )
        self.status(
            f"Separate Unibody: {len(pin_indices)} pin bodies created — "
            f"{len(document.bodies)} bodies total"
        )