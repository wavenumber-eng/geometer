"""Does snapping the ML seat level to the nearest planar VERTEX formation make
the standoff exact? For every Z-Sit hand REF, this runs the current AUTO seat
(ML up + ML level + footprint centre), then snaps the level onto the nearest
vertex plane along up, and compares both seated z-mins to the REF's.

The premise (user, 2026-06-13): the ML only has to get orientation + centre +
a ROUGHLY correct level; a deterministic vertex-plane snap lands the exact
standoff, and a downstream tool handles Z rotation. This measures the snap half.

    uv run python snap_eval.py
"""

from __future__ import annotations

import sys
from pathlib import Path

import numpy as np

PROTO = Path(__file__).resolve().parent
sys.path.insert(0, str(PROTO))

from score_zsit import _collect_refs, _seated, _up  # noqa: E402


def main() -> int:
    from app.document import EditorDocument
    from app.seat_model import snap_level_to_vertex_plane
    from app.tools.zsit import _learned_seat_level, _learned_seat_up, compute_auto_zsit

    cases = _collect_refs()
    print(f"{'case':30} {'ML z-min':>9} {'snap z-min':>11} {'ref z-min':>10} "
          f"{'ML err':>7} {'snap err':>9}  verdict")
    improved = unchanged = worsened = 0
    ml_total = snap_total = 0.0
    for base, source, m_ref in cases:
        doc = EditorDocument.load(source)
        pts = np.concatenate([b.mesh.points for b in doc.bodies if b.mesh is not None])
        result = compute_auto_zsit(doc)
        if result is None:
            continue
        m_auto = result[0]
        up = _up(m_auto)
        ref_zmin = float(_seated(m_ref, pts)[:, 2].min())
        ml_zmin = float(_seated(m_auto, pts)[:, 2].min())

        # Re-seat with the level snapped onto the nearest vertex plane. The ML
        # level is along up; snapping shifts the whole transform along up.
        ml_level = _learned_seat_level(doc, up)
        if ml_level is None:
            ml_level = float((pts @ up).min())  # no level model -> part bottom
        snapped_level = snap_level_to_vertex_plane(doc, up, ml_level)
        shift = snapped_level - ml_level
        snap_zmin = ml_zmin - shift  # raising the seat level lowers seated z-min

        ml_err = abs(ml_zmin - ref_zmin)
        snap_err = abs(snap_zmin - ref_zmin)
        ml_total += ml_err
        snap_total += snap_err
        if snap_err < ml_err - 1e-4:
            verdict, improved = "IMPROVED", improved + 1
        elif snap_err > ml_err + 1e-4:
            verdict, worsened = "WORSE", worsened + 1
        else:
            verdict, unchanged = "same", unchanged + 1
        # only print rows where the snap moved the level or there was error
        if abs(shift) > 1e-4 or ml_err > 0.02:
            print(f"{base:30} {ml_zmin:+9.3f} {snap_zmin:+11.3f} {ref_zmin:+10.3f} "
                  f"{ml_err:7.3f} {snap_err:9.3f}  {verdict}")
    print(f"\nsnap moved/relevant rows above. improved {improved}, "
          f"worse {worsened}, same {unchanged}")
    print(f"total |z-min err|: ML {ml_total:.3f}mm  ->  snap {snap_total:.3f}mm")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
