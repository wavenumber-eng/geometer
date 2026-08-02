"""model_3d_a0_body — one solid (CLOSED_SHELL) of the conditioned model.

This is the entry that finally gives a CLOSED_SHELL an identity: its `geometry_ref`
holds the literal MANIFOLD_SOLID_BREP / CLOSED_SHELL `#` ids, and `role` says
whether the shell is the package or a pin. A pin body's `pin_ref` points back to
the Model3dPin it realises, closing the CLOSED_SHELL ⇄ pin ⇄ hitbox loop.
"""

from __future__ import annotations

from dataclasses import dataclass, field
from typing import ClassVar

from .entity_ref import EntityRef


@dataclass
class Model3dBody:
    TYPE: ClassVar[str] = "model_3d_a0_body"

    id: str = ""
    name: str = ""
    role: str = ""               # package | pin | ...
    color: list[float] | None = None
    pin_ref: str = ""            # Model3dPin id when this body IS a pin
    geometry_ref: EntityRef | None = None
    metadata: dict[str, object] = field(default_factory=dict)

    def to_json(self) -> dict[str, object]:
        data: dict[str, object] = {"type": self.TYPE, "id": self.id}
        for name in ("name", "role", "pin_ref"):
            value = getattr(self, name)
            if value:
                data[name] = value
        if self.color is not None:
            data["color"] = list(self.color)
        if self.geometry_ref is not None:
            data["geometry_ref"] = self.geometry_ref.to_json()
        if self.metadata:
            data["metadata"] = self.metadata
        return data

    @classmethod
    def from_json(cls, data: dict[str, object]) -> "Model3dBody":
        color = data.get("color")
        return cls(
            id=str(data.get("id", "") or ""),
            name=str(data.get("name", "") or ""),
            role=str(data.get("role", "") or ""),
            color=[float(c) for c in color] if isinstance(color, list) else None,
            pin_ref=str(data.get("pin_ref", "") or ""),
            geometry_ref=EntityRef.from_json(data.get("geometry_ref"))
            if data.get("geometry_ref") is not None else None,
            metadata=data.get("metadata", {}) or {},
        )
