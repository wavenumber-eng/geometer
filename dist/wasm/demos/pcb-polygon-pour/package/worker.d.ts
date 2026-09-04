import type { AnalyticPlanarBooleanBatchRequestA0, AnalyticPlanarBooleanBatchResultA0, DiagnosticA0, HlrProjectionResultA0, ModelBoundsResultA0 } from "./generated/index.js";
import type { GeometerOperationAttachment, GeometerOperationResponse, GeometerWasmCapabilityCatalog, MeshHlrProjectionRequest, ModelBoundsRequest, ModelHlrProjectionRequest } from "./wasm.js";
export declare const GEOMETER_WASM_WORKER_PROTOCOL: "wn.geometer.wasm_worker.a0";
export interface GeometerWorkerClientOptions {
    /** Structured-cloneable Emscripten options other than wasmBinary. */
    readonly moduleOptions?: Readonly<Record<string, unknown>>;
    /** Browser WASM bytes. The client preserves the caller's buffer and transfers an owned copy. */
    readonly wasmBinary: ArrayBuffer | Uint8Array;
}
export interface GeometerWorkerAttachmentMessage {
    readonly data: ArrayBuffer;
    readonly mediaType: string;
    readonly name: string;
}
export type GeometerWorkerRequestMessage = {
    readonly kind: "initialize";
    readonly moduleOptions: Readonly<Record<string, unknown>>;
    readonly protocol: typeof GEOMETER_WASM_WORKER_PROTOCOL;
    readonly requestId: string;
    readonly wasmBinary: ArrayBuffer;
} | {
    readonly attachments: readonly GeometerWorkerAttachmentMessage[];
    readonly kind: "execute";
    readonly operation: string;
    readonly protocol: typeof GEOMETER_WASM_WORKER_PROTOCOL;
    readonly requestId: string;
    readonly requestJson: string;
} | {
    readonly kind: "shutdown";
    readonly protocol: typeof GEOMETER_WASM_WORKER_PROTOCOL;
    readonly requestId: string;
};
export interface GeometerWorkerSerializedError {
    readonly code?: number;
    readonly diagnostics?: readonly DiagnosticA0[];
    readonly message: string;
    readonly name: string;
    readonly operation?: string;
    readonly stack?: string;
}
export type GeometerWorkerResponseMessage = {
    readonly capabilities: GeometerWasmCapabilityCatalog;
    readonly kind: "ready";
    readonly protocol: typeof GEOMETER_WASM_WORKER_PROTOCOL;
    readonly requestId: string;
} | {
    readonly attachments: readonly GeometerWorkerAttachmentMessage[];
    readonly kind: "operation_result";
    readonly outcomeJson: string;
    readonly protocol: typeof GEOMETER_WASM_WORKER_PROTOCOL;
    readonly requestId: string;
} | {
    readonly error: GeometerWorkerSerializedError;
    readonly kind: "error";
    readonly protocol: typeof GEOMETER_WASM_WORKER_PROTOCOL;
    readonly requestId: string;
} | {
    readonly kind: "closed";
    readonly protocol: typeof GEOMETER_WASM_WORKER_PROTOCOL;
    readonly requestId: string;
};
export declare class GeometerWorkerError extends Error {
    constructor(message: string);
}
export declare class GeometerWorkerClient {
    readonly capabilities: GeometerWasmCapabilityCatalog;
    private readonly connection;
    private constructor();
    static create(worker: Worker, options: GeometerWorkerClientOptions): Promise<GeometerWorkerClient>;
    analyticPlanarBooleanBatch(request: AnalyticPlanarBooleanBatchRequestA0): Promise<AnalyticPlanarBooleanBatchResultA0>;
    modelBounds(request: ModelBoundsRequest): Promise<ModelBoundsResultA0>;
    modelHlrProjection(request: ModelHlrProjectionRequest): Promise<HlrProjectionResultA0>;
    meshHlrProjection(request: MeshHlrProjectionRequest): Promise<HlrProjectionResultA0>;
    private hlrProjection;
    execute(operation: string, requestJson: string, attachments: readonly GeometerOperationAttachment[]): Promise<GeometerOperationResponse>;
    /** Gracefully shuts down the host and terminates the underlying Worker. */
    close(): Promise<void>;
    /** Immediately terminates the Worker and rejects all outstanding requests. */
    terminate(): void;
}
export declare function createGeometerWorkerClient(worker: Worker, options: GeometerWorkerClientOptions): Promise<GeometerWorkerClient>;
