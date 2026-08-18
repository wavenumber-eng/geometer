export const REQUEST_MAGIC = "GMABRQ01";
export const RESULT_MAGIC = "GMABRS01";
export const HEADER_BYTES = 64;
export const DIRECTORY_ENTRY_BYTES = 32;
export const MAX_PACKET_BYTES = 256 * 1024 * 1024;
export const U32_NONE = 0xffff_ffff;
export const U64_MAX = (1n << 64n) - 1n;
export const I64_MIN = -(1n << 63n);
export const I64_MAX = (1n << 63n) - 1n;
export const MAX_LENGTH_NM = 1000000000000n;
export const MAX_JOBS = 65_535;
export const MAX_STAGES = 1_048_576;
export const MAX_OPERANDS = 4_194_304;
export const MAX_QUERIES = 1_048_576;
export const MAX_RING_SEGMENTS = 131_072;
export const MAX_LOGICAL_SOURCE_REFERENCE_EXPANSIONS = 1_048_576;
export const REQUEST_TABLES = [24, 32, 24, 32, 4, 32, 24, 40, 32, 40, 48, 32, 24];
export const RESULT_TABLES = [48, 56, 32, 48, 32, 4, 24, 8, 8, 32, 48, 32, 32, 4];
export const diagnosticCodes = {
    65539: "geometer.operation.analytic_planar_boolean.invalid_topology",
    65540: "geometer.operation.analytic_planar_boolean.invalid_arc",
    65541: "geometer.operation.analytic_planar_boolean.unsupported_geometry",
    65543: "geometer.operation.analytic_planar_boolean.normalization_error_exceeded",
    65544: "geometer.operation.analytic_planar_boolean.normalization_topology_collapse",
    65545: "geometer.operation.analytic_planar_boolean.nonanalytic_result",
    65546: "geometer.operation.analytic_planar_boolean.solver_failed",
    65547: "geometer.operation.analytic_planar_boolean.resource_limit_exceeded",
};
export const pathTokens = [
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
export const sourceRoles = {
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
export const outcomeKinds = {
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
export function decodeDirectory(bytes, magic, firstKind, recordSizes) {
    const directoryEnd = HEADER_BYTES + recordSizes.length * DIRECTORY_ENTRY_BYTES;
    if (bytes.byteLength < directoryEnd || bytes.byteLength > MAX_PACKET_BYTES)
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
export function encodeTables(magic, tables, jobCount, relationshipCount) {
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
export function record(size, write) {
    const value = new Uint8Array(size);
    write(new DataView(value.buffer));
    return value;
}
export function recordView(table, index) {
    if (!Number.isSafeInteger(index) || index < 0 || index >= table.count)
        fail(`Invalid index into result table ${table.kind}.`);
    return new DataView(table.view.buffer, table.view.byteOffset + table.offset + index * table.recordBytes, table.recordBytes);
}
export function mapTable(table, project) {
    const value = required(table, "result table");
    return Array.from({ length: value.count }, (_, index) => project(recordView(value, index), index));
}
export function sliceRange(values, begin, count, label) {
    if (!Number.isSafeInteger(begin) ||
        !Number.isSafeInteger(count) ||
        begin < 0 ||
        count < 0 ||
        begin + count > values.length ||
        (count === 0 && begin !== 0))
        fail(`Invalid ${label} range.`);
    return values.slice(begin, begin + count);
}
export function putU32(view, offset, value, label) {
    if (!Number.isSafeInteger(value) || value < 0 || value > U32_NONE)
        fail(`${label} is outside uint32.`);
    view.setUint32(offset, value, true);
}
export function putU64(view, offset, value, label, nonzero = false) {
    unsigned(value, label, nonzero);
    view.setBigUint64(offset, value, true);
}
export function putI64(view, offset, value, label) {
    if (typeof value !== "bigint" || value < I64_MIN || value > I64_MAX)
        fail(`${label} must be an int64 bigint.`);
    view.setBigInt64(offset, value, true);
}
export function unsigned(value, label, nonzero) {
    if (typeof value !== "bigint" || value < (nonzero ? 1n : 0n) || value > U64_MAX)
        fail(`${label} must be a${nonzero ? " nonzero" : ""} uint64 bigint.`);
}
export function length(value, label) {
    unsigned(value, label, true);
    if (value > MAX_LENGTH_NM)
        fail(`${label} exceeds ${MAX_LENGTH_NM} nanometers.`);
}
export function compareBigint(left, right) {
    return left < right ? -1 : left > right ? 1 : 0;
}
export function compareSource(left, right) {
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
export function compareNumberArrays(left, right) {
    const common = Math.min(left.length, right.length);
    for (let index = 0; index < common; index += 1) {
        const comparison = required(left[index], "left member") - required(right[index], "right member");
        if (comparison !== 0)
            return comparison;
    }
    return left.length - right.length;
}
export function equalBytes(left, right) {
    if (left.byteLength !== right.byteLength)
        return false;
    for (let index = 0; index < left.byteLength; index += 1)
        if (left[index] !== right[index])
            return false;
    return true;
}
export function strictlyIncreasing(values, label) {
    for (let index = 1; index < values.length; index += 1)
        if (required(values[index - 1], label) >= required(values[index], label))
            fail(`${label} are not strictly increasing.`);
}
export function oneBased(values, label) {
    values.forEach((value, index) => {
        if (value !== BigInt(index + 1))
            fail(`${label} ids are not canonical one-based ordinals.`);
    });
}
export function partition(ranges, total, label) {
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
export function align8(value) {
    return (value + 7) & ~7;
}
export function safeNumber(value, label) {
    if (value > BigInt(Number.MAX_SAFE_INTEGER))
        fail(`${label} exceeds the JavaScript exact integer range.`);
    return Number(value);
}
export function required(value, label) {
    if (value === undefined)
        fail(`Missing ${label}.`);
    return value;
}
export function fail(message) {
    throw new AnalyticPacketError(message);
}
export function flags(size) {
    return Array.from({ length: size }, () => false);
}
export function sparseMap(indexes) {
    const output = new Map();
    indexes.forEach((value, index) => {
        output.set(value, index);
    });
    return output;
}
export function mapped(mapping, index) {
    const value = mapping.get(index);
    if (value === undefined)
        fail("Standalone closure contains an unmapped reference.");
    return value;
}
export function remapHandle(handle, mapping) {
    return handle === 0 ? 0 : mapped(mapping, handle - 1) + 1;
}
export async function sha256Hex(bytes) {
    if (globalThis.crypto?.subtle === undefined)
        fail("SHA-256 is unavailable in this JavaScript runtime.");
    const source = bytes.byteOffset === 0 && bytes.byteLength === bytes.buffer.byteLength
        ? bytes.buffer
        : bytes.slice().buffer;
    const digest = await globalThis.crypto.subtle.digest("SHA-256", source);
    return [...new Uint8Array(digest)].map((value) => value.toString(16).padStart(2, "0")).join("");
}
