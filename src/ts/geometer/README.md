# `@wavenumber/geometer`

Generated TypeScript contracts and the high-level Geometer browser WASM client.
The package hides generic C ABI allocation, descriptor layout, pointer-out, and
ownership details from normal consumers.

```ts
import { createGeometerWasmClient } from "@wavenumber/geometer/wasm";

const client = await createGeometerWasmClient(createGeometerModule);
const bounds = await client.modelBounds({ model: stepBytes });
console.log(bounds.bounds.size);
```

Fast HLR can consume synthesized indexed meshes without first creating STEP:

```ts
const projection = await client.meshHlrProjection({
  mesh: {
    positions: [0, 0, 0, 10, 0, 0, 0, 10, 0],
    indices: [0, 1, 2],
    sourceFaces: [1],
  },
});
console.log(projection.views[0].modes.detail.segments);
```

Use `modelHlrProjection({ model: stepBytes, options })` for STEP. Both methods
return `geometry.hlr_projection.result.a0`; model projection defaults to
`poly`, while mesh projection selects the only applicable Fast backend when
the algorithm selectors are absent.

The mesh illustration module owns projection preparation, visibility ordering,
safe surface fusion and coplanar layering, colorization, and SVG/Canvas output:

```ts
import { createIllustrator, illustrateMesh } from "@wavenumber/geometer/mesh-illustration";

const input = {
  schema: "geometry.mesh_illustration.input.a0",
  meshes,
  view: { direction: [0, 0, 1], up: [0, 1, 0] },
  style: { shading: "toon", fuse_surfaces: true },
};
const svgResult = illustrateMesh(input);

const illustrator = createIllustrator(input);
illustrator.renderCanvas(context, { shading: "unlit" });
illustrator.dispose();
```

One-shot SVG results use `geometry.mesh_illustration.result.a0`. The reusable
prepared scene stays inside the returned illustrator so its ordering and cache
internals can evolve without becoming a serialized contract.

The packed analytic Boolean operation uses `bigint` for every 64-bit identity
and integer-nanometer value. The client owns packet encoding and strict result
decoding:

```ts
const result = await client.analyticPlanarBooleanBatch({
  jobs: [{
    job_id: 1n,
    stages: [{
      stage_id: 1n,
      operation: "union",
      operands: [{
        operand_id: 1n,
        kind: "disk",
        feature_id: 1n,
        center: { x: 0n, y: 0n },
        radius_nm: 1_000_000n,
      }],
    }],
  }],
  relationship_queries: [],
});
```

Window applications can keep synchronous geometry off the UI thread with the
correlated Worker client:

```ts
import { createGeometerWorkerClient } from "@wavenumber/geometer/worker";

const worker = new Worker("./geometer-worker.js");
const client = await createGeometerWorkerClient(worker, { wasmBinary });
const bounds = await client.modelBounds({ model: stepBytes });
await client.close();
```

The Worker entry loads the separately distributed Emscripten factory and calls
`startGeometerWorkerHost` from `@wavenumber/geometer/worker-host`. Input bytes
are copied into owned buffers before transfer, so caller arrays remain usable.
Output attachments transfer back without structured-clone copies.

The environment-neutral `@wavenumber/geometer/ipc-a0` export encodes and
incrementally decodes the executable protocol's bounded binary frames. It does
not spawn or supervise a native process; Node applications supply that process
boundary separately.

`@wavenumber/geometer/ipc-client-a0` adds a persistent correlated client over
injected WHATWG readable/writable byte streams. It validates the welcome
catalog and limits, generated operation envelopes, attachments, cancellation,
and graceful shutdown. Pending request count and encoded bytes are bounded by
the negotiated queue/resident limits even when the injected writer stalls;
negotiated response limits are applied while incrementally decoding headers.
Its default catalog expectation is the portable operation set. The Node process
adapter selects the native catalog, where experimental STEP topology open,
paged inspect, render, resolve hit, close, logical-group/probe mutation, journal
checkpoint, and exact journal restore are callable. Hierarchy, general save,
and recovery analysis remain structural-only and are rejected locally. None of
the topology operations is
advertised by the portable C ABI or browser/WASM runtime.

Node applications can use the separately exported process adapter:

```ts
import { GeometerNodeProcessA0 } from "@wavenumber/geometer/node-process-a0";

const process = await GeometerNodeProcessA0.spawn("/path/to/geometer", {
  clientName: "my-tool",
  clientVersion: "1.0.0",
});
const response = await process.client.execute("geometry.model_bounds.a0", {}, [
  { name: "model", mediaType: "application/step", data: stepBytes },
]);
await process.close();
```

The adapter fixes the child arguments to `serve --stdio`, hides the process
window on Windows, bounds captured stderr, applies handshake and shutdown
timeouts, and requires a clean child exit after the shutdown acknowledgment.
Failure sends termination to the direct child, waits a bounded grace interval,
then escalates to a forced direct-child kill; it does not claim descendant-tree
termination.

The Emscripten module factory and its `.wasm` file remain separate release
artifacts. Pass `wasmBinary`, `locateFile`, or other Emscripten module options
through the optional second argument to `createGeometerWasmClient`.
