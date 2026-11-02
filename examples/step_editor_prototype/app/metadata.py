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


STEP_IDS = ["repair", "identity", "zsit", "detect_pins", "front", "separate",
            "hitboxes", "pin_functions", "footprint_embed", "colors"]
# "deferred": left for the automatic pass (Steps settings), not done by hand.
# "not_marked": never applied and never marked: possibly forgotten, so it does
# NOT count as passed (the GUI asks before exporting with any).
STEP_STATES = ("applied", "inherited", "omitted_by_choice", "deferred")


def step_status(document, journal) -> dict[str, str]:
    """Per-step status: applied when the journal has the step (the record
    wins), else the user's mark (inherited / omitted_by_choice / deferred),
    else not_marked."""
    marks = dict(getattr(document, "step_status", None) or {})
    for step in getattr(document, "auto_steps", None) or ():
        marks.setdefault(step, "deferred")
    used = journal.tools_used() if journal is not None else set()
    return {step: "applied" if step in used
            else marks.get(step) if marks.get(step) in STEP_STATES[1:]
            else "not_marked"
            for step in STEP_IDS}


def _step_records(document) -> dict:
    """The per-step records kept on the document (empty dict when not done)."""
    from .restore import STEP_RECORDS

    out = {key: dict(getattr(document, attribute, None) or {}) for key, attribute in STEP_RECORDS.items()}
    out["repair"] = {str(k): v for k, v in out["repair"].items()}
    return out


def build_metadata(document, pins=None, journal=None, source_name=None) -> dict:
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
    from .pins import specialty

    nets: dict[str, dict] = {}
    mechanical = []
    for pin in (pins.pins if pins is not None else []):
        if specialty(pin) == "guide":
            # A GUIDE peg is mechanical only: no net, never a hitbox.
            mechanical.append({
                "number": pin.number, "name": pin.name, "kind": "guide", "role": pin.role,
                "centroid": list(pin.centroid), "body_ids": list(pin.body_ids),
                "face_ids": [list(item) for item in pin.face_ids],
            })
            continue
        designator = pin.name or str(pin.number)
        net = nets.setdefault(designator, {"designator": designator,
                                           "functions": [], "pins": []})
        if (pin.function and pin.function != "INHERIT"
                and pin.function not in net["functions"]):
            net["functions"].append(pin.function)
        net["pins"].append({
            "number": pin.number,
            "name": pin.name,
            # where the name came from: typed (anchor) / footprint names steer
            # prediction after a reopen; predicted / renumbered ones do not
            "name_source": getattr(pin, "name_source", ""),
            "function": pin.function,
            "signal": getattr(pin, "signal", ""),
            "kind": specialty(pin) or pin.kind,
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
        # Provenance: the file the user actually opened (a vendor container
        # extracts to a temp STEP, which is not the real source).
        "source_file": source_name or document.path.name,
        "conditioned_utc": datetime.now(timezone.utc).isoformat(timespec="seconds"),
        "bodies": bodies,
        "nets": list(nets.values()),
        "mechanical_pins": mechanical,
        # Hand cut volumes from Separate (box / cylinder + the pin each carved):
        # ground truth for a split algorithm or an ML model proposing cuts.
        "separate_cuts": list(getattr(document, "separate_cuts", None) or []),
        # Pass semantics: an exported file PASSED all 10 steps. Each step was
        # applied (journaled), inherited (already satisfied by the source) or
        # omitted by choice; all three count as passed. "demonstrates" (the
        # journaled steps) is the separate ML-training signal.
        "step_status": step_status(document, journal),
        "demonstrates": sorted(journal.tools_used() & set(STEP_IDS)) if journal is not None else [],
        # Steps 1, 2, 9 (absent when the step wasn't done)
        # Steps 1, 2, 9, 10 records; step 10's is the reference photo of the real
        # part if one was loaded (hash, where it came from, its palette; no pixels)
        **_step_records(document),
        "journal": journal.to_jsonable() if journal is not None else [],
    }
