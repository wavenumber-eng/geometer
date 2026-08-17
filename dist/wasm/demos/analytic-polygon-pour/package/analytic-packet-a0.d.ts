import type { AnalyticPlanarBooleanBatchRequestA0, AnalyticPlanarBooleanBatchResultA0 } from "./generated/contracts.js";
export declare class AnalyticPacketError extends Error {
    constructor(message: string);
}
/** Encode the frozen canonical GMABRQ01 projection using bigint IDs and nanometer coordinates. */
export declare function encodeAnalyticPlanarBooleanBatchRequestA0Packet(request: AnalyticPlanarBooleanBatchRequestA0): Uint8Array;
/** Strictly decode GMABRS01 and project it to the public logical bigint DTO. */
export declare function decodeAnalyticPlanarBooleanBatchResultA0Packet(bytes: Uint8Array): Promise<AnalyticPlanarBooleanBatchResultA0>;
