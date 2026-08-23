import { readFile } from "node:fs/promises";
import { join, resolve } from "node:path";
import { fileURLToPath } from "node:url";

import {
  decodeDiagnosticA0Json,
  decodeIpcHelloA0Json,
  decodeIpcShutdownAckA0Json,
  decodeModelBoundsOptionsA0Json,
  decodeModelBoundsResultA0Json,
  decodeOperationOutcomeA0Json,
  decodeStepTopologyInspectResultA0Json,
  decodeStepTopologyRenderResultA0Json,
  decodeStepTopologyResolveHitRequestA0Json,
  decodeStepTopologyResolveHitResultA0Json,
  encodeIpcReasonA0Json,
  encodeModelBoundsOptionsA0Json,
  encodeOperationOutcomeA0Json,
  encodeStepTopologyResolveHitRequestA0Json,
} from "../../dist/wasm/npm/geometer/generated/index.js";
import {
  StepTopologyInspectionAccumulator,
  validateStepTopologyInspection,
  validateStepTopologyRenderAttachments,
  validateStepTopologyResolveHitContext,
} from "../../dist/wasm/npm/geometer/index.js";

const root = resolve(fileURLToPath(new URL("../..", import.meta.url)));
const vectorRoot = join(root, "tests", "contracts", "vectors");
const manifest = JSON.parse(await readFile(join(vectorRoot, "manifest.json"), "utf8"));
const decoders = {
  "geometry.common.diagnostic.a0": decodeDiagnosticA0Json,
  "geometry.model_bounds.a0": decodeModelBoundsResultA0Json,
  "geometry.model_bounds.options.a0": decodeModelBoundsOptionsA0Json,
  "geometer.operation.outcome.a0": decodeOperationOutcomeA0Json,
  "geometry.step_topology.inspect.result.a0": decodeStepTopologyInspectResultA0Json,
  "geometry.step_topology.render.result.a0": decodeStepTopologyRenderResultA0Json,
  "geometry.step_topology.resolve_hit.request.a0": decodeStepTopologyResolveHitRequestA0Json,
  "geometry.step_topology.resolve_hit.result.a0": decodeStepTopologyResolveHitResultA0Json,
};

for (const vector of manifest.vectors) {
  const decoder = decoders[vector.contract_identity];
  if (!decoder) throw new Error(`${vector.id}: no generated TypeScript decoder.`);
  const stored = await readFile(join(vectorRoot, vector.file));
  const data =
    vector.oracle === "strict_parser_hex"
      ? Buffer.from(stored.toString("ascii").trim(), "hex")
      : stored;
  let decoded;
  let error = null;
  try {
    decoded = decoder(data);
  } catch (caught) {
    error = caught;
  }
  const structurallyAccepted = error === null;
  if (structurallyAccepted && vector.oracle.startsWith("step_topology_")) {
    try {
      if (vector.oracle === "step_topology_session") {
        validateStepTopologyResolveHitContext(decoded, {
          sessionHandle: vector.expected_session_handle,
          generation: vector.expected_generation,
        });
      } else if (vector.oracle === "step_topology_inspection") {
        if (vector.prior_pages) {
          const accumulator = new StepTopologyInspectionAccumulator();
          for (const priorPage of vector.prior_pages) {
            accumulator.addPage(
              decodeStepTopologyInspectResultA0Json(await readFile(join(vectorRoot, priorPage))),
            );
          }
          accumulator.addPage(decoded);
        } else {
          validateStepTopologyInspection(decoded);
        }
      } else if (vector.oracle === "step_topology_inspection_high_fan_in") {
        const accumulator = new StepTopologyInspectionAccumulator();
        for (const page of highFanInInspectionPages(decoded, vector.fan_in)) {
          accumulator.addPage(page);
        }
      } else if (vector.oracle === "step_topology_render_attachments") {
        await validateStepTopologyRenderAttachments(
          decoded,
          await Promise.all(
            vector.attachments.map(async (attachment) => ({
              name: attachment.name,
              mediaType: attachment.media_type,
              data: await readFile(join(vectorRoot, attachment.file)),
            })),
          ),
        );
      }
    } catch (caught) {
      error = caught;
    }
  }
  const accepted = error === null;
  if (accepted !== (vector.expected === "accept")) {
    throw new Error(
      `${vector.id}: expected ${vector.expected}, got ${accepted ? "accept" : String(error)}.`,
    );
  }
  if (accepted && vector.oracle === "presence_projection") {
    const projection = Object.fromEntries(
      vector.fields.map((field) => [field, Object.hasOwn(decoded, field) ? "present" : "absent"]),
    );
    if (JSON.stringify(projection) !== JSON.stringify(vector.expected_value)) {
      throw new Error(`${vector.id}: presence projection mismatch.`);
    }
  }
  if (
    structurallyAccepted &&
    vector.contract_identity === "geometry.step_topology.resolve_hit.request.a0"
  ) {
    const roundTrip = encodeStepTopologyResolveHitRequestA0Json(decoded);
    if (roundTrip !== stored.toString("utf8").trimEnd()) {
      throw new Error(`${vector.id}: generated TypeScript round-trip is not canonical.`);
    }
  }
}

function highFanInInspectionPages(seed, fanIn) {
  const definitionHandle = `gtt_${"a".repeat(64)}`;
  const shellHandle = `gtt_${"e".repeat(64)}`;
  const bodyHandles = Array.from(
    { length: fanIn },
    (_, index) => `gtt_${(index + 1).toString(16).padStart(64, "0")}`,
  );
  return Array.from({ length: Math.ceil(fanIn / 1024) }, (_, pageIndex) => {
    const offset = pageIndex * 1024;
    const handles = bodyHandles.slice(offset, offset + 1024);
    const terminal = offset + handles.length === fanIn;
    return {
      schema: "geometry.step_topology.inspect.result.a0",
      session: seed.session,
      counts: {
        definitions: 1,
        root_occurrences: 0,
        component_occurrences: 0,
        bodies: fanIn,
        shells: 1,
        faces: 0,
      },
      page: {
        definitions:
          offset === 0
            ? [
                {
                  handle: definitionHandle,
                  name: "high fan-in definition",
                  assembly: false,
                  body_count: fanIn,
                  face_count: 0,
                },
              ]
            : [],
        occurrences: [],
        bodies: handles.map((handle) => ({
          handle,
          definition_handle: definitionHandle,
          topology_kind: "solid",
          shell_handles: [shellHandle],
          face_handles: [],
          bounds_mm: [0, 0, 0, 1, 1, 1],
          volume_mm3: 1,
        })),
        shells: terminal
          ? [
              {
                handle: shellHandle,
                definition_handle: definitionHandle,
                body_handles: bodyHandles,
                face_handles: [],
              },
            ]
          : [],
        faces: [],
        ...(terminal ? {} : { next_cursor: `body-${offset + handles.length}` }),
      },
      diagnostics: [],
    };
  });
}

const topologyHit = decodeStepTopologyResolveHitRequestA0Json(
  await readFile(join(vectorRoot, "cases", "step-topology-resolve-hit-valid.json")),
);
if (typeof topologyHit.session.generation !== "number") {
  throw new Error("STEP topology generation must decode as a TypeScript number.");
}
encodeStepTopologyResolveHitRequestA0Json({
  ...topologyHit,
  session: { ...topologyHit.session, generation: 4_294_967_295 },
});
let oversizedGenerationAccepted = false;
try {
  encodeStepTopologyResolveHitRequestA0Json({
    ...topologyHit,
    session: { ...topologyHit.session, generation: 4_294_967_296 },
  });
  oversizedGenerationAccepted = true;
} catch {
  // Expected: Slice A generations are explicitly bounded to uint32.
}
if (oversizedGenerationAccepted) {
  throw new Error("STEP topology generation exceeded its uint32 contract.");
}

if (encodeModelBoundsOptionsA0Json({}) !== "{}") {
  throw new Error("Empty option patches must not materialize defaults.");
}
if (encodeModelBoundsOptionsA0Json({ format: "step" }) !== '{"format":"step"}') {
  throw new Error("Explicit default presence must be preserved by the encoder.");
}
let invalidUnicodeAccepted = false;
try {
  encodeModelBoundsOptionsA0Json({ format: `step\ud800` });
  invalidUnicodeAccepted = true;
} catch {
  // Expected.
}
if (invalidUnicodeAccepted) throw new Error("Encoder accepted an unpaired surrogate.");

const hello = decodeIpcHelloA0Json(
  '{"client_name":"typescript-test","client_version":"a0","protocols":["a0"]}',
);
if (hello.protocols[0] !== "a0" || encodeIpcReasonA0Json({}) !== "{}") {
  throw new Error("Generated IPC control codecs did not preserve their canonical shapes.");
}
for (const count of [-1, 1.5, 4_294_967_296]) {
  let invalidCountAccepted = false;
  try {
    decodeIpcShutdownAckA0Json(
      JSON.stringify({
        status: "complete",
        activeRequestCompleted: false,
        rejectedQueuedRequestCount: count,
      }),
    );
    invalidCountAccepted = true;
  } catch {
    // Expected: generated uint32 fields require exact nonnegative integers.
  }
  if (invalidCountAccepted) throw new Error(`Generated uint32 codec accepted ${count}.`);
}

for (const [label, encode] of [
  ["fixed tuple", () => encodeModelBoundsOptionsA0Json({ model_transform: new Array(16) })],
  [
    "variable array",
    () =>
      encodeOperationOutcomeA0Json({
        operation: "geometry.model_bounds.a0",
        ok: false,
        diagnostics: new Array(1),
      }),
  ],
]) {
  let sparseArrayAccepted = false;
  try {
    encode();
    sparseArrayAccepted = true;
  } catch {
    // Expected: every array index, including a hole, is validated.
  }
  if (sparseArrayAccepted) throw new Error(`Encoder accepted a sparse ${label}.`);
}

console.log(JSON.stringify({ vectors: manifest.vectors.length, generatedCodecs: 8 }));
