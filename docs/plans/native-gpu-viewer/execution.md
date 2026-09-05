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
