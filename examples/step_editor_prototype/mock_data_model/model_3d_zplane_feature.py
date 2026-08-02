"""model_3d_a0_zplane_feature — a pin's footprint primitive at the seating plane.

The bridge from the 3D model to a 2D FOOTPRINT: where a pin meets the board
(z=0). For a THR pin it is the drill hole (centre + diameter); for an SMT pin it
is the land/pad (centre + size + shape). Cross-sectioning every z-plane feature
at z=0 reconstructs the IPC footprint.

NOTE: how we actually derive these centres/sizes from the B-rep is deferred —
this primitive only fixes the SHAPE of the data so the rest of the contract can
be built around it.
"""

from __future__ import annotations

from dataclasses import dataclass, field
from typing import ClassVar

from .coordinate_3d import Coordinate3D
from .entity_ref import EntityRef


@dataclass
class Model3dZPlaneFeature:
    TYPE: ClassVar[str] = "model_3d_a0_zplane_feature"

    id: str = ""
    owner_ref: str = ""           # the Model3dPin id this feature belongs to
    kind: str = ""                # drill_hole (THR) | pad (SMT) | ...
    center: Coordinate3D | None = None   # at z=0 in the canonical frame
    # drill_hole
    drill_diameter_nm: int | None = None
    plated: bool | None = None
    # pad
    pad_shape: str = ""           # round | rect | roundrect | oval | ...
    pad_size_x_nm: int | None = None
    pad_size_y_nm: int | None = None
    rotation_z_deg: float = 0.0
    geometry_ref: EntityRef | None = None
    metadata: dict[str, object] = field(default_factory=dict)

    def to_json(self) -> dict[str, object]:
        data: dict[str, object] = {"type": self.TYPE, "id": self.id, "kind": self.kind}
        if self.owner_ref:
            data["owner_ref"] = self.owner_ref
        if self.center is not None:
            data["center"] = self.center.to_json()
        for name in ("drill_diameter_nm", "plated", "pad_size_x_nm", "pad_size_y_nm"):
            value = getattr(self, name)
            if value is not None:
                data[name] = value
        if self.pad_shape:
            data["pad_shape"] = self.pad_shape
        if self.rotation_z_deg:
            data["rotation_z_deg"] = self.rotation_z_deg
        if self.geometry_ref is not None:
            data["geometry_ref"] = self.geometry_ref.to_json()
        if self.metadata:
            data["metadata"] = self.metadata
        return data

    @classmethod
    def from_json(cls, data: dict[str, object]) -> "Model3dZPlaneFeature":
        center = data.get("center")
        return cls(
            id=str(data.get("id", "") or ""),
            owner_ref=str(data.get("owner_ref", "") or ""),
            kind=str(data.get("kind", "") or ""),
            center=Coordinate3D.from_json(center) if isinstance(center, dict) else None,
            drill_diameter_nm=(int(data["drill_diameter_nm"])
                               if data.get("drill_diameter_nm") is not None else None),
            plated=(bool(data["plated"]) if data.get("plated") is not None else None),
            pad_shape=str(data.get("pad_shape", "") or ""),
            pad_size_x_nm=(int(data["pad_size_x_nm"])
                           if data.get("pad_size_x_nm") is not None else None),
            pad_size_y_nm=(int(data["pad_size_y_nm"])
                           if data.get("pad_size_y_nm") is not None else None),
            rotation_z_deg=float(data.get("rotation_z_deg", 0.0) or 0.0),
            geometry_ref=EntityRef.from_json(data.get("geometry_ref"))
            if data.get("geometry_ref") is not None else None,
            metadata=data.get("metadata", {}) or {},
        )
