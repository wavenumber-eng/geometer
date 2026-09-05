+++
type = "plan_log"
id = "geometer-native-gpu-viewer-native-illustration-api"
plan_id = "geometer-native-gpu-viewer"
step_id = "native-illustration-api"
created = "2026-09-05"
+++

# Expose Production Mesh Illustration Through Native Geometer IPC, Rust And Python

Issue-ready upstream brief, incorporating the consumer report supplied by the
user. This records requested work for the other Geometer agent, not an already
available API, an assigned external issue, or implementation completed here.
Baseline checked in this worktree: Geometer 2026.9.4. Reconcile against the
other agent's actual revision before implementing overlapping work.

## Problem

Geometer provides supported illustration through the TypeScript package:

- `@wavenumber/geometer/mesh-illustration`
- `@wavenumber/geometer/illustrated-hlr`

It generates shared structural contracts for
`geometry.mesh_illustration.input.a0`, `geometry.mesh_illustration.style.a0`
and `geometry.mesh_illustration.result.a0`. Generated Rust/C++ DTOs describe
these values, but do not provide a callable illustration operation. In the
audited baseline it is absent from the executable's negotiated operation
catalog, native IPC dispatcher, typed Rust client and Python executable client.

The apparent WASM dependency needs a precise distinction: Fast HLR exists in
native and WASM forms, while colorized illustration preparation/composition and
SVG serialization are shared TypeScript code. The SVG path can run outside a
browser, but that does not make it an executable Geometer API. A native caller
cannot obtain the supported illustration merely by using its generated DTOs.

## Consumers And Intended Outcome

The reported immediate consumer is Alexandria, a native Rust application. It
extracts STEP model bytes from KiCad footprints and currently requests top-view
HLR geometry. It is moving toward persistent `geometer-client` and Fast HLR.
Its richer previews need Geometer's illustration without embedding JavaScript,
loading browser WASM, reimplementing illustration policy in Rust, statically
linking OCCT, or maintaining handwritten JSON/subprocess adapters.

The planned C++ GPU viewer needs the same native capability for illustrated
SVG output. The illustration implementation belongs in generic Geometer, not
in either product's UI. It must work headlessly; SDL, GPU rendering and a
desktop session are not prerequisites for executable illustration processing.
Use a native implementation rather than shifting the browser/JS workaround
into these consumers. Any different implementation strategy requires explicit
review rather than silently changing this requirement.

## Requested API

Add a governed executable illustration operation, with
`geometry.mesh_illustration.a0` as a proposed identity subject to contract
review. Reuse existing A0 input/style/result semantics rather than creating
an incompatible parallel vocabulary. Define the operation declaration,
attachment descriptors, catalog availability, strict request/outcome variants,
native dispatcher and generated projections together. A catalog entry is not
runtime implementation; publish availability only where the adapter works.

Expose a typed async Rust convenience through `geometer-client`, following its
existing request/attachment/error conventions. An illustrative consumer call is
`client.mesh_illustration(request).await?`; the exact method/request shape is a
design output, not a promised existing signature. Rust callers should not
assemble frames or untyped JSON themselves. Expose an appropriately factored
C++ value boundary backed by the same implementation for native embedding.

Python support is required in the same capability, not an optional follow-up.
Expose a public typed method on the executable-backed `geometer.GeometerClient`
and the public input/style/result models needed to call it. Support the complete
STEP workflow as well as the mesh operation through maintained helpers. Follow
the existing package's executable discovery/override, ownership, context-manager
cleanup and exception conventions; do not introduce a second subprocess stack.
Provide a straightforward one-shot convenience where consistent with the current
Python API, reusing the persistent-client implementation rather than duplicating
framing/serialization. Do not require Python consumers to invoke private methods,
author JSON frames, embed JS/WASM or link native Geometer/OCCT into Python.

Rust and Python share operation identities, generated contracts, semantics and
attachment codecs; their method shapes should remain idiomatic to each client.
Generated models alone are not callable support. Keep the Python library free
of GUI/wgpu dependencies; applications choose their own UI/rendering toolkit.

## Complete STEP Workflow

Alexandria starts with STEP bytes, not an already prepared indexed mesh.
Choose and document one complete supported path:

1. A model-illustration operation takes a bounded STEP attachment and performs
   tessellation, illustration and optional Fast HLR composition.
2. A governed STEP-to-illustration-ready-mesh operation feeds the mesh
   illustration operation using supported generated clients/codecs.
3. Another explicit composition with the same consumer guarantees, not a
   requirement to reverse-engineer internal GLB/mesh structures.

The design should preserve material colors, material-per-triangle assignments,
instance transforms and winding/mirror semantics. The existing indexed-mesh
HLR packet is not automatically a complete colored illustration transport.
Inventory its fields before reuse; do not lose normals/materials/transforms
by treating an opaque mesh attachment as complete contract coverage.

Mesh-only acceptance is insufficient: provide Rust and Python STEP-bytes-to-SVG
examples and integration tests on the selected supported composition. Keep intermediate
geometry accessible where the design exposes it; do not force SVG generation
on callers who only need HLR data.

## Semantics And Composition

Match accepted TypeScript behavior for preparation, visibility ordering,
surface fusion, coplanar material layering, material colors/opacity, view and
instance transforms, shading modes, output bounds/viewBox, background and SVG.
Preserve omitted-versus-explicit style values and current defaults. Return
statistics and warnings consistent with the A0 result. Unsupported controls
must fail explicitly, not be ignored or replaced by a materially different
renderer. Experimental AO/seam options retain their existing maturity and
must not be silently promoted with this operation.

Keep illustration and HLR independently composable. If supplied linework or a
Fast-HLR convenience is offered, govern its request and coordinate conventions
instead of adding an undocumented `hlr` JSON property to the existing A0 input.
Preserve separate outline/detail results and layer selection. Do not change
defaults or behavior of existing HLR, file CLI or TypeScript/browser APIs.

## Limits, Wire Format And Lifecycle

Use bounded binary attachments for large STEP/mesh payloads as appropriate.
Define count/byte/aggregate limits, valid indices, finite coordinates, transform
rules, normal/material references and output-work limits before allocating or
processing attacker-controlled sizes. Govern new packed projections and their
codecs/vectors; do not invent an unversioned native-only packet.

The existing A0 result requires `svg: string`. Decide and document an SVG byte
limit compatible with the IPC JSON/frame budget. If large outputs require an
attachment-oriented result, govern that deliberately; moving SVG out of the
required field while claiming unchanged A0 validity is not compatible.

Follow existing persistent-client behavior for negotiated compatibility,
serialized execution, queue cancellation, local timeout, process termination
and protocol failure. Do not promise interruption of an active native call:
A0 cancellation is queue-only. Reject unsupported options/resource exhaustion
through defined error outcomes. Test failed requests do not poison subsequent
valid work, and test process/protocol failures do fail affected pending calls.

## Cross-platform Requirement

Ship the native operation wherever the released native executable is supported:
Windows x64, Linux x64, Linux ARM64 and macOS ARM64. Provide equivalent contract
availability and headless behavior on all four. This does not add Linux GUI
work to the separately planned Windows/macOS viewer. A Windows-only prototype
is useful development evidence, not completion of this upstream issue.

## Acceptance Criteria

- The compatible released executable advertises and executes the operation via
  `serve --stdio`; unsupported/older executables are detected clearly.
- Typed Rust integration starts that executable, requests representative mesh
  illustration and exercises the supported STEP-bytes workflow without JS/WASM
  in the consumer or handwritten frame/JSON adapters.
- Typed Python integration uses the public installed executable-backed package
  to exercise the same mesh and STEP workflows, with executable discovery and
  explicit override, generated result validation, exception behavior and clean
  process shutdown. Verify declared one-shot and persistent entry points and
  public imports; source-only/private-client tests are insufficient.
- Results validate against the selected governed contract, including the
  existing `geometry.mesh_illustration.result.a0` where unchanged; SVG is
  nonempty, well formed and opens independently of the demo.
- Identical inputs/options produce deterministic SVG. Record byte-stability
  and numerical/canonical comparison rules for cross-platform parity explicitly.
- Native results match the accepted TypeScript baseline on representative
  fixtures covering colors/material partitions, transforms/mirrors, shading,
  opacity, output extent/background and optional HLR composition. Do not bless
  output solely because an unrelated GPU screenshot looks similar.
- Fast composition retains separate geometry outline/detail layers; standalone
  illustration and standalone HLR remain valid supported uses.
- Oversized/malformed payloads, bad references/options and output-limit failures
  return bounded governed errors. Queue cancellation, local timeout, process
  failure and protocol failure follow existing client semantics.
- Windows x64, Linux x64, Linux ARM64 and macOS ARM64 build/runtime checks pass,
  with exact source/artifact identities recorded. Use pinned fixtures and
  TypeScript oracle outputs; do not silently regenerate goldens to match a port.
- Release docs include complete Rust and Python examples from STEP bytes to illustration,
  operation discovery, limits/error behavior and the exact supported release.
  Catalog, schema, language projections and public interface inventories agree.

## Non-goals And Coordination

No static Geometer/OCCT linkage into Alexandria, product-specific footprint or
color policy in Geometer, removal of browser APIs, change to existing HLR
defaults, or mandatory crates.io publication. Alexandria may pin the supported
release Git revision. No GPU renderer is needed to generate vector SVG.

The other Geometer agent owns API design/implementation and native parity. The
viewer effort consumes the reviewed API; it does not race that agent with a
second compositor or client protocol. Agree the operation identities, STEP
composition, attachment/result bounds, Rust/Python public APIs and exact integration
revision before viewer integration. Keep UI/GPU work unblocked in the meantime.

## Local Source Evidence

- `src/tsp/geometer/operations/mesh-illustration-a0.tsp`: existing structural
  inputs, style presence, materials/transforms, result SVG/stats/warnings.
- `src/ts/geometer/mesh-illustration.ts` and `illustrated-hlr.ts`: supported
  illustration behavior and optional Fast-HLR composition.
- `src/rust/geometer-client/src/lib.rs` and `client.rs`: current typed exports,
  process ownership and queue-only cancellation/local timeout behavior.
- `src/cpp/lib/operation_registry.cpp` and
  `native_topology_operation_registry.cpp`: native operation dispatch boundaries.
- `tests/typescript/mesh_illustration_validation.mjs`: existing baseline fixtures
  and reusable illustration checks.

These observations apply to the audited baseline, not uninspected work being
developed by another agent. This brief does not claim an issue was published,
the operation was promoted, or production parity was already demonstrated.
