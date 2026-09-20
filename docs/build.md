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

## Release Automation

The manual `Build Release Candidate` workflow builds Windows x64, Linux x64,
Linux ARM64, macOS ARM64, all platform wheels and static SDKs, and WASM once
from its exact `main` revision. Its attested flat `qualified-release` artifact
is the sole product input to publication. After the matching immutable tag is
created, `Promote Release Candidate` accepts only the successful candidate run
ID, verifies the inventory and tag, and publishes those exact existing bytes;
promotion performs no build or packaging work.

R2 stores only the immutable OCCT dependency archives selected by
`dependencies/occt-lock.json`. It is not a product-candidate or release
channel. Missing or mismatched locked OCCT objects fail closed instead of
starting an implicit source build. Workflow-only and documentation-only
changes do not require a package version bump. See the developer guide's
[build and release model](developer/README.md#build-and-release-model) and the
[CI strategy](developer/ci-strategy.md) for commands, credentials, retention,
and recovery behavior.

## Validation And Signoff

Before release, run Rack, CTest, Ruff, Pyright, the uv lock check, package
validation, browser/WASM parity, and the L99 signoff suite as documented in the
developer guide and [test strategy](test-strategy.html).
