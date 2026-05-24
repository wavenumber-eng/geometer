# WASM Examples

## Embedded Model Viewer

`embedded_model_viewer.html` is the browser reference for the high-level
Geometer workflow. It:

- load a prepared GLB model into an interactive Three.js 3D pane;
- run STEP HLR through the browser WASM worker;
- draw simple/detail projection geometry in the adjacent SVG pane;
- expose view, algorithm, mesh, and edge-category controls.

Run it from the repository root:

```powershell
python -m http.server 8123 --bind 127.0.0.1
```

`http://127.0.0.1:8123/examples/wasm/embedded_model_viewer.html`

Build or refresh the WASM outputs first when needed:

```powershell
python scripts\build_wasm.py
```

The page uses the embedded model fixtures under `tests/fixtures/`.
Refresh those fixtures with:

```powershell
powershell -NoProfile -ExecutionPolicy Bypass -File scripts\prepare_embedded_model_fixtures.ps1
```

Test-only browser pages remain under `tests/wasm/`, including
`hlr_benchmark.html` and `browser_hlr_validation.html`.
