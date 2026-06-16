"""Headless pin-detection sweep over the raw (naked, un-seated) test models.
Reports per model: method, #primary, #mouth, seconds, or an ERROR. Writes a
sorted summary so we can see where AUTO detection is solid and where it is off."""
import sys, time, traceback
from pathlib import Path

sys.path.insert(0, ".")
from app.document import EditorDocument          # noqa: E402
from app.auto import auto_detect_pins            # noqa: E402

models = sorted(Path("TEST_STEP_FILES").glob("*.st*")) + \
         sorted(Path("TEST_STEP_FILES").glob("*.ST*"))
models = sorted({p.resolve() for p in models})

rows = []
for path in models:
    t0 = time.time()
    try:
        doc = EditorDocument.load(path)
        pins, how = auto_detect_pins(doc)
        prim = sum(1 for p in pins if p.role != "mouth")
        mouth = sum(1 for p in pins if p.role == "mouth")
        nbody = len(doc.bodies)
        rows.append((path.name, how, prim, mouth, nbody, time.time() - t0, ""))
    except Exception as exc:  # noqa: BLE001
        rows.append((path.name, "ERROR", 0, 0, 0, time.time() - t0,
                     f"{type(exc).__name__}: {exc}"))
        traceback.print_exc()

out = Path("_pin_sweep_results.txt")
with out.open("w", encoding="utf-8") as fh:
    fh.write(f"{'model':45} {'method':14} {'prim':>4} {'mouth':>5} "
             f"{'bodies':>6} {'sec':>6}\n")
    for name, how, prim, mouth, nbody, sec, err in rows:
        fh.write(f"{name[:44]:45} {how:14} {prim:4d} {mouth:5d} "
                 f"{nbody:6d} {sec:6.1f}  {err}\n")
    # summaries
    total = len(rows)
    zero = [r for r in rows if r[2] == 0 and r[1] != "ERROR"]
    errs = [r for r in rows if r[1] == "ERROR"]
    huge = [r for r in rows if r[2] > 200]
    fh.write(f"\n{total} models | {len(errs)} errors | "
             f"{len(zero)} zero-primary | {len(huge)} >200-primary\n")
    fh.write("ZERO-PRIMARY: " + ", ".join(r[0] for r in zero) + "\n")
    fh.write("ERRORS: " + ", ".join(r[0] for r in errs) + "\n")
print(f"swept {len(rows)} models -> {out}")
