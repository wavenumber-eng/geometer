const fs = require("fs");
const path = require("path");
const { performance } = require("perf_hooks");
const loadGeneratedModule = require("./load_generated_module.cjs");

const root = path.resolve(__dirname, "..", "..");
const browserDist = path.join(root, "dist", "wasm", "browser");
const createGeometerModule = loadGeneratedModule(path.join(browserDist, "geometer.js"));

function parseArgs(argv) {
  const args = {
    requestPath: "",
    responsePath: "",
    repeat: 5,
    warmup: 1,
    metricsPath: "",
  };
  for (let index = 0; index < argv.length; index += 1) {
    const arg = argv[index];
    if (arg === "--repeat" && index + 1 < argv.length) {
      args.repeat = Math.max(1, Math.trunc(Number(argv[++index]) || 1));
    } else if (arg === "--warmup" && index + 1 < argv.length) {
      args.warmup = Math.max(0, Math.trunc(Number(argv[++index]) || 0));
    } else if (arg === "--metrics" && index + 1 < argv.length) {
      args.metricsPath = argv[++index];
    } else if (!args.requestPath) {
      args.requestPath = arg;
    } else if (!args.responsePath) {
      args.responsePath = arg;
    }
  }
  if (!args.requestPath) {
    throw new Error(
      "Usage: node tests/wasm/planar_batch_solve_bytes_benchmark.js request.bin [response.bin] [--warmup N] [--repeat N] [--metrics metrics.json]",
    );
  }
  return args;
}

function callPlanarBatchSolveBytes(module, requestBytes) {
  const requestPtr = module._malloc(requestBytes.length);
  const valueOut = module._malloc(4);
  const valueSizeOut = module._malloc(4);
  const errorOut = module._malloc(4);
  try {
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
    if (code !== 0) {
      throw new Error(error || `Geometer planar solve failed with code ${code}`);
    }
    return value;
  } finally {
    module._free(requestPtr);
    module._free(valueOut);
    module._free(valueSizeOut);
    module._free(errorOut);
  }
}

function summarize(values) {
  const sorted = values.slice().sort((a, b) => a - b);
  const sum = values.reduce((total, value) => total + value, 0);
  return {
    minMs: sorted[0],
    meanMs: sum / values.length,
    maxMs: sorted[sorted.length - 1],
    lastMs: values[values.length - 1],
  };
}

async function main() {
  const args = parseArgs(process.argv.slice(2));
  const requestBytes = fs.readFileSync(args.requestPath);
  const moduleStart = performance.now();
  const module = await createGeometerModule({
    wasmBinary: fs.readFileSync(path.join(browserDist, "geometer.wasm")),
  });
  const moduleMs = performance.now() - moduleStart;
  const version = module.ccall("geometer_version_string", "string", [], []);
  const abi = module.ccall("geometer_abi_version", "number", [], []);
  if (typeof module._geometer_planar_batch_solve_bytes !== "function") {
    throw new Error("geometer_planar_batch_solve_bytes is not exported.");
  }

  for (let index = 0; index < args.warmup; index += 1) {
    callPlanarBatchSolveBytes(module, requestBytes);
  }

  let responseBytes = Buffer.alloc(0);
  const timings = [];
  for (let index = 0; index < args.repeat; index += 1) {
    const started = performance.now();
    responseBytes = callPlanarBatchSolveBytes(module, requestBytes);
    timings.push(performance.now() - started);
  }
  if (args.responsePath) {
    fs.writeFileSync(args.responsePath, responseBytes);
  }
  const metrics = Object.assign(summarize(timings), {
    version,
    abi,
    requestBytes: requestBytes.length,
    responseBytes: responseBytes.length,
    warmup: args.warmup,
    repeat: args.repeat,
    moduleMs,
  });
  if (args.metricsPath) {
    fs.writeFileSync(args.metricsPath, `${JSON.stringify(metrics, null, 2)}\n`);
  }
  process.stdout.write(`${JSON.stringify(metrics, null, 2)}\n`);
}

main().catch((error) => {
  console.error(error && error.stack ? error.stack : error);
  process.exit(1);
});
