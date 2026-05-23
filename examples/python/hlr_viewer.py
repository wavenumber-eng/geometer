from __future__ import annotations

import argparse
import io
import json
import math
import sys
from collections.abc import Sequence as SequenceABC
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

try:
    import numpy as np
except ModuleNotFoundError:
    np = None

try:
    import trimesh
except ModuleNotFoundError:
    trimesh = None

import geometer


DEFAULT_STEP = ROOT / "tests" / "fixtures" / "step" / "embedded_models" / "SOT-23.STEP"
MODEL_CANVAS_WIDTH = 560
PROJECTION_CANVAS_WIDTH = 680
CANVAS_HEIGHT = 680
MAX_PREVIEW_FACES = 4500
MODEL_CANVAS_TAG = "model_canvas"
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
OUTPUT_JSON_TAG = "output_json"
OUTPUT_GLB_TAG = "output_glb"
MODEL_YAW_TAG = "model_yaw"
MODEL_PITCH_TAG = "model_pitch"
MODEL_ZOOM_TAG = "model_zoom"
GUI_CONTEXT_READY = False


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
    model: "MeshPreview | None" = None


@dataclass
class MeshPreview:
    vertices: Any
    faces: Any
    source_face_count: int


STATE = ViewerState()


def main() -> int:
    parser = argparse.ArgumentParser(description="Geometer HLR Preview")
    parser.add_argument("step", nargs="?", default=str(DEFAULT_STEP), help="STEP/STP file to load")
    parser.add_argument("--project-once", action="store_true", help="run one projection without opening the GUI")
    parser.add_argument("--view", choices=sorted(VIEW_PRESETS), default="top", help="projection view for --project-once")
    parser.add_argument("--mode", choices=["detail", "simple", "both"], default="detail", help="geometry mode for --project-once")
    parser.add_argument("--json-out", type=Path, help="write projection JSON")
    parser.add_argument("--glb-out", type=Path, help="write STEP preview GLB")
    args = parser.parse_args()

    if args.project_once:
        result = project(Path(args.step), view_name=args.view)
        write_optional_outputs(result, Path(args.step), args.json_out, args.glb_out)
        geometries = selected_geometries(result, args.view, args.mode)
        print(projection_summary(Path(args.step), result, args.view, geometries))
        return 0

    if dpg is None:
        print("Dear PyGui is required for the viewer: python -m pip install dearpygui", file=sys.stderr)
        return 2

    start_viewer(Path(args.step))
    return 0


def start_viewer(initial_step: Path) -> None:
    global GUI_CONTEXT_READY

    dpg.create_context()
    GUI_CONTEXT_READY = True
    dpg.create_viewport(title="Geometer HLR Preview", width=1400, height=860)

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

    with dpg.window(label="Geometer HLR Preview", tag="main_window"):
        dpg.add_text(version_label_text(), color=(245, 246, 248, 255))

        with dpg.group(horizontal=True):
            dpg.add_input_text(tag=PATH_TAG, default_value=str(initial_step), width=880)
            dpg.add_button(label="Open", callback=lambda: dpg.show_item("step_file_dialog"), width=80)

        with dpg.group(horizontal=True):
            dpg.add_combo(list(VIEW_PRESETS), tag=VIEW_TAG, default_value="top", width=120)
            dpg.add_combo(["detail", "simple", "both"], tag=MODE_TAG, default_value="detail", width=120)
            dpg.add_combo(["poly", "exact"], tag=ALGO_TAG, default_value="poly", width=120)
            dpg.add_combo(["polyline", "native_arcs"], tag=CURVE_TAG, default_value="polyline", width=140)
            dpg.add_input_float(tag=TX_TAG, label="X", default_value=0.0, width=110, step=0.1, format="%.3f")
            dpg.add_input_float(tag=TY_TAG, label="Y", default_value=0.0, width=110, step=0.1, format="%.3f")
            dpg.add_input_float(tag=TZ_TAG, label="Z", default_value=0.0, width=110, step=0.1, format="%.3f")

        with dpg.group(horizontal=True):
            dpg.add_input_text(tag=OUTPUT_JSON_TAG, hint="projection JSON path", width=520)
            dpg.add_button(label="Save JSON", callback=on_save_json_clicked, width=95)
            dpg.add_input_text(tag=OUTPUT_GLB_TAG, hint="preview GLB path", width=420)
            dpg.add_button(label="Save GLB", callback=on_save_glb_clicked, width=90)

        with dpg.group(horizontal=True):
            dpg.add_slider_float(
                tag=MODEL_YAW_TAG,
                label="Yaw",
                default_value=-35.0,
                min_value=-180.0,
                max_value=180.0,
                width=180,
                callback=on_model_view_changed,
            )
            dpg.add_slider_float(
                tag=MODEL_PITCH_TAG,
                label="Pitch",
                default_value=18.0,
                min_value=-80.0,
                max_value=80.0,
                width=180,
                callback=on_model_view_changed,
            )
            dpg.add_slider_float(
                tag=MODEL_ZOOM_TAG,
                label="Zoom",
                default_value=1.0,
                min_value=0.35,
                max_value=2.5,
                width=180,
                callback=on_model_view_changed,
            )
            dpg.add_button(label="Reset 3D", callback=on_model_reset_clicked, width=90)

        dpg.add_text("", tag=STATUS_TAG)

        with dpg.group(horizontal=True):
            with dpg.group():
                dpg.add_text("3D preview")
                with dpg.drawlist(width=MODEL_CANVAS_WIDTH, height=CANVAS_HEIGHT, tag=MODEL_CANVAS_TAG):
                    pass
            with dpg.group():
                dpg.add_text("HLR projection")
                with dpg.drawlist(width=PROJECTION_CANVAS_WIDTH, height=CANVAS_HEIGHT, tag=CANVAS_TAG):
                    pass

    with dpg.handler_registry():
        dpg.add_mouse_drag_handler(button=dpg.mvMouseButton_Left, callback=on_model_drag)
        dpg.add_mouse_wheel_handler(callback=on_model_wheel)

    draw_model_empty("No model loaded")
    draw_empty("No projection loaded")
    dpg.setup_dearpygui()
    dpg.show_viewport()
    on_project_clicked()
    dpg.set_primary_window("main_window", True)
    dpg.start_dearpygui()
    GUI_CONTEXT_READY = False
    dpg.destroy_context()


def on_file_selected(_sender: int, app_data: Mapping[str, Any]) -> None:
    file_path = app_data.get("file_path_name")
    if isinstance(file_path, str):
        dpg.set_value(PATH_TAG, file_path)
        on_project_clicked()


def on_project_clicked() -> None:
    path = Path(dpg.get_value(PATH_TAG))
    set_status(f"Projecting {path.name} ...")
    preview_error: str | None = None
    try:
        STATE.model = load_model_preview(path)
        draw_model_preview()
    except Exception as exc:
        STATE.model = None
        preview_error = str(exc)
        draw_model_empty("3D preview unavailable")

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
    message = (
        f"{path.name} | detail {edge_count(detail)} | simple {edge_count(simple)} | "
        f"HLR {timings.get('hlr_ms', 0):.2f} ms | mesh {timings.get('mesh_ms', 0):.2f} ms"
    )
    if preview_error:
        message += f" | 3D preview skipped: {preview_error}"
    set_status(message)


def on_model_view_changed() -> None:
    draw_model_preview()


def on_model_reset_clicked() -> None:
    dpg.set_value(MODEL_YAW_TAG, -35.0)
    dpg.set_value(MODEL_PITCH_TAG, 18.0)
    dpg.set_value(MODEL_ZOOM_TAG, 1.0)
    draw_model_preview()


def on_model_drag(_sender: int, app_data: Any) -> None:
    if STATE.model is None or not dpg.is_item_hovered(MODEL_CANVAS_TAG):
        return
    dx, dy = drag_delta(app_data)
    if dx == 0.0 and dy == 0.0:
        return
    dpg.set_value(MODEL_YAW_TAG, float(dpg.get_value(MODEL_YAW_TAG)) + (dx * 0.35))
    dpg.set_value(MODEL_PITCH_TAG, clamp(float(dpg.get_value(MODEL_PITCH_TAG)) - (dy * 0.35), -80.0, 80.0))
    draw_model_preview()


def on_model_wheel(_sender: int, app_data: Any) -> None:
    if STATE.model is None or not dpg.is_item_hovered(MODEL_CANVAS_TAG):
        return
    delta = float(app_data if isinstance(app_data, (int, float)) else 0.0)
    zoom = float(dpg.get_value(MODEL_ZOOM_TAG)) * (1.0 + (delta * 0.08))
    dpg.set_value(MODEL_ZOOM_TAG, clamp(zoom, 0.35, 2.5))
    draw_model_preview()


def load_model_preview(path: Path) -> MeshPreview:
    if np is None or trimesh is None:
        raise RuntimeError("3D preview requires numpy and trimesh. Run with uv or install the example dependencies.")

    glb_bytes = geometer.step_to_glb(path)
    loaded = trimesh.load(io.BytesIO(glb_bytes), file_type="glb")
    if isinstance(loaded, trimesh.Scene):
        mesh = loaded.dump(concatenate=True)
    else:
        mesh = loaded
    if mesh is None or not hasattr(mesh, "vertices") or not hasattr(mesh, "faces"):
        raise RuntimeError("GLB preview did not contain a triangle mesh")

    vertices = np.asarray(mesh.vertices, dtype=np.float64)
    faces = np.asarray(mesh.faces, dtype=np.int32)
    if vertices.size == 0 or faces.size == 0:
        raise RuntimeError("GLB preview mesh is empty")

    center = (vertices.min(axis=0) + vertices.max(axis=0)) * 0.5
    radius = float(np.linalg.norm(vertices.max(axis=0) - vertices.min(axis=0)) * 0.5)
    if not math.isfinite(radius) or radius <= 0.0:
        radius = 1.0
    vertices = (vertices - center) / radius

    source_face_count = int(len(faces))
    if source_face_count > MAX_PREVIEW_FACES:
        step = max(1, math.ceil(source_face_count / MAX_PREVIEW_FACES))
        faces = faces[::step][:MAX_PREVIEW_FACES]

    return MeshPreview(vertices=vertices, faces=faces, source_face_count=source_face_count)


def draw_model_preview() -> None:
    if STATE.model is None:
        draw_model_empty("No model loaded")
        return
    if np is None:
        draw_model_empty("numpy is unavailable")
        return

    mesh = STATE.model
    clear_model_canvas()
    projected, _depth, face_order, shade = project_mesh(mesh)
    for face_index in face_order:
        face = mesh.faces[face_index]
        points = [tuple(projected[index]) for index in face]
        intensity = int(shade[face_index])
        fill = (intensity, intensity, intensity, 255)
        line = (72, 78, 84, 90)
        dpg.draw_triangle(points[0], points[1], points[2], color=line, fill=fill, parent=MODEL_CANVAS_TAG)

    drawn = len(mesh.faces)
    suffix = "" if mesh.source_face_count == drawn else f" / {mesh.source_face_count}"
    dpg.draw_text(
        (14, CANVAS_HEIGHT - 26),
        f"faces {drawn}{suffix}",
        color=(92, 101, 112, 255),
        size=13,
        parent=MODEL_CANVAS_TAG,
    )


def project_mesh(mesh: MeshPreview) -> tuple[Any, Any, Any, Any]:
    yaw = math.radians(float(current_value(MODEL_YAW_TAG, -35.0)))
    pitch = math.radians(float(current_value(MODEL_PITCH_TAG, 18.0)))
    zoom = float(current_value(MODEL_ZOOM_TAG, 1.0))

    cy, sy = math.cos(yaw), math.sin(yaw)
    cp, sp = math.cos(pitch), math.sin(pitch)
    rot_z = np.array([[cy, -sy, 0.0], [sy, cy, 0.0], [0.0, 0.0, 1.0]])
    rot_x = np.array([[1.0, 0.0, 0.0], [0.0, cp, -sp], [0.0, sp, cp]])
    camera = mesh.vertices @ (rot_x @ rot_z).T

    scale = min(MODEL_CANVAS_WIDTH, CANVAS_HEIGHT) * 0.42 * zoom
    projected = np.empty((len(camera), 2), dtype=np.float64)
    projected[:, 0] = (MODEL_CANVAS_WIDTH * 0.5) + (camera[:, 0] * scale)
    projected[:, 1] = (CANVAS_HEIGHT * 0.52) - (camera[:, 2] * scale)

    triangles = camera[mesh.faces]
    depth = triangles[:, :, 1].mean(axis=1)
    normals = np.cross(triangles[:, 1] - triangles[:, 0], triangles[:, 2] - triangles[:, 0])
    normal_lengths = np.linalg.norm(normals, axis=1)
    normal_lengths[normal_lengths == 0.0] = 1.0
    normals = normals / normal_lengths[:, None]
    light = np.array([-0.35, -0.55, 0.76], dtype=np.float64)
    light = light / np.linalg.norm(light)
    diffuse = np.clip(np.abs(normals @ light), 0.0, 1.0)
    shade = 92 + (diffuse * 118)
    face_order = np.argsort(depth)
    return projected, depth, face_order, shade


def clear_model_canvas() -> None:
    dpg.delete_item(MODEL_CANVAS_TAG, children_only=True)
    dpg.draw_rectangle(
        (0, 0),
        (MODEL_CANVAS_WIDTH, CANVAS_HEIGHT),
        color=(219, 224, 230, 255),
        fill=(250, 251, 252, 255),
        parent=MODEL_CANVAS_TAG,
    )


def draw_model_empty(message: str) -> None:
    clear_model_canvas()
    dpg.draw_text((24, 28), message, color=(92, 101, 112, 255), size=16, parent=MODEL_CANVAS_TAG)


def drag_delta(app_data: Any) -> tuple[float, float]:
    if isinstance(app_data, SequenceABC) and not isinstance(app_data, (str, bytes)):
        values = list(app_data)
        if len(values) >= 3:
            return float(values[1]), float(values[2])
        if len(values) >= 2:
            return float(values[0]), float(values[1])
    return 0.0, 0.0


def on_save_json_clicked() -> None:
    if STATE.projection is None:
        set_status("Project a STEP file before saving JSON.")
        return
    output = output_path(OUTPUT_JSON_TAG, ".projection.json")
    write_json(output, STATE.projection)
    set_status(f"Wrote {output}")


def on_save_glb_clicked() -> None:
    path = Path(dpg.get_value(PATH_TAG))
    output = output_path(OUTPUT_GLB_TAG, ".glb")
    ensure_parent(output)
    output.write_bytes(geometer.step_to_glb(path))
    set_status(f"Wrote {output}")


def project(path: Path, *, view_name: str | None = None) -> geometer.HlrProjectionResult:
    view = selected_view(view_name)
    options = geometer.HlrOptions.assembly_outline()
    options.projection_algorithm = current_value(ALGO_TAG, "poly")
    options.curve_mode = current_value(CURVE_TAG, "polyline")
    return geometer.project_step_hlr(
        path,
        views=[view],
        model_transform=model_transform(),
        options=options,
    )


def selected_view(name: str | None = None) -> geometer.ProjectionView:
    if name is None:
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
    if dpg is None or not GUI_CONTEXT_READY or not dpg.does_item_exist(tag):
        return fallback
    return dpg.get_value(tag)


def draw_projection(result: geometer.HlrProjectionResult, view_id: str, mode: str) -> None:
    clear_canvas()
    geometries = selected_geometries(result, view_id, mode)

    bounds = combined_bounds(geometry for _name, geometry, _color in geometries)
    if not bounds.valid:
        draw_empty("No projected geometry")
        return

    projector = screen_projector(bounds)
    for _name, geometry, color in geometries:
        draw_geometry(geometry, color, projector)


def clear_canvas() -> None:
    dpg.delete_item(CANVAS_TAG, children_only=True)
    dpg.draw_rectangle(
        (0, 0),
        (PROJECTION_CANVAS_WIDTH, CANVAS_HEIGHT),
        color=(34, 38, 43, 255),
        fill=(18, 21, 24, 255),
        parent=CANVAS_TAG,
    )


def draw_empty(message: str) -> None:
    clear_canvas()
    dpg.draw_text((28, 28), message, color=(185, 190, 196, 255), size=18, parent=CANVAS_TAG)


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
    scale = min((PROJECTION_CANVAS_WIDTH - (2 * margin)) / bounds.width, (CANVAS_HEIGHT - (2 * margin)) / bounds.height)
    content_width = bounds.width * scale
    content_height = bounds.height * scale
    offset_x = (PROJECTION_CANVAS_WIDTH - content_width) * 0.5
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


def edge_count(geometry: Mapping[str, Any]) -> int:
    return len(geometry.get("segments", [])) + len(geometry.get("arcs", []))


def version_label_text() -> str:
    version = geometer.version()
    return f"Geometer {version.string} | ABI {version.abi}"


def selected_geometries(
    result: geometer.HlrProjectionResult,
    view_id: str,
    mode: str,
) -> list[tuple[str, Mapping[str, Any], tuple[int, int, int, int]]]:
    geometries: list[tuple[str, Mapping[str, Any], tuple[int, int, int, int]]] = []
    if mode in {"simple", "both"}:
        geometries.append(("simple", result.geometry(view_id, "simple"), (80, 170, 220, 170)))
    if mode in {"detail", "both"}:
        geometries.append(("detail", result.geometry(view_id, "detail"), (230, 232, 235, 255)))
    return geometries


def projection_summary(
    path: Path,
    result: geometer.HlrProjectionResult,
    view_id: str,
    geometries: Sequence[tuple[str, Mapping[str, Any], tuple[int, int, int, int]]],
) -> str:
    parts = [f"{result.schema}", f"file={path.name}", f"view={view_id}"]
    for name, geometry, _color in geometries:
        parts.append(f"{name}_edges={edge_count(geometry)}")
    timings = result.timings
    if timings:
        parts.append(f"hlr_ms={float(timings.get('hlr_ms', 0.0)):.2f}")
        parts.append(f"mesh_ms={float(timings.get('mesh_ms', 0.0)):.2f}")
    return " ".join(parts)


def output_path(tag: str, suffix: str) -> Path:
    raw = str(dpg.get_value(tag)).strip()
    if raw:
        return Path(raw)
    source = Path(dpg.get_value(PATH_TAG))
    return source.with_suffix(suffix)


def write_optional_outputs(
    result: geometer.HlrProjectionResult,
    step_path: Path,
    json_out: Path | None,
    glb_out: Path | None,
) -> None:
    if json_out is not None:
        write_json(json_out, result)
    if glb_out is not None:
        ensure_parent(glb_out)
        glb_out.write_bytes(geometer.step_to_glb(step_path))


def write_json(path: Path, result: geometer.HlrProjectionResult) -> None:
    ensure_parent(path)
    path.write_text(json.dumps(result.data, indent=2) + "\n", encoding="utf-8")


def ensure_parent(path: Path) -> None:
    if path.parent != Path("."):
        path.parent.mkdir(parents=True, exist_ok=True)


def clamp(value: float, min_value: float, max_value: float) -> float:
    return max(min_value, min(max_value, value))


def set_status(message: str) -> None:
    dpg.set_value(STATUS_TAG, message)


if __name__ == "__main__":
    raise SystemExit(main())
