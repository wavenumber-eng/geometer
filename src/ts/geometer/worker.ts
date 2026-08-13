import type { DiagnosticA0, ModelBoundsResultA0 } from "./generated/index.js";
import { decodeOperationOutcomeA0Json, encodeModelBoundsOptionsA0Json } from "./generated/index.js";
import type {
  GeometerOperationAttachment,
  GeometerOperationResponse,
  GeometerWasmCapabilityCatalog,
  ModelBoundsRequest,
} from "./wasm.js";
import { GeometerOperationError, GeometerWasmTransportError } from "./wasm.js";

export const GEOMETER_WASM_WORKER_PROTOCOL = "wn.geometer.wasm_worker.a0" as const;

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

export type GeometerWorkerRequestMessage =
  | {
      readonly kind: "initialize";
      readonly moduleOptions: Readonly<Record<string, unknown>>;
      readonly protocol: typeof GEOMETER_WASM_WORKER_PROTOCOL;
      readonly requestId: string;
      readonly wasmBinary: ArrayBuffer;
    }
  | {
      readonly attachments: readonly GeometerWorkerAttachmentMessage[];
      readonly kind: "execute";
      readonly operation: string;
      readonly protocol: typeof GEOMETER_WASM_WORKER_PROTOCOL;
      readonly requestId: string;
      readonly requestJson: string;
    }
  | {
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

export type GeometerWorkerResponseMessage =
  | {
      readonly capabilities: GeometerWasmCapabilityCatalog;
      readonly kind: "ready";
      readonly protocol: typeof GEOMETER_WASM_WORKER_PROTOCOL;
      readonly requestId: string;
    }
  | {
      readonly attachments: readonly GeometerWorkerAttachmentMessage[];
      readonly kind: "operation_result";
      readonly outcomeJson: string;
      readonly protocol: typeof GEOMETER_WASM_WORKER_PROTOCOL;
      readonly requestId: string;
    }
  | {
      readonly error: GeometerWorkerSerializedError;
      readonly kind: "error";
      readonly protocol: typeof GEOMETER_WASM_WORKER_PROTOCOL;
      readonly requestId: string;
    }
  | {
      readonly kind: "closed";
      readonly protocol: typeof GEOMETER_WASM_WORKER_PROTOCOL;
      readonly requestId: string;
    };

export class GeometerWorkerError extends Error {
  constructor(message: string) {
    super(message);
    this.name = "GeometerWorkerError";
  }
}

export class GeometerWorkerClient {
  readonly capabilities: GeometerWasmCapabilityCatalog;
  private readonly connection: WorkerConnection;

  private constructor(connection: WorkerConnection, capabilities: GeometerWasmCapabilityCatalog) {
    this.connection = connection;
    this.capabilities = capabilities;
  }

  static async create(
    worker: Worker,
    options: GeometerWorkerClientOptions,
  ): Promise<GeometerWorkerClient> {
    const connection = new WorkerConnection(worker);
    try {
      const wasmBinary = copyToArrayBuffer(options.wasmBinary);
      const response = await connection.request(
        {
          kind: "initialize",
          moduleOptions: options.moduleOptions ?? {},
          wasmBinary,
        },
        [wasmBinary],
      );
      if (response.kind !== "ready" || !isCapabilityCatalog(response.capabilities)) {
        throw new GeometerWorkerError("Geometer Worker returned an invalid capability handshake.");
      }
      return new GeometerWorkerClient(connection, freezeCapabilities(response.capabilities));
    } catch (error) {
      connection.terminate(error instanceof Error ? error : new GeometerWorkerError(String(error)));
      throw error;
    }
  }

  async modelBounds(request: ModelBoundsRequest): Promise<ModelBoundsResultA0> {
    const response = await this.execute(
      "geometry.model_bounds.a0",
      encodeModelBoundsOptionsA0Json(request.options ?? {}),
      [
        {
          name: "model",
          mediaType: request.mediaType ?? "application/step",
          data: request.model,
        },
      ],
    );
    if (!response.outcome.ok) {
      throw new GeometerOperationError(response.outcome.operation, response.outcome.diagnostics);
    }
    if (response.outcome.operation !== "geometry.model_bounds.a0") {
      throw new GeometerWorkerError(
        `Expected geometry.model_bounds.a0 response, received ${response.outcome.operation}.`,
      );
    }
    if (response.attachments.length !== 0) {
      throw new GeometerWorkerError("model_bounds returned unexpected attachments.");
    }
    return response.outcome.result;
  }

  async execute(
    operation: string,
    requestJson: string,
    attachments: readonly GeometerOperationAttachment[],
  ): Promise<GeometerOperationResponse> {
    const transferred = attachments.map((attachment) => ({
      data: copyToArrayBuffer(attachment.data),
      mediaType: attachment.mediaType,
      name: attachment.name,
    }));
    const response = await this.connection.request(
      {
        attachments: transferred,
        kind: "execute",
        operation,
        requestJson,
      },
      transferred.map((attachment) => attachment.data),
    );
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
  async close(): Promise<void> {
    await this.connection.close();
  }

  /** Immediately terminates the Worker and rejects all outstanding requests. */
  terminate(): void {
    this.connection.terminate(new GeometerWorkerError("Geometer Worker was terminated."));
  }
}

export async function createGeometerWorkerClient(
  worker: Worker,
  options: GeometerWorkerClientOptions,
): Promise<GeometerWorkerClient> {
  return GeometerWorkerClient.create(worker, options);
}

type RequestBody =
  | Omit<Extract<GeometerWorkerRequestMessage, { kind: "initialize" }>, "protocol" | "requestId">
  | Omit<Extract<GeometerWorkerRequestMessage, { kind: "execute" }>, "protocol" | "requestId">
  | Omit<Extract<GeometerWorkerRequestMessage, { kind: "shutdown" }>, "protocol" | "requestId">;

interface PendingRequest {
  readonly reject: (error: Error) => void;
  readonly resolve: (response: GeometerWorkerResponseMessage) => void;
}

class WorkerConnection {
  private closed = false;
  private closing = false;
  private closePromise: Promise<void> | undefined;
  private nextRequestId = 1;
  private readonly pending = new Map<string, PendingRequest>();
  private readonly worker: Worker;

  constructor(worker: Worker) {
    this.worker = worker;
    worker.addEventListener("message", this.handleMessage);
    worker.addEventListener("error", this.handleError);
    worker.addEventListener("messageerror", this.handleMessageError);
  }

  request(
    body: RequestBody,
    transfer: readonly Transferable[],
    allowWhileClosing = false,
  ): Promise<GeometerWorkerResponseMessage> {
    if (this.closed || (this.closing && !allowWhileClosing)) {
      return Promise.reject(new GeometerWorkerError("Geometer Worker is closed."));
    }
    const requestId = `geometer-worker-${this.nextRequestId}`;
    this.nextRequestId += 1;
    const message = {
      ...body,
      protocol: GEOMETER_WASM_WORKER_PROTOCOL,
      requestId,
    } as GeometerWorkerRequestMessage;
    return new Promise((resolve, reject) => {
      this.pending.set(requestId, { reject, resolve });
      try {
        this.worker.postMessage(message, [...transfer]);
      } catch (error) {
        this.pending.delete(requestId);
        reject(error instanceof Error ? error : new GeometerWorkerError(String(error)));
      }
    });
  }

  close(): Promise<void> {
    if (this.closed) return Promise.resolve();
    if (this.closePromise) return this.closePromise;
    this.closing = true;
    this.closePromise = this.finishClose();
    return this.closePromise;
  }

  private async finishClose(): Promise<void> {
    try {
      const response = await this.request({ kind: "shutdown" }, [], true);
      if (response.kind !== "closed") {
        throw new GeometerWorkerError(`Expected closed, received ${response.kind}.`);
      }
    } finally {
      this.terminate(new GeometerWorkerError("Geometer Worker is closed."));
    }
  }

  terminate(error: Error): void {
    if (this.closed) return;
    this.closed = true;
    this.worker.removeEventListener("message", this.handleMessage);
    this.worker.removeEventListener("error", this.handleError);
    this.worker.removeEventListener("messageerror", this.handleMessageError);
    this.worker.terminate();
    for (const pending of this.pending.values()) pending.reject(error);
    this.pending.clear();
  }

  private readonly handleMessage = (event: MessageEvent<unknown>): void => {
    if (!isWorkerResponse(event.data)) {
      this.terminate(new GeometerWorkerError("Geometer Worker returned a malformed message."));
      return;
    }
    const response = event.data;
    const pending = this.pending.get(response.requestId);
    if (!pending) return;
    this.pending.delete(response.requestId);
    if (response.kind === "error") pending.reject(deserializeError(response.error));
    else pending.resolve(response);
  };

  private readonly handleError = (event: ErrorEvent): void => {
    this.terminate(
      new GeometerWorkerError(event.message || "Geometer Worker exited unexpectedly."),
    );
  };

  private readonly handleMessageError = (): void => {
    this.terminate(new GeometerWorkerError("Geometer Worker message deserialization failed."));
  };
}

function isWorkerResponse(value: unknown): value is GeometerWorkerResponseMessage {
  if (!isRecord(value) || value.protocol !== GEOMETER_WASM_WORKER_PROTOCOL) return false;
  if (typeof value.requestId !== "string" || typeof value.kind !== "string") return false;
  if (value.kind === "ready") return isCapabilityCatalog(value.capabilities);
  if (value.kind === "closed") return true;
  if (value.kind === "error") {
    return (
      isRecord(value.error) &&
      typeof value.error.name === "string" &&
      typeof value.error.message === "string"
    );
  }
  if (value.kind === "operation_result") {
    return (
      typeof value.outcomeJson === "string" &&
      Array.isArray(value.attachments) &&
      value.attachments.every(isAttachmentMessage)
    );
  }
  return false;
}

function isAttachmentMessage(value: unknown): value is GeometerWorkerAttachmentMessage {
  return (
    isRecord(value) &&
    value.data instanceof ArrayBuffer &&
    typeof value.mediaType === "string" &&
    typeof value.name === "string"
  );
}

function isCapabilityCatalog(value: unknown): value is GeometerWasmCapabilityCatalog {
  return (
    isRecord(value) &&
    Number.isSafeInteger(value.cAbiGeneration) &&
    value.genericAbi === "a0" &&
    typeof value.releaseVersion === "string" &&
    Array.isArray(value.operations) &&
    value.operations.every((operation) => typeof operation === "string")
  );
}

function freezeCapabilities(value: GeometerWasmCapabilityCatalog): GeometerWasmCapabilityCatalog {
  return Object.freeze({
    cAbiGeneration: value.cAbiGeneration,
    genericAbi: value.genericAbi,
    operations: Object.freeze([...value.operations]),
    releaseVersion: value.releaseVersion,
  });
}

function deserializeError(error: GeometerWorkerSerializedError): Error {
  if (error.name === "GeometerWasmTransportError" && typeof error.code === "number") {
    return new GeometerWasmTransportError(error.code, error.message);
  }
  if (
    error.name === "GeometerOperationError" &&
    typeof error.operation === "string" &&
    Array.isArray(error.diagnostics)
  ) {
    return new GeometerOperationError(error.operation, error.diagnostics);
  }
  const result = new GeometerWorkerError(error.message);
  if (error.stack) result.stack = error.stack;
  return result;
}

function copyToArrayBuffer(value: ArrayBuffer | Uint8Array): ArrayBuffer {
  const view = value instanceof Uint8Array ? value : new Uint8Array(value);
  const copy = new Uint8Array(view.byteLength);
  copy.set(view);
  return copy.buffer;
}

function isRecord(value: unknown): value is Record<string, unknown> {
  return typeof value === "object" && value !== null && !Array.isArray(value);
}
