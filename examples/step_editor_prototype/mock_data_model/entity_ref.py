"""EntityRef — the geometry-linking primitive.

THE contract gap this fixes: today's embedded metadata links a pin to geometry
only by *positional index* (body N, face M) + centroid. Nothing ties it to the
STEP file's own entity graph, so a CLOSED_SHELL has no idea it is a pin, and a
hitbox has no anchor in the B-rep.

An EntityRef carries BOTH:
  • the STEP entity ids it resolves to — the literal ``#NNN`` labels of the
    MANIFOLD_SOLID_BREP / CLOSED_SHELL / ADVANCED_FACE that make up the feature
    (the strong, file-native link the user wants), and
  • the index fallback — (body_index, [face_index, ...]) in OCCT map order —
    which survives a rigid re-export even when the ``#`` ids are renumbered, and
    is what the editor can populate today before the STEP-injection step exists.

Either layer can be empty; populating the ``#`` ids is a later export step.
"""

from __future__ import annotations

from dataclasses import dataclass, field


@dataclass
class EntityRef:
    """A reference from a data-model object to its geometry in the STEP file."""

    # --- strong link: literal STEP entity ids (e.g. "#147876") ---
    solid_id: str = ""          # the MANIFOLD_SOLID_BREP
    shell_id: str = ""          # its CLOSED_SHELL
    face_ids: list[str] = field(default_factory=list)   # ADVANCED_FACE ids

    # --- index fallback: OCCT map order (deterministic for a fixed shape) ---
    body_index: int | None = None
    face_indices: list[int] = field(default_factory=list)

    def to_json(self) -> dict[str, object]:
        data: dict[str, object] = {}
        if self.solid_id:
            data["solid_id"] = self.solid_id
        if self.shell_id:
            data["shell_id"] = self.shell_id
        if self.face_ids:
            data["face_ids"] = list(self.face_ids)
        if self.body_index is not None:
            data["body_index"] = self.body_index
        if self.face_indices:
            data["face_indices"] = list(self.face_indices)
        return data

    @classmethod
    def from_json(cls, data: dict[str, object] | None) -> "EntityRef":
        data = data or {}
        return cls(
            solid_id=str(data.get("solid_id", "") or ""),
            shell_id=str(data.get("shell_id", "") or ""),
            face_ids=[str(item) for item in (data.get("face_ids", []) or [])],
            body_index=(int(data["body_index"])
                        if data.get("body_index") is not None else None),
            face_indices=[int(item) for item in (data.get("face_indices", []) or [])],
        )
