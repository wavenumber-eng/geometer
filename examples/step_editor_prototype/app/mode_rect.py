"""The tool carousel: the current tool in a large solid accent box in the
middle, the previous and next tools as smaller faded boxes on either side,
labelled PREV TOOL / NEXT TOOL underneath (box + label together are as tall as
the main box). Moving forward slides everything left; moving back (Ctrl+Tab,
Left arrow) slides everything right."""

from __future__ import annotations

from PySide6.QtCore import QEasingCurve, QRectF, Qt, QVariantAnimation, Signal
from PySide6.QtGui import QColor, QFont, QFontMetrics, QPainter, QPen
from PySide6.QtWidgets import QMenu, QSizePolicy, QWidget


class ModeRect(QWidget):
    tool_selected = Signal(str)  # tool id
    next_clicked = Signal()      # NEXT box clicked
    prev_clicked = Signal()      # PREV box clicked

    def __init__(self, parent=None) -> None:
        super().__init__(parent)
        self._slots = {"prev": ("", QColor("#4477aa")), "cur": ("", QColor("#4477aa")),
                       "next": ("", QColor("#4477aa"))}
        self._old = dict(self._slots)
        self._direction = 1
        self._tools: list[tuple[str, str]] = []
        self._automatic: set[str] = set()
        self._t = 1.0
        self._anim = QVariantAnimation(self)
        self._anim.setDuration(200)
        self._anim.setEasingCurve(QEasingCurve.Type.OutCubic)
        self._anim.valueChanged.connect(self._on_anim)
        self.setMinimumSize(420, 58)
        self.setMaximumHeight(62)
        self.setSizePolicy(QSizePolicy.Policy.Expanding, QSizePolicy.Policy.Fixed)
        self.setCursor(Qt.CursorShape.PointingHandCursor)
        self.setToolTip("Current tool (click to choose any tool). Side boxes, Tab / Ctrl+Tab or the "
                        "arrow keys move to the next / previous tool.")

    def set_tools(self, tools: list[tuple[str, str]]) -> None:
        self._tools = tools

    def set_automatic(self, tool_ids) -> None:
        self._automatic = set(tool_ids)

    def set_current(self, title: str, accent: str, next_title: str = "", next_accent: str = "",
                    prev_title: str = "", prev_accent: str = "", animate: bool = False) -> None:
        new = {"prev": (prev_title, QColor(prev_accent or "#4477aa")),
               "cur": (title, QColor(accent)),
               "next": (next_title, QColor(next_accent or "#4477aa"))}
        if animate and self._slots["cur"][0]:
            if title == self._slots["next"][0]:
                self._direction = 1
            elif title == self._slots["prev"][0]:
                self._direction = -1
            else:
                animate = False
        self._old, self._slots = self._slots, new
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

    # ------------------------------------------------------------- geometry

    def _rects(self) -> dict[str, QRectF]:
        h = self.height() - 2.0
        w = self.width() - 2.0
        main_w = min(max(w * 0.42, 220.0), 380.0)
        side_w = (w - main_w) / 2 - 10
        label_h = 13.0
        side_h = h - label_h
        main = QRectF(1 + (w - main_w) / 2, 1, main_w, h)
        prev = QRectF(1, 1, side_w, side_h)
        nxt = QRectF(main.right() + 10, 1, side_w, side_h)
        return {"prev": prev, "cur": main, "next": nxt}

    # ------------------------------------------------------------- painting

    def _draw_box(self, painter, rect: QRectF, accent: QColor, alpha: float, title: str, big: bool) -> None:
        if not title or rect.width() < 4:
            return
        color = QColor(accent)
        color.setAlphaF(max(0.0, min(1.0, alpha)))
        border = QColor(accent.darker(130))
        border.setAlphaF(color.alphaF())
        painter.setPen(QPen(border, 2))
        painter.setBrush(color)
        painter.drawRoundedRect(rect, 8, 8)
        text = QColor(255, 255, 255)
        text.setAlphaF(min(1.0, alpha + 0.3))
        painter.setPen(text)
        font = QFont(self.font())
        font.setBold(True)
        font.setPointSizeF(font.pointSizeF() + (3.0 if big else -0.5))
        painter.setFont(font)
        label = QFontMetrics(font).elidedText(title, Qt.TextElideMode.ElideRight, int(rect.width() - 12))
        painter.drawText(rect, int(Qt.AlignmentFlag.AlignCenter), label)

    def _draw_caption(self, painter, rect: QRectF, text: str, alpha: float) -> None:
        if rect.width() < 4:
            return
        font = QFont(self.font())
        font.setBold(True)
        font.setPointSizeF(max(font.pointSizeF() - 2.0, 6.5))
        painter.setFont(font)
        color = QColor(200, 200, 200)
        color.setAlphaF(max(0.0, min(1.0, alpha)))
        painter.setPen(color)
        caption = QRectF(rect.x(), rect.bottom() + 1, rect.width(), 12)
        painter.drawText(caption, int(Qt.AlignmentFlag.AlignCenter), text)

    def paintEvent(self, _event) -> None:
        painter = QPainter(self)
        painter.setRenderHint(QPainter.RenderHint.Antialiasing, True)
        rects = self._rects()
        self._hit = rects
        t = self._t
        if t >= 1.0:
            for slot in ("prev", "next"):
                title, accent = self._slots[slot]
                self._draw_box(painter, rects[slot], accent, 0.45, title, big=False)
                if title:
                    self._draw_caption(painter, rects[slot], "PREV TOOL" if slot == "prev" else "NEXT TOOL", 0.8)
            title, accent = self._slots["cur"]
            self._draw_box(painter, rects["cur"], accent, 1.0, title, big=True)
            return
        # sliding: forward (+1) moves boxes left, back (-1) moves them right
        d = self._direction
        order = ["prev", "cur", "next"]

        def lerp(a: QRectF, b: QRectF, k: float) -> QRectF:
            return QRectF(a.x() + (b.x() - a.x()) * k, a.y() + (b.y() - a.y()) * k,
                          a.width() + (b.width() - a.width()) * k, a.height() + (b.height() - a.height()) * k)

        for k, slot in enumerate(order):
            title, accent = self._old[slot]
            target_index = k - d
            if 0 <= target_index < 3:
                target = rects[order[target_index]]
            else:  # slides out past the edge
                target = rects[slot].translated(-d * (rects[slot].width() + 20), 0)
            rect = lerp(rects[slot], target, t)
            alpha_from = 1.0 if slot == "cur" else 0.45
            alpha_to = 1.0 if 0 <= target_index < 3 and order[target_index] == "cur" else (
                0.45 if 0 <= target_index < 3 else 0.0)
            self._draw_box(painter, rect, accent, alpha_from + (alpha_to - alpha_from) * t, title,
                           big=(slot == "cur" and t < 0.5) or (alpha_to == 1.0 and t >= 0.5))
        incoming = "next" if d == 1 else "prev"
        title, accent = self._slots[incoming]
        self._draw_box(painter, rects[incoming], accent, 0.45 * t, title, big=False)

    def mousePressEvent(self, event) -> None:
        if event.button() != Qt.MouseButton.LeftButton:
            return
        position = event.position()
        rects = getattr(self, "_hit", None) or self._rects()
        if rects["next"].adjusted(0, 0, 0, 14).contains(position):
            self.next_clicked.emit()
            return
        if rects["prev"].adjusted(0, 0, 0, 14).contains(position):
            self.prev_clicked.emit()
            return
        if not self._tools:
            return
        menu = QMenu(self)
        for tool_id, title in self._tools:
            automatic = tool_id in self._automatic
            action = menu.addAction(f"{title}   (automatic)" if automatic else title)
            action.setData(tool_id)
            action.setEnabled(not automatic)
        chosen = menu.exec(self.mapToGlobal(rects["cur"].bottomLeft().toPoint()))
        if chosen is not None:
            self.tool_selected.emit(chosen.data())
