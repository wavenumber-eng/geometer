"""Coded Auto algorithms — one per tool. Each emits the SAME staged state
and journal ops a human session does, so any of them can later be replaced
by a trained model without touching the tools (see AUTOMATE.md). They are
pure functions over the document/registry: the GUI buttons stage their
results for review, and the headless `--auto` conditioner chains them."""

from __future__ import annotations

import numpy as np

from .document import EditorDocument, shape_volume
from .pins import (
    Band,
    Pin,
    PinRegistry,
    detect_pins_by_section,
    detect_pins_multibody,
    grow_pin_regions,
    mesh_face_areas,
    order_pins,
)

TIN_RGB = (0.78, 0.79, 0.81)  # conditioning palette: bare contact metal


def context_plane_point(document: EditorDocument) -> list[float]:
    bounds = document.bounds()
    return [(bounds[0] + bounds[1]) / 2.0, (bounds[2] + bounds[3]) / 2.0, 0.01]


def auto_detect_pins(document: EditorDocument) -> tuple[list[Pin], str]:
    """Run both detectors and keep the richer result: whole-body pins
    (multibody parts — no separation needed afterwards) when they match or
    beat the context-plane section 0.01 mm above the seat; the section wins
    on unibody connectors where the loose bodies are just shield clips."""
    section = detect_pins_by_section(
        document, context_plane_point(document), [0.0, 0.0, -1.0]
    )
    bodies: list[Pin] = []
    if len(document.bodies) > 1:
        bounds = document.bounds()
        bodies = detect_pins_multibody(
            document,
            Band(bounds[0] - 1, bounds[2] - 1, bounds[1] + 1, bounds[3] + 1),
            exclude_largest=True,
        )
    if bodies and len(bodies) >= len(section):
        return bodies, "multibody"
    if len(section) >= 2:
        return section, "context-plane"
    if bodies:
        return bodies, "multibody"
    return [], "none"


def auto_pin1_point(
    document: EditorDocument, registry: PinRegistry | None
) -> tuple[tuple[float, float, float] | None, str]:
    """Pin-1 candidate: the registry's pin 1 when numbering already exists,
    otherwise the end pin of the seat-level row. The datasheet still rules —
    a wrong guess is one 180-degree press away."""
    pins = registry.primaries() if registry is not None else []
    if pins:
        return tuple(pins[0].centroid), "registry pin 1"
    found, _how = auto_detect_pins(document)
    if len(found) >= 2:
        points = np.array([p.centroid for p in found])
        spread = points[:, :2] - points[:, :2].mean(axis=0)
        axis = np.linalg.svd(spread, full_matrices=False)[2][0]
        t = spread @ axis
        return tuple(points[int(np.argmin(t))]), f"end of a {len(found)}-pin row"
    return None, "no seat-level pins found"


def auto_separate_factor(
    document: EditorDocument,
    pins: list[Pin],
    factors=(0.25, 0.5, 1.0, 2.0, 3.0, 4.0, 6.0, 8.0),
) -> tuple[float, int]:
    """Pick the growth cutoff: the first factor of the LAST stable run of
    non-flooded growth. Stable = two successive factors grow to the same
    face count (the pins reached the body). Flooded = some region's bbox
    diagonal approaches its body's own diagonal — the flow leaked across
    the housing (through-hole headers with a big pad face in the seed do
    this at the default 4x), so those factors are discarded."""
    body_diag = {}
    for body_index, body in enumerate(document.bodies):
        mesh = body.mesh
        if mesh is not None and len(mesh.points):
            span = mesh.points.max(axis=0) - mesh.points.min(axis=0)
            body_diag[body_index] = max(float(np.linalg.norm(span)), 1e-9)

    usable: list[tuple[float, int]] = []
    for factor in factors:
        grown = grow_pin_regions(document, pins, area_factor=factor)
        count = 0
        flooded = False
        for region in grown:
            if not region:
                continue
            count += len(region)
            by_body: dict[int, list[int]] = {}
            for body_index, face_index in region:
                by_body.setdefault(body_index, []).append(face_index)
            for body_index, faces in by_body.items():
                mesh = document.bodies[body_index].mesh
                if mesh is None or body_index not in body_diag:
                    continue
                mask = np.isin(
                    mesh.tri_face_ids, np.asarray(faces, dtype=np.int32)
                )
                points = mesh.points[np.unique(mesh.tris[mask])]
                if not len(points):
                    continue
                span = points.max(axis=0) - points.min(axis=0)
                if float(np.linalg.norm(span)) > 0.6 * body_diag[body_index]:
                    flooded = True
        if count and not flooded:
            usable.append((float(factor), count))
    if not usable:
        return 4.0, 0
    best = usable[-1]
    for index in range(len(usable) - 1, 0, -1):
        if usable[index][1] == usable[index - 1][1]:
            best = usable[index - 1]
        else:
            break
    return best


def auto_inherit_functions(registry: PinRegistry) -> int:
    """Net propagation: when exactly one function is set inside a net, every
    other functionless pin of that net takes the INHERIT flag."""
    nets: dict[str, list[Pin]] = {}
    for pin in registry.pins:
        nets.setdefault(pin.name or str(pin.number), []).append(pin)
    changed = 0
    for members in nets.values():
        functions = {
            p.function for p in members if p.function and p.function != "INHERIT"
        }
        if len(functions) == 1:
            for pin in members:
                if not pin.function:
                    pin.function = "INHERIT"
                    changed += 1
    return changed


def auto_color_pins(document: EditorDocument, registry: PinRegistry) -> list:
    """Conditioning palette: every pin body/face that carries NO colour gets
    bare-metal tin, so conditioned exports always show their contacts.
    Returns the colour keys changed (the colors tool's journal format)."""
    changed: list = []
    for pin in registry.pins:
        for body_id in pin.body_ids:
            body = document.bodies[body_id]
            if body.color is None:
                body.color = TIN_RGB
                changed.append(int(body_id))
        for body_id, face_id in pin.face_ids:
            mesh = document.bodies[body_id].mesh
            if mesh is not None and face_id not in mesh.face_colors:
                mesh.face_colors[face_id] = TIN_RGB
                changed.append([int(body_id), int(face_id)])
    return changed


def auto_logo_placement(
    document: EditorDocument,
) -> tuple[int, int, float] | None:
    """LOGO target: the largest upward-facing planar face near the top of
    the largest body, watermark width 45% of its short side."""
    if not document.bodies:
        return None
    body_index = max(
        range(len(document.bodies)),
        key=lambda i: abs(shape_volume(document.bodies[i].solid) or 0.0),
    )
    body = document.bodies[body_index]
    mesh = body.mesh
    if mesh is None or not len(mesh.tris):
        return None
    z_top = float(mesh.points[:, 2].max())
    z_bottom = float(mesh.points[:, 2].min())
    areas = mesh_face_areas(mesh)
    for face_id, _area in sorted(areas.items(), key=lambda kv: -kv[1])[:60]:
        frame = document.face_plane(body_index, face_id)
        if frame is None:
            continue
        normal = np.asarray(frame["normal"])
        if normal[2] < 0.95:
            continue
        origin = np.asarray(frame["origin"])
        if origin[2] < z_top - 0.35 * (z_top - z_bottom):
            continue
        mask = mesh.tri_face_ids == face_id
        points = mesh.points[np.unique(mesh.tris[mask])]
        u = np.asarray(frame["u"])
        v = np.cross(normal, u)
        uv = np.column_stack([points @ u, points @ v])
        span = uv.max(axis=0) - uv.min(axis=0)
        width = 0.45 * float(min(span[0], span[1]))
        if width < 0.2:
            continue
        return body_index, int(face_id), width
    return None


def condition_auto(document: EditorDocument, journal) -> PinRegistry:
    """Headless rung-6 conditioner: chain every coded Auto end to end,
    journaling each step exactly like a human session (replayable)."""
    import math

    from .tools.pin1_quadrant import compute_pin1_angle, rotation_z_matrix
    from .tools.zsit import compute_auto_zsit

    registry = PinRegistry()

    seated = compute_auto_zsit(document)
    if seated is not None:
        matrix, count, _pads = seated
        document.apply_trsf(matrix)
        journal.record("zsit", {"action": "auto"}, {"seat_faces": count},
                       {"matrix": np.asarray(matrix).tolist()})

    point, _reason = auto_pin1_point(document, None)
    if point is not None:
        angle = compute_pin1_angle(point[0], point[1])
        document.apply_trsf(rotation_z_matrix(angle))
        hint = rotation_z_matrix(angle) @ np.array([*point, 1.0])
        journal.record("pin1", {"action": "pin1_click"},
                       {"clicked_point": list(point)},
                       {"angle_deg": math.degrees(angle),
                        "cumulative_deg": math.degrees(angle) % 360.0,
                        "pin1_hint": [float(v) for v in hint[:3]]})

    found, how = auto_detect_pins(document)
    if found:
        registry.set_pins(found)
        primaries = registry.primaries()
        order = order_pins(primaries, mode="serpentine")
        registry.set_pins([primaries[i] for i in order])
        if how == "context-plane":
            journal.record("detect_pins", {"action": "context_plane"},
                           {"plane_point": context_plane_point(document),
                            "plane_normal": [0.0, 0.0, -1.0]},
                           {"closed_shapes": len(found), "added": len(found),
                            "centroids": [list(p.centroid) for p in found]})
        else:
            bounds = document.bounds()
            journal.record("detect_pins",
                           {"exclude_largest": True, "flow": "any-edge",
                            "smooth_angle_deg": 30.0,
                            "ordering": "serpentine"},
                           {"bands": [[bounds[0] - 1, bounds[2] - 1,
                                       bounds[1] + 1, bounds[3] + 1]]},
                           {"added": len(found), "total": len(found),
                            "centroids": [list(p.centroid) for p in found]})
        journal.record("detect_pins", {"action": "renumber",
                                       "ordering": "serpentine"},
                       {"pin1_hint": None},
                       {"order": [p.number for p in registry.pins],
                        "names": [p.name for p in registry.pins]})

    face_pins = [p for p in registry.pins if p.face_ids]
    if face_pins:
        factor, _faces = auto_separate_factor(document, registry.pins)
        from .pins import capture_pin_face_anchors, remap_pin_faces

        anchors = capture_pin_face_anchors(document, registry.pins)
        bounds = document.bounds()
        remap_tol = float(np.linalg.norm([
            bounds[1] - bounds[0], bounds[3] - bounds[2], bounds[5] - bounds[4],
        ])) * 5.0e-3
        before = len(document.bodies)
        grown = grow_pin_regions(document, registry.pins, area_factor=factor)
        regions = [r for r in grown if r]
        region_bodies = document.split_by_face_regions(regions)
        region_position: dict[int, int] = {}
        position = 0
        for pin_index, region in enumerate(grown):
            if region:
                region_position[pin_index] = position
                position += 1
        matched = 0
        for pin_index, pin in enumerate(registry.pins):
            position = region_position.get(pin_index)
            if position is None:
                continue
            body_index = region_bodies[position]
            if body_index is None:
                continue
            pin.body_ids = [body_index]
            pin.face_ids = []
            suffix = "_HEAD" if pin.role == "mouth" else ""
            document.bodies[body_index].name = (
                pin.name or f"PIN_{pin.number}"
            ) + suffix
            document.bodies[body_index].role = "pin"
            matched += 1
        remap_pin_faces(document, anchors, remap_tol)
        journal.record("separate", {"area_factor": factor},
                       {"regions": len(regions)},
                       {"bodies_before": before,
                        "bodies_after": len(document.bodies),
                        "pin_bodies": sum(
                            1 for b in region_bodies if b is not None
                        ),
                        "pins_matched": matched})

    if registry.pins:
        # Body-per-pin contract: name every pin body, however it became one
        # (vendor multibody models never pass through Separate's naming).
        from .pins import apply_pin_body_names

        apply_pin_body_names(document, registry.pins)

    if registry.pins:
        from .hitbox import obb_from_points
        from .pins import pin_mesh_points

        applied = []
        for pin in registry.pins:
            points = pin_mesh_points(document, pin)
            if not len(points):
                continue
            box = obb_from_points(points, margin=0.05)
            if box is not None:
                pin.hitbox = box
                applied.append({"pin": pin.number, "variant": "auto",
                                "hitbox": box})
        journal.record("hitboxes", {"margin": 0.05}, {},
                       {"applied": applied})

        inherited = auto_inherit_functions(registry)
        if inherited:
            journal.record("pin_functions", {"action": "auto_inherit"}, {},
                           {"applied": inherited})

        colored = auto_color_pins(document, registry)
        if colored:
            journal.record("colors", {"action": "auto_pins"},
                           {"keys": colored}, {"color": "#c7c9ce"})
    return registry
