# Geometer

Focused C++ geometry library and CLI built on OCCT. Provides a small, controllable interface for operations like STEP-to-GLB conversion, HLR projection, planar boolean/offset solving, and polygon extrusion/meshing.

## Interfaces

Current C++, C ABI, WASM, and CLI interfaces are documented in
[INTERFACES.md](INTERFACES.md).

## Build

```bash
cmake --preset default
cmake --build build --config Release
```

On first configure, CMake automatically builds OCCT from source (~10-15 min one-time cost). Subsequent configures are instant.

To manually rebuild OCCT (e.g., after version bump or to clean):

```bash
python scripts/build_occt.py --clean
python scripts/build_occt.py
```

## WASM Build

```bash
python scripts/build_wasm.py
```

This installs emsdk, cross-compiles OCCT, and builds geometer for WASM. First run takes ~20-30 min. Outputs land in `dist/`.

The WASM build produces two targets:

- `dist/geometer.js` + `dist/geometer.wasm` - Node CLI parity target.
- `dist/geometer-browser.js` + `dist/geometer-browser.wasm` - modular browser/Web Worker target exporting the flat C ABI, including STEP bytes to GLB bytes and packed planar batch solve bytes.

`dist/` is the committed distribution directory. `.deps/`, `build/`, and
`build-wasm/` are local generated state and are not committed.

## Usage

Native:

```bash
geometer --version
geometer step-to-glb input.step output.glb
geometer step-to-glb input.step output.glb --deflection 0.05 --angular 0.3
geometer step-project-hlr input.step output.json
geometer step-project-svg input.step output.svg --mode simple --view top
geometer step-project-svg input.step output.svg --mode detail --curve-mode native-arcs
```

WASM (via Node.js):

```bash
node dist/geometer.js step-to-glb input.step output.glb
```

Embedded model browser viewer:

```bash
powershell -NoProfile -ExecutionPolicy Bypass -File scripts/prepare_embedded_model_fixtures.ps1
python -m http.server 8123 --bind 127.0.0.1
```

Open `http://127.0.0.1:8123/tests/wasm/embedded_model_viewer.html`.

HLR benchmark page:

`http://127.0.0.1:8123/tests/wasm/hlr_benchmark.html`

## Dependencies

- [OpenCASCADE Technology](https://dev.opencascade.org/) (V7_8_1) - built from source automatically.
- [RapidJSON](https://github.com/Tencent/rapidjson) (v1.1.0, header-only) - vendored in `third_party/rapidjson` for OCCT glTF export.
- [Clipper2](https://github.com/AngusJohnson/Clipper2) (2.0.1) - vendored in `third_party/clipper2` for planar polygon boolean and offset operations.
- Python 3 - needed by `scripts/build_occt.py` (invoked by CMake on first configure).

## Project structure

- `src/cpp/lib/` - libgeometer, the reusable C++ core.
- `src/cpp/cli/` - geometer CLI executable.
- `src/js/` - JavaScript/WASM code.
- `third_party/` - small vendored source dependencies used by the build.
- `tests/` - rack-based stratified test system.
- `docs/adr/` - architecture decision records.
- `docs/requirements/` - requirements.
- `docs/plans/` - work plans.
- `scripts/` - build and tooling scripts (uv-managed Python).
