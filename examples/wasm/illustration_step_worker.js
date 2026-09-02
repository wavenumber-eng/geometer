// Prototype STEP adapter for the mesh illustration lab. The illustration
// algorithm itself consumes generic meshes; this Worker adapts STEP bytes to
// GLB and exposes Geometer's existing HLR mesh-shadow outline operation.

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

function stepToGlb(module, stepBytes, meshOptions = {}) {
  const stepPtr = module._malloc(stepBytes.length);
  const optionsJson = JSON.stringify({
    linear_deflection: meshOptions.linearDeflectionMm ?? 0.1,
    angular_deflection: meshOptions.angularDeflectionRad ?? 0.5,
  });
  const optionsBytes = module.lengthBytesUTF8(optionsJson) + 1;
  const optionsPtr = module._malloc(optionsBytes);
  const valueOut = module._malloc(4);
  const valueSizeOut = module._malloc(4);
  const errorOut = module._malloc(4);
  try {
    module.HEAPU8.set(stepBytes, stepPtr);
    module.HEAPU32[valueOut >> 2] = 0;
    module.HEAPU32[valueSizeOut >> 2] = 0;
    module.HEAPU32[errorOut >> 2] = 0;
    module.stringToUTF8(optionsJson, optionsPtr, optionsBytes);
    const code = module.ccall(
      "geometer_step_to_glb_bytes",
      "number",
      ["number", "number", "number", "number", "number", "number"],
      [stepPtr, stepBytes.length, optionsPtr, valueOut, valueSizeOut, errorOut],
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
    module._free(optionsPtr);
    module._free(valueOut);
    module._free(valueSizeOut);
    module._free(errorOut);
  }
}

function projectMeshShadow(module, stepBytes, view, modelTransform, hlrOptions = {}) {
  const optionsJson = JSON.stringify({
    views: [{ id: "illustration", ...view }],
    model_transform: modelTransform,
    strip_root_placement: true,
    curve_mode: "polyline",
    round_digits: 6,
    projection_algorithm: "fast",
    outline_algorithm: "fast-mesh-shadow",
    output_outline: hlrOptions.outputOutline ?? true,
    output_detail: hlrOptions.outputDetail ?? true,
    output_bbox: false,
    mesh_deflection_mode: "bbox-relative",
    mesh_angular_deflection: hlrOptions.angularDeflectionRad ?? 0.5,
    mesh_deflection_coefficient: hlrOptions.deflectionCoefficient ?? 0.004,
    union_outline_polygons: true,
    fast: {
      include_boundaries: true,
      include_creases: true,
      include_silhouettes: true,
      include_hidden: false,
      crease_angle_rad: hlrOptions.creaseAngleRad ?? (25 * Math.PI) / 180,
    },
    // Match the HLR Lab's Detail preset: visible sharp edges plus silhouettes.
    edge_v_sharp: true,
    edge_v_outline: true,
    edge_v_smooth: false,
    edge_v_sewn: false,
    edge_v_iso: false,
    edge_h_sharp: false,
    edge_h_outline: false,
    edge_h_smooth: false,
    edge_h_sewn: false,
    edge_h_iso: false,
  });
  const stepPtr = module._malloc(stepBytes.length);
  const optionsBytes = module.lengthBytesUTF8(optionsJson) + 1;
  const optionsPtr = module._malloc(optionsBytes);
  const valueOut = module._malloc(4);
  const errorOut = module._malloc(4);
  try {
    module.HEAPU8.set(stepBytes, stepPtr);
    module.stringToUTF8(optionsJson, optionsPtr, optionsBytes);
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
      if (errorPtr) module._geometer_free_string(errorPtr);
      throw new Error(message);
    }
    const json = module.UTF8ToString(valuePtr);
    module._geometer_free_string(valuePtr);
    return JSON.parse(json);
  } finally {
    module._free(stepPtr);
    module._free(optionsPtr);
    module._free(valueOut);
    module._free(errorOut);
  }
}

self.onmessage = async (event) => {
  const {
    id,
    operation = "step-to-glb",
    stepBuffer,
    view,
    modelTransform,
    meshOptions,
    hlrOptions,
  } = event.data;
  const timings = {};
  try {
    const moduleStarted = performance.now();
    const module = await geometerModule();
    timings.moduleMs = performance.now() - moduleStarted;
    const stepBytes = new Uint8Array(stepBuffer);
    if (operation === "step-to-glb") {
      const conversionStarted = performance.now();
      const glbBuffer = stepToGlb(module, stepBytes, meshOptions);
      timings.glbMs = performance.now() - conversionStarted;
      self.postMessage({ id, ok: true, operation, glbBuffer, timings }, [glbBuffer]);
      return;
    }
    if (operation !== "mesh-shadow") throw new Error(`Unsupported operation: ${operation}`);
    const projectionStarted = performance.now();
    const projection = projectMeshShadow(module, stepBytes, view, modelTransform, hlrOptions);
    timings.hlrMs = performance.now() - projectionStarted;
    self.postMessage({ id, ok: true, operation, projection, timings });
  } catch (error) {
    self.postMessage({
      id,
      ok: false,
      error: error && error.stack ? error.stack : String(error),
      timings,
    });
  }
};
