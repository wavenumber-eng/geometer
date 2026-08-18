import { compareBigint, encodeTables, fail, length, MAX_JOBS, MAX_OPERANDS, MAX_QUERIES, MAX_RING_SEGMENTS, MAX_STAGES, putI64, putU32, putU64, REQUEST_MAGIC, REQUEST_TABLES, record, required, unsigned, } from "./analytic-packet-a0-common.js";
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
