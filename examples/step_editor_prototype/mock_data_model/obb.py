"""Obb — an oriented bounding box, the default hitbox volume.

Centre + half-extents + a rotation about Z (the conditioned part is Z-up, so a
single in-plane angle covers the common male-pin / pad / cavity boxes). A
convex-hull hitbox (curved pins) is carried separately as a vertex list on the
hitbox itself; this primitive is the box case.
"""

from __future__ import annotations

from dataclasses import dataclass

from .coordinate_3d import Coordinate3D


@dataclass
class Obb:
    """Oriented box: half-extents (nm) about a centre, rotated `rotation_z_deg`."""

    center: Coordinate3D
    half_x_nm: int = 0
    half_y_nm: int = 0
    half_z_nm: int = 0
    rotation_z_deg: float = 0.0

    def to_json(self) -> dict[str, object]:
        return {
            "center": self.center.to_json(),
            "half_x_nm": self.half_x_nm,
            "half_y_nm": self.half_y_nm,
            "half_z_nm": self.half_z_nm,
            "rotation_z_deg": self.rotation_z_deg,
        }

    @classmethod
    def from_json(cls, data: dict[str, object]) -> "Obb":
        return cls(
            center=Coordinate3D.from_json(data["center"]),
            half_x_nm=int(data.get("half_x_nm", 0) or 0),
            half_y_nm=int(data.get("half_y_nm", 0) or 0),
            half_z_nm=int(data.get("half_z_nm", 0) or 0),
            rotation_z_deg=float(data.get("rotation_z_deg", 0.0) or 0.0),
        )
