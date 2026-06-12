"""Headless journal replay — the seed of the fully automatic conditioner.
Every tool records its executions as declarative JSON; this module re-applies
them to a freshly loaded document without any GUI:

    step_editor.py --apply ops.json input.step
"""

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


def replay(document: EditorDocument, journal: Journal) -> PinRegistry:
    """Re-apply a recorded conditioning session. Geometry ops replay their
    computed transforms; detection ops re-run from their recorded inputs;
    assignment ops (names, functions, hitboxes, colours) re-apply their
    recorded results."""
    registry = PinRegistry()

    # A logo undo cancels its emboss entirely — pair each undo with the
    # nearest preceding un-undone apply and skip both (cheaper than embossing
    # and snapshotting solids).
    skip: set[int] = set()
    pending_logo: list[int] = []
    for index, op in enumerate(journal.operations):
        if op.tool != "logo":
            continue
        if op.params.get("action") == "undo":
            if pending_logo:
                skip.update((pending_logo.pop(), index))
        else:
            pending_logo.append(index)

    def _nearest_pin(centroid):
        if not registry.pins:
            return None
        points = np.array([pin.centroid for pin in registry.pins])
        return registry.pins[int(np.argmin(
            np.linalg.norm(points - np.asarray(centroid), axis=1)
        ))]

    # Separate is reversible in the GUI (snapshot of body table + pin links);
    # replay mirrors that so undo ops restore instead of being ignored.
    separate_snapshot = None
    for op_index, op in enumerate(journal.operations):
        if op_index in skip:
            continue
        tool, params, inputs, result = op.tool, op.params, op.inputs, op.result

        if tool == "zsit":
            document.apply_trsf(np.asarray(result["matrix"]))

        elif tool == "pin1":
            if params.get("action") == "reset":
                document.apply_trsf(_rotation_z(-inputs.get("undone_deg", 0.0)))
            else:
                document.apply_trsf(_rotation_z(result["angle_deg"]))

        elif tool == "detect_pins":
            action = params.get("action")
            if action == "context_plane":
                found = detect_pins_by_section(
                    document, inputs["plane_point"], inputs["plane_normal"]
                )
                registry.set_pins(registry.pins + found)
            elif action == "seed_pin":
                role = params.get("role", "primary")
                if role == "mouth":
                    # mouth seeding is normal face picking — no growth
                    region = {inputs["face"]}
                else:
                    region = grow_smooth_region(
                        document, inputs["body"], inputs["face"],
                        smooth_angle_deg=params.get("smooth_angle_deg", 30.0),
                    )
                mesh = document.bodies[inputs["body"]].mesh
                centroid = mesh_region_centroid(mesh, sorted(region))
                registry.set_pins(registry.pins + [Pin(
                    number=0, centroid=centroid,
                    face_ids=[(inputs["body"], f) for f in sorted(region)],
                    role=role,
                )])
            elif action == "detect_similar":
                from .pins import find_similar_regions

                claimed = {
                    (body, face)
                    for pin in registry.pins
                    for body, face in pin.face_ids
                }
                matches = find_similar_regions(
                    document, inputs["body"], set(inputs["faces"]),
                    smooth_angle_deg=params.get("smooth_angle_deg", 30.0),
                    claimed=claimed,
                )
                found = []
                for match in matches:
                    mesh = document.bodies[match[0][0]].mesh
                    centroid = mesh_region_centroid(
                        mesh, sorted(face for _body, face in match)
                    )
                    if centroid is not None:
                        found.append(Pin(number=0, centroid=centroid,
                                         face_ids=match, role="mouth"))
                registry.set_pins(registry.pins + found)
            elif action == "join_mouth":
                for centroid, name in zip(result.get("centroids", []),
                                          result.get("names", [])):
                    pin = _nearest_pin(centroid)
                    if pin is not None:
                        pin.name = name
                        pin.name_source = "joined"
            elif action == "renumber":
                primaries = [p for p in registry.pins if p.role != "mouth"]
                mouths = [p for p in registry.pins if p.role == "mouth"]
                order = order_pins(primaries, mode=params.get("ordering", "serpentine"))
                registry.set_pins([primaries[i] for i in order] + mouths)
                for pin, name in zip(registry.pins, result.get("names", [])):
                    pin.name = name
            elif action == "predict":
                names = result.get("names", [])
                numbers = result.get("order", [])
                by_number = {pin.number: pin for pin in registry.pins}
                ordered = [by_number[n] for n in numbers if n in by_number]
                if len(ordered) == len(registry.pins):
                    registry.set_pins(ordered)
                for pin, name in zip(registry.pins, names):
                    pin.name = name
            elif action == "rename":
                pin = _nearest_pin(inputs["centroid"])
                if pin is not None:
                    pin.name = result.get("name", "")
                    pin.name_source = result.get("name_source", "")
            elif action == "delete":
                pin = _nearest_pin(inputs["centroid"])
                if pin is not None:
                    registry.set_pins([p for p in registry.pins if p is not pin])
            elif action == "set_order":
                remaining = list(registry.pins)
                ordered = []
                for centroid in result.get("centroids", []):
                    if not remaining:
                        break
                    points = np.array([p.centroid for p in remaining])
                    nearest = int(np.argmin(np.linalg.norm(
                        points - np.asarray(centroid), axis=1
                    )))
                    ordered.append(remaining.pop(nearest))
                registry.set_pins(ordered + remaining)
            elif action == "clear":
                registry.set_pins([])
            elif "bands" in inputs:  # committed band detection
                for band in inputs["bands"]:
                    b = Band(*band)
                    if len(document.bodies) > 1:
                        found = detect_pins_multibody(
                            document, b,
                            exclude_largest=params.get("exclude_largest", True),
                        )
                    else:
                        found = detect_pins_unibody(
                            document, 0, b,
                            smooth_angle_deg=params.get("smooth_angle_deg", 30.0),
                            flow=params.get("flow", "any-edge"),
                        )
                    registry.set_pins(registry.pins + found)
                order = order_pins(registry.pins, mode=params.get("ordering", "serpentine"))
                registry.reorder(order)

        elif tool == "separate":
            if params.get("action") == "undo":
                if separate_snapshot is not None:
                    bodies, links = separate_snapshot
                    document.bodies[:] = bodies
                    for pin, body_ids, face_ids in links:
                        pin.body_ids = body_ids
                        pin.face_ids = face_ids
                    separate_snapshot = None
                continue
            # Self-heal: the recorded body count is ground truth for the
            # pre-apply state. A mismatch means an unjournaled restore
            # happened between two applies — roll back to the snapshot.
            before = result.get("bodies_before")
            if (before is not None and before != len(document.bodies)
                    and separate_snapshot is not None):
                bodies, links = separate_snapshot
                document.bodies[:] = bodies
                for pin, body_ids, face_ids in links:
                    pin.body_ids = body_ids
                    pin.face_ids = face_ids
            separate_snapshot = (
                list(document.bodies),
                [(pin, list(pin.body_ids), list(pin.face_ids))
                 for pin in registry.pins],
            )
            from .pins import capture_pin_face_anchors, remap_pin_faces

            face_anchors = capture_pin_face_anchors(document, registry.pins)
            bounds = document.bounds()
            remap_tol = float(np.linalg.norm([
                bounds[1] - bounds[0], bounds[3] - bounds[2],
                bounds[5] - bounds[4],
            ])) * 5.0e-3
            bundles = inputs.get("bundles")
            if bundles:
                # the session painted its regions by hand — split exactly those
                grown = [
                    [tuple(key) for key in bundles[str(pin.number)]]
                    if str(pin.number) in bundles else None
                    for pin in registry.pins
                ]
            else:
                grown = grow_pin_regions(
                    document, registry.pins,
                    area_factor=params.get("area_factor", 4.0),
                )
            regions = [r for r in grown if r]
            region_bodies = document.split_by_face_regions(regions)
            # exact linking by region identity (mirrors the tool)
            region_position: dict[int, int] = {}
            position = 0
            for pin_index, region in enumerate(grown):
                if region:
                    region_position[pin_index] = position
                    position += 1
            for pin_index, pin in enumerate(registry.pins):
                position = region_position.get(pin_index)
                if position is None:
                    continue
                body_index = region_bodies[position]
                if body_index is None:
                    continue  # did not separate — stays a face-region pin
                pin.body_ids = [body_index]
                pin.face_ids = []
                suffix = "_HEAD" if pin.role == "mouth" else ""
                document.bodies[body_index].name = (
                    pin.name or f"PIN_{pin.number}"
                ) + suffix
                document.bodies[body_index].role = "pin"
            remap_pin_faces(document, face_anchors, remap_tol)

        elif tool == "hitboxes":
            by_number = {pin.number: pin for pin in registry.pins}
            for item in result.get("applied", []):
                pin = by_number.get(item["pin"])
                if pin is not None:
                    pin.hitbox = item["hitbox"]
            if "hitbox" in result and "pin_number" in inputs:  # single assign
                pin = by_number.get(inputs["pin_number"])
                if pin is not None:
                    pin.hitbox = result["hitbox"]

        elif tool == "pin_functions":
            by_number = {pin.number: pin for pin in registry.pins}
            if params.get("action") == "auto_inherit":
                from .auto import auto_inherit_functions

                auto_inherit_functions(registry)
            elif params.get("action") == "set_multi":
                for number in inputs.get("pin_numbers", []):
                    pin = by_number.get(number)
                    if pin is not None:
                        pin.function = result.get("function", "")
            elif params.get("action") == "bulk":
                from .tools.pin_functions import parse_function_assignments

                for number, function in parse_function_assignments(
                    inputs.get("text", "")
                ).items():
                    if number in by_number:
                        by_number[number].function = function
            elif "pin_number" in inputs:
                pin = by_number.get(inputs["pin_number"])
                if pin is not None:
                    pin.function = result.get("function", "")

        elif tool == "colors":
            color = result.get("color")
            if not color:
                continue
            from PySide6.QtGui import QColor

            qcolor = QColor(color)
            rgb = (qcolor.redF(), qcolor.greenF(), qcolor.blueF())
            keys = inputs.get("keys") or [inputs.get("key")]
            for key in keys:
                if key is None:
                    continue
                if isinstance(key, list):
                    body_index, face_index = key
                    if not (0 <= body_index < len(document.bodies)):
                        continue  # body table diverged from the session
                    mesh = document.bodies[body_index].mesh
                    if mesh is not None:
                        mesh.face_colors[face_index] = rgb
                else:
                    if not (0 <= int(key) < len(document.bodies)):
                        continue
                    document.bodies[int(key)].color = rgb
                    if document.bodies[int(key)].mesh is not None:
                        document.bodies[int(key)].mesh.face_colors = {}

        elif tool == "logo":
            if params.get("action") == "undo":
                continue
            from .dxf_loader import emboss_logo

            here = Path(__file__).resolve().parents[1]
            emboss_logo(
                document, inputs["body"], inputs["face"],
                here / inputs.get("dxf", "WN3D.dxf"),
                width_mm=params["width_mm"],
                depth_mm=params["depth_mm"],
                offset_uv=tuple(inputs.get("offset_uv", (0.0, 0.0))),
                raised=params.get("raised", False),
                logo_rgb=tuple(params["logo_rgb"]) if params.get("logo_rgb") else None,
                rotation_deg=params.get("rotation_deg", 0.0),
            )

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