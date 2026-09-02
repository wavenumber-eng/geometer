export declare const GEOMETER_IPC_A0_LIMITS: Readonly<{
    attachmentBytes: number;
    attachmentCount: 16;
    attachmentTextBytes: 128;
    frameBytes: number;
    jsonBytes: number;
    pendingWriterBytes: number;
    queuedBytes: number;
    queuedRequests: 8;
    residentRequestBytes: number;
}>;
export type GeometerIpcFrameKind = 1 | 2 | 3 | 4 | 5 | 6 | 7 | 8 | 9 | 10;
export interface GeometerIpcAttachment {
    readonly name: string;
    readonly mediaType: string;
    readonly data: Uint8Array;
}
export interface GeometerIpcFrame {
    readonly kind: GeometerIpcFrameKind;
    readonly requestId: bigint;
    readonly json: string;
    readonly attachments: readonly GeometerIpcAttachment[];
}
export interface GeometerIpcFrameDecodeLimits {
    readonly attachmentBytes: number;
    readonly attachmentCount: number;
    readonly attachmentMediaTypeBytes: number;
    readonly attachmentNameBytes: number;
    readonly frameBytes: number;
    readonly jsonBytes: number;
}
export declare class GeometerIpcProtocolError extends Error {
    constructor(message: string);
}
export declare function validateIpcRequestOperationPair(envelope: IpcRequestA0): void;
export declare function validateIpcOutcomeOperationPair(outcome: OperationOutcomeA0): void;
export declare function encodeGeometerIpcFrame(frame: GeometerIpcFrame): Uint8Array;
export declare class GeometerIpcFrameDecoder {
    private readonly pending;
    private limits;
    /**
     * Transfers ownership of `chunk` to the decoder. The caller must not mutate or detach it after
     * this call. Decoded attachment views may retain the transferred storage without another copy.
     */
    push(chunk: Uint8Array): readonly GeometerIpcFrame[];
    /** Decodes at most one frame so negotiated limits can change between coalesced frames. */
    pushOne(chunk?: Uint8Array): GeometerIpcFrame | undefined;
    setLimits(limits: GeometerIpcFrameDecodeLimits): void;
    finish(): void;
}
import type { IpcRequestA0, OperationOutcomeA0 } from "./generated/contracts.js";
