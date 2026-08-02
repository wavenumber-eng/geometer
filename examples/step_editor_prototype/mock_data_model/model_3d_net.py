"""model_3d_a0_net — pins sharing a designator as one electrical node.

Mirrors data_models' PcbNet: a net references its member pins (here by pin id)
and carries the net-level functions. A hitbox intersection on any member pin
resolves to this net, so the whole node is one connection point.
"""

from __future__ import annotations

from dataclasses import dataclass, field
from typing import ClassVar


@dataclass
class Model3dNet:
    TYPE: ClassVar[str] = "model_3d_a0_net"

    id: str = ""
    designator: str = ""
    functions: list[str] = field(default_factory=list)
    pin_refs: list[str] = field(default_factory=list)   # Model3dPin ids
    metadata: dict[str, object] = field(default_factory=dict)

    def to_json(self) -> dict[str, object]:
        data: dict[str, object] = {
            "type": self.TYPE,
            "id": self.id,
            "designator": self.designator,
            "pin_refs": list(self.pin_refs),
        }
        if self.functions:
            data["functions"] = list(self.functions)
        if self.metadata:
            data["metadata"] = self.metadata
        return data

    @classmethod
    def from_json(cls, data: dict[str, object]) -> "Model3dNet":
        return cls(
            id=str(data.get("id", "") or ""),
            designator=str(data.get("designator", "") or ""),
            functions=[str(f) for f in (data.get("functions", []) or [])],
            pin_refs=[str(r) for r in (data.get("pin_refs", []) or [])],
            metadata=data.get("metadata", {}) or {},
        )
