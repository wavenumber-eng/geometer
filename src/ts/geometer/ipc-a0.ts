const MAGIC = new Uint8Array([0x47, 0x4d, 0x49, 0x50, 0x43, 0x41, 0x30, 0x31]);
const HEADER_BYTES = 48;
const MAX_JSON_BYTES = 8 * 1024 * 1024;
const MAX_ATTACHMENT_COUNT = 16;
const MAX_ATTACHMENT_TEXT_BYTES = 128;
const MAX_ATTACHMENT_BYTES = 256 * 1024 * 1024;
const MAX_FRAME_BYTES = 512 * 1024 * 1024;
const textEncoder = new TextEncoder();
const textDecoder = new TextDecoder("utf-8", { fatal: true, ignoreBOM: true });

export const GEOMETER_IPC_A0_LIMITS = Object.freeze({
  attachmentBytes: MAX_ATTACHMENT_BYTES,
  attachmentCount: MAX_ATTACHMENT_COUNT,
  attachmentTextBytes: MAX_ATTACHMENT_TEXT_BYTES,
  frameBytes: MAX_FRAME_BYTES,
  jsonBytes: MAX_JSON_BYTES,
  pendingWriterBytes: MAX_FRAME_BYTES,
  queuedBytes: MAX_FRAME_BYTES,
  queuedRequests: 8,
  residentRequestBytes: MAX_FRAME_BYTES,
});

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

export class GeometerIpcProtocolError extends Error {
  constructor(message: string) {
    super(message);
    this.name = "GeometerIpcProtocolError";
  }
}

export function validateIpcRequestOperationPair(envelope: IpcRequestA0): void {
  const declaration = operationDeclaration(envelope.operation);
  const projection = "requestProjection" in declaration ? declaration.requestProjection : undefined;
  const contract = validatePayloadProjection(
    envelope.request,
    declaration.runtimeDispatch,
    projection,
    declaration.runtimeDispatch === "logical_dto" ? declaration.requestContract : undefined,
  );
  if (contract !== declaration.requestContract) {
    throw new GeometerIpcProtocolError(
      `IPC request payload ${contract} does not match operation ${envelope.operation}.`,
    );
  }
}

export function validateIpcOutcomeOperationPair(outcome: OperationOutcomeA0): void {
  if (!outcome.ok) return;
  const declaration = operationDeclaration(outcome.operation);
  const projection = "resultProjection" in declaration ? declaration.resultProjection : undefined;
  const contract = validatePayloadProjection(
    outcome.result,
    declaration.runtimeDispatch,
    projection,
  );
  if (contract !== declaration.resultContract) {
    throw new GeometerIpcProtocolError(
      `IPC result payload ${contract} does not match operation ${outcome.operation}.`,
    );
  }
}

export function encodeGeometerIpcFrame(frame: GeometerIpcFrame): Uint8Array {
  validateKindAndRequestId(frame.kind, frame.requestId);
  const json = textEncoder.encode(frame.json);
  if (json.byteLength === 0 || json.byteLength > MAX_JSON_BYTES) {
    throw new GeometerIpcProtocolError("IPC JSON size is outside the A0 limit.");
  }
  validateAttachmentDirection(frame.kind, frame.attachments.length);
  if (frame.attachments.length > MAX_ATTACHMENT_COUNT) {
    throw new GeometerIpcProtocolError("IPC attachment count exceeds the A0 limit.");
  }

  const encodedAttachments: Array<{
    readonly name: Uint8Array;
    readonly mediaType: Uint8Array;
    readonly data: Uint8Array;
  }> = [];
  const names = new Set<string>();
  let attachmentBytes = 0;
  for (const attachment of frame.attachments) {
    if (names.has(attachment.name)) {
      throw new GeometerIpcProtocolError(`Duplicate IPC attachment ${attachment.name}.`);
    }
    names.add(attachment.name);
    const name = textEncoder.encode(attachment.name);
    const mediaType = textEncoder.encode(attachment.mediaType);
    if (
      name.byteLength === 0 ||
      name.byteLength > MAX_ATTACHMENT_TEXT_BYTES ||
      mediaType.byteLength === 0 ||
      mediaType.byteLength > MAX_ATTACHMENT_TEXT_BYTES
    ) {
      throw new GeometerIpcProtocolError("IPC attachment text is outside the A0 limit.");
    }
    if (attachment.data.byteLength > MAX_ATTACHMENT_BYTES) {
      throw new GeometerIpcProtocolError("IPC attachment data exceeds the A0 limit.");
    }
    attachmentBytes = checkedAdd(
      attachmentBytes,
      16 + name.byteLength + mediaType.byteLength + attachment.data.byteLength,
      "IPC attachment section",
    );
    encodedAttachments.push({ data: attachment.data, mediaType, name });
  }
  const frameBytes = checkedAdd(HEADER_BYTES + json.byteLength, attachmentBytes, "IPC frame");
  if (frameBytes > MAX_FRAME_BYTES) {
    throw new GeometerIpcProtocolError("IPC frame exceeds the A0 limit.");
  }

  const output = new Uint8Array(frameBytes);
  output.set(MAGIC, 0);
  const view = new DataView(output.buffer);
  view.setUint16(8, HEADER_BYTES, true);
  view.setUint16(10, 0, true);
  view.setUint16(12, frame.kind, true);
  view.setUint16(14, 0, true);
  view.setBigUint64(16, frame.requestId, true);
  view.setUint32(24, json.byteLength, true);
  view.setUint32(28, encodedAttachments.length, true);
  view.setBigUint64(32, BigInt(attachmentBytes), true);
  output.set(json, HEADER_BYTES);
  let offset = HEADER_BYTES + json.byteLength;
  for (const attachment of encodedAttachments) {
    view.setUint16(offset, attachment.name.byteLength, true);
    view.setUint16(offset + 2, attachment.mediaType.byteLength, true);
    view.setUint32(offset + 4, 0, true);
    view.setBigUint64(offset + 8, BigInt(attachment.data.byteLength), true);
    offset += 16;
    output.set(attachment.name, offset);
    offset += attachment.name.byteLength;
    output.set(attachment.mediaType, offset);
    offset += attachment.mediaType.byteLength;
    output.set(attachment.data, offset);
    offset += attachment.data.byteLength;
  }
  return output;
}

export class GeometerIpcFrameDecoder {
  private readonly pending = new ByteQueue();
  private limits: GeometerIpcFrameDecodeLimits = {
    attachmentBytes: MAX_ATTACHMENT_BYTES,
    attachmentCount: MAX_ATTACHMENT_COUNT,
    attachmentMediaTypeBytes: MAX_ATTACHMENT_TEXT_BYTES,
    attachmentNameBytes: MAX_ATTACHMENT_TEXT_BYTES,
    frameBytes: MAX_FRAME_BYTES,
    jsonBytes: MAX_JSON_BYTES,
  };

  /**
   * Transfers ownership of `chunk` to the decoder. The caller must not mutate or detach it after
   * this call. Decoded attachment views may retain the transferred storage without another copy.
   */
  push(chunk: Uint8Array): readonly GeometerIpcFrame[] {
    const frames: GeometerIpcFrame[] = [];
    let frame = this.pushOne(chunk);
    while (frame !== undefined) {
      frames.push(frame);
      frame = this.pushOne();
    }
    return frames;
  }

  /** Decodes at most one frame so negotiated limits can change between coalesced frames. */
  pushOne(chunk?: Uint8Array): GeometerIpcFrame | undefined {
    if (chunk !== undefined) this.pending.push(chunk);
    if (this.pending.byteLength < HEADER_BYTES) return undefined;
    const header = decodeFixedHeader(this.pending.peek(HEADER_BYTES), this.limits);
    if (this.pending.byteLength < header.frameBytes) return undefined;
    return decodeQueuedFrame(this.pending, header, this.limits);
  }

  setLimits(limits: GeometerIpcFrameDecodeLimits): void {
    const maxima: readonly (readonly [number, number])[] = [
      [limits.attachmentBytes, MAX_ATTACHMENT_BYTES],
      [limits.attachmentCount, MAX_ATTACHMENT_COUNT],
      [limits.attachmentMediaTypeBytes, MAX_ATTACHMENT_TEXT_BYTES],
      [limits.attachmentNameBytes, MAX_ATTACHMENT_TEXT_BYTES],
      [limits.frameBytes, MAX_FRAME_BYTES],
      [limits.jsonBytes, MAX_JSON_BYTES],
    ];
    if (
      maxima.some(([value, maximum]) => !Number.isInteger(value) || value <= 0 || value > maximum)
    ) {
      throw new GeometerIpcProtocolError("IPC decoder limit is outside the A0 maximum.");
    }
    this.limits = { ...limits };
  }

  finish(): void {
    if (this.pending.byteLength !== 0) {
      throw new GeometerIpcProtocolError("IPC stream ended within a frame.");
    }
  }
}

interface DecodedFixedHeader {
  readonly attachmentBytes: number;
  readonly attachmentCount: number;
  readonly frameBytes: number;
  readonly jsonBytes: number;
  readonly kind: GeometerIpcFrameKind;
  readonly requestId: bigint;
}

function decodeFixedHeader(
  bytes: Uint8Array,
  limits: GeometerIpcFrameDecodeLimits,
): DecodedFixedHeader {
  for (let index = 0; index < MAGIC.byteLength; index += 1) {
    if (bytes[index] !== MAGIC[index]) throw new GeometerIpcProtocolError("Invalid IPC magic.");
  }
  const view = new DataView(bytes.buffer, bytes.byteOffset, HEADER_BYTES);
  if (
    view.getUint16(8, true) !== HEADER_BYTES ||
    view.getUint16(10, true) !== 0 ||
    view.getUint16(14, true) !== 0 ||
    view.getUint32(40, true) !== 0 ||
    view.getUint32(44, true) !== 0
  ) {
    throw new GeometerIpcProtocolError("Unsupported IPC A0 header fields.");
  }
  const jsonBytes = view.getUint32(24, true);
  const attachmentCount = view.getUint32(28, true);
  const attachmentBytes = safeUint64(view.getBigUint64(32, true), "attachment section");
  const kindValue = view.getUint16(12, true);
  if (kindValue < 1 || kindValue > 10) {
    throw new GeometerIpcProtocolError("Unknown IPC frame kind.");
  }
  const kind = kindValue as GeometerIpcFrameKind;
  const requestId = view.getBigUint64(16, true);
  validateKindAndRequestId(kind, requestId);
  validateAttachmentDirection(kind, attachmentCount);
  if (jsonBytes === 0 || jsonBytes > limits.jsonBytes || attachmentCount > limits.attachmentCount) {
    throw new GeometerIpcProtocolError("IPC header declares an out-of-range payload.");
  }
  if ((attachmentCount === 0 && attachmentBytes !== 0) || attachmentBytes < attachmentCount * 18) {
    throw new GeometerIpcProtocolError("IPC attachment count and section size are inconsistent.");
  }
  const frameBytes = checkedAdd(HEADER_BYTES + jsonBytes, attachmentBytes, "IPC frame");
  if (frameBytes > limits.frameBytes) {
    throw new GeometerIpcProtocolError("IPC frame exceeds the A0 limit.");
  }
  return { attachmentBytes, attachmentCount, frameBytes, jsonBytes, kind, requestId };
}

function decodeQueuedFrame(
  queue: ByteQueue,
  header: DecodedFixedHeader,
  limits: GeometerIpcFrameDecodeLimits,
): GeometerIpcFrame {
  queue.read(HEADER_BYTES);
  let json: string;
  try {
    json = textDecoder.decode(queue.read(header.jsonBytes));
  } catch {
    throw new GeometerIpcProtocolError("IPC JSON is not valid UTF-8.");
  }
  const attachments: GeometerIpcAttachment[] = [];
  const names = new Set<string>();
  let attachmentBytesRead = 0;
  for (let index = 0; index < header.attachmentCount; index += 1) {
    const attachmentHeader = queue.read(16);
    attachmentBytesRead += 16;
    const view = new DataView(
      attachmentHeader.buffer,
      attachmentHeader.byteOffset,
      attachmentHeader.byteLength,
    );
    const nameBytes = view.getUint16(0, true);
    const mediaTypeBytes = view.getUint16(2, true);
    const flags = view.getUint32(4, true);
    const dataBytes = safeUint64(view.getBigUint64(8, true), "attachment data");
    if (
      flags !== 0 ||
      nameBytes === 0 ||
      nameBytes > limits.attachmentNameBytes ||
      mediaTypeBytes === 0 ||
      mediaTypeBytes > limits.attachmentMediaTypeBytes ||
      dataBytes > limits.attachmentBytes
    ) {
      throw new GeometerIpcProtocolError("IPC attachment header is invalid.");
    }
    const payloadBytes = checkedAdd(nameBytes + mediaTypeBytes, dataBytes, "IPC attachment");
    attachmentBytesRead = checkedAdd(attachmentBytesRead, payloadBytes, "IPC attachment section");
    if (attachmentBytesRead > header.attachmentBytes) {
      throw new GeometerIpcProtocolError("IPC attachment exceeds its declared section.");
    }
    let name: string;
    let mediaType: string;
    try {
      name = textDecoder.decode(queue.read(nameBytes));
      mediaType = textDecoder.decode(queue.read(mediaTypeBytes));
    } catch {
      throw new GeometerIpcProtocolError("IPC attachment text is not valid UTF-8.");
    }
    if (names.has(name)) throw new GeometerIpcProtocolError(`Duplicate IPC attachment ${name}.`);
    names.add(name);
    const data = queue.read(dataBytes);
    attachments.push({ data, mediaType, name });
  }
  if (attachmentBytesRead !== header.attachmentBytes) {
    throw new GeometerIpcProtocolError("IPC attachment section length does not match its header.");
  }
  return { attachments, json, kind: header.kind, requestId: header.requestId };
}

class ByteQueue {
  private readonly chunks: Uint8Array[] = [];
  private headIndex = 0;
  private headOffset = 0;
  byteLength = 0;

  push(chunk: Uint8Array): void {
    if (chunk.byteLength === 0) return;
    this.chunks.push(chunk);
    this.byteLength = checkedAdd(this.byteLength, chunk.byteLength, "IPC stream buffer");
  }

  peek(length: number): Uint8Array {
    return this.collect(length, false);
  }

  read(length: number): Uint8Array {
    return this.collect(length, true);
  }

  private collect(length: number, consume: boolean): Uint8Array {
    if (!Number.isSafeInteger(length) || length < 0 || length > this.byteLength) {
      throw new GeometerIpcProtocolError("IPC byte queue underflow.");
    }
    if (length === 0) return new Uint8Array(0);
    const first = this.chunks[this.headIndex];
    if (first === undefined) throw new GeometerIpcProtocolError("IPC byte queue underflow.");
    const available = first.byteLength - this.headOffset;
    if (available >= length) {
      const output = first.subarray(this.headOffset, this.headOffset + length);
      if (consume) this.discard(length);
      return output;
    }

    const output = new Uint8Array(length);
    let copied = 0;
    let chunkIndex = this.headIndex;
    let offset = this.headOffset;
    while (copied < length) {
      const chunk = this.chunks[chunkIndex];
      if (chunk === undefined) throw new GeometerIpcProtocolError("IPC byte queue underflow.");
      const count = Math.min(chunk.byteLength - offset, length - copied);
      output.set(chunk.subarray(offset, offset + count), copied);
      copied += count;
      chunkIndex += 1;
      offset = 0;
    }
    if (consume) this.discard(length);
    return output;
  }

  private discard(length: number): void {
    let remaining = length;
    while (remaining !== 0) {
      const first = this.chunks[this.headIndex];
      if (first === undefined) throw new GeometerIpcProtocolError("IPC byte queue underflow.");
      const available = first.byteLength - this.headOffset;
      if (remaining < available) {
        this.headOffset += remaining;
        remaining = 0;
      } else {
        remaining -= available;
        this.headIndex += 1;
        this.headOffset = 0;
      }
    }
    this.byteLength -= length;
    if (this.headIndex === this.chunks.length) {
      this.chunks.length = 0;
      this.headIndex = 0;
    } else if (this.headIndex >= 1024 && this.headIndex * 2 >= this.chunks.length) {
      this.chunks.splice(0, this.headIndex);
      this.headIndex = 0;
    }
  }
}

function validateKindAndRequestId(kind: GeometerIpcFrameKind, requestId: bigint): void {
  if (!Number.isInteger(kind) || kind < 1 || kind > 10) {
    throw new GeometerIpcProtocolError("Unknown IPC frame kind.");
  }
  if (requestId < 0n || requestId > 0xffff_ffff_ffff_ffffn) {
    throw new GeometerIpcProtocolError("IPC request id is outside uint64.");
  }
  const requiresZero = kind === 1 || kind === 2 || kind === 8 || kind === 9;
  const requiresNonzero = kind >= 3 && kind <= 7;
  if ((requiresZero && requestId !== 0n) || (requiresNonzero && requestId === 0n)) {
    throw new GeometerIpcProtocolError("IPC request id does not match the frame kind.");
  }
}

function validateAttachmentDirection(kind: GeometerIpcFrameKind, count: number): void {
  if (count !== 0 && kind !== 3 && kind !== 4) {
    throw new GeometerIpcProtocolError("IPC control frames cannot carry attachments.");
  }
}

function safeUint64(value: bigint, field: string): number {
  if (value > BigInt(Number.MAX_SAFE_INTEGER)) {
    throw new GeometerIpcProtocolError(`IPC ${field} exceeds safe integer range.`);
  }
  return Number(value);
}

function checkedAdd(left: number, right: number, field: string): number {
  const value = left + right;
  if (!Number.isSafeInteger(value)) {
    throw new GeometerIpcProtocolError(`${field} size overflow.`);
  }
  return value;
}

function operationDeclaration(operation: string) {
  if (!Object.hasOwn(operationCatalog, operation)) {
    throw new GeometerIpcProtocolError(`Unknown IPC operation ${operation}.`);
  }
  return operationCatalog[operation as OperationIdentity];
}

function validatePayloadProjection(
  value: object,
  runtimeDispatch: "logical_dto" | "packed_attachment",
  expectedProjection?: {
    readonly attachment_name: string;
    readonly format: string;
    readonly kind: "packed_attachment";
  },
  fallback?: string,
): string {
  const hasPacket = "packet" in value;
  if (runtimeDispatch === "logical_dto") {
    if (hasPacket) {
      throw new GeometerIpcProtocolError(
        "A logical DTO operation cannot carry a packed attachment projection.",
      );
    }
  } else {
    if (!hasPacket || expectedProjection === undefined) {
      throw new GeometerIpcProtocolError("A packed operation requires its declared projection.");
    }
    const packet = value.packet;
    if (
      typeof packet !== "object" ||
      packet === null ||
      !("attachment" in packet) ||
      packet.attachment !== expectedProjection.attachment_name ||
      !("format" in packet) ||
      packet.format !== expectedProjection.format
    ) {
      throw new GeometerIpcProtocolError(
        "Packed attachment projection metadata differs from the operation declaration.",
      );
    }
  }
  if ("schema" in value && typeof value.schema === "string") return value.schema;
  if (fallback !== undefined) return fallback;
  throw new GeometerIpcProtocolError("IPC payload has no contract schema identity.");
}

import type { IpcRequestA0, OperationOutcomeA0 } from "./generated/contracts.js";
import { type OperationIdentity, operationCatalog } from "./generated/operations.js";
