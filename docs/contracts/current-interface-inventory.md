# Current Interface Inventory

This is the implementation-backed baseline for the TypeSpec contract program
tracked by [GitHub issue #18](https://github.com/wavenumber-eng/geometer/issues/18).
It records the surface as inspected on 2026-08-12; it does not promote any
contract. The machine-readable promotion state lives in
`promotion-manifest.toml` beside this file.

## Version authorities

| Concern | Current value | Authority |
| --- | --- | --- |
| Native/WASM runtime and Python package | `2026.9.4` | `CMakeLists.txt`, `pyproject.toml`, ADR-006 |
| TypeScript/Rust client packages | `2026.9.4` | Package manifests and generated Fast HLR, illustration, and analytic contracts |
| C ABI generation | `20260904` | `CMakeLists.txt`, generated `version_config.h` |
| Executable IPC | none | No persistent pipe exists; the first contract will be `a0` |
| Planar batch packet | version `2` | `src/cpp/lib/planar_solve.cpp` |
| Triangulation packet | version `1` | `src/cpp/lib/planar_triangulate.cpp` |
| Clipper2 packets | version `1` | `src/cpp/lib/clipper2_bytes.cpp` |

These identities are independent. A change to one does not implicitly change
another.

## Structured JSON roots

| Root or structural surface | Current authority | Compatibility notes |
| --- | --- | --- |
| `geometry.model_bounds.a0` result | `model_bounds.h/.cpp` handwritten writer | Timings are nondeterministic; options accept `model_format` and `modelTransform` aliases |
| `geometry.projection.b0` result | `projection.h/.cpp` and `hlr_projection.cpp` | HLR option parser accepts snake, camel, and named legacy aliases |
| STEP-to-GLB options | `step_to_glb_options_json.cpp` | Accepts `linearDeflection`, `deflection`, `angularDeflection`, and `angular` aliases |
| `geometry.planar_step.request.a0` | `planar_step.cpp` | Accepts topology-first rings, transitional contour shapes, unit-suffixed values, and `fuseRegions`/`fuse` |
| `geometry.planar_batch_solve.a0` result | `planar_solve.cpp` handwritten writer | Input is the packed planar batch packet, not JSON |
| `geometer.batch.request.a0` | `src/cpp/cli/main.cpp` | `jobs` is the only required root field; top-level then per-job option patches |
| `geometer.batch.response.a0` | `src/cpp/cli/main.cpp` | Includes nondeterministic `elapsed_ms` per job |

There are no maintained generated JSON Schemas for these roots yet.

## Native C++ public surface

The umbrella header `src/cpp/lib/geometer.h` exposes model format and bounds,
HLR projection and option parsing, STEP-to-GLB and option parsing, planar
contours, planar solve, planar STEP, status, and version values/functions.
Triangulation and Clipper2 byte bridges are public focused headers but are not
currently included by the umbrella header.

The current structural codecs are handwritten:

- `model_bounds_options_json.cpp` and the bounds JSON writer;
- `projection_options_json.cpp` and the projection JSON writer;
- `step_to_glb_options_json.cpp`;
- `planar_step.cpp` JSON parsing;
- `planar_solve.cpp` JSON and packed packet writing/reading; and
- CLI batch parsing/writing in `src/cpp/cli/main.cpp`.

Integer `Status.code` values are local implementation statuses. They are not a
stable cross-operation wire diagnostic taxonomy.

## C ABI and browser exports

`src/cpp/lib/geometer/c_api.h` declares result structs, HLR projection,
STEP-to-GLB, planar batch solve, triangulation, Clipper2 boolean and inflate,
version, and free functions. Returned strings and byte buffers are
caller-owned and released with the matching Geometer free function.

The full browser target exports all current C ABI functions plus `_malloc` and
`_free`. The planar-only browser target exports version/free functions and the
packed planar operations. Both export Emscripten runtime helpers listed in
`docs/design/wasm.md`. The export lists are handwritten in
`src/cpp/lib/CMakeLists.txt`.

There is no browser-callable model-bounds function and no generic
operation/attachment C ABI today.

## CLI operation inventory

Direct commands include generic model bounds, HLR JSON/SVG, model-to-GLB,
planar STEP, batch request initialization/execution, planar batch solve, and
compatibility STEP-named commands.

The `geometer.batch.request.a0` dispatcher accepts these operation strings:

- canonical current names: `model_bounds_json`,
  `model_hlr_projection_json`, `model_hlr_projection_svg`, `model_to_glb`, and
  `planar_step`;
- compatibility names: `step_hlr_projection_json`,
  `step_hlr_projection_svg`, and `step_to_glb`.

Generic model jobs accept `model_path`; `step_path` remains a compatibility
input. The file-based `run` command is the production Python backend.

There is no `serve --stdio` executable mode today.

## Python public surface

The `geometer` package publicly exports version and executable discovery,
single and batch execution, generic model bounds/HLR/GLB functions, STEP
compatibility wrappers, planar STEP and planar batch solve functions,
convenience result/option dataclasses, and `GeometerError`.

The package currently builds dictionaries and decodes JSON with the Python
standard library. `HlrProjectionResult`, `ModelBoundsResult`, and
`PlanarBatchSolveResult` retain underlying dictionaries. The executable backend
and public wrapper behavior must remain compatible while generated models and
strict codecs are introduced.

## Packed packets

All current packed packets are little-endian and separately governed:

| Operation | Request magic/version | Response magic/version |
| --- | --- | --- |
| Planar batch solve | `GMPBRQ01` / 2 | `GMPBRS01` / 2 |
| Planar triangulate | `GMTRRQ01` / 1 | `GMTRRS01` / 1 |
| Clipper2 boolean | `GMC2BQ01` / 1 | `GMC2BS01` / 1 |
| Clipper2 inflate-open | `GMC2IQ01` / 1 | `GMC2IS01` / 1 |

The TypeSpec program may model the logical inputs and outputs and the catalog
may carry reviewed packet metadata. TypeSpec JSON structure does not replace
these byte-layout authorities.

## Maintained browser consumers

- `examples/wasm/embedded_model_viewer.html` with
  `hlr_projection_worker.js` owns the HLR/model-viewer migration.
- `examples/wasm/planar_ring_solver_demo.html` owns the planar batch/planar
  operation migration.
- `tests/wasm/` contains validation and benchmark harnesses, not maintained
  user-facing demos.

The model-bounds pilot must add a small TypeScript example because no current
maintained demo exercises that operation.

## Named downstream compatibility consumer

`C:/eli/wn-hw/appz/viz` currently pins `wn-geometer==2026.6.10` and vendors the
Geometer 2026.6.10 full and planar browser artifacts. Its JavaScript directly
uses the Emscripten factories, runtime/memory helpers, STEP-to-GLB byte call,
planar batch solve, triangulation, Clipper2 byte calls, version/ABI checks, and
Geometer free functions. It also encodes the current packed planar formats.

The frozen requirements are recorded in
`compatibility/viz-2026.6.10.toml`. They remain a release compatibility lane
until Viz's planned TypeScript upgrade adopts `@wavenumber/geometer` and
provides a replacement snapshot. Additive generated interfaces must not remove
or rename the legacy lane during that migration.
