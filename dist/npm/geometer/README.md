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

The Emscripten module factory and its `.wasm` file remain separate release
artifacts. Pass `wasmBinary`, `locateFile`, or other Emscripten module options
through the optional second argument to `createGeometerWasmClient`.
