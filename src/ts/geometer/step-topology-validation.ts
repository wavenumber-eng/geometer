import type {
  BodySummary,
  DefinitionSummary,
  FaceSummary,
  InspectionCounts,
  OccurrenceSummary,
  SessionReference,
  ShellSummary,
  SourceEntityEvidence,
  StepTopologyInspectResultA0,
  StepTopologyRenderResultA0,
  StepTopologyResolveHitRequestA0,
} from "./generated/contracts.js";

export interface StepTopologySessionExpectation {
  readonly sessionHandle: string;
  readonly generation: number;
}

export interface StepTopologyNamedAttachment {
  readonly name: string;
  readonly mediaType: string;
  readonly data: Uint8Array;
}

export class StepTopologySemanticError extends Error {
  readonly code: string;

  constructor(code: string, message: string) {
    super(message);
    this.name = "StepTopologySemanticError";
    this.code = code;
  }
}

export function validateStepTopologySession(
  session: SessionReference,
  expected: StepTopologySessionExpectation,
): void {
  if (
    session.session_handle !== expected.sessionHandle ||
    session.generation !== expected.generation
  ) {
    fail("geometer.step_topology.stale_session", "Session handle or generation is stale.");
  }
}

export function validateStepTopologyResolveHitContext(
  request: StepTopologyResolveHitRequestA0,
  expected: StepTopologySessionExpectation,
): void {
  validateStepTopologySession(request.session, expected);
}

export function validateStepTopologyInspection(result: StepTopologyInspectResultA0): void {
  const accumulator = new StepTopologyInspectionAccumulator();
  if (!accumulator.addPage(result)) {
    fail(
      "geometer.step_topology.incomplete_snapshot",
      "Paged inspection requires one accumulator for every page through the terminal page.",
    );
  }
}

export class StepTopologyInspectionAccumulator {
  private session: StepTopologySessionExpectation | undefined;
  private counts: InspectionCounts | undefined;
  private readonly cursors = new Set<string>();
  private readonly handles = new Set<string>();
  private readonly definitions = new Map<string, DefinitionSummary>();
  private readonly occurrences = new Map<string, OccurrenceSummary>();
  private readonly bodies = new Map<string, BodySummary>();
  private readonly shells = new Map<string, ShellSummary>();
  private readonly faces = new Map<string, FaceSummary>();
  private readonly bodyCountsByDefinition = new Map<string, number>();
  private readonly faceCountsByDefinition = new Map<string, number>();
  private rootOccurrenceCount = 0;
  private componentOccurrenceCount = 0;
  private complete = false;

  addPage(result: StepTopologyInspectResultA0): boolean {
    if (this.complete) {
      fail("geometer.step_topology.page_after_completion", "Inspection is already complete.");
    }
    if (!this.session) {
      this.session = {
        sessionHandle: result.session.session_handle,
        generation: result.session.generation,
      };
      this.counts = result.counts;
    } else {
      validateStepTopologySession(result.session, this.session);
      if (!sameCounts(result.counts, this.counts as InspectionCounts)) {
        fail("geometer.step_topology.counts_changed", "Inspection counts changed between pages.");
      }
    }

    const pageRecordCount =
      result.page.definitions.length +
      result.page.occurrences.length +
      result.page.bodies.length +
      result.page.shells.length +
      result.page.faces.length;
    for (const definition of result.page.definitions) {
      this.add(definition.handle);
      validateSourceEvidence(definition.source_entity);
      this.definitions.set(definition.handle, definition);
    }
    for (const occurrence of result.page.occurrences) {
      this.add(occurrence.handle);
      this.occurrences.set(occurrence.handle, occurrence);
      if (occurrence.kind === "root") this.rootOccurrenceCount += 1;
      else this.componentOccurrenceCount += 1;
    }
    for (const body of result.page.bodies) {
      this.add(body.handle);
      validateSourceEvidence(body.source_entity);
      this.bodies.set(body.handle, body);
      increment(this.bodyCountsByDefinition, body.definition_handle);
    }
    for (const shell of result.page.shells) {
      this.add(shell.handle);
      validateSourceEvidence(shell.source_entity);
      this.shells.set(shell.handle, shell);
    }
    for (const face of result.page.faces) {
      this.add(face.handle);
      validateSourceEvidence(face.source_entity);
      this.faces.set(face.handle, face);
      increment(this.faceCountsByDefinition, face.definition_handle);
    }
    this.validateAccumulatedCounts();

    if (result.page.next_cursor !== undefined) {
      if (pageRecordCount === 0) {
        fail(
          "geometer.step_topology.empty_continuation",
          "A nonterminal inspection page must make target-record progress.",
        );
      }
      if (this.cursors.has(result.page.next_cursor)) {
        fail(
          "geometer.step_topology.repeated_cursor",
          "Inspection repeated a continuation cursor.",
        );
      }
      this.cursors.add(result.page.next_cursor);
      return false;
    }

    this.validateCompleteSnapshot();
    this.complete = true;
    return true;
  }

  private add(handle: string): void {
    if (this.handles.has(handle)) {
      fail("geometer.step_topology.duplicate_target", `Duplicate topology handle ${handle}.`);
    }
    this.handles.add(handle);
  }

  private validateCompleteSnapshot(): void {
    const counts = this.counts as InspectionCounts;
    if (
      this.definitions.size !== counts.definitions ||
      this.rootOccurrenceCount !== counts.root_occurrences ||
      this.componentOccurrenceCount !== counts.component_occurrences ||
      this.bodies.size !== counts.bodies ||
      this.shells.size !== counts.shells ||
      this.faces.size !== counts.faces
    ) {
      fail("geometer.step_topology.incomplete_snapshot", "Terminal page does not satisfy counts.");
    }

    const occurrenceDepths = new Map<string, number>();
    const visitingOccurrences = new Set<string>();
    for (const occurrence of this.occurrences.values()) {
      this.requireDefinition(occurrence.definition_handle, occurrence.handle);
      this.resolveOccurrenceDepth(occurrence.handle, occurrenceDepths, visitingOccurrences);
    }
    for (const definition of this.definitions.values()) {
      const bodies = this.bodyCountsByDefinition.get(definition.handle) ?? 0;
      const faces = this.faceCountsByDefinition.get(definition.handle) ?? 0;
      if (definition.body_count !== bodies || definition.face_count !== faces) {
        fail(
          "geometer.step_topology.definition_count_mismatch",
          `Definition ${definition.handle} counts do not match its records.`,
        );
      }
    }
    const bodyShells = indexMemberships(this.bodies.values(), (item) => item.shell_handles);
    const bodyFaces = indexMemberships(this.bodies.values(), (item) => item.face_handles);
    const shellBodies = indexMemberships(this.shells.values(), (item) => item.body_handles);
    const shellFaces = indexMemberships(this.shells.values(), (item) => item.face_handles);
    const faceBodies = indexMemberships(this.faces.values(), (item) => item.body_handles);
    const faceShells = indexMemberships(this.faces.values(), (item) => item.shell_handles);
    for (const body of this.bodies.values()) {
      this.requireDefinition(body.definition_handle, body.handle);
      validateBodyMembership(body, this.shells, this.faces, shellBodies, faceBodies);
    }
    for (const shell of this.shells.values()) {
      this.requireDefinition(shell.definition_handle, shell.handle);
      validateShellMembership(shell, this.bodies, this.faces, bodyShells, faceShells);
    }
    for (const face of this.faces.values()) {
      this.requireDefinition(face.definition_handle, face.handle);
      validateFaceMembership(face, this.bodies, this.shells, bodyFaces, shellFaces);
    }
  }

  private requireDefinition(definitionHandle: string, ownerHandle: string): void {
    if (!this.definitions.has(definitionHandle)) {
      fail(
        "geometer.step_topology.dangling_definition",
        `${ownerHandle} references missing definition ${definitionHandle}.`,
      );
    }
  }

  private validateAccumulatedCounts(): void {
    const counts = this.counts as InspectionCounts;
    if (
      this.definitions.size > counts.definitions ||
      this.rootOccurrenceCount > counts.root_occurrences ||
      this.componentOccurrenceCount > counts.component_occurrences ||
      this.bodies.size > counts.bodies ||
      this.shells.size > counts.shells ||
      this.faces.size > counts.faces
    ) {
      fail(
        "geometer.step_topology.count_exceeded",
        "Inspection records exceed the declared snapshot counts.",
      );
    }
  }

  private resolveOccurrenceDepth(
    handle: string,
    depths: Map<string, number>,
    visiting: Set<string>,
  ): number {
    const cached = depths.get(handle);
    if (cached !== undefined) return cached;
    const occurrence = this.occurrences.get(handle);
    if (!occurrence) {
      fail("geometer.step_topology.dangling_occurrence_parent", `Missing occurrence ${handle}.`);
    }
    if (visiting.has(handle)) {
      fail("geometer.step_topology.occurrence_cycle", "Occurrence parentage contains a cycle.");
    }
    visiting.add(handle);
    const depth =
      occurrence.kind === "root"
        ? 0
        : this.resolveOccurrenceDepth(occurrence.parent_occurrence_handle, depths, visiting) + 1;
    visiting.delete(handle);
    if (occurrence.kind === "component" && occurrence.depth !== depth) {
      fail("geometer.step_topology.invalid_occurrence_depth", "Occurrence depth is inconsistent.");
    }
    depths.set(handle, depth);
    return depth;
  }
}

export async function validateStepTopologyRenderAttachments(
  result: StepTopologyRenderResultA0,
  attachments: readonly StepTopologyNamedAttachment[],
): Promise<void> {
  const expected = [
    {
      name: result.glb.name,
      mediaType: result.glb.media_type,
      bytes: result.glb.bytes,
      sha256: result.glb.sha256,
      required: true,
    },
    ...(result.compact_binding_table
      ? [
          {
            name: result.compact_binding_table.name,
            mediaType: result.compact_binding_table.media_type,
            bytes: result.compact_binding_table.bytes,
            sha256: result.compact_binding_table.sha256,
            required: true,
          },
        ]
      : []),
  ];
  if (result.artifact.content_sha256 !== result.glb.sha256) {
    fail(
      "geometer.step_topology.glb_digest_mismatch",
      "Render artifact and GLB descriptor name different GLB bytes.",
    );
  }
  const seen = new Set<string>();
  for (const attachment of attachments) {
    if (seen.has(attachment.name)) {
      fail("geometer.step_topology.duplicate_attachment", `Duplicate ${attachment.name}.`);
    }
    seen.add(attachment.name);
    const descriptor = expected.find((item) => item.name === attachment.name);
    if (!descriptor) {
      fail("geometer.step_topology.unexpected_attachment", `Unexpected ${attachment.name}.`);
    }
    if (
      attachment.mediaType !== descriptor.mediaType ||
      attachment.data.byteLength !== descriptor.bytes
    ) {
      fail(
        "geometer.step_topology.attachment_mismatch",
        `Attachment ${attachment.name} metadata does not match its descriptor.`,
      );
    }
    if ((await sha256Hex(attachment.data)) !== descriptor.sha256) {
      fail(
        "geometer.step_topology.attachment_digest_mismatch",
        `Attachment ${attachment.name} digest does not match its descriptor.`,
      );
    }
  }
  for (const descriptor of expected) {
    if (descriptor.required && !seen.has(descriptor.name)) {
      fail("geometer.step_topology.missing_attachment", `Missing ${descriptor.name}.`);
    }
  }
}

function validateSourceEvidence(evidence: SourceEntityEvidence | undefined): void {
  if (!evidence) return;
  const positiveFields = [evidence.model_number, evidence.entity_type, evidence.mapping_method];
  if (evidence.mapped) {
    if (positiveFields.some((value) => value === undefined)) {
      fail(
        "geometer.step_topology.incomplete_source_evidence",
        "Mapped source evidence is incomplete.",
      );
    }
  } else if (
    evidence.shape_result_round_trip ||
    positiveFields.some((value) => value !== undefined)
  ) {
    fail(
      "geometer.step_topology.invalid_source_evidence",
      "Unmapped source evidence carries positive mapping fields.",
    );
  }
}

function sameCounts(left: InspectionCounts, right: InspectionCounts): boolean {
  return (
    left.definitions === right.definitions &&
    left.root_occurrences === right.root_occurrences &&
    left.component_occurrences === right.component_occurrences &&
    left.bodies === right.bodies &&
    left.shells === right.shells &&
    left.faces === right.faces
  );
}

function increment(counts: Map<string, number>, key: string): void {
  counts.set(key, (counts.get(key) ?? 0) + 1);
}

function indexMemberships<T extends { handle: string }>(
  records: Iterable<T>,
  select: (record: T) => readonly string[],
): ReadonlyMap<string, ReadonlySet<string>> {
  const index = new Map<string, ReadonlySet<string>>();
  for (const record of records) {
    const selected = select(record);
    const members = new Set(selected);
    if (members.size !== selected.length) {
      fail(
        "geometer.step_topology.duplicate_membership",
        `Topology target ${record.handle} repeats a membership handle.`,
      );
    }
    index.set(record.handle, members);
  }
  return index;
}

function validateBodyMembership(
  body: BodySummary,
  shells: ReadonlyMap<string, ShellSummary>,
  faces: ReadonlyMap<string, FaceSummary>,
  shellBodies: ReadonlyMap<string, ReadonlySet<string>>,
  faceBodies: ReadonlyMap<string, ReadonlySet<string>>,
): void {
  for (const handle of body.shell_handles) {
    const shell = shells.get(handle);
    if (!shell) danglingMembership(body.handle, handle);
    if (
      shell.definition_handle !== body.definition_handle ||
      !shellBodies.get(shell.handle)?.has(body.handle)
    ) {
      membershipFailure(body.handle, handle);
    }
  }
  for (const handle of body.face_handles) {
    const face = faces.get(handle);
    if (!face) danglingMembership(body.handle, handle);
    if (
      face.definition_handle !== body.definition_handle ||
      !faceBodies.get(face.handle)?.has(body.handle)
    ) {
      membershipFailure(body.handle, handle);
    }
  }
}

function validateShellMembership(
  shell: ShellSummary,
  bodies: ReadonlyMap<string, BodySummary>,
  faces: ReadonlyMap<string, FaceSummary>,
  bodyShells: ReadonlyMap<string, ReadonlySet<string>>,
  faceShells: ReadonlyMap<string, ReadonlySet<string>>,
): void {
  for (const handle of shell.body_handles) {
    const body = bodies.get(handle);
    if (!body) danglingMembership(shell.handle, handle);
    if (
      body.definition_handle !== shell.definition_handle ||
      !bodyShells.get(body.handle)?.has(shell.handle)
    ) {
      membershipFailure(shell.handle, handle);
    }
  }
  for (const handle of shell.face_handles) {
    const face = faces.get(handle);
    if (!face) danglingMembership(shell.handle, handle);
    if (
      face.definition_handle !== shell.definition_handle ||
      !faceShells.get(face.handle)?.has(shell.handle)
    ) {
      membershipFailure(shell.handle, handle);
    }
  }
}

function validateFaceMembership(
  face: FaceSummary,
  bodies: ReadonlyMap<string, BodySummary>,
  shells: ReadonlyMap<string, ShellSummary>,
  bodyFaces: ReadonlyMap<string, ReadonlySet<string>>,
  shellFaces: ReadonlyMap<string, ReadonlySet<string>>,
): void {
  for (const handle of face.body_handles) {
    const body = bodies.get(handle);
    if (!body) danglingMembership(face.handle, handle);
    if (
      body.definition_handle !== face.definition_handle ||
      !bodyFaces.get(body.handle)?.has(face.handle)
    ) {
      membershipFailure(face.handle, handle);
    }
  }
  for (const handle of face.shell_handles) {
    const shell = shells.get(handle);
    if (!shell) danglingMembership(face.handle, handle);
    if (
      shell.definition_handle !== face.definition_handle ||
      !shellFaces.get(shell.handle)?.has(face.handle)
    ) {
      membershipFailure(face.handle, handle);
    }
  }
}

function danglingMembership(owner: string, missing: string): never {
  fail(
    "geometer.step_topology.dangling_membership",
    `${owner} references missing topology target ${missing}.`,
  );
}

function membershipFailure(left: string, right: string): never {
  fail(
    "geometer.step_topology.nonreciprocal_membership",
    `Topology membership between ${left} and ${right} is not reciprocal.`,
  );
}

async function sha256Hex(data: Uint8Array): Promise<string> {
  const copy = new Uint8Array(data);
  const digest = new Uint8Array(await crypto.subtle.digest("SHA-256", copy));
  return [...digest].map((value) => value.toString(16).padStart(2, "0")).join("");
}

function fail(code: string, message: string): never {
  throw new StepTopologySemanticError(code, message);
}
