import { decodeDirectory, diagnosticCodes, equalBytes, fail, MAX_LOGICAL_SOURCE_REFERENCE_EXPANSIONS, mapTable, outcomeKinds, pathTokens, RESULT_MAGIC, RESULT_TABLES, recordView, required, sha256Hex, sliceRange, sourceRoles, U32_NONE, } from "./analytic-packet-a0-common.js";
import { encodeResultRecords, encodeStandalone, validateResultRecords, } from "./analytic-packet-a0-validation.js";
/** Strictly decode GMABRS01 and project it to the public logical bigint DTO. */
export async function decodeAnalyticPlanarBooleanBatchResultA0Packet(bytes) {
    const tables = decodeDirectory(bytes, RESULT_MAGIC, 101, RESULT_TABLES);
    preflightLogicalSourceReferenceExpansions(tables);
    const records = {
        jobs: mapTable(tables[0], (view) => ({
            jobId: view.getBigUint64(0, true),
            status: view.getUint8(8),
            diagnosticBegin: view.getUint32(16, true),
            diagnosticCount: view.getUint32(20, true),
            regionBegin: view.getUint32(24, true),
            regionCount: view.getUint32(28, true),
            eventBegin: view.getUint32(32, true),
            eventCount: view.getUint32(36, true),
        })),
        diagnostics: mapTable(tables[1], (view) => ({
            code: view.getUint32(0, true),
            severity: view.getUint8(4),
            presence: view.getUint16(6, true),
            jobId: view.getBigUint64(8, true),
            stageId: view.getBigUint64(16, true),
            operandId: view.getBigUint64(24, true),
            geometryId: view.getBigUint64(32, true),
            pathToken: view.getUint32(40, true),
        })),
        vertices: mapTable(tables[2], (view) => ({
            id: view.getBigUint64(0, true),
            x: view.getBigInt64(8, true),
            y: view.getBigInt64(16, true),
            sourceSet: view.getUint32(24, true),
            flags: view.getUint32(28, true),
        })),
        fragments: mapTable(tables[3], (view) => ({
            id: view.getBigUint64(0, true),
            start: view.getUint32(8, true),
            end: view.getUint32(12, true),
            kind: view.getUint8(16),
            direction: view.getUint8(17),
            major: view.getUint8(18) === 1,
            radius: view.getBigUint64(24, true),
            positiveSet: view.getUint32(32, true),
            subtractionSet: view.getUint32(36, true),
        })),
        rings: mapTable(tables[4], (view) => ({
            id: view.getBigUint64(0, true),
            referenceBegin: view.getUint32(8, true),
            referenceCount: view.getUint32(12, true),
            parent: view.getUint32(16, true),
            depth: view.getUint32(20, true),
            flags: view.getUint32(24, true),
        })),
        fragmentReferences: mapTable(tables[5], (view) => view.getUint32(0, true)),
        regions: mapTable(tables[6], (view) => ({
            id: view.getBigUint64(0, true),
            outer: view.getUint32(8, true),
            positiveSet: view.getUint32(12, true),
        })),
        ringRegionReferences: mapTable(tables[7], (view) => view.getBigUint64(0, true)),
        sourceSets: mapTable(tables[8], (view) => ({
            begin: view.getUint32(0, true),
            count: view.getUint32(4, true),
        })),
        sources: mapTable(tables[9], (view) => ({
            kind: view.getUint16(0, true),
            role: view.getUint16(2, true),
            operandId: view.getBigUint64(8, true),
            primaryId: view.getBigUint64(16, true),
            secondaryId: view.getBigUint64(24, true),
        })),
        events: mapTable(tables[10], (view) => ({
            operandId: view.getBigUint64(0, true),
            kind: view.getUint16(8, true),
            referenceBegin: view.getUint32(12, true),
            referenceCount: view.getUint32(16, true),
            sourceSet: view.getUint32(20, true),
        })),
        relationships: mapTable(tables[11], (view) => ({
            queryId: view.getBigUint64(0, true),
            status: view.getUint8(8),
            dimension: view.getUint8(9),
            pairBegin: view.getUint32(12, true),
            pairCount: view.getUint32(16, true),
        })),
        pairs: mapTable(tables[12], (view) => ({
            left: view.getBigUint64(0, true),
            right: view.getBigUint64(8, true),
            dimension: view.getUint8(16),
            equality: view.getUint8(17) === 1,
            leftContains: view.getUint8(18) === 1,
            rightContains: view.getUint8(19) === 1,
        })),
        sourceIndices: mapTable(tables[13], (view) => view.getUint32(0, true)),
    };
    const selections = validateResultRecords(records, tables);
    const canonicalBytes = encodeResultRecords(records);
    if (!equalBytes(canonicalBytes, bytes))
        fail("Result packet is not canonically encoded.");
    const digests = [];
    for (let index = 0; index < records.jobs.length; index += 1) {
        digests.push(await sha256Hex(encodeStandalone(records, index, required(selections[index], "job selection"))));
    }
    const jobResults = records.jobs.map((_, index) => projectJob(records, index, required(selections[index], "job selection"), required(digests[index], "job digest")));
    const relationshipResults = records.relationships.map((relationship) => projectRelationship(records, relationship));
    const header = new DataView(bytes.buffer, bytes.byteOffset, bytes.byteLength);
    if (header.getUint32(36, true) !== jobResults.length ||
        header.getUint32(40, true) !== relationshipResults.length) {
        fail("Result header counts do not match their tables.");
    }
    return { job_results: jobResults, relationship_results: relationshipResults };
}
function preflightLogicalSourceReferenceExpansions(tables) {
    const vertices = required(tables[2], "result vertex table");
    const fragments = required(tables[3], "directed fragment table");
    const regions = required(tables[6], "result region table");
    const sourceSets = required(tables[8], "source-set table");
    const events = required(tables[10], "operand event table");
    let total = 0;
    const charge = (handle) => {
        if (handle === 0)
            return;
        if (handle > sourceSets.count)
            fail("Logical source-set handle is out of range.");
        const count = recordView(sourceSets, handle - 1).getUint32(4, true);
        if (count > MAX_LOGICAL_SOURCE_REFERENCE_EXPANSIONS - total)
            fail("Logical source-reference expansion limit exceeded.");
        total += count;
    };
    for (let index = 0; index < vertices.count; index += 1)
        charge(recordView(vertices, index).getUint32(24, true));
    for (let index = 0; index < fragments.count; index += 1) {
        const record = recordView(fragments, index);
        charge(record.getUint32(32, true));
        charge(record.getUint32(36, true));
    }
    for (let index = 0; index < regions.count; index += 1)
        charge(recordView(regions, index).getUint32(12, true));
    for (let index = 0; index < events.count; index += 1)
        charge(recordView(events, index).getUint32(20, true));
}
function projectJob(records, jobIndex, selected, digest) {
    const job = required(records.jobs[jobIndex], "job result");
    const diagnostics = sliceRange(records.diagnostics, job.diagnosticBegin, job.diagnosticCount, "job diagnostics").map(projectDiagnostic);
    if (job.status === 1)
        return { job_id: job.jobId, status: "failure", diagnostics, digest_sha256: digest };
    const vertices = selected.vertices.map((index) => {
        const value = required(records.vertices[index], "result vertex");
        return {
            vertex_id: value.id,
            point: { x: value.x, y: value.y },
            intersection_sources: projectSourceSet(records, value.sourceSet),
        };
    });
    const directedFragments = selected.fragments.map((index) => {
        const value = required(records.fragments[index], "directed fragment");
        const common = {
            fragment_id: value.id,
            start_vertex_id: required(records.vertices[value.start], "fragment start").id,
            end_vertex_id: required(records.vertices[value.end], "fragment end").id,
            coincident_positive_sources: projectSourceSet(records, value.positiveSet),
            surviving_subtraction_sources: projectSourceSet(records, value.subtractionSet),
        };
        return value.kind === 1
            ? { ...common, kind: "line" }
            : {
                ...common,
                kind: "circular_arc",
                radius_nm: value.radius,
                direction: value.direction === 1 ? "ccw" : "cw",
                major_arc: value.major,
            };
    });
    const rings = selected.rings.map((index) => {
        const value = required(records.rings[index], "result ring");
        const fragmentIds = sliceRange(records.fragmentReferences, value.referenceBegin, value.referenceCount, "ring fragments").map((fragment) => required(records.fragments[fragment], "ring fragment").id);
        return {
            ring_id: value.id,
            fragment_ids: fragmentIds,
            ...(value.parent === U32_NONE
                ? {}
                : { parent_ring_id: required(records.rings[value.parent], "parent ring").id }),
            depth: value.depth,
            hole: (value.flags & 1) !== 0,
        };
    });
    const resultRegions = selected.regions.map((index) => {
        const value = required(records.regions[index], "result region");
        return {
            result_region_id: value.id,
            outer_ring_id: required(records.rings[value.outer], "outer ring").id,
            positive_contributors: projectSourceSet(records, value.positiveSet),
        };
    });
    const operandOutcomes = selected.events.map((index) => projectEvent(records, required(records.events[index], "operand event")));
    return {
        job_id: job.jobId,
        status: "success",
        diagnostics,
        vertices,
        directed_fragments: directedFragments,
        rings,
        result_regions: resultRegions,
        operand_outcomes: operandOutcomes,
        digest_sha256: digest,
    };
}
function projectDiagnostic(value) {
    const code = diagnosticCodes[value.code];
    const path = pathTokens[value.pathToken];
    if (code === undefined ||
        value.severity < 1 ||
        value.severity > 2 ||
        value.pathToken >= pathTokens.length)
        fail("Unknown result diagnostic code, severity, or path token.");
    return {
        code,
        severity: value.severity === 1 ? "error" : "warning",
        job_id: value.jobId,
        ...((value.presence & 2) !== 0 ? { stage_id: value.stageId } : {}),
        ...((value.presence & 4) !== 0 ? { operand_id: value.operandId } : {}),
        ...((value.presence & 8) !== 0 ? { geometry_id: value.geometryId } : {}),
        ...(path === undefined ? {} : { path_identity: path }),
    };
}
function projectSourceSet(records, handle) {
    if (handle === 0)
        return { sources: [] };
    const set = required(records.sourceSets[handle - 1], "source-set handle");
    return {
        sources: sliceRange(records.sourceIndices, set.begin, set.count, "source-set indices").map((index) => {
            const value = required(records.sources[index], "source reference");
            const role = sourceRoles[value.role];
            if (role === undefined || value.kind < 1 || value.kind > 3)
                fail("Unknown source kind or role.");
            const kind = value.kind === 1
                ? "authored_segment_curve"
                : value.kind === 2
                    ? "compact_feature_role"
                    : "subtractive_operand_effect";
            return {
                kind,
                role,
                operand_id: value.operandId,
                primary_id: value.primaryId,
                secondary_id: value.secondaryId,
            };
        }),
    };
}
function projectEvent(records, value) {
    const kind = outcomeKinds[value.kind];
    if (kind === undefined)
        fail("Unknown operand outcome kind.");
    const ringIds = [];
    const regionIds = [];
    for (const reference of sliceRange(records.ringRegionReferences, value.referenceBegin, value.referenceCount, "outcome references")) {
        const referenceKind = Number(reference >> 32n);
        const index = Number(reference & 0xffffffffn);
        if (referenceKind === 1)
            ringIds.push(required(records.rings[index], "outcome ring").id);
        else if (referenceKind === 2)
            regionIds.push(required(records.regions[index], "outcome region").id);
        else
            fail("Unknown ring/region reference kind.");
    }
    return {
        operand_id: value.operandId,
        kind,
        result_ring_ids: ringIds,
        result_region_ids: regionIds,
        sources: projectSourceSet(records, value.sourceSet),
    };
}
function projectRelationship(records, value) {
    if (value.status > 1 || value.dimension > 3)
        fail("Unknown relationship status or dimension.");
    const dimensions = ["disjoint", "point", "curve", "area"];
    return {
        query_id: value.queryId,
        status: value.status === 0 ? "success" : "skipped_dependency_failed",
        aggregate_dimension: required(dimensions[value.dimension], "relationship dimension"),
        pairs: sliceRange(records.pairs, value.pairBegin, value.pairCount, "relationship pairs").map((pair) => ({
            left_result_region_id: pair.left,
            right_result_region_id: pair.right,
            dimension: required(dimensions[pair.dimension], "pair dimension"),
            equality: pair.equality,
            left_contains_right: pair.leftContains,
            right_contains_left: pair.rightContains,
        })),
    };
}
