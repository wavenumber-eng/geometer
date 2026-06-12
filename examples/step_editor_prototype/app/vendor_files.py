"""Open and bake vendor container files (.kicad_mod / .PcbLib) — Phase 2.

Pure logic, no Qt, so the flow is selftest-able headlessly. The editor's
container workflow (DESIGN_INTENT "Additive Design Intent", UX decided
2026-06-11): one part per file is the database-grade form. Single-part
containers open directly; multi-footprint PcbLibs are split (the user picks
the footprint) and only the split single-part library is conditioned. Export
always produces a single <part>_conditioned file; the opened source is never
modified.

KiCad containers are handled natively via kicad_embed. Altium containers go
through altium_bake.py running in toolz/altium_monkey's venv (its cadquery
pin clashes with our cadquery-ocp), driven over subprocess.
"""

from __future__ import annotations

import json
import shutil
import subprocess
from dataclasses import dataclass
from pathlib import Path
from typing import Callable

PROTO = Path(__file__).resolve().parents[1]
ALTIUM_MONKEY = PROTO.parents[2] / "toolz" / "altium_monkey"
BAKER = PROTO / "altium_bake.py"

VENDOR_SUFFIXES = {".kicad_mod", ".pcblib"}

# rows from altium_bake --list: [{"footprint": str, "models": [{name, kind, bytes}]}]
FootprintChooser = Callable[[list[dict]], str | None]


def is_vendor_file(path: Path) -> bool:
    return Path(path).suffix.lower() in VENDOR_SUFFIXES


def _safe_filename(name: str) -> str:
    return "".join("_" if c in '<>:"/\\|?*' else c for c in name).strip() or "part"


@dataclass
class VendorContainer:
    kind: str  # "kicad" | "altium"
    source: Path  # the file the user opened (never modified)
    part_name: str  # footprint/part name, used for output naming
    model_name: str  # embedded model name the bake targets
    work_lib: Path | None  # altium: the single-part library the bake reads
    step_path: Path  # extracted STEP the editor actually loads

    def conditioned_container_path(self) -> Path:
        suffix = ".kicad_mod" if self.kind == "kicad" else ".PcbLib"
        return self.source.parent / f"{_safe_filename(self.part_name)}_conditioned{suffix}"

    def conditioned_step_path(self) -> Path:
        return self.source.parent / (
            f"{_safe_filename(self.part_name)}_AP242_conditioned.step"
        )


def altium_available() -> str | None:
    """None when the Altium toolchain is usable, else the reason it isn't."""
    if shutil.which("uv") is None:
        return "uv not found on PATH"
    if not ALTIUM_MONKEY.is_dir():
        return f"altium_monkey not found at {ALTIUM_MONKEY}"
    return None


def _run_altium(args: list[str], timeout: int = 600) -> str:
    reason = altium_available()
    if reason:
        raise RuntimeError(f"Altium support unavailable: {reason}")
    proc = subprocess.run(
        ["uv", "run", "--project", str(ALTIUM_MONKEY), "python", str(BAKER), *args],
        capture_output=True, text=True, cwd=str(ALTIUM_MONKEY), timeout=timeout,
    )
    if proc.returncode != 0:
        raise RuntimeError(
            f"altium_bake {args[0]} failed:\n{(proc.stdout + proc.stderr)[-500:]}"
        )
    return proc.stdout


def list_pcblib(path: Path) -> list[dict]:
    return json.loads(_run_altium(["--list", str(path)]))


def open_vendor_file(
    path: Path,
    workdir: Path,
    choose_footprint: FootprintChooser | None = None,
) -> VendorContainer | None:
    """Extract the embedded STEP from a container into `workdir`.

    Returns None when the user cancels the footprint chooser. Raises with a
    user-facing message for containers we can't open (no embedded model,
    non-STEP payload, Altium toolchain missing).
    """
    path = Path(path)
    workdir = Path(workdir)

    if path.suffix.lower() == ".kicad_mod":
        from .kicad_embed import decode_payload, find_embedded_files

        with open(path, encoding="utf-8", newline="") as fh:
            text = fh.read()
        models = [e for e in find_embedded_files(text) if e.file_type == "model"]
        if not models:
            raise RuntimeError(f"{path.name} has no embedded 3D model")
        raw = decode_payload(models[0].data_base64)
        if not raw[:200].lstrip().startswith(b"ISO-10303-21"):
            raise RuntimeError(f"{path.name}: embedded model is not STEP")
        step_path = workdir / f"{_safe_filename(path.stem)}__model.step"
        step_path.write_bytes(raw)
        return VendorContainer(
            "kicad", path, path.stem, models[0].name, None, step_path
        )

    rows = list_pcblib(path)
    step_rows = [r for r in rows if any(m["kind"] == "step" for m in r["models"])]
    if not step_rows:
        raise RuntimeError(f"{path.name} has no footprint with an embedded STEP model")
    if len(step_rows) == 1:
        chosen = step_rows[0]["footprint"]
    else:
        if choose_footprint is None:
            raise RuntimeError(f"{path.name} holds {len(step_rows)} footprints — a chooser is required")
        chosen = choose_footprint(step_rows)
        if chosen is None:
            return None

    if len(rows) == 1:
        work_lib = path  # already database-grade: one part per file
    else:
        work_lib = workdir / f"{_safe_filename(chosen)}.PcbLib"
        _run_altium(["--split", str(path), chosen, str(work_lib)])

    step_path = workdir / f"{_safe_filename(chosen)}__model.step"
    out = _run_altium(["--extract", str(work_lib), str(step_path)])
    model_name = json.loads(out.strip().splitlines()[-1])["model_name"]
    return VendorContainer("altium", path, chosen, model_name, work_lib, step_path)


def bake_conditioned(
    container: VendorContainer,
    conditioned_step: Path,
    out_path: Path | None = None,
) -> Path:
    """Bake the conditioned AP242 back into a single-part container file."""
    out = out_path or container.conditioned_container_path()
    if container.kind == "kicad":
        from .kicad_embed import bake_step_into_kicad_mod

        bake_step_into_kicad_mod(
            str(container.source),
            Path(conditioned_step).read_bytes(),
            str(out),
            name=container.model_name,
        )
    else:
        _run_altium([
            "--replace", str(container.work_lib), container.model_name,
            str(conditioned_step), str(out),
        ])
    return out
