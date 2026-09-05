# Python Examples

Use the installed `wn-geometer` package or the committed native release
artifact. A source rebuild is needed only when changing native code:

```powershell
cmake --build ..\..\build --target geometer --config Release
```

The examples import the installed `geometer` package. From a source checkout,
run the package validation script to build a local wheel, install it into a
clean temporary environment, and run the headless example:

```powershell
python scripts\validate_python_package.py
```

The headless example can also be run directly in any environment where
`wn-geometer` is installed. It writes `SOT-23.projection.json`, `SOT-23.svg`,
and `SOT-23.glb` under `out\examples` by default:

```powershell
python examples\python\step_hlr_svg.py tests\fixtures\step\embedded_models\SOT-23.STEP --out-dir out\examples
```

Run the PyVista example with `uv` to create an isolated example environment
from the checked-in lock file. This is the preferred 3D viewer experiment: it
uses a real VTK/PyVista viewport on the left and Geometer HLR on the right.

```powershell
uv run --project examples\python python examples\python\pyvista_hlr_viewer.py tests\fixtures\step\embedded_models\SOT-23.STEP
```

The PyVista viewer preserves GLB material colors, hides triangle mesh edges by
default, can overlay extracted feature edges, and always projects HLR from the
current 3D camera. It also computes Geometer model bounds and can overlay the
projected bounds rectangle in the right HLR pane. The ISO/Top/Bottom/Front/Back/
Left/Right buttons are camera presets. Lighting controls adjust key direction,
key/fill/head intensity, ambient level, and material contrast. The projection
regenerates after camera movement settles.

Or install the demo dependencies into your current environment:

```powershell
python -m pip install numpy trimesh pyvista pyvistaqt pyside6
python examples\python\pyvista_hlr_viewer.py tests\fixtures\step\embedded_models\SOT-23.STEP
```

The PyVista viewer has an off-screen validation path for dependency and
GLB-preview checks:

```powershell
uv run --project examples\python python examples\python\pyvista_hlr_viewer.py --off-screen-validate tests\fixtures\step\embedded_models\SOT-23.STEP --screenshot out\pyvista-preview.png
```

Outside a source checkout, install `wn-geometer` and run the same
scripts against the installed `geometer` package.
