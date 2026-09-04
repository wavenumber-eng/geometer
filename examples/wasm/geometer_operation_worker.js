// Classic-worker adapter for Geometer's governed generic WASM operation ABI.
// Geometry behavior remains in the production C++ operation registry.
(function installGeometerOperationWorker(global) {
  const encoder = new TextEncoder();

  function allocate(module, bytes, allocations) {
    if (bytes.byteLength === 0) return 0;
    const pointer = module._malloc(bytes.byteLength);
    if (!pointer) throw new Error(`Could not allocate ${bytes.byteLength} WASM bytes.`);
    module.HEAPU8.set(bytes, pointer);
    allocations.push(pointer);
    return pointer;
  }

  function allocateSlot(module, allocations) {
    const pointer = module._malloc(4);
    if (!pointer) throw new Error("Could not allocate a WASM output slot.");
    allocations.push(pointer);
    module.HEAPU32[pointer >>> 2] = 0;
    return pointer;
  }

  function executeOneAttachment(module, operation, request, attachment) {
    const allocations = [];
    let resultPointer = 0;
    let errorPointer = 0;
    try {
      const operationBytes = encoder.encode(operation);
      const requestBytes = encoder.encode(JSON.stringify(request));
      const nameBytes = encoder.encode(attachment.name);
      const mediaTypeBytes = encoder.encode(attachment.mediaType);
      const operationPointer = allocate(module, operationBytes, allocations);
      const requestPointer = allocate(module, requestBytes, allocations);
      const namePointer = allocate(module, nameBytes, allocations);
      const mediaTypePointer = allocate(module, mediaTypeBytes, allocations);
      const dataPointer = allocate(module, attachment.data, allocations);
      const descriptorPointer = module._malloc(36);
      if (!descriptorPointer) throw new Error("Could not allocate the WASM attachment descriptor.");
      allocations.push(descriptorPointer);
      module.HEAPU32.set(
        [
          36,
          0,
          namePointer,
          nameBytes.byteLength,
          mediaTypePointer,
          mediaTypeBytes.byteLength,
          dataPointer,
          attachment.data.byteLength,
          0,
        ],
        descriptorPointer >>> 2,
      );
      const resultOut = allocateSlot(module, allocations);
      const errorOut = allocateSlot(module, allocations);
      const code = module._geometer_operation_execute(
        operationPointer,
        operationBytes.byteLength,
        requestPointer,
        requestBytes.byteLength,
        descriptorPointer,
        1,
        resultOut,
        errorOut,
      );
      resultPointer = module.HEAPU32[resultOut >>> 2];
      errorPointer = module.HEAPU32[errorOut >>> 2];
      if (code !== 0 || !resultPointer || errorPointer) {
        const detail = errorPointer ? module.UTF8ToString(errorPointer) : "No local detail.";
        throw new Error(`Geometer generic operation failed (${code}): ${detail}`);
      }
      const jsonPointer = module._geometer_operation_result_json_data(resultPointer);
      const jsonSize = module._geometer_operation_result_json_size(resultPointer);
      const outcome = JSON.parse(
        new TextDecoder().decode(module.HEAPU8.slice(jsonPointer, jsonPointer + jsonSize)),
      );
      if (!outcome.ok) {
        const detail = outcome.diagnostics
          ?.map((item) => `${item.code}${item.path ? ` at ${item.path}` : ""}: ${item.message}`)
          .join("; ");
        throw new Error(detail || `Geometer operation ${operation} failed.`);
      }
      if (outcome.operation !== operation) throw new Error("Geometer operation identity mismatch.");
      return outcome.result;
    } finally {
      if (errorPointer) module._geometer_free_string(errorPointer);
      if (resultPointer) module._geometer_operation_result_free(resultPointer);
      for (const pointer of allocations.reverse()) module._free(pointer);
    }
  }

  global.GeometerOperationWorker = Object.freeze({ executeOneAttachment });
})(self);
