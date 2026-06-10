"""Ortho2DCanvas: a 2D orthographic footprint view rendered from a real
geometer HLR projection (project_step_hlr), with pin / band overlays. This is
the IPC-footprint-style view used by Detect Pins and Assign Pin Hitboxes."""

from __future__ import annotations

import math
from typing import Any, Mapping

import geometer
from PySide6.QtCore import QPointF, Qt
from PySide6.QtGui import QColor, QFont, QPainter, QPen
from PySide6.QtWidgets import QWidget


class Ortho2DCanvas(QWidget):
    def __init__(self) -> None:
        super().__init__()
        self._result: geometer.HlrProjectionResult | None = None
        self._view_id = "top"
        self._pins: list[tuple[float, float, str]] = []      # x, y, label (world XY)
        self._band: tuple[float, float, float, float] | None = None
        self._rects: list[tuple[float, float, float, float, float]] = []  # cx, cy, hw, hh, rot_deg
        self.setMinimumSize(240, 200)

    def set_projection(self, result: geometer.HlrProjectionResult | None, view_id: str = "top") -> None:
        self._result = result
        self._view_id = view_id
        self.update()

    def set_pins(self, pins: list[tuple[float, float, str]]) -> None:
        self._pins = pins
        self.update()

    def set_band(self, band: tuple[float, float, float, float] | None) -> None:
        self._band = band
        self.update()

    def set_rects(self, rects: list[tuple[float, float, float, float, float]]) -> None:
        self._rects = rects
        self.update()

    # ------------------------------------------------------------- painting

    def paintEvent(self, _event: Any) -> None:
        painter = QPainter(self)
        painter.setRenderHint(QPainter.RenderHint.Antialiasing, True)
        painter.fillRect(self.rect(), QColor("#ffffff"))
        if self._result is None:
            painter.setPen(QColor("#657080"))
            painter.drawText(16, 24, "No footprint projection")
            return

        try:
            geometry = self._result.geometry(self._view_id, "detail")
        except KeyError:
            painter.setPen(QColor("#a33a3a"))
            painter.drawText(16, 24, "Projection view missing")
            return

        bounds = _geometry_bounds(geometry)
        for x, y, _label in self._pins:
            bounds = _include(bounds, x, y)
        if self._band is not None:
            bounds = _include(bounds, self._band[0], self._band[1])
            bounds = _include(bounds, self._band[2], self._band[3])
        if bounds is None:
            painter.setPen(QColor("#657080"))
            painter.drawText(16, 24, "Empty projection")
            return

        project = _projector(bounds, self.width(), self.height())

        painter.setPen(QPen(QColor(42, 54, 72), 1.4, Qt.PenStyle.SolidLine,
                            Qt.PenCapStyle.RoundCap, Qt.PenJoinStyle.RoundJoin))
        for segment in geometry.get("segments", []):
            if len(segment) == 4:
                painter.drawLine(
                    QPointF(*project(segment[0], segment[1])),
                    QPointF(*project(segment[2], segment[3])),
                )
        for arc in geometry.get("arcs", []):
            points = [QPointF(*project(x, y)) for x, y in _arc_points(arc)]
            for a, b in zip(points, points[1:]):
                painter.drawLine(a, b)

        if self._band is not None:
            x0, y0 = project(self._band[0], self._band[1])
            x1, y1 = project(self._band[2], self._band[3])
            painter.setPen(QPen(QColor(238, 132, 52, 230), 2.0, Qt.PenStyle.DashLine))
            painter.drawRect(QPointF(x0, y0).x(), QPointF(x1, y1).y(),
                             abs(x1 - x0), abs(y1 - y0))

        painter.setPen(QPen(QColor(31, 95, 214, 200), 1.6))
        for cx, cy, hw, hh, rot in self._rects:
            painter.save()
            px, py = project(cx, cy)
            painter.translate(px, py)
            painter.rotate(-rot)
            sx, _sy = project(cx + hw, cy)
            sx0, _ = project(cx, cy)
            half_w = abs(sx - sx0)
            _, sy = project(cx, cy + hh)
            _, sy0 = project(cx, cy)
            half_h = abs(sy - sy0)
            painter.drawRect(-half_w, -half_h, 2 * half_w, 2 * half_h)
            painter.restore()

        font = QFont(self.font())
        font.setBold(True)
        painter.setFont(font)
        for x, y, label in self._pins:
            px, py = project(x, y)
            painter.setPen(QPen(QColor("#b02020"), 1.0))
            painter.setBrush(QColor(255, 176, 0, 230))
            painter.drawEllipse(QPointF(px, py), 7.0, 7.0)
            painter.setPen(QColor("#202020"))
            painter.drawText(QPointF(px - 4 * len(label), py - 9), label)


class FootprintPadsCanvas(QWidget):
    """Pads-only land-pattern view: just the hitbox cross-sections drawn as
    copper pads on a PCB-style background, with pin numbers and the origin
    crosshair — what the part's real footprint would look like."""

    def __init__(self) -> None:
        super().__init__()
        self._rects: list[tuple[float, float, float, float, float]] = []
        self._pins: list[tuple[float, float, str]] = []
        self.setMinimumSize(200, 200)

    def set_rects(self, rects) -> None:
        self._rects = rects
        self.update()

    def set_pins(self, pins) -> None:
        self._pins = pins
        self.update()

    def paintEvent(self, _event: Any) -> None:
        painter = QPainter(self)
        painter.setRenderHint(QPainter.RenderHint.Antialiasing, True)
        painter.fillRect(self.rect(), QColor("#103e1c"))  # soldermask green
        if not self._rects:
            painter.setPen(QColor("#9fc5a8"))
            painter.drawText(16, 24, "No hitboxes assigned yet")
            return

        bounds = None
        for cx, cy, hw, hh, _rot in self._rects:
            radius = math.hypot(hw, hh)
            bounds = _include(bounds, cx - radius, cy - radius)
            bounds = _include(bounds, cx + radius, cy + radius)
        bounds = _include(bounds, 0.0, 0.0)  # keep the origin in frame
        project = _projector(bounds, self.width(), self.height())

        def length(value: float) -> float:
            x0, _ = project(0.0, 0.0)
            x1, _ = project(value, 0.0)
            return abs(x1 - x0)

        # origin crosshair
        painter.setPen(QPen(QColor(255, 255, 255, 90), 1.0, Qt.PenStyle.DashLine))
        ox, oy = project(0.0, 0.0)
        painter.drawLine(QPointF(0, oy), QPointF(self.width(), oy))
        painter.drawLine(QPointF(ox, 0), QPointF(ox, self.height()))

        for cx, cy, hw, hh, rot in self._rects:
            px, py = project(cx, cy)
            painter.save()
            painter.translate(px, py)
            painter.rotate(-rot)
            painter.setPen(QPen(QColor("#5a3415"), 1.0))
            painter.setBrush(QColor("#d8973c"))  # copper pad
            painter.drawRect(-length(hw), -length(hh), 2 * length(hw), 2 * length(hh))
            painter.restore()

        font = QFont(self.font())
        font.setBold(True)
        painter.setFont(font)
        painter.setPen(QColor("#ffffff"))
        for x, y, label in self._pins:
            px, py = project(x, y)
            painter.drawText(QPointF(px - 4 * len(label), py + 4), label)


def _geometry_bounds(geometry: Mapping[str, Any]):
    bounds = None
    for segment in geometry.get("segments", []):
        if len(segment) == 4:
            bounds = _include(bounds, segment[0], segment[1])
            bounds = _include(bounds, segment[2], segment[3])
    for arc in geometry.get("arcs", []):
        for x, y in _arc_points(arc):
            bounds = _include(bounds, x, y)
    return bounds


def _include(bounds, x: float, y: float):
    x, y = float(x), float(y)
    if bounds is None:
        return [x, y, x, y]
    bounds[0] = min(bounds[0], x)
    bounds[1] = min(bounds[1], y)
    bounds[2] = max(bounds[2], x)
    bounds[3] = max(bounds[3], y)
    return bounds


def _projector(bounds, width: int, height: int):
    margin = 24.0
    span_x = max(bounds[2] - bounds[0], 1.0e-9)
    span_y = max(bounds[3] - bounds[1], 1.0e-9)
    scale = min((width - 2 * margin) / span_x, (height - 2 * margin) / span_y)
    offset_x = (width - span_x * scale) * 0.5
    offset_y = (height - span_y * scale) * 0.5

    def project(x: float, y: float) -> tuple[float, float]:
        sx = offset_x + (float(x) - bounds[0]) * scale
        sy = height - (offset_y + (float(y) - bounds[1]) * scale)
        return sx, sy

    return project


def _arc_points(arc: Mapping[str, Any], samples: int = 48) -> list[tuple[float, float]]:
    center = arc.get("center", [0.0, 0.0])
    start = arc.get("start", [0.0, 0.0])
    radius = float(arc.get("radius", 0.0))
    if radius <= 0.0:
        return []
    cx, cy = float(center[0]), float(center[1])
    if arc.get("full_circle"):
        return [
            (cx + math.cos((2 * math.pi * i) / samples) * radius,
             cy + math.sin((2 * math.pi * i) / samples) * radius)
            for i in range(samples + 1)
        ]
    start_angle = math.atan2(float(start[1]) - cy, float(start[0]) - cx)
    extent = abs(float(arc.get("extent_rad", 0.0)))
    if not arc.get("ccw", True):
        extent = -extent
    return [
        (cx + math.cos(start_angle + (extent * i) / samples) * radius,
         cy + math.sin(start_angle + (extent * i) / samples) * radius)
        for i in range(samples + 1)
    ]
