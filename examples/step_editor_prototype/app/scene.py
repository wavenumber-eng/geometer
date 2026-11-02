"""SceneManager: owns everything in the PyVista plotter — body actors with
per-face STEP colors, click picking that resolves back to (body, B-rep face),
rubber-band selection, and highlight overlays. No OCP imports here; geometry
arrives as numpy from EditorDocument."""

from __future__ import annotations

from dataclasses import dataclass
from typing import Callable

import time

import numpy as np
import pyvista as pv
import vtk
from PySide6.QtCore import QEvent, QObject, QPoint, Qt

from .document import NEUTRAL_RGB, EditorDocument
from .style import VIEWPORT_BG
from .viewcam import (
    CAMERA_PRESETS,
    EDGE_FEATURE_ANGLE_DEG,
    LightingSettings,
    apply_camera_to_plotter,
    configure_lighting,
    material_settings,
)

FLASH_FILL = 0.35    # change flash / preview: semi-transparent green fill...
FLASH_EDGES = 0.75   # ...and its outline
FLASH_FADE = 1.0     # seconds the flash takes to fade out
FADE_TICK_S = 0.04


ZOOM_STEP = 1.15  # one wheel notch
HATCH_PLANES = 40  # selection stripes: one plane every model-size / 40
SLOW_FRAME_S = 0.25  # a 3D frame slower than this is logged
RENDER_CHECK_MS = 200  # how often an unrendered scene change is looked for


def _scene_stamp(plotter) -> int:
    """The newest modification time of anything drawn: the renderers, their
    cameras and every prop with its mapper and mapper input. Raw VTK calls
    only (through PyVista's wrappers this took 25-45 ms on a 578-body model,
    five times a second)."""
    stamp = 0
    renderers = plotter.render_window.GetRenderers()
    renderers.InitTraversal()
    for _ in range(renderers.GetNumberOfItems()):
        renderer = renderers.GetNextItem()
        stamp = max(stamp, renderer.GetMTime(), renderer.GetActiveCamera().GetMTime(), _props_stamp(renderer))
    return stamp


def _props_stamp(renderer) -> int:
    props = renderer.GetViewProps()
    props.InitTraversal()
    stamp = 0
    for _ in range(props.GetNumberOfItems()):
        stamp = max(stamp, _mapper_stamp(props.GetNextProp()))
    return stamp


def _mapper_stamp(prop) -> int:
    """A prop's own time with its mapper's and the mapper input's."""
    stamp = prop.GetMTime()
    mapper = prop.GetMapper() if hasattr(prop, "GetMapper") else None
    if mapper is None:
        return stamp
    data = mapper.GetInputDataObject(0, 0) if mapper.GetNumberOfInputPorts() else None
    return max(stamp, mapper.GetMTime(), data.GetMTime() if data is not None else 0)


LAND_OPACITY = 0.8
GHOST_OPACITY = 0.2   # ghost PCB board / ghost schematic symbol
BOARD_THICKNESS = 1.6  # mm, a standard FR-4 board under the ghost footprint  # footprint preview: 20% see-through, the part shows under it
CLICK_SLOP_PX = 4  # press→release movement beyond this is a drag, not a pick


def _nice_step(raw: float) -> float:
    """Round a raw spacing up to a 1/2/5 x 10^k grid step."""
    import math

    if raw <= 0.0:
        return 1.0
    exponent = math.floor(math.log10(raw))
    base = 10.0 ** exponent
    for multiplier in (1.0, 2.0, 5.0, 10.0):
        if multiplier * base >= raw:
            return multiplier * base
    return 10.0 * base


@dataclass(frozen=True)
class PickResult:
    body_index: int
    face_index: int          # 1-based index into the body's face map
    world_point: tuple[float, float, float]
    cell_id: int
    shift: bool = False      # Shift was held during the click (multi-select)


def _radii(pad: dict, which: str, grow: float) -> tuple[float, float]:
    """Half-sizes of a through-hole's pad or drill, grown by `grow`."""
    if which == "pad":
        return pad["w"] / 2 + grow, pad["h"] / 2 + grow
    drill = pad.get("drill") or [pad["w"], pad["h"]]
    drill = [drill, drill] if isinstance(drill, (int, float)) else drill
    return drill[0] / 2 + grow, drill[1] / 2 + grow


def _stadium(cx: float, cy: float, hx: float, hy: float, t: np.ndarray) -> np.ndarray:
    """Outline points of a slot (a stadium: two half-circles joined by
    straight sides) with half-sizes hx, hy; equal half-sizes give a circle.
    t: angles round the outline."""
    r = min(hx, hy)
    straight_x, straight_y = hx - r, hy - r
    return np.column_stack([cx + np.sign(np.cos(t)) * straight_x + r * np.cos(t),
                            cy + np.sign(np.sin(t)) * straight_y + r * np.sin(t)])


def _ring_mesh(pads: list, radii, z: float, steps: int = 40):
    """Flat rings (outer, inner half-sizes per pad; inner None = a disc)."""
    t = np.linspace(0.0, 2 * np.pi, steps, endpoint=False)
    points, faces = [], []
    for pad in pads:
        (ox, oy), inner = radii(pad)
        outer = _stadium(pad["x"], pad["y"], ox, oy, t)
        start = len(points)
        points += [[x, y, z] for x, y in outer]
        if inner is None:
            faces += [steps, *range(start, start + steps)]
            continue
        hole = _stadium(pad["x"], pad["y"], inner[0], inner[1], t)
        points += [[x, y, z] for x, y in hole]
        for k in range(steps):
            j = (k + 1) % steps
            faces += [4, start + k, start + j, start + steps + j, start + steps + k]
    return pv.PolyData(np.array(points), np.array(faces))


def _drill_cutters(pad: dict, thickness: float) -> list:
    """Solids to cut out of the board for one drill: a cylinder, or a slot's
    two round ends plus the box between them."""
    from OCP.BRepPrimAPI import BRepPrimAPI_MakeBox, BRepPrimAPI_MakeCylinder
    from OCP.gp import gp_Ax2, gp_Dir, gp_Pnt

    drill = pad["drill"]
    dx, dy = (drill, drill) if isinstance(drill, (int, float)) else drill
    r, half = min(dx, dy) / 2, max(dx, dy) / 2 - min(dx, dy) / 2
    along = (1.0, 0.0) if dx >= dy else (0.0, 1.0)
    ends = [-half, half] if half > 1e-6 else [0.0]
    cutters = [BRepPrimAPI_MakeCylinder(gp_Ax2(gp_Pnt(pad["x"] + along[0] * k, pad["y"] + along[1] * k,
                                                      -thickness - 0.1), gp_Dir(0, 0, 1)), r, thickness + 0.2).Shape()
               for k in ends]
    if half > 1e-6:
        w, h = (2 * half, 2 * r) if dx >= dy else (2 * r, 2 * half)
        cutters.append(BRepPrimAPI_MakeBox(gp_Pnt(pad["x"] - w / 2, pad["y"] - h / 2, -thickness - 0.1),
                                           w, h, thickness + 0.2).Shape())
    return cutters


_BOARD_CACHE: dict = {}  # (rect, drills, thickness) -> board mesh; re-entering Tool 9 reuses it


def _board_key(rect: tuple, pads: list[dict], thickness: float) -> tuple:
    drills = tuple(sorted((round(p["x"], 4), round(p["y"], 4), str(p["drill"])) for p in pads
                          if p.get("drill") and p.get("type") != "smd"))
    return tuple(round(v, 4) for v in rect), drills, thickness


def _board_with_holes(rect: tuple, pads: list[dict], thickness: float):
    """The ghost board as a mesh: a slab below Z=0 with every drill cut
    (round holes and oval slots), via OpenCascade, in ONE boolean with all the
    cutters as tools (cutting hole by hole took 7 s for a 294-pin DIMM socket).
    Cached: the same board is not rebuilt when Tool 9 is entered again."""
    key = _board_key(rect, pads, thickness)
    if key not in _BOARD_CACHE:
        if len(_BOARD_CACHE) > 8:
            _BOARD_CACHE.clear()
        _BOARD_CACHE[key] = _build_board(rect, pads, thickness)
    return _BOARD_CACHE[key]


def _build_board(rect: tuple, pads: list[dict], thickness: float):
    from OCP.BRepAlgoAPI import BRepAlgoAPI_Cut
    from OCP.BRepPrimAPI import BRepPrimAPI_MakeBox
    from OCP.gp import gp_Pnt
    from OCP.TopTools import TopTools_ListOfShape

    from .document import tessellate_body

    x0, x1, y0, y1 = rect
    slab = BRepPrimAPI_MakeBox(gp_Pnt(x0, y0, -thickness), x1 - x0, y1 - y0, thickness).Shape()
    tools = TopTools_ListOfShape()
    for pad in (p for p in pads if p.get("drill") and p.get("type") != "smd"):
        for cutter in _drill_cutters(pad, thickness):
            tools.Append(cutter)
    solid = slab
    if tools.Size():
        arguments = TopTools_ListOfShape()
        arguments.Append(slab)
        cut = BRepAlgoAPI_Cut()
        cut.SetArguments(arguments)
        cut.SetTools(tools)
        cut.SetRunParallel(True)
        cut.Build()
        solid = cut.Shape() if cut.IsDone() else slab
    mesh = tessellate_body(solid, max(x1 - x0, y1 - y0) / 400.0)
    if not len(mesh.tris):
        return None
    return pv.PolyData(mesh.points, np.column_stack([np.full(len(mesh.tris), 3), mesh.tris]).ravel())


def _barrel(pad: dict, thickness: float):
    """The copper lining of a plated hole, through the board."""
    hx, hy = _radii(pad, "drill", 0.0)
    if abs(hx - hy) < 1e-6:
        return pv.Cylinder(center=(pad["x"], pad["y"], -thickness / 2), direction=(0, 0, 1),
                           radius=hx, height=thickness, capping=False, resolution=32)
    # a plated slot: its stadium outline swept through the board
    outline = _stadium(pad["x"], pad["y"], hx, hy, np.linspace(0.0, 2 * np.pi, 48, endpoint=False))
    ring = pv.PolyData(np.column_stack([outline, np.zeros(len(outline))]),
                       lines=np.array([len(outline) + 1, *range(len(outline)), 0]))
    return ring.extrude((0.0, 0.0, -thickness), capping=False)


def _flat_label(label: tuple, height: float):
    """Text lying flat on the Z plane. label = (x, y, text, dx, dy): with a
    direction (a pin stub's, outward) the text starts just past (x, y), is
    centred on the stub's line and runs along it, turned to read left to
    right / bottom to top (so on left and bottom pins it ends at the tip);
    with dx = dy = 0 it is centred on (x, y) (a BGA ball)."""
    x, y, text, dx, dy = label
    flat = pv.Text3D(text, depth=0.0, height=height)
    b = flat.bounds
    width = b[1] - b[0]
    flat = flat.translate((-(b[0] + b[1]) / 2, -(b[2] + b[3]) / 2, 0.0), inplace=False)
    if dx == 0 and dy == 0:
        return flat.translate((x, y, 0.0), inplace=False)
    reading = (dx, dy) if (dx > 0.5 or dy > 0.5) else (-dx, -dy)
    flat = flat.rotate_z(float(np.degrees(np.arctan2(reading[1], reading[0]))), inplace=False)
    reach = 0.25 * height + width / 2
    return flat.translate((x + dx * reach, y + dy * reach, 0.0), inplace=False)


def _split_stubs(stubs: list, selected: set) -> tuple[list, list]:
    """(unselected, selected) pin stubs, skipping pins without one."""
    kept = [(i, st) for i, st in enumerate(stubs) if st is not None]
    return [st for i, st in kept if i not in selected], [st for i, st in kept if i in selected]


def _box_lines(pads: list[dict], margin: float) -> list:
    """The four sides of the box round all pads, grown by `margin`."""
    lo = np.array([[p["x"] - p["w"] / 2, p["y"] - p["h"] / 2] for p in pads]).min(axis=0) - margin
    hi = np.array([[p["x"] + p["w"] / 2, p["y"] + p["h"] / 2] for p in pads]).max(axis=0) + margin
    return [(lo[0], lo[1], hi[0], lo[1]), (hi[0], lo[1], hi[0], hi[1]),
            (hi[0], hi[1], lo[0], hi[1]), (lo[0], hi[1], lo[0], lo[1])]


class SceneManager:
    def __init__(self, plotter) -> None:
        self.plotter = plotter
        self.on_pick: Callable[[PickResult], None] | None = None
        self.on_miss: Callable[[], None] | None = None  # click on empty space
        self.on_right_miss: Callable[[], None] | None = None  # right-click on empty space
        self._right_press = None
        self._body_polydata: list[pv.PolyData] = []
        self._display_polydata: list = []  # the smooth-shaded copies the mappers render
        self._actor_to_body: dict[int, int] = {}  # id(vtkActor) -> body index
        self._actors: list = []
        self._band_actors: tuple | None = None
        self._band_poly = None
        self._press_position: tuple[int, int] | None = None
        self._hitbox_overlays: list[str] = []
        self._label_groups = 0
        self._pinface_groups = 0   # tinted pin-tip face overlays
        self._pinaxis_groups = 0   # through-body pin leader lines
        self._hover_callback: Callable[[PickResult | None], None] | None = None
        self._hover_observer = None
        self._picker = vtk.vtkCellPicker()
        self._picker.SetTolerance(0.0005)
        # Polygon offset keeps highlight overlays from z-fighting their bodies.
        vtk.vtkMapper.SetResolveCoincidentTopologyToPolygonOffset()

        plotter.set_background(VIEWPORT_BG)
        plotter.enable_anti_aliasing()
        plotter.enable_parallel_projection()
        configure_lighting(plotter)
        self._watch_frame_times(plotter)
        self._render_on_change(plotter)
        plotter.iren.add_observer("LeftButtonPressEvent", self._on_left_press)
        plotter.iren.add_observer("LeftButtonReleaseEvent", self._on_left_release)
        plotter.iren.add_observer("RightButtonPressEvent", self._on_right_press)
        plotter.iren.add_observer("RightButtonReleaseEvent", self._on_right_release)
        # The wheel zooms toward the point under the cursor (not the view
        # centre): an observer on the style replaces its own wheel zoom.
        style = plotter.iren.interactor.GetInteractorStyle()
        style.AddObserver("MouseWheelForwardEvent", lambda _o, _e: self.zoom_at_cursor(ZOOM_STEP))
        style.AddObserver("MouseWheelBackwardEvent", lambda _o, _e: self.zoom_at_cursor(1.0 / ZOOM_STEP))
        # VTK's right-drag dolly zoomed on every right-click (the hand drifts a
        # pixel): the right button is the editor's own (its handlers sit on the
        # interactor); zoom is the wheel only.
        style.AddObserver("RightButtonPressEvent", lambda _o, _e: None)
        style.AddObserver("RightButtonReleaseEvent", lambda _o, _e: None)

    def _render_on_change(self, plotter) -> None:
        """Replace pyvistaqt's auto-update (a full render + GPU present 5x a
        second, forever, even with nothing changing) with a check that renders
        only when something in the scene was modified and not yet drawn. An
        idle editor then presents no frames at all: the endless presents kept
        the NVIDIA overlay / multiplane-overlay compositor path busy and the
        whole Windows desktop could drop to ~13 fps."""
        from PySide6.QtCore import QTimer

        timer = getattr(plotter, "render_timer", None)
        if timer is not None:
            timer.stop()
        clock = vtk.vtkTimeStamp()
        drawn = {"stamp": -1}

        def rendered(_obj, _event) -> None:  # VTK's global modified counter: one cheap call
            clock.Modified()
            drawn["stamp"] = clock.GetMTime()

        plotter.render_window.AddObserver("EndEvent", rendered)
        self._change_timer = QTimer(plotter)
        self._change_timer.timeout.connect(lambda: self._check_unrendered(plotter, drawn))
        self._change_timer.start(RENDER_CHECK_MS)

    def _check_unrendered(self, plotter, drawn: dict) -> None:
        """Render when something changed after the last frame. The walk over
        every actor costs ~27 ms on a 585-body model, so the check spaces
        itself out to stay near 2% of the UI thread (200 ms on small parts,
        ~1.4 s on huge ones); explicit renders never wait for it."""
        if not plotter.isVisible():
            return
        start = time.perf_counter()
        changed = _scene_stamp(plotter) > drawn["stamp"]
        cost_ms = (time.perf_counter() - start) * 1000.0
        self._change_timer.setInterval(int(min(max(RENDER_CHECK_MS, cost_ms * 50), 2000)))
        if changed:
            plotter.render()

    def _watch_frame_times(self, plotter) -> None:
        """Log any 3D frame slower than SLOW_FRAME_S to editor_errors.log
        (at most once per 10 s), with what was being drawn: a stalling GPU
        frame can freeze the whole Windows desktop while the CPU sits idle."""
        import time

        window = plotter.render_window
        state = {"start": 0.0, "logged": 0.0}

        def started(_obj, _event) -> None:
            state["start"] = time.perf_counter()

        def ended(_obj, _event) -> None:
            took = time.perf_counter() - state["start"]
            if took < SLOW_FRAME_S or time.perf_counter() - state["logged"] < 10.0:
                return
            state["logged"] = time.perf_counter()
            from .app_window import log_error

            xray = getattr(self, "_xray_layer", None)
            log_error("slow 3D frame", f"{took:.2f} s; main actors {plotter.renderer.GetActors().GetNumberOfItems()}, "
                                       f"x-ray actors {xray.GetActors().GetNumberOfItems() if xray else 0}")

        window.AddObserver("StartEvent", started)
        window.AddObserver("EndEvent", ended)

    def zoom_at_cursor(self, factor: float) -> None:
        """Zoom by `factor` (> 1 in) keeping the point under the cursor where
        it is on screen, so a small feature can be zoomed straight into."""
        interactor = self.plotter.iren.interactor
        renderer = self.plotter.renderer
        camera = renderer.GetActiveCamera()
        x, y = interactor.GetEventPosition()
        focal = np.array(camera.GetFocalPoint())
        renderer.SetWorldPoint(*focal, 1.0)
        renderer.WorldToDisplay()
        depth = renderer.GetDisplayPoint()[2]
        renderer.SetDisplayPoint(x, y, depth)
        renderer.DisplayToWorld()
        world = np.array(renderer.GetWorldPoint())
        under = world[:3] / (world[3] or 1.0)
        if camera.GetParallelProjection():
            camera.SetParallelScale(camera.GetParallelScale() / factor)
        else:
            camera.Dolly(factor)
        shift = (under - focal) * (1.0 - 1.0 / factor)
        camera.SetFocalPoint(*(np.array(camera.GetFocalPoint()) + shift))
        camera.SetPosition(*(np.array(camera.GetPosition()) + shift))
        renderer.ResetCameraClippingRange()
        self.plotter.render()

    # -------------------------------------------------------------- building

    def rebuild(self, document: EditorDocument, *, show_feature_edges: bool = True) -> None:
        b = document.bounds()
        self._model_size = float(max(b[1] - b[0], b[3] - b[2], b[5] - b[4]))  # hatch spacing
        self.plotter.clear()
        configure_lighting(self.plotter, LightingSettings())
        material = material_settings()
        self._body_polydata = []
        self._display_polydata = []
        self._actor_to_body = {}
        self._actors = []
        self._band_actors = None
        self._band_poly = None
        self._hitbox_overlays = []

        # Per-body feature-edge actors double the actor count and run an
        # extraction filter per body — at DDR5 scale (577 bodies) that alone
        # is ~580 filter passes and ~1150 actors, and VTK interaction cost
        # scales with actors. Above this threshold the silhouette edges are
        # dropped; shading still reads the shapes fine.
        show_feature_edges = show_feature_edges and len(document.bodies) <= 64

        for index, body in enumerate(document.bodies):
            mesh = body.mesh
            if mesh is None or len(mesh.tris) == 0:
                self._body_polydata.append(pv.PolyData())
                self._display_polydata.append(None)
                self._actors.append(None)
                continue
            cells = np.column_stack(
                [np.full(len(mesh.tris), 3, dtype=np.int64), mesh.tris]
            ).ravel()
            polydata = pv.PolyData(mesh.points, cells)
            polydata.cell_data["face_id"] = mesh.tri_face_ids
            polydata.cell_data["rgb"] = self._cell_colors(body, mesh)
            self._body_polydata.append(polydata)

            actor = self.plotter.add_mesh(
                polydata,
                scalars="rgb",
                rgb=True,
                show_edges=False,
                smooth_shading=True,
                lighting=True,
                ambient=material["ambient"],
                diffuse=material["diffuse"],
                specular=material["specular"],
                specular_power=material["specular_power"],
                roughness=material["roughness"],
                metallic=0.0,
                name=f"body-{index}",
                reset_camera=False,  # once per model below, not once per body
                render=False,
            )
            self._actor_to_body[id(actor)] = index
            self._actors.append(actor)
            # add_mesh(smooth_shading=True) renders a normals-computed COPY;
            # cell order is preserved, so keep it for live color updates.
            try:
                self._display_polydata.append(actor.mapper.dataset)
            except Exception:
                self._display_polydata.append(polydata)

            if show_feature_edges:
                edges = polydata.extract_feature_edges(
                    feature_edges=True,
                    boundary_edges=True,
                    non_manifold_edges=False,
                    manifold_edges=False,
                    feature_angle=EDGE_FEATURE_ANGLE_DEG,
                )
                if edges.n_cells:
                    self.plotter.add_mesh(
                        edges, color="#9fb0c4", line_width=1.0,
                        pickable=False, name=f"edges-{index}",
                        reset_camera=False, render=False,
                    )

        self.plotter.add_axes()
        self.show_ground_grid(document.bounds())
        # Everything present now is the model itself; anything added later is
        # a tool overlay that clear_tool_overlays() may remove.
        self._base_actor_names = set(self.plotter.renderer.actors.keys())
        self.plotter.render()

    @staticmethod
    def _cell_colors(body, mesh) -> np.ndarray:
        base = body.color or NEUTRAL_RGB
        colors = np.empty((len(mesh.tri_face_ids), 3), dtype=np.uint8)
        colors[:] = [int(round(c * 255)) for c in base]
        for face_id, rgb in mesh.face_colors.items():
            colors[mesh.tri_face_ids == face_id] = [int(round(c * 255)) for c in rgb]
        return colors

    def _color_targets(self, body_index: int):
        targets = [self._body_polydata[body_index]]
        if body_index < len(self._display_polydata):
            display = self._display_polydata[body_index]
            if display is not None and display is not targets[0]:
                targets.append(display)
        return [t for t in targets if t is not None and t.n_cells > 0]

    def set_model_opacity(self, opacity: float) -> None:
        """Whole-model translucency (the browser's Alpha-view look) — used by
        Separate Unibody so the grown pin regions show through the body."""
        for actor in self._actors:
            if actor is not None:
                actor.GetProperty().SetOpacity(opacity)
        self.plotter.render()

    def set_body_color(self, body_index: int, rgb: tuple[float, float, float]) -> None:
        for polydata in self._color_targets(body_index):
            colors = np.empty((polydata.n_cells, 3), dtype=np.uint8)
            colors[:] = [int(round(c * 255)) for c in rgb]
            polydata.cell_data["rgb"] = colors
        self.plotter.render()

    def set_face_color(
        self, body_index: int, face_index: int, rgb: tuple[float, float, float]
    ) -> None:
        self.paint_faces(body_index, {face_index: rgb})

    def paint_faces(self, body_index: int, face_rgb: dict) -> None:
        """Batch per-face recolor (display only) with a single render."""
        if not face_rgb:
            return
        lookup_ids = np.fromiter(face_rgb.keys(), dtype=np.int64, count=len(face_rgb))
        lookup_rgb = np.array([[int(round(c * 255)) for c in rgb] for rgb in face_rgb.values()],
                              dtype=np.uint8)
        for polydata in self._color_targets(body_index):
            colors = np.asarray(polydata.cell_data["rgb"])
            face_ids = np.asarray(polydata.cell_data["face_id"])
            order = np.argsort(lookup_ids)
            pos = np.searchsorted(lookup_ids[order], face_ids)
            pos = np.clip(pos, 0, len(order) - 1)
            hit = lookup_ids[order][pos] == face_ids
            colors[hit] = lookup_rgb[order][pos[hit]]
            polydata.cell_data["rgb"] = colors
        self.plotter.render()

    # --------------------------------------------------------------- camera

    def snap_camera(self, view_id: str, bounds, aspect: float = 1.4) -> None:
        direction, up = CAMERA_PRESETS.get(view_id, CAMERA_PRESETS["iso"])
        apply_camera_to_plotter(
            self.plotter,
            bounds,
            np.array(direction, dtype=np.float64),
            np.array(up, dtype=np.float64),
            aspect,
        )

    # -------------------------------------------------------------- picking

    def _on_left_press(self, _obj, _event) -> None:
        self._press_position = self.plotter.iren.interactor.GetEventPosition()

    def _on_right_press(self, _obj, _event) -> None:
        self._right_press = self.plotter.iren.interactor.GetEventPosition()

    def _on_right_release(self, _obj, _event) -> None:
        if self._right_press is None:
            return
        x0, y0 = self._right_press
        self._right_press = None
        x1, y1 = self.plotter.iren.interactor.GetEventPosition()
        if abs(x1 - x0) > CLICK_SLOP_PX or abs(y1 - y0) > CLICK_SLOP_PX:
            return  # a right-drag zooms
        if self.pick_at(x1, y1) is None and self.on_right_miss is not None:
            self.on_right_miss()
            self.plotter.render()

    def _on_left_release(self, _obj, _event) -> None:
        if self._press_position is None:
            return
        x0, y0 = self._press_position
        self._press_position = None
        x1, y1 = self.plotter.iren.interactor.GetEventPosition()
        if abs(x1 - x0) > CLICK_SLOP_PX or abs(y1 - y0) > CLICK_SLOP_PX:
            return  # camera drag
        pick = self.pick_at(x1, y1)
        if pick is None:
            # a click on the background keeps the selection; right-click clears it
            if self.on_miss is not None:
                self.on_miss()
            self.plotter.render()
            return
        if pick is not None and self.on_pick is not None:
            shift = bool(self.plotter.iren.interactor.GetShiftKey())
            self.on_pick(
                PickResult(
                    body_index=pick.body_index,
                    face_index=pick.face_index,
                    world_point=pick.world_point,
                    cell_id=pick.cell_id,
                    shift=shift,
                )
            )

    def set_hover_callback(self, callback: Callable[[PickResult | None], None] | None) -> None:
        """Track the surface point under the cursor (None over background).
        Suppressed while a left-drag (camera orbit) is in progress."""
        self._hover_callback = callback
        if callback is None:
            if self._hover_observer is not None:
                self.plotter.iren.remove_observer(self._hover_observer)
                self._hover_observer = None
        elif self._hover_observer is None:
            self._hover_observer = self.plotter.iren.add_observer(
                "MouseMoveEvent", self._on_mouse_move
            )

    def _on_mouse_move(self, _obj, _event) -> None:
        if self._hover_callback is None or self._press_position is not None:
            return
        x, y = self.plotter.iren.interactor.GetEventPosition()
        self._hover_callback(self.pick_at(x, y))

    def pick_at_qt(self, x_qt: float, y_qt: float) -> PickResult | None:
        """Pick from Qt widget coordinates (converts to VTK display pixels)."""
        widget = self.plotter.interactor
        ratio = widget.devicePixelRatioF()
        return self.pick_at(int(x_qt * ratio), int((widget.height() - y_qt) * ratio))

    def pick_at(self, x: int, y: int) -> PickResult | None:
        if not self._picker.Pick(x, y, 0, self.plotter.renderer):
            return None
        actor = self._picker.GetActor()
        body_index = self._actor_to_body.get(id(actor))
        cell_id = self._picker.GetCellId()
        if body_index is None or cell_id < 0:
            return None
        # Cell order survives add_mesh's smooth-shading copy, so the picked
        # mapper cell id indexes our original cell arrays directly.
        face_ids = self._body_polydata[body_index].cell_data["face_id"]
        if cell_id >= len(face_ids):
            return None
        return PickResult(
            body_index=body_index,
            face_index=int(face_ids[cell_id]),
            world_point=tuple(float(v) for v in self._picker.GetPickPosition()),
            cell_id=int(cell_id),
        )

    # ------------------------------------------------------------ highlights

    def highlight_face(self, body_index: int, face_index: int) -> None:
        self.clear_highlight()
        polydata = self._body_polydata[body_index]
        if polydata.n_cells == 0:
            return
        mask = np.nonzero(polydata.cell_data["face_id"] == face_index)[0]
        if len(mask) == 0:
            return
        subset = polydata.extract_cells(mask)
        self.plotter.add_mesh(
            subset,
            color="#ffd24a",
            opacity=1.0,
            lighting=False,
            pickable=False,
            name="highlight-face",
        )
        self.plotter.render()

    def highlight_body(self, body_index: int) -> None:
        self.highlight_bodies([body_index])

    def highlight_bodies(self, body_indices) -> None:
        self.clear_highlight()
        meshes = [
            self._body_polydata[i]
            for i in body_indices
            if 0 <= i < len(self._body_polydata) and self._body_polydata[i].n_cells > 0
        ]
        if not meshes:
            self.plotter.render()
            return
        combined = meshes[0] if len(meshes) == 1 else meshes[0].merge(meshes[1:])
        self.plotter.add_mesh(
            combined,
            color="#ffd24a",
            style="wireframe",
            line_width=2.0,
            lighting=False,
            pickable=False,
            name="highlight-face",
        )
        self.plotter.render()

    def highlight_faces(self, pairs, hatched: bool = False) -> None:
        """Highlight a set of (body_index, face_index) pairs. `hatched` draws
        yellow stripes over the faces instead of a solid fill, so the
        color underneath stays visible (Colors tool)."""
        self.clear_highlight()
        subsets = []
        by_body: dict[int, list[int]] = {}
        for body_index, face_index in pairs:
            by_body.setdefault(body_index, []).append(face_index)
        for body_index, faces in by_body.items():
            polydata = self._body_polydata[body_index]
            if polydata.n_cells == 0:
                continue
            mask = np.nonzero(np.isin(polydata.cell_data["face_id"], faces))[0]
            if len(mask):
                subsets.append(polydata.extract_cells(mask))
        if not subsets:
            self.plotter.render()
            return
        combined = subsets[0] if len(subsets) == 1 else subsets[0].merge(subsets[1:])
        if hatched:
            self._add_hatch(combined)
        else:
            self.plotter.add_mesh(
                combined,
                color="#ffd24a",
                opacity=1.0,
                lighting=False,
                pickable=False,
                name="highlight-face",
            )
        self.plotter.render()

    def _add_hatch(self, surface) -> None:
        """Diagonal yellow stripes over `surface` (contour lines of a
        diagonal ramp) plus a faint tint, under the one highlight name."""
        surface = surface.extract_surface(algorithm="dataset_surface") if hasattr(surface, "extract_surface") else surface
        # ONE fixed array of parallel planes for the whole model (spacing from
        # the model's size, positioned from the world origin): any selection,
        # of any size, on any face, cuts the same planes, so the stripes keep
        # their spacing and run on seamlessly from face to face.
        size = max(getattr(self, "_model_size", 0.0), 1e-6)
        ramp = surface.points @ np.array([0.577, 0.577, 0.577])
        surface.point_data["hatch"] = ramp
        period = size / HATCH_PLANES
        levels = np.arange(np.ceil(ramp.min() / period), np.floor(ramp.max() / period) + 1) * period
        tint = surface.copy()
        tint.point_data.clear()
        self.plotter.add_mesh(tint, color="#ffd24a", opacity=0.15, lighting=False,
                              pickable=False, name="highlight-face")
        if len(levels):
            lines = surface.contour(isosurfaces=levels, scalars="hatch")
            if lines.n_lines:
                lines.point_data.clear()
                self.plotter.add_mesh(lines.tube(radius=size / 260.0), color="#ffd24a",
                                      lighting=False, pickable=False, name="highlight-hatch")

    def clear_tool_overlays(self) -> None:
        """Drop every overlay a tool added (pin faces, leaders, labels,
        markers, sections, hitboxes, triads...). Called on every tool switch so
        nothing leaks into the next tool, whichever steps were skipped."""
        base = getattr(self, "_base_actor_names", None)
        if base is None:
            return
        for name in list(self.plotter.renderer.actors.keys()):
            if name not in base:
                self.remove_overlay(name)
        self._label_groups = 0
        self._pinface_groups = 0
        self._pinaxis_groups = 0
        self._hitbox_overlays = []
        self.hide_band_2d()
        self.plotter.render()

    def clear_highlight(self) -> None:
        for name in ("highlight-face", "highlight-hatch"):
            try:
                self.plotter.remove_actor(name, render=False)
            except Exception:
                pass
        layer = getattr(self, "_xray_layer", None)
        for actor in getattr(self, "_mark_actors", []):
            if layer is not None:
                layer.RemoveActor(actor)
        self._mark_actors = []

    def _xray(self):
        """A renderer on layer 1 sharing the main camera: what it draws is
        always on top, so marks inside or behind a body show through."""
        layer = getattr(self, "_xray_layer", None)
        window = self.plotter.render_window
        if layer is None or layer.GetRenderWindow() is not window:
            layer = vtk.vtkRenderer()
            layer.SetLayer(1)
            layer.InteractiveOff()
            window.SetNumberOfLayers(max(2, window.GetNumberOfLayers()))
            window.AddRenderer(layer)
            self._xray_layer = layer
        layer.SetActiveCamera(self.plotter.renderer.GetActiveCamera())
        return layer

    def show_marks(self, lines, points, color: str = "#ffb000") -> None:
        """Light up edges (polylines) and vertices on top of the model, so
        the ones inside or behind a body show through it (Model Health)."""
        self.clear_highlight()
        layer = self._xray()
        rgb = [int(color[k:k + 2], 16) / 255.0 for k in (1, 3, 5)]
        if len(lines):
            stacked = np.vstack(lines)
            cells, start = [], 0
            for line in lines:
                cells.append(np.concatenate([[len(line)], np.arange(start, start + len(line))]))
                start += len(line)
            self._mark_actors.append(
                self._add_xray_actor(layer, pv.PolyData(stacked, lines=np.concatenate(cells)), rgb, line_width=3.0))
        if len(points):
            self._mark_actors.append(
                self._add_xray_actor(layer, pv.PolyData(np.asarray(points, dtype=float)), rgb, point_size=11.0))
        self.plotter.render()

    def flash_geometry(self, pieces, seconds: float | None = 5.0, color: str = "#2ee66b") -> None:
        """Show what a fix changes: the pieces ((points, tris) meshes) in
        green, on top of the model, for `seconds` (None: until end_flash, as a
        preview)."""
        from PySide6.QtCore import QTimer

        self.end_flash()
        pieces = [(p, t) for p, t in pieces if len(t)]
        if not pieces:
            return
        layer = self._xray()
        rgb = [int(color[k:k + 2], 16) / 255.0 for k in (1, 3, 5)]
        points, cells, offset = [], [], 0
        for p, t in pieces:
            points.append(p)
            cells.append(np.column_stack([np.full(len(t), 3), t + offset]))
            offset += len(p)
        surface = pv.PolyData(np.vstack(points), np.vstack(cells).ravel())
        fill = self._add_xray_actor(layer, surface, rgb)
        edges = self._add_xray_actor(layer, surface.extract_feature_edges(30.0), rgb, line_width=2.0)
        self._flash_actors = [fill, edges]
        self._flash_opacity = [(fill, FLASH_FILL), (edges, FLASH_EDGES)]
        for actor, opacity in self._flash_opacity:
            actor.GetProperty().SetOpacity(opacity)
        self._flash_token = getattr(self, "_flash_token", 0) + 1
        if seconds is not None:  # hold, then fade out over the last FLASH_FADE seconds
            token = self._flash_token
            QTimer.singleShot(int(max(seconds - FLASH_FADE, 0) * 1000), lambda: self._fade_step(token, 1.0))
        self.plotter.render()

    def _fade_step(self, token: int, level: float) -> None:
        """One step of the flash fading out; a newer flash cancels it."""
        from PySide6.QtCore import QTimer

        if token != getattr(self, "_flash_token", 0):
            return
        level -= FADE_TICK_S / FLASH_FADE
        if level <= 0:
            self.end_flash()
            return
        for actor, opacity in getattr(self, "_flash_opacity", []):
            actor.GetProperty().SetOpacity(opacity * level)
        self.plotter.render()
        QTimer.singleShot(int(FADE_TICK_S * 1000), lambda: self._fade_step(token, level))

    def show_pins_xray(self, selected: list, others: list) -> None:
        """Pins on top of the model (they show through it): every pin a
        faint grey, the selected one(s) green. Pieces are (points, tris)."""
        self.clear_pins_xray()
        layer = self._xray()
        for pieces, color, opacity in ((others, "#8a8f98", 0.35), (selected, "#2ee66b", 0.9)):
            pieces = [(p, t) for p, t in pieces if len(t)]
            if not pieces:
                continue
            points, cells, offset = [], [], 0
            for p, t in pieces:
                points.append(p)
                cells.append(np.column_stack([np.full(len(t), 3), t + offset]))
                offset += len(p)
            rgb = [int(color[k:k + 2], 16) / 255.0 for k in (1, 3, 5)]
            actor = self._add_xray_actor(layer, pv.PolyData(np.vstack(points), np.vstack(cells).ravel()), rgb)
            actor.GetProperty().SetOpacity(opacity)
            self._pin_xray_actors.append(actor)
        self.plotter.render()

    def show_land_pattern(self, pads: list[dict], courtyard: float = 0.25, z: float = 0.0, mask: float = 0.05,
                          silk: list | None = None, courtyard_lines: list | None = None) -> None:
        """A footprint's land pattern in the model's frame, on top of the
        model so it shows through the part, in PCB-tool colours: bare copper
        yellow, the solder-mask opening (pad + `mask` per side) an orange
        ring, silkscreen green, courtyard a faint grey. Each pad: x, y, w, h."""
        self.clear_land_pattern()
        if not pads:
            return
        layer = self._xray()
        by_type = {kind: [p for p in pads if p.get("type", "smd") == kind]
                   for kind in ("smd", "thru_hole", "np_thru_hole")}
        self._land_actors = self._smd_actors(layer, by_type["smd"], mask, z)
        self._land_actors += self._hole_actors(layer, by_type["thru_hole"], by_type["np_thru_hole"], mask, z)
        if not courtyard_lines:  # no courtyard in the footprint: pads + margin
            courtyard_lines = _box_lines(pads, mask + courtyard)
        for segments, rgb, opacity in ((courtyard_lines, [0.62, 0.64, 0.66], 0.45), (silk or [], [0.35, 0.85, 0.45], 0.9)):
            if segments:
                self._land_actors.append(self._land_lines(layer, segments, z, rgb, opacity))
        self.plotter.render()

    def _land_fill(self, layer, pads, grow: float, z: float, rgb, opacity: float):
        quads, points = [], []
        for pad in pads:
            x, y, hw, hh = pad["x"], pad["y"], pad["w"] / 2 + grow, pad["h"] / 2 + grow
            start = len(points)
            points += [[x - hw, y - hh, z], [x + hw, y - hh, z], [x + hw, y + hh, z], [x - hw, y + hh, z]]
            quads += [4, start, start + 1, start + 2, start + 3]
        actor = self._add_xray_actor(layer, pv.PolyData(np.array(points), np.array(quads)), rgb)
        actor.GetProperty().SetOpacity(opacity)
        return actor

    def _land_ring(self, layer, pads, grow: float, z: float, rgb, opacity: float):
        """The band between each pad and its opening grown by `grow` (four
        strips per pad), so it never covers the copper."""
        quads, points = [], []
        for pad in pads:
            x, y, hw, hh = pad["x"], pad["y"], pad["w"] / 2, pad["h"] / 2
            ow, oh = hw + grow, hh + grow
            inner = [(x - hw, y - hh), (x + hw, y - hh), (x + hw, y + hh), (x - hw, y + hh)]
            outer = [(x - ow, y - oh), (x + ow, y - oh), (x + ow, y + oh), (x - ow, y + oh)]
            for k in range(4):
                start = len(points)
                points += [[*outer[k], z], [*outer[(k + 1) % 4], z], [*inner[(k + 1) % 4], z], [*inner[k], z]]
                quads += [4, start, start + 1, start + 2, start + 3]
        actor = self._add_xray_actor(layer, pv.PolyData(np.array(points), np.array(quads)), rgb)
        actor.GetProperty().SetOpacity(opacity)
        return actor

    def _smd_actors(self, layer, smd: list, mask: float, z: float) -> list:
        """SMT lands: the orange mask-opening ring, then the bare copper."""
        if not smd:
            return []
        return [self._land_ring(layer, smd, mask, z, [1.00, 0.50, 0.10], LAND_OPACITY),
                self._land_fill(layer, smd, 0.0, z, [1.00, 0.88, 0.30], LAND_OPACITY)]

    def _hole_actors(self, layer, plated: list, unplated: list, mask: float, z: float) -> list:
        """Through-holes as non-overlapping rings: plated = orange mask ring,
        yellow copper annulus, dark drill; unplated = light rim, dark drill."""
        dark, light = [0.06, 0.06, 0.07], [0.80, 0.82, 0.85]
        layers = [
            (plated, lambda p: (_radii(p, "pad", mask), _radii(p, "pad", 0.0)), [1.00, 0.50, 0.10]),
            (plated, lambda p: (_radii(p, "pad", 0.0), _radii(p, "drill", 0.0)), [1.00, 0.88, 0.30]),
            (plated, lambda p: (_radii(p, "drill", 0.0), None), dark),
            (unplated, lambda p: (_radii(p, "drill", 0.05), _radii(p, "drill", 0.0)), light),
            (unplated, lambda p: (_radii(p, "drill", 0.0), None), dark),
        ]
        actors = []
        for pads, radii, rgb in layers:
            if pads:
                actor = self._add_xray_actor(layer, _ring_mesh(pads, radii, z), rgb)
                actor.GetProperty().SetOpacity(LAND_OPACITY)
                actors.append(actor)
        return actors

    def _land_lines(self, layer, segments, z: float, rgb, opacity: float):
        pts = np.array([[x, y, z] for a, b, c, d in segments for x, y in ((a, b), (c, d))])
        cells = np.array([[2, 2 * k, 2 * k + 1] for k in range(len(segments))]).ravel()
        actor = self._add_xray_actor(layer, pv.PolyData(pts, lines=cells), rgb, line_width=2.0)
        actor.GetProperty().SetOpacity(opacity)
        return actor

    def show_ghost_board(self, pads: list[dict], rect: tuple, thickness: float = BOARD_THICKNESS) -> None:
        """A generic PCB under the part, like a CAD 3D preview: a green board
        `thickness` thick below Z=0 over `rect` (xmin, xmax, ymin, ymax), 20%
        opaque, with every drill cut through; plated holes get a copper
        barrel, unplated holes stay bare."""
        self.clear_ghost_board()
        board = _board_with_holes(rect, pads, thickness)
        if board is None:
            return
        self.plotter.add_mesh(board, color="#1f7a3a", opacity=GHOST_OPACITY, lighting=True,
                              pickable=False, name="ghost-board", reset_camera=False, render=False)
        self._ghost_names = ["ghost-board"]
        barrels = [_barrel(p, thickness) for p in pads if p.get("type") == "thru_hole"]
        if barrels:
            merged = barrels[0] if len(barrels) == 1 else barrels[0].merge(barrels[1:])
            self.plotter.add_mesh(merged, color="#e8b830", opacity=0.6, pickable=False,
                                  name="ghost-barrels", reset_camera=False, render=False)
            self._ghost_names.append("ghost-barrels")
        self.plotter.render()

    def clear_ghost_board(self) -> None:
        for name in getattr(self, "_ghost_names", []):
            self.remove_overlay(name)
        self._ghost_names = []

    def show_ghost_symbol(self, rect: tuple, stubs: list, labels: list, selected: set,
                          leaders: list | None = None) -> None:
        """A schematic symbol lying on the Z=0 plane, on top of the model so
        it shows through the part: the body rectangle (xmin, xmax, ymin,
        ymax) 20% opaque in symbol yellow with a dark red outline, a stub per
        pin [(x0, y0, x1, y1) or None], thin leaders from the real pins to
        their stubs, and labels [(x, y, text)]; `selected` stubs draw green."""
        self.clear_ghost_symbol()
        layer = self._xray()
        x0, x1, y0, y1 = rect
        z = 0.0
        body = pv.PolyData(np.array([[x0, y0, z], [x1, y0, z], [x1, y1, z], [x0, y1, z]]), np.array([4, 0, 1, 2, 3]))
        fill = self._add_xray_actor(layer, body, [1.0, 1.0, 0.76])
        fill.GetProperty().SetOpacity(GHOST_OPACITY)
        outline = [(x0, y0, x1, y0), (x1, y0, x1, y1), (x1, y1, x0, y1), (x0, y1, x0, y0)]
        plain, chosen = _split_stubs(stubs, selected)
        styles = ((outline, [0.52, 0.0, 0.0], 2.0, 0.9), (leaders or [], [0.75, 0.5, 0.5], 1.0, 0.6),
                  (plain, [0.52, 0.0, 0.0], 3.0, 1.0), (chosen, [0.18, 0.9, 0.42], 3.0, 1.0))
        self._symbol_actors = [fill] + [self._segments_actor(layer, segs, rgb, width, opacity)
                                        for segs, rgb, width, opacity in styles if segs]
        height = min(max(0.035 * min(x1 - x0, y1 - y0), 0.12), 0.6)
        for group, rgb in ((set(range(len(labels))) - selected, [0.37, 0.88, 0.88]), (selected, [0.18, 0.9, 0.42])):
            texts = [_flat_label(labels[i], height) for i in sorted(group) if i < len(labels)]
            if texts:
                merged = texts[0] if len(texts) == 1 else texts[0].merge(texts[1:])
                self._symbol_actors.append(self._add_xray_actor(layer, merged, rgb))
        self._symbol_names = []
        self.plotter.render()

    def _segments_actor(self, layer, segments: list, rgb, width: float, opacity: float):
        pts = np.array([[a, b, 0.0] for s in segments for a, b in ((s[0], s[1]), (s[2], s[3]))])
        cells = np.array([[2, 2 * k, 2 * k + 1] for k in range(len(segments))]).ravel()
        actor = self._add_xray_actor(layer, pv.PolyData(pts, lines=cells), rgb, line_width=width)
        actor.GetProperty().SetOpacity(opacity)
        return actor

    def clear_ghost_symbol(self) -> None:
        for name in getattr(self, "_symbol_names", []):
            self.remove_overlay(name)
        self._symbol_names = []
        layer = getattr(self, "_xray_layer", None)
        for actor in getattr(self, "_symbol_actors", []):
            if layer is not None:
                layer.RemoveActor(actor)
        self._symbol_actors = []

    def clear_land_pattern(self) -> None:
        self.clear_ghost_board()
        layer = getattr(self, "_xray_layer", None)
        for actor in getattr(self, "_land_actors", []):
            if layer is not None:
                layer.RemoveActor(actor)
        self._land_actors = []

    def clear_pins_xray(self) -> None:
        layer = getattr(self, "_xray_layer", None)
        for actor in getattr(self, "_pin_xray_actors", []):
            if layer is not None:
                layer.RemoveActor(actor)
        self._pin_xray_actors = []

    def end_flash(self) -> None:
        self._flash_token = getattr(self, "_flash_token", 0) + 1  # stops a running fade
        layer = getattr(self, "_xray_layer", None)
        for actor in getattr(self, "_flash_actors", []):
            if layer is not None:
                layer.RemoveActor(actor)
        if getattr(self, "_flash_actors", None):
            self._flash_actors = []
            self.plotter.render()

    @staticmethod
    def _add_xray_actor(layer, polydata, rgb, line_width: float = 1.0, point_size: float = 1.0):
        mapper = vtk.vtkPolyDataMapper()
        mapper.SetInputData(polydata)
        actor = vtk.vtkActor()
        actor.SetMapper(mapper)
        actor.PickableOff()
        prop = actor.GetProperty()
        prop.SetColor(*rgb)
        prop.SetLighting(False)
        prop.SetLineWidth(line_width)
        prop.SetPointSize(point_size)
        if point_size > 1.0:
            prop.SetRepresentationToPoints()
            prop.RenderPointsAsSpheresOn()
        layer.AddActor(actor)
        return actor

    # -------------------------------------------------------------- overlays

    def show_ground_grid(self, bounds) -> None:
        """Square grid on the Z=0 seating plane around the body, with the
        X=0 / Y=0 origin lines emphasized — makes standoff and import-offset
        problems visible at a glance."""
        self.remove_overlay("ground-grid")
        self.remove_overlay("ground-grid-axes")
        xmin, xmax, ymin, ymax, _zmin, _zmax = bounds
        cx, cy = (xmin + xmax) * 0.5, (ymin + ymax) * 0.5
        side = max(xmax - xmin, ymax - ymin, 1.0e-6) * 1.8
        half = side * 0.5
        step = _nice_step(side / 10.0)

        xs = np.arange(np.ceil((cx - half) / step) * step, cx + half + 1e-9, step)
        ys = np.arange(np.ceil((cy - half) / step) * step, cy + half + 1e-9, step)
        points: list[list[float]] = []
        lines: list[int] = []
        axes_points: list[list[float]] = []
        axes_lines: list[int] = []

        def add_line(bucket_points, bucket_lines, start, end) -> None:
            bucket_lines.extend([2, len(bucket_points), len(bucket_points) + 1])
            bucket_points.append(start)
            bucket_points.append(end)

        for x in xs:
            target = (axes_points, axes_lines) if abs(x) < step * 1e-6 else (points, lines)
            add_line(*target, [x, cy - half, 0.0], [x, cy + half, 0.0])
        for y in ys:
            target = (axes_points, axes_lines) if abs(y) < step * 1e-6 else (points, lines)
            add_line(*target, [cx - half, y, 0.0], [cx + half, y, 0.0])

        if points:
            grid = pv.PolyData(np.array(points), lines=np.array(lines))
            self.plotter.add_mesh(
                grid, color="#2a2a34", line_width=1.0, lighting=False,
                pickable=False, name="ground-grid",
            )
        if axes_points:
            axes = pv.PolyData(np.array(axes_points), lines=np.array(axes_lines))
            self.plotter.add_mesh(
                axes, color="#5a5a6a", line_width=2.0, lighting=False,
                pickable=False, name="ground-grid-axes",
            )

    def set_markers(
        self,
        points,
        *,
        name: str = "pick-markers",
        color: str = "#e8443a",
        point_size: float = 16.0,
        opacity: float = 1.0,
        on_top: bool = False,
    ) -> None:
        self.remove_overlay(name)
        points = np.asarray(points, dtype=np.float64).reshape(-1, 3)
        if len(points) == 0:
            self.plotter.render()
            return
        cloud = pv.PolyData(points)
        actor = self.plotter.add_mesh(
            cloud,
            color=color,
            point_size=point_size,
            render_points_as_spheres=True,
            lighting=False,
            opacity=opacity,
            pickable=False,
            name=name,
        )
        if on_top and actor is not None:
            # Draw over the model body regardless of depth (a large negative
            # polygon offset pulls the sphere markers toward the camera in the
            # depth buffer — the same trick pyvista uses for always-visible
            # labels), so the marker is never swallowed by the geometry.
            try:
                mapper = actor.GetMapper()
                mapper.SetResolveCoincidentTopologyToPolygonOffset()
                mapper.SetRelativeCoincidentTopologyPolygonOffsetParameters(
                    -66000.0, -66000.0)
            except Exception:  # noqa: BLE001 — best-effort; fall back to in-depth
                pass
        self.plotter.render()

    def show_segment(
        self,
        p1,
        p2,
        *,
        name: str = "segment",
        color: str = "#e8443a",
        width: float = 6.0,
        opacity: float = 1.0,
    ) -> None:
        """Draw a bold line segment (a highlighted edge), always on top. Pass
        p1=None to clear it."""
        self.remove_overlay(name)
        if p1 is None or p2 is None:
            self.plotter.render()
            return
        actor = self.plotter.add_mesh(
            pv.Line(tuple(p1), tuple(p2)), color=color, line_width=width,
            render_lines_as_tubes=True, opacity=opacity, pickable=False, name=name)
        self._push_on_top(actor)
        self.plotter.render()

    def show_quad(
        self,
        corners,
        *,
        name: str = "preview-quad",
        color: str = "#1f5fd6",
        opacity: float = 0.30,
    ) -> None:
        """Translucent quadrilateral (e.g. the Z-sit seating rectangle)."""
        self.remove_overlay(name)
        self.remove_overlay(f"{name}-outline")
        corners = np.asarray(corners, dtype=np.float64).reshape(-1, 3)
        if len(corners) != 4:
            self.plotter.render()
            return
        quad = pv.PolyData(corners, np.array([4, 0, 1, 2, 3], dtype=np.int64))
        self.plotter.add_mesh(
            quad, color=color, opacity=opacity, lighting=False, pickable=False, name=name
        )
        outline = pv.PolyData(
            corners, lines=np.array([5, 0, 1, 2, 3, 0], dtype=np.int64)
        )
        self.plotter.add_mesh(
            outline, color=color, line_width=2.0, lighting=False,
            pickable=False, name=f"{name}-outline",
        )
        self.plotter.render()

    def show_arrows(
        self,
        starts,
        direction,
        *,
        scale: float,
        name: str = "preview-arrows",
        color: str = "#1f5fd6",
    ) -> None:
        """One arrow per start point, all pointing along `direction`."""
        self.remove_overlay(name)
        starts = np.asarray(starts, dtype=np.float64).reshape(-1, 3)
        if len(starts) == 0:
            self.plotter.render()
            return
        direction = np.asarray(direction, dtype=np.float64)
        arrows = [
            pv.Arrow(
                start=start,
                direction=direction,
                scale=scale,
                tip_radius=0.08,
                shaft_radius=0.03,
            )
            for start in starts
        ]
        merged = arrows[0]
        for arrow in arrows[1:]:
            merged = merged.merge(arrow)
        self.plotter.add_mesh(merged, color=color, lighting=False, pickable=False, name=name)
        self.plotter.render()

    def show_triad(self, origin, x_dir, y_dir, z_dir, scale: float) -> None:
        """Preview axes for a frame definition (e.g. the Z-sit plane)."""
        origin = np.asarray(origin, dtype=np.float64)
        for suffix, direction, color in (
            ("x", x_dir, "#d62828"),
            ("y", y_dir, "#2a9d2a"),
            ("z", z_dir, "#1f5fd6"),
        ):
            name = f"triad-{suffix}"
            self.remove_overlay(name)
            arrow = pv.Arrow(
                start=origin,
                direction=np.asarray(direction, dtype=np.float64),
                scale=scale,
                tip_radius=0.06,
                shaft_radius=0.025,
            )
            self.plotter.add_mesh(arrow, color=color, lighting=False, pickable=False, name=name)
        self.plotter.render()

    def show_section(self, segments, *, name: str = "context-section") -> None:
        """Section-curve overlay: (S, 2, 3) world-space segments."""
        self.remove_overlay(name)
        segments = np.asarray(segments, dtype=np.float64)
        if segments.size == 0:
            self.plotter.render()
            return
        points = segments.reshape(-1, 3)
        count = len(segments)
        lines = np.column_stack(
            [np.full(count, 2, dtype=np.int64),
             np.arange(0, 2 * count, 2, dtype=np.int64),
             np.arange(1, 2 * count, 2, dtype=np.int64)]
        ).ravel()
        polydata = pv.PolyData(points, lines=lines)
        self.plotter.add_mesh(
            polydata, color="#c89d00", line_width=2.5, lighting=False,
            pickable=False, name=name,
        )
        self.plotter.render()

    def clear_triad(self) -> None:
        for suffix in ("x", "y", "z"):
            self.remove_overlay(f"triad-{suffix}")
        self.plotter.render()

    def remove_overlay(self, name: str) -> None:
        try:
            self.plotter.remove_actor(name, render=False)
        except Exception:
            pass

    def show_hitboxes(
        self, hitboxes: list[dict], *, selected: int = -1, colors: list[str] | None = None
    ) -> None:
        """Translucent metadata volumes; never part of the B-rep. Boxes are
        merged into one actor per color — per-box actors made big BGAs crawl."""
        for name in self._hitbox_overlays:
            self.remove_overlay(name)
        self._hitbox_overlays = []

        groups: dict[str, list] = {}
        for index, hitbox in enumerate(hitboxes):
            if hitbox is None:
                continue
            try:
                if hitbox.get("type") == "obb":
                    hx, hy, hz = hitbox["half_extents"]
                    mesh = pv.Cube(x_length=2 * hx, y_length=2 * hy, z_length=2 * hz)
                    mesh = mesh.rotate_z(float(hitbox.get("rotation_z_deg", 0.0)))
                    mesh = mesh.translate(hitbox["center"])
                elif hitbox.get("type") == "convex_hull":
                    cloud = pv.PolyData(np.asarray(hitbox["vertices"], dtype=np.float64))
                    mesh = cloud.delaunay_3d().extract_geometry()
                elif hitbox.get("type") in ("shrink_wrap", "shallow_concave"):
                    tris = np.asarray(hitbox["faces"], dtype=np.int64)
                    cells = np.column_stack([np.full(len(tris), 3), tris]).ravel()
                    mesh = pv.PolyData(np.asarray(hitbox["vertices"], dtype=np.float64), cells)
                else:
                    continue
            except Exception:
                continue
            if index == selected:
                color = "#ff5050"
            elif colors is not None and index < len(colors):
                color = colors[index]
            else:
                color = "#27b0d6"
            groups.setdefault(color, []).append(mesh)

        for group_index, (color, meshes) in enumerate(groups.items()):
            merged = meshes[0] if len(meshes) == 1 else meshes[0].merge(meshes[1:])
            name = f"hitboxes-{group_index}"
            self._hitbox_overlays.append(name)
            self.plotter.add_mesh(
                merged,
                color=color,
                opacity=0.32,
                lighting=False,
                pickable=False,
                name=name,
            )
        self.plotter.render()

    def show_pin_labels(self, points, labels, colors=None) -> None:
        """Pin labels, optionally per-label text color (anchored green,
        predicted orange, default dark) — one label actor per color group."""
        for index in range(self._label_groups):
            self.remove_overlay(f"pin-labels-{index}")
        self._label_groups = 0
        if len(points) == 0:
            self.plotter.render()
            return
        points = np.asarray(points, dtype=np.float64)
        labels = list(labels)
        if colors is None:
            colors = ["#1a1a1a"] * len(labels)
        groups: dict[str, list[int]] = {}
        for index, color in enumerate(colors):
            groups.setdefault(color, []).append(index)
        for color, members in groups.items():
            name = f"pin-labels-{self._label_groups}"
            self._label_groups += 1
            self.plotter.add_point_labels(
                points[members],
                [labels[i] for i in members],
                name=name,
                font_size=16,
                point_size=12,
                point_color="#ff8800",
                text_color=color,
                shape_color="#ffffff",
                shape_opacity=0.85,
                always_visible=True,
                pickable=False,
            )
        self.plotter.render()

    def _push_on_top(self, actor) -> None:
        """Pull an overlay actor toward the camera in the depth buffer so it
        renders over the body regardless of occlusion (the same large negative
        polygon offset pyvista uses for always-visible labels) — used for the
        xray pin-face callouts so faces buried inside a model stay visible."""
        if actor is None:
            return
        try:
            mapper = actor.GetMapper()
            mapper.SetResolveCoincidentTopologyToPolygonOffset()
            mapper.SetRelativeCoincidentTopologyPolygonOffsetParameters(
                -66000.0, -66000.0)
        except Exception:  # noqa: BLE001 — best-effort; fall back to in-depth
            pass

    def show_pin_faces(self, faces_by_color: dict) -> None:
        """Tint the detected pin-tip faces and render them ON TOP of the body
        (xray) so you can see exactly where each face is, even buried inside a
        model. faces_by_color maps a hex color -> list of (body, face) pairs."""
        for index in range(self._pinface_groups):
            self.remove_overlay(f"pin-faces-{index}")
        self._pinface_groups = 0
        for color, pairs in faces_by_color.items():
            by_body: dict[int, list[int]] = {}
            for body_index, face_index in pairs:
                by_body.setdefault(body_index, []).append(face_index)
            subsets = []
            for body_index, faces in by_body.items():
                if not 0 <= body_index < len(self._body_polydata):
                    continue
                polydata = self._body_polydata[body_index]
                mask = np.nonzero(np.isin(polydata.cell_data["face_id"], faces))[0]
                if len(mask):
                    subsets.append(polydata.extract_cells(mask))
            if not subsets:
                continue
            combined = subsets[0] if len(subsets) == 1 else subsets[0].merge(subsets[1:])
            name = f"pin-faces-{self._pinface_groups}"
            self._pinface_groups += 1
            actor = self.plotter.add_mesh(combined, color=color, opacity=1.0,
                                          lighting=False, pickable=False, name=name)
            self._push_on_top(actor)
        self.plotter.render()

    def show_pin_axes(self, segments, colors) -> None:
        """Per-pin leader lines (face -> off-the-face label), colored by
        designation (orange THR/SMT tails, blue CON/HEAD mouths) and drawn on
        top so the whole callout reads through the body. `segments` is
        (N, 2, 3) world-space endpoints; `colors` a hex color per segment."""
        for index in range(self._pinaxis_groups):
            self.remove_overlay(f"pin-axes-{index}")
        self._pinaxis_groups = 0
        segments = np.asarray(segments, dtype=np.float64)
        if segments.size == 0:
            self.plotter.render()
            return
        groups: dict[str, list[int]] = {}
        for index, color in enumerate(colors):
            groups.setdefault(color, []).append(index)
        for color, members in groups.items():
            segs = segments[members]
            count = len(segs)
            lines = np.column_stack(
                [np.full(count, 2, dtype=np.int64),
                 np.arange(0, 2 * count, 2, dtype=np.int64),
                 np.arange(1, 2 * count, 2, dtype=np.int64)]
            ).ravel()
            polydata = pv.PolyData(segs.reshape(-1, 3), lines=lines)
            name = f"pin-axes-{self._pinaxis_groups}"
            self._pinaxis_groups += 1
            actor = self.plotter.add_mesh(polydata, color=color, line_width=3.0,
                                          lighting=False, pickable=False, name=name)
            self._push_on_top(actor)
        self.plotter.render()

    def show_pin_spines(self, segments, *, color: str = "#ffd24a") -> None:
        """Dotted yellow 'spine' bridging a pin's two end faces (THR tail <->
        CON mouth) — the per-pin axis the Separate tool carves along, drawn on
        top. `segments` is (N, 2, 3) world-space endpoints."""
        self.remove_overlay("pin-spines")
        segments = np.asarray(segments, dtype=np.float64)
        if segments.size == 0:
            self.plotter.render()
            return
        count = len(segments)
        lines = np.column_stack(
            [np.full(count, 2, dtype=np.int64),
             np.arange(0, 2 * count, 2, dtype=np.int64),
             np.arange(1, 2 * count, 2, dtype=np.int64)]
        ).ravel()
        polydata = pv.PolyData(segments.reshape(-1, 3), lines=lines)
        actor = self.plotter.add_mesh(polydata, color=color, line_width=2.0,
                                      lighting=False, pickable=False,
                                      name="pin-spines")
        try:   # dashed stipple where the GL backend supports it
            prop = actor.GetProperty()
            prop.SetLineStipplePattern(0xF0F0)
            prop.SetLineStippleRepeatFactor(2)
        except Exception:  # noqa: BLE001 — unsupported -> solid spine
            pass
        self._push_on_top(actor)
        self.plotter.render()

    # ------------------------------------------------------------- band 2D

    def show_band_2d(self, x0: float, y0: float, x1: float, y1: float) -> None:
        """Translucent selection rectangle drawn as a VTK 2D overlay (Qt
        rubber bands cannot composite over the GL surface). Coordinates are
        Qt widget pixels."""
        widget = self.plotter.interactor
        ratio = widget.devicePixelRatioF()
        height = widget.height()
        corners = [(x0, y0), (x1, y0), (x1, y1), (x0, y1)]
        points = vtk.vtkPoints()
        for qx, qy in corners:
            points.InsertNextPoint(qx * ratio, (height - qy) * ratio, 0.0)

        if self._band_actors is None:
            self._band_poly = vtk.vtkPolyData()
            polys = vtk.vtkCellArray()
            polys.InsertNextCell(4)
            for i in range(4):
                polys.InsertCellPoint(i)
            lines = vtk.vtkCellArray()
            lines.InsertNextCell(5)
            for i in (0, 1, 2, 3, 0):
                lines.InsertCellPoint(i)

            fill_poly = vtk.vtkPolyData()
            fill_poly.SetPolys(polys)
            outline_poly = vtk.vtkPolyData()
            outline_poly.SetLines(lines)
            self._band_poly = (fill_poly, outline_poly)

            actors = []
            for poly, opacity, width in ((fill_poly, 0.18, 1.0), (outline_poly, 0.9, 2.0)):
                mapper = vtk.vtkPolyDataMapper2D()
                mapper.SetInputData(poly)
                actor = vtk.vtkActor2D()
                actor.SetMapper(mapper)
                actor.GetProperty().SetColor(0.13, 0.62, 0.27)
                actor.GetProperty().SetOpacity(opacity)
                actor.GetProperty().SetLineWidth(width)
                actor.SetPickable(False)
                self.plotter.renderer.AddActor2D(actor)
                actors.append(actor)
            self._band_actors = tuple(actors)

        for poly in self._band_poly:
            poly.SetPoints(points)
            poly.Modified()
        for actor in self._band_actors:
            actor.SetVisibility(True)
        self.plotter.render()

    def hide_band_2d(self) -> None:
        if self._band_actors is not None:
            for actor in self._band_actors:
                actor.SetVisibility(False)
            self.plotter.render()

    # ---------------------------------------------------------- unprojection

    def display_to_world(self, x_qt: float, y_qt: float) -> tuple[float, float, float]:
        """Qt widget coordinates -> world point on the near plane. With a
        parallel top view this yields the world XY under the cursor."""
        widget = self.plotter.interactor
        ratio = widget.devicePixelRatioF()
        xd = x_qt * ratio
        yd = (widget.height() - y_qt) * ratio
        renderer = self.plotter.renderer
        renderer.SetDisplayPoint(xd, yd, 0.0)
        renderer.DisplayToWorld()
        wx, wy, wz, w = renderer.GetWorldPoint()
        if w not in (0.0, 1.0):
            wx, wy, wz = wx / w, wy / w, wz / w
        return float(wx), float(wy), float(wz)


    def world_to_display_qt(self, point) -> tuple[float, float]:
        renderer = self.plotter.renderer
        renderer.SetWorldPoint(float(point[0]), float(point[1]), float(point[2]), 1.0)
        renderer.WorldToDisplay()
        xd, yd, _zd = renderer.GetDisplayPoint()
        widget = self.plotter.interactor
        ratio = widget.devicePixelRatioF()
        return xd / ratio, widget.height() - yd / ratio

    def display_to_plane(self, x_qt: float, y_qt: float, origin, normal):
        """Cursor position -> intersection of the view ray with a plane."""
        near = np.asarray(self.display_to_world(x_qt, y_qt), dtype=np.float64)
        camera = self.plotter.camera
        direction = (np.asarray(camera.GetFocalPoint())
                     - np.asarray(camera.GetPosition()))
        direction /= max(float(np.linalg.norm(direction)), 1e-12)
        origin = np.asarray(origin, dtype=np.float64)
        normal = np.asarray(normal, dtype=np.float64)
        denominator = float(direction @ normal)
        if abs(denominator) < 1e-9:
            return None
        t = float((origin - near) @ normal) / denominator
        return near + direction * t


class BandSelector(QObject):
    """Qt-level rubber-band drag on the 3D viewport. While installed it
    consumes left-button drags (camera orbit stays on middle/right buttons)
    and reports the band corners in Qt widget coordinates. The visible
    rectangle itself is drawn by the scene (VTK 2D overlay) via on_update."""

    def __init__(
        self,
        widget,
        on_band: Callable[[QPoint, QPoint], None],
        on_update: Callable[[QPoint, QPoint], None] | None = None,
        on_finish: Callable[[], None] | None = None,
        on_click: Callable[[QPoint], None] | None = None,
    ) -> None:
        super().__init__(widget)
        self._widget = widget
        self._on_band = on_band
        self._on_update = on_update
        self._on_finish = on_finish
        self._on_click = on_click
        self._origin: QPoint | None = None

    def attach(self) -> None:
        self._widget.installEventFilter(self)

    def detach(self) -> None:
        self._widget.removeEventFilter(self)
        self._origin = None
        if self._on_finish is not None:
            self._on_finish()

    def eventFilter(self, obj, event) -> bool:
        if obj is not self._widget:
            return False
        etype = event.type()
        if etype == QEvent.Type.MouseButtonPress and event.button() == Qt.MouseButton.LeftButton:
            self._origin = event.position().toPoint()
            return True
        if etype == QEvent.Type.MouseMove and self._origin is not None:
            if self._on_update is not None:
                self._on_update(self._origin, event.position().toPoint())
            return True
        if etype == QEvent.Type.MouseButtonRelease and event.button() == Qt.MouseButton.LeftButton:
            if self._origin is None:
                return False
            origin, end = self._origin, event.position().toPoint()
            self._origin = None
            if self._on_finish is not None:
                self._on_finish()
            if (end - origin).manhattanLength() > 6:
                self._on_band(origin, end)
            elif self._on_click is not None:
                self._on_click(end)
            return True
        return False
