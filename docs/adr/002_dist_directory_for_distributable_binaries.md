# ADR-002: dist/ directory for distributable binaries

## Status

Accepted

## Context

Geometer produces binaries (CLI executable, static library, and eventually WASM) that need to be distributed. We need a single, known location for build outputs that can be committed to the repo and used directly by developers, CI, and tests.

## Decision

A `dist/` directory at the repo root holds distributable binaries. These are checked into git.

Post-build, CMake copies outputs to `dist/`:
- `dist/geometer.exe` (or `dist/geometer` on Unix)
- `dist/geometer_lib.lib` (or `.a`)
- Eventually: `dist/geometer.wasm`, `dist/geometer.js`

Tests run against the binaries in `dist/`, not the build tree. This ensures what ships is what gets tested.

A `setup.ps1` script at the repo root adds `dist/` to the user's PATH for the current session.

## Rationale

- Single location for all distributable artifacts regardless of build config.
- Committed binaries allow consumers to clone and use without building.
- Testing against `dist/` catches packaging and copy issues (missing files, wrong binary).
- `setup.ps1` gives immediate CLI access after clone.

## Consequences

- Repo size grows with each binary update. Acceptable for focused, infrequent releases.
- Contributors must rebuild and update `dist/` when changing code.
- `.gitkeep` preserves the directory when empty.
