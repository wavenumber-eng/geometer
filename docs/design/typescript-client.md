# TypeScript Contracts And Browser WASM Client

## Status and authority

The model-bounds JSON projection and analytic planar Boolean packed projection
are implemented. Authored TypeSpec and the normalized catalog own logical wire
structure; the separately governed analytic A0 packet owns its binary hot path.
This package is a deterministic projection plus a transport adapter. It does
not move geometry behavior out of C++ or replace the separately distributed
Emscripten JavaScript and WASM artifacts.

The package identity is `@wavenumber/geometer`, its module format is ESM, and
the repository-built package artifact is `dist/wasm/npm/geometer/`. Package version
`2026.8.18` follows the current Geometer release. Publication remains a release
action; a local artifact is not evidence that an npm release has occurred.

## Generated contracts and codecs

`scripts/generate-typescript-contracts.mjs` consumes only the normalized
catalog and emits:

- documented interfaces, literal types, enums, fixed-size tuple types, and
  discriminated unions in `src/ts/geometer/generated/contracts.ts`;
- strict named root codecs in `src/ts/geometer/generated/codecs.ts`; and
- typed operation and attachment declarations in
  `src/ts/geometer/generated/operations.ts`.

Generation fails on short-name collisions or an unsupported catalog construct.
The codec runtime rejects invalid UTF-8, unpaired Unicode surrogates, duplicate
object keys, trailing JSON data, non-finite numbers, missing and unknown fields,
literal/enum mismatches, and governed length/range violations. Encoders rebuild
objects in catalog order and preserve optional-field absence; they do not
materialize TypeSpec default intent into option patches.

The TypeScript codec vector test replays every governed pilot vector. This is
in addition to JSON Schema, C++, and generic transport tests, not a replacement
for their distinct assertion lanes.

Packed analytic roots deliberately do not use the JSON codec runtime. Their
generated DTO identities and integer-nanometer coordinates use `bigint`, and
`analytic-packet-a0.ts` performs bounded binary encoding/decoding against the
frozen `GMABRQ01`/`GMABRS01` layout. JavaScript `number` is rejected for every
64-bit analytic field.

## High-level browser client

Normal browser consumers import `createGeometerWasmClient` from
`@wavenumber/geometer/wasm`. The adapter owns:

- capability-catalog lookup and generic ABI A0/layout negotiation;
- UTF-8 encoding and decoding;
- Emscripten allocation and freeing;
- wasm32 attachment descriptor layout and pointer-out fields;
- copying input and output attachment bytes across the heap boundary;
- local C ABI error decoding and governed operation outcome decoding; and
- operation-specific attachment name, media type, requiredness, and byte-limit
  checks before calling C++.

`modelBounds()` accepts model bytes and an optional presence-preserving options
patch. It returns `ModelBoundsResultA0` on success and throws
`GeometerOperationError` with governed diagnostics on an operation failure.
`execute()` remains available for generated higher-level methods, advanced
transport integration, and operation-by-operation promotion; applications
should prefer the typed method for a promoted operation.

`analyticPlanarBooleanBatch()` accepts the generated logical request, encodes
its packed request attachment, executes the production C++ solver, strictly
decodes the packed result, and returns generated logical jobs, line/arc
fragments, rings, regions, lineage, operand outcomes, relationships, and
standalone job digests. The same typed method is available on the Worker
client.

The client verifies each promoted runtime operation against its generated
request/result identities and exact input/output attachment inventory,
including ordering, requiredness, media types, and byte limits. A module with
an unsupported generic ABI, descriptor layout, missing operation, or mismatched
declaration fails with an actionable `GeometerWasmTransportError` before
geometry execution. Zero-length byte views and zero-entry descriptor arrays
cross the ABI as the required null-pointer/zero-size pair.

## Package exports and build

The ESM package has explicit exports:

| Export | Purpose |
| --- | --- |
| `@wavenumber/geometer` | Generated contracts plus high-level client |
| `@wavenumber/geometer/contracts` | Generated DTOs, codecs, and operation metadata |
| `@wavenumber/geometer/analytic-packet-a0` | Strict analytic request/result packet projection |
| `@wavenumber/geometer/wasm` | Direct browser/Web Worker WASM transport adapter |
| `@wavenumber/geometer/worker` | Correlated main-thread client for a dedicated Worker |
| `@wavenumber/geometer/worker-host` | Worker-side host around the direct WASM adapter |

`scripts/build-typescript-package.mjs` compiles with TypeScript 5.9.3 and
strict, exact-optional-property, and unchecked-index settings. It validates and
atomically writes `dist/wasm/npm/geometer/`; check mode compares every artifact byte.
`npm pack` plus a clean temporary consumer proves the package exports and
declarations work outside repository path aliases.

The Emscripten factory and `.wasm` remain separately distributed under
`dist/wasm/browser/`. Consumers pass the factory itself and optional factory
configuration such as `wasmBinary` or `locateFile`; the npm package does not
embed a second copy of the geometry kernel.

## Dedicated Worker client

`createGeometerWorkerClient()` exposes the same typed `modelBounds()` and
`analyticPlanarBooleanBatch()` operations
without running synchronous OCCT work on the window event loop. The A0 Worker
protocol is package-local and has the identity
`wn.geometer.wasm_worker.a0`. Initialization transfers an owned copy of the
WASM bytes and returns the same capability catalog negotiated by the direct
adapter. Requests use monotonically increasing correlation identifiers and raw
attachment `ArrayBuffer` values rather than base64.

The main-thread client preserves caller-owned input arrays: it copies each
input into an owned buffer and transfers that buffer to the Worker. The host
serializes operation execution within one WASM instance and transfers owned
output attachment buffers back. It strictly re-encodes and decodes the
generated operation outcome across the message boundary. Typed operation
diagnostics, local C ABI transport errors, malformed messages, Worker errors,
and message-deserialization failures remain distinct. A structurally valid
response with an unknown or already-completed correlation identifier, or a
non-error response kind that does not match the correlated request kind, is
protocol corruption: the client terminates the connection and rejects every
outstanding request.

`close()` immediately rejects new calls, queues a graceful shutdown after
preceding requests, and then terminates the Worker. Concurrent `close()` calls
share that shutdown. `terminate()` is immediate and rejects every outstanding
request. Neither method claims cooperative cancellation inside a synchronous
OCCT operation. Application-level pools, caching, progress reporting, retry,
and scheduling remain consumer policy.

The current Emscripten loader is a classic Worker script. A Worker entry first
loads that factory with `importScripts`, then imports and installs
`@wavenumber/geometer/worker-host`. The maintained TypeScript entry is
`examples/wasm/model_bounds_worker.ts`; its committed build is
`dist/wasm/demos/model_bounds_worker.js`.

## Pilot example and verification

`examples/wasm/model_bounds_demo.ts` is the maintained model-bounds pilot. Its
HTML creates the dedicated Worker, while the TypeScript window and Worker
sources import only high-level package entry points. The committed JavaScript
builds are `dist/wasm/demos/model_bounds_demo.js` and
`dist/wasm/demos/model_bounds_worker.js`.

The interactive Three.js scene uses the prepared GLB produced from the same
SOT-23 fixture only as a display companion. The yellow volume, wireframe,
dimension lines, labels, and inspector values are derived from the
authoritative STEP/OCCT result. The example normalizes the meter-scaled GLB
display mesh into the millimeter STEP frame without changing contract data.
Its stylesheet is an explicitly identified projection of the Viz 3D visual
system, with repository-vendored Cousine replacing the internal Berkeley Mono
font and the governed Wavenumber logo retained as a low-opacity watermark.

Verification includes:

- generated-source and package-artifact freshness;
- Biome formatting/lint and strict TypeScript compilation;
- all governed TypeScript codec vectors;
- sparse fixed-tuple and variable-array rejection plus null-view ABI
  marshalling regressions;
- a packed/install/typecheck clean consumer;
- a real SOT-23 browser WASM `model_bounds` round trip through the high-level
  direct client;
- correlated SOT-23 round trips through a real Worker thread, including
  transferable ownership, governed/local errors, graceful close, and
  post-close rejection; and
- deterministic protocol regressions for unknown and duplicate correlations,
  mismatched response kinds, and immediate termination of multiple outstanding
  requests; and
- desktop and narrow real-browser smoke of the generated documentation and
  model-bounds example.

HLR and planar demos remain on their existing JavaScript interfaces until
those operations are individually promoted. Viz remains frozen at its recorded
2026.6.10 compatibility snapshot until its separate TypeScript migration passes.
The concrete operation-by-operation adoption path is documented in
[Viz TypeScript migration](viz-typescript-migration.md).
