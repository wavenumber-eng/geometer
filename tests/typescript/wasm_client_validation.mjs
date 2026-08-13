import { readFile } from "node:fs/promises";
import { createRequire } from "node:module";
import { join, resolve } from "node:path";
import { fileURLToPath } from "node:url";

import {
  createGeometerWasmClient,
  GeometerOperationError,
  GeometerWasmTransportError,
} from "../../dist/wasm/npm/geometer/wasm.js";

const root = resolve(fileURLToPath(new URL("../..", import.meta.url)));
const require = createRequire(import.meta.url);
const loadGeneratedModule = require("../wasm/load_generated_module.cjs");
const browserDist = join(root, "dist", "wasm", "browser");
const factory = loadGeneratedModule(join(browserDist, "geometer.js"));
const module = await factory({
  wasmBinary: await readFile(join(browserDist, "geometer.wasm")),
});
const originalExecute = module._geometer_operation_execute;
const observedCalls = [];
function observeNextExecute() {
  module._geometer_operation_execute = (...args) => {
    const [, , requestPointer, requestSize, descriptorPointer, attachmentCount] = args;
    observedCalls.push({
      attachmentCount,
      descriptorPointer,
      firstAttachmentDataPointer:
        attachmentCount === 0 ? undefined : module.HEAPU32[(descriptorPointer + 24) >>> 2],
      requestPointer,
      requestSize,
    });
    return originalExecute(...args);
  };
}
const client = await createGeometerWasmClient(module);
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

let emptyModelRejected = false;
observeNextExecute();
try {
  await client.modelBounds({ model: new Uint8Array() });
} catch (error) {
  emptyModelRejected = error instanceof GeometerOperationError;
}
if (!emptyModelRejected) throw new Error("Empty model did not produce a governed operation error.");
const emptyAttachmentCall = observedCalls.at(-1);
if (
  emptyAttachmentCall?.attachmentCount !== 1 ||
  emptyAttachmentCall.firstAttachmentDataPointer !== 0
) {
  throw new Error(
    `Empty attachment data was not marshalled as a null WASM view: ${JSON.stringify(emptyAttachmentCall)}.`,
  );
}

observeNextExecute();
const emptyRequest = client.execute("geometry.model_bounds.a0", "", [
  { name: "model", mediaType: "application/step", data: model },
]);
if (emptyRequest.outcome.ok) throw new Error("Empty request JSON was not rejected by the codec.");
const emptyRequestCall = observedCalls.at(-1);
if (emptyRequestCall?.requestSize !== 0 || emptyRequestCall.requestPointer !== 0) {
  throw new Error("Empty request JSON was not marshalled as a null WASM view.");
}

// White-box the generic marshaller with an additive zero-input declaration.
// The native registry will reject the synthetic operation after the ABI call,
// which is sufficient to prove the descriptor pointer/count pair.
client.runtimeCatalog.operations.push({
  identity: "test.zero_input.a0",
  request_contract: "test.zero_input.request.a0",
  result_contract: "test.zero_input.result.a0",
  input_attachments: [],
  output_attachments: [],
});
observeNextExecute();
client.execute("test.zero_input.a0", "{}", []);
const zeroAttachmentCall = observedCalls.at(-1);
if (zeroAttachmentCall?.attachmentCount !== 0 || zeroAttachmentCall.descriptorPointer !== 0) {
  throw new Error("Zero attachments were not marshalled as a null descriptor view.");
}

const originalCatalog = module._geometer_operation_catalog_json;
for (const [label, mutate] of [
  ["request contract", (catalog) => (catalog.operations[0].request_contract = "wrong.a0")],
  ["requiredness", (catalog) => (catalog.operations[0].input_attachments[0].required = false)],
  [
    "input inventory",
    (catalog) =>
      catalog.operations[0].input_attachments.push({
        ...catalog.operations[0].input_attachments[0],
        name: "extra",
      }),
  ],
  [
    "output inventory",
    (catalog) =>
      catalog.operations[0].output_attachments.push({
        name: "extra",
        required: false,
        media_types: ["application/octet-stream"],
        max_bytes: 1,
      }),
  ],
]) {
  module._geometer_operation_catalog_json = (valueOut, errorOut) => {
    const code = originalCatalog(valueOut, errorOut);
    if (code !== 0) return code;
    const originalPointer = module.HEAPU32[valueOut >>> 2];
    const catalog = JSON.parse(module.UTF8ToString(originalPointer));
    module._geometer_free_string(originalPointer);
    mutate(catalog);
    const bytes = new TextEncoder().encode(`${JSON.stringify(catalog)}\0`);
    const replacement = module._malloc(bytes.byteLength);
    module.HEAPU8.set(bytes, replacement);
    module.HEAPU32[valueOut >>> 2] = replacement;
    return code;
  };
  let incompatibleAccepted = false;
  try {
    await createGeometerWasmClient(module);
    incompatibleAccepted = true;
  } catch (error) {
    if (!(error instanceof GeometerWasmTransportError)) throw error;
  }
  if (incompatibleAccepted) throw new Error(`Client accepted incompatible ${label}.`);
}

console.log(
  JSON.stringify({
    operation: "geometry.model_bounds.a0",
    size: result.bounds.size,
    genericAbi: client.capabilities.genericAbi,
  }),
);
