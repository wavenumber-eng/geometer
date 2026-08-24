import {
  decodeIpcCancelledA0Json,
  decodeIpcCancelRejectedA0Json,
  decodeIpcProtocolErrorA0Json,
  decodeIpcShutdownAckA0Json,
  decodeIpcWelcomeA0Json,
  decodeOperationOutcomeA0Json,
  encodeIpcHelloA0Json,
  encodeIpcReasonA0Json,
  encodeIpcRequestA0Json,
} from "./generated/codecs.js";
import type {
  IpcEffectiveLimitsA0,
  IpcOperationDeclarationA0,
  IpcRequestValueA0,
  IpcShutdownAckA0,
  IpcWelcomeA0,
  OperationOutcomeA0,
} from "./generated/contracts.js";
import {
  NORMALIZED_CONTRACT_CATALOG_SHA256,
  type OperationIdentity,
  operationCatalog,
} from "./generated/operations.js";
import {
  encodeGeometerIpcFrame,
  GEOMETER_IPC_A0_LIMITS,
  type GeometerIpcAttachment,
  type GeometerIpcFrame,
  GeometerIpcFrameDecoder,
  GeometerIpcProtocolError,
  validateIpcOutcomeOperationPair,
  validateIpcRequestOperationPair,
} from "./ipc-a0.js";

const requiredCapabilities = [
  "serialized_execution",
  "queue_only_cancellation",
  "raw_attachments",
] as const;
const textEncoder = new TextEncoder();

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

export interface GeometerIpcCallA0 {
  readonly requestId: bigint;
  readonly response: Promise<GeometerIpcOperationResponseA0>;
  cancel(reason?: string): Promise<"cancelled" | "rejected">;
}

export class GeometerIpcClientError extends Error {
  constructor(message: string) {
    super(message);
    this.name = "GeometerIpcClientError";
  }
}

export class GeometerIpcCancelledError extends GeometerIpcClientError {
  constructor(readonly requestId: bigint) {
    super(`Geometer IPC request ${requestId} was cancelled.`);
    this.name = "GeometerIpcCancelledError";
  }
}

interface PendingCall {
  readonly operation: OperationIdentity;
  readonly residentBytes: number;
  readonly response: Deferred<GeometerIpcOperationResponseA0>;
}

export class GeometerIpcClientA0 {
  private readonly decoder = new GeometerIpcFrameDecoder();
  private readonly pending = new Map<bigint, PendingCall>();
  private readonly cancellation = new Map<bigint, Deferred<"cancelled" | "rejected">>();
  private readonly reader: ReadableStreamDefaultReader<Uint8Array>;
  private readonly writer: WritableStreamDefaultWriter<Uint8Array>;
  private readonly welcomeReady = deferred<IpcWelcomeA0>();
  private shutdownReady?: Deferred<IpcShutdownAckA0>;
  private writeTail: Promise<void> = Promise.resolve();
  private transportTerminated = false;
  private pendingResidentBytes = 0;
  private state: "awaiting_welcome" | "running" | "draining" | "closed" = "awaiting_welcome";
  private nextRequestId = 1n;
  private selectedWelcome?: IpcWelcomeA0;

  private constructor(
    private readonly duplex: GeometerIpcDuplexA0,
    private readonly runtimeTarget: "portable" | "native",
  ) {
    this.reader = duplex.readable.getReader();
    this.writer = duplex.writable.getWriter();
    void this.readLoop();
  }

  static async connect(
    duplex: GeometerIpcDuplexA0,
    options: GeometerIpcConnectOptionsA0,
  ): Promise<GeometerIpcClientA0> {
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
    } catch (error) {
      client.terminate(asError(error));
      throw error;
    }
  }

  get welcome(): IpcWelcomeA0 {
    if (this.selectedWelcome === undefined) {
      throw new GeometerIpcClientError("Geometer IPC handshake is incomplete.");
    }
    return this.selectedWelcome;
  }

  start(
    operation: OperationIdentity,
    request: IpcRequestValueA0,
    attachments: readonly GeometerIpcAttachment[] = [],
  ): GeometerIpcCallA0 {
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
      kind: 3 as const,
      requestId,
    };
    const bytes = this.encodeEffectiveFrame(frame);
    this.reservePending(bytes.byteLength);
    const response = deferred<GeometerIpcOperationResponseA0>();
    this.pending.set(requestId, { operation, residentBytes: bytes.byteLength, response });
    void this.writeBytes(bytes).catch((error) => this.abort(asError(error)));
    return {
      cancel: (reason?: string) => this.cancel(requestId, reason),
      requestId,
      response: response.promise,
    };
  }

  async execute(
    operation: OperationIdentity,
    request: IpcRequestValueA0,
    attachments: readonly GeometerIpcAttachment[] = [],
  ): Promise<GeometerIpcOperationResponseA0> {
    return this.start(operation, request, attachments).response;
  }

  async close(reason?: string): Promise<IpcShutdownAckA0> {
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
      kind: 8 as const,
      requestId: 0n,
    };
    const bytes = this.encodeEffectiveFrame(frame);
    this.state = "draining";
    this.shutdownReady = deferred<IpcShutdownAckA0>();
    try {
      const [, acknowledgment] = await Promise.all([
        this.writeBytes(bytes),
        this.shutdownReady.promise,
      ]);
      return acknowledgment;
    } catch (error) {
      this.abort(asError(error));
      throw error;
    }
  }

  terminate(reason = new GeometerIpcClientError("Geometer IPC connection was terminated.")): void {
    if (!this.transportTerminated) {
      this.transportTerminated = true;
      try {
        this.duplex.terminate(reason);
      } finally {
        this.fail(reason);
      }
    } else {
      this.fail(reason);
    }
  }

  private async cancel(requestId: bigint, reason?: string): Promise<"cancelled" | "rejected"> {
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
      kind: 5 as const,
      requestId,
    };
    const bytes = this.encodeEffectiveFrame(frame);
    const result = deferred<"cancelled" | "rejected">();
    this.cancellation.set(requestId, result);
    try {
      const [, outcome] = await Promise.all([this.writeBytes(bytes), result.promise]);
      return outcome;
    } catch (error) {
      this.abort(asError(error));
      throw error;
    }
  }

  private allocateRequestId(): bigint {
    for (;;) {
      const value = this.nextRequestId;
      this.nextRequestId = value === 0xffff_ffff_ffff_ffffn ? 1n : value + 1n;
      if (!this.pending.has(value)) return value;
    }
  }

  private async writeFrame(frame: GeometerIpcFrame): Promise<void> {
    return this.writeBytes(encodeGeometerIpcFrame(frame));
  }

  private encodeEffectiveFrame(frame: GeometerIpcFrame): Uint8Array {
    const bytes = encodeGeometerIpcFrame(frame);
    validateEffectiveFrame(frame, bytes, this.welcome.limits);
    return bytes;
  }

  private async writeBytes(bytes: Uint8Array): Promise<void> {
    const write = this.writeTail.then(() => {
      if (this.transportTerminated) {
        throw new GeometerIpcClientError("Geometer IPC transport is terminated.");
      }
      return this.writer.write(bytes);
    });
    this.writeTail = write.catch(() => {});
    return write;
  }

  private async readLoop(): Promise<void> {
    try {
      for (;;) {
        const item = await this.reader.read();
        if (item.done) break;
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
    } catch (error) {
      this.abort(asError(error));
    }
  }

  private acceptFrame(frame: GeometerIpcFrame): void {
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
        throw new GeometerIpcProtocolError(
          "Shutdown acknowledgment arrived before pending requests were resolved.",
        );
      }
      const acknowledgment = decodeIpcShutdownAckA0Json(frame.json);
      this.state = "closed";
      this.shutdownReady.resolve(acknowledgment);
      return;
    }
    throw new GeometerIpcProtocolError(`Unexpected Geometer IPC frame kind ${frame.kind}.`);
  }

  private acceptResponse(frame: GeometerIpcFrame): void {
    const pending = requiredPending(this.pending, frame.requestId);
    const outcome = decodeOperationOutcomeA0Json(frame.json);
    if (outcome.operation !== pending.operation) {
      throw new GeometerIpcProtocolError("Operation response identity does not match its request.");
    }
    validateIpcOutcomeOperationPair(outcome);
    const declaration = negotiatedOperation(this.welcome, pending.operation);
    validateAttachments(
      frame.attachments,
      outcome.ok ? declaration.output_attachments : [],
      "response",
    );
    this.releasePending(frame.requestId);
    pending.response.resolve({
      attachments: frame.attachments,
      outcome,
      requestId: frame.requestId,
    });
  }

  private fail(error: Error): void {
    if (this.state === "closed") return;
    this.state = "closed";
    this.welcomeReady.reject(error);
    this.shutdownReady?.reject(error);
    for (const pending of this.pending.values()) pending.response.reject(error);
    for (const cancellation of this.cancellation.values()) cancellation.reject(error);
    this.pending.clear();
    this.cancellation.clear();
    this.pendingResidentBytes = 0;
  }

  private abort(error: Error): void {
    this.terminate(error);
  }

  private reservePending(bytes: number): void {
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

  private releasePending(requestId: bigint): void {
    const pending = requiredPending(this.pending, requestId);
    this.pendingResidentBytes -= pending.residentBytes;
    this.pending.delete(requestId);
  }
}

function validateWelcome(welcome: IpcWelcomeA0, runtimeTarget: "portable" | "native"): void {
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
    .filter((item) =>
      runtimeTarget === "native"
        ? item.runtimeAvailable || item.nativeRuntimeAvailable
        : item.runtimeAvailable,
    )
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
    throw new GeometerIpcProtocolError(
      "Welcome operation catalog differs from generated runtime operations.",
    );
  }
}

function canonicalJson(value: unknown): string {
  return JSON.stringify(canonicalValue(value));
}

function canonicalValue(value: unknown): unknown {
  if (Array.isArray(value)) return value.map(canonicalValue);
  if (value === null || typeof value !== "object") return value;
  return Object.fromEntries(
    Object.entries(value)
      .sort(([left], [right]) => left.localeCompare(right))
      .map(([key, item]) => [key, canonicalValue(item)]),
  );
}

function validateEffectiveLimits(limits: IpcEffectiveLimitsA0): void {
  const values: readonly [number, number][] = [
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
  if (
    values.some(([value, maximum]) => !Number.isInteger(value) || value <= 0 || value > maximum)
  ) {
    throw new GeometerIpcProtocolError("Welcome advertises an invalid effective limit.");
  }
}

function negotiatedOperation(
  welcome: IpcWelcomeA0,
  operation: OperationIdentity,
): IpcOperationDeclarationA0 {
  const declaration = welcome.operation_catalog.operations.find(
    (item) => item.identity === operation,
  );
  if (declaration === undefined) {
    throw new GeometerIpcClientError(
      `Operation ${operation} is structural-only and absent from the negotiated runtime catalog.`,
    );
  }
  return declaration;
}

function validateAttachments(
  attachments: readonly GeometerIpcAttachment[],
  declarations: readonly IpcOperationDeclarationA0["input_attachments"][number][],
  direction: "request" | "response",
): void {
  const seen = new Set<string>();
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
      throw new GeometerIpcProtocolError(
        `Required ${direction} attachment ${declaration.name} is missing.`,
      );
    }
  }
}

function validateEffectiveFrame(
  frame: GeometerIpcFrame,
  bytes: Uint8Array,
  limits: IpcEffectiveLimitsA0,
): void {
  if (
    textEncoder.encode(frame.json).byteLength > limits.json_bytes ||
    frame.attachments.length > limits.attachment_count ||
    bytes.byteLength > limits.frame_bytes ||
    frame.attachments.some(
      (item) =>
        textEncoder.encode(item.name).byteLength > limits.attachment_name_bytes ||
        textEncoder.encode(item.mediaType).byteLength > limits.attachment_media_type_bytes ||
        item.data.byteLength > limits.attachment_bytes,
    )
  ) {
    throw new GeometerIpcProtocolError("Request exceeds an effective limit advertised by welcome.");
  }
}

function requiredPending(map: Map<bigint, PendingCall>, requestId: bigint): PendingCall {
  const pending = map.get(requestId);
  if (pending === undefined) {
    throw new GeometerIpcProtocolError(`Response has unknown request id ${requestId}.`);
  }
  return pending;
}

interface Deferred<T> {
  readonly promise: Promise<T>;
  readonly reject: (reason: Error) => void;
  readonly resolve: (value: T) => void;
}

function deferred<T>(): Deferred<T> {
  let resolve!: (value: T) => void;
  let reject!: (reason: Error) => void;
  const promise = new Promise<T>((onResolve, onReject) => {
    resolve = onResolve;
    reject = onReject;
  });
  return { promise, reject, resolve };
}

function asError(value: unknown): Error {
  return value instanceof Error ? value : new GeometerIpcClientError(String(value));
}
