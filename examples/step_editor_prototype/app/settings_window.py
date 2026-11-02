"""The Settings window: sections on the left, details on the right.

Sections: System information (default), Steps, Tool Parameter Defaults,
Models, Keybinds. Everything saves to editor_settings.json (settings_store)."""

from __future__ import annotations

import platform
import sys

from PySide6.QtCore import Qt
from PySide6.QtGui import QKeySequence
from PySide6.QtWidgets import (
    QAbstractSpinBox,
    QCheckBox,
    QComboBox,
    QDialog,
    QDialogButtonBox,
    QDoubleSpinBox,
    QFormLayout,
    QGroupBox,
    QHBoxLayout,
    QKeySequenceEdit,
    QLabel,
    QListWidget,
    QScrollArea,
    QSlider,
    QSpinBox,
    QStackedWidget,
    QTableWidget,
    QTableWidgetItem,
    QVBoxLayout,
    QWidget,
)

from . import settings_store
from .style import BORDER, TEXT_MUTED

PARAM_TYPES = (QDoubleSpinBox, QSpinBox, QComboBox, QCheckBox, QSlider)


def _version(distribution: str) -> str:
    """An installed package's version, read from its metadata (no import: a
    cold import of scikit-learn / SciPy can stall the window for seconds)."""
    from importlib.metadata import PackageNotFoundError, version

    try:
        return version(distribution)
    except PackageNotFoundError:
        return "not installed"


def system_information() -> list[tuple[str, str]]:
    """Versions of everything the editor runs on."""
    import geometer
    from PySide6.QtCore import qVersion

    try:
        geometer_version = geometer.version()
        rows = [("Geometer", geometer_version.string), ("Geometer ABI", str(geometer_version.abi))]
    except Exception:  # noqa: BLE001
        rows = [("Geometer", "not available")]
    rows += [
        ("OpenCascade (OCP)", _version("cadquery-ocp")),
        ("VTK", _version("vtk")),
        ("PyVista", _version("pyvista")),
        ("PySide6 / Qt", f"{_version('PySide6')} / {qVersion()}"),
        ("NumPy", _version("numpy")),
        ("SciPy", _version("scipy")),
        ("scikit-learn", _version("scikit-learn")),
        ("Python", sys.version.split()[0]),
        ("Operating system", f"{platform.system()} {platform.release()} ({platform.machine()})"),
        ("Settings file", str(settings_store.SETTINGS_PATH)),
    ]
    return rows


def tool_parameters(tool) -> dict[str, QWidget]:
    """A tool's adjustable parameters: its spin boxes, combos, check boxes and
    sliders, by attribute name."""
    tool.actions_widget()  # make sure the widgets exist
    return {name: widget for name, widget in vars(tool).items()
            if isinstance(widget, PARAM_TYPES) and not name.startswith("_")}


def widget_value(widget):
    if isinstance(widget, (QDoubleSpinBox, QSpinBox, QSlider)):
        return widget.value()
    if isinstance(widget, QComboBox):
        return widget.currentText()
    if isinstance(widget, QCheckBox):
        return widget.isChecked()
    return None


def set_widget_value(widget, value) -> None:
    try:
        if isinstance(widget, (QDoubleSpinBox, QSpinBox, QSlider)):
            widget.setValue(value)
        elif isinstance(widget, QComboBox):
            index = widget.findText(str(value))
            if index >= 0:
                widget.setCurrentIndex(index)
        elif isinstance(widget, QCheckBox):
            widget.setChecked(bool(value))
    except (TypeError, ValueError):
        pass


def apply_tool_defaults(tool, data: dict) -> None:
    """Set a tool's parameters to the saved defaults (on entering the tool)."""
    saved = (data.get("tool_defaults") or {}).get(tool.id) or {}
    for name, widget in tool_parameters(tool).items():
        if name in saved:
            set_widget_value(widget, saved[name])


def _clone_editor(widget):
    """An editor of the same kind and range as a tool's own widget."""
    if isinstance(widget, QDoubleSpinBox):
        editor = QDoubleSpinBox()
        editor.setRange(widget.minimum(), widget.maximum())
        editor.setDecimals(widget.decimals())
        editor.setSingleStep(widget.singleStep())
    elif isinstance(widget, (QSpinBox, QSlider)):
        editor = QSpinBox()
        editor.setRange(widget.minimum(), widget.maximum())
    elif isinstance(widget, QComboBox):
        editor = QComboBox()
        editor.addItems([widget.itemText(i) for i in range(widget.count())])
    else:
        editor = QCheckBox()
    if isinstance(editor, QAbstractSpinBox):
        editor.setButtonSymbols(QAbstractSpinBox.ButtonSymbols.NoButtons)
    return editor


PARAM_LABELS = {
    "count_combo": "Plane picks", "offset_spin": "Plane offset (mm)", "origin_combo": "XY origin",
    "point_combo": "Origin = pick no.", "custom_x": "Custom origin X (mm)", "custom_y": "Custom origin Y (mm)",
    "snap_check": "Snap picks to vertices", "snap_slider": "Snap distance (% of model)",
    "flip_check": "Flip Z", "exclude_check": "Exclude largest body (package)",
    "flow_combo": "Unibody flow", "angle_spin": "Smooth angle (deg)", "order_combo": "Numbering order",
    "autocap_button": "Auto-capitalize names", "rotation_spin": "Rotation (deg)",
    "factor_spin": "Body-face cutoff", "bundle_combo": "Paint bundle", "cut_pin_combo": "Cut pin",
    "cyl_radius_spin": "Cylinder radius (mm)", "variant_combo": "Hitbox variant",
    "margin_spin": "Hitbox margin (mm)", "size_spin": "Size / depth (mm)", "category": "Category",
    "target_combo": "Paint target", "brush_check": "Paintbrush", "function_combo": "Pin type",
}


def _pretty(name: str) -> str:
    if name in PARAM_LABELS:
        return PARAM_LABELS[name]
    words = name.replace("_spin", "").replace("_combo", "").replace("_check", "").replace("_slider", "")
    return words.replace("_", " ").strip().capitalize()


class SettingsDialog(QDialog):
    SECTIONS = ["System information", "Steps", "Tool Parameter Defaults", "Models", "Keybinds"]

    def __init__(self, window) -> None:
        super().__init__(window)
        self.window_ = window
        self.data = settings_store.load()
        self.setWindowTitle("Settings")
        self.resize(1180, 600)
        layout = QHBoxLayout(self)
        self.sections = QListWidget()
        self.sections.addItems(self.SECTIONS)
        self.sections.setFixedWidth(210)
        self.pages = QStackedWidget()
        builders = (self._system_page, self._steps_page, self._defaults_page, self._models_page,
                    self._keys_page)
        for build in builders:
            scroll = QScrollArea()
            scroll.setWidgetResizable(True)
            scroll.setWidget(self._guarded(build))  # one failing page can't take the window down
            self.pages.addWidget(scroll)
        self.sections.currentRowChanged.connect(self.pages.setCurrentIndex)
        self.sections.setCurrentRow(0)
        right = QVBoxLayout()
        right.addWidget(self.pages, 1)
        buttons = QDialogButtonBox(QDialogButtonBox.StandardButton.Save | QDialogButtonBox.StandardButton.Cancel)
        self._buttons = buttons
        buttons.accepted.connect(self._save)
        buttons.rejected.connect(self.reject)
        right.addWidget(buttons)
        layout.addWidget(self.sections)
        layout.addLayout(right, 1)

    def showEvent(self, event) -> None:  # noqa: N802 (Qt override)
        """Enter in a field edits that field; it must not press Save and
        close the window (QDialog re-picks a default button when shown)."""
        super().showEvent(event)
        for button in self._buttons.buttons():
            button.setAutoDefault(False)
            button.setDefault(False)

    @staticmethod
    def _guarded(build) -> QWidget:
        try:
            return build()
        except Exception as exc:  # noqa: BLE001 - show the problem, keep the window
            import traceback

            from .app_window import log_error

            log_error("settings page", traceback.format_exc())
            label = QLabel(f"This page could not be built: {exc}\n(details in editor_errors.log)")
            label.setWordWrap(True)
            return label

    # ---------------------------------------------------------------- pages

    def _header(self, layout, title: str, note: str) -> None:
        label = QLabel(f"<b>{title}</b>")
        label.setStyleSheet("font-size: 14px;")
        layout.addWidget(label)
        hint = QLabel(note)
        hint.setWordWrap(True)
        hint.setStyleSheet(f"color: {TEXT_MUTED}; padding-bottom: 6px; border-bottom: 1px solid {BORDER};")
        layout.addWidget(hint)

    def _system_page(self) -> QWidget:
        page = QWidget()
        layout = QVBoxLayout(page)
        self._header(layout, "System information", "What this editor runs on.")
        form = QFormLayout()
        for name, value in system_information():
            label = QLabel(value)
            label.setTextInteractionFlags(Qt.TextInteractionFlag.TextSelectableByMouse)
            form.addRow(f"{name}:", label)
        layout.addLayout(form)
        layout.addStretch(1)
        return page

    def _steps_page(self) -> QWidget:
        page = QWidget()
        layout = QVBoxLayout(page)
        self._header(layout, "Steps", "Ticked steps are filled by the automatic pass: they are skipped while "
                                      "conditioning by hand and export as deferred.")
        self.step_boxes = {}
        for tool in self.window_.tools:
            box = QCheckBox(f"{tool.title}: filled automatically")
            box.setChecked(tool.id in self.window_.auto_steps)
            layout.addWidget(box)
            self.step_boxes[tool.id] = box
        self.corpus_box = QCheckBox("Corpus mode: show step status marks and the Skip button")
        self.corpus_box.setChecked(bool(self.data.get("corpus_mode", True)))
        layout.addSpacing(10)
        layout.addWidget(self.corpus_box)
        layout.addStretch(1)
        return page

    def _defaults_page(self) -> QWidget:
        page = QWidget()
        layout = QVBoxLayout(page)
        self._header(layout, "Tool Parameter Defaults",
                     "Each tool starts with these values every time you enter it.")
        self.default_editors: dict[str, dict[str, QWidget]] = {}
        saved = self.data.get("tool_defaults") or {}
        for tool in self.window_.tools:
            params = tool_parameters(tool)
            if not params:
                continue
            group = QGroupBox(tool.title)
            form = QFormLayout(group)
            editors = {}
            for name, widget in params.items():
                editor = _clone_editor(widget)
                set_widget_value(editor, (saved.get(tool.id) or {}).get(name, widget_value(widget)))
                form.addRow(_pretty(name), editor)
                editors[name] = editor
            self.default_editors[tool.id] = editors
            layout.addWidget(group)
        layout.addStretch(1)
        return page

    def _models_page(self) -> QWidget:
        page = QWidget()
        layout = QVBoxLayout(page)
        self._header(layout, "Models",
                     "The model that runs each step's AUTO. ★ marks the model designed for the "
                     "pipeline; you can try any other. Trained models are kept, not thrown away.")
        self.model_combos = {}
        self._metrics = settings_store.model_metrics()
        chosen = self.data.get("step_models") or {}
        tools = [t for t in self.window_.tools if settings_store.STEP_MODELS.get(t.id)]
        columns = ["Step", "Model"] + [label for _key, label in settings_store.METRIC_COLUMNS]
        self.metrics_table = QTableWidget(len(tools), len(columns))
        self.metrics_table.setHorizontalHeaderLabels(columns)
        self.metrics_table.verticalHeader().setVisible(False)
        self.metrics_table.setEditTriggers(QTableWidget.EditTrigger.NoEditTriggers)
        for row, tool in enumerate(tools):
            models = settings_store.STEP_MODELS[tool.id]
            starred = (chosen.get(tool.id) or {}).get("starred") or models[0]
            self.metrics_table.setItem(row, 0, QTableWidgetItem(tool.title))
            combo = QComboBox()
            for name in models:
                combo.addItem(("★ " if name == starred else "   ") + name, name)
            selected = settings_store.step_model(self.data, tool.id)
            combo.setCurrentIndex(max(0, models.index(selected) if selected in models else 0))
            combo.currentIndexChanged.connect(lambda _i, r=row, s=tool.id: self._fill_metrics(r, s))
            self.metrics_table.setCellWidget(row, 1, combo)
            self.model_combos[tool.id] = combo
            self._fill_metrics(row, tool.id)
        self.metrics_table.resizeColumnsToContents()
        self.metrics_table.horizontalHeader().setStretchLastSection(True)
        layout.addWidget(self.metrics_table, 1)
        note = QLabel(f"Metrics are measured on the locked holdout and filled in by each training run "
                      f"({settings_store.METRICS_PATH.relative_to(settings_store.HERE).as_posix()}); "
                      "'—' = not trained yet.")
        note.setWordWrap(True)
        note.setStyleSheet(f"color: {TEXT_MUTED};")
        layout.addWidget(note)
        return page

    def _fill_metrics(self, row: int, step: str) -> None:
        name = self.model_combos[step].currentData() if step in self.model_combos else None
        values = (self._metrics.get(step) or {}).get(name) or {}
        for column, (key, _label) in enumerate(settings_store.METRIC_COLUMNS, start=2):
            value = values.get(key)
            text = "—" if value is None else (f"{value:.3f}" if isinstance(value, float) else str(value))
            self.metrics_table.setItem(row, column, QTableWidgetItem(text))

    def _keys_page(self) -> QWidget:
        page = QWidget()
        layout = QVBoxLayout(page)
        self._header(layout, "Keybinds", "Click a shortcut to change it.")
        keys = self.data.get("keybinds") or {}
        table = QTableWidget(len(settings_store.DEFAULT_KEYBINDS), 2)
        table.setHorizontalHeaderLabels(["Action", "Shortcut"])
        table.verticalHeader().setVisible(False)
        self.key_editors = {}
        for row, (action, (label, default)) in enumerate(settings_store.DEFAULT_KEYBINDS.items()):
            item = QTableWidgetItem(label)
            item.setFlags(Qt.ItemFlag.ItemIsEnabled)
            table.setItem(row, 0, item)
            editor = QKeySequenceEdit(QKeySequence(keys.get(action, default)))
            table.setCellWidget(row, 1, editor)
            self.key_editors[action] = editor
        table.horizontalHeader().setStretchLastSection(True)
        table.resizeColumnToContents(0)
        layout.addWidget(table, 1)
        return page

    # ----------------------------------------------------------------- save

    def _save(self) -> None:
        data = self.data
        if hasattr(self, "step_boxes"):
            data["auto_steps"] = sorted(t for t, box in self.step_boxes.items() if box.isChecked())
            data["corpus_mode"] = self.corpus_box.isChecked()
        if hasattr(self, "default_editors"):
            data["tool_defaults"] = {tool_id: {name: widget_value(editor) for name, editor in editors.items()}
                                     for tool_id, editors in self.default_editors.items()}
        for step, combo in getattr(self, "model_combos", {}).items():
            entry = data.setdefault("step_models", {}).setdefault(step, {})
            entry["selected"] = combo.currentData()
        if hasattr(self, "key_editors"):
            data["keybinds"] = {action: editor.keySequence().toString()
                                for action, editor in self.key_editors.items()}
        settings_store.save(data)
        self.window_.apply_settings(data)
        self.accept()
