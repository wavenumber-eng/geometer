"""Headless journal replay — the seed of the fully automatic conditioner.
Every tool records its executions as declarative JSON; this module re-applies
them to a freshly loaded document without any GUI:

    step_editor.py --apply ops.json input.step

The loop is a dispatch table: one handler per tool, and detect_pins fans out
again to one handler per recorded action. Handlers share a small mutable
_ReplayState (document, registry, the separate undo snapshot)."""

from __future__ import annotations

import math
from pathlib import Path

import numpy as np

from .document import EditorDocument
from .journal import Journal
from .pins import (
    Band,
    Pin,
    PinRegistry,
    detect_pins_by_section,
    detect_pins_multibody,
    detect_pins_unibody,
    grow_pin_regions,
    grow_smooth_region,
    mesh_region_centroid,
    order_pins,
)


def _rotation_z(angle_deg: float) -> np.ndarray:
    c, s = math.cos(math.radians(angle_deg)), math.sin(math.radians(angle_deg))
    matrix = np.eye(4)
    matrix[:2, :2] = [[c, -s], [s, c]]
    return matrix


class _ReplayState:
    """Mutable state threaded through the per-tool replay handlers."""

    def __init__(self, document: EditorDocument, registry: PinRegistry) -> None:
        self.document = document
        self.registry = registry
        self.separate_snapshot = None

    def nearest_pin(self, centroid):
        pins = self.registry.pins
        if not pins:
            return None
        points = np.array([pin.centroid for pin in pins])
        return pins[int(np.argmin(
            np.linalg.norm(points - np.asarray(centroid), axis=1)
        ))]


def _logo_undo_skips(journal: Journal) -> set[int]:
    """A logo undo cancels its emboss entirely — pair each undo with the
    nearest preceding un-undone apply and skip both (cheaper than embossing
    and snapshotting solids)."""
    skip: set[int] = set()
    pending: list[int] = []
    for index, op in enumerate(journal.operations):
        if op.tool != "logo":
            continue
        if op.params.get("action") == "undo":
            if pending:
                skip.update((pending.pop(), index))
        else:
            pending.append(index)
    return skip


# --------------------------------------------------------------- geometry ops

def _replay_zsit(state: _ReplayState, op) -> None:
    state.document.apply_trsf(np.asarray(op.result["matrix"]))


def _replay_pin1(state: _ReplayState, op) -> None:
    if op.params.get("action") == "reset":
        state.document.apply_trsf(_rotation_z(-op.inputs.get("undone_deg", 0.0)))
    else:
        state.document.apply_trsf(_rotation_z(op.result["angle_deg"]))


def _replay_front(state: _ReplayState, op) -> None:
    """Redefine Front: every action (constraints/manual/reset) replays as the
    recorded in-plane spin about Z."""
    state.document.apply_trsf(_rotation_z(op.result.get("angle_deg", 0.0)))


# ------------------------------------------------------- detect_pins actions

def _detect_context_plane(state: _ReplayState, op) -> None:
    found = detect_pins_by_section(
        state.document, op.inputs["plane_point"], op.inputs["plane_normal"]
    )
    state.registry.set_pins(state.registry.pins + found)


def _detect_seed_pin(state: _ReplayState, op) -> None:
    role = op.params.get("role", "primary")
    # Tip seeds (grow=False) and mouth seeds are single faces; older primary
    # journals (no grow key) grew across smooth edges and must be reproduced.
    if role == "mouth" or not op.params.get("grow", True):
        region = {op.inputs["face"]}
    else:
        region = grow_smooth_region(
            state.document, op.inputs["body"], op.inputs["face"],
            smooth_angle_deg=op.params.get("smooth_angle_deg", 30.0),
            area_factor=op.params.get("area_factor"),
        )
    mesh = state.document.bodies[op.inputs["body"]].mesh
    centroid = mesh_region_centroid(mesh, sorted(region))
    state.registry.set_pins(state.registry.pins + [Pin(
        number=0, centroid=centroid,
        face_ids=[(op.inputs["body"], f) for f in sorted(region)],
        role=role,
    )])


def _detect_similar(state: _ReplayState, op) -> None:
    from .pins import find_similar_regions

    claimed = {
        (body, face)
        for pin in state.registry.pins
        for body, face in pin.face_ids
    }
    matches = find_similar_regions(
        state.document, op.inputs["body"], set(op.inputs["faces"]),
        smooth_angle_deg=op.params.get("smooth_angle_deg", 30.0),
        claimed=claimed,
        area_tol=op.params.get("area_tol", 0.12),
        dim_tol=op.params.get("dim_tol", 0.25),
    )
    role = op.params.get("role", "mouth")
    found = []
    for match in matches:
        mesh = state.document.bodies[match[0][0]].mesh
        centroid = mesh_region_centroid(mesh, sorted(face for _b, face in match))
        if centroid is not None:
            found.append(Pin(number=0, centroid=centroid,
                             face_ids=match, role=role))
    state.registry.set_pins(state.registry.pins + found)


def _detect_join_mouth(state: _ReplayState, op) -> None:
    for centroid, name in zip(op.result.get("centroids", []),
                              op.result.get("names", [])):
        pin = state.nearest_pin(centroid)
        if pin is not None:
            pin.name = name
            pin.name_source = "joined"


def _detect_renumber(state: _ReplayState, op) -> None:
    primaries = [p for p in state.registry.pins if p.role != "mouth"]
    mouths = [p for p in state.registry.pins if p.role == "mouth"]
    order = order_pins(primaries, mode=op.params.get("ordering", "serpentine"))
    state.registry.set_pins([primaries[i] for i in order] + mouths)
    for pin, name in zip(state.registry.pins, op.result.get("names", [])):
        pin.name = name


def _detect_predict(state: _ReplayState, op) -> None:
    names = op.result.get("names", [])
    numbers = op.result.get("order", [])
    by_number = {pin.number: pin for pin in state.registry.pins}
    ordered = [by_number[n] for n in numbers if n in by_number]
    if len(ordered) == len(state.registry.pins):
        state.registry.set_pins(ordered)
    for pin, name in zip(state.registry.pins, names):
        pin.name = name


def _detect_rename(state: _ReplayState, op) -> None:
    pin = state.nearest_pin(op.inputs["centroid"])
    if pin is not None:
        pin.name = op.result.get("name", "")
        pin.name_source = op.result.get("name_source", "")


def _detect_delete(state: _ReplayState, op) -> None:
    pin = state.nearest_pin(op.inputs["centroid"])
    if pin is not None:
        state.registry.set_pins([p for p in state.registry.pins if p is not pin])


def _detect_set_order(state: _ReplayState, op) -> None:
    remaining = list(state.registry.pins)
    ordered = []
    for centroid in op.result.get("centroids", []):
        if not remaining:
            break
        points = np.array([p.centroid for p in remaining])
        nearest = int(np.argmin(
            np.linalg.norm(points - np.asarray(centroid), axis=1)
        ))
        ordered.append(remaining.pop(nearest))
    state.registry.set_pins(ordered + remaining)


def _detect_clear(state: _ReplayState, op) -> None:
    state.registry.set_pins([])


def _detect_bands(state: _ReplayState, op) -> None:
    """Committed band detection (no explicit action key, just bands)."""
    for band in op.inputs["bands"]:
        b = Band(*band)
        if len(state.document.bodies) > 1:
            found = detect_pins_multibody(
                state.document, b,
                exclude_largest=op.params.get("exclude_largest", True),
            )
        else:
            found = detect_pins_unibody(
                state.document, 0, b,
                smooth_angle_deg=op.params.get("smooth_angle_deg", 30.0),
                flow=op.params.get("flow", "any-edge"),
            )
        state.registry.set_pins(state.registry.pins + found)
    order = order_pins(
        state.registry.pins, mode=op.params.get("ordering", "serpentine")
    )
    state.registry.reorder(order)


_DETECT_ACTIONS = {
    "context_plane": _detect_context_plane,
    "seed_pin": _detect_seed_pin,
    "detect_similar": _detect_similar,
    "join_mouth": _detect_join_mouth,
    "renumber": _detect_renumber,
    "predict": _detect_predict,
    "rename": _detect_rename,
    "delete": _detect_delete,
    "set_order": _detect_set_order,
    "clear": _detect_clear,
}


def _replay_detect_pins(state: _ReplayState, op) -> None:
    handler = _DETECT_ACTIONS.get(op.params.get("action"))
    if handler is not None:
        handler(state, op)
    elif "bands" in op.inputs:
        _detect_bands(state, op)


# ------------------------------------------------------------------ separate

def _apply_snapshot(document: EditorDocument, snapshot) -> None:
    bodies, links = snapshot
    document.bodies[:] = bodies
    for pin, body_ids, face_ids in links:
        pin.body_ids = body_ids
        pin.face_ids = face_ids


def _separate_restore(state: _ReplayState) -> None:
    if state.separate_snapshot is not None:
        _apply_snapshot(state.document, state.separate_snapshot)
        state.separate_snapshot = None


def _separate_take_snapshot(state: _ReplayState):
    return (
        list(state.document.bodies),
        [(pin, list(pin.body_ids), list(pin.face_ids))
         for pin in state.registry.pins],
    )


def _separate_regions(state: _ReplayState, op):
    bundles = op.inputs.get("bundles")
    if bundles:  # the session painted its regions by hand — split exactly those
        return [
            [tuple(key) for key in bundles[str(pin.number)]]
            if str(pin.number) in bundles else None
            for pin in state.registry.pins
        ]
    return grow_pin_regions(
        state.document, state.registry.pins,
        area_factor=op.params.get("area_factor", 4.0),
    )


def _separate_link_pins(state: _ReplayState, grown, region_bodies) -> None:
    """Link pins to their new solids by region identity (mirrors the tool)."""
    region_position: dict[int, int] = {}
    position = 0
    for pin_index, region in enumerate(grown):
        if region:
            region_position[pin_index] = position
            position += 1
    for pin_index, pin in enumerate(state.registry.pins):
        position = region_position.get(pin_index)
        if position is None:
            continue
        body_index = region_bodies[position]
        if body_index is None:
            continue  # did not separate — stays a face-region pin
        pin.body_ids = [body_index]
        pin.face_ids = []
        suffix = "_HEAD" if pin.role == "mouth" else ""
        state.document.bodies[body_index].name = (
            pin.name or f"PIN_{pin.number}"
        ) + suffix
        state.document.bodies[body_index].role = "pin"


def _separate_link_cut_pins(state: _ReplayState, carved, diag) -> None:
    """Link each unclaimed pin to its carved piece — primary AND mouth (mirrors
    SeparateUnibodyTool._link_cut_pins): identity by the carved body hosting the
    pin's (already remapped) faces, with nearest-centroid proximity as fallback."""
    carved_set = set(carved)
    tol = 0.2 * diag + 1e-6
    centroids = {}
    for body_index in carved:
        mesh = state.document.bodies[body_index].mesh
        if mesh is not None and len(mesh.points):
            centroids[body_index] = mesh.points.mean(axis=0)
    used: set[int] = set()
    for pin in state.registry.pins:
        if pin.body_ids:
            continue
        best = None
        if pin.face_ids:
            counts: dict[int, int] = {}
            for body_index, _face in pin.face_ids:
                if body_index in carved_set and body_index not in used:
                    counts[body_index] = counts.get(body_index, 0) + 1
            if counts:
                best = max(counts, key=counts.get)
        if best is None:
            pin_centroid = np.asarray(pin.centroid, dtype=float)
            best_distance = tol
            for body_index, centroid in centroids.items():
                if body_index in used:
                    continue
                distance = float(np.linalg.norm(centroid - pin_centroid))
                if distance < best_distance:
                    best_distance, best = distance, body_index
        if best is not None:
            used.add(best)
            pin.body_ids = [best]
            pin.face_ids = []
            state.document.bodies[best].name = pin.name or f"PIN_{pin.number}"
            state.document.bodies[best].role = "pin"


def _replay_separate(state: _ReplayState, op) -> None:
    if op.params.get("action") == "undo":
        _separate_restore(state)
        return
    # Self-heal: the recorded body count is ground truth for the pre-apply
    # state. A mismatch means an unjournaled restore happened between two
    # applies — roll back to the snapshot before re-running.
    before = op.result.get("bodies_before")
    if (before is not None and before != len(state.document.bodies)
            and state.separate_snapshot is not None):
        _apply_snapshot(state.document, state.separate_snapshot)
    state.separate_snapshot = _separate_take_snapshot(state)

    from .pins import capture_pin_face_anchors, remap_pin_faces

    face_anchors = capture_pin_face_anchors(state.document, state.registry.pins)
    # Already-linked pins keep their BodyRecord identity through split_by_box's
    # table rebuild; re-resolve their indices after the cuts (see _apply_cuts).
    prelinked = [(pin, [state.document.bodies[i] for i in pin.body_ids
                       if 0 <= i < len(state.document.bodies)])
                 for pin in state.registry.pins if pin.body_ids]
    bounds = state.document.bounds()
    diag = float(np.linalg.norm([
        bounds[1] - bounds[0], bounds[3] - bounds[2], bounds[5] - bounds[4],
    ]))
    remap_tol = diag * 5.0e-3
    cuts = op.inputs.get("cuts")
    if cuts:  # manual box-bounded cut separation
        # Batch pure boxes into one multi-tool boolean per body (O(cuts) vs the
        # O(cuts²) sequential re-split), remeshing once — mirrors _apply_cuts.
        pure_boxes = all(
            float(np.linalg.norm(c.get("normal") or [0.0, 0.0, 0.0])) <= 1e-9
            for c in cuts)
        carved_objs: list = []
        if pure_boxes:
            pin_indices = state.document.split_by_boxes(cuts, remesh=False)
            carved_objs = [state.document.bodies[i] for i in pin_indices]
        else:
            for cut in cuts:
                pin_indices = state.document.split_by_box(
                    cut["limits"], cut.get("normal"), cut.get("location"), remesh=False)
                carved_objs.extend(state.document.bodies[i] for i in pin_indices)
        state.document.remesh_all()
        present = {id(body): index for index, body in enumerate(state.document.bodies)}
        carved = [present[id(o)] for o in carved_objs if id(o) in present]
        for pin, objs in prelinked:
            new_ids = [present[id(o)] for o in objs if id(o) in present]
            if new_ids:
                pin.body_ids = new_ids
        remap_pin_faces(state.document, face_anchors, remap_tol)
        _separate_link_cut_pins(state, carved, diag)
        return
    grown = _separate_regions(state, op)
    regions = [r for r in grown if r]
    region_bodies = state.document.split_by_face_regions(regions)
    _separate_link_pins(state, grown, region_bodies)
    remap_pin_faces(state.document, face_anchors, remap_tol)


# ----------------------------------------------------------- assignment ops

def _replay_hitboxes(state: _ReplayState, op) -> None:
    by_number = {pin.number: pin for pin in state.registry.pins}
    for item in op.result.get("applied", []):
        pin = by_number.get(item["pin"])
        if pin is not None:
            pin.hitbox = item["hitbox"]
    if "hitbox" in op.result and "pin_number" in op.inputs:  # single assign
        pin = by_number.get(op.inputs["pin_number"])
        if pin is not None:
            pin.hitbox = op.result["hitbox"]


def _pf_set_multi(op, by_number) -> None:
    for number in op.inputs.get("pin_numbers", []):
        pin = by_number.get(number)
        if pin is not None:
            pin.function = op.result.get("function", "")


def _pf_bulk(op, by_number) -> None:
    from .tools.pin_functions import parse_function_assignments

    for number, function in parse_function_assignments(
        op.inputs.get("text", "")
    ).items():
        if number in by_number:
            by_number[number].function = function


def _replay_pin_functions(state: _ReplayState, op) -> None:
    by_number = {pin.number: pin for pin in state.registry.pins}
    action = op.params.get("action")
    if action == "auto_inherit":
        from .auto import auto_inherit_functions

        auto_inherit_functions(state.registry)
    elif action == "set_multi":
        _pf_set_multi(op, by_number)
    elif action == "bulk":
        _pf_bulk(op, by_number)
    elif "pin_number" in op.inputs:
        pin = by_number.get(op.inputs["pin_number"])
        if pin is not None:
            pin.function = op.result.get("function", "")


def _apply_color_key(document: EditorDocument, key, rgb) -> None:
    if key is None:
        return
    if isinstance(key, list):
        body_index, face_index = key
        if 0 <= body_index < len(document.bodies):
            mesh = document.bodies[body_index].mesh
            if mesh is not None:
                mesh.face_colors[face_index] = rgb
    elif 0 <= int(key) < len(document.bodies):  # body-level colour
        document.bodies[int(key)].color = rgb
        if document.bodies[int(key)].mesh is not None:
            document.bodies[int(key)].mesh.face_colors = {}


def _replay_colors(state: _ReplayState, op) -> None:
    color = op.result.get("color")
    if not color:
        return
    from PySide6.QtGui import QColor

    qcolor = QColor(color)
    rgb = (qcolor.redF(), qcolor.greenF(), qcolor.blueF())
    keys = op.inputs.get("keys") or [op.inputs.get("key")]
    for key in keys:
        _apply_color_key(state.document, key, rgb)


def _replay_logo(state: _ReplayState, op) -> None:
    if op.params.get("action") == "undo":
        return
    from .dxf_loader import emboss_logo

    here = Path(__file__).resolve().parents[1]
    emboss_logo(
        state.document, op.inputs["body"], op.inputs["face"],
        here / op.inputs.get("dxf", "WN3D.dxf"),
        width_mm=op.params["width_mm"],
        depth_mm=op.params["depth_mm"],
        offset_uv=tuple(op.inputs.get("offset_uv", (0.0, 0.0))),
        raised=op.params.get("raised", False),
        logo_rgb=tuple(op.params["logo_rgb"]) if op.params.get("logo_rgb") else None,
        rotation_deg=op.params.get("rotation_deg", 0.0),
    )


_TOOL_HANDLERS = {
    "zsit": _replay_zsit,
    "pin1": _replay_pin1,
    "front": _replay_front,
    "detect_pins": _replay_detect_pins,
    "separate": _replay_separate,
    "hitboxes": _replay_hitboxes,
    "pin_functions": _replay_pin_functions,
    "colors": _replay_colors,
    "logo": _replay_logo,
}


def replay(document: EditorDocument, journal: Journal) -> PinRegistry:
    """Re-apply a recorded conditioning session. Geometry ops replay their
    computed transforms; detection ops re-run from their recorded inputs;
    assignment ops (names, functions, hitboxes, colours) re-apply results."""
    registry = PinRegistry()
    skip = _logo_undo_skips(journal)
    state = _ReplayState(document, registry)
    for op_index, op in enumerate(journal.operations):
        if op_index in skip:
            continue
        handler = _TOOL_HANDLERS.get(op.tool)
        if handler is not None:
            handler(state, op)

    from .pins import apply_pin_body_names

    apply_pin_body_names(document, registry.pins)
    return registry


def apply_journal_file(step_path: Path, ops_path: Path, out_path: Path | None = None):
    """CLI entry: load, replay, export the conditioned AP242 with metadata."""
    from .export_ap242 import conditioned_path, export_ap242

    document = EditorDocument.load(step_path)
    journal = Journal.load(ops_path)
    registry = replay(document, journal)
    out_path = out_path or conditioned_path(step_path)
    report = export_ap242(document, out_path, pins=registry, journal=journal)
    return report, registry
