// Prototype STEP adapter for the mesh illustration lab. The illustration
// algorithm itself consumes generic meshes; this Worker only converts local
// STEP bytes to GLB using Geometer's existing compatibility C ABI.

let modulePromise = null;

function geometerModule() {
  if (!modulePromise) {
    importScripts("/dist/wasm/browser/geometer.js");
    modulePromise = self.createGeometerModule({
      locateFile: (path) =>
        path.endsWith(".wasm") ? `/dist/wasm/browser/${path}` : path,
    });
  }
  return modulePromise;
}

function stepToGlb(module, stepBytes) {
  const stepPtr = module._malloc(stepBytes.length);
  const valueOut = module._malloc(4);
  const valueSizeOut = module._malloc(4);
  const errorOut = module._malloc(4);
  try {
    module.HEAPU8.set(stepBytes, stepPtr);
    module.HEAPU32[valueOut >> 2] = 0;
    module.HEAPU32[valueSizeOut >> 2] = 0;
    module.HEAPU32[errorOut >> 2] = 0;
    const code = module.ccall(
      "geometer_step_to_glb_bytes",
      "number",
      ["number", "number", "number", "number", "number", "number"],
      [stepPtr, stepBytes.length, 0, valueOut, valueSizeOut, errorOut],
    );
    const valuePtr = module.getValue(valueOut, "i32");
    const valueSize = module.getValue(valueSizeOut, "i32");
    const errorPtr = module.getValue(errorOut, "i32");
    if (code !== 0) {
      const message = errorPtr ? module.UTF8ToString(errorPtr) : `STEP to GLB failed: ${code}`;
      if (errorPtr) module._geometer_free_string(errorPtr);
      throw new Error(message);
    }
    const result = module.HEAPU8.slice(valuePtr, valuePtr + valueSize);
    module._geometer_free_bytes(valuePtr);
    return result.buffer;
  } finally {
    module._free(stepPtr);
    module._free(valueOut);
    module._free(valueSizeOut);
    module._free(errorOut);
  }
}

self.onmessage = async (event) => {
  const { id, stepBuffer } = event.data;
  const timings = {};
  try {
    const moduleStarted = performance.now();
    const module = await geometerModule();
    timings.moduleMs = performance.now() - moduleStarted;
    const conversionStarted = performance.now();
    const glbBuffer = stepToGlb(module, new Uint8Array(stepBuffer));
    timings.glbMs = performance.now() - conversionStarted;
    self.postMessage({ id, ok: true, glbBuffer, timings }, [glbBuffer]);
  } catch (error) {
    self.postMessage({
      id,
      ok: false,
      error: error && error.stack ? error.stack : String(error),
      timings,
    });
  }
};
