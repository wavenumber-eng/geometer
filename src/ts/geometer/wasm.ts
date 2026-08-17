import type {
  DiagnosticA0,
  ModelBoundsInputMediaType,
  ModelBoundsOptionsA0,
  ModelBoundsResultA0,
  OperationOutcomeA0,
} from "./generated/index.js";
import {
  decodeOperationOutcomeA0Json,
  encodeModelBoundsOptionsA0Json,
  operationCatalog,
} from "./generated/index.js";

export interface EmscriptenGeometerModule {
  readonly HEAPU8: Uint8Array;
  readonly HEAPU32: Uint32Array;
  UTF8ToString(pointer: number): string;
  _free(pointer: number): void;
  _geometer_free_string(pointer: number): void;
  _geometer_operation_catalog_json(valueOut: number, errorOut: number): number;
  _geometer_operation_execute(
    operationId: number,
    operationIdSize: number,
    requestJson: number,
    requestJsonSize: number,
    attachments: number,
    attachmentCount: number,
    resultOut: number,
    errorOut: number,
  ): number;
  _geometer_operation_result_attachment_count(result: number): number;
  _geometer_operation_result_attachment_data(
    result: number,
    index: number,
    sizeOut: number,
  ): number;
  _geometer_operation_result_attachment_media_type(
    result: number,
    index: number,
    sizeOut: number,
  ): number;
  _geometer_operation_result_attachment_name(
    result: number,
    index: number,
    sizeOut: number,
  ): number;
  _geometer_operation_result_free(result: number): void;
  _geometer_operation_result_json_data(result: number): number;
  _geometer_operation_result_json_size(result: number): number;
  _malloc(size: number): number;
}

export type EmscriptenGeometerFactory = (
  options?: Readonly<Record<string, unknown>>,
) => Promise<EmscriptenGeometerModule>;

export interface GeometerOperationAttachment {
  readonly data: Uint8Array;
  readonly mediaType: string;
  readonly name: string;
}

export interface GeometerOperationResponse {
  readonly attachments: readonly GeometerOperationAttachment[];
  readonly outcome: OperationOutcomeA0;
}

export interface ModelBoundsRequest {
  readonly mediaType?: ModelBoundsInputMediaType;
  readonly model: Uint8Array;
  readonly options?: ModelBoundsOptionsA0;
}

export interface GeometerWasmCapabilityCatalog {
  readonly cAbiGeneration: number;
  readonly genericAbi: "a0";
  readonly releaseVersion: string;
  readonly operations: readonly string[];
}

interface RuntimeCatalog {
  readonly attachment_descriptor: {
    readonly wasm32: {
      readonly offsets: Readonly<Record<string, number>>;
      readonly size: number;
    };
  };
  readonly c_abi_generation: number;
  readonly catalog: string;
  readonly generic_abi: string;
  readonly operations: readonly {
    readonly identity: string;
    readonly input_attachments: readonly {
      readonly max_bytes: number;
      readonly media_types: readonly string[];
      readonly name: string;
      readonly required: boolean;
    }[];
    readonly output_attachments: readonly {
      readonly max_bytes: number;
      readonly media_types: readonly string[];
      readonly name: string;
      readonly required: boolean;
    }[];
    readonly request_contract: string;
    readonly request_projection?: RuntimePackedProjection;
    readonly result_contract: string;
    readonly result_projection?: RuntimePackedProjection;
    readonly runtime_dispatch: "logical_dto" | "packed_attachment";
  }[];
  readonly release_version: string;
}

interface RuntimePackedProjection {
  readonly attachment_name: string;
  readonly format: string;
  readonly kind: "packed_attachment";
}

export class GeometerWasmTransportError extends Error {
  readonly code: number;

  constructor(code: number, message: string) {
    super(message);
    this.name = "GeometerWasmTransportError";
    this.code = code;
  }
}

export class GeometerOperationError extends Error {
  readonly diagnostics: readonly DiagnosticA0[];
  readonly operation: string;

  constructor(operation: string, diagnostics: readonly DiagnosticA0[]) {
    super(diagnostics.map((diagnostic) => diagnostic.message).join("; ") || `${operation} failed.`);
    this.name = "GeometerOperationError";
    this.operation = operation;
    this.diagnostics = diagnostics;
  }
}

export class GeometerWasmClient {
  readonly capabilities: GeometerWasmCapabilityCatalog;
  private readonly module: EmscriptenGeometerModule;
  private readonly runtimeCatalog: RuntimeCatalog;

  private constructor(module: EmscriptenGeometerModule, runtimeCatalog: RuntimeCatalog) {
    this.module = module;
    this.runtimeCatalog = runtimeCatalog;
    this.capabilities = Object.freeze({
      cAbiGeneration: runtimeCatalog.c_abi_generation,
      genericAbi: "a0" as const,
      releaseVersion: runtimeCatalog.release_version,
      operations: Object.freeze(runtimeCatalog.operations.map((operation) => operation.identity)),
    });
  }

  static fromModule(module: EmscriptenGeometerModule): GeometerWasmClient {
    assertModuleSurface(module);
    const runtimeCatalog = readRuntimeCatalog(module);
    validateRuntimeCatalog(runtimeCatalog);
    return new GeometerWasmClient(module, runtimeCatalog);
  }

  async modelBounds(request: ModelBoundsRequest): Promise<ModelBoundsResultA0> {
    const response = this.execute(
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
      throw new GeometerWasmTransportError(
        0,
        `Expected geometry.model_bounds.a0 response, received ${response.outcome.operation}.`,
      );
    }
    if (response.attachments.length !== 0) {
      throw new GeometerWasmTransportError(0, "model_bounds returned unexpected attachments.");
    }
    if (!("units" in response.outcome.result)) {
      throw new GeometerWasmTransportError(0, "model_bounds returned an incompatible result DTO.");
    }
    return response.outcome.result;
  }

  execute(
    operation: string,
    requestJson: string,
    attachments: readonly GeometerOperationAttachment[],
  ): GeometerOperationResponse {
    const declaration = this.runtimeCatalog.operations.find((item) => item.identity === operation);
    if (!declaration) {
      throw new GeometerWasmTransportError(0, `WASM module does not support ${operation}.`);
    }
    validateInputAttachments(declaration, attachments);
    return executeOperation(this.module, this.runtimeCatalog, operation, requestJson, attachments);
  }
}

export async function createGeometerWasmClient(
  moduleOrFactory: EmscriptenGeometerModule | EmscriptenGeometerFactory,
  moduleOptions?: Readonly<Record<string, unknown>>,
): Promise<GeometerWasmClient> {
  const module =
    typeof moduleOrFactory === "function" ? await moduleOrFactory(moduleOptions) : moduleOrFactory;
  return GeometerWasmClient.fromModule(module);
}

function executeOperation(
  module: EmscriptenGeometerModule,
  catalog: RuntimeCatalog,
  operation: string,
  requestJson: string,
  attachments: readonly GeometerOperationAttachment[],
): GeometerOperationResponse {
  const allocations: number[] = [];
  let resultPointer = 0;
  let localErrorPointer = 0;
  try {
    const operationBytes = encodeText(operation);
    const requestBytes = encodeText(requestJson);
    const operationPointer = allocate(module, operationBytes, allocations);
    const requestPointer = allocate(module, requestBytes, allocations);
    const descriptorSize = catalog.attachment_descriptor.wasm32.size;
    const descriptorPointer = allocateViewSize(
      module,
      descriptorSize * attachments.length,
      allocations,
    );
    const offsets = catalog.attachment_descriptor.wasm32.offsets;
    attachments.forEach((attachment, index) => {
      const name = encodeText(attachment.name);
      const mediaType = encodeText(attachment.mediaType);
      const namePointer = allocate(module, name, allocations);
      const mediaTypePointer = allocate(module, mediaType, allocations);
      const dataPointer = allocate(module, attachment.data, allocations);
      const base = descriptorPointer + index * descriptorSize;
      writeDescriptor(module, base, offsets, {
        dataPointer,
        dataSize: attachment.data.byteLength,
        mediaTypePointer,
        mediaTypeSize: mediaType.byteLength,
        namePointer,
        nameSize: name.byteLength,
        structSize: descriptorSize,
      });
    });
    const resultOut = allocateSize(module, 4, allocations);
    const errorOut = allocateSize(module, 4, allocations);
    writeU32(module, resultOut, 0);
    writeU32(module, errorOut, 0);
    const code = module._geometer_operation_execute(
      operationPointer,
      operationBytes.byteLength,
      requestPointer,
      requestBytes.byteLength,
      descriptorPointer,
      attachments.length,
      resultOut,
      errorOut,
    );
    resultPointer = readU32(module, resultOut);
    localErrorPointer = readU32(module, errorOut);
    if (code !== 0 || resultPointer === 0 || localErrorPointer !== 0) {
      const detail = localErrorPointer
        ? module.UTF8ToString(localErrorPointer)
        : "No local detail.";
      throw new GeometerWasmTransportError(
        code,
        `Geometer generic ABI failed (${code}): ${detail}`,
      );
    }
    const jsonPointer = module._geometer_operation_result_json_data(resultPointer);
    const jsonSize = module._geometer_operation_result_json_size(resultPointer);
    const jsonBytes = copyBytes(module, jsonPointer, jsonSize, "response JSON");
    const outcome = decodeOperationOutcomeA0Json(jsonBytes);
    const outputAttachments = copyResultAttachments(module, resultPointer, allocations);
    return { attachments: outputAttachments, outcome };
  } finally {
    if (localErrorPointer) module._geometer_free_string(localErrorPointer);
    if (resultPointer) module._geometer_operation_result_free(resultPointer);
    for (const pointer of allocations.reverse()) module._free(pointer);
  }
}

function copyResultAttachments(
  module: EmscriptenGeometerModule,
  resultPointer: number,
  allocations: number[],
): readonly GeometerOperationAttachment[] {
  const count = module._geometer_operation_result_attachment_count(resultPointer);
  const sizeOut = allocateSize(module, 4, allocations);
  const output: GeometerOperationAttachment[] = [];
  for (let index = 0; index < count; index += 1) {
    const namePointer = module._geometer_operation_result_attachment_name(
      resultPointer,
      index,
      sizeOut,
    );
    const nameSize = readU32(module, sizeOut);
    const name = decodeText(copyBytes(module, namePointer, nameSize, "attachment name"));
    const mediaTypePointer = module._geometer_operation_result_attachment_media_type(
      resultPointer,
      index,
      sizeOut,
    );
    const mediaTypeSize = readU32(module, sizeOut);
    const mediaType = decodeText(
      copyBytes(module, mediaTypePointer, mediaTypeSize, "attachment media type"),
    );
    const dataPointer = module._geometer_operation_result_attachment_data(
      resultPointer,
      index,
      sizeOut,
    );
    const dataSize = readU32(module, sizeOut);
    output.push({
      data: copyBytes(module, dataPointer, dataSize, "attachment data"),
      mediaType,
      name,
    });
  }
  return output;
}

function readRuntimeCatalog(module: EmscriptenGeometerModule): unknown {
  const valueOut = module._malloc(4);
  const errorOut = module._malloc(4);
  if (!valueOut || !errorOut) {
    if (valueOut) module._free(valueOut);
    if (errorOut) module._free(errorOut);
    throw new GeometerWasmTransportError(1003, "Unable to allocate catalog output slots.");
  }
  let valuePointer = 0;
  let errorPointer = 0;
  try {
    writeU32(module, valueOut, 0);
    writeU32(module, errorOut, 0);
    const code = module._geometer_operation_catalog_json(valueOut, errorOut);
    valuePointer = readU32(module, valueOut);
    errorPointer = readU32(module, errorOut);
    if (code !== 0 || !valuePointer || errorPointer) {
      const detail = errorPointer ? module.UTF8ToString(errorPointer) : "No local detail.";
      throw new GeometerWasmTransportError(code, `Operation catalog lookup failed: ${detail}`);
    }
    try {
      return JSON.parse(module.UTF8ToString(valuePointer));
    } catch {
      throw new GeometerWasmTransportError(0, "Operation catalog is not valid JSON.");
    }
  } finally {
    if (valuePointer) module._geometer_free_string(valuePointer);
    if (errorPointer) module._geometer_free_string(errorPointer);
    module._free(valueOut);
    module._free(errorOut);
  }
}

function validateRuntimeCatalog(value: unknown): asserts value is RuntimeCatalog {
  if (
    !isRecord(value) ||
    value.catalog !== "wn.geometer.operation_catalog.a0" ||
    value.generic_abi !== "a0" ||
    !Number.isSafeInteger(value.c_abi_generation) ||
    typeof value.release_version !== "string" ||
    !Array.isArray(value.operations)
  ) {
    throw new GeometerWasmTransportError(
      0,
      "WASM module exposes an incompatible operation catalog.",
    );
  }
  const descriptor = value.attachment_descriptor;
  const layout = isRecord(descriptor) ? descriptor.wasm32 : undefined;
  const requiredOffsets = {
    struct_size: 0,
    flags: 4,
    name: 8,
    name_size: 12,
    media_type: 16,
    media_type_size: 20,
    data: 24,
    data_size: 28,
    reserved0: 32,
  };
  if (
    !isRecord(layout) ||
    layout.size !== 36 ||
    JSON.stringify(layout.offsets) !== JSON.stringify(requiredOffsets)
  ) {
    throw new GeometerWasmTransportError(
      0,
      "WASM module uses an incompatible wasm32 descriptor layout.",
    );
  }
  if (!value.operations.every(isRuntimeOperation)) {
    throw new GeometerWasmTransportError(0, "WASM module operation declarations are malformed.");
  }
  for (const [identity, expected] of Object.entries(operationCatalog)) {
    const actual = value.operations.find((operation) => operation.identity === identity);
    if (!actual) throw new GeometerWasmTransportError(0, `WASM module is missing ${identity}.`);
    if (
      actual.request_contract !== expected.requestContract ||
      actual.result_contract !== expected.resultContract
    ) {
      throw new GeometerWasmTransportError(
        0,
        `WASM module contract declaration for ${identity} is incompatible.`,
      );
    }
    if (
      !sameAttachmentDeclarations(actual.input_attachments, expected.inputAttachments) ||
      !sameAttachmentDeclarations(actual.output_attachments, expected.outputAttachments)
    ) {
      throw new GeometerWasmTransportError(
        0,
        `WASM module attachment inventory for ${identity} is incompatible.`,
      );
    }
  }
}

function isRuntimeOperation(value: unknown): value is RuntimeCatalog["operations"][number] {
  if (
    !isRecord(value) ||
    typeof value.identity !== "string" ||
    typeof value.request_contract !== "string" ||
    typeof value.result_contract !== "string" ||
    (value.runtime_dispatch !== "logical_dto" && value.runtime_dispatch !== "packed_attachment") ||
    !Array.isArray(value.input_attachments) ||
    !Array.isArray(value.output_attachments)
  ) {
    return false;
  }
  if (
    value.runtime_dispatch === "packed_attachment" &&
    (!isRuntimePackedProjection(value.request_projection) ||
      !isRuntimePackedProjection(value.result_projection))
  ) {
    return false;
  }
  return (
    value.input_attachments.every(isRuntimeAttachment) &&
    value.output_attachments.every(isRuntimeAttachment)
  );
}

function isRuntimePackedProjection(value: unknown): value is RuntimePackedProjection {
  return (
    isRecord(value) &&
    value.kind === "packed_attachment" &&
    typeof value.attachment_name === "string" &&
    typeof value.format === "string"
  );
}

function isRuntimeAttachment(value: unknown): boolean {
  return (
    isRecord(value) &&
    typeof value.name === "string" &&
    typeof value.required === "boolean" &&
    Number.isSafeInteger(value.max_bytes) &&
    Array.isArray(value.media_types) &&
    value.media_types.every((mediaType) => typeof mediaType === "string")
  );
}

function sameAttachmentDeclarations(
  actual: RuntimeCatalog["operations"][number]["input_attachments"],
  expected: readonly {
    readonly max_bytes: number;
    readonly media_types: readonly string[];
    readonly name: string;
    readonly required: boolean;
  }[],
): boolean {
  return (
    actual.length === expected.length &&
    actual.every((attachment, index) => {
      const expectedAttachment = expected[index];
      return (
        expectedAttachment !== undefined &&
        attachment.name === expectedAttachment.name &&
        attachment.required === expectedAttachment.required &&
        attachment.max_bytes === expectedAttachment.max_bytes &&
        JSON.stringify(attachment.media_types) === JSON.stringify(expectedAttachment.media_types)
      );
    })
  );
}

function isRecord(value: unknown): value is Record<string, unknown> {
  return typeof value === "object" && value !== null && !Array.isArray(value);
}

function validateInputAttachments(
  operation: RuntimeCatalog["operations"][number],
  attachments: readonly GeometerOperationAttachment[],
): void {
  const names = new Set<string>();
  for (const attachment of attachments) {
    if (names.has(attachment.name)) {
      throw new GeometerWasmTransportError(1001, `Duplicate attachment ${attachment.name}.`);
    }
    names.add(attachment.name);
    const declaration = operation.input_attachments.find((item) => item.name === attachment.name);
    if (!declaration) {
      throw new GeometerWasmTransportError(1001, `Attachment ${attachment.name} is not declared.`);
    }
    if (!declaration.media_types.includes(attachment.mediaType)) {
      throw new GeometerWasmTransportError(
        1001,
        `Attachment ${attachment.name} does not accept ${attachment.mediaType}.`,
      );
    }
    if (attachment.data.byteLength > declaration.max_bytes) {
      throw new GeometerWasmTransportError(
        1002,
        `Attachment ${attachment.name} exceeds ${declaration.max_bytes} bytes.`,
      );
    }
  }
  for (const declaration of operation.input_attachments) {
    if (declaration.required && !names.has(declaration.name)) {
      throw new GeometerWasmTransportError(
        1001,
        `Required attachment ${declaration.name} is missing.`,
      );
    }
  }
}

function assertModuleSurface(module: EmscriptenGeometerModule): void {
  for (const name of [
    "_malloc",
    "_free",
    "_geometer_free_string",
    "_geometer_operation_catalog_json",
    "_geometer_operation_execute",
    "_geometer_operation_result_json_data",
    "_geometer_operation_result_json_size",
    "_geometer_operation_result_attachment_count",
    "_geometer_operation_result_attachment_name",
    "_geometer_operation_result_attachment_media_type",
    "_geometer_operation_result_attachment_data",
    "_geometer_operation_result_free",
  ] as const) {
    if (typeof module[name] !== "function") {
      throw new GeometerWasmTransportError(0, `WASM module is missing ${name}.`);
    }
  }
}

function allocate(
  module: EmscriptenGeometerModule,
  bytes: Uint8Array,
  allocations: number[],
): number {
  const pointer = allocateViewSize(module, bytes.byteLength, allocations);
  if (pointer) module.HEAPU8.set(bytes, pointer);
  return pointer;
}

function allocateViewSize(
  module: EmscriptenGeometerModule,
  size: number,
  allocations: number[],
): number {
  return size === 0 ? 0 : allocateSize(module, size, allocations);
}

function allocateSize(
  module: EmscriptenGeometerModule,
  size: number,
  allocations: number[],
): number {
  const pointer = module._malloc(size);
  if (!pointer)
    throw new GeometerWasmTransportError(1003, `Unable to allocate ${size} WASM bytes.`);
  allocations.push(pointer);
  return pointer;
}

function writeDescriptor(
  module: EmscriptenGeometerModule,
  base: number,
  offsets: Readonly<Record<string, number>>,
  values: Readonly<Record<string, number>>,
): void {
  for (const [name, offsetName] of [
    ["structSize", "struct_size"],
    ["namePointer", "name"],
    ["nameSize", "name_size"],
    ["mediaTypePointer", "media_type"],
    ["mediaTypeSize", "media_type_size"],
    ["dataPointer", "data"],
    ["dataSize", "data_size"],
  ] as const) {
    const offset = offsets[offsetName];
    const value = values[name];
    if (offset === undefined || value === undefined) {
      throw new GeometerWasmTransportError(0, `Descriptor layout is missing ${offsetName}.`);
    }
    writeU32(module, base + offset, value);
  }
  writeU32(module, base + (offsets.flags ?? 4), 0);
  writeU32(module, base + (offsets.reserved0 ?? 32), 0);
}

function copyBytes(
  module: EmscriptenGeometerModule,
  pointer: number,
  size: number,
  label: string,
): Uint8Array {
  if ((!pointer && size) || pointer + size > module.HEAPU8.byteLength) {
    throw new GeometerWasmTransportError(0, `Invalid ${label} view from WASM.`);
  }
  return module.HEAPU8.slice(pointer, pointer + size);
}

function writeU32(module: EmscriptenGeometerModule, pointer: number, value: number): void {
  module.HEAPU32[pointer >>> 2] = value >>> 0;
}

function readU32(module: EmscriptenGeometerModule, pointer: number): number {
  return module.HEAPU32[pointer >>> 2] ?? 0;
}

function encodeText(value: string): Uint8Array {
  return new TextEncoder().encode(value);
}

function decodeText(value: Uint8Array): string {
  return new TextDecoder("utf-8", { fatal: true }).decode(value);
}
