"""Inspect: the M0 placeholder mode. Clicking reports and highlights the
picked body/face — it proves the pick → B-rep face pipeline that every real
tool builds on."""

from __future__ import annotations

from PySide6.QtWidgets import QLabel, QVBoxLayout, QWidget

from .base import ToolMode


class InspectTool(ToolMode):
    id = "inspect"
    title = "Inspect"
    accent = "#44607a"

    def build_actions_widget(self) -> QWidget:
        widget = QWidget()
        layout = QVBoxLayout(widget)
        layout.setContentsMargins(8, 8, 8, 8)
        header = QLabel("<b>Inspect</b>")
        self.info_label = QLabel("Click a face in the 3D view.")
        self.info_label.setWordWrap(True)
        self.info_label.setStyleSheet("font-family: Consolas, monospace;")
        layout.addWidget(header)
        layout.addWidget(self.info_label)
        layout.addStretch(1)
        return widget

    def exit(self) -> None:
        self.ctx.scene.clear_highlight()

    def on_pick(self, pick) -> None:
        document = self.ctx.document
        if document is None:
            return
        body = document.bodies[pick.body_index]
        x, y, z = pick.world_point
        text = (
            f"body {pick.body_index} '{body.name}'\n"
            f"face {pick.face_index} / {body.mesh.face_count if body.mesh else '?'}\n"
            f"@ ({x:.3f}, {y:.3f}, {z:.3f})"
        )
        self.actions_widget()  # ensure built
        self.info_label.setText(text)
        self.ctx.scene.highlight_face(pick.body_index, pick.face_index)
        self.status(
            f"body {pick.body_index} '{body.name}' face {pick.face_index} "
            f"@ ({x:.3f}, {y:.3f}, {z:.3f})"
        )
