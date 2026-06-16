"""Train the Z-Sit seat-orientation ranker on the user's REF seatings and save
it to app/seat_model.joblib. Reports leave-one-out up-vector accuracy. Re-run
whenever new REFs are added. Usage: uv run python train_seat_model.py
"""

from __future__ import annotations

import sys
from pathlib import Path

import numpy as np

PROTO = Path(__file__).resolve().parent
sys.path.insert(0, str(PROTO))

from app.seat_model import (  # noqa: E402
    LEVEL_MODEL_PATH, MODEL_PATH, STATS_PATH, _LEVEL_FRAC, _MATCH_DEG,
    candidate_seats, candidate_levels,
)

def build_dataset():
    """Per REF: (base, candidate ups, features, labels, ground-truth up)."""
    from app.document import EditorDocument
    from app.refs import seatings

    data = []
    for ref in seatings():
        matrix = ref.zsit_matrix()
        try:
            doc = EditorDocument.load(ref.source)
        except Exception:  # noqa: BLE001
            continue
        ups, feats = candidate_seats(doc)
        if ups is None:
            continue
        gt = matrix[2, :3] / np.linalg.norm(matrix[2, :3])
        labels = np.array([
            np.degrees(np.arccos(np.clip(up @ gt, -1, 1))) < _MATCH_DEG for up in ups
        ])
        data.append((ref.base, np.array(ups), feats, labels, gt, _is_auto(ref)))
    return data


def _is_auto(ref) -> bool:
    """Was this seating auto-generated (vs hand-seated)? Used to report the
    honest fidelity on the HARD hand-seated parts separately from the
    deterministic auto-seated passives that flatter the headline number."""
    return any(op.get("tool") == "zsit"
               and op.get("params", {}).get("auto_generated")
               for op in ref.journal)


def _fit(features, labels):
    from sklearn.ensemble import RandomForestClassifier

    return RandomForestClassifier(
        n_estimators=300, random_state=0, class_weight="balanced"
    ).fit(features, labels)


def leave_one_out(data):
    """(hits, hand_hits, hand_total) — overall and hand-seated-only LOO."""
    hits = hand_hits = hand_total = 0
    for i, (base, ups, feats, _labels, gt, is_auto) in enumerate(data):
        train_x = np.vstack([d[2] for j, d in enumerate(data) if j != i])
        train_y = np.concatenate([d[3] for j, d in enumerate(data) if j != i])
        clf = _fit(train_x, train_y)
        pick = ups[int(np.argmax(clf.predict_proba(feats)[:, 1]))]
        ok = np.degrees(np.arccos(np.clip(pick @ gt, -1, 1))) < _MATCH_DEG
        hits += int(ok)
        if not is_auto:
            hand_total += 1
            hand_hits += int(ok)
        if not ok:
            print(f"  LOO miss: {base}{' (auto)' if is_auto else ''}")
    return hits, hand_hits, hand_total


def build_level_dataset():
    """Per REF: (base, candidate levels, features, labels) for the SEAT LEVEL.
    The REF's z=0 plane sits at source-frame level -M_ref[2,3] along its up."""
    from app.document import EditorDocument
    from app.refs import seatings

    data = []
    for ref in seatings():
        matrix = ref.zsit_matrix()
        try:
            doc = EditorDocument.load(ref.source)
        except Exception:  # noqa: BLE001
            continue
        up = matrix[2, :3] / np.linalg.norm(matrix[2, :3])
        seat_level = -float(matrix[2, 3])
        levels, feats = candidate_levels(doc, up)
        if not levels:
            continue
        pts = np.concatenate([b.mesh.points for b in doc.bodies if b.mesh is not None])
        span = float(np.ptp(pts @ up)) or 1.0
        labels = np.array([abs(L - seat_level) <= _LEVEL_FRAC * span for L in levels])
        data.append((ref.base, np.array(levels), feats, labels, _is_auto(ref)))
    return data


def leave_one_out_levels(data):
    """(hits, hand_hits, hand_total) — overall and hand-seated-only LOO."""
    hits = hand_hits = hand_total = 0
    for i, (base, _levels, feats, labels, is_auto) in enumerate(data):
        train_x = np.vstack([d[2] for j, d in enumerate(data) if j != i])
        train_y = np.concatenate([d[3] for j, d in enumerate(data) if j != i])
        clf = _fit(train_x, train_y)
        pick = int(np.argmax(clf.predict_proba(feats)[:, 1]))
        ok = bool(labels[pick])
        hits += int(ok)
        if not is_auto:
            hand_total += 1
            hand_hits += int(ok)
        if not ok:
            print(f"  level LOO miss: {base}{' (auto)' if is_auto else ''}")
    return hits, hand_hits, hand_total


def main() -> int:
    import joblib

    data = build_dataset()
    if not data:
        print("No REF seatings found in REFERENCE_STEP_FILES.")
        return 1
    reachable = sum(int(d[3].any()) for d in data)
    print(f"{len(data)} REF seatings ({reachable} with the seat reachable in candidates)")
    hits, hh, ht = leave_one_out(data)
    print(f"leave-one-out up-vector match: {hits}/{len(data)}"
          f"   (hand-seated only: {hh}/{ht})")
    joblib.dump(_fit(np.vstack([d[2] for d in data]),
                     np.concatenate([d[3] for d in data])), MODEL_PATH)
    print(f"trained orientation model on all {len(data)} REFs -> {MODEL_PATH.name}")

    ldata = build_level_dataset()
    if ldata:
        reach = sum(int(d[3].any()) for d in ldata)
        print(f"\n{len(ldata)} level sets ({reach} with the seat level reachable)")
        lhits, lhh, lht = leave_one_out_levels(ldata)
        print(f"leave-one-out seat-level match: {lhits}/{len(ldata)}"
              f"   (hand-seated only: {lhh}/{lht})")
        joblib.dump(_fit(np.vstack([d[2] for d in ldata]),
                         np.concatenate([d[3] for d in ldata])), LEVEL_MODEL_PATH)
        print(f"trained level model -> {LEVEL_MODEL_PATH.name}")

        import json

        STATS_PATH.write_text(json.dumps({
            "n_refs": len(data),
            "orientation": {"loo_all": hits, "n_all": len(data),
                            "loo_hand": hh, "n_hand": ht},
            "level": {"loo_all": lhits, "n_all": len(ldata),
                      "loo_hand": lhh, "n_hand": lht},
        }, indent=1))
        print(f"wrote fidelity stats -> {STATS_PATH.name}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
