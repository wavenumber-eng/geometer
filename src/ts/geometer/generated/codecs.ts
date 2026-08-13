// Generated from wn_geometer_contract_catalog.a0.json. Do not edit.

import type { ContractDescriptorMap } from "../codec-runtime.js";
import { decodeContractJson, encodeContractJson } from "../codec-runtime.js";
import type {
  DiagnosticA0,
  IpcCancelledA0,
  IpcCancelRejectedA0,
  IpcHelloA0,
  IpcOperationCatalogA0,
  IpcProtocolErrorA0,
  IpcReasonA0,
  IpcRequestA0,
  IpcShutdownAckA0,
  IpcWelcomeA0,
  ModelBoundsOptionsA0,
  ModelBoundsResultA0,
  OperationOutcomeA0,
} from "./contracts.js";

const declarations: ContractDescriptorMap = {
  "Wavenumber.Geometer.Contracts.Common.DiagnosticA0": {
    kind: "object",
    properties: {
      code: {
        type: { kind: "primitive", name: "string" },
        optional: false,
        constraints: { min_length: 1 },
      },
      category: {
        type: {
          kind: "reference",
          target: "Wavenumber.Geometer.Contracts.Common.DiagnosticCategory",
        },
        optional: false,
        constraints: {},
      },
      message: { type: { kind: "primitive", name: "string" }, optional: false, constraints: {} },
      retryable: { type: { kind: "primitive", name: "boolean" }, optional: false, constraints: {} },
      path: { type: { kind: "primitive", name: "string" }, optional: true, constraints: {} },
      operation: { type: { kind: "primitive", name: "string" }, optional: true, constraints: {} },
      request_id: { type: { kind: "primitive", name: "string" }, optional: true, constraints: {} },
    },
  },
  "Wavenumber.Geometer.Contracts.Common.DiagnosticCategory": {
    kind: "enum",
    values: ["transport", "contract", "operation"],
  },
  "Wavenumber.Geometer.Contracts.IpcA0.IpcAttachmentDeclarationA0": {
    kind: "object",
    properties: {
      name: {
        type: { kind: "primitive", name: "string" },
        optional: false,
        constraints: { min_length: 1, max_length: 128 },
      },
      required: { type: { kind: "primitive", name: "boolean" }, optional: false, constraints: {} },
      media_types: {
        type: { kind: "array", element: { kind: "primitive", name: "string" } },
        optional: false,
        constraints: { min_items: 1, max_items: 16 },
      },
      max_bytes: {
        type: { kind: "primitive", name: "uint32" },
        optional: false,
        constraints: { max_value: 268435456 },
      },
    },
  },
  "Wavenumber.Geometer.Contracts.IpcA0.IpcAttachmentDescriptorA0": {
    kind: "object",
    properties: {
      wasm32: {
        type: {
          kind: "reference",
          target: "Wavenumber.Geometer.Contracts.IpcA0.IpcAttachmentLayoutWasm32A0",
        },
        optional: false,
        constraints: {},
      },
      pointer64: {
        type: {
          kind: "reference",
          target: "Wavenumber.Geometer.Contracts.IpcA0.IpcAttachmentLayoutPointer64A0",
        },
        optional: false,
        constraints: {},
      },
    },
  },
  "Wavenumber.Geometer.Contracts.IpcA0.IpcAttachmentLayoutPointer64A0": {
    kind: "object",
    properties: {
      size: {
        type: { kind: "primitive", name: "uint32" },
        optional: false,
        constraints: { min_value: 56, max_value: 56 },
      },
      offsets: {
        type: {
          kind: "reference",
          target: "Wavenumber.Geometer.Contracts.IpcA0.IpcAttachmentOffsetsPointer64A0",
        },
        optional: false,
        constraints: {},
      },
    },
  },
  "Wavenumber.Geometer.Contracts.IpcA0.IpcAttachmentLayoutWasm32A0": {
    kind: "object",
    properties: {
      size: {
        type: { kind: "primitive", name: "uint32" },
        optional: false,
        constraints: { min_value: 36, max_value: 36 },
      },
      offsets: {
        type: {
          kind: "reference",
          target: "Wavenumber.Geometer.Contracts.IpcA0.IpcAttachmentOffsetsWasm32A0",
        },
        optional: false,
        constraints: {},
      },
    },
  },
  "Wavenumber.Geometer.Contracts.IpcA0.IpcAttachmentOffsetsPointer64A0": {
    kind: "object",
    properties: {
      struct_size: {
        type: { kind: "primitive", name: "uint32" },
        optional: false,
        constraints: { max_value: 0 },
      },
      flags: {
        type: { kind: "primitive", name: "uint32" },
        optional: false,
        constraints: { min_value: 4, max_value: 4 },
      },
      name: {
        type: { kind: "primitive", name: "uint32" },
        optional: false,
        constraints: { min_value: 8, max_value: 8 },
      },
      name_size: {
        type: { kind: "primitive", name: "uint32" },
        optional: false,
        constraints: { min_value: 16, max_value: 16 },
      },
      media_type: {
        type: { kind: "primitive", name: "uint32" },
        optional: false,
        constraints: { min_value: 24, max_value: 24 },
      },
      media_type_size: {
        type: { kind: "primitive", name: "uint32" },
        optional: false,
        constraints: { min_value: 32, max_value: 32 },
      },
      data: {
        type: { kind: "primitive", name: "uint32" },
        optional: false,
        constraints: { min_value: 40, max_value: 40 },
      },
      data_size: {
        type: { kind: "primitive", name: "uint32" },
        optional: false,
        constraints: { min_value: 48, max_value: 48 },
      },
      reserved0: {
        type: { kind: "primitive", name: "uint32" },
        optional: false,
        constraints: { min_value: 52, max_value: 52 },
      },
    },
  },
  "Wavenumber.Geometer.Contracts.IpcA0.IpcAttachmentOffsetsWasm32A0": {
    kind: "object",
    properties: {
      struct_size: {
        type: { kind: "primitive", name: "uint32" },
        optional: false,
        constraints: { max_value: 0 },
      },
      flags: {
        type: { kind: "primitive", name: "uint32" },
        optional: false,
        constraints: { min_value: 4, max_value: 4 },
      },
      name: {
        type: { kind: "primitive", name: "uint32" },
        optional: false,
        constraints: { min_value: 8, max_value: 8 },
      },
      name_size: {
        type: { kind: "primitive", name: "uint32" },
        optional: false,
        constraints: { min_value: 12, max_value: 12 },
      },
      media_type: {
        type: { kind: "primitive", name: "uint32" },
        optional: false,
        constraints: { min_value: 16, max_value: 16 },
      },
      media_type_size: {
        type: { kind: "primitive", name: "uint32" },
        optional: false,
        constraints: { min_value: 20, max_value: 20 },
      },
      data: {
        type: { kind: "primitive", name: "uint32" },
        optional: false,
        constraints: { min_value: 24, max_value: 24 },
      },
      data_size: {
        type: { kind: "primitive", name: "uint32" },
        optional: false,
        constraints: { min_value: 28, max_value: 28 },
      },
      reserved0: {
        type: { kind: "primitive", name: "uint32" },
        optional: false,
        constraints: { min_value: 32, max_value: 32 },
      },
    },
  },
  "Wavenumber.Geometer.Contracts.IpcA0.IpcCancelledA0": {
    kind: "object",
    properties: {
      status: {
        type: { kind: "literal", value_type: "string", value: "cancelled" },
        optional: false,
        constraints: {},
      },
    },
  },
  "Wavenumber.Geometer.Contracts.IpcA0.IpcCancelRejectedA0": {
    kind: "object",
    properties: {
      status: {
        type: { kind: "literal", value_type: "string", value: "rejected" },
        optional: false,
        constraints: {},
      },
      diagnostic: {
        type: { kind: "reference", target: "Wavenumber.Geometer.Contracts.Common.DiagnosticA0" },
        optional: false,
        constraints: {},
      },
    },
  },
  "Wavenumber.Geometer.Contracts.IpcA0.IpcEffectiveLimitsA0": {
    kind: "object",
    properties: {
      json_bytes: {
        type: { kind: "primitive", name: "uint32" },
        optional: false,
        constraints: { max_value: 8388608 },
      },
      attachment_count: {
        type: { kind: "primitive", name: "uint32" },
        optional: false,
        constraints: { max_value: 16 },
      },
      attachment_name_bytes: {
        type: { kind: "primitive", name: "uint32" },
        optional: false,
        constraints: { max_value: 128 },
      },
      attachment_media_type_bytes: {
        type: { kind: "primitive", name: "uint32" },
        optional: false,
        constraints: { max_value: 128 },
      },
      attachment_bytes: {
        type: { kind: "primitive", name: "uint32" },
        optional: false,
        constraints: { max_value: 268435456 },
      },
      frame_bytes: {
        type: { kind: "primitive", name: "uint32" },
        optional: false,
        constraints: { max_value: 536870912 },
      },
      queued_requests: {
        type: { kind: "primitive", name: "uint32" },
        optional: false,
        constraints: { max_value: 8 },
      },
      queued_bytes: {
        type: { kind: "primitive", name: "uint32" },
        optional: false,
        constraints: { max_value: 536870912 },
      },
      resident_request_bytes: {
        type: { kind: "primitive", name: "uint32" },
        optional: false,
        constraints: { max_value: 536870912 },
      },
      pending_writer_bytes: {
        type: { kind: "primitive", name: "uint32" },
        optional: false,
        constraints: { max_value: 536870912 },
      },
    },
  },
  "Wavenumber.Geometer.Contracts.IpcA0.IpcGenericAbiLimitsA0": {
    kind: "object",
    properties: {
      operation_id_bytes: {
        type: { kind: "primitive", name: "uint32" },
        optional: false,
        constraints: { max_value: 128 },
      },
      request_json_bytes: {
        type: { kind: "primitive", name: "uint32" },
        optional: false,
        constraints: { max_value: 8388608 },
      },
      response_json_bytes: {
        type: { kind: "primitive", name: "uint32" },
        optional: false,
        constraints: { max_value: 8388608 },
      },
      attachment_count: {
        type: { kind: "primitive", name: "uint32" },
        optional: false,
        constraints: { max_value: 16 },
      },
      attachment_name_bytes: {
        type: { kind: "primitive", name: "uint32" },
        optional: false,
        constraints: { max_value: 128 },
      },
      attachment_media_type_bytes: {
        type: { kind: "primitive", name: "uint32" },
        optional: false,
        constraints: { max_value: 128 },
      },
      attachment_bytes: {
        type: { kind: "primitive", name: "uint32" },
        optional: false,
        constraints: { max_value: 268435456 },
      },
      aggregate_attachment_bytes_native: {
        type: { kind: "primitive", name: "uint32" },
        optional: false,
        constraints: { max_value: 536870912 },
      },
      aggregate_attachment_bytes_wasm: {
        type: { kind: "primitive", name: "uint32" },
        optional: false,
        constraints: { max_value: 268435456 },
      },
    },
  },
  "Wavenumber.Geometer.Contracts.IpcA0.IpcHelloA0": {
    kind: "object",
    properties: {
      client_name: {
        type: { kind: "primitive", name: "string" },
        optional: false,
        constraints: { min_length: 1, max_length: 128 },
      },
      client_version: {
        type: { kind: "primitive", name: "string" },
        optional: false,
        constraints: { min_length: 1, max_length: 128 },
      },
      protocols: {
        type: { kind: "array", element: { kind: "primitive", name: "string" } },
        optional: false,
        constraints: { min_items: 1, max_items: 16 },
      },
      capabilities: {
        type: { kind: "array", element: { kind: "primitive", name: "string" } },
        optional: true,
        constraints: { max_items: 64 },
      },
    },
  },
  "Wavenumber.Geometer.Contracts.IpcA0.IpcOperationCatalogA0": {
    kind: "object",
    properties: {
      catalog: {
        type: { kind: "literal", value_type: "string", value: "wn.geometer.operation_catalog.a0" },
        optional: false,
        constraints: {},
      },
      generic_abi: {
        type: { kind: "literal", value_type: "string", value: "a0" },
        optional: false,
        constraints: {},
      },
      release_version: {
        type: { kind: "primitive", name: "string" },
        optional: false,
        constraints: { min_length: 1, max_length: 32 },
      },
      c_abi_generation: {
        type: { kind: "primitive", name: "uint32" },
        optional: false,
        constraints: {},
      },
      operations: {
        type: {
          kind: "array",
          element: {
            kind: "reference",
            target: "Wavenumber.Geometer.Contracts.IpcA0.IpcOperationDeclarationA0",
          },
        },
        optional: false,
        constraints: { min_items: 1 },
      },
      attachment_descriptor: {
        type: {
          kind: "reference",
          target: "Wavenumber.Geometer.Contracts.IpcA0.IpcAttachmentDescriptorA0",
        },
        optional: false,
        constraints: {},
      },
      limits: {
        type: {
          kind: "reference",
          target: "Wavenumber.Geometer.Contracts.IpcA0.IpcGenericAbiLimitsA0",
        },
        optional: false,
        constraints: {},
      },
    },
  },
  "Wavenumber.Geometer.Contracts.IpcA0.IpcOperationDeclarationA0": {
    kind: "object",
    properties: {
      identity: {
        type: { kind: "primitive", name: "string" },
        optional: false,
        constraints: { min_length: 1, max_length: 128 },
      },
      request_contract: {
        type: { kind: "primitive", name: "string" },
        optional: false,
        constraints: { min_length: 1, max_length: 128 },
      },
      result_contract: {
        type: { kind: "primitive", name: "string" },
        optional: false,
        constraints: { min_length: 1, max_length: 128 },
      },
      input_attachments: {
        type: {
          kind: "array",
          element: {
            kind: "reference",
            target: "Wavenumber.Geometer.Contracts.IpcA0.IpcAttachmentDeclarationA0",
          },
        },
        optional: false,
        constraints: { max_items: 16 },
      },
      output_attachments: {
        type: {
          kind: "array",
          element: {
            kind: "reference",
            target: "Wavenumber.Geometer.Contracts.IpcA0.IpcAttachmentDeclarationA0",
          },
        },
        optional: false,
        constraints: { max_items: 16 },
      },
    },
  },
  "Wavenumber.Geometer.Contracts.IpcA0.IpcProtocolErrorA0": {
    kind: "object",
    properties: {
      status: {
        type: { kind: "literal", value_type: "string", value: "protocol_error" },
        optional: false,
        constraints: {},
      },
      diagnostic: {
        type: { kind: "reference", target: "Wavenumber.Geometer.Contracts.Common.DiagnosticA0" },
        optional: false,
        constraints: {},
      },
    },
  },
  "Wavenumber.Geometer.Contracts.IpcA0.IpcReasonA0": {
    kind: "object",
    properties: {
      reason: {
        type: { kind: "primitive", name: "string" },
        optional: true,
        constraints: { max_length: 1024 },
      },
    },
  },
  "Wavenumber.Geometer.Contracts.IpcA0.IpcRequestA0": {
    kind: "object",
    properties: {
      operation: {
        type: { kind: "primitive", name: "string" },
        optional: false,
        constraints: { min_length: 1, max_length: 128 },
      },
      request: {
        type: {
          kind: "reference",
          target: "Wavenumber.Geometer.Contracts.ModelBoundsA0.ModelBoundsOptionsA0",
        },
        optional: false,
        constraints: {},
      },
    },
  },
  "Wavenumber.Geometer.Contracts.IpcA0.IpcShutdownAckA0": {
    kind: "object",
    properties: {
      status: {
        type: { kind: "literal", value_type: "string", value: "complete" },
        optional: false,
        constraints: {},
      },
      activeRequestCompleted: {
        type: { kind: "primitive", name: "boolean" },
        optional: false,
        constraints: {},
      },
      rejectedQueuedRequestCount: {
        type: { kind: "primitive", name: "uint32" },
        optional: false,
        constraints: {},
      },
    },
  },
  "Wavenumber.Geometer.Contracts.IpcA0.IpcWelcomeA0": {
    kind: "object",
    properties: {
      release_version: {
        type: { kind: "primitive", name: "string" },
        optional: false,
        constraints: { min_length: 1, max_length: 32 },
      },
      c_abi_generation: {
        type: { kind: "primitive", name: "uint32" },
        optional: false,
        constraints: {},
      },
      ipc: {
        type: { kind: "literal", value_type: "string", value: "a0" },
        optional: false,
        constraints: {},
      },
      catalog_sha256: {
        type: { kind: "primitive", name: "string" },
        optional: false,
        constraints: { min_length: 64, max_length: 64 },
      },
      operation_catalog: {
        type: {
          kind: "reference",
          target: "Wavenumber.Geometer.Contracts.IpcA0.IpcOperationCatalogA0",
        },
        optional: false,
        constraints: {},
      },
      limits: {
        type: {
          kind: "reference",
          target: "Wavenumber.Geometer.Contracts.IpcA0.IpcEffectiveLimitsA0",
        },
        optional: false,
        constraints: {},
      },
      capabilities: {
        type: { kind: "array", element: { kind: "primitive", name: "string" } },
        optional: false,
        constraints: { max_items: 64 },
      },
    },
  },
  "Wavenumber.Geometer.Contracts.ModelBoundsA0.Matrix4x4": {
    kind: "array",
    element: { kind: "primitive", name: "float64" },
    constraints: { min_items: 16, max_items: 16 },
  },
  "Wavenumber.Geometer.Contracts.ModelBoundsA0.ModelBoundsOptionsA0": {
    kind: "object",
    properties: {
      format: {
        type: {
          kind: "reference",
          target: "Wavenumber.Geometer.Contracts.ModelBoundsA0.ModelFormat",
        },
        optional: true,
        constraints: {},
      },
      model_transform: {
        type: {
          kind: "reference",
          target: "Wavenumber.Geometer.Contracts.ModelBoundsA0.Matrix4x4",
        },
        optional: true,
        constraints: {},
      },
    },
  },
  "Wavenumber.Geometer.Contracts.ModelBoundsA0.ModelBoundsResultA0": {
    kind: "object",
    properties: {
      schema: {
        type: { kind: "literal", value_type: "string", value: "geometry.model_bounds.a0" },
        optional: false,
        constraints: {},
      },
      units: {
        type: { kind: "literal", value_type: "string", value: "mm" },
        optional: false,
        constraints: {},
      },
      source: {
        type: {
          kind: "reference",
          target: "Wavenumber.Geometer.Contracts.ModelBoundsA0.ModelBoundsSource",
        },
        optional: false,
        constraints: {},
      },
      bounds: {
        type: {
          kind: "reference",
          target: "Wavenumber.Geometer.Contracts.ModelBoundsA0.ModelBoundsValues",
        },
        optional: false,
        constraints: {},
      },
      timings: {
        type: {
          kind: "reference",
          target: "Wavenumber.Geometer.Contracts.ModelBoundsA0.ModelBoundsTimings",
        },
        optional: false,
        constraints: {},
      },
    },
  },
  "Wavenumber.Geometer.Contracts.ModelBoundsA0.ModelBoundsSource": {
    kind: "object",
    properties: {
      format: {
        type: {
          kind: "reference",
          target: "Wavenumber.Geometer.Contracts.ModelBoundsA0.ModelFormat",
        },
        optional: false,
        constraints: {},
      },
      hash: { type: { kind: "primitive", name: "string" }, optional: false, constraints: {} },
    },
  },
  "Wavenumber.Geometer.Contracts.ModelBoundsA0.ModelBoundsTimings": {
    kind: "object",
    properties: {
      model_read_ms: {
        type: { kind: "primitive", name: "float64" },
        optional: false,
        constraints: { min_value: 0 },
      },
      bounds_ms: {
        type: { kind: "primitive", name: "float64" },
        optional: false,
        constraints: { min_value: 0 },
      },
    },
  },
  "Wavenumber.Geometer.Contracts.ModelBoundsA0.ModelBoundsValues": {
    kind: "object",
    properties: {
      min: {
        type: { kind: "reference", target: "Wavenumber.Geometer.Contracts.ModelBoundsA0.Vector3" },
        optional: false,
        constraints: {},
      },
      max: {
        type: { kind: "reference", target: "Wavenumber.Geometer.Contracts.ModelBoundsA0.Vector3" },
        optional: false,
        constraints: {},
      },
      size: {
        type: { kind: "reference", target: "Wavenumber.Geometer.Contracts.ModelBoundsA0.Vector3" },
        optional: false,
        constraints: {},
      },
      center: {
        type: { kind: "reference", target: "Wavenumber.Geometer.Contracts.ModelBoundsA0.Vector3" },
        optional: false,
        constraints: {},
      },
    },
  },
  "Wavenumber.Geometer.Contracts.ModelBoundsA0.ModelFormat": { kind: "enum", values: ["step"] },
  "Wavenumber.Geometer.Contracts.ModelBoundsA0.Vector3": {
    kind: "array",
    element: { kind: "primitive", name: "float64" },
    constraints: { min_items: 3, max_items: 3 },
  },
  "Wavenumber.Geometer.Contracts.OperationOutcomeA0.OperationFailureA0": {
    kind: "object",
    properties: {
      operation: {
        type: { kind: "primitive", name: "string" },
        optional: false,
        constraints: { min_length: 1, max_length: 128 },
      },
      ok: {
        type: { kind: "literal", value_type: "boolean", value: false },
        optional: false,
        constraints: {},
      },
      diagnostics: {
        type: {
          kind: "array",
          element: {
            kind: "reference",
            target: "Wavenumber.Geometer.Contracts.Common.DiagnosticA0",
          },
        },
        optional: false,
        constraints: { min_items: 1 },
      },
    },
  },
  "Wavenumber.Geometer.Contracts.OperationOutcomeA0.OperationOutcomeA0": {
    kind: "union",
    variants: [
      {
        kind: "reference",
        target: "Wavenumber.Geometer.Contracts.OperationOutcomeA0.OperationSuccessA0",
      },
      {
        kind: "reference",
        target: "Wavenumber.Geometer.Contracts.OperationOutcomeA0.OperationFailureA0",
      },
    ],
  },
  "Wavenumber.Geometer.Contracts.OperationOutcomeA0.OperationResultValueA0": {
    kind: "union",
    variants: [
      {
        kind: "reference",
        target: "Wavenumber.Geometer.Contracts.ModelBoundsA0.ModelBoundsResultA0",
      },
    ],
  },
  "Wavenumber.Geometer.Contracts.OperationOutcomeA0.OperationSuccessA0": {
    kind: "object",
    properties: {
      operation: {
        type: { kind: "primitive", name: "string" },
        optional: false,
        constraints: { min_length: 1, max_length: 128 },
      },
      ok: {
        type: { kind: "literal", value_type: "boolean", value: true },
        optional: false,
        constraints: {},
      },
      result: {
        type: {
          kind: "reference",
          target: "Wavenumber.Geometer.Contracts.OperationOutcomeA0.OperationResultValueA0",
        },
        optional: false,
        constraints: {},
      },
    },
  },
};

export function decodeDiagnosticA0Json(data: string | Uint8Array): DiagnosticA0 {
  return decodeContractJson(
    data,
    { kind: "reference", target: "Wavenumber.Geometer.Contracts.Common.DiagnosticA0" },
    declarations,
  ) as DiagnosticA0;
}

export function encodeDiagnosticA0Json(value: DiagnosticA0): string {
  return encodeContractJson(
    value,
    { kind: "reference", target: "Wavenumber.Geometer.Contracts.Common.DiagnosticA0" },
    declarations,
  );
}

export function decodeIpcCancelledA0Json(data: string | Uint8Array): IpcCancelledA0 {
  return decodeContractJson(
    data,
    { kind: "reference", target: "Wavenumber.Geometer.Contracts.IpcA0.IpcCancelledA0" },
    declarations,
  ) as IpcCancelledA0;
}

export function encodeIpcCancelledA0Json(value: IpcCancelledA0): string {
  return encodeContractJson(
    value,
    { kind: "reference", target: "Wavenumber.Geometer.Contracts.IpcA0.IpcCancelledA0" },
    declarations,
  );
}

export function decodeIpcCancelRejectedA0Json(data: string | Uint8Array): IpcCancelRejectedA0 {
  return decodeContractJson(
    data,
    { kind: "reference", target: "Wavenumber.Geometer.Contracts.IpcA0.IpcCancelRejectedA0" },
    declarations,
  ) as IpcCancelRejectedA0;
}

export function encodeIpcCancelRejectedA0Json(value: IpcCancelRejectedA0): string {
  return encodeContractJson(
    value,
    { kind: "reference", target: "Wavenumber.Geometer.Contracts.IpcA0.IpcCancelRejectedA0" },
    declarations,
  );
}

export function decodeIpcHelloA0Json(data: string | Uint8Array): IpcHelloA0 {
  return decodeContractJson(
    data,
    { kind: "reference", target: "Wavenumber.Geometer.Contracts.IpcA0.IpcHelloA0" },
    declarations,
  ) as IpcHelloA0;
}

export function encodeIpcHelloA0Json(value: IpcHelloA0): string {
  return encodeContractJson(
    value,
    { kind: "reference", target: "Wavenumber.Geometer.Contracts.IpcA0.IpcHelloA0" },
    declarations,
  );
}

export function decodeIpcOperationCatalogA0Json(data: string | Uint8Array): IpcOperationCatalogA0 {
  return decodeContractJson(
    data,
    { kind: "reference", target: "Wavenumber.Geometer.Contracts.IpcA0.IpcOperationCatalogA0" },
    declarations,
  ) as IpcOperationCatalogA0;
}

export function encodeIpcOperationCatalogA0Json(value: IpcOperationCatalogA0): string {
  return encodeContractJson(
    value,
    { kind: "reference", target: "Wavenumber.Geometer.Contracts.IpcA0.IpcOperationCatalogA0" },
    declarations,
  );
}

export function decodeIpcProtocolErrorA0Json(data: string | Uint8Array): IpcProtocolErrorA0 {
  return decodeContractJson(
    data,
    { kind: "reference", target: "Wavenumber.Geometer.Contracts.IpcA0.IpcProtocolErrorA0" },
    declarations,
  ) as IpcProtocolErrorA0;
}

export function encodeIpcProtocolErrorA0Json(value: IpcProtocolErrorA0): string {
  return encodeContractJson(
    value,
    { kind: "reference", target: "Wavenumber.Geometer.Contracts.IpcA0.IpcProtocolErrorA0" },
    declarations,
  );
}

export function decodeIpcReasonA0Json(data: string | Uint8Array): IpcReasonA0 {
  return decodeContractJson(
    data,
    { kind: "reference", target: "Wavenumber.Geometer.Contracts.IpcA0.IpcReasonA0" },
    declarations,
  ) as IpcReasonA0;
}

export function encodeIpcReasonA0Json(value: IpcReasonA0): string {
  return encodeContractJson(
    value,
    { kind: "reference", target: "Wavenumber.Geometer.Contracts.IpcA0.IpcReasonA0" },
    declarations,
  );
}

export function decodeIpcRequestA0Json(data: string | Uint8Array): IpcRequestA0 {
  return decodeContractJson(
    data,
    { kind: "reference", target: "Wavenumber.Geometer.Contracts.IpcA0.IpcRequestA0" },
    declarations,
  ) as IpcRequestA0;
}

export function encodeIpcRequestA0Json(value: IpcRequestA0): string {
  return encodeContractJson(
    value,
    { kind: "reference", target: "Wavenumber.Geometer.Contracts.IpcA0.IpcRequestA0" },
    declarations,
  );
}

export function decodeIpcShutdownAckA0Json(data: string | Uint8Array): IpcShutdownAckA0 {
  return decodeContractJson(
    data,
    { kind: "reference", target: "Wavenumber.Geometer.Contracts.IpcA0.IpcShutdownAckA0" },
    declarations,
  ) as IpcShutdownAckA0;
}

export function encodeIpcShutdownAckA0Json(value: IpcShutdownAckA0): string {
  return encodeContractJson(
    value,
    { kind: "reference", target: "Wavenumber.Geometer.Contracts.IpcA0.IpcShutdownAckA0" },
    declarations,
  );
}

export function decodeIpcWelcomeA0Json(data: string | Uint8Array): IpcWelcomeA0 {
  return decodeContractJson(
    data,
    { kind: "reference", target: "Wavenumber.Geometer.Contracts.IpcA0.IpcWelcomeA0" },
    declarations,
  ) as IpcWelcomeA0;
}

export function encodeIpcWelcomeA0Json(value: IpcWelcomeA0): string {
  return encodeContractJson(
    value,
    { kind: "reference", target: "Wavenumber.Geometer.Contracts.IpcA0.IpcWelcomeA0" },
    declarations,
  );
}

export function decodeModelBoundsOptionsA0Json(data: string | Uint8Array): ModelBoundsOptionsA0 {
  return decodeContractJson(
    data,
    {
      kind: "reference",
      target: "Wavenumber.Geometer.Contracts.ModelBoundsA0.ModelBoundsOptionsA0",
    },
    declarations,
  ) as ModelBoundsOptionsA0;
}

export function encodeModelBoundsOptionsA0Json(value: ModelBoundsOptionsA0): string {
  return encodeContractJson(
    value,
    {
      kind: "reference",
      target: "Wavenumber.Geometer.Contracts.ModelBoundsA0.ModelBoundsOptionsA0",
    },
    declarations,
  );
}

export function decodeModelBoundsResultA0Json(data: string | Uint8Array): ModelBoundsResultA0 {
  return decodeContractJson(
    data,
    {
      kind: "reference",
      target: "Wavenumber.Geometer.Contracts.ModelBoundsA0.ModelBoundsResultA0",
    },
    declarations,
  ) as ModelBoundsResultA0;
}

export function encodeModelBoundsResultA0Json(value: ModelBoundsResultA0): string {
  return encodeContractJson(
    value,
    {
      kind: "reference",
      target: "Wavenumber.Geometer.Contracts.ModelBoundsA0.ModelBoundsResultA0",
    },
    declarations,
  );
}

export function decodeOperationOutcomeA0Json(data: string | Uint8Array): OperationOutcomeA0 {
  return decodeContractJson(
    data,
    {
      kind: "reference",
      target: "Wavenumber.Geometer.Contracts.OperationOutcomeA0.OperationOutcomeA0",
    },
    declarations,
  ) as OperationOutcomeA0;
}

export function encodeOperationOutcomeA0Json(value: OperationOutcomeA0): string {
  return encodeContractJson(
    value,
    {
      kind: "reference",
      target: "Wavenumber.Geometer.Contracts.OperationOutcomeA0.OperationOutcomeA0",
    },
    declarations,
  );
}
