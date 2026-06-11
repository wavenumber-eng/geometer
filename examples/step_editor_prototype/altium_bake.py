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
from altium_monkey.altium_pcb_model_checksum import compute_altium_model_checksum
from altium_monkey.altium_record_pcb__model import AltiumPcbModel

MM_TO_MILS = 1.0 / 0.0254


def _model_record_spans(models_data: bytes) -> list[tuple[AltiumPcbModel, int, int]]:
    """Walk Library/Models/Data, returning (record, offset, length) per model."""
    spans = []
    offset = 0
    while offset + 4 <= len(models_data):
        record = AltiumPcbModel()
        consumed = record.parse_from_binary(models_data, offset)
        if consumed <= 0:
            break
        spans.append((record, offset, consumed))
        offset += consumed
    return spans


def replace_model_payload(
    lib_path: Path, out_path: Path, model_name: str, new_payload: bytes
) -> None:
    """Swap one embedded model's payload, preserving everything else.

    The conditioning flow's Altium bake: the target model keeps its GUID,
    name, rotations and z-offset; only the payload stream, the checksum in
    its Models/Data record (spliced — other records' bytes untouched), and
    the model_checksum on component bodies referencing it are rewritten.
    """
    lib = AltiumPcbLib.from_file(lib_path)
    if lib.raw_models_data is None:
        raise ValueError(f"{lib_path}: no embedded models")
    spans = _model_record_spans(lib.raw_models_data)
    matches = [s for s in spans if s[0].name == model_name]
    if not matches:
        raise ValueError(f"{lib_path}: no embedded model named {model_name!r}")
    record, offset, length = matches[0]
    index = next(i for i, s in enumerate(spans) if s[1] == offset)

    record.checksum = compute_altium_model_checksum(new_payload)
    new_record = record.serialize_to_binary()
    lib.raw_models_data = (
        lib.raw_models_data[:offset]
        + new_record
        + lib.raw_models_data[offset + length :]
    )
    lib.raw_models[index] = zlib.compress(new_payload)

    target_id = str(record.id).upper()
    touched_bodies = 0
    for footprint in lib.footprints:
        for body in footprint.component_bodies:
            if str(getattr(body, "model_id", "") or "").upper() == target_id:
                body.model_checksum = record.checksum & 0xFFFFFFFF
                touched_bodies += 1

    lib.save(out_path)
    print(
        f"replaced {model_name!r} (index {index}, GUID {record.id}) with "
        f"{len(new_payload)} bytes; checksum {record.checksum}; "
        f"{touched_bodies} component body(ies) updated"
    )


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


def replace_main(lib_path: str, model_name: str, step_path: str, out_path: str) -> int:
    """CLI: replace + verify (payload identity, others untouched, determinism)."""
    src, out = Path(lib_path), Path(out_path)
    new_payload = Path(step_path).read_bytes()

    before = {
        m.name: (str(m.id), zlib.decompress(c) if c[:1] == b"\x78" else bytes(c))
        for m, c in AltiumPcbLib.from_file(src).get_embedded_model_entries()
    }
    replace_model_payload(src, out, model_name, new_payload)

    after_lib = AltiumPcbLib.from_file(out)
    ok = True
    for m, c in after_lib.get_embedded_model_entries():
        raw = zlib.decompress(c) if c[:1] == b"\x78" else bytes(c)
        if m.name == model_name:
            if raw != new_payload:
                print("FAIL: swapped payload differs after reread")
                ok = False
            # Models/Data stores the checksum signed; compare the 32-bit pattern.
            if m.checksum & 0xFFFFFFFF != compute_altium_model_checksum(new_payload):
                print("FAIL: checksum not updated")
                ok = False
        else:
            old_id, old_raw = before[m.name]
            if raw != old_raw or str(m.id) != old_id:
                print(f"FAIL: untouched model {m.name!r} changed")
                ok = False

    twin = out.with_suffix(".twin.PcbLib")
    replace_model_payload(src, twin, model_name, new_payload)
    deterministic = twin.read_bytes() == out.read_bytes()
    twin.unlink()
    if not deterministic:
        print("FAIL: replace is not deterministic")
        ok = False
    print(
        f"verify: payload identity OK, {len(before) - 1} other models untouched, "
        f"deterministic {'OK' if deterministic else 'FAIL'}"
        if ok
        else "verify: FAILURES above"
    )
    return 0 if ok else 1


if __name__ == "__main__":
    if sys.argv[1] == "--replace":
        raise SystemExit(replace_main(*sys.argv[2:6]))
    raise SystemExit(main(sys.argv[1], sys.argv[2]))
