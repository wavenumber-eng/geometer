"""model_3d_a0 — the root conditioned-3D-model data model.

The single object embedded in a conditioned AP242 (replacing the ad-hoc
wn3d.step_conditioning.a0 blob). It is the MODEL leg of the PCB-entity trio
(footprint + schematic + model).

Deliberately has NO journal field: the operation journal is an *authoring*
artifact (how the model was made), not part of the cleaned model the file
ships with. Journals live in a separate schema / sidecar and are stripped from
the final export.

Linking is by id everywhere (`*_ref` → object `id`); geometry links reach the
STEP entity graph through each object's `EntityRef` (`#` ids + index fallback).
"""

from __future__ import annotations

from dataclasses import dataclass, field
from typing import ClassVar

from .model_3d_body import Model3dBody
from .model_3d_hitbox import Model3dHitbox
from .model_3d_net import Model3dNet
from .model_3d_pin import Model3dPin
from .model_3d_zplane_feature import Model3dZPlaneFeature

SCHEMA = "model_3d_a0"


@dataclass
class Model3d:
    TYPE: ClassVar[str] = SCHEMA

    id: str = ""
    units: str = "mm"
    source_file: str = ""        # the file the user opened (provenance only)
    generator: str = ""          # the conditioning system (never claims authorship)
    # canonical frame is implicit (Z-up, seated on z=0); these record the result
    standoff_nm: int | None = None      # seat height above z=0
    bbox_min: list[int] | None = None   # overall envelope, nm
    bbox_max: list[int] | None = None
    bodies: list[Model3dBody] = field(default_factory=list)
    pins: list[Model3dPin] = field(default_factory=list)
    nets: list[Model3dNet] = field(default_factory=list)
    hitboxes: list[Model3dHitbox] = field(default_factory=list)
    zplane_features: list[Model3dZPlaneFeature] = field(default_factory=list)
    metadata: dict[str, object] = field(default_factory=dict)

    @property
    def type(self) -> str:
        return self.TYPE

    def to_json(self) -> dict[str, object]:
        data: dict[str, object] = {
            "type": self.TYPE,
            "id": self.id,
            "units": self.units,
            "bodies": [b.to_json() for b in self.bodies],
            "pins": [p.to_json() for p in self.pins],
            "nets": [n.to_json() for n in self.nets],
            "hitboxes": [h.to_json() for h in self.hitboxes],
            "zplane_features": [z.to_json() for z in self.zplane_features],
        }
        for name in ("source_file", "generator"):
            value = getattr(self, name)
            if value:
                data[name] = value
        if self.standoff_nm is not None:
            data["standoff_nm"] = self.standoff_nm
        if self.bbox_min is not None and self.bbox_max is not None:
            data["bbox_min"] = list(self.bbox_min)
            data["bbox_max"] = list(self.bbox_max)
        if self.metadata:
            data["metadata"] = self.metadata
        return data

    @classmethod
    def from_json(cls, data: dict[str, object]) -> "Model3d":
        if str(data.get("type", "") or "") not in ("", SCHEMA):
            raise ValueError(f"not a {SCHEMA} payload: type={data.get('type')!r}")
        return cls(
            id=str(data.get("id", "") or ""),
            units=str(data.get("units", "mm") or "mm"),
            source_file=str(data.get("source_file", "") or ""),
            generator=str(data.get("generator", "") or ""),
            standoff_nm=(int(data["standoff_nm"])
                         if data.get("standoff_nm") is not None else None),
            bbox_min=[int(c) for c in data["bbox_min"]] if data.get("bbox_min") else None,
            bbox_max=[int(c) for c in data["bbox_max"]] if data.get("bbox_max") else None,
            bodies=[Model3dBody.from_json(b) for b in (data.get("bodies", []) or [])],
            pins=[Model3dPin.from_json(p) for p in (data.get("pins", []) or [])],
            nets=[Model3dNet.from_json(n) for n in (data.get("nets", []) or [])],
            hitboxes=[Model3dHitbox.from_json(h) for h in (data.get("hitboxes", []) or [])],
            zplane_features=[Model3dZPlaneFeature.from_json(z)
                             for z in (data.get("zplane_features", []) or [])],
            metadata=data.get("metadata", {}) or {},
        )
