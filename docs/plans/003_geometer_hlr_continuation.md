# Geometer HLR / Browser Geometry Continuation

Date: 2026-04-30

This file captures the continuation state after the planning pass for moving STEP
HLR projection and STEP mesh conversion into `geometer`.

## Completion Update

Status: implemented for v0.1.0.

Completed work:

- Native C++ HLR projection from STEP bytes now emits `geometry.projection.a0`.
- Projection output includes `detail` and `simple` modes.
- C++ planar contour extraction replaced the Python/Shapely simple-mode
  dependency for this path.
- CLI development commands now emit JSON and SVG projection outputs:
  - `step-project-hlr`
  - `step-project-svg`
- A stable C ABI exists for byte-buffer HLR projection and version checks.
- Browser WASM builds now produce:
  - `dist/geometer.js`
  - `dist/geometer.wasm`
  - `dist/geometer-browser.js`
  - `dist/geometer-browser.wasm`
- Browser worker integration exists under `tests/wasm/`.
- Embedded model fixture prep and browser viewer/benchmark pages exist:
  - `scripts/prepare_embedded_model_fixtures.ps1`
  - `tests/wasm/embedded_model_viewer.html`
  - `tests/wasm/hlr_benchmark.html`
- Browser projection settings were aligned with the viz Python path:
  `curve_mode=polyline`, `samples_per_curve=24`, `round_digits=3`,
  `include_visible=true`, `include_outline=true`, and
  `union_simple_polygons=true`.
- Versioning has been added for v0.1.0:
  - project version `0.1.0`
  - C ABI version `1`
  - `geometer --version`
  - WASM exports `geometer_version_string` and `geometer_abi_version`
- Root docs were added/updated:
  - `AGENTS.md`
  - `DEVELOPMENT.md`
  - `INTERFACES.md`

## User Intent

- Treat this as plan-first work before further implementation.
- Focus on `geometer` first, with clean generic interfaces that are not tied to
  Altium, PCB assembly SVGs, or a single downstream project.
- First runtime target is browser WebAssembly.
- Keep the C++ implementation modular instead of putting new logic into
  `geometer.cpp`.
- Provide a path for the same C++ core to be usable from Python tooling later.
- Replace the existing Shapely-based "simple" projection mode fully, not only
  approximately.
- Add local geometer tests and development tooling, including a CLI path that can
  emit simple/detail projection SVGs for inspection.

## Context From Existing Repos

- `altium_cruncher` currently computes assembly STEP HLR projection in Python at:
  `C:/ELI/wn-hw-workspace/toolz/altium_cruncher/src/py/altium_cruncher/altium_cruncher_pcb_svg_assembly_projection.py`
- The current simple-mode caveat is the Shapely polygonize/union stage around
  `_build_contour_segments_from_segments`.
- `3d-viz` currently relies on precomputed Python-side mesh/GLB-style output for
  browser rendering.
- `data_models` already has semantic board data, so `geometer` should stay focused
  on CAD/kernel geometry operations instead of board semantics.

## Plan Documents Added

- `docs/requirements/002_step_hlr_projection.md`
  - Requirements for generic STEP HLR projection.
  - Covers STEP-byte input, simple/detail outputs, generic JSON, deterministic
    rounding, dev SVG output, and non-Altium constraints.
- `docs/plans/002_browser_hlr_and_mesh_interfaces.md`
  - Main architecture plan.
  - Covers C++ API, C ABI, CLI tooling, WASM target, Python target, test strategy,
    and migration path for `altium_cruncher` / `3d-viz`.

## Design Decisions To Preserve

- `geometer` should expose generic geometry APIs:
  - STEP bytes to HLR projection geometry.
  - STEP bytes to tessellated mesh / GLB / future typed-array mesh output.
- Output schema should be backend-neutral and consumable by Canvas, SVG, Three.js,
  and future WebGPU paths.
- Do not bake board placement, component selection, Altium naming, or visualizer
  policy into `geometer`.
- Browser WASM should not depend on Node filesystem behavior. Prefer direct byte
  buffers and a modular browser build.
- Python should call the same C++ core through a stable C ABI, likely via
  `ctypes`/`cffi` initially.
- Simple-mode replacement should be implemented in C++:
  1. Snap projected coordinates to a deterministic precision grid.
  2. Dedupe undirected segments.
  3. Node intersections and overlapping collinear segments.
  4. Build a directed half-edge graph.
  5. Trace closed faces using angle-sorted outgoing edges.
  6. Drop zero-area or invalid rings.
  7. Union polygons when needed.
  8. Emit exterior and hole rings deterministically.
- Clipper2 is the preferred first candidate for polygon union because it is
  C++/WASM-friendly. GEOS should only be considered if parity testing shows it is
  necessary.

## Implementation State

The scaffolding described in the original continuation plan has been kept and
completed. The current module split is:

- `step_to_glb.cpp` - STEP to GLB conversion.
- `hlr_projection.cpp` - OCCT HLR projection and simple/detail extraction.
- `planar_contours.cpp` - deterministic planar contour construction.
- `projection.cpp` - JSON/SVG projection serialization.
- `projection_options_json.cpp` - JSON option parsing for C ABI/WASM callers.
- `c_api.cpp` - stable C ABI entry points.
- `version.cpp` - project and C ABI version entry points.

Current public C++/C/WASM/CLI interfaces are documented in
`INTERFACES.md`.

## Validation Run

Formatting:

```powershell
clang-format -i src\cpp\lib\geometer.h src\cpp\lib\geometer\status.h src\cpp\lib\geometer\step_to_glb.h src\cpp\lib\geometer\projection.h src\cpp\lib\geometer\c_api.h src\cpp\lib\projection.cpp src\cpp\lib\c_api.cpp src\cpp\lib\step_to_glb.cpp
```

Syntax-only check:

```powershell
clang++ -std=c++17 -Isrc\cpp\lib -fsyntax-only src\cpp\lib\projection.cpp src\cpp\lib\c_api.cpp src\cpp\cli\main.cpp
```

Native build and tests:

```powershell
cmake --preset default
cmake --build build --config Release
ctest --test-dir build -C Release --output-on-failure
```

Version checks:

```powershell
.\dist\geometer.exe --version
node dist\geometer.js --version
```

Browser smoke:

```text
PASS version=0.1.0 abi=1 schema=geometry.projection.a0 detail=10 simple=8
```

## Remaining Follow-Up Candidates

1. Add persistent shape/projection caching for repeated browser view changes.
2. Add a Python wrapper around the C ABI after downstream integration needs are
   clearer.
3. Compare geometer output against altium-cruncher projection payloads on a
   representative board fixture.
4. Add mesh/typed-array browser APIs when direct STEP mesh rendering is needed.
5. Add release automation for `dist/` artifact verification.

## Useful Resume Commands

```powershell
git status --short
Get-Content docs\plans\002_browser_hlr_and_mesh_interfaces.md
Get-Content docs\requirements\002_step_hlr_projection.md
Get-Content docs\plans\003_geometer_hlr_continuation.md
clang++ -std=c++17 -Isrc\cpp\lib -fsyntax-only src\cpp\lib\projection.cpp src\cpp\lib\c_api.cpp src\cpp\cli\main.cpp
```

## Closed Questions

- The scaffolding remains and is now implemented.
- The initial public projection schema is `geometry.projection.a0`.
- The dev CLI writes one requested SVG view/mode per invocation.
- The initial projection output exposes line/arc geometry, not filled polygon
  payloads.
