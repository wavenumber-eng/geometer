const REQUEST_MAGIC = "GMABRQ01";
const RESULT_MAGIC = "GMABRS01";
const HEADER_BYTES = 64;
const DIRECTORY_ENTRY_BYTES = 32;
const MAX_PACKET_BYTES = 256 * 1024 * 1024;
const U32_NONE = 0xffff_ffff;
const U64_MAX = (1n << 64n) - 1n;
const I64_MIN = -(1n << 63n);
const I64_MAX = (1n << 63n) - 1n;
const MAX_LENGTH_NM = 1000000000000n;
const MAX_JOBS = 65_535;
const MAX_STAGES = 1_048_576;
const MAX_OPERANDS = 4_194_304;
const MAX_QUERIES = 1_048_576;
const MAX_RING_SEGMENTS = 131_072;
const REQUEST_TABLES = [24, 32, 24, 32, 4, 32, 24, 40, 32, 40, 48, 32, 24];
const RESULT_TABLES = [48, 56, 32, 48, 32, 4, 24, 8, 8, 32, 48, 32, 32, 4];
const diagnosticCodes = {
    65539: "geometer.operation.analytic_planar_boolean.invalid_topology",
    65540: "geometer.operation.analytic_planar_boolean.invalid_arc",
    65541: "geometer.operation.analytic_planar_boolean.unsupported_geometry",
    65543: "geometer.operation.analytic_planar_boolean.normalization_error_exceeded",
    65544: "geometer.operation.analytic_planar_boolean.normalization_topology_collapse",
    65545: "geometer.operation.analytic_planar_boolean.nonanalytic_result",
    65546: "geometer.operation.analytic_planar_boolean.solver_failed",
    65547: "geometer.operation.analytic_planar_boolean.resource_limit_exceeded",
};
const pathTokens = [
    undefined,
    "request_jobs",
    "job_id",
    "job_stages",
    "stage_id",
    "stage_operation",
    "stage_operands",
    "operand_id",
    "operand_geometry",
    "region_outer",
    "region_holes",
    "ring_vertices",
    "ring_segments",
    "path_vertices",
    "path_segments",
    "segment_curve",
    "disk_radius",
    "annulus_inner_radius",
    "annulus_outer_radius",
    "capsule_start",
    "capsule_end",
    "capsule_width",
    "swept_path_centerline",
    "swept_path_width",
    "relationship_queries",
    "relationship_left_job_id",
    "relationship_right_job_id",
];
const sourceRoles = {
    0: "none",
    1: "authored_line",
    2: "authored_circular_arc",
    16: "primitive_outer_circle",
    17: "primitive_inner_circle",
    32: "capsule_left_line",
    33: "capsule_end_cap",
    34: "capsule_right_line",
    35: "capsule_start_cap",
    48: "swept_left_offset_line",
    49: "swept_left_offset_arc",
    50: "swept_right_offset_line",
    51: "swept_right_offset_arc",
    52: "swept_round_join",
    53: "swept_start_cap",
    54: "swept_end_cap",
};
const outcomeKinds = {
    1: "contributes_final_material",
    2: "redundant_or_absorbed_coverage",
    3: "partially_removed_later",
    4: "completely_removed_later",
    5: "subtraction_effect_survives",
    6: "subtraction_effect_overwritten_later",
    7: "no_effect",
};
export class AnalyticPacketError extends Error {
    constructor(message) {
        super(message);
        this.name = "AnalyticPacketError";
    }
}
/** Encode the frozen canonical GMABRQ01 projection using bigint IDs and nanometer coordinates. */
export function encodeAnalyticPlanarBooleanBatchRequestA0Packet(request) {
    if (!Array.isArray(request.jobs) || !Array.isArray(request.relationship_queries))
        fail("Analytic request jobs and relationship queries must be arrays.");
    if (request.jobs.length > MAX_JOBS || request.relationship_queries.length > MAX_QUERIES)
        fail("Analytic request exceeds its job or relationship-query limit.");
    const tables = REQUEST_TABLES.map((recordBytes, index) => ({
        kind: index + 1,
        recordBytes,
        records: [],
    }));
    const table = (kind) => required(tables[kind - 1], `request table ${kind}`);
    const ids = new Map();
    const uniqueId = (space, value) => {
        unsigned(value, `${space} id`, true);
        const values = ids.get(space) ?? new Set();
        if (values.has(value))
            fail(`Duplicate ${space} id ${value}.`);
        values.add(value);
        ids.set(space, values);
    };
    const addRing = (ring, open) => {
        const isPath = "path_id" in ring;
        if (isPath !== open)
            fail("Ring/path ownership does not match its geometry kind.");
        const identity = isPath ? ring.path_id : ring.ring_id;
        if (!Array.isArray(ring.vertices) || !Array.isArray(ring.segments))
            fail("Ring/path vertices and segments must be arrays.");
        if (ring.segments.length > MAX_RING_SEGMENTS ||
            (open
                ? ring.segments.length < 1 || ring.vertices.length > MAX_RING_SEGMENTS + 1
                : ring.segments.length < 2))
            fail("Ring/path record count is outside its governed bounds.");
        uniqueId(open ? "path" : "ring", identity);
        const rings = table(6);
        const ringIndex = rings.records.length;
        rings.records.push(new Uint8Array(rings.recordBytes));
        const vertexBegin = table(7).records.length;
        for (const vertex of ring.vertices) {
            uniqueId("vertex", vertex.vertex_id);
            table(7).records.push(record(24, (view) => {
                putU64(view, 0, vertex.vertex_id, "vertex id", true);
                putI64(view, 8, vertex.point.x, "vertex x");
                putI64(view, 16, vertex.point.y, "vertex y");
            }));
        }
        const segmentBegin = table(8).records.length;
        for (const segment of ring.segments) {
            uniqueId("segment", segment.segment_id);
            unsigned(segment.curve_id, "curve id", true);
            table(8).records.push(record(40, (view) => {
                putU64(view, 0, segment.segment_id, "segment id", true);
                putU64(view, 8, segment.curve_id, "curve id", true);
                if (segment.kind === "line") {
                    view.setUint8(16, 1);
                }
                else if (segment.kind === "circular_arc") {
                    view.setUint8(16, 2);
                    view.setUint8(17, segment.direction === "ccw" ? 1 : 2);
                    view.setUint8(18, segment.major_arc ? 1 : 0);
                    putI64(view, 24, segment.center.x, "arc center x");
                    putI64(view, 32, segment.center.y, "arc center y");
                }
                else {
                    fail("Unknown authored segment kind.");
                }
            }));
        }
        if (open
            ? ring.vertices.length !== ring.segments.length + 1
            : ring.vertices.length !== ring.segments.length) {
            fail(open
                ? "A path must have one more vertex than segment."
                : "A ring must have equal vertex and segment counts.");
        }
        rings.records[ringIndex] = record(32, (view) => {
            putU64(view, 0, identity, open ? "path id" : "ring id", true);
            putU32(view, 8, vertexBegin, "vertex begin");
            putU32(view, 12, ring.vertices.length, "vertex count");
            putU32(view, 16, segmentBegin, "segment begin");
            putU32(view, 20, ring.segments.length, "segment count");
            view.setUint32(24, open ? 1 : 0, true);
        });
        return ringIndex;
    };
    const addOperand = (operand) => {
        uniqueId("operand", operand.operand_id);
        let kind = 0;
        let geometryIndex = 0;
        if (operand.kind === "planar_region") {
            kind = 1;
            uniqueId("region", operand.region_id);
            geometryIndex = table(4).records.length;
            if (!Array.isArray(operand.holes) || operand.holes.length > MAX_RING_SEGMENTS - 1)
                fail("Planar-region hole count exceeds its governed bound.");
            table(4).records.push(new Uint8Array(32));
            const outer = addRing(operand.outer, false);
            const holeBegin = table(5).records.length;
            for (const hole of operand.holes) {
                const ringIndex = addRing(hole, false);
                table(5).records.push(record(4, (view) => view.setUint32(0, ringIndex, true)));
            }
            table(4).records[geometryIndex] = record(32, (view) => {
                putU64(view, 0, operand.region_id, "region id", true);
                putU32(view, 8, outer, "outer ring");
                putU32(view, 12, holeBegin, "hole begin");
                putU32(view, 16, operand.holes.length, "hole count");
            });
        }
        else if (operand.kind === "disk") {
            kind = 2;
            uniqueId("feature", operand.feature_id);
            geometryIndex = table(9).records.length;
            length(operand.radius_nm, "disk radius");
            table(9).records.push(record(32, (view) => {
                putU64(view, 0, operand.feature_id, "feature id", true);
                putI64(view, 8, operand.center.x, "disk center x");
                putI64(view, 16, operand.center.y, "disk center y");
                putU64(view, 24, operand.radius_nm, "disk radius", true);
            }));
        }
        else if (operand.kind === "annulus") {
            kind = 3;
            uniqueId("feature", operand.feature_id);
            geometryIndex = table(10).records.length;
            length(operand.inner_radius_nm, "annulus inner radius");
            length(operand.outer_radius_nm, "annulus outer radius");
            if (operand.inner_radius_nm >= operand.outer_radius_nm)
                fail("Annulus inner radius must be smaller than its outer radius.");
            table(10).records.push(record(40, (view) => {
                putU64(view, 0, operand.feature_id, "feature id", true);
                putI64(view, 8, operand.center.x, "annulus center x");
                putI64(view, 16, operand.center.y, "annulus center y");
                putU64(view, 24, operand.inner_radius_nm, "annulus inner radius", true);
                putU64(view, 32, operand.outer_radius_nm, "annulus outer radius", true);
            }));
        }
        else if (operand.kind === "capsule") {
            kind = 4;
            uniqueId("feature", operand.feature_id);
            geometryIndex = table(11).records.length;
            length(operand.width_nm, "capsule width");
            table(11).records.push(record(48, (view) => {
                putU64(view, 0, operand.feature_id, "feature id", true);
                putI64(view, 8, operand.start.x, "capsule start x");
                putI64(view, 16, operand.start.y, "capsule start y");
                putI64(view, 24, operand.end.x, "capsule end x");
                putI64(view, 32, operand.end.y, "capsule end y");
                putU64(view, 40, operand.width_nm, "capsule width", true);
            }));
        }
        else if (operand.kind === "swept_path") {
            kind = 5;
            uniqueId("feature", operand.feature_id);
            geometryIndex = table(12).records.length;
            if (operand.cap !== "round" || operand.join !== "round")
                fail("Swept-path cap and join must both be round in A0.");
            length(operand.width_nm, "swept width");
            const pathRing = addRing(operand.centerline, true);
            table(12).records.push(record(32, (view) => {
                putU64(view, 0, operand.feature_id, "feature id", true);
                putU32(view, 8, pathRing, "path ring");
                putU64(view, 16, operand.width_nm, "swept width", true);
            }));
        }
        else
            fail("Unknown analytic operand geometry kind.");
        table(3).records.push(record(24, (view) => {
            putU64(view, 0, operand.operand_id, "operand id", true);
            view.setUint16(8, kind, true);
            putU32(view, 12, geometryIndex, "geometry index");
        }));
    };
    const jobs = [...request.jobs].sort((left, right) => compareBigint(left.job_id, right.job_id));
    for (const job of jobs) {
        uniqueId("job", job.job_id);
        const stageBegin = table(2).records.length;
        if (!Array.isArray(job.stages))
            fail("Job stages must be an array.");
        if (table(2).records.length + job.stages.length > MAX_STAGES)
            fail("Analytic request exceeds its stage limit.");
        for (const stage of job.stages) {
            uniqueId("stage", stage.stage_id);
            const operandBegin = table(3).records.length;
            if (stage.operation !== "union" && stage.operation !== "difference")
                fail("Unknown analytic stage operation.");
            if (!Array.isArray(stage.operands))
                fail("Stage operands must be an array.");
            if (table(3).records.length + stage.operands.length > MAX_OPERANDS)
                fail("Analytic request exceeds its operand limit.");
            const operands = [...stage.operands].sort((left, right) => compareBigint(left.operand_id, right.operand_id));
            for (const operand of operands)
                addOperand(operand);
            table(2).records.push(record(32, (view) => {
                putU64(view, 0, stage.stage_id, "stage id", true);
                view.setUint8(8, stage.operation === "union" ? 1 : 2);
                putU32(view, 16, operandBegin, "operand begin");
                putU32(view, 20, operands.length, "operand count");
            }));
        }
        table(1).records.push(record(24, (view) => {
            putU64(view, 0, job.job_id, "job id", true);
            putU32(view, 8, stageBegin, "stage begin");
            putU32(view, 12, job.stages.length, "stage count");
        }));
    }
    const jobIds = ids.get("job") ?? new Set();
    for (const query of [...request.relationship_queries].sort((left, right) => compareBigint(left.query_id, right.query_id))) {
        uniqueId("query", query.query_id);
        if (!jobIds.has(query.left_job_id) || !jobIds.has(query.right_job_id))
            fail("Relationship query references an unknown job.");
        table(13).records.push(record(24, (view) => {
            putU64(view, 0, query.query_id, "query id", true);
            putU64(view, 8, query.left_job_id, "left job id", true);
            putU64(view, 16, query.right_job_id, "right job id", true);
        }));
    }
    return encodeTables(REQUEST_MAGIC, tables, table(1).records.length, table(13).records.length);
}
/** Strictly decode GMABRS01 and project it to the public logical bigint DTO. */
export async function decodeAnalyticPlanarBooleanBatchResultA0Packet(bytes) {
    const tables = decodeDirectory(bytes, RESULT_MAGIC, 101, RESULT_TABLES);
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
function validateResultRecords(records, tables) {
    if (records.jobs.length > MAX_JOBS || records.relationships.length > MAX_QUERIES)
        fail("Analytic result exceeds its job or relationship-result limit.");
    const reserved = (kind, index, ranges) => {
        const view = recordView(required(tables[kind - 101], `result table ${kind}`), index);
        for (const [offset, length] of ranges)
            for (let at = 0; at < length; at += 1)
                if (view.getUint8(offset + at) !== 0)
                    fail(`Nonzero reserved byte in result table ${kind}.`);
    };
    records.jobs.forEach((job, index) => {
        reserved(101, index, [
            [9, 7],
            [40, 8],
        ]);
        if (job.jobId === 0n || job.status > 1)
            fail("Invalid job-result identity or status.");
        sliceRange(records.diagnostics, job.diagnosticBegin, job.diagnosticCount, "job diagnostics");
        sliceRange(records.regions, job.regionBegin, job.regionCount, "job regions");
        sliceRange(records.events, job.eventBegin, job.eventCount, "job events");
        if (job.status === 1 && (job.regionCount !== 0 || job.diagnosticCount === 0))
            fail("Failed job owns invalid result ranges.");
        const ownedDiagnostics = sliceRange(records.diagnostics, job.diagnosticBegin, job.diagnosticCount, "job diagnostics");
        if (ownedDiagnostics.some((value) => value.jobId !== job.jobId))
            fail("Job diagnostic identity does not match its owner.");
        const hasError = ownedDiagnostics.some((value) => value.severity === 1);
        if ((job.status === 1) !== hasError)
            fail("Job status does not match its diagnostic severity closure.");
    });
    strictlyIncreasing(records.jobs.map((value) => value.jobId), "job-result ids");
    partition(records.jobs.map((value) => [value.diagnosticBegin, value.diagnosticCount]), records.diagnostics.length, "job diagnostic");
    partition(records.jobs.map((value) => [value.regionBegin, value.regionCount]), records.regions.length, "job region");
    partition(records.jobs.map((value) => [value.eventBegin, value.eventCount]), records.events.length, "job event");
    let previousDiagnostic;
    records.diagnostics.forEach((value, index) => {
        reserved(102, index, [[44, 12]]);
        const presenceMatches = ((value.presence & 1) !== 0) === (value.jobId !== 0n) &&
            ((value.presence & 2) !== 0) === (value.stageId !== 0n) &&
            ((value.presence & 4) !== 0) === (value.operandId !== 0n) &&
            ((value.presence & 8) !== 0) === (value.geometryId !== 0n);
        if (recordView(required(tables[1], "diagnostic table"), index).getUint8(5) !== 1 ||
            diagnosticCodes[value.code] === undefined ||
            value.severity < 1 ||
            value.severity > 2 ||
            value.presence > 15 ||
            (value.presence & 1) === 0 ||
            !presenceMatches ||
            value.pathToken > 26)
            fail("Invalid diagnostic presence flags.");
        const key = [
            value.jobId,
            value.severity,
            value.code,
            value.presence,
            value.stageId,
            value.operandId,
            value.geometryId,
            value.pathToken,
        ];
        if (previousDiagnostic !== undefined && compareComparable(previousDiagnostic, key) >= 0)
            fail("Diagnostic records are not strictly canonical.");
        previousDiagnostic = key;
    });
    records.vertices.forEach((value) => {
        if (value.id === 0n ||
            value.sourceSet > records.sourceSets.length ||
            value.flags > 1 ||
            value.flags !== (value.sourceSet === 0 ? 0 : 1))
            fail("Invalid result vertex.");
    });
    oneBased(records.vertices.map((value) => value.id), "result vertex");
    records.fragments.forEach((value, index) => {
        reserved(104, index, [
            [19, 5],
            [40, 8],
        ]);
        if (value.id === 0n ||
            value.start >= records.vertices.length ||
            value.end >= records.vertices.length ||
            value.start === value.end ||
            recordView(required(tables[3], "fragment table"), index).getUint8(18) > 1 ||
            value.kind < 1 ||
            value.kind > 2 ||
            value.direction > 2 ||
            (value.kind === 1 && (value.direction !== 0 || value.major || value.radius !== 0n)) ||
            (value.kind === 2 &&
                (value.direction < 1 || value.radius === 0n || value.radius > MAX_LENGTH_NM)) ||
            value.positiveSet > records.sourceSets.length ||
            value.subtractionSet > records.sourceSets.length)
            fail("Invalid directed fragment.");
        if (value.kind === 2) {
            const start = required(records.vertices[value.start], "arc start");
            const end = required(records.vertices[value.end], "arc end");
            const dx = end.x - start.x;
            const dy = end.y - start.y;
            const chordSquared = dx * dx + dy * dy;
            const diameterSquared = 4n * value.radius * value.radius;
            if (chordSquared > diameterSquared || (chordSquared === diameterSquared && value.major))
                fail("Circular-arc radius and branch are incoherent with its endpoints.");
        }
    });
    oneBased(records.fragments.map((value) => value.id), "result fragment");
    records.rings.forEach((value, index) => {
        reserved(105, index, [[28, 4]]);
        sliceRange(records.fragmentReferences, value.referenceBegin, value.referenceCount, "ring fragments");
        if (value.id === 0n ||
            (value.parent !== U32_NONE &&
                (value.parent >= index ||
                    required(records.rings[value.parent], "parent ring").depth + 1 !== value.depth)) ||
            (value.parent === U32_NONE && value.depth !== 0) ||
            (value.flags & ~1) !== 0 ||
            ((value.flags & 1) !== 0) !== (value.depth % 2 === 1))
            fail("Invalid result ring.");
        const fragments = sliceRange(records.fragmentReferences, value.referenceBegin, value.referenceCount, "ring fragments").map((fragment) => required(records.fragments[fragment], "ring fragment"));
        if (fragments.length < 2)
            fail("Result ring has too few fragments.");
        fragments.forEach((fragment, fragmentIndex) => {
            const next = required(fragments[(fragmentIndex + 1) % fragments.length], "next ring fragment");
            if (fragment.end !== next.start)
                fail("Result ring fragment topology is disconnected.");
        });
    });
    oneBased(records.rings.map((value) => value.id), "result ring");
    partition(records.rings.map((value) => [value.referenceBegin, value.referenceCount]), records.fragmentReferences.length, "ring fragment-reference");
    for (const reference of records.fragmentReferences)
        if (reference >= records.fragments.length)
            fail("Invalid fragment reference.");
    records.regions.forEach((value, index) => {
        reserved(107, index, [[16, 8]]);
        if (value.id === 0n ||
            value.outer >= records.rings.length ||
            required(records.rings[value.outer], "region outer ring").depth % 2 !== 0 ||
            value.positiveSet === 0 ||
            value.positiveSet > records.sourceSets.length)
            fail("Invalid result region.");
    });
    oneBased(records.regions.map((value) => value.id), "result region");
    const regionOuters = records.regions.map((value) => value.outer);
    if (new Set(regionOuters).size !== regionOuters.length)
        fail("A result ring is owned by more than one region.");
    const regionOuterSet = new Set(regionOuters);
    records.rings.forEach((ring, index) => {
        if ((ring.depth % 2 === 0) !== regionOuterSet.has(index))
            fail("Even-depth result rings and result regions are not one-to-one.");
    });
    records.sourceSets.forEach((value) => {
        sliceRange(records.sourceIndices, value.begin, value.count, "source indices");
        if (value.count === 0)
            fail("Empty source-set record.");
    });
    partition(records.sourceSets.map((value) => [value.begin, value.count]), records.sourceIndices.length, "source-set index");
    for (const index of records.sourceIndices)
        if (index >= records.sources.length)
            fail("Invalid source index.");
    records.sources.forEach((value, index) => {
        reserved(110, index, [[4, 4]]);
        const high = Number(value.secondaryId >> 32n);
        const low = Number(value.secondaryId & 0xffffffffn);
        let allowedRole = false;
        if (value.kind === 1) {
            allowedRole = value.secondaryId !== 0n && (value.role === 1 || value.role === 2);
        }
        else if (value.kind === 2) {
            if ([16, 17, 32, 33, 34, 35].includes(value.role))
                allowedRole = value.secondaryId === 0n;
            else if ([48, 49, 50, 51, 54].includes(value.role))
                allowedRole = high !== 0 && low === 0;
            else if (value.role === 52)
                allowedRole = high !== 0 && low !== 0;
            else if (value.role === 53)
                allowedRole = high === 1 && low === 0;
        }
        else if (value.kind === 3) {
            allowedRole = value.role === 0 && value.secondaryId === 0n;
        }
        if (value.operandId === 0n || value.primaryId === 0n || !allowedRole)
            fail("Invalid source reference.");
    });
    for (let index = 1; index < records.sources.length; index += 1)
        if (compareSource(required(records.sources[index - 1], "source"), required(records.sources[index], "source")) >= 0)
            fail("Source-reference table is not strictly canonical.");
    records.sourceSets.forEach((set) => {
        const members = sliceRange(records.sourceIndices, set.begin, set.count, "source set");
        for (let index = 1; index < members.length; index += 1)
            if (required(members[index - 1], "source member") >= required(members[index], "source member"))
                fail("Source-set members are not strictly ordered.");
    });
    for (let index = 1; index < records.sourceSets.length; index += 1) {
        const left = required(records.sourceSets[index - 1], "source set");
        const right = required(records.sourceSets[index], "source set");
        const leftMembers = sliceRange(records.sourceIndices, left.begin, left.count, "source set");
        const rightMembers = sliceRange(records.sourceIndices, right.begin, right.count, "source set");
        if (compareNumberArrays(leftMembers, rightMembers) >= 0)
            fail("Source-set table is not strictly canonical.");
    }
    records.events.forEach((value, index) => {
        reserved(111, index, [
            [10, 2],
            [24, 24],
        ]);
        sliceRange(records.ringRegionReferences, value.referenceBegin, value.referenceCount, "event references");
        if (value.operandId === 0n ||
            outcomeKinds[value.kind] === undefined ||
            value.sourceSet > records.sourceSets.length)
            fail("Invalid operand event.");
        const references = sliceRange(records.ringRegionReferences, value.referenceBegin, value.referenceCount, "event references");
        for (let offset = 0; offset < references.length; offset += 1) {
            const reference = required(references[offset], "event reference");
            if (offset > 0 && required(references[offset - 1], "event reference") >= reference)
                fail("Operand-event references are not strictly ordered.");
            const kind = Number(reference >> 32n);
            const target = Number(reference & 0xffffffffn);
            if ((kind === 1 && target >= records.rings.length) ||
                (kind === 2 && target >= records.regions.length) ||
                (kind !== 1 && kind !== 2))
                fail("Invalid operand-event result reference.");
        }
    });
    partition(records.events.map((value) => [value.referenceBegin, value.referenceCount]), records.ringRegionReferences.length, "operand-event result-reference");
    records.relationships.forEach((value, index) => {
        reserved(112, index, [
            [10, 2],
            [20, 12],
        ]);
        sliceRange(records.pairs, value.pairBegin, value.pairCount, "relationship pairs");
        if (value.queryId === 0n ||
            value.status > 1 ||
            value.dimension > 3 ||
            (value.status === 1 &&
                (value.dimension !== 0 || value.pairBegin !== 0 || value.pairCount !== 0)))
            fail("Invalid relationship result.");
        let aggregate = 0;
        let previousPair;
        for (const pair of sliceRange(records.pairs, value.pairBegin, value.pairCount, "relationship pairs")) {
            const key = [
                pair.left,
                pair.right,
                pair.dimension,
                pair.equality ? 1 : 0,
                pair.leftContains ? 1 : 0,
                pair.rightContains ? 1 : 0,
            ];
            if (previousPair !== undefined && compareComparable(previousPair, key) >= 0)
                fail("Relationship pairs are not strictly canonical.");
            previousPair = key;
            aggregate = Math.max(aggregate, pair.dimension);
        }
        if (value.status === 0 && value.dimension !== aggregate)
            fail("Relationship aggregate dimension does not match its pairs.");
    });
    strictlyIncreasing(records.relationships.map((value) => value.queryId), "relationship query ids");
    partition(records.relationships.map((value) => [value.pairBegin, value.pairCount]), records.pairs.length, "relationship pair");
    records.pairs.forEach((value, index) => {
        reserved(113, index, [[20, 12]]);
        const pair = recordView(required(tables[12], "relationship pair table"), index);
        if (value.dimension > 3 ||
            value.left < 1n ||
            value.left > BigInt(records.regions.length) ||
            value.right < 1n ||
            value.right > BigInt(records.regions.length) ||
            pair.getUint8(17) > 1 ||
            pair.getUint8(18) > 1 ||
            pair.getUint8(19) > 1)
            fail("Invalid relationship pair.");
        if (((value.equality || value.leftContains || value.rightContains) && value.dimension !== 3) ||
            (value.equality && (!value.leftContains || !value.rightContains)))
            fail("Relationship pair flags are inconsistent with its dimension.");
    });
    const usedSets = flags(records.sourceSets.length);
    const markHandle = (handle) => {
        if (handle !== 0)
            usedSets[handle - 1] = true;
    };
    records.vertices.forEach((value) => {
        markHandle(value.sourceSet);
    });
    records.fragments.forEach((value) => {
        markHandle(value.positiveSet);
        markHandle(value.subtractionSet);
    });
    records.regions.forEach((value) => {
        markHandle(value.positiveSet);
    });
    records.events.forEach((value) => {
        markHandle(value.sourceSet);
    });
    if (usedSets.some((value) => !value))
        fail("Result packet contains an unused source set.");
    const usedSources = flags(records.sources.length);
    for (const index of records.sourceIndices)
        usedSources[index] = true;
    if (usedSources.some((value) => !value))
        fail("Result packet contains an unused source reference.");
    const mutableOwners = {
        vertices: Array.from({ length: records.vertices.length }, () => -1),
        fragments: Array.from({ length: records.fragments.length }, () => -1),
        rings: Array.from({ length: records.rings.length }, () => -1),
        regions: Array.from({ length: records.regions.length }, () => -1),
        events: Array.from({ length: records.events.length }, () => -1),
    };
    records.jobs.forEach((job, jobIndex) => {
        for (let offset = 0; offset < job.regionCount; offset += 1)
            mutableOwners.regions[job.regionBegin + offset] = jobIndex;
        for (let offset = 0; offset < job.eventCount; offset += 1)
            mutableOwners.events[job.eventBegin + offset] = jobIndex;
    });
    const outerRegions = Array.from({ length: records.rings.length }, () => -1);
    records.regions.forEach((region, index) => {
        outerRegions[region.outer] = index;
    });
    records.rings.forEach((ring, index) => {
        const owner = ring.parent === U32_NONE
            ? mutableOwners.regions[required(outerRegions[index], "root-ring region")]
            : mutableOwners.rings[ring.parent];
        if (owner === undefined || owner < 0)
            fail("Result ring has no owning job.");
        mutableOwners.rings[index] = owner;
        for (const fragment of sliceRange(records.fragmentReferences, ring.referenceBegin, ring.referenceCount, "ring fragments")) {
            const previous = mutableOwners.fragments[fragment];
            if (previous === undefined || (previous !== -1 && previous !== owner))
                fail("A mutable result record is shared by jobs.");
            mutableOwners.fragments[fragment] = owner;
        }
    });
    records.regions.forEach((region, index) => {
        if (required(mutableOwners.rings[region.outer], "outer-ring owner") !==
            required(mutableOwners.regions[index], "region owner"))
            fail("Result region and its outer ring belong to different jobs.");
    });
    records.fragments.forEach((fragment, index) => {
        const owner = required(mutableOwners.fragments[index], "fragment owner");
        for (const vertex of [fragment.start, fragment.end]) {
            const previous = mutableOwners.vertices[vertex];
            if (previous === undefined || (previous !== -1 && previous !== owner))
                fail("A mutable result record is shared by jobs.");
            mutableOwners.vertices[vertex] = owner;
        }
    });
    for (const owners of Object.values(mutableOwners))
        if (owners.some((owner) => owner === -1))
            fail("Result packet contains an unowned result record.");
    const jobBounds = records.jobs.map(() => ({
        hasVertex: false,
        minX: 0n,
        maxX: 0n,
        minY: 0n,
        maxY: 0n,
    }));
    records.vertices.forEach((vertex, index) => {
        const owner = required(mutableOwners.vertices[index], "vertex owner");
        const bounds = required(jobBounds[owner], "job bounds");
        if (!bounds.hasVertex) {
            bounds.hasVertex = true;
            bounds.minX = bounds.maxX = vertex.x;
            bounds.minY = bounds.maxY = vertex.y;
        }
        else {
            bounds.minX = vertex.x < bounds.minX ? vertex.x : bounds.minX;
            bounds.maxX = vertex.x > bounds.maxX ? vertex.x : bounds.maxX;
            bounds.minY = vertex.y < bounds.minY ? vertex.y : bounds.minY;
            bounds.maxY = vertex.y > bounds.maxY ? vertex.y : bounds.maxY;
        }
    });
    if (jobBounds.some((bounds) => bounds.hasVertex &&
        (bounds.maxX - bounds.minX > MAX_LENGTH_NM || bounds.maxY - bounds.minY > MAX_LENGTH_NM)))
        fail("A job result exceeds the governed coordinate span.");
    records.events.forEach((event, index) => {
        const owner = required(mutableOwners.events[index], "event owner");
        for (const reference of sliceRange(records.ringRegionReferences, event.referenceBegin, event.referenceCount, "event references")) {
            const kind = Number(reference >> 32n);
            const target = Number(reference & 0xffffffffn);
            const referencedOwner = kind === 1
                ? required(mutableOwners.rings[target], "ring owner")
                : required(mutableOwners.regions[target], "region owner");
            if (referencedOwner !== owner)
                fail("Operand event references a different job closure.");
        }
    });
    validateCanonicalResultOrder(records, mutableOwners);
    return buildJobSelections(records, mutableOwners);
}
function validateCanonicalResultOrder(records, owners) {
    const jobId = (owner) => required(records.jobs[owner], "owner job").jobId;
    const incidents = records.vertices.map(() => []);
    records.fragments.forEach((fragment) => {
        const start = required(records.vertices[fragment.start], "fragment start vertex");
        const end = required(records.vertices[fragment.end], "fragment end vertex");
        required(incidents[fragment.start], "start incidents").push([
            0,
            end.x,
            end.y,
            fragment.kind,
            fragment.direction,
            fragment.major,
            fragment.radius,
        ]);
        required(incidents[fragment.end], "end incidents").push([
            1,
            start.x,
            start.y,
            fragment.kind,
            fragment.direction,
            fragment.major,
            fragment.radius,
        ]);
    });
    const vertexKeys = records.vertices.map((vertex, vertexIndex) => {
        const vertexIncidents = required(incidents[vertexIndex], "vertex incidents");
        vertexIncidents.sort(compareComparable);
        return [
            jobId(required(owners.vertices[vertexIndex], "vertex owner")),
            vertex.x,
            vertex.y,
            vertexIncidents,
            vertex.sourceSet,
        ];
    });
    strictlyCanonicalKeys(vertexKeys, "result vertices");
    const fragmentKeys = records.fragments.map((fragment, index) => [
        jobId(required(owners.fragments[index], "fragment owner")),
        fragment.start,
        fragment.end,
        fragment.kind,
        fragment.direction,
        fragment.major,
        fragment.radius,
        fragment.positiveSet,
        fragment.subtractionSet,
    ]);
    strictlyCanonicalKeys(fragmentKeys, "directed fragments");
    const ringKeys = records.rings.map((ring, index) => {
        const references = [
            ...sliceRange(records.fragmentReferences, ring.referenceBegin, ring.referenceCount, "ring references"),
        ];
        const rotation = leastRotation(references);
        if (rotation !== 0)
            fail("Result ring does not use its least canonical fragment rotation.");
        return [
            jobId(required(owners.rings[index], "ring owner")),
            ring.depth,
            references,
            ring.parent,
        ];
    });
    strictlyCanonicalKeys(ringKeys, "result rings");
    const regionKeys = records.regions.map((region, index) => [
        jobId(required(owners.regions[index], "region owner")),
        region.outer,
        region.positiveSet,
    ]);
    strictlyCanonicalKeys(regionKeys, "result regions");
    for (const job of records.jobs) {
        const keys = sliceRange(records.events, job.eventBegin, job.eventCount, "job events").map((event) => [
            event.operandId,
            event.kind,
            [
                ...sliceRange(records.ringRegionReferences, event.referenceBegin, event.referenceCount, "event references"),
            ],
            event.sourceSet,
        ]);
        strictlyCanonicalKeys(keys, "operand events");
    }
}
function strictlyCanonicalKeys(keys, label) {
    for (let index = 1; index < keys.length; index += 1)
        if (compareComparable(required(keys[index - 1], `${label} key`), required(keys[index], `${label} key`)) >= 0)
            fail(`${label} are not strictly canonical.`);
}
function compareComparable(left, right) {
    if (Array.isArray(left) && Array.isArray(right)) {
        const count = Math.min(left.length, right.length);
        for (let index = 0; index < count; index += 1) {
            const comparison = compareComparable(required(left[index], "comparison member"), required(right[index], "comparison member"));
            if (comparison !== 0)
                return comparison;
        }
        return left.length - right.length;
    }
    if (Array.isArray(left) || Array.isArray(right))
        fail("Canonical comparison shape mismatch.");
    const a = typeof left === "boolean" ? (left ? 1 : 0) : left;
    const b = typeof right === "boolean" ? (right ? 1 : 0) : right;
    return a < b ? -1 : a > b ? 1 : 0;
}
function leastRotation(values) {
    if (values.length === 0)
        return 0;
    let left = 0;
    let right = 1;
    let offset = 0;
    while (left < values.length && right < values.length && offset < values.length) {
        const a = required(values[(left + offset) % values.length], "rotation member");
        const b = required(values[(right + offset) % values.length], "rotation member");
        if (a === b) {
            offset += 1;
            continue;
        }
        if (a > b) {
            left += offset + 1;
            if (left === right)
                left += 1;
        }
        else {
            right += offset + 1;
            if (left === right)
                right += 1;
        }
        offset = 0;
    }
    return Math.min(left, right);
}
function buildJobSelections(records, owners) {
    const selections = records.jobs.map(() => ({
        vertices: [],
        fragments: [],
        rings: [],
        regions: [],
        events: [],
        sets: [],
        sources: [],
    }));
    const setsByJob = records.jobs.map(() => new Set());
    const appendOwned = (values, key) => {
        values.forEach((owner, index) => {
            required(selections[owner], "job selection")[key].push(index);
        });
    };
    appendOwned(owners.vertices, "vertices");
    appendOwned(owners.fragments, "fragments");
    appendOwned(owners.rings, "rings");
    appendOwned(owners.regions, "regions");
    appendOwned(owners.events, "events");
    const markSet = (owner, handle) => {
        if (handle !== 0)
            required(setsByJob[owner], "job source sets").add(handle - 1);
    };
    records.vertices.forEach((value, index) => {
        markSet(required(owners.vertices[index], "vertex owner"), value.sourceSet);
    });
    records.fragments.forEach((value, index) => {
        const owner = required(owners.fragments[index], "fragment owner");
        markSet(owner, value.positiveSet);
        markSet(owner, value.subtractionSet);
    });
    records.regions.forEach((value, index) => {
        markSet(required(owners.regions[index], "region owner"), value.positiveSet);
    });
    records.events.forEach((value, index) => {
        markSet(required(owners.events[index], "event owner"), value.sourceSet);
    });
    selections.forEach((selection, jobIndex) => {
        selection.sets = [...required(setsByJob[jobIndex], "job source sets")].sort((a, b) => a - b);
        const sources = new Set();
        for (const setIndex of selection.sets) {
            const set = required(records.sourceSets[setIndex], "source set");
            for (const source of sliceRange(records.sourceIndices, set.begin, set.count, "source set"))
                sources.add(source);
        }
        selection.sources = [...sources].sort((a, b) => a - b);
    });
    return selections;
}
function encodeStandalone(input, jobIndex, selection) {
    const vertexMap = sparseMap(selection.vertices);
    const fragmentMap = sparseMap(selection.fragments);
    const ringMap = sparseMap(selection.rings);
    const regionMap = sparseMap(selection.regions);
    const setMap = sparseMap(selection.sets);
    const sourceMap = sparseMap(selection.sources);
    const job = required(input.jobs[jobIndex], "standalone job");
    const output = {
        jobs: [],
        diagnostics: [
            ...sliceRange(input.diagnostics, job.diagnosticBegin, job.diagnosticCount, "diagnostics"),
        ],
        vertices: [],
        fragments: [],
        rings: [],
        fragmentReferences: [],
        regions: [],
        ringRegionReferences: [],
        sourceSets: [],
        sources: selection.sources.map((index) => required(input.sources[index], "source")),
        events: [],
        relationships: [],
        pairs: [],
        sourceIndices: [],
    };
    for (const index of selection.vertices) {
        const value = required(input.vertices[index], "vertex");
        output.vertices.push({
            ...value,
            id: BigInt(output.vertices.length + 1),
            sourceSet: remapHandle(value.sourceSet, setMap),
        });
    }
    for (const index of selection.fragments) {
        const value = required(input.fragments[index], "fragment");
        output.fragments.push({
            ...value,
            id: BigInt(output.fragments.length + 1),
            start: mapped(vertexMap, value.start),
            end: mapped(vertexMap, value.end),
            positiveSet: remapHandle(value.positiveSet, setMap),
            subtractionSet: remapHandle(value.subtractionSet, setMap),
        });
    }
    for (const index of selection.rings) {
        const value = required(input.rings[index], "ring");
        const begin = output.fragmentReferences.length;
        for (const reference of sliceRange(input.fragmentReferences, value.referenceBegin, value.referenceCount, "ring references"))
            output.fragmentReferences.push(mapped(fragmentMap, reference));
        output.rings.push({
            ...value,
            id: BigInt(output.rings.length + 1),
            referenceBegin: begin,
            parent: value.parent === U32_NONE ? U32_NONE : mapped(ringMap, value.parent),
        });
    }
    for (const index of selection.regions) {
        const value = required(input.regions[index], "region");
        output.regions.push({
            ...value,
            id: BigInt(output.regions.length + 1),
            outer: mapped(ringMap, value.outer),
            positiveSet: remapHandle(value.positiveSet, setMap),
        });
    }
    for (const index of selection.sets) {
        const value = required(input.sourceSets[index], "source set");
        const begin = output.sourceIndices.length;
        for (const source of sliceRange(input.sourceIndices, value.begin, value.count, "source indices"))
            output.sourceIndices.push(mapped(sourceMap, source));
        output.sourceSets.push({ begin, count: value.count });
    }
    for (const index of selection.events) {
        const value = required(input.events[index], "event");
        const begin = value.referenceCount === 0 ? 0 : output.ringRegionReferences.length;
        for (const reference of sliceRange(input.ringRegionReferences, value.referenceBegin, value.referenceCount, "event references")) {
            const kind = Number(reference >> 32n);
            const target = Number(reference & 0xffffffffn);
            output.ringRegionReferences.push((BigInt(kind) << 32n) |
                BigInt(kind === 1 ? mapped(ringMap, target) : mapped(regionMap, target)));
        }
        output.events.push({
            ...value,
            referenceBegin: begin,
            sourceSet: remapHandle(value.sourceSet, setMap),
        });
    }
    output.jobs.push({ ...job, diagnosticBegin: 0, regionBegin: 0, eventBegin: 0 });
    return encodeResultRecords(output);
}
function encodeResultRecords(records) {
    const tables = RESULT_TABLES.map((recordBytes, index) => ({
        kind: 101 + index,
        recordBytes,
        records: [],
    }));
    const add = (index, bytes) => {
        required(tables[index], `result table ${index}`).records.push(bytes);
    };
    for (const v of records.jobs)
        add(0, record(48, (d) => {
            putU64(d, 0, v.jobId, "job id", true);
            d.setUint8(8, v.status);
            d.setUint32(16, v.diagnosticBegin, true);
            d.setUint32(20, v.diagnosticCount, true);
            d.setUint32(24, v.regionBegin, true);
            d.setUint32(28, v.regionCount, true);
            d.setUint32(32, v.eventBegin, true);
            d.setUint32(36, v.eventCount, true);
        }));
    for (const v of records.diagnostics)
        add(1, record(56, (d) => {
            d.setUint32(0, v.code, true);
            d.setUint8(4, v.severity);
            d.setUint8(5, 1);
            d.setUint16(6, v.presence, true);
            d.setBigUint64(8, v.jobId, true);
            d.setBigUint64(16, v.stageId, true);
            d.setBigUint64(24, v.operandId, true);
            d.setBigUint64(32, v.geometryId, true);
            d.setUint32(40, v.pathToken, true);
        }));
    for (const v of records.vertices)
        add(2, record(32, (d) => {
            d.setBigUint64(0, v.id, true);
            d.setBigInt64(8, v.x, true);
            d.setBigInt64(16, v.y, true);
            d.setUint32(24, v.sourceSet, true);
            d.setUint32(28, v.flags, true);
        }));
    for (const v of records.fragments)
        add(3, record(48, (d) => {
            d.setBigUint64(0, v.id, true);
            d.setUint32(8, v.start, true);
            d.setUint32(12, v.end, true);
            d.setUint8(16, v.kind);
            d.setUint8(17, v.direction);
            d.setUint8(18, v.major ? 1 : 0);
            d.setBigUint64(24, v.radius, true);
            d.setUint32(32, v.positiveSet, true);
            d.setUint32(36, v.subtractionSet, true);
        }));
    for (const v of records.rings)
        add(4, record(32, (d) => {
            d.setBigUint64(0, v.id, true);
            d.setUint32(8, v.referenceBegin, true);
            d.setUint32(12, v.referenceCount, true);
            d.setUint32(16, v.parent, true);
            d.setUint32(20, v.depth, true);
            d.setUint32(24, v.flags, true);
        }));
    for (const v of records.fragmentReferences)
        add(5, record(4, (d) => d.setUint32(0, v, true)));
    for (const v of records.regions)
        add(6, record(24, (d) => {
            d.setBigUint64(0, v.id, true);
            d.setUint32(8, v.outer, true);
            d.setUint32(12, v.positiveSet, true);
        }));
    for (const v of records.ringRegionReferences)
        add(7, record(8, (d) => d.setBigUint64(0, v, true)));
    for (const v of records.sourceSets)
        add(8, record(8, (d) => {
            d.setUint32(0, v.begin, true);
            d.setUint32(4, v.count, true);
        }));
    for (const v of records.sources)
        add(9, record(32, (d) => {
            d.setUint16(0, v.kind, true);
            d.setUint16(2, v.role, true);
            d.setBigUint64(8, v.operandId, true);
            d.setBigUint64(16, v.primaryId, true);
            d.setBigUint64(24, v.secondaryId, true);
        }));
    for (const v of records.events)
        add(10, record(48, (d) => {
            d.setBigUint64(0, v.operandId, true);
            d.setUint16(8, v.kind, true);
            d.setUint32(12, v.referenceBegin, true);
            d.setUint32(16, v.referenceCount, true);
            d.setUint32(20, v.sourceSet, true);
        }));
    for (const v of records.relationships)
        add(11, record(32, (d) => {
            d.setBigUint64(0, v.queryId, true);
            d.setUint8(8, v.status);
            d.setUint8(9, v.dimension);
            d.setUint32(12, v.pairBegin, true);
            d.setUint32(16, v.pairCount, true);
        }));
    for (const v of records.pairs)
        add(12, record(32, (d) => {
            d.setBigUint64(0, v.left, true);
            d.setBigUint64(8, v.right, true);
            d.setUint8(16, v.dimension);
            d.setUint8(17, v.equality ? 1 : 0);
            d.setUint8(18, v.leftContains ? 1 : 0);
            d.setUint8(19, v.rightContains ? 1 : 0);
        }));
    for (const v of records.sourceIndices)
        add(13, record(4, (d) => d.setUint32(0, v, true)));
    return encodeTables(RESULT_MAGIC, tables, records.jobs.length, records.relationships.length);
}
function decodeDirectory(bytes, magic, firstKind, recordSizes) {
    if (bytes.byteLength < HEADER_BYTES || bytes.byteLength > MAX_PACKET_BYTES)
        fail("Packet size is outside the A0 bounds.");
    const view = new DataView(bytes.buffer, bytes.byteOffset, bytes.byteLength);
    const actualMagic = new TextDecoder("ascii", { fatal: true }).decode(bytes.subarray(0, 8));
    if (actualMagic !== magic ||
        view.getUint16(8, true) !== 1 ||
        view.getUint16(10, true) !== HEADER_BYTES ||
        view.getUint32(12, true) !== 0 ||
        view.getBigUint64(16, true) !== BigInt(bytes.byteLength) ||
        view.getBigUint64(24, true) !== 64n ||
        view.getUint32(32, true) !== recordSizes.length ||
        view.getUint32(44, true) !== 0 ||
        view.getBigUint64(56, true) !== 0n)
        fail("Invalid packet header.");
    let cursor = align8(HEADER_BYTES + recordSizes.length * DIRECTORY_ENTRY_BYTES);
    let payloadBytes = 0n;
    const tables = [];
    for (let index = 0; index < recordSizes.length; index += 1) {
        const entry = HEADER_BYTES + index * DIRECTORY_ENTRY_BYTES;
        const kind = firstKind + index;
        const recordBytes = required(recordSizes[index], "record size");
        const offset = safeNumber(view.getBigUint64(entry + 8, true), "table offset");
        const byteLength = safeNumber(view.getBigUint64(entry + 16, true), "table length");
        const count = safeNumber(view.getBigUint64(entry + 24, true), "table count");
        if (view.getUint16(entry, true) !== kind ||
            view.getUint16(entry + 2, true) !== 1 ||
            view.getUint32(entry + 4, true) !== recordBytes ||
            byteLength !== count * recordBytes ||
            offset !== cursor ||
            offset + byteLength > bytes.byteLength)
            fail("Invalid packet table directory.");
        tables.push({ count, kind, offset, recordBytes, view });
        payloadBytes += BigInt(byteLength);
        const end = offset + byteLength;
        cursor = index + 1 === recordSizes.length ? end : align8(end);
        for (let at = end; at < cursor; at += 1)
            if (view.getUint8(at) !== 0)
                fail("Packet alignment padding is nonzero.");
    }
    if (cursor !== bytes.byteLength || view.getBigUint64(48, true) !== payloadBytes)
        fail("Packet payload accounting is invalid.");
    return tables;
}
function encodeTables(magic, tables, jobCount, relationshipCount) {
    let cursor = align8(HEADER_BYTES + tables.length * DIRECTORY_ENTRY_BYTES);
    const offsets = [];
    let payloadBytes = 0;
    tables.forEach((table, index) => {
        offsets.push(cursor);
        const bytes = table.records.length * table.recordBytes;
        payloadBytes += bytes;
        const end = cursor + bytes;
        cursor = index + 1 === tables.length ? end : align8(end);
    });
    if (cursor > MAX_PACKET_BYTES)
        fail("Packet exceeds the A0 attachment bound.");
    const output = new Uint8Array(cursor);
    const view = new DataView(output.buffer);
    output.set(new TextEncoder().encode(magic), 0);
    view.setUint16(8, 1, true);
    view.setUint16(10, HEADER_BYTES, true);
    view.setBigUint64(16, BigInt(output.byteLength), true);
    view.setBigUint64(24, 64n, true);
    view.setUint32(32, tables.length, true);
    view.setUint32(36, jobCount, true);
    view.setUint32(40, relationshipCount, true);
    view.setBigUint64(48, BigInt(payloadBytes), true);
    tables.forEach((table, index) => {
        const entry = HEADER_BYTES + index * DIRECTORY_ENTRY_BYTES;
        const offset = required(offsets[index], "table offset");
        view.setUint16(entry, table.kind, true);
        view.setUint16(entry + 2, 1, true);
        view.setUint32(entry + 4, table.recordBytes, true);
        view.setBigUint64(entry + 8, BigInt(offset), true);
        view.setBigUint64(entry + 16, BigInt(table.records.length * table.recordBytes), true);
        view.setBigUint64(entry + 24, BigInt(table.records.length), true);
        table.records.forEach((value, recordIndex) => {
            output.set(value, offset + recordIndex * table.recordBytes);
        });
    });
    return output;
}
function record(size, write) {
    const value = new Uint8Array(size);
    write(new DataView(value.buffer));
    return value;
}
function recordView(table, index) {
    if (!Number.isSafeInteger(index) || index < 0 || index >= table.count)
        fail(`Invalid index into result table ${table.kind}.`);
    return new DataView(table.view.buffer, table.view.byteOffset + table.offset + index * table.recordBytes, table.recordBytes);
}
function mapTable(table, project) {
    const value = required(table, "result table");
    return Array.from({ length: value.count }, (_, index) => project(recordView(value, index), index));
}
function sliceRange(values, begin, count, label) {
    if (!Number.isSafeInteger(begin) ||
        !Number.isSafeInteger(count) ||
        begin < 0 ||
        count < 0 ||
        begin + count > values.length ||
        (count === 0 && begin !== 0))
        fail(`Invalid ${label} range.`);
    return values.slice(begin, begin + count);
}
function putU32(view, offset, value, label) {
    if (!Number.isSafeInteger(value) || value < 0 || value > U32_NONE)
        fail(`${label} is outside uint32.`);
    view.setUint32(offset, value, true);
}
function putU64(view, offset, value, label, nonzero = false) {
    unsigned(value, label, nonzero);
    view.setBigUint64(offset, value, true);
}
function putI64(view, offset, value, label) {
    if (typeof value !== "bigint" || value < I64_MIN || value > I64_MAX)
        fail(`${label} must be an int64 bigint.`);
    view.setBigInt64(offset, value, true);
}
function unsigned(value, label, nonzero) {
    if (typeof value !== "bigint" || value < (nonzero ? 1n : 0n) || value > U64_MAX)
        fail(`${label} must be a${nonzero ? " nonzero" : ""} uint64 bigint.`);
}
function length(value, label) {
    unsigned(value, label, true);
    if (value > MAX_LENGTH_NM)
        fail(`${label} exceeds ${MAX_LENGTH_NM} nanometers.`);
}
function compareBigint(left, right) {
    return left < right ? -1 : left > right ? 1 : 0;
}
function compareSource(left, right) {
    for (const comparison of [
        left.kind - right.kind,
        left.role - right.role,
        compareBigint(left.operandId, right.operandId),
        compareBigint(left.primaryId, right.primaryId),
        compareBigint(left.secondaryId, right.secondaryId),
    ])
        if (comparison !== 0)
            return comparison;
    return 0;
}
function compareNumberArrays(left, right) {
    const common = Math.min(left.length, right.length);
    for (let index = 0; index < common; index += 1) {
        const comparison = required(left[index], "left member") - required(right[index], "right member");
        if (comparison !== 0)
            return comparison;
    }
    return left.length - right.length;
}
function equalBytes(left, right) {
    if (left.byteLength !== right.byteLength)
        return false;
    for (let index = 0; index < left.byteLength; index += 1)
        if (left[index] !== right[index])
            return false;
    return true;
}
function strictlyIncreasing(values, label) {
    for (let index = 1; index < values.length; index += 1)
        if (required(values[index - 1], label) >= required(values[index], label))
            fail(`${label} are not strictly increasing.`);
}
function oneBased(values, label) {
    values.forEach((value, index) => {
        if (value !== BigInt(index + 1))
            fail(`${label} ids are not canonical one-based ordinals.`);
    });
}
function partition(ranges, total, label) {
    let cursor = 0;
    for (const [begin, count] of ranges) {
        if (count === 0) {
            if (begin !== 0)
                fail(`Empty ${label} range must begin at zero.`);
        }
        else {
            if (begin !== cursor || begin + count > total)
                fail(`${label} ranges are not gapless.`);
            cursor += count;
        }
    }
    if (cursor !== total)
        fail(`${label} ranges do not own their complete table.`);
}
function align8(value) {
    return (value + 7) & ~7;
}
function safeNumber(value, label) {
    if (value > BigInt(Number.MAX_SAFE_INTEGER))
        fail(`${label} exceeds the JavaScript exact integer range.`);
    return Number(value);
}
function required(value, label) {
    if (value === undefined)
        fail(`Missing ${label}.`);
    return value;
}
function fail(message) {
    throw new AnalyticPacketError(message);
}
function flags(size) {
    return Array.from({ length: size }, () => false);
}
function sparseMap(indexes) {
    const output = new Map();
    indexes.forEach((value, index) => {
        output.set(value, index);
    });
    return output;
}
function mapped(mapping, index) {
    const value = mapping.get(index);
    if (value === undefined)
        fail("Standalone closure contains an unmapped reference.");
    return value;
}
function remapHandle(handle, mapping) {
    return handle === 0 ? 0 : mapped(mapping, handle - 1) + 1;
}
async function sha256Hex(bytes) {
    if (globalThis.crypto?.subtle === undefined)
        fail("SHA-256 is unavailable in this JavaScript runtime.");
    const source = bytes.byteOffset === 0 && bytes.byteLength === bytes.buffer.byteLength
        ? bytes.buffer
        : bytes.slice().buffer;
    const digest = await globalThis.crypto.subtle.digest("SHA-256", source);
    return [...new Uint8Array(digest)].map((value) => value.toString(16).padStart(2, "0")).join("");
}
