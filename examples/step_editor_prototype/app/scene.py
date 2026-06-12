"""SceneManager: owns everything in the PyVista plotter — body actors with
per-face STEP colours, click picking that resolves back to (body, B-rep face),
rubber-band selection, and highlight overlays. No OCP imports here; geometry
arrives as numpy from EditorDocument."""

from __future__ import annotations

from dataclasses import dataclass
from typing import Callable

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


class SceneManager:
    def __init__(self, plotter) -> None:
        self.plotter = plotter
        self.on_pick: Callable[[PickResult], None] | None = None
        self.on_miss: Callable[[], None] | None = None  # click on empty space
        self._body_polydata: list[pv.PolyData] = []
        self._display_polydata: list = []  # the smooth-shaded copies the mappers render
        self._actor_to_body: dict[int, int] = {}  # id(vtkActor) -> body index
        self._actors: list = []
        self._band_actors: tuple | None = None
        self._band_poly = None
        self._press_position: tuple[int, int] | None = None
        self._hitbox_overlays: list[str] = []
        self._label_groups = 0
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
        plotter.iren.add_observer("LeftButtonPressEvent", self._on_left_press)
        plotter.iren.add_observer("LeftButtonReleaseEvent", self._on_left_release)

    # -------------------------------------------------------------- building

    def rebuild(self, document: EditorDocument, *, show_feature_edges: bool = True) -> None:
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
                    )

        self.plotter.add_axes()
        self.show_ground_grid(document.bounds())
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
        for polydata in self._color_targets(body_index):
            colors = np.asarray(polydata.cell_data["rgb"])
            face_ids = polydata.cell_data["face_id"]
            for face_index, rgb in face_rgb.items():
                colors[face_ids == face_index] = [int(round(c * 255)) for c in rgb]
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
            self.clear_highlight()
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

    def highlight_faces(self, pairs) -> None:
        """Highlight a set of (body_index, face_index) pairs."""
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
        self.plotter.add_mesh(
            combined,
            color="#ffd24a",
            opacity=1.0,
            lighting=False,
            pickable=False,
            name="highlight-face",
        )
        self.plotter.render()

    def clear_highlight(self) -> None:
        try:
            self.plotter.remove_actor("highlight-face", render=False)
        except Exception:
            pass

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
    ) -> None:
        self.remove_overlay(name)
        points = np.asarray(points, dtype=np.float64).reshape(-1, 3)
        if len(points) == 0:
            self.plotter.render()
            return
        cloud = pv.PolyData(points)
        self.plotter.add_mesh(
            cloud,
            color=color,
            point_size=point_size,
            render_points_as_spheres=True,
            lighting=False,
            opacity=opacity,
            pickable=False,
            name=name,
        )
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
        merged into one actor per colour — per-box actors made big BGAs crawl."""
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
        """Pin labels, optionally per-label text colour (anchored green,
        predicted orange, default dark) — one label actor per colour group."""
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
