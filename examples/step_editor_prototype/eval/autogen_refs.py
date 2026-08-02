"""Auto-generate Z-Sit reference seatings for the deterministic parts
(resistors / capacitors / inductors) so they don't have to be hand-seated.

AUTO Z-Sit nails passives, so this seats each matching un-referenced source and
writes <base>_AP242_conditioned.step straight into REFERENCE_STEP_FILES — where
it is immediately a training reference. You then RATE them in the 3D browser and
delete any the auto got wrong (rare for passives); re-run train_seat_model.py.

    uv run python autogen_refs.py                 # default passive filter, dry run
    uv run python autogen_refs.py --write         # actually generate
    uv run python autogen_refs.py --filter '^L'   # custom (e.g. inductors)
    uv run python autogen_refs.py --all --write    # every un-referenced source

Each generated ref's journal carries zsit action="auto" + auto_generated=True,
so auto-seats stay distinguishable from hand seatings.
"""

from __future__ import annotations

import argparse
import re
import sys
from pathlib import Path

import numpy as np

PROTO = Path(__file__).resolve().parents[1]  # prototype root (script in eval/)
sys.path.insert(0, str(PROTO))

REF_DIR = PROTO / "REFERENCE_STEP_FILES"
SCAN_DIRS = [
    PROTO / "TEST_STEP_FILES",
    PROTO / "TEST_STEP_FILES" / "ADDITIONAL_TEST_STEP",
    PROTO / "TEST_STEP_FILES" / "ADDITIONAL_TEST_STEP_2",
]
# Resistor / capacitor / inductor chip naming (IEC sizes + IPC RESC/CAPC + tant).
PASSIVE_RE = r"(^[CRL]\d{3,4}|^RESC|^CAPC|C_TANT|_CAP[_C]|_RES[_C]|^IHLP|^SRP)"


def _ref_bases() -> set:
    from app import refs

    return {r.base.lower() for r in refs.all_references()}


def _candidates(pattern: str | None) -> list[Path]:
    have = _ref_bases()
    rx = re.compile(pattern, re.IGNORECASE) if pattern else None
    seen, out = set(), []
    for d in SCAN_DIRS:
        if not d.is_dir():
            continue
        for f in sorted(d.iterdir()):
            if (f.is_file() and f.suffix.lower() in (".step", ".stp")
                    and "_AP242_conditioned" not in f.stem
                    and not f.stem.lower().endswith("_zsit_auto")
                    and f.stem.lower() not in have
                    and f.stem.lower() not in seen
                    and (rx is None or rx.search(f.stem))):
                seen.add(f.stem.lower())
                out.append(f)
    return out


def _seat_and_write(source: Path) -> str:
    from app.document import EditorDocument
    from app.export_ap242 import export_ap242
    from app.journal import Journal
    from app.tools.zsit import compute_auto_zsit

    doc = EditorDocument.load(source)
    result = compute_auto_zsit(doc)
    if result is None:
        return "no-seat"
    matrix = result[0]
    doc.apply_trsf(matrix)
    journal = Journal()
    journal.record("zsit", {"action": "auto", "auto_generated": True}, {},
                   {"matrix": np.asarray(matrix).tolist()})
    out = REF_DIR / f"{source.stem}_AP242_conditioned.step"
    export_ap242(doc, out, journal=journal)
    b = doc.bounds()
    return f"z[{b[4]:+.2f},{b[5]:+.2f}]"


def main() -> int:
    ap = argparse.ArgumentParser(description=__doc__)
    ap.add_argument("--write", action="store_true", help="actually generate (else dry run)")
    ap.add_argument("--filter", default=PASSIVE_RE, help="regex on the base name")
    ap.add_argument("--all", action="store_true", help="ignore the filter; every un-ref'd source")
    args = ap.parse_args()

    cands = _candidates(None if args.all else args.filter)
    print(f"{len(cands)} un-referenced source(s) match "
          f"({'ALL' if args.all else args.filter})")
    if not args.write:
        for f in cands:
            print(f"  would seat  {f.stem}")
        print("\nDRY RUN — pass --write to generate into REFERENCE_STEP_FILES")
        return 0

    ok = failed = 0
    for f in cands:
        try:
            info = _seat_and_write(f)
            if info == "no-seat":
                print(f"  NO SEAT  {f.stem}")
                failed += 1
            else:
                print(f"  seated   {f.stem}  {info}")
                ok += 1
        except Exception as exc:  # noqa: BLE001
            print(f"  ERROR    {f.stem}: {exc}")
            failed += 1
    print(f"\ngenerated {ok} auto-seat ref(s) into REFERENCE_STEP_FILES "
          f"({failed} failed). RATE them in the browser, delete any wrong ones, "
          f"then re-run train_seat_model.py.")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
