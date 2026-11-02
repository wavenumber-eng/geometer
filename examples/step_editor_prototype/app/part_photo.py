"""Reference photo of the real part -> a colour palette for the Colours step.

Option A (2026-11-01): the person drops, pastes or loads a photo of the real
part (e.g. from a distributor page or an image search). Nothing is fetched on
its own and no API key is needed, so the feature works for every user.

The photo never paints anything by itself. It only proposes:
- `extract_palette`: the part's own colours (background removed, k-means),
  each flagged metal or not;
- `suggest_roles`: which photo colour is the body and which is the metal;
- `plan_mapping`: for each colour group already on the model, a proposed photo
  colour (pin groups -> metal, the main non-pin group -> body, others kept).
The Colours page shows all of it with its settings, previews it on the model and
applies only when the person accepts (human controls every fix).

Pure numpy: the Qt side hands in an (H, W, 3) uint8 RGB array.
"""

from __future__ import annotations

import hashlib
from datetime import datetime, timezone

import numpy as np

MAX_SIDE = 400  # pixels: plenty for a palette, keeps k-means instant
SAMPLE = 20000  # foreground pixels clustered at most
METAL_SATURATION = 0.18  # HSV saturation below this ...
METAL_VALUE = 0.45  # ... and value above this reads as bare metal / tin
MERGE_DISTANCE = 18.0  # RGB distance (0-255) under which two clusters are one colour
DEFAULT_COLOURS = 4
DEFAULT_TOLERANCE = 40


def downscale(rgb: np.ndarray, max_side: int = MAX_SIDE) -> np.ndarray:
    """Nearest-neighbour shrink so the longest side is at most max_side."""
    h, w = rgb.shape[:2]
    step = max(1, int(np.ceil(max(h, w) / max_side)))
    return rgb[::step, ::step]


def background_colour(rgb: np.ndarray) -> np.ndarray:
    """The median colour of the image's outer ring (product photos sit on a
    plain backdrop that fills the border)."""
    h, w = rgb.shape[:2]
    band = max(1, int(0.04 * min(h, w)))
    ring = np.concatenate([rgb[:band].reshape(-1, 3), rgb[-band:].reshape(-1, 3),
                           rgb[:, :band].reshape(-1, 3), rgb[:, -band:].reshape(-1, 3)])
    return np.median(ring, axis=0)


def foreground(rgb: np.ndarray, tolerance: int = DEFAULT_TOLERANCE) -> np.ndarray:
    """(N, 3) pixels that differ from the backdrop by more than `tolerance` in
    any channel. If almost nothing survives (no plain backdrop), all pixels."""
    pixels = rgb.reshape(-1, 3).astype(float)
    keep = np.abs(pixels - background_colour(rgb)).max(axis=1) > tolerance
    return pixels[keep] if keep.mean() >= 0.03 else pixels


def kmeans(pixels: np.ndarray, k: int, seed: int = 0, iterations: int = 25):
    """Deterministic k-means (k-means++ start). Returns (centres, labels)."""
    rng = np.random.default_rng(seed)
    if len(pixels) > SAMPLE:
        pixels = pixels[rng.choice(len(pixels), SAMPLE, replace=False)]
    k = max(1, min(k, len(np.unique(pixels, axis=0))))
    centres = [pixels[rng.integers(len(pixels))]]
    for _ in range(1, k):
        d = np.min([((pixels - c) ** 2).sum(axis=1) for c in centres], axis=0)
        centres.append(pixels[rng.choice(len(pixels), p=d / d.sum())] if d.sum() else pixels[0])
    centres = np.array(centres, float)
    for _ in range(iterations):
        labels = np.argmin(((pixels[:, None, :] - centres[None]) ** 2).sum(axis=2), axis=1)
        moved = np.array([pixels[labels == j].mean(axis=0) if (labels == j).any() else centres[j]
                          for j in range(len(centres))])
        if np.allclose(moved, centres):
            break
        centres = moved
    return centres, labels


def is_metal(rgb255) -> bool:
    r, g, b = (np.asarray(rgb255, float) / 255.0)
    value, low = max(r, g, b), min(r, g, b)
    saturation = 0.0 if value == 0 else (value - low) / value
    return bool(saturation < METAL_SATURATION and value > METAL_VALUE)  # plain bool: goes into JSON


def _merge(centres: np.ndarray, shares: np.ndarray) -> list[tuple[np.ndarray, float]]:
    """Join clusters closer than MERGE_DISTANCE (share-weighted centre)."""
    merged: list[list] = []
    for c, s in sorted(zip(centres, shares), key=lambda cs: -cs[1]):
        near = next((m for m in merged if np.linalg.norm(m[0] - c) < MERGE_DISTANCE), None)
        if near is None:
            merged.append([c, s])
        else:
            near[0] = (near[0] * near[1] + c * s) / (near[1] + s)
            near[1] += s
    return [(c, s) for c, s in merged]


def to_hex(rgb255) -> str:
    return "#" + "".join(f"{int(round(float(v))):02x}" for v in np.clip(rgb255, 0, 255))


# The neutral plating swatches of the Colours page (app/tools/colors.py
# SWATCHES; a test keeps the two in step). A metal colour from a photo snaps
# to one of these: a photo's metal is a grey tinted by whatever it reflects
# (1984714's tin pins came out green-grey from the green housing).
METAL_SWATCHES = [("Sn", "#c0c0c0", "tin"), ("Ag", "#e1e1e1", "silver"),
                  ("Ni", "#9da1a6", "nickel / steel"), ("ZN", "#6e7378", "zinc / dark metal")]
LUMA = np.array([0.299, 0.587, 0.114])
LIT_PERCENTILE = 90  # plated metal's colour is its LIT side: shading drags the mean down


def _hex_rgb(hex_color: str) -> np.ndarray:
    return np.array([int(hex_color[i:i + 2], 16) for i in (1, 3, 5)], float)


def snap_metal(lit_luma: float) -> tuple[str, str, str]:
    """(name, hex, meaning) of the metal swatch nearest in brightness."""
    return min(METAL_SWATCHES, key=lambda s: abs(_hex_rgb(s[1]) @ LUMA - lit_luma))


def _lit_luma(pixels: np.ndarray, centres: np.ndarray, index: int) -> float:
    """Brightness of the lit side of one palette colour: the LIT_PERCENTILE
    luma of the pixels nearest that colour."""
    nearest = np.argmin(((pixels[:, None, :] - centres[None]) ** 2).sum(axis=2), axis=1)
    luma = pixels[nearest == index] @ LUMA
    return float(np.percentile(luma, LIT_PERCENTILE)) if len(luma) else float(centres[index] @ LUMA)


def _entry(pixels, centres, index, share) -> dict:
    c = centres[index]
    entry = {"hex": to_hex(c), "rgb": [round(float(v) / 255.0, 4) for v in c],
             "share": round(float(share), 4), "metal": is_metal(c)}
    if entry["metal"]:
        name, hex_color, meaning = snap_metal(_lit_luma(pixels, centres, index))
        entry.update(swatch=name, swatch_hex=hex_color, swatch_meaning=meaning)
    return entry


def extract_palette(rgb: np.ndarray, colours: int = DEFAULT_COLOURS,
                    tolerance: int = DEFAULT_TOLERANCE) -> list[dict]:
    """The part's colours, largest share first: hex, rgb (0-1), share, metal;
    a metal colour also carries the plating swatch it snaps to (swatch,
    swatch_hex, swatch_meaning), judged by its lit side."""
    pixels = foreground(downscale(rgb), tolerance)
    centres, labels = kmeans(pixels, colours)
    shares = np.bincount(labels, minlength=len(centres)) / max(1, len(labels))
    merged = [(c, s) for c, s in _merge(centres, shares) if s > 0]
    kept = np.array([c for c, _s in merged])
    return _plastic_family([_entry(pixels, kept, i, s) for i, (_c, s) in enumerate(merged)])


MOSTLY_NEUTRAL = 0.5    # neutral colours covering more than this share of the part are its plastic
SAME_TINT = 0.025       # chromaticity distance under which two colours are one material's shading


def _chroma(entry: dict) -> np.ndarray:
    rgb = np.asarray(entry["rgb"], float)
    return rgb / max(float(rgb.sum()), 1e-9)


def _plastic_family(palette: list[dict]) -> list[dict]:
    """A part is rarely mostly metal. When the colours that read as metal
    (bright, unsaturated) cover more than MOSTLY_NEUTRAL of the part, the
    largest of them is the plastic body (a white / natural / grey housing:
    JST JWPF), and those sharing its tint are its shading; they lose the metal
    flag and point at it ('shade_of'). A neutral of another tint can still be
    the metal. (A part that IS mostly metal, a shield can, is then proposed
    as plastic; every Match row still offers the plating swatches.)"""
    neutral = [p for p in palette if p["metal"]]
    if sum(p["share"] for p in neutral) <= MOSTLY_NEUTRAL:
        return palette
    body = max(neutral, key=lambda p: p["share"])
    for p in neutral:
        if p is body or np.linalg.norm(_chroma(p) - _chroma(body)) < SAME_TINT:
            _make_plastic(p, None if p is body else body["hex"])
    return palette


def _make_plastic(entry: dict, shade_of: str | None) -> None:
    entry["metal"] = False
    for key in ("swatch", "swatch_hex", "swatch_meaning"):
        entry.pop(key, None)
    if shade_of:
        entry["shade_of"] = shade_of


def paint_hex(entry: dict) -> str:
    """The colour a palette entry paints with: its plating swatch for metal,
    the photo colour itself otherwise."""
    return entry.get("swatch_hex") or entry["hex"]


def _family_share(entry: dict, palette: list[dict]) -> float:
    """A colour's share plus the shares of its shading."""
    return entry["share"] + sum(p["share"] for p in palette if p.get("shade_of") == entry["hex"])


def suggest_roles(palette: list[dict]) -> dict:
    """{'body': hex, 'metal': hex or None}: the body is the non-metal colour
    with the largest share counting its shading (the largest of all if every
    colour reads as metal); the metal is the largest metal colour that isn't
    the body, as its plating swatch."""
    if not palette:
        return {"body": None, "metal": None}
    body = _body_entry(palette)
    metal = next((p for p in palette if p["metal"] and p is not body), None)
    return {"body": body["hex"], "metal": paint_hex(metal) if metal else None}


def _body_entry(palette: list[dict]) -> dict:
    """The plastic colour with the largest share counting its shading; the
    largest colour of all when every colour reads as metal."""
    plastic = [p for p in palette if not p["metal"] and not p.get("shade_of")]
    return max(plastic, key=lambda p: _family_share(p, palette)) if plastic else palette[0]


# --------------------------------------------------------------- the model side

DEFAULT_RGB = (0.62, 0.64, 0.66)  # what the editor shows for an uncoloured face


def _pin_faces(registry) -> tuple[set, set]:
    """(pin body indices, pin (body, face) keys) from the Detect Pins registry."""
    bodies, faces = set(), set()
    for pin in getattr(registry, "pins", []) or []:
        bodies.update(pin.body_ids)
        faces.update(tuple(k) for k in pin.face_ids)
    return bodies, faces


def _face_rgb(body, face: int) -> tuple:
    rgb = body.mesh.face_colors.get(face) or body.color or DEFAULT_RGB
    return tuple(round(float(c), 3) for c in rgb)


def _add_faces(groups: dict, b: int, body, areas: dict, on_pin_body: bool, pin_faces: set) -> None:
    for face, area in areas.items():
        g = groups.setdefault(_face_rgb(body, face), {"area": 0.0, "pin_area": 0.0, "keys": []})
        g["area"] += area
        g["pin_area"] += area if on_pin_body or (b, face) in pin_faces else 0.0
        g["keys"].append((b, int(face)))


def _pins_known(document, pin_bodies: set, pin_faces: set) -> bool:
    return bool(pin_bodies or pin_faces) or any(getattr(b, "role", None) == "pin" for b in document.bodies)


def _pin_share(group: dict, rgb, pins_known: bool) -> float:
    """Share of the group's area on pins. With no pins detected yet (no Detect
    Pins, no pin bodies), a group whose own colour reads as metal counts as pins."""
    if pins_known:
        return group["pin_area"] / group["area"] if group["area"] else 0.0
    return 1.0 if is_metal(np.array(rgb) * 255) else 0.0


def colour_groups(document, registry=None) -> list[dict]:
    """The model's current colours as groups, largest area first: rgb, hex,
    area share, share of the group's area on pins, and its (body, face) keys."""
    from .pins import mesh_face_areas

    pin_bodies, pin_faces = _pin_faces(registry)
    groups: dict[tuple, dict] = {}
    for b, body in enumerate(document.bodies):
        if body.mesh is not None:
            on_pin_body = b in pin_bodies or getattr(body, "role", None) == "pin"
            _add_faces(groups, b, body, mesh_face_areas(body.mesh), on_pin_body, pin_faces)
    total = sum(g["area"] for g in groups.values()) or 1.0
    known = _pins_known(document, pin_bodies, pin_faces)
    out = [{"rgb": list(rgb), "hex": to_hex(np.array(rgb) * 255), "share": g["area"] / total,
            "on_pins": _pin_share(g, rgb, known), "keys": g["keys"]}
           for rgb, g in groups.items()]
    return sorted(out, key=lambda g: -g["share"])


def plan_mapping(groups: list[dict], roles: dict) -> list[str | None]:
    """A proposed photo colour per model group (None = keep): groups mostly on
    pins take the metal colour; the largest group not on pins takes the body
    colour; every other group keeps its colour."""
    plan: list[str | None] = []
    body_done = False
    for g in groups:
        if g["on_pins"] >= 0.5 and roles.get("metal"):
            plan.append(roles["metal"])
        elif not body_done and g["on_pins"] < 0.5 and roles.get("body"):
            plan.append(roles["body"])
            body_done = True
        else:
            plan.append(None)
    return plan


def photo_record(raw: bytes, name: str, source: str, palette: list[dict],
                 colours: int, tolerance: int) -> dict:
    """What the export keeps about the photo (for the ML and for provenance):
    no pixels, only a hash to find it again, where it came from and its palette."""
    return {"name": name, "source": source, "sha256": hashlib.sha256(raw).hexdigest(),
            "loaded_utc": datetime.now(timezone.utc).isoformat(timespec="seconds"),
            "colours": colours, "tolerance": tolerance,
            "palette": [{k: p[k] for k in ("hex", "share", "metal", "swatch", "swatch_hex") if k in p}
                        for p in palette]}
