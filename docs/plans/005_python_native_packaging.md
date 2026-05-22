# Plan 005 - Python Native Packaging And Release Builds

Date: 2026-05-22
Status: planning

## Goal

Add a clean Python interface to Geometer's native HLR and geometry functions
without rewriting the implementation against Python OCCT.

Geometer should keep one C++ implementation and expose it through two release
paths:

- browser/Web Worker builds through Emscripten WASM
- native shared-library builds through a Python package

The first implementation target is Windows on this development PC. The build
system should be structured so Linux and macOS wheels can be added without a
redesign. Short-term Linux validation should run through WSL. Long-term release
builds should run through GitHub Actions CI/CD.

## Motivation

Downstream tools such as `kicad_monkey`, board visualization tools, and future
CAD workflows need reliable hidden-line projection and geometry operations. They
should call the same Geometer implementation used by the browser, not maintain
parallel Python OCCT projection code.

The existing C ABI is already the right boundary for this:

- `geometer_step_hlr_projection_json_bytes`
- `geometer_step_to_glb_bytes`
- `geometer_planar_batch_solve_bytes`
- `geometer_free_string`
- `geometer_free_bytes`
- version and ABI checks

The Python layer should initially be a thin wrapper over that C ABI.

## Dependency Policy

OCCT remains hard-pinned. HLR behavior and performance are sensitive to OCCT
version, build options, compiler, and dependency layout. Python packages must
therefore know exactly which OCCT build they bundle or load.

The release plan should relieve developer pressure by keeping prebuilt OCCT
artifacts available for supported platforms. The first target is Windows.

Candidate artifact locations:

- checked-in prebuilt OCCT libraries under a clearly versioned dependency
  artifact folder
- GitHub release assets downloaded by build scripts
- a dedicated internal dependency-artifact repository
- CI cache for repeat builds, with release assets used for reproducibility

Decision to make before implementation: whether prebuilt OCCT libraries belong
directly in this repo. Direct check-in makes local wrapping easiest, but it
increases repository size and makes platform churn more visible in normal Git
history. Release assets or a dependency-artifact repo keep source history
cleaner, but add bootstrap logic.

Regardless of storage location, every prebuilt OCCT artifact should record:

- OCCT version and source revision
- Geometer dependency artifact schema version
- target OS and architecture
- compiler/toolchain version
- static vs dynamic build mode
- build flags that affect exported/runtime behavior
- SHA-256 hashes for shipped files

## Package Shape

Proposed Python package name:

```text
geometer
```

Initial Python API shape:

```python
from geometer import hlr_projection_json, step_to_glb, version

projection = hlr_projection_json(step_bytes, options={...})
glb_bytes = step_to_glb(step_bytes, options={...})
```

Implementation layers:

- `geometer/__init__.py` - public Python API
- `geometer/_native.py` - `ctypes` loader and C ABI signatures
- `geometer/_paths.py` - locate bundled native library and dependency DLLs
- `geometer/_errors.py` - exception types for C ABI errors
- `geometer/_version.py` - project and C ABI version checks

Use `ctypes` first. The C ABI calls are coarse and byte-buffer oriented, so
Python call overhead is not important. Avoid `pybind11` until we have a reason
to expose fine-grained C++ value objects.

## Native Library Targets

Add a shared-library build target in addition to the existing CLI/static outputs:

- Windows: `geometer.dll`
- Linux: `libgeometer.so`
- macOS: `libgeometer.dylib`

The shared library should export the same C ABI documented in `INTERFACES.md`.
The CLI can continue to exist as a separate executable. The Python wheel should
bundle the shared library and all required runtime libraries.

On Windows, the wrapper should use `os.add_dll_directory(...)` before loading
`geometer.dll` so bundled OCCT/runtime DLLs resolve from the package directory.

## Build System Direction

Use a Python packaging frontend that works with CMake:

- `scikit-build-core` for Python/CMake integration
- `cibuildwheel` for matrix wheel builds
- `pytest` smoke tests against the installed wheel

The first Windows milestone can be simpler:

1. Add `pyproject.toml`.
2. Build or copy `geometer.dll` plus dependencies into the wheel.
3. Load the library with `ctypes`.
4. Run a smoke test from an installed wheel on Windows.

Do not force the first milestone to solve every platform. Instead, keep the
CMake and package layout platform-neutral.

## Local Build Plan

### Phase 1 - Windows Developer Wheel

Deliverables:

- Windows shared-library target.
- Python package skeleton.
- `ctypes` wrapper for version, HLR projection JSON, and GLB bytes.
- Wheel build command that works on this PC.
- Smoke tests using a small STEP fixture.

Candidate commands:

```powershell
python -m build
python -m pip install --force-reinstall dist\geometer-*.whl
python -m pytest tests\python
```

Acceptance:

- `import geometer` works from a clean venv.
- `geometer.version()` returns the bundled library version and ABI.
- `geometer.hlr_projection_json(step_bytes, options)` emits
  `geometry.projection.a0`.
- `geometer.step_to_glb(step_bytes, options)` returns non-empty GLB bytes.

### Phase 2 - WSL Linux Validation

Deliverables:

- Linux shared-library build under WSL.
- Linux wheel build under WSL.
- Smoke tests pass from an installed Linux wheel.

This phase proves that the package layout is not Windows-only before investing
in CI/CD.

### Phase 3 - GitHub Actions Wheel CI

Deliverables:

- CI workflow that builds wheels on:
  - Windows x64
  - Linux x64
  - macOS x64
  - macOS arm64
- CI smoke tests installed wheels.
- Build artifacts uploaded for manual inspection.

Use `cibuildwheel` for the matrix. Use GitHub-hosted runners rather than trying
to cross-compile every platform from a single PC.

Linux ARM64 can be added later through native ARM runners or emulation.
Windows ARM64 should wait until there is a real consumer and a test strategy.

### Phase 4 - PyPI/TestPyPI Release

Deliverables:

- TestPyPI publishing workflow.
- PyPI publishing workflow gated by tags.
- PyPI Trusted Publishing through GitHub Actions OIDC.
- Release checklist that builds native wheels, runs smoke tests, builds WASM,
  and verifies version/ABI consistency.

Tag-driven release shape:

```powershell
git tag v1.2.0
git push origin v1.2.0
```

CI should publish only if all target wheels and smoke tests pass.

## Cross-Platform Notes

Local Windows PC can reasonably cover:

- native Windows wheel build
- Linux x64 wheel preflight through WSL
- Linux container preflight through Docker/manylinux later
- Emscripten WASM build

Local Windows PC should not be treated as the authoritative release builder for:

- macOS x64
- macOS arm64
- Linux ARM64, unless using emulation only for preflight

Official release artifacts should come from CI so downstream users exercise the
same path that will publish to PyPI.

## OCCT Packaging Notes

OCCT itself is source/CMake-oriented. Common ecosystem package routes include
system packages, Homebrew/MacPorts, vcpkg, conda-forge, and official Windows
binary downloads. For Geometer Python wheels, relying on a user's system OCCT is
too fragile. Wheels should bundle the runtime pieces they need.

Preferred Geometer policy:

- pin OCCT version in build scripts
- build OCCT with known flags
- bundle runtime libraries in wheels
- validate with wheel repair tools:
  - `delvewheel` on Windows
  - `auditwheel` on Linux
  - `delocate` on macOS

The source build path should remain available for maintainers, but most users
and downstream packages should consume prebuilt wheels.

## Relationship To WASM

The Python package should not run the browser WASM build. Emscripten browser
outputs are optimized for JavaScript/Web Worker use and include JS runtime glue.
Python should call native Geometer.

Keep the two release paths parallel:

- `dist/geometer-browser.js` and `.wasm` for web tooling
- Python wheels with native shared library for Python tooling

Both paths should expose the same semantic operations and should report the same
project version and C ABI version.

## Risks

- OCCT wheel size may be large.
- Static vs dynamic OCCT linking needs platform-specific testing.
- Windows DLL search behavior can be brittle without careful package layout.
- Linux manylinux compatibility may require tuning compiler and dependency
  choices.
- macOS notarization is not expected for PyPI wheels, but dependency rpaths must
  be correct.
- HLR output can drift when OCCT, compiler, or algorithm defaults change.

## Open Questions

1. Store prebuilt OCCT artifacts in this repo, GitHub release assets, or a
   dependency-artifact repo?
2. Static-link OCCT into `geometer` where possible, or bundle dynamic OCCT
   libraries next to the Geometer shared library?
3. Should the first PyPI package publish as public `geometer`, private/internal
   package, or a scoped/company-specific name?
4. Which Python versions are required for `kicad_monkey` and other downstream
   tools?
5. Should `dist/` continue to contain native binaries once Python wheels become
   the main native distribution artifact?

## First Implementation Checklist

- [ ] Decide Windows OCCT artifact storage strategy.
- [ ] Add shared-library CMake target exporting the C ABI.
- [ ] Add Python package skeleton and `ctypes` loader.
- [ ] Add Windows wheel build command.
- [ ] Add smoke fixture and Python tests.
- [ ] Validate from a clean Windows venv.
- [ ] Validate Linux wheel under WSL.
- [ ] Draft GitHub Actions `cibuildwheel` workflow.
- [ ] Draft TestPyPI trusted-publishing workflow.
