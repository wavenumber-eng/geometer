# Plan 005 - Python Native Packaging And Release Builds

Date: 2026-05-22
Status: in progress

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

Adopt the same broad packaging lesson as the existing Python OCCT ecosystem:
build and package a normal shared-library OCCT SDK per target platform, then
build Geometer against that SDK and repair the wheel. The current
static-OCCT-in-`geometer.dll` path is acceptable for early local experiments,
but it should not be the final PyPI packaging model because it can leave Python
processes vulnerable to Windows teardown crashes after HLR/GLB work.

The release plan should relieve developer pressure by keeping prebuilt OCCT
artifacts available for supported platforms. The first target is Windows.

Candidate artifact locations:

- GitHub release assets downloaded by build scripts
- a dedicated internal dependency-artifact repository
- CI cache for repeat builds, with release assets used for reproducibility

Do not plan on checking full prebuilt OCCT SDKs directly into the source tree.
Release assets or a dependency-artifact repo keep source history cleaner and
match the way Python OCCT wrapper projects separate source code from platform
native payloads.

Regardless of storage location, every prebuilt OCCT artifact should record:

- OCCT version and source revision
- Geometer dependency artifact schema version
- target OS and architecture
- compiler/toolchain version
- static vs dynamic build mode
- build flags that affect exported/runtime behavior
- SHA-256 hashes for shipped files

## Date-Based Versioning Policy

The canonical policy is [ADR 006](../adr/006_date_based_versioning_policy.md).
In short, release tags use `vYYYY-MM-DD`, PyPI/CMake versions use `YYYY.M.D`,
C ABI generations use `YYYYMMDD`, and all generated build metadata must use
UTC.

## Package Shape

Proposed Python package name:

```text
geometer
```

Initial Python API shape:

```python
from geometer import hlr_projection_json, project_step_hlr, step_to_glb, version

projection = project_step_hlr(step_bytes, views=[...], options={...})
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

Windows implementation note: the current OCCT dependency is statically linked
into Geometer. Direct `ctypes` calls work, but STEP HLR/GLB calls can leave the
process vulnerable to OCCT teardown crashes at exit. The first Python package
therefore uses direct `ctypes` for version/library loading and routes
OCCT-heavy operations through a small worker subprocess by default on Windows.
Set `GEOMETER_PYTHON_DIRECT=1` only for debugging the raw native call path.

## STEP Geometry Surface

The Python package should expose more than HLR. The first geometry preview API
should wrap the existing C ABI STEP-to-GLB operation:

```python
glb_bytes = geometer.step_to_glb(step_bytes)
```

That keeps the first milestone grounded in an implementation Geometer already
ships. It is enough for downstream Python tools that want to generate or hand
off a renderable model asset without linking Python OCCT.

A later native mesh/tessellation API should be considered if the Python viewer
or downstream tools need direct triangles instead of GLB bytes:

```python
mesh = geometer.step_to_mesh(step_bytes)
```

That API should return backend-neutral arrays such as positions, normals,
indices, material ids, and bounds. It should not expose OCCT classes and should
not carry PCB/CAD placement policy. Keep it separate from HLR: HLR is projected
line art; STEP geometry is renderable 3D geometry.

## Early Python Viewer

One early work product should be a small Python viewer that exercises the
package like a real downstream user. A Dear PyGui app is a reasonable first
target because it can be local, lightweight, and easy to run from a developer
checkout.

Viewer goals:

- load a STEP file from disk;
- call `project_step_hlr(...)` and plot simple/detail HLR in a 2D canvas;
- expose top/bottom/custom view controls and `model_transform` presets;
- call `step_to_glb(...)` for geometry-preview/export plumbing where useful;
- show version/ABI, projection timings, edge counts, and native errors;
- avoid Altium/KiCad-specific assumptions.

The viewer should start as an example or tool, not as the core package API. Its
job is to validate that the Python wrapper is friendly enough and that the HLR
request/result shape works in practice.

## Example Suite Exit Requirements

The release should include an `examples/` folder that shows the high-level APIs
in the environments Geometer is meant to support:

- `examples/python/` - Python wrapper demo using `project_step_hlr(...)`,
  `ProjectionView`, `HlrOptions`, and `step_to_glb(...)` where useful.
- `examples/cpp/` - native C++ demo using the friendly C++ API once that API is
  added.
- `examples/wasm/` - browser/WASM demo using the high-level JavaScript/worker
  API once that wrapper is added.

The first runnable artifact is the Python viewer. It should load a STEP file,
project HLR, draw simple/detail geometry, expose view and transform controls,
and report timings/errors.

The C++ example should be a small Dear ImGui application that exercises the same
user flow from native C++: load STEP, project HLR, display simple/detail line
art, expose view/transform controls, and report timing/error data. Dear ImGui
must be pinned, either by vendoring a minimal source snapshot under
`third_party/` with license/commit metadata or by using a checked-in dependency
manifest with an exact commit and source archive SHA-256. Do not use a floating
branch for example dependencies.

First implementation target: SDL3 + OpenGL3. SDL3 provides the portable
window/input/event layer, while OpenGL3 keeps the initial Windows/Linux renderer
simple and leaves a viable Emscripten/WebGL example path. Do not treat OpenGL3
as the long-term macOS renderer strategy: Apple deprecated OpenGL in macOS 10.14
and recommends Metal for GPU work. When macOS support is added, use Dear ImGui's
SDL3 + Metal backend instead.

SDL3 + Vulkan is a reasonable later variant if we need to validate
Vulkan-specific behavior, but on macOS it would imply a Vulkan-over-Metal layer
and has more setup surface than this viewer needs. SDL3 + SDL_GPU is also worth
watching, but it is newer and should not block the first portable C++ example.

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
- `geometer.project_step_hlr(step_bytes, views=[...], options=...)` returns a
  parsed result wrapper.
- `geometer.step_to_glb(step_bytes, options)` returns non-empty GLB bytes.
- A small Python viewer can load a STEP fixture and plot HLR output.

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
git tag v2026-05-23
git push origin v2026-05-23
```

CI should publish only if all target wheels and smoke tests pass.

### Phase 5 - Internal Downstream Migration

After the first dated Geometer release lands, patch internal consumers to use
the released package/artifacts instead of local experiments:

- `toolz/viz` and Altium Cruncher projection paths;
- KiCad Monkey plugin/tooling experiments that need HLR;
- board visualization scripts that currently depend on Python OCCT/OCP for
  projection.

This migration can be delegated to other agents after the Geometer release is
cut. The release should include a short migration note with the Geometer date
version, C ABI date, Python package name, expected projection defaults, and
known behavior differences from the old Python OCCT path.

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
- build OCCT as a shared-library SDK with known flags for Python wheels
- build Geometer's shared library against that SDK
- bundle OCCT runtime libraries next to Geometer in wheels
- validate with wheel repair tools:
  - `delvewheel` on Windows
  - `auditwheel` on Linux
  - `delocate` on macOS

The source build path should remain available for maintainers, and static OCCT
can remain useful for CLI/WASM/local experiments. PyPI wheels should prefer the
shared-library SDK layout so Python imports resemble the packaging architecture
used by existing Python OCCT wrappers instead of relying on a monolithic
static-OCCT DLL.

The current Windows worker subprocess is a compatibility bridge for the static
local build. It should be removed from the default path once shared-OCCT wheels
prove direct `ctypes` HLR/GLB calls can exit cleanly.

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

1. Store prebuilt OCCT artifacts in GitHub release assets or a dedicated
   dependency-artifact repo?
2. Keep static OCCT for CLI/local builds only, or move every native build path
   to shared OCCT once Python wheels are stable?
3. Should the first PyPI package publish as public `geometer`, private/internal
   package, or a scoped/company-specific name?
4. Which Python versions are required for `kicad_monkey` and other downstream
   tools?
5. Should `dist/` continue to contain native binaries once Python wheels become
   the main native distribution artifact?
6. Should date-version generation live in CMake, a small Python script, or both
   with one manifest file as source of truth?

## First Implementation Checklist

- [ ] Decide Windows shared OCCT artifact storage strategy.
- [ ] Add date-version source of truth and CMake/Python version generation.
- [x] Add shared-library CMake target exporting the C ABI.
- [x] Add Python package skeleton and `ctypes` loader.
- [ ] Add Windows wheel build command.
- [x] Add smoke fixture and Python tests.
- [ ] Validate from a clean Windows venv.
- [ ] Validate Linux wheel under WSL.
- [ ] Draft GitHub Actions `cibuildwheel` workflow.
- [ ] Draft TestPyPI trusted-publishing workflow.
- [ ] Draft internal downstream migration checklist for `toolz/viz`,
      Altium Cruncher, and KiCad Monkey.
