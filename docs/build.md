+++
type = "build_doc"
id = "geometer-build"
domain = "geometer"
status = "accepted"
title = "Geometer Build Guide"
created = "2026-07-07"
+++

# Geometer Build Guide

Canonical build reference for the native core, the Python package, and the
WASM targets. Deeper background lives in `docs/design/` and
`docs/developer/README.md`.

## Tools and setup

Prerequisites (each dependency is fetched or cached automatically on first
build):

- CMake 3.27+ and Ninja (presets assume Ninja)
- A C++20 compiler (MSVC, clang, or gcc per platform)
- `uv` for the Python package and test tooling
- emsdk (fetched under `deps/` by the WASM setup) for browser builds
- OCCT binaries come from the R2 dependency cache (see
  `docs/developer/r2-dependency-cache.md`)

## Commands

Native build (run from the repository root):

```
cmake --preset release
cmake --build --preset release
```

Python package (editable, with the native library on the path):

```
uv sync
uv run python -c "import geometer; print(geometer.version())"
```

WASM build:

```
scripts/build_wasm.(sh|ps1)
```

## Outputs and artifacts

Build outputs are grouped under `dist/` (see `docs/governance/artifacts.toml`):

- `dist/native/<platform>/` — `geometer` CLI, `geometer_hlr_preview`, and the
  `libgeometer` static library per platform
- `dist/wasm/` — browser and node WASM bundles plus demo pages
- PyPI wheels for the `wn-geometer` package are produced by the release
  workflow (`.github/workflows/release.yml`)

## Validation and signoff

- C++ tests: `ctest --preset release`
- Python tests: `uv run pytest`
- Standards gate: `uvx --from git+https://github.com/wavenumber-eng/wn-dev-std wn-dev-std check .`
- CI (`.github/workflows/ci.yml`) runs the native matrix plus the standards
  gate on every pull request; treat a green run as the signoff baseline.
