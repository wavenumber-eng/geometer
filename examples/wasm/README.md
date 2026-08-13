# WASM Examples

## Generated Model Bounds Client

`model_bounds_demo.html` is the first TypeScript/generated-contract example.
It loads a STEP fixture through `@wavenumber/geometer/wasm`, computes bounds
through the generic ABI, and renders the authoritative result as a translucent
volume, wireframe, dimension lines, and an inspector around an interactive
Three.js model. The prepared GLB for the same fixture is a display companion;
the box and every reported number come from the STEP/OCCT result. The
TypeScript source contains no pointer or Emscripten heap management.

The page projects the Viz 3D visual language into a focused demo stylesheet,
uses the repository-vendored Cousine font and Wavenumber watermark, and pins
the same Three.js 0.161.0 browser import used by the existing embedded-model
viewer. Serving the page requires network access to that pinned Three.js CDN
module.

Build the package and example, serve the repository root, then open it:

```powershell
npm run generate:contracts
python -m http.server 8123 --bind 127.0.0.1
```

`http://127.0.0.1:8123/examples/wasm/model_bounds_demo.html`

HLR and planar examples remain on their existing JavaScript surfaces until
their respective operation contracts are promoted.

## Embedded Model Viewer

`embedded_model_viewer.html` is the browser reference for the high-level
Geometer workflow. It:

- load a prepared GLB model into an interactive Three.js 3D pane;
- run STEP HLR through the browser WASM worker;
- draw detail, outline, and optional bounding-box projection geometry in the adjacent SVG pane;
- expose view, algorithm, outline, mesh, color, and edge-category controls.

Run it from the repository root:

```powershell
python -m http.server 8123 --bind 127.0.0.1
```

`http://127.0.0.1:8123/examples/wasm/embedded_model_viewer.html`

Build or refresh the WASM outputs first when needed:

```powershell
python scripts\build_wasm.py
```

Release/demo review uses a standalone one-file copy:

```powershell
python scripts\build_self_contained_hlr_demo.py
```

Open `dist/wasm/demos/hlr_demo.html` directly from disk. The generated file
embeds the Geometer browser WASM, the demo model assets, the Worker body,
Three.js viewer runtime, and the watermark.

The page uses the embedded model fixtures under `tests/fixtures/`.
Refresh those fixtures with:

```powershell
powershell -NoProfile -ExecutionPolicy Bypass -File scripts\prepare_embedded_model_fixtures.ps1
```

Test-only browser pages remain under `tests/wasm/`, including
`hlr_benchmark.html` and `browser_hlr_validation.html`.

## Planar Ring Solver Demo

`planar_ring_solver_demo.html` demonstrates the planar batch solver JSON ring
output. It shows draggable subject/subtract rings, curved source shapes,
solver JSON, and the resulting SVG rings.

`http://127.0.0.1:8123/examples/wasm/planar_ring_solver_demo.html`

After building `dist/wasm/planar-browser`, bake a standalone demo with:

```powershell
python scripts\build_self_contained_planar_ring_solver_demo.py
```

Open `dist/wasm/demos/planar_ring_solver_demo.html` directly from disk. The
generated file embeds the planar WASM runtime and watermark.
