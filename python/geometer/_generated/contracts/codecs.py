# Generated from wn_geometer_contract_catalog.a0.json. Do not edit.

from collections.abc import Callable
from typing import Any, cast

from ..._contract_runtime import decode_contract_json, encode_contract_json
from .models import (
    ENUM_TYPES,
    MODEL_TYPES,
    DiagnosticA0,
    HlrProjectionOptionsA0,
    HlrProjectionResultA0,
    IpcCancelledA0,
    IpcCancelRejectedA0,
    IpcHelloA0,
    IpcOperationCatalogA0,
    IpcProtocolErrorA0,
    IpcReasonA0,
    IpcRequestA0,
    IpcShutdownAckA0,
    IpcWelcomeA0,
    MeshIllustrationInputA0,
    MeshIllustrationResultA0,
    MeshIllustrationStyleA0,
    ModelBoundsOptionsA0,
    ModelBoundsResultA0,
    MeshCollectionA0,
    ModelTessellationRequestA0,
    ModelTessellationResultA0,
    OperationOutcomeA0,
    StepTopologyAnalyzeRecoveryRequestA0,
    StepTopologyAnalyzeRecoveryResultA0,
    StepTopologyApplyHierarchyRequestA0,
    StepTopologyApplyHierarchyResultA0,
    StepTopologyApplyLogicalGroupsRequestA0,
    StepTopologyApplyLogicalGroupsResultA0,
    StepTopologyApplyMetadataProbesRequestA0,
    StepTopologyApplyMetadataProbesResultA0,
    StepTopologyCheckpointEditJournalRequestA0,
    StepTopologyCheckpointEditJournalResultA0,
    StepTopologyCloseRequestA0,
    StepTopologyCloseResultA0,
    StepTopologyInspectRequestA0,
    StepTopologyInspectResultA0,
    StepTopologyOpenRequestA0,
    StepTopologyOpenResultA0,
    StepTopologyRenderRequestA0,
    StepTopologyRenderResultA0,
    StepTopologyResolveHitRequestA0,
    StepTopologyResolveHitResultA0,
    StepTopologyRestoreRequestA0,
    StepTopologyRestoreResultA0,
    StepTopologySaveRequestA0,
    StepTopologySaveResultA0,
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
    "Wavenumber.Geometer.Contracts.HlrProjectionA0.FastHlrLimitsA0": {
        "kind": "object",
        "properties": {
            "max_vertices": {
                "type": {
                    "kind": "primitive",
                    "name": "uint32",
                },
                "optional": True,
                "constraints": {
                    "min_value": 1,
                    "max_value": 4294967295,
                },
                "field": "max_vertices",
            },
            "max_triangles": {
                "type": {
                    "kind": "primitive",
                    "name": "uint32",
                },
                "optional": True,
                "constraints": {
                    "min_value": 1,
                    "max_value": 4294967295,
                },
                "field": "max_triangles",
            },
            "max_edges": {
                "type": {
                    "kind": "primitive",
                    "name": "uint32",
                },
                "optional": True,
                "constraints": {
                    "min_value": 1,
                    "max_value": 4294967295,
                },
                "field": "max_edges",
            },
            "max_grid_references": {
                "type": {
                    "kind": "primitive",
                    "name": "uint32",
                },
                "optional": True,
                "constraints": {
                    "min_value": 1,
                    "max_value": 4294967295,
                },
                "field": "max_grid_references",
            },
            "max_candidate_pairs": {
                "type": {
                    "kind": "primitive",
                    "name": "uint32",
                },
                "optional": True,
                "constraints": {
                    "min_value": 1,
                    "max_value": 4294967295,
                },
                "field": "max_candidate_pairs",
            },
            "max_fragments": {
                "type": {
                    "kind": "primitive",
                    "name": "uint32",
                },
                "optional": True,
                "constraints": {
                    "min_value": 1,
                    "max_value": 4294967295,
                },
                "field": "max_fragments",
            },
            "max_output_segments": {
                "type": {
                    "kind": "primitive",
                    "name": "uint32",
                },
                "optional": True,
                "constraints": {
                    "min_value": 1,
                    "max_value": 4294967295,
                },
                "field": "max_output_segments",
            },
        },
    },
    "Wavenumber.Geometer.Contracts.HlrProjectionA0.FastHlrOptionsA0": {
        "kind": "object",
        "properties": {
            "include_boundaries": {
                "type": {
                    "kind": "primitive",
                    "name": "boolean",
                },
                "optional": True,
                "constraints": {},
                "field": "include_boundaries",
            },
            "include_creases": {
                "type": {
                    "kind": "primitive",
                    "name": "boolean",
                },
                "optional": True,
                "constraints": {},
                "field": "include_creases",
            },
            "include_silhouettes": {
                "type": {
                    "kind": "primitive",
                    "name": "boolean",
                },
                "optional": True,
                "constraints": {},
                "field": "include_silhouettes",
            },
            "include_hidden": {
                "type": {
                    "kind": "primitive",
                    "name": "boolean",
                },
                "optional": True,
                "constraints": {},
                "field": "include_hidden",
            },
            "suppress_coplanar_seams": {
                "type": {
                    "kind": "primitive",
                    "name": "boolean",
                },
                "optional": True,
                "constraints": {},
                "field": "suppress_coplanar_seams",
            },
            "crease_angle_rad": {
                "type": {
                    "kind": "primitive",
                    "name": "float64",
                },
                "optional": True,
                "constraints": {
                    "min_value": 0,
                    "max_value": 3.141592653589793,
                },
                "field": "crease_angle_rad",
            },
            "weld_tolerance": {
                "type": {
                    "kind": "primitive",
                    "name": "float64",
                },
                "optional": True,
                "constraints": {
                    "min_value_exclusive": 0,
                },
                "field": "weld_tolerance",
            },
            "projected_tolerance": {
                "type": {
                    "kind": "primitive",
                    "name": "float64",
                },
                "optional": True,
                "constraints": {
                    "min_value_exclusive": 0,
                },
                "field": "projected_tolerance",
            },
            "depth_tolerance": {
                "type": {
                    "kind": "primitive",
                    "name": "float64",
                },
                "optional": True,
                "constraints": {
                    "min_value": 0,
                },
                "field": "depth_tolerance",
            },
            "coplanar_seam_angle_rad": {
                "type": {
                    "kind": "primitive",
                    "name": "float64",
                },
                "optional": True,
                "constraints": {
                    "min_value": 0,
                    "max_value": 1.5707963267948966,
                },
                "field": "coplanar_seam_angle_rad",
            },
            "coplanar_seam_depth_tolerance": {
                "type": {
                    "kind": "primitive",
                    "name": "float64",
                },
                "optional": True,
                "constraints": {
                    "min_value": 0,
                },
                "field": "coplanar_seam_depth_tolerance",
            },
            "coplanar_seam_lateral_tolerance": {
                "type": {
                    "kind": "primitive",
                    "name": "float64",
                },
                "optional": True,
                "constraints": {
                    "min_value": 0,
                },
                "field": "coplanar_seam_lateral_tolerance",
            },
            "limits": {
                "type": {
                    "kind": "reference",
                    "target": "Wavenumber.Geometer.Contracts.HlrProjectionA0.FastHlrLimitsA0",
                },
                "optional": True,
                "constraints": {},
                "field": "limits",
            },
        },
    },
    "Wavenumber.Geometer.Contracts.HlrProjectionA0.HlrCurveMode": {
        "kind": "enum",
        "values": ["native_arcs", "polyline"],
    },
    "Wavenumber.Geometer.Contracts.HlrProjectionA0.HlrMatrix4x4": {
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
    "Wavenumber.Geometer.Contracts.HlrProjectionA0.HlrMeshDeflectionMode": {
        "kind": "enum",
        "values": ["absolute", "bbox-relative"],
    },
    "Wavenumber.Geometer.Contracts.HlrProjectionA0.HlrOutlineAlgorithm": {
        "kind": "enum",
        "values": ["hlr-close", "mesh-shadow", "fast-mesh-shadow"],
    },
    "Wavenumber.Geometer.Contracts.HlrProjectionA0.HlrProjectedView": {
        "kind": "object",
        "properties": {
            "id": {
                "type": {
                    "kind": "primitive",
                    "name": "string",
                },
                "optional": False,
                "constraints": {
                    "min_length": 1,
                    "max_length": 128,
                },
                "field": "id",
            },
            "direction": {
                "type": {
                    "kind": "reference",
                    "target": "Wavenumber.Geometer.Contracts.HlrProjectionA0.HlrVector3",
                },
                "optional": False,
                "constraints": {},
                "field": "direction",
            },
            "up": {
                "type": {
                    "kind": "reference",
                    "target": "Wavenumber.Geometer.Contracts.HlrProjectionA0.HlrVector3",
                },
                "optional": False,
                "constraints": {},
                "field": "up",
            },
            "modes": {
                "type": {
                    "kind": "reference",
                    "target": "Wavenumber.Geometer.Contracts.HlrProjectionA0.HlrProjectionModes",
                },
                "optional": False,
                "constraints": {},
                "field": "modes",
            },
        },
    },
    "Wavenumber.Geometer.Contracts.HlrProjectionA0.HlrProjectionAlgorithm": {
        "kind": "enum",
        "values": ["poly", "exact", "fast"],
    },
    "Wavenumber.Geometer.Contracts.HlrProjectionA0.HlrProjectionModes": {
        "kind": "object",
        "properties": {
            "outline": {
                "type": {
                    "kind": "reference",
                    "target": "Wavenumber.Geometer.Contracts.HlrProjectionA0.ProjectedGeometry",
                },
                "optional": False,
                "constraints": {},
                "field": "outline",
            },
            "detail": {
                "type": {
                    "kind": "reference",
                    "target": "Wavenumber.Geometer.Contracts.HlrProjectionA0.ProjectedGeometry",
                },
                "optional": False,
                "constraints": {},
                "field": "detail",
            },
            "bbox": {
                "type": {
                    "kind": "reference",
                    "target": "Wavenumber.Geometer.Contracts.HlrProjectionA0.ProjectedGeometry",
                },
                "optional": False,
                "constraints": {},
                "field": "bbox",
            },
        },
    },
    "Wavenumber.Geometer.Contracts.HlrProjectionA0.HlrProjectionOptionsA0": {
        "kind": "object",
        "properties": {
            "views": {
                "type": {
                    "kind": "array",
                    "element": {
                        "kind": "reference",
                        "target": "Wavenumber.Geometer.Contracts.HlrProjectionA0.HlrViewSpec",
                    },
                },
                "optional": True,
                "constraints": {
                    "max_items": 64,
                },
                "field": "views",
            },
            "output_outline": {
                "type": {
                    "kind": "primitive",
                    "name": "boolean",
                },
                "optional": True,
                "constraints": {},
                "field": "output_outline",
            },
            "output_detail": {
                "type": {
                    "kind": "primitive",
                    "name": "boolean",
                },
                "optional": True,
                "constraints": {},
                "field": "output_detail",
            },
            "output_bbox": {
                "type": {
                    "kind": "primitive",
                    "name": "boolean",
                },
                "optional": True,
                "constraints": {},
                "field": "output_bbox",
            },
            "model_transform": {
                "type": {
                    "kind": "reference",
                    "target": "Wavenumber.Geometer.Contracts.HlrProjectionA0.HlrMatrix4x4",
                },
                "optional": True,
                "constraints": {},
                "field": "model_transform",
            },
            "strip_root_placement": {
                "type": {
                    "kind": "primitive",
                    "name": "boolean",
                },
                "optional": True,
                "constraints": {},
                "field": "strip_root_placement",
            },
            "curve_mode": {
                "type": {
                    "kind": "reference",
                    "target": "Wavenumber.Geometer.Contracts.HlrProjectionA0.HlrCurveMode",
                },
                "optional": True,
                "constraints": {},
                "field": "curve_mode",
            },
            "samples_per_curve": {
                "type": {
                    "kind": "primitive",
                    "name": "uint32",
                },
                "optional": True,
                "constraints": {
                    "min_value": 1,
                    "max_value": 100000,
                },
                "field": "samples_per_curve",
            },
            "round_digits": {
                "type": {
                    "kind": "primitive",
                    "name": "uint32",
                },
                "optional": True,
                "constraints": {
                    "min_value": 0,
                    "max_value": 9,
                },
                "field": "round_digits",
            },
            "edge_v_sharp": {
                "type": {
                    "kind": "primitive",
                    "name": "boolean",
                },
                "optional": True,
                "constraints": {},
                "field": "edge_v_sharp",
            },
            "edge_v_outline": {
                "type": {
                    "kind": "primitive",
                    "name": "boolean",
                },
                "optional": True,
                "constraints": {},
                "field": "edge_v_outline",
            },
            "edge_v_smooth": {
                "type": {
                    "kind": "primitive",
                    "name": "boolean",
                },
                "optional": True,
                "constraints": {},
                "field": "edge_v_smooth",
            },
            "edge_v_sewn": {
                "type": {
                    "kind": "primitive",
                    "name": "boolean",
                },
                "optional": True,
                "constraints": {},
                "field": "edge_v_sewn",
            },
            "edge_v_iso": {
                "type": {
                    "kind": "primitive",
                    "name": "boolean",
                },
                "optional": True,
                "constraints": {},
                "field": "edge_v_iso",
            },
            "edge_h_sharp": {
                "type": {
                    "kind": "primitive",
                    "name": "boolean",
                },
                "optional": True,
                "constraints": {},
                "field": "edge_h_sharp",
            },
            "edge_h_outline": {
                "type": {
                    "kind": "primitive",
                    "name": "boolean",
                },
                "optional": True,
                "constraints": {},
                "field": "edge_h_outline",
            },
            "edge_h_smooth": {
                "type": {
                    "kind": "primitive",
                    "name": "boolean",
                },
                "optional": True,
                "constraints": {},
                "field": "edge_h_smooth",
            },
            "edge_h_sewn": {
                "type": {
                    "kind": "primitive",
                    "name": "boolean",
                },
                "optional": True,
                "constraints": {},
                "field": "edge_h_sewn",
            },
            "edge_h_iso": {
                "type": {
                    "kind": "primitive",
                    "name": "boolean",
                },
                "optional": True,
                "constraints": {},
                "field": "edge_h_iso",
            },
            "union_outline_polygons": {
                "type": {
                    "kind": "primitive",
                    "name": "boolean",
                },
                "optional": True,
                "constraints": {},
                "field": "union_outline_polygons",
            },
            "projection_algorithm": {
                "type": {
                    "kind": "reference",
                    "target": "Wavenumber.Geometer.Contracts.HlrProjectionA0.HlrProjectionAlgorithm",
                },
                "optional": True,
                "constraints": {},
                "field": "projection_algorithm",
            },
            "mesh_linear_deflection": {
                "type": {
                    "kind": "primitive",
                    "name": "float64",
                },
                "optional": True,
                "constraints": {
                    "min_value": 0,
                },
                "field": "mesh_linear_deflection",
            },
            "mesh_angular_deflection": {
                "type": {
                    "kind": "primitive",
                    "name": "float64",
                },
                "optional": True,
                "constraints": {
                    "min_value": 0,
                    "max_value": 3.141592653589793,
                },
                "field": "mesh_angular_deflection",
            },
            "mesh_relative": {
                "type": {
                    "kind": "primitive",
                    "name": "boolean",
                },
                "optional": True,
                "constraints": {},
                "field": "mesh_relative",
            },
            "mesh_deflection_mode": {
                "type": {
                    "kind": "reference",
                    "target": "Wavenumber.Geometer.Contracts.HlrProjectionA0.HlrMeshDeflectionMode",
                },
                "optional": True,
                "constraints": {},
                "field": "mesh_deflection_mode",
            },
            "mesh_deflection_coefficient": {
                "type": {
                    "kind": "primitive",
                    "name": "float64",
                },
                "optional": True,
                "constraints": {
                    "min_value": 0,
                },
                "field": "mesh_deflection_coefficient",
            },
            "outline_algorithm": {
                "type": {
                    "kind": "reference",
                    "target": "Wavenumber.Geometer.Contracts.HlrProjectionA0.HlrOutlineAlgorithm",
                },
                "optional": True,
                "constraints": {},
                "field": "outline_algorithm",
            },
            "hlr_angle_tolerance": {
                "type": {
                    "kind": "primitive",
                    "name": "float64",
                },
                "optional": True,
                "constraints": {
                    "min_value": 0,
                    "max_value": 3.141592653589793,
                },
                "field": "hlr_angle_tolerance",
            },
            "fast": {
                "type": {
                    "kind": "reference",
                    "target": "Wavenumber.Geometer.Contracts.HlrProjectionA0.FastHlrOptionsA0",
                },
                "optional": True,
                "constraints": {},
                "field": "fast",
            },
        },
    },
    "Wavenumber.Geometer.Contracts.HlrProjectionA0.HlrProjectionResultA0": {
        "kind": "object",
        "properties": {
            "schema": {
                "type": {
                    "kind": "literal",
                    "value_type": "string",
                    "value": "geometry.hlr_projection.result.a0",
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
                    "target": "Wavenumber.Geometer.Contracts.HlrProjectionA0.HlrProjectionSource",
                },
                "optional": False,
                "constraints": {},
                "field": "source",
            },
            "views": {
                "type": {
                    "kind": "array",
                    "element": {
                        "kind": "reference",
                        "target": "Wavenumber.Geometer.Contracts.HlrProjectionA0.HlrProjectedView",
                    },
                },
                "optional": False,
                "constraints": {
                    "max_items": 64,
                },
                "field": "views",
            },
            "timings": {
                "type": {
                    "kind": "reference",
                    "target": "Wavenumber.Geometer.Contracts.HlrProjectionA0.HlrProjectionTimings",
                },
                "optional": False,
                "constraints": {},
                "field": "timings",
            },
        },
    },
    "Wavenumber.Geometer.Contracts.HlrProjectionA0.HlrProjectionSource": {
        "kind": "object",
        "properties": {
            "kind": {
                "type": {
                    "kind": "reference",
                    "target": "Wavenumber.Geometer.Contracts.HlrProjectionA0.HlrSourceKind",
                },
                "optional": False,
                "constraints": {},
                "field": "kind",
            },
            "hash": {
                "type": {
                    "kind": "primitive",
                    "name": "string",
                },
                "optional": False,
                "constraints": {
                    "min_length": 64,
                    "max_length": 64,
                },
                "field": "hash",
            },
        },
    },
    "Wavenumber.Geometer.Contracts.HlrProjectionA0.HlrProjectionTimings": {
        "kind": "object",
        "properties": {
            "step_read_ms": {
                "type": {
                    "kind": "primitive",
                    "name": "float64",
                },
                "optional": False,
                "constraints": {
                    "min_value": 0,
                },
                "field": "step_read_ms",
            },
            "mesh_ms": {
                "type": {
                    "kind": "primitive",
                    "name": "float64",
                },
                "optional": False,
                "constraints": {
                    "min_value": 0,
                },
                "field": "mesh_ms",
            },
            "hlr_ms": {
                "type": {
                    "kind": "primitive",
                    "name": "float64",
                },
                "optional": False,
                "constraints": {
                    "min_value": 0,
                },
                "field": "hlr_ms",
            },
            "extract_ms": {
                "type": {
                    "kind": "primitive",
                    "name": "float64",
                },
                "optional": False,
                "constraints": {
                    "min_value": 0,
                },
                "field": "extract_ms",
            },
        },
    },
    "Wavenumber.Geometer.Contracts.HlrProjectionA0.HlrSourceKind": {
        "kind": "enum",
        "values": ["step", "indexed_mesh"],
    },
    "Wavenumber.Geometer.Contracts.HlrProjectionA0.HlrVector2": {
        "kind": "array",
        "element": {
            "kind": "primitive",
            "name": "float64",
        },
        "constraints": {
            "min_items": 2,
            "max_items": 2,
        },
    },
    "Wavenumber.Geometer.Contracts.HlrProjectionA0.HlrVector3": {
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
    "Wavenumber.Geometer.Contracts.HlrProjectionA0.HlrViewSpec": {
        "kind": "object",
        "properties": {
            "id": {
                "type": {
                    "kind": "primitive",
                    "name": "string",
                },
                "optional": False,
                "constraints": {
                    "min_length": 1,
                    "max_length": 128,
                },
                "field": "id",
            },
            "direction": {
                "type": {
                    "kind": "reference",
                    "target": "Wavenumber.Geometer.Contracts.HlrProjectionA0.HlrVector3",
                },
                "optional": False,
                "constraints": {},
                "field": "direction",
            },
            "up": {
                "type": {
                    "kind": "reference",
                    "target": "Wavenumber.Geometer.Contracts.HlrProjectionA0.HlrVector3",
                },
                "optional": False,
                "constraints": {},
                "field": "up",
            },
        },
    },
    "Wavenumber.Geometer.Contracts.HlrProjectionA0.ProjectedArc": {
        "kind": "object",
        "properties": {
            "start": {
                "type": {
                    "kind": "reference",
                    "target": "Wavenumber.Geometer.Contracts.HlrProjectionA0.HlrVector2",
                },
                "optional": False,
                "constraints": {},
                "field": "start",
            },
            "end": {
                "type": {
                    "kind": "reference",
                    "target": "Wavenumber.Geometer.Contracts.HlrProjectionA0.HlrVector2",
                },
                "optional": False,
                "constraints": {},
                "field": "end",
            },
            "center": {
                "type": {
                    "kind": "reference",
                    "target": "Wavenumber.Geometer.Contracts.HlrProjectionA0.HlrVector2",
                },
                "optional": False,
                "constraints": {},
                "field": "center",
            },
            "radius": {
                "type": {
                    "kind": "primitive",
                    "name": "float64",
                },
                "optional": False,
                "constraints": {
                    "min_value": 0,
                },
                "field": "radius",
            },
            "extent_rad": {
                "type": {
                    "kind": "primitive",
                    "name": "float64",
                },
                "optional": False,
                "constraints": {
                    "min_value": 0,
                    "max_value": 6.283185307179586,
                },
                "field": "extent_rad",
            },
            "ccw": {
                "type": {
                    "kind": "primitive",
                    "name": "boolean",
                },
                "optional": False,
                "constraints": {},
                "field": "ccw",
            },
            "full_circle": {
                "type": {
                    "kind": "primitive",
                    "name": "boolean",
                },
                "optional": False,
                "constraints": {},
                "field": "full_circle",
            },
        },
    },
    "Wavenumber.Geometer.Contracts.HlrProjectionA0.ProjectedGeometry": {
        "kind": "object",
        "properties": {
            "segments": {
                "type": {
                    "kind": "array",
                    "element": {
                        "kind": "reference",
                        "target": "Wavenumber.Geometer.Contracts.HlrProjectionA0.ProjectedSegment",
                    },
                },
                "optional": False,
                "constraints": {
                    "max_items": 4000000,
                },
                "field": "segments",
            },
            "arcs": {
                "type": {
                    "kind": "array",
                    "element": {
                        "kind": "reference",
                        "target": "Wavenumber.Geometer.Contracts.HlrProjectionA0.ProjectedArc",
                    },
                },
                "optional": False,
                "constraints": {
                    "max_items": 4000000,
                },
                "field": "arcs",
            },
            "bounds": {
                "type": {
                    "kind": "reference",
                    "target": "Wavenumber.Geometer.Contracts.HlrProjectionA0.ProjectionBounds",
                },
                "optional": True,
                "constraints": {},
                "field": "bounds",
            },
        },
    },
    "Wavenumber.Geometer.Contracts.HlrProjectionA0.ProjectedSegment": {
        "kind": "array",
        "element": {
            "kind": "primitive",
            "name": "float64",
        },
        "constraints": {
            "min_items": 4,
            "max_items": 4,
        },
    },
    "Wavenumber.Geometer.Contracts.HlrProjectionA0.ProjectionBounds": {
        "kind": "object",
        "properties": {
            "min_x": {
                "type": {
                    "kind": "primitive",
                    "name": "float64",
                },
                "optional": False,
                "constraints": {},
                "field": "min_x",
            },
            "min_y": {
                "type": {
                    "kind": "primitive",
                    "name": "float64",
                },
                "optional": False,
                "constraints": {},
                "field": "min_y",
            },
            "max_x": {
                "type": {
                    "kind": "primitive",
                    "name": "float64",
                },
                "optional": False,
                "constraints": {},
                "field": "max_x",
            },
            "max_y": {
                "type": {
                    "kind": "primitive",
                    "name": "float64",
                },
                "optional": False,
                "constraints": {},
                "field": "max_y",
            },
            "width": {
                "type": {
                    "kind": "primitive",
                    "name": "float64",
                },
                "optional": False,
                "constraints": {
                    "min_value": 0,
                },
                "field": "width",
            },
            "height": {
                "type": {
                    "kind": "primitive",
                    "name": "float64",
                },
                "optional": False,
                "constraints": {
                    "min_value": 0,
                },
                "field": "height",
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
                "target": "Wavenumber.Geometer.Contracts.ModelTessellationA0.ModelTessellationRequestA0",
            },
            {
                "kind": "reference",
                "target": "Wavenumber.Geometer.Contracts.ModelBoundsA0.ModelBoundsOptionsA0",
            },
            {
                "kind": "reference",
                "target": "Wavenumber.Geometer.Contracts.HlrProjectionA0.HlrProjectionOptionsA0",
            },
            {
                "kind": "reference",
                "target": "Wavenumber.Geometer.Contracts.Common.PackedAttachmentProjectionA0",
            },
            {
                "kind": "reference",
                "target": "Wavenumber.Geometer.Contracts.StepTopologyA0.StepTopologyOpenRequestA0",
            },
            {
                "kind": "reference",
                "target": "Wavenumber.Geometer.Contracts.StepTopologyA0.StepTopologyCloseRequestA0",
            },
            {
                "kind": "reference",
                "target": "Wavenumber.Geometer.Contracts.StepTopologyA0.StepTopologyInspectRequestA0",
            },
            {
                "kind": "reference",
                "target": "Wavenumber.Geometer.Contracts.StepTopologyA0.StepTopologyRenderRequestA0",
            },
            {
                "kind": "reference",
                "target": "Wavenumber.Geometer.Contracts.StepTopologyA0.StepTopologyResolveHitRequestA0",
            },
            {
                "kind": "reference",
                "target": "Wavenumber.Geometer.Contracts.StepTopologyA0.StepTopologyApplyLogicalGroupsRequestA0",
            },
            {
                "kind": "reference",
                "target": "Wavenumber.Geometer.Contracts.StepTopologyA0.StepTopologyApplyMetadataProbesRequestA0",
            },
            {
                "kind": "reference",
                "target": "Wavenumber.Geometer.Contracts.StepTopologyA0.StepTopologyCheckpointEditJournalRequestA0",
            },
            {
                "kind": "reference",
                "target": "Wavenumber.Geometer.Contracts.StepTopologyA0.StepTopologyApplyHierarchyRequestA0",
            },
            {
                "kind": "reference",
                "target": "Wavenumber.Geometer.Contracts.StepTopologyA0.StepTopologySaveRequestA0",
            },
            {
                "kind": "reference",
                "target": "Wavenumber.Geometer.Contracts.StepTopologyA0.StepTopologyRestoreRequestA0",
            },
            {
                "kind": "reference",
                "target": "Wavenumber.Geometer.Contracts.StepTopologyA0.StepTopologyAnalyzeRecoveryRequestA0",
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
    "Wavenumber.Geometer.Contracts.MeshIllustrationA0.IllustrationMatrix4x4": {
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
    "Wavenumber.Geometer.Contracts.MeshIllustrationA0.IllustrationVector3": {
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
    "Wavenumber.Geometer.Contracts.MeshIllustrationA0.MeshIllustrationInputA0": {
        "kind": "object",
        "properties": {
            "schema": {
                "type": {
                    "kind": "literal",
                    "value_type": "string",
                    "value": "geometry.mesh_illustration.input.a0",
                },
                "optional": False,
                "constraints": {},
                "field": "schema",
            },
            "meshes": {
                "type": {
                    "kind": "array",
                    "element": {
                        "kind": "reference",
                        "target": "Wavenumber.Geometer.Contracts.MeshIllustrationA0.MeshIllustrationMesh",
                    },
                },
                "optional": False,
                "constraints": {
                    "min_items": 1,
                    "max_items": 65536,
                },
                "field": "meshes",
            },
            "view": {
                "type": {
                    "kind": "reference",
                    "target": "Wavenumber.Geometer.Contracts.MeshIllustrationA0.MeshIllustrationView",
                },
                "optional": False,
                "constraints": {},
                "field": "view",
            },
            "prepare": {
                "type": {
                    "kind": "reference",
                    "target": "Wavenumber.Geometer.Contracts.MeshIllustrationA0.MeshIllustrationPrepareOptions",
                },
                "optional": True,
                "constraints": {},
                "field": "prepare",
            },
            "style": {
                "type": {
                    "kind": "reference",
                    "target": "Wavenumber.Geometer.Contracts.MeshIllustrationA0.MeshIllustrationStyleA0",
                },
                "optional": True,
                "constraints": {},
                "field": "style",
            },
            "svg": {
                "type": {
                    "kind": "reference",
                    "target": "Wavenumber.Geometer.Contracts.MeshIllustrationA0.MeshIllustrationSvgOptions",
                },
                "optional": True,
                "constraints": {},
                "field": "svg",
            },
        },
    },
    "Wavenumber.Geometer.Contracts.MeshIllustrationA0.MeshIllustrationMaterial": {
        "kind": "object",
        "properties": {
            "color": {
                "type": {
                    "kind": "reference",
                    "target": "Wavenumber.Geometer.Contracts.MeshIllustrationA0.IllustrationVector3",
                },
                "optional": False,
                "constraints": {},
                "field": "color",
            },
            "opacity": {
                "type": {
                    "kind": "primitive",
                    "name": "float64",
                },
                "optional": True,
                "constraints": {
                    "min_value": 0,
                    "max_value": 1,
                },
                "field": "opacity",
            },
            "name": {
                "type": {
                    "kind": "primitive",
                    "name": "string",
                },
                "optional": True,
                "constraints": {
                    "max_length": 1024,
                },
                "field": "name",
            },
        },
    },
    "Wavenumber.Geometer.Contracts.MeshIllustrationA0.MeshIllustrationMesh": {
        "kind": "object",
        "properties": {
            "id": {
                "type": {
                    "kind": "primitive",
                    "name": "string",
                },
                "optional": False,
                "constraints": {
                    "min_length": 1,
                    "max_length": 1024,
                },
                "field": "id",
            },
            "positions": {
                "type": {
                    "kind": "array",
                    "element": {
                        "kind": "primitive",
                        "name": "float64",
                    },
                },
                "optional": False,
                "constraints": {
                    "min_items": 9,
                    "max_items": 6000000,
                },
                "field": "positions",
            },
            "normals": {
                "type": {
                    "kind": "array",
                    "element": {
                        "kind": "primitive",
                        "name": "float64",
                    },
                },
                "optional": True,
                "constraints": {
                    "max_items": 6000000,
                },
                "field": "normals",
            },
            "indices": {
                "type": {
                    "kind": "array",
                    "element": {
                        "kind": "primitive",
                        "name": "uint32",
                    },
                },
                "optional": True,
                "constraints": {
                    "max_items": 6000000,
                },
                "field": "indices",
            },
            "matrix": {
                "type": {
                    "kind": "reference",
                    "target": "Wavenumber.Geometer.Contracts.MeshIllustrationA0.IllustrationMatrix4x4",
                },
                "optional": True,
                "constraints": {},
                "field": "matrix",
            },
            "materials": {
                "type": {
                    "kind": "array",
                    "element": {
                        "kind": "reference",
                        "target": "Wavenumber.Geometer.Contracts.MeshIllustrationA0.MeshIllustrationMaterial",
                    },
                },
                "optional": False,
                "constraints": {
                    "min_items": 1,
                    "max_items": 65536,
                },
                "field": "materials",
            },
            "triangle_material_indices": {
                "type": {
                    "kind": "array",
                    "element": {
                        "kind": "primitive",
                        "name": "uint32",
                    },
                },
                "optional": True,
                "constraints": {
                    "max_items": 2000000,
                },
                "field": "triangle_material_indices",
            },
            "double_sided": {
                "type": {
                    "kind": "primitive",
                    "name": "boolean",
                },
                "optional": True,
                "constraints": {},
                "field": "double_sided",
            },
        },
    },
    "Wavenumber.Geometer.Contracts.MeshIllustrationA0.MeshIllustrationPrepareOptions": {
        "kind": "object",
        "properties": {
            "max_triangles": {
                "type": {
                    "kind": "primitive",
                    "name": "uint32",
                },
                "optional": True,
                "constraints": {
                    "min_value": 1,
                    "max_value": 2000000,
                },
                "field": "max_triangles",
            },
            "weld_tolerance": {
                "type": {
                    "kind": "primitive",
                    "name": "float64",
                },
                "optional": True,
                "constraints": {
                    "min_value_exclusive": 0,
                },
                "field": "weld_tolerance",
            },
        },
    },
    "Wavenumber.Geometer.Contracts.MeshIllustrationA0.MeshIllustrationRenderStats": {
        "kind": "object",
        "properties": {
            "triangles": {
                "type": {
                    "kind": "primitive",
                    "name": "uint32",
                },
                "optional": False,
                "constraints": {
                    "min_value": 0,
                },
                "field": "triangles",
            },
            "surface_draws": {
                "type": {
                    "kind": "primitive",
                    "name": "uint32",
                },
                "optional": False,
                "constraints": {
                    "min_value": 0,
                },
                "field": "surface_draws",
            },
            "layered_surfaces": {
                "type": {
                    "kind": "primitive",
                    "name": "uint32",
                },
                "optional": False,
                "constraints": {
                    "min_value": 0,
                },
                "field": "layered_surfaces",
            },
            "outlines": {
                "type": {
                    "kind": "primitive",
                    "name": "uint32",
                },
                "optional": False,
                "constraints": {
                    "min_value": 0,
                },
                "field": "outlines",
            },
            "details": {
                "type": {
                    "kind": "primitive",
                    "name": "uint32",
                },
                "optional": False,
                "constraints": {
                    "min_value": 0,
                },
                "field": "details",
            },
            "creases": {
                "type": {
                    "kind": "primitive",
                    "name": "uint32",
                },
                "optional": False,
                "constraints": {
                    "min_value": 0,
                },
                "field": "creases",
            },
            "commands": {
                "type": {
                    "kind": "primitive",
                    "name": "uint32",
                },
                "optional": False,
                "constraints": {
                    "min_value": 0,
                },
                "field": "commands",
            },
        },
    },
    "Wavenumber.Geometer.Contracts.MeshIllustrationA0.MeshIllustrationResultA0": {
        "kind": "object",
        "properties": {
            "schema": {
                "type": {
                    "kind": "literal",
                    "value_type": "string",
                    "value": "geometry.mesh_illustration.result.a0",
                },
                "optional": False,
                "constraints": {},
                "field": "schema",
            },
            "svg": {
                "type": {
                    "kind": "primitive",
                    "name": "string",
                },
                "optional": False,
                "constraints": {},
                "field": "svg",
            },
            "stats": {
                "type": {
                    "kind": "reference",
                    "target": "Wavenumber.Geometer.Contracts.MeshIllustrationA0.MeshIllustrationRenderStats",
                },
                "optional": False,
                "constraints": {},
                "field": "stats",
            },
            "warnings": {
                "type": {
                    "kind": "array",
                    "element": {
                        "kind": "primitive",
                        "name": "string",
                    },
                },
                "optional": False,
                "constraints": {
                    "max_items": 256,
                },
                "field": "warnings",
            },
        },
    },
    "Wavenumber.Geometer.Contracts.MeshIllustrationA0.MeshIllustrationShading": {
        "kind": "enum",
        "values": ["unlit", "flat", "lambert", "banded", "toon"],
    },
    "Wavenumber.Geometer.Contracts.MeshIllustrationA0.MeshIllustrationStyleA0": {
        "kind": "object",
        "properties": {
            "shading": {
                "type": {
                    "kind": "reference",
                    "target": "Wavenumber.Geometer.Contracts.MeshIllustrationA0.MeshIllustrationShading",
                },
                "optional": True,
                "constraints": {},
                "field": "shading",
            },
            "ambient": {
                "type": {
                    "kind": "primitive",
                    "name": "float64",
                },
                "optional": True,
                "constraints": {
                    "min_value": 0,
                    "max_value": 1,
                },
                "field": "ambient",
            },
            "key_intensity": {
                "type": {
                    "kind": "primitive",
                    "name": "float64",
                },
                "optional": True,
                "constraints": {
                    "min_value": 0,
                    "max_value": 4,
                },
                "field": "key_intensity",
            },
            "light_direction": {
                "type": {
                    "kind": "reference",
                    "target": "Wavenumber.Geometer.Contracts.MeshIllustrationA0.IllustrationVector3",
                },
                "optional": True,
                "constraints": {},
                "field": "light_direction",
            },
            "bands": {
                "type": {
                    "kind": "primitive",
                    "name": "uint32",
                },
                "optional": True,
                "constraints": {
                    "min_value": 1,
                    "max_value": 256,
                },
                "field": "bands",
            },
            "source_colors": {
                "type": {
                    "kind": "primitive",
                    "name": "boolean",
                },
                "optional": True,
                "constraints": {},
                "field": "source_colors",
            },
            "fallback_color": {
                "type": {
                    "kind": "reference",
                    "target": "Wavenumber.Geometer.Contracts.MeshIllustrationA0.IllustrationVector3",
                },
                "optional": True,
                "constraints": {},
                "field": "fallback_color",
            },
            "background": {
                "type": {
                    "kind": "primitive",
                    "name": "string",
                },
                "optional": True,
                "constraints": {
                    "min_length": 1,
                    "max_length": 128,
                },
                "field": "background",
            },
            "transparent_background": {
                "type": {
                    "kind": "primitive",
                    "name": "boolean",
                },
                "optional": True,
                "constraints": {},
                "field": "transparent_background",
            },
            "fuse_surfaces": {
                "type": {
                    "kind": "primitive",
                    "name": "boolean",
                },
                "optional": True,
                "constraints": {},
                "field": "fuse_surfaces",
            },
            "layer_coplanar_materials": {
                "type": {
                    "kind": "primitive",
                    "name": "boolean",
                },
                "optional": True,
                "constraints": {},
                "field": "layer_coplanar_materials",
            },
            "show_hlr_outline": {
                "type": {
                    "kind": "primitive",
                    "name": "boolean",
                },
                "optional": True,
                "constraints": {},
                "field": "show_hlr_outline",
            },
            "show_hlr_detail": {
                "type": {
                    "kind": "primitive",
                    "name": "boolean",
                },
                "optional": True,
                "constraints": {},
                "field": "show_hlr_detail",
            },
            "show_outlines": {
                "type": {
                    "kind": "primitive",
                    "name": "boolean",
                },
                "optional": True,
                "constraints": {},
                "field": "show_outlines",
            },
            "show_creases": {
                "type": {
                    "kind": "primitive",
                    "name": "boolean",
                },
                "optional": True,
                "constraints": {},
                "field": "show_creases",
            },
            "crease_angle_degrees": {
                "type": {
                    "kind": "primitive",
                    "name": "float64",
                },
                "optional": True,
                "constraints": {
                    "min_value": 0,
                    "max_value": 180,
                },
                "field": "crease_angle_degrees",
            },
            "outline_color": {
                "type": {
                    "kind": "primitive",
                    "name": "string",
                },
                "optional": True,
                "constraints": {
                    "min_length": 1,
                    "max_length": 128,
                },
                "field": "outline_color",
            },
            "crease_color": {
                "type": {
                    "kind": "primitive",
                    "name": "string",
                },
                "optional": True,
                "constraints": {
                    "min_length": 1,
                    "max_length": 128,
                },
                "field": "crease_color",
            },
            "outline_width": {
                "type": {
                    "kind": "primitive",
                    "name": "float64",
                },
                "optional": True,
                "constraints": {
                    "min_value": 0,
                },
                "field": "outline_width",
            },
            "crease_width": {
                "type": {
                    "kind": "primitive",
                    "name": "float64",
                },
                "optional": True,
                "constraints": {
                    "min_value": 0,
                },
                "field": "crease_width",
            },
            "double_sided": {
                "type": {
                    "kind": "primitive",
                    "name": "boolean",
                },
                "optional": True,
                "constraints": {},
                "field": "double_sided",
            },
            "rim_amount": {
                "type": {
                    "kind": "primitive",
                    "name": "float64",
                },
                "optional": True,
                "constraints": {
                    "min_value": 0,
                    "max_value": 1,
                },
                "field": "rim_amount",
            },
        },
    },
    "Wavenumber.Geometer.Contracts.MeshIllustrationA0.MeshIllustrationSvgOptions": {
        "kind": "object",
        "properties": {
            "coordinate_span": {
                "type": {
                    "kind": "primitive",
                    "name": "uint32",
                },
                "optional": True,
                "constraints": {
                    "min_value": 10000,
                    "max_value": 1000000000,
                },
                "field": "coordinate_span",
            },
            "title": {
                "type": {
                    "kind": "primitive",
                    "name": "string",
                },
                "optional": True,
                "constraints": {
                    "min_length": 1,
                    "max_length": 1024,
                },
                "field": "title",
            },
        },
    },
    "Wavenumber.Geometer.Contracts.MeshIllustrationA0.MeshIllustrationView": {
        "kind": "object",
        "properties": {
            "direction": {
                "type": {
                    "kind": "reference",
                    "target": "Wavenumber.Geometer.Contracts.MeshIllustrationA0.IllustrationVector3",
                },
                "optional": False,
                "constraints": {},
                "field": "direction",
            },
            "up": {
                "type": {
                    "kind": "reference",
                    "target": "Wavenumber.Geometer.Contracts.MeshIllustrationA0.IllustrationVector3",
                },
                "optional": False,
                "constraints": {},
                "field": "up",
            },
            "mirror_x": {
                "type": {
                    "kind": "primitive",
                    "name": "boolean",
                },
                "optional": True,
                "constraints": {},
                "field": "mirror_x",
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
    "Wavenumber.Geometer.Contracts.ModelTessellationA0.MeshCollectionA0": {
        "kind": "object",
        "properties": {
            "schema": {
                "type": {
                    "kind": "literal",
                    "value_type": "string",
                    "value": "geometry.mesh_collection.a0",
                },
                "optional": False,
                "constraints": {},
                "field": "schema",
            },
            "length_unit": {
                "type": {
                    "kind": "literal",
                    "value_type": "string",
                    "value": "millimeter",
                },
                "optional": False,
                "constraints": {},
                "field": "length_unit",
            },
            "meshes": {
                "type": {
                    "kind": "array",
                    "element": {
                        "kind": "reference",
                        "target": "Wavenumber.Geometer.Contracts.MeshIllustrationA0.MeshIllustrationMesh",
                    },
                },
                "optional": False,
                "constraints": {
                    "min_items": 1,
                    "max_items": 65536,
                },
                "field": "meshes",
            },
        },
    },
    "Wavenumber.Geometer.Contracts.ModelTessellationA0.MeshCollectionAttachment": {
        "kind": "object",
        "properties": {
            "attachment": {
                "type": {
                    "kind": "literal",
                    "value_type": "string",
                    "value": "mesh_collection",
                },
                "optional": False,
                "constraints": {},
                "field": "attachment",
            },
            "schema": {
                "type": {
                    "kind": "literal",
                    "value_type": "string",
                    "value": "geometry.mesh_collection.a0",
                },
                "optional": False,
                "constraints": {},
                "field": "schema",
            },
            "byte_length": {
                "type": {
                    "kind": "primitive",
                    "name": "uint32",
                },
                "optional": False,
                "constraints": {
                    "min_value": 1,
                    "max_value": 268435456,
                },
                "field": "byte_length",
            },
            "sha256": {
                "type": {
                    "kind": "primitive",
                    "name": "string",
                },
                "optional": False,
                "constraints": {
                    "min_length": 64,
                    "max_length": 64,
                },
                "field": "sha256",
            },
        },
    },
    "Wavenumber.Geometer.Contracts.ModelTessellationA0.ModelRootPlacement": {
        "kind": "enum",
        "values": ["strip", "preserve"],
    },
    "Wavenumber.Geometer.Contracts.ModelTessellationA0.ModelTessellationRequestA0": {
        "kind": "object",
        "properties": {
            "schema": {
                "type": {
                    "kind": "literal",
                    "value_type": "string",
                    "value": "geometry.model_tessellation.request.a0",
                },
                "optional": False,
                "constraints": {},
                "field": "schema",
            },
            "linear_deflection_mm": {
                "type": {
                    "kind": "primitive",
                    "name": "float64",
                },
                "optional": True,
                "constraints": {
                    "min_value": 0.000001,
                    "max_value": 1000,
                },
                "field": "linear_deflection_mm",
            },
            "angular_deflection_rad": {
                "type": {
                    "kind": "primitive",
                    "name": "float64",
                },
                "optional": True,
                "constraints": {
                    "min_value": 0.000001,
                    "max_value": 3.141592653589793,
                },
                "field": "angular_deflection_rad",
            },
            "root_placement": {
                "type": {
                    "kind": "reference",
                    "target": "Wavenumber.Geometer.Contracts.ModelTessellationA0.ModelRootPlacement",
                },
                "optional": True,
                "constraints": {},
                "field": "root_placement",
            },
            "max_triangles": {
                "type": {
                    "kind": "primitive",
                    "name": "uint32",
                },
                "optional": True,
                "constraints": {
                    "min_value": 1,
                    "max_value": 2000000,
                },
                "field": "max_triangles",
            },
        },
    },
    "Wavenumber.Geometer.Contracts.ModelTessellationA0.ModelTessellationResultA0": {
        "kind": "object",
        "properties": {
            "schema": {
                "type": {
                    "kind": "literal",
                    "value_type": "string",
                    "value": "geometry.model_tessellation.result.a0",
                },
                "optional": False,
                "constraints": {},
                "field": "schema",
            },
            "mesh_collection": {
                "type": {
                    "kind": "reference",
                    "target": "Wavenumber.Geometer.Contracts.ModelTessellationA0.MeshCollectionAttachment",
                },
                "optional": False,
                "constraints": {},
                "field": "mesh_collection",
            },
            "source_sha256": {
                "type": {
                    "kind": "primitive",
                    "name": "string",
                },
                "optional": False,
                "constraints": {
                    "min_length": 64,
                    "max_length": 64,
                },
                "field": "source_sha256",
            },
            "meshes": {
                "type": {
                    "kind": "primitive",
                    "name": "uint32",
                },
                "optional": False,
                "constraints": {
                    "min_value": 1,
                    "max_value": 65536,
                },
                "field": "meshes",
            },
            "triangles": {
                "type": {
                    "kind": "primitive",
                    "name": "uint32",
                },
                "optional": False,
                "constraints": {
                    "min_value": 1,
                    "max_value": 2000000,
                },
                "field": "triangles",
            },
            "warnings": {
                "type": {
                    "kind": "array",
                    "element": {
                        "kind": "primitive",
                        "name": "string",
                    },
                },
                "optional": False,
                "constraints": {
                    "max_items": 256,
                },
                "field": "warnings",
            },
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
                "target": "Wavenumber.Geometer.Contracts.ModelTessellationA0.ModelTessellationResultA0",
            },
            {
                "kind": "reference",
                "target": "Wavenumber.Geometer.Contracts.ModelBoundsA0.ModelBoundsResultA0",
            },
            {
                "kind": "reference",
                "target": "Wavenumber.Geometer.Contracts.HlrProjectionA0.HlrProjectionResultA0",
            },
            {
                "kind": "reference",
                "target": "Wavenumber.Geometer.Contracts.Common.PackedAttachmentProjectionA0",
            },
            {
                "kind": "reference",
                "target": "Wavenumber.Geometer.Contracts.StepTopologyA0.StepTopologyOpenResultA0",
            },
            {
                "kind": "reference",
                "target": "Wavenumber.Geometer.Contracts.StepTopologyA0.StepTopologyCloseResultA0",
            },
            {
                "kind": "reference",
                "target": "Wavenumber.Geometer.Contracts.StepTopologyA0.StepTopologyInspectResultA0",
            },
            {
                "kind": "reference",
                "target": "Wavenumber.Geometer.Contracts.StepTopologyA0.StepTopologyRenderResultA0",
            },
            {
                "kind": "reference",
                "target": "Wavenumber.Geometer.Contracts.StepTopologyA0.StepTopologyResolveHitResultA0",
            },
            {
                "kind": "reference",
                "target": "Wavenumber.Geometer.Contracts.StepTopologyA0.StepTopologyApplyLogicalGroupsResultA0",
            },
            {
                "kind": "reference",
                "target": "Wavenumber.Geometer.Contracts.StepTopologyA0.StepTopologyApplyMetadataProbesResultA0",
            },
            {
                "kind": "reference",
                "target": "Wavenumber.Geometer.Contracts.StepTopologyA0.StepTopologyCheckpointEditJournalResultA0",
            },
            {
                "kind": "reference",
                "target": "Wavenumber.Geometer.Contracts.StepTopologyA0.StepTopologyApplyHierarchyResultA0",
            },
            {
                "kind": "reference",
                "target": "Wavenumber.Geometer.Contracts.StepTopologyA0.StepTopologySaveResultA0",
            },
            {
                "kind": "reference",
                "target": "Wavenumber.Geometer.Contracts.StepTopologyA0.StepTopologyRestoreResultA0",
            },
            {
                "kind": "reference",
                "target": "Wavenumber.Geometer.Contracts.StepTopologyA0.StepTopologyAnalyzeRecoveryResultA0",
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
    "Wavenumber.Geometer.Contracts.StepTopologyA0.AttachMetadataProbeCommand": {
        "kind": "object",
        "properties": {
            "kind": {
                "type": {
                    "kind": "literal",
                    "value_type": "string",
                    "value": "attach",
                },
                "optional": False,
                "constraints": {},
                "field": "kind",
            },
            "authored_id": {
                "type": {
                    "kind": "primitive",
                    "name": "string",
                },
                "optional": False,
                "constraints": {
                    "min_length": 28,
                    "max_length": 128,
                },
                "field": "authored_id",
            },
            "target": {
                "type": {
                    "kind": "reference",
                    "target": "Wavenumber.Geometer.Contracts.StepTopologyA0.MetadataProbeTarget",
                },
                "optional": False,
                "constraints": {},
                "field": "target",
            },
            "key": {
                "type": {
                    "kind": "primitive",
                    "name": "string",
                },
                "optional": False,
                "constraints": {
                    "min_length": 32,
                    "max_length": 128,
                },
                "field": "key",
            },
            "value": {
                "type": {
                    "kind": "primitive",
                    "name": "string",
                },
                "optional": False,
                "constraints": {
                    "min_length": 1,
                    "max_length": 4096,
                },
                "field": "value",
            },
        },
    },
    "Wavenumber.Geometer.Contracts.StepTopologyA0.BodyProbeTarget": {
        "kind": "object",
        "properties": {
            "kind": {
                "type": {
                    "kind": "literal",
                    "value_type": "string",
                    "value": "body",
                },
                "optional": False,
                "constraints": {},
                "field": "kind",
            },
            "target_handle": {
                "type": {
                    "kind": "primitive",
                    "name": "string",
                },
                "optional": False,
                "constraints": {
                    "min_length": 68,
                    "max_length": 68,
                },
                "field": "target_handle",
            },
        },
    },
    "Wavenumber.Geometer.Contracts.StepTopologyA0.BodySummary": {
        "kind": "object",
        "properties": {
            "handle": {
                "type": {
                    "kind": "primitive",
                    "name": "string",
                },
                "optional": False,
                "constraints": {
                    "min_length": 68,
                    "max_length": 68,
                },
                "field": "handle",
            },
            "definition_handle": {
                "type": {
                    "kind": "primitive",
                    "name": "string",
                },
                "optional": False,
                "constraints": {
                    "min_length": 68,
                    "max_length": 68,
                },
                "field": "definition_handle",
            },
            "topology_kind": {
                "type": {
                    "kind": "primitive",
                    "name": "string",
                },
                "optional": False,
                "constraints": {
                    "min_length": 1,
                    "max_length": 64,
                },
                "field": "topology_kind",
            },
            "shell_count": {
                "type": {
                    "kind": "primitive",
                    "name": "uint32",
                },
                "optional": False,
                "constraints": {
                    "max_value": 250000,
                },
                "field": "shell_count",
            },
            "face_count": {
                "type": {
                    "kind": "primitive",
                    "name": "uint32",
                },
                "optional": False,
                "constraints": {
                    "max_value": 1000000,
                },
                "field": "face_count",
            },
            "bounds_mm": {
                "type": {
                    "kind": "array",
                    "element": {
                        "kind": "primitive",
                        "name": "float64",
                    },
                },
                "optional": False,
                "constraints": {
                    "min_items": 6,
                    "max_items": 6,
                },
                "field": "bounds_mm",
            },
            "volume_mm3": {
                "type": {
                    "kind": "primitive",
                    "name": "float64",
                },
                "optional": False,
                "constraints": {
                    "min_value": 0,
                },
                "field": "volume_mm3",
            },
            "source_entity": {
                "type": {
                    "kind": "reference",
                    "target": "Wavenumber.Geometer.Contracts.StepTopologyA0.SourceEntityEvidence",
                },
                "optional": True,
                "constraints": {},
                "field": "source_entity",
            },
        },
    },
    "Wavenumber.Geometer.Contracts.StepTopologyA0.CarrierCapability": {
        "kind": "object",
        "properties": {
            "carrier": {
                "type": {
                    "kind": "reference",
                    "target": "Wavenumber.Geometer.Contracts.StepTopologyA0.PersistenceCarrier",
                },
                "optional": False,
                "constraints": {},
                "field": "carrier",
            },
            "save": {
                "type": {
                    "kind": "reference",
                    "target": "Wavenumber.Geometer.Contracts.StepTopologyA0.CarrierSupportState",
                },
                "optional": False,
                "constraints": {},
                "field": "save",
            },
            "restore": {
                "type": {
                    "kind": "reference",
                    "target": "Wavenumber.Geometer.Contracts.StepTopologyA0.CarrierSupportState",
                },
                "optional": False,
                "constraints": {},
                "field": "restore",
            },
            "authored_payload": {
                "type": {
                    "kind": "reference",
                    "target": "Wavenumber.Geometer.Contracts.StepTopologyA0.CarrierSupportState",
                },
                "optional": False,
                "constraints": {},
                "field": "authored_payload",
            },
            "topology_links": {
                "type": {
                    "kind": "reference",
                    "target": "Wavenumber.Geometer.Contracts.StepTopologyA0.CarrierSupportState",
                },
                "optional": False,
                "constraints": {},
                "field": "topology_links",
            },
            "notes": {
                "type": {
                    "kind": "array",
                    "element": {
                        "kind": "reference",
                        "target": "Wavenumber.Geometer.Contracts.StepTopologyA0.CarrierCapabilityNote",
                    },
                },
                "optional": False,
                "constraints": {
                    "max_items": 16,
                },
                "field": "notes",
            },
        },
    },
    "Wavenumber.Geometer.Contracts.StepTopologyA0.CarrierCapabilityNote": {
        "kind": "object",
        "properties": {
            "value": {
                "type": {
                    "kind": "primitive",
                    "name": "string",
                },
                "optional": False,
                "constraints": {
                    "min_length": 1,
                    "max_length": 256,
                },
                "field": "value",
            },
        },
    },
    "Wavenumber.Geometer.Contracts.StepTopologyA0.CarrierSupportState": {
        "kind": "enum",
        "values": ["supported", "experimental", "unsupported"],
    },
    "Wavenumber.Geometer.Contracts.StepTopologyA0.ComponentOccurrenceProbeTarget": {
        "kind": "object",
        "properties": {
            "kind": {
                "type": {
                    "kind": "literal",
                    "value_type": "string",
                    "value": "occurrence",
                },
                "optional": False,
                "constraints": {},
                "field": "kind",
            },
            "target_handle": {
                "type": {
                    "kind": "primitive",
                    "name": "string",
                },
                "optional": False,
                "constraints": {
                    "min_length": 68,
                    "max_length": 68,
                },
                "field": "target_handle",
            },
        },
    },
    "Wavenumber.Geometer.Contracts.StepTopologyA0.ComponentOccurrenceSummary": {
        "kind": "object",
        "properties": {
            "kind": {
                "type": {
                    "kind": "literal",
                    "value_type": "string",
                    "value": "component",
                },
                "optional": False,
                "constraints": {},
                "field": "kind",
            },
            "handle": {
                "type": {
                    "kind": "primitive",
                    "name": "string",
                },
                "optional": False,
                "constraints": {
                    "min_length": 68,
                    "max_length": 68,
                },
                "field": "handle",
            },
            "definition_handle": {
                "type": {
                    "kind": "primitive",
                    "name": "string",
                },
                "optional": False,
                "constraints": {
                    "min_length": 68,
                    "max_length": 68,
                },
                "field": "definition_handle",
            },
            "parent_occurrence_handle": {
                "type": {
                    "kind": "primitive",
                    "name": "string",
                },
                "optional": False,
                "constraints": {
                    "min_length": 68,
                    "max_length": 68,
                },
                "field": "parent_occurrence_handle",
            },
            "depth": {
                "type": {
                    "kind": "primitive",
                    "name": "uint32",
                },
                "optional": False,
                "constraints": {
                    "min_value": 1,
                    "max_value": 64,
                },
                "field": "depth",
            },
            "name": {
                "type": {
                    "kind": "primitive",
                    "name": "string",
                },
                "optional": False,
                "constraints": {
                    "max_length": 4096,
                },
                "field": "name",
            },
            "transform": {
                "type": {
                    "kind": "array",
                    "element": {
                        "kind": "primitive",
                        "name": "float64",
                    },
                },
                "optional": False,
                "constraints": {
                    "min_items": 12,
                    "max_items": 12,
                },
                "field": "transform",
            },
        },
    },
    "Wavenumber.Geometer.Contracts.StepTopologyA0.CreateHierarchyAssemblyCommand": {
        "kind": "object",
        "properties": {
            "kind": {
                "type": {
                    "kind": "literal",
                    "value_type": "string",
                    "value": "create_assembly",
                },
                "optional": False,
                "constraints": {},
                "field": "kind",
            },
            "authored_id": {
                "type": {
                    "kind": "primitive",
                    "name": "string",
                },
                "optional": False,
                "constraints": {
                    "min_length": 28,
                    "max_length": 128,
                },
                "field": "authored_id",
            },
            "name": {
                "type": {
                    "kind": "primitive",
                    "name": "string",
                },
                "optional": False,
                "constraints": {
                    "min_length": 1,
                    "max_length": 4096,
                },
                "field": "name",
            },
        },
    },
    "Wavenumber.Geometer.Contracts.StepTopologyA0.CreateHierarchyOccurrenceCommand": {
        "kind": "object",
        "properties": {
            "kind": {
                "type": {
                    "kind": "literal",
                    "value_type": "string",
                    "value": "create_occurrence",
                },
                "optional": False,
                "constraints": {},
                "field": "kind",
            },
            "authored_id": {
                "type": {
                    "kind": "primitive",
                    "name": "string",
                },
                "optional": False,
                "constraints": {
                    "min_length": 28,
                    "max_length": 128,
                },
                "field": "authored_id",
            },
            "child_authored_id": {
                "type": {
                    "kind": "primitive",
                    "name": "string",
                },
                "optional": False,
                "constraints": {
                    "min_length": 28,
                    "max_length": 128,
                },
                "field": "child_authored_id",
            },
            "parent_assembly_authored_id": {
                "type": {
                    "kind": "primitive",
                    "name": "string",
                },
                "optional": False,
                "constraints": {
                    "min_length": 28,
                    "max_length": 128,
                },
                "field": "parent_assembly_authored_id",
            },
            "transform": {
                "type": {
                    "kind": "array",
                    "element": {
                        "kind": "primitive",
                        "name": "float64",
                    },
                },
                "optional": False,
                "constraints": {
                    "min_items": 12,
                    "max_items": 12,
                },
                "field": "transform",
            },
        },
    },
    "Wavenumber.Geometer.Contracts.StepTopologyA0.CreateHierarchyProductCommand": {
        "kind": "object",
        "properties": {
            "kind": {
                "type": {
                    "kind": "literal",
                    "value_type": "string",
                    "value": "create_product",
                },
                "optional": False,
                "constraints": {},
                "field": "kind",
            },
            "authored_id": {
                "type": {
                    "kind": "primitive",
                    "name": "string",
                },
                "optional": False,
                "constraints": {
                    "min_length": 28,
                    "max_length": 128,
                },
                "field": "authored_id",
            },
            "name": {
                "type": {
                    "kind": "primitive",
                    "name": "string",
                },
                "optional": False,
                "constraints": {
                    "min_length": 1,
                    "max_length": 4096,
                },
                "field": "name",
            },
            "source_kind": {
                "type": {
                    "kind": "reference",
                    "target": "Wavenumber.Geometer.Contracts.StepTopologyA0.HierarchySourceKind",
                },
                "optional": False,
                "constraints": {},
                "field": "source_kind",
            },
            "source_handle": {
                "type": {
                    "kind": "primitive",
                    "name": "string",
                },
                "optional": False,
                "constraints": {
                    "min_length": 68,
                    "max_length": 68,
                },
                "field": "source_handle",
            },
        },
    },
    "Wavenumber.Geometer.Contracts.StepTopologyA0.CreateLogicalGroupCommand": {
        "kind": "object",
        "properties": {
            "kind": {
                "type": {
                    "kind": "literal",
                    "value_type": "string",
                    "value": "create",
                },
                "optional": False,
                "constraints": {},
                "field": "kind",
            },
            "authored_id": {
                "type": {
                    "kind": "primitive",
                    "name": "string",
                },
                "optional": False,
                "constraints": {
                    "min_length": 28,
                    "max_length": 128,
                },
                "field": "authored_id",
            },
            "name": {
                "type": {
                    "kind": "primitive",
                    "name": "string",
                },
                "optional": False,
                "constraints": {
                    "min_length": 1,
                    "max_length": 4096,
                },
                "field": "name",
            },
            "member_handles": {
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
                    "max_items": 100000,
                },
                "field": "member_handles",
            },
        },
    },
    "Wavenumber.Geometer.Contracts.StepTopologyA0.DefinitionProbeTarget": {
        "kind": "object",
        "properties": {
            "kind": {
                "type": {
                    "kind": "literal",
                    "value_type": "string",
                    "value": "definition",
                },
                "optional": False,
                "constraints": {},
                "field": "kind",
            },
            "target_handle": {
                "type": {
                    "kind": "primitive",
                    "name": "string",
                },
                "optional": False,
                "constraints": {
                    "min_length": 68,
                    "max_length": 68,
                },
                "field": "target_handle",
            },
        },
    },
    "Wavenumber.Geometer.Contracts.StepTopologyA0.DefinitionSummary": {
        "kind": "object",
        "properties": {
            "handle": {
                "type": {
                    "kind": "primitive",
                    "name": "string",
                },
                "optional": False,
                "constraints": {
                    "min_length": 68,
                    "max_length": 68,
                },
                "field": "handle",
            },
            "name": {
                "type": {
                    "kind": "primitive",
                    "name": "string",
                },
                "optional": False,
                "constraints": {
                    "max_length": 4096,
                },
                "field": "name",
            },
            "assembly": {
                "type": {
                    "kind": "primitive",
                    "name": "boolean",
                },
                "optional": False,
                "constraints": {},
                "field": "assembly",
            },
            "body_count": {
                "type": {
                    "kind": "primitive",
                    "name": "uint32",
                },
                "optional": False,
                "constraints": {
                    "max_value": 100000,
                },
                "field": "body_count",
            },
            "face_count": {
                "type": {
                    "kind": "primitive",
                    "name": "uint32",
                },
                "optional": False,
                "constraints": {
                    "max_value": 1000000,
                },
                "field": "face_count",
            },
            "source_entity": {
                "type": {
                    "kind": "reference",
                    "target": "Wavenumber.Geometer.Contracts.StepTopologyA0.SourceEntityEvidence",
                },
                "optional": True,
                "constraints": {},
                "field": "source_entity",
            },
        },
    },
    "Wavenumber.Geometer.Contracts.StepTopologyA0.DocumentProbeTarget": {
        "kind": "object",
        "properties": {
            "kind": {
                "type": {
                    "kind": "literal",
                    "value_type": "string",
                    "value": "document",
                },
                "optional": False,
                "constraints": {},
                "field": "kind",
            },
        },
    },
    "Wavenumber.Geometer.Contracts.StepTopologyA0.EditJournalAttachmentDescriptor": {
        "kind": "object",
        "properties": {
            "name": {
                "type": {
                    "kind": "literal",
                    "value_type": "string",
                    "value": "edit_journal",
                },
                "optional": False,
                "constraints": {},
                "field": "name",
            },
            "media_type": {
                "type": {
                    "kind": "literal",
                    "value_type": "string",
                    "value": "application/vnd.wavenumber.geometer.step-topology-edit-journal",
                },
                "optional": False,
                "constraints": {},
                "field": "media_type",
            },
            "format": {
                "type": {
                    "kind": "literal",
                    "value_type": "string",
                    "value": "geometer.step_topology_edit_journal.a0",
                },
                "optional": False,
                "constraints": {},
                "field": "format",
            },
            "bytes": {
                "type": {
                    "kind": "primitive",
                    "name": "uint32",
                },
                "optional": False,
                "constraints": {
                    "min_value": 1,
                    "max_value": 67108864,
                },
                "field": "bytes",
            },
            "sha256": {
                "type": {
                    "kind": "primitive",
                    "name": "string",
                },
                "optional": False,
                "constraints": {
                    "min_length": 64,
                    "max_length": 64,
                },
                "field": "sha256",
            },
        },
    },
    "Wavenumber.Geometer.Contracts.StepTopologyA0.EditJournalPersistenceArtifact": {
        "kind": "object",
        "properties": {
            "carrier": {
                "type": {
                    "kind": "literal",
                    "value_type": "string",
                    "value": "edit_journal",
                },
                "optional": False,
                "constraints": {},
                "field": "carrier",
            },
            "name": {
                "type": {
                    "kind": "literal",
                    "value_type": "string",
                    "value": "state_artifact",
                },
                "optional": False,
                "constraints": {},
                "field": "name",
            },
            "media_type": {
                "type": {
                    "kind": "literal",
                    "value_type": "string",
                    "value": "application/vnd.wavenumber.geometer.step-topology-edit-journal",
                },
                "optional": False,
                "constraints": {},
                "field": "media_type",
            },
            "format": {
                "type": {
                    "kind": "literal",
                    "value_type": "string",
                    "value": "geometer.step_topology_edit_journal.a0",
                },
                "optional": False,
                "constraints": {},
                "field": "format",
            },
            "bytes": {
                "type": {
                    "kind": "primitive",
                    "name": "uint32",
                },
                "optional": False,
                "constraints": {
                    "min_value": 1,
                    "max_value": 67108864,
                },
                "field": "bytes",
            },
            "sha256": {
                "type": {
                    "kind": "primitive",
                    "name": "string",
                },
                "optional": False,
                "constraints": {
                    "min_length": 64,
                    "max_length": 64,
                },
                "field": "sha256",
            },
        },
    },
    "Wavenumber.Geometer.Contracts.StepTopologyA0.EditJournalReplayPreconditions": {
        "kind": "object",
        "properties": {
            "source_sha256": {
                "type": {
                    "kind": "primitive",
                    "name": "string",
                },
                "optional": False,
                "constraints": {
                    "min_length": 64,
                    "max_length": 64,
                },
                "field": "source_sha256",
            },
            "source_brep_sha256": {
                "type": {
                    "kind": "primitive",
                    "name": "string",
                },
                "optional": False,
                "constraints": {
                    "min_length": 64,
                    "max_length": 64,
                },
                "field": "source_brep_sha256",
            },
            "target_inventory_sha256": {
                "type": {
                    "kind": "primitive",
                    "name": "string",
                },
                "optional": False,
                "constraints": {
                    "min_length": 64,
                    "max_length": 64,
                },
                "field": "target_inventory_sha256",
            },
            "occt_version": {
                "type": {
                    "kind": "primitive",
                    "name": "string",
                },
                "optional": False,
                "constraints": {
                    "min_length": 1,
                    "max_length": 64,
                },
                "field": "occt_version",
            },
            "transaction_count": {
                "type": {
                    "kind": "primitive",
                    "name": "uint32",
                },
                "optional": False,
                "constraints": {
                    "max_value": 100000,
                },
                "field": "transaction_count",
            },
        },
    },
    "Wavenumber.Geometer.Contracts.StepTopologyA0.EraseHierarchyNodeCommand": {
        "kind": "object",
        "properties": {
            "kind": {
                "type": {
                    "kind": "literal",
                    "value_type": "string",
                    "value": "erase_node",
                },
                "optional": False,
                "constraints": {},
                "field": "kind",
            },
            "authored_id": {
                "type": {
                    "kind": "primitive",
                    "name": "string",
                },
                "optional": False,
                "constraints": {
                    "min_length": 28,
                    "max_length": 128,
                },
                "field": "authored_id",
            },
            "expected_revision": {
                "type": {
                    "kind": "primitive",
                    "name": "uint32",
                },
                "optional": False,
                "constraints": {
                    "min_value": 1,
                },
                "field": "expected_revision",
            },
        },
    },
    "Wavenumber.Geometer.Contracts.StepTopologyA0.EraseHierarchyOccurrenceCommand": {
        "kind": "object",
        "properties": {
            "kind": {
                "type": {
                    "kind": "literal",
                    "value_type": "string",
                    "value": "erase_occurrence",
                },
                "optional": False,
                "constraints": {},
                "field": "kind",
            },
            "authored_id": {
                "type": {
                    "kind": "primitive",
                    "name": "string",
                },
                "optional": False,
                "constraints": {
                    "min_length": 28,
                    "max_length": 128,
                },
                "field": "authored_id",
            },
            "expected_revision": {
                "type": {
                    "kind": "primitive",
                    "name": "uint32",
                },
                "optional": False,
                "constraints": {
                    "min_value": 1,
                },
                "field": "expected_revision",
            },
        },
    },
    "Wavenumber.Geometer.Contracts.StepTopologyA0.EraseLogicalGroupCommand": {
        "kind": "object",
        "properties": {
            "kind": {
                "type": {
                    "kind": "literal",
                    "value_type": "string",
                    "value": "erase",
                },
                "optional": False,
                "constraints": {},
                "field": "kind",
            },
            "authored_id": {
                "type": {
                    "kind": "primitive",
                    "name": "string",
                },
                "optional": False,
                "constraints": {
                    "min_length": 28,
                    "max_length": 128,
                },
                "field": "authored_id",
            },
            "expected_revision": {
                "type": {
                    "kind": "primitive",
                    "name": "uint32",
                },
                "optional": False,
                "constraints": {
                    "min_value": 1,
                },
                "field": "expected_revision",
            },
        },
    },
    "Wavenumber.Geometer.Contracts.StepTopologyA0.EraseMetadataProbeCommand": {
        "kind": "object",
        "properties": {
            "kind": {
                "type": {
                    "kind": "literal",
                    "value_type": "string",
                    "value": "erase",
                },
                "optional": False,
                "constraints": {},
                "field": "kind",
            },
            "authored_id": {
                "type": {
                    "kind": "primitive",
                    "name": "string",
                },
                "optional": False,
                "constraints": {
                    "min_length": 28,
                    "max_length": 128,
                },
                "field": "authored_id",
            },
            "expected_revision": {
                "type": {
                    "kind": "primitive",
                    "name": "uint32",
                },
                "optional": False,
                "constraints": {
                    "min_value": 1,
                },
                "field": "expected_revision",
            },
        },
    },
    "Wavenumber.Geometer.Contracts.StepTopologyA0.FaceProbeTarget": {
        "kind": "object",
        "properties": {
            "kind": {
                "type": {
                    "kind": "literal",
                    "value_type": "string",
                    "value": "face",
                },
                "optional": False,
                "constraints": {},
                "field": "kind",
            },
            "target_handle": {
                "type": {
                    "kind": "primitive",
                    "name": "string",
                },
                "optional": False,
                "constraints": {
                    "min_length": 68,
                    "max_length": 68,
                },
                "field": "target_handle",
            },
        },
    },
    "Wavenumber.Geometer.Contracts.StepTopologyA0.FaceSummary": {
        "kind": "object",
        "properties": {
            "handle": {
                "type": {
                    "kind": "primitive",
                    "name": "string",
                },
                "optional": False,
                "constraints": {
                    "min_length": 68,
                    "max_length": 68,
                },
                "field": "handle",
            },
            "definition_handle": {
                "type": {
                    "kind": "primitive",
                    "name": "string",
                },
                "optional": False,
                "constraints": {
                    "min_length": 68,
                    "max_length": 68,
                },
                "field": "definition_handle",
            },
            "body_count": {
                "type": {
                    "kind": "primitive",
                    "name": "uint32",
                },
                "optional": False,
                "constraints": {
                    "max_value": 100000,
                },
                "field": "body_count",
            },
            "shell_count": {
                "type": {
                    "kind": "primitive",
                    "name": "uint32",
                },
                "optional": False,
                "constraints": {
                    "max_value": 250000,
                },
                "field": "shell_count",
            },
            "bounds_mm": {
                "type": {
                    "kind": "array",
                    "element": {
                        "kind": "primitive",
                        "name": "float64",
                    },
                },
                "optional": False,
                "constraints": {
                    "min_items": 6,
                    "max_items": 6,
                },
                "field": "bounds_mm",
            },
            "area_mm2": {
                "type": {
                    "kind": "primitive",
                    "name": "float64",
                },
                "optional": False,
                "constraints": {
                    "min_value": 0,
                },
                "field": "area_mm2",
            },
            "centroid_mm": {
                "type": {
                    "kind": "array",
                    "element": {
                        "kind": "primitive",
                        "name": "float64",
                    },
                },
                "optional": False,
                "constraints": {
                    "min_items": 3,
                    "max_items": 3,
                },
                "field": "centroid_mm",
            },
            "source_entity": {
                "type": {
                    "kind": "reference",
                    "target": "Wavenumber.Geometer.Contracts.StepTopologyA0.SourceEntityEvidence",
                },
                "optional": True,
                "constraints": {},
                "field": "source_entity",
            },
        },
    },
    "Wavenumber.Geometer.Contracts.StepTopologyA0.GlbAttachmentDescriptor": {
        "kind": "object",
        "properties": {
            "name": {
                "type": {
                    "kind": "literal",
                    "value_type": "string",
                    "value": "glb",
                },
                "optional": False,
                "constraints": {},
                "field": "name",
            },
            "media_type": {
                "type": {
                    "kind": "literal",
                    "value_type": "string",
                    "value": "model/gltf-binary",
                },
                "optional": False,
                "constraints": {},
                "field": "media_type",
            },
            "format": {
                "type": {
                    "kind": "literal",
                    "value_type": "string",
                    "value": "glb-2.0",
                },
                "optional": False,
                "constraints": {},
                "field": "format",
            },
            "bytes": {
                "type": {
                    "kind": "primitive",
                    "name": "uint32",
                },
                "optional": False,
                "constraints": {
                    "min_value": 1,
                    "max_value": 268435456,
                },
                "field": "bytes",
            },
            "sha256": {
                "type": {
                    "kind": "primitive",
                    "name": "string",
                },
                "optional": False,
                "constraints": {
                    "min_length": 64,
                    "max_length": 64,
                },
                "field": "sha256",
            },
        },
    },
    "Wavenumber.Geometer.Contracts.StepTopologyA0.HierarchyCommand": {
        "kind": "union",
        "variants": [
            {
                "kind": "reference",
                "target": "Wavenumber.Geometer.Contracts.StepTopologyA0.CreateHierarchyProductCommand",
            },
            {
                "kind": "reference",
                "target": "Wavenumber.Geometer.Contracts.StepTopologyA0.CreateHierarchyAssemblyCommand",
            },
            {
                "kind": "reference",
                "target": "Wavenumber.Geometer.Contracts.StepTopologyA0.CreateHierarchyOccurrenceCommand",
            },
            {
                "kind": "reference",
                "target": "Wavenumber.Geometer.Contracts.StepTopologyA0.ReparentHierarchyOccurrenceCommand",
            },
            {
                "kind": "reference",
                "target": "Wavenumber.Geometer.Contracts.StepTopologyA0.RenameHierarchyNodeCommand",
            },
            {
                "kind": "reference",
                "target": "Wavenumber.Geometer.Contracts.StepTopologyA0.EraseHierarchyOccurrenceCommand",
            },
            {
                "kind": "reference",
                "target": "Wavenumber.Geometer.Contracts.StepTopologyA0.EraseHierarchyNodeCommand",
            },
        ],
    },
    "Wavenumber.Geometer.Contracts.StepTopologyA0.HierarchyNode": {
        "kind": "object",
        "properties": {
            "authored_id": {
                "type": {
                    "kind": "primitive",
                    "name": "string",
                },
                "optional": False,
                "constraints": {
                    "min_length": 28,
                    "max_length": 128,
                },
                "field": "authored_id",
            },
            "revision": {
                "type": {
                    "kind": "primitive",
                    "name": "uint32",
                },
                "optional": False,
                "constraints": {
                    "min_value": 1,
                },
                "field": "revision",
            },
            "kind": {
                "type": {
                    "kind": "reference",
                    "target": "Wavenumber.Geometer.Contracts.StepTopologyA0.HierarchyNodeKind",
                },
                "optional": False,
                "constraints": {},
                "field": "kind",
            },
            "name": {
                "type": {
                    "kind": "primitive",
                    "name": "string",
                },
                "optional": False,
                "constraints": {
                    "min_length": 1,
                    "max_length": 4096,
                },
                "field": "name",
            },
            "source_kind": {
                "type": {
                    "kind": "reference",
                    "target": "Wavenumber.Geometer.Contracts.StepTopologyA0.HierarchySourceKind",
                },
                "optional": True,
                "constraints": {},
                "field": "source_kind",
            },
            "source_handle": {
                "type": {
                    "kind": "primitive",
                    "name": "string",
                },
                "optional": True,
                "constraints": {
                    "min_length": 68,
                    "max_length": 68,
                },
                "field": "source_handle",
            },
        },
    },
    "Wavenumber.Geometer.Contracts.StepTopologyA0.HierarchyNodeKind": {
        "kind": "enum",
        "values": ["product", "assembly"],
    },
    "Wavenumber.Geometer.Contracts.StepTopologyA0.HierarchyOccurrence": {
        "kind": "object",
        "properties": {
            "authored_id": {
                "type": {
                    "kind": "primitive",
                    "name": "string",
                },
                "optional": False,
                "constraints": {
                    "min_length": 28,
                    "max_length": 128,
                },
                "field": "authored_id",
            },
            "revision": {
                "type": {
                    "kind": "primitive",
                    "name": "uint32",
                },
                "optional": False,
                "constraints": {
                    "min_value": 1,
                },
                "field": "revision",
            },
            "child_authored_id": {
                "type": {
                    "kind": "primitive",
                    "name": "string",
                },
                "optional": False,
                "constraints": {
                    "min_length": 28,
                    "max_length": 128,
                },
                "field": "child_authored_id",
            },
            "parent_assembly_authored_id": {
                "type": {
                    "kind": "primitive",
                    "name": "string",
                },
                "optional": False,
                "constraints": {
                    "min_length": 28,
                    "max_length": 128,
                },
                "field": "parent_assembly_authored_id",
            },
            "transform": {
                "type": {
                    "kind": "array",
                    "element": {
                        "kind": "primitive",
                        "name": "float64",
                    },
                },
                "optional": False,
                "constraints": {
                    "min_items": 12,
                    "max_items": 12,
                },
                "field": "transform",
            },
        },
    },
    "Wavenumber.Geometer.Contracts.StepTopologyA0.HierarchySourceKind": {
        "kind": "enum",
        "values": ["definition", "body"],
    },
    "Wavenumber.Geometer.Contracts.StepTopologyA0.HierarchyState": {
        "kind": "object",
        "properties": {
            "hierarchy_revision": {
                "type": {
                    "kind": "primitive",
                    "name": "uint32",
                },
                "optional": False,
                "constraints": {},
                "field": "hierarchy_revision",
            },
            "source_brep_sha256": {
                "type": {
                    "kind": "primitive",
                    "name": "string",
                },
                "optional": False,
                "constraints": {
                    "min_length": 64,
                    "max_length": 64,
                },
                "field": "source_brep_sha256",
            },
            "nodes": {
                "type": {
                    "kind": "array",
                    "element": {
                        "kind": "reference",
                        "target": "Wavenumber.Geometer.Contracts.StepTopologyA0.HierarchyNode",
                    },
                },
                "optional": False,
                "constraints": {
                    "max_items": 10000,
                },
                "field": "nodes",
            },
            "occurrences": {
                "type": {
                    "kind": "array",
                    "element": {
                        "kind": "reference",
                        "target": "Wavenumber.Geometer.Contracts.StepTopologyA0.HierarchyOccurrence",
                    },
                },
                "optional": False,
                "constraints": {
                    "max_items": 100000,
                },
                "field": "occurrences",
            },
        },
    },
    "Wavenumber.Geometer.Contracts.StepTopologyA0.InspectionCounts": {
        "kind": "object",
        "properties": {
            "definitions": {
                "type": {
                    "kind": "primitive",
                    "name": "uint32",
                },
                "optional": False,
                "constraints": {
                    "max_value": 10000,
                },
                "field": "definitions",
            },
            "root_occurrences": {
                "type": {
                    "kind": "primitive",
                    "name": "uint32",
                },
                "optional": False,
                "constraints": {
                    "max_value": 100000,
                },
                "field": "root_occurrences",
            },
            "component_occurrences": {
                "type": {
                    "kind": "primitive",
                    "name": "uint32",
                },
                "optional": False,
                "constraints": {
                    "max_value": 100000,
                },
                "field": "component_occurrences",
            },
            "bodies": {
                "type": {
                    "kind": "primitive",
                    "name": "uint32",
                },
                "optional": False,
                "constraints": {
                    "max_value": 100000,
                },
                "field": "bodies",
            },
            "shells": {
                "type": {
                    "kind": "primitive",
                    "name": "uint32",
                },
                "optional": False,
                "constraints": {
                    "max_value": 250000,
                },
                "field": "shells",
            },
            "faces": {
                "type": {
                    "kind": "primitive",
                    "name": "uint32",
                },
                "optional": False,
                "constraints": {
                    "max_value": 1000000,
                },
                "field": "faces",
            },
            "memberships": {
                "type": {
                    "kind": "primitive",
                    "name": "uint32",
                },
                "optional": False,
                "constraints": {
                    "max_value": 5000000,
                },
                "field": "memberships",
            },
        },
    },
    "Wavenumber.Geometer.Contracts.StepTopologyA0.JsonSidecarPersistenceArtifact": {
        "kind": "object",
        "properties": {
            "carrier": {
                "type": {
                    "kind": "literal",
                    "value_type": "string",
                    "value": "json_sidecar",
                },
                "optional": False,
                "constraints": {},
                "field": "carrier",
            },
            "name": {
                "type": {
                    "kind": "literal",
                    "value_type": "string",
                    "value": "state_artifact",
                },
                "optional": False,
                "constraints": {},
                "field": "name",
            },
            "media_type": {
                "type": {
                    "kind": "literal",
                    "value_type": "string",
                    "value": "application/vnd.wavenumber.geometer.step-topology-sidecar+json",
                },
                "optional": False,
                "constraints": {},
                "field": "media_type",
            },
            "format": {
                "type": {
                    "kind": "literal",
                    "value_type": "string",
                    "value": "geometer.step_topology_sidecar.a0",
                },
                "optional": False,
                "constraints": {},
                "field": "format",
            },
            "bytes": {
                "type": {
                    "kind": "primitive",
                    "name": "uint32",
                },
                "optional": False,
                "constraints": {
                    "min_value": 1,
                    "max_value": 67108864,
                },
                "field": "bytes",
            },
            "sha256": {
                "type": {
                    "kind": "primitive",
                    "name": "string",
                },
                "optional": False,
                "constraints": {
                    "min_length": 64,
                    "max_length": 64,
                },
                "field": "sha256",
            },
        },
    },
    "Wavenumber.Geometer.Contracts.StepTopologyA0.LogicalGroup": {
        "kind": "object",
        "properties": {
            "authored_id": {
                "type": {
                    "kind": "primitive",
                    "name": "string",
                },
                "optional": False,
                "constraints": {
                    "min_length": 28,
                    "max_length": 128,
                },
                "field": "authored_id",
            },
            "revision": {
                "type": {
                    "kind": "primitive",
                    "name": "uint32",
                },
                "optional": False,
                "constraints": {
                    "min_value": 1,
                },
                "field": "revision",
            },
            "name": {
                "type": {
                    "kind": "primitive",
                    "name": "string",
                },
                "optional": False,
                "constraints": {
                    "min_length": 1,
                    "max_length": 4096,
                },
                "field": "name",
            },
            "members": {
                "type": {
                    "kind": "array",
                    "element": {
                        "kind": "reference",
                        "target": "Wavenumber.Geometer.Contracts.StepTopologyA0.LogicalGroupMember",
                    },
                },
                "optional": False,
                "constraints": {
                    "min_items": 1,
                    "max_items": 100000,
                },
                "field": "members",
            },
        },
    },
    "Wavenumber.Geometer.Contracts.StepTopologyA0.LogicalGroupCommand": {
        "kind": "union",
        "variants": [
            {
                "kind": "reference",
                "target": "Wavenumber.Geometer.Contracts.StepTopologyA0.CreateLogicalGroupCommand",
            },
            {
                "kind": "reference",
                "target": "Wavenumber.Geometer.Contracts.StepTopologyA0.RenameLogicalGroupCommand",
            },
            {
                "kind": "reference",
                "target": "Wavenumber.Geometer.Contracts.StepTopologyA0.ReplaceLogicalGroupMembersCommand",
            },
            {
                "kind": "reference",
                "target": "Wavenumber.Geometer.Contracts.StepTopologyA0.EraseLogicalGroupCommand",
            },
        ],
    },
    "Wavenumber.Geometer.Contracts.StepTopologyA0.LogicalGroupMember": {
        "kind": "object",
        "properties": {
            "kind": {
                "type": {
                    "kind": "reference",
                    "target": "Wavenumber.Geometer.Contracts.StepTopologyA0.LogicalGroupMemberKind",
                },
                "optional": False,
                "constraints": {},
                "field": "kind",
            },
            "target_handle": {
                "type": {
                    "kind": "primitive",
                    "name": "string",
                },
                "optional": False,
                "constraints": {
                    "min_length": 68,
                    "max_length": 68,
                },
                "field": "target_handle",
            },
        },
    },
    "Wavenumber.Geometer.Contracts.StepTopologyA0.LogicalGroupMemberKind": {
        "kind": "enum",
        "values": ["body", "face"],
    },
    "Wavenumber.Geometer.Contracts.StepTopologyA0.LogicalGroupProbeTarget": {
        "kind": "object",
        "properties": {
            "kind": {
                "type": {
                    "kind": "literal",
                    "value_type": "string",
                    "value": "logical_group",
                },
                "optional": False,
                "constraints": {},
                "field": "kind",
            },
            "group_authored_id": {
                "type": {
                    "kind": "primitive",
                    "name": "string",
                },
                "optional": False,
                "constraints": {
                    "min_length": 28,
                    "max_length": 128,
                },
                "field": "group_authored_id",
            },
        },
    },
    "Wavenumber.Geometer.Contracts.StepTopologyA0.MetadataProbe": {
        "kind": "object",
        "properties": {
            "authored_id": {
                "type": {
                    "kind": "primitive",
                    "name": "string",
                },
                "optional": False,
                "constraints": {
                    "min_length": 28,
                    "max_length": 128,
                },
                "field": "authored_id",
            },
            "revision": {
                "type": {
                    "kind": "primitive",
                    "name": "uint32",
                },
                "optional": False,
                "constraints": {
                    "min_value": 1,
                },
                "field": "revision",
            },
            "target": {
                "type": {
                    "kind": "reference",
                    "target": "Wavenumber.Geometer.Contracts.StepTopologyA0.MetadataProbeTarget",
                },
                "optional": False,
                "constraints": {},
                "field": "target",
            },
            "key": {
                "type": {
                    "kind": "primitive",
                    "name": "string",
                },
                "optional": False,
                "constraints": {
                    "min_length": 32,
                    "max_length": 128,
                },
                "field": "key",
            },
            "value": {
                "type": {
                    "kind": "primitive",
                    "name": "string",
                },
                "optional": False,
                "constraints": {
                    "min_length": 1,
                    "max_length": 4096,
                },
                "field": "value",
            },
        },
    },
    "Wavenumber.Geometer.Contracts.StepTopologyA0.MetadataProbeCommand": {
        "kind": "union",
        "variants": [
            {
                "kind": "reference",
                "target": "Wavenumber.Geometer.Contracts.StepTopologyA0.AttachMetadataProbeCommand",
            },
            {
                "kind": "reference",
                "target": "Wavenumber.Geometer.Contracts.StepTopologyA0.ReplaceMetadataProbeCommand",
            },
            {
                "kind": "reference",
                "target": "Wavenumber.Geometer.Contracts.StepTopologyA0.EraseMetadataProbeCommand",
            },
        ],
    },
    "Wavenumber.Geometer.Contracts.StepTopologyA0.MetadataProbeTarget": {
        "kind": "union",
        "variants": [
            {
                "kind": "reference",
                "target": "Wavenumber.Geometer.Contracts.StepTopologyA0.DocumentProbeTarget",
            },
            {
                "kind": "reference",
                "target": "Wavenumber.Geometer.Contracts.StepTopologyA0.DefinitionProbeTarget",
            },
            {
                "kind": "reference",
                "target": "Wavenumber.Geometer.Contracts.StepTopologyA0.RootOccurrenceProbeTarget",
            },
            {
                "kind": "reference",
                "target": "Wavenumber.Geometer.Contracts.StepTopologyA0.ComponentOccurrenceProbeTarget",
            },
            {
                "kind": "reference",
                "target": "Wavenumber.Geometer.Contracts.StepTopologyA0.BodyProbeTarget",
            },
            {
                "kind": "reference",
                "target": "Wavenumber.Geometer.Contracts.StepTopologyA0.FaceProbeTarget",
            },
            {
                "kind": "reference",
                "target": "Wavenumber.Geometer.Contracts.StepTopologyA0.LogicalGroupProbeTarget",
            },
        ],
    },
    "Wavenumber.Geometer.Contracts.StepTopologyA0.MutationSessionState": {
        "kind": "object",
        "properties": {
            "session": {
                "type": {
                    "kind": "reference",
                    "target": "Wavenumber.Geometer.Contracts.StepTopologyA0.SessionReference",
                },
                "optional": False,
                "constraints": {},
                "field": "session",
            },
            "edit_journal_revision": {
                "type": {
                    "kind": "primitive",
                    "name": "uint32",
                },
                "optional": False,
                "constraints": {
                    "max_value": 100000,
                },
                "field": "edit_journal_revision",
            },
            "accounted_string_bytes": {
                "type": {
                    "kind": "primitive",
                    "name": "uint32",
                },
                "optional": False,
                "constraints": {
                    "max_value": 16777216,
                },
                "field": "accounted_string_bytes",
            },
            "estimated_resident_bytes": {
                "type": {
                    "kind": "primitive",
                    "name": "uint32",
                },
                "optional": False,
                "constraints": {
                    "max_value": 536870912,
                },
                "field": "estimated_resident_bytes",
            },
        },
    },
    "Wavenumber.Geometer.Contracts.StepTopologyA0.OccurrenceSummary": {
        "kind": "union",
        "variants": [
            {
                "kind": "reference",
                "target": "Wavenumber.Geometer.Contracts.StepTopologyA0.RootOccurrenceSummary",
            },
            {
                "kind": "reference",
                "target": "Wavenumber.Geometer.Contracts.StepTopologyA0.ComponentOccurrenceSummary",
            },
        ],
    },
    "Wavenumber.Geometer.Contracts.StepTopologyA0.PageRequest": {
        "kind": "object",
        "properties": {
            "cursor": {
                "type": {
                    "kind": "primitive",
                    "name": "string",
                },
                "optional": True,
                "constraints": {
                    "max_length": 256,
                },
                "field": "cursor",
            },
            "limit": {
                "type": {
                    "kind": "primitive",
                    "name": "uint32",
                },
                "optional": False,
                "constraints": {
                    "min_value": 1,
                    "max_value": 1024,
                },
                "field": "limit",
            },
        },
    },
    "Wavenumber.Geometer.Contracts.StepTopologyA0.PersistenceCarrier": {
        "kind": "enum",
        "values": ["xbf", "xml_xcaf", "step_ap242", "json_sidecar", "edit_journal"],
    },
    "Wavenumber.Geometer.Contracts.StepTopologyA0.RecoveryCandidate": {
        "kind": "object",
        "properties": {
            "target_handle": {
                "type": {
                    "kind": "primitive",
                    "name": "string",
                },
                "optional": False,
                "constraints": {
                    "min_length": 68,
                    "max_length": 68,
                },
                "field": "target_handle",
            },
            "kind": {
                "type": {
                    "kind": "reference",
                    "target": "Wavenumber.Geometer.Contracts.StepTopologyA0.LogicalGroupMemberKind",
                },
                "optional": False,
                "constraints": {},
                "field": "kind",
            },
            "authored_target_id": {
                "type": {
                    "kind": "primitive",
                    "name": "string",
                },
                "optional": True,
                "constraints": {
                    "min_length": 28,
                    "max_length": 128,
                },
                "field": "authored_target_id",
            },
            "topology_link_verified": {
                "type": {
                    "kind": "primitive",
                    "name": "boolean",
                },
                "optional": False,
                "constraints": {},
                "field": "topology_link_verified",
            },
            "carrier_locator": {
                "type": {
                    "kind": "primitive",
                    "name": "string",
                },
                "optional": False,
                "constraints": {
                    "max_length": 4096,
                },
                "field": "carrier_locator",
            },
            "carrier_locator_validated": {
                "type": {
                    "kind": "primitive",
                    "name": "boolean",
                },
                "optional": False,
                "constraints": {},
                "field": "carrier_locator_validated",
            },
            "carrier_record": {
                "type": {
                    "kind": "primitive",
                    "name": "string",
                },
                "optional": False,
                "constraints": {
                    "max_length": 4096,
                },
                "field": "carrier_record",
            },
            "lineage": {
                "type": {
                    "kind": "reference",
                    "target": "Wavenumber.Geometer.Contracts.StepTopologyA0.RecoveryLineage",
                },
                "optional": False,
                "constraints": {},
                "field": "lineage",
            },
            "fingerprint": {
                "type": {
                    "kind": "reference",
                    "target": "Wavenumber.Geometer.Contracts.StepTopologyA0.RecoveryFingerprint",
                },
                "optional": True,
                "constraints": {},
                "field": "fingerprint",
            },
        },
    },
    "Wavenumber.Geometer.Contracts.StepTopologyA0.RecoveryCarrierRecord": {
        "kind": "object",
        "properties": {
            "value": {
                "type": {
                    "kind": "primitive",
                    "name": "string",
                },
                "optional": False,
                "constraints": {
                    "min_length": 1,
                    "max_length": 4096,
                },
                "field": "value",
            },
        },
    },
    "Wavenumber.Geometer.Contracts.StepTopologyA0.RecoveryComparedField": {
        "kind": "object",
        "properties": {
            "value": {
                "type": {
                    "kind": "primitive",
                    "name": "string",
                },
                "optional": False,
                "constraints": {
                    "min_length": 1,
                    "max_length": 128,
                },
                "field": "value",
            },
        },
    },
    "Wavenumber.Geometer.Contracts.StepTopologyA0.RecoveryConfidence": {
        "kind": "enum",
        "values": ["high", "medium", "low", "none"],
    },
    "Wavenumber.Geometer.Contracts.StepTopologyA0.RecoveryEvidence": {
        "kind": "object",
        "properties": {
            "candidate_count": {
                "type": {
                    "kind": "primitive",
                    "name": "uint32",
                },
                "optional": False,
                "constraints": {
                    "max_value": 16,
                },
                "field": "candidate_count",
            },
            "matching_candidate_count": {
                "type": {
                    "kind": "primitive",
                    "name": "uint32",
                },
                "optional": False,
                "constraints": {
                    "max_value": 16,
                },
                "field": "matching_candidate_count",
            },
            "compared_fields": {
                "type": {
                    "kind": "array",
                    "element": {
                        "kind": "reference",
                        "target": "Wavenumber.Geometer.Contracts.StepTopologyA0.RecoveryComparedField",
                    },
                },
                "optional": False,
                "constraints": {
                    "max_items": 16,
                },
                "field": "compared_fields",
            },
            "tolerances": {
                "type": {
                    "kind": "reference",
                    "target": "Wavenumber.Geometer.Contracts.StepTopologyA0.RecoveryTolerances",
                },
                "optional": False,
                "constraints": {},
                "field": "tolerances",
            },
            "carrier_records": {
                "type": {
                    "kind": "array",
                    "element": {
                        "kind": "reference",
                        "target": "Wavenumber.Geometer.Contracts.StepTopologyA0.RecoveryCarrierRecord",
                    },
                },
                "optional": False,
                "constraints": {
                    "max_items": 16,
                },
                "field": "carrier_records",
            },
            "rejected_alternatives": {
                "type": {
                    "kind": "array",
                    "element": {
                        "kind": "reference",
                        "target": "Wavenumber.Geometer.Contracts.StepTopologyA0.RecoveryRejectedAlternative",
                    },
                },
                "optional": False,
                "constraints": {
                    "max_items": 16,
                },
                "field": "rejected_alternatives",
            },
        },
    },
    "Wavenumber.Geometer.Contracts.StepTopologyA0.RecoveryFingerprint": {
        "kind": "object",
        "properties": {
            "normalized_length_unit": {
                "type": {
                    "kind": "literal",
                    "value_type": "string",
                    "value": "millimeter",
                },
                "optional": False,
                "constraints": {},
                "field": "normalized_length_unit",
            },
            "coordinate_frame": {
                "type": {
                    "kind": "primitive",
                    "name": "string",
                },
                "optional": False,
                "constraints": {
                    "min_length": 1,
                    "max_length": 128,
                },
                "field": "coordinate_frame",
            },
            "occurrence_context": {
                "type": {
                    "kind": "primitive",
                    "name": "string",
                },
                "optional": False,
                "constraints": {
                    "min_length": 1,
                    "max_length": 128,
                },
                "field": "occurrence_context",
            },
            "geometry_kind": {
                "type": {
                    "kind": "primitive",
                    "name": "string",
                },
                "optional": False,
                "constraints": {
                    "min_length": 1,
                    "max_length": 128,
                },
                "field": "geometry_kind",
            },
            "area_mm2": {
                "type": {
                    "kind": "primitive",
                    "name": "float64",
                },
                "optional": False,
                "constraints": {
                    "min_value": 0,
                },
                "field": "area_mm2",
            },
            "volume_mm3": {
                "type": {
                    "kind": "primitive",
                    "name": "float64",
                },
                "optional": False,
                "constraints": {
                    "min_value": 0,
                },
                "field": "volume_mm3",
            },
            "centroid_mm": {
                "type": {
                    "kind": "array",
                    "element": {
                        "kind": "primitive",
                        "name": "float64",
                    },
                },
                "optional": False,
                "constraints": {
                    "min_items": 3,
                    "max_items": 3,
                },
                "field": "centroid_mm",
            },
            "bounds_mm": {
                "type": {
                    "kind": "array",
                    "element": {
                        "kind": "primitive",
                        "name": "float64",
                    },
                },
                "optional": False,
                "constraints": {
                    "min_items": 6,
                    "max_items": 6,
                },
                "field": "bounds_mm",
            },
            "adjacency_sha256": {
                "type": {
                    "kind": "primitive",
                    "name": "string",
                },
                "optional": False,
                "constraints": {
                    "min_length": 64,
                    "max_length": 64,
                },
                "field": "adjacency_sha256",
            },
        },
    },
    "Wavenumber.Geometer.Contracts.StepTopologyA0.RecoveryGroupCompleteness": {
        "kind": "enum",
        "values": ["fully_recovered", "partially_recovered", "unrecovered", "unsupported"],
    },
    "Wavenumber.Geometer.Contracts.StepTopologyA0.RecoveryGroupRequest": {
        "kind": "object",
        "properties": {
            "group_authored_id": {
                "type": {
                    "kind": "primitive",
                    "name": "string",
                },
                "optional": False,
                "constraints": {
                    "min_length": 28,
                    "max_length": 128,
                },
                "field": "group_authored_id",
            },
            "provenance": {
                "type": {
                    "kind": "reference",
                    "target": "Wavenumber.Geometer.Contracts.StepTopologyA0.RecoveryProvenance",
                },
                "optional": False,
                "constraints": {},
                "field": "provenance",
            },
            "tolerances": {
                "type": {
                    "kind": "reference",
                    "target": "Wavenumber.Geometer.Contracts.StepTopologyA0.RecoveryTolerances",
                },
                "optional": False,
                "constraints": {},
                "field": "tolerances",
            },
            "members": {
                "type": {
                    "kind": "array",
                    "element": {
                        "kind": "reference",
                        "target": "Wavenumber.Geometer.Contracts.StepTopologyA0.RecoveryMemberRequest",
                    },
                },
                "optional": False,
                "constraints": {
                    "min_items": 1,
                    "max_items": 256,
                },
                "field": "members",
            },
        },
    },
    "Wavenumber.Geometer.Contracts.StepTopologyA0.RecoveryGroupResult": {
        "kind": "object",
        "properties": {
            "group_authored_id": {
                "type": {
                    "kind": "primitive",
                    "name": "string",
                },
                "optional": False,
                "constraints": {
                    "min_length": 28,
                    "max_length": 128,
                },
                "field": "group_authored_id",
            },
            "provenance": {
                "type": {
                    "kind": "reference",
                    "target": "Wavenumber.Geometer.Contracts.StepTopologyA0.RecoveryProvenance",
                },
                "optional": False,
                "constraints": {},
                "field": "provenance",
            },
            "resolution_state": {
                "type": {
                    "kind": "reference",
                    "target": "Wavenumber.Geometer.Contracts.StepTopologyA0.RecoveryResolutionState",
                },
                "optional": False,
                "constraints": {},
                "field": "resolution_state",
            },
            "completeness": {
                "type": {
                    "kind": "reference",
                    "target": "Wavenumber.Geometer.Contracts.StepTopologyA0.RecoveryGroupCompleteness",
                },
                "optional": False,
                "constraints": {},
                "field": "completeness",
            },
            "resolved_member_count": {
                "type": {
                    "kind": "primitive",
                    "name": "uint32",
                },
                "optional": False,
                "constraints": {
                    "max_value": 256,
                },
                "field": "resolved_member_count",
            },
            "ambiguous_member_count": {
                "type": {
                    "kind": "primitive",
                    "name": "uint32",
                },
                "optional": False,
                "constraints": {
                    "max_value": 256,
                },
                "field": "ambiguous_member_count",
            },
            "unresolved_member_count": {
                "type": {
                    "kind": "primitive",
                    "name": "uint32",
                },
                "optional": False,
                "constraints": {
                    "max_value": 256,
                },
                "field": "unresolved_member_count",
            },
            "unsupported_member_count": {
                "type": {
                    "kind": "primitive",
                    "name": "uint32",
                },
                "optional": False,
                "constraints": {
                    "max_value": 256,
                },
                "field": "unsupported_member_count",
            },
            "members": {
                "type": {
                    "kind": "array",
                    "element": {
                        "kind": "reference",
                        "target": "Wavenumber.Geometer.Contracts.StepTopologyA0.RecoveryMemberResult",
                    },
                },
                "optional": False,
                "constraints": {
                    "min_items": 1,
                    "max_items": 256,
                },
                "field": "members",
            },
        },
    },
    "Wavenumber.Geometer.Contracts.StepTopologyA0.RecoveryLineage": {
        "kind": "enum",
        "values": ["none", "split_from_source", "merged_from_sources"],
    },
    "Wavenumber.Geometer.Contracts.StepTopologyA0.RecoveryMemberRequest": {
        "kind": "object",
        "properties": {
            "member_record_id": {
                "type": {
                    "kind": "primitive",
                    "name": "string",
                },
                "optional": False,
                "constraints": {
                    "min_length": 28,
                    "max_length": 128,
                },
                "field": "member_record_id",
            },
            "kind": {
                "type": {
                    "kind": "reference",
                    "target": "Wavenumber.Geometer.Contracts.StepTopologyA0.LogicalGroupMemberKind",
                },
                "optional": False,
                "constraints": {},
                "field": "kind",
            },
            "authored_target_id": {
                "type": {
                    "kind": "primitive",
                    "name": "string",
                },
                "optional": False,
                "constraints": {
                    "max_length": 128,
                },
                "field": "authored_target_id",
            },
            "carrier_locator": {
                "type": {
                    "kind": "primitive",
                    "name": "string",
                },
                "optional": False,
                "constraints": {
                    "max_length": 4096,
                },
                "field": "carrier_locator",
            },
            "source_fingerprint": {
                "type": {
                    "kind": "reference",
                    "target": "Wavenumber.Geometer.Contracts.StepTopologyA0.RecoveryFingerprint",
                },
                "optional": True,
                "constraints": {},
                "field": "source_fingerprint",
            },
            "candidates": {
                "type": {
                    "kind": "array",
                    "element": {
                        "kind": "reference",
                        "target": "Wavenumber.Geometer.Contracts.StepTopologyA0.RecoveryCandidate",
                    },
                },
                "optional": False,
                "constraints": {
                    "max_items": 16,
                },
                "field": "candidates",
            },
        },
    },
    "Wavenumber.Geometer.Contracts.StepTopologyA0.RecoveryMemberResult": {
        "kind": "object",
        "properties": {
            "member_record_id": {
                "type": {
                    "kind": "primitive",
                    "name": "string",
                },
                "optional": False,
                "constraints": {
                    "min_length": 28,
                    "max_length": 128,
                },
                "field": "member_record_id",
            },
            "kind": {
                "type": {
                    "kind": "reference",
                    "target": "Wavenumber.Geometer.Contracts.StepTopologyA0.LogicalGroupMemberKind",
                },
                "optional": False,
                "constraints": {},
                "field": "kind",
            },
            "authored_target_id": {
                "type": {
                    "kind": "primitive",
                    "name": "string",
                },
                "optional": False,
                "constraints": {
                    "max_length": 128,
                },
                "field": "authored_target_id",
            },
            "resolution_state": {
                "type": {
                    "kind": "reference",
                    "target": "Wavenumber.Geometer.Contracts.StepTopologyA0.RecoveryResolutionState",
                },
                "optional": False,
                "constraints": {},
                "field": "resolution_state",
            },
            "resolution_method": {
                "type": {
                    "kind": "reference",
                    "target": "Wavenumber.Geometer.Contracts.StepTopologyA0.RecoveryResolutionMethod",
                },
                "optional": False,
                "constraints": {},
                "field": "resolution_method",
            },
            "topology_comparison": {
                "type": {
                    "kind": "reference",
                    "target": "Wavenumber.Geometer.Contracts.StepTopologyA0.RecoveryTopologyComparison",
                },
                "optional": False,
                "constraints": {},
                "field": "topology_comparison",
            },
            "confidence": {
                "type": {
                    "kind": "reference",
                    "target": "Wavenumber.Geometer.Contracts.StepTopologyA0.RecoveryConfidence",
                },
                "optional": False,
                "constraints": {},
                "field": "confidence",
            },
            "resolved_target_handle": {
                "type": {
                    "kind": "primitive",
                    "name": "string",
                },
                "optional": True,
                "constraints": {
                    "min_length": 68,
                    "max_length": 68,
                },
                "field": "resolved_target_handle",
            },
            "evidence": {
                "type": {
                    "kind": "reference",
                    "target": "Wavenumber.Geometer.Contracts.StepTopologyA0.RecoveryEvidence",
                },
                "optional": False,
                "constraints": {},
                "field": "evidence",
            },
        },
    },
    "Wavenumber.Geometer.Contracts.StepTopologyA0.RecoveryProvenance": {
        "kind": "object",
        "properties": {
            "source_artifact_sha256": {
                "type": {
                    "kind": "primitive",
                    "name": "string",
                },
                "optional": False,
                "constraints": {
                    "min_length": 64,
                    "max_length": 64,
                },
                "field": "source_artifact_sha256",
            },
            "candidate_artifact_sha256": {
                "type": {
                    "kind": "primitive",
                    "name": "string",
                },
                "optional": False,
                "constraints": {
                    "min_length": 64,
                    "max_length": 64,
                },
                "field": "candidate_artifact_sha256",
            },
            "source_occt_version": {
                "type": {
                    "kind": "primitive",
                    "name": "string",
                },
                "optional": False,
                "constraints": {
                    "min_length": 1,
                    "max_length": 64,
                },
                "field": "source_occt_version",
            },
            "candidate_occt_version": {
                "type": {
                    "kind": "primitive",
                    "name": "string",
                },
                "optional": False,
                "constraints": {
                    "min_length": 1,
                    "max_length": 64,
                },
                "field": "candidate_occt_version",
            },
            "source_driver": {
                "type": {
                    "kind": "primitive",
                    "name": "string",
                },
                "optional": False,
                "constraints": {
                    "min_length": 1,
                    "max_length": 128,
                },
                "field": "source_driver",
            },
            "candidate_driver": {
                "type": {
                    "kind": "primitive",
                    "name": "string",
                },
                "optional": False,
                "constraints": {
                    "min_length": 1,
                    "max_length": 128,
                },
                "field": "candidate_driver",
            },
            "source_writer_settings": {
                "type": {
                    "kind": "primitive",
                    "name": "string",
                },
                "optional": False,
                "constraints": {
                    "min_length": 1,
                    "max_length": 4096,
                },
                "field": "source_writer_settings",
            },
            "candidate_writer_settings": {
                "type": {
                    "kind": "primitive",
                    "name": "string",
                },
                "optional": False,
                "constraints": {
                    "min_length": 1,
                    "max_length": 4096,
                },
                "field": "candidate_writer_settings",
            },
            "command_provenance": {
                "type": {
                    "kind": "primitive",
                    "name": "string",
                },
                "optional": False,
                "constraints": {
                    "min_length": 1,
                    "max_length": 8192,
                },
                "field": "command_provenance",
            },
            "measured_wall_time_milliseconds": {
                "type": {
                    "kind": "primitive",
                    "name": "float64",
                },
                "optional": False,
                "constraints": {
                    "min_value": 0,
                },
                "field": "measured_wall_time_milliseconds",
            },
        },
    },
    "Wavenumber.Geometer.Contracts.StepTopologyA0.RecoveryRejectedAlternative": {
        "kind": "object",
        "properties": {
            "target_handle": {
                "type": {
                    "kind": "primitive",
                    "name": "string",
                },
                "optional": False,
                "constraints": {
                    "min_length": 68,
                    "max_length": 68,
                },
                "field": "target_handle",
            },
            "reason": {
                "type": {
                    "kind": "primitive",
                    "name": "string",
                },
                "optional": False,
                "constraints": {
                    "min_length": 1,
                    "max_length": 4096,
                },
                "field": "reason",
            },
        },
    },
    "Wavenumber.Geometer.Contracts.StepTopologyA0.RecoveryResolutionMethod": {
        "kind": "enum",
        "values": [
            "authored_id_topology_link",
            "validated_carrier_locator",
            "unique_geometry_adjacency_fingerprint",
            "none",
        ],
    },
    "Wavenumber.Geometer.Contracts.StepTopologyA0.RecoveryResolutionState": {
        "kind": "enum",
        "values": ["resolved", "ambiguous", "unresolved", "unsupported"],
    },
    "Wavenumber.Geometer.Contracts.StepTopologyA0.RecoveryTolerances": {
        "kind": "object",
        "properties": {
            "length_mm": {
                "type": {
                    "kind": "primitive",
                    "name": "float64",
                },
                "optional": False,
                "constraints": {
                    "min_value": 1e-9,
                },
                "field": "length_mm",
            },
            "area_mm2": {
                "type": {
                    "kind": "primitive",
                    "name": "float64",
                },
                "optional": False,
                "constraints": {
                    "min_value": 1e-9,
                },
                "field": "area_mm2",
            },
            "volume_mm3": {
                "type": {
                    "kind": "primitive",
                    "name": "float64",
                },
                "optional": False,
                "constraints": {
                    "min_value": 1e-9,
                },
                "field": "volume_mm3",
            },
        },
    },
    "Wavenumber.Geometer.Contracts.StepTopologyA0.RecoveryTopologyComparison": {
        "kind": "enum",
        "values": ["unchanged", "relocated", "split", "merged", "otherwise_changed", "not_compared", "unavailable"],
    },
    "Wavenumber.Geometer.Contracts.StepTopologyA0.RenameHierarchyNodeCommand": {
        "kind": "object",
        "properties": {
            "kind": {
                "type": {
                    "kind": "literal",
                    "value_type": "string",
                    "value": "rename_node",
                },
                "optional": False,
                "constraints": {},
                "field": "kind",
            },
            "authored_id": {
                "type": {
                    "kind": "primitive",
                    "name": "string",
                },
                "optional": False,
                "constraints": {
                    "min_length": 28,
                    "max_length": 128,
                },
                "field": "authored_id",
            },
            "expected_revision": {
                "type": {
                    "kind": "primitive",
                    "name": "uint32",
                },
                "optional": False,
                "constraints": {
                    "min_value": 1,
                },
                "field": "expected_revision",
            },
            "name": {
                "type": {
                    "kind": "primitive",
                    "name": "string",
                },
                "optional": False,
                "constraints": {
                    "min_length": 1,
                    "max_length": 4096,
                },
                "field": "name",
            },
        },
    },
    "Wavenumber.Geometer.Contracts.StepTopologyA0.RenameLogicalGroupCommand": {
        "kind": "object",
        "properties": {
            "kind": {
                "type": {
                    "kind": "literal",
                    "value_type": "string",
                    "value": "rename",
                },
                "optional": False,
                "constraints": {},
                "field": "kind",
            },
            "authored_id": {
                "type": {
                    "kind": "primitive",
                    "name": "string",
                },
                "optional": False,
                "constraints": {
                    "min_length": 28,
                    "max_length": 128,
                },
                "field": "authored_id",
            },
            "expected_revision": {
                "type": {
                    "kind": "primitive",
                    "name": "uint32",
                },
                "optional": False,
                "constraints": {
                    "min_value": 1,
                },
                "field": "expected_revision",
            },
            "name": {
                "type": {
                    "kind": "primitive",
                    "name": "string",
                },
                "optional": False,
                "constraints": {
                    "min_length": 1,
                    "max_length": 4096,
                },
                "field": "name",
            },
        },
    },
    "Wavenumber.Geometer.Contracts.StepTopologyA0.RenderArtifactDescriptor": {
        "kind": "object",
        "properties": {
            "artifact_handle": {
                "type": {
                    "kind": "primitive",
                    "name": "string",
                },
                "optional": False,
                "constraints": {
                    "min_length": 68,
                    "max_length": 68,
                },
                "field": "artifact_handle",
            },
            "content_sha256": {
                "type": {
                    "kind": "primitive",
                    "name": "string",
                },
                "optional": False,
                "constraints": {
                    "min_length": 64,
                    "max_length": 64,
                },
                "field": "content_sha256",
            },
            "render_artifact_handle": {
                "type": {
                    "kind": "primitive",
                    "name": "string",
                },
                "optional": False,
                "constraints": {
                    "min_length": 68,
                    "max_length": 68,
                },
                "field": "render_artifact_handle",
            },
            "render_content_sha256": {
                "type": {
                    "kind": "primitive",
                    "name": "string",
                },
                "optional": False,
                "constraints": {
                    "min_length": 64,
                    "max_length": 64,
                },
                "field": "render_content_sha256",
            },
            "binding_layout": {
                "type": {
                    "kind": "literal",
                    "value_type": "string",
                    "value": "node-primitive-a0",
                },
                "optional": False,
                "constraints": {},
                "field": "binding_layout",
            },
            "geometry_length_unit": {
                "type": {
                    "kind": "literal",
                    "value_type": "string",
                    "value": "meter",
                },
                "optional": False,
                "constraints": {},
                "field": "geometry_length_unit",
            },
            "source_length_unit": {
                "type": {
                    "kind": "literal",
                    "value_type": "string",
                    "value": "millimeter",
                },
                "optional": False,
                "constraints": {},
                "field": "source_length_unit",
            },
            "counts": {
                "type": {
                    "kind": "reference",
                    "target": "Wavenumber.Geometer.Contracts.StepTopologyA0.RenderCounts",
                },
                "optional": False,
                "constraints": {},
                "field": "counts",
            },
        },
    },
    "Wavenumber.Geometer.Contracts.StepTopologyA0.RenderCounts": {
        "kind": "object",
        "properties": {
            "meshes": {
                "type": {
                    "kind": "primitive",
                    "name": "uint32",
                },
                "optional": False,
                "constraints": {
                    "max_value": 10000,
                },
                "field": "meshes",
            },
            "instances": {
                "type": {
                    "kind": "primitive",
                    "name": "uint32",
                },
                "optional": False,
                "constraints": {
                    "max_value": 100000,
                },
                "field": "instances",
            },
            "primitives": {
                "type": {
                    "kind": "primitive",
                    "name": "uint32",
                },
                "optional": False,
                "constraints": {
                    "max_value": 1000000,
                },
                "field": "primitives",
            },
            "geometry_triangles": {
                "type": {
                    "kind": "primitive",
                    "name": "uint32",
                },
                "optional": False,
                "constraints": {
                    "max_value": 10000000,
                },
                "field": "geometry_triangles",
            },
            "instanced_triangles": {
                "type": {
                    "kind": "primitive",
                    "name": "uint32",
                },
                "optional": False,
                "constraints": {
                    "max_value": 50000000,
                },
                "field": "instanced_triangles",
            },
        },
    },
    "Wavenumber.Geometer.Contracts.StepTopologyA0.ReparentHierarchyOccurrenceCommand": {
        "kind": "object",
        "properties": {
            "kind": {
                "type": {
                    "kind": "literal",
                    "value_type": "string",
                    "value": "reparent_occurrence",
                },
                "optional": False,
                "constraints": {},
                "field": "kind",
            },
            "authored_id": {
                "type": {
                    "kind": "primitive",
                    "name": "string",
                },
                "optional": False,
                "constraints": {
                    "min_length": 28,
                    "max_length": 128,
                },
                "field": "authored_id",
            },
            "expected_revision": {
                "type": {
                    "kind": "primitive",
                    "name": "uint32",
                },
                "optional": False,
                "constraints": {
                    "min_value": 1,
                },
                "field": "expected_revision",
            },
            "parent_assembly_authored_id": {
                "type": {
                    "kind": "primitive",
                    "name": "string",
                },
                "optional": False,
                "constraints": {
                    "min_length": 28,
                    "max_length": 128,
                },
                "field": "parent_assembly_authored_id",
            },
            "transform": {
                "type": {
                    "kind": "array",
                    "element": {
                        "kind": "primitive",
                        "name": "float64",
                    },
                },
                "optional": False,
                "constraints": {
                    "min_items": 12,
                    "max_items": 12,
                },
                "field": "transform",
            },
        },
    },
    "Wavenumber.Geometer.Contracts.StepTopologyA0.ReplaceLogicalGroupMembersCommand": {
        "kind": "object",
        "properties": {
            "kind": {
                "type": {
                    "kind": "literal",
                    "value_type": "string",
                    "value": "replace_members",
                },
                "optional": False,
                "constraints": {},
                "field": "kind",
            },
            "authored_id": {
                "type": {
                    "kind": "primitive",
                    "name": "string",
                },
                "optional": False,
                "constraints": {
                    "min_length": 28,
                    "max_length": 128,
                },
                "field": "authored_id",
            },
            "expected_revision": {
                "type": {
                    "kind": "primitive",
                    "name": "uint32",
                },
                "optional": False,
                "constraints": {
                    "min_value": 1,
                },
                "field": "expected_revision",
            },
            "member_handles": {
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
                    "max_items": 100000,
                },
                "field": "member_handles",
            },
        },
    },
    "Wavenumber.Geometer.Contracts.StepTopologyA0.ReplaceMetadataProbeCommand": {
        "kind": "object",
        "properties": {
            "kind": {
                "type": {
                    "kind": "literal",
                    "value_type": "string",
                    "value": "replace",
                },
                "optional": False,
                "constraints": {},
                "field": "kind",
            },
            "authored_id": {
                "type": {
                    "kind": "primitive",
                    "name": "string",
                },
                "optional": False,
                "constraints": {
                    "min_length": 28,
                    "max_length": 128,
                },
                "field": "authored_id",
            },
            "expected_revision": {
                "type": {
                    "kind": "primitive",
                    "name": "uint32",
                },
                "optional": False,
                "constraints": {
                    "min_value": 1,
                },
                "field": "expected_revision",
            },
            "target": {
                "type": {
                    "kind": "reference",
                    "target": "Wavenumber.Geometer.Contracts.StepTopologyA0.MetadataProbeTarget",
                },
                "optional": False,
                "constraints": {},
                "field": "target",
            },
            "key": {
                "type": {
                    "kind": "primitive",
                    "name": "string",
                },
                "optional": False,
                "constraints": {
                    "min_length": 32,
                    "max_length": 128,
                },
                "field": "key",
            },
            "value": {
                "type": {
                    "kind": "primitive",
                    "name": "string",
                },
                "optional": False,
                "constraints": {
                    "min_length": 1,
                    "max_length": 4096,
                },
                "field": "value",
            },
        },
    },
    "Wavenumber.Geometer.Contracts.StepTopologyA0.RestoreStateArtifact": {
        "kind": "union",
        "variants": [
            {
                "kind": "reference",
                "target": "Wavenumber.Geometer.Contracts.StepTopologyA0.XbfPersistenceArtifact",
            },
            {
                "kind": "reference",
                "target": "Wavenumber.Geometer.Contracts.StepTopologyA0.XmlXcafPersistenceArtifact",
            },
            {
                "kind": "reference",
                "target": "Wavenumber.Geometer.Contracts.StepTopologyA0.StepAp242PersistenceArtifact",
            },
            {
                "kind": "reference",
                "target": "Wavenumber.Geometer.Contracts.StepTopologyA0.JsonSidecarPersistenceArtifact",
            },
            {
                "kind": "reference",
                "target": "Wavenumber.Geometer.Contracts.StepTopologyA0.EditJournalPersistenceArtifact",
            },
        ],
    },
    "Wavenumber.Geometer.Contracts.StepTopologyA0.RootOccurrenceProbeTarget": {
        "kind": "object",
        "properties": {
            "kind": {
                "type": {
                    "kind": "literal",
                    "value_type": "string",
                    "value": "root_occurrence",
                },
                "optional": False,
                "constraints": {},
                "field": "kind",
            },
            "target_handle": {
                "type": {
                    "kind": "primitive",
                    "name": "string",
                },
                "optional": False,
                "constraints": {
                    "min_length": 68,
                    "max_length": 68,
                },
                "field": "target_handle",
            },
        },
    },
    "Wavenumber.Geometer.Contracts.StepTopologyA0.RootOccurrenceSummary": {
        "kind": "object",
        "properties": {
            "kind": {
                "type": {
                    "kind": "literal",
                    "value_type": "string",
                    "value": "root",
                },
                "optional": False,
                "constraints": {},
                "field": "kind",
            },
            "handle": {
                "type": {
                    "kind": "primitive",
                    "name": "string",
                },
                "optional": False,
                "constraints": {
                    "min_length": 68,
                    "max_length": 68,
                },
                "field": "handle",
            },
            "definition_handle": {
                "type": {
                    "kind": "primitive",
                    "name": "string",
                },
                "optional": False,
                "constraints": {
                    "min_length": 68,
                    "max_length": 68,
                },
                "field": "definition_handle",
            },
            "name": {
                "type": {
                    "kind": "primitive",
                    "name": "string",
                },
                "optional": False,
                "constraints": {
                    "max_length": 4096,
                },
                "field": "name",
            },
            "transform": {
                "type": {
                    "kind": "array",
                    "element": {
                        "kind": "primitive",
                        "name": "float64",
                    },
                },
                "optional": False,
                "constraints": {
                    "min_items": 12,
                    "max_items": 12,
                },
                "field": "transform",
            },
        },
    },
    "Wavenumber.Geometer.Contracts.StepTopologyA0.SaveCarrier": {
        "kind": "enum",
        "values": ["xbf", "xml_xcaf", "step_ap242", "json_sidecar"],
    },
    "Wavenumber.Geometer.Contracts.StepTopologyA0.SavePersistenceArtifact": {
        "kind": "union",
        "variants": [
            {
                "kind": "reference",
                "target": "Wavenumber.Geometer.Contracts.StepTopologyA0.XbfPersistenceArtifact",
            },
            {
                "kind": "reference",
                "target": "Wavenumber.Geometer.Contracts.StepTopologyA0.XmlXcafPersistenceArtifact",
            },
            {
                "kind": "reference",
                "target": "Wavenumber.Geometer.Contracts.StepTopologyA0.StepAp242PersistenceArtifact",
            },
            {
                "kind": "reference",
                "target": "Wavenumber.Geometer.Contracts.StepTopologyA0.JsonSidecarPersistenceArtifact",
            },
        ],
    },
    "Wavenumber.Geometer.Contracts.StepTopologyA0.SessionReference": {
        "kind": "object",
        "properties": {
            "session_handle": {
                "type": {
                    "kind": "primitive",
                    "name": "string",
                },
                "optional": False,
                "constraints": {
                    "min_length": 68,
                    "max_length": 68,
                },
                "field": "session_handle",
            },
            "generation": {
                "type": {
                    "kind": "primitive",
                    "name": "uint32",
                },
                "optional": False,
                "constraints": {
                    "min_value": 1,
                },
                "field": "generation",
            },
        },
    },
    "Wavenumber.Geometer.Contracts.StepTopologyA0.ShellSummary": {
        "kind": "object",
        "properties": {
            "handle": {
                "type": {
                    "kind": "primitive",
                    "name": "string",
                },
                "optional": False,
                "constraints": {
                    "min_length": 68,
                    "max_length": 68,
                },
                "field": "handle",
            },
            "definition_handle": {
                "type": {
                    "kind": "primitive",
                    "name": "string",
                },
                "optional": False,
                "constraints": {
                    "min_length": 68,
                    "max_length": 68,
                },
                "field": "definition_handle",
            },
            "body_count": {
                "type": {
                    "kind": "primitive",
                    "name": "uint32",
                },
                "optional": False,
                "constraints": {
                    "max_value": 100000,
                },
                "field": "body_count",
            },
            "face_count": {
                "type": {
                    "kind": "primitive",
                    "name": "uint32",
                },
                "optional": False,
                "constraints": {
                    "max_value": 1000000,
                },
                "field": "face_count",
            },
            "source_entity": {
                "type": {
                    "kind": "reference",
                    "target": "Wavenumber.Geometer.Contracts.StepTopologyA0.SourceEntityEvidence",
                },
                "optional": True,
                "constraints": {},
                "field": "source_entity",
            },
        },
    },
    "Wavenumber.Geometer.Contracts.StepTopologyA0.SourceDescriptor": {
        "kind": "object",
        "properties": {
            "format": {
                "type": {
                    "kind": "literal",
                    "value_type": "string",
                    "value": "step",
                },
                "optional": False,
                "constraints": {},
                "field": "format",
            },
            "sha256": {
                "type": {
                    "kind": "primitive",
                    "name": "string",
                },
                "optional": False,
                "constraints": {
                    "min_length": 64,
                    "max_length": 64,
                },
                "field": "sha256",
            },
            "bytes": {
                "type": {
                    "kind": "primitive",
                    "name": "uint32",
                },
                "optional": False,
                "constraints": {
                    "min_value": 1,
                    "max_value": 268435456,
                },
                "field": "bytes",
            },
            "normalized_length_unit": {
                "type": {
                    "kind": "literal",
                    "value_type": "string",
                    "value": "millimeter",
                },
                "optional": False,
                "constraints": {},
                "field": "normalized_length_unit",
            },
        },
    },
    "Wavenumber.Geometer.Contracts.StepTopologyA0.SourceEntityEvidence": {
        "kind": "object",
        "properties": {
            "mapped": {
                "type": {
                    "kind": "primitive",
                    "name": "boolean",
                },
                "optional": False,
                "constraints": {},
                "field": "mapped",
            },
            "shape_result_round_trip": {
                "type": {
                    "kind": "primitive",
                    "name": "boolean",
                },
                "optional": False,
                "constraints": {},
                "field": "shape_result_round_trip",
            },
            "model_number": {
                "type": {
                    "kind": "primitive",
                    "name": "uint32",
                },
                "optional": True,
                "constraints": {
                    "min_value": 1,
                    "max_value": 5000000,
                },
                "field": "model_number",
            },
            "entity_type": {
                "type": {
                    "kind": "primitive",
                    "name": "string",
                },
                "optional": True,
                "constraints": {
                    "min_length": 1,
                    "max_length": 128,
                },
                "field": "entity_type",
            },
            "mapping_method": {
                "type": {
                    "kind": "primitive",
                    "name": "string",
                },
                "optional": True,
                "constraints": {
                    "min_length": 1,
                    "max_length": 128,
                },
                "field": "mapping_method",
            },
        },
    },
    "Wavenumber.Geometer.Contracts.StepTopologyA0.StepAp242PersistenceArtifact": {
        "kind": "object",
        "properties": {
            "carrier": {
                "type": {
                    "kind": "literal",
                    "value_type": "string",
                    "value": "step_ap242",
                },
                "optional": False,
                "constraints": {},
                "field": "carrier",
            },
            "name": {
                "type": {
                    "kind": "literal",
                    "value_type": "string",
                    "value": "state_artifact",
                },
                "optional": False,
                "constraints": {},
                "field": "name",
            },
            "media_type": {
                "type": {
                    "kind": "literal",
                    "value_type": "string",
                    "value": "application/step",
                },
                "optional": False,
                "constraints": {},
                "field": "media_type",
            },
            "format": {
                "type": {
                    "kind": "literal",
                    "value_type": "string",
                    "value": "ap242-managed-model-based-3d-engineering",
                },
                "optional": False,
                "constraints": {},
                "field": "format",
            },
            "bytes": {
                "type": {
                    "kind": "primitive",
                    "name": "uint32",
                },
                "optional": False,
                "constraints": {
                    "min_value": 1,
                    "max_value": 536870912,
                },
                "field": "bytes",
            },
            "sha256": {
                "type": {
                    "kind": "primitive",
                    "name": "string",
                },
                "optional": False,
                "constraints": {
                    "min_length": 64,
                    "max_length": 64,
                },
                "field": "sha256",
            },
        },
    },
    "Wavenumber.Geometer.Contracts.StepTopologyA0.StepTopologyAnalyzeRecoveryRequestA0": {
        "kind": "object",
        "properties": {
            "schema": {
                "type": {
                    "kind": "literal",
                    "value_type": "string",
                    "value": "geometry.step_topology.analyze_recovery.request.a0",
                },
                "optional": False,
                "constraints": {},
                "field": "schema",
            },
            "groups": {
                "type": {
                    "kind": "array",
                    "element": {
                        "kind": "reference",
                        "target": "Wavenumber.Geometer.Contracts.StepTopologyA0.RecoveryGroupRequest",
                    },
                },
                "optional": False,
                "constraints": {
                    "min_items": 1,
                    "max_items": 16,
                },
                "field": "groups",
            },
        },
    },
    "Wavenumber.Geometer.Contracts.StepTopologyA0.StepTopologyAnalyzeRecoveryResultA0": {
        "kind": "object",
        "properties": {
            "schema": {
                "type": {
                    "kind": "literal",
                    "value_type": "string",
                    "value": "geometry.step_topology.analyze_recovery.result.a0",
                },
                "optional": False,
                "constraints": {},
                "field": "schema",
            },
            "groups": {
                "type": {
                    "kind": "array",
                    "element": {
                        "kind": "reference",
                        "target": "Wavenumber.Geometer.Contracts.StepTopologyA0.RecoveryGroupResult",
                    },
                },
                "optional": False,
                "constraints": {
                    "max_items": 16,
                },
                "field": "groups",
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
                    "max_items": 256,
                },
                "field": "diagnostics",
            },
        },
    },
    "Wavenumber.Geometer.Contracts.StepTopologyA0.StepTopologyApplyHierarchyRequestA0": {
        "kind": "object",
        "properties": {
            "schema": {
                "type": {
                    "kind": "literal",
                    "value_type": "string",
                    "value": "geometry.step_topology.apply_hierarchy.request.a0",
                },
                "optional": False,
                "constraints": {},
                "field": "schema",
            },
            "session": {
                "type": {
                    "kind": "reference",
                    "target": "Wavenumber.Geometer.Contracts.StepTopologyA0.SessionReference",
                },
                "optional": False,
                "constraints": {},
                "field": "session",
            },
            "expected_hierarchy_revision": {
                "type": {
                    "kind": "primitive",
                    "name": "uint32",
                },
                "optional": False,
                "constraints": {},
                "field": "expected_hierarchy_revision",
            },
            "commands": {
                "type": {
                    "kind": "array",
                    "element": {
                        "kind": "reference",
                        "target": "Wavenumber.Geometer.Contracts.StepTopologyA0.HierarchyCommand",
                    },
                },
                "optional": False,
                "constraints": {
                    "min_items": 1,
                    "max_items": 10000,
                },
                "field": "commands",
            },
        },
    },
    "Wavenumber.Geometer.Contracts.StepTopologyA0.StepTopologyApplyHierarchyResultA0": {
        "kind": "object",
        "properties": {
            "schema": {
                "type": {
                    "kind": "literal",
                    "value_type": "string",
                    "value": "geometry.step_topology.apply_hierarchy.result.a0",
                },
                "optional": False,
                "constraints": {},
                "field": "schema",
            },
            "state": {
                "type": {
                    "kind": "reference",
                    "target": "Wavenumber.Geometer.Contracts.StepTopologyA0.MutationSessionState",
                },
                "optional": False,
                "constraints": {},
                "field": "state",
            },
            "hierarchy": {
                "type": {
                    "kind": "reference",
                    "target": "Wavenumber.Geometer.Contracts.StepTopologyA0.HierarchyState",
                },
                "optional": False,
                "constraints": {},
                "field": "hierarchy",
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
                    "max_items": 256,
                },
                "field": "diagnostics",
            },
        },
    },
    "Wavenumber.Geometer.Contracts.StepTopologyA0.StepTopologyApplyLogicalGroupsRequestA0": {
        "kind": "object",
        "properties": {
            "schema": {
                "type": {
                    "kind": "literal",
                    "value_type": "string",
                    "value": "geometry.step_topology.apply_logical_groups.request.a0",
                },
                "optional": False,
                "constraints": {},
                "field": "schema",
            },
            "session": {
                "type": {
                    "kind": "reference",
                    "target": "Wavenumber.Geometer.Contracts.StepTopologyA0.SessionReference",
                },
                "optional": False,
                "constraints": {},
                "field": "session",
            },
            "commands": {
                "type": {
                    "kind": "array",
                    "element": {
                        "kind": "reference",
                        "target": "Wavenumber.Geometer.Contracts.StepTopologyA0.LogicalGroupCommand",
                    },
                },
                "optional": False,
                "constraints": {
                    "min_items": 1,
                    "max_items": 10000,
                },
                "field": "commands",
            },
        },
    },
    "Wavenumber.Geometer.Contracts.StepTopologyA0.StepTopologyApplyLogicalGroupsResultA0": {
        "kind": "object",
        "properties": {
            "schema": {
                "type": {
                    "kind": "literal",
                    "value_type": "string",
                    "value": "geometry.step_topology.apply_logical_groups.result.a0",
                },
                "optional": False,
                "constraints": {},
                "field": "schema",
            },
            "state": {
                "type": {
                    "kind": "reference",
                    "target": "Wavenumber.Geometer.Contracts.StepTopologyA0.MutationSessionState",
                },
                "optional": False,
                "constraints": {},
                "field": "state",
            },
            "groups": {
                "type": {
                    "kind": "array",
                    "element": {
                        "kind": "reference",
                        "target": "Wavenumber.Geometer.Contracts.StepTopologyA0.LogicalGroup",
                    },
                },
                "optional": False,
                "constraints": {
                    "max_items": 10000,
                },
                "field": "groups",
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
                    "max_items": 256,
                },
                "field": "diagnostics",
            },
        },
    },
    "Wavenumber.Geometer.Contracts.StepTopologyA0.StepTopologyApplyMetadataProbesRequestA0": {
        "kind": "object",
        "properties": {
            "schema": {
                "type": {
                    "kind": "literal",
                    "value_type": "string",
                    "value": "geometry.step_topology.apply_metadata_probes.request.a0",
                },
                "optional": False,
                "constraints": {},
                "field": "schema",
            },
            "session": {
                "type": {
                    "kind": "reference",
                    "target": "Wavenumber.Geometer.Contracts.StepTopologyA0.SessionReference",
                },
                "optional": False,
                "constraints": {},
                "field": "session",
            },
            "commands": {
                "type": {
                    "kind": "array",
                    "element": {
                        "kind": "reference",
                        "target": "Wavenumber.Geometer.Contracts.StepTopologyA0.MetadataProbeCommand",
                    },
                },
                "optional": False,
                "constraints": {
                    "min_items": 1,
                    "max_items": 10000,
                },
                "field": "commands",
            },
        },
    },
    "Wavenumber.Geometer.Contracts.StepTopologyA0.StepTopologyApplyMetadataProbesResultA0": {
        "kind": "object",
        "properties": {
            "schema": {
                "type": {
                    "kind": "literal",
                    "value_type": "string",
                    "value": "geometry.step_topology.apply_metadata_probes.result.a0",
                },
                "optional": False,
                "constraints": {},
                "field": "schema",
            },
            "state": {
                "type": {
                    "kind": "reference",
                    "target": "Wavenumber.Geometer.Contracts.StepTopologyA0.MutationSessionState",
                },
                "optional": False,
                "constraints": {},
                "field": "state",
            },
            "groups": {
                "type": {
                    "kind": "array",
                    "element": {
                        "kind": "reference",
                        "target": "Wavenumber.Geometer.Contracts.StepTopologyA0.LogicalGroup",
                    },
                },
                "optional": False,
                "constraints": {
                    "max_items": 10000,
                },
                "field": "groups",
            },
            "probes": {
                "type": {
                    "kind": "array",
                    "element": {
                        "kind": "reference",
                        "target": "Wavenumber.Geometer.Contracts.StepTopologyA0.MetadataProbe",
                    },
                },
                "optional": False,
                "constraints": {
                    "max_items": 10000,
                },
                "field": "probes",
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
                    "max_items": 256,
                },
                "field": "diagnostics",
            },
        },
    },
    "Wavenumber.Geometer.Contracts.StepTopologyA0.StepTopologyCheckpointEditJournalRequestA0": {
        "kind": "object",
        "properties": {
            "schema": {
                "type": {
                    "kind": "literal",
                    "value_type": "string",
                    "value": "geometry.step_topology.checkpoint_edit_journal.request.a0",
                },
                "optional": False,
                "constraints": {},
                "field": "schema",
            },
            "session": {
                "type": {
                    "kind": "reference",
                    "target": "Wavenumber.Geometer.Contracts.StepTopologyA0.SessionReference",
                },
                "optional": False,
                "constraints": {},
                "field": "session",
            },
        },
    },
    "Wavenumber.Geometer.Contracts.StepTopologyA0.StepTopologyCheckpointEditJournalResultA0": {
        "kind": "object",
        "properties": {
            "schema": {
                "type": {
                    "kind": "literal",
                    "value_type": "string",
                    "value": "geometry.step_topology.checkpoint_edit_journal.result.a0",
                },
                "optional": False,
                "constraints": {},
                "field": "schema",
            },
            "state": {
                "type": {
                    "kind": "reference",
                    "target": "Wavenumber.Geometer.Contracts.StepTopologyA0.MutationSessionState",
                },
                "optional": False,
                "constraints": {},
                "field": "state",
            },
            "source_sha256": {
                "type": {
                    "kind": "primitive",
                    "name": "string",
                },
                "optional": False,
                "constraints": {
                    "min_length": 64,
                    "max_length": 64,
                },
                "field": "source_sha256",
            },
            "source_brep_sha256": {
                "type": {
                    "kind": "primitive",
                    "name": "string",
                },
                "optional": False,
                "constraints": {
                    "min_length": 64,
                    "max_length": 64,
                },
                "field": "source_brep_sha256",
            },
            "target_inventory_sha256": {
                "type": {
                    "kind": "primitive",
                    "name": "string",
                },
                "optional": False,
                "constraints": {
                    "min_length": 64,
                    "max_length": 64,
                },
                "field": "target_inventory_sha256",
            },
            "occt_version": {
                "type": {
                    "kind": "primitive",
                    "name": "string",
                },
                "optional": False,
                "constraints": {
                    "min_length": 1,
                    "max_length": 64,
                },
                "field": "occt_version",
            },
            "transaction_count": {
                "type": {
                    "kind": "primitive",
                    "name": "uint32",
                },
                "optional": False,
                "constraints": {
                    "max_value": 100000,
                },
                "field": "transaction_count",
            },
            "journal": {
                "type": {
                    "kind": "reference",
                    "target": "Wavenumber.Geometer.Contracts.StepTopologyA0.EditJournalAttachmentDescriptor",
                },
                "optional": False,
                "constraints": {},
                "field": "journal",
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
                    "max_items": 256,
                },
                "field": "diagnostics",
            },
        },
    },
    "Wavenumber.Geometer.Contracts.StepTopologyA0.StepTopologyCloseRequestA0": {
        "kind": "object",
        "properties": {
            "schema": {
                "type": {
                    "kind": "literal",
                    "value_type": "string",
                    "value": "geometry.step_topology.close.request.a0",
                },
                "optional": False,
                "constraints": {},
                "field": "schema",
            },
            "session": {
                "type": {
                    "kind": "reference",
                    "target": "Wavenumber.Geometer.Contracts.StepTopologyA0.SessionReference",
                },
                "optional": False,
                "constraints": {},
                "field": "session",
            },
        },
    },
    "Wavenumber.Geometer.Contracts.StepTopologyA0.StepTopologyCloseResultA0": {
        "kind": "object",
        "properties": {
            "schema": {
                "type": {
                    "kind": "literal",
                    "value_type": "string",
                    "value": "geometry.step_topology.close.result.a0",
                },
                "optional": False,
                "constraints": {},
                "field": "schema",
            },
            "session_handle": {
                "type": {
                    "kind": "primitive",
                    "name": "string",
                },
                "optional": False,
                "constraints": {
                    "min_length": 68,
                    "max_length": 68,
                },
                "field": "session_handle",
            },
            "closed": {
                "type": {
                    "kind": "literal",
                    "value_type": "boolean",
                    "value": True,
                },
                "optional": False,
                "constraints": {},
                "field": "closed",
            },
        },
    },
    "Wavenumber.Geometer.Contracts.StepTopologyA0.StepTopologyInspectRequestA0": {
        "kind": "object",
        "properties": {
            "schema": {
                "type": {
                    "kind": "literal",
                    "value_type": "string",
                    "value": "geometry.step_topology.inspect.request.a0",
                },
                "optional": False,
                "constraints": {},
                "field": "schema",
            },
            "session": {
                "type": {
                    "kind": "reference",
                    "target": "Wavenumber.Geometer.Contracts.StepTopologyA0.SessionReference",
                },
                "optional": False,
                "constraints": {},
                "field": "session",
            },
            "page": {
                "type": {
                    "kind": "reference",
                    "target": "Wavenumber.Geometer.Contracts.StepTopologyA0.PageRequest",
                },
                "optional": False,
                "constraints": {},
                "field": "page",
            },
            "include_source_entity_evidence": {
                "type": {
                    "kind": "primitive",
                    "name": "boolean",
                },
                "optional": False,
                "constraints": {},
                "field": "include_source_entity_evidence",
            },
            "include_diagnostics": {
                "type": {
                    "kind": "primitive",
                    "name": "boolean",
                },
                "optional": False,
                "constraints": {},
                "field": "include_diagnostics",
            },
        },
    },
    "Wavenumber.Geometer.Contracts.StepTopologyA0.StepTopologyInspectResultA0": {
        "kind": "object",
        "properties": {
            "schema": {
                "type": {
                    "kind": "literal",
                    "value_type": "string",
                    "value": "geometry.step_topology.inspect.result.a0",
                },
                "optional": False,
                "constraints": {},
                "field": "schema",
            },
            "session": {
                "type": {
                    "kind": "reference",
                    "target": "Wavenumber.Geometer.Contracts.StepTopologyA0.SessionReference",
                },
                "optional": False,
                "constraints": {},
                "field": "session",
            },
            "counts": {
                "type": {
                    "kind": "reference",
                    "target": "Wavenumber.Geometer.Contracts.StepTopologyA0.InspectionCounts",
                },
                "optional": False,
                "constraints": {},
                "field": "counts",
            },
            "page": {
                "type": {
                    "kind": "reference",
                    "target": "Wavenumber.Geometer.Contracts.StepTopologyA0.TopologyPage",
                },
                "optional": False,
                "constraints": {},
                "field": "page",
            },
            "compact_table": {
                "type": {
                    "kind": "reference",
                    "target": "Wavenumber.Geometer.Contracts.StepTopologyA0.TopologyTableAttachmentDescriptor",
                },
                "optional": True,
                "constraints": {},
                "field": "compact_table",
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
                    "max_items": 256,
                },
                "field": "diagnostics",
            },
        },
    },
    "Wavenumber.Geometer.Contracts.StepTopologyA0.StepTopologyOpenRequestA0": {
        "kind": "object",
        "properties": {
            "schema": {
                "type": {
                    "kind": "literal",
                    "value_type": "string",
                    "value": "geometry.step_topology.open.request.a0",
                },
                "optional": False,
                "constraints": {},
                "field": "schema",
            },
        },
    },
    "Wavenumber.Geometer.Contracts.StepTopologyA0.StepTopologyOpenResultA0": {
        "kind": "object",
        "properties": {
            "schema": {
                "type": {
                    "kind": "literal",
                    "value_type": "string",
                    "value": "geometry.step_topology.open.result.a0",
                },
                "optional": False,
                "constraints": {},
                "field": "schema",
            },
            "session": {
                "type": {
                    "kind": "reference",
                    "target": "Wavenumber.Geometer.Contracts.StepTopologyA0.SessionReference",
                },
                "optional": False,
                "constraints": {},
                "field": "session",
            },
            "source": {
                "type": {
                    "kind": "reference",
                    "target": "Wavenumber.Geometer.Contracts.StepTopologyA0.SourceDescriptor",
                },
                "optional": False,
                "constraints": {},
                "field": "source",
            },
            "tool": {
                "type": {
                    "kind": "reference",
                    "target": "Wavenumber.Geometer.Contracts.StepTopologyA0.ToolDescriptor",
                },
                "optional": False,
                "constraints": {},
                "field": "tool",
            },
            "evicted_session_handles": {
                "type": {
                    "kind": "array",
                    "element": {
                        "kind": "primitive",
                        "name": "string",
                    },
                },
                "optional": False,
                "constraints": {
                    "max_items": 8,
                },
                "field": "evicted_session_handles",
            },
        },
    },
    "Wavenumber.Geometer.Contracts.StepTopologyA0.StepTopologyRenderRequestA0": {
        "kind": "object",
        "properties": {
            "schema": {
                "type": {
                    "kind": "literal",
                    "value_type": "string",
                    "value": "geometry.step_topology.render.request.a0",
                },
                "optional": False,
                "constraints": {},
                "field": "schema",
            },
            "session": {
                "type": {
                    "kind": "reference",
                    "target": "Wavenumber.Geometer.Contracts.StepTopologyA0.SessionReference",
                },
                "optional": False,
                "constraints": {},
                "field": "session",
            },
            "tessellation": {
                "type": {
                    "kind": "reference",
                    "target": "Wavenumber.Geometer.Contracts.StepTopologyA0.TessellationOptions",
                },
                "optional": False,
                "constraints": {},
                "field": "tessellation",
            },
        },
    },
    "Wavenumber.Geometer.Contracts.StepTopologyA0.StepTopologyRenderResultA0": {
        "kind": "object",
        "properties": {
            "schema": {
                "type": {
                    "kind": "literal",
                    "value_type": "string",
                    "value": "geometry.step_topology.render.result.a0",
                },
                "optional": False,
                "constraints": {},
                "field": "schema",
            },
            "session": {
                "type": {
                    "kind": "reference",
                    "target": "Wavenumber.Geometer.Contracts.StepTopologyA0.SessionReference",
                },
                "optional": False,
                "constraints": {},
                "field": "session",
            },
            "artifact": {
                "type": {
                    "kind": "reference",
                    "target": "Wavenumber.Geometer.Contracts.StepTopologyA0.RenderArtifactDescriptor",
                },
                "optional": False,
                "constraints": {},
                "field": "artifact",
            },
            "glb": {
                "type": {
                    "kind": "reference",
                    "target": "Wavenumber.Geometer.Contracts.StepTopologyA0.GlbAttachmentDescriptor",
                },
                "optional": False,
                "constraints": {},
                "field": "glb",
            },
            "compact_binding_table": {
                "type": {
                    "kind": "reference",
                    "target": "Wavenumber.Geometer.Contracts.StepTopologyA0.TopologyBindingTableAttachmentDescriptor",
                },
                "optional": True,
                "constraints": {},
                "field": "compact_binding_table",
            },
        },
    },
    "Wavenumber.Geometer.Contracts.StepTopologyA0.StepTopologyResolveHitRequestA0": {
        "kind": "object",
        "properties": {
            "schema": {
                "type": {
                    "kind": "literal",
                    "value_type": "string",
                    "value": "geometry.step_topology.resolve_hit.request.a0",
                },
                "optional": False,
                "constraints": {},
                "field": "schema",
            },
            "session": {
                "type": {
                    "kind": "reference",
                    "target": "Wavenumber.Geometer.Contracts.StepTopologyA0.SessionReference",
                },
                "optional": False,
                "constraints": {},
                "field": "session",
            },
            "artifact_handle": {
                "type": {
                    "kind": "primitive",
                    "name": "string",
                },
                "optional": False,
                "constraints": {
                    "min_length": 68,
                    "max_length": 68,
                },
                "field": "artifact_handle",
            },
            "content_sha256": {
                "type": {
                    "kind": "primitive",
                    "name": "string",
                },
                "optional": False,
                "constraints": {
                    "min_length": 64,
                    "max_length": 64,
                },
                "field": "content_sha256",
            },
            "instance_index": {
                "type": {
                    "kind": "primitive",
                    "name": "uint32",
                },
                "optional": False,
                "constraints": {
                    "max_value": 99999,
                },
                "field": "instance_index",
            },
            "primitive_index": {
                "type": {
                    "kind": "primitive",
                    "name": "uint32",
                },
                "optional": False,
                "constraints": {
                    "max_value": 999999,
                },
                "field": "primitive_index",
            },
            "primitive_triangle_index": {
                "type": {
                    "kind": "primitive",
                    "name": "uint32",
                },
                "optional": False,
                "constraints": {
                    "max_value": 9999999,
                },
                "field": "primitive_triangle_index",
            },
            "occurrence_handle": {
                "type": {
                    "kind": "primitive",
                    "name": "string",
                },
                "optional": False,
                "constraints": {
                    "min_length": 68,
                    "max_length": 68,
                },
                "field": "occurrence_handle",
            },
            "body_handle": {
                "type": {
                    "kind": "primitive",
                    "name": "string",
                },
                "optional": False,
                "constraints": {
                    "min_length": 68,
                    "max_length": 68,
                },
                "field": "body_handle",
            },
            "face_handle": {
                "type": {
                    "kind": "primitive",
                    "name": "string",
                },
                "optional": False,
                "constraints": {
                    "min_length": 68,
                    "max_length": 68,
                },
                "field": "face_handle",
            },
        },
    },
    "Wavenumber.Geometer.Contracts.StepTopologyA0.StepTopologyResolveHitResultA0": {
        "kind": "object",
        "properties": {
            "schema": {
                "type": {
                    "kind": "literal",
                    "value_type": "string",
                    "value": "geometry.step_topology.resolve_hit.result.a0",
                },
                "optional": False,
                "constraints": {},
                "field": "schema",
            },
            "session": {
                "type": {
                    "kind": "reference",
                    "target": "Wavenumber.Geometer.Contracts.StepTopologyA0.SessionReference",
                },
                "optional": False,
                "constraints": {},
                "field": "session",
            },
            "instance_index": {
                "type": {
                    "kind": "primitive",
                    "name": "uint32",
                },
                "optional": False,
                "constraints": {
                    "max_value": 99999,
                },
                "field": "instance_index",
            },
            "primitive_index": {
                "type": {
                    "kind": "primitive",
                    "name": "uint32",
                },
                "optional": False,
                "constraints": {
                    "max_value": 999999,
                },
                "field": "primitive_index",
            },
            "triangle_index": {
                "type": {
                    "kind": "primitive",
                    "name": "uint32",
                },
                "optional": False,
                "constraints": {
                    "max_value": 9999999,
                },
                "field": "triangle_index",
            },
            "occurrence_handle": {
                "type": {
                    "kind": "primitive",
                    "name": "string",
                },
                "optional": False,
                "constraints": {
                    "min_length": 68,
                    "max_length": 68,
                },
                "field": "occurrence_handle",
            },
            "body_handle": {
                "type": {
                    "kind": "primitive",
                    "name": "string",
                },
                "optional": False,
                "constraints": {
                    "min_length": 68,
                    "max_length": 68,
                },
                "field": "body_handle",
            },
            "face_handle": {
                "type": {
                    "kind": "primitive",
                    "name": "string",
                },
                "optional": False,
                "constraints": {
                    "min_length": 68,
                    "max_length": 68,
                },
                "field": "face_handle",
            },
        },
    },
    "Wavenumber.Geometer.Contracts.StepTopologyA0.StepTopologyRestoreRequestA0": {
        "kind": "object",
        "properties": {
            "schema": {
                "type": {
                    "kind": "literal",
                    "value_type": "string",
                    "value": "geometry.step_topology.restore.request.a0",
                },
                "optional": False,
                "constraints": {},
                "field": "schema",
            },
            "source": {
                "type": {
                    "kind": "reference",
                    "target": "Wavenumber.Geometer.Contracts.StepTopologyA0.SourceDescriptor",
                },
                "optional": False,
                "constraints": {},
                "field": "source",
            },
            "state_artifact": {
                "type": {
                    "kind": "reference",
                    "target": "Wavenumber.Geometer.Contracts.StepTopologyA0.RestoreStateArtifact",
                },
                "optional": False,
                "constraints": {},
                "field": "state_artifact",
            },
            "replay_preconditions": {
                "type": {
                    "kind": "reference",
                    "target": "Wavenumber.Geometer.Contracts.StepTopologyA0.EditJournalReplayPreconditions",
                },
                "optional": True,
                "constraints": {},
                "field": "replay_preconditions",
            },
            "include_diagnostics": {
                "type": {
                    "kind": "primitive",
                    "name": "boolean",
                },
                "optional": False,
                "constraints": {},
                "field": "include_diagnostics",
            },
        },
    },
    "Wavenumber.Geometer.Contracts.StepTopologyA0.StepTopologyRestoreResultA0": {
        "kind": "object",
        "properties": {
            "schema": {
                "type": {
                    "kind": "literal",
                    "value_type": "string",
                    "value": "geometry.step_topology.restore.result.a0",
                },
                "optional": False,
                "constraints": {},
                "field": "schema",
            },
            "session": {
                "type": {
                    "kind": "reference",
                    "target": "Wavenumber.Geometer.Contracts.StepTopologyA0.SessionReference",
                },
                "optional": False,
                "constraints": {},
                "field": "session",
            },
            "source": {
                "type": {
                    "kind": "reference",
                    "target": "Wavenumber.Geometer.Contracts.StepTopologyA0.SourceDescriptor",
                },
                "optional": False,
                "constraints": {},
                "field": "source",
            },
            "tool": {
                "type": {
                    "kind": "reference",
                    "target": "Wavenumber.Geometer.Contracts.StepTopologyA0.ToolDescriptor",
                },
                "optional": False,
                "constraints": {},
                "field": "tool",
            },
            "replayed_transaction_count": {
                "type": {
                    "kind": "primitive",
                    "name": "uint32",
                },
                "optional": False,
                "constraints": {
                    "max_value": 100000,
                },
                "field": "replayed_transaction_count",
            },
            "evicted_session_handles": {
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
                "field": "evicted_session_handles",
            },
            "recovery": {
                "type": {
                    "kind": "array",
                    "element": {
                        "kind": "reference",
                        "target": "Wavenumber.Geometer.Contracts.StepTopologyA0.RecoveryGroupResult",
                    },
                },
                "optional": False,
                "constraints": {
                    "max_items": 16,
                },
                "field": "recovery",
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
                    "max_items": 256,
                },
                "field": "diagnostics",
            },
        },
    },
    "Wavenumber.Geometer.Contracts.StepTopologyA0.StepTopologySaveRequestA0": {
        "kind": "object",
        "properties": {
            "schema": {
                "type": {
                    "kind": "literal",
                    "value_type": "string",
                    "value": "geometry.step_topology.save.request.a0",
                },
                "optional": False,
                "constraints": {},
                "field": "schema",
            },
            "session": {
                "type": {
                    "kind": "reference",
                    "target": "Wavenumber.Geometer.Contracts.StepTopologyA0.SessionReference",
                },
                "optional": False,
                "constraints": {},
                "field": "session",
            },
            "carrier": {
                "type": {
                    "kind": "reference",
                    "target": "Wavenumber.Geometer.Contracts.StepTopologyA0.SaveCarrier",
                },
                "optional": False,
                "constraints": {},
                "field": "carrier",
            },
            "include_diagnostics": {
                "type": {
                    "kind": "primitive",
                    "name": "boolean",
                },
                "optional": False,
                "constraints": {},
                "field": "include_diagnostics",
            },
        },
    },
    "Wavenumber.Geometer.Contracts.StepTopologyA0.StepTopologySaveResultA0": {
        "kind": "object",
        "properties": {
            "schema": {
                "type": {
                    "kind": "literal",
                    "value_type": "string",
                    "value": "geometry.step_topology.save.result.a0",
                },
                "optional": False,
                "constraints": {},
                "field": "schema",
            },
            "state": {
                "type": {
                    "kind": "reference",
                    "target": "Wavenumber.Geometer.Contracts.StepTopologyA0.MutationSessionState",
                },
                "optional": False,
                "constraints": {},
                "field": "state",
            },
            "source_sha256": {
                "type": {
                    "kind": "primitive",
                    "name": "string",
                },
                "optional": False,
                "constraints": {
                    "min_length": 64,
                    "max_length": 64,
                },
                "field": "source_sha256",
            },
            "artifact": {
                "type": {
                    "kind": "reference",
                    "target": "Wavenumber.Geometer.Contracts.StepTopologyA0.SavePersistenceArtifact",
                },
                "optional": False,
                "constraints": {},
                "field": "artifact",
            },
            "capabilities": {
                "type": {
                    "kind": "array",
                    "element": {
                        "kind": "reference",
                        "target": "Wavenumber.Geometer.Contracts.StepTopologyA0.CarrierCapability",
                    },
                },
                "optional": False,
                "constraints": {
                    "min_items": 5,
                    "max_items": 5,
                },
                "field": "capabilities",
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
                    "max_items": 256,
                },
                "field": "diagnostics",
            },
        },
    },
    "Wavenumber.Geometer.Contracts.StepTopologyA0.TessellationOptions": {
        "kind": "object",
        "properties": {
            "linear_deflection_mm": {
                "type": {
                    "kind": "primitive",
                    "name": "float64",
                },
                "optional": False,
                "constraints": {
                    "min_value": 0.000001,
                    "max_value": 1000,
                },
                "field": "linear_deflection_mm",
            },
            "angular_deflection_rad": {
                "type": {
                    "kind": "primitive",
                    "name": "float64",
                },
                "optional": False,
                "constraints": {
                    "min_value": 0.000001,
                    "max_value": 3.141592653589793,
                },
                "field": "angular_deflection_rad",
            },
            "relative": {
                "type": {
                    "kind": "primitive",
                    "name": "boolean",
                },
                "optional": False,
                "constraints": {},
                "field": "relative",
            },
            "parallel": {
                "type": {
                    "kind": "primitive",
                    "name": "boolean",
                },
                "optional": False,
                "constraints": {},
                "field": "parallel",
            },
            "source_to_render": {
                "type": {
                    "kind": "array",
                    "element": {
                        "kind": "primitive",
                        "name": "float64",
                    },
                },
                "optional": False,
                "constraints": {
                    "min_items": 12,
                    "max_items": 12,
                },
                "field": "source_to_render",
            },
        },
    },
    "Wavenumber.Geometer.Contracts.StepTopologyA0.ToolDescriptor": {
        "kind": "object",
        "properties": {
            "name": {
                "type": {
                    "kind": "literal",
                    "value_type": "string",
                    "value": "geometer",
                },
                "optional": False,
                "constraints": {},
                "field": "name",
            },
            "release_version": {
                "type": {
                    "kind": "primitive",
                    "name": "string",
                },
                "optional": False,
                "constraints": {
                    "min_length": 1,
                    "max_length": 64,
                },
                "field": "release_version",
            },
            "occt_version": {
                "type": {
                    "kind": "primitive",
                    "name": "string",
                },
                "optional": False,
                "constraints": {
                    "min_length": 1,
                    "max_length": 64,
                },
                "field": "occt_version",
            },
        },
    },
    "Wavenumber.Geometer.Contracts.StepTopologyA0.TopologyBindingTableAttachmentDescriptor": {
        "kind": "object",
        "properties": {
            "name": {
                "type": {
                    "kind": "literal",
                    "value_type": "string",
                    "value": "topology_binding_table",
                },
                "optional": False,
                "constraints": {},
                "field": "name",
            },
            "media_type": {
                "type": {
                    "kind": "literal",
                    "value_type": "string",
                    "value": "application/vnd.wavenumber.geometer.step-topology-binding-table",
                },
                "optional": False,
                "constraints": {},
                "field": "media_type",
            },
            "format": {
                "type": {
                    "kind": "literal",
                    "value_type": "string",
                    "value": "wn.geometer.step-topology-binding-table.a0",
                },
                "optional": False,
                "constraints": {},
                "field": "format",
            },
            "bytes": {
                "type": {
                    "kind": "primitive",
                    "name": "uint32",
                },
                "optional": False,
                "constraints": {
                    "min_value": 1,
                    "max_value": 134217728,
                },
                "field": "bytes",
            },
            "sha256": {
                "type": {
                    "kind": "primitive",
                    "name": "string",
                },
                "optional": False,
                "constraints": {
                    "min_length": 64,
                    "max_length": 64,
                },
                "field": "sha256",
            },
        },
    },
    "Wavenumber.Geometer.Contracts.StepTopologyA0.TopologyMembership": {
        "kind": "object",
        "properties": {
            "kind": {
                "type": {
                    "kind": "reference",
                    "target": "Wavenumber.Geometer.Contracts.StepTopologyA0.TopologyMembershipKind",
                },
                "optional": False,
                "constraints": {},
                "field": "kind",
            },
            "owner_handle": {
                "type": {
                    "kind": "primitive",
                    "name": "string",
                },
                "optional": False,
                "constraints": {
                    "min_length": 68,
                    "max_length": 68,
                },
                "field": "owner_handle",
            },
            "member_handle": {
                "type": {
                    "kind": "primitive",
                    "name": "string",
                },
                "optional": False,
                "constraints": {
                    "min_length": 68,
                    "max_length": 68,
                },
                "field": "member_handle",
            },
        },
    },
    "Wavenumber.Geometer.Contracts.StepTopologyA0.TopologyMembershipKind": {
        "kind": "enum",
        "values": ["body_shell", "body_face", "shell_face"],
    },
    "Wavenumber.Geometer.Contracts.StepTopologyA0.TopologyPage": {
        "kind": "object",
        "properties": {
            "definitions": {
                "type": {
                    "kind": "array",
                    "element": {
                        "kind": "reference",
                        "target": "Wavenumber.Geometer.Contracts.StepTopologyA0.DefinitionSummary",
                    },
                },
                "optional": False,
                "constraints": {
                    "max_items": 1024,
                },
                "field": "definitions",
            },
            "occurrences": {
                "type": {
                    "kind": "array",
                    "element": {
                        "kind": "reference",
                        "target": "Wavenumber.Geometer.Contracts.StepTopologyA0.OccurrenceSummary",
                    },
                },
                "optional": False,
                "constraints": {
                    "max_items": 1024,
                },
                "field": "occurrences",
            },
            "bodies": {
                "type": {
                    "kind": "array",
                    "element": {
                        "kind": "reference",
                        "target": "Wavenumber.Geometer.Contracts.StepTopologyA0.BodySummary",
                    },
                },
                "optional": False,
                "constraints": {
                    "max_items": 1024,
                },
                "field": "bodies",
            },
            "shells": {
                "type": {
                    "kind": "array",
                    "element": {
                        "kind": "reference",
                        "target": "Wavenumber.Geometer.Contracts.StepTopologyA0.ShellSummary",
                    },
                },
                "optional": False,
                "constraints": {
                    "max_items": 1024,
                },
                "field": "shells",
            },
            "faces": {
                "type": {
                    "kind": "array",
                    "element": {
                        "kind": "reference",
                        "target": "Wavenumber.Geometer.Contracts.StepTopologyA0.FaceSummary",
                    },
                },
                "optional": False,
                "constraints": {
                    "max_items": 1024,
                },
                "field": "faces",
            },
            "memberships": {
                "type": {
                    "kind": "array",
                    "element": {
                        "kind": "reference",
                        "target": "Wavenumber.Geometer.Contracts.StepTopologyA0.TopologyMembership",
                    },
                },
                "optional": False,
                "constraints": {
                    "max_items": 1024,
                },
                "field": "memberships",
            },
            "next_cursor": {
                "type": {
                    "kind": "primitive",
                    "name": "string",
                },
                "optional": True,
                "constraints": {
                    "max_length": 256,
                },
                "field": "next_cursor",
            },
        },
    },
    "Wavenumber.Geometer.Contracts.StepTopologyA0.TopologyTableAttachmentDescriptor": {
        "kind": "object",
        "properties": {
            "name": {
                "type": {
                    "kind": "literal",
                    "value_type": "string",
                    "value": "topology_table",
                },
                "optional": False,
                "constraints": {},
                "field": "name",
            },
            "media_type": {
                "type": {
                    "kind": "literal",
                    "value_type": "string",
                    "value": "application/vnd.wavenumber.geometer.step-topology-table",
                },
                "optional": False,
                "constraints": {},
                "field": "media_type",
            },
            "format": {
                "type": {
                    "kind": "literal",
                    "value_type": "string",
                    "value": "wn.geometer.step-topology-table.a0",
                },
                "optional": False,
                "constraints": {},
                "field": "format",
            },
            "bytes": {
                "type": {
                    "kind": "primitive",
                    "name": "uint32",
                },
                "optional": False,
                "constraints": {
                    "min_value": 1,
                    "max_value": 134217728,
                },
                "field": "bytes",
            },
            "sha256": {
                "type": {
                    "kind": "primitive",
                    "name": "string",
                },
                "optional": False,
                "constraints": {
                    "min_length": 64,
                    "max_length": 64,
                },
                "field": "sha256",
            },
        },
    },
    "Wavenumber.Geometer.Contracts.StepTopologyA0.XbfPersistenceArtifact": {
        "kind": "object",
        "properties": {
            "carrier": {
                "type": {
                    "kind": "literal",
                    "value_type": "string",
                    "value": "xbf",
                },
                "optional": False,
                "constraints": {},
                "field": "carrier",
            },
            "name": {
                "type": {
                    "kind": "literal",
                    "value_type": "string",
                    "value": "state_artifact",
                },
                "optional": False,
                "constraints": {},
                "field": "name",
            },
            "media_type": {
                "type": {
                    "kind": "literal",
                    "value_type": "string",
                    "value": "application/vnd.opencascade.xbf",
                },
                "optional": False,
                "constraints": {},
                "field": "media_type",
            },
            "format": {
                "type": {
                    "kind": "literal",
                    "value_type": "string",
                    "value": "ocaf-xbf-version-12",
                },
                "optional": False,
                "constraints": {},
                "field": "format",
            },
            "bytes": {
                "type": {
                    "kind": "primitive",
                    "name": "uint32",
                },
                "optional": False,
                "constraints": {
                    "min_value": 1,
                    "max_value": 536870912,
                },
                "field": "bytes",
            },
            "sha256": {
                "type": {
                    "kind": "primitive",
                    "name": "string",
                },
                "optional": False,
                "constraints": {
                    "min_length": 64,
                    "max_length": 64,
                },
                "field": "sha256",
            },
        },
    },
    "Wavenumber.Geometer.Contracts.StepTopologyA0.XmlXcafPersistenceArtifact": {
        "kind": "object",
        "properties": {
            "carrier": {
                "type": {
                    "kind": "literal",
                    "value_type": "string",
                    "value": "xml_xcaf",
                },
                "optional": False,
                "constraints": {},
                "field": "carrier",
            },
            "name": {
                "type": {
                    "kind": "literal",
                    "value_type": "string",
                    "value": "state_artifact",
                },
                "optional": False,
                "constraints": {},
                "field": "name",
            },
            "media_type": {
                "type": {
                    "kind": "literal",
                    "value_type": "string",
                    "value": "application/vnd.opencascade.xml-xcaf",
                },
                "optional": False,
                "constraints": {},
                "field": "media_type",
            },
            "format": {
                "type": {
                    "kind": "literal",
                    "value_type": "string",
                    "value": "ocaf-xml-xcaf-version-12",
                },
                "optional": False,
                "constraints": {},
                "field": "format",
            },
            "bytes": {
                "type": {
                    "kind": "primitive",
                    "name": "uint32",
                },
                "optional": False,
                "constraints": {
                    "min_value": 1,
                    "max_value": 536870912,
                },
                "field": "bytes",
            },
            "sha256": {
                "type": {
                    "kind": "primitive",
                    "name": "string",
                },
                "optional": False,
                "constraints": {
                    "min_length": 64,
                    "max_length": 64,
                },
                "field": "sha256",
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


def decode_hlr_projection_options_a0_json(data: str | bytes | bytearray | memoryview) -> HlrProjectionOptionsA0:
    return cast(
        HlrProjectionOptionsA0,
        decode_contract_json(
            data,
            "Wavenumber.Geometer.Contracts.HlrProjectionA0.HlrProjectionOptionsA0",
            DECLARATIONS,
            MODEL_TYPES,
            ENUM_TYPES,
        ),
    )


def encode_hlr_projection_options_a0_json(value: HlrProjectionOptionsA0) -> bytes:
    return encode_contract_json(
        value,
        "Wavenumber.Geometer.Contracts.HlrProjectionA0.HlrProjectionOptionsA0",
        DECLARATIONS,
        MODEL_TYPES,
        ENUM_TYPES,
    )


def decode_hlr_projection_result_a0_json(data: str | bytes | bytearray | memoryview) -> HlrProjectionResultA0:
    return cast(
        HlrProjectionResultA0,
        decode_contract_json(
            data,
            "Wavenumber.Geometer.Contracts.HlrProjectionA0.HlrProjectionResultA0",
            DECLARATIONS,
            MODEL_TYPES,
            ENUM_TYPES,
        ),
    )


def encode_hlr_projection_result_a0_json(value: HlrProjectionResultA0) -> bytes:
    return encode_contract_json(
        value,
        "Wavenumber.Geometer.Contracts.HlrProjectionA0.HlrProjectionResultA0",
        DECLARATIONS,
        MODEL_TYPES,
        ENUM_TYPES,
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


def decode_mesh_illustration_input_a0_json(data: str | bytes | bytearray | memoryview) -> MeshIllustrationInputA0:
    return cast(
        MeshIllustrationInputA0,
        decode_contract_json(
            data,
            "Wavenumber.Geometer.Contracts.MeshIllustrationA0.MeshIllustrationInputA0",
            DECLARATIONS,
            MODEL_TYPES,
            ENUM_TYPES,
        ),
    )


def encode_mesh_illustration_input_a0_json(value: MeshIllustrationInputA0) -> bytes:
    return encode_contract_json(
        value,
        "Wavenumber.Geometer.Contracts.MeshIllustrationA0.MeshIllustrationInputA0",
        DECLARATIONS,
        MODEL_TYPES,
        ENUM_TYPES,
    )


def decode_mesh_illustration_result_a0_json(data: str | bytes | bytearray | memoryview) -> MeshIllustrationResultA0:
    return cast(
        MeshIllustrationResultA0,
        decode_contract_json(
            data,
            "Wavenumber.Geometer.Contracts.MeshIllustrationA0.MeshIllustrationResultA0",
            DECLARATIONS,
            MODEL_TYPES,
            ENUM_TYPES,
        ),
    )


def encode_mesh_illustration_result_a0_json(value: MeshIllustrationResultA0) -> bytes:
    return encode_contract_json(
        value,
        "Wavenumber.Geometer.Contracts.MeshIllustrationA0.MeshIllustrationResultA0",
        DECLARATIONS,
        MODEL_TYPES,
        ENUM_TYPES,
    )


def decode_mesh_illustration_style_a0_json(data: str | bytes | bytearray | memoryview) -> MeshIllustrationStyleA0:
    return cast(
        MeshIllustrationStyleA0,
        decode_contract_json(
            data,
            "Wavenumber.Geometer.Contracts.MeshIllustrationA0.MeshIllustrationStyleA0",
            DECLARATIONS,
            MODEL_TYPES,
            ENUM_TYPES,
        ),
    )


def encode_mesh_illustration_style_a0_json(value: MeshIllustrationStyleA0) -> bytes:
    return encode_contract_json(
        value,
        "Wavenumber.Geometer.Contracts.MeshIllustrationA0.MeshIllustrationStyleA0",
        DECLARATIONS,
        MODEL_TYPES,
        ENUM_TYPES,
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


def decode_mesh_collection_a0_json(data: str | bytes | bytearray | memoryview) -> MeshCollectionA0:
    return cast(
        MeshCollectionA0,
        decode_contract_json(
            data,
            "Wavenumber.Geometer.Contracts.ModelTessellationA0.MeshCollectionA0",
            DECLARATIONS,
            MODEL_TYPES,
            ENUM_TYPES,
        ),
    )


def encode_mesh_collection_a0_json(value: MeshCollectionA0) -> bytes:
    return encode_contract_json(
        value,
        "Wavenumber.Geometer.Contracts.ModelTessellationA0.MeshCollectionA0",
        DECLARATIONS,
        MODEL_TYPES,
        ENUM_TYPES,
    )


def decode_model_tessellation_request_a0_json(data: str | bytes | bytearray | memoryview) -> ModelTessellationRequestA0:
    return cast(
        ModelTessellationRequestA0,
        decode_contract_json(
            data,
            "Wavenumber.Geometer.Contracts.ModelTessellationA0.ModelTessellationRequestA0",
            DECLARATIONS,
            MODEL_TYPES,
            ENUM_TYPES,
        ),
    )


def encode_model_tessellation_request_a0_json(value: ModelTessellationRequestA0) -> bytes:
    return encode_contract_json(
        value,
        "Wavenumber.Geometer.Contracts.ModelTessellationA0.ModelTessellationRequestA0",
        DECLARATIONS,
        MODEL_TYPES,
        ENUM_TYPES,
    )


def decode_model_tessellation_result_a0_json(data: str | bytes | bytearray | memoryview) -> ModelTessellationResultA0:
    return cast(
        ModelTessellationResultA0,
        decode_contract_json(
            data,
            "Wavenumber.Geometer.Contracts.ModelTessellationA0.ModelTessellationResultA0",
            DECLARATIONS,
            MODEL_TYPES,
            ENUM_TYPES,
        ),
    )


def encode_model_tessellation_result_a0_json(value: ModelTessellationResultA0) -> bytes:
    return encode_contract_json(
        value,
        "Wavenumber.Geometer.Contracts.ModelTessellationA0.ModelTessellationResultA0",
        DECLARATIONS,
        MODEL_TYPES,
        ENUM_TYPES,
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


def decode_step_topology_analyze_recovery_request_a0_json(
    data: str | bytes | bytearray | memoryview,
) -> StepTopologyAnalyzeRecoveryRequestA0:
    return cast(
        StepTopologyAnalyzeRecoveryRequestA0,
        decode_contract_json(
            data,
            "Wavenumber.Geometer.Contracts.StepTopologyA0.StepTopologyAnalyzeRecoveryRequestA0",
            DECLARATIONS,
            MODEL_TYPES,
            ENUM_TYPES,
        ),
    )


def encode_step_topology_analyze_recovery_request_a0_json(value: StepTopologyAnalyzeRecoveryRequestA0) -> bytes:
    return encode_contract_json(
        value,
        "Wavenumber.Geometer.Contracts.StepTopologyA0.StepTopologyAnalyzeRecoveryRequestA0",
        DECLARATIONS,
        MODEL_TYPES,
        ENUM_TYPES,
    )


def decode_step_topology_analyze_recovery_result_a0_json(
    data: str | bytes | bytearray | memoryview,
) -> StepTopologyAnalyzeRecoveryResultA0:
    return cast(
        StepTopologyAnalyzeRecoveryResultA0,
        decode_contract_json(
            data,
            "Wavenumber.Geometer.Contracts.StepTopologyA0.StepTopologyAnalyzeRecoveryResultA0",
            DECLARATIONS,
            MODEL_TYPES,
            ENUM_TYPES,
        ),
    )


def encode_step_topology_analyze_recovery_result_a0_json(value: StepTopologyAnalyzeRecoveryResultA0) -> bytes:
    return encode_contract_json(
        value,
        "Wavenumber.Geometer.Contracts.StepTopologyA0.StepTopologyAnalyzeRecoveryResultA0",
        DECLARATIONS,
        MODEL_TYPES,
        ENUM_TYPES,
    )


def decode_step_topology_apply_hierarchy_request_a0_json(
    data: str | bytes | bytearray | memoryview,
) -> StepTopologyApplyHierarchyRequestA0:
    return cast(
        StepTopologyApplyHierarchyRequestA0,
        decode_contract_json(
            data,
            "Wavenumber.Geometer.Contracts.StepTopologyA0.StepTopologyApplyHierarchyRequestA0",
            DECLARATIONS,
            MODEL_TYPES,
            ENUM_TYPES,
        ),
    )


def encode_step_topology_apply_hierarchy_request_a0_json(value: StepTopologyApplyHierarchyRequestA0) -> bytes:
    return encode_contract_json(
        value,
        "Wavenumber.Geometer.Contracts.StepTopologyA0.StepTopologyApplyHierarchyRequestA0",
        DECLARATIONS,
        MODEL_TYPES,
        ENUM_TYPES,
    )


def decode_step_topology_apply_hierarchy_result_a0_json(
    data: str | bytes | bytearray | memoryview,
) -> StepTopologyApplyHierarchyResultA0:
    return cast(
        StepTopologyApplyHierarchyResultA0,
        decode_contract_json(
            data,
            "Wavenumber.Geometer.Contracts.StepTopologyA0.StepTopologyApplyHierarchyResultA0",
            DECLARATIONS,
            MODEL_TYPES,
            ENUM_TYPES,
        ),
    )


def encode_step_topology_apply_hierarchy_result_a0_json(value: StepTopologyApplyHierarchyResultA0) -> bytes:
    return encode_contract_json(
        value,
        "Wavenumber.Geometer.Contracts.StepTopologyA0.StepTopologyApplyHierarchyResultA0",
        DECLARATIONS,
        MODEL_TYPES,
        ENUM_TYPES,
    )


def decode_step_topology_apply_logical_groups_request_a0_json(
    data: str | bytes | bytearray | memoryview,
) -> StepTopologyApplyLogicalGroupsRequestA0:
    return cast(
        StepTopologyApplyLogicalGroupsRequestA0,
        decode_contract_json(
            data,
            "Wavenumber.Geometer.Contracts.StepTopologyA0.StepTopologyApplyLogicalGroupsRequestA0",
            DECLARATIONS,
            MODEL_TYPES,
            ENUM_TYPES,
        ),
    )


def encode_step_topology_apply_logical_groups_request_a0_json(value: StepTopologyApplyLogicalGroupsRequestA0) -> bytes:
    return encode_contract_json(
        value,
        "Wavenumber.Geometer.Contracts.StepTopologyA0.StepTopologyApplyLogicalGroupsRequestA0",
        DECLARATIONS,
        MODEL_TYPES,
        ENUM_TYPES,
    )


def decode_step_topology_apply_logical_groups_result_a0_json(
    data: str | bytes | bytearray | memoryview,
) -> StepTopologyApplyLogicalGroupsResultA0:
    return cast(
        StepTopologyApplyLogicalGroupsResultA0,
        decode_contract_json(
            data,
            "Wavenumber.Geometer.Contracts.StepTopologyA0.StepTopologyApplyLogicalGroupsResultA0",
            DECLARATIONS,
            MODEL_TYPES,
            ENUM_TYPES,
        ),
    )


def encode_step_topology_apply_logical_groups_result_a0_json(value: StepTopologyApplyLogicalGroupsResultA0) -> bytes:
    return encode_contract_json(
        value,
        "Wavenumber.Geometer.Contracts.StepTopologyA0.StepTopologyApplyLogicalGroupsResultA0",
        DECLARATIONS,
        MODEL_TYPES,
        ENUM_TYPES,
    )


def decode_step_topology_apply_metadata_probes_request_a0_json(
    data: str | bytes | bytearray | memoryview,
) -> StepTopologyApplyMetadataProbesRequestA0:
    return cast(
        StepTopologyApplyMetadataProbesRequestA0,
        decode_contract_json(
            data,
            "Wavenumber.Geometer.Contracts.StepTopologyA0.StepTopologyApplyMetadataProbesRequestA0",
            DECLARATIONS,
            MODEL_TYPES,
            ENUM_TYPES,
        ),
    )


def encode_step_topology_apply_metadata_probes_request_a0_json(
    value: StepTopologyApplyMetadataProbesRequestA0,
) -> bytes:
    return encode_contract_json(
        value,
        "Wavenumber.Geometer.Contracts.StepTopologyA0.StepTopologyApplyMetadataProbesRequestA0",
        DECLARATIONS,
        MODEL_TYPES,
        ENUM_TYPES,
    )


def decode_step_topology_apply_metadata_probes_result_a0_json(
    data: str | bytes | bytearray | memoryview,
) -> StepTopologyApplyMetadataProbesResultA0:
    return cast(
        StepTopologyApplyMetadataProbesResultA0,
        decode_contract_json(
            data,
            "Wavenumber.Geometer.Contracts.StepTopologyA0.StepTopologyApplyMetadataProbesResultA0",
            DECLARATIONS,
            MODEL_TYPES,
            ENUM_TYPES,
        ),
    )


def encode_step_topology_apply_metadata_probes_result_a0_json(value: StepTopologyApplyMetadataProbesResultA0) -> bytes:
    return encode_contract_json(
        value,
        "Wavenumber.Geometer.Contracts.StepTopologyA0.StepTopologyApplyMetadataProbesResultA0",
        DECLARATIONS,
        MODEL_TYPES,
        ENUM_TYPES,
    )


def decode_step_topology_checkpoint_edit_journal_request_a0_json(
    data: str | bytes | bytearray | memoryview,
) -> StepTopologyCheckpointEditJournalRequestA0:
    return cast(
        StepTopologyCheckpointEditJournalRequestA0,
        decode_contract_json(
            data,
            "Wavenumber.Geometer.Contracts.StepTopologyA0.StepTopologyCheckpointEditJournalRequestA0",
            DECLARATIONS,
            MODEL_TYPES,
            ENUM_TYPES,
        ),
    )


def encode_step_topology_checkpoint_edit_journal_request_a0_json(
    value: StepTopologyCheckpointEditJournalRequestA0,
) -> bytes:
    return encode_contract_json(
        value,
        "Wavenumber.Geometer.Contracts.StepTopologyA0.StepTopologyCheckpointEditJournalRequestA0",
        DECLARATIONS,
        MODEL_TYPES,
        ENUM_TYPES,
    )


def decode_step_topology_checkpoint_edit_journal_result_a0_json(
    data: str | bytes | bytearray | memoryview,
) -> StepTopologyCheckpointEditJournalResultA0:
    return cast(
        StepTopologyCheckpointEditJournalResultA0,
        decode_contract_json(
            data,
            "Wavenumber.Geometer.Contracts.StepTopologyA0.StepTopologyCheckpointEditJournalResultA0",
            DECLARATIONS,
            MODEL_TYPES,
            ENUM_TYPES,
        ),
    )


def encode_step_topology_checkpoint_edit_journal_result_a0_json(
    value: StepTopologyCheckpointEditJournalResultA0,
) -> bytes:
    return encode_contract_json(
        value,
        "Wavenumber.Geometer.Contracts.StepTopologyA0.StepTopologyCheckpointEditJournalResultA0",
        DECLARATIONS,
        MODEL_TYPES,
        ENUM_TYPES,
    )


def decode_step_topology_close_request_a0_json(
    data: str | bytes | bytearray | memoryview,
) -> StepTopologyCloseRequestA0:
    return cast(
        StepTopologyCloseRequestA0,
        decode_contract_json(
            data,
            "Wavenumber.Geometer.Contracts.StepTopologyA0.StepTopologyCloseRequestA0",
            DECLARATIONS,
            MODEL_TYPES,
            ENUM_TYPES,
        ),
    )


def encode_step_topology_close_request_a0_json(value: StepTopologyCloseRequestA0) -> bytes:
    return encode_contract_json(
        value,
        "Wavenumber.Geometer.Contracts.StepTopologyA0.StepTopologyCloseRequestA0",
        DECLARATIONS,
        MODEL_TYPES,
        ENUM_TYPES,
    )


def decode_step_topology_close_result_a0_json(data: str | bytes | bytearray | memoryview) -> StepTopologyCloseResultA0:
    return cast(
        StepTopologyCloseResultA0,
        decode_contract_json(
            data,
            "Wavenumber.Geometer.Contracts.StepTopologyA0.StepTopologyCloseResultA0",
            DECLARATIONS,
            MODEL_TYPES,
            ENUM_TYPES,
        ),
    )


def encode_step_topology_close_result_a0_json(value: StepTopologyCloseResultA0) -> bytes:
    return encode_contract_json(
        value,
        "Wavenumber.Geometer.Contracts.StepTopologyA0.StepTopologyCloseResultA0",
        DECLARATIONS,
        MODEL_TYPES,
        ENUM_TYPES,
    )


def decode_step_topology_inspect_request_a0_json(
    data: str | bytes | bytearray | memoryview,
) -> StepTopologyInspectRequestA0:
    return cast(
        StepTopologyInspectRequestA0,
        decode_contract_json(
            data,
            "Wavenumber.Geometer.Contracts.StepTopologyA0.StepTopologyInspectRequestA0",
            DECLARATIONS,
            MODEL_TYPES,
            ENUM_TYPES,
        ),
    )


def encode_step_topology_inspect_request_a0_json(value: StepTopologyInspectRequestA0) -> bytes:
    return encode_contract_json(
        value,
        "Wavenumber.Geometer.Contracts.StepTopologyA0.StepTopologyInspectRequestA0",
        DECLARATIONS,
        MODEL_TYPES,
        ENUM_TYPES,
    )


def decode_step_topology_inspect_result_a0_json(
    data: str | bytes | bytearray | memoryview,
) -> StepTopologyInspectResultA0:
    return cast(
        StepTopologyInspectResultA0,
        decode_contract_json(
            data,
            "Wavenumber.Geometer.Contracts.StepTopologyA0.StepTopologyInspectResultA0",
            DECLARATIONS,
            MODEL_TYPES,
            ENUM_TYPES,
        ),
    )


def encode_step_topology_inspect_result_a0_json(value: StepTopologyInspectResultA0) -> bytes:
    return encode_contract_json(
        value,
        "Wavenumber.Geometer.Contracts.StepTopologyA0.StepTopologyInspectResultA0",
        DECLARATIONS,
        MODEL_TYPES,
        ENUM_TYPES,
    )


def decode_step_topology_open_request_a0_json(data: str | bytes | bytearray | memoryview) -> StepTopologyOpenRequestA0:
    return cast(
        StepTopologyOpenRequestA0,
        decode_contract_json(
            data,
            "Wavenumber.Geometer.Contracts.StepTopologyA0.StepTopologyOpenRequestA0",
            DECLARATIONS,
            MODEL_TYPES,
            ENUM_TYPES,
        ),
    )


def encode_step_topology_open_request_a0_json(value: StepTopologyOpenRequestA0) -> bytes:
    return encode_contract_json(
        value,
        "Wavenumber.Geometer.Contracts.StepTopologyA0.StepTopologyOpenRequestA0",
        DECLARATIONS,
        MODEL_TYPES,
        ENUM_TYPES,
    )


def decode_step_topology_open_result_a0_json(data: str | bytes | bytearray | memoryview) -> StepTopologyOpenResultA0:
    return cast(
        StepTopologyOpenResultA0,
        decode_contract_json(
            data,
            "Wavenumber.Geometer.Contracts.StepTopologyA0.StepTopologyOpenResultA0",
            DECLARATIONS,
            MODEL_TYPES,
            ENUM_TYPES,
        ),
    )


def encode_step_topology_open_result_a0_json(value: StepTopologyOpenResultA0) -> bytes:
    return encode_contract_json(
        value,
        "Wavenumber.Geometer.Contracts.StepTopologyA0.StepTopologyOpenResultA0",
        DECLARATIONS,
        MODEL_TYPES,
        ENUM_TYPES,
    )


def decode_step_topology_render_request_a0_json(
    data: str | bytes | bytearray | memoryview,
) -> StepTopologyRenderRequestA0:
    return cast(
        StepTopologyRenderRequestA0,
        decode_contract_json(
            data,
            "Wavenumber.Geometer.Contracts.StepTopologyA0.StepTopologyRenderRequestA0",
            DECLARATIONS,
            MODEL_TYPES,
            ENUM_TYPES,
        ),
    )


def encode_step_topology_render_request_a0_json(value: StepTopologyRenderRequestA0) -> bytes:
    return encode_contract_json(
        value,
        "Wavenumber.Geometer.Contracts.StepTopologyA0.StepTopologyRenderRequestA0",
        DECLARATIONS,
        MODEL_TYPES,
        ENUM_TYPES,
    )


def decode_step_topology_render_result_a0_json(
    data: str | bytes | bytearray | memoryview,
) -> StepTopologyRenderResultA0:
    return cast(
        StepTopologyRenderResultA0,
        decode_contract_json(
            data,
            "Wavenumber.Geometer.Contracts.StepTopologyA0.StepTopologyRenderResultA0",
            DECLARATIONS,
            MODEL_TYPES,
            ENUM_TYPES,
        ),
    )


def encode_step_topology_render_result_a0_json(value: StepTopologyRenderResultA0) -> bytes:
    return encode_contract_json(
        value,
        "Wavenumber.Geometer.Contracts.StepTopologyA0.StepTopologyRenderResultA0",
        DECLARATIONS,
        MODEL_TYPES,
        ENUM_TYPES,
    )


def decode_step_topology_resolve_hit_request_a0_json(
    data: str | bytes | bytearray | memoryview,
) -> StepTopologyResolveHitRequestA0:
    return cast(
        StepTopologyResolveHitRequestA0,
        decode_contract_json(
            data,
            "Wavenumber.Geometer.Contracts.StepTopologyA0.StepTopologyResolveHitRequestA0",
            DECLARATIONS,
            MODEL_TYPES,
            ENUM_TYPES,
        ),
    )


def encode_step_topology_resolve_hit_request_a0_json(value: StepTopologyResolveHitRequestA0) -> bytes:
    return encode_contract_json(
        value,
        "Wavenumber.Geometer.Contracts.StepTopologyA0.StepTopologyResolveHitRequestA0",
        DECLARATIONS,
        MODEL_TYPES,
        ENUM_TYPES,
    )


def decode_step_topology_resolve_hit_result_a0_json(
    data: str | bytes | bytearray | memoryview,
) -> StepTopologyResolveHitResultA0:
    return cast(
        StepTopologyResolveHitResultA0,
        decode_contract_json(
            data,
            "Wavenumber.Geometer.Contracts.StepTopologyA0.StepTopologyResolveHitResultA0",
            DECLARATIONS,
            MODEL_TYPES,
            ENUM_TYPES,
        ),
    )


def encode_step_topology_resolve_hit_result_a0_json(value: StepTopologyResolveHitResultA0) -> bytes:
    return encode_contract_json(
        value,
        "Wavenumber.Geometer.Contracts.StepTopologyA0.StepTopologyResolveHitResultA0",
        DECLARATIONS,
        MODEL_TYPES,
        ENUM_TYPES,
    )


def decode_step_topology_restore_request_a0_json(
    data: str | bytes | bytearray | memoryview,
) -> StepTopologyRestoreRequestA0:
    return cast(
        StepTopologyRestoreRequestA0,
        decode_contract_json(
            data,
            "Wavenumber.Geometer.Contracts.StepTopologyA0.StepTopologyRestoreRequestA0",
            DECLARATIONS,
            MODEL_TYPES,
            ENUM_TYPES,
        ),
    )


def encode_step_topology_restore_request_a0_json(value: StepTopologyRestoreRequestA0) -> bytes:
    return encode_contract_json(
        value,
        "Wavenumber.Geometer.Contracts.StepTopologyA0.StepTopologyRestoreRequestA0",
        DECLARATIONS,
        MODEL_TYPES,
        ENUM_TYPES,
    )


def decode_step_topology_restore_result_a0_json(
    data: str | bytes | bytearray | memoryview,
) -> StepTopologyRestoreResultA0:
    return cast(
        StepTopologyRestoreResultA0,
        decode_contract_json(
            data,
            "Wavenumber.Geometer.Contracts.StepTopologyA0.StepTopologyRestoreResultA0",
            DECLARATIONS,
            MODEL_TYPES,
            ENUM_TYPES,
        ),
    )


def encode_step_topology_restore_result_a0_json(value: StepTopologyRestoreResultA0) -> bytes:
    return encode_contract_json(
        value,
        "Wavenumber.Geometer.Contracts.StepTopologyA0.StepTopologyRestoreResultA0",
        DECLARATIONS,
        MODEL_TYPES,
        ENUM_TYPES,
    )


def decode_step_topology_save_request_a0_json(data: str | bytes | bytearray | memoryview) -> StepTopologySaveRequestA0:
    return cast(
        StepTopologySaveRequestA0,
        decode_contract_json(
            data,
            "Wavenumber.Geometer.Contracts.StepTopologyA0.StepTopologySaveRequestA0",
            DECLARATIONS,
            MODEL_TYPES,
            ENUM_TYPES,
        ),
    )


def encode_step_topology_save_request_a0_json(value: StepTopologySaveRequestA0) -> bytes:
    return encode_contract_json(
        value,
        "Wavenumber.Geometer.Contracts.StepTopologyA0.StepTopologySaveRequestA0",
        DECLARATIONS,
        MODEL_TYPES,
        ENUM_TYPES,
    )


def decode_step_topology_save_result_a0_json(data: str | bytes | bytearray | memoryview) -> StepTopologySaveResultA0:
    return cast(
        StepTopologySaveResultA0,
        decode_contract_json(
            data,
            "Wavenumber.Geometer.Contracts.StepTopologyA0.StepTopologySaveResultA0",
            DECLARATIONS,
            MODEL_TYPES,
            ENUM_TYPES,
        ),
    )


def encode_step_topology_save_result_a0_json(value: StepTopologySaveResultA0) -> bytes:
    return encode_contract_json(
        value,
        "Wavenumber.Geometer.Contracts.StepTopologyA0.StepTopologySaveResultA0",
        DECLARATIONS,
        MODEL_TYPES,
        ENUM_TYPES,
    )


ROOT_DECODERS: dict[str, Callable[[str | bytes | bytearray | memoryview], Any]] = {
    "geometry.common.diagnostic.a0": decode_diagnostic_a0_json,
    "geometry.hlr_projection.options.a0": decode_hlr_projection_options_a0_json,
    "geometry.hlr_projection.result.a0": decode_hlr_projection_result_a0_json,
    "geometer.ipc.cancelled.a0": decode_ipc_cancelled_a0_json,
    "geometer.ipc.cancel_rejected.a0": decode_ipc_cancel_rejected_a0_json,
    "geometer.ipc.hello.a0": decode_ipc_hello_a0_json,
    "geometer.ipc.operation_catalog.a0": decode_ipc_operation_catalog_a0_json,
    "geometer.ipc.protocol_error.a0": decode_ipc_protocol_error_a0_json,
    "geometer.ipc.reason.a0": decode_ipc_reason_a0_json,
    "geometer.ipc.request.a0": decode_ipc_request_a0_json,
    "geometer.ipc.shutdown_ack.a0": decode_ipc_shutdown_ack_a0_json,
    "geometer.ipc.welcome.a0": decode_ipc_welcome_a0_json,
    "geometry.mesh_illustration.input.a0": decode_mesh_illustration_input_a0_json,
    "geometry.mesh_illustration.result.a0": decode_mesh_illustration_result_a0_json,
    "geometry.mesh_illustration.style.a0": decode_mesh_illustration_style_a0_json,
    "geometry.model_bounds.options.a0": decode_model_bounds_options_a0_json,
    "geometry.model_bounds.a0": decode_model_bounds_result_a0_json,
    "geometry.mesh_collection.a0": decode_mesh_collection_a0_json,
    "geometry.model_tessellation.request.a0": decode_model_tessellation_request_a0_json,
    "geometry.model_tessellation.result.a0": decode_model_tessellation_result_a0_json,
    "geometer.operation.outcome.a0": decode_operation_outcome_a0_json,
    "geometry.step_topology.analyze_recovery.request.a0": decode_step_topology_analyze_recovery_request_a0_json,
    "geometry.step_topology.analyze_recovery.result.a0": decode_step_topology_analyze_recovery_result_a0_json,
    "geometry.step_topology.apply_hierarchy.request.a0": decode_step_topology_apply_hierarchy_request_a0_json,
    "geometry.step_topology.apply_hierarchy.result.a0": decode_step_topology_apply_hierarchy_result_a0_json,
    "geometry.step_topology.apply_logical_groups.request.a0": decode_step_topology_apply_logical_groups_request_a0_json,
    "geometry.step_topology.apply_logical_groups.result.a0": decode_step_topology_apply_logical_groups_result_a0_json,
    "geometry.step_topology.apply_metadata_probes.request.a0": decode_step_topology_apply_metadata_probes_request_a0_json,
    "geometry.step_topology.apply_metadata_probes.result.a0": decode_step_topology_apply_metadata_probes_result_a0_json,
    "geometry.step_topology.checkpoint_edit_journal.request.a0": decode_step_topology_checkpoint_edit_journal_request_a0_json,
    "geometry.step_topology.checkpoint_edit_journal.result.a0": decode_step_topology_checkpoint_edit_journal_result_a0_json,
    "geometry.step_topology.close.request.a0": decode_step_topology_close_request_a0_json,
    "geometry.step_topology.close.result.a0": decode_step_topology_close_result_a0_json,
    "geometry.step_topology.inspect.request.a0": decode_step_topology_inspect_request_a0_json,
    "geometry.step_topology.inspect.result.a0": decode_step_topology_inspect_result_a0_json,
    "geometry.step_topology.open.request.a0": decode_step_topology_open_request_a0_json,
    "geometry.step_topology.open.result.a0": decode_step_topology_open_result_a0_json,
    "geometry.step_topology.render.request.a0": decode_step_topology_render_request_a0_json,
    "geometry.step_topology.render.result.a0": decode_step_topology_render_result_a0_json,
    "geometry.step_topology.resolve_hit.request.a0": decode_step_topology_resolve_hit_request_a0_json,
    "geometry.step_topology.resolve_hit.result.a0": decode_step_topology_resolve_hit_result_a0_json,
    "geometry.step_topology.restore.request.a0": decode_step_topology_restore_request_a0_json,
    "geometry.step_topology.restore.result.a0": decode_step_topology_restore_result_a0_json,
    "geometry.step_topology.save.request.a0": decode_step_topology_save_request_a0_json,
    "geometry.step_topology.save.result.a0": decode_step_topology_save_result_a0_json,
}
