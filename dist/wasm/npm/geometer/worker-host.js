import { encodeOperationOutcomeA0Json } from "./generated/index.js";
import { createGeometerWasmClient, GeometerOperationError, GeometerWasmTransportError, } from "./wasm.js";
import { GEOMETER_WASM_WORKER_PROTOCOL } from "./worker.js";
/** Installs the generic Geometer A0 protocol into a dedicated Worker scope. */
export function startGeometerWorkerHost(factory, scope, options = {}) {
    let clientPromise;
    let operationQueue = Promise.resolve();
    let shutdownQueued = false;
    scope.addEventListener("message", (event) => {
        const request = event.data;
        if (!isWorkerRequest(request)) {
            if (isRecord(request) &&
                request.protocol === GEOMETER_WASM_WORKER_PROTOCOL &&
                typeof request.requestId === "string") {
                postError(request.requestId, new Error("Malformed Geometer Worker request."));
            }
            return;
        }
        if (request.kind === "initialize") {
            void initialize(request);
            return;
        }
        if (shutdownQueued) {
            postError(request.requestId, new Error("Geometer Worker shutdown is already queued."));
            return;
        }
        shutdownQueued = request.kind === "shutdown";
        operationQueue = operationQueue.then(() => dispatchSerialized(request), () => dispatchSerialized(request));
    });
    async function initialize(request) {
        if (clientPromise) {
            postError(request.requestId, new Error("Geometer Worker is already initialized."));
            return;
        }
        clientPromise = createGeometerWasmClient(factory, {
            ...(options.moduleOptions ?? {}),
            ...request.moduleOptions,
            wasmBinary: request.wasmBinary,
        });
        try {
            const client = await clientPromise;
            scope.postMessage({
                capabilities: client.capabilities,
                kind: "ready",
                protocol: GEOMETER_WASM_WORKER_PROTOCOL,
                requestId: request.requestId,
            });
        }
        catch (error) {
            clientPromise = undefined;
            postError(request.requestId, error);
        }
    }
    async function dispatchSerialized(request) {
        try {
            if (request.kind === "shutdown") {
                scope.postMessage({
                    kind: "closed",
                    protocol: GEOMETER_WASM_WORKER_PROTOCOL,
                    requestId: request.requestId,
                });
                scope.close?.();
                return;
            }
            if (!clientPromise)
                throw new Error("Geometer Worker is not initialized.");
            const client = await clientPromise;
            const response = client.execute(request.operation, request.requestJson, request.attachments.map((attachment) => ({
                data: new Uint8Array(attachment.data),
                mediaType: attachment.mediaType,
                name: attachment.name,
            })));
            const attachments = response.attachments.map(toTransferAttachment);
            scope.postMessage({
                attachments,
                kind: "operation_result",
                outcomeJson: encodeOperationOutcomeA0Json(response.outcome),
                protocol: GEOMETER_WASM_WORKER_PROTOCOL,
                requestId: request.requestId,
            }, attachments.map((attachment) => attachment.data));
        }
        catch (error) {
            postError(request.requestId, error);
        }
    }
    function postError(requestId, error) {
        scope.postMessage({
            error: serializeError(error),
            kind: "error",
            protocol: GEOMETER_WASM_WORKER_PROTOCOL,
            requestId,
        });
    }
}
function isWorkerRequest(value) {
    if (!isRecord(value) || value.protocol !== GEOMETER_WASM_WORKER_PROTOCOL)
        return false;
    if (typeof value.requestId !== "string" || typeof value.kind !== "string")
        return false;
    if (value.kind === "shutdown")
        return true;
    if (value.kind === "initialize") {
        return isRecord(value.moduleOptions) && value.wasmBinary instanceof ArrayBuffer;
    }
    if (value.kind === "execute") {
        return (typeof value.operation === "string" &&
            typeof value.requestJson === "string" &&
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
function toTransferAttachment(attachment) {
    if (attachment.data.buffer instanceof ArrayBuffer &&
        attachment.data.byteOffset === 0 &&
        attachment.data.byteLength === attachment.data.buffer.byteLength) {
        return {
            data: attachment.data.buffer,
            mediaType: attachment.mediaType,
            name: attachment.name,
        };
    }
    const copy = new Uint8Array(attachment.data.byteLength);
    copy.set(attachment.data);
    return { data: copy.buffer, mediaType: attachment.mediaType, name: attachment.name };
}
function serializeError(error) {
    if (error instanceof GeometerWasmTransportError) {
        return {
            code: error.code,
            message: error.message,
            name: error.name,
            ...(error.stack ? { stack: error.stack } : {}),
        };
    }
    if (error instanceof GeometerOperationError) {
        return {
            diagnostics: error.diagnostics,
            message: error.message,
            name: error.name,
            operation: error.operation,
            ...(error.stack ? { stack: error.stack } : {}),
        };
    }
    if (error instanceof Error) {
        return {
            message: error.message,
            name: error.name,
            ...(error.stack ? { stack: error.stack } : {}),
        };
    }
    return { message: String(error), name: "Error" };
}
function isRecord(value) {
    return typeof value === "object" && value !== null && !Array.isArray(value);
}
