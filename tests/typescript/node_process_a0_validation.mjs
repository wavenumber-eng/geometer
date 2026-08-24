import assert from "node:assert/strict";
import { EventEmitter } from "node:events";
import { readFile } from "node:fs/promises";
import { dirname, join } from "node:path";
import { fileURLToPath } from "node:url";

import { GeometerNodeProcessA0 } from "../../dist/wasm/npm/geometer/node-process-a0.js";
import {
  ChildProcessObservationA0,
  ChildTerminationA0,
} from "../../dist/wasm/npm/geometer/node-process-termination-a0.js";

const root = join(dirname(fileURLToPath(import.meta.url)), "..", "..");
const fixtureDirectory = join(root, "tests", "typescript", "node_process_fixture");
const platformDirectory = {
  darwin: "macos-arm64",
  linux: "linux-x64",
  win32: "windows-x64",
}[process.platform];
if (platformDirectory === undefined) {
  throw new Error(`No native Geometer test artifact is defined for ${process.platform}.`);
}
const executable = join(
  root,
  "dist",
  "native",
  platformDirectory,
  process.platform === "win32" ? "geometer.exe" : "geometer",
);
const model = new Uint8Array(
  await readFile(join(root, "tests", "fixtures", "step", "embedded_models", "SOT-23.STEP")),
);

const processClient = await GeometerNodeProcessA0.spawn(executable, {
  clientName: "node-process-a0-validation",
  clientVersion: "a0",
});
assert.ok(processClient.processId > 0);

const opened = await processClient.client.execute(
  "geometry.step_topology.open.a0",
  { schema: "geometry.step_topology.open.request.a0" },
  [{ data: model, mediaType: "application/step", name: "step" }],
);
assert.equal(opened.outcome.ok, true);
assert.equal(opened.outcome.result.schema, "geometry.step_topology.open.result.a0");
assert.equal(opened.outcome.result.source.bytes, model.byteLength);
assert.match(opened.outcome.result.session.session_handle, /^gts_[0-9a-f]{64}$/);
const topologySession = opened.outcome.result.session;

const staleInspection = await processClient.client.execute("geometry.step_topology.inspect.a0", {
  include_diagnostics: false,
  include_source_entity_evidence: false,
  page: { limit: 1 },
  schema: "geometry.step_topology.inspect.request.a0",
  session: { ...topologySession, generation: topologySession.generation + 1 },
});
assert.equal(staleInspection.outcome.ok, false);
assert.equal(
  staleInspection.outcome.diagnostics[0].code,
  "geometer.operation.step_topology.stale_generation",
);

const deferredDiagnostics = await processClient.client.execute(
  "geometry.step_topology.inspect.a0",
  {
    include_diagnostics: true,
    include_source_entity_evidence: false,
    page: { limit: 1 },
    schema: "geometry.step_topology.inspect.request.a0",
    session: topologySession,
  },
);
assert.equal(deferredDiagnostics.outcome.ok, false);
assert.equal(
  deferredDiagnostics.outcome.diagnostics[0].code,
  "geometer.operation.step_topology.unsupported_option",
);

let cursor;
let pagedItems = 0;
let expectedItems;
let selectedFaceHandle;
do {
  const inspected = await processClient.client.execute("geometry.step_topology.inspect.a0", {
    include_diagnostics: false,
    include_source_entity_evidence: true,
    page: { ...(cursor === undefined ? {} : { cursor }), limit: 2 },
    schema: "geometry.step_topology.inspect.request.a0",
    session: topologySession,
  });
  assert.equal(inspected.outcome.ok, true);
  const result = inspected.outcome.result;
  const count =
    result.page.definitions.length +
    result.page.occurrences.length +
    result.page.bodies.length +
    result.page.shells.length +
    result.page.faces.length +
    result.page.memberships.length;
  assert.ok(count > 0 && count <= 2);
  pagedItems += count;
  selectedFaceHandle ??= result.page.faces[0]?.handle;
  expectedItems ??=
    result.counts.definitions +
    result.counts.root_occurrences +
    result.counts.component_occurrences +
    result.counts.bodies +
    result.counts.shells +
    result.counts.faces +
    result.counts.memberships;
  cursor = result.page.next_cursor;
} while (cursor !== undefined);
assert.equal(pagedItems, expectedItems);
assert.match(selectedFaceHandle, /^gtt_[0-9a-f]{64}$/);

const rendered = await processClient.client.execute("geometry.step_topology.render.a0", {
  schema: "geometry.step_topology.render.request.a0",
  session: topologySession,
  tessellation: {
    angular_deflection_rad: 0.5,
    linear_deflection_mm: 0.1,
    parallel: false,
    relative: false,
    source_to_render: [1, 0, 0, 0, 0, 0, 1, 0, 0, -1, 0, 0],
  },
});
assert.equal(rendered.outcome.ok, true);
assert.equal(rendered.outcome.result.schema, "geometry.step_topology.render.result.a0");
assert.equal(rendered.attachments.length, 1);
assert.equal(rendered.attachments[0].name, "glb");
assert.equal(rendered.attachments[0].mediaType, "model/gltf-binary");
assert.equal(rendered.attachments[0].data.byteLength, rendered.outcome.result.glb.bytes);

const glb = rendered.attachments[0].data;
const glbView = new DataView(glb.buffer, glb.byteOffset, glb.byteLength);
assert.equal(glbView.getUint32(0, true), 0x46546c67);
assert.equal(glbView.getUint32(12 + 4, true), 0x4e4f534a);
const jsonLength = glbView.getUint32(12, true);
const glbJson = JSON.parse(new TextDecoder().decode(glb.subarray(20, 20 + jsonLength)).trimEnd());
const nodeBinding = glbJson.nodes[0].extras.wn_geometer;
const primitiveBinding = glbJson.meshes[nodeBinding.mesh_index].primitives[0].extras.wn_geometer;
const resolved = await processClient.client.execute("geometry.step_topology.resolve_hit.a0", {
  artifact_handle: rendered.outcome.result.artifact.artifact_handle,
  body_handle: primitiveBinding.body_handle,
  content_sha256: rendered.outcome.result.artifact.content_sha256,
  face_handle: primitiveBinding.face_handle,
  instance_index: nodeBinding.instance_index,
  occurrence_handle: nodeBinding.occurrence_handle,
  primitive_index: primitiveBinding.primitive_index,
  primitive_triangle_index: 0,
  schema: "geometry.step_topology.resolve_hit.request.a0",
  session: topologySession,
});
assert.equal(resolved.outcome.ok, true);
assert.equal(resolved.outcome.result.occurrence_handle, nodeBinding.occurrence_handle);
assert.equal(resolved.outcome.result.body_handle, primitiveBinding.body_handle);
assert.equal(resolved.outcome.result.face_handle, primitiveBinding.face_handle);
assert.equal(resolved.outcome.result.triangle_index, primitiveBinding.first_triangle);

const groupId = "wn.geometer.research.group.node-reference";
const grouped = await processClient.client.execute(
  "geometry.step_topology.apply_logical_groups.a0",
  {
    commands: [
      {
        authored_id: groupId,
        kind: "create",
        member_handles: [selectedFaceHandle],
        name: "Node reference face",
      },
    ],
    schema: "geometry.step_topology.apply_logical_groups.request.a0",
    session: topologySession,
  },
);
assert.equal(grouped.outcome.ok, true);
assert.equal(grouped.outcome.result.groups.length, 1);
assert.equal(grouped.outcome.result.groups[0].authored_id, groupId);
assert.match(grouped.outcome.result.groups[0].members[0].target_handle, /^gtt_[0-9a-f]{64}$/);
assert.notEqual(grouped.outcome.result.groups[0].members[0].target_handle, selectedFaceHandle);

const probeId = "wn.geometer.research.probe.node-reference";
const probed = await processClient.client.execute(
  "geometry.step_topology.apply_metadata_probes.a0",
  {
    commands: [
      {
        authored_id: probeId,
        key: "wn.geometer.research.probe.key.note",
        kind: "attach",
        target: { group_authored_id: groupId, kind: "logical_group" },
        value: "student-reference",
      },
    ],
    schema: "geometry.step_topology.apply_metadata_probes.request.a0",
    session: grouped.outcome.result.state.session,
  },
);
assert.equal(probed.outcome.ok, true);
assert.equal(probed.outcome.result.probes.length, 1);
assert.equal(probed.outcome.result.probes[0].authored_id, probeId);

const checkpointed = await processClient.client.execute(
  "geometry.step_topology.checkpoint_edit_journal.a0",
  {
    schema: "geometry.step_topology.checkpoint_edit_journal.request.a0",
    session: probed.outcome.result.state.session,
  },
);
assert.equal(checkpointed.outcome.ok, true);
assert.equal(checkpointed.attachments.length, 1);
assert.equal(checkpointed.attachments[0].name, "edit_journal");
assert.equal(
  checkpointed.attachments[0].data.byteLength,
  checkpointed.outcome.result.journal.bytes,
);

const closedTopology = await processClient.client.execute("geometry.step_topology.close.a0", {
  schema: "geometry.step_topology.close.request.a0",
  session: checkpointed.outcome.result.state.session,
});
assert.equal(closedTopology.outcome.ok, true);
assert.equal(closedTopology.outcome.result.closed, true);

const restoreRequest = {
  include_diagnostics: false,
  replay_preconditions: {
    occt_version: checkpointed.outcome.result.occt_version,
    source_brep_sha256: checkpointed.outcome.result.source_brep_sha256,
    source_sha256: checkpointed.outcome.result.source_sha256,
    target_inventory_sha256: checkpointed.outcome.result.target_inventory_sha256,
    transaction_count: checkpointed.outcome.result.transaction_count,
  },
  schema: "geometry.step_topology.restore.request.a0",
  source: opened.outcome.result.source,
  state_artifact: {
    bytes: checkpointed.outcome.result.journal.bytes,
    carrier: "edit_journal",
    format: "geometer.step_topology_edit_journal.a0",
    media_type: "application/vnd.wavenumber.geometer.step-topology-edit-journal",
    name: "state_artifact",
    sha256: checkpointed.outcome.result.journal.sha256,
  },
};
const restoreAttachments = [
  { data: model, mediaType: "application/step", name: "source" },
  {
    data: checkpointed.attachments[0].data,
    mediaType: "application/vnd.wavenumber.geometer.step-topology-edit-journal",
    name: "state_artifact",
  },
];
const rejectedRestore = await processClient.client.execute(
  "geometry.step_topology.restore.a0",
  {
    ...restoreRequest,
    replay_preconditions: {
      ...restoreRequest.replay_preconditions,
      transaction_count: restoreRequest.replay_preconditions.transaction_count + 1,
    },
  },
  restoreAttachments,
);
assert.equal(rejectedRestore.outcome.ok, false);
assert.equal(
  rejectedRestore.outcome.diagnostics[0].code,
  "geometer.operation.step_topology.restore_failed",
);
assert.match(rejectedRestore.outcome.diagnostics[0].message, /replay preconditions/i);

const restored = await processClient.client.execute(
  "geometry.step_topology.restore.a0",
  restoreRequest,
  restoreAttachments,
);
assert.equal(restored.outcome.ok, true);
assert.notEqual(restored.outcome.result.session.session_handle, topologySession.session_handle);
assert.equal(
  restored.outcome.result.replayed_transaction_count,
  checkpointed.outcome.result.transaction_count,
);

const renamedAfterRestore = await processClient.client.execute(
  "geometry.step_topology.apply_logical_groups.a0",
  {
    commands: [
      {
        authored_id: groupId,
        expected_revision: 1,
        kind: "rename",
        name: "Restored node reference face",
      },
    ],
    schema: "geometry.step_topology.apply_logical_groups.request.a0",
    session: restored.outcome.result.session,
  },
);
assert.equal(renamedAfterRestore.outcome.ok, true);
assert.equal(renamedAfterRestore.outcome.result.groups[0].name, "Restored node reference face");

const closedRestored = await processClient.client.execute("geometry.step_topology.close.a0", {
  schema: "geometry.step_topology.close.request.a0",
  session: renamedAfterRestore.outcome.result.state.session,
});
assert.equal(closedRestored.outcome.ok, true);

const response = await processClient.client.execute("geometry.model_bounds.a0", {}, [
  { data: model, mediaType: "application/step", name: "model" },
]);
assert.equal(response.attachments.length, 0);
assert.equal(response.outcome.operation, "geometry.model_bounds.a0");
assert.equal(response.outcome.ok, true);
assert.equal(response.outcome.result.schema, "geometry.model_bounds.a0");
assert.equal(response.outcome.result.units, "mm");
assert.ok(
  response.outcome.result.bounds.size.every((value) => Number.isFinite(value) && value > 0),
);

const shutdown = await processClient.close("node process validation complete");
assert.equal(shutdown.status, "complete");
assert.equal(processClient.stderrText(), "");

await assert.rejects(
  GeometerNodeProcessA0.spawn(join(fixtureDirectory, "does-not-exist"), {
    clientName: "node-process-missing-executable",
    clientVersion: "a0",
    connectTimeoutMs: 500,
  }),
  /ENOENT|not found|cannot find/i,
);

await assert.rejects(
  GeometerNodeProcessA0.spawn(process.execPath, {
    clientName: "node-process-early-exit",
    clientVersion: "a0",
    connectTimeoutMs: 1_000,
    environment: { GEOMETER_NODE_PROCESS_FIXTURE_MODE: "early_exit_stderr" },
    maxStderrBytes: 8,
    terminationGraceMs: 50,
    workingDirectory: fixtureDirectory,
  }),
  /code 23[\s\S]*6789TAIL/,
);

await assert.rejects(
  GeometerNodeProcessA0.spawn(process.execPath, {
    clientName: "node-process-handshake-timeout",
    clientVersion: "a0",
    connectTimeoutMs: 50,
    environment: { GEOMETER_NODE_PROCESS_FIXTURE_MODE: "handshake_timeout" },
    terminationGraceMs: 50,
    workingDirectory: fixtureDirectory,
  }),
  /handshake timed out/,
);

const acknowledgedButRunning = await GeometerNodeProcessA0.spawn(process.execPath, {
  clientName: "node-process-ack-without-exit",
  clientVersion: "a0",
  environment: { GEOMETER_NODE_PROCESS_FIXTURE_MODE: "ack_hang" },
  shutdownTimeoutMs: 50,
  terminationGraceMs: 50,
  workingDirectory: fixtureDirectory,
});
await assert.rejects(
  acknowledgedButRunning.close("test post-ack forced termination"),
  /did not exit after shutdown acknowledgment/,
);

class KillErrorChild extends EventEmitter {
  exitCode = null;
  signalCode = null;
  signals = [];

  kill(signal) {
    this.signals.push(signal);
    if (signal === "SIGTERM") {
      queueMicrotask(() => this.emit("error", new Error("signal delivery failed")));
    } else {
      this.signalCode = "SIGKILL";
      queueMicrotask(() => this.emit("exit", null, "SIGKILL"));
    }
    return false;
  }
}

const killErrorChild = new KillErrorChild();
const killErrorObservation = new ChildProcessObservationA0(killErrorChild);
const killErrorTermination = new ChildTerminationA0(killErrorChild, killErrorObservation.exit, 10);
killErrorTermination.request();
assert.match((await killErrorObservation.firstError).message, /signal delivery failed/);
let exitedAfterError = false;
void killErrorObservation.exit.then(() => {
  exitedAfterError = true;
});
await new Promise((resolve) => setImmediate(resolve));
assert.equal(exitedAfterError, false);
assert.equal((await killErrorTermination.terminateAndWait()).signal, "SIGKILL");
assert.deepEqual(killErrorChild.signals, ["SIGTERM", "SIGKILL"]);

process.stdout.write(
  JSON.stringify({
    modelBounds: response.outcome.result.bounds.size,
    processSupervision: true,
  }),
);
