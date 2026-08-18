const fs = require("fs");
const path = require("path");
const loadGeneratedModule = require("./load_generated_module.cjs");

const root = path.resolve(__dirname, "..", "..");
const browserDist = process.env.GEOMETER_WASM_BROWSER_DIST
  ? path.resolve(process.env.GEOMETER_WASM_BROWSER_DIST)
  : path.join(root, "dist", "wasm", "browser");

const OPERATION = "geometry.analytic_planar_boolean_batch.a0";
const REQUEST_NAME = "analytic_planar_boolean_request";
const REQUEST_MEDIA = "application/vnd.wavenumber.geometer.analytic-planar-boolean-request";
const RESULT_NAME = "analytic_planar_boolean_result";
const RESULT_MEDIA = "application/vnd.wavenumber.geometer.analytic-planar-boolean-result";

function allocateBytes(module, bytes) {
  const pointer = bytes.length === 0 ? 0 : module._malloc(bytes.length);
  if (pointer) module.HEAPU8.set(bytes, pointer);
  return pointer;
}

function allocateText(module, value) {
  return allocateBytes(module, Buffer.from(value, "utf8"));
}

function readSizedBytes(module, pointer, size) {
  return Buffer.from(module.HEAPU8.subarray(pointer, pointer + size));
}

function readSizedText(module, pointer, size) {
  return readSizedBytes(module, pointer, size).toString("utf8");
}

async function execute(requestPath, outputPath) {
  const requestPacket = Buffer.from(
    fs.readFileSync(requestPath, "utf8").replace(/\s+/g, ""),
    "hex",
  );
  if (requestPacket.subarray(0, 8).toString("ascii") !== "GMABRQ01") {
    throw new Error("Cross-transport request is not a GMABRQ01 packet.");
  }

  const createGeometerModule = loadGeneratedModule(path.join(browserDist, "geometer.js"));
  const module = await createGeometerModule({
    wasmBinary: fs.readFileSync(path.join(browserDist, "geometer.wasm")),
  });
  const requestJson = JSON.stringify({
    schema: "geometry.analytic_planar_boolean_batch.request.a0",
    packet: {
      attachment: REQUEST_NAME,
      format: "geometry.analytic_planar_boolean.packet.a0",
    },
  });
  const pointers = [
    allocateText(module, OPERATION),
    allocateText(module, requestJson),
    allocateText(module, REQUEST_NAME),
    allocateText(module, REQUEST_MEDIA),
    allocateBytes(module, requestPacket),
    module._malloc(36),
    module._malloc(4),
    module._malloc(4),
    module._malloc(4),
  ];
  const [
    operationPointer,
    requestJsonPointer,
    requestNamePointer,
    requestMediaPointer,
    requestPacketPointer,
    descriptorPointer,
    resultOut,
    errorOut,
    sizeOut,
  ] = pointers;

  module.HEAPU32.set(
    [
      36,
      0,
      requestNamePointer,
      REQUEST_NAME.length,
      requestMediaPointer,
      REQUEST_MEDIA.length,
      requestPacketPointer,
      requestPacket.length,
      0,
    ],
    descriptorPointer >> 2,
  );
  module.HEAPU32[resultOut >> 2] = 0;
  module.HEAPU32[errorOut >> 2] = 0;

  let result = 0;
  try {
    const code = module._geometer_operation_execute(
      operationPointer,
      OPERATION.length,
      requestJsonPointer,
      Buffer.byteLength(requestJson),
      descriptorPointer,
      1,
      resultOut,
      errorOut,
    );
    result = module.HEAPU32[resultOut >> 2];
    const errorPointer = module.HEAPU32[errorOut >> 2];
    if (code !== 0 || result === 0 || errorPointer !== 0) {
      const detail = errorPointer === 0 ? "" : module.UTF8ToString(errorPointer);
      throw new Error(`Browser WASM analytic operation failed with code ${code}: ${detail}`);
    }

    const jsonPointer = module._geometer_operation_result_json_data(result);
    const jsonSize = module._geometer_operation_result_json_size(result);
    const outcome = JSON.parse(readSizedText(module, jsonPointer, jsonSize));
    if (!outcome.ok || outcome.result?.schema !== "geometry.analytic_planar_boolean_batch.result.a0") {
      throw new Error(`Unexpected browser WASM outcome: ${JSON.stringify(outcome)}`);
    }
    if (module._geometer_operation_result_attachment_count(result) !== 1) {
      throw new Error("Browser WASM analytic operation did not return exactly one attachment.");
    }

    const namePointer = module._geometer_operation_result_attachment_name(result, 0, sizeOut);
    const name = readSizedText(module, namePointer, module.HEAPU32[sizeOut >> 2]);
    const mediaPointer = module._geometer_operation_result_attachment_media_type(result, 0, sizeOut);
    const media = readSizedText(module, mediaPointer, module.HEAPU32[sizeOut >> 2]);
    const dataPointer = module._geometer_operation_result_attachment_data(result, 0, sizeOut);
    const data = readSizedBytes(module, dataPointer, module.HEAPU32[sizeOut >> 2]);
    if (name !== RESULT_NAME || media !== RESULT_MEDIA || data.subarray(0, 8).toString("ascii") !== "GMABRS01") {
      throw new Error("Browser WASM analytic result attachment metadata drifted.");
    }
    fs.writeFileSync(outputPath, data);
    process.stdout.write(`${JSON.stringify({ bytes: data.length })}\n`);
  } finally {
    if (result) module._geometer_operation_result_free(result);
    for (const pointer of pointers) {
      if (pointer) module._free(pointer);
    }
  }
}

if (process.argv.length !== 4) {
  console.error("usage: node analytic_cross_transport_runner.js REQUEST.hex OUTPUT.bin");
  process.exit(2);
}

execute(path.resolve(process.argv[2]), path.resolve(process.argv[3])).catch((error) => {
  console.error(error);
  process.exit(1);
});
