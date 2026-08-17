import { readFile } from "node:fs/promises";
import { createRequire } from "node:module";
import { join, resolve } from "node:path";
import { fileURLToPath } from "node:url";
import { makeAnalyticPolygonPourRequest } from "../../dist/wasm/demos/analytic_polygon_pour_fixture.js";
import {
  createGeometerWasmClient,
  GeometerOperationError,
  GeometerWasmTransportError,
} from "../../dist/wasm/npm/geometer/wasm.js";

const root = resolve(fileURLToPath(new URL("../..", import.meta.url)));
const require = createRequire(import.meta.url);
const loadGeneratedModule = require("../wasm/load_generated_module.cjs");
const browserDist = process.env.GEOMETER_WASM_BROWSER_DIST
  ? resolve(process.env.GEOMETER_WASM_BROWSER_DIST)
  : join(root, "dist", "wasm", "browser");
const vectorRoot = join(root, "tests", "contracts", "vectors");
const factory = loadGeneratedModule(join(browserDist, "geometer.js"));
const module = await factory({
  wasmBinary: await readFile(join(browserDist, "geometer.wasm")),
});
const originalExecute = module._geometer_operation_execute;
const observedCalls = [];
function observeNextExecute() {
  module._geometer_operation_execute = (...args) => {
    const [, , requestPointer, requestSize, descriptorPointer, attachmentCount] = args;
    observedCalls.push({
      attachmentCount,
      descriptorPointer,
      firstAttachmentDataPointer:
        attachmentCount === 0 ? undefined : module.HEAPU32[(descriptorPointer + 24) >>> 2],
      requestPointer,
      requestSize,
    });
    return originalExecute(...args);
  };
}
const client = await createGeometerWasmClient(module);
const model = await readFile(
  join(root, "tests", "fixtures", "step", "embedded_models", "SOT-23.STEP"),
);

function requireClose(actual, expected, tolerance, path) {
  const allowed =
    tolerance.absolute + tolerance.relative * Math.max(Math.abs(actual), Math.abs(expected));
  if (Math.abs(actual - expected) > allowed) {
    throw new Error(`${path}: expected ${expected}, received ${actual}, tolerance ${allowed}.`);
  }
}

function fnv1a64(data) {
  let hash = 0xcbf29ce484222325n;
  for (const value of data) {
    hash ^= BigInt(value);
    hash = BigInt.asUintN(64, hash * 0x100000001b3n);
  }
  return `fnv1a64:${hash.toString(16).padStart(16, "0")}`;
}

function compareModelBoundsProjection(actual, expected, tolerance, computedFields, modelBytes) {
  for (const field of ["schema", "units"]) {
    if (actual[field] !== expected[field]) throw new Error(`/${field}: exact value mismatch.`);
  }
  if (actual.source.format !== expected.source.format) {
    throw new Error("/source/format: exact value mismatch.");
  }
  if (
    computedFields.length !== 1 ||
    computedFields[0].path !== "/result/source/hash" ||
    expected.source.hash !== "computed:fnv1a64:model" ||
    actual.source.hash !== fnv1a64(modelBytes)
  ) {
    throw new Error("/source/hash: raw attachment FNV-1a oracle mismatch.");
  }
  for (const field of ["min", "max", "size", "center"]) {
    for (const [index, value] of actual.bounds[field].entries()) {
      requireClose(value, expected.bounds[field][index], tolerance, `/bounds/${field}/${index}`);
    }
  }
  if (!Object.values(actual.timings).every((value) => Number.isFinite(value) && value >= 0)) {
    throw new Error("Excluded timing fields must still be valid nonnegative numbers.");
  }
}

const operationManifest = JSON.parse(await readFile(join(vectorRoot, "manifest.json"), "utf8"));
if (operationManifest.operation_vectors.length !== 2) {
  throw new Error("WASM must replay every governed operation vector.");
}
for (const vector of operationManifest.operation_vectors) {
  if (!vector.runtimes.includes("browser_wasm")) continue;
  const request = await readFile(join(vectorRoot, vector.request_file), "utf8");
  const response = client.execute(vector.operation, request, [
    { name: "model", mediaType: "application/step", data: model },
  ]);
  if (vector.expected === "success") {
    if (!response.outcome.ok) throw new Error(`${vector.id}: WASM operation unexpectedly failed.`);
    const expected = JSON.parse(
      await readFile(join(vectorRoot, vector.expected_result_file), "utf8"),
    );
    compareModelBoundsProjection(
      response.outcome.result,
      expected,
      vector.tolerance,
      vector.computed_fields,
      model,
    );
  } else {
    if (response.outcome.ok)
      throw new Error(`${vector.id}: WASM operation unexpectedly succeeded.`);
    const [diagnostic] = response.outcome.diagnostics;
    const expected = vector.expected_diagnostic;
    if (
      diagnostic.code !== expected.code ||
      diagnostic.category !== expected.category ||
      diagnostic.retryable !== expected.retryable ||
      Object.hasOwn(diagnostic, "path") !== (expected.path !== "absent")
    ) {
      throw new Error(`${vector.id}: WASM governed diagnostic mismatch.`);
    }
  }
}
const result = await client.modelBounds({ model });
if (result.schema !== "geometry.model_bounds.a0" || result.units !== "mm") {
  throw new Error(`Unexpected model_bounds result ${JSON.stringify(result)}.`);
}
if (!result.bounds.size.every((value) => Number.isFinite(value) && value >= 0)) {
  throw new Error("model_bounds returned invalid extents.");
}
if (!client.capabilities.operations.includes("geometry.model_bounds.a0")) {
  throw new Error("Generated client did not negotiate model_bounds.");
}

const analytic = await client.analyticPlanarBooleanBatch({
  jobs: [
    {
      job_id: 1n,
      stages: [
        {
          stage_id: 1n,
          operation: "union",
          operands: [
            {
              operand_id: 1n,
              kind: "disk",
              feature_id: 1n,
              center: { x: 0n, y: 0n },
              radius_nm: 1_000_000n,
            },
          ],
        },
      ],
    },
  ],
  relationship_queries: [],
});
const [analyticJob] = analytic.job_results;
if (
  analyticJob?.status !== "success" ||
  analyticJob.result_regions.length !== 1 ||
  analyticJob.rings.length !== 1 ||
  analyticJob.directed_fragments.length !== 2 ||
  !analyticJob.directed_fragments.every((fragment) => fragment.kind === "circular_arc") ||
  !/^[0-9a-f]{64}$/u.test(analyticJob.digest_sha256)
) {
  throw new Error("Nonempty analytic disk solve did not decode to the expected logical result.");
}

const hero = await client.analyticPlanarBooleanBatch(makeAnalyticPolygonPourRequest(10));
const heroJob = hero.job_results.find((job) => job.job_id === 7n);
const failureJob = hero.job_results.find((job) => job.job_id === 8n);
if (
  hero.job_results.length !== 2 ||
  heroJob?.status !== "success" ||
  heroJob.result_regions.length !== 2 ||
  heroJob.rings.length !== 6 ||
  heroJob.rings.filter((ring) => ring.hole).length !== 4 ||
  heroJob.directed_fragments.length !== 16 ||
  heroJob.directed_fragments.filter((fragment) => fragment.kind === "line").length !== 8 ||
  heroJob.directed_fragments.filter((fragment) => fragment.kind === "circular_arc").length !== 8 ||
  heroJob.operand_outcomes.length !== 12 ||
  !heroJob.operand_outcomes.some(
    (outcome) => outcome.operand_id === 7002n && outcome.kind === "redundant_or_absorbed_coverage",
  ) ||
  !heroJob.operand_outcomes.some(
    (outcome) => outcome.operand_id === 7005n && outcome.kind === "subtraction_effect_survives",
  ) ||
  heroJob.digest_sha256 !== "89190b783c4c82f1c46f2984c95c813c5d0b49677fd3120a3b295be1696c42fc"
) {
  throw new Error("Polygon-pour hero fixture drifted from its pinned analytic closure.");
}
if (
  failureJob?.status !== "failure" ||
  !failureJob.diagnostics.some(
    (diagnostic) =>
      diagnostic.code === "geometer.operation.analytic_planar_boolean.unsupported_geometry",
  )
) {
  throw new Error("Polygon-pour structured-failure fixture was not job-local and governed.");
}

let rejected = false;
try {
  client.execute("geometry.model_bounds.a0", "{}", []);
} catch (error) {
  rejected = error instanceof GeometerWasmTransportError && error.code === 1001;
}
if (!rejected) throw new Error("Generated client did not reject a missing required attachment.");

let emptyModelRejected = false;
observeNextExecute();
try {
  await client.modelBounds({ model: new Uint8Array() });
} catch (error) {
  emptyModelRejected = error instanceof GeometerOperationError;
}
if (!emptyModelRejected) throw new Error("Empty model did not produce a governed operation error.");
const emptyAttachmentCall = observedCalls.at(-1);
if (
  emptyAttachmentCall?.attachmentCount !== 1 ||
  emptyAttachmentCall.firstAttachmentDataPointer !== 0
) {
  throw new Error(
    `Empty attachment data was not marshalled as a null WASM view: ${JSON.stringify(emptyAttachmentCall)}.`,
  );
}

observeNextExecute();
const emptyRequest = client.execute("geometry.model_bounds.a0", "", [
  { name: "model", mediaType: "application/step", data: model },
]);
if (emptyRequest.outcome.ok) throw new Error("Empty request JSON was not rejected by the codec.");
const emptyRequestCall = observedCalls.at(-1);
if (emptyRequestCall?.requestSize !== 0 || emptyRequestCall.requestPointer !== 0) {
  throw new Error("Empty request JSON was not marshalled as a null WASM view.");
}

// White-box the generic marshaller with an additive zero-input declaration.
// The native registry will reject the synthetic operation after the ABI call,
// which is sufficient to prove the descriptor pointer/count pair.
client.runtimeCatalog.operations.push({
  identity: "test.zero_input.a0",
  request_contract: "test.zero_input.request.a0",
  result_contract: "test.zero_input.result.a0",
  input_attachments: [],
  output_attachments: [],
});
observeNextExecute();
client.execute("test.zero_input.a0", "{}", []);
const zeroAttachmentCall = observedCalls.at(-1);
if (zeroAttachmentCall?.attachmentCount !== 0 || zeroAttachmentCall.descriptorPointer !== 0) {
  throw new Error("Zero attachments were not marshalled as a null descriptor view.");
}

const originalCatalog = module._geometer_operation_catalog_json;
for (const [label, mutate] of [
  ["request contract", (catalog) => (catalog.operations[0].request_contract = "wrong.a0")],
  ["runtime dispatch", (catalog) => (catalog.operations[0].runtime_dispatch = "logical_dto")],
  [
    "packed result format",
    (catalog) => (catalog.operations[0].result_projection.format = "wrong.packet.a0"),
  ],
  ["requiredness", (catalog) => (catalog.operations[0].input_attachments[0].required = false)],
  [
    "input inventory",
    (catalog) =>
      catalog.operations[0].input_attachments.push({
        ...catalog.operations[0].input_attachments[0],
        name: "extra",
      }),
  ],
  [
    "output inventory",
    (catalog) =>
      catalog.operations[0].output_attachments.push({
        name: "extra",
        required: false,
        media_types: ["application/octet-stream"],
        max_bytes: 1,
      }),
  ],
]) {
  module._geometer_operation_catalog_json = (valueOut, errorOut) => {
    const code = originalCatalog(valueOut, errorOut);
    if (code !== 0) return code;
    const originalPointer = module.HEAPU32[valueOut >>> 2];
    const catalog = JSON.parse(module.UTF8ToString(originalPointer));
    module._geometer_free_string(originalPointer);
    mutate(catalog);
    const bytes = new TextEncoder().encode(`${JSON.stringify(catalog)}\0`);
    const replacement = module._malloc(bytes.byteLength);
    module.HEAPU8.set(bytes, replacement);
    module.HEAPU32[valueOut >>> 2] = replacement;
    return code;
  };
  let incompatibleAccepted = false;
  try {
    await createGeometerWasmClient(module);
    incompatibleAccepted = true;
  } catch (error) {
    if (!(error instanceof GeometerWasmTransportError)) throw error;
  }
  if (incompatibleAccepted) throw new Error(`Client accepted incompatible ${label}.`);
}

console.log(
  JSON.stringify({
    operation: "geometry.model_bounds.a0",
    size: result.bounds.size,
    genericAbi: client.capabilities.genericAbi,
  }),
);
