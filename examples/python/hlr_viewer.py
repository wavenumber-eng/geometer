from __future__ import annotations

import argparse
import math
import sys
from dataclasses import dataclass
from pathlib import Path
from typing import Any, Iterable, Mapping, Sequence


ROOT = Path(__file__).resolve().parents[2]
PYTHON_DIR = ROOT / "python"
if PYTHON_DIR.exists():
    sys.path.insert(0, str(PYTHON_DIR))

try:
    import dearpygui.dearpygui as dpg
except ModuleNotFoundError:
    dpg = None

import geometer


DEFAULT_STEP = ROOT / "tests" / "fixtures" / "step" / "embedded_models" / "SOT-23.STEP"
CANVAS_WIDTH = 1120
CANVAS_HEIGHT = 720
CANVAS_TAG = "hlr_canvas"
STATUS_TAG = "status_text"
PATH_TAG = "step_path"
VIEW_TAG = "view_name"
MODE_TAG = "mode_name"
ALGO_TAG = "algorithm_name"
CURVE_TAG = "curve_mode"
TX_TAG = "transform_x"
TY_TAG = "transform_y"
TZ_TAG = "transform_z"


VIEW_PRESETS = {
    "top": geometer.ProjectionView.top,
    "bottom": geometer.ProjectionView.bottom,
    "front": geometer.ProjectionView.front,
    "back": geometer.ProjectionView.back,
    "right": geometer.ProjectionView.right,
    "left": geometer.ProjectionView.left,
}


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


@dataclass
class ViewerState:
    projection: geometer.HlrProjectionResult | None = None
    view_id: str = "top"


STATE = ViewerState()


def main() -> int:
    parser = argparse.ArgumentParser(description="Preview Geometer STEP HLR output.")
    parser.add_argument("step", nargs="?", default=str(DEFAULT_STEP), help="STEP/STP file to load")
    parser.add_argument("--project-once", action="store_true", help="run one projection without opening the GUI")
    args = parser.parse_args()

    if args.project_once:
        result = project(Path(args.step))
        detail = result.geometry(result.views[0]["id"], "detail")
        print(f"{result.schema} detail_edges={len(detail['segments']) + len(detail['arcs'])}")
        return 0

    if dpg is None:
        print("Dear PyGui is required for the viewer: python -m pip install dearpygui", file=sys.stderr)
        return 2

    start_viewer(Path(args.step))
    return 0


def start_viewer(initial_step: Path) -> None:
    dpg.create_context()
    dpg.create_viewport(title="Geometer HLR Viewer", width=1280, height=860)

    with dpg.file_dialog(
        directory_selector=False,
        show=False,
        callback=on_file_selected,
        tag="step_file_dialog",
        width=720,
        height=460,
    ):
        dpg.add_file_extension(".step", color=(80, 180, 255, 255))
        dpg.add_file_extension(".stp", color=(80, 180, 255, 255))
        dpg.add_file_extension(".*")

    with dpg.window(label="Geometer HLR Viewer", tag="main_window"):
        with dpg.group(horizontal=True):
            dpg.add_input_text(tag=PATH_TAG, default_value=str(initial_step), width=780)
            dpg.add_button(label="Open", callback=lambda: dpg.show_item("step_file_dialog"), width=80)
            dpg.add_button(label="Project", callback=on_project_clicked, width=90)

        with dpg.group(horizontal=True):
            dpg.add_combo(list(VIEW_PRESETS), tag=VIEW_TAG, default_value="top", width=120)
            dpg.add_combo(["detail", "simple", "both"], tag=MODE_TAG, default_value="detail", width=120)
            dpg.add_combo(["poly", "exact"], tag=ALGO_TAG, default_value="poly", width=120)
            dpg.add_combo(["polyline", "native_arcs"], tag=CURVE_TAG, default_value="polyline", width=140)
            dpg.add_input_float(tag=TX_TAG, label="X", default_value=0.0, width=110, step=0.1, format="%.3f")
            dpg.add_input_float(tag=TY_TAG, label="Y", default_value=0.0, width=110, step=0.1, format="%.3f")
            dpg.add_input_float(tag=TZ_TAG, label="Z", default_value=0.0, width=110, step=0.1, format="%.3f")

        dpg.add_text("", tag=STATUS_TAG)
        with dpg.drawlist(width=CANVAS_WIDTH, height=CANVAS_HEIGHT, tag=CANVAS_TAG):
            pass

    draw_empty("No projection loaded")
    dpg.setup_dearpygui()
    dpg.show_viewport()
    dpg.set_primary_window("main_window", True)
    dpg.start_dearpygui()
    dpg.destroy_context()


def on_file_selected(_sender: int, app_data: Mapping[str, Any]) -> None:
    file_path = app_data.get("file_path_name")
    if isinstance(file_path, str):
        dpg.set_value(PATH_TAG, file_path)
        on_project_clicked()


def on_project_clicked() -> None:
    path = Path(dpg.get_value(PATH_TAG))
    set_status(f"Projecting {path.name} ...")
    try:
        result = project(path)
    except Exception as exc:
        STATE.projection = None
        set_status(str(exc))
        draw_empty("Projection failed")
        return

    STATE.projection = result
    STATE.view_id = selected_view().id
    draw_projection(result, STATE.view_id, dpg.get_value(MODE_TAG))
    detail = result.geometry(STATE.view_id, "detail")
    simple = result.geometry(STATE.view_id, "simple")
    timings = result.timings
    set_status(
        f"{path.name} | detail {edge_count(detail)} | simple {edge_count(simple)} | "
        f"HLR {timings.get('hlr_ms', 0):.2f} ms | mesh {timings.get('mesh_ms', 0):.2f} ms"
    )


def project(path: Path) -> geometer.HlrProjectionResult:
    view = selected_view()
    options = geometer.HlrOptions.assembly_outline()
    options.projection_algorithm = current_value(ALGO_TAG, "poly")
    options.curve_mode = current_value(CURVE_TAG, "polyline")
    return geometer.project_step_hlr(
        path,
        views=[view],
        model_transform=model_transform(),
        options=options,
    )


def selected_view() -> geometer.ProjectionView:
    name = current_value(VIEW_TAG, "top")
    return VIEW_PRESETS.get(name, geometer.ProjectionView.top)()


def model_transform() -> list[list[float]]:
    tx = float(current_value(TX_TAG, 0.0))
    ty = float(current_value(TY_TAG, 0.0))
    tz = float(current_value(TZ_TAG, 0.0))
    return [
        [1.0, 0.0, 0.0, tx],
        [0.0, 1.0, 0.0, ty],
        [0.0, 0.0, 1.0, tz],
        [0.0, 0.0, 0.0, 1.0],
    ]


def current_value(tag: str, fallback: Any) -> Any:
    if dpg is None or not dpg.does_item_exist(tag):
        return fallback
    return dpg.get_value(tag)


def draw_projection(result: geometer.HlrProjectionResult, view_id: str, mode: str) -> None:
    clear_canvas()
    geometries: list[tuple[Mapping[str, Any], tuple[int, int, int, int]]] = []
    if mode in {"simple", "both"}:
        geometries.append((result.geometry(view_id, "simple"), (80, 170, 220, 170)))
    if mode in {"detail", "both"}:
        geometries.append((result.geometry(view_id, "detail"), (230, 232, 235, 255)))

    bounds = combined_bounds(geometry for geometry, _color in geometries)
    if not bounds.valid:
        draw_empty("No projected geometry")
        return

    projector = screen_projector(bounds)
    draw_grid(projector, bounds)
    for geometry, color in geometries:
        draw_geometry(geometry, color, projector)


def clear_canvas() -> None:
    dpg.delete_item(CANVAS_TAG, children_only=True)
    dpg.draw_rectangle(
        (0, 0),
        (CANVAS_WIDTH, CANVAS_HEIGHT),
        color=(34, 38, 43, 255),
        fill=(18, 21, 24, 255),
        parent=CANVAS_TAG,
    )


def draw_empty(message: str) -> None:
    clear_canvas()
    dpg.draw_text((28, 28), message, color=(185, 190, 196, 255), size=18, parent=CANVAS_TAG)


def draw_grid(projector: Any, bounds: Bounds) -> None:
    for x in nice_ticks(bounds.min_x, bounds.max_x):
        x0, y0 = projector(x, bounds.min_y)
        x1, y1 = projector(x, bounds.max_y)
        dpg.draw_line((x0, y0), (x1, y1), color=(54, 59, 65, 255), parent=CANVAS_TAG)
    for y in nice_ticks(bounds.min_y, bounds.max_y):
        x0, y0 = projector(bounds.min_x, y)
        x1, y1 = projector(bounds.max_x, y)
        dpg.draw_line((x0, y0), (x1, y1), color=(54, 59, 65, 255), parent=CANVAS_TAG)


def draw_geometry(geometry: Mapping[str, Any], color: tuple[int, int, int, int], projector: Any) -> None:
    for segment in geometry.get("segments", []):
        if len(segment) != 4:
            continue
        x0, y0 = projector(float(segment[0]), float(segment[1]))
        x1, y1 = projector(float(segment[2]), float(segment[3]))
        dpg.draw_line((x0, y0), (x1, y1), color=color, thickness=1.4, parent=CANVAS_TAG)

    for arc in geometry.get("arcs", []):
        points = [projector(x, y) for x, y in arc_points(arc)]
        if len(points) >= 2:
            dpg.draw_polyline(points, color=color, thickness=1.4, parent=CANVAS_TAG)


def screen_projector(bounds: Bounds) -> Any:
    margin = 42.0
    scale = min((CANVAS_WIDTH - (2 * margin)) / bounds.width, (CANVAS_HEIGHT - (2 * margin)) / bounds.height)
    content_width = bounds.width * scale
    content_height = bounds.height * scale
    offset_x = (CANVAS_WIDTH - content_width) * 0.5
    offset_y = (CANVAS_HEIGHT - content_height) * 0.5

    def project(x: float, y: float) -> tuple[float, float]:
        sx = offset_x + ((x - bounds.min_x) * scale)
        sy = CANVAS_HEIGHT - (offset_y + ((y - bounds.min_y) * scale))
        return sx, sy

    return project


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


def nice_ticks(min_value: float, max_value: float, count: int = 8) -> Sequence[float]:
    span = max(max_value - min_value, 1.0e-9)
    raw_step = span / max(count, 1)
    magnitude = 10.0 ** math.floor(math.log10(raw_step))
    normalized = raw_step / magnitude
    if normalized <= 1.0:
        step = magnitude
    elif normalized <= 2.0:
        step = 2.0 * magnitude
    elif normalized <= 5.0:
        step = 5.0 * magnitude
    else:
        step = 10.0 * magnitude
    first = math.ceil(min_value / step) * step
    ticks = []
    value = first
    while value <= max_value + (step * 0.5):
        ticks.append(value)
        value += step
    return ticks


def edge_count(geometry: Mapping[str, Any]) -> int:
    return len(geometry.get("segments", [])) + len(geometry.get("arcs", []))


def set_status(message: str) -> None:
    dpg.set_value(STATUS_TAG, message)


if __name__ == "__main__":
    raise SystemExit(main())
