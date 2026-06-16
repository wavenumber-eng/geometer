"""Single source of truth for the hand-conditioned reference seatings.

Before this, REFs were scattered across TEST_STEP_FILES, ADDITIONAL_TEST_STEP,
and REFERENCE_STEP_FILES under two suffixes, with three tools each re-deriving
discovery — and the same part could be seated in two files (1711725 was once
seated 180 deg apart in two of them). Now every conditioned reference lives in
REFERENCE_STEP_FILES as `<base>_AP242_conditioned.step`, and the trainer,
the Z-sit scorer, and the split benchmark all enumerate them through here.

Each tool picks what it needs from a reference's JOURNAL, not its filename:
a Z-sit-only seating carries just a `zsit` op; a full session also carries
`detect_pins`/`separate`/etc. So one folder + one suffix serves both the
Z-sit ranker (needs the zsit op) and the split benchmark (needs separate ops).
"""

from __future__ import annotations

from dataclasses import dataclass
import re
from functools import lru_cache
from pathlib import Path

import numpy as np

PROTO = Path(__file__).resolve().parents[1]
REF_DIR = PROTO / "REFERENCE_STEP_FILES"
SUFFIX = "_AP242_conditioned"
# The reference folder serves the two ML tools. Each part's ground truth lives
# in up to two files, distinguished by tag:
#   _REF_ZSIT  — the Z-Sit seating (Step 1). Holds the original/full journal.
#   _REF_FDEF  — the Front DEFinition / in-plane rotation (Step 2).
# (Legacy `_AP242_conditioned`/`_AP242_WNCn` tags still parse, for transition.)
# Both files of a base are combined into one Reference; the ZSIT file is the
# representative (its metadata also carries nets/separate for the benchmarks).
_REF_TAG_RE = re.compile(
    r"^(?P<base>.*)_(?:REF_ZSIT|REF_FDEF|AP242_conditioned|AP242_WNC\d+)$")


def _base_of(stem: str) -> str | None:
    m = _REF_TAG_RE.match(stem)
    return m.group("base") if m else None
# Where raw source models may live (refs only carry the seating; the source is
# the un-conditioned geometry the tools actually load and re-seat).
SOURCE_DIRS = [
    PROTO / "TEST_STEP_FILES",
    PROTO / "TEST_STEP_FILES" / "ADDITIONAL_TEST_STEP",
    PROTO / "TEST_STEP_FILES" / "ADDITIONAL_TEST_STEP_2",
    REF_DIR,
    PROTO.parents[1] / "tests" / "fixtures" / "step" / "embedded_models",
]


@dataclass
class Reference:
    base: str
    source: Path          # the raw, un-conditioned model
    conditioned: Path      # the LATEST-version conditioned file
    journal: list          # ops COMBINED across every WNC version of this base,
                           # earliest first (so a later rebake that adds a Front
                           # op doesn't drop the earlier Z-Sit ground truth)

    def zsit_matrix(self):
        """The 4x4 seating transform from the earliest version's zsit op."""
        for op in self.journal:
            if op.get("tool") == "zsit" and op.get("result", {}).get("matrix"):
                return np.asarray(op["result"]["matrix"], dtype=float)
        return None

    def front_op(self):
        """The latest Front (in-plane rotation) op, or None — the Step-2 ground
        truth added by re-conditioning a seated ref with Redefine Front."""
        for op in reversed(self.journal):
            if op.get("tool") == "front":
                return op
        return None

    def has_op(self, tool: str) -> bool:
        return any(op.get("tool") == tool for op in self.journal)


def _source_for(base: str) -> Path | None:
    for directory in SOURCE_DIRS:
        if not directory.is_dir():
            continue
        for cand in directory.iterdir():
            if (cand.is_file() and cand.suffix.lower() in (".step", ".stp")
                    and SUFFIX not in cand.stem
                    and not cand.stem.lower().endswith("_zsit_auto")
                    and cand.stem.lower() == base.lower()):
                return cand
    return None


@lru_cache(maxsize=1)
def _load_all() -> tuple:
    from app.export_ap242 import extract_metadata

    # Group every ref file by base and combine their journals, so a part's
    # Z-Sit (_REF_ZSIT) and Front (_REF_FDEF) ground truths land on one
    # Reference. The representative `conditioned` file is the one carrying a
    # zsit op (the original/full metadata — Front-only files have no nets).
    by_base: dict[str, list[tuple[Path, list]]] = {}
    for cond in REF_DIR.glob("*.step"):
        base = _base_of(cond.stem)
        if base is None:
            continue
        meta = extract_metadata(cond) or {}
        by_base.setdefault(base, []).append((cond, meta.get("journal", [])))
    refs = []
    for base in sorted(by_base):
        source = _source_for(base)
        if source is None:
            continue
        files = by_base[base]
        rep = next(
            (p for p, j in files
             if any(op.get("tool") == "zsit" for op in j)),
            files[0][0],
        )
        combined: list = []
        for _path, journal in sorted(files, key=lambda f: str(f[0])):
            combined.extend(journal)
        refs.append(Reference(base, source, rep, combined))
    return tuple(refs)


def all_references() -> list[Reference]:
    """Every hand-conditioned reference with a locatable source, deduped by base."""
    return list(_load_all())


def seatings() -> list[Reference]:
    """References that carry a Z-sit seating (for the Z-sit ranker/scorer)."""
    return [r for r in all_references() if r.zsit_matrix() is not None]
