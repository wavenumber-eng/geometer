# ADR-002: dist/ directory for distributable binaries

## Status

Accepted

## Context

Geometer produces binaries (CLI executable, static library, Node WASM CLI, and browser WASM C ABI) that need to be distributed. We need a single, known location for build outputs that can be committed to the repo and used directly by developers, CI, tests, and downstream projects.

## Decision

A `dist/` directory at the repo root holds distributable binaries. These are checked into git.

Post-build, CMake copies outputs to grouped runtime directories under `dist/`:

- `dist/native/<platform>/geometer.exe` (or `geometer` on Unix)
- `dist/native/<platform>/geometer.lib` or `libgeometer.a`
- `dist/wasm/browser/geometer.js` and `dist/wasm/browser/geometer.wasm` for
  full browser/Web Worker integration
- `dist/wasm/node-test/geometer-node-test.js` and
  `dist/wasm/node-test/geometer-node-test.wasm` for Node CLI parity tests
- `dist/wasm/planar-browser/geometer-planar-browser.js` and
  `dist/wasm/planar-browser/geometer-planar-browser.wasm` for smaller
  planar-only browser/Web Worker consumers

Root-level build artifacts such as `dist/geometer.exe`, `dist/geometer.js`, or
`dist/libgeometer.a` are intentionally not produced. Source-checkout consumers
must use grouped paths.

Generated dependency and build state remains outside version control:

- `.deps/`
- `build/`
- `build-wasm/`

Tests run against the binaries in `dist/`, not the build tree. This ensures what ships is what gets tested.

## Rationale

- Single location for all distributable artifacts regardless of build config.
- Grouped paths prevent Windows, Linux, macOS, and WASM artifacts from
  overwriting each other.
- Committed binaries allow consumers to clone and use native and WASM interfaces without building.
- Testing against `dist/` catches packaging and copy issues (missing files, wrong binary).

## Consequences

- Repo size grows with each binary update. Acceptable for focused, infrequent releases.
- Contributors must rebuild and update `dist/` when changing code.
- `.gitkeep` preserves the directory when empty.
