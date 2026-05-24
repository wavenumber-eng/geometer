from __future__ import annotations

import argparse
import io
import math
import sys
from dataclasses import dataclass
from pathlib import Path
from typing import Any, Iterable, Mapping, Sequence

import numpy as np
import pyvista as pv
import trimesh
from PySide6.QtCore import QPointF, Qt, QTimer
from PySide6.QtGui import QAction, QColor, QPainter, QPen, QPolygonF
from PySide6.QtWidgets import (
    QApplication,
    QCheckBox,
    QComboBox,
    QDoubleSpinBox,
    QFileDialog,
    QHBoxLayout,
    QLabel,
    QMainWindow,
    QPushButton,
    QSizePolicy,
    QSplitter,
    QStatusBar,
    QVBoxLayout,
    QWidget,
)
from pyvistaqt import QtInteractor

import geometer


ROOT = Path(__file__).resolve().parents[2]
DEFAULT_STEP = ROOT / "tests" / "fixtures" / "step" / "embedded_models" / "SOT-23.STEP"
EDGE_FEATURE_ANGLE_DEG = 38.0
CAMERA_PADDING = 1.12


@dataclass(frozen=True)
class LightingSettings:
    key_azimuth_deg: float = -42.0
    key_elevation_deg: float = 38.0
    key_intensity: float = 0.88
    fill_intensity: float = 0.34
    head_intensity: float = 0.28
    ambient: float = 0.30
    contrast: float = 0.74


VIEW_FACTORIES = {
    "top": geometer.ProjectionView.top,
    "bottom": geometer.ProjectionView.bottom,
    "front": geometer.ProjectionView.front,
    "back": geometer.ProjectionView.back,
    "right": geometer.ProjectionView.right,
    "left": geometer.ProjectionView.left,
}


CAMERA_PRESETS = {
    "iso": ((0.72, 0.54, 1.0), (0.0, 0.0, 1.0)),
    "top": (VIEW_FACTORIES["top"]().direction, VIEW_FACTORIES["top"]().up),
    "bottom": (VIEW_FACTORIES["bottom"]().direction, VIEW_FACTORIES["bottom"]().up),
    "front": (VIEW_FACTORIES["front"]().direction, VIEW_FACTORIES["front"]().up),
    "back": (VIEW_FACTORIES["back"]().direction, VIEW_FACTORIES["back"]().up),
    "left": (VIEW_FACTORIES["left"]().direction, VIEW_FACTORIES["left"]().up),
    "right": (VIEW_FACTORIES["right"]().direction, VIEW_FACTORIES["right"]().up),
}


@dataclass(frozen=True)
class PreviewMesh:
    polydata: pv.PolyData
    color: tuple[float, float, float]
    opacity: float


@dataclass
class Bounds:
    min_x: float = math.inf
    min_y: float = math.inf
    max_x: float = -math.inf
    max_y: float = -math.inf

    def add(self, x: float, y: float) -> None:
        self.min_x = min(self.min_x, x)
        self.min_y = min(self.min_y, y)
        self.max_x = max(self.max_x, x)
        self.max_y = max(self.max_y, y)

    @property
    def valid(self) -> bool:
        return all(math.isfinite(value) for value in (self.min_x, self.min_y, self.max_x, self.max_y))

    @property
    def width(self) -> float:
        return max(self.max_x - self.min_x, 1.0e-9)

    @property
    def height(self) -> float:
        return max(self.max_y - self.min_y, 1.0e-9)


class ProjectionCanvas(QWidget):
    def __init__(self) -> None:
        super().__init__()
        self._result: geometer.HlrProjectionResult | None = None
        self._view_id = "top"
        self._mode = "detail"
        self.setMinimumSize(320, 260)

    def set_projection(self, result: geometer.HlrProjectionResult, view_id: str, mode: str) -> None:
        self._result = result
        self._view_id = view_id
        self._mode = mode
        self.update()

    def clear_projection(self) -> None:
        self._result = None
        self.update()

    def paintEvent(self, _event: Any) -> None:
        painter = QPainter(self)
        painter.setRenderHint(QPainter.RenderHint.Antialiasing, True)
        painter.fillRect(self.rect(), QColor("#ffffff"))

        if self._result is None:
            painter.setPen(QColor("#657080"))
            painter.drawText(24, 32, "No HLR projection loaded")
            return

        try:
            geometries = self._selected_geometries()
        except KeyError as exc:
            painter.setPen(QColor("#a33a3a"))
            painter.drawText(24, 32, f"Projection view missing: {exc}")
            return

        bounds = combined_bounds(geometry for _name, geometry, _color in geometries)
        if not bounds.valid:
            painter.setPen(QColor("#657080"))
            painter.drawText(24, 32, "No projected geometry")
            return

        project = screen_projector(bounds, self.width(), self.height())
        for _name, geometry, color in geometries:
            self._draw_geometry(painter, geometry, color, project)

        detail = self._result.geometry(self._view_id, "detail")
        simple = self._result.geometry(self._view_id, "simple")
        painter.setPen(QColor("#243044"))
        painter.drawText(
            16,
            self.height() - 18,
            f"{self._view_id} detail {edge_count(detail)} simple {edge_count(simple)}",
        )

    def _selected_geometries(self) -> list[tuple[str, Mapping[str, Any], QColor]]:
        result: list[tuple[str, Mapping[str, Any], QColor]] = []
        if self._result is None:
            return result
        if self._mode in {"simple", "both"}:
            result.append(("simple", self._result.geometry(self._view_id, "simple"), QColor(32, 128, 110, 210)))
        if self._mode in {"detail", "both"}:
            result.append(("detail", self._result.geometry(self._view_id, "detail"), QColor(42, 54, 72, 255)))
        return result

    def _draw_geometry(self, painter: QPainter, geometry: Mapping[str, Any], color: QColor, project: Any) -> None:
        painter.setPen(QPen(color, 1.5, Qt.PenStyle.SolidLine, Qt.PenCapStyle.RoundCap, Qt.PenJoinStyle.RoundJoin))
        for segment in geometry.get("segments", []):
            if len(segment) != 4:
                continue
            p0 = project(float(segment[0]), float(segment[1]))
            p1 = project(float(segment[2]), float(segment[3]))
            painter.drawLine(QPointF(*p0), QPointF(*p1))
        for arc in geometry.get("arcs", []):
            points = [QPointF(*project(x, y)) for x, y in arc_points(arc)]
            if len(points) >= 2:
                painter.drawPolyline(QPolygonF(points))


class PyVistaHlrViewer(QMainWindow):
    def __init__(self, step_path: Path) -> None:
        super().__init__()
        self.step_path = step_path
        self.preview_meshes: list[PreviewMesh] = []
        self.current_projection: geometer.HlrProjectionResult | None = None
        self._loading_camera = False

        self.setWindowTitle("Geometer HLR Preview")
        self.resize(980, 720)

        self.plotter = QtInteractor(self)
        self.plotter.interactor.setMinimumSize(320, 260)
        self.plotter.set_background("white")
        self.plotter.enable_anti_aliasing()
        self.plotter.enable_parallel_projection()
        configure_lighting(self.plotter)
        self.projection = ProjectionCanvas()

        self.path_label = QLabel(str(step_path))
        self.path_label.setMinimumWidth(80)
        self.path_label.setSizePolicy(QSizePolicy.Policy.Ignored, QSizePolicy.Policy.Preferred)
        self.path_label.setToolTip(str(step_path))
        self.version_label = QLabel(version_label_text())
        self.version_label.setStyleSheet("font-weight: 600; color: #243044;")
        self.mode_combo = QComboBox()
        self.mode_combo.addItems(["detail", "simple", "both"])
        self.feature_edges_check = QCheckBox("Feature edges")
        self.feature_edges_check.setChecked(True)
        self.key_azimuth = lighting_spin(-180.0, 180.0, LightingSettings.key_azimuth_deg, 5.0)
        self.key_elevation = lighting_spin(-80.0, 80.0, LightingSettings.key_elevation_deg, 5.0)
        self.key_intensity = lighting_spin(0.0, 1.0, LightingSettings.key_intensity, 0.05)
        self.fill_intensity = lighting_spin(0.0, 1.0, LightingSettings.fill_intensity, 0.05)
        self.head_intensity = lighting_spin(0.0, 1.0, LightingSettings.head_intensity, 0.05)
        self.ambient = lighting_spin(0.0, 1.0, LightingSettings.ambient, 0.05)
        self.contrast = lighting_spin(0.0, 1.0, LightingSettings.contrast, 0.05)

        open_action = QAction("Open STEP", self)
        open_action.triggered.connect(self.open_step)
        self.fit_button = QPushButton("Fit")
        self.fit_button.clicked.connect(self.fit_camera_to_current_direction)

        camera_controls = QWidget()
        camera_layout = QHBoxLayout(camera_controls)
        camera_layout.setContentsMargins(6, 6, 6, 0)
        camera_layout.setSpacing(4)
        camera_layout.addWidget(compact_button("Open", self.open_step, 58))
        camera_layout.addWidget(QLabel("Camera"))
        for view_id, label in [
            ("iso", "ISO"),
            ("top", "Top"),
            ("bottom", "Bottom"),
            ("front", "Front"),
            ("back", "Back"),
            ("left", "Left"),
            ("right", "Right"),
        ]:
            camera_layout.addWidget(compact_button(label, lambda value=view_id: self.apply_camera_preset(value), 58))
        self.fit_button.setMaximumWidth(58)
        camera_layout.addWidget(self.fit_button)
        camera_layout.addStretch(1)
        camera_layout.addWidget(self.version_label)

        option_controls = QWidget()
        option_layout = QHBoxLayout(option_controls)
        option_layout.setContentsMargins(6, 2, 6, 2)
        option_layout.setSpacing(6)
        option_layout.addWidget(QLabel("Mode"))
        self.mode_combo.setMaximumWidth(96)
        option_layout.addWidget(self.mode_combo)
        option_layout.addWidget(self.feature_edges_check)
        option_layout.addWidget(QLabel("File"))
        option_layout.addWidget(self.path_label, 1)

        lighting_controls = QWidget()
        lighting_layout = QVBoxLayout(lighting_controls)
        lighting_layout.setContentsMargins(6, 0, 6, 6)
        lighting_layout.setSpacing(1)
        lighting_top = QHBoxLayout()
        lighting_top.setSpacing(4)
        lighting_bottom = QHBoxLayout()
        lighting_bottom.setSpacing(4)
        lighting_top.addWidget(QLabel("Lighting"))
        for label, spin in [
            ("Az", self.key_azimuth),
            ("El", self.key_elevation),
            ("Key", self.key_intensity),
            ("Fill", self.fill_intensity),
        ]:
            lighting_top.addWidget(QLabel(label))
            lighting_top.addWidget(spin)
            spin.valueChanged.connect(self.on_lighting_changed)
        lighting_top.addStretch(1)
        lighting_bottom.addWidget(QLabel("Light/Mat"))
        for label, spin in [
            ("Head", self.head_intensity),
            ("Amb", self.ambient),
            ("Contrast", self.contrast),
        ]:
            lighting_bottom.addWidget(QLabel(label))
            lighting_bottom.addWidget(spin)
            spin.valueChanged.connect(self.on_lighting_changed)
        lighting_bottom.addStretch(1)
        lighting_layout.addLayout(lighting_top)
        lighting_layout.addLayout(lighting_bottom)

        splitter = QSplitter(Qt.Orientation.Horizontal)
        splitter.addWidget(self.plotter.interactor)
        splitter.addWidget(self.projection)
        splitter.setStretchFactor(0, 1)
        splitter.setStretchFactor(1, 1)

        central = QWidget()
        layout = QVBoxLayout(central)
        layout.setContentsMargins(0, 0, 0, 0)
        layout.addWidget(camera_controls)
        layout.addWidget(option_controls)
        layout.addWidget(lighting_controls)
        layout.addWidget(splitter, 1)
        self.setCentralWidget(central)

        self.status = QStatusBar()
        self.setStatusBar(self.status)
        self.addAction(open_action)

        self.camera_timer = QTimer(self)
        self.camera_timer.setSingleShot(True)
        self.camera_timer.setInterval(350)
        self.camera_timer.timeout.connect(self._project_camera_after_idle)

        self.mode_combo.currentTextChanged.connect(self.on_mode_changed)
        self.feature_edges_check.stateChanged.connect(lambda _state: self.redraw_model())
        self.plotter.camera.AddObserver("ModifiedEvent", self.on_camera_modified)

        self.load_step(step_path)

    def open_step(self) -> None:
        file_name, _filter = QFileDialog.getOpenFileName(
            self,
            "Open STEP",
            str(self.step_path.parent),
            "STEP files (*.step *.stp);;All files (*.*)",
        )
        if file_name:
            self.load_step(Path(file_name))

    def load_step(self, path: Path) -> None:
        self.step_path = path
        self.path_label.setText(str(path))
        self.path_label.setToolTip(str(path))
        self.status.showMessage(f"Loading {path.name}")
        QApplication.setOverrideCursor(Qt.CursorShape.WaitCursor)
        try:
            glb_bytes = geometer.step_to_glb(path)
            self.preview_meshes = load_preview_meshes(glb_bytes)
            self.redraw_model(reset_camera=True)
            self.project_current_view()
        except Exception as exc:
            self.preview_meshes = []
            self.current_projection = None
            self.projection.clear_projection()
            self.status.showMessage(f"Load failed: {exc}")
        finally:
            QApplication.restoreOverrideCursor()

    def redraw_model(self, *, reset_camera: bool = False) -> None:
        self._loading_camera = True
        try:
            self.plotter.clear()
            configure_lighting(self.plotter, self.lighting_settings())
            material = self.material_settings()
            for index, preview in enumerate(self.preview_meshes):
                self.plotter.add_mesh(
                    preview.polydata,
                    color=preview.color,
                    opacity=preview.opacity,
                    show_edges=False,
                    smooth_shading=True,
                    lighting=True,
                    ambient=material["ambient"],
                    diffuse=material["diffuse"],
                    specular=material["specular"],
                    specular_power=material["specular_power"],
                    roughness=material["roughness"],
                    metallic=0.0,
                    name=f"mesh-{index}",
                )
                if self.feature_edges_check.isChecked():
                    edges = preview.polydata.extract_feature_edges(
                        feature_edges=True,
                        boundary_edges=True,
                        non_manifold_edges=False,
                        manifold_edges=False,
                        feature_angle=EDGE_FEATURE_ANGLE_DEG,
                    )
                    if edges.n_cells:
                        self.plotter.add_mesh(edges, color="#253044", line_width=1.0, name=f"edges-{index}")
            self.plotter.add_axes()
            if reset_camera:
                self.snap_camera_to_view("iso")
            self.plotter.render()
        finally:
            self._loading_camera = False

    def on_lighting_changed(self, _value: float) -> None:
        if self.preview_meshes:
            self.redraw_model()

    def lighting_settings(self) -> LightingSettings:
        return LightingSettings(
            key_azimuth_deg=float(self.key_azimuth.value()),
            key_elevation_deg=float(self.key_elevation.value()),
            key_intensity=float(self.key_intensity.value()),
            fill_intensity=float(self.fill_intensity.value()),
            head_intensity=float(self.head_intensity.value()),
            ambient=float(self.ambient.value()),
            contrast=float(self.contrast.value()),
        )

    def material_settings(self) -> dict[str, float]:
        settings = self.lighting_settings()
        contrast = max(0.0, min(1.0, settings.contrast))
        ambient = max(0.0, min(1.0, settings.ambient))
        return {
            "ambient": ambient,
            "diffuse": 0.52 + (contrast * 0.42),
            "specular": 0.04 + (contrast * 0.22),
            "specular_power": 12.0 + (contrast * 28.0),
            "roughness": 0.82 - (contrast * 0.34),
        }

    def apply_camera_preset(self, view_id: str) -> None:
        self.snap_camera_to_view(view_id)
        self.project_current_view()

    def on_mode_changed(self, mode: str) -> None:
        if self.current_projection is not None:
            self.projection.set_projection(self.current_projection, self.current_view_id(), mode)

    def on_camera_modified(self, _obj: Any, _event: str) -> None:
        if self._loading_camera:
            return
        if self.preview_meshes:
            self.camera_timer.start()

    def _project_camera_after_idle(self) -> None:
        if self.preview_meshes:
            self.project_current_view()

    def project_current_view(self) -> None:
        view = self.current_projection_view()
        self.status.showMessage(f"Projecting {self.step_path.name} from {view.id}")
        QApplication.setOverrideCursor(Qt.CursorShape.WaitCursor)
        try:
            result = geometer.project_step_hlr(
                self.step_path,
                views=[view],
                options=geometer.HlrOptions.assembly_outline(),
            )
            self.current_projection = result
            self.projection.set_projection(result, view.id, self.mode_combo.currentText())
            detail = result.geometry(view.id, "detail")
            simple = result.geometry(view.id, "simple")
            timings = result.timings
            self.status.showMessage(
                f"{self.step_path.name} | {view.id} | detail {edge_count(detail)} | "
                f"simple {edge_count(simple)} | HLR {float(timings.get('hlr_ms', 0.0)):.2f} ms"
            )
        except Exception as exc:
            self.status.showMessage(f"Projection failed: {exc}")
        finally:
            QApplication.restoreOverrideCursor()

    def current_projection_view(self) -> geometer.ProjectionView:
        direction, up = self.camera_projection_vectors()
        return geometer.ProjectionView.camera(direction=direction, up=up, id="camera")

    def current_view_id(self) -> str:
        return "camera"

    def camera_projection_vectors(self) -> tuple[tuple[float, float, float], tuple[float, float, float]]:
        camera = self.plotter.camera
        position = np.array(camera.GetPosition(), dtype=np.float64)
        target = np.array(camera.GetFocalPoint(), dtype=np.float64)
        direction = position - target
        length = float(np.linalg.norm(direction))
        if not math.isfinite(length) or length <= 1.0e-9:
            direction = np.array([0.0, 0.0, 1.0], dtype=np.float64)
        else:
            direction /= length
        up = np.array(camera.GetViewUp(), dtype=np.float64)
        up_length = float(np.linalg.norm(up))
        if not math.isfinite(up_length) or up_length <= 1.0e-9:
            up = np.array([0.0, 1.0, 0.0], dtype=np.float64)
        else:
            up /= up_length
        return tuple(float(value) for value in direction), tuple(float(value) for value in up)

    def snap_camera_to_view(self, view_id: str) -> None:
        direction, up = CAMERA_PRESETS.get(view_id, CAMERA_PRESETS["iso"])
        self.set_camera_direction(np.array(direction, dtype=np.float64), np.array(up, dtype=np.float64))

    def fit_camera_to_current_direction(self) -> None:
        camera = self.plotter.camera
        position = np.array(camera.GetPosition(), dtype=np.float64)
        target = np.array(camera.GetFocalPoint(), dtype=np.float64)
        direction = position - target
        if float(np.linalg.norm(direction)) <= 1.0e-9:
            direction = np.array([0.0, 0.0, 1.0], dtype=np.float64)
        up = np.array(camera.GetViewUp(), dtype=np.float64)
        self.set_camera_direction(direction, up)

    def set_camera_direction(self, direction: np.ndarray, up: np.ndarray) -> None:
        bounds = self.preview_bounds()
        self._loading_camera = True
        try:
            apply_camera_to_plotter(self.plotter, bounds, direction, up, self.plotter_aspect())
        finally:
            self._loading_camera = False

    def preview_bounds(self) -> tuple[float, float, float, float, float, float]:
        return combined_preview_bounds(self.preview_meshes)

    def plotter_aspect(self) -> float:
        size = self.plotter.interactor.size()
        height = max(float(size.height()), 1.0)
        return max(float(size.width()) / height, 0.01)


def combined_preview_bounds(preview_meshes: Sequence[PreviewMesh]) -> tuple[float, float, float, float, float, float]:
    if not preview_meshes:
        return (-0.5, 0.5, -0.5, 0.5, -0.5, 0.5)
    bounds = np.array([math.inf, -math.inf, math.inf, -math.inf, math.inf, -math.inf], dtype=np.float64)
    for preview in preview_meshes:
        current = preview.polydata.bounds
        bounds[0] = min(bounds[0], current[0])
        bounds[1] = max(bounds[1], current[1])
        bounds[2] = min(bounds[2], current[2])
        bounds[3] = max(bounds[3], current[3])
        bounds[4] = min(bounds[4], current[4])
        bounds[5] = max(bounds[5], current[5])
    if not np.all(np.isfinite(bounds)):
        return (-0.5, 0.5, -0.5, 0.5, -0.5, 0.5)
    return tuple(float(value) for value in bounds)


def load_preview_meshes(glb_bytes: bytes) -> list[PreviewMesh]:
    loaded = trimesh.load(io.BytesIO(glb_bytes), file_type="glb")
    if isinstance(loaded, trimesh.Scene):
        meshes = loaded.dump(concatenate=False)
    else:
        meshes = [loaded]

    result: list[PreviewMesh] = []
    for mesh in meshes:
        if mesh is None or len(mesh.vertices) == 0 or len(mesh.faces) == 0:
            continue
        vertices = np.asarray(mesh.vertices, dtype=np.float64)
        faces = np.asarray(mesh.faces, dtype=np.int64)
        if faces.ndim != 2 or faces.shape[1] != 3:
            continue
        face_data = np.column_stack([np.full(len(faces), 3, dtype=np.int64), faces]).ravel()
        polydata = pv.PolyData(vertices, face_data)
        color, opacity = mesh_color(mesh)
        result.append(PreviewMesh(polydata=polydata, color=color, opacity=opacity))

    if not result:
        raise RuntimeError("GLB did not contain any triangle meshes")
    return result


def mesh_color(mesh: Any) -> tuple[tuple[float, float, float], float]:
    material = getattr(getattr(mesh, "visual", None), "material", None)
    color = getattr(material, "main_color", None)
    if color is None:
        color = getattr(material, "baseColorFactor", None)
    if color is None:
        return (0.55, 0.55, 0.55), 1.0
    values = np.asarray(color, dtype=np.float64).flatten()
    if values.size < 3:
        return (0.55, 0.55, 0.55), 1.0
    if values.max(initial=1.0) > 1.0:
        values = values / 255.0
    opacity = float(values[3]) if values.size >= 4 else 1.0
    return (float(values[0]), float(values[1]), float(values[2])), opacity


def combined_bounds(geometries: Iterable[Mapping[str, Any]]) -> Bounds:
    bounds = Bounds()
    for geometry in geometries:
        include_geometry(bounds, geometry)
    return bounds


def include_geometry(bounds: Bounds, geometry: Mapping[str, Any]) -> None:
    for segment in geometry.get("segments", []):
        if len(segment) == 4:
            bounds.add(float(segment[0]), float(segment[1]))
            bounds.add(float(segment[2]), float(segment[3]))
    for arc in geometry.get("arcs", []):
        for x, y in arc_points(arc):
            bounds.add(x, y)


def screen_projector(bounds: Bounds, width: int, height: int) -> Any:
    margin = 42.0
    scale = min((width - (2 * margin)) / bounds.width, (height - (2 * margin)) / bounds.height)
    content_width = bounds.width * scale
    content_height = bounds.height * scale
    offset_x = (width - content_width) * 0.5
    offset_y = (height - content_height) * 0.5

    def project(x: float, y: float) -> tuple[float, float]:
        sx = offset_x + ((x - bounds.min_x) * scale)
        sy = height - (offset_y + ((y - bounds.min_y) * scale))
        return sx, sy

    return project


def arc_points(arc: Mapping[str, Any], samples: int = 48) -> list[tuple[float, float]]:
    center = arc.get("center", [0.0, 0.0])
    start = arc.get("start", [0.0, 0.0])
    radius = float(arc.get("radius", 0.0))
    if radius <= 0.0:
        return []
    cx = float(center[0])
    cy = float(center[1])
    if arc.get("full_circle"):
        return [
            (cx + math.cos((2.0 * math.pi * i) / samples) * radius, cy + math.sin((2.0 * math.pi * i) / samples) * radius)
            for i in range(samples + 1)
        ]

    start_angle = math.atan2(float(start[1]) - cy, float(start[0]) - cx)
    extent = abs(float(arc.get("extent_rad", 0.0)))
    if not arc.get("ccw", True):
        extent = -extent
    return [
        (cx + math.cos(start_angle + ((extent * i) / samples)) * radius, cy + math.sin(start_angle + ((extent * i) / samples)) * radius)
        for i in range(samples + 1)
    ]


def edge_count(geometry: Mapping[str, Any]) -> int:
    return len(geometry.get("segments", [])) + len(geometry.get("arcs", []))


def version_label_text() -> str:
    version = geometer.version()
    return f"Geometer {version.string} | ABI {version.abi}"


def compact_button(text: str, callback: Any, width: int) -> QPushButton:
    button = QPushButton(text)
    button.setMinimumWidth(min(width, 52))
    button.setMaximumWidth(width)
    button.setSizePolicy(QSizePolicy.Policy.Fixed, QSizePolicy.Policy.Preferred)
    button.clicked.connect(lambda _checked=False: callback())
    return button


def lighting_spin(minimum: float, maximum: float, value: float, step: float) -> QDoubleSpinBox:
    spin = QDoubleSpinBox()
    spin.setRange(minimum, maximum)
    spin.setValue(value)
    spin.setSingleStep(step)
    spin.setDecimals(2)
    spin.setMinimumWidth(96)
    spin.setMaximumWidth(112)
    return spin

def configure_lighting(plotter: pv.Plotter, settings: LightingSettings | None = None) -> None:
    settings = settings or LightingSettings()
    try:
        plotter.remove_all_lights()
    except Exception:
        pass

    azimuth = math.radians(settings.key_azimuth_deg)
    elevation = math.radians(settings.key_elevation_deg)
    key = np.array(
        [
            math.cos(elevation) * math.cos(azimuth),
            math.cos(elevation) * math.sin(azimuth),
            math.sin(elevation),
        ],
        dtype=np.float64,
    )
    key /= max(float(np.linalg.norm(key)), 1.0e-9)
    fill = np.array([-key[0], -key[1], max(0.25, key[2] * 0.35)], dtype=np.float64)
    fill /= max(float(np.linalg.norm(fill)), 1.0e-9)

    plotter.add_light(pv.Light(light_type="headlight", intensity=settings.head_intensity))
    plotter.add_light(
        pv.Light(
            position=tuple(float(value * 10.0) for value in key),
            focal_point=(0.0, 0.0, 0.0),
            color="white",
            intensity=settings.key_intensity,
            light_type="scene light",
        )
    )
    plotter.add_light(
        pv.Light(
            position=tuple(float(value * 10.0) for value in fill),
            focal_point=(0.0, 0.0, 0.0),
            color="#d7ecff",
            intensity=settings.fill_intensity,
            light_type="scene light",
        )
    )


def apply_camera_to_plotter(
    plotter: pv.Plotter,
    bounds: tuple[float, float, float, float, float, float],
    direction: np.ndarray,
    up: np.ndarray,
    aspect: float,
) -> None:
    center, diagonal = bounds_center_and_diagonal(bounds)
    direction = np.array(direction, dtype=np.float64)
    direction /= max(float(np.linalg.norm(direction)), 1.0e-9)
    up = orthogonalized_up(up, direction)
    distance = max(diagonal * 2.0, 1.0e-6)
    position = center + (direction * distance)

    plotter.enable_parallel_projection()
    plotter.camera.SetPosition(*position)
    plotter.camera.SetFocalPoint(*center)
    plotter.camera.SetViewUp(*up)
    plotter.camera.SetParallelScale(camera_parallel_scale(bounds, direction, up, aspect))
    plotter.reset_camera_clipping_range()
    plotter.render()


def bounds_center_and_diagonal(bounds: tuple[float, float, float, float, float, float]) -> tuple[np.ndarray, float]:
    min_x, max_x, min_y, max_y, min_z, max_z = bounds
    center = np.array([(min_x + max_x) * 0.5, (min_y + max_y) * 0.5, (min_z + max_z) * 0.5], dtype=np.float64)
    diagonal = float(np.linalg.norm([max_x - min_x, max_y - min_y, max_z - min_z]))
    if not math.isfinite(diagonal) or diagonal <= 1.0e-9:
        diagonal = 1.0
    return center, diagonal


def orthogonalized_up(up: np.ndarray, direction: np.ndarray) -> np.ndarray:
    up = np.array(up, dtype=np.float64)
    up = up - (direction * float(np.dot(up, direction)))
    length = float(np.linalg.norm(up))
    if math.isfinite(length) and length > 1.0e-9:
        return up / length
    candidates = [
        np.array([0.0, 1.0, 0.0], dtype=np.float64),
        np.array([0.0, 0.0, 1.0], dtype=np.float64),
        np.array([1.0, 0.0, 0.0], dtype=np.float64),
    ]
    fallback = min(candidates, key=lambda item: abs(float(np.dot(item, direction))))
    fallback = fallback - (direction * float(np.dot(fallback, direction)))
    return fallback / max(float(np.linalg.norm(fallback)), 1.0e-9)


def camera_parallel_scale(
    bounds: tuple[float, float, float, float, float, float],
    direction: np.ndarray,
    up: np.ndarray,
    aspect: float,
) -> float:
    min_x, max_x, min_y, max_y, min_z, max_z = bounds
    corners = np.array(
        [
            [x, y, z]
            for x in (min_x, max_x)
            for y in (min_y, max_y)
            for z in (min_z, max_z)
        ],
        dtype=np.float64,
    )
    right = np.cross(up, direction)
    right /= max(float(np.linalg.norm(right)), 1.0e-9)
    vertical_extent = float(np.ptp(corners @ up))
    horizontal_extent = float(np.ptp(corners @ right))
    _center, diagonal = bounds_center_and_diagonal(bounds)
    scale = max(vertical_extent * 0.5, horizontal_extent / (2.0 * max(aspect, 0.01)), diagonal * 0.05)
    return max(scale * CAMERA_PADDING, 1.0e-6)


def main() -> int:
    parser = argparse.ArgumentParser(description="Geometer HLR Preview")
    parser.add_argument("step", nargs="?", default=str(DEFAULT_STEP), help="STEP/STP file to load")
    parser.add_argument("--off-screen-validate", action="store_true", help="load the STEP and write an off-screen screenshot")
    parser.add_argument("--screenshot", type=Path, default=ROOT / "out" / "pyvista-preview.png")
    args = parser.parse_args()

    if args.off_screen_validate:
        glb_bytes = geometer.step_to_glb(Path(args.step))
        meshes = load_preview_meshes(glb_bytes)
        plotter = pv.Plotter(off_screen=True, window_size=(900, 650))
        plotter.set_background("white")
        plotter.enable_parallel_projection()
        configure_lighting(plotter)
        for index, mesh in enumerate(meshes):
            plotter.add_mesh(
                mesh.polydata,
                color=mesh.color,
                opacity=mesh.opacity,
                show_edges=False,
                smooth_shading=True,
                lighting=True,
                ambient=0.34,
                diffuse=0.78,
                specular=0.18,
                specular_power=24,
                name=f"mesh-{index}",
            )
        direction, up = CAMERA_PRESETS["iso"]
        apply_camera_to_plotter(
            plotter,
            combined_preview_bounds(meshes),
            np.array(direction, dtype=np.float64),
            np.array(up, dtype=np.float64),
            900.0 / 650.0,
        )
        args.screenshot.parent.mkdir(parents=True, exist_ok=True)
        plotter.screenshot(str(args.screenshot))
        print(f"meshes={len(meshes)} screenshot={args.screenshot}")
        return 0

    app = QApplication.instance() or QApplication(sys.argv[:1])
    viewer = PyVistaHlrViewer(Path(args.step))
    viewer.show()
    return int(app.exec())


if __name__ == "__main__":
    raise SystemExit(main())
