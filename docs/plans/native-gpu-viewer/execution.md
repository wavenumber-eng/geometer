+++
type = "plan_log"
id = "geometer-native-gpu-viewer-execution"
plan_id = "geometer-native-gpu-viewer"
step_id = "architecture-and-parity"
created = "2026-09-05"
+++

# Execution evidence

Branch: `feature/native-api-rust-demo`, from `b7605c7`. User confirmed the other
agent consumes WASM for now; this branch owns native illustration implementation.
No public release/push or demo bundling is implied by local implementation.

## Slice 1: generated Rust IPC dispatch

Removed the handwritten bounds/HLR-only dispatch whitelist. Request decoding
and logical request/result identity matching now generate from TypeSpec's
normalized catalog and IPC unions. Existing packed dispatch and negotiated
availability remain authoritative. No contract digest or executable changed.

Independent reviewer approved after adding live topology open/close and wrong
logical-result tests. Focused verification: 28 library tests, 10 process tests
(released Windows executable), 3 generated-dispatch tests; test execution under
0.2 seconds excluding compilation. Rust structural audit, Clippy, generated
freshness and documentation checks passed. New tests are discovered by existing
RUST_001 Cargo coverage; no GUI dependencies enter that lane.

## Accepted architecture review findings

- Do not use experimental topology render for colored preview: its current
  contract explicitly supplies a single neutral material, session lifecycle,
  and meter-based GLB geometry.
- Add stateless tessellation with a generated mesh-collection JSON attachment,
  reusing illustration mesh/material DTOs. Raw IPC attachments remain bounded;
  do not invent an ungoverned colored binary format or enlarge 8 MiB envelopes.
- Keep positions/placement in millimeters; document root-placement policy,
  component transforms, normals/winding, sRGB and occurrence color precedence.
- Pure mesh illustration retains A0 semantics. Fast HLR composition requires a
  separate TypeSpec request/result wrapper; do not silently compute extra
  linework in the pure operation.
- Inline A0 SVG results retain their identity and must explicitly reject output
  exceeding the IPC envelope. An attachment-oriented result needs its own
  generated wrapper, not a mislabeled A0 result.
- Native implementation must preserve TS preparation/defaults, visibility
  ordering, fusion, material layering and SVG behavior. It must not execute JS
  or substitute approximate GPU shading for the vector illustrator.
- The Rust shell will use pinned eframe/egui with wgpu, a hardware depth target,
  stable orthographic camera and a left/right resizable controls panel. This is
  separate from the later direct-linked C++ SDL GPU track.

Remaining: new native contracts/implementation, Rust shell and native API
integration, Python facade, parity fixtures and platform/user acceptance.

## Slice 2: stateless colored native tessellation

Implemented TypeSpec `geometry.mesh_collection.a0` and tessellation request,
result and operation declarations, generated across all existing projections.
Native C++ uses an in-memory STEPCAF reader and XCAF/RWMesh iterators to preserve
source styles, transforms, reflected winding and normals. Rust and Python expose
typed methods and verify attachment hashes, source hash, layout, counts and caps.

Reviewer findings resolved: force document units to mm; reject external STEP
references before transfer; distinguish valid refinement/reuse meshing flags
from partial/failure states. A newly exposed invalid-STEP case also required
routing OCCT diagnostics to stderr during IPC to preserve binary framing.

Focused evidence: CTest direct-link unit-state, external-reference and mesher
status checks (0.11s); 30 Rust library + 11 live process + 3 dispatch tests
(0.20s runtime); two Python live/metadata tests (0.29s); all 130 contract vectors,
generated freshness, Clippy, Ruff, Pyright and Rust structural hygiene. Local
Windows executable is in `out/docs-cleanup/native-build/src/cpp/cli/geometer.exe`;
compiled distributions remain unchanged. The new generated catalog is not
compatible with the old released binary; process tests use explicit feature
executable overrides. No OCCT rebuild, public upload or four-platform approval.

This is a necessary STEP/colored-mesh boundary, not native illustration itself.
Full native compositor parity, Rust GPU UI and Python illustration remain open.

Contract-inventory maintenance: historical topology evidence still records its
original catalog digest. The test now removes only this additive tessellation
namespace/operation/union variants and reconstructs the exact original catalog
digest, proving preexisting definitions unchanged without rewriting evidence.
Generated C++ membership checks ignore formatter-only adjacent literal/space
changes. The provenance-test file lock was refreshed for those inventory-only
edits; solver evidence and its source hashes are unchanged.

## Slice 3: native illustration value renderer

Implemented focused native preparation, math/style, grid/visibility, fusion,
coplanar layering, line chaining and SVG modules using existing TypeSpec-generated
A0 input/style/result values. No handwritten parallel public DTO or JavaScript
runtime. Native executable illustration dispatch is still the next slice.

Independent review found output-size accounting, decimal halfway rounding,
XML text and finite-intermediate issues. Fixes enforce a complete SVG cap,
ECMAScript decimal rounding, shared TypeScript/C++ XML hardening and explicit
overflow rejection. Follow-up review added Unicode CSS whitespace/case-folding
parity and registered differential coverage through CTest and the TypeScript
Rack lane. Shared fixtures compare exact A0 results and SVG, repeat native
renders, parse all SVG documents, and include the colored SOT-23 STEP workflow.

Native limits fail explicitly rather than switching renderers. They bound
accepted work/output, not peak process memory. This source-only direct-link
checkpoint does not claim released IPC/client availability or macOS/Linux
runtime qualification. No OCCT rebuild or published binary replacement.

Final review approved the core checkpoint and independently reran both CTests:
40 exact parity/determinism cases plus native smoke passed (about 1 second).
Existing TypeScript illustration validation, all 130 contract vectors,
generated freshness, documentation checks and touched-file formatting passed.

## Slice 4: executable illustration and typed clients

Added TypeSpec `geometry.mesh_illustration.request.a0` and the operation
`geometry.mesh_illustration.a0`, with existing A0 view/prepare/style/SVG values,
one bounded `geometry.mesh_collection.a0` JSON attachment and the unchanged
inline `geometry.mesh_illustration.result.a0`. Generated all projections and
IPC unions/dispatch; no parallel handwritten wire DTOs. Rust and Python typed
methods accept the existing generated illustration input and adapt transport
placement internally.

The native operation calls the reviewed renderer. IPC now checks the encoded
8 MiB response JSON limit before handing a frame to the writer, returning a
recoverable operation failure. A real 128,000-triangle nonoverlapping fixture
exercises that limit and proves the same process accepts a subsequent small
request. Rust STEP-to-SVG output matches the test-only TypeScript oracle exactly;
typed Python validates the generated result and parses SVG. Missing/wrong-media
attachments are rejected before dispatch; malformed attachment content fails
without poisoning the process.

Catalog lineage tests reconstruct the exact historical catalog after removing
only the reviewed tessellation/illustration additions and exact IPC variants.
Historical solver/topology evidence is unchanged. Native HLR composition,
installed wheel, cross-platform and viewer acceptance remain open; compiled
release artifacts have not been replaced or published.

Independent reviewer approved slice 4. Focused final checks passed: 30 Rust
library, 12 live process and 3 dispatch tests; Python illustration plus catalog
promotion tests (10 cases, about 3 seconds including the oversized response);
both CTests with all 40 exact renderer fixtures; Clippy, Rust structural hygiene,
Ruff/Pyright, 130 contract vectors, generation and documentation freshness.
The Windows operation/typed Rust workflow checkpoint is done; this does not
complete platform qualification, Python installed-package acceptance or demos.

## Slice 5: native executable-backed Rust/wgpu demonstration

Added the independent, locked `examples/rust/native_viewer` crate. Hardware
depth renders generated colored STEP tessellation; generated typed native
illustration produces original vector SVG; typed model HLR produces separate
Fast detail/fast mesh shadow layers, with retained legacy algorithm selectors.
No Geometer/OCCT static linking, JS/WASM runtime or parallel wire DTOs. The
maintained client now exposes its existing executable discovery and suppresses
Windows child console windows.

Left/right resizable controls, a resizable model/result split, stable
orthographic orbit/pan/zoom/Fit and independent output tabs are implemented.
Loading/native calls/conversion/rasterization/export run off the UI thread;
phase/elapsed indicators, debounced latest-view work, revision/epoch filtering
and explicit owned-process termination preserve coherent snapshots. Original
SVG and generated result/style/HLR/mesh JSON exports retain immutable background
snapshots. The diagnostic GPU preview is explicitly opaque; native SVG retains
material opacity. Rasterized display is not substituted for vector export.

Independent reviewer found alpha/depth-write ambiguity, a stale mesh after
failed load, and successful exit codes on failed smoke. All were fixed and
the reviewer approved the checkpoint, including inspection of the app-only
GPU screenshot. Stop now invalidates every old event kind and waits for its
termination result before reconnect. The preview clears both mesh and bounds.

Focused Windows checks passed: five GUI unit tests, GUI and maintained-client
Clippy, opt-in Rack RUST_002 (positive window/native/SVG/HLR screenshot and
negative missing-model smoke, 2 tests in 15.36 seconds), 30 client library,
12 live process and 3 generated dispatch tests, Ruff/Pyright, structural audit,
documentation freshness (312 links/12 demos/149 public sources), whitespace.
Read-only process inspection after checks found no remaining geometer.exe.
Radeon RX 7600 XT/Vulkan screenshot showed both colored GPU geometry and native
illustrated output; the 823-triangle SOT23-6 example produced 48,111 SVG bytes
and 12,492 HLR JSON bytes. These are development observations, not release or
exhaustive interaction/depth certification.

Windows human acceptance is the next checkpoint: inspect the previously bad
angle, camera behavior, docking/DPI/narrow sizing, selectors, busy work, failures
and exports. Mac source-build/Metal instructions are prepared, but no Mac build
or runtime is claimed. Installed Python wheel qualification, optional native
HLR composition, four-platform release qualification and later C++ GPU work
remain open. No release artifacts were replaced, uploaded, tagged or published.

## User feedback: Lab line visibility and settings

User reported illustration lines in front of foreground surfaces and requested
comparison with the TypeScript Lab plus equivalent controls. Source analysis
found the Lab explicitly disables raw mesh silhouettes/creases and attaches
visibility-filtered HLR to its prepared scene. Both TS core and native renderer
draw raw mesh strokes after all surfaces; the first Rust demo incorrectly
enabled that diagnostic path while keeping HLR separate. Renderer parity did
not establish Lab-workflow parity. Durable analysis and the full control map
are in `docs/developer/native-illustration-lab-parity.md`.

Rust now uses Lab shading/material/line/background defaults and disables raw
mesh strokes by default, retaining labeled diagnostics. Added bands/key/rim,
colors/back faces/line width, separate Fast crease (25 degrees) and experimental
seam controls, independent HLR toggles, bbox-relative HLR quality, STEP quality
presets/custom values and explicit Retessellate. Existing generated contracts
carry every supported setting. Native HLR composition and browser-only
experimental AO are explicitly unavailable; no fake wire fields or renderer
fallback were added. Core illustration and TypeSpec contracts are unchanged.

Independent review caught camera zoom clamping on zero scroll against temporary
empty bounds during retessellation. The fix skips zero-scroll/absent-model zoom,
disables empty-scene Fit and has a large/small-extent regression. Failed/stopped
retessellation keeps the original STEP snapshot for retry without displaying
stale outputs. The reviewer approved this scoped checkpoint after correction.

Focused checks: seven unit tests, Clippy, opt-in native settings integration
(same generated TS/native A0 result; raw overlays off/on; increased triangles
for finer settings; changed detail for Fast crease 1/80 degrees; both HLR layers
off), Rack window success/failure checks (2 passed, 14.34 seconds), Ruff/Pyright,
documentation freshness and whitespace. A fresh app-only screenshot was
inspected: SOT-23 rendered 220 triangles, 8,632 SVG bytes and 10,514 HLR JSON
bytes on Radeon/Vulkan. No OCCT/native kernel rebuild or release publication.

## User correction: compose HLR in the illustration and exported SVG

Implemented the requested web-Lab linework workflow rather than treating raw
mesh diagnostics or separate HLR views as equivalent. The existing TypeSpec
illustration operation now declares an optional bounded `hlr_projection`
attachment, reusing the generated HLR result. No parallel DTO format was added.
Native C++ composes visible detail then outline over fills, with the shared
projection basis, millimeter frame, viewport and mirror. Rust exposes
`mesh_illustration_with_hlr`; Python accepts `hlr_projection=`. The original
pure illustration call remains available. Unsupported arcs, mismatched bases,
malformed attachments and more than 1,000,000 total segments fail explicitly.
Matching source model/placement and visible-only HLR remain caller preconditions;
the result contract cannot authenticate those relationships from 2D lines alone.

The Rust demo's line toggles now control the preview and original SVG export,
with only selected HLR layers computed. Both off bypasses HLR entirely. The
result pane is all white and centered, and a fixed status bar prevents its
layout shifting during recompute. Save SVG is beside the output selector.
Surface fusion and automatic SVG style/path coalescing remain native renderer
features with the same TypeScript semantics, not client postprocessing.

Independent review identified normalization of scaled/skewed up vectors and
unnecessary HLR failure coupling when line layers were off. Both were corrected
with regressions. A final independent read-only reviewer approved the slice
without blocking findings, including ordering, units/mirror, caps, client APIs,
toggles, pure-fill bypass, fixed output and vector export.

Focused Windows checks passed: both native CTests (40 existing pure-renderer
parity cases plus direct composition/error/cap checks); 24 complete composed
TypeScript/native result comparisons and deterministic repeats; 12 existing
live Rust IPC and 3 generated dispatch tests; 11 Python illustration/promotion
tests; 11 GUI unit/native tests, including deliberately invalid STEP bytes in
a fill-only job proving no HLR call occurs. Clippy, Ruff/Pyright and contract
freshness passed. The updated Rust executable example wrote a composed SVG.
An actual GUI smoke and inspected screenshot showed the composed SOT-23:
220 triangles, 12,723 SVG bytes, Radeon RX 7600 XT/Vulkan. Cached OCCT was reused;
only Geometer source was rebuilt. Public releases and platform qualification
are still separate; human angle/interaction acceptance remains next.
