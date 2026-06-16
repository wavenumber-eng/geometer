"""Audit geometric AUTO pin detection against hand-marked _REF_PDET ground
truth.

A _REF_PDET file is the Pin DETection ground truth for a part: the hand-marked
pins, baked to AP242, living in REFERENCE_STEP_FILES as <base>_REF_PDET.step.
This script:

  1. PROMOTES any baked file that carries pins (_AP242_WNC* / _AP242_conditioned
     in TEST_STEP_FILES or REFERENCE_STEP_FILES) into a <base>_REF_PDET.step
     reference, so a hand-marked bake is saved as auditable ground truth.
  2. AUDITS the algorithm: runs auto_detect_pins(use_reference=False) -- the
     PURE GEOMETRIC detectors, reference replay disabled -- on each part's raw
     source and reports how many ground-truth pins it recovers. Face/body
     indices are preserved through the rigid conditioning transform, so the
     ground-truth keys compare verbatim to the freshly detected ones.

Run from the prototype dir:  uv run python score_pdet.py
"""
from __future__ import annotations

import shutil
import sys
from pathlib import Path

sys.path.insert(0, ".")
from app.auto import _base_name, auto_detect_pins        # noqa: E402
from app.document import EditorDocument                  # noqa: E402
from app.export_ap242 import extract_metadata            # noqa: E402
from app import refs                                     # noqa: E402

PROTO = Path(__file__).resolve().parent
REF_DIR = PROTO / "REFERENCE_STEP_FILES"


def _pin_keys(meta: dict | None) -> list:
    """One frozenset key per pin: its (body, face) tips and/or whole-body
    markers. Frame-invariant (indices survive the rigid conditioning)."""
    out = []
    for net in (meta or {}).get("nets", []):
        for pin in net.get("pins", []):
            key = {tuple(f) for f in pin.get("face_ids", [])}
            key |= {("B", b) for b in pin.get("body_ids", [])}
            if key:
                out.append(frozenset(key))
    return out


def promote() -> None:
    """Save every baked pin-bearing file as a <base>_REF_PDET.step reference."""
    made, seen = 0, set()
    for directory in (PROTO / "TEST_STEP_FILES", REF_DIR):
        if not directory.is_dir():
            continue
        for baked in sorted(directory.glob("*_AP242_*.step")):
            base = _base_name(baked.stem)
            if base in seen or not _pin_keys(extract_metadata(baked)):
                continue
            seen.add(base)
            dst = REF_DIR / f"{base}_REF_PDET.step"
            if not dst.exists():
                shutil.copy2(baked, dst)
                made += 1
                print(f"  + {dst.name}")
    print(f"promoted {made} _REF_PDET reference(s)")


def _source_for(base: str) -> Path | None:
    return refs._source_for(base)


def audit() -> None:
    rows = []
    for ref in sorted(REF_DIR.glob("*_REF_PDET.step")):
        base = _base_name(ref.stem)
        source = _source_for(base)
        if source is None:
            rows.append((base, None, None, None, "NO SOURCE"))
            continue
        ground = _pin_keys(extract_metadata(ref))
        detected, how = auto_detect_pins(
            EditorDocument.load(source), use_reference=False)
        det_keys = [
            frozenset({tuple(f) for f in p.face_ids}
                      | {("B", b) for b in p.body_ids})
            for p in detected
        ]
        found = sum(1 for g in ground if any(g & d for d in det_keys))
        rows.append((base, len(ground), len(detected), found, how))

    print(f"\n{'part':42} {'truth':>5} {'auto':>5} {'found':>5}  method")
    tot_g = tot_f = 0
    for base, ground, auto, found, how in rows:
        if ground is None:
            print(f"{base[:41]:42} {'-':>5} {'-':>5} {'-':>5}  {how}")
            continue
        flag = "" if found == ground == auto else "  <-- off"
        print(f"{base[:41]:42} {ground:5} {auto:5} {found:5}  {how}{flag}")
        tot_g, tot_f = tot_g + ground, tot_f + found
    if tot_g:
        print(f"\nGEOMETRIC-AUTO recall: {tot_f}/{tot_g} "
              f"({100 * tot_f // tot_g}%) of hand-marked pins recovered")


if __name__ == "__main__":
    promote()
    audit()
