"""model_3d_a0_hitbox — a pin's physical connection volume (metadata-only).

A hitbox is the region any other program intersection-tests to decide whether a
pin is physically connected to something. Male pins get a box around them,
female contacts a cavity box a male pin sits in, BGA balls a small cube, ground
pads a flat prism, curved pins a convex hull. It links to its owning pin
(`owner_ref`) and to the B-rep it bounds (`geometry_ref`).
"""

from __future__ import annotations

from dataclasses import dataclass, field
from typing import ClassVar

from .entity_ref import EntityRef
from .obb import Obb


@dataclass
class Model3dHitbox:
    TYPE: ClassVar[str] = "model_3d_a0_hitbox"

    id: str = ""
    owner_ref: str = ""          # the Model3dPin id this hitbox belongs to
    kind: str = "male_box"       # male_box | female_cavity | bga_cube | pad_prism | convex_hull
    obb: Obb | None = None       # set for box-shaped kinds
    hull_points: list[list[int]] = field(default_factory=list)  # nm verts, convex_hull kind
    geometry_ref: EntityRef | None = None
    metadata: dict[str, object] = field(default_factory=dict)

    def to_json(self) -> dict[str, object]:
        data: dict[str, object] = {"type": self.TYPE, "id": self.id, "kind": self.kind}
        if self.owner_ref:
            data["owner_ref"] = self.owner_ref
        if self.obb is not None:
            data["obb"] = self.obb.to_json()
        if self.hull_points:
            data["hull_points"] = [list(p) for p in self.hull_points]
        if self.geometry_ref is not None:
            data["geometry_ref"] = self.geometry_ref.to_json()
        if self.metadata:
            data["metadata"] = self.metadata
        return data

    @classmethod
    def from_json(cls, data: dict[str, object]) -> "Model3dHitbox":
        return cls(
            id=str(data.get("id", "") or ""),
            owner_ref=str(data.get("owner_ref", "") or ""),
            kind=str(data.get("kind", "male_box") or "male_box"),
            obb=Obb.from_json(data["obb"]) if isinstance(data.get("obb"), dict) else None,
            hull_points=[[int(c) for c in p] for p in (data.get("hull_points", []) or [])],
            geometry_ref=EntityRef.from_json(data.get("geometry_ref"))
            if data.get("geometry_ref") is not None else None,
            metadata=data.get("metadata", {}) or {},
        )
