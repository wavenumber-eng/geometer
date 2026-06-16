"""Learned Z-Sit seat orientation — the trained half of step 1.

Pure geometry can't pick the seating plane (a resistor and a capacitor look
alike but seat oppositely; YZ parts stand upright); the convention is part-type
knowledge. So we featurise every candidate "up" direction of a model and let a
model trained on the user's REF seatings RANK them. Leave-one-out over the REF
set: 56/59 vs 32/59 for the best deterministic rule.

`predict_up(document)` returns the best up vector (or None when no model is
trained yet, so compute_auto_zsit falls back to the deterministic rule).
"""

from __future__ import annotations

from pathlib import Path

import numpy as np

MODEL_PATH = Path(__file__).resolve().parent / "seat_model.joblib"
LEVEL_MODEL_PATH = Path(__file__).resolve().parent / "seat_level_model.joblib"
STATS_PATH = Path(__file__).resolve().parent / "seat_model_stats.json"
_MATCH_DEG = 12.0  # a candidate counts as the seat if within this of the REF up
_LEVEL_FRAC = 0.02  # a candidate level counts as the seat if within this*span


def fidelity_summary() -> str:
    """One-line trained leave-one-out fidelity from the last training run —
    written by train_seat_model.py, read into the AUTO button's tooltip."""
    if not STATS_PATH.is_file():
        return "not yet benchmarked — run train_seat_model.py"
    import json

    try:
        s = json.loads(STATS_PATH.read_text())
        o, lv = s["orientation"], s["level"]

        def pct(a, b):
            return f"{round(100 * a / b)}%" if b else "—"

        return (f"orientation {pct(o['loo_hand'], o['n_hand'])} hand-seated "
                f"({pct(o['loo_all'], o['n_all'])} overall) · "
                f"level {pct(lv['loo_hand'], lv['n_hand'])} hand "
                f"({pct(lv['loo_all'], lv['n_all'])} overall) · "
                f"{s['n_refs']} references")
    except Exception:  # noqa: BLE001
        return "fidelity stats unavailable"


def seat_chunks(document):
    """(face centres, unit normals, areas, all points) over every body."""
    centres, normals, areas, pts = [], [], [], []
    for body in document.bodies:
        mesh = body.mesh
        if mesh is None or not len(mesh.tris):
            continue
        corners = mesh.points[mesh.tris]
        cross = np.cross(corners[:, 1] - corners[:, 0], corners[:, 2] - corners[:, 0])
        length = np.linalg.norm(cross, axis=1)
        keep = length > 1e-12
        centres.append(corners.mean(axis=1)[keep])
        normals.append(cross[keep] / length[keep, None])
        areas.append(0.5 * length[keep])
        pts.append(mesh.points)
    if not centres:
        return None
    return (np.concatenate(centres), np.concatenate(normals),
            np.concatenate(areas), np.concatenate(pts))


def candidate_ups(normals, areas):
    """Candidate seat 'up' directions: the six axes plus the model's dominant
    face normals (both signs, so a flip is a candidate the ranker can pick)."""
    base = [np.array(v, dtype=np.float64) for v in
            ((1, 0, 0), (-1, 0, 0), (0, 1, 0), (0, -1, 0), (0, 0, 1), (0, 0, -1))]
    for index in np.argsort(-areas)[:4000]:
        normal = normals[index]
        for signed in (normal, -normal):
            if all(float(signed @ c) < 0.985 for c in base):
                base.append(signed.copy())
        if len(base) >= 40:
            break
    return [c / np.linalg.norm(c) for c in base]


def _basis(up):
    helper = np.array([1.0, 0, 0]) if abs(up[0]) < 0.9 else np.array([0, 1.0, 0])
    u = helper - up * float(helper @ up)
    u /= np.linalg.norm(u)
    return u, np.cross(up, u)


def _hull_area(points_2d) -> float:
    """Convex-hull area of 2D points (rotation-invariant), 0 if degenerate."""
    from scipy.spatial import ConvexHull

    if len(points_2d) < 3:
        return 0.0
    try:
        return float(ConvexHull(points_2d).volume)
    except Exception:  # noqa: BLE001 — collinear / degenerate projection
        return 0.0


def seat_features(up, centres, normals, areas, pts, total_area, diag):
    """How 'seat-like' `up` is — every feature ROTATION-INVARIANT about up (and
    scale-invariant), so the same physical seat scores identically no matter
    how the part is oriented in the file: footprint, downward pad area +
    hull-coverage, top area (sign), downward-face fraction, height, flatness
    aspect, and bulk side. Must match the training extractor exactly."""
    u, v = _basis(up)
    z = pts @ up
    height = float(z.max() - z.min())
    footprint = _hull_area(np.column_stack([pts @ u, pts @ v]))
    cz = centres @ up
    band = max(0.04 * height, 0.2)
    down = (normals @ up < -0.9) & (cz <= z.min() + band)
    top = (normals @ up > 0.9) & (cz >= z.max() - band)
    pad_foot = _hull_area(np.column_stack([centres[down] @ u, centres[down] @ v]))
    cov = pad_foot / max(footprint, 1e-9)
    bulk = float((pts.mean(0) - (pts.max(0) + pts.min(0)) / 2) @ up) / max(height, 1e-9)
    return [
        footprint / max(diag ** 2, 1e-9),
        float(areas[down].sum()) / total_area,
        min(cov, 1.0),
        float(areas[top].sum()) / total_area,
        float((normals @ up < -0.7).mean()),
        height / max(diag, 1e-9),
        height / max(np.sqrt(footprint), 1e-9),  # flatness aspect (invariant)
        bulk,
    ]


def candidate_seats(document):
    """(candidate up vectors, feature matrix) for one model, or (None, None)."""
    data = seat_chunks(document)
    if data is None:
        return None, None
    centres, normals, areas, pts = data
    diag = float(np.linalg.norm(pts.max(0) - pts.min(0)))
    total = float(areas.sum()) or 1.0
    ups = candidate_ups(normals, areas)
    feats = np.array([
        seat_features(up, centres, normals, areas, pts, total, diag) for up in ups
    ])
    return ups, feats


def predict_up(document):
    """Best seat up vector from the trained model, or None if no model yet."""
    if not MODEL_PATH.is_file():
        return None
    import joblib

    ups, feats = candidate_seats(document)
    if ups is None:
        return None
    model = joblib.load(MODEL_PATH)
    scores = model.predict_proba(feats)[:, 1]
    return ups[int(np.argmax(scores))]


# ── seat LEVEL (where z=0 goes, given the orientation) ───────────────────────

def _level_candidates(centres, normals, areas, pts, up, tol):
    """Candidate seat levels along `up`: each downward-flat-face cluster, plus
    the part's absolute lowest extent (for BGA balls / round tips that have no
    flat face there). Sorted ascending, deduped within tol."""
    z = pts @ up
    cands = [float(z.min())]
    mask = normals @ up < -0.85
    if mask.any():
        cz = centres[mask] @ up
        ca = areas[mask]
        order = np.argsort(cz)
        scz, sca = cz[order], ca[order]
        group = [0]
        for k in range(1, len(order)):
            if scz[k] - scz[k - 1] <= tol:
                group.append(k)
            else:
                cands.append(float((scz[group] * sca[group]).sum()
                                   / max(sca[group].sum(), 1e-12)))
                group = [k]
        cands.append(float((scz[group] * sca[group]).sum()
                           / max(sca[group].sum(), 1e-12)))
    cands.sort()
    out = [cands[0]]
    for c in cands[1:]:
        if c - out[-1] > tol:
            out.append(c)
    return out


def _level_features(level, centres, normals, areas, pts, up, ctx):
    """Per-candidate-level features (scale-invariant): where it sits, its flat
    area + footprint coverage, mass below it (leads), gap to the next lower
    level (lead length), and mean triangle size (BGA ball tips are tiny)."""
    zmin, span, total_flat, max_area, footprint, u, v, diag, levels, tol = ctx
    cz, pz = centres @ up, pts @ up
    flat = (np.abs(cz - level) <= tol) & (normals @ up < -0.85)
    area = float(areas[flat].sum())
    near_pts = pts[np.abs(pz - level) <= tol]
    cov = (_hull_area(np.column_stack([near_pts @ u, near_pts @ v])) / footprint
           if len(near_pts) >= 3 and footprint > 0 else 0.0)
    lower = [lv for lv in levels if lv < level - tol]
    gap_lower = (level - max(lower)) / span if lower else 0.0
    n_tri = int(flat.sum())
    return [
        (level - zmin) / span,                 # where along the part
        area / max(total_flat, 1e-9),          # flat-area fraction
        area / max(max_area, 1e-9),            # vs the biggest flat level
        min(cov, 1.0),                          # footprint coverage at this level
        float((pz < level).mean()),            # mass below (leads below the seat)
        1.0 if abs(level - zmin) <= tol else 0.0,  # is the absolute bottom
        gap_lower,                              # gap to next lower level (lead len)
        (area / max(n_tri, 1)) / max(diag * diag, 1e-9),  # mean tri area (BGA tips)
    ]


def candidate_levels(document, up):
    """(candidate seat levels along up, feature matrix), or ([], None)."""
    data = seat_chunks(document)
    if data is None:
        return [], None
    centres, normals, areas, pts = data
    up = np.asarray(up, dtype=np.float64)
    up = up / float(np.linalg.norm(up))
    z = pts @ up
    span = max(float(z.max() - z.min()), 1e-9)
    tol = max(0.01 * span, 1e-4)
    levels = _level_candidates(centres, normals, areas, pts, up, tol)
    u, v = _basis(up)
    footprint = _hull_area(np.column_stack([pts @ u, pts @ v]))
    mask = normals @ up < -0.85
    total_flat = float(areas[mask].sum()) if mask.any() else 1.0
    cz = centres @ up
    max_area = max((float(areas[(np.abs(cz - L) <= tol)
                                & (normals @ up < -0.85)].sum()) for L in levels),
                   default=1.0)
    diag = float(np.linalg.norm(pts.max(0) - pts.min(0)))
    ctx = (float(z.min()), span, total_flat, max_area, footprint, u, v, diag,
           levels, tol)
    feats = np.array([
        _level_features(L, centres, normals, areas, pts, up, ctx) for L in levels
    ])
    return levels, feats


def vertex_plane_levels(document, up, tol=None, min_cov=0.04):
    """Z-levels (along `up`) where the mesh VERTICES form a real plane — a tight
    z-cluster whose XY footprint covers at least `min_cov` of the model
    footprint. A flat feature perpendicular to up deposits a dense band of
    coplanar vertices with real XY spread; an edge or corner deposits a band
    with none. Returns [(level, coverage)] sorted ascending by level.

    This is the snap target for the user's decomposition: the ML gets the seat
    orientation + a roughly-correct level, and we snap that level onto the
    nearest real planar vertex formation so the standoff is EXACT (flush with a
    true feature) instead of merely close."""
    data = seat_chunks(document)
    if data is None:
        return []
    _centres, _normals, _areas, pts = data
    up = np.asarray(up, dtype=np.float64)
    up = up / float(np.linalg.norm(up))
    z = pts @ up
    span = max(float(z.max() - z.min()), 1e-9)
    tol = tol if tol is not None else max(0.004 * span, 1e-4)
    u, v = _basis(up)
    foot = _hull_area(np.column_stack([pts @ u, pts @ v])) or 1e-9

    order = np.argsort(z)
    zs = z[order]
    pu, pv = (pts @ u)[order], (pts @ v)[order]
    levels: list[tuple[float, float]] = []
    start = 0
    for k in range(1, len(zs) + 1):
        # close a cluster when the next vertex jumps more than tol above the
        # cluster's running floor (a band thicker than tol is a ramp, not a plane)
        if k == len(zs) or zs[k] - zs[start] > tol:
            grp = slice(start, k)
            if k - start >= 3:
                cov = _hull_area(np.column_stack([pu[grp], pv[grp]])) / foot
                if cov >= min_cov:
                    levels.append((float(zs[grp].mean()), float(min(cov, 1.0))))
            start = k
    return levels


def snap_level_to_vertex_plane(document, up, level, max_snap_frac=0.06):
    """Snap a roughly-correct seat `level` onto the nearest planar vertex
    formation — but only when it actually helps:

    * DEADBAND — if the level is already flush with a plane (within the cluster
      tolerance), it is seated on a real feature; leave it. This stops the snap
      from nudging an already-exact seat off by the cluster-mean-vs-true-min
      difference.
    * GUARD — only snap when the nearest plane is within `max_snap_frac` of the
      model height. A grossly-wrong level is a level-model failure that snapping
      must not paper over (the nearest plane there is not the seat).

    So the snap fires exactly in the useful band: the level sits in no-man's-land
    between planes, close enough that the nearest one is clearly the intended
    seat (the 'almost flush' / standoff-not-flush cases)."""
    data = seat_chunks(document)
    if data is None:
        return level
    _c, _n, _a, pts = data
    up = np.asarray(up, dtype=np.float64)
    up = up / float(np.linalg.norm(up))
    z = pts @ up
    span = max(float(z.max() - z.min()), 1e-9)
    tol = max(0.004 * span, 1e-4)
    planes = vertex_plane_levels(document, up, tol=tol)
    if not planes:
        return level
    nearest, dist = level, float("inf")
    for plane_level, _cov in planes:
        d = abs(plane_level - level)
        if d < dist:
            nearest, dist = plane_level, d
    if dist <= tol:          # already flush with a real plane — leave it
        return level
    return nearest if dist <= max_snap_frac * span else level


def predict_level(document, up):
    """Best seat level (z along up) from the trained level model, or None."""
    if not LEVEL_MODEL_PATH.is_file():
        return None
    import joblib

    levels, feats = candidate_levels(document, up)
    if not levels:
        return None
    model = joblib.load(LEVEL_MODEL_PATH)
    scores = model.predict_proba(feats)[:, 1]
    return levels[int(np.argmax(scores))]
