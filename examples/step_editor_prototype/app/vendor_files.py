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
import os
import shutil
import subprocess
from dataclasses import dataclass
from functools import lru_cache
from pathlib import Path
from typing import Callable

PROTO = Path(__file__).resolve().parents[1]
HOME = Path.home()
# Where an altium_monkey checkout may live, first hit wins. WN3D_ALTIUM_MONKEY
# overrides; the rest cover the geometer dev tree (toolz/ three levels up), the
# usual clone folders, and the wn-hw-workspace layouts on Windows and macOS.
ALTIUM_MONKEY_CANDIDATES = [
    PROTO.parents[2] / "toolz" / "altium_monkey",
    HOME / "GitHub-Projects" / "toolz" / "altium_monkey",
    HOME / "Projects" / "GitHub" / "toolz" / "altium_monkey",
    HOME / "toolz" / "altium_monkey",
    Path("C:/wn-hw-workspace/_from-gh-projects/toolz/altium_monkey"),
    HOME / "wn-hw-workspace" / "_from-gh-projects" / "toolz" / "altium_monkey",
    Path("C:/wn-hw-workspace/appz/cad/altium_monkey"),
    HOME / "wn-hw-workspace" / "appz" / "cad" / "altium_monkey",
]
# The baker ships next to the app in the skill; in geometer it lives in eval/.
BAKER = next((p for p in (PROTO / "altium_bake.py", PROTO / "eval" / "altium_bake.py")
              if p.is_file()), PROTO / "altium_bake.py")


def altium_monkey_dir() -> Path | None:
    env = os.environ.get("WN3D_ALTIUM_MONKEY")
    candidates = ([Path(env).expanduser()] if env else []) + ALTIUM_MONKEY_CANDIDATES
    return next((c for c in candidates if (c / "pyproject.toml").is_file()), None)


@lru_cache(maxsize=1)
def _python_with_altium_monkey() -> str | None:
    """A system Python that already has altium_monkey installed (pip), used
    when no checkout is found. Never the editor's own venv: altium_monkey's
    cadquery pin clashes with ours."""
    for name in ("python3", "python"):
        exe = shutil.which(name)
        if not exe:
            continue
        try:
            done = subprocess.run([exe, "-c", "import altium_monkey"],
                                  capture_output=True, timeout=60)
        except (OSError, subprocess.TimeoutExpired):
            continue
        if done.returncode == 0:
            return exe
    return None

VENDOR_SUFFIXES = {".kicad_mod", ".pcblib"}

# rows from altium_bake --list: [{"footprint": str, "models": [{name, kind, bytes}]}]
FootprintChooser = Callable[[list[dict]], str | None]


def sniff_model_format(raw: bytes) -> str:
    """What an embedded model really is: never trust its file extension (some
    GCT parts embed a binary mesh under a .STEP name)."""
    head = raw[:256].lstrip()
    if head.startswith(b"ISO-10303-21"):
        return "step"
    if head.startswith(b"**"):
        return "parasolid"
    if head.startswith(b"#VRML"):
        return "vrml"
    if head[:5].lower() == b"solid":
        return "stl (ascii)"
    if head.startswith(b"glTF"):
        return "glb"
    if raw[:2] == b"\x78\x9c" or raw[:2] == b"\x78\xda":
        return "zlib-compressed data"
    if len(raw) > 84 and int.from_bytes(raw[80:84], "little") * 50 + 84 == len(raw):
        return "stl (binary)"
    return "unknown binary" if b"\x00" in raw[:256] else "unknown text"


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


def _altium_command() -> tuple[list[str], str | None] | None:
    """(command prefix, cwd) that runs a script with altium_monkey importable."""
    checkout = altium_monkey_dir()
    if checkout is not None and shutil.which("uv") is not None:
        return ["uv", "run", "--project", str(checkout), "python"], str(checkout)
    python = _python_with_altium_monkey()
    if python is not None:
        return [python], None
    return None


def altium_available() -> str | None:
    """None when the Altium toolchain is usable, else the reason it isn't."""
    if not BAKER.is_file():
        return f"altium_bake.py is missing (expected at {BAKER})"
    if _altium_command() is None:
        return ("altium_monkey not found: clone toolz (altium_monkey) or set "
                "WN3D_ALTIUM_MONKEY to its folder, or pip install it into a system Python")
    return None


def _run_altium(args: list[str], timeout: int = 600) -> str:
    reason = altium_available()
    if reason:
        raise RuntimeError(f"Altium support unavailable: {reason}")
    prefix, cwd = _altium_command()
    proc = subprocess.run(
        [*prefix, str(BAKER), *args],
        capture_output=True, text=True, cwd=cwd, timeout=timeout,
    )
    if proc.returncode != 0:
        raise RuntimeError(
            f"altium_bake {args[0]} failed:\n{(proc.stdout + proc.stderr)[-500:]}"
        )
    return proc.stdout


def symbol_pins(schlib: Path, key: str | None = None) -> list[dict]:
    """A schematic symbol's pins [{number, name, type}] (altium_monkey)."""
    return json.loads(_run_altium(["--symbol-pins", str(schlib), *([key] if key else [])], timeout=120))


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
        kind = sniff_model_format(raw)
        if kind != "step":
            message = (f"{path.name}: unsupported embedded model format: {kind} "
                       f"(named {models[0].name!r}; only STEP models can be conditioned)")
            print(message, flush=True)  # visible in headless runs too
            raise RuntimeError(message)
        step_path = workdir / f"{_safe_filename(path.stem)}__model.step"
        step_path.write_bytes(raw)
        return VendorContainer(
            "kicad", path, path.stem, models[0].name, None, step_path
        )

    rows = list_pcblib(path)
    step_rows = [r for r in rows if any(m["kind"] == "step" for m in r["models"])]
    if not step_rows:
        kinds = sorted({m["kind"] for r in rows for m in r["models"]})
        message = (f"{path.name} has no footprint with an embedded STEP model"
                   + (f" (embedded formats found: {', '.join(kinds)})" if kinds else ""))
        print(message, flush=True)
        raise RuntimeError(message)
    if len(step_rows) == 1:
        chosen = step_rows[0]["footprint"]
    else:
        if choose_footprint is None:
            raise RuntimeError(f"{path.name} holds {len(step_rows)} footprints — a chooser is required")
        # The picker gets every footprint so unsupported ones show greyed out
        # with their real format, instead of failing after the user picks.
        chosen = choose_footprint(rows)
        if chosen is None:
            return None
        if chosen not in {r["footprint"] for r in step_rows}:
            raise RuntimeError(f"{chosen}: its embedded model is not STEP")

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
