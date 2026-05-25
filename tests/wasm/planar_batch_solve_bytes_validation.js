const fs = require("fs");
const path = require("path");

const root = path.resolve(__dirname, "..", "..");
const browserDist = path.join(root, "dist", "wasm", "browser");
const createGeometerModule = require(path.join(browserDist, "geometer.js"));

const requestMagic = Buffer.from("GMPBRQ01", "ascii");
const responseMagic = Buffer.from("GMPBRS01", "ascii");
const formatVersion = 2;
const jobSubtractCommonRings = 1 << 0;
const jobFilterCommonSubtractByBounds = 1 << 1;

class Writer {
  constructor() {
    this.bytes = [];
  }

  raw(buffer) {
    for (const byte of buffer) {
      this.bytes.push(byte);
    }
  }

  u32(value) {
    const buffer = Buffer.alloc(4);
    buffer.writeUInt32LE(value >>> 0, 0);
    this.raw(buffer);
  }

  f64(value) {
    const buffer = Buffer.alloc(8);
    buffer.writeDoubleLE(Number(value), 0);
    this.raw(buffer);
  }

  ring(points) {
    this.u32(points.length);
    for (const point of points) {
      this.f64(point[0]);
      this.f64(point[1]);
    }
  }

  take() {
    return Buffer.from(this.bytes);
  }
}

class Reader {
  constructor(buffer) {
    this.buffer = buffer;
    this.offset = 0;
  }

  magic(expected) {
    const value = this.buffer.subarray(this.offset, this.offset + expected.length);
    this.offset += expected.length;
    if (!value.equals(expected)) {
      throw new Error(`Unexpected magic ${value.toString("ascii")}`);
    }
  }

  u32() {
    const value = this.buffer.readUInt32LE(this.offset);
    this.offset += 4;
    return value;
  }

  f64() {
    const value = this.buffer.readDoubleLE(this.offset);
    this.offset += 8;
    return value;
  }
}

function rect(minX, minY, maxX, maxY) {
  return [
    [minX, minY],
    [maxX, minY],
    [maxX, maxY],
    [minX, maxY],
  ];
}

function makeRequest() {
  const writer = new Writer();
  writer.raw(requestMagic);
  writer.u32(formatVersion);
  writer.u32(0);
  writer.u32(6);
  writer.u32(1);
  writer.f64(0.0);
  writer.f64(2.0);
  writer.f64(0.0);
  writer.u32(1);
  writer.u32(0);
  writer.u32(0);
  writer.u32(0);
  writer.ring(rect(1.0, 1.0, 2.0, 2.0));

  writer.u32(jobSubtractCommonRings | jobFilterCommonSubtractByBounds);
  writer.f64(0.0);
  writer.u32(1);
  writer.u32(0);
  writer.u32(0);
  writer.u32(0);
  writer.ring(rect(0.0, 0.0, 3.0, 3.0));
  return writer.take();
}

function callPlanarBatchSolveBytes(module, requestBytes) {
  const requestPtr = module._malloc(requestBytes.length);
  const valueOut = module._malloc(4);
  const valueSizeOut = module._malloc(4);
  const errorOut = module._malloc(4);
  module.HEAPU8.set(requestBytes, requestPtr);
  module.HEAPU32[valueOut >> 2] = 0;
  module.HEAPU32[valueSizeOut >> 2] = 0;
  module.HEAPU32[errorOut >> 2] = 0;

  const code = module.ccall(
    "geometer_planar_batch_solve_bytes",
    "number",
    ["number", "number", "number", "number", "number"],
    [requestPtr, requestBytes.length, valueOut, valueSizeOut, errorOut],
  );
  const valuePtr = module.getValue(valueOut, "*");
  const valueSize = module.getValue(valueSizeOut, "i32");
  const errorPtr = module.getValue(errorOut, "*");
  const error = errorPtr ? module.UTF8ToString(errorPtr) : "";
  const value = valuePtr
    ? Buffer.from(module.HEAPU8.subarray(valuePtr, valuePtr + valueSize))
    : Buffer.alloc(0);

  if (valuePtr) {
    module._geometer_free_bytes(valuePtr);
  }
  if (errorPtr) {
    module._geometer_free_string(errorPtr);
  }
  module._free(requestPtr);
  module._free(valueOut);
  module._free(valueSizeOut);
  module._free(errorOut);

  return { code, value, error };
}

async function main() {
  const module = await createGeometerModule({
    wasmBinary: fs.readFileSync(path.join(browserDist, "geometer.wasm")),
  });

  const version = module.ccall("geometer_version_string", "string", [], []);
  const abi = module.ccall("geometer_abi_version", "number", [], []);
  if (version !== "2026.5.25" || abi !== 20260525) {
    throw new Error(`Expected geometer 2026.5.25 ABI 20260525, got ${version} ABI ${abi}`);
  }
  if (typeof module._geometer_planar_batch_solve_bytes !== "function") {
    throw new Error("geometer_planar_batch_solve_bytes is not exported.");
  }

  const result = callPlanarBatchSolveBytes(module, makeRequest());
  if (result.code !== 0) {
    throw new Error(`Planar solve failed: ${result.error}`);
  }

  const reader = new Reader(result.value);
  reader.magic(responseMagic);
  if (reader.u32() !== formatVersion) {
    throw new Error("Unexpected response version.");
  }
  if (reader.u32() !== 1) {
    throw new Error("Unexpected response job count.");
  }
  if (reader.u32() !== 1) {
    throw new Error("Unexpected response region count.");
  }
  reader.u32();
  reader.u32();
  reader.u32();
  if (reader.u32() !== 1) {
    throw new Error("Unexpected job region count.");
  }
  reader.u32();
  reader.u32();
  if (reader.u32() !== 1) {
    throw new Error("Unexpected prepared subject ring count.");
  }
  const area = reader.f64();
  if (Math.abs(area - 8.0) > 1e-6) {
    throw new Error(`Unexpected area ${area}`);
  }
  if (reader.u32() !== 1) {
    throw new Error("Unexpected raw subject ring count.");
  }
  if (reader.u32() !== 0 || reader.u32() !== 0 || reader.u32() !== 0 || reader.u32() !== 1) {
    throw new Error("Unexpected job diagnostic counts.");
  }
  reader.u32();

  console.log(JSON.stringify({ version, abi, area, bytes: result.value.length }));
}

main().catch((error) => {
  console.error(error);
  process.exit(1);
});
