# Plan 005 - Python Native Packaging And Release Builds

Date: 2026-05-22
Status: in progress

## Goal

Add a clean Python interface to Geometer's native HLR and geometry functions
without rewriting the implementation against Python OCCT.

Geometer should keep one C++ implementation and expose it through two release
paths:

- browser/Web Worker builds through Emscripten WASM
- native executable-backed builds through a Python package

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

The preferred Python packaging model is now executable-backed, not
library-backed. Python should call a platform-native `geometer` CLI subprocess
by default and keep the public Python API friendly. This preserves process
isolation around OCCT, makes the command-line tool directly useful to non-Python
consumers, and avoids making every Python environment solve native library load
and teardown behavior.

The primary native release artifacts should be:

- one statically linked CLI executable per target platform;
- one static native library for C++ consumers;
- no OCCT runtime DLL/SO/dylib bundle in the normal Python wheel path.

The shared-OCCT/`ctypes` path was useful as a spike, but it is not a PyPI
release path.

The release plan should relieve developer pressure by keeping prebuilt OCCT
artifacts available for supported platforms. The first target is Windows.

Candidate artifact locations:

- GitHub release assets downloaded by build scripts
- a dedicated internal dependency-artifact repository
- CI cache for repeat builds, with release assets used for reproducibility

Do not plan on checking full prebuilt OCCT SDKs directly into the source tree.
Release assets or a dependency-artifact repo keep source history cleaner.
Python wheels should bundle the final platform executable, not full OCCT SDKs.

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

## Current Status - 2026-05-23

The first Python package and viewer milestone is usable from the source
checkout. The package exposes version, STEP HLR projection, parsed projection
results, and STEP-to-GLB bytes through the friendly Python API. The preferred
Python example is now `examples/python/pyvista_hlr_viewer.py`, a PyVista/Qt
preview app with:

- a 3D STEP-to-GLB preview pane;
- an adjacent HLR projection pane;
- camera preset buttons and camera-driven HLR regeneration;
- simple/detail/both projection modes;
- feature-edge overlay for the 3D preview;
- lighting/material controls;
- Geometer version and C ABI visibility.

The older Dear PyGui viewer remains as a lightweight fallback and smoke path.

The native C++ example is also usable with Dear ImGui, SDL3, and OpenGL3. Both
interactive examples show the Geometer version and C ABI generation.

Version reporting has been moved to the ADR 006 date values for the current
work: `2026.5.23` for package/runtime version and `20260523` for the C ABI
generation. A later cleanup should replace the remaining manual synchronization
with a generated single source of truth.

A Windows shared-OCCT spike proved that direct `ctypes` calls can work when the
runtime layout is carefully controlled, but that packaging direction is now
retired. The release direction is a statically linked `geometer.exe`/`geometer`
driven from Python through subprocess calls. `dist/` no longer persists
`geometer.dll` or OCCT `TK*.dll` runtime files.

Implementation update: the Python API now defaults to the executable backend.
It locates `GEOMETER_EXE` or the source-checkout `dist/geometer.exe`, writes a
temporary JSON batch request, calls `geometer run`, and reads generated
HLR/GLB outputs back into the friendly Python API. `GEOMETER_BACKEND=ctypes`
and `GEOMETER_BACKEND=worker` remain available for developer/debug use.

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
- `geometer/_cli.py` - subprocess runner for the bundled or configured CLI
- `geometer/_paths.py` - locate bundled executable and optional native library
- `geometer/_native.py` - optional `ctypes` loader and C ABI signatures
- `geometer/_errors.py` - exception types for C ABI errors
- `geometer/_version.py` - project and C ABI version checks

Use the CLI subprocess backend first. HLR and STEP-to-GLB are coarse, heavy
operations, so process startup overhead is acceptable for the first Python
release and buys much simpler deployment. Avoid `pybind11` until there is a
reason to expose fine-grained C++ value objects.

Backend selection policy:

- `GEOMETER_BACKEND=exe` - default; run the bundled/static CLI.
- `GEOMETER_BACKEND=ctypes` - optional development/performance backend.
- `GEOMETER_BACKEND=worker` - legacy subprocess bridge around the C ABI.
- `GEOMETER_EXE` - override the executable path.
- `GEOMETER_NATIVE_LIBRARY` - override the optional native library path.

The executable backend should preserve the same friendly Python API. It may use
temporary files internally for STEP input and GLB/JSON/SVG output, but those
details should not leak to normal callers.

## CLI JSON Batch Interface

The CLI now has an initial JSON request mode for downstream migration:

```powershell
geometer run request.json response.json
```

It can also generate a starter request from a STEP file:

```powershell
geometer init-request request.json --step U1.step --operation step_hlr_projection_json --output U1.projection.json
geometer init-request request.json --step U1.step --operation step_to_glb --output U1.glb
```

For interactive convenience, if a user gives `geometer run` a STEP file instead
of a request JSON, the CLI may emit a clear message pointing at
`geometer init-request` or offer a `--write-template` mode. Scripted workflows
should use the explicit template command so behavior is deterministic.

Initial request shape:

```json
{
  "schema": "geometer.batch.request.a0",
  "version": "2026.5.23",
  "jobs": [
    {
      "id": "u1-top",
      "operation": "step_hlr_projection_json",
      "step_path": "U1.step",
      "output_path": "U1.top.projection.json",
      "options": {
        "views": [{"id": "top", "direction": [0, 0, 1], "up": [0, 1, 0]}],
        "curve_mode": "polyline",
        "model_transform": [[1, 0, 0, 0], [0, 1, 0, 0], [0, 0, 1, 0], [0, 0, 0, 1]]
      }
    },
    {
      "id": "u1-glb",
      "operation": "step_to_glb",
      "step_path": "U1.step",
      "output_path": "U1.glb",
      "options": {"linear_deflection": 0.1, "angular_deflection": 0.5}
    }
  ]
}
```

Initial response shape:

```json
{
  "schema": "geometer.batch.response.a0",
  "version": "2026.5.23",
  "abi": 20260523,
  "ok": true,
  "jobs": [
    {
      "id": "u1-top",
      "ok": true,
      "operation": "step_hlr_projection_json",
      "code": 0,
      "output_path": "U1.top.projection.json",
      "elapsed_ms": 12.3
    }
  ]
}
```

The batch interface should support multiple STEP inputs in one process. That
lets Altium Cruncher, KiCad Monkey, and other Python callers amortize process
startup across repeated HLR/GLB operations. Prefer `step_path` and
`output_path` for large CAD data. Inline byte transport can be added later if a
consumer truly needs it.

The generated template should include:

- current Geometer version and ABI;
- one job with the requested operation;
- default HLR or GLB options;
- a comment-free JSON shape that can be consumed directly after editing paths;
- stable job ids derived from input file names where possible.

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

Every interactive example should show the Geometer version string and C ABI
generation conspicuously in the UI. These examples double as developer smoke
tools, so version visibility is part of the contract.

The WASM/browser example can be promoted later from the existing
`tests/wasm/embedded_model_viewer.html` reference page. Treat that promotion as
exit criteria for the full example suite, not as a blocker for the first Python
viewer milestone.

The first runnable artifact is the Python viewer. It should load a STEP file,
show an interactive 3D preview from Geometer's STEP-to-GLB path, project HLR,
draw simple/detail geometry in an adjacent projection pane, expose view and
transform controls, and report timings/errors.

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

## Native Distribution Targets

Primary release artifacts:

- Windows: statically linked `geometer.exe`.
- Linux: statically linked or mostly-static `geometer`.
- macOS: self-contained `geometer` app-binary layout as far as platform rules
  reasonably allow.
- Native link artifact: `geometer.lib` on Windows and `libgeometer.a` on
  Unix-like platforms.

The executable should be easy to copy and use directly from a terminal. The
Python package should bundle that executable and drive it through a subprocess
by default.

The shared library target can remain for experiments and future high-throughput
in-process users, but it is not the first Python release artifact.

## Build System Direction

Use a Python packaging frontend that works with CMake:

- `scikit-build-core` for Python/CMake integration
- `cibuildwheel` for matrix wheel builds
- `pytest` smoke tests against the installed wheel

The first Windows milestone can be simpler:

1. Add `pyproject.toml`.
2. Build or copy the statically linked `geometer.exe` into the wheel.
3. Drive it from Python through a subprocess backend.
4. Run a smoke test from an installed wheel on Windows.

Current local wheel build command:

```powershell
python -m pip wheel . -w out\wheelhouse --no-deps
```

The setuptools build command copies `dist/geometer.exe` / `dist/geometer` into
`geometer/native/` inside the wheel and marks the wheel platform-specific.

Do not force the first milestone to solve every platform. Instead, keep the
CMake and package layout platform-neutral.

## Local Build Plan

### Phase 1 - Windows Developer Wheel

Deliverables:

- Windows statically linked CLI target.
- Python executable backend.
- Python package skeleton.
- friendly wrappers for version, HLR projection JSON, and GLB bytes.
- Wheel build command that works on this PC.
- Smoke tests using a small STEP fixture.

Candidate commands:

```powershell
python -m pip wheel . -w out\wheelhouse --no-deps
python -m venv out\venv-wheel-smoke
out\venv-wheel-smoke\Scripts\python.exe -m pip install out\wheelhouse\geometer-*.whl
out\venv-wheel-smoke\Scripts\python.exe -c "import geometer; print(geometer.version()); print(geometer.executable_path())"
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
- The wheel can be installed on a clean Windows machine without OCCT DLLs on
  `PATH`.

Validation update: on 2026-05-23, a local platform wheel installed into a fresh
Windows venv from `out\wheelhouse-*`, resolved
`site-packages\geometer\native\geometer.exe`, produced
`geometry.projection.a0` for the SOT-23 STEP fixture, and returned GLB bytes.

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
- Public PyPI package named `geometer` published for the dated release so
  downstream tools can depend on `geometer==YYYY.M.D` without a local path
  override.
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
- `wn-altium-cruncher`/`wn-viz-core` `pyproject.toml` dependencies should use
  the PyPI package, for example `geometer==2026.5.23`, instead of the temporary
  workspace-local `../../geometer` source override;
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

- pin OCCT version in build scripts;
- build OCCT static libraries for the release CLI and native static library;
- statically link the CLI so `geometer.exe`/`geometer` is easy to copy and run;
- make the Python wheel a client for that executable;
- validate the executable in a clean environment without OCCT DLL/SO/dylib
  search-path setup.

On Windows, the target should be "no OCCT DLLs required." If we also want "no
Visual C++ redistributable required," the release build needs a deliberate MSVC
runtime policy, likely `/MT`, applied consistently to OCCT and Geometer.

The shared-OCCT `ctypes` work remains historical evidence for possible future
in-process users, but the executable-backed Python release does not depend on
that layout and does not persist its runtime DLLs.

## Relationship To WASM

The Python package should not run the browser WASM build. Emscripten browser
outputs are optimized for JavaScript/Web Worker use and include JS runtime glue.
Python should call native Geometer.

Keep the two release paths parallel:

- `dist/geometer.js` and `.wasm` for full browser/Web Worker integration
- `dist/geometer-planar-browser.js` and `.wasm` as the smaller optional
  planar-only browser/Web Worker optimization
- Python wheels with a bundled static CLI for Python tooling

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
2. Should the optional shared-library backend stay in release builds, or remain
   development-only until there is a concrete in-process consumer?
3. Resolved for this plan: publish the first package as public `geometer` on
   PyPI after TestPyPI validation.
4. Which Python versions are required for `kicad_monkey` and other downstream
   tools?
5. Should `dist/` continue to contain native binaries once Python wheels become
   the main native distribution artifact?
6. Should date-version generation live in CMake, a small Python script, or both
   with one manifest file as source of truth?

## First Implementation Checklist

- [x] Decide first Python packaging backend: executable subprocess by default.
- [x] Add CLI JSON batch request/response command.
- [x] Make Windows release CLI statically linked and easy to copy.
- [ ] Add date-version source of truth and CMake/Python version generation.
- [x] Add shared-library CMake target exporting the C ABI.
- [x] Spike Windows shared-OCCT developer build preset and direct-exit smoke
      harness, then retire it from the release path.
- [x] Add Python package skeleton and `ctypes` loader.
- [x] Add Python executable backend and make it default.
- [x] Add Windows wheel build command.
- [x] Add smoke fixture and Python tests.
- [x] Add interactive Python HLR/3D preview example with visible version/ABI.
- [x] Add native C++ Dear ImGui/SDL3/OpenGL HLR preview example with visible
      version/ABI.
- [x] Validate from a clean Windows venv.
- [ ] Validate Linux wheel under WSL.
- [ ] Draft GitHub Actions `cibuildwheel` workflow.
- [ ] Draft TestPyPI trusted-publishing workflow.
- [ ] Publish the dated release as public `geometer` on PyPI after TestPyPI
      validation.
- [ ] Draft internal downstream migration checklist for `toolz/viz`,
      Altium Cruncher, and KiCad Monkey.
