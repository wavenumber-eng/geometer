export const NM_PER_MM = 1_000_000;
export const TRACE_WIDTH_NM = 150_000;
export const CLEARANCE_NM = 200_000;
export const TRACE_ENVELOPE_WIDTH_NM = TRACE_WIDTH_NM + CLEARANCE_NM * 2;
export const VIA_COPPER_RADIUS_NM = 350_000;
export const VIA_CLEARANCE_RADIUS_NM = VIA_COPPER_RADIUS_NM + CLEARANCE_NM;
export const THERMAL_SPOKE_WIDTH_NM = 180_000;
export const THERMAL_REACH_NM = VIA_CLEARANCE_RADIUS_NM + 120_000;
export function classifyPcbLayerJob(jobId) {
    if (jobId === 31n)
        return "base";
    if (jobId >= 40000n && jobId < 50000n)
        return "clearance";
    if (jobId >= 50000n)
        return "thermal";
    throw new RangeError(`Unknown PCB layer job ${jobId}.`);
}
export function initialPcbDemoState() {
    return {
        board: [
            { x: 0, y: 0 },
            { x: 26, y: 0 },
            { x: 26, y: 15 },
            { x: 19, y: 15 },
            { x: 17, y: 13 },
            { x: 0, y: 13 },
        ],
        vias: [
            { id: 1, x: 7, y: 4 },
        ],
        traces: [{ id: 1, points: [{ x: 4.5, y: 9.5 }, { x: 10, y: 9.5 }] }],
        nextViaId: 2,
        nextTraceId: 2,
    };
}
export function moveVia(state, id, point) {
    return {
        ...state,
        vias: state.vias.map((via) => (via.id === id ? { ...via, ...point } : via)),
    };
}
export function addVia(state, point) {
    return {
        ...state,
        vias: [...state.vias, { id: state.nextViaId, ...point }],
        nextViaId: state.nextViaId + 1,
    };
}
export function moveBoardVertex(state, index, point) {
    return {
        ...state,
        board: state.board.map((vertex, candidate) => (candidate === index ? point : vertex)),
    };
}
export function addTrace(state, points) {
    if (points.length < 2)
        return state;
    return {
        ...state,
        traces: [...state.traces, { id: state.nextTraceId, points: points.map((point) => ({ ...point })) }],
        nextTraceId: state.nextTraceId + 1,
    };
}
export function snapPointTo45(anchor, candidate) {
    const dx = candidate.x - anchor.x;
    const dy = candidate.y - anchor.y;
    if (dx === 0 && dy === 0)
        return { ...anchor };
    const magnitude = Math.max(Math.abs(dx), Math.abs(dy));
    const angle = Math.atan2(dy, dx);
    const snappedAngle = Math.round(angle / (Math.PI / 4)) * (Math.PI / 4);
    const unitX = Math.cos(snappedAngle);
    const unitY = Math.sin(snappedAngle);
    const scale = magnitude / Math.max(Math.abs(unitX), Math.abs(unitY));
    return {
        x: roundGrid(anchor.x + unitX * scale),
        y: roundGrid(anchor.y + unitY * scale),
    };
}
export function nearestHit(points, target, tolerance) {
    let winner;
    let distance = tolerance;
    points.forEach((point, index) => {
        const candidate = Math.hypot(point.x - target.x, point.y - target.y);
        if (candidate <= distance) {
            distance = candidate;
            winner = index;
        }
    });
    return winner;
}
export function makePcbPolygonPourRequest(state) {
    let nextOperand = 10000n;
    const operandId = () => {
        nextOperand += 1n;
        return nextOperand;
    };
    const pads = footprintPads().map((pad) => disk(operandId(), pad, 520000n));
    const clearances = [];
    for (const trace of state.traces)
        for (let index = 1; index < trace.points.length; index += 1) {
            const start = trace.points[index - 1];
            const end = trace.points[index];
            if (start !== undefined && end !== undefined)
                clearances.push(capsule(operandId(), start, end, BigInt(TRACE_ENVELOPE_WIDTH_NM)));
        }
    for (const via of state.vias)
        clearances.push(disk(operandId(), via, BigInt(VIA_CLEARANCE_RADIUS_NM)));
    let clearanceJobId = 40000n;
    const clearanceJobs = clearances.map((operand) => {
        clearanceJobId += 1n;
        return singleOperandJob(clearanceJobId, operand);
    });
    let thermalJobId = 50000n;
    const thermalJobs = state.vias.flatMap((via) => {
        const horizontalStart = { x: via.x - THERMAL_REACH_NM / NM_PER_MM, y: via.y };
        const horizontalEnd = { x: via.x + THERMAL_REACH_NM / NM_PER_MM, y: via.y };
        const verticalStart = { x: via.x, y: via.y - THERMAL_REACH_NM / NM_PER_MM };
        const verticalEnd = { x: via.x, y: via.y + THERMAL_REACH_NM / NM_PER_MM };
        const thermalCopper = [
            disk(operandId(), via, BigInt(VIA_COPPER_RADIUS_NM)),
            stripRegion(operandId(), horizontalStart, horizontalEnd, THERMAL_SPOKE_WIDTH_NM),
            stripRegion(operandId(), verticalStart, verticalEnd, THERMAL_SPOKE_WIDTH_NM),
        ];
        return thermalCopper.map((operand) => {
            thermalJobId += 1n;
            return singleOperandJob(thermalJobId, operand);
        });
    });
    return {
        jobs: [
            {
                job_id: 31n,
                stages: [
                    {
                        stage_id: 3101n,
                        operation: "union",
                        operands: [region(operandId(), state.board)],
                    },
                    { stage_id: 3102n, operation: "difference", operands: pads },
                ],
            },
            ...clearanceJobs,
            ...thermalJobs,
        ],
        relationship_queries: [],
    };
}
export function footprintPads() {
    return [
        { x: 4.5, y: 9.5 },
        { x: 4.5, y: 7.9 },
    ];
}
function region(operandId, points) {
    const base = operandId * 100n;
    const vertices = points.map((point, index) => ({
        vertex_id: base + BigInt(index + 1),
        point: pointNm(point),
    }));
    return {
        operand_id: operandId,
        kind: "planar_region",
        region_id: base + 40n,
        outer: {
            ring_id: base + 50n,
            vertices,
            segments: points.map((_, index) => ({
                segment_id: base + 60n + BigInt(index),
                curve_id: base + 80n + BigInt(index),
                kind: "line",
            })),
        },
        holes: [],
    };
}
function disk(operandId, center, radius) {
    return {
        operand_id: operandId,
        kind: "disk",
        feature_id: operandId * 100n + 1n,
        center: pointNm(center),
        radius_nm: radius,
    };
}
function capsule(operandId, start, end, width) {
    return {
        operand_id: operandId,
        kind: "capsule",
        feature_id: operandId * 100n + 1n,
        start: pointNm(start),
        end: pointNm(end),
        width_nm: width,
    };
}
function stripRegion(operandId, start, end, widthNm) {
    const dx = end.x - start.x;
    const dy = end.y - start.y;
    const length = Math.hypot(dx, dy);
    if (length === 0)
        return region(operandId, [
            { x: start.x - widthNm / NM_PER_MM / 2, y: start.y - widthNm / NM_PER_MM / 2 },
            { x: start.x + widthNm / NM_PER_MM / 2, y: start.y - widthNm / NM_PER_MM / 2 },
            { x: start.x + widthNm / NM_PER_MM / 2, y: start.y + widthNm / NM_PER_MM / 2 },
            { x: start.x - widthNm / NM_PER_MM / 2, y: start.y + widthNm / NM_PER_MM / 2 },
        ]);
    const half = widthNm / NM_PER_MM / 2;
    const ox = -dy / length * half;
    const oy = dx / length * half;
    return region(operandId, [
        gridNm({ x: start.x + ox, y: start.y + oy }),
        gridNm({ x: start.x - ox, y: start.y - oy }),
        gridNm({ x: end.x - ox, y: end.y - oy }),
        gridNm({ x: end.x + ox, y: end.y + oy }),
    ]);
}
function singleOperandJob(jobId, operand) {
    return {
        job_id: jobId,
        stages: [{ stage_id: jobId * 10n, operation: "union", operands: [operand] }],
    };
}
function pointNm(point) {
    return {
        x: BigInt(Math.round(point.x * NM_PER_MM)),
        y: BigInt(Math.round(point.y * NM_PER_MM)),
    };
}
function roundGrid(value) {
    return Math.round(value * 20) / 20;
}
function gridNm(point) {
    return {
        x: Math.round(point.x * NM_PER_MM) / NM_PER_MM,
        y: Math.round(point.y * NM_PER_MM) / NM_PER_MM,
    };
}
