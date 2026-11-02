"""Working through a folder of models: find the next one not yet conditioned.

A model counts as conditioned once a `<stem>_AP242_WNCn.step` export of it
sits in the same folder. The order follows the folder's worklist when there is
one (`worklist.csv` rank, else `paint_list.csv`), then the rest by name, so
the Next button walks the list the way it was ranked.
"""

from __future__ import annotations

import csv
from pathlib import Path

from .export_ap242 import _COND_TAG_RE

STEP_SUFFIXES = {".step", ".stp"}
ORDER_FILES = ("worklist.csv", "paint_list.csv")


def source_stem(path: Path) -> str:
    """The raw model's stem, whether `path` is the raw file or an export of it."""
    match = _COND_TAG_RE.match(path.stem)
    return match.group("base") if match else path.stem


def _listed_order(folder: Path) -> list[str]:
    for name in ORDER_FILES:
        sheet = folder / name
        if not sheet.is_file():
            continue
        try:
            with open(sheet, encoding="utf-8") as fh:
                return [row["copy"] for row in csv.DictReader(fh) if row.get("copy")]
        except (OSError, KeyError, csv.Error):
            continue
    return []


def raw_models(folder: Path) -> list[Path]:
    """Raw (never-exported) STEP files in `folder`, in worklist order."""
    raws = {p.name: p for p in folder.iterdir()
            if p.is_file() and p.suffix.lower() in STEP_SUFFIXES and not _COND_TAG_RE.match(p.stem)}
    ordered = [raws.pop(name) for name in _listed_order(folder) if name in raws]
    return ordered + sorted(raws.values(), key=lambda p: p.name.lower())


def is_conditioned(model: Path) -> bool:
    return any(model.parent.glob(f"{model.stem}_AP242_WNC*.step"))


def next_unconditioned(current: Path) -> tuple[Path | None, int]:
    """(the next raw model after `current` with no export yet, how many such
    models are left in the folder besides `current`). Wraps around to the
    top of the list; None when everything else is done."""
    models = raw_models(current.parent)
    stem = source_stem(current)
    start = next((i + 1 for i, m in enumerate(models) if m.stem == stem), 0)
    todo = [m for m in models[start:] + models[:start] if m.stem != stem and not is_conditioned(m)]
    return (todo[0] if todo else None), len(todo)
