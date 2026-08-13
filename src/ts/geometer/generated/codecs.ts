// Generated from wn_geometer_contract_catalog.a0.json. Do not edit.

import type { ContractDescriptorMap } from "../codec-runtime.js";
import { decodeContractJson, encodeContractJson } from "../codec-runtime.js";
import type {
  DiagnosticA0,
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
