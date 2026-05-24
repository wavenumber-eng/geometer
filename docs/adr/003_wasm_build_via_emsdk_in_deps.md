# ADR-003: WASM build via emsdk managed in .deps

## Status

Accepted

## Context

Geometer needs a WASM build for browser-based geometry operations. Emscripten
(emsdk) is required to cross-compile OCCT and Geometer to WebAssembly.

Options considered:

- Global emsdk install: rejected because each developer must install and
  version-match manually.
- emsdk managed in `.deps/`: adopted because it follows the same generated
  dependency pattern as OCCT.

## Decision

`scripts/build_wasm.py` manages the full WASM toolchain:

1. Clones emsdk into `.deps/emsdk/` and installs a pinned Emscripten version.
2. Cross-compiles OCCT to WASM static libraries
   (`.deps/occt-wasm-install/`).
3. Cross-compiles Geometer against the WASM OCCT build.
4. Copies the Node CLI and browser C ABI WASM outputs to grouped
   `dist/wasm/<target>/` folders.

OCCT source is shared with the native `build_occt.py` flow. RapidJSON is
vendored under `third_party/rapidjson` and passed to OCCT during configure.

## WASM Build Specifics

- `NODERAWFS=1`: the WASM CLI uses Node's real filesystem for file I/O.
- `ALLOW_MEMORY_GROWTH=1`: STEP models vary in size.
- `INITIAL_MEMORY=64MB`: reasonable starting point for OCCT.
- `ENVIRONMENT=node`: the Node CLI target.
- `ENVIRONMENT=web,worker`: the modular browser target.
- `EXIT_RUNTIME=1`: clean exit after the Node CLI `main()` returns.
- `NO_EXIT_RUNTIME=1`: keep the browser runtime alive for repeated C ABI calls.
- OCCT is built with `-pthread` flags for threading support.

## Usage

```bash
python scripts/build_wasm.py
node dist/wasm/node-test/geometer-node-test.js step-to-glb input.step output.glb
python scripts/build_wasm.py --clean
```

## Rationale

- No global toolchain install required.
- Pinned emsdk version ensures reproducible builds.
- Same pattern as the native OCCT build, so contributors use one mental model.
- WASM and native builds are independent build trees using the same source.

## Consequences

- `.deps/emsdk/` is about 1.5 GB on disk.
- OCCT WASM build takes about 15-20 minutes the first time.
- Total first-time WASM setup is about 20-30 minutes.
