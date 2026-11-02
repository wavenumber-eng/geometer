"""Keep every tool panel narrow (PANEL_BUDGET px) so the 3D view keeps its room.

compact_panel() runs once per tool page:
- the long header paragraph becomes one short sentence, with the full text in
  a "How ▸" fold-out underneath;
- every label word-wraps; dropdowns stop sizing themselves to their longest item;
- any row still wider than the budget is reflowed into a 2-column grid (a label
  stays glued to the field after it), or 1 column if a pair alone is too wide.
"""

from __future__ import annotations

from PySide6.QtCore import QEvent, QObject, Qt
from PySide6.QtWidgets import (
    QScrollArea,
    QAbstractButton,
    QBoxLayout,
    QComboBox,
    QGridLayout,
    QHBoxLayout,
    QLabel,
    QWidget,
)

PANEL_BUDGET = 320


class Foldout(QLabel):
    """A one-line label that expands to its full text when clicked."""

    def __init__(self, title: str, full: str, short: str | None = None, parent=None) -> None:
        super().__init__(parent)
        self._title, self._full, self._short = title, full, short
        self._open = False
        self.setWordWrap(True)
        self.setTextFormat(Qt.TextFormat.RichText)
        self.setCursor(Qt.CursorShape.PointingHandCursor)
        self._render()

    def _render(self) -> None:
        arrow = "▾" if self._open else "▸"
        body = self._full if self._open else (self._short or "")
        sep = "<br>" if body else ""
        self.setText(f"<span style='color:#8a8f98'>{self._title} {arrow}</span>{sep}{body}")

    def mousePressEvent(self, event) -> None:  # noqa: N802 (Qt override)
        self._open = not self._open
        self._render()
        super().mousePressEvent(event)


def _row_width(layout) -> int:
    return layout.minimumSize().width()


def _units(layout: QHBoxLayout) -> list[list[QWidget]]:
    """The row's widgets, with each plain label glued to the widget after it."""
    widgets = [layout.itemAt(i).widget() for i in range(layout.count())]
    widgets = [w for w in widgets if w is not None]
    units, i = [], 0
    while i < len(widgets):
        w = widgets[i]
        if isinstance(w, QLabel) and not isinstance(w, QAbstractButton) and i + 1 < len(widgets):
            units.append([w, widgets[i + 1]])
            i += 2
        else:
            units.append([w])
            i += 1
    return units


def _reflow(row: QHBoxLayout, parent: QBoxLayout, budget: int) -> None:
    if any(row.itemAt(i).layout() is not None for i in range(row.count())):
        return  # nested layouts: leave alone
    units = _units(row)
    if len(units) < 2:
        return
    index = parent.indexOf(row)
    if index < 0:
        return
    unit_widths = [sum(w.minimumSizeHint().width() for w in u) + 6 * (len(u) - 1) for u in units]
    cols = 2 if max(unit_widths) * 2 + 6 <= budget else 1
    grid = QGridLayout()
    grid.setContentsMargins(0, 0, 0, 0)
    grid.setHorizontalSpacing(6)
    grid.setVerticalSpacing(4)
    for w in [w for u in units for w in u]:
        row.removeWidget(w)
    for n, unit in enumerate(units):
        cell = QHBoxLayout()
        cell.setContentsMargins(0, 0, 0, 0)
        for w in unit:
            cell.addWidget(w, 1 if not isinstance(w, QLabel) else 0)
        grid.addLayout(cell, n // cols, n % cols)
    parent.takeAt(index)
    parent.insertLayout(index, grid)


def _all_box_layouts(widget: QWidget):
    seen = []

    def walk(layout):
        if layout is None or layout in seen:
            return
        seen.append(layout)
        for i in range(layout.count()):
            item = layout.itemAt(i)
            if item.layout() is not None:
                walk(item.layout())
            elif item.widget() is not None:
                walk(item.widget().layout())

    walk(widget.layout())
    for child in widget.findChildren(QWidget):
        walk(child.layout())
    return [l for l in seen if isinstance(l, QBoxLayout)]


def compact_panel(widget: QWidget, short_help: str | None = None, budget: int = PANEL_BUDGET) -> None:
    layout = widget.layout()
    if layout is not None and layout.count():
        first = layout.itemAt(0).widget()
        if isinstance(first, QLabel) and first.text().lstrip().startswith("<b>") and short_help:
            full = first.text()
            index = layout.indexOf(first)
            layout.takeAt(index)
            first.hide()
            first.deleteLater()
            short = QLabel(short_help)
            short.setWordWrap(True)
            layout.insertWidget(index, short)
            layout.insertWidget(index + 1, Foldout("How", full))
    for label in widget.findChildren(QLabel):
        label.setWordWrap(True)
    for combo in widget.findChildren(QComboBox):
        combo.setSizeAdjustPolicy(QComboBox.SizeAdjustPolicy.AdjustToMinimumContentsLengthWithIcon)
        combo.setMinimumContentsLength(8)
    # Rows, widest first, until the panel fits.
    for _ in range(3):
        rows = []
        for parent in _all_box_layouts(widget):
            for i in range(parent.count()):
                child = parent.itemAt(i).layout()
                if isinstance(child, QHBoxLayout) and _row_width(child) > budget:
                    rows.append((child, parent))
        if not rows:
            break
        for row, parent in rows:
            _reflow(row, parent, budget)


class PanelViewWheel(QObject):
    """For a small 3D view embedded in a scrolling tool panel: the mouse wheel
    scrolls the PANEL (the view no longer steals it to zoom); Ctrl+wheel still
    zooms the view. Click-drag rotates as usual."""

    def eventFilter(self, obj, event) -> bool:  # noqa: N802 (Qt override)
        if event.type() != QEvent.Type.Wheel:
            return False
        if event.modifiers() & Qt.KeyboardModifier.ControlModifier:
            return False  # Ctrl+wheel: zoom the view
        parent = obj.parent()
        while parent is not None and not isinstance(parent, QScrollArea):
            parent = parent.parent()
        if parent is not None:
            bar = parent.verticalScrollBar()
            bar.setValue(bar.value() - event.angleDelta().y())
        return True


def make_panel_view(plotter) -> None:
    """Orthographic, wheel-scrolls-the-panel setup for an embedded 3D view."""
    try:
        plotter.enable_parallel_projection()
    except Exception:  # noqa: BLE001
        pass
    wheel = PanelViewWheel(plotter.interactor)
    plotter.interactor.installEventFilter(wheel)
    plotter.interactor.setToolTip("Drag to rotate · Ctrl+wheel to zoom · wheel scrolls the panel")
    plotter._panel_wheel_filter = wheel  # keep it alive
    refit = PanelViewRefit(plotter)
    plotter.interactor.installEventFilter(refit)
    plotter._panel_refit_filter = refit


class PanelViewRefit(QObject):
    """A panel view that the layout resizes (the panel narrows, a square
    pair re-squares) re-centres and re-fits its model at the new size,
    keeping the angle it is seen from: framing done at the old size would
    leave the model too big and off to one corner."""

    def __init__(self, plotter) -> None:
        super().__init__(plotter.interactor)
        self.plotter = plotter

    def eventFilter(self, obj, event) -> bool:  # noqa: N802 (Qt override)
        if event.type() == QEvent.Type.Resize:
            from PySide6.QtCore import QTimer

            QTimer.singleShot(0, self._refit)  # after the render window has the new size
        return False

    def _refit(self) -> None:
        try:
            _centre_on_screen(self.plotter, 1.0)
            self.plotter.render()
        except Exception:  # noqa: BLE001 - a view being torn down
            pass


FRAME_MARGIN = 1.12  # box-fit framing: 12% space around the model


def frame_close(plotter, zoom: float = 1.1) -> None:
    """Isometric, orthographic, framed tight on the model and centred on
    what is drawn (the box corners as seen), not on the bounding sphere,
    which leaves a flat part sitting low in an isometric view."""
    plotter.view_isometric()
    plotter.reset_camera()
    try:
        _centre_on_screen(plotter, zoom)
    except Exception:  # noqa: BLE001 - keep the plain reset framing
        try:
            plotter.camera.zoom(zoom)
        except Exception:  # noqa: BLE001
            pass


def _centre_on_screen(plotter, zoom: float) -> None:
    import numpy as np

    renderer = plotter.renderer
    b = renderer.ComputeVisiblePropBounds()
    if b[0] > b[1]:
        return
    corners = np.array([[x, y, z] for x in b[0:2] for y in b[2:4] for z in b[4:6]])
    camera = renderer.GetActiveCamera()
    focal, position = np.array(camera.GetFocalPoint()), np.array(camera.GetPosition())
    forward = focal - position
    forward /= np.linalg.norm(forward)
    up = np.array(camera.GetViewUp())
    up = up - forward * (up @ forward)
    up /= np.linalg.norm(up)
    right = np.cross(forward, up)
    across, high = (corners - focal) @ right, (corners - focal) @ up
    shift = right * (across.max() + across.min()) / 2 + up * (high.max() + high.min()) / 2
    camera.SetFocalPoint(*(focal + shift))
    camera.SetPosition(*(position + shift))
    width, height = renderer.GetSize()
    aspect = width / height if height else 1.0
    half = max(np.ptp(high) / 2, np.ptp(across) / 2 / max(aspect, 1e-6))
    if camera.GetParallelProjection():
        camera.SetParallelScale(half * FRAME_MARGIN)  # the box fits exactly: leave a margin all round
    else:
        camera.Zoom(zoom)
    renderer.ResetCameraClippingRange()
