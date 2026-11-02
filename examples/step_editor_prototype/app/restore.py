"""Resume a conditioned model: read the wn3d.step_conditioning block embedded
by a previous export (or a crash-recovery journal checkpoint) back into live
editor state, so a part can be reopened and continued instead of redone.

Pins are rebuilt from the exported nets. Face ids survive the AP242 round trip
in practice; when one doesn't (its mesh centroid no longer matches the stored
pin centroid), the pin is re-pointed by geometry like Separate does."""

from __future__ import annotations

import json
from pathlib import Path

import numpy as np

from .journal import Journal, Operation
from .pins import Pin, PinRegistry, mesh_region_centroid, remap_pin_faces

CHECKPOINT_SUFFIX = ".wn3d-journal.json"



# metadata key -> document attribute for the per-step records written by
# metadata.build_metadata (kept here so both sides agree on the names)
STEP_RECORDS = {"repair": "repair_log", "identity": "identity", "footprint": "footprint",
                "reference_photo": "reference_photo"}

def pins_from_metadata(meta: dict) -> list[Pin]:
    pins = []
    for net in meta.get("nets", []):
        for item in net.get("pins", []):
            pins.append(Pin(
                number=int(item.get("number", 0)),
                centroid=tuple(item.get("centroid", (0.0, 0.0, 0.0))),
                body_ids=[int(b) for b in item.get("body_ids", [])],
                face_ids=[(int(b), int(f)) for b, f in item.get("face_ids", [])],
                kind=item.get("kind", "pin"),
                role=item.get("role", "primary"),
                name=item.get("name", ""),
                # files from before name_source was recorded: every name is
                # treated as typed
                name_source=item.get("name_source", "anchor" if item.get("name") else ""),
                function=item.get("function", ""),
                signal=item.get("signal", ""),
                hitbox=item.get("hitbox"),
            ))
    for item in meta.get("mechanical_pins", []):
        pins.append(Pin(
            number=int(item.get("number", 0)),
            centroid=tuple(item.get("centroid", (0.0, 0.0, 0.0))),
            body_ids=[int(b) for b in item.get("body_ids", [])],
            face_ids=[(int(b), int(f)) for b, f in item.get("face_ids", [])],
            kind=item.get("kind", "guide"), role=item.get("role", "primary"),
            name=item.get("name", ""), name_source="anchor", hitbox=item.get("hitbox"),
        ))
    pins.sort(key=lambda p: p.number)
    return pins


def _faces_valid(document, pin: Pin) -> bool:
    """The pin's stored (body, face) ids still exist in the reloaded model."""
    if not pin.face_ids and not pin.body_ids:
        return False
    for body_index in pin.body_ids:
        if not 0 <= body_index < len(document.bodies):
            return False
    for body_index, face_index in pin.face_ids:
        if not 0 <= body_index < len(document.bodies):
            return False
        mesh = document.bodies[body_index].mesh
        if mesh is None or not 1 <= face_index <= mesh.face_count:
            return False
    return True


def _geometric_centroid(document, pin: Pin):
    """A pin's centroid from its own geometry (faces, else whole bodies)."""
    points = []
    for body_index, face_index in pin.face_ids:
        c = mesh_region_centroid(document.bodies[body_index].mesh, [face_index])
        if c is not None:
            points.append(c)
    if not points:
        for body_index in pin.body_ids:
            mesh = document.bodies[body_index].mesh
            if mesh is not None and len(mesh.points):
                points.append(mesh.points.mean(axis=0))
    return tuple(float(v) for v in np.mean(points, axis=0)) if points else None


def restore_state(document, meta: dict) -> dict:
    """Apply an embedded conditioning payload to a freshly loaded document.
    Returns {"pins": PinRegistry, "journal": Journal, "remapped": n, "lost": n}."""
    b = document.bounds()
    tolerance = float(np.linalg.norm([b[1] - b[0], b[3] - b[2], b[5] - b[4]])) * 5.0e-3
    pins = pins_from_metadata(meta)
    # Face ids survive the AP242 round trip; the stored centroid may not (files
    # exported before pins moved with Front / Z-Sit carry stale ones). So trust
    # valid face ids and recompute the centroid from them; only a pin whose
    # faces no longer exist is re-found by its stored position.
    bad = [p for p in pins if not _faces_valid(document, p)]
    remapped = 0
    if bad:
        anchors = [(p, [tuple(p.centroid)]) for p in bad if p.face_ids]
        for p in bad:
            p.body_ids = [i for i in p.body_ids if 0 <= i < len(document.bodies)]
        remapped = remap_pin_faces(document, anchors, tolerance * 4)
    for p in pins:
        if _faces_valid(document, p):
            centre = _geometric_centroid(document, p)
            if centre is not None:
                p.centroid = centre
    lost = sum(1 for p in bad if not p.body_ids and not p.face_ids)

    registry = PinRegistry()
    registry.pins = pins
    registry._renumber()

    for index, body in enumerate(meta.get("bodies", [])):
        if index < len(document.bodies) and body.get("role"):
            document.bodies[index].role = body["role"]
    from .pins import apply_pin_body_names, link_own_solids, link_pin_pieces

    linked = link_pin_pieces(document, pins)  # files split before pieces were linked
    if link_own_solids(document, pins) + linked:  # leads that were their own solids all along
        apply_pin_body_names(document, pins)
    for key, attribute in STEP_RECORDS.items():  # steps 1, 2, 9, 10 (absent when not done)
        if meta.get(key):
            setattr(document, attribute, meta[key])
    if meta.get("separate_cuts"):
        document.separate_cuts = list(meta["separate_cuts"])
    if meta.get("step_status"):
        document.step_status = dict(meta["step_status"])

    journal = Journal()
    for item in meta.get("journal", []):
        journal.operations.append(Operation(item["tool"], item.get("params", {}),
                                            item.get("inputs", {}), item.get("result", {})))
    return {"pins": registry, "journal": journal, "remapped": remapped, "lost": lost}


# ------------------------------------------------------- crash recovery

def checkpoint_path(source: Path) -> Path:
    """The autosave journal lives next to the file the user opened."""
    return source.with_name(source.name + CHECKPOINT_SUFFIX)


def write_checkpoint(source: Path, journal: Journal, extra: dict | None = None) -> Path | None:
    path = checkpoint_path(source)
    payload = {"source": source.name, "journal": journal.to_jsonable(), **(extra or {})}
    tmp = path.with_name(path.name + ".tmp")
    try:
        tmp.write_text(json.dumps(payload), encoding="utf-8")
        tmp.replace(path)
        return path
    except OSError:
        return None  # read-only folder: autosave is best-effort


def read_checkpoint(source: Path) -> dict | None:
    path = checkpoint_path(source)
    if not path.is_file():
        return None
    try:
        data = json.loads(path.read_text(encoding="utf-8"))
    except (OSError, json.JSONDecodeError):
        return None
    return data if data.get("journal") else None


def clear_checkpoint(source: Path) -> None:
    try:
        checkpoint_path(source).unlink(missing_ok=True)
    except OSError:
        pass
