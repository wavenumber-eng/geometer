# Geometer Agent Guide

Geometer is a focused C++17 geometry library and CLI built on OpenCASCADE
Technology (OCCT). It provides generic CAD/kernel operations for STEP conversion,
hidden-line projection, planar contouring, browser WASM workflows, and future
Python tooling.

For build setup, dependency policy, tests, and troubleshooting, read
[DEVELOPMENT.md](DEVELOPMENT.md). Treat that file as the canonical developer
build guide.

For the current C++, C ABI, WASM, and CLI surface, read
[INTERFACES.md](INTERFACES.md).

## Working Rules

- Keep `geometer` generic. Do not add Altium-specific, board-specific, or
  visualizer-specific policy to the core library.
- Keep implementation modules small and responsibility-focused. Do not grow a
  catch-all `geometer.cpp`.
- Prefer direct C++ value APIs in `src/cpp/lib/`, with stable C ABI wrappers for
  Python/WASM boundaries.
- `.deps/` is generated local dependency state and must not be committed.
- `dist/` contains distributable outputs by current project policy.
- Versioned releases use ADR 006 date versions: release tags use
  `vYYYY-MM-DD`, CMake/PyPI use `YYYY.M.D`, and the C ABI generation uses
  `YYYYMMDD`.
- Use CMake for proper native builds and CTest for registered tests.
- The default native preset uses Ninja.
- Format touched C++ files with `clang-format`.

## Build Shortcuts

Native:

```powershell
cmake --preset default
cmake --build build --config Release
ctest --test-dir build -C Release --output-on-failure
```

WASM:

```powershell
python scripts\build_wasm.py
```

This builds both the Node CLI WASM output and the browser-oriented modular C ABI
output.

Manual OCCT rebuild:

```powershell
python scripts\build_occt.py --clean
python scripts\build_occt.py
```
