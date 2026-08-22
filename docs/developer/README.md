# Geometer Development

This document is the practical setup guide for building, testing, and working on
Geometer from a fresh checkout.

## Project Summary

Geometer is a focused C++17 geometry library and CLI built on OpenCASCADE
Technology (OCCT). Its job is to provide generic CAD/kernel geometry operations
for browser, native CLI, and Python tooling.

Current and planned library surfaces include:

- STEP to GLB conversion.
- STEP hidden-line projection geometry.
- Planar contour extraction for simplified projected outlines.
- Planar batch boolean/offset solving for filled 2D geometry.
- Future STEP mesh/tessellation APIs for browser rendering.

The core library must stay generic. Do not put board placement rules, Altium
specific names, visualizer policy, or downstream application semantics into
`geometer`.

For the current callable C++, C ABI, Python, WASM, CLI, JSON, and binary
formats, start at [../design/README.md](../design/README.md).

## Repository Layout

- `src/cpp/lib/` - reusable C++ library code.
- `src/cpp/cli/` - thin CLI wrapper around the library.
- `python/geometer/` - executable-backed Python package.
- `examples/` - Python, C++, and WASM-facing examples.
- `tests/` - stratified tests and C++ test sources.
- `docs/geometer/adr/` - architecture decisions.
- `docs/geometer/requirements/` - numbered requirements.
- `docs/design/` - maintained interface and format documentation.
- `docs/developer/` - developer setup, validation, and release commands.
- `scripts/` - dependency/build helper scripts.
- `dist/` - distributable binaries and WASM outputs.
- `.deps/` - local generated dependencies and toolchains.

## Dependency Policy

`.deps/` is local generated state and must not be committed. It contains cloned
and built dependencies such as OCCT, emsdk, and OCCT WASM artifacts. It is
intentionally ignored by Git.

`third_party/rapidjson/` is different. RapidJSON is header-only, small enough to
vendor, and required by OCCT's GLB export path. The vendored copy is checked in
so a fresh clone does not need a separate RapidJSON git checkout.

`third_party/clipper2/` is also different. Clipper2 is a compact BSL-1.0 C++
library used by Geometer's generic planar batch solve API. The checked-in copy
contains only the C++ library sources, headers, upstream license, and a local
vendoring note.

`dist/` is different. This repository currently treats `dist/` as the location
for distributable binaries. CMake and WASM builds copy final outputs there.
Those outputs are committed when publishing changes so another project can clone
and use Geometer without a local native/WASM rebuild. Canonical native artifacts
live under `dist/native/<platform>/`, canonical WASM artifacts live under
`dist/wasm/<target>/`, and the generated ESM package lives under
`dist/wasm/npm/geometer/`. Root-level `dist/geometer*` artifacts are intentionally
not produced.

OCCT is not vendored into the repository and is not added with CMake
`FetchContent`, because OCCT uses `CMAKE_SOURCE_DIR` internally and does not work
correctly as a subdirectory dependency. Instead, Geometer restores or builds
OCCT as generated dependency state and finds it with
`find_package(OpenCASCADE)`.

Geometer can optionally restore prebuilt OCCT install trees from the Wavenumber
public artifact cache. This is a speed optimization only: restored archives are
validated by manifest and SHA-256, still land under `.deps/`, and source builds
remain the fallback when the cache misses.

For dependency cache setup, see
[r2-dependency-cache.md](r2-dependency-cache.md).

Pinned dependency versions live in scripts:

- OCCT and emsdk: `scripts/dependency_versions.py`
- Boost.Multiprecision: `scripts/dependency_versions.py` and `scripts/build_boost.py`
- RapidJSON: `third_party/rapidjson/`
- Clipper2: `third_party/clipper2/`

The non-primary exact real-algebraic oracle and bounded fallback use the pinned
header-only Boost
source tree under `.deps/boost_1_92_0/`. CMake restores it automatically after
verifying the official archive SHA-256, or it can be prepared explicitly:

```powershell
python scripts\build_boost.py
```

The archive cache and extracted headers are generated `.deps/` state and must
not be committed.

## TypeSpec Contract Toolchain

Contract generation uses Node 24 and exact npm 11.16.0 as build/test tools;
neither is a Geometer runtime dependency. Provision and restore the pinned
toolchain from the repository root:

```powershell
npm install --global npm@11.16.0
if ((npm --version).Trim() -ne "11.16.0") { throw "npm version mismatch" }
npm ci
```

Regenerate the committed normalized catalog, JSON Schemas, and styled HTML
reference after editing `src/tsp/geometer/`, then run the deterministic
freshness check:

```powershell
npm run generate:contracts
npm run check:contracts
```

The check compiles with warnings as errors, validates the normalized catalog,
cross-checks pilot roots and operations against the promotion manifest,
rejects stale, missing, unexpected, unlinked, or externally dependent generated
files, verifies vendored documentation assets, and replays the governed raw
contract vectors under `tests/contracts/vectors/`. Use `npm run generate:docs`
or `npm run check:docs` for a focused documentation-only pass. See
[../design/typespec-toolchain.md](../design/typespec-toolchain.md) for authority,
supported constructs, identities, and output paths.

## Workspace Copy Setup

For agent workspaces or downstream monorepo workspaces, prefer copying an
already-prepared Geometer checkout when one is available locally. A prepared
checkout includes:

- `.deps/` with native OCCT state and WASM emsdk/OCCT state.
- `build/` for the native CMake build.
- `build-wasm/` for the Emscripten build.
- `dist/` with the committed/runtime artifacts.

On this development machine, the prepared checkout may live at
`C:\ELI\geometer`. Copying that directory into a sibling workspace preserves the
expensive dependency builds and allows fast API iteration. A fresh clone is still
valid, but the first native/WASM dependency build can take tens of minutes.

After copying into a workspace:

```powershell
git status --short --branch
.\dist\native\windows-x64\geometer.exe --version
node .\dist\wasm\node-test\geometer-node-test.js --version
```

Do not run `python scripts\build_occt.py --clean`,
`python scripts\build_wasm.py --clean`, or delete `.deps/` unless intentionally
refreshing dependencies. Those operations remove the cached dependency builds.

## Prerequisites

Native builds require:

- Git.
- Python 3.
- uv.
- CMake 3.24 or newer.
- Ninja.
- A C++17 compiler toolchain.

On Windows, use a Visual Studio developer environment or another shell where the
selected C++ compiler is available to CMake. The default CMake preset uses
Ninja. The OCCT dependency is built with the active native compiler for that
platform, so use the same shell consistently for configure/build/validation.
Binary-cache keys and local install markers include the native toolchain ABI and
complete build-recipe hash; a compiler, configuration, or recipe change cannot
silently reuse a stale local install. The published `windows-x64` OCCT 8.0.1
cache is an MSVC v143-ABI install and must be consumed from a Visual Studio
Developer PowerShell or Developer Command Prompt. MinGW and other compiler
families use a different key and cannot restore the MSVC archive.

On WSL2/Linux, install the usual build toolchain first. For Debian/Ubuntu
distros, the minimum package set is:

```bash
sudo apt update
sudo apt install -y build-essential git cmake ninja-build python3 python3-venv
```

On macOS, install equivalent tools with Homebrew:

```bash
brew install cmake ninja python
```

WASM builds additionally require enough disk space for emsdk and a WASM OCCT
build. The script manages emsdk locally under `.deps/`.

## Native Build

From the repository root:

```powershell
uv sync --group dev
cmake --preset default
cmake --build build --config Release
```

From WSL2/Linux/macOS, the same preset is intended to work:

```bash
uv sync --group dev
cmake --preset default
cmake --build build --config Release
```

The native validation script runs the native build, verifies the
platform-specific `dist/native/<platform>/geometer` executable, projects the
SOT-23 STEP fixture to JSON/SVG/GLB, exercises the source-checkout Python wrapper
through `GEOMETER_EXE`, checks Linux dynamic dependencies where applicable, and
runs CTest:

```bash
uv run python scripts/validate_native.py
```

Pass `--skip-ctest` to run only the build, CLI, source-checkout Python-wrapper,
and dependency checks.

On first configure, CMake looks for OCCT at:

```text
.deps/native/<platform>/occt-install/lib/cmake/opencascade
```

If OCCT is missing, top-level CMake automatically invokes:

```powershell
python scripts\build_occt.py
```

That script uses vendored RapidJSON, checks the public binary dependency cache,
and otherwise clones OCCT, builds OCCT as static libraries, and installs it into
`.deps/native/<platform>/occt-install/`. The first uncached source build is
slow. Later configures reuse that platform-specific `.deps/` state and should be
fast.

Normal local and CI builds use public HTTPS reads from:

```env
GEOMETER_OCCT_BINARY=auto
GEOMETER_OCCT_CACHE_PUBLIC_BASE_URL=https://artifacts.wavenumber.net
```

`GEOMETER_OCCT_BINARY` accepts:

- `auto` - use public cache, then any configured signed R2 fallback, then
  source.
- `off` - ignore binary caches and build from source.
- `only` - require a binary cache hit and fail otherwise.

Cache recipe keys cover structured CMake definitions and explicit semantic
values that can change the installed OCCT bytes. The same definition records
emit the CMake `-D` arguments and feed the recipe hash. Workspace paths, build
parallelism, orchestration scripts, cache transport code, and the indirect
dependency-version file are not hashed. The selected OCCT repository/tag,
platform, configuration, library type, Emscripten version when applicable, and
platform baselines are included directly. Native profiles also identify the
compiler family, ABI-relevant major version, and C++ runtime/ABI selection;
compiler patch releases do not rotate the key.
Previously accepted OCCT 8.0.1 archives may be reached only through exact
profile, destination-recipe, and SHA aliases; there is no generic stale-cache
fallback. Reviewed local marker-only recipe transitions are also exact and
one-way: all non-recipe profile fields and the installed OCCT version must
already match, and the install tree is retained without a rebuild.

R2 credentials are only needed for producer uploads or explicit private fallback
testing. Copy `.env.example` to `.env` for those cases and fill the `R2_*`
values locally.

Root `.env` is for local development only and must not be present when running
release signoff or `wn-dev-std check`. Move it to an ignored local location, or
set the same variables in the shell environment before signoff.

The vendored RapidJSON v1.1.0 copy includes Geometer's small modern Clang
compatibility patch so OCCT's GLTF toolkit compiles in native and WASM builds.

Build outputs are copied into `dist/native/<platform>/` after a successful
native build. The CLI post-build step also writes
`geometer.build-attestation.json` beside the build-tree executable and copies
it beside the distributed executable. The deterministic sidecar binds the
exact executable SHA-256 and version/C ABI to compiler, platform, build type,
OCCT tag, CMake generator, and Git source state. Validate it directly with:

```powershell
uv run python scripts/native_build_attestation.py validate --executable dist/native/windows-x64/geometer.exe
```

Dirty worktrees produce diagnostic sidecars but cannot set the qualification
promotion field `build_provenance_attested` to true. Release qualification
therefore requires a rebuild from a clean authoritative Git worktree.
Current platform directory names use `windows-x64`, `linux-x64`, `linux-arm64`,
and `macos-arm64`.

Common native CLI commands:

```powershell
.\dist\native\windows-x64\geometer.exe --version
.\dist\native\windows-x64\geometer.exe step-to-glb input.step output.glb
.\dist\native\windows-x64\geometer.exe step-project-hlr input.step output.json
.\dist\native\windows-x64\geometer.exe step-project-svg input.step output.svg --mode outline --view top
.\dist\native\windows-x64\geometer.exe init-request request.json --step input.step --operation step_hlr_projection_json --output output.json
.\dist\native\windows-x64\geometer.exe run request.json response.json
.\dist\native\windows-x64\geometer.exe planar-batch-solve request.bin response.bin --warmup 1 --repeat 5 --metrics metrics.json
```

The Python package uses the native CLI by default. From a source checkout,
`GEOMETER_EXE` is optional if `dist/native/<platform>/geometer(.exe)` exists.
The old Python wheel direction based on `geometer.dll` plus OCCT `TK*.dll`
runtime files is retired; `dist/` should not persist those files.

The PyPI distribution name is `wn-geometer`; the import package remains
`geometer`.

To build a local Python wheel, first build the native CLI so
`dist/native/<platform>/geometer(.exe)` exists. Then run:

```powershell
python -m build --wheel --outdir out\wheelhouse
python -m twine check out\wheelhouse\*.whl
```

The package validation script builds the local wheel, installs it into a clean
temporary environment, verifies that Python resolves the bundled executable from
inside the installed package, imports the public analytic DTO/client surface,
executes empty and nontrivial analytic IPC calls against that bundled
executable, verifies the generated `geometer` console script, and runs the
headless package example:

```powershell
uv run python scripts\validate_python_package.py
```

The wheel build copies the platform executable into
`geometer/native/<platform>/` inside the wheel, exposes the `geometer` console
script, and marks the wheel platform-specific. The Windows executable wheel
should use a `py3-none-win_amd64` tag because it contains no CPython extension
module.

### macOS arm64 wheels

Geometer currently publishes Apple Silicon macOS wheels and does not publish an
Intel macOS wheel. The standard macOS release target is:

```text
py3-none-macosx_11_0_arm64
```

The release scripts default Darwin native and wheel builds to
`MACOSX_DEPLOYMENT_TARGET=11.0`. When changing the target, rebuild OCCT and the
native CLI from clean generated state because static OCCT objects carry their
own Mach-O minimum OS metadata:

```bash
python scripts/build_occt.py --clean
rm -rf build-native-macos-arm64
uv run python scripts/validate_native.py
uv run python scripts/validate_python_package.py --skip-native-validation
```

Before uploading, verify both the filename and the bundled executable:

```bash
otool -l dist/native/macos-arm64/geometer | rg -A5 'LC_BUILD_VERSION|LC_VERSION_MIN_MACOSX'
python -m twine check out/wheelhouse/macos-arm64/wn_geometer-2026.8.21-py3-none-macosx_11_0_arm64.whl
```

The Mach-O `minos` value must not be newer than the wheel platform tag. Do not
retag an existing macOS wheel to a lower version without rebuilding native
artifacts.

Treat GitHub Actions `macos-latest` as a moving CI label, not as a release
target. GitHub announces `macos-latest` migrations and rolls them out gradually,
so normal maintenance is to keep the Geometer wheel target pinned to the
supported floor (`macosx_11_0_arm64`) and test that wheel on the current
`macos-latest` runner. A `macos-latest` migration only requires repackaging if
the wheel install or native smoke test fails on the new hosted runner, or if a
new SDK/toolchain raises the Mach-O `minos` above the published wheel tag.

For WSL/Linux release wheels, build on the Ubuntu 22.04 release runners. The
expected Linux x64 wheel tag is `manylinux_2_35_x86_64`; the Linux ARM64 tag is
`manylinux_2_35_aarch64`. Do not retag a wheel to a lower glibc baseline
without rebuilding the bundled native executable and OCCT dependency tree on
that baseline.

```bash
uvx --from auditwheel --with patchelf auditwheel show out/wheelhouse/linux-x64/wn_geometer-*.whl
```

`auditwheel show` should not report a newer glibc floor than the wheel filename
tag.

PyPI upload commands:

```powershell
# Preflight metadata.
python -m twine check out\wheelhouse\windows-x64\wn_geometer-2026.8.21-py3-none-win_amd64.whl out\wheelhouse\linux-x64\wn_geometer-2026.8.21-py3-none-manylinux_2_35_x86_64.whl out\wheelhouse\macos-arm64\wn_geometer-2026.8.21-py3-none-macosx_11_0_arm64.whl

# Optional dry-run project on TestPyPI.
python -m twine upload --repository testpypi out\wheelhouse\windows-x64\wn_geometer-2026.8.21-py3-none-win_amd64.whl out\wheelhouse\linux-x64\wn_geometer-2026.8.21-py3-none-manylinux_2_35_x86_64.whl out\wheelhouse\macos-arm64\wn_geometer-2026.8.21-py3-none-macosx_11_0_arm64.whl

# Public PyPI release.
python -m twine upload --repository pypi out\wheelhouse\windows-x64\wn_geometer-2026.8.21-py3-none-win_amd64.whl out\wheelhouse\linux-x64\wn_geometer-2026.8.21-py3-none-manylinux_2_35_x86_64.whl out\wheelhouse\macos-arm64\wn_geometer-2026.8.21-py3-none-macosx_11_0_arm64.whl
```

For token-based upload, set `TWINE_USERNAME=__token__` and put the PyPI or
TestPyPI API token in `TWINE_PASSWORD`, or use an equivalent `.pypirc`/keyring
setup. Do not write upload tokens into the repository.

The current release target is `wn-geometer==2026.8.21`; callers install
`wn-geometer==2026.8.21` and import `geometer`.

For local token setup, copy `.env.example` to `.env`, fill the token values,
and keep `.env` out of version control.

## Manual OCCT Rebuild

Use this when changing the pinned OCCT version or when the local OCCT build is
suspect:

```powershell
python scripts\build_occt.py --clean
python scripts\build_occt.py
cmake --preset default
cmake --build build --config Release
```

`--clean` removes OCCT build/install state for the current native platform under
`.deps/native/<platform>/`; it does not remove vendored RapidJSON, the shared
OCCT source checkout, or the Geometer `build/` directory. Add `--clean-source`
only when intentionally refreshing the shared OCCT source checkout too.

To inspect the dependency cache key without building:

```powershell
python scripts\build_occt.py --print-binary-cache-key
python scripts\build_wasm.py --print-occt-binary-cache-key
```

For a side-by-side kernel upgrade decision, use the isolated exact-tag
qualification harness rather than editing `dependency_versions.py` between
builds. See [OCCT qualification](occt-qualification.md). It keeps dependency
state below `.deps/occt-qualification/<tag>/`, build evidence below
`out/occt-qualification/<tag>/`, and committed `dist/` artifacts untouched.

The trusted GitHub workflow `.github/workflows/occt-deps.yml` publishes OCCT
archives to R2. Normal CI and release workflows consume the public artifact
cache and do not need R2 secrets. When the printed keys are missing from
`https://artifacts.wavenumber.net/deps/v1/geometer/occt/`, run the `OCCT
Dependency Cache` workflow with `target=all` to publish the current generation.

The public Python package uses the executable backend only. Keep ctypes/native
loading experiments out of the normal wheel and application path unless a future
ADR explicitly reopens that backend.

## Analytic Production Qualification

Use an existing Release executable to replay the governed analytic request
through production IPC without rebuilding OCCT:

```powershell
uv run python scripts\qualify_analytic_planar_boolean.py --power-mode balanced
```

The default governed request is synthetic cross-transport evidence. Supply an
external corpus with `--corpus`; its `external_real_board` cases require source
and exporter identity, source digest, exporter revision, explicit qualification
redistribution authorization, and license scope. Reports are written below
`out/analytic-qualification/` by default. Process RSS remains a separately
labelled external envelope.

Corpus request paths are contained relative paths. Canonical lowercase `.hex`
files and exact raw `.gmabrq01` files are supported; the suffix selects the
decoder, and magic/accounting mismatches fail closed. Replay the governed RT
local candidate without duplicating its raw bytes as hex:

```powershell
uv run python scripts\qualify_analytic_planar_boolean.py `
  --corpus tests\contracts\vectors\analytic\real-board\rt_super_c1_pwr4\corpus.json `
  --executable build\src\cpp\cli\geometer.exe `
  --telemetry-helper build\tests\cpp\geometer_analytic_solver_telemetry_helper.exe `
  --warmup-count 1 --repeat-count 2 --require-target --require-solver-telemetry
```

The checked-in report is a local Ryzen 9 5950X, dirty-build observation. It is
not a clean-build promotion record. Its machine identity and the 1-second/
512-MiB target remain comparative observations rather than release gates.

Native builds also produce the test/qualification-only
`geometer_analytic_solver_telemetry_helper` target. The harness discovers it
only from `--telemetry-helper`, `GEOMETER_ANALYTIC_TELEMETRY_HELPER`, or the
canonical current-workspace native build paths. Use the required gate explicitly:

```powershell
uv run python scripts\qualify_analytic_planar_boolean.py --require-solver-telemetry
```

Release runs must additionally pass
`--require-promotion-attested`; the command then exits unsuccessfully unless
the executable sidecar matches the current authoritative clean source and a
verified resolved OCCT install profile, and any external real-board case passes
the portable expected-result, repeat, byte-equality, zero-failure/fallback, and
5-second/1-GiB hard-ceiling gates.

An external real-board corpus enables this requirement automatically. Missing,
unexecutable, or incompatible helpers fail closed. For every measured run the
helper consumes the identical `GMABRQ01` bytes through
`decode_analytic_request_packet` and `build_analytic_filtered_batch`; the
harness rejects its counters unless its complete result packet is byte-for-byte
equal to the production executable IPC result.

Auto-discovered helpers must also be newer than their adjacent `geometer_lib`
artifact, and that library must be newer than the current native solver source
closure. A stale build is rejected with the exact helper target to rebuild.

The report schema is
`wn.geometer.analytic_planar_boolean_qualification.a2`; the internal counter
schema is `wn.geometer.analytic_solver_telemetry.a0`. Report identity binds the
request and expected-result digests, production executable/toolchain profile,
helper executable SHA-256, telemetry schema, machine profile, run counts, power
mode, and target policy. Runtime timing, RSS, and counter observations are not
identity inputs. Per-job `emitted_bytes` is the logical encoded footprint of
that isolated job before canonical batch merging and shared-table
deduplication; batch `emitted_bytes` is the exact canonical result-packet size.
The helper is neither distributed nor a public operation/API, and it does not
change the A0 request/result wire. Internal telemetry alone cannot claim the
real-board promotion gate: build-attested executable provenance is a separate
required gate. Machine identity is recorded for comparison but does not affect
portable production eligibility. Missing
sidecars fall back to explicitly un-attested workspace hints; invalid or stale
adjacent sidecars fail closed. Every external real-board
case also needs a governed expected-result SHA-256; deterministic self-output
without that authority remains comparative evidence only. Promotion also needs
at least two production-result and byte-matched telemetry observations; a
single run is permitted for smoke/diagnostic use but cannot establish the
promotion determinism gate.

## WASM Build

From the repository root:

```powershell
python scripts\build_wasm.py
```

This script:

1. Clones and activates pinned emsdk under `.deps/emsdk/`.
2. Uses vendored RapidJSON and reuses or clones OCCT source under `.deps/`.
3. Cross-compiles OCCT to `.deps/occt-wasm-install/`.
4. Builds Geometer in `build-wasm/`.
5. Copies the full browser/Web Worker C ABI outputs `geometer.js` /
   `geometer.wasm` into `dist/wasm/browser/`. This is the official application
   integration WASM and includes OCCT-backed STEP/HLR/GLB plus planar byte APIs.
6. Copies the Node CLI parity/test outputs `geometer-node-test.js` /
   `geometer-node-test.wasm` into `dist/wasm/node-test/` and writes its local
   CommonJS package boundary for direct execution beneath the repository's ESM
   root.
7. Copies the planar-only browser C ABI outputs `geometer-planar-browser.js` /
   `geometer-planar-browser.wasm` into `dist/wasm/planar-browser/`. This
   smaller build intentionally excludes OCCT/STEP and is retained for
   planar-only browser workers.
8. Writes `dist/README.md`.

To remove WASM-specific generated state:

```powershell
python scripts\build_wasm.py --clean
```

The Node CLI target uses filesystem access for command-line parity. The full
browser target is modularized and exports the flat C ABI entry points
`geometer_step_hlr_projection_json_bytes` and `geometer_step_to_glb_bytes` for
direct byte-buffer calls from JavaScript or a Web Worker.

The full browser target also exports `geometer_version_string` and
`geometer_abi_version`. Downstream browser consumers should check those before
depending on a specific ABI. Earlier pre-date ABI integers tracked planar batch,
diagnostic, and triangulation additions; current releases use the ADR 006
date-based ABI generation, for example `20260821`.

## Versioning

Geometer follows [ADR 006](../geometer/adr/geometer-adr-006-date_based_versioning_policy.md).
The current release identity is `v2026-08-21`; the CMake/PyPI package version
is `2026.8.21`; the C ABI generation is `20260821`.

The root `CMakeLists.txt` declares `GEOMETER_RELEASE_DATE`,
`GEOMETER_RELEASE_VERSION`, and `GEOMETER_ABI_VERSION`. The root
`pyproject.toml` package version must match `GEOMETER_RELEASE_VERSION`.
Generated build metadata must use UTC. Rebuild the persisted `dist/` artifacts
when version or interface values change.

## Embedded Model Viewer

To refresh the copied STEP fixtures, GLB display meshes, and manifest from an
embedded model folder:

```powershell
powershell -NoProfile -ExecutionPolicy Bypass -File scripts\prepare_embedded_model_fixtures.ps1
```

The script copies STEP/STP files into `tests/fixtures/step/embedded_models/`,
converts each one to GLB under `tests/fixtures/glb/embedded_models/`, and writes
`tests/fixtures/embedded_models_manifest.json`.

Serve the repository root and open the viewer:

```powershell
python -m http.server 8123 --bind 127.0.0.1
```

`http://127.0.0.1:8123/examples/wasm/embedded_model_viewer.html`

The source viewer loads the GLB for the 3D pane and sends the matching STEP
bytes to the browser WASM HLR API for the projection pane. The maintained HLR
Lab also accepts local STEP uploads, keeps OCCT work in a Worker, and uses the
shared TypeScript panel system.

Build the self-contained artifact and its deploy-unchanged review directory:

```powershell
python scripts\build_hlr_site.py
python -m http.server 8123 --bind 127.0.0.1 --directory dist\wasm\demos\hlr
```

Open `http://127.0.0.1:8123/`. The page runtime is only `index.html`; the
adjacent `_headers` and `asset-manifest.json` files are deployment and closure
metadata. This command builds and serves locally—it does not publish. See
[Browser demo packaging and UI](../design/browser-demos.md) before adding or
hosting another demo.

The generated TypeScript model-bounds example uses the packaged high-level
Worker client and the same full-browser WASM artifact. It keeps synchronous
OCCT execution off the window event loop:

```powershell
npm run generate:contracts
python -m http.server 8123 --bind 127.0.0.1
```

`http://127.0.0.1:8123/examples/wasm/model_bounds_demo.html`

The HLR timing page runs the same browser worker projection path across the
fixture set and reports STEP fetch-to-bytes timing separately from HLR timing:

`http://127.0.0.1:8123/tests/wasm/hlr_benchmark.html`

## Tests

After a native CMake build:

```powershell
ctest --test-dir build -C Release --output-on-failure
```

After a WASM build, validate the generic operation ABI/model-bounds round trip,
the retained STEP-to-GLB export, and the packed planar export:

```powershell
node tests\wasm\operation_contract_validation.js
node tests\wasm\step_to_glb_bytes_validation.js
node tests\wasm\planar_batch_solve_bytes_validation.js
```

Validate generated TypeScript codecs, a clean packed consumer, and the direct
plus real Worker-thread high-level WASM clients:

```powershell
npm run check:typescript
uv run pytest tests\typescript -q
```

Validate generated Rust codecs, formatting/lints, exact A0 framing, a clean
packaged-crate consumer running friendly analytic IPC against the platform
`dist` executable, and live persistent native-process model bounds:

```powershell
node scripts\generate-rust-contracts.mjs --check
cargo fmt --manifest-path src\rust\geometer-client\Cargo.toml --all -- --check
cargo clippy --manifest-path src\rust\geometer-client\Cargo.toml --all-targets --locked -- -D warnings
wn-dev-std audit src\rust\geometer-client --scope language
uv run pytest tests\rust -q
```

To benchmark the browser C ABI planar batch solver against a packed request:

```powershell
node tests\wasm\planar_batch_solve_bytes_benchmark.js request.bin response.bin --warmup 1 --repeat 5 --metrics metrics.json
```

To run the same benchmark in headless Chrome, including the Web Worker path:

```powershell
python tests\wasm\planar_batch_solve_bytes_chrome_benchmark.py request.bin --worker --warmup 1 --repeat 5 --metrics metrics.json
```

The Rack metadata under `tests/` describes test strata, but the C++ tests are
registered through CTest.

Current limitation: the top-level CMake configure always resolves OCCT first.
That means even low-level tests need the native dependency setup when run
through CMake. For isolated low-level work before `.deps/` exists, direct
compile checks are acceptable, but they are not a replacement for the CMake/CTest
path before publishing changes.

Example isolated contour test compile:

```powershell
New-Item -ItemType Directory -Force out\developer | Out-Null
clang++ -std=c++17 -Isrc\cpp\lib src\cpp\lib\planar_contours.cpp tests\cpp\planar_contours_test.cpp -o out\developer\geometer_planar_contours_test.exe
.\out\developer\geometer_planar_contours_test.exe
Remove-Item .\out\developer\geometer_planar_contours_test.exe
```

## Formatting And Checks

Format touched C++ files with the repository `.clang-format`:

```powershell
clang-format -i <files>
```

Useful lightweight checks:

```powershell
git diff --check
clang++ -std=c++17 -Isrc\cpp\lib -fsyntax-only <files>
```

Run the full native build and CTest path before treating C++ changes as ready.

Release signoff also requires:

```powershell
uv sync --group dev
uv run pytest tests\L99_release -q
uv run python scripts\validate_native.py
uv run python scripts\validate_python_package.py
```

The L99 release stratum includes Ruff, Pyright, `uv lock --check`,
clang-format dry-run, Lizard complexity checking, and repository hygiene checks.

## Common Troubleshooting

If CMake cannot find OCCT:

```powershell
python scripts\build_occt.py
cmake --preset default
```

If OCCT configure/build state looks stale:

```powershell
python scripts\build_occt.py --clean
python scripts\build_occt.py
```

If CMake cached the wrong dependency path, remove or recreate `build/` and
configure again:

```powershell
Remove-Item -Recurse -Force .\build
cmake --preset default
```

Before deleting generated directories, verify the path is inside this repository.
Never delete `.deps/` or `build/` paths computed from an untrusted variable.
