// Generated from wn_geometer_contract_catalog.a0.json. Do not edit.
import { decodeContractJson, encodeContractJson } from "../codec-runtime.js";
const declarations = {
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
    "Wavenumber.Geometer.Contracts.Common.PackedAttachmentProjectionA0": {
        kind: "object",
        properties: {
            schema: {
                type: { kind: "primitive", name: "string" },
                optional: false,
                constraints: { min_length: 1, max_length: 128 },
            },
            packet: {
                type: {
                    kind: "reference",
                    target: "Wavenumber.Geometer.Contracts.Common.PackedAttachmentReferenceA0",
                },
                optional: false,
                constraints: {},
            },
        },
    },
    "Wavenumber.Geometer.Contracts.Common.PackedAttachmentReferenceA0": {
        kind: "object",
        properties: {
            attachment: {
                type: { kind: "primitive", name: "string" },
                optional: false,
                constraints: { min_length: 1, max_length: 128 },
            },
            format: {
                type: { kind: "primitive", name: "string" },
                optional: false,
                constraints: { min_length: 1, max_length: 128 },
            },
        },
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
            runtime_dispatch: {
                type: {
                    kind: "reference",
                    target: "Wavenumber.Geometer.Contracts.IpcA0.IpcRuntimeDispatchA0",
                },
                optional: false,
                constraints: {},
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
            request_projection: {
                type: {
                    kind: "reference",
                    target: "Wavenumber.Geometer.Contracts.IpcA0.IpcPackedProjectionA0",
                },
                optional: true,
                constraints: {},
            },
            result_projection: {
                type: {
                    kind: "reference",
                    target: "Wavenumber.Geometer.Contracts.IpcA0.IpcPackedProjectionA0",
                },
                optional: true,
                constraints: {},
            },
        },
    },
    "Wavenumber.Geometer.Contracts.IpcA0.IpcPackedProjectionA0": {
        kind: "object",
        properties: {
            kind: {
                type: { kind: "literal", value_type: "string", value: "packed_attachment" },
                optional: false,
                constraints: {},
            },
            attachment_name: {
                type: { kind: "primitive", name: "string" },
                optional: false,
                constraints: { min_length: 1, max_length: 128 },
            },
            format: {
                type: { kind: "primitive", name: "string" },
                optional: false,
                constraints: { min_length: 1, max_length: 128 },
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
                    target: "Wavenumber.Geometer.Contracts.IpcA0.IpcRequestValueA0",
                },
                optional: false,
                constraints: {},
            },
        },
    },
    "Wavenumber.Geometer.Contracts.IpcA0.IpcRequestValueA0": {
        kind: "union",
        variants: [
            {
                kind: "reference",
                target: "Wavenumber.Geometer.Contracts.ModelBoundsA0.ModelBoundsOptionsA0",
            },
            {
                kind: "reference",
                target: "Wavenumber.Geometer.Contracts.Common.PackedAttachmentProjectionA0",
            },
        ],
    },
    "Wavenumber.Geometer.Contracts.IpcA0.IpcRuntimeDispatchA0": {
        kind: "enum",
        values: ["logical_dto", "packed_attachment"],
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
            {
                kind: "reference",
                target: "Wavenumber.Geometer.Contracts.Common.PackedAttachmentProjectionA0",
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
    "Wavenumber.Geometer.Contracts.StepTopologyA0.BodySummary": {
        kind: "object",
        properties: {
            handle: {
                type: { kind: "primitive", name: "string" },
                optional: false,
                constraints: { min_length: 68, max_length: 68 },
            },
            definition_handle: {
                type: { kind: "primitive", name: "string" },
                optional: false,
                constraints: { min_length: 68, max_length: 68 },
            },
            topology_kind: {
                type: { kind: "primitive", name: "string" },
                optional: false,
                constraints: { min_length: 1, max_length: 64 },
            },
            shell_handles: {
                type: { kind: "array", element: { kind: "primitive", name: "string" } },
                optional: false,
                constraints: { max_items: 250000 },
            },
            face_handles: {
                type: { kind: "array", element: { kind: "primitive", name: "string" } },
                optional: false,
                constraints: { max_items: 1000000 },
            },
            bounds_mm: {
                type: { kind: "array", element: { kind: "primitive", name: "float64" } },
                optional: false,
                constraints: { min_items: 6, max_items: 6 },
            },
            volume_mm3: {
                type: { kind: "primitive", name: "float64" },
                optional: false,
                constraints: { min_value: 0 },
            },
            source_entity: {
                type: {
                    kind: "reference",
                    target: "Wavenumber.Geometer.Contracts.StepTopologyA0.SourceEntityEvidence",
                },
                optional: true,
                constraints: {},
            },
        },
    },
    "Wavenumber.Geometer.Contracts.StepTopologyA0.ComponentOccurrenceSummary": {
        kind: "object",
        properties: {
            kind: {
                type: { kind: "literal", value_type: "string", value: "component" },
                optional: false,
                constraints: {},
            },
            handle: {
                type: { kind: "primitive", name: "string" },
                optional: false,
                constraints: { min_length: 68, max_length: 68 },
            },
            definition_handle: {
                type: { kind: "primitive", name: "string" },
                optional: false,
                constraints: { min_length: 68, max_length: 68 },
            },
            parent_occurrence_handle: {
                type: { kind: "primitive", name: "string" },
                optional: false,
                constraints: { min_length: 68, max_length: 68 },
            },
            depth: {
                type: { kind: "primitive", name: "uint32" },
                optional: false,
                constraints: { min_value: 1, max_value: 64 },
            },
            name: {
                type: { kind: "primitive", name: "string" },
                optional: false,
                constraints: { max_length: 4096 },
            },
            transform: {
                type: { kind: "array", element: { kind: "primitive", name: "float64" } },
                optional: false,
                constraints: { min_items: 12, max_items: 12 },
            },
        },
    },
    "Wavenumber.Geometer.Contracts.StepTopologyA0.DefinitionSummary": {
        kind: "object",
        properties: {
            handle: {
                type: { kind: "primitive", name: "string" },
                optional: false,
                constraints: { min_length: 68, max_length: 68 },
            },
            name: {
                type: { kind: "primitive", name: "string" },
                optional: false,
                constraints: { max_length: 4096 },
            },
            assembly: { type: { kind: "primitive", name: "boolean" }, optional: false, constraints: {} },
            body_count: {
                type: { kind: "primitive", name: "uint32" },
                optional: false,
                constraints: { max_value: 100000 },
            },
            face_count: {
                type: { kind: "primitive", name: "uint32" },
                optional: false,
                constraints: { max_value: 1000000 },
            },
            source_entity: {
                type: {
                    kind: "reference",
                    target: "Wavenumber.Geometer.Contracts.StepTopologyA0.SourceEntityEvidence",
                },
                optional: true,
                constraints: {},
            },
        },
    },
    "Wavenumber.Geometer.Contracts.StepTopologyA0.FaceSummary": {
        kind: "object",
        properties: {
            handle: {
                type: { kind: "primitive", name: "string" },
                optional: false,
                constraints: { min_length: 68, max_length: 68 },
            },
            definition_handle: {
                type: { kind: "primitive", name: "string" },
                optional: false,
                constraints: { min_length: 68, max_length: 68 },
            },
            body_handles: {
                type: { kind: "array", element: { kind: "primitive", name: "string" } },
                optional: false,
                constraints: { max_items: 100000 },
            },
            shell_handles: {
                type: { kind: "array", element: { kind: "primitive", name: "string" } },
                optional: false,
                constraints: { max_items: 250000 },
            },
            bounds_mm: {
                type: { kind: "array", element: { kind: "primitive", name: "float64" } },
                optional: false,
                constraints: { min_items: 6, max_items: 6 },
            },
            area_mm2: {
                type: { kind: "primitive", name: "float64" },
                optional: false,
                constraints: { min_value: 0 },
            },
            centroid_mm: {
                type: { kind: "array", element: { kind: "primitive", name: "float64" } },
                optional: false,
                constraints: { min_items: 3, max_items: 3 },
            },
            source_entity: {
                type: {
                    kind: "reference",
                    target: "Wavenumber.Geometer.Contracts.StepTopologyA0.SourceEntityEvidence",
                },
                optional: true,
                constraints: {},
            },
        },
    },
    "Wavenumber.Geometer.Contracts.StepTopologyA0.GlbAttachmentDescriptor": {
        kind: "object",
        properties: {
            name: {
                type: { kind: "literal", value_type: "string", value: "glb" },
                optional: false,
                constraints: {},
            },
            media_type: {
                type: { kind: "literal", value_type: "string", value: "model/gltf-binary" },
                optional: false,
                constraints: {},
            },
            format: {
                type: { kind: "literal", value_type: "string", value: "glb-2.0" },
                optional: false,
                constraints: {},
            },
            bytes: {
                type: { kind: "primitive", name: "uint32" },
                optional: false,
                constraints: { min_value: 1, max_value: 268435456 },
            },
            sha256: {
                type: { kind: "primitive", name: "string" },
                optional: false,
                constraints: { min_length: 64, max_length: 64 },
            },
        },
    },
    "Wavenumber.Geometer.Contracts.StepTopologyA0.InspectionCounts": {
        kind: "object",
        properties: {
            definitions: {
                type: { kind: "primitive", name: "uint32" },
                optional: false,
                constraints: { max_value: 10000 },
            },
            root_occurrences: {
                type: { kind: "primitive", name: "uint32" },
                optional: false,
                constraints: { max_value: 100000 },
            },
            component_occurrences: {
                type: { kind: "primitive", name: "uint32" },
                optional: false,
                constraints: { max_value: 100000 },
            },
            bodies: {
                type: { kind: "primitive", name: "uint32" },
                optional: false,
                constraints: { max_value: 100000 },
            },
            shells: {
                type: { kind: "primitive", name: "uint32" },
                optional: false,
                constraints: { max_value: 250000 },
            },
            faces: {
                type: { kind: "primitive", name: "uint32" },
                optional: false,
                constraints: { max_value: 1000000 },
            },
        },
    },
    "Wavenumber.Geometer.Contracts.StepTopologyA0.OccurrenceSummary": {
        kind: "union",
        variants: [
            {
                kind: "reference",
                target: "Wavenumber.Geometer.Contracts.StepTopologyA0.RootOccurrenceSummary",
            },
            {
                kind: "reference",
                target: "Wavenumber.Geometer.Contracts.StepTopologyA0.ComponentOccurrenceSummary",
            },
        ],
    },
    "Wavenumber.Geometer.Contracts.StepTopologyA0.PageRequest": {
        kind: "object",
        properties: {
            cursor: {
                type: { kind: "primitive", name: "string" },
                optional: true,
                constraints: { max_length: 256 },
            },
            limit: {
                type: { kind: "primitive", name: "uint32" },
                optional: false,
                constraints: { min_value: 1, max_value: 1024 },
            },
        },
    },
    "Wavenumber.Geometer.Contracts.StepTopologyA0.RenderArtifactDescriptor": {
        kind: "object",
        properties: {
            artifact_handle: {
                type: { kind: "primitive", name: "string" },
                optional: false,
                constraints: { min_length: 68, max_length: 68 },
            },
            content_sha256: {
                type: { kind: "primitive", name: "string" },
                optional: false,
                constraints: { min_length: 64, max_length: 64 },
            },
            render_artifact_handle: {
                type: { kind: "primitive", name: "string" },
                optional: false,
                constraints: { min_length: 68, max_length: 68 },
            },
            render_content_sha256: {
                type: { kind: "primitive", name: "string" },
                optional: false,
                constraints: { min_length: 64, max_length: 64 },
            },
            binding_layout: {
                type: { kind: "literal", value_type: "string", value: "node-primitive-a0" },
                optional: false,
                constraints: {},
            },
            geometry_length_unit: {
                type: { kind: "literal", value_type: "string", value: "meter" },
                optional: false,
                constraints: {},
            },
            source_length_unit: {
                type: { kind: "literal", value_type: "string", value: "millimeter" },
                optional: false,
                constraints: {},
            },
            counts: {
                type: {
                    kind: "reference",
                    target: "Wavenumber.Geometer.Contracts.StepTopologyA0.RenderCounts",
                },
                optional: false,
                constraints: {},
            },
        },
    },
    "Wavenumber.Geometer.Contracts.StepTopologyA0.RenderCounts": {
        kind: "object",
        properties: {
            meshes: {
                type: { kind: "primitive", name: "uint32" },
                optional: false,
                constraints: { max_value: 10000 },
            },
            instances: {
                type: { kind: "primitive", name: "uint32" },
                optional: false,
                constraints: { max_value: 100000 },
            },
            primitives: {
                type: { kind: "primitive", name: "uint32" },
                optional: false,
                constraints: { max_value: 1000000 },
            },
            geometry_triangles: {
                type: { kind: "primitive", name: "uint32" },
                optional: false,
                constraints: { max_value: 10000000 },
            },
            instanced_triangles: {
                type: { kind: "primitive", name: "uint32" },
                optional: false,
                constraints: { max_value: 50000000 },
            },
        },
    },
    "Wavenumber.Geometer.Contracts.StepTopologyA0.RootOccurrenceSummary": {
        kind: "object",
        properties: {
            kind: {
                type: { kind: "literal", value_type: "string", value: "root" },
                optional: false,
                constraints: {},
            },
            handle: {
                type: { kind: "primitive", name: "string" },
                optional: false,
                constraints: { min_length: 68, max_length: 68 },
            },
            definition_handle: {
                type: { kind: "primitive", name: "string" },
                optional: false,
                constraints: { min_length: 68, max_length: 68 },
            },
            name: {
                type: { kind: "primitive", name: "string" },
                optional: false,
                constraints: { max_length: 4096 },
            },
            transform: {
                type: { kind: "array", element: { kind: "primitive", name: "float64" } },
                optional: false,
                constraints: { min_items: 12, max_items: 12 },
            },
        },
    },
    "Wavenumber.Geometer.Contracts.StepTopologyA0.SessionReference": {
        kind: "object",
        properties: {
            session_handle: {
                type: { kind: "primitive", name: "string" },
                optional: false,
                constraints: { min_length: 68, max_length: 68 },
            },
            generation: {
                type: { kind: "primitive", name: "uint32" },
                optional: false,
                constraints: { min_value: 1 },
            },
        },
    },
    "Wavenumber.Geometer.Contracts.StepTopologyA0.ShellSummary": {
        kind: "object",
        properties: {
            handle: {
                type: { kind: "primitive", name: "string" },
                optional: false,
                constraints: { min_length: 68, max_length: 68 },
            },
            definition_handle: {
                type: { kind: "primitive", name: "string" },
                optional: false,
                constraints: { min_length: 68, max_length: 68 },
            },
            body_handles: {
                type: { kind: "array", element: { kind: "primitive", name: "string" } },
                optional: false,
                constraints: { max_items: 100000 },
            },
            face_handles: {
                type: { kind: "array", element: { kind: "primitive", name: "string" } },
                optional: false,
                constraints: { max_items: 1000000 },
            },
            source_entity: {
                type: {
                    kind: "reference",
                    target: "Wavenumber.Geometer.Contracts.StepTopologyA0.SourceEntityEvidence",
                },
                optional: true,
                constraints: {},
            },
        },
    },
    "Wavenumber.Geometer.Contracts.StepTopologyA0.SourceDescriptor": {
        kind: "object",
        properties: {
            format: {
                type: { kind: "literal", value_type: "string", value: "step" },
                optional: false,
                constraints: {},
            },
            sha256: {
                type: { kind: "primitive", name: "string" },
                optional: false,
                constraints: { min_length: 64, max_length: 64 },
            },
            bytes: {
                type: { kind: "primitive", name: "uint32" },
                optional: false,
                constraints: { min_value: 1, max_value: 268435456 },
            },
            normalized_length_unit: {
                type: { kind: "literal", value_type: "string", value: "millimeter" },
                optional: false,
                constraints: {},
            },
        },
    },
    "Wavenumber.Geometer.Contracts.StepTopologyA0.SourceEntityEvidence": {
        kind: "object",
        properties: {
            mapped: { type: { kind: "primitive", name: "boolean" }, optional: false, constraints: {} },
            shape_result_round_trip: {
                type: { kind: "primitive", name: "boolean" },
                optional: false,
                constraints: {},
            },
            model_number: {
                type: { kind: "primitive", name: "uint32" },
                optional: true,
                constraints: { min_value: 1, max_value: 5000000 },
            },
            entity_type: {
                type: { kind: "primitive", name: "string" },
                optional: true,
                constraints: { min_length: 1, max_length: 128 },
            },
            mapping_method: {
                type: { kind: "primitive", name: "string" },
                optional: true,
                constraints: { min_length: 1, max_length: 128 },
            },
        },
    },
    "Wavenumber.Geometer.Contracts.StepTopologyA0.StepTopologyCloseRequestA0": {
        kind: "object",
        properties: {
            schema: {
                type: {
                    kind: "literal",
                    value_type: "string",
                    value: "geometry.step_topology.close.request.a0",
                },
                optional: false,
                constraints: {},
            },
            session: {
                type: {
                    kind: "reference",
                    target: "Wavenumber.Geometer.Contracts.StepTopologyA0.SessionReference",
                },
                optional: false,
                constraints: {},
            },
        },
    },
    "Wavenumber.Geometer.Contracts.StepTopologyA0.StepTopologyCloseResultA0": {
        kind: "object",
        properties: {
            schema: {
                type: {
                    kind: "literal",
                    value_type: "string",
                    value: "geometry.step_topology.close.result.a0",
                },
                optional: false,
                constraints: {},
            },
            session_handle: {
                type: { kind: "primitive", name: "string" },
                optional: false,
                constraints: { min_length: 68, max_length: 68 },
            },
            closed: {
                type: { kind: "literal", value_type: "boolean", value: true },
                optional: false,
                constraints: {},
            },
        },
    },
    "Wavenumber.Geometer.Contracts.StepTopologyA0.StepTopologyInspectRequestA0": {
        kind: "object",
        properties: {
            schema: {
                type: {
                    kind: "literal",
                    value_type: "string",
                    value: "geometry.step_topology.inspect.request.a0",
                },
                optional: false,
                constraints: {},
            },
            session: {
                type: {
                    kind: "reference",
                    target: "Wavenumber.Geometer.Contracts.StepTopologyA0.SessionReference",
                },
                optional: false,
                constraints: {},
            },
            page: {
                type: {
                    kind: "reference",
                    target: "Wavenumber.Geometer.Contracts.StepTopologyA0.PageRequest",
                },
                optional: false,
                constraints: {},
            },
            include_source_entity_evidence: {
                type: { kind: "primitive", name: "boolean" },
                optional: false,
                constraints: {},
            },
            include_diagnostics: {
                type: { kind: "primitive", name: "boolean" },
                optional: false,
                constraints: {},
            },
        },
    },
    "Wavenumber.Geometer.Contracts.StepTopologyA0.StepTopologyInspectResultA0": {
        kind: "object",
        properties: {
            schema: {
                type: {
                    kind: "literal",
                    value_type: "string",
                    value: "geometry.step_topology.inspect.result.a0",
                },
                optional: false,
                constraints: {},
            },
            session: {
                type: {
                    kind: "reference",
                    target: "Wavenumber.Geometer.Contracts.StepTopologyA0.SessionReference",
                },
                optional: false,
                constraints: {},
            },
            counts: {
                type: {
                    kind: "reference",
                    target: "Wavenumber.Geometer.Contracts.StepTopologyA0.InspectionCounts",
                },
                optional: false,
                constraints: {},
            },
            page: {
                type: {
                    kind: "reference",
                    target: "Wavenumber.Geometer.Contracts.StepTopologyA0.TopologyPage",
                },
                optional: false,
                constraints: {},
            },
            compact_table: {
                type: {
                    kind: "reference",
                    target: "Wavenumber.Geometer.Contracts.StepTopologyA0.TopologyTableAttachmentDescriptor",
                },
                optional: true,
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
                constraints: { max_items: 256 },
            },
        },
    },
    "Wavenumber.Geometer.Contracts.StepTopologyA0.StepTopologyOpenRequestA0": {
        kind: "object",
        properties: {
            schema: {
                type: {
                    kind: "literal",
                    value_type: "string",
                    value: "geometry.step_topology.open.request.a0",
                },
                optional: false,
                constraints: {},
            },
        },
    },
    "Wavenumber.Geometer.Contracts.StepTopologyA0.StepTopologyOpenResultA0": {
        kind: "object",
        properties: {
            schema: {
                type: {
                    kind: "literal",
                    value_type: "string",
                    value: "geometry.step_topology.open.result.a0",
                },
                optional: false,
                constraints: {},
            },
            session: {
                type: {
                    kind: "reference",
                    target: "Wavenumber.Geometer.Contracts.StepTopologyA0.SessionReference",
                },
                optional: false,
                constraints: {},
            },
            source: {
                type: {
                    kind: "reference",
                    target: "Wavenumber.Geometer.Contracts.StepTopologyA0.SourceDescriptor",
                },
                optional: false,
                constraints: {},
            },
            tool: {
                type: {
                    kind: "reference",
                    target: "Wavenumber.Geometer.Contracts.StepTopologyA0.ToolDescriptor",
                },
                optional: false,
                constraints: {},
            },
            evicted_session_handles: {
                type: { kind: "array", element: { kind: "primitive", name: "string" } },
                optional: false,
                constraints: { max_items: 8 },
            },
        },
    },
    "Wavenumber.Geometer.Contracts.StepTopologyA0.StepTopologyRenderRequestA0": {
        kind: "object",
        properties: {
            schema: {
                type: {
                    kind: "literal",
                    value_type: "string",
                    value: "geometry.step_topology.render.request.a0",
                },
                optional: false,
                constraints: {},
            },
            session: {
                type: {
                    kind: "reference",
                    target: "Wavenumber.Geometer.Contracts.StepTopologyA0.SessionReference",
                },
                optional: false,
                constraints: {},
            },
            tessellation: {
                type: {
                    kind: "reference",
                    target: "Wavenumber.Geometer.Contracts.StepTopologyA0.TessellationOptions",
                },
                optional: false,
                constraints: {},
            },
        },
    },
    "Wavenumber.Geometer.Contracts.StepTopologyA0.StepTopologyRenderResultA0": {
        kind: "object",
        properties: {
            schema: {
                type: {
                    kind: "literal",
                    value_type: "string",
                    value: "geometry.step_topology.render.result.a0",
                },
                optional: false,
                constraints: {},
            },
            session: {
                type: {
                    kind: "reference",
                    target: "Wavenumber.Geometer.Contracts.StepTopologyA0.SessionReference",
                },
                optional: false,
                constraints: {},
            },
            artifact: {
                type: {
                    kind: "reference",
                    target: "Wavenumber.Geometer.Contracts.StepTopologyA0.RenderArtifactDescriptor",
                },
                optional: false,
                constraints: {},
            },
            glb: {
                type: {
                    kind: "reference",
                    target: "Wavenumber.Geometer.Contracts.StepTopologyA0.GlbAttachmentDescriptor",
                },
                optional: false,
                constraints: {},
            },
            compact_binding_table: {
                type: {
                    kind: "reference",
                    target: "Wavenumber.Geometer.Contracts.StepTopologyA0.TopologyBindingTableAttachmentDescriptor",
                },
                optional: true,
                constraints: {},
            },
        },
    },
    "Wavenumber.Geometer.Contracts.StepTopologyA0.StepTopologyResolveHitRequestA0": {
        kind: "object",
        properties: {
            schema: {
                type: {
                    kind: "literal",
                    value_type: "string",
                    value: "geometry.step_topology.resolve_hit.request.a0",
                },
                optional: false,
                constraints: {},
            },
            session: {
                type: {
                    kind: "reference",
                    target: "Wavenumber.Geometer.Contracts.StepTopologyA0.SessionReference",
                },
                optional: false,
                constraints: {},
            },
            artifact_handle: {
                type: { kind: "primitive", name: "string" },
                optional: false,
                constraints: { min_length: 68, max_length: 68 },
            },
            content_sha256: {
                type: { kind: "primitive", name: "string" },
                optional: false,
                constraints: { min_length: 64, max_length: 64 },
            },
            instance_index: {
                type: { kind: "primitive", name: "uint32" },
                optional: false,
                constraints: { max_value: 99999 },
            },
            primitive_index: {
                type: { kind: "primitive", name: "uint32" },
                optional: false,
                constraints: { max_value: 999999 },
            },
            primitive_triangle_index: {
                type: { kind: "primitive", name: "uint32" },
                optional: false,
                constraints: { max_value: 9999999 },
            },
            occurrence_handle: {
                type: { kind: "primitive", name: "string" },
                optional: false,
                constraints: { min_length: 68, max_length: 68 },
            },
            body_handle: {
                type: { kind: "primitive", name: "string" },
                optional: false,
                constraints: { min_length: 68, max_length: 68 },
            },
            face_handle: {
                type: { kind: "primitive", name: "string" },
                optional: false,
                constraints: { min_length: 68, max_length: 68 },
            },
        },
    },
    "Wavenumber.Geometer.Contracts.StepTopologyA0.StepTopologyResolveHitResultA0": {
        kind: "object",
        properties: {
            schema: {
                type: {
                    kind: "literal",
                    value_type: "string",
                    value: "geometry.step_topology.resolve_hit.result.a0",
                },
                optional: false,
                constraints: {},
            },
            session: {
                type: {
                    kind: "reference",
                    target: "Wavenumber.Geometer.Contracts.StepTopologyA0.SessionReference",
                },
                optional: false,
                constraints: {},
            },
            instance_index: {
                type: { kind: "primitive", name: "uint32" },
                optional: false,
                constraints: { max_value: 99999 },
            },
            primitive_index: {
                type: { kind: "primitive", name: "uint32" },
                optional: false,
                constraints: { max_value: 999999 },
            },
            triangle_index: {
                type: { kind: "primitive", name: "uint32" },
                optional: false,
                constraints: { max_value: 9999999 },
            },
            occurrence_handle: {
                type: { kind: "primitive", name: "string" },
                optional: false,
                constraints: { min_length: 68, max_length: 68 },
            },
            body_handle: {
                type: { kind: "primitive", name: "string" },
                optional: false,
                constraints: { min_length: 68, max_length: 68 },
            },
            face_handle: {
                type: { kind: "primitive", name: "string" },
                optional: false,
                constraints: { min_length: 68, max_length: 68 },
            },
        },
    },
    "Wavenumber.Geometer.Contracts.StepTopologyA0.TessellationOptions": {
        kind: "object",
        properties: {
            linear_deflection_mm: {
                type: { kind: "primitive", name: "float64" },
                optional: false,
                constraints: { min_value: 0.000001, max_value: 1000 },
            },
            angular_deflection_rad: {
                type: { kind: "primitive", name: "float64" },
                optional: false,
                constraints: { min_value: 0.000001, max_value: Math.PI },
            },
            relative: { type: { kind: "primitive", name: "boolean" }, optional: false, constraints: {} },
            parallel: { type: { kind: "primitive", name: "boolean" }, optional: false, constraints: {} },
            source_to_render: {
                type: { kind: "array", element: { kind: "primitive", name: "float64" } },
                optional: false,
                constraints: { min_items: 12, max_items: 12 },
            },
        },
    },
    "Wavenumber.Geometer.Contracts.StepTopologyA0.ToolDescriptor": {
        kind: "object",
        properties: {
            name: {
                type: { kind: "literal", value_type: "string", value: "geometer" },
                optional: false,
                constraints: {},
            },
            release_version: {
                type: { kind: "primitive", name: "string" },
                optional: false,
                constraints: { min_length: 1, max_length: 64 },
            },
            occt_version: {
                type: { kind: "primitive", name: "string" },
                optional: false,
                constraints: { min_length: 1, max_length: 64 },
            },
        },
    },
    "Wavenumber.Geometer.Contracts.StepTopologyA0.TopologyBindingTableAttachmentDescriptor": {
        kind: "object",
        properties: {
            name: {
                type: { kind: "literal", value_type: "string", value: "topology_binding_table" },
                optional: false,
                constraints: {},
            },
            media_type: {
                type: {
                    kind: "literal",
                    value_type: "string",
                    value: "application/vnd.wavenumber.geometer.step-topology-binding-table",
                },
                optional: false,
                constraints: {},
            },
            format: {
                type: {
                    kind: "literal",
                    value_type: "string",
                    value: "wn.geometer.step-topology-binding-table.a0",
                },
                optional: false,
                constraints: {},
            },
            bytes: {
                type: { kind: "primitive", name: "uint32" },
                optional: false,
                constraints: { min_value: 1, max_value: 134217728 },
            },
            sha256: {
                type: { kind: "primitive", name: "string" },
                optional: false,
                constraints: { min_length: 64, max_length: 64 },
            },
        },
    },
    "Wavenumber.Geometer.Contracts.StepTopologyA0.TopologyPage": {
        kind: "object",
        properties: {
            definitions: {
                type: {
                    kind: "array",
                    element: {
                        kind: "reference",
                        target: "Wavenumber.Geometer.Contracts.StepTopologyA0.DefinitionSummary",
                    },
                },
                optional: false,
                constraints: { max_items: 1024 },
            },
            occurrences: {
                type: {
                    kind: "array",
                    element: {
                        kind: "reference",
                        target: "Wavenumber.Geometer.Contracts.StepTopologyA0.OccurrenceSummary",
                    },
                },
                optional: false,
                constraints: { max_items: 1024 },
            },
            bodies: {
                type: {
                    kind: "array",
                    element: {
                        kind: "reference",
                        target: "Wavenumber.Geometer.Contracts.StepTopologyA0.BodySummary",
                    },
                },
                optional: false,
                constraints: { max_items: 1024 },
            },
            shells: {
                type: {
                    kind: "array",
                    element: {
                        kind: "reference",
                        target: "Wavenumber.Geometer.Contracts.StepTopologyA0.ShellSummary",
                    },
                },
                optional: false,
                constraints: { max_items: 1024 },
            },
            faces: {
                type: {
                    kind: "array",
                    element: {
                        kind: "reference",
                        target: "Wavenumber.Geometer.Contracts.StepTopologyA0.FaceSummary",
                    },
                },
                optional: false,
                constraints: { max_items: 1024 },
            },
            next_cursor: {
                type: { kind: "primitive", name: "string" },
                optional: true,
                constraints: { max_length: 256 },
            },
        },
    },
    "Wavenumber.Geometer.Contracts.StepTopologyA0.TopologyTableAttachmentDescriptor": {
        kind: "object",
        properties: {
            name: {
                type: { kind: "literal", value_type: "string", value: "topology_table" },
                optional: false,
                constraints: {},
            },
            media_type: {
                type: {
                    kind: "literal",
                    value_type: "string",
                    value: "application/vnd.wavenumber.geometer.step-topology-table",
                },
                optional: false,
                constraints: {},
            },
            format: {
                type: {
                    kind: "literal",
                    value_type: "string",
                    value: "wn.geometer.step-topology-table.a0",
                },
                optional: false,
                constraints: {},
            },
            bytes: {
                type: { kind: "primitive", name: "uint32" },
                optional: false,
                constraints: { min_value: 1, max_value: 134217728 },
            },
            sha256: {
                type: { kind: "primitive", name: "string" },
                optional: false,
                constraints: { min_length: 64, max_length: 64 },
            },
        },
    },
};
export function decodeDiagnosticA0Json(data) {
    return decodeContractJson(data, { kind: "reference", target: "Wavenumber.Geometer.Contracts.Common.DiagnosticA0" }, declarations);
}
export function encodeDiagnosticA0Json(value) {
    return encodeContractJson(value, { kind: "reference", target: "Wavenumber.Geometer.Contracts.Common.DiagnosticA0" }, declarations);
}
export function decodeIpcCancelledA0Json(data) {
    return decodeContractJson(data, { kind: "reference", target: "Wavenumber.Geometer.Contracts.IpcA0.IpcCancelledA0" }, declarations);
}
export function encodeIpcCancelledA0Json(value) {
    return encodeContractJson(value, { kind: "reference", target: "Wavenumber.Geometer.Contracts.IpcA0.IpcCancelledA0" }, declarations);
}
export function decodeIpcCancelRejectedA0Json(data) {
    return decodeContractJson(data, { kind: "reference", target: "Wavenumber.Geometer.Contracts.IpcA0.IpcCancelRejectedA0" }, declarations);
}
export function encodeIpcCancelRejectedA0Json(value) {
    return encodeContractJson(value, { kind: "reference", target: "Wavenumber.Geometer.Contracts.IpcA0.IpcCancelRejectedA0" }, declarations);
}
export function decodeIpcHelloA0Json(data) {
    return decodeContractJson(data, { kind: "reference", target: "Wavenumber.Geometer.Contracts.IpcA0.IpcHelloA0" }, declarations);
}
export function encodeIpcHelloA0Json(value) {
    return encodeContractJson(value, { kind: "reference", target: "Wavenumber.Geometer.Contracts.IpcA0.IpcHelloA0" }, declarations);
}
export function decodeIpcOperationCatalogA0Json(data) {
    return decodeContractJson(data, { kind: "reference", target: "Wavenumber.Geometer.Contracts.IpcA0.IpcOperationCatalogA0" }, declarations);
}
export function encodeIpcOperationCatalogA0Json(value) {
    return encodeContractJson(value, { kind: "reference", target: "Wavenumber.Geometer.Contracts.IpcA0.IpcOperationCatalogA0" }, declarations);
}
export function decodeIpcProtocolErrorA0Json(data) {
    return decodeContractJson(data, { kind: "reference", target: "Wavenumber.Geometer.Contracts.IpcA0.IpcProtocolErrorA0" }, declarations);
}
export function encodeIpcProtocolErrorA0Json(value) {
    return encodeContractJson(value, { kind: "reference", target: "Wavenumber.Geometer.Contracts.IpcA0.IpcProtocolErrorA0" }, declarations);
}
export function decodeIpcReasonA0Json(data) {
    return decodeContractJson(data, { kind: "reference", target: "Wavenumber.Geometer.Contracts.IpcA0.IpcReasonA0" }, declarations);
}
export function encodeIpcReasonA0Json(value) {
    return encodeContractJson(value, { kind: "reference", target: "Wavenumber.Geometer.Contracts.IpcA0.IpcReasonA0" }, declarations);
}
export function decodeIpcRequestA0Json(data) {
    return decodeContractJson(data, { kind: "reference", target: "Wavenumber.Geometer.Contracts.IpcA0.IpcRequestA0" }, declarations);
}
export function encodeIpcRequestA0Json(value) {
    return encodeContractJson(value, { kind: "reference", target: "Wavenumber.Geometer.Contracts.IpcA0.IpcRequestA0" }, declarations);
}
export function decodeIpcShutdownAckA0Json(data) {
    return decodeContractJson(data, { kind: "reference", target: "Wavenumber.Geometer.Contracts.IpcA0.IpcShutdownAckA0" }, declarations);
}
export function encodeIpcShutdownAckA0Json(value) {
    return encodeContractJson(value, { kind: "reference", target: "Wavenumber.Geometer.Contracts.IpcA0.IpcShutdownAckA0" }, declarations);
}
export function decodeIpcWelcomeA0Json(data) {
    return decodeContractJson(data, { kind: "reference", target: "Wavenumber.Geometer.Contracts.IpcA0.IpcWelcomeA0" }, declarations);
}
export function encodeIpcWelcomeA0Json(value) {
    return encodeContractJson(value, { kind: "reference", target: "Wavenumber.Geometer.Contracts.IpcA0.IpcWelcomeA0" }, declarations);
}
export function decodeModelBoundsOptionsA0Json(data) {
    return decodeContractJson(data, {
        kind: "reference",
        target: "Wavenumber.Geometer.Contracts.ModelBoundsA0.ModelBoundsOptionsA0",
    }, declarations);
}
export function encodeModelBoundsOptionsA0Json(value) {
    return encodeContractJson(value, {
        kind: "reference",
        target: "Wavenumber.Geometer.Contracts.ModelBoundsA0.ModelBoundsOptionsA0",
    }, declarations);
}
export function decodeModelBoundsResultA0Json(data) {
    return decodeContractJson(data, {
        kind: "reference",
        target: "Wavenumber.Geometer.Contracts.ModelBoundsA0.ModelBoundsResultA0",
    }, declarations);
}
export function encodeModelBoundsResultA0Json(value) {
    return encodeContractJson(value, {
        kind: "reference",
        target: "Wavenumber.Geometer.Contracts.ModelBoundsA0.ModelBoundsResultA0",
    }, declarations);
}
export function decodeOperationOutcomeA0Json(data) {
    return decodeContractJson(data, {
        kind: "reference",
        target: "Wavenumber.Geometer.Contracts.OperationOutcomeA0.OperationOutcomeA0",
    }, declarations);
}
export function encodeOperationOutcomeA0Json(value) {
    return encodeContractJson(value, {
        kind: "reference",
        target: "Wavenumber.Geometer.Contracts.OperationOutcomeA0.OperationOutcomeA0",
    }, declarations);
}
export function decodeStepTopologyCloseRequestA0Json(data) {
    return decodeContractJson(data, {
        kind: "reference",
        target: "Wavenumber.Geometer.Contracts.StepTopologyA0.StepTopologyCloseRequestA0",
    }, declarations);
}
export function encodeStepTopologyCloseRequestA0Json(value) {
    return encodeContractJson(value, {
        kind: "reference",
        target: "Wavenumber.Geometer.Contracts.StepTopologyA0.StepTopologyCloseRequestA0",
    }, declarations);
}
export function decodeStepTopologyCloseResultA0Json(data) {
    return decodeContractJson(data, {
        kind: "reference",
        target: "Wavenumber.Geometer.Contracts.StepTopologyA0.StepTopologyCloseResultA0",
    }, declarations);
}
export function encodeStepTopologyCloseResultA0Json(value) {
    return encodeContractJson(value, {
        kind: "reference",
        target: "Wavenumber.Geometer.Contracts.StepTopologyA0.StepTopologyCloseResultA0",
    }, declarations);
}
export function decodeStepTopologyInspectRequestA0Json(data) {
    return decodeContractJson(data, {
        kind: "reference",
        target: "Wavenumber.Geometer.Contracts.StepTopologyA0.StepTopologyInspectRequestA0",
    }, declarations);
}
export function encodeStepTopologyInspectRequestA0Json(value) {
    return encodeContractJson(value, {
        kind: "reference",
        target: "Wavenumber.Geometer.Contracts.StepTopologyA0.StepTopologyInspectRequestA0",
    }, declarations);
}
export function decodeStepTopologyInspectResultA0Json(data) {
    return decodeContractJson(data, {
        kind: "reference",
        target: "Wavenumber.Geometer.Contracts.StepTopologyA0.StepTopologyInspectResultA0",
    }, declarations);
}
export function encodeStepTopologyInspectResultA0Json(value) {
    return encodeContractJson(value, {
        kind: "reference",
        target: "Wavenumber.Geometer.Contracts.StepTopologyA0.StepTopologyInspectResultA0",
    }, declarations);
}
export function decodeStepTopologyOpenRequestA0Json(data) {
    return decodeContractJson(data, {
        kind: "reference",
        target: "Wavenumber.Geometer.Contracts.StepTopologyA0.StepTopologyOpenRequestA0",
    }, declarations);
}
export function encodeStepTopologyOpenRequestA0Json(value) {
    return encodeContractJson(value, {
        kind: "reference",
        target: "Wavenumber.Geometer.Contracts.StepTopologyA0.StepTopologyOpenRequestA0",
    }, declarations);
}
export function decodeStepTopologyOpenResultA0Json(data) {
    return decodeContractJson(data, {
        kind: "reference",
        target: "Wavenumber.Geometer.Contracts.StepTopologyA0.StepTopologyOpenResultA0",
    }, declarations);
}
export function encodeStepTopologyOpenResultA0Json(value) {
    return encodeContractJson(value, {
        kind: "reference",
        target: "Wavenumber.Geometer.Contracts.StepTopologyA0.StepTopologyOpenResultA0",
    }, declarations);
}
export function decodeStepTopologyRenderRequestA0Json(data) {
    return decodeContractJson(data, {
        kind: "reference",
        target: "Wavenumber.Geometer.Contracts.StepTopologyA0.StepTopologyRenderRequestA0",
    }, declarations);
}
export function encodeStepTopologyRenderRequestA0Json(value) {
    return encodeContractJson(value, {
        kind: "reference",
        target: "Wavenumber.Geometer.Contracts.StepTopologyA0.StepTopologyRenderRequestA0",
    }, declarations);
}
export function decodeStepTopologyRenderResultA0Json(data) {
    return decodeContractJson(data, {
        kind: "reference",
        target: "Wavenumber.Geometer.Contracts.StepTopologyA0.StepTopologyRenderResultA0",
    }, declarations);
}
export function encodeStepTopologyRenderResultA0Json(value) {
    return encodeContractJson(value, {
        kind: "reference",
        target: "Wavenumber.Geometer.Contracts.StepTopologyA0.StepTopologyRenderResultA0",
    }, declarations);
}
export function decodeStepTopologyResolveHitRequestA0Json(data) {
    return decodeContractJson(data, {
        kind: "reference",
        target: "Wavenumber.Geometer.Contracts.StepTopologyA0.StepTopologyResolveHitRequestA0",
    }, declarations);
}
export function encodeStepTopologyResolveHitRequestA0Json(value) {
    return encodeContractJson(value, {
        kind: "reference",
        target: "Wavenumber.Geometer.Contracts.StepTopologyA0.StepTopologyResolveHitRequestA0",
    }, declarations);
}
export function decodeStepTopologyResolveHitResultA0Json(data) {
    return decodeContractJson(data, {
        kind: "reference",
        target: "Wavenumber.Geometer.Contracts.StepTopologyA0.StepTopologyResolveHitResultA0",
    }, declarations);
}
export function encodeStepTopologyResolveHitResultA0Json(value) {
    return encodeContractJson(value, {
        kind: "reference",
        target: "Wavenumber.Geometer.Contracts.StepTopologyA0.StepTopologyResolveHitResultA0",
    }, declarations);
}
