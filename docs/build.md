+++
type = "build_doc"
id = "build"
title = "Geometer Build"
status = "accepted"
+++

# Geometer Build

## Tools And Setup

The canonical prerequisites and setup instructions are in
[the developer guide](developer/README.md). Native builds require CMake, Ninja,
a C++17 toolchain, Python, and the generated OCCT dependency under `.deps/`.
WASM builds additionally require Node and the managed Emscripten SDK.

## Commands And Invocation

Run `cmake --preset default`, `cmake --build build --config Release`, and
`ctest --test-dir build -C Release --output-on-failure` for native work. Run
`python scripts/build_wasm.py` for browser and Node outputs, and use `uv build`
for the Python wheel.

## Outputs And Artifacts

Distributable output is grouped under `dist/native/<platform>/` and
`dist/wasm/<target>/`. Release packages and checksums are created with
`scripts/package_release_artifacts.py`; local temporary output remains under
`build*`, `.deps`, and `out`.

## Validation And Signoff

Before release, run Rack, CTest, Ruff, Pyright, the uv lock check, package
validation, browser/WASM parity, and the L99 signoff suite as documented in the
developer guide and [test strategy](test-strategy.html).
