"""Step 9 Embedded Footprint: attach the land pattern this model sits on.

Two sources, both embedded in the conditioned file's metadata:
- From geometry: every pin's outline where it cuts the Z-Sit plane (SMT feet,
  THR leads, ANCHOR casing pins, GUIDE pegs), the real contact shapes the IPC
  land-pattern step is built from.
- A KiCad footprint (.kicad_mod): pads are matched to the 3D pins by number and
  XY position. A mismatch report shows pins with no pad, pads with no pin, and
  pins far from their pad, which is how a wrong pin 1 / front or a misplaced
  model shows up."""

from __future__ import annotations

import math
import re
from pathlib import Path

import numpy as np
from PySide6.QtWidgets import QFileDialog, QLabel, QPushButton, QTextEdit, QVBoxLayout, QWidget

from ..pins import section_segments, specialty
from .base import ToolMode, make_apply_button

SECTION_Z = 0.01  # mm above the seat, same as the Context Plane

PAD_RE = re.compile(r'\(pad\s+"?([^"\s)]*)"?\s+(\w+)\s+(\w+)(.*?)(?=\n\s*\(pad\s|\Z)', re.S)
# KiCad 9 writes "(at x y r\n)" across lines: allow space before the ")"
AT_RE = re.compile(r"\(at\s+([-\d.eE]+)\s+([-\d.eE]+)(?:\s+([-\d.eE]+))?\s*\)")
SIZE_RE = re.compile(r"\(size\s+([-\d.eE]+)\s+([-\d.eE]+)\s*\)")
DRILL_RE = re.compile(r"\(drill\s+(oval\s+)?([-\d.eE]+)(?:\s+([-\d.eE]+))?")


def parse_kicad_mod(text: str) -> dict:
    name = re.search(r'\((?:footprint|module)\s+"?([^"\s)]+)', text)
    pads = []
    for number, kind, shape, body in PAD_RE.findall(text):
        at, size = AT_RE.search(body), SIZE_RE.search(body)
        if not at:
            continue
        # KiCad's Y axis points down; flip so pads sit in the model's frame
        drill = DRILL_RE.search(body)
        pads.append({"number": number, "type": kind, "shape": shape,
                     "x": float(at.group(1)), "y": -float(at.group(2)),
                     "w": float(size.group(1)) if size else None, "h": float(size.group(2)) if size else None,
                     "drill": [float(drill.group(2)), float(drill.group(3) or drill.group(2))] if drill else None})
    return {"name": name.group(1) if name else "", "pads": pads}


def match_pins(pins, pads, tolerance_mm: float = 0.5) -> dict:
    """Pin <-> pad by number; report distance and unmatched on both sides."""
    by_number = {str(p["number"]): p for p in pads if p["type"] != "np_thru_hole" and p["number"]}
    rows, far = [], []
    for pin in pins:
        key = pin.name or str(pin.number)
        pad = by_number.get(key) or by_number.get(str(pin.number))
        if pad is None:
            rows.append((key, None))
            continue
        d = math.dist(pin.centroid[:2], (pad["x"], pad["y"]))
        rows.append((key, d))
        if d > tolerance_mm:
            far.append(key)
    used = {k for k, d in rows if d is not None}
    return {"rows": rows, "pins_without_pad": [k for k, d in rows if d is None],
            "pads_without_pin": sorted(set(by_number) - used - {str(p.number) for p in pins}),
            "far": far}


def _chain_loops(segments: np.ndarray, decimals: int = 5) -> list[np.ndarray]:
    """Join plane-section segments into closed XY loops (open chains dropped)."""
    keys = np.round(segments[:, :, :2], decimals)
    ends: dict[tuple, list[int]] = {}
    for index, seg in enumerate(keys):
        for end in (0, 1):
            ends.setdefault(tuple(seg[end]), []).append(index)
    used = np.zeros(len(segments), dtype=bool)
    loops = []
    for start in range(len(segments)):
        if used[start]:
            continue
        used[start] = True
        loop = [tuple(keys[start, 0]), tuple(keys[start, 1])]
        while True:
            nxt = next((i for i in ends.get(loop[-1], ()) if not used[i]), None)
            if nxt is None:
                break
            used[nxt] = True
            a, b = tuple(keys[nxt, 0]), tuple(keys[nxt, 1])
            loop.append(b if a == loop[-1] else a)
        if len(loop) > 3 and loop[0] == loop[-1]:
            loops.append(np.asarray(loop[:-1]))
    return loops


def _area(loop: np.ndarray) -> float:
    x, y = loop[:, 0], loop[:, 1]
    return 0.5 * float(np.dot(x, np.roll(y, 1)) - np.dot(y, np.roll(x, 1)))


def _inside(point, loop: np.ndarray) -> bool:
    x, y = point
    inside = False
    for (x1, y1), (x2, y2) in zip(loop, np.roll(loop, -1, axis=0)):
        if (y1 > y) != (y2 > y) and x < x1 + (y - y1) * (x2 - x1) / (y2 - y1):
            inside = not inside
    return inside


def _section_loops(document, z: float) -> list[np.ndarray]:
    loops = []
    for body in document.bodies:
        if body.mesh is None or not len(body.mesh.tris):
            continue
        segments = section_segments(body.mesh, (0.0, 0.0, z), (0.0, 0.0, 1.0))
        if len(segments):
            loops.extend(_chain_loops(segments))
    return [lp for lp in loops if abs(_area(lp)) > 1e-6]


def _containment(loops: list, xys: list) -> np.ndarray:
    """inside[i, j]: pin centre j lies in loop i. One vectorised test per
    loop (a per-point Python loop took 3 s on a 294-pin DIMM socket)."""
    from matplotlib.path import Path as _Path

    if not loops or not xys:
        return np.zeros((len(loops), len(xys)), dtype=bool)
    points = np.asarray(xys, dtype=float)
    return np.array([_Path(lp).contains_points(points) for lp in loops], dtype=bool)


def _section_loop(loops: list, xy, xys, inside: np.ndarray | None = None, index: int | None = None) -> np.ndarray | None:
    """The section loop that is this pin's: the smallest holding its centre,
    else the nearest one (within 0.5 mm) holding no other pin. `inside` is
    _containment(loops, xys) and `index` this pin's column, when known."""
    if inside is None or index is None:
        inside, index = _containment(loops, list(xys) + [xy]), len(xys)
    holding = [lp for lp, hit in zip(loops, inside[:, index]) if hit]
    if holding:
        return min(holding, key=lambda lp: abs(_area(lp)))
    return _nearest_free_loop(loops, xy, inside[:, :len(xys)])


def _nearest_free_loop(loops: list, xy, inside: np.ndarray) -> np.ndarray | None:
    """The nearest loop (within 0.5 mm) that holds no pin centre."""
    if not loops:
        return None
    distance = [float(np.linalg.norm(lp.mean(axis=0) - xy)) for lp in loops]
    nearest = int(np.argmin(distance))
    return loops[nearest] if distance[nearest] < 0.5 and not inside[nearest].any() else None


def _pin_triangles(document, pin) -> np.ndarray:
    """(n, 3, 3) triangles of a pin: its own solid(s), else its face region."""
    out = []
    for b in pin.body_ids:
        mesh = document.bodies[b].mesh
        if mesh is not None and len(mesh.tris):
            out.append(mesh.points[mesh.tris])
    by_body: dict[int, list[int]] = {}
    for b, f in pin.face_ids if not pin.body_ids else ():
        by_body.setdefault(b, []).append(f)
    for b, faces in by_body.items():
        mesh = document.bodies[b].mesh
        out.append(mesh.points[mesh.tris[np.isin(mesh.tri_face_ids, faces)]])
    return np.concatenate(out) if out else np.zeros((0, 3, 3))


def _bottom_outline(document, pin, tol: float = SECTION_Z) -> np.ndarray | None:
    """The XY outline of what a surface-mount pin puts on the board: its
    downward-facing faces within `tol` of its lowest Z (the real contact at
    the seat, however thin the pad is modelled). `tol` is the SMT slice
    height: a part seated a fraction of a degree off flat (a plane through a
    D2PAK's tab and its lead feet, BTN8982TA) lifts the far end of a 2 mm foot
    by ~0.008 mm, and a 0.002 mm tolerance then found no contact at all.
    None when it has none."""
    from scipy.spatial import ConvexHull

    tri = _pin_triangles(document, pin)
    if not len(tri):
        return None
    normal = np.cross(tri[:, 1] - tri[:, 0], tri[:, 2] - tri[:, 0])
    length = np.linalg.norm(normal, axis=1)
    down = (normal[:, 2] < -0.9 * np.maximum(length, 1e-30)) & (length > 0)
    floor = tri[:, :, 2].min()
    touching = down & (tri[:, :, 2].max(axis=1) <= floor + tol)
    points = tri[touching][:, :, :2].reshape(-1, 2)
    if len(points) < 3 or np.ptp(points, axis=0).min() < 1e-6:
        return None
    hull = ConvexHull(points)
    return points[hull.vertices]


def _contact_outline(document, pin) -> np.ndarray | None:
    """What a pin puts on the board: a BGA ball's whole silhouette (it touches
    at one point, so its flat-face outline is a speck), else the downward
    faces at the seat (_bottom_outline)."""
    if pin.kind != "bga":
        return _bottom_outline(document, pin)
    from scipy.spatial import ConvexHull

    points = _pin_triangles(document, pin).reshape(-1, 3)[:, :2]
    if len(points) < 3 or np.ptp(points, axis=0).min() < 1e-6:
        return None
    return points[ConvexHull(points).vertices]


def _with_own_solids(document, pins: list) -> list:
    """The pins as the footprint measures them: a face pin on a lead that is
    only its own solid (hitbox.pin_solid_ids) stands for that whole lead (a
    copy: the registry is not touched). A pin clicked on a gull-wing lead's
    tip face otherwise has no outline on the board (BTN8982TA)."""
    import copy

    from ..hitbox import pin_solid_ids

    out = []
    for pin in pins:
        own = [] if pin.body_ids else pin_solid_ids(document, pin, pins)
        if own:
            pin = copy.copy(pin)
            pin.body_ids = list(own)
        out.append(pin)
    return out


def geometry_footprint(document, pins, z: float = SECTION_Z) -> dict:
    """Each pin's outline where the model crosses the board surface. SMT feet
    are sliced just ABOVE the seat (z), through-hole leads, ANCHOR posts and
    GUIDE pegs (tips below the seat) just BELOW it (-5z), where only they
    exist. A loop holding more than one pin is a housing outline, never a
    pin's. Pins with no own loop are listed as unresolved."""
    levels: dict[float, list] = {}
    # mating contacts (mouth pins) never touch the board, and they sit at the
    # same XY as their tails: leave them out, or every lead looks like a housing
    pins = [pin for pin in pins if pin.role != "mouth"]
    xys = [np.asarray(pin.centroid[:2], dtype=float) for pin in pins]
    pads, unresolved = [], []
    pins = _with_own_solids(document, pins)
    for index, pin in enumerate(pins):
        level = -5 * z if pin.centroid[2] < -z else z
        if level not in levels:
            loops = _section_loops(document, level)
            inside = _containment(loops, xys)
            keep = inside.sum(axis=1) <= 1
            levels[level] = ([lp for lp, k in zip(loops, keep) if k], inside[keep])
        bottom = _contact_outline(document, pin) if level > 0 else None
        # SMT: the faces that actually sit on the board; else the section loop
        loops, inside = levels[level]
        loop = bottom if bottom is not None else _section_loop(loops, xys[index], xys, inside, index)
        name = pin.name or str(pin.number)
        if loop is None:
            unresolved.append(name)
            continue
        kind = specialty(pin) or ("bga" if pin.kind == "bga" else "con" if pin.role == "mouth" else "pin")
        lo, hi = loop.min(axis=0), loop.max(axis=0)
        pads.append({
            "pin": name, "kind": kind, "section_z_mm": level,
            # ANCHOR through the board: plated hole (on a net); an ANCHOR tab
            # on the surface is an SMD land. GUIDE: non-plated hole.
            "hole": _hole_kind(kind, level),
            "centre_xy": [round(float(v), 4) for v in (lo + hi) / 2],
            "size_xy": [round(float(v), 4) for v in hi - lo],
            "area_mm2": round(abs(_area(loop)), 5),
            "outline_xy": [[round(float(x), 4), round(float(y), 4)] for x, y in loop],
        })
    return {"section_z_mm": z, "pads": pads, "unresolved": unresolved}


def _hole_kind(kind: str, level: float) -> str:
    """'plated' / 'non-plated' / '' (no hole). An ANCHOR is plated only when it
    goes through the board (sliced below the seat); a surface-mount fixing tab
    (JST SH / GH side tabs) has no hole. A GUIDE peg always has its hole."""
    if kind == "guide":
        return "non-plated"
    return "plated" if kind == "anchor" and level < 0 else ""


# Nominal IPC-7351B fillet goals, mm (toe, heel, side): the same values the
# KiCad footprint generator's tables carry. Density level N ("nominal").
GOALS = {"gull_wing": (0.35, 0.35, 0.03), "no_lead": (0.30, 0.00, 0.05), "chip": (0.35, 0.00, 0.00)}
THT_HOLE_ALLOWANCE = 0.20   # hole = lead + this (level B)
THT_ANNULAR = 0.25          # pad = hole + 2 x this
NPTH_ALLOWANCE = 0.10       # GUIDE peg hole = peg + this
SLOT_ASPECT = 1.3           # a lead this much longer than wide gets a slot, not a round hole
SLOT_MIN_LENGTH = 0.8       # mm: shorter leads get a round (drilled) hole whatever their shape


def ipc_land_pattern(geometry: dict, document) -> list[dict]:
    """IPC land pattern from the model's own pin outlines (Pins from
    geometry): SMT lands = contact + toe / heel / side goals along the pin's
    outward direction (gull-wing outside the body, no-lead under it); THT and
    ANCHOR through the board = a plated pad around a hole sized from the
    lead, an ANCHOR tab on the surface = an unnumbered SMD land (same IPC
    goals as the pins); GUIDE = an unplated hole. Pads are in the model's
    frame (Y up)."""
    b = document.bounds()
    centre = np.array([(b[0] + b[1]) / 2, (b[2] + b[3]) / 2])
    body = [b[0], b[1], b[2], b[3]]
    pads = []
    rows = _row_axes(_surface_contacts(geometry))
    for pad in geometry.get("pads", []):
        cx, cy = pad["centre_xy"]
        sx, sy = pad["size_xy"]
        if pad["kind"] == "guide" or pad["section_z_mm"] < 0:
            pads.append(_hole_land(pad))  # GUIDE: unplated; leads and ANCHORs through the board: plated
            continue
        if pad["kind"] == "anchor":  # a fixing tab on the surface: SMD, no number
            pads.append(_tab_land(pad))
            continue
        if pad["kind"] == "bga":
            pads.append(_bga_land(pad))
            continue
        if pad["kind"] == "therm" or (abs(cx - centre[0]) < sx / 2 and abs(cy - centre[1]) < sy / 2):
            # an exposed / thermal pad (it covers the body centre): the land is
            # the pad itself, no toe / heel / side fillets
            pads.append({"number": pad["pin"], "type": "smd", "shape": "rect", "x": round(cx, 4),
                         "y": round(cy, 4), "w": round(sx, 4), "h": round(sy, 4), "drill": None})
            continue
        pads.append(_smd_land(pad, centre, body, rows.get(id(pad))))
    return pads


TAB_FILLET_ACROSS = 0.5   # mm each side of a fixing tab's plate (fillets on BOTH faces)
TAB_FILLET_ALONG = 0.15   # mm past each end of the plate


def _tab_land(pad: dict) -> dict:
    """An unnumbered SMD land for a surface-mount fixing tab (ANCHOR sliced at
    the seat). The tab is a plate standing on its edge, so solder fillets on
    both faces: the land grows TAB_FILLET_ACROSS each side across the plate
    and TAB_FILLET_ALONG past each end (JST SH: a 0.2 x 1.5 mm tab -> the
    recommended 1.2 x 1.8 mm mounting pad)."""
    cx, cy = pad["centre_xy"]
    sx, sy = pad["size_xy"]
    across_x = sx <= sy
    w = sx + 2 * (TAB_FILLET_ACROSS if across_x else TAB_FILLET_ALONG)
    h = sy + 2 * (TAB_FILLET_ALONG if across_x else TAB_FILLET_ACROSS)
    return {"number": "", "type": "smd", "shape": "rect", "x": round(cx, 4), "y": round(cy, 4),
            "w": round(w, 4), "h": round(h, 4), "drill": None}


def _surface_contacts(geometry: dict) -> list[dict]:
    """The pins that sit on the board (not through it, not pegs / anchors)."""
    return [g for g in geometry.get("pads", []) if g["section_z_mm"] >= 0 and g["kind"] not in ("guide", "anchor")]


ROW_TOLERANCE = 0.1  # mm: contacts this close in Y (X) share a row (column)


def _row_axes(contacts: list[dict]) -> dict[int, bool]:
    """id(contact) -> along_x for contacts that sit in a row or column of
    others: a horizontal row (same Y) runs out in Y, a column (same X) runs out
    in X, whatever each contact's own shape (a squarish 0.5 x 0.6 contact on a
    2 mm pitch must keep the pitch). Contacts with no clear row are left out."""
    xy = np.array([c["centre_xy"] for c in contacts], dtype=float).reshape(-1, 2)
    out = {}
    for i, contact in enumerate(contacts):
        in_row = int(np.sum(np.abs(xy[:, 1] - xy[i, 1]) < ROW_TOLERANCE)) - 1
        in_column = int(np.sum(np.abs(xy[:, 0] - xy[i, 0]) < ROW_TOLERANCE)) - 1
        if in_row != in_column:
            out[id(contact)] = in_column > in_row
    return out


BGA_LAND_RATIO = 0.8  # IPC-7351 collapsing balls: land = 80% of the ball diameter


def _bga_land(pad: dict) -> dict:
    """A round SMD land under a BGA ball, BGA_LAND_RATIO of its diameter."""
    cx, cy = pad["centre_xy"]
    diameter = round(BGA_LAND_RATIO * max(pad["size_xy"]), 4)
    return {"number": pad["pin"], "type": "smd", "shape": "circle", "x": round(cx, 4),
            "y": round(cy, 4), "w": diameter, "h": diameter, "drill": None}


def _peg_is_round(pad: dict) -> bool:
    """A round peg / lead fills ~pi/4 of its bounding box; a rectangle ~1."""
    sx, sy = pad["size_xy"]
    return pad["area_mm2"] < 0.9 * sx * sy


def _hole_land(pad: dict) -> dict:
    """The hole for a peg or lead crossing the board, from its outline at
    the board plane. GUIDE: unplated; round peg -> round hole of its
    diameter + NPTH_ALLOWANCE, rectangular peg -> an oval slot. Plated lead /
    ANCHOR: a round hole that clears the lead (its diameter, or a square or
    rectangular lead's DIAGONAL, IPC-2222) + THT_HOLE_ALLOWANCE, with a
    THT_ANNULAR copper ring."""
    cx, cy = pad["centre_xy"]
    sx, sy = pad["size_xy"]
    round_peg = _peg_is_round(pad)
    if pad["kind"] == "guide":
        if round_peg or abs(sx - sy) < 0.15 * max(sx, sy):
            hole = round(max(sx, sy) + NPTH_ALLOWANCE, 4)
            return {"number": "", "type": "np_thru_hole", "shape": "circle", "x": cx, "y": cy,
                    "w": hole, "h": hole, "drill": [hole, hole]}
        slot = [round(sx + NPTH_ALLOWANCE, 4), round(sy + NPTH_ALLOWANCE, 4)]
        return {"number": "", "type": "np_thru_hole", "shape": "oval", "x": cx, "y": cy,
                "w": slot[0], "h": slot[1], "drill": slot}
    number = "" if pad["kind"] == "anchor" else pad["pin"]
    if not round_peg and _is_slot_tab(sx, sy):
        # a flat tab (a USB shell leg, a stamped anchor): a plated oval slot
        # that clears it by THT_HOLE_ALLOWANCE in both directions
        drill = [round(sx + THT_HOLE_ALLOWANCE, 4), round(sy + THT_HOLE_ALLOWANCE, 4)]
        return {"number": number, "type": "thru_hole", "shape": "oval", "x": cx, "y": cy,
                "w": round(drill[0] + 2 * THT_ANNULAR, 4), "h": round(drill[1] + 2 * THT_ANNULAR, 4),
                "drill": drill}
    lead = max(sx, sy) if round_peg else float(np.hypot(sx, sy))
    hole = round(lead + THT_HOLE_ALLOWANCE, 4)
    size = round(hole + 2 * THT_ANNULAR, 4)
    return {"number": number, "type": "thru_hole",
            "shape": "circle", "x": cx, "y": cy, "w": size, "h": size, "drill": [hole, hole]}


def _is_slot_tab(sx: float, sy: float) -> bool:
    """A flat tab worth a routed slot: elongated AND long enough (a 0.3 x 0.2
    DIMM lead gets a round drilled hole that clears its diagonal; a 0.9 x 0.4
    USB shell leg gets a slot)."""
    return _is_elongated(sx, sy) and max(sx, sy) >= SLOT_MIN_LENGTH


def _is_elongated(sx: float, sy: float) -> bool:
    """Clearly longer than wide (SLOT_ASPECT): a tab, a flat lead."""
    return max(sx, sy) >= SLOT_ASPECT * max(min(sx, sy), 1e-9)


def _outward_axis(cx: float, cy: float, sx: float, sy: float, centre, row_axis: bool | None = None) -> tuple[bool, float]:
    """(along_x, sign) of the direction a contact runs out from the body: the
    axis across its row when it has one (row_axis); else an elongated contact's
    own long axis (the end pins of a long row sit further out in X than in Y,
    but their leads still run in Y); else away from the body centre."""
    outward = np.array([cx, cy]) - centre
    if row_axis is not None:
        along_x = row_axis
    else:
        along_x = sx > sy if _is_elongated(sx, sy) else abs(outward[0]) >= abs(outward[1])
    return along_x, float(np.sign(outward[0] if along_x else outward[1]) or 1.0)


def _under_body(cx, cy, sx, sy, body) -> bool:
    """The contact lies wholly inside the body's outline (a no-lead pin)."""
    return body[0] < cx - sx / 2 and cx + sx / 2 < body[1] and body[2] < cy - sy / 2 and cy + sy / 2 < body[3]


def _smd_land(pad: dict, centre, body, row_axis: bool | None = None) -> dict:
    """An SMT land: the contact + IPC toe / heel / side goals along the pin's
    outward direction (gull-wing outside the body, no-lead under it)."""
    cx, cy = pad["centre_xy"]
    sx, sy = pad["size_xy"]
    along_x, sign = _outward_axis(cx, cy, sx, sy, centre, row_axis)
    length, width = (sx, sy) if along_x else (sy, sx)
    toe, heel, side = GOALS["no_lead" if _under_body(cx, cy, sx, sy, body) else "gull_wing"]
    land_len, land_w = length + toe + heel, width + 2 * side
    shift = sign * (toe - heel) / 2
    x, y = (cx + shift, cy) if along_x else (cx, cy + shift)
    w, h = (land_len, land_w) if along_x else (land_w, land_len)
    return {"number": pad["pin"], "type": "smd", "shape": "roundrect", "x": round(x, 4),
            "y": round(y, 4), "w": round(w, 4), "h": round(h, 4), "drill": None}


MASK_MARGIN = 0.05     # mm per side, solder-mask opening around each pad (IPC-7351 nominal)
SILK_CLEARANCE = 0.2   # mm, silkscreen kept off the mask opening (IPC-7351 relief)
COURTYARD_EXCESS = 0.25  # mm round pads + body (IPC nominal)


def _edge_runs(a0: float, a1: float, blocked: list[tuple[float, float]], min_len: float = 0.1) -> list:
    """The parts of [a0, a1] not covered by any blocked interval."""
    runs, start = [], a0
    for lo, hi in sorted(blocked):
        if hi <= start or lo >= a1:
            continue
        if lo > start:
            runs.append((start, lo))
        start = max(start, hi)
    if start < a1:
        runs.append((start, a1))
    return [(lo, hi) for lo, hi in runs if hi - lo >= min_len]


def silkscreen(body_xy: tuple, pads: list[dict], clearance: float = SILK_CLEARANCE) -> list:
    """The package outline as silkscreen segments [(x0, y0, x1, y1)], broken
    wherever it would come within `clearance` of a pad (on a QFN that leaves
    the corner brackets)."""
    x0, x1, y0, y1 = body_xy
    segments = []
    for fixed, lo, hi, horizontal in ((y0, x0, x1, True), (y1, x0, x1, True), (x0, y0, y1, False), (x1, y0, y1, False)):
        blocked = []
        for p in pads:
            px, py = p["x"], p["y"]
            hw, hh = p["w"] / 2 + MASK_MARGIN + clearance, p["h"] / 2 + MASK_MARGIN + clearance
            across, along, half_across, half_along = (py, px, hh, hw) if horizontal else (px, py, hw, hh)
            if abs(fixed - across) <= half_across:
                blocked.append((along - half_along, along + half_along))
        for a, b in _edge_runs(lo, hi, blocked):
            segments.append((a, fixed, b, fixed) if horizontal else (fixed, a, fixed, b))
    return segments


def _pin1_dot(body_xy: tuple, pads: list[dict], clearance: float = SILK_CLEARANCE) -> tuple | None:
    """A silkscreen dot (centre x, y, radius) marking pin 1 ('1', or 'A1' on a
    BGA): just outside the pad, or, when pin 1 sits under the body (a BGA, a
    QFN), just outside the body corner nearest it, where it stays visible."""
    first = next((p for p in pads if str(p.get("number")) in ("1", "A1")), None)
    if first is None:
        return None
    x0, x1, y0, y1 = body_xy
    if x0 < first["x"] < x1 and y0 < first["y"] < y1:
        return _corner_dot(body_xy, first, clearance)
    cx, cy = (x0 + x1) / 2, (y0 + y1) / 2
    out = np.array([first["x"] - cx, first["y"] - cy])
    out = out / (np.linalg.norm(out) or 1.0)
    reach = max(first["w"], first["h"]) / 2 + clearance + 0.15
    return (round(first["x"] + out[0] * reach, 4), round(first["y"] + out[1] * reach, 4), 0.1)


def _corner_dot(body_xy: tuple, first: dict, clearance: float) -> tuple:
    """Just outside the body corner nearest pin 1, on its diagonal."""
    x0, x1, y0, y1 = body_xy
    sx = -1.0 if first["x"] < (x0 + x1) / 2 else 1.0
    sy = -1.0 if first["y"] < (y0 + y1) / 2 else 1.0
    step = (clearance + 0.15) / np.sqrt(2.0)
    corner_x, corner_y = (x0 if sx < 0 else x1), (y0 if sy < 0 else y1)
    return (round(corner_x + sx * step, 4), round(corner_y + sy * step, 4), 0.1)


def courtyard(body_xy: tuple, pads: list[dict], excess: float = COURTYARD_EXCESS) -> tuple:
    xs = [body_xy[0], body_xy[1]] + [p["x"] + s * p["w"] / 2 for p in pads for s in (-1, 1)]
    ys = [body_xy[2], body_xy[3]] + [p["y"] + s * p["h"] / 2 for p in pads for s in (-1, 1)]
    return (min(xs) - excess, max(xs) + excess, min(ys) - excess, max(ys) + excess)


def kicad_mod_text(name: str, pads: list[dict], body_xy: tuple | None = None) -> str:
    """A KiCad footprint for generated pads (KiCad's Y axis down): copper,
    and with the package outline `body_xy` (xmin, xmax, ymin, ymax) also
    silkscreen with pad relief, a pin-1 dot, courtyard and fab outline."""
    lines = [f'(footprint "{name}"', '  (layer "F.Cu")', '  (attr smd)' if all(
        p["type"] == "smd" for p in pads) else '  (attr through_hole)']
    if body_xy is not None:
        lines += _outline_lines(body_xy, pads)
    for p in pads:
        layers = '"F.Cu" "F.Paste" "F.Mask"' if p["type"] == "smd" else '"*.Cu" "*.Mask"'
        drill = _drill_text(p.get("drill"))
        lines.append(f'  (pad "{p["number"]}" {p["type"]} {p["shape"]} (at {p["x"]:.4f} {-p["y"]:.4f})'
                     f' (size {p["w"]:.4f} {p["h"]:.4f}){drill} (layers {layers})'
                     f' (solder_mask_margin {MASK_MARGIN}))')
    lines.append(")")
    return "\n".join(lines) + "\n"


def _drill_text(drill) -> str:
    """KiCad drill: round "(drill D)" or slot "(drill oval W H)"."""
    if not drill:
        return ""
    w, h = (drill, drill) if isinstance(drill, (int, float)) else drill
    return f" (drill {w:.4f})" if abs(w - h) < 1e-6 else f" (drill oval {w:.4f} {h:.4f})"


def _rect_lines(rect: tuple, layer: str, width: float) -> list[str]:
    x0, x1, y0, y1 = rect
    corners = [(x0, y0), (x1, y0), (x1, y1), (x0, y1), (x0, y0)]
    return [f'  (fp_line (start {a[0]:.4f} {-a[1]:.4f}) (end {b[0]:.4f} {-b[1]:.4f}) '
            f'(stroke (width {width}) (type solid)) (layer "{layer}"))' for a, b in zip(corners, corners[1:])]


def _outline_lines(body_xy: tuple, pads: list[dict]) -> list[str]:
    lines = [f'  (fp_line (start {a:.4f} {-b:.4f}) (end {c:.4f} {-d:.4f}) '
             f'(stroke (width 0.12) (type solid)) (layer "F.SilkS"))' for a, b, c, d in silkscreen(body_xy, pads)]
    dot = _pin1_dot(body_xy, pads)
    if dot is not None:
        lines.append(f'  (fp_circle (center {dot[0]:.4f} {-dot[1]:.4f}) (end {dot[0] + dot[2]:.4f} {-dot[1]:.4f}) '
                     f'(stroke (width 0.12) (type solid)) (fill solid) (layer "F.SilkS"))')
    return lines + _rect_lines(courtyard(body_xy, pads), "F.CrtYd", 0.05) + _rect_lines(body_xy, "F.Fab", 0.1)


SILK_RE = re.compile(r'\(fp_line\s+\(start\s+([-\d.]+)\s+([-\d.]+)\)\s+\(end\s+([-\d.]+)\s+([-\d.]+)\)[^\n]*?'
                     r'\(layer\s+"?F\.SilkS"?\)')
CRTYD_RE = re.compile(r'\(fp_line\s+\(start\s+([-\d.]+)\s+([-\d.]+)\)\s+\(end\s+([-\d.]+)\s+([-\d.]+)\)[^\n]*?'
                      r'\(layer\s+"?F\.CrtYd"?\)')


def footprint_lines(text: str) -> dict:
    """Silkscreen and courtyard segments of a .kicad_mod, in the model frame (Y up)."""
    def segs(regex):
        return [(float(a), -float(b), float(c), -float(d)) for a, b, c, d in regex.findall(text)]
    return {"silk": segs(SILK_RE), "courtyard": segs(CRTYD_RE)}


def apply_footprint(document, path: str | None, text: str) -> dict:
    fp = parse_kicad_mod(text)
    fp["file"] = Path(path).name if path else ""
    fp["kicad_mod"] = text
    geometry = (getattr(document, "footprint", None) or {}).get("geometry")
    if geometry:
        fp["geometry"] = geometry  # keep the pin outlines from geometry
    document.footprint = fp
    return fp


def _package_xy(document, pins) -> tuple:
    """The package outline (xmin, xmax, ymin, ymax): the largest body that is
    not a pin."""
    from ..document import shape_bounds, shape_volume

    pin_bodies = {b for pin in pins for b in pin.body_ids}
    candidates = [b for i, b in enumerate(document.bodies) if i not in pin_bodies] or document.bodies
    body = max(candidates, key=lambda b: shape_volume(b.solid) or 0.0)
    xmin, xmax, ymin, ymax, _z0, _z1 = shape_bounds([body.solid])
    return (xmin, xmax, ymin, ymax)


def _in_model_frame(pads: list[dict], placement) -> list[dict]:
    """Pads moved into the model's frame by the solved placement (as-is without one)."""
    if not placement:
        return pads
    a = math.radians(-placement["rotate_deg"])
    rot = np.array([[math.cos(a), -math.sin(a)], [math.sin(a), math.cos(a)]])
    origin = np.asarray(placement["offset_xy_mm"])
    return [{**pad, **dict(zip(("x", "y"), (rot @ (np.array([pad["x"], pad["y"]]) - origin)).tolist()))}
            for pad in pads]


def _plated_with_lead(pads: list[dict], geometry: dict | None) -> list[tuple[dict, dict]]:
    """(plated pad, its lead's outline at the board) pairs, matched by number."""
    leads = {g["pin"]: g for g in (geometry or {}).get("pads", []) if g["section_z_mm"] < 0}
    plated = [p for p in pads if p.get("type") == "thru_hole" and p.get("drill")]
    return [(p, leads[str(p.get("number"))]) for p in plated if str(p.get("number")) in leads]


def _undersized_holes(pads: list[dict], geometry: dict | None) -> list[str]:
    """Plated holes that would not take their lead: drill < the lead's
    diameter (round) or diagonal (square / rectangular) at the board."""
    out = []
    for pad, lead in _plated_with_lead(pads, geometry):
        short = _hole_shortfall(pad, lead)
        if short:
            out.append(f"pin {pad['number']}: hole {short[0]:.2f} mm < lead {short[1]:.2f} mm")
    return out


def _hole_shortfall(pad: dict, lead: dict) -> tuple[float, float] | None:
    """(hole, lead size needed) when the plated hole is smaller than the lead."""
    sx, sy = lead["size_xy"]
    drill = pad["drill"]
    dx, dy = (drill, drill) if isinstance(drill, (int, float)) else drill
    if abs(dx - dy) > 1e-6:  # a slot: it must clear the lead along each axis
        short = [(d, s) for d, s in zip(sorted((dx, dy)), sorted((sx, sy))) if d < s]
        return short[0] if short else None
    need = max(sx, sy) if _peg_is_round(lead) else float(np.hypot(sx, sy))
    return (dx, need) if dx < need else None


def _match_report(m: dict) -> list[str]:
    ok = sum(1 for _k, d in m["rows"] if d is not None and d <= 0.5)
    lines = [f"{ok} pins sit on their pad (within 0.5 mm)."]
    for key, text in (("far", "Far from their pad: "), ("pins_without_pad", "Pins with no pad: "),
                      ("pads_without_pin", "Pads with no pin: ")):
        if m[key]:
            lines.append(text + ", ".join(m[key][:20]) + ("  ← check pin 1 / front / seating" if key == "far" else ""))
    return lines + [""] + [f"  pin {k}: " + ("no pad" if d is None else f"{d:.2f} mm") for k, d in m["rows"][:60]]


class FootprintEmbedTool(ToolMode):
    id = "footprint_embed"
    title = "9 Embedded Footprint"
    accent = "#117a65"
    auto_method = "_generate"

    def __init__(self, ctx) -> None:
        super().__init__(ctx)
        self._pending = None  # (path, text)

    def build_actions_widget(self) -> QWidget:
        widget = QWidget()
        layout = QVBoxLayout(widget)
        layout.setContentsMargins(8, 8, 8, 8)
        layout.addWidget(QLabel(
            "<b>Step 9 · Embedded Footprint</b><br>"
            "Generate an IPC land pattern from the model, or load the intended "
            "footprint and solve the model onto it; CONFIRM embeds it."
        ))
        generate = QPushButton("Generate land pattern (pins from geometry + IPC)")
        generate.setToolTip(
            "One step: read every pin's real contact at the board from the model\n"
            "(SMT: the faces sitting at Z=0; THT / ANCHOR / GUIDE: their holes), then\n"
            "build the IPC-7351B land pattern from it: nominal toe / heel / side\n"
            "fillets, exposed pads as-is, silkscreen with pad relief, pin-1 dot,\n"
            "courtyard. In the model's own frame: nothing is rotated or moved.")
        generate.clicked.connect(self._generate)
        layout.addWidget(generate)
        load = QPushButton("Load .kicad_mod ...")
        load.setToolTip("Load the intended footprint: the model is solved onto it (pins to pads)\n"
                        "and the footprint plus that placement are embedded")
        load.clicked.connect(self._choose)
        layout.addWidget(load)
        self.opened_button = QPushButton("Use the opened .kicad_mod")
        self.opened_button.setToolTip("The model came from a KiCad footprint: match against it.")
        self.opened_button.clicked.connect(self._use_opened)
        layout.addWidget(self.opened_button)
        self.report = QTextEdit()
        self.report.setReadOnly(True)
        layout.addWidget(self.report, 1)
        self.apply_button = make_apply_button("Embed Footprint")
        self.apply_button.setEnabled(False)
        self.apply_button.clicked.connect(self.apply)
        layout.addWidget(self.apply_button)
        return widget

    def _generate(self) -> None:
        doc = self.ctx.document
        if doc is None or not self.ctx.window.pins.pins:
            self.status("Footprint: run Z-Sit and Detect Pins first")
            return
        self._from_geometry()  # always from the model as it is now
        pads = ipc_land_pattern(doc.footprint["geometry"], doc)
        name = (getattr(doc, "identity", None) or {}).get("mpn") or doc.path.stem
        text = kicad_mod_text(f"{name}_IPC", pads, _package_xy(doc, self.ctx.window.pins.pins))
        self._pending = (f"{name}_IPC.kicad_mod", text)
        self._pending_generated = True
        self._show({**parse_kicad_mod(text), "generated": True})
        self.apply_button.setEnabled(True)
        self.status(f"Footprint: {len(pads)} IPC lands generated; CONFIRM to embed")

    def _placement(self, fp: dict):
        """Solve the model onto the footprint: where the footprint would put
        this model (stored, the geometry never moves)."""
        from ..pins import match_pins_to_pads

        pins = self.ctx.window.pins.pins
        fit = match_pins_to_pads(pins, fp["pads"]) if pins and fp["pads"] else None
        if not fit or fit.get("rms") is None:
            return None
        angle = math.radians(fit["rotation"])
        rot = np.array([[math.cos(angle), -math.sin(angle)], [math.sin(angle), math.cos(angle)]])
        offset = -(rot.T @ np.asarray(fit["shift"]))  # model origin in the footprint's frame
        return {"rotate_deg": -float(fit["rotation"]) % 360.0,
                "offset_xy_mm": [round(float(v), 4) for v in offset],
                "rms_mm": round(float(fit["rms"]), 4), "matched": len(fit["assign"])}

    def _opened_kicad_mod(self):
        vendor = getattr(self.ctx.window, "vendor", None)
        source = Path(vendor.source) if vendor is not None else None
        return source if source is not None and source.suffix.lower() == ".kicad_mod" else None

    def _use_opened(self) -> None:
        source = self._opened_kicad_mod()
        if source is None:
            self.status("Footprint: the open model didn't come from a .kicad_mod")
            return
        text = source.read_text(encoding="utf-8", errors="replace")
        self._pending = (str(source), text)
        self._pending_generated = False
        self._show(parse_kicad_mod(text))
        self.apply_button.setEnabled(True)

    def _from_geometry(self) -> None:
        doc = self.ctx.document
        pins = self.ctx.window.pins.pins
        if doc is None or not pins:
            self.status("Footprint: run Z-Sit and Detect Pins first")
            return
        geo = geometry_footprint(doc, pins)
        fp = dict(getattr(doc, "footprint", None) or {})
        fp["geometry"] = geo
        doc.footprint = fp
        self.ctx.journal.record("footprint_embed", {"action": "from_geometry", "z": SECTION_Z},
                                {}, {"pads": len(geo["pads"]), "unresolved": geo["unresolved"]})
        kinds = {}
        for pad in geo["pads"]:
            kinds[pad["kind"]] = kinds.get(pad["kind"], 0) + 1
        lines = [f"Pin outlines at Z={SECTION_Z} mm: {len(geo['pads'])} of {len(pins)} pins "
                 f"({', '.join(f'{v} {k}' for k, v in sorted(kinds.items()))})."]
        if geo["unresolved"]:
            lines.append("No outline (nothing crosses the plane there — an SMT foot exactly "
                         "on Z=0, or a mouth contact): " + ", ".join(geo["unresolved"][:30]))
        lines.append("")
        lines += [f"  {p['pin']:>8} {p['kind']:<6} {p['size_xy'][0]:.3f} x {p['size_xy'][1]:.3f} mm"
                  f" at ({p['centre_xy'][0]:+.3f}, {p['centre_xy'][1]:+.3f})"
                  + (f"  {p['hole']} hole" if p["hole"] else "") for p in geo["pads"][:80]]
        self.report.setPlainText("\n".join(lines))
        self.status(f"Footprint: {len(geo['pads'])} pin outlines embedded from geometry")

    def enter(self) -> None:
        self.status("Footprint: build it from geometry, or load the part's .kicad_mod")
        if self._actions_widget is not None:
            self.opened_button.setVisible(self._opened_kicad_mod() is not None)
        doc = self.ctx.document
        if doc is not None and (getattr(doc, "footprint", None) or {}).get("pads"):  # outlines alone: nothing to show
            self._show(doc.footprint)

    def exit(self) -> None:
        self.ctx.scene.clear_land_pattern()
        self.ctx.scene.plotter.render()

    def _choose(self) -> None:
        start = str(self.ctx.document.path.parent) if self.ctx.document else ""
        path, _ = QFileDialog.getOpenFileName(self._actions_widget, "Footprint", start,
                                              "KiCad footprint (*.kicad_mod);;All files (*)")
        if not path:
            return
        text = Path(path).read_text(encoding="utf-8", errors="replace")
        self._pending = (path, text)
        self._pending_generated = False
        self._show(parse_kicad_mod(text))
        self.apply_button.setEnabled(True)

    def _show(self, fp: dict) -> None:
        pins = self.ctx.window.pins.pins
        # a generated land pattern is in the model's own frame: nothing to solve
        placement = self._placement(fp) if pins and not fp.get("generated") else None
        pads = _in_model_frame(fp["pads"], placement)
        self._draw_land(fp, pads, placement)
        m = match_pins([p for p in pins if p.role != "mouth"], pads)
        lines = [f"Footprint {fp.get('name') or '?'}: {len(fp['pads'])} pads, {len(pins)} pins."]
        lines += _match_report(m) if pins else ["No pins yet: run Detect Pins first to check the match."]
        small = _undersized_holes(pads, (getattr(self.ctx.document, "footprint", None) or {}).get("geometry"))
        if small:
            lines.insert(1, "HOLES TOO SMALL for their leads (press Generate land pattern to rebuild): "
                            + ", ".join(small))
            self.status("Footprint: " + str(len(small)) + " plated hole(s) smaller than their lead - press Generate")
        if placement:
            lines.insert(1, f"Model placement on this footprint: rotate {placement['rotate_deg']:.0f}°, "
                            f"offset ({placement['offset_xy_mm'][0]:+.3f}, {placement['offset_xy_mm'][1]:+.3f}) mm, "
                            f"fit {placement['rms_mm']:.3f} mm rms over {placement['matched']} pins.")
        self.report.setPlainText("\n".join(lines))

    def _draw_land(self, fp: dict, pads: list[dict], placement) -> None:
        """Pads, mask openings, silkscreen and courtyard in the 3D view (the
        silk / courtyard lines only when the footprint is in the model's frame)."""
        if not self.is_active():
            return  # refreshed while another tool is on screen: draw nothing
        text = fp.get("kicad_mod") or (self._pending[1] if self._pending else "")
        if text:  # drills come from the footprint text (older embedded pads lack them)
            pads = _in_model_frame(parse_kicad_mod(text)["pads"], placement)
        lines = footprint_lines(text)
        self.ctx.scene.show_land_pattern(pads, mask=MASK_MARGIN, silk=[] if placement else lines["silk"],
                                         courtyard_lines=[] if placement else lines["courtyard"])
        rect = courtyard(_package_xy(self.ctx.document, self.ctx.window.pins.pins), pads, excess=2.0)
        self.ctx.scene.show_ghost_board(pads, rect)  # a generic 1.6 mm PCB under the part

    def apply(self) -> None:
        doc = self.ctx.document
        if doc is None or self._pending is None:
            return
        path, text = self._pending
        fp = apply_footprint(doc, path, text)
        generated = getattr(self, "_pending_generated", False)
        if generated:
            fp["generated"] = True  # built in the model's frame: nothing to place
        placement = None if generated else self._placement(fp)
        if placement:
            fp["placement"] = placement  # metadata only: the model keeps its own frame
        self._embedded = self._pending
        self.ctx.journal.record("footprint_embed", {"action": "embed", "file": fp["file"]}, {"kicad_mod": text},
                                {"pads": len(fp["pads"]), "placement": placement})
        self.status(f"Footprint {fp['name']} embedded ({len(fp['pads'])} pads)")

    def staged_rows(self) -> list[tuple[str, str, str]]:
        """A generated / chosen footprint that is not embedded yet."""
        if self._pending is None or getattr(self, "_embedded", None) == self._pending:
            return []
        pads = len(parse_kicad_mod(self._pending[1])["pads"])
        return [("footprint", f"{Path(self._pending[0]).name}: {pads} pads", "not embedded: CONFIRM")]
