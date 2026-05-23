# Geometer Development

This document is the practical setup guide for building, testing, and working on
Geometer from a fresh checkout.

## Project Summary

Geometer is a focused C++17 geometry library and CLI built on OpenCASCADE
Technology (OCCT). Its job is to provide generic CAD/kernel geometry operations
for browser, native CLI, and future Python tooling.

Current and planned library surfaces include:

- STEP to GLB conversion.
- STEP hidden-line projection geometry.
- Planar contour extraction for simplified projected outlines.
- Planar batch boolean/offset solving for filled 2D geometry.
- Future STEP mesh/tessellation APIs for browser rendering.

The core library must stay generic. Do not put board placement rules, Altium
specific names, visualizer policy, or downstream application semantics into
`geometer`.

For the current callable C++, C ABI, WASM, and CLI surfaces, see
[INTERFACES.md](INTERFACES.md).

## Repository Layout

- `src/cpp/lib/` - reusable C++ library code.
- `src/cpp/cli/` - thin CLI wrapper around the library.
- `tests/` - stratified tests and C++ test sources.
- `docs/adr/` - architecture decisions.
- `docs/requirements/` - numbered requirements.
- `docs/plans/` - numbered implementation plans.
- `scripts/` - dependency/build helper scripts.
- `dist/` - distributable binaries and WASM outputs.
- `.deps/` - local generated dependencies and toolchains.

## Dependency Policy

`.deps/` is local generated state and must not be committed. It contains cloned
and built dependencies such as OCCT, emsdk, and OCCT WASM artifacts. It is
intentionally ignored by Git.

`third_party/rapidjson/` is different. RapidJSON is header-only, small enough to
vendor, and required by OCCT's GLB export path. The vendored copy is checked in
so a fresh clone does not need a separate RapidJSON git checkout.

`third_party/clipper2/` is also different. Clipper2 is a compact BSL-1.0 C++
library used by Geometer's generic planar batch solve API. The checked-in copy
contains only the C++ library sources, headers, upstream license, and a local
vendoring note.

`dist/` is different. This repository currently treats `dist/` as the location
for distributable binaries. CMake and WASM builds copy final outputs there.
Those outputs are committed when publishing changes so another project can clone
and use Geometer without a local native/WASM rebuild.

OCCT is not vendored into the repository and is not added with CMake
`FetchContent`, because OCCT uses `CMAKE_SOURCE_DIR` internally and does not work
correctly as a subdirectory dependency. Instead, Geometer builds OCCT as a
standalone project and finds it with `find_package(OpenCASCADE)`.

Pinned dependency versions live in scripts:

- OCCT: `scripts/build_occt.py`
- RapidJSON: `third_party/rapidjson/`
- Clipper2: `third_party/clipper2/`
- emsdk: `scripts/build_wasm.py`

## Workspace Copy Setup

For agent workspaces or downstream monorepo workspaces, prefer copying an
already-prepared Geometer checkout when one is available locally. A prepared
checkout includes:

- `.deps/` with native OCCT state and WASM emsdk/OCCT state.
- `build/` for the native CMake build.
- `build-wasm/` for the Emscripten build.
- `dist/` with the committed/runtime artifacts.

On this development machine, the prepared checkout may live at
`C:\ELI\geometer`. Copying that directory into a sibling workspace preserves the
expensive dependency builds and allows fast API iteration. A fresh clone is still
valid, but the first native/WASM dependency build can take tens of minutes.

After copying into a workspace:

```powershell
git status --short --branch
.\dist\geometer.exe --version
node .\dist\geometer-node-test.js --version
```

Do not run `python scripts\build_occt.py --clean`,
`python scripts\build_wasm.py --clean`, or delete `.deps/` unless intentionally
refreshing dependencies. Those operations remove the cached dependency builds.

## Prerequisites

Native builds require:

- Git.
- Python 3.
- CMake 3.24 or newer.
- Ninja.
- A C++17 compiler toolchain.

On Windows, use a Visual Studio developer environment or another shell where the
selected C++ compiler is available to CMake. The default CMake preset uses
Ninja; in the current Windows setup, LLVM-MinGW `clang`/`clang++` are used.

WASM builds additionally require enough disk space for emsdk and a WASM OCCT
build. The script manages emsdk locally under `.deps/`.

## Native Build

From the repository root:

```powershell
cmake --preset default
cmake --build build --config Release
```

On first configure, CMake looks for OCCT at:

```text
.deps/occt-install/cmake
```

If OCCT is missing, top-level CMake automatically invokes:

```powershell
python scripts\build_occt.py
```

That script uses vendored RapidJSON, clones OCCT, builds OCCT as static
libraries, and installs it into `.deps/occt-install/`. The first run is slow.
Later configures reuse `.deps/` and should be fast.

The vendored RapidJSON v1.1.0 copy includes Geometer's small modern Clang
compatibility patch so OCCT's GLTF toolkit compiles in native and WASM builds.

For Python wheel work, a shared-OCCT Windows build is available as a separate
developer path:

```powershell
python scripts\build_occt.py --library-type Shared
cmake --preset shared-occt
cmake --build build-shared-occt --config Release
ctest --test-dir build-shared-occt -C Release --output-on-failure
python scripts\smoke_python_direct_exit.py --runs 5
```

This installs OCCT DLLs into `.deps/occt-shared-install/`, builds Geometer in
`build-shared-occt/`, and copies `geometer.dll` plus the OCCT `TK*.dll` runtime
set into `dist/`. The default `cmake --preset default` path remains the static
OCCT local build and is the preferred path for the executable-backed Python
package direction.

Build outputs are copied into `dist/` after a successful build.

Common native CLI commands:

```powershell
.\dist\geometer.exe --version
.\dist\geometer.exe step-to-glb input.step output.glb
.\dist\geometer.exe step-project-hlr input.step output.json
.\dist\geometer.exe step-project-svg input.step output.svg --mode simple --view top
.\dist\geometer.exe init-request request.json --step input.step --operation step_hlr_projection_json --output output.json
.\dist\geometer.exe run request.json response.json
.\dist\geometer.exe planar-batch-solve request.bin response.bin --warmup 1 --repeat 5 --metrics metrics.json
```

## Manual OCCT Rebuild

Use this when changing the pinned OCCT version or when the local OCCT build is
suspect:

```powershell
python scripts\build_occt.py --clean
python scripts\build_occt.py
cmake --preset default
cmake --build build --config Release
```

`--clean` removes OCCT state under `.deps/`; it does not remove vendored
RapidJSON or the Geometer `build/` directory.

To rebuild the shared OCCT SDK used by Python wheel/package experiments:

```powershell
python scripts\build_occt.py --library-type Shared
cmake --preset shared-occt
cmake --build build-shared-occt --config Release
```

## WASM Build

From the repository root:

```powershell
python scripts\build_wasm.py
```

This script:

1. Clones and activates pinned emsdk under `.deps/emsdk/`.
2. Uses vendored RapidJSON and reuses or clones OCCT source under `.deps/`.
3. Cross-compiles OCCT to `.deps/occt-wasm-install/`.
4. Builds Geometer in `build-wasm/`.
5. Copies the full browser/Web Worker C ABI outputs `geometer.js` /
   `geometer.wasm` into `dist/`. This is the official application integration
   WASM and includes OCCT-backed STEP/HLR/GLB plus planar byte APIs.
6. Copies the Node CLI parity/test outputs `geometer-node-test.js` /
   `geometer-node-test.wasm` into `dist/`.
7. Copies the planar-only browser C ABI outputs `geometer-planar-browser.js` /
   `geometer-planar-browser.wasm` into `dist/`. This smaller build intentionally
   excludes OCCT/STEP and is retained for planar-only browser workers.

To remove WASM-specific generated state:

```powershell
python scripts\build_wasm.py --clean
```

The Node CLI target uses filesystem access for command-line parity. The full
browser target is modularized and exports the flat C ABI entry points
`geometer_step_hlr_projection_json_bytes` and `geometer_step_to_glb_bytes` for
direct byte-buffer calls from JavaScript or a Web Worker.

The full browser target also exports `geometer_version_string` and
`geometer_abi_version`. Downstream browser consumers should check those before
depending on a specific ABI. Geometer ABI 2 added the planar batch solve byte
entry point used for packed browser geometry offload. ABI 3 adds per-job
planar batch diagnostics to that byte response. ABI 5 adds browser planar
triangulation and Clipper2 byte APIs.

## Versioning

Geometer follows [ADR 006](docs/adr/006_date_based_versioning_policy.md).
The current release identity is `v2026-05-23`; the CMake/PyPI package version
is `2026.5.23`; the C ABI generation is `20260523`.

The root `CMakeLists.txt` declares `GEOMETER_RELEASE_DATE`,
`GEOMETER_RELEASE_VERSION`, and `GEOMETER_ABI_VERSION`. The root
`pyproject.toml` package version must match `GEOMETER_RELEASE_VERSION`.
Generated build metadata must use UTC. Rebuild the persisted `dist/` artifacts
when version or interface values change.

## Embedded Model Viewer

To refresh the copied STEP fixtures, GLB display meshes, and manifest from an
embedded model folder:

```powershell
powershell -NoProfile -ExecutionPolicy Bypass -File scripts\prepare_embedded_model_fixtures.ps1
```

The script copies STEP/STP files into `tests/fixtures/step/embedded_models/`,
converts each one to GLB under `tests/fixtures/glb/embedded_models/`, and writes
`tests/fixtures/embedded_models_manifest.json`.

Serve the repository root and open the viewer:

```powershell
python -m http.server 8123 --bind 127.0.0.1
```

`http://127.0.0.1:8123/tests/wasm/embedded_model_viewer.html`

The viewer loads the GLB for the 3D pane and sends the matching STEP bytes to
the browser WASM HLR API for the projection pane.

The HLR timing page runs the same browser worker projection path across the
fixture set and reports STEP fetch-to-bytes timing separately from HLR timing:

`http://127.0.0.1:8123/tests/wasm/hlr_benchmark.html`

## Tests

After a native CMake build:

```powershell
ctest --test-dir build -C Release --output-on-failure
```

After a WASM build, validate the browser STEP-byte-to-GLB-byte export:

```powershell
node tests\wasm\step_to_glb_bytes_smoke.js
```

To benchmark the browser C ABI planar batch solver against a packed request:

```powershell
node tests\wasm\planar_batch_solve_bytes_benchmark.js request.bin response.bin --warmup 1 --repeat 5 --metrics metrics.json
```

To run the same benchmark in headless Chrome, including the Web Worker path:

```powershell
python tests\wasm\planar_batch_solve_bytes_chrome_benchmark.py request.bin --worker --warmup 1 --repeat 5 --metrics metrics.json
```

The Rack metadata under `tests/` describes test strata, but the C++ tests are
registered through CTest.

Current limitation: the top-level CMake configure always resolves OCCT first.
That means even low-level tests need the native dependency setup when run
through CMake. For isolated low-level work before `.deps/` exists, direct
compile checks are acceptable, but they are not a replacement for the CMake/CTest
path before publishing changes.

Example isolated contour test compile:

```powershell
clang++ -std=c++17 -Isrc\cpp\lib src\cpp\lib\planar_contours.cpp tests\cpp\planar_contours_test.cpp -o dist\geometer_planar_contours_test.exe
.\dist\geometer_planar_contours_test.exe
Remove-Item .\dist\geometer_planar_contours_test.exe
```

## Formatting And Checks

Format touched C++ files with the repository `.clang-format`:

```powershell
clang-format -i <files>
```

Useful lightweight checks:

```powershell
git diff --check
clang++ -std=c++17 -Isrc\cpp\lib -fsyntax-only <files>
```

Run the full native build and CTest path before treating C++ changes as ready.

## Common Troubleshooting

If CMake cannot find OCCT:

```powershell
python scripts\build_occt.py
cmake --preset default
```

If OCCT configure/build state looks stale:

```powershell
python scripts\build_occt.py --clean
python scripts\build_occt.py
```

If CMake cached the wrong dependency path, remove or recreate `build/` and
configure again:

```powershell
Remove-Item -Recurse -Force .\build
cmake --preset default
```

Before deleting generated directories, verify the path is inside this repository.
Never delete `.deps/` or `build/` paths computed from an untrusted variable.
