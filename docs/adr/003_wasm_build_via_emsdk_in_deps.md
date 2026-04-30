# ADR-003: WASM build via emsdk managed in .deps/

## Status

Accepted

## Context

Geometer needs a WASM build for browser-based geometry operations. Emscripten (emsdk) is required to cross-compile OCCT and geometer to WebAssembly.

Options considered:
- Global emsdk install - requires each developer to install and version-match manually.
- emsdk managed in `.deps/` - same self-contained pattern as OCCT.

## Decision

`scripts/build_wasm.py` manages the full WASM toolchain:

1. Clones emsdk into `.deps/emsdk/` and installs a pinned Emscripten version.
2. Cross-compiles OCCT to WASM static libraries (`.deps/occt-wasm-install/`).
3. Cross-compiles geometer against the WASM OCCT build.
4. Copies the Node CLI and browser C ABI WASM outputs to `dist/`.

OCCT source and RapidJSON are shared with the native `build_occt.py` - only one clone needed.

## WASM build specifics

- `NODERAWFS=1` - the WASM CLI uses Node's real filesystem for file I/O.
- `ALLOW_MEMORY_GROWTH=1` - STEP models vary in size.
- `INITIAL_MEMORY=64MB` - reasonable starting point for OCCT.
- `ENVIRONMENT=node` - the Node CLI target.
- `ENVIRONMENT=web,worker` - the modular browser target.
- `EXIT_RUNTIME=1` - clean exit after the Node CLI `main()` returns.
- `NO_EXIT_RUNTIME=1` - keep the browser runtime alive for repeated C ABI calls.
- OCCT is built with `-pthread` flags for threading support.

## Usage

```bash
python scripts/build_wasm.py           # full WASM build
node dist/geometer.js step-to-glb input.step output.glb
python scripts/build_wasm.py --clean   # wipe WASM artifacts
```

## Rationale

- No global toolchain install required.
- Pinned emsdk version ensures reproducible builds.
- Same pattern as native OCCT build - familiar to contributors.
- WASM and native builds are independent - different build trees, same source.

## Consequences

- `.deps/emsdk/` is ~1.5 GB on disk.
- OCCT WASM build takes ~15-20 minutes (first time).
- Total first-time WASM setup is ~20-30 minutes.
