# Python Examples

Build the native CLI first:

```powershell
cmake --build ..\..\build --target geometer --config Release
```

Run the PyVista example with `uv` to create an isolated example environment from
the checked-in lock file. This is the preferred 3D viewer experiment: it uses a
real VTK/PyVista viewport on the left and Geometer HLR on the right.

```powershell
uv run --project examples\python python examples\python\pyvista_hlr_viewer.py tests\fixtures\step\embedded_models\SOT-23.STEP
```

The PyVista viewer preserves GLB material colors, hides triangle mesh edges by
default, can overlay extracted feature edges, and always projects HLR from the
current 3D camera. The ISO/Top/Bottom/Front/Back/Left/Right buttons are camera
presets. Lighting controls adjust key direction, key/fill/head intensity,
ambient level, and material contrast. The projection regenerates after camera
movement settles.

The older Dear PyGui viewer is still available as a lightweight fallback:

```powershell
uv run --project examples\python python examples\python\hlr_viewer.py tests\fixtures\step\embedded_models\SOT-23.STEP
```

Or install the demo dependencies into your current environment:

```powershell
python -m pip install dearpygui numpy trimesh pyvista pyvistaqt pyside6
python examples\python\pyvista_hlr_viewer.py tests\fixtures\step\embedded_models\SOT-23.STEP
```

Run a non-GUI projection smoke check:

```powershell
uv run --project examples\python python examples\python\hlr_viewer.py --project-once tests\fixtures\step\embedded_models\SOT-23.STEP
```

The headless mode can also write the high-level API outputs:

```powershell
uv run --project examples\python python examples\python\hlr_viewer.py --project-once tests\fixtures\step\embedded_models\SOT-23.STEP --json-out out\projection.json --glb-out out\preview.glb
```

The PyVista viewer has an off-screen smoke path for dependency and GLB-preview
checks:

```powershell
uv run --project examples\python python examples\python\pyvista_hlr_viewer.py --off-screen-smoke tests\fixtures\step\embedded_models\SOT-23.STEP --screenshot out\pyvista-preview.png
```

The examples import the checkout package from `../../python` when run from this
repository, so they do not need the package installed into the active
environment. Outside a source checkout, install `wn-geometer==2026.5.23` and
run the same scripts against the installed `geometer` package.
