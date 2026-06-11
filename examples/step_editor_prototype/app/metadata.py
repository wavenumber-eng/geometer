"""wn3d.step_conditioning.a0 — the machine-readable payload embedded in every
conditioned AP242 file. A scene full of conditioned chips can be queried
straight from the 3D files: pins are geometric (centroid + hitbox), functions
live on the pins, and pins sharing a designator are the SAME NET, so their
hitboxes form one electrical node (e.g. a tact button's bridged pairs).

The generator field describes the conditioning system only — it never claims
authorship of the model."""

from __future__ import annotations

from datetime import datetime, timezone

SCHEMA = "wn3d.step_conditioning.a0"
GENERATOR = "wn3d-step-editor/0.1"


def build_metadata(document, pins=None, journal=None) -> dict:
    bodies = [
        {
            "id": index,
            "name": body.name,
            "role": body.role,
            "color": list(body.color) if body.color else None,
        }
        for index, body in enumerate(document.bodies)
    ]

    # Pins sharing a designator are one net: their hitboxes are linked into a
    # single electrical node. Unnamed pins net by their number. The special
    # INHERIT function is a per-pin flag meaning "this pin carries whatever
    # function the net has" — it stays on the pin but never becomes a net
    # function itself.
    nets: dict[str, dict] = {}
    for pin in (pins.pins if pins is not None else []):
        designator = pin.name or str(pin.number)
        net = nets.setdefault(designator, {"designator": designator,
                                           "functions": [], "pins": []})
        if (pin.function and pin.function != "INHERIT"
                and pin.function not in net["functions"]):
            net["functions"].append(pin.function)
        net["pins"].append({
            "number": pin.number,
            "name": pin.name,
            "function": pin.function,
            "kind": pin.kind,
            "role": pin.role,
            "centroid": list(pin.centroid),
            "body_ids": list(pin.body_ids),
            "face_ids": [list(item) for item in pin.face_ids],
            "hitbox": pin.hitbox,
        })

    return {
        "schema": SCHEMA,
        "generator": GENERATOR,
        "units": "mm",
        "source_file": document.path.name,
        "conditioned_utc": datetime.now(timezone.utc).isoformat(timespec="seconds"),
        "bodies": bodies,
        "nets": list(nets.values()),
        "journal": journal.to_jsonable() if journal is not None else [],
    }
