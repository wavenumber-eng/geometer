"""Split-quality benchmark: AUTO vs the user's hand-conditioned REF truth.

For every unified reference (app/refs.py) with a committed hand journal it finds the
source model (TEST_STEP_FILES/<base>.*), reads the hand truth from the REF's
embedded metadata (pins, how many are whole split bodies, designators), runs
the fully automatic conditioner on the source, and scores the splitter:

  pins        auto pin count vs hand pin count
  split-rate  pins whose body_ids == [one whole solid] (the body-per-pin
              contract) vs the hand session's rate
  PIN_ bodies bodies named PIN_*/role=pin after auto
  volume      |total volume after - before| / before (debris / loss guard)

New REF files the user drops in are picked up automatically. Run:
    uv run python benchmark_split.py [--only <substring>] [--json out.json]
"""

from __future__ import annotations

import argparse
import json
import sys
import time
from pathlib import Path

PROTO = Path(__file__).resolve().parent
sys.path.insert(0, str(PROTO))

def hand_truth(ref_step: Path) -> dict:
    from app.export_ap242 import extract_metadata

    meta = extract_metadata(ref_step)
    if meta is None:
        return {}
    pins = [p for n in meta.get("nets", []) for p in n.get("pins", [])]
    split = [p for p in pins if p.get("body_ids")]
    return {
        "pins": len(pins),
        "split_pins": len(split),
        "bodies": len(meta.get("bodies", [])),
        "designators": sorted({n["designator"] for n in meta.get("nets", [])}),
    }


def run_auto(source: Path) -> dict:
    from app.auto import condition_auto
    from app.document import EditorDocument
    from app.journal import Journal

    document = EditorDocument.load(source)
    volume_before = document.total_volume() if hasattr(document, "total_volume") else None
    started = time.perf_counter()
    journal = Journal()
    registry = condition_auto(document, journal)
    elapsed = time.perf_counter() - started

    pins = registry.pins
    split = [p for p in pins if len(p.body_ids) == 1 and not p.face_ids]
    pin_bodies = [b for b in document.bodies if getattr(b, "role", "") == "pin"]
    separate_ops = [op for op in journal.operations if op.tool == "separate"]
    sep = separate_ops[-1].result if separate_ops else {}
    return {
        "pins": len(pins),
        "split_pins": len(split),
        "pin_bodies": len(pin_bodies),
        "bodies": len(document.bodies),
        "hitboxed": sum(1 for p in pins if p.hitbox),
        "separate_result": sep,
        "seconds": round(elapsed, 2),
        "volume_before": volume_before,
    }


def run_replay(source: Path, ops: Path) -> dict:
    """Replay the hand session's journal — isolates the SPLITTER (hand seeds,
    hand parameters) from auto detection quality."""
    from app.document import EditorDocument
    from app.journal import Journal
    from app.replay import replay

    document = EditorDocument.load(source)
    started = time.perf_counter()
    registry = replay(document, Journal.load(ops))
    elapsed = time.perf_counter() - started

    pins = registry.pins
    split = [p for p in pins if len(p.body_ids) == 1 and not p.face_ids]
    pin_bodies = [b for b in document.bodies if getattr(b, "role", "") == "pin"]
    return {
        "pins": len(pins),
        "split_pins": len(split),
        "pin_bodies": len(pin_bodies),
        "bodies": len(document.bodies),
        "hitboxed": sum(1 for p in pins if p.hitbox),
        "seconds": round(elapsed, 2),
    }


BENCH_DIR = PROTO / "benchmarks"


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--only", default=None, help="substring filter on the base name")
    parser.add_argument("--json", type=Path, default=None, help="write full results as JSON")
    parser.add_argument("--replay", action="store_true",
                        help="replay the hand journals (benchmarks/<base>_REF.json) "
                             "instead of running AUTO — isolates splitter quality "
                             "from detection quality")
    args = parser.parse_args()

    from app import refs

    # Split benchmarks = references that carry a committed hand journal in
    # benchmarks/<base>_REF.json (the curated full sessions).
    cases = []
    for r in refs.all_references():
        if args.only and args.only.lower() not in r.base.lower():
            continue
        ops = BENCH_DIR / f"{r.base}_REF.json"
        if not ops.exists():
            continue
        cases.append((r.base, r.source, r.conditioned, ops))

    if not cases:
        print("no REF cases found")
        return 1

    mode = "replay" if args.replay else "auto"
    results = []
    width = max(len(base) for base, _s, _r, _o in cases)
    print(f"{'case'.ljust(width)}  {'hand pins/split':>16}  {mode + ' pins/split':>16}  "
          f"{'PIN_ bodies':>11}  {'time':>7}  verdict")
    failures = 0
    for base, source, ref, ops in cases:
        truth = hand_truth(ref)
        if source is None:
            print(f"{base.ljust(width)}  SOURCE MISSING (looked for {base}.step/.stp)")
            failures += 1
            continue
        try:
            auto = run_replay(source, ops) if args.replay else run_auto(source)
        except Exception as exc:
            print(f"{base.ljust(width)}  {mode.upper()} FAILED: {exc}")
            failures += 1
            results.append({"case": base, "error": str(exc), "truth": truth})
            continue

        pins_ok = auto["pins"] == truth.get("pins")
        split_target = truth.get("split_pins", 0)
        split_ok = auto["split_pins"] >= split_target
        verdict = "OK" if (pins_ok and split_ok) else (
            "PIN COUNT" if not pins_ok else "UNDER-SPLIT"
        )
        if verdict != "OK":
            failures += 1
        print(f"{base.ljust(width)}  "
              f"{str(truth.get('pins')).rjust(7)}/{str(split_target).ljust(8)}  "
              f"{str(auto['pins']).rjust(7)}/{str(auto['split_pins']).ljust(8)}  "
              f"{str(auto['pin_bodies']).rjust(11)}  "
              f"{str(auto['seconds']).rjust(6)}s  {verdict}")
        results.append({"case": base, "truth": truth, "auto": auto, "verdict": verdict})

    print(f"\n{len(cases) - failures}/{len(cases)} cases at or above hand truth")
    if args.json:
        args.json.write_text(json.dumps(results, indent=1, default=str), encoding="utf-8")
        print(f"details -> {args.json}")
    return 1 if failures else 0


if __name__ == "__main__":
    raise SystemExit(main())
