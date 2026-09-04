export const INDEXED_TRIANGLE_MESH_MEDIA_TYPE = "application/vnd.wavenumber.geometer.indexed-triangle-mesh";
export const INDEXED_TRIANGLE_MESH_PACKET_FORMAT = "geometry.indexed_triangle_mesh.packet.a0";
const MAGIC = new TextEncoder().encode("GMIMSH01");
const HEADER_BYTES = 64;
const HAS_SOURCE_FACES = 1;
const MAX_PACKET_BYTES = 268_435_456;
const MAX_VERTICES = 2_000_000;
const MAX_TRIANGLES = 2_000_000;
const UNSPECIFIED_SOURCE_FACE = 0xffff_ffff;
export class IndexedTriangleMeshPacketError extends Error {
    constructor(message) {
        super(message);
        this.name = "IndexedTriangleMeshPacketError";
    }
}
export function encodeIndexedTriangleMeshA0Packet(mesh) {
    if (mesh.positions.length < 9 || mesh.positions.length % 3 !== 0) {
        fail("positions must contain at least three complete xyz vertices.");
    }
    if (mesh.indices.length < 3 || mesh.indices.length % 3 !== 0) {
        fail("indices must contain at least one complete triangle.");
    }
    const vertexCount = mesh.positions.length / 3;
    const triangleCount = mesh.indices.length / 3;
    if (vertexCount > MAX_VERTICES || triangleCount > MAX_TRIANGLES) {
        fail("indexed mesh exceeds its governed vertex or triangle limit.");
    }
    if (mesh.sourceFaces !== undefined && mesh.sourceFaces.length !== triangleCount) {
        fail("sourceFaces must contain exactly one value per triangle.");
    }
    let hasSourceFaces = false;
    if (mesh.sourceFaces !== undefined) {
        for (let index = 0; index < triangleCount; index += 1) {
            const sourceFace = mesh.sourceFaces[index];
            assertUint32(sourceFace, "source face");
            hasSourceFaces ||= sourceFace !== UNSPECIFIED_SOURCE_FACE;
        }
    }
    const positionsOffset = HEADER_BYTES;
    const trianglesOffset = positionsOffset + vertexCount * 24;
    const trianglesEnd = trianglesOffset + triangleCount * 12;
    const sourceFacesOffset = hasSourceFaces ? alignEight(trianglesEnd) : 0;
    const payloadEnd = hasSourceFaces ? sourceFacesOffset + triangleCount * 4 : trianglesEnd;
    const packetBytes = alignEight(payloadEnd);
    if (packetBytes > MAX_PACKET_BYTES)
        fail("indexed-mesh packet exceeds 268 MiB.");
    const output = new Uint8Array(packetBytes);
    output.set(MAGIC, 0);
    const view = new DataView(output.buffer);
    view.setUint16(8, 1, true);
    view.setUint16(10, HEADER_BYTES, true);
    view.setUint32(12, hasSourceFaces ? HAS_SOURCE_FACES : 0, true);
    view.setBigUint64(16, BigInt(packetBytes), true);
    view.setUint32(24, vertexCount, true);
    view.setUint32(28, triangleCount, true);
    view.setBigUint64(32, BigInt(positionsOffset), true);
    view.setBigUint64(40, BigInt(trianglesOffset), true);
    view.setBigUint64(48, BigInt(sourceFacesOffset), true);
    for (let index = 0; index < mesh.positions.length; index += 1) {
        const coordinate = mesh.positions[index];
        if (coordinate === undefined || !Number.isFinite(coordinate)) {
            fail("positions must contain only finite numbers.");
        }
        view.setFloat64(positionsOffset + index * 8, coordinate, true);
    }
    for (let index = 0; index < mesh.indices.length; index += 1) {
        const vertex = mesh.indices[index];
        assertUint32(vertex, "triangle index");
        if (vertex >= vertexCount)
            fail("triangle index is outside the vertex table.");
        view.setUint32(trianglesOffset + index * 4, vertex, true);
    }
    for (let triangle = 0; triangle < triangleCount; triangle += 1) {
        const begin = triangle * 3;
        const first = mesh.indices[begin];
        const second = mesh.indices[begin + 1];
        const third = mesh.indices[begin + 2];
        if (first === second || second === third || third === first) {
            fail("each triangle must reference three distinct vertex indices.");
        }
        if (hasSourceFaces) {
            view.setUint32(sourceFacesOffset + triangle * 4, mesh.sourceFaces?.[triangle] ?? UNSPECIFIED_SOURCE_FACE, true);
        }
    }
    return output;
}
export function decodeIndexedTriangleMeshA0Packet(packet) {
    if (packet.byteLength < HEADER_BYTES || packet.byteLength > MAX_PACKET_BYTES) {
        fail("indexed-mesh packet length is outside its governed bounds.");
    }
    for (let index = 0; index < MAGIC.length; index += 1) {
        if (packet[index] !== MAGIC[index])
            fail("indexed-mesh packet magic is invalid.");
    }
    const view = new DataView(packet.buffer, packet.byteOffset, packet.byteLength);
    const flags = view.getUint32(12, true);
    const vertexCount = view.getUint32(24, true);
    const triangleCount = view.getUint32(28, true);
    const positionsOffset = exactOffset(view.getBigUint64(32, true));
    const trianglesOffset = exactOffset(view.getBigUint64(40, true));
    const sourceFacesOffset = exactOffset(view.getBigUint64(48, true));
    const hasSourceFaces = (flags & HAS_SOURCE_FACES) !== 0;
    const trianglesEnd = trianglesOffset + triangleCount * 12;
    const payloadEnd = hasSourceFaces ? sourceFacesOffset + triangleCount * 4 : trianglesEnd;
    if (view.getUint16(8, true) !== 1 ||
        view.getUint16(10, true) !== HEADER_BYTES ||
        (flags & ~HAS_SOURCE_FACES) !== 0 ||
        exactOffset(view.getBigUint64(16, true)) !== packet.byteLength ||
        vertexCount === 0 ||
        vertexCount > MAX_VERTICES ||
        triangleCount === 0 ||
        triangleCount > MAX_TRIANGLES ||
        positionsOffset !== HEADER_BYTES ||
        trianglesOffset !== positionsOffset + vertexCount * 24 ||
        sourceFacesOffset !== (hasSourceFaces ? alignEight(trianglesEnd) : 0) ||
        alignEight(payloadEnd) !== packet.byteLength ||
        view.getBigUint64(56, true) !== 0n) {
        fail("indexed-mesh packet header or table layout is invalid.");
    }
    assertZeroRange(packet, trianglesEnd, hasSourceFaces ? sourceFacesOffset : packet.byteLength);
    assertZeroRange(packet, payloadEnd, packet.byteLength);
    const positions = new Float64Array(vertexCount * 3);
    for (let index = 0; index < positions.length; index += 1) {
        const coordinate = view.getFloat64(positionsOffset + index * 8, true);
        if (!Number.isFinite(coordinate))
            fail("indexed-mesh position is non-finite.");
        positions[index] = coordinate;
    }
    const indices = new Uint32Array(triangleCount * 3);
    for (let index = 0; index < indices.length; index += 1) {
        const vertex = view.getUint32(trianglesOffset + index * 4, true);
        if (vertex >= vertexCount)
            fail("triangle index is outside the vertex table.");
        indices[index] = vertex;
    }
    for (let triangle = 0; triangle < triangleCount; triangle += 1) {
        const begin = triangle * 3;
        if (indices[begin] === indices[begin + 1] ||
            indices[begin + 1] === indices[begin + 2] ||
            indices[begin + 2] === indices[begin]) {
            fail("triangle contains repeated vertex indices.");
        }
    }
    let sourceFaces;
    if (hasSourceFaces) {
        sourceFaces = new Uint32Array(triangleCount);
        for (let index = 0; index < triangleCount; index += 1) {
            sourceFaces[index] = view.getUint32(sourceFacesOffset + index * 4, true);
        }
    }
    return sourceFaces === undefined ? { indices, positions } : { indices, positions, sourceFaces };
}
function alignEight(value) {
    return Math.ceil(value / 8) * 8;
}
function assertUint32(value, label) {
    if (value === undefined || !Number.isInteger(value) || value < 0 || value > 0xffff_ffff) {
        fail(`${label} must be an unsigned 32-bit integer.`);
    }
}
function exactOffset(value) {
    if (value > BigInt(Number.MAX_SAFE_INTEGER))
        fail("packet offset exceeds JavaScript range.");
    return Number(value);
}
function assertZeroRange(packet, begin, end) {
    if (begin < 0 || end < begin || end > packet.byteLength)
        fail("packet range is invalid.");
    for (let index = begin; index < end; index += 1) {
        if (packet[index] !== 0)
            fail("packet alignment or reserved bytes must be zero.");
    }
}
function fail(message) {
    throw new IndexedTriangleMeshPacketError(message);
}
