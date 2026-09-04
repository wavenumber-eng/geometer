export declare const INDEXED_TRIANGLE_MESH_MEDIA_TYPE: "application/vnd.wavenumber.geometer.indexed-triangle-mesh";
export declare const INDEXED_TRIANGLE_MESH_PACKET_FORMAT: "geometry.indexed_triangle_mesh.packet.a0";
export interface IndexedTriangleMeshA0 {
    /** Flat xyz coordinates in millimeters. */
    readonly positions: ArrayLike<number>;
    /** Flat triangle vertex indices. */
    readonly indices: ArrayLike<number>;
    /** Optional source CAD-face identity per triangle. */
    readonly sourceFaces?: ArrayLike<number>;
}
export interface DecodedIndexedTriangleMeshA0 {
    readonly indices: Uint32Array;
    readonly positions: Float64Array;
    readonly sourceFaces?: Uint32Array;
}
export declare class IndexedTriangleMeshPacketError extends Error {
    constructor(message: string);
}
export declare function encodeIndexedTriangleMeshA0Packet(mesh: IndexedTriangleMeshA0): Uint8Array;
export declare function decodeIndexedTriangleMeshA0Packet(packet: Uint8Array): DecodedIndexedTriangleMeshA0;
