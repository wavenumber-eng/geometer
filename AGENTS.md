# Geometer Agent Guide

Geometer is a focused C++17 geometry library and CLI built on OpenCASCADE
Technology (OCCT). It provides generic CAD/kernel operations for STEP conversion,
hidden-line projection, planar contouring, browser WASM workflows, and future
Python tooling.

For build setup, dependency policy, tests, and troubleshooting, read
[docs/developer/README.md](docs/developer/README.md). Treat that file as the
canonical developer build guide.

For the current C++, C ABI, Python, WASM, CLI, JSON, and binary format surface,
start at [docs/design/README.md](docs/design/README.md).

## Working Rules

- Keep `geometer` generic. Do not add Altium-specific, board-specific, or
  visualizer-specific policy to the core library.
- Keep implementation modules small and responsibility-focused. Do not grow a
  catch-all `geometer.cpp`.
- Prefer direct C++ value APIs in `src/cpp/lib/`, with stable C ABI wrappers for
  WASM and possible future non-C++ bindings. The public Python package currently
  uses the executable-backed CLI path.
- `.deps/` is generated local dependency state and must not be committed.
- OCCT may be restored from the optional Wavenumber R2 binary dependency cache,
  but it still lands under `.deps/` and remains generated state. Do not commit
  OCCT archives, `.deps/`, `.env`, or R2 credentials.
- `dist/` contains distributable outputs by current project policy. Prefer
  grouped paths (`dist/native/<platform>/`, `dist/wasm/<target>/`) for all
  consumers. Do not recreate root-level `dist/geometer*` artifacts.
- Plans are temporary working notes only. Do not persist completed plans in the
  repo; once work ships, the docs of record are updated code, ADRs,
  requirements, and design docs.
- Versioned releases use ADR 006 date versions: release tags use
  `vYYYY-MM-DD`, CMake/PyPI use `YYYY.M.D`, and the C ABI generation uses
  `YYYYMMDD`.
- GitHub workflow edits must use Node 24-capable action majors where available
  (for example `actions/cache@v5`, `actions/setup-node@v6`,
  `actions/upload-artifact@v7`, and `actions/download-artifact@v8`). Do not
  reintroduce Node 20-backed action majors.
- Use CMake for proper native builds and CTest for registered tests.
- Rack strata are the test index of record. `tests/python` is an enabled Python
  unit stratum; keep new Python tests represented in `tests/python/STRATUM.toml`.
- The default native preset uses Ninja.
- CMake presets must set `CMAKE_EXPORT_COMPILE_COMMANDS=ON`.
- Format touched C++ files with `clang-format`.
- Keep `.clang-format`, `.clang-tidy`, `.gitattributes`, and
  `tool.wn_dev_std` aligned with the Wavenumber `python-native-wasm` profile.
- Run Ruff, Pyright, uv lock checks, clang-format checks, native validation,
  package validation, and the L99 release gate before tagging a release.

## Build Shortcuts

Native:

```powershell
uv sync --group dev
cmake --preset default
cmake --build build --config Release
ctest --test-dir build -C Release --output-on-failure
uv run --group dev rack run --all
uv run pytest tests\L99_release -q
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

`build_occt.py` cleans/builds the current native platform under
`.deps/native/<platform>/` by default. Use `--clean-source` only when refreshing
the shared OCCT source checkout too.

Optional OCCT binary cache:

```powershell
python scripts\build_occt.py --print-binary-cache-key
python scripts\build_wasm.py --print-occt-binary-cache-key
```

Set R2 credentials in local `.env` only. Normal CI consumes the cache; the
manual `OCCT Dependency Cache` GitHub workflow publishes cache archives.
