# ADR-001: OCCT built as standalone project, auto-triggered by CMake

## Status

Accepted (revised)

## Context

Geometer depends on OCCT, a large C++ library with its own CMake build. We need
a dependency strategy that works across PC, Mac, Linux, and Emscripten.

Options considered:

- Git submodules: rejected because they add developer friction.
- Zephyr West manifest: rejected because it is the wrong tool for this C++
  project.
- vcpkg / Conan: rejected because they add an external package manager
  dependency.
- CMake FetchContent: attempted first, does not work. OCCT's CMakeLists.txt
  references `CMAKE_SOURCE_DIR` internally for its own cmake modules, which
  resolves to Geometer's root when OCCT is added as a subdirectory. This causes
  immediate configure failure.
- Standalone pre-build with `find_package`: adopted.

## Decision

OCCT is built as a standalone CMake project via `scripts/build_occt.py`, which
clones the source, configures, builds, and installs to `.deps/occt-install/`.
Geometer's CMakeLists.txt uses `find_package(OpenCASCADE)` to locate the
installed artifacts.

If OCCT is not found at configure time, CMake automatically invokes
`build_occt.py`. This makes the first `cmake --preset default`
self-bootstrapping with no separate manual step required.

The OCCT build uses vendored RapidJSON from `third_party/rapidjson` for
glTF/GLB export support.

The script can also be run manually for clean rebuilds or alternate
configurations.

## Dependency Sources

- **OCCT** (V7_8_1): cloned shallow from GitHub, built as static libraries.
- **RapidJSON** (v1.1.0, header-only): vendored under
  `third_party/rapidjson`, required by OCCT's `RWGltf_CafWriter` for glTF/GLB
  export.

## OCCT Build Options

The script disables modules and features not needed by Geometer:

- `BUILD_MODULE_Draw=OFF`
- `BUILD_MODULE_Visualization=OFF` (TKService/TKV3d are still built because
  TKXCAF depends on them)
- `BUILD_MODULE_ApplicationFramework=OFF`
- `USE_FREETYPE=OFF`, `USE_TBB=OFF`, `USE_FREEIMAGE=OFF`, `USE_OPENVR=OFF`
- `USE_RAPIDJSON=ON`

## Rationale

- Single command build: `cmake --preset default && cmake --build build --config Release`.
- OCCT version is pinned in one place (`scripts/build_occt.py`).
- RapidJSON is local and checked in, avoiding a second generated dependency
  checkout.
- `.deps/` is gitignored and survives build directory wipes.
- The same approach extends to Emscripten.
- No extra package manager tools are required.

## Consequences

- First configure takes about 10-15 minutes because OCCT must build. Subsequent
  configures are fast.
- OCCT source clone is about 80 MB when shallow.
- `.deps/` holds large generated build artifacts.
- Updating OCCT requires changing the tag in `build_occt.py` and rebuilding.
- Updating RapidJSON requires an intentional vendored source refresh under
  `third_party/rapidjson`.
