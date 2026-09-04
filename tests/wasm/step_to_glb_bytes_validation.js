const fs = require("fs");
const crypto = require("crypto");
const path = require("path");
const loadGeneratedModule = require("./load_generated_module.cjs");

const root = path.resolve(__dirname, "..", "..");
const browserDist = process.env.GEOMETER_WASM_BROWSER_DIST
  ? path.resolve(process.env.GEOMETER_WASM_BROWSER_DIST)
  : path.join(root, "dist", "wasm", "browser");
const createGeometerModule = loadGeneratedModule(path.join(browserDist, "geometer.js"));

function writeString(module, text) {
  const size = module.lengthBytesUTF8(text) + 1;
  const ptr = module._malloc(size);
  module.stringToUTF8(text, ptr, size);
  return ptr;
}

function callStepToGlbBytes(module, stepBytes, optionsJson) {
  const stepPtr = stepBytes.length > 0 ? module._malloc(stepBytes.length) : 0;
  if (stepPtr) {
    module.HEAPU8.set(stepBytes, stepPtr);
  }

  const optionsPtr = optionsJson === null ? 0 : writeString(module, optionsJson);
  const valueOut = module._malloc(4);
  const valueSizeOut = module._malloc(4);
  const errorOut = module._malloc(4);
  module.HEAPU32[valueOut >> 2] = 0;
  module.HEAPU32[valueSizeOut >> 2] = 0;
  module.HEAPU32[errorOut >> 2] = 0;

  const code = module.ccall(
    "geometer_step_to_glb_bytes",
    "number",
    ["number", "number", "number", "number", "number", "number"],
    [stepPtr, stepBytes.length, optionsPtr, valueOut, valueSizeOut, errorOut],
  );
  const valuePtr = module.getValue(valueOut, "*");
  const valueSize = module.getValue(valueSizeOut, "i32");
  const errorPtr = module.getValue(errorOut, "*");
  const error = errorPtr ? module.UTF8ToString(errorPtr) : "";
  const magic = valuePtr
    ? Buffer.from(module.HEAPU8.subarray(valuePtr, valuePtr + 4)).toString("ascii")
    : "";
  const sha256 = valuePtr
    ? crypto
        .createHash("sha256")
        .update(Buffer.from(module.HEAPU8.subarray(valuePtr, valuePtr + valueSize)))
        .digest("hex")
    : "";

  if (valuePtr) {
    module._geometer_free_bytes(valuePtr);
  }
  if (errorPtr) {
    module._geometer_free_string(errorPtr);
  }
  if (stepPtr) {
    module._free(stepPtr);
  }
  if (optionsPtr) {
    module._free(optionsPtr);
  }
  module._free(valueOut);
  module._free(valueSizeOut);
  module._free(errorOut);

  return { code, valueSize, magic, sha256, error };
}

async function main() {
  const module = await createGeometerModule({
    wasmBinary: fs.readFileSync(path.join(browserDist, "geometer.wasm")),
  });
  const initialMemoryBytes = module.HEAPU8.byteLength;

  const version = module.ccall("geometer_version_string", "string", [], []);
  if (version !== "2026.8.21") {
    throw new Error(`Expected geometer 2026.8.21, got ${version}`);
  }
  if (typeof module._geometer_step_to_glb_bytes !== "function") {
    throw new Error("geometer_step_to_glb_bytes is not exported.");
  }

  const empty = callStepToGlbBytes(module, Buffer.alloc(0), "{\"linearDeflection\":0.05}");
  if (empty.code !== 1 || !empty.error.includes("STEP byte input is empty")) {
    throw new Error(`Unexpected empty STEP result: ${JSON.stringify(empty)}`);
  }

  const fixturePath = path.join(
    root,
    "tests",
    "fixtures",
    "step",
    "embedded_models",
    "RESC1608X06N.step",
  );
  const fixtureBytes = fs.readFileSync(fixturePath);
  const fixtureRuns = [];
  for (let index = 0; index < 3; index += 1) {
    const result = callStepToGlbBytes(module, fixtureBytes, null);
    fixtureRuns.push({ ...result, heapBytes: module.HEAPU8.byteLength });
    if (result.code !== 0 || result.valueSize <= 20 || result.magic !== "glTF") {
      throw new Error(`Unexpected fixture STEP-to-GLB result: ${JSON.stringify(result)}`);
    }
    if (index > 0 && result.valueSize !== fixtureRuns[0].valueSize) {
      throw new Error("Repeated STEP-to-GLB output size changed within one runtime.");
    }
    if (index > 0 && result.sha256 !== fixtureRuns[0].sha256) {
      throw new Error("Repeated STEP-to-GLB output bytes changed within one runtime.");
    }
  }

  console.log(
    JSON.stringify({
      version,
      fixtureBytes: fixtureRuns[0].valueSize,
      fixtureSha256: fixtureRuns[0].sha256,
      magic: fixtureRuns[0].magic,
      initialMemoryBytes,
      runMemoryBytes: fixtureRuns.map((result) => result.heapBytes),
    }),
  );
}

main().catch((error) => {
  console.error(error);
  process.exit(1);
});
