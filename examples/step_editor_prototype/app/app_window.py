"""Main window: logo upper-left, tool-mode rectangle upper-middle, 3D view
left, per-tool actions in a right split panel, model info below the actions.
Layout follows DESIGN_INTENT.md."""

from __future__ import annotations

from pathlib import Path

import geometer
import numpy as np
from PySide6.QtCore import QObject, QRunnable, QSettings, Qt, QThreadPool, Signal
from PySide6.QtGui import QKeySequence, QPixmap, QShortcut
from PySide6.QtWidgets import (
    QApplication,
    QFileDialog,
    QHBoxLayout,
    QLabel,
    QMainWindow,
    QMessageBox,
    QProgressBar,
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
from .export_ap242 import conditioned_path, document_step_bytes, export_ap242
from .journal import Journal
from .mode_rect import ModeRect
from .pins import PinRegistry
from .style import ACCENT_HOVER, BORDER, CONSOLE_BG, TEXT_MUTED, TEXT_PRIMARY, WINDOW_BG
from .scene import SceneManager
from .tools import TOOL_CLASSES
from .tools.base import ToolContext


HERE = Path(__file__).resolve().parent.parent
# White version generated from WN3D_LOGO.png for the dark theme.
LOGO_CANDIDATES = [HERE / "wn3d_logo_white.png", HERE / "WN3D_LOGO.png", HERE / "wn3d_logo.png"]
LOGO_PATH = next((p for p in LOGO_CANDIDATES if p.is_file()), LOGO_CANDIDATES[0])
DEFAULT_OPEN_DIR = HERE / "TEST_STEP_FILES"
CAMERA_BUTTONS = ["iso", "top", "bottom", "front", "back", "left", "right"]


class _FootprintSignals(QObject):
    finished = Signal(object, int)  # HlrProjectionResult | None, revision


class _FootprintTask(QRunnable):
    """Top-view HLR projection on a worker thread. Safe off the main thread:
    geometer drives its own subprocess — no OCP, no VTK involved."""

    def __init__(self, step_bytes: bytes, revision: int, signals: _FootprintSignals) -> None:
        super().__init__()
        self.step_bytes = step_bytes
        self.revision = revision
        self.signals = signals

    def run(self) -> None:
        try:
            result = geometer.project_step_hlr(
                self.step_bytes,
                views=[geometer.ProjectionView.top()],
                options=geometer.HlrOptions.visible_detail(),
            )
        except Exception:
            result = None
        self.signals.finished.emit(result, self.revision)


class MainWindow(QMainWindow):
    def __init__(self, step_path: Path | None) -> None:
        super().__init__()
        self.setWindowTitle("Wavenumber 3D STEP Editor")
        self.resize(1280, 800)

        self.document: EditorDocument | None = None
        self.journal = Journal()
        self.model_bounds: geometer.ModelBoundsResult | None = None
        self.pin1_hint: tuple[float, float, float] | None = None
        self.context_plane: tuple[list, list] | None = None  # (point, normal)
        self.pins = PinRegistry()
        self._doc_revision = 0
        self._footprint_cache = None
        self._footprint_cache_rev = -1
        self._footprint_inflight_rev: int | None = None
        self._footprint_callbacks: list = []
        self._footprint_signals = _FootprintSignals()
        self._footprint_signals.finished.connect(self._on_footprint_done)

        self.plotter = QtInteractor(self)
        self.plotter.interactor.setMinimumSize(480, 360)
        self.scene = SceneManager(self.plotter)
        self.scene.on_pick = self._route_pick
        self.scene.on_miss = self._route_miss

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
        title_label = QLabel("Wavenumber 3D STEP Editor")
        title_label.setStyleSheet(
            f"font-weight: 700; font-size: 15px; padding-left: 6px; color: {TEXT_PRIMARY};"
        )
        top_layout.addWidget(title_label)
        top_layout.addStretch(1)

        self.mode_rect = ModeRect()
        top_layout.addWidget(self.mode_rect, 0, Qt.AlignmentFlag.AlignHCenter)
        self.model_label = QLabel("no model")
        self.model_label.setStyleSheet(
            f"font-weight: 600; color: {ACCENT_HOVER}; padding-left: 10px;"
        )
        top_layout.addWidget(self.model_label)
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
            button.setMaximumWidth(82)
            button.clicked.connect(lambda _checked=False, v=view_id: self.snap_camera(v))
            camera_layout.addWidget(button)
        camera_layout.addStretch(1)
        version = geometer.version()
        version_label = QLabel(f"Geometer {version.string} | ABI {version.abi}")
        version_label.setStyleSheet(f"color: {TEXT_MUTED};")
        camera_layout.addWidget(version_label)

        # --- right panel: actions stack + info -----------------------------
        self.actions_stack = QStackedWidget()
        self.info_label = QLabel("No file loaded.")
        self.info_label.setWordWrap(True)
        self.info_label.setAlignment(Qt.AlignmentFlag.AlignTop)
        self.info_label.setStyleSheet(
            f"background: {CONSOLE_BG}; color: {TEXT_PRIMARY}; "
            f"border-top: 1px solid {BORDER}; padding: 8px;"
        )
        self.info_label.setMinimumHeight(84)
        self.info_label.setMaximumHeight(112)

        right_panel = QWidget()
        right_layout = QVBoxLayout(right_panel)
        right_layout.setContentsMargins(0, 0, 0, 0)
        actions_header = QLabel("  TOOL ACTIONS")
        actions_header.setStyleSheet(
            f"font-weight: 700; padding: 4px; background: {WINDOW_BG}; "
            f"color: {TEXT_MUTED}; border-bottom: 1px solid {BORDER};"
        )
        right_layout.addWidget(actions_header)
        right_layout.addWidget(self.actions_stack, 1)
        info_header = QLabel("  MODEL INFO")
        info_header.setStyleSheet(
            f"font-weight: 700; padding: 4px; background: {WINDOW_BG}; "
            f"color: {TEXT_MUTED}; border-bottom: 1px solid {BORDER};"
        )
        right_layout.addWidget(info_header)
        right_layout.addWidget(self.info_label, 0)

        right_panel.setMinimumWidth(280)
        splitter = QSplitter(Qt.Orientation.Horizontal)
        splitter.addWidget(self.plotter.interactor)
        splitter.addWidget(right_panel)
        splitter.setStretchFactor(0, 2)
        splitter.setStretchFactor(1, 1)
        splitter.setCollapsible(0, False)
        self._splitter = splitter
        self._splitter_initialized = False

        central = QWidget()
        layout = QVBoxLayout(central)
        layout.setContentsMargins(0, 0, 0, 0)
        layout.setSpacing(2)
        layout.addWidget(top_bar)
        layout.addWidget(camera_bar)
        layout.addWidget(splitter, 1)
        self.setCentralWidget(central)
        self.setStatusBar(QStatusBar())
        self.progress_bar = QProgressBar()
        self.progress_bar.setMaximumWidth(240)
        self.progress_bar.setMaximumHeight(14)
        self.progress_bar.setStyleSheet(
            f"QProgressBar {{border: 1px solid {BORDER}; background: {CONSOLE_BG};"
            f" color: {TEXT_PRIMARY}; text-align: center; font-size: 9px;}}"
            "QProgressBar::chunk {background-color: #1f9d3a;}"
        )
        self.progress_bar.setVisible(False)
        self.statusBar().addPermanentWidget(self.progress_bar)

        # --- tools ----------------------------------------------------------
        context = ToolContext(document=None, scene=self.scene, journal=self.journal, window=self)
        self.tools = [cls(context) for cls in TOOL_CLASSES]
        self._tool_context = context
        for tool in self.tools:
            self.actions_stack.addWidget(tool.actions_widget())
        self.mode_rect.set_tools([(tool.id, tool.title) for tool in self.tools])
        self.mode_rect.tool_selected.connect(self.set_tool)
        self.mode_rect.next_clicked.connect(self.cycle_tool)
        self.current_tool_index = 0
        self._apply_tool(0, animate=False)

        QShortcut(QKeySequence("T"), self, activated=self.cycle_tool)
        tab_shortcut = QShortcut(QKeySequence(Qt.Key.Key_Tab), self, activated=self.cycle_tool)
        tab_shortcut.setContext(Qt.ShortcutContext.ApplicationShortcut)
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

    def _apply_tool(self, index: int, *, animate: bool = True) -> None:
        tool = self.tools[index]
        next_tool = self.tools[(index + 1) % len(self.tools)]
        self.actions_stack.setCurrentIndex(index)
        self.mode_rect.set_current(
            tool.title, tool.accent, next_tool.title, next_tool.accent,
            animate=animate,
        )
        tool.enter()

    def _route_pick(self, pick) -> None:
        self.tools[self.current_tool_index].on_pick(pick)

    def _route_miss(self) -> None:
        tool = self.tools[self.current_tool_index]
        if hasattr(tool, "on_miss"):
            tool.on_miss()

    # ------------------------------------------------------------------- load

    def open_step_dialog(self) -> None:
        file_name, _filter = QFileDialog.getOpenFileName(
            self,
            "Open STEP",
            self._open_dir(),
            "STEP files (*.step *.stp);;All files (*.*)",
        )
        if file_name:
            path = Path(file_name)
            QSettings("Wavenumber", "StepEditor").setValue("open_dir", str(path.parent))
            self.load_step(path)

    def _open_dir(self) -> str:
        """Last folder the user opened from; defaults to TEST_STEP_FILES."""
        saved = QSettings("Wavenumber", "StepEditor").value("open_dir")
        for candidate in (saved, DEFAULT_OPEN_DIR, HERE):
            if candidate and Path(candidate).is_dir():
                return str(candidate)
        return str(HERE)

    def load_step(self, path: Path) -> None:
        self.show_status(f"Loading {path.name} ...")
        QApplication.setOverrideCursor(Qt.CursorShape.WaitCursor)
        try:
            self.document = EditorDocument.load(path)
            self._doc_revision += 1
            self._tool_context.document = self.document
            self.model_label.setText(path.name)
            self.model_label.setToolTip(str(path))
            self.pins.clear()
            self.pin1_hint = None
            self.context_plane = None
            self.journal = Journal()
            self._tool_context.journal = self.journal
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
            self.model_label.setText("no model")
            self.model_label.setToolTip("")
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
            report = export_ap242(
                self.document, out_path, pins=self.pins, journal=self.journal,
                progress=self.progress,
            )
            self.show_status(f"{report.summary()} | embedded metadata verified")
            if not report.ok:
                QMessageBox.warning(self, "Export validation", report.summary())
        except Exception as exc:
            self.show_status(f"Export failed: {exc}")
            QMessageBox.critical(self, "Export failed", str(exc))
        finally:
            self.progress_done()
            QApplication.restoreOverrideCursor()

    # ------------------------------------------------------------------ misc

    def snap_camera(self, view_id: str) -> None:
        bounds = self.document.bounds() if self.document else (-1, 1, -1, 1, -1, 1)
        size = self.plotter.interactor.size()
        aspect = max(float(size.width()) / max(float(size.height()), 1.0), 0.01)
        self.scene.snap_camera(view_id, bounds, aspect)

    def showEvent(self, event) -> None:
        super().showEvent(event)
        if not self._splitter_initialized:
            self._splitter_initialized = True
            # 3D edit view gets 2/3 of the width, the tool panel 1/3.
            width = max(self._splitter.width(), 1)
            self._splitter.setSizes([(width * 2) // 3, width // 3])

    def show_status(self, message: str) -> None:
        self.statusBar().showMessage(message)

    def progress(self, label: str, value: int, maximum: int) -> None:
        """Long main-thread operations report here; the green bar repaints
        between steps so the app never looks hung. maximum=0 shows a busy
        indicator."""
        self.progress_bar.setVisible(True)
        self.progress_bar.setMaximum(maximum)
        self.progress_bar.setValue(value)
        self.progress_bar.setFormat(f"{label}  %p%" if maximum else label)
        self.show_status(label)
        QApplication.processEvents()

    def progress_done(self) -> None:
        self.progress_bar.setVisible(False)
        QApplication.processEvents()

    def document_mutated(self) -> None:
        """A tool changed the B-rep: re-render and refresh dependent state,
        keeping the current camera."""
        if self.document is None:
            return
        self._doc_revision += 1
        self.scene.rebuild(self.document)
        self._refresh_info()
        for index, tool in enumerate(self.tools):
            if index != self.current_tool_index:
                tool.on_document_changed()

    # ------------------------------------------------------- footprint cache

    def request_footprint(self, callback) -> None:
        """Deliver the top-view HLR projection of the current geometry to
        `callback`, from cache when fresh, else computed on a worker thread.
        Tool switching must never block on this."""
        if self.document is None:
            return
        if self._footprint_cache_rev == self._doc_revision and self._footprint_cache is not None:
            callback(self._footprint_cache)
            return
        self._footprint_callbacks.append(callback)
        if self._footprint_inflight_rev == self._doc_revision:
            return
        self._footprint_inflight_rev = self._doc_revision
        try:
            step_bytes = document_step_bytes(self.document)  # OCP: main thread only
        except Exception as exc:
            self._footprint_inflight_rev = None
            self._footprint_callbacks.clear()
            self.show_status(f"footprint: STEP snapshot failed ({exc})")
            return
        QThreadPool.globalInstance().start(
            _FootprintTask(step_bytes, self._doc_revision, self._footprint_signals)
        )

    def _on_footprint_done(self, result, revision: int) -> None:
        self._footprint_inflight_rev = None
        callbacks = self._footprint_callbacks
        self._footprint_callbacks = []
        if revision != self._doc_revision:
            # Stale: the document changed while projecting — re-request.
            for callback in callbacks:
                self.request_footprint(callback)
            return
        self._footprint_cache = result
        self._footprint_cache_rev = revision
        if result is not None:
            for callback in callbacks:
                callback(result)

    def _refresh_info(self) -> None:
        if self.document is None:
            self.info_label.setText("No file loaded.")
            return
        info = self.document.info()
        units = (self.model_bounds.units if self.model_bounds else None) or "mm"
        xmin, xmax, ymin, ymax, zmin, zmax = info.bounds
        pairs = [
            ("file", info.path.name),
            ("size", f"{xmax - xmin:.2f} x {ymax - ymin:.2f} x {zmax - zmin:.2f} {units}"),
            ("schema", info.schema),
            ("z", f"[{zmin:.3f}, {zmax:.3f}]"),
            ("bodies", str(info.body_count)),
            ("faces", str(info.face_count)),
            ("edges", str(info.edge_count)),
            ("verts", str(info.vertex_count)),
            ("output", conditioned_path(info.path).name),
            ("pins", str(len(self.pins.pins))),
        ]
        cells = []
        for index in range(0, 10, 2):
            left = pairs[index]
            right = pairs[index + 1]
            cells.append(
                f"<tr><td><b>{left[0]}</b></td><td>{left[1]}</td>"
                f"<td><b>{right[0]}</b></td><td>{right[1]}</td></tr>"
            )
        self.info_label.setText(
            "<table cellspacing='0' cellpadding='2' style='font-size: 11px;'>"
            + "".join(cells)
            + "</table>"
        )
