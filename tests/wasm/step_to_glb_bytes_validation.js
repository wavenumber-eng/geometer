const fs = require("fs");
const path = require("path");

const root = path.resolve(__dirname, "..", "..");
const browserDist = path.join(root, "dist", "wasm", "browser");
const createGeometerModule = require(path.join(browserDist, "geometer.js"));

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

  return { code, valueSize, magic, error };
}

async function main() {
  const module = await createGeometerModule({
    wasmBinary: fs.readFileSync(path.join(browserDist, "geometer.wasm")),
  });

  const version = module.ccall("geometer_version_string", "string", [], []);
  if (version !== "2026.5.24") {
    throw new Error(`Expected geometer 2026.5.24, got ${version}`);
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
  const fixture = callStepToGlbBytes(module, fs.readFileSync(fixturePath), null);
  if (fixture.code !== 0 || fixture.valueSize <= 20 || fixture.magic !== "glTF") {
    throw new Error(`Unexpected fixture STEP-to-GLB result: ${JSON.stringify(fixture)}`);
  }

  console.log(JSON.stringify({ version, fixtureBytes: fixture.valueSize, magic: fixture.magic }));
}

main().catch((error) => {
  console.error(error);
  process.exit(1);
});
