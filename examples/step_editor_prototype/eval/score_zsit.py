"""Step 1 of the staged AUTO build: AUTO Z-Sit vs the user's hand REF.

The user hand-defines the Z-Sit plane for each TEST_STEP_FILES model and saves
it as a Z-Sit-only conditioned file (its embedded journal has just the zsit op).
This script finds every such REF, runs compute_auto_zsit on the source, WRITES
the auto-seated result into Z_Sit_Output/ for visual comparison, and scores
whether the auto seat matches the hand REF:

  up err   angle between the REF's seated +Z axis and AUTO's (the seating plane)
  z-min    is AUTO seated on z=0 (and does it match the REF's z-min)?
  ctr      XY-centre offset between AUTO and REF seatings

Goal: 100% PASS, then we move to the next tool. Run:
    uv run python score_zsit.py
"""

from __future__ import annotations

import math
import sys
from pathlib import Path

import numpy as np

PROTO = Path(__file__).resolve().parents[1]  # prototype root (script in eval/)
sys.path.insert(0, str(PROTO))

TEST_DIR = PROTO / "TEST_STEP_FILES"
REF_DIR = PROTO / "REFERENCE_STEP_FILES"
OUT_DIR = TEST_DIR / "Z_Sit_Output"
FIXTURES = PROTO.parents[1] / "tests" / "fixtures" / "step" / "embedded_models"
SUFFIX = "_AP242_conditioned"
UP_TOL_DEG = 3.0
ZMIN_TOL = 0.15      # match the REF's z-min (NOT zero — pegs hang below the seat)
CENTER_FRAC_TOL = 0.02  # XY-centre offset as a fraction of the model diagonal
CENTER_ABS_TOL = 0.15   # ...or this absolute mm floor (tiny parts: 2% is too strict)
ORTHO_DEV_TOL = 6.0     # as-seated straight edges should sit within ~6° of an axis
ORTHO_RESIDUAL_TOL = 3.0  # ...and a "fixable" rotation is one an in-plane spin
                          # brings under this (round parts stay above → not flagged)
ORTHO_GROSS_TOL = 15.0  # ...or flag outright if it's grossly off (no accept > ~10°)
ORTHO_MIN_STRAIGHT = 1.0  # only judge when straight-edge length ≥ the diagonal


def _collect_refs() -> list:
    """(base, source, zsit_matrix) for every unified reference seating —
    all hand REFs now live in REFERENCE_STEP_FILES (see app/refs.py)."""
    from app import refs

    return [(r.base, r.source, r.zsit_matrix()) for r in refs.seatings()]


def _up(matrix) -> np.ndarray:
    vec = np.asarray(matrix)[2, :3]
    norm = float(np.linalg.norm(vec))
    return vec / norm if norm > 1e-12 else vec


def _seated(matrix, pts) -> np.ndarray:
    matrix = np.asarray(matrix)
    return pts @ matrix[:3, :3].T + matrix[:3, 3]


def _write_auto(doc, matrix, source) -> None:
    from app.export_ap242 import export_ap242
    from app.journal import Journal

    OUT_DIR.mkdir(parents=True, exist_ok=True)
    doc.apply_trsf(matrix)
    journal = Journal()
    journal.record("zsit", {"action": "auto"}, {},
                   {"matrix": np.asarray(matrix).tolist()})
    try:
        export_ap242(doc, OUT_DIR / f"{source.stem}_ZSIT_AUTO.step", journal=journal)
    except Exception:  # noqa: BLE001 — a write hiccup must not abort scoring
        pass


def _line_edges(source: Path):
    """(unit direction, length) for every straight (LINE) edge of the SOURCE B-rep,
    plus their total length. OCC lines are unit-speed, so the parameter span IS the
    edge length. Used to score how axis-aligned the SEATED part ends up."""
    from OCP.BRepAdaptor import BRepAdaptor_Curve
    from OCP.GeomAbs import GeomAbs_Line
    from OCP.IFSelect import IFSelect_RetDone
    from OCP.STEPControl import STEPControl_Reader
    from OCP.TopAbs import TopAbs_EDGE
    from OCP.TopExp import TopExp_Explorer
    from OCP.TopoDS import TopoDS

    reader = STEPControl_Reader()
    if reader.ReadFile(str(source)) != IFSelect_RetDone:
        return [], 0.0
    reader.TransferRoots()
    shape = reader.OneShape()
    lines, total = [], 0.0
    exp = TopExp_Explorer(shape, TopAbs_EDGE)
    while exp.More():
        edge = TopoDS.Edge_s(exp.Current())
        exp.Next()
        try:
            curve = BRepAdaptor_Curve(edge)
            length = curve.LastParameter() - curve.FirstParameter()
            if curve.GetType() != GeomAbs_Line or not (length > 1e-6):
                continue
            d = curve.Line().Direction()
            lines.append(((d.X(), d.Y(), d.Z()), length))
            total += length
        except Exception:  # noqa: BLE001
            continue
    return lines, total


def _wmean_dev(dirs, lens):
    """Length-weighted mean angle (deg) of edge directions to their nearest axis."""
    align = np.clip(np.max(np.abs(dirs), axis=1), 0.0, 1.0)   # 1.0 = on-axis
    return float(np.average(np.degrees(np.arccos(align)), weights=lens))


def _ortho_metric(source: Path, matrix):
    """How axis-aligned are the SEATED straight edges, and could an in-plane spin
    fix them? Returns (as_seated_dev, best_dev, straight_len):
      as_seated_dev — length-weighted mean angle to nearest axis, as it sits.
      best_dev      — same after the BEST rotation about Z (the seat's up axis).
                      A genuinely mis-rotated boxy part drops to ~0 here; a round
                      part stays high (scattered facet edges can't be aligned).
    Z-sit fixes the up axis, so the only residual freedom is rotation about Z."""
    try:
        lines, straight = _line_edges(source)
    except Exception:  # noqa: BLE001 — OCP hiccup must not abort scoring
        return 0.0, 0.0, 0.0
    if straight <= 0 or not lines:
        return 0.0, 0.0, 0.0
    rot = np.asarray(matrix)[:3, :3]
    dirs = np.array([rot @ np.asarray(d, dtype=float) for d, _ in lines])
    lens = np.array([length for _, length in lines])
    as_seated = _wmean_dev(dirs, lens)
    best = as_seated
    for deg in range(1, 90):                          # search Z-rotations 1..89°
        th = math.radians(deg)
        c, s = math.cos(th), math.sin(th)
        x = dirs[:, 0] * c - dirs[:, 1] * s
        y = dirs[:, 0] * s + dirs[:, 1] * c
        best = min(best, _wmean_dev(np.stack([x, y, dirs[:, 2]], axis=1), lens))
    return as_seated, best, straight


def score_one(source: Path, m_ref, write: bool = False) -> dict | None:
    from app.document import EditorDocument
    from app.tools.zsit import compute_auto_zsit

    doc = EditorDocument.load(source)
    pts = np.concatenate([b.mesh.points for b in doc.bodies if b.mesh is not None])
    result = compute_auto_zsit(doc)
    if result is None:
        return None
    m_auto = result[0]
    up_err = float(np.degrees(np.arccos(np.clip(_up(m_ref) @ _up(m_auto), -1.0, 1.0))))
    seated_ref, seated_auto = _seated(m_ref, pts), _seated(m_auto, pts)
    diag = float(np.linalg.norm(pts.max(0) - pts.min(0))) or 1.0
    # Centering is rotation-invariant about Z (in-plane rotation is Step 2's
    # job): compare how far each seating puts the part centroid from the Z
    # axis, not the raw vector (which an in-plane spin would change).
    center_err = abs(float(np.linalg.norm(seated_auto[:, :2].mean(0)))
                     - float(np.linalg.norm(seated_ref[:, :2].mean(0))))
    ortho_dev, ortho_best, straight_len = _ortho_metric(source, m_auto)
    # Flag a genuine mis-rotation when there's enough straight structure to judge
    # AND either: an in-plane spin would align it (boxy part spun off-axis), or it's
    # grossly off (well beyond any correctly-seated part). Round parts have residual
    # that no spin fixes and never reach the gross bar, so they're left alone.
    rotated = straight_len >= ORTHO_MIN_STRAIGHT * diag and (
        (ortho_dev > ORTHO_DEV_TOL and ortho_best < ORTHO_RESIDUAL_TOL)
        or ortho_dev > ORTHO_GROSS_TOL)
    card = {
        "up_err": up_err,
        "auto_zmin": float(seated_auto[:, 2].min()),
        "ref_zmin": float(seated_ref[:, 2].min()),
        "center_err": center_err,
        "center_frac": center_err / diag,
        "ortho_dev": ortho_dev,
        "ortho_best": ortho_best,
        "rotated": rotated,
    }
    if write:
        _write_auto(doc, m_auto, source)
    return card


def generate_folder(folder: Path) -> int:
    """Held-out generation: run AUTO Z-Sit on every raw STEP in `folder` and
    write the seated result to Z_Sit_Output/ for inspection (no REF needed)."""
    from app.document import EditorDocument
    from app.export_ap242 import export_ap242
    from app.journal import Journal
    from app.tools.zsit import compute_auto_zsit

    OUT_DIR.mkdir(parents=True, exist_ok=True)
    files = [f for f in sorted(folder.iterdir())
             if f.suffix.lower() in (".step", ".stp")
             and "_AP242_conditioned" not in f.stem]
    for f in files:
        try:
            doc = EditorDocument.load(f)
            result = compute_auto_zsit(doc)
            if result is None:
                print(f"{f.name:34} no seat found")
                continue
            doc.apply_trsf(result[0])
            journal = Journal()
            journal.record("zsit", {"action": "auto"}, {},
                           {"matrix": np.asarray(result[0]).tolist()})
            export_ap242(doc, OUT_DIR / f"{f.stem}_ZSIT_AUTO.step", journal=journal)
            b = doc.bounds()
            print(f"{f.name:34} seated z[{b[4]:+.2f},{b[5]:+.2f}] -> {f.stem}_ZSIT_AUTO.step")
        except Exception as exc:  # noqa: BLE001
            print(f"{f.name:34} ERROR: {exc}")
    print(f"\ngenerated {len(files)} seatings -> {OUT_DIR}")
    return 0


def main() -> int:
    import argparse

    parser = argparse.ArgumentParser(description="AUTO Z-Sit vs hand REF scorer")
    parser.add_argument("--write", action="store_true",
                        help="also write auto-seated files into Z_Sit_Output/")
    parser.add_argument("--only", default=None, help="substring filter on base")
    parser.add_argument("--generate", type=Path, default=None, metavar="DIR",
                        help="seat every raw STEP in DIR (no REF) -> Z_Sit_Output")
    args = parser.parse_args()

    if args.generate:
        return generate_folder(args.generate)

    cases = _collect_refs()
    if args.only:
        cases = [c for c in cases if args.only.lower() in c[0].lower()]
    if not cases:
        print("No Z-Sit REFs found. Populate TEST_STEP_FILES with Z-Sit-only "
              "conditioned models (<base>_AP242_conditioned.step).")
        return 0
    passed = rotated_n = 0
    for base, source, m_ref in cases:
        try:
            card = score_one(source, m_ref, write=args.write)
        except Exception as exc:  # noqa: BLE001
            print(f"{base:30} ERROR: {exc}")
            continue
        if card is None:
            print(f"{base:30} auto-zsit found no seat")
            continue
        seat_ok = (card["up_err"] < UP_TOL_DEG
                   and abs(card["auto_zmin"] - card["ref_zmin"]) < ZMIN_TOL
                   and (card["center_frac"] < CENTER_FRAC_TOL
                        or card["center_err"] < CENTER_ABS_TOL))
        ok = seat_ok and not card["rotated"]
        passed += int(ok)
        rotated_n += int(card["rotated"])
        flag = "  ROT" if card["rotated"] else ""
        print(f"{base:30} up {card['up_err']:5.1f}°   z-min {card['auto_zmin']:+6.2f} "
              f"(ref {card['ref_zmin']:+.2f})   ctr {card['center_err']:5.2f}   "
              f"ortho {card['ortho_dev']:4.1f}°/best {card['ortho_best']:4.1f}°   "
              f"{'PASS' if ok else 'FAIL'}{flag}")
    print(f"\n{passed}/{len(cases)} seat PASS   ({rotated_n} flagged rotated)  ->  {OUT_DIR}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
