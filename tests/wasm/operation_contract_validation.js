const fs = require("fs");
const path = require("path");
const loadGeneratedModule = require("./load_generated_module.cjs");

const root = path.resolve(__dirname, "..", "..");
const browserDist = path.join(root, "dist", "wasm", "browser");
const createGeometerModule = loadGeneratedModule(path.join(browserDist, "geometer.js"));

function allocateBytes(module, bytes) {
  const pointer = bytes.length ? module._malloc(bytes.length) : 0;
  if (pointer) module.HEAPU8.set(bytes, pointer);
  return pointer;
}

function allocateText(module, text) {
  return allocateBytes(module, Buffer.from(text, "utf8"));
}

async function main() {
  const module = await createGeometerModule({
    wasmBinary: fs.readFileSync(path.join(browserDist, "geometer.wasm")),
  });
  for (const symbol of [
    "_geometer_operation_catalog_json",
    "_geometer_operation_execute",
    "_geometer_operation_result_json_data",
    "_geometer_operation_result_free",
  ]) {
    if (typeof module[symbol] !== "function") throw new Error(`${symbol} is not exported.`);
  }

  const catalogOut = module._malloc(4);
  const catalogErrorOut = module._malloc(4);
  module.HEAPU32[catalogOut >> 2] = 0;
  module.HEAPU32[catalogErrorOut >> 2] = 0;
  const catalogCode = module._geometer_operation_catalog_json(catalogOut, catalogErrorOut);
  const catalogPointer = module.HEAPU32[catalogOut >> 2];
  if (catalogCode !== 0 || !catalogPointer) throw new Error("Operation catalog lookup failed.");
  const catalog = JSON.parse(module.UTF8ToString(catalogPointer));
  module._geometer_free_string(catalogPointer);
  module._free(catalogOut);
  module._free(catalogErrorOut);
  if (!catalog.operations.some((operation) => operation.identity === "geometry.model_bounds.a0")) {
    throw new Error("model_bounds is absent from the operation catalog.");
  }
  if (catalog.attachment_descriptor.wasm32.size !== 36) {
    throw new Error("Unexpected wasm32 attachment descriptor size.");
  }

  const operation = "geometry.model_bounds.a0";
  const request = "{}";
  const name = "model";
  const mediaType = "application/step";
  const model = fs.readFileSync(
    path.join(root, "tests", "fixtures", "step", "embedded_models", "SOT-23.STEP"),
  );
  const operationPointer = allocateText(module, operation);
  const requestPointer = allocateText(module, request);
  const namePointer = allocateText(module, name);
  const mediaTypePointer = allocateText(module, mediaType);
  const modelPointer = allocateBytes(module, model);
  const descriptorPointer = module._malloc(36);
  const descriptor = descriptorPointer >> 2;
  module.HEAPU32.set(
    [36, 0, namePointer, name.length, mediaTypePointer, mediaType.length, modelPointer, model.length, 0],
    descriptor,
  );
  const resultOut = module._malloc(4);
  const errorOut = module._malloc(4);
  module.HEAPU32[resultOut >> 2] = 0;
  module.HEAPU32[errorOut >> 2] = 0;

  const code = module._geometer_operation_execute(
    operationPointer,
    operation.length,
    requestPointer,
    request.length,
    descriptorPointer,
    1,
    resultOut,
    errorOut,
  );
  const result = module.HEAPU32[resultOut >> 2];
  const error = module.HEAPU32[errorOut >> 2];
  if (code !== 0 || !result || error) throw new Error(`model_bounds failed locally with ${code}.`);
  const jsonPointer = module._geometer_operation_result_json_data(result);
  const jsonSize = module._geometer_operation_result_json_size(result);
  const outcome = JSON.parse(
    Buffer.from(module.HEAPU8.subarray(jsonPointer, jsonPointer + jsonSize)).toString("utf8"),
  );
  if (!outcome.ok || outcome.result.schema !== "geometry.model_bounds.a0") {
    throw new Error(`Unexpected model_bounds outcome: ${JSON.stringify(outcome)}`);
  }
  if (module._geometer_operation_result_attachment_count(result) !== 0) {
    throw new Error("model_bounds unexpectedly returned an attachment.");
  }

  module._geometer_operation_result_free(result);
  for (const pointer of [
    operationPointer,
    requestPointer,
    namePointer,
    mediaTypePointer,
    modelPointer,
    descriptorPointer,
    resultOut,
    errorOut,
  ]) {
    module._free(pointer);
  }
  console.log(JSON.stringify({ operation: outcome.operation, ok: outcome.ok, schema: outcome.result.schema }));
}

main().catch((error) => {
  console.error(error);
  process.exit(1);
});
