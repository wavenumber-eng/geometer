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

The Emscripten module factory and its `.wasm` file remain separate release
artifacts. Pass `wasmBinary`, `locateFile`, or other Emscripten module options
through the optional second argument to `createGeometerWasmClient`.
