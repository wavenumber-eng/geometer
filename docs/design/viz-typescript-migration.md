# Viz TypeScript Migration

## Status and boundary

This guide defines the intended migration from Viz's frozen Geometer 2026.6.10
JavaScript integration to `@wavenumber/geometer`. It does not declare that Viz
has migrated. The compatibility snapshot at
`docs/contracts/compatibility/viz-2026.6.10.toml` remains enforced until a
released Geometer artifact is vendored into a temporary Viz workspace, the
targeted integration tests pass, and Viz explicitly replaces that snapshot.

Geometer owns generated operation contracts, strict codecs, generic C ABI
marshalling, capability negotiation, and the dedicated-Worker request channel.
Viz continues to own application-level worker pools, IndexedDB caches,
compressed design payloads, progress UI, render-packet assembly, retry policy,
and board/viewer scheduling. Geometer must not acquire PCB or viewer policy.

## Current-to-target mapping

| Frozen Viz surface | Current responsibility | Target | Adoption gate |
| --- | --- | --- | --- |
| `createGeometerModule` and full browser JS/WASM | Instantiate full OCCT module | Keep as the separately vendored factory used by `@wavenumber/geometer/wasm` or the Worker host | Package and candidate WASM versions match |
| `createGeometerPlanarModule` and planar browser JS/WASM | Instantiate smaller planar module | Same generated adapter once the needed planar operation is promoted | Planar operation catalog is present in that artifact |
| Vendor manifest capability strings | Handwritten symbol availability | Runtime operation catalog plus generated request/result and attachment declarations | Candidate catalog comparison passes |
| `GeometerStepWorker.callStepToGlbBytes` | `_malloc`, pointer-out reads, byte copy, and free | Generated `stepToGlb` Worker method | STEP-to-GLB contract is promoted; not available in the pilot package |
| HLR worker calls | JSON options plus low-level HLR C ABI | Generated HLR Worker method | HLR contract and examples are promoted |
| `PlanarClipper2Bytes` and planar batch helpers | Handwritten packed encoders and direct symbols | Generated operation method with governed packet attachment codec | Each packed planar operation is promoted independently |
| Worker request maps | Correlation and local error propagation | `GeometerWorkerClient` per Worker | Package Worker smoke passes in Viz |
| Worker pool and cache managers | Pool size, queue selection, cache ownership, timing | Remain in Viz around generated clients | No Geometer dependency on Viz policy |

The model-bounds pilot is the only promoted high-level method today. Viz should
not replace working STEP-to-GLB, HLR, or planar calls with an invented generic
shape before those operation contracts are promoted.

## Direct browser integration

Code already executing inside a Worker can use the direct adapter:

```ts
import { createGeometerWasmClient } from "@wavenumber/geometer/wasm";

const client = await createGeometerWasmClient(createGeometerModule, {
  wasmBinary: vendoredWasmBytes,
});
const bounds = await client.modelBounds({ model: stepBytes });
```

Viz code must stop reproducing allocation, descriptor offsets, pointer-out
decoding, and ownership calls once the corresponding generated method exists.
Focused ABI validation may continue to call the low-level symbols.

## Dedicated Worker integration

Window code creates one generated client per dedicated Worker:

```ts
import { createGeometerWorkerClient } from "@wavenumber/geometer/worker";

const worker = new Worker(vizGeometerWorkerUrl);
const client = await createGeometerWorkerClient(worker, {
  wasmBinary: vendoredWasmBytes,
});
const bounds = await client.modelBounds({ model: stepBytes });
```

The Worker entry loads Viz's vendored classic Emscripten factory, then installs
the package host:

```ts
importScripts("./geometer-browser.js");
const { startGeometerWorkerHost } = await import(workerHostModuleUrl);
startGeometerWorkerHost(createGeometerModule, self);
```

Viz's build must resolve `workerHostModuleUrl`; import maps on the window do not
resolve bare specifiers inside a classic Worker. The Geometer model-bounds
example is the executable reference for this bootstrap.

Each host serializes calls within its WASM instance. The client correlates
multiple outstanding promises, but this does not promise overlapping OCCT
execution. Viz may retain a pool of isolated Worker/module instances when its
operation-specific qualification justifies the memory and concurrency cost.

Input attachments are copied into client-owned buffers and transferred, so
Viz's source arrays are not detached. Output attachments are transferred back.
`close()` immediately rejects new calls, drains preceding requests, and shuts
down cleanly; `terminate()` is the explicit immediate-cancellation mechanism.
A terminated in-flight OCCT operation has no typed operation outcome.

## Capability and error migration

The generated client rejects a mismatched generic ABI, descriptor layout,
request/result identity, requiredness flag, media-type list, byte limit, or
input/output attachment inventory during initialization. Viz's vendor manifest
still governs artifact filenames, factory names, and legacy symbols during the
migration; it does not replace runtime catalog negotiation.

Viz should handle:

- `GeometerOperationError` for governed contract or geometry diagnostics;
- `GeometerWasmTransportError` for local ABI/module incompatibility;
- `GeometerWorkerError` for malformed messages, Worker failure, or lifecycle
  misuse; and
- application cache, retry, and progress failures in Viz's existing layers.

Do not parse error-message text to recover a contract code.

## Operation-by-operation adoption

For each operation:

1. Wait for its TypeSpec contract, generated method, attachments, vectors, and
   native/WASM parity to be promoted in Geometer.
2. Vendor the candidate npm artifact together with the matching full or planar
   Emscripten JavaScript and WASM files in a temporary Viz workspace.
3. Replace only that operation's handwritten marshalling behind the existing
   Viz application abstraction.
4. Compare semantic results, diagnostics, packet versions, transfer behavior,
   worker memory, and representative performance with the frozen integration.
5. Keep the legacy implementation available until the candidate test lane and
   relevant browser tests pass.
6. Pin the tagged Geometer release, remove the displaced pointer code, and
   update Viz's package and vendor manifests.

The expected sequence is model bounds when Viz needs it, STEP-to-GLB, HLR, and
then the packed planar operations as their contracts are promoted. Existing
HLR and planar demos migrate with those same operation gates.

## Snapshot replacement gate

Replace `viz-2026.6.10.toml` only after all of the following are recorded:

- released Geometer and npm package versions;
- exact vendored artifact mappings and digests;
- generated operation identities used by Viz;
- direct and Worker browser integration results;
- retained legacy operations and symbols, if any;
- targeted Viz unit, browser, cache, and representative-design results; and
- the Viz revision that adopted the generated client.

Until then, Geometer's existing factory names, runtime methods, C ABI exports,
free functions, and packed format versions remain compatibility requirements.
