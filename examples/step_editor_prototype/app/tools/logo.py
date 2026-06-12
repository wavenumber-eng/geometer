"""Apply LOGO (M7): emboss the WN3D.dxf watermark onto a picked planar face.
The extruded tool body is built by geometer.planar_step from the DXF contours;
engrave cuts it into the surface (raised fuses it on top), and the faces the
boolean creates take the chosen logo colour."""

from __future__ import annotations

from pathlib import Path

import numpy as np
from PySide6.QtCore import QEvent, QObject, Qt
from PySide6.QtGui import QColor
from PySide6.QtWidgets import (
    QColorDialog,
    QComboBox,
    QDoubleSpinBox,
    QHBoxLayout,
    QLabel,
    QPushButton,
    QVBoxLayout,
    QWidget,
)

from ..dxf_loader import emboss_logo, load_watermark_loops
from .base import ToolMode, make_apply_button

HERE = Path(__file__).resolve().parents[2]
DXF_PATH = HERE / "WN3D.dxf"
HANDLE_RADIUS_PX = 16.0


class LogoHandles(QObject):
    """Drag the logo's MOVE (centre) and SCALE (edge) handles directly on the
    face. Presses away from a handle fall through to normal picking/orbit."""

    def __init__(self, widget, tool) -> None:
        super().__init__(widget)
        self._widget = widget
        self._tool = tool
        self._dragging: str | None = None

    def attach(self) -> None:
        self._widget.installEventFilter(self)

    def detach(self) -> None:
        self._widget.removeEventFilter(self)
        self._dragging = None

    def eventFilter(self, obj, event) -> bool:
        if obj is not self._widget:
            return False
        etype = event.type()
        if etype == QEvent.Type.MouseButtonPress and event.button() == Qt.MouseButton.LeftButton:
            self._dragging = self._tool.hit_handle(event.position())
            return self._dragging is not None
        if etype == QEvent.Type.MouseMove and self._dragging:
            self._tool.drag_handle(self._dragging, event.position())
            return True
        if etype == QEvent.Type.MouseButtonRelease and self._dragging:
            self._dragging = None
            return True
        return False


class LogoTool(ToolMode):
    id = "logo"
    title = "Apply LOGO"
    accent = "#1f7a5a"

    def __init__(self, ctx) -> None:
        super().__init__(ctx)
        self._target: tuple[int, int] | None = None  # (body, face)
        self._frame: dict | None = None
        self._loops: list | None = None
        self._logo_rgb = (1.0, 0.84, 0.0)  # gold
        self._handles: LogoHandles | None = None

    # ------------------------------------------------------------------- UI

    def build_actions_widget(self) -> QWidget:
        widget = QWidget()
        layout = QVBoxLayout(widget)
        layout.setContentsMargins(8, 8, 8, 8)
        layout.addWidget(QLabel(
            "<b>Apply LOGO</b><br>"
            "Click a flat face, size the watermark, pick engrave or raised, "
            "then Apply. The contour preview shows the placement."
        ))

        size_row = QHBoxLayout()
        size_row.addWidget(QLabel("Width"))
        self.width_spin = QDoubleSpinBox()
        self.width_spin.setRange(0.1, 1000.0)
        self.width_spin.setValue(2.0)
        self.width_spin.setSuffix(" mm")
        self.width_spin.valueChanged.connect(lambda _v: self._preview())
        size_row.addWidget(self.width_spin)
        size_row.addWidget(QLabel("Depth"))
        self.depth_spin = QDoubleSpinBox()
        self.depth_spin.setRange(0.005, 10.0)
        self.depth_spin.setDecimals(3)
        self.depth_spin.setSingleStep(0.01)
        self.depth_spin.setValue(0.05)
        self.depth_spin.setSuffix(" mm")
        size_row.addWidget(self.depth_spin)
        layout.addLayout(size_row)

        offset_row = QHBoxLayout()
        offset_row.addWidget(QLabel("Offset U"))
        self.offset_u = QDoubleSpinBox()
        self.offset_u.setRange(-1000.0, 1000.0)
        self.offset_u.setSingleStep(0.1)
        self.offset_u.valueChanged.connect(lambda _v: self._preview())
        offset_row.addWidget(self.offset_u)
        offset_row.addWidget(QLabel("V"))
        self.offset_v = QDoubleSpinBox()
        self.offset_v.setRange(-1000.0, 1000.0)
        self.offset_v.setSingleStep(0.1)
        self.offset_v.valueChanged.connect(lambda _v: self._preview())
        offset_row.addWidget(self.offset_v)
        offset_row.addWidget(QLabel("Rot"))
        self.rotation_spin = QDoubleSpinBox()
        self.rotation_spin.setRange(-360.0, 360.0)
        self.rotation_spin.setSuffix(" °")
        self.rotation_spin.setSingleStep(5.0)
        self.rotation_spin.valueChanged.connect(lambda _v: self._preview())
        offset_row.addWidget(self.rotation_spin)
        layout.addLayout(offset_row)

        style_row = QHBoxLayout()
        self.mode_combo = QComboBox()
        self.mode_combo.addItems(["engrave (cut)", "raised (fuse)"])
        style_row.addWidget(self.mode_combo, 1)
        self.color_button = QPushButton("Logo colour")
        self.color_button.clicked.connect(self._pick_color)
        style_row.addWidget(self.color_button)
        layout.addLayout(style_row)
        self._update_color_button()

        self.target_label = QLabel("No face picked.")
        layout.addWidget(self.target_label)

        apply_row = QHBoxLayout()
        auto_button = QPushButton("Auto")
        auto_button.setToolTip(
            "Stage the watermark on the largest upward flat face of the\n"
            "largest body, centered, width 45% of its short side — adjust\n"
            "the handles or Apply."
        )
        auto_button.clicked.connect(self.run_auto)
        apply_row.addWidget(auto_button)
        self.apply_button = make_apply_button("Apply LOGO")
        self.apply_button.setEnabled(False)
        self.apply_button.clicked.connect(self.apply)
        apply_row.addWidget(self.apply_button, 1)
        layout.addLayout(apply_row)
        self.undo_button = QPushButton("Undo LOGO")
        self.undo_button.setEnabled(False)
        self.undo_button.clicked.connect(self.undo)
        layout.addWidget(self.undo_button)
        layout.addStretch(1)
        return widget

    # ------------------------------------------------------------ lifecycle

    def enter(self) -> None:
        if self._loops is None and DXF_PATH.is_file():
            try:
                self._loops = load_watermark_loops(DXF_PATH)
            except Exception as exc:
                self.status(f"LOGO: could not read {DXF_PATH.name} ({exc})")
                self._loops = []
        self.status("LOGO: click a flat face to place the watermark")

    def exit(self) -> None:
        self._target = None
        self._frame = None
        self._detach_handles()
        scene = self.ctx.scene
        scene.remove_overlay("logo-preview")
        scene.set_markers([], name="logo-handle-move")
        scene.set_markers([], name="logo-handle-scale")
        scene.set_markers([], name="logo-handle-rotate")
        scene.clear_highlight()
        scene.plotter.render()

    def _detach_handles(self) -> None:
        if self._handles is not None:
            self._handles.detach()
            self._handles = None

    def on_document_changed(self) -> None:
        self._target = None
        self._frame = None
        self._detach_handles()
        if self._actions_widget is not None:
            self.target_label.setText("No face picked.")
            self.apply_button.setEnabled(False)

    # -------------------------------------------------------------- picking

    def run_auto(self) -> None:
        """Stage the watermark automatically — same staging as a face pick."""
        from ..auto import auto_logo_placement

        document = self.ctx.document
        if document is None:
            return
        placement = auto_logo_placement(document)
        if placement is None:
            self.status("Auto LOGO: no flat top face found — pick one manually")
            return
        body_index, face_index, width = placement
        frame = document.face_plane(body_index, face_index)
        if frame is None:
            return
        self._target = (body_index, face_index)
        self._frame = frame
        for spin in (self.offset_u, self.offset_v):
            spin.blockSignals(True)
            spin.setValue(0.0)
            spin.blockSignals(False)
        self.width_spin.blockSignals(True)
        self.width_spin.setValue(max(0.1, width))
        self.width_spin.blockSignals(False)
        self.ctx.scene.highlight_face(body_index, face_index)
        body = document.bodies[body_index]
        self.target_label.setText(
            f"AUTO: face {face_index} of body {body_index} '{body.name}'"
        )
        self.apply_button.setEnabled(True)
        if self._handles is None:
            self._handles = LogoHandles(self.ctx.scene.plotter.interactor, self)
            self._handles.attach()
        self._preview()
        self.status(
            f"Auto LOGO: largest top face, width {width:.2f} mm — adjust "
            f"the handles or press Apply"
        )

    def on_pick(self, pick) -> None:
        document = self.ctx.document
        if document is None:
            return
        frame = document.face_frame_at(
            pick.body_index, pick.face_index, pick.world_point
        )
        if frame is None:
            self.status("LOGO: no surface normal at that point — pick another spot")
            return
        self._target = (pick.body_index, pick.face_index)
        self._frame = frame
        # the clicked POINT is the logo centre
        clicked = np.asarray(pick.world_point) - np.asarray(frame["origin"])
        u = np.asarray(frame["u"])
        v = np.cross(np.asarray(frame["normal"]), u)
        for spin, value in ((self.offset_u, float(clicked @ u)),
                            (self.offset_v, float(clicked @ v))):
            spin.blockSignals(True)
            spin.setValue(value)
            spin.blockSignals(False)
        self.ctx.scene.highlight_face(pick.body_index, pick.face_index)
        body = document.bodies[pick.body_index]
        self.target_label.setText(
            f"face {pick.face_index} of body {pick.body_index} '{body.name}'"
        )
        self.apply_button.setEnabled(True)
        if self._handles is None:
            self._handles = LogoHandles(self.ctx.scene.plotter.interactor, self)
            self._handles.attach()
        self._preview()
        self.status(
            "LOGO: drag the blue handle to move, the amber handle to scale, "
            "then Apply"
        )

    # -------------------------------------------------------------- preview

    def _preview(self) -> None:
        if self._frame is None or not self._loops:
            return
        stack = np.vstack(self._loops)
        low, high = stack.min(axis=0), stack.max(axis=0)
        center = (low + high) / 2
        scale = float(self.width_spin.value()) / max(high[0] - low[0], 1e-9)
        origin = np.asarray(self._frame["origin"])
        u = np.asarray(self._frame["u"])
        n = np.asarray(self._frame["normal"])
        v = np.cross(n, u)
        origin = (origin + u * float(self.offset_u.value())
                  + v * float(self.offset_v.value()) + n * 0.02)
        theta = np.radians(float(self.rotation_spin.value()))
        c, s = np.cos(theta), np.sin(theta)
        u_eff = u * c + v * s
        v_eff = -u * s + v * c
        segments = []
        for loop in self._loops:
            pts2 = (loop - center) * scale
            pts3 = origin + np.outer(pts2[:, 0], u_eff) + np.outer(pts2[:, 1], v_eff)
            closed = np.vstack([pts3, pts3[:1]])
            segments.extend(
                [[closed[i], closed[i + 1]] for i in range(len(closed) - 1)]
            )
        self.ctx.scene.show_section(np.asarray(segments), name="logo-preview")
        center, edge, rotator = self._handle_points()
        self.ctx.scene.set_markers(
            [center], name="logo-handle-move", color="#1f5fd6", point_size=18.0
        )
        self.ctx.scene.set_markers(
            [edge], name="logo-handle-scale", color="#c89d00", point_size=16.0
        )
        self.ctx.scene.set_markers(
            [rotator], name="logo-handle-rotate", color="#2a9d2a", point_size=16.0
        )

    def _handle_points(self):
        origin = np.asarray(self._frame["origin"])
        u = np.asarray(self._frame["u"])
        n = np.asarray(self._frame["normal"])
        v = np.cross(n, u)
        theta = np.radians(float(self.rotation_spin.value()))
        u_eff = u * np.cos(theta) + v * np.sin(theta)
        v_eff = -u * np.sin(theta) + v * np.cos(theta)
        center = (origin + u * float(self.offset_u.value())
                  + v * float(self.offset_v.value()) + n * 0.03)
        half = float(self.width_spin.value()) / 2.0
        return center, center + u_eff * half, center + v_eff * half

    # ------------------------------------------------------------- handles

    def hit_handle(self, position) -> str | None:
        if self._frame is None:
            return None
        center, edge, rotator = self._handle_points()
        scene = self.ctx.scene
        for name, point in (("scale", edge), ("rotate", rotator), ("move", center)):
            sx, sy = scene.world_to_display_qt(point)
            if (abs(sx - position.x()) ** 2
                    + abs(sy - position.y()) ** 2) ** 0.5 <= HANDLE_RADIUS_PX:
                return name
        return None

    def drag_handle(self, which: str, position) -> None:
        if self._frame is None:
            return
        scene = self.ctx.scene
        hit = scene.display_to_plane(
            position.x(), position.y(),
            self._frame["origin"], self._frame["normal"],
        )
        if hit is None:
            return
        origin = np.asarray(self._frame["origin"])
        u = np.asarray(self._frame["u"])
        n = np.asarray(self._frame["normal"])
        v = np.cross(n, u)
        if which == "move":
            local = hit - origin
            for spin, value in ((self.offset_u, float(local @ u)),
                                (self.offset_v, float(local @ v))):
                spin.blockSignals(True)
                spin.setValue(value)
                spin.blockSignals(False)
        else:
            center, _edge, _rot = self._handle_points()
            vector = hit - (center - n * 0.03)
            if which == "scale":
                width = 2.0 * float(np.linalg.norm(vector))
                self.width_spin.blockSignals(True)
                self.width_spin.setValue(max(0.1, width))
                self.width_spin.blockSignals(False)
            else:  # rotate: the handle sits on the logo's +v axis
                angle = np.degrees(np.arctan2(float(vector @ v), float(vector @ u))) - 90.0
                self.rotation_spin.blockSignals(True)
                self.rotation_spin.setValue(float((angle + 180.0) % 360.0 - 180.0))
                self.rotation_spin.blockSignals(False)
        self._preview()

    def _pick_color(self) -> None:
        color = QColorDialog.getColor(parent=self.actions_widget())
        if color.isValid():
            self._logo_rgb = (color.redF(), color.greenF(), color.blueF())
            self._update_color_button()

    def _update_color_button(self) -> None:
        rgb = [int(c * 255) for c in self._logo_rgb]
        self.color_button.setStyleSheet(
            f"background: rgb({rgb[0]},{rgb[1]},{rgb[2]}); color: #101010;"
        )

    # --------------------------------------------------------------- apply

    def apply(self) -> None:
        document = self.ctx.document
        if document is None or self._target is None:
            return
        body_index, face_index = self._target
        raised = self.mode_combo.currentIndex() == 1
        window = self.ctx.window
        body = document.bodies[body_index]
        self._undo_state = (body_index, body.solid, body.mesh)
        try:
            window.progress("Embossing logo", 0, 0)
            delta = emboss_logo(
                document, body_index, face_index, DXF_PATH,
                width_mm=float(self.width_spin.value()),
                depth_mm=float(self.depth_spin.value()),
                offset_uv=(float(self.offset_u.value()),
                           float(self.offset_v.value())),
                raised=raised,
                logo_rgb=self._logo_rgb,
                rotation_deg=float(self.rotation_spin.value()),
            )
        except Exception as exc:
            self.status(f"LOGO: emboss failed — {exc}")
            return
        finally:
            window.progress_done()
        self.ctx.journal.record(
            tool=self.id,
            params={
                "width_mm": float(self.width_spin.value()),
                "depth_mm": float(self.depth_spin.value()),
                "rotation_deg": float(self.rotation_spin.value()),
                "raised": raised,
                "logo_rgb": list(self._logo_rgb),
            },
            inputs={"body": body_index, "face": face_index,
                    "offset_uv": [float(self.offset_u.value()),
                                  float(self.offset_v.value())],
                    "dxf": DXF_PATH.name},
            result={"volume_delta": delta},
        )
        self._target = None
        self._frame = None
        self._detach_handles()
        self.apply_button.setEnabled(False)
        self.undo_button.setEnabled(True)
        window.document_mutated()
        self.status(
            f"LOGO: {'raised' if raised else 'engraved'} — volume change "
            f"{delta:+.6f} mm^3 (Undo available)"
        )

    def undo(self) -> None:
        """The emboss only replaced one body's in-memory solid — restore it."""
        document = self.ctx.document
        state = getattr(self, "_undo_state", None)
        if document is None or state is None:
            return
        body_index, solid, mesh = state
        self._undo_state = None
        if body_index >= len(document.bodies):
            return
        document.bodies[body_index].solid = solid
        document.bodies[body_index].mesh = mesh
        self.undo_button.setEnabled(False)
        self.ctx.journal.record(
            tool=self.id, params={"action": "undo"}, inputs={}, result={}
        )
        self.ctx.window.document_mutated()
        self.status("LOGO: undone — body restored")