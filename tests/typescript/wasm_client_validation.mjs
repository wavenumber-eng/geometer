import { readFile } from "node:fs/promises";
import { createRequire } from "node:module";
import { join, resolve } from "node:path";
import { fileURLToPath } from "node:url";

import {
  createGeometerWasmClient,
  GeometerWasmTransportError,
} from "../../dist/npm/geometer/wasm.js";

const root = resolve(fileURLToPath(new URL("../..", import.meta.url)));
const require = createRequire(import.meta.url);
const loadGeneratedModule = require("../wasm/load_generated_module.cjs");
const browserDist = join(root, "dist", "wasm", "browser");
const factory = loadGeneratedModule(join(browserDist, "geometer.js"));
const client = await createGeometerWasmClient(factory, {
  wasmBinary: await readFile(join(browserDist, "geometer.wasm")),
});
const model = await readFile(
  join(root, "tests", "fixtures", "step", "embedded_models", "SOT-23.STEP"),
);
const result = await client.modelBounds({ model });
if (result.schema !== "geometry.model_bounds.a0" || result.units !== "mm") {
  throw new Error(`Unexpected model_bounds result ${JSON.stringify(result)}.`);
}
if (!result.bounds.size.every((value) => Number.isFinite(value) && value >= 0)) {
  throw new Error("model_bounds returned invalid extents.");
}
if (!client.capabilities.operations.includes("geometry.model_bounds.a0")) {
  throw new Error("Generated client did not negotiate model_bounds.");
}

let rejected = false;
try {
  client.execute("geometry.model_bounds.a0", "{}", []);
} catch (error) {
  rejected = error instanceof GeometerWasmTransportError && error.code === 1001;
}
if (!rejected) throw new Error("Generated client did not reject a missing required attachment.");

console.log(
  JSON.stringify({
    operation: "geometry.model_bounds.a0",
    size: result.bounds.size,
    genericAbi: client.capabilities.genericAbi,
  }),
);
