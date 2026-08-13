# TypeScript Contracts And Browser WASM Client

## Status and authority

The model-bounds pilot TypeScript projection is implemented. Authored TypeSpec
and the normalized catalog own wire structure; this package is a deterministic
projection plus a transport adapter. It does not move geometry behavior out of
C++ or replace the separately distributed Emscripten JavaScript and WASM
artifacts.

The package identity is `@wavenumber/geometer`, its module format is ESM, and
the repository-built package artifact is `dist/npm/geometer/`. Package version
`2026.6.23` follows the current Geometer release. Publication remains a release
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

The client verifies the runtime operation catalog against generated input
attachment declarations. A module with an unsupported generic ABI, descriptor
layout, missing operation, or mismatched attachment declaration fails with an
actionable `GeometerWasmTransportError` before geometry execution.

## Package exports and build

The ESM package has explicit exports:

| Export | Purpose |
| --- | --- |
| `@wavenumber/geometer` | Generated contracts plus high-level client |
| `@wavenumber/geometer/contracts` | Generated DTOs, codecs, and operation metadata |
| `@wavenumber/geometer/wasm` | Direct browser/Web Worker WASM transport adapter |

`scripts/build-typescript-package.mjs` compiles with TypeScript 5.9.3 and
strict, exact-optional-property, and unchecked-index settings. It validates and
atomically writes `dist/npm/geometer/`; check mode compares every artifact byte.
`npm pack` plus a clean temporary consumer proves the package exports and
declarations work outside repository path aliases.

The Emscripten factory and `.wasm` remain separately distributed under
`dist/wasm/browser/`. Consumers pass the factory itself and optional factory
configuration such as `wasmBinary` or `locateFile`; the npm package does not
embed a second copy of the geometry kernel.

## Pilot example and verification

`examples/wasm/model_bounds_demo.ts` is the maintained model-bounds pilot. Its
HTML loads the full-browser Emscripten artifact, while the TypeScript source
imports only the high-level package. The committed JavaScript build is
`dist/wasm/demos/model_bounds_demo.js`.

Verification includes:

- generated-source and package-artifact freshness;
- Biome formatting/lint and strict TypeScript compilation;
- all governed TypeScript codec vectors;
- a packed/install/typecheck clean consumer;
- a real SOT-23 browser WASM `model_bounds` round trip through the high-level
  client; and
- desktop and narrow real-browser smoke of the generated documentation and
  model-bounds example.

HLR and planar demos remain on their existing JavaScript interfaces until
those operations are individually promoted. Viz remains frozen at its recorded
2026.6.10 compatibility snapshot until its separate TypeScript migration passes.
