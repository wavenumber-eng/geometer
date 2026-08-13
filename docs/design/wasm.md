# WASM Interfaces

`scripts/build_wasm.py` builds three WASM targets into `dist/`.

Full Browser/Web Worker target:

- `dist/wasm/browser/geometer.js`
- `dist/wasm/browser/geometer.wasm`

This is the official application integration target. It is modularized with
the factory name `createGeometerModule` and includes the OCCT-backed STEP/HLR/GLB
APIs plus the planar byte APIs.

Node CLI parity/test target:

- `dist/wasm/node-test/geometer-node-test.js`
- `dist/wasm/node-test/geometer-node-test.wasm`

This target uses Node filesystem access for command-line parity and diagnostics.
Do not use it for browser integration.

Planar-only Browser/Web Worker target:

- `dist/wasm/planar-browser/geometer-planar-browser.js`
- `dist/wasm/planar-browser/geometer-planar-browser.wasm`

The full browser target exports:

- `_malloc`
- `_free`
- `_geometer_version_string`
- `_geometer_version_major`
- `_geometer_version_minor`
- `_geometer_version_patch`
- `_geometer_abi_version`
- `_geometer_step_hlr_projection_json`
- `_geometer_step_hlr_projection_json_bytes`
- `_geometer_step_to_glb`
- `_geometer_step_to_glb_bytes`
- `_geometer_planar_batch_solve`
- `_geometer_planar_batch_solve_bytes`
- `_geometer_planar_batch_solve_json`
- `_geometer_planar_batch_solve_json_bytes`
- `_geometer_planar_triangulate`
- `_geometer_planar_triangulate_bytes`
- `_geometer_clipper2_boolean`
- `_geometer_clipper2_boolean_bytes`
- `_geometer_clipper2_inflate_open`
- `_geometer_clipper2_inflate_open_bytes`
- `_geometer_operation_catalog_json`
- `_geometer_operation_execute`
- `_geometer_operation_result_json_data`
- `_geometer_operation_result_json_size`
- `_geometer_operation_result_attachment_count`
- `_geometer_operation_result_attachment_name`
- `_geometer_operation_result_attachment_media_type`
- `_geometer_operation_result_attachment_data`
- `_geometer_operation_result_free`
- `_geometer_free_string`
- `_geometer_free_bytes`

It also exports these Emscripten runtime helpers:

- `ccall`
- `cwrap`
- `UTF8ToString`
- `stringToUTF8`
- `lengthBytesUTF8`
- `getValue`

The planar-only browser target is modularized with the factory name
`createGeometerPlanarModule`. It exports only the version/free functions plus:

- `_malloc`
- `_free`
- `_geometer_planar_batch_solve`
- `_geometer_planar_batch_solve_bytes`
- `_geometer_planar_batch_solve_json`
- `_geometer_planar_batch_solve_json_bytes`
- `_geometer_planar_triangulate`
- `_geometer_planar_triangulate_bytes`
- `_geometer_clipper2_boolean`
- `_geometer_clipper2_boolean_bytes`
- `_geometer_clipper2_inflate_open`
- `_geometer_clipper2_inflate_open_bytes`

Use this target for browser workers that only need packed planar geometry
operations and should not pay the full OCCT/STEP WASM size, startup, and
worker-memory cost. The full browser target also exports these planar APIs, so
the planar-only target is an optimization, not a separate semantic API.
The generic operation ABI is intentionally exported only by the full browser
target because the pilot `model_bounds` operation requires OCCT STEP support.
Existing full-browser and planar-only symbols are unchanged.

Minimal browser-worker shape:

```js
importScripts("/dist/wasm/browser/geometer.js");

const module = await createGeometerModule({
  locateFile: (path) => path.endsWith(".wasm") ? `/dist/wasm/browser/${path}` : path,
});

// Allocate STEP bytes and options JSON, then call:
module.ccall(
  "geometer_step_hlr_projection_json_bytes",
  "number",
  ["number", "number", "number", "number", "number"],
  [stepPtr, stepSize, optionsPtr, valueOutPtr, errorOutPtr],
);

module.ccall(
  "geometer_step_to_glb_bytes",
  "number",
  ["number", "number", "number", "number", "number", "number"],
  [stepPtr, stepSize, optionsPtr, valueOutPtr, valueSizeOutPtr, errorOutPtr],
);
```

The browser-worker example lives at `examples/wasm/hlr_projection_worker.js`.
