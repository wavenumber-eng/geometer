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

FUNCTIONS = [
    "GND", "PWR", "AGND", "VREF", "GPIO", "PWM", "CLK", "SDA", "SCL",
    "RX", "TX", "EN", "CS", "MISO", "MOSI", "NC",
]


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


class MiniSchematicWidget(QWidget):
    pin_clicked = Signal(int)  # index into the registry

    def __init__(self) -> None:
        super().__init__()
        self._pins: list = []      # (number, name, function, centroid)
        self._selected = -1
        self._hits: list[tuple[QRectF, int]] = []
        self.setMinimumSize(220, 240)

    def set_pins(self, pins, selected: int = -1) -> None:
        self._pins = pins
        self._selected = selected
        self.update()

    def mousePressEvent(self, event) -> None:
        point = event.position()
        for rect, index in self._hits:
            if rect.contains(point):
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

        sides: dict[str, list[int]] = {"left": [], "right": [], "top": [], "bottom": []}
        for index, (_n, _name, _f, c) in enumerate(self._pins):
            dx = (c[0] - cx) / sx
            dy = (c[1] - cy) / sy
            if abs(dx) >= abs(dy):
                sides["right" if dx >= 0 else "left"].append(index)
            else:
                sides["top" if dy >= 0 else "bottom"].append(index)
        key = lambda i: self._pins[i][3]
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
            selected = index == self._selected
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
            "Click a pin in the schematic or the 3D view, pick a function "
            "(or type one), press Set. Functions are geometric — each label "
            "lives on a physically located pin."
        ))

        self.schematic = MiniSchematicWidget()
        self.schematic.pin_clicked.connect(self._select)
        layout.addWidget(self.schematic, 3)

        row = QHBoxLayout()
        self.selected_label = QLabel("—")
        row.addWidget(self.selected_label)
        self.function_combo = QComboBox()
        self.function_combo.setEditable(True)
        self.function_combo.addItems(FUNCTIONS)
        row.addWidget(self.function_combo, 1)
        set_button = QPushButton("Set")
        set_button.setMinimumWidth(36)
        set_button.clicked.connect(self._set_function)
        row.addWidget(set_button)
        layout.addLayout(row)

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
        self._selected = -1
        self._refresh()
        self.status("Pin Functions: click a pin, choose a function, press Set")

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

    def _select(self, index: int) -> None:
        self._selected = index
        registry = self.ctx.window.pins
        if not (0 <= index < len(registry.pins)):
            return
        pin = registry.pins[index]
        self.selected_label.setText(f"Pin {pin.number} {pin.name or ''}".strip())
        if pin.function:
            self.function_combo.setCurrentText(pin.function)
        if pin.body_ids:
            self.ctx.scene.highlight_body(pin.body_ids[0])
        elif pin.face_ids:
            self.ctx.scene.highlight_faces(pin.face_ids)
        self._refresh()

    def _set_function(self) -> None:
        registry = self.ctx.window.pins
        index = getattr(self, "_selected", -1)
        if not (0 <= index < len(registry.pins)):
            self.status("Pin Functions: select a pin first")
            return
        pin = registry.pins[index]
        pin.function = self.function_combo.currentText().strip()
        self.ctx.journal.record(
            tool=self.id, params={},
            inputs={"pin_number": pin.number},
            result={"function": pin.function},
        )
        self._refresh()
        self.status(f"Pin Functions: pin {pin.number} -> {pin.function}")

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
        self.schematic.set_pins(
            [(pin.number, pin.name, pin.function, pin.centroid)
             for pin in registry.pins],
            getattr(self, "_selected", -1),
        )
