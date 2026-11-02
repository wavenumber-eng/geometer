"""Assign Pin Functions/Names (M5): a mini schematic of the chip in the
actions panel — pin stubs placed from the real 3D centroids — with function
labels (GND, PWR, SDA, ...) per pin. Functions are geometric metadata: each
label is tied to a physically located pin, ready for the AP242 metadata
embedding (M8)."""

from __future__ import annotations

import numpy as np

from PySide6.QtCore import QPointF, QRectF, Qt, Signal
from PySide6.QtGui import QColor, QFont, QPainter, QPen
from PySide6.QtWidgets import (
    QComboBox,
    QHBoxLayout,
    QLabel,
    QLineEdit,
    QPushButton,
    QTableWidget,
    QTableWidgetItem,
    QVBoxLayout,
    QWidget,
)

from ..style import ACCENT_HOVER, BORDER, CONSOLE_BG, SELECT_BG, TEXT_MUTED, TEXT_PRIMARY
from ..pins import specialty
from .base import ToolMode

# (designator, short description shown in the dropdown) — grouped: power,
# grounds, digital buses, control, analog, switch/relay, diode/LED, misc.
FUNCTIONS = [
    ("INHERIT", "takes its net's type"),
    ("UNKNOWN", "no data on this pin's function (no datasheet / part record): a valid answer, not a gap"),
    ("PWR", "power, generic"),
    ("VCC", "+ supply (bipolar)"),
    ("VDD", "+ supply (MOS)"),
    ("VIN", "voltage in"),
    ("VOUT", "voltage out"),
    ("VBAT", "battery in"),
    ("VBUS", "USB 5 V bus"),
    ("VREF", "voltage reference"),
    ("GND", "ground"),
    ("AGND", "analog ground"),
    ("DGND", "digital ground"),
    ("PGND", "power ground"),
    ("VSS", "- supply / ground"),
    ("EP", "exposed pad"),
    ("GPIO", "general I/O"),
    ("IO", "digital I/O"),
    ("PWM", "PWM output"),
    ("CLK", "clock"),
    ("XTAL", "crystal pin"),
    ("SDA", "I2C data"),
    ("SCL", "I2C clock"),
    ("SCK", "SPI clock"),
    ("CS", "SPI chip select"),
    ("MISO", "SPI data in"),
    ("MOSI", "SPI data out"),
    ("RX", "UART receive"),
    ("TX", "UART transmit"),
    ("RTS", "UART RTS"),
    ("CTS", "UART CTS"),
    ("DP", "USB data +"),
    ("DM", "USB data -"),
    ("CANH", "CAN high"),
    ("CANL", "CAN low"),
    ("EN", "enable"),
    ("RST", "reset"),
    ("BOOT", "boot select"),
    ("INT", "interrupt out"),
    ("WP", "write protect"),
    ("PG", "power good"),
    ("FB", "feedback"),
    ("SW", "switch node"),
    ("SS", "soft start"),
    ("AIN", "analog in"),
    ("AOUT", "analog out"),
    ("INP", "amp input +"),
    ("INN", "amp input -"),
    ("OUT", "output"),
    ("ANT", "antenna"),
    ("NO", "normally open"),
    ("COM", "common"),
    ("A", "anode (+)"),
    ("K", "cathode (-)"),
    ("SHIELD", "shield / chassis"),
    ("MNT", "mount, not electrical"),
    ("TP", "test point"),
    ("NC", "not connected"),
    ("DNC", "do not connect"),
]
_SEPARATOR = " — "


def combo_token(text: str) -> str:
    """'GND — ground / 0 V return' -> 'GND'; free text passes through."""
    return text.split(_SEPARATOR)[0].strip()


def parse_function_assignments(text: str) -> dict[int, str]:
    """Bulk paste: '1:GND, 2:VCC, 5:SDA' -> {1: 'GND', 2: 'VCC', 5: 'SDA'}."""
    result: dict[int, str] = {}
    for chunk in text.replace(";", ",").split(","):
        if ":" not in chunk:
            continue
        number, _, function = chunk.partition(":")
        try:
            result[int(number.strip())] = function.strip()
        except ValueError:
            continue
    return result


SIDE_MODES = ["Auto", "2 sides", "4 sides"]


STUB_MM = 0.8  # length of a ghost-symbol pin stub out of the body edge


def _edge_stub(rect: tuple, x: float, y: float) -> tuple:
    """A pin stub (x0, y0, x1, y1) leaving the symbol body's nearest edge at
    the pin's own position along that edge (a schematic pin always sits on
    the body's outline, wherever the real pin is)."""
    x0, x1, y0, y1 = rect
    gaps = {"left": x - x0, "right": x1 - x, "bottom": y - y0, "top": y1 - y}
    side = min(gaps, key=gaps.get)
    px, py = min(max(x, x0), x1), min(max(y, y0), y1)
    return {"left": (x0, py, x0 - STUB_MM, py), "right": (x1, py, x1 + STUB_MM, py),
            "bottom": (px, y0, px, y0 - STUB_MM), "top": (px, y1, px, y1 + STUB_MM)}[side]


def _symbol_layout(shown: list, rect: tuple, grid: bool) -> tuple[list, list, list]:
    """(stubs, labels, leaders) of the ghost symbol: each pin's stub on the
    body edge with its label at the stub's tip and a leader from the real
    pin; a BGA (grid) gets each ball's label in place and no stubs."""
    stubs, labels, leaders = [], [], []
    for pin in shown:
        x, y = float(pin.centroid[0]), float(pin.centroid[1])
        tag = f"{pin.name or pin.number} {pin.function}".strip()
        if grid:
            stubs.append(None)
            labels.append((x, y, tag, 0.0, 0.0))
            continue
        stub = _edge_stub(rect, x, y)
        stubs.append(stub)
        out = np.array([stub[2] - stub[0], stub[3] - stub[1]])
        out = out / (np.linalg.norm(out) or 1.0)
        labels.append((stub[2], stub[3], tag, float(out[0]), float(out[1])))
        leaders.append((x, y, stub[0], stub[1]))
    return stubs, labels, leaders


PIN_PITCH_PX = 26.0  # spacing of pins along a symbol side


def _symbol_body(sides: dict, width: float, height: float, margin: float) -> QRectF:
    """The symbol box sized by its pins, not stretched to the widget: each
    side as long as its pins need at PIN_PITCH_PX (a square when pins sit on
    all four sides, as on a QFN / QFP), shrunk to fit, centred."""
    across = (max(len(sides["top"]), len(sides["bottom"]), 1) + 1) * PIN_PITCH_PX
    down = (max(len(sides["left"]), len(sides["right"]), 1) + 1) * PIN_PITCH_PX
    across = max(across, 150.0)  # room for the pin-type labels inside
    if all(sides[k] for k in ("left", "right", "top", "bottom")):
        across = down = max(across, down)
    room_w, room_h = max(width - 2 * margin, 40.0), max(height - 2 * margin, 40.0)
    scale = min(1.0, room_w / across, room_h / down)
    across, down = across * scale, down * scale
    return QRectF((width - across) / 2, (height - down) / 2, across, down)


_NAME_RULES = (  # symbol pin name -> function, first match wins (names upper-cased)
    (("AGND",), "AGND"), (("DGND",), "DGND"), (("PGND",), "PGND"),
    (("GND", "VSS", "EPAD"), "GND"), (("VBUS",), "VBUS"), (("VBAT",), "VBAT"),
    (("VREF",), "VREF"), (("VIN",), "VIN"), (("VOUT",), "VOUT"), (("VDD",), "VDD"),
    (("VCC",), "VCC"), (("SDA",), "SDA"), (("SCL",), "SCL"), (("CLK", "SCK"), "CLK"),
    (("XTAL", "XIN", "XOUT"), "XTAL"), (("GPIO",), "GPIO"), (("PWM",), "PWM"),
)
_TYPE_RULES = {"power": "PWR", "input": "IO", "output": "IO", "i/o": "IO", "io": "IO",
               "open collector": "IO", "open emitter": "IO", "hiz": "IO", "bidirectional": "IO"}


def function_from_symbol(name: str, electrical: str) -> str:
    """A pin-type acronym from a symbol pin's name (GND, VBUS, SDA ...),
    else from its electrical type (power -> PWR, input/output -> IO); ''
    when neither says (passive), for the person to set."""
    upper = name.upper()
    for keys, function in _NAME_RULES:
        if any(key in upper for key in keys):
            return function
    return _TYPE_RULES.get(electrical.strip().lower(), "")


class MiniSchematicWidget(QWidget):
    pin_clicked = Signal(int)        # index into the registry
    pins_band_selected = Signal(list)  # indices captured by a drag rectangle

    def __init__(self) -> None:
        super().__init__()
        self._pins: list = []      # (number, name, function, centroid)
        self._selected: set[int] = set()
        self._hits: list[tuple[QRectF, int]] = []
        self._side_mode = "Auto"
        self._band_origin: QPointF | None = None
        self._band_rect: QRectF | None = None
        self.setMinimumSize(220, 320)

    def set_pins(self, pins, selected=()) -> None:
        self._pins = pins
        if isinstance(selected, int):
            selected = {selected} if selected >= 0 else set()
        self._selected = set(selected)
        self.update()

    def set_side_mode(self, mode: str) -> None:
        self._side_mode = mode
        self.update()

    # Drag a rectangle to select many pins at once; a short click still
    # selects the single pin under the cursor.
    def mousePressEvent(self, event) -> None:
        self._band_origin = event.position()
        self._band_rect = None

    def mouseMoveEvent(self, event) -> None:
        if self._band_origin is None:
            return
        point = event.position()
        if (point - self._band_origin).manhattanLength() > 6:
            self._band_rect = QRectF(self._band_origin, point).normalized()
            self.update()

    def mouseReleaseEvent(self, event) -> None:
        origin, band = self._band_origin, self._band_rect
        self._band_origin = None
        self._band_rect = None
        self.update()
        if band is not None:
            captured = sorted(
                index for rect, index in self._hits
                if band.contains(rect.center())
            )
            if captured:
                self.pins_band_selected.emit(captured)
            return
        if origin is None:
            return
        for rect, index in self._hits:
            if rect.contains(origin):
                self.pin_clicked.emit(index)
                return

    def paintEvent(self, _event) -> None:
        """Standard Wavenumber schematic symbol: a box with pin leads
        sticking out each side; the pin's FUNCTION sits inside the box
        justified toward its side, the designator sits outside the lead."""
        painter = QPainter(self)
        painter.setRenderHint(QPainter.RenderHint.Antialiasing, True)
        painter.fillRect(self.rect(), QColor(CONSOLE_BG))
        self._hits = []
        if not self._pins:
            painter.setPen(QColor(TEXT_MUTED))
            painter.drawText(14, 22, "No pins — run Detect Pins first")
            return

        xs = [p[3][0] for p in self._pins]
        ys = [p[3][1] for p in self._pins]
        cx, cy = (min(xs) + max(xs)) / 2, (min(ys) + max(ys)) / 2
        sx = max(max(xs) - min(xs), 1e-9)
        sy = max(max(ys) - min(ys), 1e-9)

        key = lambda i: self._pins[i][3]
        sides: dict[str, list[int]] = {"left": [], "right": [], "top": [], "bottom": []}
        if self._side_mode == "2 sides":
            # connector layout: split along the axis with the SMALLER spread
            # (the row/column separation axis), everything on left or right
            axis = 0 if sx <= sy else 1
            center = cx if axis == 0 else cy
            for index, (_n, _name, _f, c) in enumerate(self._pins):
                sides["left" if c[axis] < center else "right"].append(index)
            sides["left"].sort(key=lambda i: (-key(i)[1], key(i)[0]))
            sides["right"].sort(key=lambda i: (-key(i)[1], -key(i)[0]))
        else:
            # Auto / 4 sides (QFN): nearest side by normalized centroid
            for index, (_n, _name, _f, c) in enumerate(self._pins):
                dx = (c[0] - cx) / sx
                dy = (c[1] - cy) / sy
                if abs(dx) >= abs(dy):
                    sides["right" if dx >= 0 else "left"].append(index)
                else:
                    sides["top" if dy >= 0 else "bottom"].append(index)
            sides["left"].sort(key=lambda i: -key(i)[1])
            sides["right"].sort(key=lambda i: -key(i)[1])
            sides["top"].sort(key=lambda i: key(i)[0])
            sides["bottom"].sort(key=lambda i: key(i)[0])

        # classic KiCad symbol colors
        BODY_FILL = QColor(255, 255, 194)
        OUTLINE = QColor(132, 0, 0)
        PIN_NAME = QColor(0, 132, 132)
        PIN_NUMBER = QColor(132, 0, 0)

        margin, stub = 44.0, 16.0  # room for the leads and their numbers
        body = _symbol_body(sides, self.width(), self.height(), margin)
        painter.setBrush(BODY_FILL)
        painter.setPen(QPen(OUTLINE, 2.0))
        painter.drawRect(body)
        painter.setBrush(Qt.BrushStyle.NoBrush)

        font = QFont(self.font())
        font.setPointSizeF(max(font.pointSizeF() - 1.5, 6.5))
        painter.setFont(font)

        def draw_pin(index, line_a, line_b, designator_rect, d_align,
                     function_rect, f_align):
            number, name, function, _c = self._pins[index]
            selected = index in self._selected
            painter.setPen(QPen(OUTLINE, 1.2))
            painter.drawLine(line_a, line_b)
            if selected:
                painter.setPen(QPen(QColor(SELECT_BG), 2.0))
                painter.setBrush(Qt.BrushStyle.NoBrush)
                painter.drawEllipse(QRectF(line_a.x() - 6, line_a.y() - 6, 12, 12))
            painter.setPen(QColor(SELECT_BG) if selected else PIN_NUMBER)
            painter.drawText(designator_rect, d_align, name or str(number))
            if function:
                painter.setPen(PIN_NAME)
                painter.drawText(function_rect, f_align, function)
            self._hits.append(
                (QRectF(line_a.x() - 8, line_a.y() - 8, 16, 16), index)
            )

        v = Qt.AlignmentFlag.AlignVCenter
        for k, indices in (("left", sides["left"]), ("right", sides["right"])):
            n = len(indices)
            for slot, index in enumerate(indices):
                y = body.top() + (slot + 1) * body.height() / (n + 1)
                if k == "left":
                    draw_pin(index,
                             QPointF(body.left() - stub, y), QPointF(body.left(), y),
                             # KiCad: pin number ABOVE the lead
                             QRectF(body.left() - stub - 14, y - 16, stub + 28, 13),
                             Qt.AlignmentFlag.AlignHCenter | Qt.AlignmentFlag.AlignBottom,
                             QRectF(body.left() + 4, y - 8, body.width() / 2 - 8, 16),
                             Qt.AlignmentFlag.AlignLeft | v)
                else:
                    draw_pin(index,
                             QPointF(body.right() + stub, y), QPointF(body.right(), y),
                             QRectF(body.right() - 14, y - 16, stub + 28, 13),
                             Qt.AlignmentFlag.AlignHCenter | Qt.AlignmentFlag.AlignBottom,
                             QRectF(body.center().x() + 4, y - 8,
                                    body.width() / 2 - 8, 16),
                             Qt.AlignmentFlag.AlignRight | v)
        h = Qt.AlignmentFlag.AlignHCenter
        for k, indices in (("top", sides["top"]), ("bottom", sides["bottom"])):
            n = len(indices)
            for slot, index in enumerate(indices):
                x = body.left() + (slot + 1) * body.width() / (n + 1)
                if k == "top":
                    draw_pin(index,
                             QPointF(x, body.top() - stub), QPointF(x, body.top()),
                             QRectF(x - 30, body.top() - stub - 18, 60, 14),
                             h | Qt.AlignmentFlag.AlignBottom,
                             QRectF(x - 30, body.top() + 3, 60, 14),
                             h | Qt.AlignmentFlag.AlignTop)
                else:
                    draw_pin(index,
                             QPointF(x, body.bottom() + stub), QPointF(x, body.bottom()),
                             QRectF(x - 30, body.bottom() + stub + 4, 60, 14),
                             h | Qt.AlignmentFlag.AlignTop,
                             QRectF(x - 30, body.bottom() - 17, 60, 14),
                             h | Qt.AlignmentFlag.AlignBottom)

        if self._band_rect is not None:
            band_pen = QPen(QColor(ACCENT_HOVER), 1.0, Qt.PenStyle.DashLine)
            painter.setPen(band_pen)
            painter.setBrush(Qt.BrushStyle.NoBrush)
            painter.drawRect(self._band_rect)


class PinFunctionsTool(ToolMode):
    id = "pin_functions"
    title = "8 Embedded Schematic"
    accent = "#9e2f6e"
    auto_method = "_run_auto"

    def build_actions_widget(self) -> QWidget:
        widget = QWidget()
        layout = QVBoxLayout(widget)
        layout.setContentsMargins(8, 8, 8, 8)
        # the pin table: designator | type | schematic name (Type and Name edit in place)
        self.table = QTableWidget(0, 3)
        self.table.setHorizontalHeaderLabels(["Pin", "Type", "Schematic name"])
        self.table.verticalHeader().setVisible(False)
        self.table.verticalHeader().setDefaultSectionSize(18)
        self.table.horizontalHeader().setStretchLastSection(True)
        self.table.setToolTip("Type: the function acronym (GND, PWR, SDA ...). Schematic name: what\n"
                              "the datasheet / symbol calls the pin. Double-click a cell to edit.")
        self.table.itemChanged.connect(self._on_table_edit)
        self.table.currentCellChanged.connect(lambda row, _c, _pr, _pc: self._select_row(row))
        layout.addWidget(self.table, 3)

        sides_row = QHBoxLayout()
        auto_button = QPushButton("Auto")
        auto_button.setToolTip(
            "Net propagation: in every net where exactly one function is\n"
            "set, the other functionless pins take INHERIT. Real function\n"
            "names still come from the datasheet (you, or later an LLM)."
        )
        auto_button.clicked.connect(self._run_auto)
        sides_row.addWidget(auto_button)
        self.sides_button = QPushButton("Sides: Auto")
        self.sides_button.setToolTip(
            "How pins distribute on the schematic symbol:\n"
            "Auto — nearest side from the 3D positions\n"
            "2 sides — left/right only (connectors)\n"
            "4 sides — all four sides (QFN etc.)"
        )
        self.sides_button.clicked.connect(self._cycle_sides)
        sides_row.addWidget(self.sides_button)
        sides_row.addStretch(1)
        layout.addLayout(sides_row)

        self.schematic = MiniSchematicWidget()
        self.schematic.pin_clicked.connect(self._select)
        self.schematic.pins_band_selected.connect(self._select_many)
        self.schematic.setMinimumHeight(150)
        layout.addWidget(self.schematic, 2)

        row = QHBoxLayout()
        self.selected_label = QLabel("—")
        row.addWidget(self.selected_label)
        self.function_combo = QComboBox()
        self.function_combo.setEditable(True)
        for token, description in FUNCTIONS:
            self.function_combo.addItem(f"{token}{_SEPARATOR}{description}")
        # the description is information only: picking an item puts just the
        # designator in the text field; the description shows greyed below
        self.function_combo.setEditText("")
        self.function_combo.lineEdit().setPlaceholderText("type for several pins")
        self.function_combo.activated.connect(self._on_combo_pick)
        self.function_combo.lineEdit().textEdited.connect(self._update_description)
        row.addWidget(self.function_combo, 1)
        set_button = QPushButton("Set")
        set_button.setMinimumWidth(36)
        set_button.clicked.connect(self._set_function)
        row.addWidget(set_button)
        layout.addLayout(row)

        self.description_label = QLabel("")
        self.description_label.setStyleSheet(
            f"color: {TEXT_MUTED}; font-style: italic; padding-left: 4px;"
        )
        layout.addWidget(self.description_label)

        bulk_row = QHBoxLayout()
        self.bulk_edit = QLineEdit()
        self.bulk_edit.setPlaceholderText("bulk: 1:GND, 2:VCC, 5:SDA ...")
        self.bulk_edit.returnPressed.connect(self._bulk_assign)
        bulk_row.addWidget(self.bulk_edit, 1)
        bulk_button = QPushButton("Assign")
        bulk_button.setMinimumWidth(36)
        bulk_button.clicked.connect(self._bulk_assign)
        bulk_row.addWidget(bulk_button)
        layout.addLayout(bulk_row)
        return widget

    def enter(self) -> None:
        self._selection: list[int] = []
        if self.ctx.document is not None:
            self.ctx.window.snap_camera("top")  # pins read best from above
        self._refresh()
        self._show_pins()
        self.status(
            "Pin Functions: click a pin (or drag over several), choose a "
            "function, press Set"
        )

    def _run_auto(self) -> None:
        from ..auto import auto_inherit_functions

        registry = self.ctx.window.pins
        if not registry.pins:
            self.status("Pin Functions: no pins yet — run Detect Pins first")
            return
        changed = auto_inherit_functions(registry)
        if changed:
            self.ctx.journal.record(
                tool=self.id, params={"action": "auto_inherit"},
                inputs={}, result={"applied": changed},
            )
        self._refresh()
        self.status(
            f"Auto: {changed} pin(s) took INHERIT from their net"
            if changed else
            "Auto: nothing to propagate — set one function per net first"
        )

    def _cycle_sides(self) -> None:
        current = self.sides_button.text().split(": ", 1)[1]
        mode = SIDE_MODES[(SIDE_MODES.index(current) + 1) % len(SIDE_MODES)]
        self.sides_button.setText(f"Sides: {mode}")
        self.schematic.set_side_mode(mode)

    def exit(self) -> None:
        self.ctx.scene.clear_highlight()
        self.ctx.scene.clear_pins_xray()
        self.ctx.scene.clear_ghost_symbol()
        self.ctx.scene.plotter.render()

    def _pin_pieces(self, pin) -> list:
        """(points, tris) of a pin: its own solid, else its faces."""
        from ..document import _face_triangles

        bodies = self.ctx.document.bodies
        if pin.body_ids:
            return [_face_triangles(bodies[b].mesh, range(1, bodies[b].mesh.face_count + 1))
                    for b in pin.body_ids if 0 <= b < len(bodies) and bodies[b].mesh is not None]
        by_body: dict[int, list[int]] = {}
        for body_index, face in pin.face_ids:
            by_body.setdefault(body_index, []).append(face)
        return [_face_triangles(bodies[b].mesh, faces) for b, faces in by_body.items()
                if 0 <= b < len(bodies) and bodies[b].mesh is not None]

    def _show_pins(self) -> None:
        """Every pin grey through the model, the selected one(s) green."""
        if self.ctx.document is None:
            return
        registry = self.ctx.window.pins
        chosen = set(getattr(self, "_selection", []))
        selected, others = [], []
        for index, pin in enumerate(registry.pins):
            (selected if index in chosen else others).extend(self._pin_pieces(pin))
        self.ctx.scene.show_pins_xray(selected, others)

    def on_document_changed(self) -> None:
        if self._actions_widget is not None:
            self._refresh()

    def on_pick(self, pick) -> None:
        registry = self.ctx.window.pins
        for index, pin in enumerate(registry.pins):
            if pick.body_index in pin.body_ids or (
                (pick.body_index, pick.face_index) in pin.face_ids
            ):
                self._select(index)
                return

    # ---------------------------------------------------------------- impl

    def _on_combo_pick(self, index: int) -> None:
        token = combo_token(self.function_combo.itemText(index))
        self.function_combo.setEditText(token)
        self._update_description(token)

    def _update_description(self, text: str) -> None:
        token = combo_token(text).upper()
        description = next(
            (desc for tok, desc in FUNCTIONS if tok == token), ""
        )
        self.description_label.setText(description)

    def _select(self, index: int) -> None:
        registry = self.ctx.window.pins
        if not (0 <= index < len(registry.pins)):
            return
        self._selection = [index]
        pin = registry.pins[index]
        self.selected_label.setText(f"Pin {pin.number} {pin.name or ''}".strip())
        if pin.function:
            self.function_combo.setEditText(pin.function)
            self._update_description(pin.function)
        self._show_pins()
        self._refresh()

    def _select_many(self, indices: list[int]) -> None:
        registry = self.ctx.window.pins
        self._selection = [i for i in indices if 0 <= i < len(registry.pins)]
        if not self._selection:
            return
        self.selected_label.setText(f"{len(self._selection)} pins")
        self._show_pins()
        self._refresh()
        self.status(
            f"Pin Functions: {len(self._selection)} pins selected — choose a "
            f"function and press Set to assign it to all of them"
        )

    def _set_function(self) -> None:
        registry = self.ctx.window.pins
        selection = [
            i for i in getattr(self, "_selection", [])
            if 0 <= i < len(registry.pins)
        ]
        if not selection:
            self.status("Pin Functions: select a pin first")
            return
        function = combo_token(self.function_combo.currentText())
        for index in selection:
            registry.pins[index].function = function
        if len(selection) == 1:
            self.ctx.journal.record(
                tool=self.id, params={},
                inputs={"pin_number": registry.pins[selection[0]].number},
                result={"function": function},
            )
        else:
            self.ctx.journal.record(
                tool=self.id, params={"action": "set_multi"},
                inputs={"pin_numbers": [registry.pins[i].number
                                        for i in selection]},
                result={"function": function},
            )
        numbers = ", ".join(str(registry.pins[i].number) for i in selection[:8])
        self._refresh()
        # single assignment: step to the next pin so the user can rattle
        # down the sequence (skipping CON/HEAD pins — they inherit via nets)
        following = selection[-1] + 1
        if (
            len(selection) == 1
            and following < len(registry.pins)
            and registry.pins[following].role != "mouth"
        ):
            self._select(following)
            self.status(
                f"Pin Functions: pin {numbers} -> {function or '(cleared)'} — "
                f"next: pin {registry.pins[following].number}"
            )
        else:
            self.status(
                f"Pin Functions: pin(s) {numbers} -> {function or '(cleared)'}"
            )

    def _bulk_assign(self) -> None:
        registry = self.ctx.window.pins
        assignments = parse_function_assignments(self.bulk_edit.text())
        if not assignments:
            self.status("Pin Functions: nothing parsed — format is 1:GND, 2:VCC")
            return
        by_number = {pin.number: pin for pin in registry.pins}
        applied = 0
        for number, function in assignments.items():
            pin = by_number.get(number)
            if pin is not None:
                pin.function = function
                applied += 1
        self.ctx.journal.record(
            tool=self.id, params={"action": "bulk"},
            inputs={"text": self.bulk_edit.text()},
            result={"applied": applied},
        )
        self._refresh()
        self.status(f"Pin Functions: {applied} function(s) assigned")

    def _shown_pins(self) -> list:
        """The schematic's pins: no mouth pins (second exposures of a contact)
        and no GUIDE pegs (an unplated hole, never on a net); ANCHOR pins
        stay, they are soldered and netted."""
        return [pin for pin in self.ctx.window.pins.pins if pin.role != "mouth" and specialty(pin) != "guide"]

    def _fill_table(self) -> None:
        self.table.blockSignals(True)
        pins = self._shown_pins()
        self.table.setRowCount(len(pins))
        for row, pin in enumerate(pins):
            designator = QTableWidgetItem(pin.name or str(pin.number))
            designator.setFlags(designator.flags() & ~Qt.ItemFlag.ItemIsEditable)
            self.table.setItem(row, 0, designator)
            self.table.setItem(row, 1, QTableWidgetItem(pin.function))
            self.table.setItem(row, 2, QTableWidgetItem(getattr(pin, "signal", "")))
        self.table.resizeColumnsToContents()
        self.table.blockSignals(False)

    def _select_row(self, row: int) -> None:
        pins = self._shown_pins()
        if 0 <= row < len(pins):
            index = self.ctx.window.pins.pins.index(pins[row])
            if getattr(self, "_selection", []) != [index]:
                self._select(index)

    def _on_table_edit(self, item) -> None:
        pins = self._shown_pins()
        if not (0 <= item.row() < len(pins)) or item.column() == 0:
            return
        pin = pins[item.row()]
        text = item.text().strip()
        if item.column() == 1:
            pin.function = text.upper()
            self.ctx.journal.record(tool=self.id, params={}, inputs={"pin_number": pin.number},
                                    result={"function": pin.function})
        else:
            pin.signal = text
            self.ctx.journal.record(tool=self.id, params={"action": "signal"},
                                    inputs={"pin_number": pin.number}, result={"signal": text})
        self._refresh()

    def _refresh(self) -> None:
        if self._actions_widget is None:
            return
        self._fill_table()
        registry = self.ctx.window.pins
        # mouth pins are second exposures of a primary contact (joined by
        # designator) — drawing them would double every schematic stub
        shown = self._shown_pins()
        chosen = {id(registry.pins[i]) for i in getattr(self, "_selection", []) if 0 <= i < len(registry.pins)}
        selected = {k for k, pin in enumerate(shown) if id(pin) in chosen}
        self.schematic.set_pins([(pin.number, pin.name, pin.function, pin.centroid) for pin in shown], selected)
        self._show_ghost_symbol(shown, selected)

    def _show_ghost_symbol(self, shown: list, selected: set) -> None:
        """The symbol on the Z=0 plane in the 3D view: the package outline as
        a faint symbol body, a stub out of its edge at each edge pin's real
        position, and each pin's number + type at the pin (balls under a BGA
        get their label in place)."""
        if not self.is_active():
            return  # refreshed while another tool is on screen: draw nothing
        if self.ctx.document is None or not shown:
            self.ctx.scene.clear_ghost_symbol()
            return
        from .footprint_embed import _package_xy

        from ..pins import pin_pattern

        rect = _package_xy(self.ctx.document, self.ctx.window.pins.pins)
        grid = len(shown) >= 4 and pin_pattern([p.centroid for p in shown]) == "grid"
        stubs, labels, leaders = _symbol_layout(shown, rect, grid)
        self.ctx.scene.show_ghost_symbol(rect, stubs, labels, selected, leaders=leaders)
