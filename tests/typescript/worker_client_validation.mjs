import { readFile } from "node:fs/promises";
import { resolve } from "node:path";
import { Worker } from "node:worker_threads";
import {
  GeometerOperationError,
  GeometerWasmTransportError,
} from "../../dist/wasm/npm/geometer/wasm.js";
import { createGeometerWorkerClient, GeometerWorkerError } from "../../dist/wasm/npm/geometer/worker.js";

class BrowserWorkerAdapter {
  constructor(worker) {
    this.worker = worker;
    this.listeners = new Map();
  }

  addEventListener(type, listener) {
    let wrapped;
    if (type === "message") wrapped = (data) => listener({ data });
    else if (type === "error") {
      wrapped = (error) => listener({ error, message: error.message });
    } else wrapped = (error) => listener({ data: error });
    this.listeners.set(listener, { type, wrapped });
    this.worker.on(type, wrapped);
  }

  removeEventListener(_type, listener) {
    const registered = this.listeners.get(listener);
    if (!registered) return;
    this.worker.off(registered.type, registered.wrapped);
    this.listeners.delete(listener);
  }

  postMessage(message, transfer) {
    this.worker.postMessage(message, transfer);
  }

  terminate() {
    void this.worker.terminate();
  }
}

const nodeWorker = new Worker(new URL("./geometer_worker_entry.mjs", import.meta.url));
const worker = new BrowserWorkerAdapter(nodeWorker);
const wasmBinaryPath = process.env.GEOMETER_WASM_BROWSER_DIST
  ? resolve(process.env.GEOMETER_WASM_BROWSER_DIST, "geometer.wasm")
  : new URL("../../dist/wasm/browser/geometer.wasm", import.meta.url);
const wasmBinary = new Uint8Array(await readFile(wasmBinaryPath));
const model = new Uint8Array(
  await readFile(new URL("../fixtures/step/embedded_models/SOT-23.STEP", import.meta.url)),
);
const wasmSize = wasmBinary.byteLength;
const modelSize = model.byteLength;
const client = await createGeometerWorkerClient(worker, { wasmBinary });

if (wasmBinary.byteLength !== wasmSize) {
  throw new Error("Worker initialization detached the caller's WASM bytes.");
}
if (!client.capabilities.operations.includes("geometry.model_bounds.a0")) {
  throw new Error("Worker handshake omitted model_bounds capability.");
}

const results = await Promise.all([
  client.modelBounds({ model }),
  client.modelBounds({ model }),
  client.modelBounds({ model }),
]);
if (model.byteLength !== modelSize) {
  throw new Error("Worker execution detached the caller's model bytes.");
}
if (
  !results.every(
    (result) =>
      result.schema === "geometry.model_bounds.a0" &&
      result.bounds.size.every((value) => Number.isFinite(value) && value >= 0),
  )
) {
  throw new Error("Correlated Worker model_bounds results are invalid.");
}

let emptyRejected = false;
try {
  await client.modelBounds({ model: new Uint8Array() });
} catch (error) {
  emptyRejected = error instanceof GeometerOperationError;
}
if (!emptyRejected) throw new Error("Worker did not preserve governed operation failures.");

let unsupportedRejected = false;
try {
  await client.execute("test.unsupported.a0", "{}", []);
} catch (error) {
  unsupportedRejected = error instanceof GeometerWasmTransportError;
}
if (!unsupportedRejected) throw new Error("Worker did not preserve local transport failures.");

const closePromise = Promise.all([client.close(), client.close()]);
let closedRejected = false;
try {
  await client.modelBounds({ model });
} catch (error) {
  closedRejected = error instanceof GeometerWorkerError;
}
if (!closedRejected) throw new Error("Closing Worker client accepted another request.");
await closePromise;

console.log(
  JSON.stringify({
    correlatedRequests: results.length,
    operation: results[0].schema,
    size: results[0].bounds.size,
  }),
);
