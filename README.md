# Geometer

Focused C++ geometry library, CLI, Python package, and WASM interface built on
OCCT. Geometer provides generic CAD/kernel operations for STEP-to-GLB
conversion, STEP HLR projection, exact planar STEP synthesis, planar
contouring, and packed planar boolean work.

## Documentation

- [Developer guide](docs/developer/README.md)
- [Setup](docs/setup.html)
- [Architecture](docs/architecture.html)
- [Design and interface docs](docs/design/README.md)
- [Requirements](docs/requirements/README.md)
- [Contracts](docs/contracts/README.md)
- [ADRs](docs/adr/README.md)
- [Release notes](docs/releases/README.md)
- [Examples](examples/README.md)

## Build And Validate

```bash
uv sync --group dev
cmake --preset default
cmake --build build --config Release
uv run --group dev rack run --all
uv run pytest tests/L99_release -q
uv run python scripts/validate_native.py
uv run python scripts/validate_python_package.py
```

Native artifacts are copied to `dist/native/<platform>/`. Root-level
`dist/geometer*` artifacts are intentionally not produced.

OCCT is generated dependency state under `.deps/`. Build scripts first try
verified public prebuilt OCCT archives from
`https://artifacts.wavenumber.net` before falling back to source builds. R2
credentials are only needed for producer uploads or private fallback testing.

Build WASM artifacts:

```bash
python scripts/build_wasm.py
```

WASM artifacts are copied to:

- `dist/wasm/browser/`
- `dist/wasm/node-test/`
- `dist/wasm/planar-browser/`

Generate the TypeSpec projections, `@wavenumber/geometer` ESM package, and
TypeScript browser example with:

```bash
npm ci
npm run generate:contracts
npm run check:contracts
```

The package artifact is `dist/npm/geometer/`. The current repository artifact
is a release input; it is not a claim that the package has been published to
npm.

## Python Package

PyPI distribution: `wn-geometer`

Import package: `geometer`

Install the current release:

```bash
python -m pip install wn-geometer==2026.6.23
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
step_bytes = geometer.planar_step(
    {
        "schema": "geometry.planar_step.request.a0",
        "units": "mm",
        "bodies": [
            {
                "id": "copper",
                "thickness_mm": 0.035,
                "regions": [
                    {
                        "outer": {
                            "points": [[0, 0], [10, 0], [10, 5], [0, 5]],
                            "segments": [{"kind": "line"}] * 4,
                        }
                    }
                ],
            }
        ],
    }
)
geometer.write_planar_step(
    {
        "schema": "geometry.planar_step.request.a0",
        "units": "mm",
        "bodies": [
            {
                "id": "copper",
                "thickness_mm": 0.035,
                "fuse_regions": True,
                "regions": [
                    {
                        "outer": {
                            "points": [[0, 0], [10, 0], [10, 5], [0, 5]],
                            "segments": [{"kind": "line"}] * 4,
                        }
                    }
                ],
            }
        ],
    },
    "layer.step",
)
```

The package is executable-backed. Wheels bundle the platform executable under
`geometer/native/<platform>/`, expose a `geometer` console command in the
install environment, and call the executable through the JSON batch CLI.

## CLI

```bash
geometer --version
geometer step-to-glb input.step output.glb
geometer step-project-hlr input.step output.json
geometer step-project-svg input.step output.svg --mode outline --view top
geometer planar-step planar-step-request.json output.step
geometer init-request request.json --step input.step --operation step_hlr_projection_json --output output.json
geometer run request.json response.json
```

## Examples

- `examples/python/step_hlr_svg.py` - no-GUI package example that writes HLR
  projection JSON, SVG, and GLB outputs.
- `examples/python/pyvista_hlr_viewer.py` - PyVista/Qt STEP 3D + HLR preview.
- `examples/wasm/embedded_model_viewer.html` - browser viewer using prepared GLB
  fixtures and the WASM HLR worker.
- `examples/wasm/model_bounds_demo.html` - TypeScript/generated-client pilot
  that computes and visualizes STEP model bounds through browser WASM.
- `dist/wasm/demos/hlr_demo.html` and
  `dist/wasm/demos/planar_ring_solver_demo.html` - one-file standalone browser
  demos for release review.
- `examples/cpp/` - native Dear ImGui + SDL3 + OpenGL HLR preview.

Serve browser examples from the repo root:

```bash
python -m http.server 8123 --bind 127.0.0.1
```

Open `http://127.0.0.1:8123/examples/wasm/embedded_model_viewer.html`.

The `dist/wasm/demos/*.html` files can also be opened directly from disk after
running `python scripts/build_wasm.py` and the demo bake scripts.

## Release

Geometer uses date-based releases per ADR 006:

- Git tag: `vYYYY-MM-DD`
- PyPI/CMake version: `YYYY.M.D`
- C ABI generation: `YYYYMMDD`

Same-day follow-up releases append a serial to the tag and package version, for
example `v2026-05-24-2` and `2026.5.24.2`. The C ABI generation stays at
`YYYYMMDD` unless the C ABI generation itself changes.

Before tagging, run the L99 release gate plus native and package validation:

```bash
uv sync --group dev
uv run pytest tests/L99_release -q
uv run python scripts/validate_native.py
uv run python scripts/validate_python_package.py
```

The repository declares the `python-native-wasm` Wavenumber development
standards profile. Lightweight CI runs the L99 gate on Ubuntu, Windows, and
macOS. Full native/WASM rebuilds remain explicit validation steps because fresh
OCCT and Emscripten dependency builds are expensive.

## License

MIT. See [LICENSE](LICENSE).
