# Changelog

All notable changes to geometer are documented here.

## [Unreleased]

### Changed

- Updated the pinned OCCT dependency to `V8_0_1` after exact-tag native,
  WASM, browser, generated-client, and cross-tag qualification.

## [2026.6.23] - 2026-06-23

### Fixed

- Built Linux wheels on Ubuntu 22.04 so the bundled native executable remains
  compatible with GLIBC 2.35 systems such as common WSL Ubuntu 22.04 installs.
- Scoped native OCCT dependency caches by Linux glibc baseline so Ubuntu 24.04
  binary artifacts cannot be restored into Ubuntu 22.04 release jobs.

### Changed

- Updated the release identity to `v2026-06-23`,
  `wn-geometer==2026.6.23`, and C ABI generation `20260623`.

## [2026.6.10] - 2026-06-10

### Added

- Added Wavenumber R2 dependency-cache support for OCCT native and WASM
  install trees, including CI producer workflow coverage for Windows, Linux,
  Linux ARM64, macOS ARM64, and WASM targets.
- Added developer documentation for read-only R2 cache setup on new machines.
- Added Lizard complexity checking and reproducible `clang-format` tooling to
  the release signoff environment.

### Changed

- Updated the pinned OCCT dependency to `V8_0_0`.
- Updated GitHub workflows to use the OCCT dependency cache and Node
  24-capable action majors.
- Updated the release identity to `v2026-06-10`,
  `wn-geometer==2026.6.10`, and C ABI generation `20260610`.

## [2026.6.9] - 2026-06-09

### Added

- Added `geometry.projection.b0` with `outline`, `detail`, and `bbox`
  projection modes.
- Added the mesh-shadow outline algorithm for assembly projection silhouettes.
- Added planar batch solve JSON ring output through the C ABI, CLI, WASM, and
  Python package.
- Added platform-grouped native preview app output under
  `dist/native/<platform>/`.

### Changed

- Removed the public `simple` projection mode name. Use `outline`.
- Updated the release identity to `v2026-06-09`,
  `wn-geometer==2026.6.9`, and C ABI generation `20260609`.

## [2026.6.4] - 2026-06-04

### Fixed

- Default macOS native and wheel builds to deployment target 11.0 for Apple
  Silicon compatibility, and validate the Mach-O minimum OS before packaging.

### Added

- Added Wavenumber `python-native-wasm` development standards metadata,
  root hygiene files, C++ static-analysis config, root uv lockfile, and
  lightweight multi-OS L99 CI.
- Added Pyright and `uv lock --check` to the L99 release signoff gate with a
  documented legacy ratchet toward strict Python typing.

### Changed

- Updated the release identity to `v2026-06-04`,
  `wn-geometer==2026.6.4`, and C ABI generation `20260604`.

## [2026.5.25] - 2026-05-25

### Added

- Added macOS arm64 native build and Python wheel validation for
  `wn-geometer`.
- Added a `geometer` console script entry point to the Python wheel; it forwards
  arguments to the bundled native executable in the active install environment.
- Added model-bounds projection overlays to the native Dear ImGui and PyVista
  HLR preview examples.

### Changed

- Updated the release identity to `v2026-05-25`,
  `wn-geometer==2026.5.25`, and C ABI generation `20260525`.

## [2026.5.24.2] - 2026-05-24

### Added

- Added `geometry.planar_step.request.a0` and the native `planar-step`
  operation for exact planar-region-to-STEP synthesis.
- Added Python `geometer.planar_step(...)` and `geometer.write_planar_step(...)`.
- Added native, CLI, batch, and package validation coverage for planar STEP
  generation.
- Documented the planar STEP contract across direct CLI, batch CLI, Python, and
  C++ entry points. Python callers should use the `geometer` package wrapper;
  the package remains executable-backed internally.
- Added same-day release serial support for release `v2026-05-24-2`
  (`wn-geometer==2026.5.24.2`) while retaining C ABI generation `20260524`.

### Changed

- `geometer --help`, `geometer -h`, and `geometer help` now print usage and
  exit successfully.

## [2026.5.24] - 2026-05-24

### Changed

- Published the second public PyPI release as `wn-geometer==2026.5.24`.
  The release includes `py3-none-win_amd64` and
  `py3-none-manylinux_2_39_x86_64` wheels with bundled native executables.
- Promoted Linux/WSL2 native and installed-wheel validation to first-class
  release flow.
- Validated public PyPI installs on Windows and WSL2, including STEP HLR JSON,
  SVG, and GLB generation through the installed Python package.
- Validated Toolz / Altium Cruncher on WSL2 against the published PyPI package.
- Removed legacy root-level `dist` artifacts. Native and WASM artifacts now use
  grouped `dist/native/<platform>/` and `dist/wasm/<target>/` paths only.
- Moved maintained interface documentation into `docs/design/` and split it by
  interface/function area.
- Moved the developer guide to `docs/developer/README.md`.
- Replaced stale requirements with current release requirements.
- Removed persisted completed implementation plans; ADRs, requirements, design
  docs, and code are the docs of record after work ships.
- Promoted the embedded model browser viewer to `examples/wasm/`.
- Added the L99 release signoff stratum for Ruff, clang-format, code hygiene,
  and stale artifact checks.

### Removed

- Removed `CLAUDE.md`.
- Removed the retired Dear PyGui Python HLR viewer.

## [2026.5.23] - 2026-05-23

### Changed

- Switched Geometer's current release identity, CMake version, Python package
  version, runtime version string, and C ABI generation to the ADR 006
  date-based scheme: `v2026-05-23`, `2026.5.23`, and ABI `20260523`.
- Removed the old `dist/baseline/` WASM snapshot and the browser UI/script
  paths that selected it.
- Made the public Python package executable-backed only for the first PyPI
  release path. `GEOMETER_BACKEND=exe`/`cli` are accepted explicit names;
  ctypes/native/worker names are rejected by the public API.
- Started the transition from flat `dist/` artifacts to grouped
  `dist/native/<platform>/` and `dist/wasm/<target>/` paths.
- Python wheel builds now bundle the executable under
  `geometer/native/<platform>/`, matching the source-checkout lookup policy.
- Renamed the Python distribution package to `wn-geometer`; the import package
  remains `geometer`.
- Published the first public Windows x64 wheel to PyPI as
  `wn-geometer==2026.5.23`.

### Added

- `geometer run request.json response.json` batch requests now support
  request-level default `options` with per-job overrides.
- Python `geometer.run_batch(...)` and `geometer.GeometerBatchRunner` for
  chunked multi-process execution of repeated HLR/GLB jobs.
- Python PyVista and native Dear ImGui example viewers that show the Geometer
  version and C ABI generation.

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

The full per-model bench was kept as a temporary implementation artifact and is
not part of the maintained docs of record.

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
- WASM viewer UI for the new
  options: backend / algorithm selector, mesh tunables, edge category profile
  presets, and live "Cam" projection from the 3D camera direction.
- `scripts/bench_hlr.js` for
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
