"""Assign Pin Functions/Names (M5): a mini schematic of the chip in the
actions panel — pin stubs placed from the real 3D centroids — with function
labels (GND, PWR, SDA, ...) per pin. Functions are geometric metadata: each
label is tied to a physically located pin, ready for the AP242 metadata
embedding (M8)."""

from __future__ import annotations

from PySide6.QtCore import QPointF, QRectF, Qt, Signal
from PySide6.QtGui import QColor, QFont, QPainter, QPen
from PySide6.QtWidgets import (
    QComboBox,
    QHBoxLayout,
    QLabel,
    QLineEdit,
    QPushButton,
    QVBoxLayout,
    QWidget,
)

from ..style import ACCENT_HOVER, BORDER, CONSOLE_BG, SELECT_BG, TEXT_MUTED, TEXT_PRIMARY
from .base import ToolMode

# (designator, short description shown in the dropdown) — grouped: power,
# grounds, digital buses, control, analog, switch/relay, diode/LED, misc.
FUNCTIONS = [
    ("INHERIT", "flag: takes whatever function the other pins on this NET have"),
    ("PWR", "power supply, generic"),
    ("VCC", "positive supply (bipolar convention)"),
    ("VDD", "positive supply (MOS convention)"),
    ("VIN", "voltage input (regulator/converter)"),
    ("VOUT", "voltage output (regulator/converter)"),
    ("VBAT", "battery supply input"),
    ("VBUS", "USB 5 V bus power"),
    ("VREF", "voltage reference"),
    ("GND", "ground / 0 V return"),
    ("AGND", "analog ground"),
    ("DGND", "digital ground"),
    ("PGND", "power ground (high current)"),
    ("VSS", "negative/ground supply (MOS convention)"),
    ("EP", "exposed pad (thermal/ground slug)"),
    ("GPIO", "general-purpose I/O"),
    ("IO", "digital input/output"),
    ("PWM", "pulse-width modulated output"),
    ("CLK", "clock signal"),
    ("XTAL", "crystal oscillator pin"),
    ("SDA", "I2C data"),
    ("SCL", "I2C clock"),
    ("SCK", "SPI clock"),
    ("CS", "SPI chip select"),
    ("MISO", "SPI data, peripheral to host"),
    ("MOSI", "SPI data, host to peripheral"),
    ("RX", "UART receive"),
    ("TX", "UART transmit"),
    ("RTS", "UART request to send"),
    ("CTS", "UART clear to send"),
    ("DP", "USB data +"),
    ("DM", "USB data -"),
    ("CANH", "CAN bus high"),
    ("CANL", "CAN bus low"),
    ("EN", "enable / shutdown control"),
    ("RST", "reset input"),
    ("BOOT", "boot mode select / bootstrap"),
    ("INT", "interrupt request output"),
    ("WP", "write protect"),
    ("PG", "power good indicator"),
    ("FB", "regulator feedback"),
    ("SW", "switch node (DC-DC inductor pin)"),
    ("SS", "soft start"),
    ("AIN", "analog input (ADC)"),
    ("AOUT", "analog output (DAC)"),
    ("INP", "amplifier non-inverting input (+)"),
    ("INN", "amplifier inverting input (-)"),
    ("OUT", "output, generic"),
    ("ANT", "RF antenna"),
    ("NO", "normally-open contact (switch/relay)"),
    ("COM", "common terminal (switch/relay)"),
    ("A", "anode (diode/LED +)"),
    ("K", "cathode (diode/LED -)"),
    ("SHIELD", "shield / chassis connection"),
    ("MNT", "mechanical mount, not electrical"),
    ("TP", "test point"),
    ("NC", "not connected (on switches: normally-closed contact)"),
    ("DNC", "do not connect (internally used)"),
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
        self.setMinimumSize(220, 240)

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

        # classic KiCad symbol colours
        BODY_FILL = QColor(255, 255, 194)
        OUTLINE = QColor(132, 0, 0)
        PIN_NAME = QColor(0, 132, 132)
        PIN_NUMBER = QColor(132, 0, 0)

        margin, stub = 70.0, 16.0
        body = QRectF(margin, margin,
                      self.width() - 2 * margin, self.height() - 2 * margin)
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
    title = "Assign Pin Functions/Names"
    accent = "#9e2f6e"

    def build_actions_widget(self) -> QWidget:
        widget = QWidget()
        layout = QVBoxLayout(widget)
        layout.setContentsMargins(8, 8, 8, 8)
        layout.addWidget(QLabel(
            "<b>Assign Pin Functions/Names</b><br>"
            "Click a pin (or drag a rectangle over several) in the schematic "
            "or the 3D view, pick a function, press Set — it applies to the "
            "whole selection and steps to the next pin."
        ))

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
        layout.addWidget(self.schematic, 3)

        row = QHBoxLayout()
        self.selected_label = QLabel("—")
        row.addWidget(self.selected_label)
        self.function_combo = QComboBox()
        self.function_combo.setEditable(True)
        for token, description in FUNCTIONS:
            self.function_combo.addItem(f"{token}{_SEPARATOR}{description}")
        # the description is information only: picking an item puts just the
        # designator in the text field; the description shows greyed below
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
        self._refresh()
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
        if pin.body_ids:
            self.ctx.scene.highlight_body(pin.body_ids[0])
        elif pin.face_ids:
            self.ctx.scene.highlight_faces(pin.face_ids)
        self._refresh()

    def _select_many(self, indices: list[int]) -> None:
        registry = self.ctx.window.pins
        self._selection = [i for i in indices if 0 <= i < len(registry.pins)]
        if not self._selection:
            return
        self.selected_label.setText(f"{len(self._selection)} pins")
        faces = [
            key
            for i in self._selection
            for key in registry.pins[i].face_ids
        ]
        if faces:
            self.ctx.scene.highlight_faces(faces)
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

    def _refresh(self) -> None:
        if self._actions_widget is None:
            return
        registry = self.ctx.window.pins
        # mouth pins are second exposures of a primary contact (joined by
        # designator) — drawing them would double every schematic stub
        self.schematic.set_pins(
            [(pin.number, pin.name, pin.function, pin.centroid)
             for pin in registry.pins if pin.role != "mouth"],
            set(getattr(self, "_selection", [])),
        )
