import { createRequire } from "node:module";
import { join, resolve } from "node:path";
import { fileURLToPath } from "node:url";
import { parentPort } from "node:worker_threads";

import { startGeometerWorkerHost } from "../../dist/wasm/npm/geometer/worker-host.js";

if (!parentPort) throw new Error("Geometer worker test requires a parent port.");

const root = resolve(fileURLToPath(new URL("../..", import.meta.url)));
const require = createRequire(import.meta.url);
const loadGeneratedModule = require("../wasm/load_generated_module.cjs");
const browserDist = process.env.GEOMETER_WASM_BROWSER_DIST
  ? resolve(process.env.GEOMETER_WASM_BROWSER_DIST)
  : join(root, "dist", "wasm", "browser");
const factory = loadGeneratedModule(join(browserDist, "geometer.js"));
const listeners = new Set();

parentPort.on("message", (data) => {
  for (const listener of listeners) listener({ data });
});

startGeometerWorkerHost(factory, {
  addEventListener(type, listener) {
    if (type === "message") listeners.add(listener);
  },
  postMessage(message, transfer = []) {
    parentPort.postMessage(message, transfer);
  },
});
