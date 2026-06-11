"""DXF -> geometer planar_step request, and the shared emboss pipeline used
by both the Apply LOGO tool and the m7 selftest. The extruded tool body is
built by geometer (planar_step, schema geometry.planar_step.request.a0) and
boolean-applied with OpenCascade."""

from __future__ import annotations

import tempfile
from pathlib import Path

import numpy as np

import geometer


def load_dxf_loops(path: Path, flatten_tol: float = 0.02) -> list[np.ndarray]:
    """Closed loops from a DXF (LWPOLYLINE/LINE/ARC/CIRCLE/SPLINE...), each an
    (N, 2) array, flattened with the given sagitta tolerance."""
    import ezdxf
    from ezdxf import path as ezpath

    doc = ezdxf.readfile(str(path))
    loops: list[np.ndarray] = []
    for entity in doc.modelspace():
        try:
            p = ezpath.make_path(entity)
        except Exception:
            continue
        points = np.array([(v.x, v.y) for v in p.flattening(flatten_tol)])
        if len(points) < 3:
            continue
        if np.linalg.norm(points[0] - points[-1]) < flatten_tol * 2:
            points = points[:-1]
            if len(points) >= 3:
                loops.append(points)
    return loops


def _point_in_polygon(point, polygon) -> bool:
    x, y = point
    inside = False
    j = len(polygon) - 1
    for i in range(len(polygon)):
        xi, yi = polygon[i]
        xj, yj = polygon[j]
        if (yi > y) != (yj > y) and x < (xj - xi) * (y - yi) / (yj - yi) + xi:
            inside = not inside
        j = i
    return inside


def loops_to_regions(loops: list[np.ndarray]) -> list[dict]:
    """Nest loops even-odd: a loop inside an odd number of others is a hole
    of its tightest container."""
    containers = []
    for i, loop in enumerate(loops):
        count = sum(
            1 for j, other in enumerate(loops)
            if j != i and _point_in_polygon(loop[0], other)
        )
        containers.append(count)
    regions = []
    outer_index_map = {}
    for i, loop in enumerate(loops):
        if containers[i] % 2 == 0:
            outer_index_map[i] = len(regions)
            regions.append({"outer": loop, "holes": []})
    for i, loop in enumerate(loops):
        if containers[i] % 2 == 1:
            # attach to the smallest containing outer
            best, best_area = None, np.inf
            for j in outer_index_map:
                if _point_in_polygon(loop[0], loops[j]):
                    area = float(np.ptp(loops[j][:, 0]) * np.ptp(loops[j][:, 1]))
                    if area < best_area:
                        best, best_area = j, area
            if best is not None:
                regions[outer_index_map[best]]["holes"].append(loop)
    return regions


def logo_tool_shape(
    dxf_path: Path,
    *,
    width_mm: float,
    depth_mm: float,
):
    """Build the extruded logo tool solid via geometer.planar_step, centred
    on the origin, spanning z in [0, depth_mm]. Returns a TopoDS shape."""
    from OCP.IFSelect import IFSelect_RetDone
    from OCP.STEPControl import STEPControl_Reader

    loops = load_dxf_loops(dxf_path)
    if not loops:
        raise ValueError(f"no closed loops in {dxf_path}")
    stack = np.vstack(loops)
    low, high = stack.min(axis=0), stack.max(axis=0)
    center = (low + high) / 2
    scale = width_mm / max(high[0] - low[0], 1e-9)

    def ring(points: np.ndarray) -> dict:
        scaled = (points - center) * scale
        return {
            "points": [[float(x), float(y)] for x, y in scaled],
            "segments": [{"kind": "line"}] * len(scaled),
        }

    regions = [
        {"outer": ring(region["outer"]),
         "holes": [ring(hole) for hole in region["holes"]]}
        for region in loops_to_regions(loops)
    ]
    request = {
        "schema": "geometry.planar_step.request.a0",
        "units": "mm",
        "name": "wn3d_logo_tool",
        "bodies": [{
            "id": "logo",
            "thickness_mm": float(depth_mm),
            "z_mm": 0.0,
            "fuse_regions": True,
            "regions": regions,
        }],
    }
    step_bytes = geometer.planar_step(request)
    with tempfile.TemporaryDirectory(prefix="step_editor_logo_") as temp:
        tool_path = Path(temp) / "logo.step"
        tool_path.write_bytes(step_bytes)
        reader = STEPControl_Reader()
        if reader.ReadFile(str(tool_path)) != IFSelect_RetDone:
            raise RuntimeError("could not read the geometer logo STEP back")
        reader.TransferRoots()
        return reader.OneShape()


def emboss_logo(
    document,
    body_index: int,
    face_index: int,
    dxf_path: Path,
    *,
    width_mm: float,
    depth_mm: float,
    offset_uv: tuple[float, float] = (0.0, 0.0),
    raised: bool = False,
    logo_rgb: tuple[float, float, float] | None = None,
) -> float:
    """Full pipeline: logo tool from the DXF -> placed on the picked planar
    face (sunk by depth for engrave) -> boolean cut/fuse. Returns the volume
    delta. Pure enough for the journal to replay."""
    frame = document.face_plane(body_index, face_index)
    if frame is None:
        raise ValueError("the picked face is not planar")
    tool = logo_tool_shape(dxf_path, width_mm=width_mm, depth_mm=depth_mm)
    return document.emboss(
        body_index, tool, frame, depth_mm=depth_mm,
        offset_uv=offset_uv, raised=raised, logo_rgb=logo_rgb,
    )
