import { decodeAnalyticPlanarBooleanBatchResultA0Packet, encodeAnalyticPlanarBooleanBatchRequestA0Packet, } from "./analytic-packet-a0.js";
import { decodeOperationOutcomeA0Json, encodeModelBoundsOptionsA0Json, operationCatalog, } from "./generated/index.js";
export class GeometerWasmTransportError extends Error {
    code;
    constructor(code, message) {
        super(message);
        this.name = "GeometerWasmTransportError";
        this.code = code;
    }
}
export class GeometerOperationError extends Error {
    diagnostics;
    operation;
    constructor(operation, diagnostics) {
        super(diagnostics.map((diagnostic) => diagnostic.message).join("; ") || `${operation} failed.`);
        this.name = "GeometerOperationError";
        this.operation = operation;
        this.diagnostics = diagnostics;
    }
}
export class GeometerWasmClient {
    capabilities;
    module;
    runtimeCatalog;
    constructor(module, runtimeCatalog) {
        this.module = module;
        this.runtimeCatalog = runtimeCatalog;
        this.capabilities = Object.freeze({
            cAbiGeneration: runtimeCatalog.c_abi_generation,
            genericAbi: "a0",
            releaseVersion: runtimeCatalog.release_version,
            operations: Object.freeze(runtimeCatalog.operations.map((operation) => operation.identity)),
        });
    }
    static fromModule(module) {
        assertModuleSurface(module);
        const runtimeCatalog = readRuntimeCatalog(module);
        validateRuntimeCatalog(runtimeCatalog);
        return new GeometerWasmClient(module, runtimeCatalog);
    }
    async analyticPlanarBooleanBatch(request) {
        const declaration = operationCatalog["geometry.analytic_planar_boolean_batch.a0"];
        const requestProjection = declaration.requestProjection;
        const resultProjection = declaration.resultProjection;
        const input = requiredAttachment(declaration.inputAttachments, requestProjection.attachment_name);
        const output = requiredAttachment(declaration.outputAttachments, resultProjection.attachment_name);
        const inputMediaType = requiredMediaType(input.media_types, input.name);
        const packet = encodeAnalyticPlanarBooleanBatchRequestA0Packet(request);
        const response = this.execute(declaration.identity, JSON.stringify({
            schema: declaration.requestContract,
            packet: {
                attachment: requestProjection.attachment_name,
                format: requestProjection.format,
            },
        }), [{ name: input.name, mediaType: inputMediaType, data: packet }]);
        if (!response.outcome.ok) {
            throw new GeometerOperationError(response.outcome.operation, response.outcome.diagnostics);
        }
        const attachment = response.attachments.find((item) => item.name === output.name);
        if (attachment === undefined) {
            throw new GeometerWasmTransportError(0, "Packed analytic result attachment is missing.");
        }
        return decodeAnalyticPlanarBooleanBatchResultA0Packet(attachment.data);
    }
    async modelBounds(request) {
        const response = this.execute("geometry.model_bounds.a0", encodeModelBoundsOptionsA0Json(request.options ?? {}), [
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
            throw new GeometerWasmTransportError(0, `Expected geometry.model_bounds.a0 response, received ${response.outcome.operation}.`);
        }
        if (response.attachments.length !== 0) {
            throw new GeometerWasmTransportError(0, "model_bounds returned unexpected attachments.");
        }
        if (!("units" in response.outcome.result)) {
            throw new GeometerWasmTransportError(0, "model_bounds returned an incompatible result DTO.");
        }
        return response.outcome.result;
    }
    execute(operation, requestJson, attachments) {
        const declaration = this.runtimeCatalog.operations.find((item) => item.identity === operation);
        if (!declaration) {
            throw new GeometerWasmTransportError(0, `WASM module does not support ${operation}.`);
        }
        validateInputAttachments(declaration, attachments);
        return executeOperation(this.module, this.runtimeCatalog, operation, requestJson, attachments);
    }
}
export async function createGeometerWasmClient(moduleOrFactory, moduleOptions) {
    const module = typeof moduleOrFactory === "function" ? await moduleOrFactory(moduleOptions) : moduleOrFactory;
    return GeometerWasmClient.fromModule(module);
}
function executeOperation(module, catalog, operation, requestJson, attachments) {
    const allocations = [];
    let resultPointer = 0;
    let localErrorPointer = 0;
    try {
        const operationBytes = encodeText(operation);
        const requestBytes = encodeText(requestJson);
        const operationPointer = allocate(module, operationBytes, allocations);
        const requestPointer = allocate(module, requestBytes, allocations);
        const descriptorSize = catalog.attachment_descriptor.wasm32.size;
        const descriptorPointer = allocateViewSize(module, descriptorSize * attachments.length, allocations);
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
        const code = module._geometer_operation_execute(operationPointer, operationBytes.byteLength, requestPointer, requestBytes.byteLength, descriptorPointer, attachments.length, resultOut, errorOut);
        resultPointer = readU32(module, resultOut);
        localErrorPointer = readU32(module, errorOut);
        if (code !== 0 || resultPointer === 0 || localErrorPointer !== 0) {
            const detail = localErrorPointer
                ? module.UTF8ToString(localErrorPointer)
                : "No local detail.";
            throw new GeometerWasmTransportError(code, `Geometer generic ABI failed (${code}): ${detail}`);
        }
        const jsonPointer = module._geometer_operation_result_json_data(resultPointer);
        const jsonSize = module._geometer_operation_result_json_size(resultPointer);
        const jsonBytes = copyBytes(module, jsonPointer, jsonSize, "response JSON");
        const outcome = decodeOperationOutcomeA0Json(jsonBytes);
        const outputAttachments = copyResultAttachments(module, resultPointer, allocations);
        const declaration = catalog.operations.find((item) => item.identity === operation);
        if (declaration === undefined) {
            throw new GeometerWasmTransportError(0, `Operation ${operation} disappeared during execution.`);
        }
        validateOutputAttachments(declaration, outputAttachments, outcome);
        return { attachments: outputAttachments, outcome };
    }
    finally {
        if (localErrorPointer)
            module._geometer_free_string(localErrorPointer);
        if (resultPointer)
            module._geometer_operation_result_free(resultPointer);
        for (const pointer of allocations.reverse())
            module._free(pointer);
    }
}
function copyResultAttachments(module, resultPointer, allocations) {
    const count = module._geometer_operation_result_attachment_count(resultPointer);
    const sizeOut = allocateSize(module, 4, allocations);
    const output = [];
    for (let index = 0; index < count; index += 1) {
        const namePointer = module._geometer_operation_result_attachment_name(resultPointer, index, sizeOut);
        const nameSize = readU32(module, sizeOut);
        const name = decodeText(copyBytes(module, namePointer, nameSize, "attachment name"));
        const mediaTypePointer = module._geometer_operation_result_attachment_media_type(resultPointer, index, sizeOut);
        const mediaTypeSize = readU32(module, sizeOut);
        const mediaType = decodeText(copyBytes(module, mediaTypePointer, mediaTypeSize, "attachment media type"));
        const dataPointer = module._geometer_operation_result_attachment_data(resultPointer, index, sizeOut);
        const dataSize = readU32(module, sizeOut);
        output.push({
            data: copyBytes(module, dataPointer, dataSize, "attachment data"),
            mediaType,
            name,
        });
    }
    return output;
}
function readRuntimeCatalog(module) {
    const valueOut = module._malloc(4);
    const errorOut = module._malloc(4);
    if (!valueOut || !errorOut) {
        if (valueOut)
            module._free(valueOut);
        if (errorOut)
            module._free(errorOut);
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
        }
        catch {
            throw new GeometerWasmTransportError(0, "Operation catalog is not valid JSON.");
        }
    }
    finally {
        if (valuePointer)
            module._geometer_free_string(valuePointer);
        if (errorPointer)
            module._geometer_free_string(errorPointer);
        module._free(valueOut);
        module._free(errorOut);
    }
}
function validateRuntimeCatalog(value) {
    if (!isRecord(value) ||
        value.catalog !== "wn.geometer.operation_catalog.a0" ||
        value.generic_abi !== "a0" ||
        !Number.isSafeInteger(value.c_abi_generation) ||
        typeof value.release_version !== "string" ||
        !Array.isArray(value.operations)) {
        throw new GeometerWasmTransportError(0, "WASM module exposes an incompatible operation catalog.");
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
    if (!isRecord(layout) ||
        layout.size !== 36 ||
        JSON.stringify(layout.offsets) !== JSON.stringify(requiredOffsets)) {
        throw new GeometerWasmTransportError(0, "WASM module uses an incompatible wasm32 descriptor layout.");
    }
    if (!value.operations.every(isRuntimeOperation)) {
        throw new GeometerWasmTransportError(0, "WASM module operation declarations are malformed.");
    }
    for (const [identity, expected] of Object.entries(operationCatalog)) {
        if (!expected.runtimeAvailable)
            continue;
        const actual = value.operations.find((operation) => operation.identity === identity);
        if (!actual)
            throw new GeometerWasmTransportError(0, `WASM module is missing ${identity}.`);
        if (actual.request_contract !== expected.requestContract ||
            actual.result_contract !== expected.resultContract ||
            actual.runtime_dispatch !== expected.runtimeDispatch ||
            !samePackedProjection(actual.request_projection, "requestProjection" in expected ? expected.requestProjection : undefined) ||
            !samePackedProjection(actual.result_projection, "resultProjection" in expected ? expected.resultProjection : undefined)) {
            throw new GeometerWasmTransportError(0, `WASM module contract declaration for ${identity} is incompatible.`);
        }
        if (!sameAttachmentDeclarations(actual.input_attachments, expected.inputAttachments) ||
            !sameAttachmentDeclarations(actual.output_attachments, expected.outputAttachments)) {
            throw new GeometerWasmTransportError(0, `WASM module attachment inventory for ${identity} is incompatible.`);
        }
    }
}
function samePackedProjection(actual, expected) {
    return ((actual === undefined && expected === undefined) ||
        (actual !== undefined &&
            expected !== undefined &&
            actual.kind === expected.kind &&
            actual.attachment_name === expected.attachment_name &&
            actual.format === expected.format));
}
function isRuntimeOperation(value) {
    if (!isRecord(value) ||
        typeof value.identity !== "string" ||
        typeof value.request_contract !== "string" ||
        typeof value.result_contract !== "string" ||
        (value.runtime_dispatch !== "logical_dto" && value.runtime_dispatch !== "packed_attachment") ||
        !Array.isArray(value.input_attachments) ||
        !Array.isArray(value.output_attachments)) {
        return false;
    }
    if (value.runtime_dispatch === "packed_attachment" &&
        (!isRuntimePackedProjection(value.request_projection) ||
            !isRuntimePackedProjection(value.result_projection))) {
        return false;
    }
    return (value.input_attachments.every(isRuntimeAttachment) &&
        value.output_attachments.every(isRuntimeAttachment));
}
function isRuntimePackedProjection(value) {
    return (isRecord(value) &&
        value.kind === "packed_attachment" &&
        typeof value.attachment_name === "string" &&
        typeof value.format === "string");
}
function isRuntimeAttachment(value) {
    return (isRecord(value) &&
        typeof value.name === "string" &&
        typeof value.required === "boolean" &&
        Number.isSafeInteger(value.max_bytes) &&
        Array.isArray(value.media_types) &&
        value.media_types.every((mediaType) => typeof mediaType === "string"));
}
function sameAttachmentDeclarations(actual, expected) {
    return (actual.length === expected.length &&
        actual.every((attachment, index) => {
            const expectedAttachment = expected[index];
            return (expectedAttachment !== undefined &&
                attachment.name === expectedAttachment.name &&
                attachment.required === expectedAttachment.required &&
                attachment.max_bytes === expectedAttachment.max_bytes &&
                JSON.stringify(attachment.media_types) === JSON.stringify(expectedAttachment.media_types));
        }));
}
function isRecord(value) {
    return typeof value === "object" && value !== null && !Array.isArray(value);
}
function requiredAttachment(attachments, name) {
    const attachment = attachments.find((item) => item.name === name);
    if (attachment === undefined) {
        throw new GeometerWasmTransportError(0, `Generated attachment ${name} is missing.`);
    }
    return attachment;
}
function requiredMediaType(mediaTypes, attachment) {
    const mediaType = mediaTypes[0];
    if (mediaType === undefined) {
        throw new GeometerWasmTransportError(0, `Generated attachment ${attachment} has no media type.`);
    }
    return mediaType;
}
function validateInputAttachments(operation, attachments) {
    const names = new Set();
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
            throw new GeometerWasmTransportError(1001, `Attachment ${attachment.name} does not accept ${attachment.mediaType}.`);
        }
        if (attachment.data.byteLength > declaration.max_bytes) {
            throw new GeometerWasmTransportError(1002, `Attachment ${attachment.name} exceeds ${declaration.max_bytes} bytes.`);
        }
    }
    for (const declaration of operation.input_attachments) {
        if (declaration.required && !names.has(declaration.name)) {
            throw new GeometerWasmTransportError(1001, `Required attachment ${declaration.name} is missing.`);
        }
    }
}
function validateOutputAttachments(operation, attachments, outcome) {
    if (outcome.operation !== operation.identity) {
        throw new GeometerWasmTransportError(0, "Operation response identity does not match its request.");
    }
    if (!outcome.ok) {
        if (attachments.length !== 0) {
            throw new GeometerWasmTransportError(0, "A failed operation returned attachments.");
        }
        return;
    }
    const names = new Set();
    for (const attachment of attachments) {
        if (names.has(attachment.name)) {
            throw new GeometerWasmTransportError(0, `Duplicate output attachment ${attachment.name}.`);
        }
        names.add(attachment.name);
        const declaration = operation.output_attachments.find((item) => item.name === attachment.name);
        if (declaration === undefined ||
            !declaration.media_types.includes(attachment.mediaType) ||
            attachment.data.byteLength > declaration.max_bytes) {
            throw new GeometerWasmTransportError(0, `Output attachment ${attachment.name} is incompatible.`);
        }
    }
    for (const declaration of operation.output_attachments) {
        if (declaration.required && !names.has(declaration.name)) {
            throw new GeometerWasmTransportError(0, `Required output attachment ${declaration.name} is missing.`);
        }
    }
    if (operation.runtime_dispatch === "packed_attachment") {
        const projection = operation.result_projection;
        const result = outcome.result;
        if (projection === undefined ||
            !("packet" in result) ||
            result.schema !== operation.result_contract ||
            result.packet.attachment !== projection.attachment_name ||
            result.packet.format !== projection.format ||
            !names.has(projection.attachment_name)) {
            throw new GeometerWasmTransportError(0, "Packed result metadata does not match the catalog.");
        }
    }
}
function assertModuleSurface(module) {
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
    ]) {
        if (typeof module[name] !== "function") {
            throw new GeometerWasmTransportError(0, `WASM module is missing ${name}.`);
        }
    }
}
function allocate(module, bytes, allocations) {
    const pointer = allocateViewSize(module, bytes.byteLength, allocations);
    if (pointer)
        module.HEAPU8.set(bytes, pointer);
    return pointer;
}
function allocateViewSize(module, size, allocations) {
    return size === 0 ? 0 : allocateSize(module, size, allocations);
}
function allocateSize(module, size, allocations) {
    const pointer = module._malloc(size);
    if (!pointer)
        throw new GeometerWasmTransportError(1003, `Unable to allocate ${size} WASM bytes.`);
    allocations.push(pointer);
    return pointer;
}
function writeDescriptor(module, base, offsets, values) {
    for (const [name, offsetName] of [
        ["structSize", "struct_size"],
        ["namePointer", "name"],
        ["nameSize", "name_size"],
        ["mediaTypePointer", "media_type"],
        ["mediaTypeSize", "media_type_size"],
        ["dataPointer", "data"],
        ["dataSize", "data_size"],
    ]) {
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
function copyBytes(module, pointer, size, label) {
    if ((!pointer && size) || pointer + size > module.HEAPU8.byteLength) {
        throw new GeometerWasmTransportError(0, `Invalid ${label} view from WASM.`);
    }
    return module.HEAPU8.slice(pointer, pointer + size);
}
function writeU32(module, pointer, value) {
    module.HEAPU32[pointer >>> 2] = value >>> 0;
}
function readU32(module, pointer) {
    return module.HEAPU32[pointer >>> 2] ?? 0;
}
function encodeText(value) {
    return new TextEncoder().encode(value);
}
function decodeText(value) {
    return new TextDecoder("utf-8", { fatal: true }).decode(value);
}
