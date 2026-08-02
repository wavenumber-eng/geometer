"""Hitbox math: metadata-only volumes that declare where a pin electrically
meets the world (the design intent's 'IPC footprint in 3D'). Two shapes:

  obb         {type, center[3], half_extents[3], rotation_z_deg}
  convex_hull {type, vertices[[x,y,z], ...]}

All pure functions — the GUI only collects clicks and parameters."""

from __future__ import annotations

import math

import numpy as np


def obb_from_three_clicks(
    p1,
    p2,
    p3,
    *,
    z_range: tuple[float, float],
    margin: float = 0.0,
) -> dict:
    """p1 -> p2 is the base edge (length + Z rotation), p3 the opposite
    corner (width). Height comes from z_range (usually the pin's Z extent)."""
    a = np.asarray(p1, dtype=np.float64)
    b = np.asarray(p2, dtype=np.float64)
    c = np.asarray(p3, dtype=np.float64)

    u = b[:2] - a[:2]
    length = float(np.linalg.norm(u))
    if length < 1.0e-9:
        raise ValueError("first two clicks coincide in XY")
    u /= length
    perp = np.array([-u[1], u[0]])
    width = float((c[:2] - a[:2]) @ perp)

    center_xy = a[:2] + u * (length * 0.5) + perp * (width * 0.5)
    z_min, z_max = min(z_range), max(z_range)
    return {
        "type": "obb",
        "center": [float(center_xy[0]), float(center_xy[1]), (z_min + z_max) * 0.5],
        "half_extents": [
            length * 0.5 + margin,
            abs(width) * 0.5 + margin,
            (z_max - z_min) * 0.5 + margin,
        ],
        "rotation_z_deg": math.degrees(math.atan2(u[1], u[0])),
    }


def obb_from_points(
    points: np.ndarray,
    *,
    margin: float = 0.0,
    height_override: float | None = None,
    flat_height: float | None = None,
) -> dict:
    """Axis-aligned box around a point set (auto-box / BGA cube / pad prism).
    flat_height clamps the box to a thin prism rising from the point minimum
    (ground pads); height_override forces a symmetric height about center."""
    points = np.asarray(points, dtype=np.float64)
    low = points.min(axis=0)
    high = points.max(axis=0)
    center = (low + high) * 0.5
    half = (high - low) * 0.5 + margin

    if flat_height is not None:
        center[2] = low[2] + flat_height * 0.5
        half[2] = flat_height * 0.5
    elif height_override is not None:
        half[2] = height_override * 0.5

    return {
        "type": "obb",
        "center": [float(v) for v in center],
        "half_extents": [float(v) for v in half],
        "rotation_z_deg": 0.0,
    }


def cube_from_point(center, size: float) -> dict:
    """BGA ball cube centered on a click."""
    center = np.asarray(center, dtype=np.float64)
    half = max(size, 1.0e-9) * 0.5
    return {
        "type": "obb",
        "center": [float(v) for v in center],
        "half_extents": [half, half, half],
        "rotation_z_deg": 0.0,
    }


def convex_hull_hitbox(points: np.ndarray, *, margin: float = 0.0, max_vertices: int = 64) -> dict:
    """Convex hull of the pin's mesh vertices, dilated by margin from the
    hull centroid (curved pins)."""
    import trimesh

    points = np.asarray(points, dtype=np.float64)
    hull = trimesh.PointCloud(points).convex_hull
    vertices = np.asarray(hull.vertices, dtype=np.float64)
    if margin > 0.0:
        centroid = vertices.mean(axis=0)
        offsets = vertices - centroid
        norms = np.linalg.norm(offsets, axis=1, keepdims=True)
        norms[norms < 1.0e-12] = 1.0
        vertices = vertices + offsets / norms * margin
        hull = trimesh.PointCloud(vertices).convex_hull
        vertices = np.asarray(hull.vertices, dtype=np.float64)
    if len(vertices) > max_vertices:
        simplified = hull.simplify_quadric_decimation(face_count=max_vertices * 2 - 4)
        if len(simplified.vertices) >= 4:
            vertices = np.asarray(simplified.convex_hull.vertices, dtype=np.float64)
    return {"type": "convex_hull", "vertices": [[float(v) for v in row] for row in vertices]}


def obb_contains(hitbox: dict, points: np.ndarray, *, tolerance: float = 1.0e-9) -> np.ndarray:
    """Boolean mask: which points lie inside the OBB."""
    points = np.asarray(points, dtype=np.float64)
    center = np.asarray(hitbox["center"], dtype=np.float64)
    half = np.asarray(hitbox["half_extents"], dtype=np.float64)
    angle = math.radians(-float(hitbox.get("rotation_z_deg", 0.0)))
    cos_a, sin_a = math.cos(angle), math.sin(angle)
    local = points - center
    x = local[:, 0] * cos_a - local[:, 1] * sin_a
    y = local[:, 0] * sin_a + local[:, 1] * cos_a
    z = local[:, 2]
    return (
        (np.abs(x) <= half[0] + tolerance)
        & (np.abs(y) <= half[1] + tolerance)
        & (np.abs(z) <= half[2] + tolerance)
    )


def obb_xy_cross_section(hitbox: dict) -> tuple[float, float, float, float, float]:
    """(cx, cy, half_w, half_h, rotation_deg) for 2D footprint drawing."""
    center = hitbox["center"]
    half = hitbox["half_extents"]
    return (
        float(center[0]),
        float(center[1]),
        float(half[0]),
        float(half[1]),
        float(hitbox.get("rotation_z_deg", 0.0)),
    )
