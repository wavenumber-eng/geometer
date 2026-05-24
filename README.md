# Geometer

Focused C++ geometry library, CLI, Python package, and WASM interface built on
OCCT. Geometer provides generic CAD/kernel operations for STEP-to-GLB
conversion, STEP HLR projection, planar contouring, and packed planar boolean
work.

## Documentation

- [Developer guide](docs/developer/README.md)
- [Design and interface docs](docs/design/README.md)
- [Requirements](docs/requirements/README.md)
- [ADRs](docs/adr/README.md)
- [Examples](examples/README.md)

## Build And Validate

```bash
cmake --preset default
cmake --build build --config Release
python scripts/validate_native.py
python scripts/validate_python_package.py
```

Native artifacts are copied to `dist/native/<platform>/`. Root-level
`dist/geometer*` artifacts are intentionally not produced.

Build WASM artifacts:

```bash
python scripts/build_wasm.py
```

WASM artifacts are copied to:

- `dist/wasm/browser/`
- `dist/wasm/node-test/`
- `dist/wasm/planar-browser/`

## Python Package

PyPI distribution: `wn-geometer`

Import package: `geometer`

Install the current release:

```bash
python -m pip install wn-geometer==2026.5.24
```

Basic Python use:

```python
from pathlib import Path
import geometer

version = geometer.version()
projection = geometer.project_step_hlr(
    Path("part.step"),
    views=[geometer.ProjectionView.top()],
)
glb_bytes = geometer.step_to_glb(Path("part.step"))
```

The package is executable-backed. Wheels bundle the platform executable under
`geometer/native/<platform>/` and call it through the JSON batch CLI.

## CLI

```bash
geometer --version
geometer step-to-glb input.step output.glb
geometer step-project-hlr input.step output.json
geometer step-project-svg input.step output.svg --mode simple --view top
geometer init-request request.json --step input.step --operation step_hlr_projection_json --output output.json
geometer run request.json response.json
```

## Examples

- `examples/python/step_hlr_svg.py` - no-GUI package example that writes HLR
  projection JSON, SVG, and GLB outputs.
- `examples/python/pyvista_hlr_viewer.py` - PyVista/Qt STEP 3D + HLR preview.
- `examples/wasm/embedded_model_viewer.html` - browser viewer using prepared GLB
  fixtures and the WASM HLR worker.
- `examples/cpp/` - native Dear ImGui + SDL3 + OpenGL HLR preview.

Serve browser examples from the repo root:

```bash
python -m http.server 8123 --bind 127.0.0.1
```

Open `http://127.0.0.1:8123/examples/wasm/embedded_model_viewer.html`.

## Release

Geometer uses date-based releases per ADR 006:

- Git tag: `vYYYY-MM-DD`
- PyPI/CMake version: `YYYY.M.D`
- C ABI generation: `YYYYMMDD`

Before tagging, run the L99 release gate plus native and package validation:

```bash
python -m pytest tests/L99_release -q
python scripts/validate_native.py
python scripts/validate_python_package.py
```
