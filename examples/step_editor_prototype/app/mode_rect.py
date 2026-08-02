"""The tool-mode rectangle: current tool in a solid accent box, the NEXT tool
in the chain as a 0.5-alpha box to its right. Pressing Tab slides the next box
left into place while a new next-box grows in from the right."""

from __future__ import annotations

from PySide6.QtCore import QEasingCurve, QRectF, Qt, QVariantAnimation, Signal
from PySide6.QtGui import QColor, QFont, QFontMetrics, QPainter, QPen
from PySide6.QtWidgets import QMenu, QSizePolicy, QWidget


class ModeRect(QWidget):
    tool_selected = Signal(str)  # tool id
    next_clicked = Signal()      # ghost box clicked -> cycle

    def __init__(self, parent=None) -> None:
        super().__init__(parent)
        self._title = ""
        self._accent = QColor("#4477aa")
        self._next_title = ""
        self._next_accent = QColor("#4477aa")
        self._old_title = ""
        self._old_accent = QColor("#4477aa")
        self._tools: list[tuple[str, str]] = []
        self._main_w = 240.0
        self._ghost_w = 130.0
        self._t = 1.0  # animation progress; 1 = idle
        self._anim = QVariantAnimation(self)
        self._anim.setDuration(190)
        self._anim.setEasingCurve(QEasingCurve.Type.OutCubic)
        self._anim.valueChanged.connect(self._on_anim)
        self.setMinimumSize(300, 44)
        self.setMaximumHeight(48)
        self.setSizePolicy(QSizePolicy.Policy.Preferred, QSizePolicy.Policy.Fixed)
        self.setCursor(Qt.CursorShape.PointingHandCursor)
        self.setToolTip("Click to choose a tool; click the faded box (or Tab) for the next one")

    def set_tools(self, tools: list[tuple[str, str]]) -> None:
        self._tools = tools

    def set_current(
        self, title: str, accent: str,
        next_title: str = "", next_accent: str = "",
        animate: bool = False,
    ) -> None:
        animate = animate and title == self._next_title and bool(self._title)
        self._old_title, self._old_accent = self._title, self._accent
        self._title = title
        self._accent = QColor(accent)
        self._next_title = next_title
        self._next_accent = QColor(next_accent or "#4477aa")

        title_font = QFont(self.font())
        title_font.setBold(True)
        title_font.setPointSizeF(title_font.pointSizeF() + 1.5)
        metrics = QFontMetrics(title_font)
        self._main_w = metrics.horizontalAdvance(f"TOOL: {title}") + 44
        self._ghost_w = max(
            96.0, min(220.0, metrics.horizontalAdvance(next_title) * 0.78 + 36)
        )
        self.setMinimumWidth(int(self._main_w + self._ghost_w + 10))

        if animate:
            self._anim.stop()
            self._anim.setStartValue(0.0)
            self._anim.setEndValue(1.0)
            self._anim.start()
        else:
            self._t = 1.0
            self.update()

    def _on_anim(self, value) -> None:
        self._t = float(value)
        self.update()

    # ------------------------------------------------------------- painting

    def _draw_box(self, painter, rect: QRectF, accent: QColor, alpha: float,
                  title: str, *, big: bool, hint: str = "") -> None:
        color = QColor(accent)
        color.setAlphaF(max(0.0, min(1.0, alpha)))
        border = QColor(accent.darker(130))
        border.setAlphaF(color.alphaF())
        painter.setPen(QPen(border, 2))
        painter.setBrush(color)
        painter.drawRoundedRect(rect, 8, 8)

        text = QColor(255, 255, 255)
        text.setAlphaF(min(1.0, alpha + 0.25))
        painter.setPen(text)
        font = QFont(self.font())
        font.setBold(big)
        font.setPointSizeF(font.pointSizeF() + (1.5 if big else -0.5))
        painter.setFont(font)
        label = f"TOOL: {title}" if big else title
        flags = Qt.AlignmentFlag.AlignHCenter | Qt.AlignmentFlag.AlignVCenter
        text_rect = rect.adjusted(8, 2, -8, -12 if hint else -2)
        painter.drawText(text_rect, flags, label)
        if hint:
            hint_font = QFont(self.font())
            hint_font.setPointSizeF(max(hint_font.pointSizeF() - 2.0, 6.5))
            painter.setFont(hint_font)
            faded = QColor(255, 255, 255)
            faded.setAlphaF(min(1.0, alpha + 0.1))
            painter.setPen(faded)
            painter.drawText(
                rect.adjusted(8, 0, -8, -4),
                Qt.AlignmentFlag.AlignHCenter | Qt.AlignmentFlag.AlignBottom,
                hint,
            )

    def paintEvent(self, _event) -> None:
        painter = QPainter(self)
        painter.setRenderHint(QPainter.RenderHint.Antialiasing, True)
        height = self.height() - 2
        gap = 8.0
        main = QRectF(1, 1, self._main_w, height)
        ghost = QRectF(self._main_w + gap, 1 + height * 0.1,
                       self._ghost_w, height * 0.8)
        self._main_rect, self._ghost_rect = main, ghost
        t = self._t

        if t >= 1.0:
            self._draw_box(painter, main, self._accent, 1.0, self._title, big=True)
            if self._next_title:
                self._draw_box(painter, ghost, self._next_accent, 0.5,
                               self._next_title, big=False, hint="Tab ⇥")
            return

        # outgoing current slides left and fades
        out_rect = main.translated(-36.0 * t, 0)
        self._draw_box(painter, out_rect, self._old_accent, (1.0 - t) * 0.8,
                       self._old_title, big=True)
        # the old ghost slides left and solidifies into the main slot
        x = ghost.x() + (main.x() - ghost.x()) * t
        y = ghost.y() + (main.y() - ghost.y()) * t
        w = ghost.width() + (main.width() - ghost.width()) * t
        h = ghost.height() + (main.height() - ghost.height()) * t
        self._draw_box(painter, QRectF(x, y, w, h), self._accent,
                       0.5 + 0.5 * t, self._title, big=True)
        # the new ghost grows in from the right edge
        if self._next_title:
            gw = ghost.width() * t
            grow = QRectF(ghost.right() - gw, ghost.y(), gw, ghost.height())
            self._draw_box(painter, grow, self._next_accent, 0.5 * t,
                           self._next_title, big=False)

    def mousePressEvent(self, event) -> None:
        if event.button() != Qt.MouseButton.LeftButton:
            return
        position = event.position()
        if getattr(self, "_ghost_rect", None) and self._ghost_rect.contains(position):
            self.next_clicked.emit()
            return
        if not self._tools:
            return
        menu = QMenu(self)
        for tool_id, title in self._tools:
            action = menu.addAction(title)
            action.setData(tool_id)
        chosen = menu.exec(self.mapToGlobal(self.rect().bottomLeft()))
        if chosen is not None:
            self.tool_selected.emit(chosen.data())
