# Generated from wn_geometer_contract_catalog.a0.json. Do not edit.

from collections.abc import Callable
from typing import Any, cast

from ..._contract_runtime import decode_contract_json, encode_contract_json
from .models import (
    ENUM_TYPES,
    MODEL_TYPES,
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
)

DECLARATIONS: dict[str, dict[str, Any]] = {
    "Wavenumber.Geometer.Contracts.Common.DiagnosticA0": {
        "kind": "object",
        "properties": {
            "code": {
                "type": {
                    "kind": "primitive",
                    "name": "string",
                },
                "optional": False,
                "constraints": {
                    "min_length": 1,
                },
                "field": "code",
            },
            "category": {
                "type": {
                    "kind": "reference",
                    "target": "Wavenumber.Geometer.Contracts.Common.DiagnosticCategory",
                },
                "optional": False,
                "constraints": {},
                "field": "category",
            },
            "message": {
                "type": {
                    "kind": "primitive",
                    "name": "string",
                },
                "optional": False,
                "constraints": {},
                "field": "message",
            },
            "retryable": {
                "type": {
                    "kind": "primitive",
                    "name": "boolean",
                },
                "optional": False,
                "constraints": {},
                "field": "retryable",
            },
            "path": {
                "type": {
                    "kind": "primitive",
                    "name": "string",
                },
                "optional": True,
                "constraints": {},
                "field": "path",
            },
            "operation": {
                "type": {
                    "kind": "primitive",
                    "name": "string",
                },
                "optional": True,
                "constraints": {},
                "field": "operation",
            },
            "request_id": {
                "type": {
                    "kind": "primitive",
                    "name": "string",
                },
                "optional": True,
                "constraints": {},
                "field": "request_id",
            },
        },
    },
    "Wavenumber.Geometer.Contracts.Common.DiagnosticCategory": {
        "kind": "enum",
        "values": ["transport", "contract", "operation"],
    },
    "Wavenumber.Geometer.Contracts.Common.PackedAttachmentProjectionA0": {
        "kind": "object",
        "properties": {
            "schema": {
                "type": {
                    "kind": "primitive",
                    "name": "string",
                },
                "optional": False,
                "constraints": {
                    "min_length": 1,
                    "max_length": 128,
                },
                "field": "schema",
            },
            "packet": {
                "type": {
                    "kind": "reference",
                    "target": "Wavenumber.Geometer.Contracts.Common.PackedAttachmentReferenceA0",
                },
                "optional": False,
                "constraints": {},
                "field": "packet",
            },
        },
    },
    "Wavenumber.Geometer.Contracts.Common.PackedAttachmentReferenceA0": {
        "kind": "object",
        "properties": {
            "attachment": {
                "type": {
                    "kind": "primitive",
                    "name": "string",
                },
                "optional": False,
                "constraints": {
                    "min_length": 1,
                    "max_length": 128,
                },
                "field": "attachment",
            },
            "format": {
                "type": {
                    "kind": "primitive",
                    "name": "string",
                },
                "optional": False,
                "constraints": {
                    "min_length": 1,
                    "max_length": 128,
                },
                "field": "format",
            },
        },
    },
    "Wavenumber.Geometer.Contracts.IpcA0.IpcAttachmentDeclarationA0": {
        "kind": "object",
        "properties": {
            "name": {
                "type": {
                    "kind": "primitive",
                    "name": "string",
                },
                "optional": False,
                "constraints": {
                    "min_length": 1,
                    "max_length": 128,
                },
                "field": "name",
            },
            "required": {
                "type": {
                    "kind": "primitive",
                    "name": "boolean",
                },
                "optional": False,
                "constraints": {},
                "field": "required",
            },
            "media_types": {
                "type": {
                    "kind": "array",
                    "element": {
                        "kind": "primitive",
                        "name": "string",
                    },
                },
                "optional": False,
                "constraints": {
                    "min_items": 1,
                    "max_items": 16,
                },
                "field": "media_types",
            },
            "max_bytes": {
                "type": {
                    "kind": "primitive",
                    "name": "uint32",
                },
                "optional": False,
                "constraints": {
                    "max_value": 268435456,
                },
                "field": "max_bytes",
            },
        },
    },
    "Wavenumber.Geometer.Contracts.IpcA0.IpcAttachmentDescriptorA0": {
        "kind": "object",
        "properties": {
            "wasm32": {
                "type": {
                    "kind": "reference",
                    "target": "Wavenumber.Geometer.Contracts.IpcA0.IpcAttachmentLayoutWasm32A0",
                },
                "optional": False,
                "constraints": {},
                "field": "wasm32",
            },
            "pointer64": {
                "type": {
                    "kind": "reference",
                    "target": "Wavenumber.Geometer.Contracts.IpcA0.IpcAttachmentLayoutPointer64A0",
                },
                "optional": False,
                "constraints": {},
                "field": "pointer64",
            },
        },
    },
    "Wavenumber.Geometer.Contracts.IpcA0.IpcAttachmentLayoutPointer64A0": {
        "kind": "object",
        "properties": {
            "size": {
                "type": {
                    "kind": "primitive",
                    "name": "uint32",
                },
                "optional": False,
                "constraints": {
                    "min_value": 56,
                    "max_value": 56,
                },
                "field": "size",
            },
            "offsets": {
                "type": {
                    "kind": "reference",
                    "target": "Wavenumber.Geometer.Contracts.IpcA0.IpcAttachmentOffsetsPointer64A0",
                },
                "optional": False,
                "constraints": {},
                "field": "offsets",
            },
        },
    },
    "Wavenumber.Geometer.Contracts.IpcA0.IpcAttachmentLayoutWasm32A0": {
        "kind": "object",
        "properties": {
            "size": {
                "type": {
                    "kind": "primitive",
                    "name": "uint32",
                },
                "optional": False,
                "constraints": {
                    "min_value": 36,
                    "max_value": 36,
                },
                "field": "size",
            },
            "offsets": {
                "type": {
                    "kind": "reference",
                    "target": "Wavenumber.Geometer.Contracts.IpcA0.IpcAttachmentOffsetsWasm32A0",
                },
                "optional": False,
                "constraints": {},
                "field": "offsets",
            },
        },
    },
    "Wavenumber.Geometer.Contracts.IpcA0.IpcAttachmentOffsetsPointer64A0": {
        "kind": "object",
        "properties": {
            "struct_size": {
                "type": {
                    "kind": "primitive",
                    "name": "uint32",
                },
                "optional": False,
                "constraints": {
                    "max_value": 0,
                },
                "field": "struct_size",
            },
            "flags": {
                "type": {
                    "kind": "primitive",
                    "name": "uint32",
                },
                "optional": False,
                "constraints": {
                    "min_value": 4,
                    "max_value": 4,
                },
                "field": "flags",
            },
            "name": {
                "type": {
                    "kind": "primitive",
                    "name": "uint32",
                },
                "optional": False,
                "constraints": {
                    "min_value": 8,
                    "max_value": 8,
                },
                "field": "name",
            },
            "name_size": {
                "type": {
                    "kind": "primitive",
                    "name": "uint32",
                },
                "optional": False,
                "constraints": {
                    "min_value": 16,
                    "max_value": 16,
                },
                "field": "name_size",
            },
            "media_type": {
                "type": {
                    "kind": "primitive",
                    "name": "uint32",
                },
                "optional": False,
                "constraints": {
                    "min_value": 24,
                    "max_value": 24,
                },
                "field": "media_type",
            },
            "media_type_size": {
                "type": {
                    "kind": "primitive",
                    "name": "uint32",
                },
                "optional": False,
                "constraints": {
                    "min_value": 32,
                    "max_value": 32,
                },
                "field": "media_type_size",
            },
            "data": {
                "type": {
                    "kind": "primitive",
                    "name": "uint32",
                },
                "optional": False,
                "constraints": {
                    "min_value": 40,
                    "max_value": 40,
                },
                "field": "data",
            },
            "data_size": {
                "type": {
                    "kind": "primitive",
                    "name": "uint32",
                },
                "optional": False,
                "constraints": {
                    "min_value": 48,
                    "max_value": 48,
                },
                "field": "data_size",
            },
            "reserved0": {
                "type": {
                    "kind": "primitive",
                    "name": "uint32",
                },
                "optional": False,
                "constraints": {
                    "min_value": 52,
                    "max_value": 52,
                },
                "field": "reserved0",
            },
        },
    },
    "Wavenumber.Geometer.Contracts.IpcA0.IpcAttachmentOffsetsWasm32A0": {
        "kind": "object",
        "properties": {
            "struct_size": {
                "type": {
                    "kind": "primitive",
                    "name": "uint32",
                },
                "optional": False,
                "constraints": {
                    "max_value": 0,
                },
                "field": "struct_size",
            },
            "flags": {
                "type": {
                    "kind": "primitive",
                    "name": "uint32",
                },
                "optional": False,
                "constraints": {
                    "min_value": 4,
                    "max_value": 4,
                },
                "field": "flags",
            },
            "name": {
                "type": {
                    "kind": "primitive",
                    "name": "uint32",
                },
                "optional": False,
                "constraints": {
                    "min_value": 8,
                    "max_value": 8,
                },
                "field": "name",
            },
            "name_size": {
                "type": {
                    "kind": "primitive",
                    "name": "uint32",
                },
                "optional": False,
                "constraints": {
                    "min_value": 12,
                    "max_value": 12,
                },
                "field": "name_size",
            },
            "media_type": {
                "type": {
                    "kind": "primitive",
                    "name": "uint32",
                },
                "optional": False,
                "constraints": {
                    "min_value": 16,
                    "max_value": 16,
                },
                "field": "media_type",
            },
            "media_type_size": {
                "type": {
                    "kind": "primitive",
                    "name": "uint32",
                },
                "optional": False,
                "constraints": {
                    "min_value": 20,
                    "max_value": 20,
                },
                "field": "media_type_size",
            },
            "data": {
                "type": {
                    "kind": "primitive",
                    "name": "uint32",
                },
                "optional": False,
                "constraints": {
                    "min_value": 24,
                    "max_value": 24,
                },
                "field": "data",
            },
            "data_size": {
                "type": {
                    "kind": "primitive",
                    "name": "uint32",
                },
                "optional": False,
                "constraints": {
                    "min_value": 28,
                    "max_value": 28,
                },
                "field": "data_size",
            },
            "reserved0": {
                "type": {
                    "kind": "primitive",
                    "name": "uint32",
                },
                "optional": False,
                "constraints": {
                    "min_value": 32,
                    "max_value": 32,
                },
                "field": "reserved0",
            },
        },
    },
    "Wavenumber.Geometer.Contracts.IpcA0.IpcCancelledA0": {
        "kind": "object",
        "properties": {
            "status": {
                "type": {
                    "kind": "literal",
                    "value_type": "string",
                    "value": "cancelled",
                },
                "optional": False,
                "constraints": {},
                "field": "status",
            },
        },
    },
    "Wavenumber.Geometer.Contracts.IpcA0.IpcCancelRejectedA0": {
        "kind": "object",
        "properties": {
            "status": {
                "type": {
                    "kind": "literal",
                    "value_type": "string",
                    "value": "rejected",
                },
                "optional": False,
                "constraints": {},
                "field": "status",
            },
            "diagnostic": {
                "type": {
                    "kind": "reference",
                    "target": "Wavenumber.Geometer.Contracts.Common.DiagnosticA0",
                },
                "optional": False,
                "constraints": {},
                "field": "diagnostic",
            },
        },
    },
    "Wavenumber.Geometer.Contracts.IpcA0.IpcEffectiveLimitsA0": {
        "kind": "object",
        "properties": {
            "json_bytes": {
                "type": {
                    "kind": "primitive",
                    "name": "uint32",
                },
                "optional": False,
                "constraints": {
                    "max_value": 8388608,
                },
                "field": "json_bytes",
            },
            "attachment_count": {
                "type": {
                    "kind": "primitive",
                    "name": "uint32",
                },
                "optional": False,
                "constraints": {
                    "max_value": 16,
                },
                "field": "attachment_count",
            },
            "attachment_name_bytes": {
                "type": {
                    "kind": "primitive",
                    "name": "uint32",
                },
                "optional": False,
                "constraints": {
                    "max_value": 128,
                },
                "field": "attachment_name_bytes",
            },
            "attachment_media_type_bytes": {
                "type": {
                    "kind": "primitive",
                    "name": "uint32",
                },
                "optional": False,
                "constraints": {
                    "max_value": 128,
                },
                "field": "attachment_media_type_bytes",
            },
            "attachment_bytes": {
                "type": {
                    "kind": "primitive",
                    "name": "uint32",
                },
                "optional": False,
                "constraints": {
                    "max_value": 268435456,
                },
                "field": "attachment_bytes",
            },
            "frame_bytes": {
                "type": {
                    "kind": "primitive",
                    "name": "uint32",
                },
                "optional": False,
                "constraints": {
                    "max_value": 536870912,
                },
                "field": "frame_bytes",
            },
            "queued_requests": {
                "type": {
                    "kind": "primitive",
                    "name": "uint32",
                },
                "optional": False,
                "constraints": {
                    "max_value": 8,
                },
                "field": "queued_requests",
            },
            "queued_bytes": {
                "type": {
                    "kind": "primitive",
                    "name": "uint32",
                },
                "optional": False,
                "constraints": {
                    "max_value": 536870912,
                },
                "field": "queued_bytes",
            },
            "resident_request_bytes": {
                "type": {
                    "kind": "primitive",
                    "name": "uint32",
                },
                "optional": False,
                "constraints": {
                    "max_value": 536870912,
                },
                "field": "resident_request_bytes",
            },
            "pending_writer_bytes": {
                "type": {
                    "kind": "primitive",
                    "name": "uint32",
                },
                "optional": False,
                "constraints": {
                    "max_value": 536870912,
                },
                "field": "pending_writer_bytes",
            },
        },
    },
    "Wavenumber.Geometer.Contracts.IpcA0.IpcGenericAbiLimitsA0": {
        "kind": "object",
        "properties": {
            "operation_id_bytes": {
                "type": {
                    "kind": "primitive",
                    "name": "uint32",
                },
                "optional": False,
                "constraints": {
                    "max_value": 128,
                },
                "field": "operation_id_bytes",
            },
            "request_json_bytes": {
                "type": {
                    "kind": "primitive",
                    "name": "uint32",
                },
                "optional": False,
                "constraints": {
                    "max_value": 8388608,
                },
                "field": "request_json_bytes",
            },
            "response_json_bytes": {
                "type": {
                    "kind": "primitive",
                    "name": "uint32",
                },
                "optional": False,
                "constraints": {
                    "max_value": 8388608,
                },
                "field": "response_json_bytes",
            },
            "attachment_count": {
                "type": {
                    "kind": "primitive",
                    "name": "uint32",
                },
                "optional": False,
                "constraints": {
                    "max_value": 16,
                },
                "field": "attachment_count",
            },
            "attachment_name_bytes": {
                "type": {
                    "kind": "primitive",
                    "name": "uint32",
                },
                "optional": False,
                "constraints": {
                    "max_value": 128,
                },
                "field": "attachment_name_bytes",
            },
            "attachment_media_type_bytes": {
                "type": {
                    "kind": "primitive",
                    "name": "uint32",
                },
                "optional": False,
                "constraints": {
                    "max_value": 128,
                },
                "field": "attachment_media_type_bytes",
            },
            "attachment_bytes": {
                "type": {
                    "kind": "primitive",
                    "name": "uint32",
                },
                "optional": False,
                "constraints": {
                    "max_value": 268435456,
                },
                "field": "attachment_bytes",
            },
            "aggregate_attachment_bytes_native": {
                "type": {
                    "kind": "primitive",
                    "name": "uint32",
                },
                "optional": False,
                "constraints": {
                    "max_value": 536870912,
                },
                "field": "aggregate_attachment_bytes_native",
            },
            "aggregate_attachment_bytes_wasm": {
                "type": {
                    "kind": "primitive",
                    "name": "uint32",
                },
                "optional": False,
                "constraints": {
                    "max_value": 268435456,
                },
                "field": "aggregate_attachment_bytes_wasm",
            },
        },
    },
    "Wavenumber.Geometer.Contracts.IpcA0.IpcHelloA0": {
        "kind": "object",
        "properties": {
            "client_name": {
                "type": {
                    "kind": "primitive",
                    "name": "string",
                },
                "optional": False,
                "constraints": {
                    "min_length": 1,
                    "max_length": 128,
                },
                "field": "client_name",
            },
            "client_version": {
                "type": {
                    "kind": "primitive",
                    "name": "string",
                },
                "optional": False,
                "constraints": {
                    "min_length": 1,
                    "max_length": 128,
                },
                "field": "client_version",
            },
            "protocols": {
                "type": {
                    "kind": "array",
                    "element": {
                        "kind": "primitive",
                        "name": "string",
                    },
                },
                "optional": False,
                "constraints": {
                    "min_items": 1,
                    "max_items": 16,
                },
                "field": "protocols",
            },
            "capabilities": {
                "type": {
                    "kind": "array",
                    "element": {
                        "kind": "primitive",
                        "name": "string",
                    },
                },
                "optional": True,
                "constraints": {
                    "max_items": 64,
                },
                "field": "capabilities",
            },
        },
    },
    "Wavenumber.Geometer.Contracts.IpcA0.IpcOperationCatalogA0": {
        "kind": "object",
        "properties": {
            "catalog": {
                "type": {
                    "kind": "literal",
                    "value_type": "string",
                    "value": "wn.geometer.operation_catalog.a0",
                },
                "optional": False,
                "constraints": {},
                "field": "catalog",
            },
            "generic_abi": {
                "type": {
                    "kind": "literal",
                    "value_type": "string",
                    "value": "a0",
                },
                "optional": False,
                "constraints": {},
                "field": "generic_abi",
            },
            "release_version": {
                "type": {
                    "kind": "primitive",
                    "name": "string",
                },
                "optional": False,
                "constraints": {
                    "min_length": 1,
                    "max_length": 32,
                },
                "field": "release_version",
            },
            "c_abi_generation": {
                "type": {
                    "kind": "primitive",
                    "name": "uint32",
                },
                "optional": False,
                "constraints": {},
                "field": "c_abi_generation",
            },
            "operations": {
                "type": {
                    "kind": "array",
                    "element": {
                        "kind": "reference",
                        "target": "Wavenumber.Geometer.Contracts.IpcA0.IpcOperationDeclarationA0",
                    },
                },
                "optional": False,
                "constraints": {
                    "min_items": 1,
                },
                "field": "operations",
            },
            "attachment_descriptor": {
                "type": {
                    "kind": "reference",
                    "target": "Wavenumber.Geometer.Contracts.IpcA0.IpcAttachmentDescriptorA0",
                },
                "optional": False,
                "constraints": {},
                "field": "attachment_descriptor",
            },
            "limits": {
                "type": {
                    "kind": "reference",
                    "target": "Wavenumber.Geometer.Contracts.IpcA0.IpcGenericAbiLimitsA0",
                },
                "optional": False,
                "constraints": {},
                "field": "limits",
            },
        },
    },
    "Wavenumber.Geometer.Contracts.IpcA0.IpcOperationDeclarationA0": {
        "kind": "object",
        "properties": {
            "identity": {
                "type": {
                    "kind": "primitive",
                    "name": "string",
                },
                "optional": False,
                "constraints": {
                    "min_length": 1,
                    "max_length": 128,
                },
                "field": "identity",
            },
            "request_contract": {
                "type": {
                    "kind": "primitive",
                    "name": "string",
                },
                "optional": False,
                "constraints": {
                    "min_length": 1,
                    "max_length": 128,
                },
                "field": "request_contract",
            },
            "result_contract": {
                "type": {
                    "kind": "primitive",
                    "name": "string",
                },
                "optional": False,
                "constraints": {
                    "min_length": 1,
                    "max_length": 128,
                },
                "field": "result_contract",
            },
            "runtime_dispatch": {
                "type": {
                    "kind": "reference",
                    "target": "Wavenumber.Geometer.Contracts.IpcA0.IpcRuntimeDispatchA0",
                },
                "optional": False,
                "constraints": {},
                "field": "runtime_dispatch",
            },
            "input_attachments": {
                "type": {
                    "kind": "array",
                    "element": {
                        "kind": "reference",
                        "target": "Wavenumber.Geometer.Contracts.IpcA0.IpcAttachmentDeclarationA0",
                    },
                },
                "optional": False,
                "constraints": {
                    "max_items": 16,
                },
                "field": "input_attachments",
            },
            "output_attachments": {
                "type": {
                    "kind": "array",
                    "element": {
                        "kind": "reference",
                        "target": "Wavenumber.Geometer.Contracts.IpcA0.IpcAttachmentDeclarationA0",
                    },
                },
                "optional": False,
                "constraints": {
                    "max_items": 16,
                },
                "field": "output_attachments",
            },
            "request_projection": {
                "type": {
                    "kind": "reference",
                    "target": "Wavenumber.Geometer.Contracts.IpcA0.IpcPackedProjectionA0",
                },
                "optional": True,
                "constraints": {},
                "field": "request_projection",
            },
            "result_projection": {
                "type": {
                    "kind": "reference",
                    "target": "Wavenumber.Geometer.Contracts.IpcA0.IpcPackedProjectionA0",
                },
                "optional": True,
                "constraints": {},
                "field": "result_projection",
            },
        },
    },
    "Wavenumber.Geometer.Contracts.IpcA0.IpcPackedProjectionA0": {
        "kind": "object",
        "properties": {
            "kind": {
                "type": {
                    "kind": "literal",
                    "value_type": "string",
                    "value": "packed_attachment",
                },
                "optional": False,
                "constraints": {},
                "field": "kind",
            },
            "attachment_name": {
                "type": {
                    "kind": "primitive",
                    "name": "string",
                },
                "optional": False,
                "constraints": {
                    "min_length": 1,
                    "max_length": 128,
                },
                "field": "attachment_name",
            },
            "format": {
                "type": {
                    "kind": "primitive",
                    "name": "string",
                },
                "optional": False,
                "constraints": {
                    "min_length": 1,
                    "max_length": 128,
                },
                "field": "format",
            },
        },
    },
    "Wavenumber.Geometer.Contracts.IpcA0.IpcProtocolErrorA0": {
        "kind": "object",
        "properties": {
            "status": {
                "type": {
                    "kind": "literal",
                    "value_type": "string",
                    "value": "protocol_error",
                },
                "optional": False,
                "constraints": {},
                "field": "status",
            },
            "diagnostic": {
                "type": {
                    "kind": "reference",
                    "target": "Wavenumber.Geometer.Contracts.Common.DiagnosticA0",
                },
                "optional": False,
                "constraints": {},
                "field": "diagnostic",
            },
        },
    },
    "Wavenumber.Geometer.Contracts.IpcA0.IpcReasonA0": {
        "kind": "object",
        "properties": {
            "reason": {
                "type": {
                    "kind": "primitive",
                    "name": "string",
                },
                "optional": True,
                "constraints": {
                    "max_length": 1024,
                },
                "field": "reason",
            },
        },
    },
    "Wavenumber.Geometer.Contracts.IpcA0.IpcRequestA0": {
        "kind": "object",
        "properties": {
            "operation": {
                "type": {
                    "kind": "primitive",
                    "name": "string",
                },
                "optional": False,
                "constraints": {
                    "min_length": 1,
                    "max_length": 128,
                },
                "field": "operation",
            },
            "request": {
                "type": {
                    "kind": "reference",
                    "target": "Wavenumber.Geometer.Contracts.IpcA0.IpcRequestValueA0",
                },
                "optional": False,
                "constraints": {},
                "field": "request",
            },
        },
    },
    "Wavenumber.Geometer.Contracts.IpcA0.IpcRequestValueA0": {
        "kind": "union",
        "variants": [
            {
                "kind": "reference",
                "target": "Wavenumber.Geometer.Contracts.ModelBoundsA0.ModelBoundsOptionsA0",
            },
            {
                "kind": "reference",
                "target": "Wavenumber.Geometer.Contracts.Common.PackedAttachmentProjectionA0",
            },
        ],
    },
    "Wavenumber.Geometer.Contracts.IpcA0.IpcRuntimeDispatchA0": {
        "kind": "enum",
        "values": ["logical_dto", "packed_attachment"],
    },
    "Wavenumber.Geometer.Contracts.IpcA0.IpcShutdownAckA0": {
        "kind": "object",
        "properties": {
            "status": {
                "type": {
                    "kind": "literal",
                    "value_type": "string",
                    "value": "complete",
                },
                "optional": False,
                "constraints": {},
                "field": "status",
            },
            "activeRequestCompleted": {
                "type": {
                    "kind": "primitive",
                    "name": "boolean",
                },
                "optional": False,
                "constraints": {},
                "field": "active_request_completed",
            },
            "rejectedQueuedRequestCount": {
                "type": {
                    "kind": "primitive",
                    "name": "uint32",
                },
                "optional": False,
                "constraints": {},
                "field": "rejected_queued_request_count",
            },
        },
    },
    "Wavenumber.Geometer.Contracts.IpcA0.IpcWelcomeA0": {
        "kind": "object",
        "properties": {
            "release_version": {
                "type": {
                    "kind": "primitive",
                    "name": "string",
                },
                "optional": False,
                "constraints": {
                    "min_length": 1,
                    "max_length": 32,
                },
                "field": "release_version",
            },
            "c_abi_generation": {
                "type": {
                    "kind": "primitive",
                    "name": "uint32",
                },
                "optional": False,
                "constraints": {},
                "field": "c_abi_generation",
            },
            "ipc": {
                "type": {
                    "kind": "literal",
                    "value_type": "string",
                    "value": "a0",
                },
                "optional": False,
                "constraints": {},
                "field": "ipc",
            },
            "catalog_sha256": {
                "type": {
                    "kind": "primitive",
                    "name": "string",
                },
                "optional": False,
                "constraints": {
                    "min_length": 64,
                    "max_length": 64,
                },
                "field": "catalog_sha256",
            },
            "operation_catalog": {
                "type": {
                    "kind": "reference",
                    "target": "Wavenumber.Geometer.Contracts.IpcA0.IpcOperationCatalogA0",
                },
                "optional": False,
                "constraints": {},
                "field": "operation_catalog",
            },
            "limits": {
                "type": {
                    "kind": "reference",
                    "target": "Wavenumber.Geometer.Contracts.IpcA0.IpcEffectiveLimitsA0",
                },
                "optional": False,
                "constraints": {},
                "field": "limits",
            },
            "capabilities": {
                "type": {
                    "kind": "array",
                    "element": {
                        "kind": "primitive",
                        "name": "string",
                    },
                },
                "optional": False,
                "constraints": {
                    "max_items": 64,
                },
                "field": "capabilities",
            },
        },
    },
    "Wavenumber.Geometer.Contracts.ModelBoundsA0.Matrix4x4": {
        "kind": "array",
        "element": {
            "kind": "primitive",
            "name": "float64",
        },
        "constraints": {
            "min_items": 16,
            "max_items": 16,
        },
    },
    "Wavenumber.Geometer.Contracts.ModelBoundsA0.ModelBoundsOptionsA0": {
        "kind": "object",
        "properties": {
            "format": {
                "type": {
                    "kind": "reference",
                    "target": "Wavenumber.Geometer.Contracts.ModelBoundsA0.ModelFormat",
                },
                "optional": True,
                "constraints": {},
                "field": "format",
            },
            "model_transform": {
                "type": {
                    "kind": "reference",
                    "target": "Wavenumber.Geometer.Contracts.ModelBoundsA0.Matrix4x4",
                },
                "optional": True,
                "constraints": {},
                "field": "model_transform",
            },
        },
    },
    "Wavenumber.Geometer.Contracts.ModelBoundsA0.ModelBoundsResultA0": {
        "kind": "object",
        "properties": {
            "schema": {
                "type": {
                    "kind": "literal",
                    "value_type": "string",
                    "value": "geometry.model_bounds.a0",
                },
                "optional": False,
                "constraints": {},
                "field": "schema",
            },
            "units": {
                "type": {
                    "kind": "literal",
                    "value_type": "string",
                    "value": "mm",
                },
                "optional": False,
                "constraints": {},
                "field": "units",
            },
            "source": {
                "type": {
                    "kind": "reference",
                    "target": "Wavenumber.Geometer.Contracts.ModelBoundsA0.ModelBoundsSource",
                },
                "optional": False,
                "constraints": {},
                "field": "source",
            },
            "bounds": {
                "type": {
                    "kind": "reference",
                    "target": "Wavenumber.Geometer.Contracts.ModelBoundsA0.ModelBoundsValues",
                },
                "optional": False,
                "constraints": {},
                "field": "bounds",
            },
            "timings": {
                "type": {
                    "kind": "reference",
                    "target": "Wavenumber.Geometer.Contracts.ModelBoundsA0.ModelBoundsTimings",
                },
                "optional": False,
                "constraints": {},
                "field": "timings",
            },
        },
    },
    "Wavenumber.Geometer.Contracts.ModelBoundsA0.ModelBoundsSource": {
        "kind": "object",
        "properties": {
            "format": {
                "type": {
                    "kind": "reference",
                    "target": "Wavenumber.Geometer.Contracts.ModelBoundsA0.ModelFormat",
                },
                "optional": False,
                "constraints": {},
                "field": "format",
            },
            "hash": {
                "type": {
                    "kind": "primitive",
                    "name": "string",
                },
                "optional": False,
                "constraints": {},
                "field": "hash",
            },
        },
    },
    "Wavenumber.Geometer.Contracts.ModelBoundsA0.ModelBoundsTimings": {
        "kind": "object",
        "properties": {
            "model_read_ms": {
                "type": {
                    "kind": "primitive",
                    "name": "float64",
                },
                "optional": False,
                "constraints": {
                    "min_value": 0,
                },
                "field": "model_read_ms",
            },
            "bounds_ms": {
                "type": {
                    "kind": "primitive",
                    "name": "float64",
                },
                "optional": False,
                "constraints": {
                    "min_value": 0,
                },
                "field": "bounds_ms",
            },
        },
    },
    "Wavenumber.Geometer.Contracts.ModelBoundsA0.ModelBoundsValues": {
        "kind": "object",
        "properties": {
            "min": {
                "type": {
                    "kind": "reference",
                    "target": "Wavenumber.Geometer.Contracts.ModelBoundsA0.Vector3",
                },
                "optional": False,
                "constraints": {},
                "field": "min",
            },
            "max": {
                "type": {
                    "kind": "reference",
                    "target": "Wavenumber.Geometer.Contracts.ModelBoundsA0.Vector3",
                },
                "optional": False,
                "constraints": {},
                "field": "max",
            },
            "size": {
                "type": {
                    "kind": "reference",
                    "target": "Wavenumber.Geometer.Contracts.ModelBoundsA0.Vector3",
                },
                "optional": False,
                "constraints": {},
                "field": "size",
            },
            "center": {
                "type": {
                    "kind": "reference",
                    "target": "Wavenumber.Geometer.Contracts.ModelBoundsA0.Vector3",
                },
                "optional": False,
                "constraints": {},
                "field": "center",
            },
        },
    },
    "Wavenumber.Geometer.Contracts.ModelBoundsA0.ModelFormat": {
        "kind": "enum",
        "values": ["step"],
    },
    "Wavenumber.Geometer.Contracts.ModelBoundsA0.Vector3": {
        "kind": "array",
        "element": {
            "kind": "primitive",
            "name": "float64",
        },
        "constraints": {
            "min_items": 3,
            "max_items": 3,
        },
    },
    "Wavenumber.Geometer.Contracts.OperationOutcomeA0.OperationFailureA0": {
        "kind": "object",
        "properties": {
            "operation": {
                "type": {
                    "kind": "primitive",
                    "name": "string",
                },
                "optional": False,
                "constraints": {
                    "min_length": 1,
                    "max_length": 128,
                },
                "field": "operation",
            },
            "ok": {
                "type": {
                    "kind": "literal",
                    "value_type": "boolean",
                    "value": False,
                },
                "optional": False,
                "constraints": {},
                "field": "ok",
            },
            "diagnostics": {
                "type": {
                    "kind": "array",
                    "element": {
                        "kind": "reference",
                        "target": "Wavenumber.Geometer.Contracts.Common.DiagnosticA0",
                    },
                },
                "optional": False,
                "constraints": {
                    "min_items": 1,
                },
                "field": "diagnostics",
            },
        },
    },
    "Wavenumber.Geometer.Contracts.OperationOutcomeA0.OperationOutcomeA0": {
        "kind": "union",
        "variants": [
            {
                "kind": "reference",
                "target": "Wavenumber.Geometer.Contracts.OperationOutcomeA0.OperationSuccessA0",
            },
            {
                "kind": "reference",
                "target": "Wavenumber.Geometer.Contracts.OperationOutcomeA0.OperationFailureA0",
            },
        ],
    },
    "Wavenumber.Geometer.Contracts.OperationOutcomeA0.OperationResultValueA0": {
        "kind": "union",
        "variants": [
            {
                "kind": "reference",
                "target": "Wavenumber.Geometer.Contracts.ModelBoundsA0.ModelBoundsResultA0",
            },
            {
                "kind": "reference",
                "target": "Wavenumber.Geometer.Contracts.Common.PackedAttachmentProjectionA0",
            },
        ],
    },
    "Wavenumber.Geometer.Contracts.OperationOutcomeA0.OperationSuccessA0": {
        "kind": "object",
        "properties": {
            "operation": {
                "type": {
                    "kind": "primitive",
                    "name": "string",
                },
                "optional": False,
                "constraints": {
                    "min_length": 1,
                    "max_length": 128,
                },
                "field": "operation",
            },
            "ok": {
                "type": {
                    "kind": "literal",
                    "value_type": "boolean",
                    "value": True,
                },
                "optional": False,
                "constraints": {},
                "field": "ok",
            },
            "result": {
                "type": {
                    "kind": "reference",
                    "target": "Wavenumber.Geometer.Contracts.OperationOutcomeA0.OperationResultValueA0",
                },
                "optional": False,
                "constraints": {},
                "field": "result",
            },
        },
    },
}


def decode_diagnostic_a0_json(data: str | bytes | bytearray | memoryview) -> DiagnosticA0:
    return cast(
        DiagnosticA0,
        decode_contract_json(
            data, "Wavenumber.Geometer.Contracts.Common.DiagnosticA0", DECLARATIONS, MODEL_TYPES, ENUM_TYPES
        ),
    )


def encode_diagnostic_a0_json(value: DiagnosticA0) -> bytes:
    return encode_contract_json(
        value, "Wavenumber.Geometer.Contracts.Common.DiagnosticA0", DECLARATIONS, MODEL_TYPES, ENUM_TYPES
    )


def decode_ipc_cancelled_a0_json(data: str | bytes | bytearray | memoryview) -> IpcCancelledA0:
    return cast(
        IpcCancelledA0,
        decode_contract_json(
            data, "Wavenumber.Geometer.Contracts.IpcA0.IpcCancelledA0", DECLARATIONS, MODEL_TYPES, ENUM_TYPES
        ),
    )


def encode_ipc_cancelled_a0_json(value: IpcCancelledA0) -> bytes:
    return encode_contract_json(
        value, "Wavenumber.Geometer.Contracts.IpcA0.IpcCancelledA0", DECLARATIONS, MODEL_TYPES, ENUM_TYPES
    )


def decode_ipc_cancel_rejected_a0_json(data: str | bytes | bytearray | memoryview) -> IpcCancelRejectedA0:
    return cast(
        IpcCancelRejectedA0,
        decode_contract_json(
            data, "Wavenumber.Geometer.Contracts.IpcA0.IpcCancelRejectedA0", DECLARATIONS, MODEL_TYPES, ENUM_TYPES
        ),
    )


def encode_ipc_cancel_rejected_a0_json(value: IpcCancelRejectedA0) -> bytes:
    return encode_contract_json(
        value, "Wavenumber.Geometer.Contracts.IpcA0.IpcCancelRejectedA0", DECLARATIONS, MODEL_TYPES, ENUM_TYPES
    )


def decode_ipc_hello_a0_json(data: str | bytes | bytearray | memoryview) -> IpcHelloA0:
    return cast(
        IpcHelloA0,
        decode_contract_json(
            data, "Wavenumber.Geometer.Contracts.IpcA0.IpcHelloA0", DECLARATIONS, MODEL_TYPES, ENUM_TYPES
        ),
    )


def encode_ipc_hello_a0_json(value: IpcHelloA0) -> bytes:
    return encode_contract_json(
        value, "Wavenumber.Geometer.Contracts.IpcA0.IpcHelloA0", DECLARATIONS, MODEL_TYPES, ENUM_TYPES
    )


def decode_ipc_operation_catalog_a0_json(data: str | bytes | bytearray | memoryview) -> IpcOperationCatalogA0:
    return cast(
        IpcOperationCatalogA0,
        decode_contract_json(
            data, "Wavenumber.Geometer.Contracts.IpcA0.IpcOperationCatalogA0", DECLARATIONS, MODEL_TYPES, ENUM_TYPES
        ),
    )


def encode_ipc_operation_catalog_a0_json(value: IpcOperationCatalogA0) -> bytes:
    return encode_contract_json(
        value, "Wavenumber.Geometer.Contracts.IpcA0.IpcOperationCatalogA0", DECLARATIONS, MODEL_TYPES, ENUM_TYPES
    )


def decode_ipc_protocol_error_a0_json(data: str | bytes | bytearray | memoryview) -> IpcProtocolErrorA0:
    return cast(
        IpcProtocolErrorA0,
        decode_contract_json(
            data, "Wavenumber.Geometer.Contracts.IpcA0.IpcProtocolErrorA0", DECLARATIONS, MODEL_TYPES, ENUM_TYPES
        ),
    )


def encode_ipc_protocol_error_a0_json(value: IpcProtocolErrorA0) -> bytes:
    return encode_contract_json(
        value, "Wavenumber.Geometer.Contracts.IpcA0.IpcProtocolErrorA0", DECLARATIONS, MODEL_TYPES, ENUM_TYPES
    )


def decode_ipc_reason_a0_json(data: str | bytes | bytearray | memoryview) -> IpcReasonA0:
    return cast(
        IpcReasonA0,
        decode_contract_json(
            data, "Wavenumber.Geometer.Contracts.IpcA0.IpcReasonA0", DECLARATIONS, MODEL_TYPES, ENUM_TYPES
        ),
    )


def encode_ipc_reason_a0_json(value: IpcReasonA0) -> bytes:
    return encode_contract_json(
        value, "Wavenumber.Geometer.Contracts.IpcA0.IpcReasonA0", DECLARATIONS, MODEL_TYPES, ENUM_TYPES
    )


def decode_ipc_request_a0_json(data: str | bytes | bytearray | memoryview) -> IpcRequestA0:
    return cast(
        IpcRequestA0,
        decode_contract_json(
            data, "Wavenumber.Geometer.Contracts.IpcA0.IpcRequestA0", DECLARATIONS, MODEL_TYPES, ENUM_TYPES
        ),
    )


def encode_ipc_request_a0_json(value: IpcRequestA0) -> bytes:
    return encode_contract_json(
        value, "Wavenumber.Geometer.Contracts.IpcA0.IpcRequestA0", DECLARATIONS, MODEL_TYPES, ENUM_TYPES
    )


def decode_ipc_shutdown_ack_a0_json(data: str | bytes | bytearray | memoryview) -> IpcShutdownAckA0:
    return cast(
        IpcShutdownAckA0,
        decode_contract_json(
            data, "Wavenumber.Geometer.Contracts.IpcA0.IpcShutdownAckA0", DECLARATIONS, MODEL_TYPES, ENUM_TYPES
        ),
    )


def encode_ipc_shutdown_ack_a0_json(value: IpcShutdownAckA0) -> bytes:
    return encode_contract_json(
        value, "Wavenumber.Geometer.Contracts.IpcA0.IpcShutdownAckA0", DECLARATIONS, MODEL_TYPES, ENUM_TYPES
    )


def decode_ipc_welcome_a0_json(data: str | bytes | bytearray | memoryview) -> IpcWelcomeA0:
    return cast(
        IpcWelcomeA0,
        decode_contract_json(
            data, "Wavenumber.Geometer.Contracts.IpcA0.IpcWelcomeA0", DECLARATIONS, MODEL_TYPES, ENUM_TYPES
        ),
    )


def encode_ipc_welcome_a0_json(value: IpcWelcomeA0) -> bytes:
    return encode_contract_json(
        value, "Wavenumber.Geometer.Contracts.IpcA0.IpcWelcomeA0", DECLARATIONS, MODEL_TYPES, ENUM_TYPES
    )


def decode_model_bounds_options_a0_json(data: str | bytes | bytearray | memoryview) -> ModelBoundsOptionsA0:
    return cast(
        ModelBoundsOptionsA0,
        decode_contract_json(
            data,
            "Wavenumber.Geometer.Contracts.ModelBoundsA0.ModelBoundsOptionsA0",
            DECLARATIONS,
            MODEL_TYPES,
            ENUM_TYPES,
        ),
    )


def encode_model_bounds_options_a0_json(value: ModelBoundsOptionsA0) -> bytes:
    return encode_contract_json(
        value, "Wavenumber.Geometer.Contracts.ModelBoundsA0.ModelBoundsOptionsA0", DECLARATIONS, MODEL_TYPES, ENUM_TYPES
    )


def decode_model_bounds_result_a0_json(data: str | bytes | bytearray | memoryview) -> ModelBoundsResultA0:
    return cast(
        ModelBoundsResultA0,
        decode_contract_json(
            data,
            "Wavenumber.Geometer.Contracts.ModelBoundsA0.ModelBoundsResultA0",
            DECLARATIONS,
            MODEL_TYPES,
            ENUM_TYPES,
        ),
    )


def encode_model_bounds_result_a0_json(value: ModelBoundsResultA0) -> bytes:
    return encode_contract_json(
        value, "Wavenumber.Geometer.Contracts.ModelBoundsA0.ModelBoundsResultA0", DECLARATIONS, MODEL_TYPES, ENUM_TYPES
    )


def decode_operation_outcome_a0_json(data: str | bytes | bytearray | memoryview) -> OperationOutcomeA0:
    return cast(
        OperationOutcomeA0,
        decode_contract_json(
            data,
            "Wavenumber.Geometer.Contracts.OperationOutcomeA0.OperationOutcomeA0",
            DECLARATIONS,
            MODEL_TYPES,
            ENUM_TYPES,
        ),
    )


def encode_operation_outcome_a0_json(value: OperationOutcomeA0) -> bytes:
    return encode_contract_json(
        value,
        "Wavenumber.Geometer.Contracts.OperationOutcomeA0.OperationOutcomeA0",
        DECLARATIONS,
        MODEL_TYPES,
        ENUM_TYPES,
    )


ROOT_DECODERS: dict[str, Callable[[str | bytes | bytearray | memoryview], Any]] = {
    "geometry.common.diagnostic.a0": decode_diagnostic_a0_json,
    "geometer.ipc.cancelled.a0": decode_ipc_cancelled_a0_json,
    "geometer.ipc.cancel_rejected.a0": decode_ipc_cancel_rejected_a0_json,
    "geometer.ipc.hello.a0": decode_ipc_hello_a0_json,
    "geometer.ipc.operation_catalog.a0": decode_ipc_operation_catalog_a0_json,
    "geometer.ipc.protocol_error.a0": decode_ipc_protocol_error_a0_json,
    "geometer.ipc.reason.a0": decode_ipc_reason_a0_json,
    "geometer.ipc.request.a0": decode_ipc_request_a0_json,
    "geometer.ipc.shutdown_ack.a0": decode_ipc_shutdown_ack_a0_json,
    "geometer.ipc.welcome.a0": decode_ipc_welcome_a0_json,
    "geometry.model_bounds.options.a0": decode_model_bounds_options_a0_json,
    "geometry.model_bounds.a0": decode_model_bounds_result_a0_json,
    "geometer.operation.outcome.a0": decode_operation_outcome_a0_json,
}
