"""Camera cube: a small translucent cube in the 3D view's upper-left corner.

Its surface is cut 3 x 3 on every side into 26 click targets: 6 faces,
12 edges and 8 corners. Clicking one turns the main camera to look from that
direction. The cube keeps its own orientation (drag it to turn the cube, not
the model), so a hidden face can be brought round and clicked without the
model spinning. Faces carry bold letters: F front (-Y), B back (+Y),
L left (-X), R right (+X), T top (+Z), G ground / bottom (-Z). Hovering tints
the target yellow."""

from __future__ import annotations

import itertools
import math

import numpy as np
from PySide6.QtCore import QPointF, Qt, Signal
from PySide6.QtGui import QColor, QFont, QPainter, QPen, QPolygonF
from PySide6.QtWidgets import QWidget

LETTERS = {(0, -1, 0): "F", (0, 1, 0): "B", (-1, 0, 0): "L", (1, 0, 0): "R", (0, 0, 1): "T", (0, 0, -1): "G"}
EDGE = 0.32  # width of the edge / corner bands, as a share of the half-size


def _tiles():
    """Every surface tile: (corners 4x3, outward normal, target direction)."""
    cuts = [-1.0, -1.0 + EDGE, 1.0 - EDGE, 1.0]
    tiles = []
    for axis in range(3):
        for sign in (-1, 1):
            u_axis, v_axis = [a for a in range(3) if a != axis]
            for i, j in itertools.product(range(3), range(3)):
                corners = []
                for du, dv in ((0, 0), (1, 0), (1, 1), (0, 1)):
                    p = [0.0, 0.0, 0.0]
                    p[axis] = sign
                    p[u_axis] = cuts[i + du]
                    p[v_axis] = cuts[j + dv]
                    corners.append(p)
                target = [0, 0, 0]
                target[axis] = sign
                target[u_axis] = (-1, 0, 1)[i]
                target[v_axis] = (-1, 0, 1)[j]
                normal = [0, 0, 0]
                normal[axis] = sign
                tiles.append((np.array(corners), np.array(normal, dtype=float), tuple(target)))
    return tiles


TILES = _tiles()


def camera_for(target: tuple[int, int, int]):
    """(direction the camera looks FROM, view-up) for a cube target."""
    d = np.array(target, dtype=float)
    d /= np.linalg.norm(d)
    if abs(d[2]) > 0.999:  # straight down / up: +Y up on screen
        return d, np.array([0.0, 1.0, 0.0])
    return d, np.array([0.0, 0.0, 1.0])


class CameraCube(QWidget):
    view_requested = Signal(object, object)  # direction, up (numpy arrays)

    def __init__(self, parent=None, size: int = 118) -> None:
        super().__init__(parent)
        self.setFixedSize(size, size)
        self.setMouseTracking(True)
        self.setAttribute(Qt.WidgetAttribute.WA_TranslucentBackground)
        self.setToolTip("Click a face, edge or corner to view from there; drag to turn the cube")
        self.yaw, self.pitch = -45.0, 28.0  # F toward the lower left, seen from above
        self._hover = None
        self._press = None
        self._dragged = False

    # ------------------------------------------------------------ geometry

    def _rotation(self) -> np.ndarray:
        """World -> view: yaw about Z, then pitch about the screen X."""
        y, p = math.radians(self.yaw), math.radians(self.pitch)
        rz = np.array([[math.cos(y), -math.sin(y), 0], [math.sin(y), math.cos(y), 0], [0, 0, 1]])
        # view axes: x right, y up, z toward the viewer; world Z is up, world -Y faces the viewer
        to_view = np.array([[1, 0, 0], [0, 0, 1], [0, -1, 0]], dtype=float)
        rx = np.array([[1, 0, 0], [0, math.cos(p), -math.sin(p)], [0, math.sin(p), math.cos(p)]])
        return rx @ to_view @ rz

    def _project(self, points: np.ndarray) -> list[QPointF]:
        r = self._rotation()
        v = points @ r.T
        scale = self.width() * 0.27
        cx, cy = self.width() / 2, self.height() / 2
        return [QPointF(cx + p[0] * scale, cy - p[1] * scale) for p in v]

    def _visible_tiles(self):
        r = self._rotation()
        out = []
        for corners, normal, target in TILES:
            facing = (r @ normal)[2]
            if facing > 1e-3:
                depth = float((corners @ r.T)[:, 2].mean())
                out.append((depth, corners, normal, target, facing))
        out.sort(key=lambda t: t[0])
        return out

    def _target_at(self, pos: QPointF):
        for _depth, corners, _normal, target, _f in reversed(self._visible_tiles()):
            if QPolygonF(self._project(corners)).containsPoint(pos, Qt.FillRule.OddEvenFill):
                return target
        return None

    # ------------------------------------------------------------- painting

    def paintEvent(self, _event) -> None:
        painter = QPainter(self)
        painter.setRenderHint(QPainter.RenderHint.Antialiasing, True)
        for _depth, corners, _normal, target, facing in self._visible_tiles():
            shade = 150 + int(70 * facing)
            fill = QColor(shade, shade, shade + 8, 150)
            if target == self._hover:
                fill = QColor(255, 214, 40, 170)
            painter.setBrush(fill)
            painter.setPen(QPen(QColor(40, 40, 48, 200), 1))
            painter.drawPolygon(QPolygonF(self._project(corners)))
        font = QFont(self.font())
        font.setBold(True)
        font.setPointSizeF(max(9.0, self.width() / 9.5))
        painter.setFont(font)
        r = self._rotation()
        for direction, letter in LETTERS.items():
            normal = np.array(direction, dtype=float)
            if (r @ normal)[2] <= 0.15:
                continue
            centre = self._project(np.array([normal]))[0]
            painter.setPen(QColor(15, 15, 20, 230))
            painter.drawText(int(centre.x() - 20), int(centre.y() - 12), 40, 24,
                             int(Qt.AlignmentFlag.AlignCenter), letter)

    # ---------------------------------------------------------------- mouse

    def mousePressEvent(self, event) -> None:
        if event.button() == Qt.MouseButton.LeftButton:
            self._press = event.position()
            self._dragged = False

    def mouseMoveEvent(self, event) -> None:
        pos = event.position()
        if self._press is not None and event.buttons() & Qt.MouseButton.LeftButton:
            dx, dy = pos.x() - self._press.x(), pos.y() - self._press.y()
            if self._dragged or abs(dx) + abs(dy) > 3:
                self._dragged = True
                self.yaw += dx * 0.8
                self.pitch = max(-89.0, min(89.0, self.pitch + dy * 0.8))
                self._press = pos
        target = self._target_at(pos)
        if target != self._hover:
            self._hover = target
        self.update()

    def mouseReleaseEvent(self, event) -> None:
        if event.button() == Qt.MouseButton.LeftButton and not self._dragged:
            target = self._target_at(event.position())
            if target is not None:
                direction, up = camera_for(target)
                self.view_requested.emit(direction, up)
        self._press = None
        self._dragged = False

    def leaveEvent(self, _event) -> None:
        self._hover = None
        self.update()
