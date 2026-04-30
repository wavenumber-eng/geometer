# Plan 002: Browser HLR And Mesh Interfaces

## Goal

Turn geometer into the geometry kernel used by browser and Python tooling for
STEP projection and tessellated model assets, while keeping the package generic
enough for non-Altium projects.

## Status

Implemented for the v0.1.0 HLR/browser milestone.

Completed:

- Native C++ value API for STEP-byte HLR projection.
- C ABI for byte-buffer HLR projection.
- Browser/Web Worker WASM target with direct byte-buffer input.
- CLI JSON/SVG projection outputs.
- C++ planar contour module for simple projection output.
- Browser viewer and benchmark pages for embedded STEP/GLB fixtures.
- Version API and WASM version exports for consumers.

Still future work:

- Direct typed-array STEP tessellation/mesh API.
- Python package/wrapper around the C ABI.
- Downstream replacement of altium-cruncher/viz projection internals.

## API Shape

The core library should expose three layers:

1. C++ value API
   - `step_to_glb(...)`
   - `step_hlr_projection_from_bytes(...)`
   - future `step_tessellate_mesh_from_bytes(...)`
2. Stable C ABI
   - string/byte-buffer functions for Python `ctypes`/`cffi`
   - the same ABI can be exported by Emscripten
3. CLI/dev tools
   - `geometer step-to-glb input.step output.glb`
   - `geometer step-project-hlr input.step output.json`
   - `geometer step-project-svg input.step output.svg --mode simple|detail`

## Projection Output

Use a backend-neutral projection contract:

```json
{
  "schema": "geometry.projection.a0",
  "units": "mm",
  "source": {
    "kind": "step",
    "hash": "sha256..."
  },
  "views": [
    {
      "id": "top",
      "direction": [0, 0, 1],
      "up": [0, 1, 0],
      "modes": {
        "simple": {
          "segments": [[0, 0, 1, 0]],
          "arcs": []
        },
        "detail": {
          "segments": [[0, 0, 1, 0]],
          "arcs": [
            {
              "start": [1, 0],
              "end": [0, 1],
              "center": [0, 0],
              "radius": 1,
              "extent_rad": 1.5707963268,
              "ccw": true,
              "full_circle": false
            }
          ]
        }
      }
    }
  ]
}
```

Consumers can derive their own retained command buffers, SVG elements, Three.js
line buffers, or WebGPU storage buffers from this without changing geometer.

## Simple Mode Replacement

The current altium-cruncher simple mode depends on Shapely:

1. round/dedupe projected HLR edge segments
2. build `LineString` objects
3. `unary_union` to node and merge linework
4. `polygonize`
5. optionally `unary_union` polygons
6. emit exterior and hole rings as line segments

Geometer should replace that with a C++ planar contour module:

1. Snap projected coordinates to the requested precision grid.
2. Deduplicate undirected segments.
3. Node segment intersections and overlapping collinear segments.
4. Build a directed half-edge graph.
5. Trace closed faces using angle-sorted outgoing edges.
6. Drop zero-area or invalid rings.
7. Union resulting polygons when requested.
8. Emit exterior and hole rings as deterministic line segments.

For the union step, prefer Clipper2 because it is C++ and WASM-friendly. If
exact parity against Shapely requires GEOS, treat GEOS as a measured fallback,
not the default dependency.

## File Layout

Keep the C++ tree split by responsibility:

```text
src/cpp/lib/geometer.h                 public umbrella header
src/cpp/lib/geometer/status.h          small status/error type
src/cpp/lib/geometer/step_to_glb.h     existing GLB conversion API
src/cpp/lib/geometer/projection.h      HLR projection value API
src/cpp/lib/geometer/c_api.h           stable C ABI for Python/WASM
src/cpp/lib/step_to_glb.cpp            STEP-to-GLB implementation
src/cpp/lib/step_reader.cpp            future STEP bytes/path reader helpers
src/cpp/lib/hlr_projection.cpp         future OCCT HLR implementation
src/cpp/lib/planar_contours.cpp        future simple-mode contour engine
src/cpp/lib/projection_json.cpp        future JSON serialization
src/cpp/lib/projection_svg.cpp         future dev SVG serialization
src/cpp/cli/main.cpp                   CLI routing only
```

No future implementation should grow `geometer.cpp` into a catch-all file.

## WASM Target

Add a browser build target separate from the Node CLI target:

- modularized Emscripten output
- no `NODERAWFS`
- no Node-only environment setting
- direct byte-buffer input from JavaScript
- exported C ABI functions
- intended use from a Web Worker

The existing Node CLI WASM target can stay for command-line parity, but it is
not the browser integration target.

Implemented browser artifacts:

- `dist/geometer-browser.js`
- `dist/geometer-browser.wasm`

Implemented Node CLI WASM artifacts:

- `dist/geometer.js`
- `dist/geometer.wasm`

## Python Target

Use the same C ABI for Python:

- package a platform-specific shared library later
- load with `ctypes` or `cffi`
- return UTF-8 JSON for projection APIs
- return binary buffers for future tessellation APIs

Avoid `pybind11` initially so Python wrapping does not become the primary API
surface or add another build dependency.

## Test Strategy

1. Unit tests for the planar contour module with hand-authored segment cases:
   - closed square
   - square with hole
   - overlapping rectangles
   - crossed/noded linework
   - duplicate reversed segments
2. Golden projection tests using small STEP fixtures committed under
   `tests/fixtures/step/`.
3. Dev SVG snapshots:
   - `*-detail.svg`
   - `*-simple.svg`
4. Parity tests against the current altium-cruncher projection output during
   migration, kept outside geometer if private Altium fixtures are involved.
5. Browser smoke test after the browser WASM target lands:
   - load wasm
   - feed embedded STEP bytes
   - assert JSON parses and contains non-empty simple/detail geometry

## Migration Path

1. Add geometer projection CLI and JSON output.
2. Swap altium-cruncher projection cache internals to call geometer.
3. Keep the existing Python/OCP path as a fallback until parity tests pass.
4. Remove SVG overlay parsing bridge once Canvas consumes geometer projection
   JSON directly.
5. Add browser worker integration for direct projection from embedded STEP
   bytes in `pcb-viz` and `sch-viz`.
6. Add browser-side 3D mesh asset conversion for STEP component models.
