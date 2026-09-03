import { decodeIpcCancelledA0Json, decodeIpcCancelRejectedA0Json, decodeIpcProtocolErrorA0Json, decodeIpcShutdownAckA0Json, decodeIpcWelcomeA0Json, decodeOperationOutcomeA0Json, encodeIpcHelloA0Json, encodeIpcReasonA0Json, encodeIpcRequestA0Json, } from "./generated/codecs.js";
import { NORMALIZED_CONTRACT_CATALOG_SHA256, operationCatalog, } from "./generated/operations.js";
import { encodeIndexedTriangleMeshA0Packet, INDEXED_TRIANGLE_MESH_MEDIA_TYPE, } from "./indexed-mesh-packet-a0.js";
import { encodeGeometerIpcFrame, GEOMETER_IPC_A0_LIMITS, GeometerIpcFrameDecoder, GeometerIpcProtocolError, validateIpcOutcomeOperationPair, validateIpcRequestOperationPair, } from "./ipc-a0.js";
const requiredCapabilities = [
    "serialized_execution",
    "queue_only_cancellation",
    "raw_attachments",
];
const textEncoder = new TextEncoder();
export class GeometerIpcClientError extends Error {
    constructor(message) {
        super(message);
        this.name = "GeometerIpcClientError";
    }
}
export class GeometerIpcCancelledError extends GeometerIpcClientError {
    requestId;
    constructor(requestId) {
        super(`Geometer IPC request ${requestId} was cancelled.`);
        this.requestId = requestId;
        this.name = "GeometerIpcCancelledError";
    }
}
export class GeometerIpcClientA0 {
    duplex;
    runtimeTarget;
    decoder = new GeometerIpcFrameDecoder();
    pending = new Map();
    cancellation = new Map();
    reader;
    writer;
    welcomeReady = deferred();
    shutdownReady;
    writeTail = Promise.resolve();
    transportTerminated = false;
    pendingResidentBytes = 0;
    state = "awaiting_welcome";
    nextRequestId = 1n;
    selectedWelcome;
    constructor(duplex, runtimeTarget) {
        this.duplex = duplex;
        this.runtimeTarget = runtimeTarget;
        this.reader = duplex.readable.getReader();
        this.writer = duplex.writable.getWriter();
        void this.readLoop();
    }
    static async connect(duplex, options) {
        const client = new GeometerIpcClientA0(duplex, options.runtimeTarget ?? "portable");
        try {
            await Promise.all([
                client.writeFrame({
                    attachments: [],
                    json: encodeIpcHelloA0Json({
                        ...(options.capabilities === undefined ? {} : { capabilities: options.capabilities }),
                        client_name: options.clientName,
                        client_version: options.clientVersion,
                        protocols: ["a0"],
                    }),
                    kind: 1,
                    requestId: 0n,
                }),
                client.welcomeReady.promise,
            ]);
            return client;
        }
        catch (error) {
            client.terminate(asError(error));
            throw error;
        }
    }
    get welcome() {
        if (this.selectedWelcome === undefined) {
            throw new GeometerIpcClientError("Geometer IPC handshake is incomplete.");
        }
        return this.selectedWelcome;
    }
    start(operation, request, attachments = []) {
        if (this.state !== "running") {
            throw new GeometerIpcClientError("Geometer IPC connection is not accepting requests.");
        }
        const declaration = negotiatedOperation(this.welcome, operation);
        const envelope = { operation, request };
        validateIpcRequestOperationPair(envelope);
        validateAttachments(attachments, declaration.input_attachments, "request");
        const requestId = this.allocateRequestId();
        const frame = {
            attachments,
            json: encodeIpcRequestA0Json(envelope),
            kind: 3,
            requestId,
        };
        const bytes = this.encodeEffectiveFrame(frame);
        this.reservePending(bytes.byteLength);
        const response = deferred();
        this.pending.set(requestId, { operation, residentBytes: bytes.byteLength, response });
        void this.writeBytes(bytes).catch((error) => this.abort(asError(error)));
        return {
            cancel: (reason) => this.cancel(requestId, reason),
            requestId,
            response: response.promise,
        };
    }
    async execute(operation, request, attachments = []) {
        return this.start(operation, request, attachments).response;
    }
    async modelHlrProjection(request) {
        return this.hlrProjection("geometry.model_hlr_projection.a0", "model", request.mediaType ?? "application/step", request.model, request.options);
    }
    async meshHlrProjection(request) {
        const packet = request.mesh instanceof Uint8Array
            ? request.mesh
            : encodeIndexedTriangleMeshA0Packet(request.mesh);
        return this.hlrProjection("geometry.mesh_hlr_projection.a0", "mesh", INDEXED_TRIANGLE_MESH_MEDIA_TYPE, packet, request.options);
    }
    async hlrProjection(operation, attachmentName, mediaType, data, options = {}) {
        const response = await this.execute(operation, { ...options, output_detail: options.output_detail ?? true }, [{ data, mediaType, name: attachmentName }]);
        if (!response.outcome.ok) {
            throw new GeometerIpcClientError(response.outcome.diagnostics.map((item) => item.message).join("; ") ||
                `${operation} failed.`);
        }
        if (response.outcome.operation !== operation ||
            !("views" in response.outcome.result) ||
            response.attachments.length !== 0) {
            throw new GeometerIpcProtocolError(`${operation} returned an incompatible result.`);
        }
        return response.outcome.result;
    }
    async close(reason) {
        if (this.state === "closed") {
            throw new GeometerIpcClientError("Geometer IPC connection is closed.");
        }
        if (this.state === "draining" && this.shutdownReady !== undefined) {
            return this.shutdownReady.promise;
        }
        if (this.state !== "running") {
            throw new GeometerIpcClientError("Geometer IPC handshake is incomplete.");
        }
        const frame = {
            attachments: [],
            json: encodeIpcReasonA0Json(reason === undefined ? {} : { reason }),
            kind: 8,
            requestId: 0n,
        };
        const bytes = this.encodeEffectiveFrame(frame);
        this.state = "draining";
        this.shutdownReady = deferred();
        try {
            const [, acknowledgment] = await Promise.all([
                this.writeBytes(bytes),
                this.shutdownReady.promise,
            ]);
            return acknowledgment;
        }
        catch (error) {
            this.abort(asError(error));
            throw error;
        }
    }
    terminate(reason = new GeometerIpcClientError("Geometer IPC connection was terminated.")) {
        if (!this.transportTerminated) {
            this.transportTerminated = true;
            try {
                this.duplex.terminate(reason);
            }
            finally {
                this.fail(reason);
            }
        }
        else {
            this.fail(reason);
        }
    }
    async cancel(requestId, reason) {
        if (this.state !== "running") {
            throw new GeometerIpcClientError("Geometer IPC connection is not accepting cancellation.");
        }
        if (!this.pending.has(requestId)) {
            throw new GeometerIpcClientError(`Geometer IPC request ${requestId} is not pending.`);
        }
        if (this.cancellation.has(requestId)) {
            throw new GeometerIpcClientError(`Cancellation is already pending for request ${requestId}.`);
        }
        const frame = {
            attachments: [],
            json: encodeIpcReasonA0Json(reason === undefined ? {} : { reason }),
            kind: 5,
            requestId,
        };
        const bytes = this.encodeEffectiveFrame(frame);
        const result = deferred();
        this.cancellation.set(requestId, result);
        try {
            const [, outcome] = await Promise.all([this.writeBytes(bytes), result.promise]);
            return outcome;
        }
        catch (error) {
            this.abort(asError(error));
            throw error;
        }
    }
    allocateRequestId() {
        for (;;) {
            const value = this.nextRequestId;
            this.nextRequestId = value === 0xffffffffffffffffn ? 1n : value + 1n;
            if (!this.pending.has(value))
                return value;
        }
    }
    async writeFrame(frame) {
        return this.writeBytes(encodeGeometerIpcFrame(frame));
    }
    encodeEffectiveFrame(frame) {
        const bytes = encodeGeometerIpcFrame(frame);
        validateEffectiveFrame(frame, bytes, this.welcome.limits);
        return bytes;
    }
    async writeBytes(bytes) {
        const write = this.writeTail.then(() => {
            if (this.transportTerminated) {
                throw new GeometerIpcClientError("Geometer IPC transport is terminated.");
            }
            return this.writer.write(bytes);
        });
        this.writeTail = write.catch(() => { });
        return write;
    }
    async readLoop() {
        try {
            for (;;) {
                const item = await this.reader.read();
                if (item.done)
                    break;
                let frame = this.decoder.pushOne(item.value);
                while (frame !== undefined) {
                    this.acceptFrame(frame);
                    frame = this.decoder.pushOne();
                }
            }
            this.decoder.finish();
            if (this.state !== "closed") {
                throw new GeometerIpcClientError("Geometer IPC stream closed before shutdown completed.");
            }
        }
        catch (error) {
            this.abort(asError(error));
        }
    }
    acceptFrame(frame) {
        if (frame.kind === 10) {
            const failure = decodeIpcProtocolErrorA0Json(frame.json);
            throw new GeometerIpcClientError(failure.diagnostic.message);
        }
        if (this.state === "awaiting_welcome") {
            if (frame.kind !== 2 || frame.requestId !== 0n || frame.attachments.length !== 0) {
                throw new GeometerIpcProtocolError("Expected one attachment-free welcome frame.");
            }
            const welcome = decodeIpcWelcomeA0Json(frame.json);
            validateWelcome(welcome, this.runtimeTarget);
            this.decoder.setLimits({
                attachmentBytes: welcome.limits.attachment_bytes,
                attachmentCount: welcome.limits.attachment_count,
                attachmentMediaTypeBytes: welcome.limits.attachment_media_type_bytes,
                attachmentNameBytes: welcome.limits.attachment_name_bytes,
                frameBytes: welcome.limits.frame_bytes,
                jsonBytes: welcome.limits.json_bytes,
            });
            this.selectedWelcome = welcome;
            this.state = "running";
            this.welcomeReady.resolve(welcome);
            return;
        }
        if (frame.kind === 4 && (this.state === "running" || this.state === "draining")) {
            this.acceptResponse(frame);
            return;
        }
        if (frame.kind === 6 && (this.state === "running" || this.state === "draining")) {
            decodeIpcCancelledA0Json(frame.json);
            const pending = requiredPending(this.pending, frame.requestId);
            if (!this.cancellation.has(frame.requestId)) {
                throw new GeometerIpcProtocolError("Cancellation arrived without a pending cancellation.");
            }
            this.releasePending(frame.requestId);
            pending.response.reject(new GeometerIpcCancelledError(frame.requestId));
            this.cancellation.get(frame.requestId)?.resolve("cancelled");
            this.cancellation.delete(frame.requestId);
            return;
        }
        if (frame.kind === 7 && (this.state === "running" || this.state === "draining")) {
            decodeIpcCancelRejectedA0Json(frame.json);
            const cancellation = this.cancellation.get(frame.requestId);
            if (cancellation === undefined) {
                throw new GeometerIpcProtocolError("Cancel rejection has no pending cancellation.");
            }
            this.cancellation.delete(frame.requestId);
            cancellation.resolve("rejected");
            return;
        }
        if (frame.kind === 9 && this.state === "draining" && this.shutdownReady !== undefined) {
            if (this.pending.size !== 0 || this.cancellation.size !== 0) {
                throw new GeometerIpcProtocolError("Shutdown acknowledgment arrived before pending requests were resolved.");
            }
            const acknowledgment = decodeIpcShutdownAckA0Json(frame.json);
            this.state = "closed";
            this.shutdownReady.resolve(acknowledgment);
            return;
        }
        throw new GeometerIpcProtocolError(`Unexpected Geometer IPC frame kind ${frame.kind}.`);
    }
    acceptResponse(frame) {
        const pending = requiredPending(this.pending, frame.requestId);
        const outcome = decodeOperationOutcomeA0Json(frame.json);
        if (outcome.operation !== pending.operation) {
            throw new GeometerIpcProtocolError("Operation response identity does not match its request.");
        }
        validateIpcOutcomeOperationPair(outcome);
        const declaration = negotiatedOperation(this.welcome, pending.operation);
        validateAttachments(frame.attachments, outcome.ok ? declaration.output_attachments : [], "response");
        this.releasePending(frame.requestId);
        pending.response.resolve({
            attachments: frame.attachments,
            outcome,
            requestId: frame.requestId,
        });
    }
    fail(error) {
        if (this.state === "closed")
            return;
        this.state = "closed";
        this.welcomeReady.reject(error);
        this.shutdownReady?.reject(error);
        for (const pending of this.pending.values())
            pending.response.reject(error);
        for (const cancellation of this.cancellation.values())
            cancellation.reject(error);
        this.pending.clear();
        this.cancellation.clear();
        this.pendingResidentBytes = 0;
    }
    abort(error) {
        this.terminate(error);
    }
    reservePending(bytes) {
        const limits = this.welcome.limits;
        const byteLimit = Math.min(limits.queued_bytes, limits.resident_request_bytes);
        if (this.pending.size >= limits.queued_requests) {
            throw new GeometerIpcClientError("Geometer IPC pending request-count limit is exhausted.");
        }
        if (bytes > byteLimit - this.pendingResidentBytes) {
            throw new GeometerIpcClientError("Geometer IPC pending request-byte limit is exhausted.");
        }
        this.pendingResidentBytes += bytes;
    }
    releasePending(requestId) {
        const pending = requiredPending(this.pending, requestId);
        this.pendingResidentBytes -= pending.residentBytes;
        this.pending.delete(requestId);
    }
}
function validateWelcome(welcome, runtimeTarget) {
    if (welcome.catalog_sha256 !== NORMALIZED_CONTRACT_CATALOG_SHA256) {
        throw new GeometerIpcProtocolError("Welcome selected an unsupported contract catalog.");
    }
    for (const capability of requiredCapabilities) {
        if (!welcome.capabilities.includes(capability)) {
            throw new GeometerIpcProtocolError(`Welcome is missing required capability ${capability}.`);
        }
    }
    validateEffectiveLimits(welcome.limits);
    const expected = Object.values(operationCatalog)
        .filter((item) => runtimeTarget === "native"
        ? item.runtimeAvailable || item.nativeRuntimeAvailable
        : item.runtimeAvailable)
        .map((item) => ({
        identity: item.identity,
        input_attachments: item.inputAttachments,
        output_attachments: item.outputAttachments,
        request_contract: item.requestContract,
        ...("requestProjection" in item ? { request_projection: item.requestProjection } : {}),
        result_contract: item.resultContract,
        ...("resultProjection" in item ? { result_projection: item.resultProjection } : {}),
        runtime_dispatch: item.runtimeDispatch,
    }));
    if (canonicalJson(welcome.operation_catalog.operations) !== canonicalJson(expected)) {
        throw new GeometerIpcProtocolError("Welcome operation catalog differs from generated runtime operations.");
    }
}
function canonicalJson(value) {
    return JSON.stringify(canonicalValue(value));
}
function canonicalValue(value) {
    if (Array.isArray(value))
        return value.map(canonicalValue);
    if (value === null || typeof value !== "object")
        return value;
    return Object.fromEntries(Object.entries(value)
        .sort(([left], [right]) => left.localeCompare(right))
        .map(([key, item]) => [key, canonicalValue(item)]));
}
function validateEffectiveLimits(limits) {
    const values = [
        [limits.json_bytes, GEOMETER_IPC_A0_LIMITS.jsonBytes],
        [limits.attachment_count, GEOMETER_IPC_A0_LIMITS.attachmentCount],
        [limits.attachment_name_bytes, GEOMETER_IPC_A0_LIMITS.attachmentTextBytes],
        [limits.attachment_media_type_bytes, GEOMETER_IPC_A0_LIMITS.attachmentTextBytes],
        [limits.attachment_bytes, GEOMETER_IPC_A0_LIMITS.attachmentBytes],
        [limits.frame_bytes, GEOMETER_IPC_A0_LIMITS.frameBytes],
        [limits.queued_requests, GEOMETER_IPC_A0_LIMITS.queuedRequests],
        [limits.queued_bytes, GEOMETER_IPC_A0_LIMITS.queuedBytes],
        [limits.resident_request_bytes, GEOMETER_IPC_A0_LIMITS.residentRequestBytes],
        [limits.pending_writer_bytes, GEOMETER_IPC_A0_LIMITS.pendingWriterBytes],
    ];
    if (values.some(([value, maximum]) => !Number.isInteger(value) || value <= 0 || value > maximum)) {
        throw new GeometerIpcProtocolError("Welcome advertises an invalid effective limit.");
    }
}
function negotiatedOperation(welcome, operation) {
    const declaration = welcome.operation_catalog.operations.find((item) => item.identity === operation);
    if (declaration === undefined) {
        throw new GeometerIpcClientError(`Operation ${operation} is structural-only and absent from the negotiated runtime catalog.`);
    }
    return declaration;
}
function validateAttachments(attachments, declarations, direction) {
    const seen = new Set();
    for (const attachment of attachments) {
        if (seen.has(attachment.name)) {
            throw new GeometerIpcProtocolError(`Duplicate ${direction} attachment ${attachment.name}.`);
        }
        seen.add(attachment.name);
        const declaration = declarations.find((item) => item.name === attachment.name);
        if (declaration === undefined) {
            throw new GeometerIpcProtocolError(`Undeclared ${direction} attachment ${attachment.name}.`);
        }
        if (!declaration.media_types.includes(attachment.mediaType)) {
            throw new GeometerIpcProtocolError(`${direction} attachment media type is not declared.`);
        }
        if (attachment.data.byteLength > declaration.max_bytes) {
            throw new GeometerIpcProtocolError(`${direction} attachment exceeds its operation limit.`);
        }
    }
    for (const declaration of declarations) {
        if (declaration.required && !seen.has(declaration.name)) {
            throw new GeometerIpcProtocolError(`Required ${direction} attachment ${declaration.name} is missing.`);
        }
    }
}
function validateEffectiveFrame(frame, bytes, limits) {
    if (textEncoder.encode(frame.json).byteLength > limits.json_bytes ||
        frame.attachments.length > limits.attachment_count ||
        bytes.byteLength > limits.frame_bytes ||
        frame.attachments.some((item) => textEncoder.encode(item.name).byteLength > limits.attachment_name_bytes ||
            textEncoder.encode(item.mediaType).byteLength > limits.attachment_media_type_bytes ||
            item.data.byteLength > limits.attachment_bytes)) {
        throw new GeometerIpcProtocolError("Request exceeds an effective limit advertised by welcome.");
    }
}
function requiredPending(map, requestId) {
    const pending = map.get(requestId);
    if (pending === undefined) {
        throw new GeometerIpcProtocolError(`Response has unknown request id ${requestId}.`);
    }
    return pending;
}
function deferred() {
    let resolve;
    let reject;
    const promise = new Promise((onResolve, onReject) => {
        resolve = onResolve;
        reject = onReject;
    });
    return { promise, reject, resolve };
}
function asError(value) {
    return value instanceof Error ? value : new GeometerIpcClientError(String(value));
}
