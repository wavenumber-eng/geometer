"""Per-dimension AUTO-vs-REF scorer — the measurement half of the REF loop.

For every REFERENCE_STEP_FILES/<base>_AP242_conditioned_REF.step (the user's
hand-conditioned ground truth), find the source model, run the headless AUTO
conditioner on it, and score how far AUTO is from the REF on each axis:

  seat    is the model seated at z=0, Z-up, with the REF's height (right axis up)?
  count   pin count, and SMT/THR vs CON/HEAD role split
  match   REF pins matched to AUTO pins by centroid (catches pin-1 rotation and
          placement errors); mean centroid distance over the matches
  split   pins that became their own body (the unibody split)

This turns "every model is wrong" into a scorecard per tool, so each new REF
shows exactly which rung is drifting. Run:
    uv run python score_auto.py [--only <substring>]
"""

from __future__ import annotations

import argparse
import sys
from pathlib import Path

import numpy as np

PROTO = Path(__file__).resolve().parent
sys.path.insert(0, str(PROTO))

def _pins_from_meta(meta: dict) -> list:
    return [
        {"centroid": p.get("centroid"), "role": p.get("role", "primary"),
         "body_ids": p.get("body_ids") or [], "hitbox": p.get("hitbox")}
        for net in meta.get("nets", [])
        for p in net.get("pins", [])
        if p.get("centroid")
    ]


def ref_truth(ref_step: Path) -> dict:
    from app.document import EditorDocument
    from app.export_ap242 import extract_metadata

    meta = extract_metadata(ref_step) or {}
    doc = EditorDocument.load(ref_step)
    return {"pins": _pins_from_meta(meta),
            "bodies": len(meta.get("bodies", [])), "bounds": doc.bounds()}


def auto_result(source: Path) -> dict:
    from app.auto import condition_auto
    from app.document import EditorDocument
    from app.journal import Journal

    doc = EditorDocument.load(source)
    registry = condition_auto(doc, Journal())
    pins = [{"centroid": list(p.centroid), "role": p.role,
             "body_ids": list(p.body_ids), "hitbox": p.hitbox}
            for p in registry.pins]
    return {"pins": pins, "bodies": len(doc.bodies), "bounds": doc.bounds()}


def _diag(bounds) -> float:
    return float(np.linalg.norm([bounds[1] - bounds[0],
                                 bounds[3] - bounds[2], bounds[5] - bounds[4]]))


def _match_pins(ref_pins: list, auto_pins: list, tol: float) -> list:
    """Greedy nearest ref->auto match in the seated frame (one-to-one, within
    tol). Returns [(ref_pin, auto_pin, distance)]."""
    auto_pts = [np.array(p["centroid"], dtype=float) for p in auto_pins]
    used: set = set()
    matches = []
    for ref_pin in ref_pins:
        centroid = np.array(ref_pin["centroid"], dtype=float)
        best, best_d = -1, float("inf")
        for index, auto_pt in enumerate(auto_pts):
            if index in used:
                continue
            dist = float(np.linalg.norm(centroid - auto_pt))
            if dist < best_d:
                best, best_d = index, dist
        if best >= 0 and best_d <= tol:
            used.add(best)
            matches.append((ref_pin, auto_pins[best], best_d))
    return matches


def _roles(pins: list) -> tuple:
    primary = sum(1 for p in pins if p["role"] != "mouth")
    return primary, len(pins) - primary


def score(ref: dict, auto: dict) -> dict:
    tol = 0.05 * _diag(ref["bounds"])
    matches = _match_pins(ref["pins"], auto["pins"], tol)
    dists = [d for _r, _a, d in matches]
    return {
        "seat_zmin": abs(auto["bounds"][4]),
        "height_err": abs((auto["bounds"][5] - auto["bounds"][4])
                          - (ref["bounds"][5] - ref["bounds"][4])),
        "ref_pins": len(ref["pins"]), "auto_pins": len(auto["pins"]),
        "ref_roles": _roles(ref["pins"]), "auto_roles": _roles(auto["pins"]),
        "matched": len(matches),
        "match_frac": len(matches) / max(len(ref["pins"]), 1),
        "mean_dist": float(np.mean(dists)) if dists else None,
        "role_ok": sum(1 for r, a, _d in matches if r["role"] == a["role"]),
        "ref_split": sum(1 for p in ref["pins"] if p["body_ids"]),
        "auto_split": sum(1 for p in auto["pins"] if p["body_ids"]),
    }


def print_card(base: str, sc: dict) -> None:
    rr, ar = sc["ref_roles"], sc["auto_roles"]
    md = f"{sc['mean_dist']:.3f}mm" if sc["mean_dist"] is not None else "-"
    print(f"\n== {base} ==")
    print(f"  seat   z-min {sc['seat_zmin']:.2f} (want ~0)   height err {sc['height_err']:.2f}")
    print(f"  count  ref {sc['ref_pins']} ({rr[0]} SMT + {rr[1]} CON)   "
          f"auto {sc['auto_pins']} ({ar[0]} SMT + {ar[1]} CON)")
    print(f"  match  {sc['matched']}/{sc['ref_pins']} ({sc['match_frac'] * 100:.0f}%)   "
          f"mean dist {md}   role ok {sc['role_ok']}/{sc['matched']}")
    print(f"  split  ref {sc['ref_split']}   auto {sc['auto_split']}")


def main() -> int:
    parser = argparse.ArgumentParser(description="AUTO vs REF per-dimension scorer")
    parser.add_argument("--only", default=None, help="substring filter on base name")
    args = parser.parse_args()

    from app import refs as refmod

    # Full-session refs only: score_auto compares the whole auto chain
    # (pins/nets/separate) against the hand truth, so it needs refs that were
    # conditioned beyond Z-sit.
    cases = [r for r in refmod.all_references() if r.has_op("detect_pins")]
    scored = 0
    for r in cases:
        if args.only and args.only.lower() not in r.base.lower():
            continue
        try:
            print_card(r.base, score(ref_truth(r.conditioned), auto_result(r.source)))
            scored += 1
        except Exception as exc:  # noqa: BLE001
            print(f"\n== {r.base} ==\n  ERROR: {exc}")
    print(f"\nscored {scored} full-session REF case(s) in REFERENCE_STEP_FILES")
    return 0 if scored or not cases else 1


if __name__ == "__main__":
    raise SystemExit(main())
