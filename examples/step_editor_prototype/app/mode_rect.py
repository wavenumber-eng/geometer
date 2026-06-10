"""The tool-mode rectangle: the design intent's upper-middle indicator.
Click it (or press T, wired in the main window) to change tools."""

from __future__ import annotations

from PySide6.QtCore import Qt, Signal
from PySide6.QtGui import QColor, QFont, QFontMetrics, QPainter, QPen
from PySide6.QtWidgets import QMenu, QSizePolicy, QWidget


class ModeRect(QWidget):
    tool_selected = Signal(str)  # tool id

    def __init__(self, parent=None) -> None:
        super().__init__(parent)
        self._title = ""
        self._next_title = ""
        self._accent = QColor("#4477aa")
        self._tools: list[tuple[str, str]] = []  # (id, title)
        self.setMinimumSize(260, 44)
        self.setMaximumHeight(48)
        self.setSizePolicy(QSizePolicy.Policy.Preferred, QSizePolicy.Policy.Fixed)
        self.setCursor(Qt.CursorShape.PointingHandCursor)
        self.setToolTip("Click to choose a tool (Tab or T cycles)")

    def set_tools(self, tools: list[tuple[str, str]]) -> None:
        self._tools = tools

    def set_current(self, title: str, accent: str, next_title: str = "") -> None:
        self._title = title
        self._accent = QColor(accent)
        self._next_title = next_title
        # Size the rectangle to its text — long tool names must fit.
        title_font = QFont(self.font())
        title_font.setBold(True)
        title_font.setPointSizeF(title_font.pointSizeF() + 1.5)
        title_width = QFontMetrics(title_font).horizontalAdvance(f"TOOL: {title}")
        hint_font = QFont(self.font())
        hint_font.setPointSizeF(max(hint_font.pointSizeF() - 2.0, 6.5))
        hint_width = QFontMetrics(hint_font).horizontalAdvance(self._hint_text())
        self.setMinimumWidth(max(title_width, hint_width) + 48)
        self.update()

    def _hint_text(self) -> str:
        if self._next_title:
            return f"Tab ⇥ next: {self._next_title}"
        return "click or press Tab to change tool"

    def paintEvent(self, _event) -> None:
        painter = QPainter(self)
        painter.setRenderHint(QPainter.RenderHint.Antialiasing, True)
        rect = self.rect().adjusted(1, 1, -1, -1)
        painter.setPen(QPen(self._accent.darker(130), 2))
        painter.setBrush(self._accent)
        painter.drawRoundedRect(rect, 8, 8)

        painter.setPen(QColor("#ffffff"))
        font = QFont(self.font())
        font.setBold(True)
        font.setPointSizeF(font.pointSizeF() + 1.5)
        painter.setFont(font)
        painter.drawText(
            rect.adjusted(12, 2, -12, -14),
            Qt.AlignmentFlag.AlignHCenter | Qt.AlignmentFlag.AlignVCenter,
            f"TOOL: {self._title}",
        )
        hint_font = QFont(self.font())
        hint_font.setPointSizeF(max(hint_font.pointSizeF() - 2.0, 6.5))
        painter.setFont(hint_font)
        painter.setPen(QColor(255, 255, 255, 190))
        painter.drawText(
            rect.adjusted(12, 0, -12, -4),
            Qt.AlignmentFlag.AlignHCenter | Qt.AlignmentFlag.AlignBottom,
            self._hint_text(),
        )

    def mousePressEvent(self, event) -> None:
        if event.button() != Qt.MouseButton.LeftButton or not self._tools:
            return
        menu = QMenu(self)
        for tool_id, title in self._tools:
            action = menu.addAction(title)
            action.setData(tool_id)
        chosen = menu.exec(self.mapToGlobal(self.rect().bottomLeft()))
        if chosen is not None:
            self.tool_selected.emit(chosen.data())
