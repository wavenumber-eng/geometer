import type { HlrProjectionOptionsA0, HlrProjectionResultA0, IpcRequestValueA0, IpcShutdownAckA0, IpcWelcomeA0, OperationOutcomeA0 } from "./generated/contracts.js";
import { type OperationIdentity } from "./generated/operations.js";
import { type IndexedTriangleMeshA0 } from "./indexed-mesh-packet-a0.js";
import { type GeometerIpcAttachment } from "./ipc-a0.js";
export interface GeometerIpcDuplexA0 {
    readonly readable: ReadableStream<Uint8Array>;
    readonly writable: WritableStream<Uint8Array>;
    /** Immediately terminates the underlying worker or process. */
    readonly terminate: (reason: Error) => void;
}
export interface GeometerIpcConnectOptionsA0 {
    readonly clientName: string;
    readonly clientVersion: string;
    readonly capabilities?: readonly string[];
    /** Runtime catalog expected from the peer. Defaults to the portable C ABI set. */
    readonly runtimeTarget?: "portable" | "native";
}
export interface GeometerIpcOperationResponseA0 {
    readonly requestId: bigint;
    readonly outcome: OperationOutcomeA0;
    readonly attachments: readonly GeometerIpcAttachment[];
}
export interface GeometerIpcModelHlrProjectionRequestA0 {
    readonly mediaType?: "application/step" | "model/step";
    readonly model: Uint8Array;
    readonly options?: HlrProjectionOptionsA0;
}
export interface GeometerIpcMeshHlrProjectionRequestA0 {
    readonly mesh: IndexedTriangleMeshA0 | Uint8Array;
    readonly options?: HlrProjectionOptionsA0;
}
export interface GeometerIpcCallA0 {
    readonly requestId: bigint;
    readonly response: Promise<GeometerIpcOperationResponseA0>;
    cancel(reason?: string): Promise<"cancelled" | "rejected">;
}
export declare class GeometerIpcClientError extends Error {
    constructor(message: string);
}
export declare class GeometerIpcCancelledError extends GeometerIpcClientError {
    readonly requestId: bigint;
    constructor(requestId: bigint);
}
export declare class GeometerIpcClientA0 {
    private readonly duplex;
    private readonly runtimeTarget;
    private readonly decoder;
    private readonly pending;
    private readonly cancellation;
    private readonly reader;
    private readonly writer;
    private readonly welcomeReady;
    private shutdownReady?;
    private writeTail;
    private transportTerminated;
    private pendingResidentBytes;
    private state;
    private nextRequestId;
    private selectedWelcome?;
    private constructor();
    static connect(duplex: GeometerIpcDuplexA0, options: GeometerIpcConnectOptionsA0): Promise<GeometerIpcClientA0>;
    get welcome(): IpcWelcomeA0;
    start(operation: OperationIdentity, request: IpcRequestValueA0, attachments?: readonly GeometerIpcAttachment[]): GeometerIpcCallA0;
    execute(operation: OperationIdentity, request: IpcRequestValueA0, attachments?: readonly GeometerIpcAttachment[]): Promise<GeometerIpcOperationResponseA0>;
    modelHlrProjection(request: GeometerIpcModelHlrProjectionRequestA0): Promise<HlrProjectionResultA0>;
    meshHlrProjection(request: GeometerIpcMeshHlrProjectionRequestA0): Promise<HlrProjectionResultA0>;
    private hlrProjection;
    close(reason?: string): Promise<IpcShutdownAckA0>;
    terminate(reason?: GeometerIpcClientError): void;
    private cancel;
    private allocateRequestId;
    private writeFrame;
    private encodeEffectiveFrame;
    private writeBytes;
    private readLoop;
    private acceptFrame;
    private acceptResponse;
    private fail;
    private abort;
    private reservePending;
    private releasePending;
}
