# Geometer

Focused C++ geometry library and CLI built on OCCT. Provides a small, controllable interface for operations like STEP-to-GLB conversion, HLR projection, planar boolean/offset solving, and polygon extrusion/meshing.

## Interfaces

Current C++, C ABI, Python, WASM, and CLI interfaces are documented in
[INTERFACES.md](INTERFACES.md).

## Build

```bash
cmake --preset default
cmake --build build --config Release
```

On first configure, CMake automatically builds OCCT from source (~10-15 min one-time cost). Subsequent configures are instant.

On Linux, WSL2, or macOS, this helper runs the native build, CLI smoke, Python
wrapper smoke, and CTest:

```bash
bash scripts/native_posix_smoke.sh
```

To manually rebuild OCCT (e.g., after version bump or to clean):

```bash
python scripts/build_occt.py --clean
python scripts/build_occt.py
```

The Python package is executable-backed: bundle a platform
`geometer` CLI and have Python call it through a subprocess. That keeps the CLI
useful on its own and avoids native library loading issues in Python. The CLI
now has a JSON batch command so repeated STEP operations can run in one process.

## WASM Build

```bash
python scripts/build_wasm.py
```

This installs emsdk, cross-compiles OCCT, and builds geometer for WASM. First
run takes ~20-30 min. Outputs land in `dist/wasm/` with flat compatibility
copies at the root of `dist/`.

The WASM build produces three targets:

- `dist/wasm/browser/geometer.js` + `dist/wasm/browser/geometer.wasm` -
  official browser/Web Worker
  integration target. This is the full OCCT-backed build exporting
  `createGeometerModule` and the flat C ABI, including STEP bytes to GLB bytes,
  HLR projection, and planar byte APIs.
- `dist/wasm/node-test/geometer-node-test.js` +
  `dist/wasm/node-test/geometer-node-test.wasm` - Node CLI parity/test target
  with filesystem access. Use this for tests and diagnostics, not browser
  integration.
- `dist/wasm/planar-browser/geometer-planar-browser.js` +
  `dist/wasm/planar-browser/geometer-planar-browser.wasm` -
  smaller optional browser/Web Worker target exporting `createGeometerPlanarModule`
  and the planar byte APIs only. It exists so planar-only consumers can avoid
  paying the full OCCT/STEP WASM size, startup, and worker-memory cost.

Native builds copy platform-specific artifacts under
`dist/native/<platform>/`, using names such as `windows-x64`, `linux-x64`,
`macos-x64`, and `macos-arm64`. Flat files at the root of `dist/` are retained
as compatibility aliases during the transition to the grouped layout.

`dist/` is the committed distribution directory. `.deps/`, `build/`,
`build-native/`, and `build-wasm/` are local generated state and are not
committed.

## Usage

Python distribution name: `wn-geometer`.

Python import package name: `geometer`.

Install the released Windows x64 wheel from PyPI:

```bash
python -m pip install wn-geometer==2026.5.23
```

Linux and macOS wheels are planned after the Windows release path is stable.

Native:

```bash
geometer --version
geometer step-to-glb input.step output.glb
geometer step-to-glb input.step output.glb --deflection 0.05 --angular 0.3
geometer step-project-hlr input.step output.json
geometer step-project-svg input.step output.svg --mode simple --view top
geometer step-project-svg input.step output.svg --mode detail --curve-mode native-arcs
geometer init-request request.json --step input.step --operation step_hlr_projection_json --output output.json
geometer run request.json response.json
```

Batch request JSON accepts a top-level `options` object for settings shared by
all jobs. A job-level `options` object overrides those defaults for that job.

WASM (via Node.js):

```bash
node dist/wasm/node-test/geometer-node-test.js step-to-glb input.step output.glb
```

Python from a source checkout:

```bash
cmake --build build --target geometer --config Release
python -m pytest tests/python
```

```python
from pathlib import Path
import geometer

version = geometer.version()
projection = geometer.project_step_hlr(
    Path("part.step"),
    views=[geometer.ProjectionView.top()],
)
glb_bytes = geometer.step_to_glb(Path("part.step"))

response = geometer.run_batch(
    [
        {
            "id": "part-top",
            "operation": "step_hlr_projection_json",
            "step_path": "part.step",
            "output_path": "part.top.projection.json",
        }
    ],
    options={"curve_mode": "polyline"},
)

runner = geometer.GeometerBatchRunner(max_workers=8, chunk_size=5)
runner_version = runner.version()
response = runner.run(
    [
        {
            "id": "part-top",
            "operation": "step_hlr_projection_json",
            "step_path": "part.step",
            "output_path": "part.top.projection.json",
        }
    ],
    options={"curve_mode": "polyline"},
)
```

Python uses the bundled wheel executable or source-checkout
`dist/native/<platform>/geometer(.exe)` by default through the CLI JSON batch
mode. The legacy flat `dist/geometer.exe` / `dist/geometer` path is still
accepted. Set `GEOMETER_EXE` to override the executable path. The Python package
intentionally uses the executable backend only for now.

Build a local Python wheel after building the native CLI. Keep Python package
artifacts under `out/wheelhouse`; repo `dist/` is reserved for Geometer runtime
artifacts:

```bash
python -m build --wheel --outdir out/wheelhouse
python -m twine check out/wheelhouse/*.whl
```

Embedded model browser viewer:

```bash
powershell -NoProfile -ExecutionPolicy Bypass -File scripts/prepare_embedded_model_fixtures.ps1
python -m http.server 8123 --bind 127.0.0.1
```

Open `http://127.0.0.1:8123/tests/wasm/embedded_model_viewer.html`.

HLR benchmark page:

`http://127.0.0.1:8123/tests/wasm/hlr_benchmark.html`

Examples:

- `examples/python/pyvista_hlr_viewer.py` - PyVista/Qt STEP 3D + HLR preview.
- `examples/python/hlr_viewer.py` - Dear PyGui STEP HLR fallback viewer.
- `examples/cpp/` - native Dear ImGui + SDL3 + OpenGL HLR preview.
- `examples/wasm/` - browser/WASM example home, with current test-backed pages
  still under `tests/wasm/`.

## Dependencies

- [OpenCASCADE Technology](https://dev.opencascade.org/) (V7_8_1) - built from source automatically.
- [RapidJSON](https://github.com/Tencent/rapidjson) (v1.1.0, header-only) - vendored in `third_party/rapidjson` for OCCT glTF export.
- [Clipper2](https://github.com/AngusJohnson/Clipper2) (2.0.1) - vendored in `third_party/clipper2` for planar polygon boolean and offset operations.
- Python 3 - needed by `scripts/build_occt.py` (invoked by CMake on first configure).

## Project structure

- `src/cpp/lib/` - libgeometer, the reusable C++ core.
- `src/cpp/cli/` - geometer CLI executable.
- `python/geometer/` - Python package using the native CLI by default.
- `examples/` - user-facing Python and browser/WASM examples.
- `third_party/` - small vendored source dependencies used by the build.
- `tests/` - rack-based stratified test system.
- `docs/adr/` - architecture decision records.
- `docs/requirements/` - requirements.
- `docs/plans/` - work plans.
- `scripts/` - build and tooling scripts (uv-managed Python).
