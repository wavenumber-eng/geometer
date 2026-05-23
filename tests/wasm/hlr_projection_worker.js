// HLR projection worker. By default it loads the current Geometer browser build
// from /dist/. Tests can pass a custom backend directory, but the old checked-in
// baseline build is no longer shipped.

let activeBackend = null;
let modulePromise = null;

function normalizeBackend(backend) {
  if (!backend || backend === "current") {
    return { dir: "/dist", factory: "createGeometerModule", script: "geometer.js", wasm: "geometer.wasm" };
  }
  // Allow callers to point at any directory under /dist/...
  return { dir: backend, factory: "createGeometerModule", script: "geometer.js", wasm: "geometer.wasm" };
}

function geometerModule(backend) {
  const desc = normalizeBackend(backend);
  const key = desc.dir;
  if (activeBackend !== key) {
    importScripts(`${desc.dir}/${desc.script}`);
    activeBackend = key;
    modulePromise = self[desc.factory]({
      locateFile: (path) => path.endsWith(".wasm") ? `${desc.dir}/${path}` : path,
    });
  }
  return modulePromise;
}

const EDGE_FLAGS = [
  "edge_v_sharp", "edge_v_outline", "edge_v_smooth", "edge_v_sewn", "edge_v_iso",
  "edge_h_sharp", "edge_h_outline", "edge_h_smooth", "edge_h_sewn", "edge_h_iso",
];

function buildOptionsJson(views, options) {
  const opts = options || {};
  const payload = {
    views,
    curve_mode: opts.curve_mode || "polyline",
    samples_per_curve: opts.samples_per_curve ?? 24,
    round_digits: opts.round_digits ?? 3,
    union_simple_polygons: opts.union_simple_polygons ?? true,
  };
  if (Array.isArray(opts.model_transform)) {
    payload.model_transform = opts.model_transform;
  }
  // Legacy aliases (still accepted by the parser for back-compat).
  if (typeof opts.include_visible === "boolean") payload.include_visible = opts.include_visible;
  if (typeof opts.include_outline === "boolean") payload.include_outline = opts.include_outline;
  // Granular OCCT HLR edge categories.
  for (const flag of EDGE_FLAGS) {
    if (typeof opts[flag] === "boolean") payload[flag] = opts[flag];
  }
  if (opts.projection_algorithm) {
    payload.projection_algorithm = opts.projection_algorithm;
  }
  if (Number.isFinite(opts.mesh_linear_deflection)) {
    payload.mesh_linear_deflection = opts.mesh_linear_deflection;
  }
  if (Number.isFinite(opts.mesh_angular_deflection)) {
    payload.mesh_angular_deflection = opts.mesh_angular_deflection;
  }
  if (typeof opts.mesh_relative === "boolean") {
    payload.mesh_relative = opts.mesh_relative;
  }
  if (Number.isFinite(opts.hlr_angle_tolerance)) {
    payload.hlr_angle_tolerance = opts.hlr_angle_tolerance;
  }
  return JSON.stringify(payload);
}

function project(module, stepBytes, views, options) {
  const optionsJson = buildOptionsJson(views, options);

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
  const { id, stepBuffer, views, options, backend } = event.data;
  const timings = {};

  try {
    const moduleStart = performance.now();
    const module = await geometerModule(backend);
    timings.moduleMs = performance.now() - moduleStart;

    const stepBytes = new Uint8Array(stepBuffer);
    const hlrStart = performance.now();
    const json = project(module, stepBytes, views, options);
    timings.hlrMs = performance.now() - hlrStart;

    const parseStart = performance.now();
    const projection = JSON.parse(json);
    timings.jsonParseMs = performance.now() - parseStart;

    // Surface per-phase native timings from the library if present.
    if (projection && projection.timings) {
      timings.stepReadMs = projection.timings.step_read_ms;
      timings.meshMs = projection.timings.mesh_ms;
      timings.nativeHlrMs = projection.timings.hlr_ms;
      timings.extractMs = projection.timings.extract_ms;
    }

    self.postMessage({ id, ok: true, projection, timings, backend: activeBackend });
  } catch (error) {
    self.postMessage({
      id,
      ok: false,
      error: error && error.stack ? error.stack : String(error),
      timings,
      backend: activeBackend,
    });
  }
};
