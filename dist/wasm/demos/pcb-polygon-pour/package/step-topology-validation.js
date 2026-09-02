export class StepTopologySemanticError extends Error {
    code;
    constructor(code, message) {
        super(message);
        this.name = "StepTopologySemanticError";
        this.code = code;
    }
}
export function validateStepTopologySession(session, expected) {
    if (session.session_handle !== expected.sessionHandle ||
        session.generation !== expected.generation) {
        fail("geometer.step_topology.stale_session", "Session handle or generation is stale.");
    }
}
export function validateStepTopologyResolveHitContext(request, expected) {
    validateStepTopologySession(request.session, expected);
}
export function validateStepTopologyLogicalGroupCommands(request) {
    const created = new Set();
    let memberReferenceCount = 0;
    for (const command of request.commands) {
        validateResearchName(command.authored_id, "wn.geometer.research.group.", "logical-group authored id");
        if (command.kind === "create") {
            if (created.has(command.authored_id)) {
                fail("geometer.step_topology.duplicate_group_create", `Logical group ${command.authored_id} is created more than once.`);
            }
            created.add(command.authored_id);
        }
        else if (command.kind === "erase") {
            created.delete(command.authored_id);
        }
        if (command.kind === "create" || command.kind === "replace_members") {
            memberReferenceCount += command.member_handles.length;
            if (memberReferenceCount > 100_000) {
                fail("geometer.step_topology.group_member_limit", "Logical-group transaction member references exceed the aggregate limit.");
            }
            validateUniqueHandles(command.member_handles, "logical-group member");
        }
    }
}
export function validateStepTopologyLogicalGroupResult(result) {
    let memberCount = 0;
    for (const group of result.groups) {
        validateResearchName(group.authored_id, "wn.geometer.research.group.", "logical-group authored id");
        memberCount += group.members.length;
        if (memberCount > 100_000) {
            fail("geometer.step_topology.group_member_limit", "Published logical-group members exceed the aggregate limit.");
        }
        validateUniqueHandles(group.members.map((member) => member.target_handle), "logical-group member");
    }
}
export function validateStepTopologyMetadataProbeCommands(request) {
    const attached = new Set();
    for (const command of request.commands) {
        validateResearchName(command.authored_id, "wn.geometer.research.probe.", "metadata-probe authored id");
        if (command.kind === "attach") {
            if (attached.has(command.authored_id)) {
                fail("geometer.step_topology.duplicate_probe_attach", `Metadata probe ${command.authored_id} is attached more than once.`);
            }
            attached.add(command.authored_id);
        }
        else if (command.kind === "erase") {
            attached.delete(command.authored_id);
        }
        if (command.kind !== "erase") {
            validateResearchName(command.key, "wn.geometer.research.probe.key.", "metadata-probe key");
            if (command.target.kind === "logical_group") {
                validateResearchName(command.target.group_authored_id, "wn.geometer.research.group.", "metadata-probe logical-group target");
            }
        }
    }
}
export function validateStepTopologyHierarchyCommands(request) {
    const createdNodes = new Map();
    const createdOccurrences = new Set();
    for (const command of request.commands) {
        if (command.kind === "create_product" || command.kind === "create_assembly") {
            const nodeKind = command.kind === "create_product" ? "product" : "assembly";
            validateHierarchyNodeId(command.authored_id, nodeKind);
            if (createdNodes.has(command.authored_id)) {
                fail("geometer.step_topology.duplicate_hierarchy_node", `Hierarchy node ${command.authored_id} is created more than once.`);
            }
            createdNodes.set(command.authored_id, nodeKind);
        }
        else if (command.kind === "create_occurrence") {
            validateOccurrenceId(command.authored_id);
            if (createdOccurrences.has(command.authored_id)) {
                fail("geometer.step_topology.duplicate_hierarchy_occurrence", `Hierarchy occurrence ${command.authored_id} is created more than once.`);
            }
            createdOccurrences.add(command.authored_id);
            validateHierarchyNodeReference(command.child_authored_id, "hierarchy child id");
            validateLowercaseResearchName(command.parent_assembly_authored_id, "wn.geometer.research.assembly.", "hierarchy parent id");
        }
        else if (command.kind === "reparent_occurrence") {
            validateOccurrenceId(command.authored_id);
            validateLowercaseResearchName(command.parent_assembly_authored_id, "wn.geometer.research.assembly.", "hierarchy parent id");
        }
        else if (command.kind === "erase_occurrence") {
            validateOccurrenceId(command.authored_id);
            createdOccurrences.delete(command.authored_id);
        }
        else {
            validateHierarchyNodeReference(command.authored_id, "hierarchy node id");
            if (command.kind === "erase_node")
                createdNodes.delete(command.authored_id);
        }
    }
}
export function validateStepTopologyHierarchyResult(result) {
    const nodes = new Map(result.hierarchy.nodes.map((node) => [node.authored_id, node]));
    if (nodes.size !== result.hierarchy.nodes.length) {
        fail("geometer.step_topology.duplicate_hierarchy_node", "Hierarchy node ids are not unique.");
    }
    const occurrenceIds = new Set();
    const childrenByParent = new Map();
    for (const node of result.hierarchy.nodes) {
        validateHierarchyNodeId(node.authored_id, node.kind);
        const hasSource = node.source_kind !== undefined && node.source_handle !== undefined;
        if ((node.kind === "product") !== hasSource) {
            fail("geometer.step_topology.invalid_hierarchy_source", "Products require one source while assemblies must not bind source geometry.");
        }
    }
    for (const occurrence of result.hierarchy.occurrences) {
        validateOccurrenceId(occurrence.authored_id);
        if (occurrenceIds.has(occurrence.authored_id)) {
            fail("geometer.step_topology.duplicate_hierarchy_occurrence", "Hierarchy occurrence ids are not unique.");
        }
        occurrenceIds.add(occurrence.authored_id);
        if (!nodes.has(occurrence.child_authored_id)) {
            fail("geometer.step_topology.dangling_hierarchy_child", "Hierarchy child is missing.");
        }
        if (nodes.get(occurrence.parent_assembly_authored_id)?.kind !== "assembly") {
            fail("geometer.step_topology.invalid_hierarchy_parent", "Hierarchy parent must be an existing assembly.");
        }
        const children = childrenByParent.get(occurrence.parent_assembly_authored_id) ?? [];
        children.push(occurrence.child_authored_id);
        childrenByParent.set(occurrence.parent_assembly_authored_id, children);
    }
    const assemblies = result.hierarchy.nodes.filter((node) => node.kind === "assembly");
    const indegree = new Map(assemblies.map((node) => [node.authored_id, 0]));
    for (const children of childrenByParent.values()) {
        for (const child of children) {
            if (indegree.has(child))
                indegree.set(child, (indegree.get(child) ?? 0) + 1);
        }
    }
    const ready = [...indegree].filter(([, degree]) => degree === 0).map(([id]) => id);
    let visited = 0;
    for (let index = 0; index < ready.length; index += 1) {
        const parent = ready[index];
        if (parent === undefined)
            continue;
        visited += 1;
        for (const child of childrenByParent.get(parent) ?? []) {
            const degree = indegree.get(child);
            if (degree === undefined)
                continue;
            const next = degree - 1;
            indegree.set(child, next);
            if (next === 0)
                ready.push(child);
        }
    }
    if (visited !== assemblies.length) {
        fail("geometer.step_topology.hierarchy_cycle", "Hierarchy contains an assembly cycle.");
    }
}
export function validateStepTopologyRecoveryRequest(groups) {
    const groupIds = new Set();
    let totalCandidates = 0;
    for (const group of groups) {
        validateResearchName(group.group_authored_id, "wn.geometer.research.group.", "recovery group id");
        if (groupIds.has(group.group_authored_id)) {
            fail("geometer.step_topology.duplicate_recovery_group", "Recovery group ids are not unique.");
        }
        groupIds.add(group.group_authored_id);
        const memberIds = new Set();
        for (const member of group.members) {
            if (memberIds.has(member.member_record_id)) {
                fail("geometer.step_topology.duplicate_recovery_member", "Recovery member ids are not unique within a group.");
            }
            memberIds.add(member.member_record_id);
            const handles = new Set();
            totalCandidates += member.candidates.length;
            if (totalCandidates > 65_536) {
                fail("geometer.step_topology.recovery_candidate_limit", "Recovery candidates exceed the aggregate request limit.");
            }
            for (const candidate of member.candidates) {
                validateTopologyHandle(candidate.target_handle, "recovery candidate");
                if (candidate.kind !== member.kind) {
                    fail("geometer.step_topology.recovery_kind_mismatch", "Recovery candidate kind differs from its source member.");
                }
                if (handles.has(candidate.target_handle)) {
                    fail("geometer.step_topology.duplicate_recovery_candidate", "Recovery candidate handles are not unique within a member.");
                }
                handles.add(candidate.target_handle);
                if (candidate.topology_link_verified && !candidate.authored_target_id) {
                    fail("geometer.step_topology.missing_recovery_authored_id", "Verified topology-link evidence requires an authored target id.");
                }
                if (candidate.carrier_locator_validated && candidate.carrier_locator.length === 0) {
                    fail("geometer.step_topology.missing_recovery_carrier_locator", "Validated carrier evidence requires a carrier locator.");
                }
                const hasVerifiedCarrier = candidate.topology_link_verified || candidate.carrier_locator_validated;
                if (hasVerifiedCarrier && candidate.carrier_record.length === 0) {
                    fail("geometer.step_topology.missing_recovery_carrier_record", "Verified carrier evidence requires a carrier record.");
                }
                if (candidate.lineage !== "none" && !hasVerifiedCarrier) {
                    fail("geometer.step_topology.unverified_recovery_lineage", "Split or merge lineage requires verified carrier evidence.");
                }
                if (candidate.fingerprint !== undefined)
                    validateRecoveryFingerprint(candidate.fingerprint);
            }
            if (member.source_fingerprint !== undefined)
                validateRecoveryFingerprint(member.source_fingerprint);
        }
        validateLowercaseSha256(group.provenance.source_artifact_sha256, "recovery source artifact");
        validateLowercaseSha256(group.provenance.candidate_artifact_sha256, "recovery candidate artifact");
    }
}
export function validateStepTopologyRecoveryResults(groups) {
    const groupIds = new Set();
    for (const group of groups) {
        validateResearchName(group.group_authored_id, "wn.geometer.research.group.", "recovery group id");
        if (groupIds.has(group.group_authored_id)) {
            fail("geometer.step_topology.duplicate_recovery_group", "Recovery group ids are not unique.");
        }
        groupIds.add(group.group_authored_id);
        const counts = {
            resolved: 0,
            ambiguous: 0,
            unresolved: 0,
            unsupported: 0,
        };
        const memberIds = new Set();
        for (const member of group.members) {
            counts[member.resolution_state] += 1;
            if (memberIds.has(member.member_record_id)) {
                fail("geometer.step_topology.duplicate_recovery_member", "Recovery result member ids are not unique.");
            }
            memberIds.add(member.member_record_id);
            if (member.resolved_target_handle !== undefined) {
                validateTopologyHandle(member.resolved_target_handle, "resolved recovery target");
            }
            for (const alternative of member.evidence.rejected_alternatives) {
                validateTopologyHandle(alternative.target_handle, "rejected recovery target");
            }
            const rejectedHandles = member.evidence.rejected_alternatives.map((alternative) => alternative.target_handle);
            if (new Set(rejectedHandles).size !== rejectedHandles.length) {
                fail("geometer.step_topology.duplicate_rejected_recovery_target", "Rejected recovery target handles must be unique.");
            }
            if (member.resolved_target_handle !== undefined &&
                rejectedHandles.includes(member.resolved_target_handle)) {
                fail("geometer.step_topology.contradictory_recovery_target", "A resolved recovery target cannot also be a rejected alternative.");
            }
            const validResolutionFields = member.resolution_state === "resolved"
                ? member.resolved_target_handle !== undefined &&
                    member.resolution_method !== "none" &&
                    (member.resolution_method === "unique_geometry_adjacency_fingerprint"
                        ? member.confidence === "medium"
                        : member.confidence === "high") &&
                    member.evidence.matching_candidate_count === 1 &&
                    member.topology_comparison !== "not_compared"
                : member.resolution_state === "ambiguous"
                    ? member.resolved_target_handle === undefined &&
                        member.resolution_method !== "none" &&
                        member.confidence === "none" &&
                        member.topology_comparison === "not_compared" &&
                        member.evidence.matching_candidate_count > 1
                    : member.resolution_state === "unresolved"
                        ? member.resolved_target_handle === undefined &&
                            member.resolution_method === "none" &&
                            member.confidence === "none" &&
                            member.topology_comparison === "not_compared" &&
                            member.evidence.matching_candidate_count === 0
                        : member.resolved_target_handle === undefined &&
                            member.resolution_method === "none" &&
                            member.confidence === "none" &&
                            member.topology_comparison === "unavailable" &&
                            member.evidence.matching_candidate_count === 0;
            if (!validResolutionFields) {
                fail("geometer.step_topology.inconsistent_recovery_member", "Resolved target, method, and confidence must agree with member resolution state.");
            }
            if (member.evidence.matching_candidate_count > member.evidence.candidate_count) {
                fail("geometer.step_topology.invalid_recovery_evidence_count", "Matching recovery candidates exceed candidate count.");
            }
            if (member.evidence.matching_candidate_count + member.evidence.rejected_alternatives.length !==
                member.evidence.candidate_count) {
                fail("geometer.step_topology.invalid_recovery_evidence_count", "Matching and rejected recovery candidates must account for every candidate.");
            }
        }
        if (counts.resolved !== group.resolved_member_count ||
            counts.ambiguous !== group.ambiguous_member_count ||
            counts.unresolved !== group.unresolved_member_count ||
            counts.unsupported !== group.unsupported_member_count) {
            fail("geometer.step_topology.recovery_count_mismatch", "Recovery group counts do not match member results.");
        }
        const expectedCompleteness = counts.resolved === group.members.length
            ? "fully_recovered"
            : counts.resolved > 0
                ? "partially_recovered"
                : counts.unsupported === group.members.length
                    ? "unsupported"
                    : "unrecovered";
        const expectedState = expectedCompleteness === "fully_recovered"
            ? "resolved"
            : expectedCompleteness === "unsupported"
                ? "unsupported"
                : counts.ambiguous > 0
                    ? "ambiguous"
                    : "unresolved";
        if (group.completeness !== expectedCompleteness || group.resolution_state !== expectedState) {
            fail("geometer.step_topology.recovery_aggregate_mismatch", "Recovery group state or completeness does not match its member outcomes.");
        }
    }
}
export async function validateStepTopologySaveAttachments(request, result, attachments) {
    validateLowercaseSha256(result.source_sha256, "save source artifact");
    if (request.carrier !== result.artifact.carrier) {
        fail("geometer.step_topology.save_carrier_mismatch", "The emitted persistence carrier does not match the requested carrier.");
    }
    validateCarrierCapabilities(result);
    await validateExactAttachments([
        {
            name: result.artifact.name,
            mediaTypes: [result.artifact.media_type],
            bytes: result.artifact.bytes,
            sha256: result.artifact.sha256,
        },
    ], attachments);
}
export async function validateStepTopologyRestoreAttachments(request, attachments) {
    validateLowercaseSha256(request.source.sha256, "restore source artifact");
    const isJournal = request.state_artifact.carrier === "edit_journal";
    if (isJournal !== (request.replay_preconditions !== undefined)) {
        fail("geometer.step_topology.invalid_replay_preconditions", "Edit-journal restore requires replay preconditions and persisted-state restore forbids them.");
    }
    if (request.replay_preconditions !== undefined) {
        validateLowercaseSha256(request.replay_preconditions.source_sha256, "replay source artifact");
        validateLowercaseSha256(request.replay_preconditions.source_brep_sha256, "replay source BREP");
        validateLowercaseSha256(request.replay_preconditions.target_inventory_sha256, "replay target inventory");
        if (request.replay_preconditions.source_sha256 !== request.source.sha256) {
            fail("geometer.step_topology.replay_source_mismatch", "Edit-journal replay source does not match the supplied STEP source.");
        }
    }
    await validateExactAttachments([
        {
            name: "source",
            mediaTypes: ["application/step", "model/step"],
            bytes: request.source.bytes,
            sha256: request.source.sha256,
        },
        {
            name: request.state_artifact.name,
            mediaTypes: [request.state_artifact.media_type],
            bytes: request.state_artifact.bytes,
            sha256: request.state_artifact.sha256,
        },
    ], attachments);
}
export function validateStepTopologyRestoreResult(request, result) {
    if (result.source.format !== request.source.format ||
        result.source.sha256 !== request.source.sha256 ||
        result.source.bytes !== request.source.bytes ||
        result.source.normalized_length_unit !== request.source.normalized_length_unit) {
        fail("geometer.step_topology.restore_source_mismatch", "Restore result source does not match the requested exact source.");
    }
    const expectedTransactions = request.replay_preconditions?.transaction_count ?? 0;
    if (result.replayed_transaction_count !== expectedTransactions) {
        fail("geometer.step_topology.restore_replay_count_mismatch", "Restore result replay count does not match its replay preconditions.");
    }
    validateStepTopologyRecoveryResults(result.recovery);
}
export async function validateStepTopologyCheckpointAttachment(result, attachments) {
    if (result.transaction_count !== result.state.edit_journal_revision) {
        fail("geometer.step_topology.journal_revision_mismatch", "Checkpoint transaction count does not match the session journal revision.");
    }
    if (attachments.length !== 1) {
        fail("geometer.step_topology.journal_attachment_count", "Checkpoint requires exactly one edit-journal attachment.");
    }
    const attachment = attachments[0];
    if (attachment === undefined) {
        fail("geometer.step_topology.journal_attachment_count", "Checkpoint requires exactly one edit-journal attachment.");
    }
    if (attachment.name !== result.journal.name ||
        attachment.mediaType !== result.journal.media_type ||
        attachment.data.byteLength !== result.journal.bytes) {
        fail("geometer.step_topology.attachment_mismatch", "Edit-journal attachment metadata does not match its descriptor.");
    }
    if ((await sha256Hex(attachment.data)) !== result.journal.sha256) {
        fail("geometer.step_topology.attachment_digest_mismatch", "Edit-journal attachment digest does not match its descriptor.");
    }
}
export function validateStepTopologyInspection(result) {
    const accumulator = new StepTopologyInspectionAccumulator();
    if (!accumulator.addPage(result)) {
        fail("geometer.step_topology.incomplete_snapshot", "Paged inspection requires one accumulator for every page through the terminal page.");
    }
}
export class StepTopologyInspectionAccumulator {
    session;
    counts;
    cursors = new Set();
    handles = new Set();
    definitions = new Map();
    occurrences = new Map();
    bodies = new Map();
    shells = new Map();
    faces = new Map();
    memberships = new Map();
    bodyCountsByDefinition = new Map();
    faceCountsByDefinition = new Map();
    rootOccurrenceCount = 0;
    componentOccurrenceCount = 0;
    complete = false;
    addPage(result) {
        if (this.complete) {
            fail("geometer.step_topology.page_after_completion", "Inspection is already complete.");
        }
        if (!this.session) {
            this.session = {
                sessionHandle: result.session.session_handle,
                generation: result.session.generation,
            };
            this.counts = result.counts;
        }
        else {
            validateStepTopologySession(result.session, this.session);
            if (!sameCounts(result.counts, this.counts)) {
                fail("geometer.step_topology.counts_changed", "Inspection counts changed between pages.");
            }
        }
        const pageRecordCount = result.page.definitions.length +
            result.page.occurrences.length +
            result.page.bodies.length +
            result.page.shells.length +
            result.page.faces.length +
            result.page.memberships.length;
        for (const definition of result.page.definitions) {
            this.add(definition.handle);
            validateSourceEvidence(definition.source_entity);
            this.definitions.set(definition.handle, definition);
        }
        for (const occurrence of result.page.occurrences) {
            this.add(occurrence.handle);
            this.occurrences.set(occurrence.handle, occurrence);
            if (occurrence.kind === "root")
                this.rootOccurrenceCount += 1;
            else
                this.componentOccurrenceCount += 1;
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
        for (const membership of result.page.memberships) {
            const key = `${membership.kind}:${membership.owner_handle}:${membership.member_handle}`;
            if (this.memberships.has(key)) {
                fail("geometer.step_topology.duplicate_membership", `Duplicate topology membership ${key}.`);
            }
            this.memberships.set(key, membership);
        }
        this.validateAccumulatedCounts();
        if (result.page.next_cursor !== undefined) {
            if (pageRecordCount === 0) {
                fail("geometer.step_topology.empty_continuation", "A nonterminal inspection page must make target-record progress.");
            }
            if (this.cursors.has(result.page.next_cursor)) {
                fail("geometer.step_topology.repeated_cursor", "Inspection repeated a continuation cursor.");
            }
            this.cursors.add(result.page.next_cursor);
            return false;
        }
        this.validateCompleteSnapshot();
        this.complete = true;
        return true;
    }
    add(handle) {
        if (this.handles.has(handle)) {
            fail("geometer.step_topology.duplicate_target", `Duplicate topology handle ${handle}.`);
        }
        this.handles.add(handle);
    }
    validateCompleteSnapshot() {
        const counts = this.counts;
        if (this.definitions.size !== counts.definitions ||
            this.rootOccurrenceCount !== counts.root_occurrences ||
            this.componentOccurrenceCount !== counts.component_occurrences ||
            this.bodies.size !== counts.bodies ||
            this.shells.size !== counts.shells ||
            this.faces.size !== counts.faces ||
            this.memberships.size !== counts.memberships) {
            fail("geometer.step_topology.incomplete_snapshot", "Terminal page does not satisfy counts.");
        }
        const occurrenceDepths = new Map();
        const visitingOccurrences = new Set();
        for (const occurrence of this.occurrences.values()) {
            this.requireDefinition(occurrence.definition_handle, occurrence.handle);
            this.resolveOccurrenceDepth(occurrence.handle, occurrenceDepths, visitingOccurrences);
        }
        for (const definition of this.definitions.values()) {
            const bodies = this.bodyCountsByDefinition.get(definition.handle) ?? 0;
            const faces = this.faceCountsByDefinition.get(definition.handle) ?? 0;
            if (definition.body_count !== bodies || definition.face_count !== faces) {
                fail("geometer.step_topology.definition_count_mismatch", `Definition ${definition.handle} counts do not match its records.`);
            }
        }
        const bodyShells = membershipIndex(this.memberships.values(), "body_shell");
        const bodyFaces = membershipIndex(this.memberships.values(), "body_face");
        const shellFaces = membershipIndex(this.memberships.values(), "shell_face");
        const shellBodies = reverseMembershipIndex(bodyShells);
        const faceBodies = reverseMembershipIndex(bodyFaces);
        const faceShells = reverseMembershipIndex(shellFaces);
        for (const body of this.bodies.values()) {
            this.requireDefinition(body.definition_handle, body.handle);
            validateBodyMembership(body, this.shells, this.faces, bodyShells, bodyFaces);
        }
        for (const shell of this.shells.values()) {
            this.requireDefinition(shell.definition_handle, shell.handle);
            validateShellMembership(shell, this.bodies, this.faces, shellBodies, shellFaces);
        }
        for (const face of this.faces.values()) {
            this.requireDefinition(face.definition_handle, face.handle);
            validateFaceMembership(face, this.bodies, this.shells, faceBodies, faceShells);
        }
    }
    requireDefinition(definitionHandle, ownerHandle) {
        if (!this.definitions.has(definitionHandle)) {
            fail("geometer.step_topology.dangling_definition", `${ownerHandle} references missing definition ${definitionHandle}.`);
        }
    }
    validateAccumulatedCounts() {
        const counts = this.counts;
        if (this.definitions.size > counts.definitions ||
            this.rootOccurrenceCount > counts.root_occurrences ||
            this.componentOccurrenceCount > counts.component_occurrences ||
            this.bodies.size > counts.bodies ||
            this.shells.size > counts.shells ||
            this.faces.size > counts.faces ||
            this.memberships.size > counts.memberships) {
            fail("geometer.step_topology.count_exceeded", "Inspection records exceed the declared snapshot counts.");
        }
    }
    resolveOccurrenceDepth(handle, depths, visiting) {
        const cached = depths.get(handle);
        if (cached !== undefined)
            return cached;
        const occurrence = this.occurrences.get(handle);
        if (!occurrence) {
            fail("geometer.step_topology.dangling_occurrence_parent", `Missing occurrence ${handle}.`);
        }
        if (visiting.has(handle)) {
            fail("geometer.step_topology.occurrence_cycle", "Occurrence parentage contains a cycle.");
        }
        visiting.add(handle);
        const depth = occurrence.kind === "root"
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
export async function validateStepTopologyRenderAttachments(result, attachments) {
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
        fail("geometer.step_topology.glb_digest_mismatch", "Render artifact and GLB descriptor name different GLB bytes.");
    }
    const seen = new Set();
    for (const attachment of attachments) {
        if (seen.has(attachment.name)) {
            fail("geometer.step_topology.duplicate_attachment", `Duplicate ${attachment.name}.`);
        }
        seen.add(attachment.name);
        const descriptor = expected.find((item) => item.name === attachment.name);
        if (!descriptor) {
            fail("geometer.step_topology.unexpected_attachment", `Unexpected ${attachment.name}.`);
        }
        if (attachment.mediaType !== descriptor.mediaType ||
            attachment.data.byteLength !== descriptor.bytes) {
            fail("geometer.step_topology.attachment_mismatch", `Attachment ${attachment.name} metadata does not match its descriptor.`);
        }
        if ((await sha256Hex(attachment.data)) !== descriptor.sha256) {
            fail("geometer.step_topology.attachment_digest_mismatch", `Attachment ${attachment.name} digest does not match its descriptor.`);
        }
    }
    for (const descriptor of expected) {
        if (descriptor.required && !seen.has(descriptor.name)) {
            fail("geometer.step_topology.missing_attachment", `Missing ${descriptor.name}.`);
        }
    }
}
function validateSourceEvidence(evidence) {
    if (!evidence)
        return;
    const positiveFields = [evidence.model_number, evidence.entity_type, evidence.mapping_method];
    if (evidence.mapped) {
        if (positiveFields.some((value) => value === undefined)) {
            fail("geometer.step_topology.incomplete_source_evidence", "Mapped source evidence is incomplete.");
        }
    }
    else if (evidence.shape_result_round_trip ||
        positiveFields.some((value) => value !== undefined)) {
        fail("geometer.step_topology.invalid_source_evidence", "Unmapped source evidence carries positive mapping fields.");
    }
}
function validateUniqueHandles(handles, label) {
    if (handles.some((handle) => !/^gtt_[0-9a-f]{64}$/u.test(handle))) {
        fail("geometer.step_topology.invalid_target", `A ${label} handle is malformed.`);
    }
    if (new Set(handles).size !== handles.length) {
        fail("geometer.step_topology.duplicate_target", `A ${label} handle is repeated.`);
    }
}
function validateTopologyHandle(value, label) {
    if (!/^gtt_[0-9a-f]{64}$/u.test(value)) {
        fail("geometer.step_topology.invalid_target", `${label} handle is malformed.`);
    }
}
function validateHierarchyNodeId(value, kind) {
    validateLowercaseResearchName(value, kind === "product" ? "wn.geometer.research.product." : "wn.geometer.research.assembly.", `${kind} hierarchy node id`);
}
function validateHierarchyNodeReference(value, label) {
    if (!matchesLowercaseResearchName(value, "wn.geometer.research.product.") &&
        !matchesLowercaseResearchName(value, "wn.geometer.research.assembly.")) {
        fail("geometer.step_topology.invalid_research_namespace", `${label} does not use a product or assembly research namespace.`);
    }
}
function validateOccurrenceId(value) {
    validateLowercaseResearchName(value, "wn.geometer.research.occurrence.", "hierarchy occurrence id");
}
function validateLowercaseResearchName(value, prefix, label) {
    if (!matchesLowercaseResearchName(value, prefix)) {
        fail("geometer.step_topology.invalid_research_namespace", `${label} does not use its required lowercase research namespace.`);
    }
}
function validateLowercaseSha256(value, label) {
    if (!/^[0-9a-f]{64}$/u.test(value)) {
        fail("geometer.step_topology.invalid_sha256", `${label} SHA-256 must be lowercase hex.`);
    }
}
function validateRecoveryFingerprint(fingerprint) {
    const numericValues = [
        fingerprint.area_mm2,
        fingerprint.volume_mm3,
        ...fingerprint.centroid_mm,
        ...fingerprint.bounds_mm,
    ];
    if (numericValues.some((value) => !Number.isFinite(value))) {
        fail("geometer.step_topology.invalid_recovery_fingerprint", "Fingerprint values must be finite.");
    }
    const [minX, minY, minZ, maxX, maxY, maxZ] = fingerprint.bounds_mm;
    if (minX === undefined ||
        minY === undefined ||
        minZ === undefined ||
        maxX === undefined ||
        maxY === undefined ||
        maxZ === undefined) {
        fail("geometer.step_topology.invalid_recovery_bounds", "Fingerprint bounds require six values.");
    }
    if (minX > maxX || minY > maxY || minZ > maxZ) {
        fail("geometer.step_topology.invalid_recovery_bounds", "Fingerprint bounds minima must not exceed maxima.");
    }
    validateLowercaseSha256(fingerprint.adjacency_sha256, "recovery adjacency");
}
function validateCarrierCapabilities(result) {
    const required = new Set(["xbf", "xml_xcaf", "step_ap242", "json_sidecar", "edit_journal"]);
    for (const capability of result.capabilities) {
        if (!required.delete(capability.carrier)) {
            fail("geometer.step_topology.invalid_carrier_capabilities", "Carrier capabilities must contain each carrier exactly once.");
        }
        if (capability.notes.some((note) => note.value.length === 0)) {
            fail("geometer.step_topology.invalid_carrier_capabilities", "Capability notes must be nonempty.");
        }
    }
    if (required.size !== 0) {
        fail("geometer.step_topology.invalid_carrier_capabilities", "Carrier capabilities must contain each carrier exactly once.");
    }
    const selected = result.capabilities.find((capability) => capability.carrier === result.artifact.carrier);
    if (selected?.save === "unsupported") {
        fail("geometer.step_topology.unsupported_save_carrier", "The emitted artifact carrier must support save.");
    }
}
async function validateExactAttachments(expected, attachments) {
    if (attachments.length !== expected.length) {
        fail("geometer.step_topology.attachment_count", "Attachment count does not match descriptors.");
    }
    const seen = new Set();
    for (const attachment of attachments) {
        if (seen.has(attachment.name)) {
            fail("geometer.step_topology.duplicate_attachment", `Duplicate ${attachment.name}.`);
        }
        seen.add(attachment.name);
        const descriptor = expected.find((candidate) => candidate.name === attachment.name);
        if (descriptor === undefined ||
            !descriptor.mediaTypes.includes(attachment.mediaType) ||
            descriptor.bytes !== attachment.data.byteLength) {
            fail("geometer.step_topology.attachment_mismatch", `Attachment ${attachment.name} metadata does not match its descriptor.`);
        }
        validateLowercaseSha256(descriptor.sha256, `${attachment.name} attachment`);
        if ((await sha256Hex(attachment.data)) !== descriptor.sha256) {
            fail("geometer.step_topology.attachment_digest_mismatch", `Attachment ${attachment.name} digest does not match its descriptor.`);
        }
    }
}
function validateResearchName(value, prefix, label) {
    if (!matchesResearchName(value, prefix)) {
        fail("geometer.step_topology.invalid_research_namespace", `${label} does not use its required research namespace.`);
    }
}
function matchesResearchName(value, prefix) {
    const suffix = value.slice(prefix.length);
    return value.startsWith(prefix) && suffix.length > 0 && /^[A-Za-z0-9._-]+$/u.test(suffix);
}
function matchesLowercaseResearchName(value, prefix) {
    const suffix = value.slice(prefix.length);
    return value.startsWith(prefix) && suffix.length > 0 && /^[a-z0-9._-]+$/u.test(suffix);
}
function sameCounts(left, right) {
    return (left.definitions === right.definitions &&
        left.root_occurrences === right.root_occurrences &&
        left.component_occurrences === right.component_occurrences &&
        left.bodies === right.bodies &&
        left.shells === right.shells &&
        left.faces === right.faces &&
        left.memberships === right.memberships);
}
function increment(counts, key) {
    counts.set(key, (counts.get(key) ?? 0) + 1);
}
function membershipIndex(memberships, kind) {
    const mutable = new Map();
    for (const membership of memberships) {
        if (membership.kind !== kind)
            continue;
        const members = mutable.get(membership.owner_handle) ?? new Set();
        members.add(membership.member_handle);
        mutable.set(membership.owner_handle, members);
    }
    return mutable;
}
function reverseMembershipIndex(source) {
    const reversed = new Map();
    for (const [owner, members] of source) {
        for (const member of members) {
            const owners = reversed.get(member) ?? new Set();
            owners.add(owner);
            reversed.set(member, owners);
        }
    }
    return reversed;
}
function validateBodyMembership(body, shells, faces, bodyShells, bodyFaces) {
    const shellHandles = bodyShells.get(body.handle) ?? new Set();
    const faceHandles = bodyFaces.get(body.handle) ?? new Set();
    if (shellHandles.size !== body.shell_count || faceHandles.size !== body.face_count) {
        membershipFailure(body.handle, "declared counts");
    }
    for (const handle of shellHandles) {
        const shell = shells.get(handle);
        if (!shell)
            danglingMembership(body.handle, handle);
        if (shell.definition_handle !== body.definition_handle)
            membershipFailure(body.handle, handle);
    }
    for (const handle of faceHandles) {
        const face = faces.get(handle);
        if (!face)
            danglingMembership(body.handle, handle);
        if (face.definition_handle !== body.definition_handle)
            membershipFailure(body.handle, handle);
    }
}
function validateShellMembership(shell, bodies, faces, shellBodies, shellFaces) {
    const bodyHandles = shellBodies.get(shell.handle) ?? new Set();
    const faceHandles = shellFaces.get(shell.handle) ?? new Set();
    if (bodyHandles.size !== shell.body_count || faceHandles.size !== shell.face_count) {
        membershipFailure(shell.handle, "declared counts");
    }
    for (const handle of bodyHandles) {
        const body = bodies.get(handle);
        if (!body)
            danglingMembership(shell.handle, handle);
        if (body.definition_handle !== shell.definition_handle)
            membershipFailure(shell.handle, handle);
    }
    for (const handle of faceHandles) {
        const face = faces.get(handle);
        if (!face)
            danglingMembership(shell.handle, handle);
        if (face.definition_handle !== shell.definition_handle)
            membershipFailure(shell.handle, handle);
    }
}
function validateFaceMembership(face, bodies, shells, faceBodies, faceShells) {
    const bodyHandles = faceBodies.get(face.handle) ?? new Set();
    const shellHandles = faceShells.get(face.handle) ?? new Set();
    if (bodyHandles.size !== face.body_count || shellHandles.size !== face.shell_count) {
        membershipFailure(face.handle, "declared counts");
    }
    for (const handle of bodyHandles) {
        const body = bodies.get(handle);
        if (!body)
            danglingMembership(face.handle, handle);
        if (body.definition_handle !== face.definition_handle)
            membershipFailure(face.handle, handle);
    }
    for (const handle of shellHandles) {
        const shell = shells.get(handle);
        if (!shell)
            danglingMembership(face.handle, handle);
        if (shell.definition_handle !== face.definition_handle)
            membershipFailure(face.handle, handle);
    }
}
function danglingMembership(owner, missing) {
    fail("geometer.step_topology.dangling_membership", `${owner} references missing topology target ${missing}.`);
}
function membershipFailure(left, right) {
    fail("geometer.step_topology.nonreciprocal_membership", `Topology membership between ${left} and ${right} is not reciprocal.`);
}
async function sha256Hex(data) {
    const copy = new Uint8Array(data);
    const digest = new Uint8Array(await crypto.subtle.digest("SHA-256", copy));
    return [...digest].map((value) => value.toString(16).padStart(2, "0")).join("");
}
function fail(code, message) {
    throw new StepTopologySemanticError(code, message);
}
