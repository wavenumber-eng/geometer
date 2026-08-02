"""model_3d_a0_{thr,smt,con,head}_pin — a geometric pin on the 3D model.

A pin is a contact realised in the B-rep. Its `kind` selects the schema type
string the user enumerated:

    THR  -> model_3d_a0_thr_pin    (through-hole lead → drill-hole z-feature)
    SMT  -> model_3d_a0_smt_pin    (surface-mount land → pad z-feature)
    CON  -> model_3d_a0_con_pin    (connector mating contact)
    HEAD -> model_3d_a0_head_pin   (header/mouth contact)

One dataclass with a `kind` field rather than four near-identical classes;
split into per-kind modules later if a kind grows kind-specific fields.

Linking: `geometry_ref` ties the pin to its CLOSED_SHELL / faces in the STEP;
`hitbox_ref` and `zplane_feature_ref` point to the owned primitives;
`net_ref`/`net_name` group pins sharing a designator into one electrical node.
"""

from __future__ import annotations

from dataclasses import dataclass, field
from typing import ClassVar

from .coordinate_3d import Coordinate3D
from .entity_ref import EntityRef

KIND_TO_TYPE = {
    "THR": "model_3d_a0_thr_pin",
    "SMT": "model_3d_a0_smt_pin",
    "CON": "model_3d_a0_con_pin",
    "HEAD": "model_3d_a0_head_pin",
}
TYPE_TO_KIND = {v: k for k, v in KIND_TO_TYPE.items()}


@dataclass
class Model3dPin:
    SCHEMA_FAMILY: ClassVar[str] = "model_3d_a0_pin"

    id: str = ""
    number: int = 0
    designator: str = ""         # net key — pins sharing it are one net
    kind: str = "SMT"            # THR | SMT | CON | HEAD
    function: str = ""           # GND, VCC, CLK, ... (or INHERIT)
    net_ref: str = ""
    net_name: str = ""
    centroid: Coordinate3D | None = None
    geometry_ref: EntityRef | None = None    # the pin's CLOSED_SHELL / faces
    hitbox_ref: str = ""                     # Model3dHitbox id
    zplane_feature_ref: str = ""             # Model3dZPlaneFeature id
    metadata: dict[str, object] = field(default_factory=dict)

    @property
    def type(self) -> str:
        return KIND_TO_TYPE.get(self.kind, self.SCHEMA_FAMILY)

    def to_json(self) -> dict[str, object]:
        data: dict[str, object] = {
            "type": self.type,
            "id": self.id,
            "number": self.number,
            "designator": self.designator,
            "kind": self.kind,
        }
        for name in ("function", "net_ref", "net_name", "hitbox_ref",
                     "zplane_feature_ref"):
            value = getattr(self, name)
            if value:
                data[name] = value
        if self.centroid is not None:
            data["centroid"] = self.centroid.to_json()
        if self.geometry_ref is not None:
            data["geometry_ref"] = self.geometry_ref.to_json()
        if self.metadata:
            data["metadata"] = self.metadata
        return data

    @classmethod
    def from_json(cls, data: dict[str, object]) -> "Model3dPin":
        kind = str(data.get("kind", "") or "")
        if not kind:
            kind = TYPE_TO_KIND.get(str(data.get("type", "") or ""), "SMT")
        centroid = data.get("centroid")
        return cls(
            id=str(data.get("id", "") or ""),
            number=int(data.get("number", 0) or 0),
            designator=str(data.get("designator", "") or ""),
            kind=kind,
            function=str(data.get("function", "") or ""),
            net_ref=str(data.get("net_ref", "") or ""),
            net_name=str(data.get("net_name", "") or ""),
            centroid=Coordinate3D.from_json(centroid) if isinstance(centroid, dict) else None,
            geometry_ref=EntityRef.from_json(data.get("geometry_ref"))
            if data.get("geometry_ref") is not None else None,
            hitbox_ref=str(data.get("hitbox_ref", "") or ""),
            zplane_feature_ref=str(data.get("zplane_feature_ref", "") or ""),
            metadata=data.get("metadata", {}) or {},
        )
