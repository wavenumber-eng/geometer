# WASM Examples

Standalone browser/WASM examples will live here.

## Future Embedded Model Viewer

The current browser reference for the high-level Geometer workflow still lives
under `tests/wasm/embedded_model_viewer.html`. It should be promoted or
refactored into this folder later, after the Python example has the same basic
3D-preview-plus-HLR workflow.

That future example should:

- load a prepared GLB model into an interactive Three.js 3D pane;
- run STEP HLR through the browser WASM worker;
- draw simple/detail projection geometry in the adjacent SVG pane;
- expose view, algorithm, mesh, and edge-category controls.

For now, run the existing test-backed page from the repository root:

```powershell
python -m http.server 8123 --bind 127.0.0.1
```

`http://127.0.0.1:8123/tests/wasm/embedded_model_viewer.html`

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
`hlr_benchmark.html` and `browser_hlr_smoke.html`.
