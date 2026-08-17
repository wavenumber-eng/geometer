import { decodeOperationOutcomeA0Json, encodeModelBoundsOptionsA0Json } from "./generated/index.js";
import { GeometerOperationError, GeometerWasmTransportError } from "./wasm.js";
export const GEOMETER_WASM_WORKER_PROTOCOL = "wn.geometer.wasm_worker.a0";
export class GeometerWorkerError extends Error {
    constructor(message) {
        super(message);
        this.name = "GeometerWorkerError";
    }
}
export class GeometerWorkerClient {
    capabilities;
    connection;
    constructor(connection, capabilities) {
        this.connection = connection;
        this.capabilities = capabilities;
    }
    static async create(worker, options) {
        const connection = new WorkerConnection(worker);
        try {
            const wasmBinary = copyToArrayBuffer(options.wasmBinary);
            const response = await connection.request({
                kind: "initialize",
                moduleOptions: options.moduleOptions ?? {},
                wasmBinary,
            }, [wasmBinary]);
            if (response.kind !== "ready" || !isCapabilityCatalog(response.capabilities)) {
                throw new GeometerWorkerError("Geometer Worker returned an invalid capability handshake.");
            }
            return new GeometerWorkerClient(connection, freezeCapabilities(response.capabilities));
        }
        catch (error) {
            connection.terminate(error instanceof Error ? error : new GeometerWorkerError(String(error)));
            throw error;
        }
    }
    async modelBounds(request) {
        const response = await this.execute("geometry.model_bounds.a0", encodeModelBoundsOptionsA0Json(request.options ?? {}), [
            {
                name: "model",
                mediaType: request.mediaType ?? "application/step",
                data: request.model,
            },
        ]);
        if (!response.outcome.ok) {
            throw new GeometerOperationError(response.outcome.operation, response.outcome.diagnostics);
        }
        if (response.outcome.operation !== "geometry.model_bounds.a0") {
            throw new GeometerWorkerError(`Expected geometry.model_bounds.a0 response, received ${response.outcome.operation}.`);
        }
        if (response.attachments.length !== 0) {
            throw new GeometerWorkerError("model_bounds returned unexpected attachments.");
        }
        if (!("units" in response.outcome.result)) {
            throw new GeometerWorkerError("model_bounds returned an incompatible result DTO.");
        }
        return response.outcome.result;
    }
    async execute(operation, requestJson, attachments) {
        const transferred = attachments.map((attachment) => ({
            data: copyToArrayBuffer(attachment.data),
            mediaType: attachment.mediaType,
            name: attachment.name,
        }));
        const response = await this.connection.request({
            attachments: transferred,
            kind: "execute",
            operation,
            requestJson,
        }, transferred.map((attachment) => attachment.data));
        if (response.kind !== "operation_result") {
            throw new GeometerWorkerError(`Expected operation_result, received ${response.kind}.`);
        }
        const outcome = decodeOperationOutcomeA0Json(response.outcomeJson);
        return {
            attachments: response.attachments.map((attachment) => ({
                data: new Uint8Array(attachment.data),
                mediaType: attachment.mediaType,
                name: attachment.name,
            })),
            outcome,
        };
    }
    /** Gracefully shuts down the host and terminates the underlying Worker. */
    async close() {
        await this.connection.close();
    }
    /** Immediately terminates the Worker and rejects all outstanding requests. */
    terminate() {
        this.connection.terminate(new GeometerWorkerError("Geometer Worker was terminated."));
    }
}
export async function createGeometerWorkerClient(worker, options) {
    return GeometerWorkerClient.create(worker, options);
}
class WorkerConnection {
    closed = false;
    closing = false;
    closePromise;
    nextRequestId = 1;
    pending = new Map();
    worker;
    constructor(worker) {
        this.worker = worker;
        worker.addEventListener("message", this.handleMessage);
        worker.addEventListener("error", this.handleError);
        worker.addEventListener("messageerror", this.handleMessageError);
    }
    request(body, transfer, allowWhileClosing = false) {
        if (this.closed || (this.closing && !allowWhileClosing)) {
            return Promise.reject(new GeometerWorkerError("Geometer Worker is closed."));
        }
        const requestId = `geometer-worker-${this.nextRequestId}`;
        this.nextRequestId += 1;
        const message = {
            ...body,
            protocol: GEOMETER_WASM_WORKER_PROTOCOL,
            requestId,
        };
        return new Promise((resolve, reject) => {
            this.pending.set(requestId, {
                expectedKind: expectedResponseKind(body.kind),
                reject,
                resolve,
            });
            try {
                this.worker.postMessage(message, [...transfer]);
            }
            catch (error) {
                this.pending.delete(requestId);
                reject(error instanceof Error ? error : new GeometerWorkerError(String(error)));
            }
        });
    }
    close() {
        if (this.closed)
            return Promise.resolve();
        if (this.closePromise)
            return this.closePromise;
        this.closing = true;
        this.closePromise = this.finishClose();
        return this.closePromise;
    }
    async finishClose() {
        try {
            const response = await this.request({ kind: "shutdown" }, [], true);
            if (response.kind !== "closed") {
                throw new GeometerWorkerError(`Expected closed, received ${response.kind}.`);
            }
        }
        finally {
            this.terminate(new GeometerWorkerError("Geometer Worker is closed."));
        }
    }
    terminate(error) {
        if (this.closed)
            return;
        this.closed = true;
        this.worker.removeEventListener("message", this.handleMessage);
        this.worker.removeEventListener("error", this.handleError);
        this.worker.removeEventListener("messageerror", this.handleMessageError);
        this.worker.terminate();
        for (const pending of this.pending.values())
            pending.reject(error);
        this.pending.clear();
    }
    handleMessage = (event) => {
        if (!isWorkerResponse(event.data)) {
            this.terminate(new GeometerWorkerError("Geometer Worker returned a malformed message."));
            return;
        }
        const response = event.data;
        const pending = this.pending.get(response.requestId);
        if (!pending) {
            this.terminate(new GeometerWorkerError(`Geometer Worker returned unknown or completed request ID ${response.requestId}.`));
            return;
        }
        if (response.kind !== "error" && response.kind !== pending.expectedKind) {
            this.terminate(new GeometerWorkerError(`Geometer Worker returned ${response.kind} for ${response.requestId}; expected ${pending.expectedKind}.`));
            return;
        }
        this.pending.delete(response.requestId);
        if (response.kind === "error")
            pending.reject(deserializeError(response.error));
        else
            pending.resolve(response);
    };
    handleError = (event) => {
        this.terminate(new GeometerWorkerError(event.message || "Geometer Worker exited unexpectedly."));
    };
    handleMessageError = () => {
        this.terminate(new GeometerWorkerError("Geometer Worker message deserialization failed."));
    };
}
function expectedResponseKind(requestKind) {
    if (requestKind === "initialize")
        return "ready";
    if (requestKind === "execute")
        return "operation_result";
    return "closed";
}
function isWorkerResponse(value) {
    if (!isRecord(value) || value.protocol !== GEOMETER_WASM_WORKER_PROTOCOL)
        return false;
    if (typeof value.requestId !== "string" || typeof value.kind !== "string")
        return false;
    if (value.kind === "ready")
        return isCapabilityCatalog(value.capabilities);
    if (value.kind === "closed")
        return true;
    if (value.kind === "error") {
        return (isRecord(value.error) &&
            typeof value.error.name === "string" &&
            typeof value.error.message === "string");
    }
    if (value.kind === "operation_result") {
        return (typeof value.outcomeJson === "string" &&
            Array.isArray(value.attachments) &&
            value.attachments.every(isAttachmentMessage));
    }
    return false;
}
function isAttachmentMessage(value) {
    return (isRecord(value) &&
        value.data instanceof ArrayBuffer &&
        typeof value.mediaType === "string" &&
        typeof value.name === "string");
}
function isCapabilityCatalog(value) {
    return (isRecord(value) &&
        Number.isSafeInteger(value.cAbiGeneration) &&
        value.genericAbi === "a0" &&
        typeof value.releaseVersion === "string" &&
        Array.isArray(value.operations) &&
        value.operations.every((operation) => typeof operation === "string"));
}
function freezeCapabilities(value) {
    return Object.freeze({
        cAbiGeneration: value.cAbiGeneration,
        genericAbi: value.genericAbi,
        operations: Object.freeze([...value.operations]),
        releaseVersion: value.releaseVersion,
    });
}
function deserializeError(error) {
    if (error.name === "GeometerWasmTransportError" && typeof error.code === "number") {
        return new GeometerWasmTransportError(error.code, error.message);
    }
    if (error.name === "GeometerOperationError" &&
        typeof error.operation === "string" &&
        Array.isArray(error.diagnostics)) {
        return new GeometerOperationError(error.operation, error.diagnostics);
    }
    const result = new GeometerWorkerError(error.message);
    if (error.stack)
        result.stack = error.stack;
    return result;
}
function copyToArrayBuffer(value) {
    const view = value instanceof Uint8Array ? value : new Uint8Array(value);
    const copy = new Uint8Array(view.byteLength);
    copy.set(view);
    return copy.buffer;
}
function isRecord(value) {
    return typeof value === "object" && value !== null && !Array.isArray(value);
}
