"""Main window: logo upper-left, tool-mode rectangle upper-middle, 3D view
left, per-tool actions in a right split panel, model info below the actions.
Layout follows DESIGN_INTENT.md."""

from __future__ import annotations

from pathlib import Path

import geometer
import numpy as np
from PySide6.QtCore import Qt
from PySide6.QtGui import QKeySequence, QPixmap, QShortcut
from PySide6.QtWidgets import (
    QApplication,
    QFileDialog,
    QHBoxLayout,
    QLabel,
    QMainWindow,
    QMessageBox,
    QPushButton,
    QSizePolicy,
    QSplitter,
    QStackedWidget,
    QStatusBar,
    QVBoxLayout,
    QWidget,
)
from pyvistaqt import QtInteractor

from .document import EditorDocument
from .export_ap242 import conditioned_path, export_ap242
from .journal import Journal
from .mode_rect import ModeRect
from .scene import SceneManager
from .tools import TOOL_CLASSES
from .tools.base import ToolContext


HERE = Path(__file__).resolve().parent.parent
LOGO_PATH = HERE / "wn3d_logo.png"
CAMERA_BUTTONS = ["iso", "top", "bottom", "front", "back", "left", "right"]


class MainWindow(QMainWindow):
    def __init__(self, step_path: Path | None) -> None:
        super().__init__()
        self.setWindowTitle("WN3D STEP Conditioning Editor (prototype)")
        self.resize(1280, 800)

        self.document: EditorDocument | None = None
        self.journal = Journal()
        self.model_bounds: geometer.ModelBoundsResult | None = None

        self.plotter = QtInteractor(self)
        self.plotter.interactor.setMinimumSize(480, 360)
        self.scene = SceneManager(self.plotter)
        self.scene.on_pick = self._route_pick

        # --- top bar: logo | mode rect | open/save -------------------------
        top_bar = QWidget()
        top_layout = QHBoxLayout(top_bar)
        top_layout.setContentsMargins(8, 6, 8, 2)
        logo_label = QLabel()
        if LOGO_PATH.is_file():
            pixmap = QPixmap(str(LOGO_PATH))
            logo_label.setPixmap(
                pixmap.scaledToHeight(40, Qt.TransformationMode.SmoothTransformation)
            )
        else:
            logo_label.setText("<b>WN3D</b>")
        top_layout.addWidget(logo_label)
        top_layout.addStretch(1)

        self.mode_rect = ModeRect()
        top_layout.addWidget(self.mode_rect, 0, Qt.AlignmentFlag.AlignHCenter)
        top_layout.addStretch(1)

        open_button = QPushButton("Open STEP")
        open_button.clicked.connect(self.open_step_dialog)
        save_button = QPushButton("Write AP242")
        save_button.setToolTip("Write <name>_AP242_conditioned.step next to the input (Ctrl+S)")
        save_button.clicked.connect(self.write_ap242)
        top_layout.addWidget(open_button)
        top_layout.addWidget(save_button)

        # --- camera row -----------------------------------------------------
        camera_bar = QWidget()
        camera_layout = QHBoxLayout(camera_bar)
        camera_layout.setContentsMargins(8, 0, 8, 2)
        camera_layout.setSpacing(4)
        camera_layout.addWidget(QLabel("Camera"))
        for view_id in CAMERA_BUTTONS:
            button = QPushButton(view_id.upper() if view_id == "iso" else view_id.title())
            button.setMaximumWidth(64)
            button.clicked.connect(lambda _checked=False, v=view_id: self.snap_camera(v))
            camera_layout.addWidget(button)
        camera_layout.addStretch(1)
        version = geometer.version()
        version_label = QLabel(f"Geometer {version.string} | ABI {version.abi}")
        version_label.setStyleSheet("color: #44506a; font-weight: 600;")
        camera_layout.addWidget(version_label)

        # --- right panel: actions stack + info -----------------------------
        self.actions_stack = QStackedWidget()
        self.info_label = QLabel("No file loaded.")
        self.info_label.setWordWrap(True)
        self.info_label.setAlignment(Qt.AlignmentFlag.AlignTop)
        self.info_label.setStyleSheet(
            "font-family: Consolas, monospace; background: #f4f6f8; padding: 8px;"
        )
        self.info_label.setMinimumHeight(140)

        right_panel = QWidget()
        right_layout = QVBoxLayout(right_panel)
        right_layout.setContentsMargins(0, 0, 0, 0)
        actions_header = QLabel("  Tool actions")
        actions_header.setStyleSheet("font-weight: 700; padding: 4px; background: #e8ecf2;")
        right_layout.addWidget(actions_header)
        right_layout.addWidget(self.actions_stack, 3)
        info_header = QLabel("  Model info")
        info_header.setStyleSheet("font-weight: 700; padding: 4px; background: #e8ecf2;")
        right_layout.addWidget(info_header)
        right_layout.addWidget(self.info_label, 2)

        splitter = QSplitter(Qt.Orientation.Horizontal)
        splitter.addWidget(self.plotter.interactor)
        splitter.addWidget(right_panel)
        splitter.setStretchFactor(0, 3)
        splitter.setStretchFactor(1, 1)
        splitter.setSizes([900, 360])

        central = QWidget()
        layout = QVBoxLayout(central)
        layout.setContentsMargins(0, 0, 0, 0)
        layout.setSpacing(2)
        layout.addWidget(top_bar)
        layout.addWidget(camera_bar)
        layout.addWidget(splitter, 1)
        self.setCentralWidget(central)
        self.setStatusBar(QStatusBar())

        # --- tools ----------------------------------------------------------
        context = ToolContext(document=None, scene=self.scene, journal=self.journal, window=self)
        self.tools = [cls(context) for cls in TOOL_CLASSES]
        self._tool_context = context
        for tool in self.tools:
            self.actions_stack.addWidget(tool.actions_widget())
        self.mode_rect.set_tools([(tool.id, tool.title) for tool in self.tools])
        self.mode_rect.tool_selected.connect(self.set_tool)
        self.current_tool_index = 0
        self._apply_tool(0)

        QShortcut(QKeySequence("T"), self, activated=self.cycle_tool)
        QShortcut(QKeySequence("Ctrl+S"), self, activated=self.write_ap242)

        if step_path is not None:
            self.load_step(step_path)

    # ------------------------------------------------------------------ tools

    def set_tool(self, tool_id: str) -> None:
        for index, tool in enumerate(self.tools):
            if tool.id == tool_id:
                self._switch_tool(index)
                return

    def cycle_tool(self) -> None:
        self._switch_tool((self.current_tool_index + 1) % len(self.tools))

    def _switch_tool(self, index: int) -> None:
        if index == self.current_tool_index:
            self._apply_tool(index)
            return
        self.tools[self.current_tool_index].exit()
        self.current_tool_index = index
        self._apply_tool(index)

    def _apply_tool(self, index: int) -> None:
        tool = self.tools[index]
        self.actions_stack.setCurrentIndex(index)
        self.mode_rect.set_current(tool.title, tool.accent)
        tool.enter()

    def _route_pick(self, pick) -> None:
        self.tools[self.current_tool_index].on_pick(pick)

    # ------------------------------------------------------------------- load

    def open_step_dialog(self) -> None:
        start_dir = str(self.document.path.parent) if self.document else str(HERE)
        file_name, _filter = QFileDialog.getOpenFileName(
            self, "Open STEP", start_dir, "STEP files (*.step *.stp);;All files (*.*)"
        )
        if file_name:
            self.load_step(Path(file_name))

    def load_step(self, path: Path) -> None:
        self.show_status(f"Loading {path.name} ...")
        QApplication.setOverrideCursor(Qt.CursorShape.WaitCursor)
        try:
            self.document = EditorDocument.load(path)
            self._tool_context.document = self.document
            try:
                self.model_bounds = geometer.model_bounds(path)
            except Exception:
                self.model_bounds = None
            self.scene.rebuild(self.document)
            self.snap_camera("iso")
            self._refresh_info()
            for tool in self.tools:
                tool.on_document_changed()
            self.show_status(
                f"Loaded {path.name}: {len(self.document.bodies)} bodies, "
                f"{self.document.info().face_count} faces"
            )
        except Exception as exc:
            self.document = None
            self._tool_context.document = None
            self.show_status(f"Load failed: {exc}")
            QMessageBox.critical(self, "Load failed", str(exc))
        finally:
            QApplication.restoreOverrideCursor()

    # ------------------------------------------------------------------ save

    def write_ap242(self) -> None:
        if self.document is None:
            self.show_status("Nothing to write — open a STEP file first.")
            return
        out_path = conditioned_path(self.document.path)
        self.show_status(f"Writing {out_path.name} ...")
        QApplication.setOverrideCursor(Qt.CursorShape.WaitCursor)
        try:
            report = export_ap242(self.document, out_path)
            self.show_status(report.summary())
            if not report.ok:
                QMessageBox.warning(self, "Export validation", report.summary())
        except Exception as exc:
            self.show_status(f"Export failed: {exc}")
            QMessageBox.critical(self, "Export failed", str(exc))
        finally:
            QApplication.restoreOverrideCursor()

    # ------------------------------------------------------------------ misc

    def snap_camera(self, view_id: str) -> None:
        bounds = self.document.bounds() if self.document else (-1, 1, -1, 1, -1, 1)
        size = self.plotter.interactor.size()
        aspect = max(float(size.width()) / max(float(size.height()), 1.0), 0.01)
        self.scene.snap_camera(view_id, bounds, aspect)

    def show_status(self, message: str) -> None:
        self.statusBar().showMessage(message)

    def _refresh_info(self) -> None:
        if self.document is None:
            self.info_label.setText("No file loaded.")
            return
        info = self.document.info()
        lines = [
            f"file    {info.path.name}",
            f"schema  {info.schema}",
            f"bodies  {info.body_count}",
            f"faces   {info.face_count}",
            f"edges   {info.edge_count}",
            f"verts   {info.vertex_count}",
        ]
        if self.model_bounds is not None:
            size = self.model_bounds.bounds.get("size")
            units = self.model_bounds.units or "mm"
            if size:
                lines.append(
                    f"size    {size[0]:.3f} x {size[1]:.3f} x {size[2]:.3f} {units}"
                )
        lines.append(f"output  {conditioned_path(info.path).name}")
        self.info_label.setText("\n".join(lines))
