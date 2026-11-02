"""Main window: logo upper-left, tool-mode rectangle upper-middle, 3D view
left, per-tool actions in a right split panel, model info below the actions.
Layout follows DESIGN_INTENT.md."""

from __future__ import annotations

import os
import shutil
from pathlib import Path

import geometer
import numpy as np
from PySide6.QtCore import QObject, QRunnable, QSettings, Qt, QThreadPool, QTimer, Signal
from PySide6.QtGui import QAction, QColor, QDragEnterEvent, QDropEvent, QKeySequence, QPixmap, QShortcut
from PySide6.QtWidgets import (
    QApplication,
    QComboBox,
    QDialog,
    QDialogButtonBox,
    QFileDialog,
    QHBoxLayout,
    QLabel,
    QListWidget,
    QListWidgetItem,
    QMainWindow,
    QMenu,
    QMessageBox,
    QProgressBar,
    QPushButton,
    QScrollArea,
    QSplitter,
    QStackedWidget,
    QStatusBar,
    QTableWidget,
    QTableWidgetItem,
    QVBoxLayout,
    QWidget,
)
from pyvistaqt import QtInteractor

from . import settings_store
from .camera_cube import CameraCube
from .document import EditorDocument, step_length_units
from .history import History
from .export_ap242 import conditioned_path, document_step_bytes, export_ap242, extract_metadata
from .metadata import STEP_IDS
from .restore import checkpoint_path, clear_checkpoint, read_checkpoint, restore_state, write_checkpoint
from .vendor_files import VendorContainer, bake_conditioned, is_vendor_file, open_vendor_file
from .worklist import next_unconditioned
from .journal import Journal
from .mode_rect import ModeRect
from .pins import PinRegistry
from .style import ACCENT_HOVER, BORDER, CONSOLE_BG, TEXT_MUTED, TEXT_PRIMARY, WINDOW_BG
from .scene import SceneManager
from .compact import compact_panel
from .tools import SHORT_HELP, TOOL_CLASSES
from .tools.base import OperationCancelled, ToolContext


HERE = Path(__file__).resolve().parent.parent
# White version generated from WN3D_LOGO.png for the dark theme.
LOGO_CANDIDATES = [HERE / "wn3d_logo_white.png", HERE / "WN3D_LOGO.png", HERE / "wn3d_logo.png"]
LOGO_PATH = next((p for p in LOGO_CANDIDATES if p.is_file()), LOGO_CANDIDATES[0])
DEFAULT_OPEN_DIR = HERE / "TEST_STEP_FILES"
HELP_DIR = HERE / "assets" / "help"  # per-tool video (placeholder stills until recorded)
STEP_MARKS = [  # (label, step_status value or None for automatic)
    ("Status: not marked (applied if you use the tool)", None),
    ("Status: already satisfied", "inherited"),
    ("Status: skipped by choice", "omitted_by_choice"),
]


ERROR_LOG = HERE / "editor_errors.log"


def log_error(where: str, details: str) -> None:
    """Append an error and its traceback to editor_errors.log next to the
    editor (pythonw has no console, so this is where errors go)."""
    from datetime import datetime

    try:
        with open(ERROR_LOG, "a", encoding="utf-8") as fh:
            fh.write(f"--- {datetime.now():%Y-%m-%d %H:%M:%S} {where}\n{details}\n")
    except OSError:
        pass


def install_error_log() -> None:
    """Uncaught errors (e.g. inside a button handler) go to the log instead
    of vanishing; the window keeps running."""
    import sys
    import traceback

    previous = sys.excepthook

    def hook(kind, value, tb):
        log_error("uncaught", "".join(traceback.format_exception(kind, value, tb)))
        previous(kind, value, tb)

    sys.excepthook = hook


def _help_video(tool_id: str) -> Path | None:
    """A tool's HELP video: assets/help/<NN>_<tool id>.mp4 (numbered in tool
    order), or the plain <tool id>.mp4."""
    numbered = sorted(HELP_DIR.glob(f"[0-9][0-9]_{tool_id}.mp4"))
    plain = HELP_DIR / f"{tool_id}.mp4"
    return numbered[0] if numbered else (plain if plain.is_file() else None)


def _help_still(dialog, still: Path) -> QLabel:
    picture = QLabel()
    picture.setAlignment(Qt.AlignmentFlag.AlignCenter)
    if still.is_file():
        picture.setPixmap(QPixmap(str(still)).scaled(
            dialog.width() - 40, dialog.height() - 90, Qt.AspectRatioMode.KeepAspectRatio,
            Qt.TransformationMode.SmoothTransformation))
    else:
        picture.setText("Video slot: a recording of this tool used by hand goes here.")
    return picture


def _help_player(dialog, layout, video: Path):
    """A looping video with play / pause and a scrub bar; returns the player."""
    from PySide6.QtCore import QUrl
    from PySide6.QtMultimedia import QMediaPlayer
    from PySide6.QtMultimediaWidgets import QVideoWidget
    from PySide6.QtWidgets import QSlider

    view = QVideoWidget(dialog)
    player = QMediaPlayer(dialog)
    player.setVideoOutput(view)
    player.setSource(QUrl.fromLocalFile(str(video)))
    player.setLoops(QMediaPlayer.Loops.Infinite)
    layout.addWidget(view, 1)
    controls = QHBoxLayout()
    toggle = QPushButton("Pause")
    toggle.setFixedWidth(80)
    toggle.clicked.connect(lambda: _toggle_play(player, toggle))
    controls.addWidget(toggle)
    scrub = QSlider(Qt.Orientation.Horizontal)
    player.durationChanged.connect(lambda ms: scrub.setRange(0, ms))
    player.positionChanged.connect(lambda ms: scrub.isSliderDown() or scrub.setValue(ms))
    scrub.sliderMoved.connect(player.setPosition)
    controls.addWidget(scrub, 1)
    layout.addLayout(controls)
    player.play()
    return player


def _toggle_play(player, button) -> None:
    from PySide6.QtMultimedia import QMediaPlayer

    if player.playbackState() == QMediaPlayer.PlaybackState.PlayingState:
        player.pause()
        button.setText("Play")
    else:
        player.play()
        button.setText("Pause")


def _ask_env(name: str) -> bool | None:
    """Headless runs answer the restore / recover prompts from the
    environment (WN3D_AUTO_RESTORE / WN3D_AUTO_RECOVER = yes|no)."""
    value = os.environ.get(name, "").strip().lower()
    return {"yes": True, "1": True, "no": False, "0": False}.get(value)


_COUNT_KEYS = ("pads", "matches", "applied", "assigned", "pending", "pins", "bodies_after", "count", "faces")


def _op_summary(op) -> str:
    """'embed: 18 pads', 'detect_similar: 24 matches' ... from a journal op."""
    action = str(op.params.get("action") or op.params.get("mode") or "applied")
    for key in _COUNT_KEYS:
        value = (op.result or {}).get(key)
        if isinstance(value, (list, dict)):
            value = len(value)
        if isinstance(value, int) and not isinstance(value, bool):
            return f"{action}: {value} {key}"
    return action


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
        # Fits a 13" laptop: tool panels scroll instead of forcing the height.
        self.setMinimumSize(760, 480)
        self._cancel_requested = False
        self._restored_ops = 0  # journal ops that came from the opened file

        self.document: EditorDocument | None = None
        self.vendor: VendorContainer | None = None
        self._vendor_workdir = None  # TemporaryDirectory kept alive with the container
        self.journal = Journal()
        self.model_units: str | None = None
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
        self.plotter.interactor.setMinimumSize(320, 240)
        self.scene = SceneManager(self.plotter)
        self.scene.on_pick = self._route_pick
        self.scene.on_miss = self._route_miss
        self.scene.on_right_miss = self._route_right_miss

        # --- top bar: logo | tool carousel | skip | Settings | Options -------
        self.settings = settings_store.load()
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
        logo_label.setToolTip("Wavenumber 3D STEP Editor")
        top_layout.addWidget(logo_label)
        title_label = QLabel("Wavenumber 3D STEP Editor")
        title_label.setStyleSheet("font-weight: 700; font-size: 15px; padding: 0 8px 0 6px; color: #ffffff;")
        top_layout.addWidget(title_label)

        self.mode_rect = ModeRect()
        top_layout.addWidget(self.mode_rect, 1)
        self.skip_button = QPushButton("Skip step")
        self.skip_button.setToolTip(
            "Gold-corpus mode: record this step as 'skipped by choice' (not forgotten)\n"
            "and open the next one. Press again on a skipped step to undo.")
        self.skip_button.setStyleSheet(
            "QPushButton {font-weight: 700; padding: 4px 10px;}"
            "QPushButton[skipped=\"true\"] {background-color: #8a6d1f; color: #ffffff;}")
        self.skip_button.clicked.connect(self._skip_current_step)
        top_layout.addWidget(self.skip_button, 0, Qt.AlignmentFlag.AlignVCenter)

        settings_button = QPushButton("Settings")
        settings_button.setToolTip("System information, steps, tool parameter defaults, "
                                   "ML models and keybinds")
        settings_button.clicked.connect(self.open_settings)
        options_button = QPushButton("Options")
        options_button.setToolTip("Open, next 3D model, write, undo / redo")
        self.options_menu = QMenu(options_button)
        self._options_actions = {}
        for key, label, handler in (
                ("open", "Open Model…", self.open_step_dialog),
                ("next_model", "Next 3D Model", self.open_next_unconditioned),
                (None, None, None),
                ("write_new", "Write AP242 (new file: <name>_AP242_WNCn.step)", self.write_ap242),
                ("write_replace", "Write AP242 (replace the original, keep a .bak)", self.write_ap242_replace),
                ("write_same", "Write to Same Type   [Model will not contain metadata]", self.write_same_type),
                (None, None, None),
                ("undo", "Undo", self.undo),
                ("redo", "Redo", self.redo)):
            if key is None:
                self.options_menu.addSeparator()
                continue
            action = QAction(label, self.options_menu)
            action.triggered.connect(handler)
            self.options_menu.addAction(action)
            self._options_actions[key] = action
        self.options_menu.aboutToShow.connect(self._refresh_options)
        options_button.setMenu(self.options_menu)
        top_layout.addWidget(settings_button, 0, Qt.AlignmentFlag.AlignVCenter)
        top_layout.addWidget(options_button, 0, Qt.AlignmentFlag.AlignVCenter)

        # --- load row: progress bar (always drawn) | model name ------------
        load_bar = QWidget()
        load_layout = QHBoxLayout(load_bar)
        load_layout.setContentsMargins(8, 0, 8, 2)
        load_layout.setSpacing(6)
        self.progress_bar = QProgressBar()
        self.progress_bar.setTextVisible(True)
        self.progress_bar.setFormat("")
        self.progress_bar.setMaximum(1)
        self.progress_bar.setValue(0)
        self.progress_bar.setFixedHeight(16)
        self.progress_bar.setStyleSheet(
            f"QProgressBar {{border: 1px solid {BORDER}; background: {CONSOLE_BG};"
            f" color: {TEXT_PRIMARY}; text-align: center; font-size: 9px;}}"
            "QProgressBar::chunk {background-color: #1f9d3a;}"
        )
        load_layout.addWidget(self.progress_bar, 2)
        # what the last tool action did: applied (green), nothing (amber),
        # staged but NOT applied when leaving the tool (red)
        self.applied_label = QLabel("")
        self.applied_label.setFixedHeight(16)
        self.applied_label.setToolTip("Did the tool apply anything? Updated on every applied change "
                                      "and when you leave a tool.")
        load_layout.addWidget(self.applied_label, 1)
        self._ops_at_entry = 0
        self.cancel_button = QPushButton("Cancel")
        self.cancel_button.setToolTip("Stop the running operation; the model is left as it was")
        self.cancel_button.setFixedHeight(18)
        self.cancel_button.setVisible(False)
        self.cancel_button.clicked.connect(self._request_cancel)
        load_layout.addWidget(self.cancel_button)
        self.model_label = QLabel("no model")
        self.model_label.setStyleSheet(f"font-weight: 600; color: {ACCENT_HOVER}; padding-left: 10px;")
        self.model_label.setAlignment(Qt.AlignmentFlag.AlignRight | Qt.AlignmentFlag.AlignVCenter)
        load_layout.addWidget(self.model_label, 1)

        # --- camera cube over the 3D view ---------------------------------
        self.camera_cube = CameraCube(self.plotter.interactor)
        self.camera_cube.move(8, 8)
        self.camera_cube.view_requested.connect(self._view_from)
        self.camera_cube.raise_()

        # --- right panel: TOOL ACTIONS frame -----------------------------
        self.actions_stack = QStackedWidget()
        right_panel = QWidget()
        right_layout = QVBoxLayout(right_panel)
        right_layout.setContentsMargins(0, 0, 0, 0)
        right_layout.setSpacing(3)
        right_layout.addWidget(self._build_action_header())
        right_layout.addWidget(self.actions_stack, 1)
        right_layout.addWidget(self._build_staged_footer())
        right_panel.setMinimumWidth(300)

        splitter = QSplitter(Qt.Orientation.Horizontal)
        splitter.addWidget(self.plotter.interactor)
        splitter.addWidget(right_panel)
        splitter.setStretchFactor(0, 2)
        splitter.setStretchFactor(1, 1)
        splitter.setCollapsible(0, False)
        self._splitter = splitter
        self._splitter_initialized = False

        # --- model info: three small lines at the very bottom -------------
        self.info_label = QLabel("No file loaded.")
        self.info_label.setWordWrap(False)
        self.info_label.setTextFormat(Qt.TextFormat.RichText)
        self.info_label.setStyleSheet(
            f"background: {CONSOLE_BG}; color: {TEXT_MUTED}; font-size: 10px;"
            f" border-top: 1px solid {BORDER}; padding: 2px 8px;")
        self.info_label.setFixedHeight(46)

        central = QWidget()
        layout = QVBoxLayout(central)
        layout.setContentsMargins(0, 0, 0, 0)
        layout.setSpacing(2)
        layout.addWidget(top_bar)
        layout.addWidget(load_bar)
        layout.addWidget(splitter, 1)
        layout.addWidget(self.info_label)
        self.setCentralWidget(central)
        self.setStatusBar(QStatusBar())

        # --- tools ----------------------------------------------------------
        context = ToolContext(document=None, scene=self.scene, journal=self.journal, window=self)
        self.tools = [cls(context) for cls in TOOL_CLASSES]
        self._tool_context = context
        self._status_combos: dict[str, QComboBox] = {}
        for tool in self.tools:
            self.actions_stack.addWidget(self._tool_page(tool))
        self.mode_rect.set_tools([(tool.id, tool.title) for tool in self.tools])
        self.auto_steps: set[str] = set(self.settings.get("auto_steps") or [])
        self.mode_rect.set_automatic(self.auto_steps)
        self.mode_rect.tool_selected.connect(self.set_tool)
        self.mode_rect.next_clicked.connect(self.cycle_tool)
        self.mode_rect.prev_clicked.connect(self.cycle_tool_back)
        self.current_tool_index = self._step_from(0, 1)
        self._switching = False
        self.history = History()
        self._shortcuts: list[QShortcut] = []
        self.apply_settings(self.settings, first=True)
        self._apply_tool(self.current_tool_index, animate=False)

        self._staged_timer = QTimer(self)
        self._staged_timer.timeout.connect(self._refresh_staged)
        self._staged_timer.start(400)

        # Drop a STEP/.stp (or .kicad_mod/.PcbLib container) anywhere on the
        # window to open it. The VTK interactor must not claim drops or they
        # never reach the main window.
        self.setAcceptDrops(True)
        self.plotter.interactor.setAcceptDrops(False)

        if step_path is not None:
            self.load_any(step_path)

    # ------------------------------------------------------------------ tools

    # tool buttons the shared frame replaces (AUTO / CONFIRM drive them)
    _FRAME_BUTTONS = {"Auto", "Auto Front"}  # Hitboxes keeps "Auto ALL pins" visible beside Auto selected
    # tooltips for controls that were built without one: by button text, then
    # by the tool attribute that holds the control
    _TIPS_BY_TEXT = {
        "Paintbrush (each click paints)": "Every click paints the clicked face or body at once (no CONFIRM)",
        "Custom color ...": "Pick any color from the color dialog",
        "Select all pins": "Select every detected pin (bodies or pin faces)",
        "Clear selection": "Drop the current selection (right-click on the background does the same)",
        "Reset to original": "Give the selected bodies back the colors the file came with",
        "Set": "Apply the typed value to the selected pin(s)",
        "Assign": "Apply the bulk list (1:GND, 2:VCC ...)",
        "Clear hitbox": "Remove the selected pin's hitbox",
        "Clear Cuts": "Remove every banked cut",
        "Delete cut": "Remove the selected cut",
        "Delete": "Remove the selected pin",
        "Clear": "Remove every pin",
        "Snap to vertices": "Snap each pick to the nearest mesh vertex (within the slider's distance)",
        "Reset": "Start the picks over",
        "Undo pick": "Remove the last pick",
    }
    _TIPS_BY_ATTR = {
        "target_combo": "Paint whole bodies, or single faces",
        "variant_combo": "Hitbox shape: convex hull of each pin (default), shrink wrap (concave), boxes, BGA cubes",
        "margin_spin": "Grow each hitbox this far beyond its pin (mm)",
        "size_spin": "BGA cube size / female depth / pad prism height (mm)",
        "cut_pin_combo": "Which pin this cut carves (auto = the pin inside it)",
        "point_combo": "Which picked point becomes the origin",
        "function_combo": "A pin type (GND, PWR, SDA ...) for every selected pin; Set applies it",
        "category": "What kind of part this is",
        "cyl_radius_spin": "Radius of the selected cylinder cut",
    }

    def _tool_page(self, tool) -> QScrollArea:
        """A tool's own controls (the middle of the TOOL ACTIONS frame), in a
        scroll area so a tall tool never forces the window taller. The tool's
        own apply button stays visible (apply and stay in the tool, like
        Enter); CONFIRM presses it too and then moves on. Its auto button is
        hidden: AUTO drives it."""
        page = QWidget()
        layout = QVBoxLayout(page)
        layout.setContentsMargins(0, 0, 0, 0)
        layout.setSpacing(2)
        actions = tool.actions_widget()
        compact_panel(actions, SHORT_HELP.get(tool.id))
        for button in actions.findChildren(QPushButton):
            if button.text() in self._FRAME_BUTTONS:
                button.setVisible(False)
                button.setProperty("wn3d_frame_hidden", True)
        from PySide6.QtWidgets import QAbstractButton

        for button in actions.findChildren(QAbstractButton):
            if not button.toolTip().strip() and button.text() in self._TIPS_BY_TEXT:
                button.setToolTip(self._TIPS_BY_TEXT[button.text()])
        for attr, tip in self._TIPS_BY_ATTR.items():
            control = getattr(tool, attr, None)
            if control is not None and not control.toolTip().strip():
                control.setToolTip(tip)
        layout.addWidget(actions, 1)
        if tool.id in STEP_IDS:
            combo = QComboBox()
            for label, _value in STEP_MARKS:
                combo.addItem(label)
            combo.setToolTip(
                "Gold-corpus mode: every step passes once the model is exported: it was "
                "applied, was already satisfied by the source, or was skipped by choice.")
            combo.setSizeAdjustPolicy(QComboBox.SizeAdjustPolicy.AdjustToMinimumContentsLengthWithIcon)
            combo.setMinimumContentsLength(12)
            combo.currentIndexChanged.connect(
                lambda index, step=tool.id: self._set_step_mark(step, index))
            self._status_combos[tool.id] = combo
            layout.addWidget(combo)
        scroll = QScrollArea()
        scroll.setWidgetResizable(True)
        scroll.setHorizontalScrollBarPolicy(Qt.ScrollBarPolicy.ScrollBarAlwaysOff)
        scroll.setWidget(page)
        return scroll

    @staticmethod
    def _frame_button(text: str, color: str, tooltip: str) -> QPushButton:
        button = QPushButton(text)
        button.setToolTip(tooltip)
        hover = QColor(color).lighter(125).name()
        button.setStyleSheet(
            f"QPushButton {{background-color: {color}; color: #ffffff; font-weight: 700;"
            f" padding: 4px 9px; border: 1px solid #00000040;}}"
            f"QPushButton:hover {{background-color: {hover};}}"
            "QPushButton:disabled {background-color: #3a3a40; color: #8a8a90;}")
        return button

    def _build_action_header(self) -> QWidget:
        """TOOL ACTIONS · AUTO · the step's ML model · HELP · CONFIRM."""
        header = QWidget()
        header.setStyleSheet(f"background: {WINDOW_BG}; border-bottom: 1px solid {BORDER};")
        row = QHBoxLayout(header)
        row.setContentsMargins(6, 4, 6, 4)
        row.setSpacing(6)
        title = QLabel("TOOL ACTIONS")
        title.setStyleSheet(f"font-weight: 700; color: {TEXT_MUTED}; border: none;")
        row.addWidget(title)
        self.auto_frame_button = self._frame_button(
            "AUTO", "#7a4fc4", "Run this step automatically with its model (Settings → Models)")
        self.auto_frame_button.clicked.connect(self.run_tool_auto)
        row.addWidget(self.auto_frame_button)
        self.step_model_label = QLabel("")
        self.step_model_label.setStyleSheet(f"color: {TEXT_MUTED}; font-size: 10px; border: none;")
        self.step_model_label.setMinimumWidth(10)
        row.addWidget(self.step_model_label, 1)
        help_button = self._frame_button("HELP", "#2f6fa8", "Watch this tool used by hand on an example model")
        help_button.clicked.connect(self.show_tool_help)
        row.addWidget(help_button)
        self.confirm_button = self._frame_button("CONFIRM", "#1f9d3a", "Apply the staged changes (Enter)")
        self.confirm_button.clicked.connect(self.confirm_staged)
        row.addWidget(self.confirm_button)
        return header

    def _build_staged_footer(self) -> QWidget:
        """DENY · DETAILED, then the staged-changes list (3 lines; DETAILED
        expands it)."""
        footer = QWidget()
        footer.setStyleSheet(f"border-top: 1px solid {BORDER};")
        column = QVBoxLayout(footer)
        column.setContentsMargins(6, 4, 6, 4)
        column.setSpacing(3)
        row = QHBoxLayout()
        caption = QLabel("STAGED CHANGES")
        caption.setStyleSheet(f"font-weight: 700; color: {TEXT_MUTED}; border: none;")
        row.addWidget(caption)
        row.addStretch(1)
        self.deny_button = self._frame_button("DENY", "#5a5a60", "Drop every staged change (Esc)")
        self.deny_button.clicked.connect(self.deny_staged)
        row.addWidget(self.deny_button)
        self.detailed_button = self._frame_button("DETAILED", "#a86b1f", "Show the full staged list")
        self.detailed_button.setCheckable(True)
        self.detailed_button.toggled.connect(self._toggle_detailed)
        row.addWidget(self.detailed_button)
        column.addLayout(row)
        self.staged_table = QTableWidget(0, 3)
        self.staged_table.horizontalHeader().setVisible(False)
        self.staged_table.verticalHeader().setVisible(False)
        self.staged_table.verticalHeader().setDefaultSectionSize(16)
        self.staged_table.horizontalHeader().setStretchLastSection(True)
        self.staged_table.setEditTriggers(QTableWidget.EditTrigger.NoEditTriggers)
        self.staged_table.setShowGrid(False)
        self.staged_table.setWordWrap(False)
        self.staged_table.setStyleSheet("font-size: 10px; border: none;")
        self.staged_table.setFixedHeight(3 * 16 + 4)
        column.addWidget(self.staged_table)
        return footer

    def _toggle_detailed(self, on: bool) -> None:
        self.staged_table.setFixedHeight(16 * 14 + 4 if on else 3 * 16 + 4)

    def _refresh_staged(self) -> None:
        """Keep the staged list and DENY in step with the current tool."""
        if not hasattr(self, "tools"):
            return
        try:
            rows = self.tools[self.current_tool_index].staged_rows()
        except Exception:  # noqa: BLE001 - a listing must never break the window
            rows = []
        key = tuple(rows)
        if key == getattr(self, "_staged_key", None):
            return
        self._staged_key = key
        self._restate_last_op()
        self.staged_table.setRowCount(len(rows))
        for r, cells in enumerate(rows):
            for c, text in enumerate(cells[:3]):
                self.staged_table.setItem(r, c, QTableWidgetItem(str(text)))
        self.staged_table.resizeColumnToContents(0)
        pending = bool(rows)
        color = "#c0392b" if pending else "#5a5a60"
        self.deny_button.setStyleSheet(self.deny_button.styleSheet().split("QPushButton:hover")[0]
                                       .replace("#c0392b", color).replace("#5a5a60", color)
                                       + f"QPushButton:hover {{background-color: {QColor(color).lighter(125).name()};}}")

    def confirm_staged(self) -> None:
        """CONFIRM: apply what the current tool has staged, which finishes the
        tool, and move on to the next tool (skipping automatic steps)."""
        page = self.actions_stack.currentWidget()
        buttons = [b for b in page.findChildren(QPushButton) if b.property("wn3d_apply") and b.isEnabled()]
        tool = self.tools[self.current_tool_index]
        try:
            if buttons:
                buttons[0].click()
            elif hasattr(tool, "apply"):
                tool.apply()
        except Exception as exc:  # noqa: BLE001 - a failed apply keeps you in the tool
            import traceback

            log_error(f"confirm {tool.id}", traceback.format_exc())
            self.show_status(f"{tool.title}: could not apply ({exc})")
            return
        if self.document is not None:
            self._next_tool()

    def apply_staged(self) -> None:
        """Enter: a field being typed in gets its own Enter first (e.g. a pin
        name is set); otherwise the current tool applies what it has staged
        and stays put (CONFIRM is what finishes a tool)."""
        from PySide6.QtWidgets import QAbstractSpinBox, QLineEdit

        focus = QApplication.focusWidget()
        if isinstance(focus, QLineEdit):
            focus.returnPressed.emit()
            return
        if isinstance(focus, QAbstractSpinBox):
            focus.interpretText()
            focus.editingFinished.emit()
            return
        if self.document is None:
            return
        tool = self.tools[self.current_tool_index]
        try:
            tool.apply_staged()
        except Exception as exc:  # noqa: BLE001
            import traceback

            log_error(f"apply staged {tool.id}", traceback.format_exc())
            self.show_status(f"{tool.title}: could not apply ({exc})")

    def deny_staged(self) -> None:
        tool = self.tools[self.current_tool_index]
        tool.deny()
        self.show_status(f"{tool.title}: staged changes dropped")

    def run_tool_auto(self) -> None:
        tool = self.tools[self.current_tool_index]
        if self.document is None:
            self.show_status("Open a model first.")
            return
        if not tool.run_step_auto():
            self.show_status(f"{tool.title}: no automatic action for this step yet")

    def show_tool_help(self) -> None:
        """HELP: the tool's how-to video (assets/help/<tool id>.mp4) in the
        middle of the screen, looping; a tool without one yet shows a still."""
        tool = self.tools[self.current_tool_index]
        dialog = QDialog(self)
        dialog.setWindowTitle(f"HELP: {tool.title}")
        dialog.resize(int(self.width() * 0.7), int(self.height() * 0.75))
        layout = QVBoxLayout(dialog)
        video = _help_video(tool.id)
        if video is not None:
            player = _help_player(dialog, layout, video)
            dialog.finished.connect(player.stop)
            caption = f"{tool.title}: {SHORT_HELP.get(tool.id, '')}"
        else:
            layout.addWidget(_help_still(dialog, HELP_DIR / f"{tool.id}.png"), 1)
            caption = f"{tool.title}: {SHORT_HELP.get(tool.id, '')}   (video coming: placeholder still)"
        note = QLabel(caption)
        note.setWordWrap(True)
        note.setStyleSheet(f"color: {TEXT_MUTED};")
        layout.addWidget(note)
        dialog.exec()

    # --------------------------------------------------------------- settings

    def open_settings(self) -> None:
        from .settings_window import SettingsDialog

        QApplication.setOverrideCursor(Qt.CursorShape.WaitCursor)
        try:
            dialog = SettingsDialog(self)
        except Exception as exc:  # noqa: BLE001
            import traceback

            log_error("open settings", traceback.format_exc())
            QMessageBox.warning(self, "Settings", f"Settings could not open: {exc}\n"
                                f"(details in {ERROR_LOG.name})")
            return
        finally:
            QApplication.restoreOverrideCursor()
        dialog.exec()

    def apply_settings(self, data: dict, first: bool = False) -> None:
        """Steps, corpus mode and keybinds from the settings file."""
        self.settings = data
        corpus = bool(data.get("corpus_mode", True))
        self.skip_button.setVisible(corpus)
        for combo in self._status_combos.values():
            combo.setVisible(corpus)
        self._bind_keys(data)
        if not first:
            self.set_auto_steps(set(data.get("auto_steps") or []))
            self._apply_tool(self.current_tool_index, animate=False)

    def _bind_keys(self, data: dict) -> None:
        """(Re)create the window shortcuts from the settings' keybinds."""
        for shortcut in self._shortcuts:
            shortcut.setEnabled(False)
            shortcut.deleteLater()
        self._shortcuts = []
        handlers = {
            "next_tool": self.cycle_tool, "prev_tool": self.cycle_tool_back,
            "next_tool_arrow": self.cycle_tool, "prev_tool_arrow": self.cycle_tool_back,
            "confirm": self.confirm_staged, "deny": self.deny_staged, "apply_staged": self.apply_staged,
            "undo": self.undo, "redo": self.redo,
            "write_new": self.write_ap242, "write_replace": self.write_ap242_replace,
            "open": self.open_step_dialog, "next_model": self.open_next_unconditioned,
            "auto": self.run_tool_auto,
        }
        keys = dict(data.get("keybinds") or {})
        if keys.get("confirm") in ("Return", "Enter"):
            keys["confirm"] = ""  # older settings: Enter was CONFIRM; it now applies staged changes
        used: dict[str, str] = {}
        for action, handler in handlers.items():
            sequence = keys.get(action) or settings_store.DEFAULT_KEYBINDS[action][1]
            if not sequence:
                continue
            if sequence in used:  # two shortcuts on one key fire neither and swallow it
                log_error("keybinds", f"{sequence} is bound to both {used[sequence]} and {action}; "
                                      f"kept {used[sequence]}")
                continue
            used[sequence] = action
            shortcut = QShortcut(QKeySequence(sequence), self, activated=handler)
            # window shortcuts only: in Settings or a file dialog, Tab / Enter /
            # arrows keep their usual meaning (and a keybind field can record them)
            shortcut.setContext(Qt.ShortcutContext.WindowShortcut)
            self._shortcuts.append(shortcut)
            label = self._options_actions.get(action)
            if label is not None:
                label.setShortcut(QKeySequence(sequence))
                label.setShortcutContext(Qt.ShortcutContext.WidgetShortcut)  # the QShortcut fires it

    def _refresh_options(self) -> None:
        self._options_actions["undo"].setEnabled(self.history.can_undo())
        self._options_actions["redo"].setEnabled(self.history.can_redo())
        loaded = self.document is not None
        for key in ("write_new", "write_replace", "write_same", "next_model"):
            self._options_actions[key].setEnabled(loaded)

    # ------------------------------------------------------------ undo / redo

    def _on_journal(self, journal) -> None:
        if journal.operations:
            self._show_last_op(journal.operations[-1])
        self._autosave()
        tool = self.tools[self.current_tool_index]
        self.history.capture(self, tool.title)

    def undo(self) -> None:
        label = self.history.undo(self) if self.document is not None else None
        self._after_history(label, "Undo")

    def redo(self) -> None:
        label = self.history.redo(self) if self.document is not None else None
        self._after_history(label, "Redo")

    def _after_history(self, label: str | None, verb: str) -> None:
        if label is None:
            self.show_status(f"Nothing to {verb.lower()}")
            return
        self._doc_revision += 1
        self.scene.rebuild(self.document)
        self._refresh_info()
        for tool in self.tools:
            tool.on_document_changed()
        self.tools[self.current_tool_index].enter()
        self._autosave()
        self.show_status(f"{verb}: {label}")

    # ----------------------------------------------------------------- camera

    def _view_from(self, direction, up) -> None:
        bounds = self.document.bounds() if self.document else (-1, 1, -1, 1, -1, 1)
        size = self.plotter.interactor.size()
        aspect = max(float(size.width()) / max(float(size.height()), 1.0), 0.01)
        from .viewcam import apply_camera_to_plotter

        apply_camera_to_plotter(self.plotter, bounds, np.asarray(direction, dtype=np.float64),
                                np.asarray(up, dtype=np.float64), aspect)

    # ------------------------------------------------------ write same type

    def write_same_type(self) -> None:
        """Write the conditioned model back in the format it came in (a
        .kicad_mod / .PcbLib gets its embedded model replaced; a STEP stays a
        plain STEP). The file carries no WN3D metadata."""
        if self.document is None:
            self.show_status("Nothing to write: open a model first.")
            return
        source = self._source_path()
        try:
            step_bytes = document_step_bytes(self.document)
            if self.vendor is not None:
                tmp = Path(self._vendor_workdir.name) / "plain_conditioned.step"
                tmp.write_bytes(step_bytes)
                out = bake_conditioned(self.vendor, tmp)
            else:
                out = source.with_name(f"{source.stem}_conditioned{source.suffix}")
                out.write_bytes(step_bytes)
            self.show_status(f"Wrote {out.name} (same type, no metadata)")
        except Exception as exc:  # noqa: BLE001
            QMessageBox.critical(self, "Write to same type failed", str(exc))

    def _set_step_mark(self, step: str, index: int) -> None:
        if self.document is None:
            return
        marks = getattr(self.document, "step_status", None) or {}
        value = STEP_MARKS[index][1]
        if value is None:
            marks.pop(step, None)
        else:
            marks[step] = value
        self.document.step_status = marks
        self._update_skip_button()
        self._autosave()

    def _skip_current_step(self) -> None:
        """Mark the current step 'skipped by choice' and move on, or undo it."""
        if self.document is None:
            self.show_status("Open a model first.")
            return
        tool = self.tools[self.current_tool_index]
        marks = getattr(self.document, "step_status", None) or {}
        values = [value for _label, value in STEP_MARKS]
        combo = self._status_combos.get(tool.id)
        if marks.get(tool.id) == "omitted_by_choice":
            if combo is not None:
                combo.setCurrentIndex(values.index(None))
            self._update_skip_button()
            self.show_status(f"{tool.title}: skip undone")
            return
        if tool.id in self.journal.tools_used():
            self.show_status(f"{tool.title} already has applied work; it stays 'applied'")
        if combo is not None:
            combo.setCurrentIndex(values.index("omitted_by_choice"))
        self.show_status(f"{tool.title}: skipped by choice")
        self.cycle_tool()

    def _update_skip_button(self) -> None:
        if not hasattr(self, "skip_button"):
            return
        tool = self.tools[self.current_tool_index]
        marks = (getattr(self.document, "step_status", None) or {}) if self.document else {}
        skipped = marks.get(tool.id) == "omitted_by_choice"
        self.skip_button.setText("Undo skip" if skipped else "Skip step")
        self.skip_button.setProperty("skipped", skipped)
        self.skip_button.style().unpolish(self.skip_button)
        self.skip_button.style().polish(self.skip_button)

    def _unmarked_steps(self) -> list[str]:
        from .metadata import step_status

        status = step_status(self.document, self.journal)
        return [tool.title for tool in self.tools if status.get(tool.id) == "not_marked"]

    def _sync_step_marks(self) -> None:
        marks = (getattr(self.document, "step_status", None) or {}) if self.document else {}
        values = [value for _label, value in STEP_MARKS]
        for step, combo in self._status_combos.items():
            combo.blockSignals(True)
            mark = marks.get(step)
            combo.setCurrentIndex(values.index(mark) if mark in values else 0)
            combo.blockSignals(False)
        self._update_skip_button()

    def set_tool(self, tool_id: str) -> None:
        for index, tool in enumerate(self.tools):
            if tool.id == tool_id:
                self._switch_tool(index)
                return

    def _step_from(self, index: int, direction: int) -> int:
        """`index`, or the next tool in `direction` that isn't automatic."""
        count = len(self.tools)
        for offset in range(count):
            candidate = (index + direction * offset) % count
            if self.tools[candidate].id not in self.auto_steps:
                return candidate
        return index

    def cycle_tool(self) -> None:
        """Tab / Right: on to the next tool; unsaved work in this one (a
        filled-in but unsaved Identity, staged pins ...) is applied first, as
        CONFIRM does."""
        tool = self.tools[self.current_tool_index]
        if self.document is not None and tool.has_unsaved():
            self.confirm_staged()
            return
        self._next_tool()

    def _next_tool(self) -> None:
        self._switch_tool(self._step_from(self.current_tool_index + 1, 1))

    def cycle_tool_back(self) -> None:
        self._switch_tool(self._step_from(self.current_tool_index - 1, -1))

    def set_auto_steps(self, steps) -> None:
        self.auto_steps = set(steps)
        self.settings["auto_steps"] = sorted(self.auto_steps)
        settings_store.save(self.settings)
        self.mode_rect.set_automatic(self.auto_steps)
        if self.document is not None:
            self.document.auto_steps = set(self.auto_steps)
        if self.tools[self.current_tool_index].id in self.auto_steps:
            self._switch_tool(self._step_from(self.current_tool_index, 1))
        else:
            self._apply_tool(self.current_tool_index, animate=False)
        self.show_status("Automatic steps: " + (", ".join(
            t.title for t in self.tools if t.id in self.auto_steps) or "none"))

    def _switch_tool(self, index: int) -> None:
        if self._switching:
            return  # a heavy enter() is mid-flight; ignore re-entrant Tab
        if index == self.current_tool_index:
            self._apply_tool(index)
            return
        self._report_leaving(self.tools[self.current_tool_index])
        self.tools[self.current_tool_index].exit()
        self.scene.clear_tool_overlays()  # nothing leaks into the next tool
        self.current_tool_index = index
        self._apply_tool(index)

    def _model_is_heavy(self) -> bool:
        """A switch only lags on big models (face-adjacency, HLR, billboards).
        Use mesh size as a cheap proxy so small parts switch flicker-free."""
        if self.document is None:
            return False
        points = sum(
            len(b.mesh.points)
            for b in self.document.bodies
            if b.mesh is not None
        )
        return points > 40000

    _APPLIED_COLOURS = {"ok": "#1f9d3a", "none": "#c89d00", "pending": "#c0392b"}

    def _show_applied(self, kind: str, text: str) -> None:
        colour = self._APPLIED_COLOURS[kind]
        self.applied_label.setText(text)
        self.applied_label.setStyleSheet(f"color: {colour}; font-weight: 700; font-size: 10px;")

    def _show_last_op(self, op) -> None:
        """Green when the change is in; amber when it only staged work that
        still needs CONFIRM (e.g. Generate land pattern)."""
        tool = self.tools[self.current_tool_index]
        title = self._tool_title(op.tool)
        if tool.id == op.tool and tool.has_unsaved():
            self._show_applied("none", f"◐ {title}: {_op_summary(op)} (staged, CONFIRM to apply)")
        else:
            self._show_applied("ok", f"✓ {title}: {_op_summary(op)}")

    def _restate_last_op(self) -> None:
        """The staged list changed: re-colour the last op of this tool (a
        tool may stage its work just after journalling it)."""
        journal = getattr(self, "journal", None)
        if self.document is None or journal is None or len(journal.operations) <= self._ops_at_entry:
            return
        op = journal.operations[-1]
        if op.tool == self.tools[self.current_tool_index].id:
            self._show_last_op(op)

    def _tool_title(self, tool_id: str) -> str:
        return next((t.title for t in self.tools if t.id == tool_id), tool_id)

    def _report_leaving(self, tool) -> None:
        """On leaving a tool: say whether it applied anything this visit."""
        if self.document is None:
            return
        applied = len(self.journal.operations) - self._ops_at_entry
        if tool.has_unsaved():
            self._show_applied("pending", f"⚠ {tool.title}: staged but NOT applied")
        elif applied <= 0:
            self._show_applied("none", f"— {tool.title}: nothing applied")
        else:
            self._show_applied("ok", f"✓ {tool.title}: {applied} change(s) applied")

    def _apply_tool(self, index: int, *, animate: bool = True) -> None:
        self._ops_at_entry = len(self.journal.operations) if getattr(self, "journal", None) else 0
        tool = self.tools[index]
        next_tool = self.tools[self._step_from(index + 1, 1)]
        prev_tool = self.tools[self._step_from(index - 1, -1)]
        self._update_skip_button()
        self.actions_stack.setCurrentIndex(index)
        self.mode_rect.set_current(
            tool.title, tool.accent, next_tool.title, next_tool.accent,
            prev_tool.title, prev_tool.accent, animate=animate,
        )
        from .settings_window import apply_tool_defaults

        apply_tool_defaults(tool, self.settings)
        model = settings_store.step_model(self.settings, tool.id)
        self.step_model_label.setText(model or "No Machine Learning Model")
        self.step_model_label.setToolTip(model or "")
        self.auto_frame_button.setEnabled(tool.auto_method is not None)
        heavy = self._model_is_heavy()
        self._switching = True
        QApplication.setOverrideCursor(Qt.CursorShape.WaitCursor)
        try:
            if heavy:
                self.progress(f"Entering {tool.title}…", 0, 0)
            tool.enter()
        except OperationCancelled:
            self.show_status(f"{tool.title}: cancelled")
        finally:
            if heavy:
                self.progress_done()
            QApplication.restoreOverrideCursor()
            self._switching = False

    def _route_pick(self, pick) -> None:
        self.tools[self.current_tool_index].on_pick(pick)

    def _route_right_miss(self) -> None:
        """Right-click on the background clears the current selection."""
        tool = self.tools[self.current_tool_index]
        if hasattr(tool, "on_right_miss"):
            tool.on_right_miss()
        else:
            self.scene.clear_highlight()

    def _route_miss(self) -> None:
        tool = self.tools[self.current_tool_index]
        if hasattr(tool, "on_miss"):
            tool.on_miss()

    # ------------------------------------------------------------------- load

    def open_step_dialog(self) -> None:
        file_name, _filter = QFileDialog.getOpenFileName(
            self,
            "Open Model",
            self._open_dir(),
            "3D model files (*.step *.stp *.kicad_mod *.PcbLib);;"
            "STEP files (*.step *.stp);;"
            "KiCad footprints (*.kicad_mod);;"
            "Altium PCB libraries (*.PcbLib);;"
            "All files (*.*)",
        )
        if file_name:
            path = Path(file_name)
            QSettings("Wavenumber", "StepEditor").setValue("open_dir", str(path.parent))
            self.load_any(path)

    def open_next_unconditioned(self) -> None:
        """Move on to the next model in the current folder that hasn't been
        exported yet. Unexported work on this one stays in its autosave."""
        source = self._source_path()
        if source is None:
            self.show_status("Open a model from the folder first; Next walks that folder.")
            return
        if checkpoint_path(source).is_file() and not self._ask(
                "WN3D_NEXT_UNSAVED", "Not written yet",
                f"{source.name} has changes that were not written (Write AP242).\n\n"
                "They stay in its autosave and come back when you reopen it.\n\nMove on anyway?"):
            return
        nxt, left = next_unconditioned(source)
        if nxt is None:
            self.show_status(f"Every model in {source.parent.name} has an export: folder done.")
            return
        self.load_any(nxt)
        self.show_status(f"{self.statusBar().currentMessage()} | {left - 1} more to condition in {source.parent.name}")

    def load_any(self, path: Path) -> None:
        """Route by file type: vendor containers extract first, STEP loads
        directly. Loading a plain STEP drops any active container."""
        if is_vendor_file(path):
            self._load_vendor(path)
        else:
            self.load_step(path)

    # ------------------------------------------------------------- drag&drop

    def _dropped_model_path(self, event) -> Path | None:
        mime = event.mimeData()
        if not mime.hasUrls():
            return None
        for url in mime.urls():
            path = Path(url.toLocalFile())
            if path.is_file() and (
                path.suffix.lower() in {".step", ".stp"} or is_vendor_file(path)
            ):
                return path
        return None

    def dragEnterEvent(self, event: QDragEnterEvent) -> None:
        if self._dropped_model_path(event) is not None:
            event.acceptProposedAction()

    def dropEvent(self, event: QDropEvent) -> None:
        path = self._dropped_model_path(event)
        if path is None:
            return
        event.acceptProposedAction()
        self.load_any(path)

    def _load_vendor(self, path: Path) -> None:
        import tempfile

        self.show_status(f"Opening {path.name} ...")
        workdir = tempfile.TemporaryDirectory(prefix="step_editor_vendor_")
        container = None
        try:
            QApplication.setOverrideCursor(Qt.CursorShape.WaitCursor)
            try:
                container = open_vendor_file(
                    Path(path), Path(workdir.name),
                    choose_footprint=self._choose_footprint,
                )
            finally:
                QApplication.restoreOverrideCursor()
        except Exception as exc:
            workdir.cleanup()
            self.show_status(f"Open failed: {exc}")
            QMessageBox.critical(self, "Open failed", str(exc))
            return
        if container is None:  # user cancelled the footprint chooser
            workdir.cleanup()
            self.show_status("Open cancelled.")
            return
        self.load_step(container.step_path, vendor=container, vendor_workdir=workdir)
        if self.document is not None:
            self.model_label.setText(f"{path.name} :: {container.part_name}")
            self.model_label.setToolTip(
                f"{path}\nembedded model: {container.model_name}\n"
                f"Write AP242 also bakes {container.conditioned_container_path().name}"
            )
            self.show_status(
                f"Opened {container.part_name!r} from {path.name} "
                f"({container.model_name}) — export bakes a conditioned "
                f"{container.conditioned_container_path().suffix} next to it"
            )

    def _choose_footprint(self, rows: list[dict]) -> str | None:
        """Library-split picker for multi-footprint PcbLibs: choose the one
        part to split out and condition."""
        QApplication.restoreOverrideCursor()  # dialog needs a normal cursor
        try:
            dialog = QDialog(self)
            dialog.setWindowTitle("Choose footprint to condition")
            layout = QVBoxLayout(dialog)
            label = QLabel(
                f"This library holds {len(rows)} footprints with embedded STEP "
                "models.\nThe chosen part is split into its own single-part "
                "library; only that part is conditioned."
            )
            label.setWordWrap(True)
            layout.addWidget(label)
            listing = QListWidget()
            first = -1
            for row in rows:
                model = next((m for m in row["models"] if m["kind"] == "step"), None)
                if model is None:
                    kinds = ", ".join(sorted({m["kind"] for m in row["models"]})) or "no model"
                    item = QListWidgetItem(f"{row['footprint']}  —  unsupported: {kinds}")
                    item.setFlags(Qt.ItemFlag.NoItemFlags)  # greyed, not selectable
                else:
                    item = QListWidgetItem(
                        f"{row['footprint']}  —  {model['name']} "
                        f"({model['bytes'] / 1e6:.1f} MB)"
                    )
                    first = listing.count() if first < 0 else first
                item.setData(Qt.ItemDataRole.UserRole, row["footprint"])
                listing.addItem(item)
            listing.setCurrentRow(max(first, 0))
            listing.itemDoubleClicked.connect(lambda _item: dialog.accept())
            layout.addWidget(listing)
            buttons = QDialogButtonBox(
                QDialogButtonBox.StandardButton.Ok
                | QDialogButtonBox.StandardButton.Cancel
            )
            buttons.accepted.connect(dialog.accept)
            buttons.rejected.connect(dialog.reject)
            layout.addWidget(buttons)
            dialog.resize(560, 420)
            if dialog.exec() != QDialog.DialogCode.Accepted:
                return None
            current = listing.currentItem()
            return current.data(Qt.ItemDataRole.UserRole) if current else None
        finally:
            QApplication.setOverrideCursor(Qt.CursorShape.WaitCursor)

    def _open_dir(self) -> str:
        """Last folder the user opened from; defaults to TEST_STEP_FILES."""
        saved = QSettings("Wavenumber", "StepEditor").value("open_dir")
        for candidate in (saved, DEFAULT_OPEN_DIR, HERE):
            if candidate and Path(candidate).is_dir():
                return str(candidate)
        return str(HERE)

    def load_step(self, path: Path, vendor: VendorContainer | None = None,
                  vendor_workdir=None) -> None:
        if self._vendor_workdir is not None and self._vendor_workdir is not vendor_workdir:
            try:
                self._vendor_workdir.cleanup()
            except Exception:
                pass
        self.vendor = vendor
        self._vendor_workdir = vendor_workdir
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
            self._restored_ops = 0
            self.model_units = step_length_units(path)
            self.document.auto_steps = set(self.auto_steps)
            resumed = self._resume_conditioning(path)
            self.journal.listener = self._on_journal
            self.history.reset(self, "opened")
            self.scene.rebuild(self.document)
            self.snap_camera("iso")
            self._refresh_info()
            self._sync_step_marks()
            for tool in self.tools:
                tool.on_document_changed()
            self._ops_at_entry = len(self.journal.operations)  # restored ops are not this visit's
            self.applied_label.setText("")
            self.show_status(
                f"Loaded {path.name}: {len(self.document.bodies)} bodies, "
                f"{self.document.info().face_count} faces" + (f" | {resumed}" if resumed else "")
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

    # ---------------------------------------------------- resume / recovery

    def _source_path(self) -> Path | None:
        """The file the user actually opened (a container, or the STEP)."""
        if self.vendor is not None:
            return Path(self.vendor.source)
        return self.document.path if self.document is not None else None

    def _ask(self, env: str, title: str, text: str) -> bool:
        answer = _ask_env(env)
        if answer is not None:
            return answer
        return QMessageBox.question(
            self, title, text,
            QMessageBox.StandardButton.Yes | QMessageBox.StandardButton.No,
            QMessageBox.StandardButton.Yes) == QMessageBox.StandardButton.Yes

    def _resume_conditioning(self, step_path: Path) -> str:
        """Pick up where a previous session left off: restore the embedded
        conditioning of an already-exported file, then offer to recover an
        autosaved journal from a session that never exported."""
        notes = []
        try:
            meta = extract_metadata(step_path)
        except Exception:
            meta = None
        if meta and str(meta.get("schema", "")).startswith("wn3d.step_conditioning") and self._ask(
                "WN3D_AUTO_RESTORE", "Already conditioned",
                f"{step_path.name} was already conditioned "
                f"({sum(len(n.get('pins', [])) for n in meta.get('nets', []))} pins, "
                f"{len(meta.get('journal', []))} recorded operations).\n\n"
                "Restore its pins, names, hitboxes and journal so you can continue?"):
            state = restore_state(self.document, meta)
            self.pins.pins = state["pins"].pins
            self.journal.operations = state["journal"].operations
            self._restored_ops = len(self.journal.operations)
            note = f"restored {len(self.pins.pins)} pins + {self._restored_ops} ops"
            if state["lost"]:
                note += f" ({state['lost']} pin(s) could not be matched)"
            notes.append(note)

        source = self._source_path()
        checkpoint = read_checkpoint(source) if source is not None else None
        if checkpoint:
            ops = checkpoint["journal"]
            base = int(checkpoint.get("restored_ops", 0)) if self._restored_ops else 0
            new_ops = len(ops) - base
            if new_ops > 0 and self._ask(
                    "WN3D_AUTO_RECOVER", "Recover previous session",
                    f"An autosaved session for {source.name} has {new_ops} operation(s) "
                    "that were never exported (the editor closed or crashed).\n\n"
                    "Recover them now? (No discards the autosave.)"):
                from .replay import replay

                saved = Journal()
                for item in ops:
                    saved.record(item["tool"], item.get("params", {}),
                                 item.get("inputs", {}), item.get("result", {}))
                try:
                    replay(self.document, saved, registry=self.pins, start=base)
                    self.journal.operations = saved.operations
                    if checkpoint.get("step_status"):
                        self.document.step_status = dict(checkpoint["step_status"])
                    notes.append(f"recovered {new_ops} autosaved op(s)")
                except Exception as exc:
                    QMessageBox.warning(self, "Recover failed",
                                        f"Replaying the autosave failed: {exc}\n"
                                        "The autosave is kept; nothing was changed.")
            elif new_ops > 0:
                clear_checkpoint(source)
        return "; ".join(notes)

    def _autosave(self) -> None:
        """Checkpoint the journal next to the source after every Apply, so a
        force-quit never loses a session (offered back on the next open)."""
        source = self._source_path()
        if source is None or self.document is None or os.environ.get("WN3D_NO_AUTOSAVE") == "1":
            return  # WN3D_NO_AUTOSAVE: scripted / test sessions must not overwrite a person's checkpoint
        write_checkpoint(source, self.journal, {
            "restored_ops": self._restored_ops,
            "step_status": getattr(self.document, "step_status", None) or {},
        })

    # ------------------------------------------------------------------ save

    def write_ap242(self) -> None:
        if self.document is None:
            self.show_status("Nothing to write — open a STEP file first.")
            return
        unmarked = self._unmarked_steps()
        if unmarked and not self._ask(
                "WN3D_EXPORT_UNMARKED", "Steps not done",
                "These steps were neither done nor marked:\n\n  " + "\n  ".join(unmarked)
                + "\n\nThey export as 'not marked' (not passed). Use Skip step, or the "
                "Status box (already satisfied), to record them.\n\nExport anyway?"):
            self.show_status("Export cancelled: mark or do the open steps first")
            return
        # Container opens load a temp extraction; route outputs next to the
        # container the user actually opened, named after the part.
        if self.vendor is not None:
            out_path = self.vendor.conditioned_step_path()
        else:
            out_path = conditioned_path(self.document.path)
        self.show_status(f"Writing {out_path.name} ...")
        QApplication.setOverrideCursor(Qt.CursorShape.WaitCursor)
        try:
            report = export_ap242(
                self.document, out_path, pins=self.pins, journal=self.journal,
                progress=self.progress,
                source_name=self.vendor.source.name if self.vendor else None,
            )
            status = f"{report.summary()} | embedded metadata verified"
            if not report.ok:
                QMessageBox.warning(self, "Export validation", report.summary())
            else:
                clear_checkpoint(self._source_path())  # exported: nothing to recover
                if self.vendor is not None:
                    self.progress("Baking conditioned model into "
                                  f"{self.vendor.conditioned_container_path().name}", 0, 0)
                    baked = bake_conditioned(self.vendor, out_path)
                    status += f" | baked {baked.name}"
            self.show_status(status)
        except Exception as exc:
            self.show_status(f"Export failed: {exc}")
            QMessageBox.critical(self, "Export failed", str(exc))
        finally:
            self.progress_done()
            QApplication.restoreOverrideCursor()

    def write_ap242_replace(self) -> None:
        """REPLACE mode: condition into a temp file next to the original, and
        only after the export validates swap it over the opened STEP. The
        previous file is kept as <name>.bak."""
        if self.document is None:
            self.show_status("Nothing to write — open a STEP file first.")
            return
        if self.vendor is not None:
            QMessageBox.information(
                self, "Replace original",
                "REPLACE works on plain STEP opens. Container opens "
                "(.kicad_mod/.PcbLib) still export a conditioned copy via "
                "WRITE NEW.")
            return
        target = self.document.path
        backup = target.with_name(target.name + ".bak")
        answer = QMessageBox.question(
            self, "Replace original",
            f"Replace {target.name} with the conditioned AP242?\n"
            f"The current file is kept as {backup.name}.",
            QMessageBox.StandardButton.Yes | QMessageBox.StandardButton.No,
            QMessageBox.StandardButton.No)
        if answer != QMessageBox.StandardButton.Yes:
            return
        tmp_path = target.with_name(f"{target.stem}_AP242_replace_tmp.step")
        self.show_status(f"Writing {target.name} in place ...")
        QApplication.setOverrideCursor(Qt.CursorShape.WaitCursor)
        try:
            report = export_ap242(
                self.document, tmp_path, pins=self.pins, journal=self.journal,
                progress=self.progress,
            )
            if not report.ok:
                tmp_path.unlink(missing_ok=True)
                self.show_status(f"{report.summary()} | {target.name} NOT replaced")
                QMessageBox.warning(
                    self, "Export validation",
                    f"{report.summary()}\n{target.name} was NOT replaced.")
                return
            shutil.copy2(target, backup)
            os.replace(tmp_path, target)
            clear_checkpoint(target)
            self.show_status(
                f"Replaced {target.name} (backup: {backup.name}) | "
                "embedded metadata verified")
        except Exception as exc:
            tmp_path.unlink(missing_ok=True)
            tmp_path.with_suffix(".metadata.json").unlink(missing_ok=True)
            self.show_status(f"Replace failed: {exc}")
            QMessageBox.critical(self, "Replace failed", str(exc))
        finally:
            self.progress_done()
            QApplication.restoreOverrideCursor()

    # ------------------------------------------------------------------ misc

    def _reframe_if_lost(self) -> None:
        """A transform (Z-Sit on a model with a big offset, Front) can move the
        bulk of the model off screen. If most of its bounding box is no longer
        in view, snap the camera back onto the model."""
        if self.document is None:
            return
        try:
            renderer = self.plotter.renderer
            b = self.document.bounds()
            corners = [(x, y, z) for x in b[0:2] for y in b[2:4] for z in b[4:6]]
            width, height = renderer.GetSize()
            if width <= 0 or height <= 0:
                return
            inside = 0
            centre = [(b[0] + b[1]) / 2, (b[2] + b[3]) / 2, (b[4] + b[5]) / 2]
            for point in corners + [centre]:
                renderer.SetWorldPoint(*point, 1.0)
                renderer.WorldToDisplay()
                x, y, _z = renderer.GetDisplayPoint()
                inside += 0 <= x <= width and 0 <= y <= height
            if inside < 5:  # centre plus at least half the corners must be visible
                self.snap_camera("iso")
                self.show_status("View re-framed: the model had moved off screen")
        except Exception:  # noqa: BLE001 - framing must never break an edit
            pass

    def snap_camera(self, view_id: str) -> None:
        bounds = self.document.bounds() if self.document else (-1, 1, -1, 1, -1, 1)
        size = self.plotter.interactor.size()
        aspect = max(float(size.width()) / max(float(size.height()), 1.0), 0.01)
        self.scene.snap_camera(view_id, bounds, aspect)

    def showEvent(self, event) -> None:
        super().showEvent(event)
        if not self._splitter_initialized:
            self._splitter_initialized = True
            # 3D view gets 2/3 of the width, the tool pane 1/3; a divider
            # the user dragged is remembered.
            width = max(self._splitter.width(), 1)
            saved = QSettings("Wavenumber", "StepEditor").value("panel_width_v2")
            panel = int(saved) if saved else max(width // 3, 330)
            self._splitter.setSizes([width - panel, panel])
            self._splitter.splitterMoved.connect(
                lambda _pos, _index: QSettings("Wavenumber", "StepEditor").setValue(
                    "panel_width_v2", self._splitter.sizes()[1]))

    def show_status(self, message: str) -> None:
        self.statusBar().showMessage(message)

    def _request_cancel(self) -> None:
        self._cancel_requested = True
        self.cancel_button.setEnabled(False)
        self.show_status("Cancelling at the next safe point ...")

    def progress(self, label: str, value: int, maximum: int) -> None:
        """Long main-thread operations report here; the green bar repaints
        between steps so the app never looks hung, and the Cancel button
        next to it raises OperationCancelled at the next report. maximum=0
        shows a busy indicator."""
        if self._cancel_requested:
            self._cancel_requested = False
            self.progress_done()
            raise OperationCancelled(label)
        if not self.cancel_button.isVisible():
            self.cancel_button.setEnabled(True)
            self.cancel_button.setVisible(True)
        self.progress_bar.setMaximum(maximum)
        self.progress_bar.setValue(value)
        self.progress_bar.setFormat(f"{label}  %p%" if maximum else label)
        self.show_status(label)
        QApplication.processEvents()

    def progress_done(self) -> None:
        # the bar stays as an empty rectangle between operations
        self.progress_bar.setMaximum(1)
        self.progress_bar.setValue(0)
        self.progress_bar.setFormat("")
        self.cancel_button.setVisible(False)
        self._cancel_requested = False
        QApplication.processEvents()

    def document_mutated(self) -> None:
        """A tool changed the B-rep: re-render and refresh dependent state,
        keeping the current camera."""
        if self.document is None:
            return
        self._doc_revision += 1
        self.scene.rebuild(self.document)
        self._refresh_info()
        self._reframe_if_lost()
        for index, tool in enumerate(self.tools):
            if index != self.current_tool_index:
                tool.on_document_changed()

    # ------------------------------------------------------- footprint cache

    def request_footprint(self, callback, force: bool = False) -> bool:
        """Deliver the top-view HLR projection of the current geometry to
        `callback`, from cache when fresh, else computed on a worker thread.
        On a large model the projection holds the interpreter for many seconds
        even off the main thread, so it only runs when asked (force=True,
        the canvas's click). Returns False when it was not started."""
        if self.document is None:
            return False
        if self._footprint_cache_rev == self._doc_revision and self._footprint_cache is not None:
            callback(self._footprint_cache)
            return True
        if self._model_is_heavy() and not force:
            return False
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
        return True

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
        # OpenCascade reads every STEP into mm; the file's own unit is noted
        units = "mm" if self.model_units in (None, "mm") else f"mm (file in {self.model_units})"
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
            ("output",
             self.vendor.conditioned_container_path().name
             if self.vendor else conditioned_path(info.path).name),
            ("pins", str(len(self.pins.pins))),
        ]
        lines = [pairs[0:4], pairs[4:7], pairs[7:10]]
        self.info_label.setText("<br>".join(
            " &nbsp;·&nbsp; ".join(f"<b>{key}</b> {value}" for key, value in line) for line in lines))
