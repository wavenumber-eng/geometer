import { readFile } from "node:fs/promises";
import { join, resolve } from "node:path";
import { fileURLToPath } from "node:url";

import {
  decodeDiagnosticA0Json,
  decodeHlrProjectionOptionsA0Json,
  decodeHlrProjectionResultA0Json,
  decodeIpcHelloA0Json,
  decodeIpcRequestA0Json,
  decodeIpcShutdownAckA0Json,
  decodeMeshIllustrationInputA0Json,
  decodeMeshIllustrationResultA0Json,
  decodeModelBoundsOptionsA0Json,
  decodeModelBoundsResultA0Json,
  decodeOperationOutcomeA0Json,
  decodeStepTopologyAnalyzeRecoveryRequestA0Json,
  decodeStepTopologyAnalyzeRecoveryResultA0Json,
  decodeStepTopologyApplyHierarchyRequestA0Json,
  decodeStepTopologyApplyHierarchyResultA0Json,
  decodeStepTopologyApplyLogicalGroupsRequestA0Json,
  decodeStepTopologyApplyLogicalGroupsResultA0Json,
  decodeStepTopologyApplyMetadataProbesRequestA0Json,
  decodeStepTopologyApplyMetadataProbesResultA0Json,
  decodeStepTopologyCheckpointEditJournalRequestA0Json,
  decodeStepTopologyCheckpointEditJournalResultA0Json,
  decodeStepTopologyInspectResultA0Json,
  decodeStepTopologyRenderResultA0Json,
  decodeStepTopologyResolveHitRequestA0Json,
  decodeStepTopologyResolveHitResultA0Json,
  decodeStepTopologyRestoreRequestA0Json,
  decodeStepTopologyRestoreResultA0Json,
  decodeStepTopologySaveRequestA0Json,
  decodeStepTopologySaveResultA0Json,
  encodeIpcReasonA0Json,
  encodeModelBoundsOptionsA0Json,
  encodeOperationOutcomeA0Json,
  encodeStepTopologyApplyLogicalGroupsRequestA0Json,
  encodeStepTopologyResolveHitRequestA0Json,
} from "../../dist/wasm/npm/geometer/generated/index.js";
import {
  operationCatalog,
  StepTopologyInspectionAccumulator,
  validateIpcOutcomeOperationPair,
  validateIpcRequestOperationPair,
  validateStepTopologyCheckpointAttachment,
  validateStepTopologyHierarchyCommands,
  validateStepTopologyHierarchyResult,
  validateStepTopologyInspection,
  validateStepTopologyLogicalGroupCommands,
  validateStepTopologyLogicalGroupResult,
  validateStepTopologyMetadataProbeCommands,
  validateStepTopologyRecoveryRequest,
  validateStepTopologyRecoveryResults,
  validateStepTopologyRenderAttachments,
  validateStepTopologyResolveHitContext,
  validateStepTopologyRestoreAttachments,
  validateStepTopologyRestoreResult,
  validateStepTopologySaveAttachments,
} from "../../dist/wasm/npm/geometer/index.js";

const root = resolve(fileURLToPath(new URL("../..", import.meta.url)));
const vectorRoot = join(root, "tests", "contracts", "vectors");
const manifest = JSON.parse(await readFile(join(vectorRoot, "manifest.json"), "utf8"));
const decoders = {
  "geometry.common.diagnostic.a0": decodeDiagnosticA0Json,
  "geometry.hlr_projection.options.a0": decodeHlrProjectionOptionsA0Json,
  "geometry.hlr_projection.result.a0": decodeHlrProjectionResultA0Json,
  "geometry.mesh_illustration.input.a0": decodeMeshIllustrationInputA0Json,
  "geometry.mesh_illustration.result.a0": decodeMeshIllustrationResultA0Json,
  "geometry.model_bounds.a0": decodeModelBoundsResultA0Json,
  "geometry.model_bounds.options.a0": decodeModelBoundsOptionsA0Json,
  "geometer.operation.outcome.a0": decodeOperationOutcomeA0Json,
  "geometer.ipc.request.a0": decodeIpcRequestA0Json,
  "geometry.step_topology.apply_logical_groups.request.a0":
    decodeStepTopologyApplyLogicalGroupsRequestA0Json,
  "geometry.step_topology.apply_logical_groups.result.a0":
    decodeStepTopologyApplyLogicalGroupsResultA0Json,
  "geometry.step_topology.apply_metadata_probes.request.a0":
    decodeStepTopologyApplyMetadataProbesRequestA0Json,
  "geometry.step_topology.apply_metadata_probes.result.a0":
    decodeStepTopologyApplyMetadataProbesResultA0Json,
  "geometry.step_topology.checkpoint_edit_journal.request.a0":
    decodeStepTopologyCheckpointEditJournalRequestA0Json,
  "geometry.step_topology.checkpoint_edit_journal.result.a0":
    decodeStepTopologyCheckpointEditJournalResultA0Json,
  "geometry.step_topology.apply_hierarchy.request.a0":
    decodeStepTopologyApplyHierarchyRequestA0Json,
  "geometry.step_topology.apply_hierarchy.result.a0": decodeStepTopologyApplyHierarchyResultA0Json,
  "geometry.step_topology.save.result.a0": decodeStepTopologySaveResultA0Json,
  "geometry.step_topology.restore.request.a0": decodeStepTopologyRestoreRequestA0Json,
  "geometry.step_topology.restore.result.a0": decodeStepTopologyRestoreResultA0Json,
  "geometry.step_topology.analyze_recovery.request.a0":
    decodeStepTopologyAnalyzeRecoveryRequestA0Json,
  "geometry.step_topology.analyze_recovery.result.a0":
    decodeStepTopologyAnalyzeRecoveryResultA0Json,
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
        const attachments = await Promise.all(
          vector.attachments.map(async (attachment) => ({
            name: attachment.name,
            mediaType: attachment.media_type,
            data: await readFile(join(vectorRoot, attachment.file)),
          })),
        );
        await validateStepTopologyRenderAttachments(decoded, attachments);
        if (vector.expected === "accept") {
          const tampered = attachments.map((attachment, index) => ({
            ...attachment,
            data:
              index === 0
                ? Uint8Array.from(attachment.data, (value, byteIndex) =>
                    byteIndex === 0 ? value ^ 1 : value,
                  )
                : attachment.data,
          }));
          let tamperedAccepted = false;
          try {
            await validateStepTopologyRenderAttachments(decoded, tampered);
            tamperedAccepted = true;
          } catch {
            // Expected: a visual artifact is authenticated before Three.js consumes it.
          }
          if (tamperedAccepted) throw new Error("Render validation accepted tampered GLB bytes.");
        }
      } else if (vector.oracle === "step_topology_logical_group_commands") {
        validateStepTopologyLogicalGroupCommands(decoded);
      } else if (vector.oracle === "step_topology_logical_group_commands_high_fan_in") {
        const split = Math.floor(vector.fan_in / 2);
        validateStepTopologyLogicalGroupCommands({
          ...decoded,
          commands: [
            {
              kind: "create",
              authored_id: "wn.geometer.research.group.aggregate-a",
              name: "Aggregate A",
              member_handles: generatedTargetHandles(split),
            },
            {
              kind: "create",
              authored_id: "wn.geometer.research.group.aggregate-b",
              name: "Aggregate B",
              member_handles: generatedTargetHandles(vector.fan_in - split, split),
            },
          ],
        });
      } else if (vector.oracle === "step_topology_logical_group_result") {
        validateStepTopologyLogicalGroupResult(decoded);
      } else if (vector.oracle === "step_topology_logical_group_result_high_fan_in") {
        const split = Math.floor(vector.fan_in / 2);
        validateStepTopologyLogicalGroupResult({
          ...decoded,
          groups: [
            {
              ...decoded.groups[0],
              authored_id: "wn.geometer.research.group.aggregate-a",
              name: "Aggregate A",
              members: generatedTargetHandles(split).map((target_handle) => ({
                kind: "face",
                target_handle,
              })),
            },
            {
              ...decoded.groups[0],
              authored_id: "wn.geometer.research.group.aggregate-b",
              name: "Aggregate B",
              members: generatedTargetHandles(vector.fan_in - split, split).map(
                (target_handle) => ({
                  kind: "face",
                  target_handle,
                }),
              ),
            },
          ],
        });
      } else if (vector.oracle === "step_topology_metadata_probe_result") {
        validateStepTopologyLogicalGroupResult(decoded);
      } else if (vector.oracle === "step_topology_metadata_probe_commands") {
        validateStepTopologyMetadataProbeCommands(decoded);
      } else if (vector.oracle === "step_topology_checkpoint_attachment") {
        await validateStepTopologyCheckpointAttachment(
          decoded,
          await Promise.all(
            vector.attachments.map(async (attachment) => ({
              name: attachment.name,
              mediaType: attachment.media_type,
              data: await readFile(join(vectorRoot, attachment.file)),
            })),
          ),
        );
      } else if (vector.oracle === "step_topology_hierarchy_commands") {
        validateStepTopologyHierarchyCommands(decoded);
      } else if (vector.oracle === "step_topology_hierarchy_result") {
        validateStepTopologyHierarchyResult(decoded);
      } else if (vector.oracle === "step_topology_recovery_request") {
        validateStepTopologyRecoveryRequest(decoded.groups);
      } else if (vector.oracle === "step_topology_recovery_result") {
        validateStepTopologyRecoveryResults(decoded.groups);
      } else if (vector.oracle === "step_topology_save_attachments") {
        await validateStepTopologySaveAttachments(
          decodeStepTopologySaveRequestA0Json(
            await readFile(join(vectorRoot, vector.context_file)),
          ),
          decoded,
          await loadAttachments(vector),
        );
      } else if (vector.oracle === "step_topology_restore_attachments") {
        await validateStepTopologyRestoreAttachments(decoded, await loadAttachments(vector));
      } else if (vector.oracle === "step_topology_restore_result") {
        validateStepTopologyRestoreResult(
          decodeStepTopologyRestoreRequestA0Json(
            await readFile(join(vectorRoot, vector.context_file)),
          ),
          decoded,
        );
      } else if (vector.oracle === "step_topology_ipc_request_pair") {
        validateIpcRequestOperationPair(decoded);
      } else if (vector.oracle === "step_topology_ipc_result_pair") {
        validateIpcOutcomeOperationPair(decoded);
      } else if (vector.oracle === "step_topology_ipc_pair_matrix") {
        validateIpcRequestOperationPair(decoded);
        for (const [operation, contract] of vector.request_pairs) {
          if (operationCatalog[operation]?.requestContract !== contract) {
            throw new Error(`Request pair mismatch for ${operation}.`);
          }
        }
        for (const [operation, contract] of vector.result_pairs) {
          if (operationCatalog[operation]?.resultContract !== contract) {
            throw new Error(`Result pair mismatch for ${operation}.`);
          }
        }
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
  if (
    structurallyAccepted &&
    vector.contract_identity === "geometry.step_topology.apply_logical_groups.request.a0"
  ) {
    const roundTrip = encodeStepTopologyApplyLogicalGroupsRequestA0Json(decoded);
    if (roundTrip !== stored.toString("utf8").trimEnd()) {
      throw new Error(
        `${vector.id}: generated TypeScript group-command round-trip is not canonical.`,
      );
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
  const pages = [];
  for (let offset = 0; offset < fanIn; offset += 1024) {
    const handles = bodyHandles.slice(offset, offset + 1024);
    const finalBodyPage = offset + handles.length >= fanIn;
    pages.push({
      schema: "geometry.step_topology.inspect.result.a0",
      session: seed.session,
      counts: {
        definitions: 1,
        root_occurrences: 0,
        component_occurrences: 0,
        bodies: fanIn,
        shells: 1,
        faces: 0,
        memberships: fanIn,
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
          shell_count: 1,
          face_count: 0,
          bounds_mm: [0, 0, 0, 1, 1, 1],
          volume_mm3: 1,
        })),
        shells: finalBodyPage
          ? [
              {
                handle: shellHandle,
                definition_handle: definitionHandle,
                body_count: fanIn,
                face_count: 0,
              },
            ]
          : [],
        faces: [],
        memberships: [],
        next_cursor: `body-${offset + handles.length}`,
      },
      diagnostics: [],
    });
  }
  for (let offset = 0; offset < fanIn; offset += 1024) {
    const handles = bodyHandles.slice(offset, offset + 1024);
    const terminal = offset + handles.length >= fanIn;
    pages.push({
      schema: "geometry.step_topology.inspect.result.a0",
      session: seed.session,
      counts: pages[0].counts,
      page: {
        definitions: [],
        occurrences: [],
        bodies: [],
        shells: [],
        faces: [],
        memberships: handles.map((handle) => ({
          kind: "body_shell",
          owner_handle: handle,
          member_handle: shellHandle,
        })),
        ...(terminal ? {} : { next_cursor: `membership-${offset + handles.length}` }),
      },
      diagnostics: [],
    });
  }
  return pages;
}

function generatedTargetHandles(count, offset = 0) {
  return Array.from(
    { length: count },
    (_, index) => `gtt_${(offset + index + 1).toString(16).padStart(64, "0")}`,
  );
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

async function loadAttachments(vector) {
  return Promise.all(
    vector.attachments.map(async (attachment) => ({
      name: attachment.name,
      mediaType: attachment.media_type,
      data: await readFile(join(vectorRoot, attachment.file)),
    })),
  );
}

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

console.log(JSON.stringify({ vectors: manifest.vectors.length, generatedCodecs: 11 }));
