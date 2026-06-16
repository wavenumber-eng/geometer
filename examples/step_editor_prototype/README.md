# WN3D STEP Conditioning Editor (prototype)

A desktop GUI for **conditioning** electronic-component STEP files: take any
vendor model (AP203 / AP214 / AP242), stand it upright, find and name its pins,
draw connection hitboxes, assign pin functions, recolor bodies, emboss a logo —
and write a clean **AP242** file with all of that captured as embedded metadata
next to the original:

```
wheel4506.step  ->  wheel4506_AP242_conditioned.step
```

The conditioned file carries a JSON metadata block (frame, bodies, pins with
number / name / function / net / hitbox) **plus the operation journal** — the
declarative list of tool steps that produced it. The journal replays headlessly,
which is what lets the same hand-edits later be done automatically.

> **Status:** working prototype on branch `prototype/step-file-editor`. The full
> tool chain (load → Z-sit → front → detect pins → separate → hitboxes →
> functions → colors → logo → export) runs end-to-end, with a headless
> auto-conditioner (`--auto`) and per-milestone selftests. See
> [`docs/DESIGN_INTENT.md`](docs/DESIGN_INTENT.md) for the spec it implements and
> [`docs/BIG_DREAM.md`](docs/BIG_DREAM.md) for where it is headed.

---

## Why condition a STEP file?

A conditioned model knows where its pins are *in 3D space* and what they mean.
That unlocks:

- **Footprint + schematic embedded in the 3D file** — one source of truth.
- **Nets tracked across real 3D space** between board models in a stackup.
- **Pin hitboxes** — any program can test whether a pin physically connects to
  geometry, so connectors and DuPont headers gain real electrical meaning.
- **Simulation** of chips driven by their actual geometric pin outputs.

---

## Quickstart

**Prerequisites**

- [uv](https://docs.astral.sh/uv/) on `PATH`
- Python 3.12 (pinned in `.python-version`; `requires-python >= 3.11`)
- This folder lives inside the **geometer** repo — it depends on `wn-geometer`
  as an editable path dependency (`../..`), so keep it in place.

**Run the GUI**

Windows — double-click `run_step_editor.bat` (or drag a `.step` file onto it).
The first run creates the environment with `uv sync` automatically.

Cross-platform / from a shell, run from the **geometer repo root**:

```bash
uv run --project examples/step_editor_prototype \
  python examples/step_editor_prototype/step_editor.py path/to/model.step
```

You can also open a KiCad `.kicad_mod` or an Altium `.PcbLib` directly — the
editor extracts the embedded STEP, conditions it, and can bake it back.

---

## The tools

A **tool** is a mode shown as a rectangle in the top-center of the window. Click
it or press **T** to cycle. Each tool's actions appear in the right-hand panel;
defaults are sensible but every nuanced choice is an adjustable parameter,
because *defining how the tools execute is the point of the prototype*.

| Tool | What it does |
|------|--------------|
| **Inspect** | Click bodies/faces; read body/face IDs and model info. |
| **Redefine Z-Sit Plane** | 3 picks define the seating plane, a 4th picks +Z. Re-seats the whole model Z-up on `z = 0`. |
| **Redefine Front** | In-plane rotation about +Z: a line becomes parallel to X, a point drops into −Y, fixing the canonical "front". |
| **Detect and Name Pins** | Drag a rectangle in top view; edge-flow grows pin regions (multi-body or unibody) and numbers them by geometric centroid. |
| **Separate Unibody** | Split a single-solid model into per-pin bodies. |
| **Assign Pin Hitboxes** | Place metadata-only bounding volumes per pin (male box / female cavity / BGA cube / pad prism / convex hull). |
| **Assign Pin Functions/Names** | Mini-schematic; label each pin (GND, PWR, CLK, SDA…); names are geometric, not just informational. |
| **Redefine Colors** | Recolor individual bodies. |
| **Apply LOGO** | Emboss `WN3D.dxf` onto a picked face at a chosen depth. |

When you're done, **Write AP242** exports the conditioned file + metadata block.

---

## Headless / automation

```bash
# Fully automatic: chain every tool's Auto, journal it, export AP242
python step_editor.py --auto model.step [--out out.step]

# Replay a recorded operation journal onto a (possibly new) source
python step_editor.py --apply ops.json model.step [--out out.step]

# Per-milestone selftests (offscreen, no GUI): m0..m8 or 'all'
python step_editor.py --selftest all
```

(Prefix with `uv run --project . ` when not already in the project venv.)

---

## Vendor round-trip (KiCad / Altium)

The editor opens KiCad `.kicad_mod` and Altium `.PcbLib` containers, conditions
the embedded STEP, and bakes the AP242 back in. The discipline (per
`docs/DESIGN_INTENT.md`) is a byte-identical round trip:

```
kicad_mod → AP242 → kicad_mod → AP242 → kicad_mod
```

KiCad embedding is built in. The Altium bake runs through `eval/altium_bake.py`
in `toolz/altium_monkey`'s own venv (its OCP pin differs) — see that file's
header for the invocation.

---

## Repository layout

```
step_editor_prototype/
├── step_editor.py          # entry point (GUI, --auto, --apply, --selftest)
├── run_step_editor.bat     # Windows launcher (self-bootstraps the venv)
├── pyproject.toml / uv.lock / .python-version
│
├── app/                    # the application
│   ├── app_window.py       # main window / layout
│   ├── document.py         # EditorDocument — ALL OpenCascade access lives here
│   ├── scene.py            # rendering, picking, highlights, overlays
│   ├── viewcam.py          # camera + lighting presets
│   ├── mode_rect.py        # the tool-mode rectangle (T to cycle)
│   ├── ortho2d.py          # HLR 2D canvas (top-view footprint preview)
│   ├── tools/              # one file per tool (zsit, front_face, detect_pins, …)
│   ├── pins.py             # PinRegistry, ordering, numbering propagation
│   ├── hitbox.py           # hitbox geometry
│   ├── metadata.py         # wn3d.step_conditioning schema (de)serialization
│   ├── journal.py          # operation journal (record + replay)
│   ├── export_ap242.py     # AP242 writer + metadata injection + re-read validate
│   ├── replay.py           # headless --apply driver
│   ├── auto.py             # headless --auto conditioner (chains every Auto)
│   ├── seat_model.py       # learned Z-sit seating ranker
│   ├── dxf_loader.py       # ezdxf → planar tool body for the logo
│   ├── kicad_embed.py      # KiCad embedded-STEP bake/extract
│   ├── vendor_files.py     # KiCad/Altium container open + bake orchestration
│   ├── refs.py             # reference-corpus registry
│   ├── style.py            # Wavenumber dark theme
│   └── selftest.py         # headless per-milestone selftests
│
├── docs/                   # design + vision documents (read these)
│   ├── DESIGN_INTENT.md    # the spec this implements
│   ├── AUTOMATE.md         # how sessions become training data for full auto
│   ├── BIG_DREAM.md        # the "PCB Entity" north star
│   └── Z_SIT_INSPECTION.md # manual Z-sit accuracy verdicts
│
├── eval/                   # internal dev tooling (needs the private corpus)
│   ├── score_zsit.py       # AUTO Z-sit vs hand REF (incl. orthogonality gate)
│   ├── score_pdet.py       # AUTO pin-detection audit vs ground truth
│   ├── score_auto.py       # per-dimension AUTO-vs-REF scorecard
│   ├── benchmark_split.py  # unibody split-quality benchmark
│   ├── train_seat_model.py # train the seat-level ML ranker
│   ├── autogen_refs.py     # auto-seat passives into references
│   ├── snap_eval.py        # vertex-plane snap evaluation
│   ├── altium_bake.py      # Altium PcbLib bake (runs in altium_monkey's venv)
│   └── scratch/            # throwaway triage drivers
│
├── benchmarks/             # committed REF journals / sessions (test data)
├── tests/                  # pytest suite (7 files)
├── WN3D.dxf                # logo geometry for Apply LOGO
└── wn3d_logo*.png          # window/taskbar branding
```

> Folders ignored by git (the user's local corpus): `TEST_STEP_FILES/`,
> `REFERENCE_STEP_FILES/`, `TEST_KICAD_MOD/`, `TEST_ALTIUM_PCB/`, trained
> `*.joblib` models, and `*_AP242_conditioned.step` outputs. The `eval/` scripts
> operate on that corpus and so are for local development, not a fresh checkout.

---

## Tests

```bash
uv run --project . pytest                      # the pytest suite (tests/)
python step_editor.py --selftest all           # end-to-end milestone selftests
```

`eval/` scripts must be run from this folder (the prototype root) so they can
find `app/` and the data dirs, e.g. `uv run python eval/score_zsit.py`.
