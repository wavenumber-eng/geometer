"""Triage which raw models the GEOMETRIC pin detector does NOT handle well, so
they can be hand-baked into _REF_PDET references. For each raw model: run
auto_detect_pins(use_reference=False), compare to the filename lead-count hint
(package_hint) when available, and flag zeros / count mismatches / gross
over-detection. Models that already have a _REF_PDET are marked [REF]."""
import sys
from pathlib import Path

sys.path.insert(0, ".")
from app.auto import auto_detect_pins, _base_name      # noqa: E402
from app.document import EditorDocument                # noqa: E402
from app.package_hint import package_hint              # noqa: E402

PROTO = Path(".")
have_ref = {_base_name(p.stem)
            for p in (PROTO / "REFERENCE_STEP_FILES").glob("*_REF_PDET.step")}

models = sorted({p.resolve() for p in (PROTO / "TEST_STEP_FILES").glob("*.st*")}
                | {p.resolve() for p in (PROTO / "TEST_STEP_FILES").glob("*.ST*")})

rows = []
for path in models:
    base = _base_name(path.stem)
    try:
        pins, how = auto_detect_pins(EditorDocument.load(path), use_reference=False)
    except Exception as exc:  # noqa: BLE001
        rows.append((path.name, "ERR", 0, 0, None, f"ERROR {type(exc).__name__}"))
        continue
    prim = sum(1 for p in pins if p.role != "mouth")
    total = len(pins)
    hint = package_hint(path.name)
    leads = hint.get("leads") if hint else None
    flag = ""
    if total == 0:
        flag = "ZERO"
    elif leads and prim != leads and total != leads:
        flag = f"name~{leads} != {prim}"
    elif leads and total > 1.6 * leads:
        flag = f"over-detect (name~{leads})"
    rows.append((path.name, how, prim, total - prim, leads, flag))

flagged = [r for r in rows if r[5] and _base_name(Path(r[0]).stem) not in have_ref]
out = PROTO / "_pdet_triage.txt"
with out.open("w", encoding="utf-8") as fh:
    fh.write("NOT-FULLY-GOOD models without a _REF_PDET yet "
             "(candidates to hand-bake):\n")
    fh.write(f"{'model':42} {'method':13} {'prim':>4} {'mouth':>5} "
             f"{'name':>5}  flag\n")
    for name, how, prim, mouth, leads, flag in sorted(flagged, key=lambda r: r[5]):
        fh.write(f"{name[:41]:42} {how:13} {prim:4d} {mouth:5d} "
                 f"{str(leads or '-'):>5}  {flag}\n")
    fh.write(f"\n{len(flagged)} flagged of {len(rows)} models "
             f"({len(have_ref)} already have a _REF_PDET).\n")
print(f"triaged {len(rows)} models -> {out} ({len(flagged)} flagged)")
