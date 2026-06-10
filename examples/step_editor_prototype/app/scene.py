"""SceneManager: owns everything in the PyVista plotter — body actors with
per-face STEP colours, click picking that resolves back to (body, B-rep face),
and highlight overlays. No OCP imports here; geometry arrives as numpy from
EditorDocument."""

from __future__ import annotations

from dataclasses import dataclass
from typing import Callable

import numpy as np
import pyvista as pv
import vtk

from .document import NEUTRAL_RGB, EditorDocument
from .viewcam import (
    CAMERA_PRESETS,
    EDGE_FEATURE_ANGLE_DEG,
    LightingSettings,
    apply_camera_to_plotter,
    configure_lighting,
    material_settings,
)


CLICK_SLOP_PX = 4  # press→release movement beyond this is a drag, not a pick


@dataclass(frozen=True)
class PickResult:
    body_index: int
    face_index: int          # 1-based index into the body's face map
    world_point: tuple[float, float, float]
    cell_id: int


class SceneManager:
    def __init__(self, plotter) -> None:
        self.plotter = plotter
        self.on_pick: Callable[[PickResult], None] | None = None
        self._body_polydata: list[pv.PolyData] = []
        self._actor_to_body: dict[int, int] = {}  # id(vtkActor) -> body index
        self._actors: list = []
        self._press_position: tuple[int, int] | None = None
        self._picker = vtk.vtkCellPicker()
        self._picker.SetTolerance(0.0005)
        # Polygon offset keeps highlight overlays from z-fighting their bodies.
        vtk.vtkMapper.SetResolveCoincidentTopologyToPolygonOffset()

        plotter.set_background("white")
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
        self._actor_to_body = {}
        self._actors = []

        for index, body in enumerate(document.bodies):
            mesh = body.mesh
            if mesh is None or len(mesh.tris) == 0:
                self._body_polydata.append(pv.PolyData())
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
                        edges, color="#253044", line_width=1.0, name=f"edges-{index}"
                    )

        self.plotter.add_axes()
        self.plotter.render()

    @staticmethod
    def _cell_colors(body, mesh) -> np.ndarray:
        base = body.color or NEUTRAL_RGB
        colors = np.empty((len(mesh.tri_face_ids), 3), dtype=np.uint8)
        colors[:] = [int(round(c * 255)) for c in base]
        for face_id, rgb in mesh.face_colors.items():
            colors[mesh.tri_face_ids == face_id] = [int(round(c * 255)) for c in rgb]
        return colors

    def set_body_color(self, body_index: int, rgb: tuple[float, float, float]) -> None:
        polydata = self._body_polydata[body_index]
        if polydata.n_cells == 0:
            return
        colors = np.empty((polydata.n_cells, 3), dtype=np.uint8)
        colors[:] = [int(round(c * 255)) for c in rgb]
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
        if pick is not None and self.on_pick is not None:
            self.on_pick(pick)

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
            name="highlight-face",
        )
        self.plotter.render()

    def highlight_body(self, body_index: int) -> None:
        self.clear_highlight()
        polydata = self._body_polydata[body_index]
        if polydata.n_cells == 0:
            return
        self.plotter.add_mesh(
            polydata,
            color="#ffd24a",
            style="wireframe",
            line_width=2.0,
            lighting=False,
            name="highlight-face",
        )
        self.plotter.render()

    def clear_highlight(self) -> None:
        try:
            self.plotter.remove_actor("highlight-face", render=False)
        except Exception:
            pass
