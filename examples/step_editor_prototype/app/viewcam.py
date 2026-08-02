"""Camera and lighting helpers, lifted from examples/python/pyvista_hlr_viewer.py
so the editor matches the repo's established look and orthographic behavior."""

from __future__ import annotations

import math
from dataclasses import dataclass

import numpy as np
import pyvista as pv

import geometer


CAMERA_PADDING = 1.12
EDGE_FEATURE_ANGLE_DEG = 38.0


@dataclass(frozen=True)
class LightingSettings:
    key_azimuth_deg: float = -42.0
    key_elevation_deg: float = 38.0
    key_intensity: float = 0.88
    fill_intensity: float = 0.34
    head_intensity: float = 0.28
    ambient: float = 0.30
    contrast: float = 0.74


CAMERA_PRESETS = {
    "iso": ((0.72, 0.54, 1.0), (0.0, 0.0, 1.0)),
    "top": (geometer.ProjectionView.top().direction, geometer.ProjectionView.top().up),
    "bottom": (geometer.ProjectionView.bottom().direction, geometer.ProjectionView.bottom().up),
    "front": (geometer.ProjectionView.front().direction, geometer.ProjectionView.front().up),
    "back": (geometer.ProjectionView.back().direction, geometer.ProjectionView.back().up),
    "left": (geometer.ProjectionView.left().direction, geometer.ProjectionView.left().up),
    "right": (geometer.ProjectionView.right().direction, geometer.ProjectionView.right().up),
}


def material_settings(settings: LightingSettings | None = None) -> dict[str, float]:
    settings = settings or LightingSettings()
    contrast = max(0.0, min(1.0, settings.contrast))
    ambient = max(0.0, min(1.0, settings.ambient))
    return {
        "ambient": ambient,
        "diffuse": 0.52 + (contrast * 0.42),
        "specular": 0.04 + (contrast * 0.22),
        "specular_power": 12.0 + (contrast * 28.0),
        "roughness": 0.82 - (contrast * 0.34),
    }


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


def bounds_center_and_diagonal(
    bounds: tuple[float, float, float, float, float, float],
) -> tuple[np.ndarray, float]:
    min_x, max_x, min_y, max_y, min_z, max_z = bounds
    center = np.array(
        [(min_x + max_x) * 0.5, (min_y + max_y) * 0.5, (min_z + max_z) * 0.5],
        dtype=np.float64,
    )
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
    scale = max(
        vertical_extent * 0.5,
        horizontal_extent / (2.0 * max(aspect, 0.01)),
        diagonal * 0.05,
    )
    return max(scale * CAMERA_PADDING, 1.0e-6)
