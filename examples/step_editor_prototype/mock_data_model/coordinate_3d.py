"""Coordinate3D — 3D position/vector in nanometers.

The 3D analogue of data_models' Coordinate2D (core/coordinate.py): signed
int64 nanometers for lossless mm/mil/inch conversion. A conditioned 3D model
lives in millimetres in its canonical frame (Z-up, seated on z=0), so every
stored point is integer-nm in that frame.
"""

from __future__ import annotations

from dataclasses import dataclass

NM_PER_MM = 1_000_000  # 1 mm = 1,000,000 nm (exact)


@dataclass(slots=True)
class Coordinate3D:
    """3D coordinate stored as 64-bit integers in nanometers (X-right, Y-up,
    Z-out of the board — the conditioned canonical frame)."""

    x: int  # nanometers
    y: int  # nanometers
    z: int  # nanometers

    @property
    def x_mm(self) -> float:
        return self.x / NM_PER_MM

    @property
    def y_mm(self) -> float:
        return self.y / NM_PER_MM

    @property
    def z_mm(self) -> float:
        return self.z / NM_PER_MM

    def to_mm(self) -> tuple[float, float, float]:
        return (self.x_mm, self.y_mm, self.z_mm)

    @classmethod
    def from_mm(cls, x_mm: float, y_mm: float, z_mm: float) -> "Coordinate3D":
        return cls(x=round(x_mm * NM_PER_MM), y=round(y_mm * NM_PER_MM),
                   z=round(z_mm * NM_PER_MM))

    def to_json(self) -> dict[str, object]:
        return {"x": self.x, "y": self.y, "z": self.z}

    @classmethod
    def from_json(cls, data: dict[str, object]) -> "Coordinate3D":
        return cls(x=int(data["x"]), y=int(data["y"]), z=int(data["z"]))
