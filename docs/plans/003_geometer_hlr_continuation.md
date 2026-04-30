# Geometer HLR / Browser Geometry Continuation

Date: 2026-04-30

This file captures the continuation state after the planning pass for moving STEP
HLR projection and STEP mesh conversion into `geometer`.

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

## Code State Already Touched

The following scaffolding/refactor changes were made before the user clarified
that the immediate work should be plan-only:

- Replaced monolithic `src/cpp/lib/geometer.cpp` with
  `src/cpp/lib/step_to_glb.cpp`.
- Added an umbrella-style `src/cpp/lib/geometer.h`.
- Added headers under `src/cpp/lib/geometer/`:
  - `status.h`
  - `step_to_glb.h`
  - `projection.h`
  - `c_api.h`
- Added stub/source files:
  - `src/cpp/lib/projection.cpp`
  - `src/cpp/lib/c_api.cpp`
- Updated `src/cpp/lib/CMakeLists.txt` to include the new source files.

No real STEP HLR implementation has been added yet. The projection API currently
contains value types and stubs only.

## Validation Already Run

Formatting:

```powershell
clang-format -i src\cpp\lib\geometer.h src\cpp\lib\geometer\status.h src\cpp\lib\geometer\step_to_glb.h src\cpp\lib\geometer\projection.h src\cpp\lib\geometer\c_api.h src\cpp\lib\projection.cpp src\cpp\lib\c_api.cpp src\cpp\lib\step_to_glb.cpp
```

Syntax-only check:

```powershell
clang++ -std=c++17 -Isrc\cpp\lib -fsyntax-only src\cpp\lib\projection.cpp src\cpp\lib\c_api.cpp src\cpp\cli\main.cpp
```

Full OCCT/CMake build was not run because `.deps/` and `build/` were not present
in this checkout, and rebuilding OCCT would be a longer operation.

## Suggested Resume Steps

1. Review `docs/plans/002_browser_hlr_and_mesh_interfaces.md` and this
   continuation file before writing more code.
2. Decide whether to keep the scaffolding changes that were already made or
   restore to docs-only planning state.
3. Confirm the public projection schema name. The planning doc currently proposes
   `wn.geometry.projection.a0`.
4. Confirm whether Clipper2 is acceptable as the C++/WASM polygon union engine.
5. Add focused unit tests for the Shapely replacement before implementing HLR:
   - closed square
   - hole ring
   - duplicate reversed segments
   - overlapping collinear segments
   - crossed/noded linework
   - nested faces and deterministic ordering
6. Implement `planar_contours` as its own module.
7. Implement OCCT HLR extraction in a separate `hlr_projection` module.
8. Add CLI commands for development snapshots:
   - JSON projection output
   - simple SVG output
   - detailed SVG output
9. Add browser-specific WASM build/export path after native tests are stable.
10. Add Python wrapper after the C ABI has stabilized.

## Useful Resume Commands

```powershell
git status --short
Get-Content docs\plans\002_browser_hlr_and_mesh_interfaces.md
Get-Content docs\requirements\002_step_hlr_projection.md
Get-Content docs\plans\003_geometer_hlr_continuation.md
clang++ -std=c++17 -Isrc\cpp\lib -fsyntax-only src\cpp\lib\projection.cpp src\cpp\lib\c_api.cpp src\cpp\cli\main.cpp
```

## Open Questions

- Should the current scaffolding remain, or should this branch be restored to a
  docs-only planning state before continuing?
- Is Clipper2 acceptable for the polygon union stage?
- Should the dev CLI write two separate SVG files, or one SVG with simple/detail
  layers?
- Should projection output include only curves first, or also prebuilt rings and
  filled polygons in the initial schema?
