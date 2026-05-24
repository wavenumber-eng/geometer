# Changelog

All notable changes to geometer are documented here.

## [2026.5.23] - 2026-05-23

### Changed

- Switched Geometer's current release identity, CMake version, Python package
  version, runtime version string, and C ABI generation to the ADR 006
  date-based scheme: `v2026-05-23`, `2026.5.23`, and ABI `20260523`.
- Removed the old `dist/baseline/` WASM snapshot and the browser UI/script
  paths that selected it. The completed HLR performance work remains documented
  in `docs/plans/004_poly_hlr_perf_results.md`.
- Made the public Python package executable-backed only for the first PyPI
  release path. `GEOMETER_BACKEND=exe`/`cli` are accepted explicit names;
  ctypes/native/worker names are rejected by the public API.
- Started the transition from flat `dist/` artifacts to grouped
  `dist/native/<platform>/` and `dist/wasm/<target>/` paths. Flat root-level
  `dist/` copies remain compatibility aliases for existing tools.
- Python wheel builds now bundle the executable under
  `geometer/native/<platform>/`, matching the source-checkout lookup policy.

### Added

- `geometer run request.json response.json` batch requests now support
  request-level default `options` with per-job overrides.
- Python `geometer.run_batch(...)` and `geometer.GeometerBatchRunner` for
  chunked multi-process execution of repeated HLR/GLB jobs.

### Removed

- Removed the unused Python ctypes/worker backend modules from the package.
  The C ABI remains available for native C++/WASM boundaries, but PyPI-facing
  Python uses the executable backend.

## [2026.5.16] - 2026-05-16

### Highlights

HLR projection performance push. The default algorithm switches from OCCT's
exact `HLRBRep_Algo` to the tessellation-based `HLRBRep_PolyAlgo`, yielding a
**~4x overall corpus speedup** with the largest wins on dense BGA-style models
(up to 20x). Same JSON schema, same `geometry.projection.a0` contract.

See `docs/plans/004_poly_hlr_perf_results.md` for the full per-model bench.

### Changed

- **Default `projection_algorithm` is now `Poly`.** Existing callers continue
  to use the same C ABI and JSON schema. Set `projection_algorithm = "exact"`
  to opt back into the previous `HLRBRep_Algo` behavior.
- Default mesh deflection: `mesh_linear_deflection = 0.01` mm,
  `mesh_angular_deflection = 0.5` rad (matches the values used to produce the
  benchmarks). Tune these per-call if you need looser or tighter tessellation.

### Added

- `ProjectionAlgorithm` enum (`Poly` / `Exact`) on `HlrProjectionOptions`.
- Tunable mesh / HLR fields on `HlrProjectionOptions`:
  `mesh_linear_deflection`, `mesh_angular_deflection`, `mesh_relative`,
  `hlr_angle_tolerance`. All optional with safe defaults.
- Granular OCCT edge category flags on `HlrProjectionOptions`. Ten booleans
  mapped 1:1 to `HLRBRep_HLRToShape::VCompound / OutLineVCompound /
  Rg1LineVCompound / RgNLineVCompound / IsoLineVCompound` and the matching H
  variants. Defaults reproduce 1.0 behavior (visible sharp + outline). The
  poly path only supports V/H Compound + OutLine; the smooth/sewn/iso flags
  are no-ops in poly.
- `HlrProjectionTimings timings` on `HlrProjectionResult` exposing per-phase
  native timings (`step_read_ms`, `mesh_ms`, `hlr_ms`, `extract_ms`).
  Serialized as a `"timings"` JSON object on the projection result.
- WASM viewer (`tests/wasm/embedded_model_viewer.html`) UI for the new
  options: backend / algorithm selector, mesh tunables, edge category profile
  presets, and live "Cam" projection from the 3D camera direction.
- `scripts/bench_hlr.js` plus `docs/plans/004_poly_hlr_perf*.md` for
  reproducible perf comparisons between 1.0 and 1.1.
- Temporary 1.0 WASM snapshot for side-by-side comparison during the completed
  browser HLR performance work.

### Removed

- `include_visible` / `include_outline` are no longer struct fields on
  `HlrProjectionOptions`. They are still accepted as JSON aliases by the
  options parser (`include_visible: false` clears `edge_v_sharp` +
  `edge_v_outline`; `include_outline` maps to `edge_v_outline`). Source
  callers that wrote `opts.include_visible = false` need a one-line rename
  to the granular flags.

### Migration notes

- Existing JSON callers: nothing required. The `geometry.projection.a0`
  schema is unchanged. Optional new fields are ignored by 1.0 readers.
- C++ callers: replace direct reads/writes of `opts.include_visible` /
  `opts.include_outline` with the corresponding `opts.edge_v_*` flags.
- Existing wall-clock comparisons may shift; see the bench doc for numbers.

## Prior Release

Initial public release. HLR projection via exact `HLRBRep_Algo`, planar
geometry helpers, STEP / GLB conversion, browser and native build targets.
