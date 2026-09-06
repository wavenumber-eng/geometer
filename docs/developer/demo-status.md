# Demo And Example Audit

Audited 2026-09-05 using committed `v2026-09-04` Windows x64/native and
browser artifacts. Owner for all retained entries: Geometer maintainers;
application-specific consumers retain their own support responsibility.
Passing a demo does not promote its solver. No demo has been deleted.

See [native demo follow-ups](native-demo-followups.md) for remaining C++ GPU
requirements, deferred UI acceptance and the separate Mac agent handoff.

User decision on 2026-09-05: retain all 11 demos/examples for now. Pruning is
deferred to a separate explicitly approved change. Retention does not alter
experimental maturity or resolve the verification gaps recorded below.

Native GUI follow-up on 2026-09-05: user inspection found an angled-view
occlusion defect in the C++ preview. Its demo renderer now uses cached per-pixel
depth testing instead of centroid sorting. Both GUIs expose fast detail and
fast mesh-shadow selectors, enabled by default. The C++ example was rebuilt
against unchanged kernel sources using cached OCCT; no new kernel release is
implied. Updated windows were launched for user recheck; visual acceptance of
the repaired angle remains pending.

## Runtime Evidence And Disposition

The optional [Native Rust API Lab](../../examples/rust/native_viewer/README.md)
is a twelfth entry added on the native API feature branch, not a replacement
for the 11 retained demos. It uses the maintained Rust executable client and
TypeSpec-generated contracts. Windows Radeon RX 7600 XT/Vulkan smoke produced
colored STEP triangles, native illustrated SVG and separate Fast HLR layers;
the app-window screenshot was inspected. The user accepted this Rust workflow
proof; further GUI polish, exhaustive mouse/docking/resize checks and macOS
build/Metal testing are deferred. The 3D preview is explicitly
opaque; native SVG preserves source opacity. Public packaging is deferred.

The [Lab parity analysis](native-illustration-lab-parity.md) identifies raw
mesh lines drawn over foreground surfaces: the browser Lab disables them and
supplies HLR lines, whereas the initial Rust demo enabled raw overlays. Rust
now defaults those diagnostics off and exposes supported Lab shading, color,
line, Fast crease/seam and tessellation controls. Native combined HLR/SVG now
matches the web Lab's visibility-filtered detail/outline composition: toggles
affect preview and original SVG export. The white centered result pane no longer
shifts with job status, and Save SVG is next to its selector. Experimental
browser AO remains explicitly unavailable, not a hidden fallback.

| Demo / audience | Source | Verification on this host | Approved disposition: retain for now |
| --- | --- | --- | --- |
| Native Rust API Lab / executable-backed Rust consumers | [source](../../examples/rust/native_viewer/src/main.rs), [guide](../../examples/rust/native_viewer/README.md) | Native STEP/illustration/Fast HLR smoke and GPU screenshot inspected on Windows; focused camera/opaque-material/epoch/raster tests. User accepted workflow proof; exhaustive UI and Mac checks deferred. | Keep separate from direct-linked C++; optional Cargo crate, not release-packaged. |
| Model bounds / generated browser consumers | [page](../../examples/wasm/model_bounds_demo.html) | Real Worker-client validation passed; page interaction not separately tested. | Keep as generated-contract pilot. |
| HLR Lab / browser integration | [page](../../examples/wasm/embedded_model_viewer.html) | Chrome upload, projection and SVG export passed. | Keep primary browser demo. |
| Illustration Lab / package evaluation | [page](../../examples/wasm/illustration_demo.html) | Chrome mesh/render/upload/export passed. | Keep evaluation demo; not a production renderer application. |
| Analytic polygon pour / solver research | [page](../../examples/wasm/analytic_polygon_pour_demo.html) | Standalone Chrome runtime test passed. | Retain as experimental research. |
| PCB polygon pour / abandoned application direction | [page](../../examples/wasm/pcb_polygon_pour_demo.html) | Standalone interaction test passed. | Retain as experimental research; any later pruning requires separate approval. |
| Planar ring solver / packed polygon example | [page](../../examples/wasm/planar_ring_solver_demo.html) | Headless Chrome loaded the committed standalone page and solved: 1 region, 2 holes, 27.223347 square mm, runtime 2026.9.4 / ABI 20260904. | Keep provisionally as a working packed polygon example; do not prune solely for age. |
| Native C++ HLR preview / direct embedding | [source](../../examples/cpp/hlr_preview.cpp) | User inspected the original GUI and reported depth-order artifacts. Updated demo built/launched; focused depth regression passed in 0.01 seconds. Corrected angled-view appearance awaits user recheck. | Keep; repair remains subject to visual acceptance. |
| Python headless HLR/SVG / Python consumers | [source](../../examples/python/step_hlr_svg.py) | Executed against committed binary; produced projection JSON, SVG and GLB. | Keep primary Python example. |
| Python PyVista/Qt viewer / optional GUI experiment | [source](../../examples/python/pyvista_hlr_viewer.py) | Existing GUI environment passed off-screen rendering (53 meshes/bounds), then user inspection. Follow-up Qt control smoke switched fast/poly/exact detail and all outline choices, returning nonempty geometry; updated GUI relaunched for user recheck. | Keep optional GUI experiment; no dependency upgrade required. |
| Node topology reference / experimental IPC consumers | [source](../../examples/node/step_topology_annotation_reference.ts), [guide](../../examples/node/README.md) | Native open/inspect/group/probe/checkpoint/restart/replay passed. | Keep clearly experimental, not the introductory IPC example. |
| Node model-bounds quick start / introductory IPC | [source](../../examples/node/ipc-model-bounds.mjs) | Native handshake, discovery, STEP attachment, bounds and graceful close passed. | Keep minimal introductory IPC example. |

## Reproduce The Focused Audit

Commands run from the repository root; no native/OCCT rebuild is required:

```powershell
uv run pytest tests/wasm/test_hlr_static_site.py tests/wasm/test_illustration_static_site.py tests/wasm/test_analytic_standalone_demo.py tests/wasm/test_pcb_standalone_demo.py -q
node tests/typescript/worker_client_validation.mjs
uv run python examples/python/step_hlr_svg.py tests/fixtures/step/embedded_models/SOT-23.STEP --out-dir out/docs-cleanup/python-headless
node dist/native/examples/step-topology-annotation-reference.mjs dist/native/windows-x64/geometer.exe tests/fixtures/step/embedded_models/SOT-23.STEP
node examples/node/ipc-model-bounds.mjs dist/native/windows-x64/geometer.exe tests/fixtures/step/embedded_models/SOT-23.STEP
```

The four Chrome tests passed in 25.62 seconds on this host. These observations
are not cross-platform certification or native build attestation. GUI and
model-bounds page-interaction limitations above remain explicit.

For the successful PyVista off-screen replay, an existing GUI interpreter was
reused without modifying its environment. `PYTHONPATH` selected this checkout's
`python/` and `GEOMETER_EXE` selected its committed Windows executable, then
the documented `--off-screen-validate` command produced
`out/docs-cleanup/pyvista-preview.png`. A fresh GUI environment should use the
locked setup below; interactive controls were not exercised.

## Build, Launch And Distribution Map

| Family | Build when sources change | Launch/output and constraints |
| --- | --- | --- |
| Model bounds | `npm run generate:contracts` | Serve repository root; `examples/wasm/model_bounds_demo.html` loads committed JS/Worker/WASM. |
| HLR / illustration | `python scripts/build_hlr_site.py` / `python scripts/build_illustration_site.py` | `dist/wasm/demos/hlr/` / `illustration/` hosted outputs; corresponding standalone HTML also committed. |
| Analytic / PCB | `python scripts/build_analytic_polygon_pour_site.py` / `python scripts/build_pcb_polygon_pour_site.py` | `dist/wasm/demos/analytic-polygon-pour/` / `pcb-polygon-pour/`; experimental hosted and single-file outputs. |
| Planar ring | `python scripts/build_standalone_demos.py planar-ring` | `dist/wasm/demos/planar_ring_solver_demo.html`; source page also works when serving repository root. |
| Native C++ preview | Optional CMake example target; [instructions](../../examples/cpp/README.md) | `dist/native/<platform>/geometer_hlr_preview(.exe)`; needs a working desktop/OpenGL stack. |
| Python headless | No demo compilation; installed package or source environment | Writes JSON/SVG/GLB under selected output directory; STEP fixture required. |
| Python GUI | `uv sync --project examples/python --locked` | [GUI/off-screen commands](../../examples/python/README.md); optional Qt/VTK dependency install required. |
| Node topology | `node scripts/build-node-reference-example.mjs` | Bundled `dist/native/examples/step-topology-annotation-reference.mjs`; compatible native binary required. |
| Node quick start | No compilation | Source imports committed ESM client; Node 24 and compatible native binary required. |

Browser fixtures include the repository's SOT-23 and larger embedded models.
Use `python -m http.server 8123 --bind 127.0.0.1` from the root for source
pages, or the hosted-directory commands in [demo packaging](browser-demos.md).
Rack metadata under `tests/wasm/` and `tests/typescript/` indexes existing
coverage; Python package validation exercises the headless example. Do not
treat registration as a successful runtime observation.

## Retention Decision And Future Pruning

All demos are retained by user decision; no source or committed output is
removed by this audit. Before any future pruning of PCB,
analytic, ring, or GUI demos, approve an explicit disposition and remove the
complete build/test/manifest/distribution reference set. Retention of research
evidence is separate from keeping every historical demo runnable.
