const fs = require("fs");
const path = require("path");
const loadGeneratedModule = require("./load_generated_module.cjs");

const root = path.resolve(__dirname, "..", "..");
const browserDist = process.env.GEOMETER_WASM_BROWSER_DIST
  ? path.resolve(process.env.GEOMETER_WASM_BROWSER_DIST)
  : path.join(root, "dist", "wasm", "browser");
const createGeometerModule = loadGeneratedModule(path.join(browserDist, "geometer.js"));

function allocateBytes(module, bytes) {
  const pointer = bytes.length ? module._malloc(bytes.length) : 0;
  if (pointer) module.HEAPU8.set(bytes, pointer);
  return pointer;
}

function allocateText(module, text) {
  return allocateBytes(module, Buffer.from(text, "utf8"));
}

function emptyAnalyticRequestPacket() {
  const recordBytes = [24, 32, 24, 32, 4, 32, 24, 40, 32, 40, 48, 32, 24];
  const packet = Buffer.alloc(64 + 32 * recordBytes.length);
  packet.write("GMABRQ01", 0, "ascii");
  packet.writeUInt16LE(1, 8);
  packet.writeUInt16LE(64, 10);
  packet.writeBigUInt64LE(BigInt(packet.length), 16);
  packet.writeBigUInt64LE(64n, 24);
  packet.writeUInt32LE(recordBytes.length, 32);
  for (let index = 0; index < recordBytes.length; index += 1) {
    const entry = 64 + index * 32;
    packet.writeUInt16LE(index + 1, entry);
    packet.writeUInt16LE(1, entry + 2);
    packet.writeUInt32LE(recordBytes[index], entry + 4);
    packet.writeBigUInt64LE(BigInt(packet.length), entry + 8);
  }
  return packet;
}

async function main() {
  const module = await createGeometerModule({
    wasmBinary: fs.readFileSync(path.join(browserDist, "geometer.wasm")),
  });
  for (const symbol of [
    "_geometer_operation_catalog_json",
    "_geometer_operation_execute",
    "_geometer_operation_result_json_data",
    "_geometer_operation_result_attachment_count",
    "_geometer_operation_result_attachment_name",
    "_geometer_operation_result_attachment_media_type",
    "_geometer_operation_result_attachment_data",
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
  const analyticDeclaration = catalog.operations.find(
    (operation) => operation.identity === "geometry.analytic_planar_boolean_batch.a0",
  );
  if (
    analyticDeclaration?.runtime_dispatch !== "packed_attachment" ||
    analyticDeclaration.request_projection?.attachment_name !== "analytic_planar_boolean_request" ||
    analyticDeclaration.result_projection?.attachment_name !== "analytic_planar_boolean_result"
  ) {
    throw new Error("Packed analytic dispatch is absent from the operation catalog.");
  }
  if (catalog.attachment_descriptor.wasm32.size !== 36) {
    throw new Error("Unexpected wasm32 attachment descriptor size.");
  }
  if (
    catalog.limits.response_json_bytes !== 8 * 1024 * 1024 ||
    catalog.limits.attachment_count !== 16 ||
    catalog.limits.aggregate_attachment_bytes_wasm !== 256 * 1024 * 1024
  ) {
    throw new Error("Unexpected generic ABI response limits in the generated catalog.");
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

  const analyticOperation = "geometry.analytic_planar_boolean_batch.a0";
  const analyticRequest = JSON.stringify({
    schema: "geometry.analytic_planar_boolean_batch.request.a0",
    packet: {
      attachment: "analytic_planar_boolean_request",
      format: "geometry.analytic_planar_boolean.packet.a0",
    },
  });
  const analyticName = "analytic_planar_boolean_request";
  const analyticMediaType =
    "application/vnd.wavenumber.geometer.analytic-planar-boolean-request";
  const analyticPacket = emptyAnalyticRequestPacket();
  const analyticPointers = [
    allocateText(module, analyticOperation),
    allocateText(module, analyticRequest),
    allocateText(module, analyticName),
    allocateText(module, analyticMediaType),
    allocateBytes(module, analyticPacket),
    module._malloc(36),
    module._malloc(4),
    module._malloc(4),
    module._malloc(4),
  ];
  const [
    analyticOperationPointer,
    analyticRequestPointer,
    analyticNamePointer,
    analyticMediaPointer,
    analyticPacketPointer,
    analyticDescriptorPointer,
    analyticResultOut,
    analyticErrorOut,
    analyticSizeOut,
  ] = analyticPointers;
  module.HEAPU32.set(
    [
      36,
      0,
      analyticNamePointer,
      analyticName.length,
      analyticMediaPointer,
      analyticMediaType.length,
      analyticPacketPointer,
      analyticPacket.length,
      0,
    ],
    analyticDescriptorPointer >> 2,
  );
  module.HEAPU32[analyticResultOut >> 2] = 0;
  module.HEAPU32[analyticErrorOut >> 2] = 0;
  const analyticCode = module._geometer_operation_execute(
    analyticOperationPointer,
    analyticOperation.length,
    analyticRequestPointer,
    Buffer.byteLength(analyticRequest),
    analyticDescriptorPointer,
    1,
    analyticResultOut,
    analyticErrorOut,
  );
  const analyticResult = module.HEAPU32[analyticResultOut >> 2];
  if (analyticCode !== 0 || !analyticResult || module.HEAPU32[analyticErrorOut >> 2]) {
    throw new Error(`Packed analytic operation failed locally with ${analyticCode}.`);
  }
  const analyticJsonPointer = module._geometer_operation_result_json_data(analyticResult);
  const analyticJsonSize = module._geometer_operation_result_json_size(analyticResult);
  const analyticOutcome = JSON.parse(
    Buffer.from(
      module.HEAPU8.subarray(analyticJsonPointer, analyticJsonPointer + analyticJsonSize),
    ).toString("utf8"),
  );
  if (
    !analyticOutcome.ok ||
    analyticOutcome.result.schema !== "geometry.analytic_planar_boolean_batch.result.a0" ||
    module._geometer_operation_result_attachment_count(analyticResult) !== 1
  ) {
    throw new Error(`Unexpected packed analytic outcome: ${JSON.stringify(analyticOutcome)}`);
  }
  const resultNamePointer = module._geometer_operation_result_attachment_name(
    analyticResult,
    0,
    analyticSizeOut,
  );
  const resultNameSize = module.HEAPU32[analyticSizeOut >> 2];
  const resultName = Buffer.from(
    module.HEAPU8.subarray(resultNamePointer, resultNamePointer + resultNameSize),
  ).toString("utf8");
  const resultMediaPointer = module._geometer_operation_result_attachment_media_type(
    analyticResult,
    0,
    analyticSizeOut,
  );
  const resultMediaSize = module.HEAPU32[analyticSizeOut >> 2];
  const resultMedia = Buffer.from(
    module.HEAPU8.subarray(resultMediaPointer, resultMediaPointer + resultMediaSize),
  ).toString("utf8");
  const resultDataPointer = module._geometer_operation_result_attachment_data(
    analyticResult,
    0,
    analyticSizeOut,
  );
  const resultDataSize = module.HEAPU32[analyticSizeOut >> 2];
  const resultMagic = Buffer.from(
    module.HEAPU8.subarray(resultDataPointer, resultDataPointer + 8),
  ).toString("ascii");
  if (
    resultName !== "analytic_planar_boolean_result" ||
    resultMedia !== "application/vnd.wavenumber.geometer.analytic-planar-boolean-result" ||
    resultMagic !== "GMABRS01"
  ) {
    throw new Error("Packed analytic result attachment metadata or magic drifted.");
  }
  module._geometer_operation_result_free(analyticResult);

  module.HEAPU8[analyticPacketPointer] = "X".charCodeAt(0);
  module.HEAPU32[analyticResultOut >> 2] = 0;
  module.HEAPU32[analyticErrorOut >> 2] = 0;
  const malformedCode = module._geometer_operation_execute(
    analyticOperationPointer,
    analyticOperation.length,
    analyticRequestPointer,
    Buffer.byteLength(analyticRequest),
    analyticDescriptorPointer,
    1,
    analyticResultOut,
    analyticErrorOut,
  );
  const malformedResult = module.HEAPU32[analyticResultOut >> 2];
  if (malformedCode !== 0 || !malformedResult || module.HEAPU32[analyticErrorOut >> 2]) {
    throw new Error(`Malformed packed analytic request failed locally with ${malformedCode}.`);
  }
  const malformedJsonPointer = module._geometer_operation_result_json_data(malformedResult);
  const malformedJsonSize = module._geometer_operation_result_json_size(malformedResult);
  const malformedOutcome = JSON.parse(
    Buffer.from(
      module.HEAPU8.subarray(
        malformedJsonPointer,
        malformedJsonPointer + malformedJsonSize,
      ),
    ).toString("utf8"),
  );
  if (
    malformedOutcome.ok ||
    malformedOutcome.diagnostics?.[0]?.code !==
      "geometer.contract.analytic_planar_boolean_packet.invalid_packet" ||
    module._geometer_operation_result_attachment_count(malformedResult) !== 0
  ) {
    throw new Error(`Unexpected malformed analytic outcome: ${JSON.stringify(malformedOutcome)}`);
  }
  module._geometer_operation_result_free(malformedResult);
  for (const pointer of analyticPointers) module._free(pointer);

  console.log(
    JSON.stringify({
      operation: outcome.operation,
      ok: outcome.ok,
      schema: outcome.result.schema,
      analyticBytes: resultDataSize,
    }),
  );
}

main().catch((error) => {
  console.error(error);
  process.exit(1);
});
