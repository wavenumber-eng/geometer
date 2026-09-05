+++
type = "plan"
id = "geometer-native-gpu-viewer"
status = "active"
created = "2026-09-05"

[[steps]]
id = "baseline-inventory"
title = "Inspect native viewer, web illustration, export contracts and platform build paths"
status = "done"

[[steps]]
id = "architecture-and-parity"
title = "Approve viewer boundaries, web parity matrix and shared illustration execution strategy"
status = "done"
depends_on = ["baseline-inventory", "rust-demo-scope"]

[[steps]]
id = "rust-demo-scope"
title = "Retain separate direct-linked C++ and executable-backed Rust demos; prioritize Rust"
status = "done"
depends_on = ["baseline-inventory"]

[[steps]]
id = "native-illustration-windows"
title = "Implement the native Windows operation and typed Rust STEP/mesh workflow"
status = "done"
depends_on = ["architecture-and-parity"]

[[steps]]
id = "rust-demo-foundation"
title = "Build the Rust/wgpu shell, responsive controls and managed GeometerClient connection"
status = "pending"
depends_on = ["architecture-and-parity"]

[[steps]]
id = "rust-demo-integration"
title = "Demonstrate typed native geometry and illustration with progress, GPU preview and JSON/SVG exports"
status = "pending"
depends_on = ["rust-demo-foundation", "native-illustration-windows"]

[[steps]]
id = "rust-windows-acceptance"
title = "Validate the priority Windows Rust API demonstration with the user"
status = "pending"
depends_on = ["rust-demo-integration"]

[[steps]]
id = "python-native-illustration"
title = "Accept required executable-backed Python illustration methods and installed-package tests"
status = "pending"
depends_on = ["native-illustration-windows"]

[[steps]]
id = "rust-macos-build-path"
title = "Prepare a macOS arm64 Rust/wgpu build and compatible native executable"
status = "pending"
depends_on = ["rust-demo-foundation"]

[[steps]]
id = "rust-macos-agent-validation"
title = "Have a separate Mac agent validate the matching Rust demo through the native client"
status = "pending"
depends_on = ["rust-macos-build-path", "rust-windows-acceptance"]

[[steps]]
id = "gpu-foundation"
title = "Later C++ track: pin docking-capable ImGui and implement SDL GPU/shaders"
status = "pending"
depends_on = ["architecture-and-parity", "rust-windows-acceptance"]

[[steps]]
id = "viewport-camera"
title = "Replace software preview with GPU mesh rendering and stable orbit/pan/zoom"
status = "pending"
depends_on = ["gpu-foundation"]

[[steps]]
id = "dock-layout"
title = "Create a responsive left/right controls dock and result-focused workspace"
status = "pending"
depends_on = ["gpu-foundation"]

[[steps]]
id = "background-jobs"
title = "Move loading and geometry work off the UI thread with accurate busy states"
status = "pending"
depends_on = ["architecture-and-parity", "dock-layout"]

[[steps]]
id = "geometry-svg-exports"
title = "Export governed geometry JSON and vector SVG from coherent completed results"
status = "pending"
depends_on = ["viewport-camera", "background-jobs"]

[[steps]]
id = "native-illustration-api"
title = "Accept governed native illustration with typed Rust/Python APIs and complete STEP workflows"
status = "pending"
depends_on = ["native-illustration-windows", "python-native-illustration"]

[[steps]]
id = "illustration-integration"
title = "Consume native Geometer illustration with existing styles and tested web SVG parity"
status = "pending"
depends_on = ["geometry-svg-exports", "architecture-and-parity", "native-illustration-api"]

[[steps]]
id = "windows-acceptance"
title = "Validate the later direct-linked C++ Windows viewer with the user"
status = "pending"
depends_on = ["illustration-integration", "dock-layout"]

[[steps]]
id = "macos-build-path"
title = "Produce the later C++ macOS arm64 Metal build and handoff package"
status = "pending"
depends_on = ["viewport-camera", "dock-layout"]

[[steps]]
id = "macos-agent-validation"
title = "Have a separate Mac-equipped agent verify the later C++ build"
status = "pending"
depends_on = ["macos-build-path", "windows-acceptance"]

[[steps]]
id = "design-doc-intent-audit"
title = "Audit durable design and requirements against user intent and actual implementation"
status = "pending"
depends_on = ["rust-windows-acceptance", "rust-macos-agent-validation", "native-illustration-api", "windows-acceptance", "macos-agent-validation"]

[[steps]]
id = "test-runtime-impact-audit"
title = "Review changed tests, Rack registration and measured fast versus GUI-lane costs"
status = "pending"
depends_on = ["rust-windows-acceptance", "rust-macos-agent-validation", "native-illustration-api", "windows-acceptance", "macos-agent-validation"]

[[steps]]
id = "external-review"
title = "Independently review the implementation after design and runtime audits"
status = "pending"
depends_on = ["design-doc-intent-audit", "test-runtime-impact-audit"]

[[steps]]
id = "closeout"
title = "Record durable decisions, platform evidence and release-facing changes; retire this plan"
status = "pending"
depends_on = ["external-review"]

[[exit_criteria]]
id = "rust-consumer-proof"
title = "The separate Rust/wgpu app demonstrates typed GeometerClient calls to the native executable without direct kernel linkage"
status = "pending"

[[exit_criteria]]
id = "gpu-camera"
title = "Windows uses hardware depth testing and rotation does not silently refit or zoom"
status = "pending"

[[exit_criteria]]
id = "responsive-layout"
title = "Controls dock left or right, remain usable at audited DPI/sizes, and leave results the remaining area"
status = "pending"

[[exit_criteria]]
id = "responsive-jobs"
title = "Loading/recompute remain responsive with honest progress, stale-result protection and visible failures"
status = "pending"

[[exit_criteria]]
id = "exports"
title = "Geometry JSON and real vector SVG export the selected completed state using documented existing contracts"
status = "pending"

[[exit_criteria]]
id = "illustration-parity"
title = "Native illustration preserves accepted TypeScript semantics without a JavaScript/WASM workaround in the viewer"
status = "pending"

[[exit_criteria]]
id = "native-illustration-api"
title = "The governed native operation and typed Rust/Python STEP-to-illustration paths pass upstream four-platform acceptance"
status = "pending"

[[exit_criteria]]
id = "macos-handoff"
title = "A matching macOS arm64 build has recorded independent agent Metal/UI/export verification"
status = "pending"

[[exit_criteria]]
id = "design-doc-intent-audit"
title = "Required design documents match user intent and implemented behavior"
status = "pending"

[[exit_criteria]]
id = "test-runtime-impact-audit"
title = "New tests are listed and runtime impact is reviewed"
status = "pending"

[[exit_criteria]]
id = "external-review"
title = "Independent implementation review has no unresolved blocking findings"
status = "pending"
+++

# Native API And GPU Demonstrations Development Plan

## Objective And Scope

Prioritize callable native Geometer APIs and a Rust/wgpu demonstration of the
public executable-backed client. Retain the C++ viewer as a separate example
of direct native linking; its planned SDL GPU upgrade is the later track.
Both illustrate related workflows, not interchangeable integration boundaries.
The shared goals are a real GPU model, comfortable controls, responsive
geometry computation and reusable geometry/SVG outputs. Windows x64 is the
primary development and user-acceptance platform. Establish the macOS arm64
Metal build early; a separate Mac-equipped agent performs runtime validation.
Windows work need not wait for that agent, but a Mac build is not a Mac pass.

Execution approved 2026-09-05 on `feature/native-api-rust-demo`, branched from
`b7605c7`; the planning branch is preserved. The previous rough renderer-only
estimate does not cover the added docking, illustration, exports and jobs.

User clarification: native consumers need an actual Geometer illustration API,
not a JavaScript/WASM workaround. The [native illustration API issue brief](native-illustration-api.md)
is now owned by this feature branch: the other agent will use WASM until this
release is ready. TypeSpec remains the authority for new IPC definitions and
generated bindings. GPU, docking, camera, loading and HLR exports can proceed
independently; illustration integration requires reviewed native parity.

Python exposure is required alongside Rust. The user confirmed that the
[Rust/wgpu consumer demonstration](rust-consumer-demo.md) is a separate app,
not a replacement: it calls `geometer.exe` through `geometer-client`; the C++
app demonstrates direct linking. Rust and native API availability take priority.

Delivery order:

1. Reconcile published API availability and agree the upstream native
   illustration/STEP/client contracts. Implement/accept the Windows executable
   and Rust client slice first; generated DTOs do not count as runtime support.
2. Build the Rust/wgpu app and prove the complete typed geometry/illustration
   workflow on Windows. Its shell may develop alongside API work, but fixture
   mockups alone are not consumer acceptance.
3. Complete required Python exposure and four-platform native API qualification;
   prepare the Rust Mac build early and hand it to a separate Mac agent. Do not
   block the first Windows Rust checkpoint on later C++ UI work or completion of
   every platform's qualification. Record incomplete platform support honestly.
4. Continue the direct-linked C++/SDL GPU upgrade after the Rust Windows
   milestone. Share requirements/fixtures/output expectations, not a compulsory
   shared UI implementation. Preserve both demos and their distinct purposes.

The availability audit maps advertised/published operations to native C++,
executable catalog, Rust and Python invocation. Prioritize illustration, Fast
HLR and the model/mesh path needed by these consumers. Record other gaps as
follow-ups; this is not an implicit implementation of every TypeSpec migration.

Native demo inclusion in GitHub releases is a deferred user decision, not an
approved deliverable or a prerequisite for this plan's implementation closeout.
Build/test artifacts and the Mac handoff remain in scope. Do not automatically
add demo release uploads, installers, signing/notarization, or a bundled kernel
distribution. Preserve current repository artifact policy and existing demos;
this decision does not remove already committed C++ outputs. Keep artifact
layout and executable discovery suitable for later optional release packaging.

Retain the existing C++ example identity/build target where practical. Keep
Geometer generic; do not import PCB/application policy. Do not rebuild or alter
geometry algorithms, migrate every operation to TypeSpec, prune other demos,
or redesign the Python GUI as part of this plan. Its recently added fast
selectors remain. The separate documentation-cleanup plan is not implicitly
completed or superseded by this implementation plan.

## Inspected Baseline

| Area | Evidence / implication |
| --- | --- |
| Native shell | `examples/cpp/CMakeLists.txt` pins SDL 3.2.30 and ImGui 1.92.8, currently building SDL3/OpenGL3 backends. The inspected ImGui header has no docking API; choose a reviewed exact docking commit, not a floating branch. Its source tree already contains an SDL GPU backend. |
| Preview | `examples/cpp/hlr_preview.cpp` uploads a CPU depth-buffer image. Its scale is recomputed from projected bounds on rotation, causing the reported apparent approach/zoom. The GPU migration must fix camera policy separately. |
| Controls / work | Fixed-width controls and long `SameLine` rows do not adapt well; `IniFilename` is disabled. Loading and HLR run synchronously on the UI thread. |
| Native results | `src/cpp/lib/geometer/projection.h` exposes HLR JSON and SVG writers. Reuse them where their documented format matches the chosen export; do not relabel legacy JSON as generated A0. |
| Web behavior | `examples/wasm/illustration_demo.ts` has a Three.js orthographic/trackball preview, preserved camera extent, mesh-quality presets, separate vector output, SVG download and style JSON. Use it and the HLR Lab as acceptance references, not just screenshots. |
| Shared illustration | `src/ts/geometer/illustrated-hlr.ts` combines native/WASM Fast HLR with `createIllustrator`. `mesh-illustration.ts` owns preparation, visibility ordering, filled surfaces, fusion, coplanar layering, shading and SVG serialization. Canvas is a separate presentation path. |
| Platforms | Existing CI includes macOS arm64 and Windows native builds. Explicitly enable/package the optional example and required shaders; kernel CI alone does not establish viewer coverage. |

## Rust-First Architecture And Later C++ Track

The priority app uses Rust/wgpu and public `geometer-client` async methods;
Geometer runs in its owned native subprocess. Keep GPU/UI dependencies out of
the client crate and OCCT/C++ linkage out of the app. Choose the Rust UI/docking
integration in the architecture step, carrying forward the left/right controls,
results area, DPI sizing and progress requirements below. See the Rust brief
for process lifecycle and exact consumer proof. It is not required to use the
Dear ImGui/SDL integration written for the C++ app.

The following renderer-specific design applies to the later C++ track; common
camera, output, progress and UI behavior applies to both apps.

Keep Dear ImGui for UI and SDL3 for windows/input; replace the OpenGL renderer
integration with SDL GPU for both ImGui and the 3D viewport. Windows uses D3D12;
macOS uses Metal. Do not implement a full scene/game engine or maintain separate
handwritten D3D/Metal renderers. Vulkan/Linux are not this phase's acceptance
targets, although avoid gratuitous platform coupling.

Split responsibilities out of the large example file: application state and
job orchestration; camera/math; SDL GPU resource/rendering code; docked controls;
geometry/export adapters; illustration adapter. Keep demo policy outside the
kernel. Existing mesh positions, indices, transforms and material colors feed
the GPU; preserve winding, mirrored transforms and normal semantics.

Use vertex/index buffers, small lighting shaders, a color target and hardware
depth attachment. Draw the target inside the results workspace using ImGui's
SDL GPU texture binding. Handle resize, minimized/zero-sized windows, DPI,
resource lifetime and device/initialization failures. Report the selected GPU
backend and clear unsupported-device errors. No silent fallback to the old
software renderer. Start with opaque STEP solids; transparency is an explicit
capability decision, not something depth testing alone solves.

Pin shader tooling and record source/hash/format provenance. Build/package
DXIL or another validated D3D12 format for Windows and Metal-compatible shader
artifacts for Mac. Prefer build-time compilation; do not require end users to
install a shader compiler. Verify ImGui backend compatibility at the pinned
SDL/docking revisions rather than assuming current online examples match them.

## Controls And Results Layout

```text
+----------------------+---------------------------------------------+
| Controls (left/right) | Results: 3D | HLR | Illustration | Compare   |
| File / mesh quality  |                                             |
| Camera / Fit         | Active result fills the remaining workspace |
| Detail / outline     | Compare optionally splits model and output  |
| Illustration / light |                                             |
| Export               |                                             |
+----------------------+---------------------------------------------+
| Status: loading / computing / ready / failed; elapsed; diagnostics  |
+--------------------------------------------------------------------+
```

Default controls dock left; allow moving to the right, resizing, collapsing,
and restoring a sensible default layout. Use one native OS window initially;
docking is not permission to add multi-OS-window ImGui viewports. Persist layout
under the user's application settings directory, never the repository or CWD.
Ignore/recover invalid or off-screen settings with Reset Layout.

Use collapsible sections and width-aware label/control tables, stretch inputs,
wrapped help/error text, scrollable overflow and font-relative minimums. Avoid
fixed pixel widths and hard-coded top-row positions. Camera buttons can wrap
or use a compact menu when the dock is narrow. Result textures follow the actual
content area and framebuffer scale. Audit 1280x720 and larger windows at
Windows 100%, 150% and 200% scaling; narrow docks must remain usable without
clipped controls. Export dimensions must not depend on dock width or DPI.

## Camera And Geometry Consistency

Use an orthographic engineering camera by default, with orbit, pan, wheel zoom,
named views and explicit Fit. Preserve target and world-space extent while
orbiting. Fit establishes the extent when a model loads or the user requests
it; resizing adjusts aspect without silently fitting each rotated silhouette.
Changing HLR algorithms or illustration style must not reset the camera.

Use one canonical camera/model snapshot for 3D, HLR, illustration and export.
Document units (mm), up/direction, handedness, root placement, instance
transforms, mirror conventions and the distinction between display zoom and
physical geometry. For comparisons, choose whether both sides show the same
mesh or STEP-derived HLR; do not mistake tessellation differences for renderer
errors. Fast detail and fast mesh-shadow remain initial defaults with poly,
exact and older outline modes available for comparison.

## Loading, Recompute And Progress

For Rust, run public async client calls without blocking the wgpu/UI event
loop; results cross back through revision-tagged messages. Use the client's
existing process ownership, timeout and queued-cancellation behavior. Do not
replace it with a new subprocess or handwritten wire layer in the demo.

For the later C++ direct-link demo, move file loading, STEP parsing/meshing and Geometer computation off the UI
thread. Keep ImGui/SDL GPU access on the UI thread. Prefer a serialized worker
for the existing native value APIs initially; pass immutable inputs/results
and avoid concurrent mutation of OCCT/model state. Prepared-mesh reuse may be
added through existing supported APIs, without changing solver behavior.

Show idle/loading/preparing/computing/exporting/ready/failed states, current
phase, elapsed time and a live busy indicator. Display numerical progress only
when real completed/total work is exposed. Existing timing results are not live
progress callbacks. Keep the window repainting and the controls usable while
native calls execute; no synchronous wait on the UI thread.

Debounce camera-driven work, bound the queue and coalesce superseded requests.
Tag jobs with model/view/options revisions; a late completion must not replace
a newer scene. Show the previous completed image as stale while computing,
or a placeholder if no result exists. Export is disabled for stale/error state
unless the user explicitly chooses the named last-completed snapshot.

Do not promise interruption of an active OCCT call. Cancel queued jobs and
discard superseded results. Specify shutdown ownership and continue indicating
active work while it drains; do not detach a thread accessing destroyed state.
If hard cancellation, timeout recovery or bounded process shutdown becomes a
requirement, approve an owned worker-process design separately rather than
unsafe thread termination. Exercise failure/retry and model-switch races.

## Geometry And SVG Exports

Required outputs are geometry data and SVG, not screenshots of the UI. Offer
geometry JSON and linework SVG for the selected view/layers, plus illustrated
SVG when illustration is enabled. Native preview PNG capture is optional and
does not substitute for vector output.

The architecture step records the exact schema for each file. Prefer existing
generated A0 HLR and illustration contracts for new interchange. Preserve any
legacy writer output under its true identity; validate the adapter if projecting
native values into A0. Reuse existing SVG writers/composition, retain arcs where
the selected result supports them, and preserve units/viewBox/layers/colors.
No independently invented public viewer-only geometry schema.

Snapshot model, camera, algorithms, layers and illustration style together so
JSON and SVG agree. Export should not recompute with unrelated defaults. Support
style JSON import/export through the existing A0 style contract. Save through
native dialogs, handle errors/cancel safely and confirm replacement of existing
files. Prefer temporary-file/atomic completion so a failed export does not
leave a partially overwritten user file. Check SVG is actual vector artwork,
contains no unexpected external asset references, and opens outside the demo.

## Native Illustration API Dependency

Yes, the existing illustration work is reusable. Fast vector detail/shadow is
already native; the shared colorized SVG compositor is TypeScript, not an
existing C++ entry point. Merely using a similar GPU shader is not equivalent to
the web illustrator's visibility/fusion/material-layering behavior.

The upstream agent should implement the generic native illustration capability
in Geometer, expose it through negotiated executable IPC, typed Rust
`geometer-client` and the Python executable-backed client, and preserve the
existing A0 input/style/result semantics.
The proposed operation name `geometry.mesh_illustration.a0` is not yet an
implemented or approved identity. A complete STEP-to-illustration composition
is required; exposing only a mesh call is insufficient for the STEP consumer.
See the issue brief for design choices, attachment constraints and acceptance.

Do not duplicate that implementation in the viewer or substitute a bundled
JavaScript runtime, browser WASM package or approximate renderer. Consume the
maintained native C++ boundary where available, or a maintained executable
client adapter if IPC is chosen; do not create an ungoverned private protocol.
The upstream owner must supply the exact supported contract, native release or
integration revision, source/binary provenance and native/TypeScript parity
fixtures. Absence from the negotiated catalog means unavailable, not permission
to fall back. Keep illustration controls disabled with an actionable explanation
until the required capability exists; do not call that complete integration.

Native API acceptance covers Windows x64, Linux x64, Linux ARM64 and macOS ARM64.
That is distinct from this viewer's Windows-first/macOS GUI targets. The native
service must produce SVG headlessly without SDL GPU or a desktop session.
Its limits and cancellation semantics must remain compatible with existing IPC.

Use the same canonical model/view/style snapshot for native geometry and
illustration. Display the returned SVG through a reviewed native SVG
rasterizer/display adapter, preserving the original vector file for export.
A GPU-shaded preview is a separate presentation, not proof of SVG parity.
Existing TypeScript modules remain supported and serve as a conformance
baseline; this plan does not remove or relabel their implementation.

Start parity with source/fallback colors, unlit/Lambert/banded/toon modes as
actually supported by the shared style, background/transparency controls,
outline/detail toggles and widths, surface fusion and coplanar material layers.
Inventory each web control as reuse/native equivalent/deferred with rationale.
Experimental ambient occlusion and coplanar-seam filtering retain their labels
and default-off behavior; they are not silently promoted by adding native UI.

## Windows First; Mac Build And Separate Validation

Deliver the Rust Windows/API vertical slice first. Prepare its Cargo/wgpu Mac
build during Windows development and hand off the same feature-complete
revision for separate-agent validation. For the later C++ track, deliver
hardware-depth model, stable camera,
controls dock and busy loading. Then add exports and illustration. Prepare the
macOS CMake/Metal/shader path once the first renderer/layout slice builds; do not
wait for every Windows polish item to discover Mac compile problems.

Keep the optional native example out of solver-only builds. Package canonical
platform outputs plus shaders, required native Geometer artifacts and licenses; run from an arbitrary
working directory without sibling appz files, source paths or development tools.
Use an actual Mac runner/host to build and validate Metal assets. Do not claim
Windows cross-compilation produces a tested macOS application. Reuse existing
CI/cache conventions and Node-24-capable workflow actions if workflows change.

The [Mac agent handoff](macos-validation.md) defines artifact identity, commands,
interactive checks and reporting. Initially target macOS arm64, matching current
CI; Intel/universal distribution and public notarization are separate decisions.
A matching feature-complete build of each app must reach that agent after its
Windows acceptance; validate Rust first and do not conflate the two binaries.
Record build success separately from actual Metal/UI/export observations.

## Focused Acceptance And Closeout

Fast tests: camera extent invariance under rotation; projected axes/units;
job coalescing and stale-result rejection; error/export gating; deterministic
geometry/SVG serialization; and shared illustration fixtures. Register through
existing Rack/CTest/TypeScript strata. Reuse small synthetic visibility cases
and SOT-23; add one larger existing model for responsiveness and one colored
coplanar-material fixture for illustration. Avoid full solver qualification
for viewer-only edits.

GPU/manual lane: overlapping body/pin visibility at the reported angle; orbit
without apparent zoom; pan/Fit; docking left/right; resize/minimize/DPI; busy
animation during loading and HLR; fast/poly/exact switching; rapid view/model
changes; invalid STEP; save errors; and independent opening of JSON/SVG exports.
Record measured frame responsiveness and geometry job duration separately.
Native/web vector equality uses the same inputs/style/backend and canonical
geometry comparisons; GPU images use tolerances, not cross-driver byte equality.

Before closeout update durable native-viewer design, example/build guides,
demo inventory, export authority and release-facing changes. Perform the
required design-intent and test-runtime audits, then independent implementation
review. Remove only this completed plan/log directory when its evidence has
been transferred. Preserve the unrelated documentation-cleanup plan. The
repository's no-plans hygiene gate remains blocked by any active plan; do not
weaken it or claim release signoff. Public release still needs its normal gates.

## Primary Upstream References

Draft validation: `wn-dev-std plan show geometer-native-gpu-viewer --root .
--format json`, repository documentation file-link checks and whitespace checks
passed. No tests or application code changed and no builds ran for this draft.
The earlier demo code review is not approval of this new implementation plan.

- [Dear ImGui docking](https://github.com/ocornut/imgui/wiki/Docking): docking
  support and single-window dockspace setup; pin a compatible exact revision.
- [Dear ImGui SDL GPU example](https://github.com/ocornut/imgui/blob/master/examples/example_sdl3_sdlgpu3/main.cpp): renderer integration, not a complete 3D engine.
- [SDL GPU API](https://wiki.libsdl.org/SDL3/CategoryGPU): render/depth targets,
  resource lifecycle, platform drivers and shader format requirements.
- [SDL shadercross](https://github.com/libsdl-org/SDL_shadercross): candidate
  build-time cross-platform shader tooling, subject to the pinned build spike.
