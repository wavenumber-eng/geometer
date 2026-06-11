"""Bake conditioned AP242 models into Altium .PcbLib files (Phase 3).

Runs in altium_monkey's venv, NOT the prototype venv (cadquery/ocp pins
differ there):

    cd C:/Users/natha/GitHub-Projects/toolz/altium_monkey
    uv run python <this file> <ref_dir> <out_dir>

For each <ref_dir>/*.step a one-footprint .PcbLib is authored with the STEP
embedded (zlib + native Altium checksum, handled by altium_monkey). Explicit
bounds come from <ref_dir>/bounds_mm.json (precomputed with geometer) so the
slow OCCT bounds-inference path is skipped. Each library is then reparsed and
its payload extracted and byte-compared against the source STEP — the same
identity discipline as the KiCad side.
"""

from __future__ import annotations

import json
import sys
import zlib
from pathlib import Path

from altium_monkey import AltiumPcbLib

MM_TO_MILS = 1.0 / 0.0254


def main(ref_dir: str, out_dir: str) -> int:
    ref = Path(ref_dir)
    out = Path(out_dir)
    out.mkdir(parents=True, exist_ok=True)
    bounds_all = json.loads((ref / "bounds_mm.json").read_text())

    ok = True
    for step in sorted(ref.glob("*.step")):
        stem = step.stem
        raw = step.read_bytes()
        b = bounds_all[stem]
        lo = [v * MM_TO_MILS for v in b["min"]]
        hi = [v * MM_TO_MILS for v in b["max"]]

        lib = AltiumPcbLib()
        fp = lib.add_footprint(stem, description="wn3d conditioned AP242 model")
        model = lib.add_embedded_model(name=stem + ".step", model_data=raw)
        fp.add_embedded_3d_model(
            model,
            bounds_mils=(lo[0], lo[1], hi[0], hi[1]),
            overall_height_mils=hi[2],
            standoff_height_mils=lo[2],
            name=stem,
        )
        lib_path = out / (stem + ".PcbLib")
        lib.save(lib_path)

        reread = AltiumPcbLib.from_file(lib_path)
        entries = reread.get_embedded_model_entries()
        payloads = [zlib.decompress(compressed) for _, compressed in entries]
        identical = len(payloads) == 1 and payloads[0] == raw
        names_ok = entries[0][0].name == stem + ".step" if entries else False
        ok &= identical and names_ok
        print(
            f"{stem}.PcbLib: {len(raw)} -> {lib_path.stat().st_size} bytes on disk, "
            f"re-extract {'IDENTICAL' if identical else 'DIFFERS'}, "
            f"model name {'OK' if names_ok else 'WRONG'}"
        )
    return 0 if ok else 1


if __name__ == "__main__":
    raise SystemExit(main(sys.argv[1], sys.argv[2]))
