# ADR-001: OCCT built as standalone project, auto-triggered by CMake

## Status

Accepted (revised)

## Context

Geometer depends on OCCT, a large C++ library with its own CMake build. We need a dependency strategy that works across PC/Mac/Linux and eventually Emscripten (WASM).

Options considered:
- Git submodules — rejected (developer friction, user preference).
- Zephyr West manifest — rejected (wrong tool for general C++ projects).
- vcpkg / Conan — rejected (adds external tool dependency).
- CMake FetchContent — attempted first, does not work. OCCT's CMakeLists.txt references `CMAKE_SOURCE_DIR` internally for its own cmake modules, which resolves to geometer's root when OCCT is added as a subdirectory. This causes immediate configure failure.
- Standalone pre-build with find_package — adopted.

## Decision

OCCT is built as a standalone CMake project via `scripts/build_occt.py`, which clones the source, configures, builds, and installs to `.deps/occt-install/`. Geometer's CMakeLists.txt uses `find_package(OpenCASCADE)` to locate the installed artifacts.

If OCCT is not found at configure time, CMake automatically invokes `build_occt.py`. This makes the first `cmake --preset default` self-bootstrapping — no separate manual step required.

The script can also be run manually for clean rebuilds or alternate configurations.

## Dependencies managed by build_occt.py

- **OCCT** (V7_8_1) — cloned shallow from GitHub, built as static libraries.
- **RapidJSON** (v1.1.0, header-only) — required by OCCT's `RWGltf_CafWriter` for glTF/GLB export.

## OCCT build options

The script disables modules and features not needed by geometer:
- `BUILD_MODULE_Draw=OFF`
- `BUILD_MODULE_Visualization=OFF` (TKService/TKV3d are still built because TKXCAF depends on them)
- `BUILD_MODULE_ApplicationFramework=OFF`
- `USE_FREETYPE=OFF`, `USE_TBB=OFF`, `USE_FREEIMAGE=OFF`, `USE_OPENVR=OFF`
- `USE_RAPIDJSON=ON`

## Rationale

- Single command build: `cmake --preset default && cmake --build build --config Release`.
- OCCT version is pinned in one place (`scripts/build_occt.py`).
- `.deps/` is gitignored and survives build directory wipes.
- The same approach extends to Emscripten — `build_occt.py` could accept a `--toolchain` flag.
- No extra package manager tools to install.

## Consequences

- First configure takes ~10-15 minutes (OCCT build). Subsequent configures are instant.
- OCCT source clone is ~80MB (shallow).
- `.deps/` directory holds ~500MB+ of build artifacts on disk.
- Updating OCCT version requires changing the tag in `build_occt.py` and re-running the build.
