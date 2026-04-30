importScripts("/dist/geometer-browser.js");

let modulePromise = null;

function geometerModule() {
  if (!modulePromise) {
    modulePromise = createGeometerModule({
      locateFile: (path) => path.endsWith(".wasm") ? `/dist/${path}` : path,
    });
  }
  return modulePromise;
}

function project(module, stepBytes, views) {
  const options = JSON.stringify({
    views,
    curve_mode: "polyline",
    samples_per_curve: 24,
    round_digits: 3,
    include_visible: true,
    include_outline: true,
    union_simple_polygons: true,
  });

  const stepPtr = module._malloc(stepBytes.length);
  const optionsBytes = module.lengthBytesUTF8(options) + 1;
  const optionsPtr = module._malloc(optionsBytes);
  const valueOut = module._malloc(4);
  const errorOut = module._malloc(4);

  try {
    module.HEAPU8.set(stepBytes, stepPtr);
    module.stringToUTF8(options, optionsPtr, optionsBytes);
    module.HEAPU32[valueOut >> 2] = 0;
    module.HEAPU32[errorOut >> 2] = 0;

    const code = module.ccall(
      "geometer_step_hlr_projection_json_bytes",
      "number",
      ["number", "number", "number", "number", "number"],
      [stepPtr, stepBytes.length, optionsPtr, valueOut, errorOut],
    );

    const valuePtr = module.getValue(valueOut, "i32");
    const errorPtr = module.getValue(errorOut, "i32");
    if (code !== 0) {
      const message = errorPtr ? module.UTF8ToString(errorPtr) : `HLR failed: ${code}`;
      if (errorPtr) {
        module._geometer_free_string(errorPtr);
      }
      throw new Error(message);
    }

    const json = module.UTF8ToString(valuePtr);
    module._geometer_free_string(valuePtr);
    return json;
  } finally {
    module._free(stepPtr);
    module._free(optionsPtr);
    module._free(valueOut);
    module._free(errorOut);
  }
}

self.onmessage = async (event) => {
  const { id, stepBuffer, views } = event.data;
  const timings = {};

  try {
    const moduleStart = performance.now();
    const module = await geometerModule();
    timings.moduleMs = performance.now() - moduleStart;

    const stepBytes = new Uint8Array(stepBuffer);
    const hlrStart = performance.now();
    const json = project(module, stepBytes, views);
    timings.hlrMs = performance.now() - hlrStart;

    const parseStart = performance.now();
    const projection = JSON.parse(json);
    timings.jsonParseMs = performance.now() - parseStart;

    self.postMessage({ id, ok: true, projection, timings });
  } catch (error) {
    self.postMessage({
      id,
      ok: false,
      error: error && error.stack ? error.stack : String(error),
      timings,
    });
  }
};
