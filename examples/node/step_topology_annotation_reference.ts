import { readFile } from "node:fs/promises";
import { resolve } from "node:path";

import { BufferAttribute, DoubleSide, Mesh, type Object3D, Raycaster, Vector3 } from "three";
import { type GLTF, GLTFLoader } from "three/examples/jsm/loaders/GLTFLoader.js";

import type {
  OperationResultValueA0,
  StepTopologyApplyLogicalGroupsResultA0,
  StepTopologyApplyMetadataProbesResultA0,
  StepTopologyCheckpointEditJournalResultA0,
  StepTopologyInspectResultA0,
  StepTopologyOpenResultA0,
  StepTopologyRenderResultA0,
  StepTopologyResolveHitResultA0,
  StepTopologyRestoreResultA0,
} from "../../src/ts/geometer/generated/contracts.js";
import type { GeometerIpcOperationResponseA0 } from "../../src/ts/geometer/ipc-client-a0.js";
import { GeometerNodeProcessA0 } from "../../src/ts/geometer/node-process-a0.js";
import {
  validateStepTopologyInspection,
  validateStepTopologyRenderAttachments,
} from "../../src/ts/geometer/step-topology-validation.js";

const groupAuthoredId = "wn.geometer.research.group.example-selected-face";
const probeAuthoredId = "wn.geometer.research.probe.example-selected-face-note";

interface OccurrenceBinding {
  readonly instance_index: number;
  readonly occurrence_handle: string;
}

interface PrimitiveBinding {
  readonly primitive_index: number;
  readonly first_triangle: number;
  readonly triangle_count: number;
  readonly body_handle: string;
  readonly face_handle: string;
}

interface RaycastSelection {
  readonly instance: OccurrenceBinding;
  readonly primitive: PrimitiveBinding;
  readonly primitiveTriangleIndex: number;
}

function expectedResult<T extends OperationResultValueA0>(
  response: GeometerIpcOperationResponseA0,
  schema: T["schema"],
): T {
  if (!response.outcome.ok) {
    const detail = response.outcome.diagnostics
      .map((diagnostic) => `${diagnostic.code}: ${diagnostic.message}`)
      .join("; ");
    throw new Error(`${response.outcome.operation} failed: ${detail}`);
  }
  if (response.outcome.result.schema !== schema) {
    throw new Error(
      `${response.outcome.operation} returned ${response.outcome.result.schema}; expected ${schema}.`,
    );
  }
  return response.outcome.result as T;
}

function expectedAttachment(
  response: GeometerIpcOperationResponseA0,
  name: string,
  mediaType: string,
): Uint8Array {
  const attachment = response.attachments.find((candidate) => candidate.name === name);
  if (attachment === undefined || attachment.mediaType !== mediaType) {
    throw new Error(`Response is missing ${name} (${mediaType}).`);
  }
  return attachment.data;
}

function record(value: unknown, label: string): Readonly<Record<string, unknown>> {
  if (typeof value !== "object" || value === null || Array.isArray(value)) {
    throw new Error(`${label} is not an object.`);
  }
  return value as Readonly<Record<string, unknown>>;
}

function requiredString(value: Readonly<Record<string, unknown>>, key: string): string {
  const field = value[key];
  if (typeof field !== "string" || field.length === 0) {
    throw new Error(`GLB binding field ${key} is missing.`);
  }
  return field;
}

function requiredInteger(value: Readonly<Record<string, unknown>>, key: string): number {
  const field = value[key];
  if (!Number.isSafeInteger(field) || (field as number) < 0) {
    throw new Error(`GLB binding field ${key} is not a nonnegative safe integer.`);
  }
  return field as number;
}

function occurrenceBinding(mesh: Mesh): OccurrenceBinding {
  for (let current: Object3D | null = mesh; current !== null; current = current.parent) {
    const candidate: unknown = current.userData.wn_geometer;
    if (candidate === undefined) continue;
    const binding = record(candidate, "occurrence binding");
    if (binding.occurrence_handle === undefined) continue;
    return {
      instance_index: requiredInteger(binding, "instance_index"),
      occurrence_handle: requiredString(binding, "occurrence_handle"),
    };
  }
  throw new Error("The raycast mesh has no Geometer occurrence binding in its parent chain.");
}

function primitiveBinding(mesh: Mesh): PrimitiveBinding {
  const binding = record(mesh.geometry.userData.wn_geometer, "primitive binding");
  return {
    body_handle: requiredString(binding, "body_handle"),
    face_handle: requiredString(binding, "face_handle"),
    first_triangle: requiredInteger(binding, "first_triangle"),
    primitive_index: requiredInteger(binding, "primitive_index"),
    triangle_count: requiredInteger(binding, "triangle_count"),
  };
}

function firstTriangleInWorld(mesh: Mesh): { readonly center: Vector3; readonly normal: Vector3 } {
  const positions = mesh.geometry.getAttribute("position");
  const indices = mesh.geometry.index;
  if (!(positions instanceof BufferAttribute) || !(indices instanceof BufferAttribute)) {
    throw new Error("The Geometer GLB primitive is not an indexed buffer geometry.");
  }
  if (indices.count < 3) throw new Error("The Geometer GLB primitive has no triangle.");
  const vertices = [0, 1, 2].map((corner) =>
    new Vector3()
      .fromBufferAttribute(positions, indices.getX(corner))
      .applyMatrix4(mesh.matrixWorld),
  );
  const [first, second, third] = vertices;
  if (first === undefined || second === undefined || third === undefined) {
    throw new Error("The first rendered triangle is incomplete.");
  }
  const center = first
    .clone()
    .add(second)
    .add(third)
    .multiplyScalar(1 / 3);
  const normal = second.clone().sub(first).cross(third.clone().sub(first)).normalize();
  if (!Number.isFinite(normal.x) || normal.lengthSq() < 0.99) {
    throw new Error("The first rendered triangle is degenerate.");
  }
  return { center, normal };
}

async function loadGlb(bytes: Uint8Array): Promise<GLTF> {
  const arrayBuffer = bytes.buffer.slice(
    bytes.byteOffset,
    bytes.byteOffset + bytes.byteLength,
  ) as ArrayBuffer;
  return await new Promise((resolveLoad, rejectLoad) => {
    new GLTFLoader().parse(arrayBuffer, "", resolveLoad, rejectLoad);
  });
}

async function raycastFirstPrimitive(glbBytes: Uint8Array): Promise<RaycastSelection> {
  const gltf = await loadGlb(glbBytes);
  gltf.scene.updateMatrixWorld(true);
  let selected: Mesh | undefined;
  gltf.scene.traverse((object) => {
    if (selected === undefined && object instanceof Mesh) selected = object;
  });
  if (selected === undefined) throw new Error("The rendered GLB contains no mesh.");

  const materials = Array.isArray(selected.material) ? selected.material : [selected.material];
  if (materials.length === 0 || materials.some((material) => material.side !== DoubleSide)) {
    throw new Error("The Geometer selection packet must expose double-sided raycast geometry.");
  }
  const primitive = primitiveBinding(selected);
  const { center, normal } = firstTriangleInWorld(selected);
  const raycaster = new Raycaster(
    center.clone().addScaledVector(normal, 0.01),
    normal.clone().negate(),
  );
  const intersection = raycaster.intersectObject(selected, false)[0];
  if (
    intersection?.object !== selected ||
    intersection.faceIndex === undefined ||
    intersection.faceIndex === null ||
    intersection.faceIndex < 0 ||
    intersection.faceIndex >= primitive.triangle_count
  ) {
    throw new Error("Three.js could not resolve a bounded primitive-local triangle hit.");
  }
  return {
    instance: occurrenceBinding(selected),
    primitive,
    primitiveTriangleIndex: intersection.faceIndex,
  };
}

async function main(): Promise<void> {
  const [executableArgument, stepArgument] = process.argv.slice(2);
  if (executableArgument === undefined || stepArgument === undefined || process.argv.length !== 4) {
    throw new Error(
      "Usage: node step-topology-annotation-reference.mjs GEOMETER_EXECUTABLE STEP_FILE",
    );
  }
  const executable = resolve(executableArgument);
  const source = new Uint8Array(await readFile(resolve(stepArgument)));
  let authoringProcess: GeometerNodeProcessA0 | undefined;
  let restoredProcess: GeometerNodeProcessA0 | undefined;
  let authoringClosed = false;
  let restoredClosed = false;

  try {
    authoringProcess = await GeometerNodeProcessA0.spawn(executable, {
      clientName: "step-topology-annotation-reference-authoring",
      clientVersion: "a0",
    });
    const openedResponse = await authoringProcess.client.execute(
      "geometry.step_topology.open.a0",
      { schema: "geometry.step_topology.open.request.a0" },
      [{ data: source, mediaType: "application/step", name: "step" }],
    );
    const opened = expectedResult<StepTopologyOpenResultA0>(
      openedResponse,
      "geometry.step_topology.open.result.a0",
    );

    const inspectedResponse = await authoringProcess.client.execute(
      "geometry.step_topology.inspect.a0",
      {
        include_diagnostics: false,
        include_source_entity_evidence: false,
        page: { limit: 1024 },
        schema: "geometry.step_topology.inspect.request.a0",
        session: opened.session,
      },
    );
    const inspected = expectedResult<StepTopologyInspectResultA0>(
      inspectedResponse,
      "geometry.step_topology.inspect.result.a0",
    );
    validateStepTopologyInspection(inspected);
    if (inspected.page.next_cursor !== undefined) {
      throw new Error("The reference fixture exceeds the one-page student example budget.");
    }

    const renderedResponse = await authoringProcess.client.execute(
      "geometry.step_topology.render.a0",
      {
        schema: "geometry.step_topology.render.request.a0",
        session: opened.session,
        tessellation: {
          angular_deflection_rad: 0.5,
          linear_deflection_mm: 0.1,
          parallel: false,
          relative: false,
          source_to_render: [1, 0, 0, 0, 0, 0, 1, 0, 0, -1, 0, 0],
        },
      },
    );
    const rendered = expectedResult<StepTopologyRenderResultA0>(
      renderedResponse,
      "geometry.step_topology.render.result.a0",
    );
    await validateStepTopologyRenderAttachments(rendered, renderedResponse.attachments);
    const glb = expectedAttachment(renderedResponse, "glb", "model/gltf-binary");
    const selection = await raycastFirstPrimitive(glb);

    const resolvedResponse = await authoringProcess.client.execute(
      "geometry.step_topology.resolve_hit.a0",
      {
        artifact_handle: rendered.artifact.artifact_handle,
        body_handle: selection.primitive.body_handle,
        content_sha256: rendered.artifact.content_sha256,
        face_handle: selection.primitive.face_handle,
        instance_index: selection.instance.instance_index,
        occurrence_handle: selection.instance.occurrence_handle,
        primitive_index: selection.primitive.primitive_index,
        primitive_triangle_index: selection.primitiveTriangleIndex,
        schema: "geometry.step_topology.resolve_hit.request.a0",
        session: opened.session,
      },
    );
    const resolved = expectedResult<StepTopologyResolveHitResultA0>(
      resolvedResponse,
      "geometry.step_topology.resolve_hit.result.a0",
    );

    const groupedResponse = await authoringProcess.client.execute(
      "geometry.step_topology.apply_logical_groups.a0",
      {
        commands: [
          {
            authored_id: groupAuthoredId,
            kind: "create",
            member_handles: [resolved.face_handle],
            name: "Selected face",
          },
        ],
        schema: "geometry.step_topology.apply_logical_groups.request.a0",
        session: resolved.session,
      },
    );
    const grouped = expectedResult<StepTopologyApplyLogicalGroupsResultA0>(
      groupedResponse,
      "geometry.step_topology.apply_logical_groups.result.a0",
    );
    const remintedMemberHandle = grouped.groups[0]?.members[0]?.target_handle;
    if (remintedMemberHandle === undefined || remintedMemberHandle === resolved.face_handle) {
      throw new Error("The mutation did not remint its generation-scoped topology handle.");
    }

    const probedResponse = await authoringProcess.client.execute(
      "geometry.step_topology.apply_metadata_probes.a0",
      {
        commands: [
          {
            authored_id: probeAuthoredId,
            key: "wn.geometer.research.probe.key.example-note",
            kind: "attach",
            target: { group_authored_id: groupAuthoredId, kind: "logical_group" },
            value: "Created from a real Three.js ray hit",
          },
        ],
        schema: "geometry.step_topology.apply_metadata_probes.request.a0",
        session: grouped.state.session,
      },
    );
    const probed = expectedResult<StepTopologyApplyMetadataProbesResultA0>(
      probedResponse,
      "geometry.step_topology.apply_metadata_probes.result.a0",
    );

    const checkpointResponse = await authoringProcess.client.execute(
      "geometry.step_topology.checkpoint_edit_journal.a0",
      {
        schema: "geometry.step_topology.checkpoint_edit_journal.request.a0",
        session: probed.state.session,
      },
    );
    const checkpoint = expectedResult<StepTopologyCheckpointEditJournalResultA0>(
      checkpointResponse,
      "geometry.step_topology.checkpoint_edit_journal.result.a0",
    );
    const journal = expectedAttachment(
      checkpointResponse,
      "edit_journal",
      "application/vnd.wavenumber.geometer.step-topology-edit-journal",
    );

    await authoringProcess.close("restart reference after checkpoint");
    authoringClosed = true;
    restoredProcess = await GeometerNodeProcessA0.spawn(executable, {
      clientName: "step-topology-annotation-reference-restore",
      clientVersion: "a0",
    });
    const restoredResponse = await restoredProcess.client.execute(
      "geometry.step_topology.restore.a0",
      {
        include_diagnostics: false,
        replay_preconditions: {
          occt_version: checkpoint.occt_version,
          source_brep_sha256: checkpoint.source_brep_sha256,
          source_sha256: checkpoint.source_sha256,
          target_inventory_sha256: checkpoint.target_inventory_sha256,
          transaction_count: checkpoint.transaction_count,
        },
        schema: "geometry.step_topology.restore.request.a0",
        source: opened.source,
        state_artifact: {
          bytes: checkpoint.journal.bytes,
          carrier: "edit_journal",
          format: "geometer.step_topology_edit_journal.a0",
          media_type: "application/vnd.wavenumber.geometer.step-topology-edit-journal",
          name: "state_artifact",
          sha256: checkpoint.journal.sha256,
        },
      },
      [
        { data: source, mediaType: "application/step", name: "source" },
        {
          data: journal,
          mediaType: "application/vnd.wavenumber.geometer.step-topology-edit-journal",
          name: "state_artifact",
        },
      ],
    );
    const restored = expectedResult<StepTopologyRestoreResultA0>(
      restoredResponse,
      "geometry.step_topology.restore.result.a0",
    );
    if (restored.session.session_handle === opened.session.session_handle) {
      throw new Error("Restart restore reused a transient session identity.");
    }

    const renamedResponse = await restoredProcess.client.execute(
      "geometry.step_topology.apply_logical_groups.a0",
      {
        commands: [
          {
            authored_id: groupAuthoredId,
            expected_revision: 1,
            kind: "rename",
            name: "Selected face after process restart",
          },
        ],
        schema: "geometry.step_topology.apply_logical_groups.request.a0",
        session: restored.session,
      },
    );
    const renamed = expectedResult<StepTopologyApplyLogicalGroupsResultA0>(
      renamedResponse,
      "geometry.step_topology.apply_logical_groups.result.a0",
    );

    const replacedProbeResponse = await restoredProcess.client.execute(
      "geometry.step_topology.apply_metadata_probes.a0",
      {
        commands: [
          {
            authored_id: probeAuthoredId,
            expected_revision: 1,
            key: "wn.geometer.research.probe.key.example-note",
            kind: "replace",
            target: { group_authored_id: groupAuthoredId, kind: "logical_group" },
            value: "Edit journal replayed after native process restart",
          },
        ],
        schema: "geometry.step_topology.apply_metadata_probes.request.a0",
        session: renamed.state.session,
      },
    );
    const replacedProbe = expectedResult<StepTopologyApplyMetadataProbesResultA0>(
      replacedProbeResponse,
      "geometry.step_topology.apply_metadata_probes.result.a0",
    );

    await restoredProcess.close("restart reference complete");
    restoredClosed = true;
    const report = {
      checkpoint_sha256: checkpoint.journal.sha256,
      exact_preconditions_replayed: restored.replayed_transaction_count === 2,
      glb_sha256: rendered.glb.sha256,
      group_authored_id: renamed.groups[0]?.authored_id,
      inspected_face_count: inspected.counts.faces,
      logical_group_replayed: renamed.groups[0]?.revision === 2,
      metadata_probe_replayed: replacedProbe.probes[0]?.revision === 2,
      probe_authored_id: replacedProbe.probes[0]?.authored_id,
      schema: "wn.geometer.step_topology.annotation_reference_report.a0",
      session_identity_reminted: restored.session.session_handle !== opened.session.session_handle,
      source_sha256: opened.source.sha256,
      topology_handle_reminted: remintedMemberHandle !== resolved.face_handle,
      transaction_count: checkpoint.transaction_count,
    } as const;
    process.stdout.write(`${JSON.stringify(report, null, 2)}\n`);
  } finally {
    if (restoredProcess !== undefined && !restoredClosed) await restoredProcess.terminate();
    if (authoringProcess !== undefined && !authoringClosed) await authoringProcess.terminate();
  }
}

await main();
