// Generated from wn_geometer_contract_catalog.a0.json. Do not edit.

import type { ContractDescriptorMap } from "../codec-runtime.js";
import { decodeContractJson, encodeContractJson } from "../codec-runtime.js";
import type {
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
  MeshCollectionA0,
  MeshIllustrationInputA0,
  MeshIllustrationResultA0,
  MeshIllustrationStyleA0,
  ModelBoundsOptionsA0,
  ModelBoundsResultA0,
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
  "Wavenumber.Geometer.Contracts.HlrProjectionA0.FastHlrLimitsA0": {
    kind: "object",
    properties: {
      max_vertices: {
        type: { kind: "primitive", name: "uint32" },
        optional: true,
        constraints: { min_value: 1, max_value: 4294967295 },
      },
      max_triangles: {
        type: { kind: "primitive", name: "uint32" },
        optional: true,
        constraints: { min_value: 1, max_value: 4294967295 },
      },
      max_edges: {
        type: { kind: "primitive", name: "uint32" },
        optional: true,
        constraints: { min_value: 1, max_value: 4294967295 },
      },
      max_grid_references: {
        type: { kind: "primitive", name: "uint32" },
        optional: true,
        constraints: { min_value: 1, max_value: 4294967295 },
      },
      max_candidate_pairs: {
        type: { kind: "primitive", name: "uint32" },
        optional: true,
        constraints: { min_value: 1, max_value: 4294967295 },
      },
      max_fragments: {
        type: { kind: "primitive", name: "uint32" },
        optional: true,
        constraints: { min_value: 1, max_value: 4294967295 },
      },
      max_output_segments: {
        type: { kind: "primitive", name: "uint32" },
        optional: true,
        constraints: { min_value: 1, max_value: 4294967295 },
      },
    },
  },
  "Wavenumber.Geometer.Contracts.HlrProjectionA0.FastHlrOptionsA0": {
    kind: "object",
    properties: {
      include_boundaries: {
        type: { kind: "primitive", name: "boolean" },
        optional: true,
        constraints: {},
      },
      include_creases: {
        type: { kind: "primitive", name: "boolean" },
        optional: true,
        constraints: {},
      },
      include_silhouettes: {
        type: { kind: "primitive", name: "boolean" },
        optional: true,
        constraints: {},
      },
      include_hidden: {
        type: { kind: "primitive", name: "boolean" },
        optional: true,
        constraints: {},
      },
      suppress_coplanar_seams: {
        type: { kind: "primitive", name: "boolean" },
        optional: true,
        constraints: {},
      },
      crease_angle_rad: {
        type: { kind: "primitive", name: "float64" },
        optional: true,
        constraints: { min_value: 0, max_value: Math.PI },
      },
      weld_tolerance: {
        type: { kind: "primitive", name: "float64" },
        optional: true,
        constraints: { min_value_exclusive: 0 },
      },
      projected_tolerance: {
        type: { kind: "primitive", name: "float64" },
        optional: true,
        constraints: { min_value_exclusive: 0 },
      },
      depth_tolerance: {
        type: { kind: "primitive", name: "float64" },
        optional: true,
        constraints: { min_value: 0 },
      },
      coplanar_seam_angle_rad: {
        type: { kind: "primitive", name: "float64" },
        optional: true,
        constraints: { min_value: 0, max_value: 1.5707963267948966 },
      },
      coplanar_seam_depth_tolerance: {
        type: { kind: "primitive", name: "float64" },
        optional: true,
        constraints: { min_value: 0 },
      },
      coplanar_seam_lateral_tolerance: {
        type: { kind: "primitive", name: "float64" },
        optional: true,
        constraints: { min_value: 0 },
      },
      limits: {
        type: {
          kind: "reference",
          target: "Wavenumber.Geometer.Contracts.HlrProjectionA0.FastHlrLimitsA0",
        },
        optional: true,
        constraints: {},
      },
    },
  },
  "Wavenumber.Geometer.Contracts.HlrProjectionA0.HlrCurveMode": {
    kind: "enum",
    values: ["native_arcs", "polyline"],
  },
  "Wavenumber.Geometer.Contracts.HlrProjectionA0.HlrMatrix4x4": {
    kind: "array",
    element: { kind: "primitive", name: "float64" },
    constraints: { min_items: 16, max_items: 16 },
  },
  "Wavenumber.Geometer.Contracts.HlrProjectionA0.HlrMeshDeflectionMode": {
    kind: "enum",
    values: ["absolute", "bbox-relative"],
  },
  "Wavenumber.Geometer.Contracts.HlrProjectionA0.HlrOutlineAlgorithm": {
    kind: "enum",
    values: ["hlr-close", "mesh-shadow", "fast-mesh-shadow"],
  },
  "Wavenumber.Geometer.Contracts.HlrProjectionA0.HlrProjectedView": {
    kind: "object",
    properties: {
      id: {
        type: { kind: "primitive", name: "string" },
        optional: false,
        constraints: { min_length: 1, max_length: 128 },
      },
      direction: {
        type: {
          kind: "reference",
          target: "Wavenumber.Geometer.Contracts.HlrProjectionA0.HlrVector3",
        },
        optional: false,
        constraints: {},
      },
      up: {
        type: {
          kind: "reference",
          target: "Wavenumber.Geometer.Contracts.HlrProjectionA0.HlrVector3",
        },
        optional: false,
        constraints: {},
      },
      modes: {
        type: {
          kind: "reference",
          target: "Wavenumber.Geometer.Contracts.HlrProjectionA0.HlrProjectionModes",
        },
        optional: false,
        constraints: {},
      },
    },
  },
  "Wavenumber.Geometer.Contracts.HlrProjectionA0.HlrProjectionAlgorithm": {
    kind: "enum",
    values: ["poly", "exact", "fast"],
  },
  "Wavenumber.Geometer.Contracts.HlrProjectionA0.HlrProjectionModes": {
    kind: "object",
    properties: {
      outline: {
        type: {
          kind: "reference",
          target: "Wavenumber.Geometer.Contracts.HlrProjectionA0.ProjectedGeometry",
        },
        optional: false,
        constraints: {},
      },
      detail: {
        type: {
          kind: "reference",
          target: "Wavenumber.Geometer.Contracts.HlrProjectionA0.ProjectedGeometry",
        },
        optional: false,
        constraints: {},
      },
      bbox: {
        type: {
          kind: "reference",
          target: "Wavenumber.Geometer.Contracts.HlrProjectionA0.ProjectedGeometry",
        },
        optional: false,
        constraints: {},
      },
    },
  },
  "Wavenumber.Geometer.Contracts.HlrProjectionA0.HlrProjectionOptionsA0": {
    kind: "object",
    properties: {
      views: {
        type: {
          kind: "array",
          element: {
            kind: "reference",
            target: "Wavenumber.Geometer.Contracts.HlrProjectionA0.HlrViewSpec",
          },
        },
        optional: true,
        constraints: { max_items: 64 },
      },
      output_outline: {
        type: { kind: "primitive", name: "boolean" },
        optional: true,
        constraints: {},
      },
      output_detail: {
        type: { kind: "primitive", name: "boolean" },
        optional: true,
        constraints: {},
      },
      output_bbox: {
        type: { kind: "primitive", name: "boolean" },
        optional: true,
        constraints: {},
      },
      model_transform: {
        type: {
          kind: "reference",
          target: "Wavenumber.Geometer.Contracts.HlrProjectionA0.HlrMatrix4x4",
        },
        optional: true,
        constraints: {},
      },
      strip_root_placement: {
        type: { kind: "primitive", name: "boolean" },
        optional: true,
        constraints: {},
      },
      curve_mode: {
        type: {
          kind: "reference",
          target: "Wavenumber.Geometer.Contracts.HlrProjectionA0.HlrCurveMode",
        },
        optional: true,
        constraints: {},
      },
      samples_per_curve: {
        type: { kind: "primitive", name: "uint32" },
        optional: true,
        constraints: { min_value: 1, max_value: 100000 },
      },
      round_digits: {
        type: { kind: "primitive", name: "uint32" },
        optional: true,
        constraints: { min_value: 0, max_value: 9 },
      },
      edge_v_sharp: {
        type: { kind: "primitive", name: "boolean" },
        optional: true,
        constraints: {},
      },
      edge_v_outline: {
        type: { kind: "primitive", name: "boolean" },
        optional: true,
        constraints: {},
      },
      edge_v_smooth: {
        type: { kind: "primitive", name: "boolean" },
        optional: true,
        constraints: {},
      },
      edge_v_sewn: {
        type: { kind: "primitive", name: "boolean" },
        optional: true,
        constraints: {},
      },
      edge_v_iso: { type: { kind: "primitive", name: "boolean" }, optional: true, constraints: {} },
      edge_h_sharp: {
        type: { kind: "primitive", name: "boolean" },
        optional: true,
        constraints: {},
      },
      edge_h_outline: {
        type: { kind: "primitive", name: "boolean" },
        optional: true,
        constraints: {},
      },
      edge_h_smooth: {
        type: { kind: "primitive", name: "boolean" },
        optional: true,
        constraints: {},
      },
      edge_h_sewn: {
        type: { kind: "primitive", name: "boolean" },
        optional: true,
        constraints: {},
      },
      edge_h_iso: { type: { kind: "primitive", name: "boolean" }, optional: true, constraints: {} },
      union_outline_polygons: {
        type: { kind: "primitive", name: "boolean" },
        optional: true,
        constraints: {},
      },
      projection_algorithm: {
        type: {
          kind: "reference",
          target: "Wavenumber.Geometer.Contracts.HlrProjectionA0.HlrProjectionAlgorithm",
        },
        optional: true,
        constraints: {},
      },
      mesh_linear_deflection: {
        type: { kind: "primitive", name: "float64" },
        optional: true,
        constraints: { min_value: 0 },
      },
      mesh_angular_deflection: {
        type: { kind: "primitive", name: "float64" },
        optional: true,
        constraints: { min_value: 0, max_value: Math.PI },
      },
      mesh_relative: {
        type: { kind: "primitive", name: "boolean" },
        optional: true,
        constraints: {},
      },
      mesh_deflection_mode: {
        type: {
          kind: "reference",
          target: "Wavenumber.Geometer.Contracts.HlrProjectionA0.HlrMeshDeflectionMode",
        },
        optional: true,
        constraints: {},
      },
      mesh_deflection_coefficient: {
        type: { kind: "primitive", name: "float64" },
        optional: true,
        constraints: { min_value: 0 },
      },
      outline_algorithm: {
        type: {
          kind: "reference",
          target: "Wavenumber.Geometer.Contracts.HlrProjectionA0.HlrOutlineAlgorithm",
        },
        optional: true,
        constraints: {},
      },
      hlr_angle_tolerance: {
        type: { kind: "primitive", name: "float64" },
        optional: true,
        constraints: { min_value: 0, max_value: Math.PI },
      },
      fast: {
        type: {
          kind: "reference",
          target: "Wavenumber.Geometer.Contracts.HlrProjectionA0.FastHlrOptionsA0",
        },
        optional: true,
        constraints: {},
      },
    },
  },
  "Wavenumber.Geometer.Contracts.HlrProjectionA0.HlrProjectionResultA0": {
    kind: "object",
    properties: {
      schema: {
        type: { kind: "literal", value_type: "string", value: "geometry.hlr_projection.result.a0" },
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
          target: "Wavenumber.Geometer.Contracts.HlrProjectionA0.HlrProjectionSource",
        },
        optional: false,
        constraints: {},
      },
      views: {
        type: {
          kind: "array",
          element: {
            kind: "reference",
            target: "Wavenumber.Geometer.Contracts.HlrProjectionA0.HlrProjectedView",
          },
        },
        optional: false,
        constraints: { max_items: 64 },
      },
      timings: {
        type: {
          kind: "reference",
          target: "Wavenumber.Geometer.Contracts.HlrProjectionA0.HlrProjectionTimings",
        },
        optional: false,
        constraints: {},
      },
    },
  },
  "Wavenumber.Geometer.Contracts.HlrProjectionA0.HlrProjectionSource": {
    kind: "object",
    properties: {
      kind: {
        type: {
          kind: "reference",
          target: "Wavenumber.Geometer.Contracts.HlrProjectionA0.HlrSourceKind",
        },
        optional: false,
        constraints: {},
      },
      hash: {
        type: { kind: "primitive", name: "string" },
        optional: false,
        constraints: { min_length: 64, max_length: 64 },
      },
    },
  },
  "Wavenumber.Geometer.Contracts.HlrProjectionA0.HlrProjectionTimings": {
    kind: "object",
    properties: {
      step_read_ms: {
        type: { kind: "primitive", name: "float64" },
        optional: false,
        constraints: { min_value: 0 },
      },
      mesh_ms: {
        type: { kind: "primitive", name: "float64" },
        optional: false,
        constraints: { min_value: 0 },
      },
      hlr_ms: {
        type: { kind: "primitive", name: "float64" },
        optional: false,
        constraints: { min_value: 0 },
      },
      extract_ms: {
        type: { kind: "primitive", name: "float64" },
        optional: false,
        constraints: { min_value: 0 },
      },
    },
  },
  "Wavenumber.Geometer.Contracts.HlrProjectionA0.HlrSourceKind": {
    kind: "enum",
    values: ["step", "indexed_mesh"],
  },
  "Wavenumber.Geometer.Contracts.HlrProjectionA0.HlrVector2": {
    kind: "array",
    element: { kind: "primitive", name: "float64" },
    constraints: { min_items: 2, max_items: 2 },
  },
  "Wavenumber.Geometer.Contracts.HlrProjectionA0.HlrVector3": {
    kind: "array",
    element: { kind: "primitive", name: "float64" },
    constraints: { min_items: 3, max_items: 3 },
  },
  "Wavenumber.Geometer.Contracts.HlrProjectionA0.HlrViewSpec": {
    kind: "object",
    properties: {
      id: {
        type: { kind: "primitive", name: "string" },
        optional: false,
        constraints: { min_length: 1, max_length: 128 },
      },
      direction: {
        type: {
          kind: "reference",
          target: "Wavenumber.Geometer.Contracts.HlrProjectionA0.HlrVector3",
        },
        optional: false,
        constraints: {},
      },
      up: {
        type: {
          kind: "reference",
          target: "Wavenumber.Geometer.Contracts.HlrProjectionA0.HlrVector3",
        },
        optional: false,
        constraints: {},
      },
    },
  },
  "Wavenumber.Geometer.Contracts.HlrProjectionA0.ProjectedArc": {
    kind: "object",
    properties: {
      start: {
        type: {
          kind: "reference",
          target: "Wavenumber.Geometer.Contracts.HlrProjectionA0.HlrVector2",
        },
        optional: false,
        constraints: {},
      },
      end: {
        type: {
          kind: "reference",
          target: "Wavenumber.Geometer.Contracts.HlrProjectionA0.HlrVector2",
        },
        optional: false,
        constraints: {},
      },
      center: {
        type: {
          kind: "reference",
          target: "Wavenumber.Geometer.Contracts.HlrProjectionA0.HlrVector2",
        },
        optional: false,
        constraints: {},
      },
      radius: {
        type: { kind: "primitive", name: "float64" },
        optional: false,
        constraints: { min_value: 0 },
      },
      extent_rad: {
        type: { kind: "primitive", name: "float64" },
        optional: false,
        constraints: { min_value: 0, max_value: 6.283185307179586 },
      },
      ccw: { type: { kind: "primitive", name: "boolean" }, optional: false, constraints: {} },
      full_circle: {
        type: { kind: "primitive", name: "boolean" },
        optional: false,
        constraints: {},
      },
    },
  },
  "Wavenumber.Geometer.Contracts.HlrProjectionA0.ProjectedGeometry": {
    kind: "object",
    properties: {
      segments: {
        type: {
          kind: "array",
          element: {
            kind: "reference",
            target: "Wavenumber.Geometer.Contracts.HlrProjectionA0.ProjectedSegment",
          },
        },
        optional: false,
        constraints: { max_items: 4000000 },
      },
      arcs: {
        type: {
          kind: "array",
          element: {
            kind: "reference",
            target: "Wavenumber.Geometer.Contracts.HlrProjectionA0.ProjectedArc",
          },
        },
        optional: false,
        constraints: { max_items: 4000000 },
      },
      bounds: {
        type: {
          kind: "reference",
          target: "Wavenumber.Geometer.Contracts.HlrProjectionA0.ProjectionBounds",
        },
        optional: true,
        constraints: {},
      },
    },
  },
  "Wavenumber.Geometer.Contracts.HlrProjectionA0.ProjectedSegment": {
    kind: "array",
    element: { kind: "primitive", name: "float64" },
    constraints: { min_items: 4, max_items: 4 },
  },
  "Wavenumber.Geometer.Contracts.HlrProjectionA0.ProjectionBounds": {
    kind: "object",
    properties: {
      min_x: { type: { kind: "primitive", name: "float64" }, optional: false, constraints: {} },
      min_y: { type: { kind: "primitive", name: "float64" }, optional: false, constraints: {} },
      max_x: { type: { kind: "primitive", name: "float64" }, optional: false, constraints: {} },
      max_y: { type: { kind: "primitive", name: "float64" }, optional: false, constraints: {} },
      width: {
        type: { kind: "primitive", name: "float64" },
        optional: false,
        constraints: { min_value: 0 },
      },
      height: {
        type: { kind: "primitive", name: "float64" },
        optional: false,
        constraints: { min_value: 0 },
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
        target: "Wavenumber.Geometer.Contracts.ModelTessellationA0.ModelTessellationRequestA0",
      },
      {
        kind: "reference",
        target: "Wavenumber.Geometer.Contracts.ModelBoundsA0.ModelBoundsOptionsA0",
      },
      {
        kind: "reference",
        target: "Wavenumber.Geometer.Contracts.HlrProjectionA0.HlrProjectionOptionsA0",
      },
      {
        kind: "reference",
        target: "Wavenumber.Geometer.Contracts.Common.PackedAttachmentProjectionA0",
      },
      {
        kind: "reference",
        target: "Wavenumber.Geometer.Contracts.StepTopologyA0.StepTopologyOpenRequestA0",
      },
      {
        kind: "reference",
        target: "Wavenumber.Geometer.Contracts.StepTopologyA0.StepTopologyCloseRequestA0",
      },
      {
        kind: "reference",
        target: "Wavenumber.Geometer.Contracts.StepTopologyA0.StepTopologyInspectRequestA0",
      },
      {
        kind: "reference",
        target: "Wavenumber.Geometer.Contracts.StepTopologyA0.StepTopologyRenderRequestA0",
      },
      {
        kind: "reference",
        target: "Wavenumber.Geometer.Contracts.StepTopologyA0.StepTopologyResolveHitRequestA0",
      },
      {
        kind: "reference",
        target:
          "Wavenumber.Geometer.Contracts.StepTopologyA0.StepTopologyApplyLogicalGroupsRequestA0",
      },
      {
        kind: "reference",
        target:
          "Wavenumber.Geometer.Contracts.StepTopologyA0.StepTopologyApplyMetadataProbesRequestA0",
      },
      {
        kind: "reference",
        target:
          "Wavenumber.Geometer.Contracts.StepTopologyA0.StepTopologyCheckpointEditJournalRequestA0",
      },
      {
        kind: "reference",
        target: "Wavenumber.Geometer.Contracts.StepTopologyA0.StepTopologyApplyHierarchyRequestA0",
      },
      {
        kind: "reference",
        target: "Wavenumber.Geometer.Contracts.StepTopologyA0.StepTopologySaveRequestA0",
      },
      {
        kind: "reference",
        target: "Wavenumber.Geometer.Contracts.StepTopologyA0.StepTopologyRestoreRequestA0",
      },
      {
        kind: "reference",
        target: "Wavenumber.Geometer.Contracts.StepTopologyA0.StepTopologyAnalyzeRecoveryRequestA0",
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
  "Wavenumber.Geometer.Contracts.MeshIllustrationA0.IllustrationMatrix4x4": {
    kind: "array",
    element: { kind: "primitive", name: "float64" },
    constraints: { min_items: 16, max_items: 16 },
  },
  "Wavenumber.Geometer.Contracts.MeshIllustrationA0.IllustrationVector3": {
    kind: "array",
    element: { kind: "primitive", name: "float64" },
    constraints: { min_items: 3, max_items: 3 },
  },
  "Wavenumber.Geometer.Contracts.MeshIllustrationA0.MeshIllustrationInputA0": {
    kind: "object",
    properties: {
      schema: {
        type: {
          kind: "literal",
          value_type: "string",
          value: "geometry.mesh_illustration.input.a0",
        },
        optional: false,
        constraints: {},
      },
      meshes: {
        type: {
          kind: "array",
          element: {
            kind: "reference",
            target: "Wavenumber.Geometer.Contracts.MeshIllustrationA0.MeshIllustrationMesh",
          },
        },
        optional: false,
        constraints: { min_items: 1, max_items: 65536 },
      },
      view: {
        type: {
          kind: "reference",
          target: "Wavenumber.Geometer.Contracts.MeshIllustrationA0.MeshIllustrationView",
        },
        optional: false,
        constraints: {},
      },
      prepare: {
        type: {
          kind: "reference",
          target: "Wavenumber.Geometer.Contracts.MeshIllustrationA0.MeshIllustrationPrepareOptions",
        },
        optional: true,
        constraints: {},
      },
      style: {
        type: {
          kind: "reference",
          target: "Wavenumber.Geometer.Contracts.MeshIllustrationA0.MeshIllustrationStyleA0",
        },
        optional: true,
        constraints: {},
      },
      svg: {
        type: {
          kind: "reference",
          target: "Wavenumber.Geometer.Contracts.MeshIllustrationA0.MeshIllustrationSvgOptions",
        },
        optional: true,
        constraints: {},
      },
    },
  },
  "Wavenumber.Geometer.Contracts.MeshIllustrationA0.MeshIllustrationMaterial": {
    kind: "object",
    properties: {
      color: {
        type: {
          kind: "reference",
          target: "Wavenumber.Geometer.Contracts.MeshIllustrationA0.IllustrationVector3",
        },
        optional: false,
        constraints: {},
      },
      opacity: {
        type: { kind: "primitive", name: "float64" },
        optional: true,
        constraints: { min_value: 0, max_value: 1 },
      },
      name: {
        type: { kind: "primitive", name: "string" },
        optional: true,
        constraints: { max_length: 1024 },
      },
    },
  },
  "Wavenumber.Geometer.Contracts.MeshIllustrationA0.MeshIllustrationMesh": {
    kind: "object",
    properties: {
      id: {
        type: { kind: "primitive", name: "string" },
        optional: false,
        constraints: { min_length: 1, max_length: 1024 },
      },
      positions: {
        type: { kind: "array", element: { kind: "primitive", name: "float64" } },
        optional: false,
        constraints: { min_items: 9, max_items: 6000000 },
      },
      normals: {
        type: { kind: "array", element: { kind: "primitive", name: "float64" } },
        optional: true,
        constraints: { max_items: 6000000 },
      },
      indices: {
        type: { kind: "array", element: { kind: "primitive", name: "uint32" } },
        optional: true,
        constraints: { max_items: 6000000 },
      },
      matrix: {
        type: {
          kind: "reference",
          target: "Wavenumber.Geometer.Contracts.MeshIllustrationA0.IllustrationMatrix4x4",
        },
        optional: true,
        constraints: {},
      },
      materials: {
        type: {
          kind: "array",
          element: {
            kind: "reference",
            target: "Wavenumber.Geometer.Contracts.MeshIllustrationA0.MeshIllustrationMaterial",
          },
        },
        optional: false,
        constraints: { min_items: 1, max_items: 65536 },
      },
      triangle_material_indices: {
        type: { kind: "array", element: { kind: "primitive", name: "uint32" } },
        optional: true,
        constraints: { max_items: 2000000 },
      },
      double_sided: {
        type: { kind: "primitive", name: "boolean" },
        optional: true,
        constraints: {},
      },
    },
  },
  "Wavenumber.Geometer.Contracts.MeshIllustrationA0.MeshIllustrationPrepareOptions": {
    kind: "object",
    properties: {
      max_triangles: {
        type: { kind: "primitive", name: "uint32" },
        optional: true,
        constraints: { min_value: 1, max_value: 2000000 },
      },
      weld_tolerance: {
        type: { kind: "primitive", name: "float64" },
        optional: true,
        constraints: { min_value_exclusive: 0 },
      },
    },
  },
  "Wavenumber.Geometer.Contracts.MeshIllustrationA0.MeshIllustrationRenderStats": {
    kind: "object",
    properties: {
      triangles: {
        type: { kind: "primitive", name: "uint32" },
        optional: false,
        constraints: { min_value: 0 },
      },
      surface_draws: {
        type: { kind: "primitive", name: "uint32" },
        optional: false,
        constraints: { min_value: 0 },
      },
      layered_surfaces: {
        type: { kind: "primitive", name: "uint32" },
        optional: false,
        constraints: { min_value: 0 },
      },
      outlines: {
        type: { kind: "primitive", name: "uint32" },
        optional: false,
        constraints: { min_value: 0 },
      },
      details: {
        type: { kind: "primitive", name: "uint32" },
        optional: false,
        constraints: { min_value: 0 },
      },
      creases: {
        type: { kind: "primitive", name: "uint32" },
        optional: false,
        constraints: { min_value: 0 },
      },
      commands: {
        type: { kind: "primitive", name: "uint32" },
        optional: false,
        constraints: { min_value: 0 },
      },
    },
  },
  "Wavenumber.Geometer.Contracts.MeshIllustrationA0.MeshIllustrationResultA0": {
    kind: "object",
    properties: {
      schema: {
        type: {
          kind: "literal",
          value_type: "string",
          value: "geometry.mesh_illustration.result.a0",
        },
        optional: false,
        constraints: {},
      },
      svg: { type: { kind: "primitive", name: "string" }, optional: false, constraints: {} },
      stats: {
        type: {
          kind: "reference",
          target: "Wavenumber.Geometer.Contracts.MeshIllustrationA0.MeshIllustrationRenderStats",
        },
        optional: false,
        constraints: {},
      },
      warnings: {
        type: { kind: "array", element: { kind: "primitive", name: "string" } },
        optional: false,
        constraints: { max_items: 256 },
      },
    },
  },
  "Wavenumber.Geometer.Contracts.MeshIllustrationA0.MeshIllustrationShading": {
    kind: "enum",
    values: ["unlit", "flat", "lambert", "banded", "toon"],
  },
  "Wavenumber.Geometer.Contracts.MeshIllustrationA0.MeshIllustrationStyleA0": {
    kind: "object",
    properties: {
      shading: {
        type: {
          kind: "reference",
          target: "Wavenumber.Geometer.Contracts.MeshIllustrationA0.MeshIllustrationShading",
        },
        optional: true,
        constraints: {},
      },
      ambient: {
        type: { kind: "primitive", name: "float64" },
        optional: true,
        constraints: { min_value: 0, max_value: 1 },
      },
      key_intensity: {
        type: { kind: "primitive", name: "float64" },
        optional: true,
        constraints: { min_value: 0, max_value: 4 },
      },
      light_direction: {
        type: {
          kind: "reference",
          target: "Wavenumber.Geometer.Contracts.MeshIllustrationA0.IllustrationVector3",
        },
        optional: true,
        constraints: {},
      },
      bands: {
        type: { kind: "primitive", name: "uint32" },
        optional: true,
        constraints: { min_value: 1, max_value: 256 },
      },
      source_colors: {
        type: { kind: "primitive", name: "boolean" },
        optional: true,
        constraints: {},
      },
      fallback_color: {
        type: {
          kind: "reference",
          target: "Wavenumber.Geometer.Contracts.MeshIllustrationA0.IllustrationVector3",
        },
        optional: true,
        constraints: {},
      },
      background: {
        type: { kind: "primitive", name: "string" },
        optional: true,
        constraints: { min_length: 1, max_length: 128 },
      },
      transparent_background: {
        type: { kind: "primitive", name: "boolean" },
        optional: true,
        constraints: {},
      },
      fuse_surfaces: {
        type: { kind: "primitive", name: "boolean" },
        optional: true,
        constraints: {},
      },
      layer_coplanar_materials: {
        type: { kind: "primitive", name: "boolean" },
        optional: true,
        constraints: {},
      },
      show_hlr_outline: {
        type: { kind: "primitive", name: "boolean" },
        optional: true,
        constraints: {},
      },
      show_hlr_detail: {
        type: { kind: "primitive", name: "boolean" },
        optional: true,
        constraints: {},
      },
      show_outlines: {
        type: { kind: "primitive", name: "boolean" },
        optional: true,
        constraints: {},
      },
      show_creases: {
        type: { kind: "primitive", name: "boolean" },
        optional: true,
        constraints: {},
      },
      crease_angle_degrees: {
        type: { kind: "primitive", name: "float64" },
        optional: true,
        constraints: { min_value: 0, max_value: 180 },
      },
      outline_color: {
        type: { kind: "primitive", name: "string" },
        optional: true,
        constraints: { min_length: 1, max_length: 128 },
      },
      crease_color: {
        type: { kind: "primitive", name: "string" },
        optional: true,
        constraints: { min_length: 1, max_length: 128 },
      },
      outline_width: {
        type: { kind: "primitive", name: "float64" },
        optional: true,
        constraints: { min_value: 0 },
      },
      crease_width: {
        type: { kind: "primitive", name: "float64" },
        optional: true,
        constraints: { min_value: 0 },
      },
      double_sided: {
        type: { kind: "primitive", name: "boolean" },
        optional: true,
        constraints: {},
      },
      rim_amount: {
        type: { kind: "primitive", name: "float64" },
        optional: true,
        constraints: { min_value: 0, max_value: 1 },
      },
    },
  },
  "Wavenumber.Geometer.Contracts.MeshIllustrationA0.MeshIllustrationSvgOptions": {
    kind: "object",
    properties: {
      coordinate_span: {
        type: { kind: "primitive", name: "uint32" },
        optional: true,
        constraints: { min_value: 10000, max_value: 1000000000 },
      },
      title: {
        type: { kind: "primitive", name: "string" },
        optional: true,
        constraints: { min_length: 1, max_length: 1024 },
      },
    },
  },
  "Wavenumber.Geometer.Contracts.MeshIllustrationA0.MeshIllustrationView": {
    kind: "object",
    properties: {
      direction: {
        type: {
          kind: "reference",
          target: "Wavenumber.Geometer.Contracts.MeshIllustrationA0.IllustrationVector3",
        },
        optional: false,
        constraints: {},
      },
      up: {
        type: {
          kind: "reference",
          target: "Wavenumber.Geometer.Contracts.MeshIllustrationA0.IllustrationVector3",
        },
        optional: false,
        constraints: {},
      },
      mirror_x: { type: { kind: "primitive", name: "boolean" }, optional: true, constraints: {} },
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
  "Wavenumber.Geometer.Contracts.ModelTessellationA0.MeshCollectionA0": {
    kind: "object",
    properties: {
      schema: {
        type: { kind: "literal", value_type: "string", value: "geometry.mesh_collection.a0" },
        optional: false,
        constraints: {},
      },
      length_unit: {
        type: { kind: "literal", value_type: "string", value: "millimeter" },
        optional: false,
        constraints: {},
      },
      meshes: {
        type: {
          kind: "array",
          element: {
            kind: "reference",
            target: "Wavenumber.Geometer.Contracts.MeshIllustrationA0.MeshIllustrationMesh",
          },
        },
        optional: false,
        constraints: { min_items: 1, max_items: 65536 },
      },
    },
  },
  "Wavenumber.Geometer.Contracts.ModelTessellationA0.MeshCollectionAttachment": {
    kind: "object",
    properties: {
      attachment: {
        type: { kind: "literal", value_type: "string", value: "mesh_collection" },
        optional: false,
        constraints: {},
      },
      schema: {
        type: { kind: "literal", value_type: "string", value: "geometry.mesh_collection.a0" },
        optional: false,
        constraints: {},
      },
      byte_length: {
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
  "Wavenumber.Geometer.Contracts.ModelTessellationA0.ModelRootPlacement": {
    kind: "enum",
    values: ["strip", "preserve"],
  },
  "Wavenumber.Geometer.Contracts.ModelTessellationA0.ModelTessellationRequestA0": {
    kind: "object",
    properties: {
      schema: {
        type: {
          kind: "literal",
          value_type: "string",
          value: "geometry.model_tessellation.request.a0",
        },
        optional: false,
        constraints: {},
      },
      linear_deflection_mm: {
        type: { kind: "primitive", name: "float64" },
        optional: true,
        constraints: { min_value: 0.000001, max_value: 1000 },
      },
      angular_deflection_rad: {
        type: { kind: "primitive", name: "float64" },
        optional: true,
        constraints: { min_value: 0.000001, max_value: Math.PI },
      },
      root_placement: {
        type: {
          kind: "reference",
          target: "Wavenumber.Geometer.Contracts.ModelTessellationA0.ModelRootPlacement",
        },
        optional: true,
        constraints: {},
      },
      max_triangles: {
        type: { kind: "primitive", name: "uint32" },
        optional: true,
        constraints: { min_value: 1, max_value: 2000000 },
      },
    },
  },
  "Wavenumber.Geometer.Contracts.ModelTessellationA0.ModelTessellationResultA0": {
    kind: "object",
    properties: {
      schema: {
        type: {
          kind: "literal",
          value_type: "string",
          value: "geometry.model_tessellation.result.a0",
        },
        optional: false,
        constraints: {},
      },
      mesh_collection: {
        type: {
          kind: "reference",
          target: "Wavenumber.Geometer.Contracts.ModelTessellationA0.MeshCollectionAttachment",
        },
        optional: false,
        constraints: {},
      },
      source_sha256: {
        type: { kind: "primitive", name: "string" },
        optional: false,
        constraints: { min_length: 64, max_length: 64 },
      },
      meshes: {
        type: { kind: "primitive", name: "uint32" },
        optional: false,
        constraints: { min_value: 1, max_value: 65536 },
      },
      triangles: {
        type: { kind: "primitive", name: "uint32" },
        optional: false,
        constraints: { min_value: 1, max_value: 2000000 },
      },
      warnings: {
        type: { kind: "array", element: { kind: "primitive", name: "string" } },
        optional: false,
        constraints: { max_items: 256 },
      },
    },
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
        target: "Wavenumber.Geometer.Contracts.ModelTessellationA0.ModelTessellationResultA0",
      },
      {
        kind: "reference",
        target: "Wavenumber.Geometer.Contracts.ModelBoundsA0.ModelBoundsResultA0",
      },
      {
        kind: "reference",
        target: "Wavenumber.Geometer.Contracts.HlrProjectionA0.HlrProjectionResultA0",
      },
      {
        kind: "reference",
        target: "Wavenumber.Geometer.Contracts.Common.PackedAttachmentProjectionA0",
      },
      {
        kind: "reference",
        target: "Wavenumber.Geometer.Contracts.StepTopologyA0.StepTopologyOpenResultA0",
      },
      {
        kind: "reference",
        target: "Wavenumber.Geometer.Contracts.StepTopologyA0.StepTopologyCloseResultA0",
      },
      {
        kind: "reference",
        target: "Wavenumber.Geometer.Contracts.StepTopologyA0.StepTopologyInspectResultA0",
      },
      {
        kind: "reference",
        target: "Wavenumber.Geometer.Contracts.StepTopologyA0.StepTopologyRenderResultA0",
      },
      {
        kind: "reference",
        target: "Wavenumber.Geometer.Contracts.StepTopologyA0.StepTopologyResolveHitResultA0",
      },
      {
        kind: "reference",
        target:
          "Wavenumber.Geometer.Contracts.StepTopologyA0.StepTopologyApplyLogicalGroupsResultA0",
      },
      {
        kind: "reference",
        target:
          "Wavenumber.Geometer.Contracts.StepTopologyA0.StepTopologyApplyMetadataProbesResultA0",
      },
      {
        kind: "reference",
        target:
          "Wavenumber.Geometer.Contracts.StepTopologyA0.StepTopologyCheckpointEditJournalResultA0",
      },
      {
        kind: "reference",
        target: "Wavenumber.Geometer.Contracts.StepTopologyA0.StepTopologyApplyHierarchyResultA0",
      },
      {
        kind: "reference",
        target: "Wavenumber.Geometer.Contracts.StepTopologyA0.StepTopologySaveResultA0",
      },
      {
        kind: "reference",
        target: "Wavenumber.Geometer.Contracts.StepTopologyA0.StepTopologyRestoreResultA0",
      },
      {
        kind: "reference",
        target: "Wavenumber.Geometer.Contracts.StepTopologyA0.StepTopologyAnalyzeRecoveryResultA0",
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
  "Wavenumber.Geometer.Contracts.StepTopologyA0.AttachMetadataProbeCommand": {
    kind: "object",
    properties: {
      kind: {
        type: { kind: "literal", value_type: "string", value: "attach" },
        optional: false,
        constraints: {},
      },
      authored_id: {
        type: { kind: "primitive", name: "string" },
        optional: false,
        constraints: { min_length: 28, max_length: 128 },
      },
      target: {
        type: {
          kind: "reference",
          target: "Wavenumber.Geometer.Contracts.StepTopologyA0.MetadataProbeTarget",
        },
        optional: false,
        constraints: {},
      },
      key: {
        type: { kind: "primitive", name: "string" },
        optional: false,
        constraints: { min_length: 32, max_length: 128 },
      },
      value: {
        type: { kind: "primitive", name: "string" },
        optional: false,
        constraints: { min_length: 1, max_length: 4096 },
      },
    },
  },
  "Wavenumber.Geometer.Contracts.StepTopologyA0.BodyProbeTarget": {
    kind: "object",
    properties: {
      kind: {
        type: { kind: "literal", value_type: "string", value: "body" },
        optional: false,
        constraints: {},
      },
      target_handle: {
        type: { kind: "primitive", name: "string" },
        optional: false,
        constraints: { min_length: 68, max_length: 68 },
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
      shell_count: {
        type: { kind: "primitive", name: "uint32" },
        optional: false,
        constraints: { max_value: 250000 },
      },
      face_count: {
        type: { kind: "primitive", name: "uint32" },
        optional: false,
        constraints: { max_value: 1000000 },
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
  "Wavenumber.Geometer.Contracts.StepTopologyA0.CarrierCapability": {
    kind: "object",
    properties: {
      carrier: {
        type: {
          kind: "reference",
          target: "Wavenumber.Geometer.Contracts.StepTopologyA0.PersistenceCarrier",
        },
        optional: false,
        constraints: {},
      },
      save: {
        type: {
          kind: "reference",
          target: "Wavenumber.Geometer.Contracts.StepTopologyA0.CarrierSupportState",
        },
        optional: false,
        constraints: {},
      },
      restore: {
        type: {
          kind: "reference",
          target: "Wavenumber.Geometer.Contracts.StepTopologyA0.CarrierSupportState",
        },
        optional: false,
        constraints: {},
      },
      authored_payload: {
        type: {
          kind: "reference",
          target: "Wavenumber.Geometer.Contracts.StepTopologyA0.CarrierSupportState",
        },
        optional: false,
        constraints: {},
      },
      topology_links: {
        type: {
          kind: "reference",
          target: "Wavenumber.Geometer.Contracts.StepTopologyA0.CarrierSupportState",
        },
        optional: false,
        constraints: {},
      },
      notes: {
        type: {
          kind: "array",
          element: {
            kind: "reference",
            target: "Wavenumber.Geometer.Contracts.StepTopologyA0.CarrierCapabilityNote",
          },
        },
        optional: false,
        constraints: { max_items: 16 },
      },
    },
  },
  "Wavenumber.Geometer.Contracts.StepTopologyA0.CarrierCapabilityNote": {
    kind: "object",
    properties: {
      value: {
        type: { kind: "primitive", name: "string" },
        optional: false,
        constraints: { min_length: 1, max_length: 256 },
      },
    },
  },
  "Wavenumber.Geometer.Contracts.StepTopologyA0.CarrierSupportState": {
    kind: "enum",
    values: ["supported", "experimental", "unsupported"],
  },
  "Wavenumber.Geometer.Contracts.StepTopologyA0.ComponentOccurrenceProbeTarget": {
    kind: "object",
    properties: {
      kind: {
        type: { kind: "literal", value_type: "string", value: "occurrence" },
        optional: false,
        constraints: {},
      },
      target_handle: {
        type: { kind: "primitive", name: "string" },
        optional: false,
        constraints: { min_length: 68, max_length: 68 },
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
  "Wavenumber.Geometer.Contracts.StepTopologyA0.CreateHierarchyAssemblyCommand": {
    kind: "object",
    properties: {
      kind: {
        type: { kind: "literal", value_type: "string", value: "create_assembly" },
        optional: false,
        constraints: {},
      },
      authored_id: {
        type: { kind: "primitive", name: "string" },
        optional: false,
        constraints: { min_length: 28, max_length: 128 },
      },
      name: {
        type: { kind: "primitive", name: "string" },
        optional: false,
        constraints: { min_length: 1, max_length: 4096 },
      },
    },
  },
  "Wavenumber.Geometer.Contracts.StepTopologyA0.CreateHierarchyOccurrenceCommand": {
    kind: "object",
    properties: {
      kind: {
        type: { kind: "literal", value_type: "string", value: "create_occurrence" },
        optional: false,
        constraints: {},
      },
      authored_id: {
        type: { kind: "primitive", name: "string" },
        optional: false,
        constraints: { min_length: 28, max_length: 128 },
      },
      child_authored_id: {
        type: { kind: "primitive", name: "string" },
        optional: false,
        constraints: { min_length: 28, max_length: 128 },
      },
      parent_assembly_authored_id: {
        type: { kind: "primitive", name: "string" },
        optional: false,
        constraints: { min_length: 28, max_length: 128 },
      },
      transform: {
        type: { kind: "array", element: { kind: "primitive", name: "float64" } },
        optional: false,
        constraints: { min_items: 12, max_items: 12 },
      },
    },
  },
  "Wavenumber.Geometer.Contracts.StepTopologyA0.CreateHierarchyProductCommand": {
    kind: "object",
    properties: {
      kind: {
        type: { kind: "literal", value_type: "string", value: "create_product" },
        optional: false,
        constraints: {},
      },
      authored_id: {
        type: { kind: "primitive", name: "string" },
        optional: false,
        constraints: { min_length: 28, max_length: 128 },
      },
      name: {
        type: { kind: "primitive", name: "string" },
        optional: false,
        constraints: { min_length: 1, max_length: 4096 },
      },
      source_kind: {
        type: {
          kind: "reference",
          target: "Wavenumber.Geometer.Contracts.StepTopologyA0.HierarchySourceKind",
        },
        optional: false,
        constraints: {},
      },
      source_handle: {
        type: { kind: "primitive", name: "string" },
        optional: false,
        constraints: { min_length: 68, max_length: 68 },
      },
    },
  },
  "Wavenumber.Geometer.Contracts.StepTopologyA0.CreateLogicalGroupCommand": {
    kind: "object",
    properties: {
      kind: {
        type: { kind: "literal", value_type: "string", value: "create" },
        optional: false,
        constraints: {},
      },
      authored_id: {
        type: { kind: "primitive", name: "string" },
        optional: false,
        constraints: { min_length: 28, max_length: 128 },
      },
      name: {
        type: { kind: "primitive", name: "string" },
        optional: false,
        constraints: { min_length: 1, max_length: 4096 },
      },
      member_handles: {
        type: { kind: "array", element: { kind: "primitive", name: "string" } },
        optional: false,
        constraints: { min_items: 1, max_items: 100000 },
      },
    },
  },
  "Wavenumber.Geometer.Contracts.StepTopologyA0.DefinitionProbeTarget": {
    kind: "object",
    properties: {
      kind: {
        type: { kind: "literal", value_type: "string", value: "definition" },
        optional: false,
        constraints: {},
      },
      target_handle: {
        type: { kind: "primitive", name: "string" },
        optional: false,
        constraints: { min_length: 68, max_length: 68 },
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
  "Wavenumber.Geometer.Contracts.StepTopologyA0.DocumentProbeTarget": {
    kind: "object",
    properties: {
      kind: {
        type: { kind: "literal", value_type: "string", value: "document" },
        optional: false,
        constraints: {},
      },
    },
  },
  "Wavenumber.Geometer.Contracts.StepTopologyA0.EditJournalAttachmentDescriptor": {
    kind: "object",
    properties: {
      name: {
        type: { kind: "literal", value_type: "string", value: "edit_journal" },
        optional: false,
        constraints: {},
      },
      media_type: {
        type: {
          kind: "literal",
          value_type: "string",
          value: "application/vnd.wavenumber.geometer.step-topology-edit-journal",
        },
        optional: false,
        constraints: {},
      },
      format: {
        type: {
          kind: "literal",
          value_type: "string",
          value: "geometer.step_topology_edit_journal.a0",
        },
        optional: false,
        constraints: {},
      },
      bytes: {
        type: { kind: "primitive", name: "uint32" },
        optional: false,
        constraints: { min_value: 1, max_value: 67108864 },
      },
      sha256: {
        type: { kind: "primitive", name: "string" },
        optional: false,
        constraints: { min_length: 64, max_length: 64 },
      },
    },
  },
  "Wavenumber.Geometer.Contracts.StepTopologyA0.EditJournalPersistenceArtifact": {
    kind: "object",
    properties: {
      carrier: {
        type: { kind: "literal", value_type: "string", value: "edit_journal" },
        optional: false,
        constraints: {},
      },
      name: {
        type: { kind: "literal", value_type: "string", value: "state_artifact" },
        optional: false,
        constraints: {},
      },
      media_type: {
        type: {
          kind: "literal",
          value_type: "string",
          value: "application/vnd.wavenumber.geometer.step-topology-edit-journal",
        },
        optional: false,
        constraints: {},
      },
      format: {
        type: {
          kind: "literal",
          value_type: "string",
          value: "geometer.step_topology_edit_journal.a0",
        },
        optional: false,
        constraints: {},
      },
      bytes: {
        type: { kind: "primitive", name: "uint32" },
        optional: false,
        constraints: { min_value: 1, max_value: 67108864 },
      },
      sha256: {
        type: { kind: "primitive", name: "string" },
        optional: false,
        constraints: { min_length: 64, max_length: 64 },
      },
    },
  },
  "Wavenumber.Geometer.Contracts.StepTopologyA0.EditJournalReplayPreconditions": {
    kind: "object",
    properties: {
      source_sha256: {
        type: { kind: "primitive", name: "string" },
        optional: false,
        constraints: { min_length: 64, max_length: 64 },
      },
      source_brep_sha256: {
        type: { kind: "primitive", name: "string" },
        optional: false,
        constraints: { min_length: 64, max_length: 64 },
      },
      target_inventory_sha256: {
        type: { kind: "primitive", name: "string" },
        optional: false,
        constraints: { min_length: 64, max_length: 64 },
      },
      occt_version: {
        type: { kind: "primitive", name: "string" },
        optional: false,
        constraints: { min_length: 1, max_length: 64 },
      },
      transaction_count: {
        type: { kind: "primitive", name: "uint32" },
        optional: false,
        constraints: { max_value: 100000 },
      },
    },
  },
  "Wavenumber.Geometer.Contracts.StepTopologyA0.EraseHierarchyNodeCommand": {
    kind: "object",
    properties: {
      kind: {
        type: { kind: "literal", value_type: "string", value: "erase_node" },
        optional: false,
        constraints: {},
      },
      authored_id: {
        type: { kind: "primitive", name: "string" },
        optional: false,
        constraints: { min_length: 28, max_length: 128 },
      },
      expected_revision: {
        type: { kind: "primitive", name: "uint32" },
        optional: false,
        constraints: { min_value: 1 },
      },
    },
  },
  "Wavenumber.Geometer.Contracts.StepTopologyA0.EraseHierarchyOccurrenceCommand": {
    kind: "object",
    properties: {
      kind: {
        type: { kind: "literal", value_type: "string", value: "erase_occurrence" },
        optional: false,
        constraints: {},
      },
      authored_id: {
        type: { kind: "primitive", name: "string" },
        optional: false,
        constraints: { min_length: 28, max_length: 128 },
      },
      expected_revision: {
        type: { kind: "primitive", name: "uint32" },
        optional: false,
        constraints: { min_value: 1 },
      },
    },
  },
  "Wavenumber.Geometer.Contracts.StepTopologyA0.EraseLogicalGroupCommand": {
    kind: "object",
    properties: {
      kind: {
        type: { kind: "literal", value_type: "string", value: "erase" },
        optional: false,
        constraints: {},
      },
      authored_id: {
        type: { kind: "primitive", name: "string" },
        optional: false,
        constraints: { min_length: 28, max_length: 128 },
      },
      expected_revision: {
        type: { kind: "primitive", name: "uint32" },
        optional: false,
        constraints: { min_value: 1 },
      },
    },
  },
  "Wavenumber.Geometer.Contracts.StepTopologyA0.EraseMetadataProbeCommand": {
    kind: "object",
    properties: {
      kind: {
        type: { kind: "literal", value_type: "string", value: "erase" },
        optional: false,
        constraints: {},
      },
      authored_id: {
        type: { kind: "primitive", name: "string" },
        optional: false,
        constraints: { min_length: 28, max_length: 128 },
      },
      expected_revision: {
        type: { kind: "primitive", name: "uint32" },
        optional: false,
        constraints: { min_value: 1 },
      },
    },
  },
  "Wavenumber.Geometer.Contracts.StepTopologyA0.FaceProbeTarget": {
    kind: "object",
    properties: {
      kind: {
        type: { kind: "literal", value_type: "string", value: "face" },
        optional: false,
        constraints: {},
      },
      target_handle: {
        type: { kind: "primitive", name: "string" },
        optional: false,
        constraints: { min_length: 68, max_length: 68 },
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
      body_count: {
        type: { kind: "primitive", name: "uint32" },
        optional: false,
        constraints: { max_value: 100000 },
      },
      shell_count: {
        type: { kind: "primitive", name: "uint32" },
        optional: false,
        constraints: { max_value: 250000 },
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
  "Wavenumber.Geometer.Contracts.StepTopologyA0.HierarchyCommand": {
    kind: "union",
    variants: [
      {
        kind: "reference",
        target: "Wavenumber.Geometer.Contracts.StepTopologyA0.CreateHierarchyProductCommand",
      },
      {
        kind: "reference",
        target: "Wavenumber.Geometer.Contracts.StepTopologyA0.CreateHierarchyAssemblyCommand",
      },
      {
        kind: "reference",
        target: "Wavenumber.Geometer.Contracts.StepTopologyA0.CreateHierarchyOccurrenceCommand",
      },
      {
        kind: "reference",
        target: "Wavenumber.Geometer.Contracts.StepTopologyA0.ReparentHierarchyOccurrenceCommand",
      },
      {
        kind: "reference",
        target: "Wavenumber.Geometer.Contracts.StepTopologyA0.RenameHierarchyNodeCommand",
      },
      {
        kind: "reference",
        target: "Wavenumber.Geometer.Contracts.StepTopologyA0.EraseHierarchyOccurrenceCommand",
      },
      {
        kind: "reference",
        target: "Wavenumber.Geometer.Contracts.StepTopologyA0.EraseHierarchyNodeCommand",
      },
    ],
  },
  "Wavenumber.Geometer.Contracts.StepTopologyA0.HierarchyNode": {
    kind: "object",
    properties: {
      authored_id: {
        type: { kind: "primitive", name: "string" },
        optional: false,
        constraints: { min_length: 28, max_length: 128 },
      },
      revision: {
        type: { kind: "primitive", name: "uint32" },
        optional: false,
        constraints: { min_value: 1 },
      },
      kind: {
        type: {
          kind: "reference",
          target: "Wavenumber.Geometer.Contracts.StepTopologyA0.HierarchyNodeKind",
        },
        optional: false,
        constraints: {},
      },
      name: {
        type: { kind: "primitive", name: "string" },
        optional: false,
        constraints: { min_length: 1, max_length: 4096 },
      },
      source_kind: {
        type: {
          kind: "reference",
          target: "Wavenumber.Geometer.Contracts.StepTopologyA0.HierarchySourceKind",
        },
        optional: true,
        constraints: {},
      },
      source_handle: {
        type: { kind: "primitive", name: "string" },
        optional: true,
        constraints: { min_length: 68, max_length: 68 },
      },
    },
  },
  "Wavenumber.Geometer.Contracts.StepTopologyA0.HierarchyNodeKind": {
    kind: "enum",
    values: ["product", "assembly"],
  },
  "Wavenumber.Geometer.Contracts.StepTopologyA0.HierarchyOccurrence": {
    kind: "object",
    properties: {
      authored_id: {
        type: { kind: "primitive", name: "string" },
        optional: false,
        constraints: { min_length: 28, max_length: 128 },
      },
      revision: {
        type: { kind: "primitive", name: "uint32" },
        optional: false,
        constraints: { min_value: 1 },
      },
      child_authored_id: {
        type: { kind: "primitive", name: "string" },
        optional: false,
        constraints: { min_length: 28, max_length: 128 },
      },
      parent_assembly_authored_id: {
        type: { kind: "primitive", name: "string" },
        optional: false,
        constraints: { min_length: 28, max_length: 128 },
      },
      transform: {
        type: { kind: "array", element: { kind: "primitive", name: "float64" } },
        optional: false,
        constraints: { min_items: 12, max_items: 12 },
      },
    },
  },
  "Wavenumber.Geometer.Contracts.StepTopologyA0.HierarchySourceKind": {
    kind: "enum",
    values: ["definition", "body"],
  },
  "Wavenumber.Geometer.Contracts.StepTopologyA0.HierarchyState": {
    kind: "object",
    properties: {
      hierarchy_revision: {
        type: { kind: "primitive", name: "uint32" },
        optional: false,
        constraints: {},
      },
      source_brep_sha256: {
        type: { kind: "primitive", name: "string" },
        optional: false,
        constraints: { min_length: 64, max_length: 64 },
      },
      nodes: {
        type: {
          kind: "array",
          element: {
            kind: "reference",
            target: "Wavenumber.Geometer.Contracts.StepTopologyA0.HierarchyNode",
          },
        },
        optional: false,
        constraints: { max_items: 10000 },
      },
      occurrences: {
        type: {
          kind: "array",
          element: {
            kind: "reference",
            target: "Wavenumber.Geometer.Contracts.StepTopologyA0.HierarchyOccurrence",
          },
        },
        optional: false,
        constraints: { max_items: 100000 },
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
      memberships: {
        type: { kind: "primitive", name: "uint32" },
        optional: false,
        constraints: { max_value: 5000000 },
      },
    },
  },
  "Wavenumber.Geometer.Contracts.StepTopologyA0.JsonSidecarPersistenceArtifact": {
    kind: "object",
    properties: {
      carrier: {
        type: { kind: "literal", value_type: "string", value: "json_sidecar" },
        optional: false,
        constraints: {},
      },
      name: {
        type: { kind: "literal", value_type: "string", value: "state_artifact" },
        optional: false,
        constraints: {},
      },
      media_type: {
        type: {
          kind: "literal",
          value_type: "string",
          value: "application/vnd.wavenumber.geometer.step-topology-sidecar+json",
        },
        optional: false,
        constraints: {},
      },
      format: {
        type: { kind: "literal", value_type: "string", value: "geometer.step_topology_sidecar.a0" },
        optional: false,
        constraints: {},
      },
      bytes: {
        type: { kind: "primitive", name: "uint32" },
        optional: false,
        constraints: { min_value: 1, max_value: 67108864 },
      },
      sha256: {
        type: { kind: "primitive", name: "string" },
        optional: false,
        constraints: { min_length: 64, max_length: 64 },
      },
    },
  },
  "Wavenumber.Geometer.Contracts.StepTopologyA0.LogicalGroup": {
    kind: "object",
    properties: {
      authored_id: {
        type: { kind: "primitive", name: "string" },
        optional: false,
        constraints: { min_length: 28, max_length: 128 },
      },
      revision: {
        type: { kind: "primitive", name: "uint32" },
        optional: false,
        constraints: { min_value: 1 },
      },
      name: {
        type: { kind: "primitive", name: "string" },
        optional: false,
        constraints: { min_length: 1, max_length: 4096 },
      },
      members: {
        type: {
          kind: "array",
          element: {
            kind: "reference",
            target: "Wavenumber.Geometer.Contracts.StepTopologyA0.LogicalGroupMember",
          },
        },
        optional: false,
        constraints: { min_items: 1, max_items: 100000 },
      },
    },
  },
  "Wavenumber.Geometer.Contracts.StepTopologyA0.LogicalGroupCommand": {
    kind: "union",
    variants: [
      {
        kind: "reference",
        target: "Wavenumber.Geometer.Contracts.StepTopologyA0.CreateLogicalGroupCommand",
      },
      {
        kind: "reference",
        target: "Wavenumber.Geometer.Contracts.StepTopologyA0.RenameLogicalGroupCommand",
      },
      {
        kind: "reference",
        target: "Wavenumber.Geometer.Contracts.StepTopologyA0.ReplaceLogicalGroupMembersCommand",
      },
      {
        kind: "reference",
        target: "Wavenumber.Geometer.Contracts.StepTopologyA0.EraseLogicalGroupCommand",
      },
    ],
  },
  "Wavenumber.Geometer.Contracts.StepTopologyA0.LogicalGroupMember": {
    kind: "object",
    properties: {
      kind: {
        type: {
          kind: "reference",
          target: "Wavenumber.Geometer.Contracts.StepTopologyA0.LogicalGroupMemberKind",
        },
        optional: false,
        constraints: {},
      },
      target_handle: {
        type: { kind: "primitive", name: "string" },
        optional: false,
        constraints: { min_length: 68, max_length: 68 },
      },
    },
  },
  "Wavenumber.Geometer.Contracts.StepTopologyA0.LogicalGroupMemberKind": {
    kind: "enum",
    values: ["body", "face"],
  },
  "Wavenumber.Geometer.Contracts.StepTopologyA0.LogicalGroupProbeTarget": {
    kind: "object",
    properties: {
      kind: {
        type: { kind: "literal", value_type: "string", value: "logical_group" },
        optional: false,
        constraints: {},
      },
      group_authored_id: {
        type: { kind: "primitive", name: "string" },
        optional: false,
        constraints: { min_length: 28, max_length: 128 },
      },
    },
  },
  "Wavenumber.Geometer.Contracts.StepTopologyA0.MetadataProbe": {
    kind: "object",
    properties: {
      authored_id: {
        type: { kind: "primitive", name: "string" },
        optional: false,
        constraints: { min_length: 28, max_length: 128 },
      },
      revision: {
        type: { kind: "primitive", name: "uint32" },
        optional: false,
        constraints: { min_value: 1 },
      },
      target: {
        type: {
          kind: "reference",
          target: "Wavenumber.Geometer.Contracts.StepTopologyA0.MetadataProbeTarget",
        },
        optional: false,
        constraints: {},
      },
      key: {
        type: { kind: "primitive", name: "string" },
        optional: false,
        constraints: { min_length: 32, max_length: 128 },
      },
      value: {
        type: { kind: "primitive", name: "string" },
        optional: false,
        constraints: { min_length: 1, max_length: 4096 },
      },
    },
  },
  "Wavenumber.Geometer.Contracts.StepTopologyA0.MetadataProbeCommand": {
    kind: "union",
    variants: [
      {
        kind: "reference",
        target: "Wavenumber.Geometer.Contracts.StepTopologyA0.AttachMetadataProbeCommand",
      },
      {
        kind: "reference",
        target: "Wavenumber.Geometer.Contracts.StepTopologyA0.ReplaceMetadataProbeCommand",
      },
      {
        kind: "reference",
        target: "Wavenumber.Geometer.Contracts.StepTopologyA0.EraseMetadataProbeCommand",
      },
    ],
  },
  "Wavenumber.Geometer.Contracts.StepTopologyA0.MetadataProbeTarget": {
    kind: "union",
    variants: [
      {
        kind: "reference",
        target: "Wavenumber.Geometer.Contracts.StepTopologyA0.DocumentProbeTarget",
      },
      {
        kind: "reference",
        target: "Wavenumber.Geometer.Contracts.StepTopologyA0.DefinitionProbeTarget",
      },
      {
        kind: "reference",
        target: "Wavenumber.Geometer.Contracts.StepTopologyA0.RootOccurrenceProbeTarget",
      },
      {
        kind: "reference",
        target: "Wavenumber.Geometer.Contracts.StepTopologyA0.ComponentOccurrenceProbeTarget",
      },
      { kind: "reference", target: "Wavenumber.Geometer.Contracts.StepTopologyA0.BodyProbeTarget" },
      { kind: "reference", target: "Wavenumber.Geometer.Contracts.StepTopologyA0.FaceProbeTarget" },
      {
        kind: "reference",
        target: "Wavenumber.Geometer.Contracts.StepTopologyA0.LogicalGroupProbeTarget",
      },
    ],
  },
  "Wavenumber.Geometer.Contracts.StepTopologyA0.MutationSessionState": {
    kind: "object",
    properties: {
      session: {
        type: {
          kind: "reference",
          target: "Wavenumber.Geometer.Contracts.StepTopologyA0.SessionReference",
        },
        optional: false,
        constraints: {},
      },
      edit_journal_revision: {
        type: { kind: "primitive", name: "uint32" },
        optional: false,
        constraints: { max_value: 100000 },
      },
      accounted_string_bytes: {
        type: { kind: "primitive", name: "uint32" },
        optional: false,
        constraints: { max_value: 16777216 },
      },
      estimated_resident_bytes: {
        type: { kind: "primitive", name: "uint32" },
        optional: false,
        constraints: { max_value: 536870912 },
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
  "Wavenumber.Geometer.Contracts.StepTopologyA0.PersistenceCarrier": {
    kind: "enum",
    values: ["xbf", "xml_xcaf", "step_ap242", "json_sidecar", "edit_journal"],
  },
  "Wavenumber.Geometer.Contracts.StepTopologyA0.RecoveryCandidate": {
    kind: "object",
    properties: {
      target_handle: {
        type: { kind: "primitive", name: "string" },
        optional: false,
        constraints: { min_length: 68, max_length: 68 },
      },
      kind: {
        type: {
          kind: "reference",
          target: "Wavenumber.Geometer.Contracts.StepTopologyA0.LogicalGroupMemberKind",
        },
        optional: false,
        constraints: {},
      },
      authored_target_id: {
        type: { kind: "primitive", name: "string" },
        optional: true,
        constraints: { min_length: 28, max_length: 128 },
      },
      topology_link_verified: {
        type: { kind: "primitive", name: "boolean" },
        optional: false,
        constraints: {},
      },
      carrier_locator: {
        type: { kind: "primitive", name: "string" },
        optional: false,
        constraints: { max_length: 4096 },
      },
      carrier_locator_validated: {
        type: { kind: "primitive", name: "boolean" },
        optional: false,
        constraints: {},
      },
      carrier_record: {
        type: { kind: "primitive", name: "string" },
        optional: false,
        constraints: { max_length: 4096 },
      },
      lineage: {
        type: {
          kind: "reference",
          target: "Wavenumber.Geometer.Contracts.StepTopologyA0.RecoveryLineage",
        },
        optional: false,
        constraints: {},
      },
      fingerprint: {
        type: {
          kind: "reference",
          target: "Wavenumber.Geometer.Contracts.StepTopologyA0.RecoveryFingerprint",
        },
        optional: true,
        constraints: {},
      },
    },
  },
  "Wavenumber.Geometer.Contracts.StepTopologyA0.RecoveryCarrierRecord": {
    kind: "object",
    properties: {
      value: {
        type: { kind: "primitive", name: "string" },
        optional: false,
        constraints: { min_length: 1, max_length: 4096 },
      },
    },
  },
  "Wavenumber.Geometer.Contracts.StepTopologyA0.RecoveryComparedField": {
    kind: "object",
    properties: {
      value: {
        type: { kind: "primitive", name: "string" },
        optional: false,
        constraints: { min_length: 1, max_length: 128 },
      },
    },
  },
  "Wavenumber.Geometer.Contracts.StepTopologyA0.RecoveryConfidence": {
    kind: "enum",
    values: ["high", "medium", "low", "none"],
  },
  "Wavenumber.Geometer.Contracts.StepTopologyA0.RecoveryEvidence": {
    kind: "object",
    properties: {
      candidate_count: {
        type: { kind: "primitive", name: "uint32" },
        optional: false,
        constraints: { max_value: 16 },
      },
      matching_candidate_count: {
        type: { kind: "primitive", name: "uint32" },
        optional: false,
        constraints: { max_value: 16 },
      },
      compared_fields: {
        type: {
          kind: "array",
          element: {
            kind: "reference",
            target: "Wavenumber.Geometer.Contracts.StepTopologyA0.RecoveryComparedField",
          },
        },
        optional: false,
        constraints: { max_items: 16 },
      },
      tolerances: {
        type: {
          kind: "reference",
          target: "Wavenumber.Geometer.Contracts.StepTopologyA0.RecoveryTolerances",
        },
        optional: false,
        constraints: {},
      },
      carrier_records: {
        type: {
          kind: "array",
          element: {
            kind: "reference",
            target: "Wavenumber.Geometer.Contracts.StepTopologyA0.RecoveryCarrierRecord",
          },
        },
        optional: false,
        constraints: { max_items: 16 },
      },
      rejected_alternatives: {
        type: {
          kind: "array",
          element: {
            kind: "reference",
            target: "Wavenumber.Geometer.Contracts.StepTopologyA0.RecoveryRejectedAlternative",
          },
        },
        optional: false,
        constraints: { max_items: 16 },
      },
    },
  },
  "Wavenumber.Geometer.Contracts.StepTopologyA0.RecoveryFingerprint": {
    kind: "object",
    properties: {
      normalized_length_unit: {
        type: { kind: "literal", value_type: "string", value: "millimeter" },
        optional: false,
        constraints: {},
      },
      coordinate_frame: {
        type: { kind: "primitive", name: "string" },
        optional: false,
        constraints: { min_length: 1, max_length: 128 },
      },
      occurrence_context: {
        type: { kind: "primitive", name: "string" },
        optional: false,
        constraints: { min_length: 1, max_length: 128 },
      },
      geometry_kind: {
        type: { kind: "primitive", name: "string" },
        optional: false,
        constraints: { min_length: 1, max_length: 128 },
      },
      area_mm2: {
        type: { kind: "primitive", name: "float64" },
        optional: false,
        constraints: { min_value: 0 },
      },
      volume_mm3: {
        type: { kind: "primitive", name: "float64" },
        optional: false,
        constraints: { min_value: 0 },
      },
      centroid_mm: {
        type: { kind: "array", element: { kind: "primitive", name: "float64" } },
        optional: false,
        constraints: { min_items: 3, max_items: 3 },
      },
      bounds_mm: {
        type: { kind: "array", element: { kind: "primitive", name: "float64" } },
        optional: false,
        constraints: { min_items: 6, max_items: 6 },
      },
      adjacency_sha256: {
        type: { kind: "primitive", name: "string" },
        optional: false,
        constraints: { min_length: 64, max_length: 64 },
      },
    },
  },
  "Wavenumber.Geometer.Contracts.StepTopologyA0.RecoveryGroupCompleteness": {
    kind: "enum",
    values: ["fully_recovered", "partially_recovered", "unrecovered", "unsupported"],
  },
  "Wavenumber.Geometer.Contracts.StepTopologyA0.RecoveryGroupRequest": {
    kind: "object",
    properties: {
      group_authored_id: {
        type: { kind: "primitive", name: "string" },
        optional: false,
        constraints: { min_length: 28, max_length: 128 },
      },
      provenance: {
        type: {
          kind: "reference",
          target: "Wavenumber.Geometer.Contracts.StepTopologyA0.RecoveryProvenance",
        },
        optional: false,
        constraints: {},
      },
      tolerances: {
        type: {
          kind: "reference",
          target: "Wavenumber.Geometer.Contracts.StepTopologyA0.RecoveryTolerances",
        },
        optional: false,
        constraints: {},
      },
      members: {
        type: {
          kind: "array",
          element: {
            kind: "reference",
            target: "Wavenumber.Geometer.Contracts.StepTopologyA0.RecoveryMemberRequest",
          },
        },
        optional: false,
        constraints: { min_items: 1, max_items: 256 },
      },
    },
  },
  "Wavenumber.Geometer.Contracts.StepTopologyA0.RecoveryGroupResult": {
    kind: "object",
    properties: {
      group_authored_id: {
        type: { kind: "primitive", name: "string" },
        optional: false,
        constraints: { min_length: 28, max_length: 128 },
      },
      provenance: {
        type: {
          kind: "reference",
          target: "Wavenumber.Geometer.Contracts.StepTopologyA0.RecoveryProvenance",
        },
        optional: false,
        constraints: {},
      },
      resolution_state: {
        type: {
          kind: "reference",
          target: "Wavenumber.Geometer.Contracts.StepTopologyA0.RecoveryResolutionState",
        },
        optional: false,
        constraints: {},
      },
      completeness: {
        type: {
          kind: "reference",
          target: "Wavenumber.Geometer.Contracts.StepTopologyA0.RecoveryGroupCompleteness",
        },
        optional: false,
        constraints: {},
      },
      resolved_member_count: {
        type: { kind: "primitive", name: "uint32" },
        optional: false,
        constraints: { max_value: 256 },
      },
      ambiguous_member_count: {
        type: { kind: "primitive", name: "uint32" },
        optional: false,
        constraints: { max_value: 256 },
      },
      unresolved_member_count: {
        type: { kind: "primitive", name: "uint32" },
        optional: false,
        constraints: { max_value: 256 },
      },
      unsupported_member_count: {
        type: { kind: "primitive", name: "uint32" },
        optional: false,
        constraints: { max_value: 256 },
      },
      members: {
        type: {
          kind: "array",
          element: {
            kind: "reference",
            target: "Wavenumber.Geometer.Contracts.StepTopologyA0.RecoveryMemberResult",
          },
        },
        optional: false,
        constraints: { min_items: 1, max_items: 256 },
      },
    },
  },
  "Wavenumber.Geometer.Contracts.StepTopologyA0.RecoveryLineage": {
    kind: "enum",
    values: ["none", "split_from_source", "merged_from_sources"],
  },
  "Wavenumber.Geometer.Contracts.StepTopologyA0.RecoveryMemberRequest": {
    kind: "object",
    properties: {
      member_record_id: {
        type: { kind: "primitive", name: "string" },
        optional: false,
        constraints: { min_length: 28, max_length: 128 },
      },
      kind: {
        type: {
          kind: "reference",
          target: "Wavenumber.Geometer.Contracts.StepTopologyA0.LogicalGroupMemberKind",
        },
        optional: false,
        constraints: {},
      },
      authored_target_id: {
        type: { kind: "primitive", name: "string" },
        optional: false,
        constraints: { max_length: 128 },
      },
      carrier_locator: {
        type: { kind: "primitive", name: "string" },
        optional: false,
        constraints: { max_length: 4096 },
      },
      source_fingerprint: {
        type: {
          kind: "reference",
          target: "Wavenumber.Geometer.Contracts.StepTopologyA0.RecoveryFingerprint",
        },
        optional: true,
        constraints: {},
      },
      candidates: {
        type: {
          kind: "array",
          element: {
            kind: "reference",
            target: "Wavenumber.Geometer.Contracts.StepTopologyA0.RecoveryCandidate",
          },
        },
        optional: false,
        constraints: { max_items: 16 },
      },
    },
  },
  "Wavenumber.Geometer.Contracts.StepTopologyA0.RecoveryMemberResult": {
    kind: "object",
    properties: {
      member_record_id: {
        type: { kind: "primitive", name: "string" },
        optional: false,
        constraints: { min_length: 28, max_length: 128 },
      },
      kind: {
        type: {
          kind: "reference",
          target: "Wavenumber.Geometer.Contracts.StepTopologyA0.LogicalGroupMemberKind",
        },
        optional: false,
        constraints: {},
      },
      authored_target_id: {
        type: { kind: "primitive", name: "string" },
        optional: false,
        constraints: { max_length: 128 },
      },
      resolution_state: {
        type: {
          kind: "reference",
          target: "Wavenumber.Geometer.Contracts.StepTopologyA0.RecoveryResolutionState",
        },
        optional: false,
        constraints: {},
      },
      resolution_method: {
        type: {
          kind: "reference",
          target: "Wavenumber.Geometer.Contracts.StepTopologyA0.RecoveryResolutionMethod",
        },
        optional: false,
        constraints: {},
      },
      topology_comparison: {
        type: {
          kind: "reference",
          target: "Wavenumber.Geometer.Contracts.StepTopologyA0.RecoveryTopologyComparison",
        },
        optional: false,
        constraints: {},
      },
      confidence: {
        type: {
          kind: "reference",
          target: "Wavenumber.Geometer.Contracts.StepTopologyA0.RecoveryConfidence",
        },
        optional: false,
        constraints: {},
      },
      resolved_target_handle: {
        type: { kind: "primitive", name: "string" },
        optional: true,
        constraints: { min_length: 68, max_length: 68 },
      },
      evidence: {
        type: {
          kind: "reference",
          target: "Wavenumber.Geometer.Contracts.StepTopologyA0.RecoveryEvidence",
        },
        optional: false,
        constraints: {},
      },
    },
  },
  "Wavenumber.Geometer.Contracts.StepTopologyA0.RecoveryProvenance": {
    kind: "object",
    properties: {
      source_artifact_sha256: {
        type: { kind: "primitive", name: "string" },
        optional: false,
        constraints: { min_length: 64, max_length: 64 },
      },
      candidate_artifact_sha256: {
        type: { kind: "primitive", name: "string" },
        optional: false,
        constraints: { min_length: 64, max_length: 64 },
      },
      source_occt_version: {
        type: { kind: "primitive", name: "string" },
        optional: false,
        constraints: { min_length: 1, max_length: 64 },
      },
      candidate_occt_version: {
        type: { kind: "primitive", name: "string" },
        optional: false,
        constraints: { min_length: 1, max_length: 64 },
      },
      source_driver: {
        type: { kind: "primitive", name: "string" },
        optional: false,
        constraints: { min_length: 1, max_length: 128 },
      },
      candidate_driver: {
        type: { kind: "primitive", name: "string" },
        optional: false,
        constraints: { min_length: 1, max_length: 128 },
      },
      source_writer_settings: {
        type: { kind: "primitive", name: "string" },
        optional: false,
        constraints: { min_length: 1, max_length: 4096 },
      },
      candidate_writer_settings: {
        type: { kind: "primitive", name: "string" },
        optional: false,
        constraints: { min_length: 1, max_length: 4096 },
      },
      command_provenance: {
        type: { kind: "primitive", name: "string" },
        optional: false,
        constraints: { min_length: 1, max_length: 8192 },
      },
      measured_wall_time_milliseconds: {
        type: { kind: "primitive", name: "float64" },
        optional: false,
        constraints: { min_value: 0 },
      },
    },
  },
  "Wavenumber.Geometer.Contracts.StepTopologyA0.RecoveryRejectedAlternative": {
    kind: "object",
    properties: {
      target_handle: {
        type: { kind: "primitive", name: "string" },
        optional: false,
        constraints: { min_length: 68, max_length: 68 },
      },
      reason: {
        type: { kind: "primitive", name: "string" },
        optional: false,
        constraints: { min_length: 1, max_length: 4096 },
      },
    },
  },
  "Wavenumber.Geometer.Contracts.StepTopologyA0.RecoveryResolutionMethod": {
    kind: "enum",
    values: [
      "authored_id_topology_link",
      "validated_carrier_locator",
      "unique_geometry_adjacency_fingerprint",
      "none",
    ],
  },
  "Wavenumber.Geometer.Contracts.StepTopologyA0.RecoveryResolutionState": {
    kind: "enum",
    values: ["resolved", "ambiguous", "unresolved", "unsupported"],
  },
  "Wavenumber.Geometer.Contracts.StepTopologyA0.RecoveryTolerances": {
    kind: "object",
    properties: {
      length_mm: {
        type: { kind: "primitive", name: "float64" },
        optional: false,
        constraints: { min_value: 1e-9 },
      },
      area_mm2: {
        type: { kind: "primitive", name: "float64" },
        optional: false,
        constraints: { min_value: 1e-9 },
      },
      volume_mm3: {
        type: { kind: "primitive", name: "float64" },
        optional: false,
        constraints: { min_value: 1e-9 },
      },
    },
  },
  "Wavenumber.Geometer.Contracts.StepTopologyA0.RecoveryTopologyComparison": {
    kind: "enum",
    values: [
      "unchanged",
      "relocated",
      "split",
      "merged",
      "otherwise_changed",
      "not_compared",
      "unavailable",
    ],
  },
  "Wavenumber.Geometer.Contracts.StepTopologyA0.RenameHierarchyNodeCommand": {
    kind: "object",
    properties: {
      kind: {
        type: { kind: "literal", value_type: "string", value: "rename_node" },
        optional: false,
        constraints: {},
      },
      authored_id: {
        type: { kind: "primitive", name: "string" },
        optional: false,
        constraints: { min_length: 28, max_length: 128 },
      },
      expected_revision: {
        type: { kind: "primitive", name: "uint32" },
        optional: false,
        constraints: { min_value: 1 },
      },
      name: {
        type: { kind: "primitive", name: "string" },
        optional: false,
        constraints: { min_length: 1, max_length: 4096 },
      },
    },
  },
  "Wavenumber.Geometer.Contracts.StepTopologyA0.RenameLogicalGroupCommand": {
    kind: "object",
    properties: {
      kind: {
        type: { kind: "literal", value_type: "string", value: "rename" },
        optional: false,
        constraints: {},
      },
      authored_id: {
        type: { kind: "primitive", name: "string" },
        optional: false,
        constraints: { min_length: 28, max_length: 128 },
      },
      expected_revision: {
        type: { kind: "primitive", name: "uint32" },
        optional: false,
        constraints: { min_value: 1 },
      },
      name: {
        type: { kind: "primitive", name: "string" },
        optional: false,
        constraints: { min_length: 1, max_length: 4096 },
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
  "Wavenumber.Geometer.Contracts.StepTopologyA0.ReparentHierarchyOccurrenceCommand": {
    kind: "object",
    properties: {
      kind: {
        type: { kind: "literal", value_type: "string", value: "reparent_occurrence" },
        optional: false,
        constraints: {},
      },
      authored_id: {
        type: { kind: "primitive", name: "string" },
        optional: false,
        constraints: { min_length: 28, max_length: 128 },
      },
      expected_revision: {
        type: { kind: "primitive", name: "uint32" },
        optional: false,
        constraints: { min_value: 1 },
      },
      parent_assembly_authored_id: {
        type: { kind: "primitive", name: "string" },
        optional: false,
        constraints: { min_length: 28, max_length: 128 },
      },
      transform: {
        type: { kind: "array", element: { kind: "primitive", name: "float64" } },
        optional: false,
        constraints: { min_items: 12, max_items: 12 },
      },
    },
  },
  "Wavenumber.Geometer.Contracts.StepTopologyA0.ReplaceLogicalGroupMembersCommand": {
    kind: "object",
    properties: {
      kind: {
        type: { kind: "literal", value_type: "string", value: "replace_members" },
        optional: false,
        constraints: {},
      },
      authored_id: {
        type: { kind: "primitive", name: "string" },
        optional: false,
        constraints: { min_length: 28, max_length: 128 },
      },
      expected_revision: {
        type: { kind: "primitive", name: "uint32" },
        optional: false,
        constraints: { min_value: 1 },
      },
      member_handles: {
        type: { kind: "array", element: { kind: "primitive", name: "string" } },
        optional: false,
        constraints: { min_items: 1, max_items: 100000 },
      },
    },
  },
  "Wavenumber.Geometer.Contracts.StepTopologyA0.ReplaceMetadataProbeCommand": {
    kind: "object",
    properties: {
      kind: {
        type: { kind: "literal", value_type: "string", value: "replace" },
        optional: false,
        constraints: {},
      },
      authored_id: {
        type: { kind: "primitive", name: "string" },
        optional: false,
        constraints: { min_length: 28, max_length: 128 },
      },
      expected_revision: {
        type: { kind: "primitive", name: "uint32" },
        optional: false,
        constraints: { min_value: 1 },
      },
      target: {
        type: {
          kind: "reference",
          target: "Wavenumber.Geometer.Contracts.StepTopologyA0.MetadataProbeTarget",
        },
        optional: false,
        constraints: {},
      },
      key: {
        type: { kind: "primitive", name: "string" },
        optional: false,
        constraints: { min_length: 32, max_length: 128 },
      },
      value: {
        type: { kind: "primitive", name: "string" },
        optional: false,
        constraints: { min_length: 1, max_length: 4096 },
      },
    },
  },
  "Wavenumber.Geometer.Contracts.StepTopologyA0.RestoreStateArtifact": {
    kind: "union",
    variants: [
      {
        kind: "reference",
        target: "Wavenumber.Geometer.Contracts.StepTopologyA0.XbfPersistenceArtifact",
      },
      {
        kind: "reference",
        target: "Wavenumber.Geometer.Contracts.StepTopologyA0.XmlXcafPersistenceArtifact",
      },
      {
        kind: "reference",
        target: "Wavenumber.Geometer.Contracts.StepTopologyA0.StepAp242PersistenceArtifact",
      },
      {
        kind: "reference",
        target: "Wavenumber.Geometer.Contracts.StepTopologyA0.JsonSidecarPersistenceArtifact",
      },
      {
        kind: "reference",
        target: "Wavenumber.Geometer.Contracts.StepTopologyA0.EditJournalPersistenceArtifact",
      },
    ],
  },
  "Wavenumber.Geometer.Contracts.StepTopologyA0.RootOccurrenceProbeTarget": {
    kind: "object",
    properties: {
      kind: {
        type: { kind: "literal", value_type: "string", value: "root_occurrence" },
        optional: false,
        constraints: {},
      },
      target_handle: {
        type: { kind: "primitive", name: "string" },
        optional: false,
        constraints: { min_length: 68, max_length: 68 },
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
  "Wavenumber.Geometer.Contracts.StepTopologyA0.SaveCarrier": {
    kind: "enum",
    values: ["xbf", "xml_xcaf", "step_ap242", "json_sidecar"],
  },
  "Wavenumber.Geometer.Contracts.StepTopologyA0.SavePersistenceArtifact": {
    kind: "union",
    variants: [
      {
        kind: "reference",
        target: "Wavenumber.Geometer.Contracts.StepTopologyA0.XbfPersistenceArtifact",
      },
      {
        kind: "reference",
        target: "Wavenumber.Geometer.Contracts.StepTopologyA0.XmlXcafPersistenceArtifact",
      },
      {
        kind: "reference",
        target: "Wavenumber.Geometer.Contracts.StepTopologyA0.StepAp242PersistenceArtifact",
      },
      {
        kind: "reference",
        target: "Wavenumber.Geometer.Contracts.StepTopologyA0.JsonSidecarPersistenceArtifact",
      },
    ],
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
  "Wavenumber.Geometer.Contracts.StepTopologyA0.StepAp242PersistenceArtifact": {
    kind: "object",
    properties: {
      carrier: {
        type: { kind: "literal", value_type: "string", value: "step_ap242" },
        optional: false,
        constraints: {},
      },
      name: {
        type: { kind: "literal", value_type: "string", value: "state_artifact" },
        optional: false,
        constraints: {},
      },
      media_type: {
        type: { kind: "literal", value_type: "string", value: "application/step" },
        optional: false,
        constraints: {},
      },
      format: {
        type: {
          kind: "literal",
          value_type: "string",
          value: "ap242-managed-model-based-3d-engineering",
        },
        optional: false,
        constraints: {},
      },
      bytes: {
        type: { kind: "primitive", name: "uint32" },
        optional: false,
        constraints: { min_value: 1, max_value: 536870912 },
      },
      sha256: {
        type: { kind: "primitive", name: "string" },
        optional: false,
        constraints: { min_length: 64, max_length: 64 },
      },
    },
  },
  "Wavenumber.Geometer.Contracts.StepTopologyA0.StepTopologyAnalyzeRecoveryRequestA0": {
    kind: "object",
    properties: {
      schema: {
        type: {
          kind: "literal",
          value_type: "string",
          value: "geometry.step_topology.analyze_recovery.request.a0",
        },
        optional: false,
        constraints: {},
      },
      groups: {
        type: {
          kind: "array",
          element: {
            kind: "reference",
            target: "Wavenumber.Geometer.Contracts.StepTopologyA0.RecoveryGroupRequest",
          },
        },
        optional: false,
        constraints: { min_items: 1, max_items: 16 },
      },
    },
  },
  "Wavenumber.Geometer.Contracts.StepTopologyA0.StepTopologyAnalyzeRecoveryResultA0": {
    kind: "object",
    properties: {
      schema: {
        type: {
          kind: "literal",
          value_type: "string",
          value: "geometry.step_topology.analyze_recovery.result.a0",
        },
        optional: false,
        constraints: {},
      },
      groups: {
        type: {
          kind: "array",
          element: {
            kind: "reference",
            target: "Wavenumber.Geometer.Contracts.StepTopologyA0.RecoveryGroupResult",
          },
        },
        optional: false,
        constraints: { max_items: 16 },
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
  "Wavenumber.Geometer.Contracts.StepTopologyA0.StepTopologyApplyHierarchyRequestA0": {
    kind: "object",
    properties: {
      schema: {
        type: {
          kind: "literal",
          value_type: "string",
          value: "geometry.step_topology.apply_hierarchy.request.a0",
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
      expected_hierarchy_revision: {
        type: { kind: "primitive", name: "uint32" },
        optional: false,
        constraints: {},
      },
      commands: {
        type: {
          kind: "array",
          element: {
            kind: "reference",
            target: "Wavenumber.Geometer.Contracts.StepTopologyA0.HierarchyCommand",
          },
        },
        optional: false,
        constraints: { min_items: 1, max_items: 10000 },
      },
    },
  },
  "Wavenumber.Geometer.Contracts.StepTopologyA0.StepTopologyApplyHierarchyResultA0": {
    kind: "object",
    properties: {
      schema: {
        type: {
          kind: "literal",
          value_type: "string",
          value: "geometry.step_topology.apply_hierarchy.result.a0",
        },
        optional: false,
        constraints: {},
      },
      state: {
        type: {
          kind: "reference",
          target: "Wavenumber.Geometer.Contracts.StepTopologyA0.MutationSessionState",
        },
        optional: false,
        constraints: {},
      },
      hierarchy: {
        type: {
          kind: "reference",
          target: "Wavenumber.Geometer.Contracts.StepTopologyA0.HierarchyState",
        },
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
        constraints: { max_items: 256 },
      },
    },
  },
  "Wavenumber.Geometer.Contracts.StepTopologyA0.StepTopologyApplyLogicalGroupsRequestA0": {
    kind: "object",
    properties: {
      schema: {
        type: {
          kind: "literal",
          value_type: "string",
          value: "geometry.step_topology.apply_logical_groups.request.a0",
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
      commands: {
        type: {
          kind: "array",
          element: {
            kind: "reference",
            target: "Wavenumber.Geometer.Contracts.StepTopologyA0.LogicalGroupCommand",
          },
        },
        optional: false,
        constraints: { min_items: 1, max_items: 10000 },
      },
    },
  },
  "Wavenumber.Geometer.Contracts.StepTopologyA0.StepTopologyApplyLogicalGroupsResultA0": {
    kind: "object",
    properties: {
      schema: {
        type: {
          kind: "literal",
          value_type: "string",
          value: "geometry.step_topology.apply_logical_groups.result.a0",
        },
        optional: false,
        constraints: {},
      },
      state: {
        type: {
          kind: "reference",
          target: "Wavenumber.Geometer.Contracts.StepTopologyA0.MutationSessionState",
        },
        optional: false,
        constraints: {},
      },
      groups: {
        type: {
          kind: "array",
          element: {
            kind: "reference",
            target: "Wavenumber.Geometer.Contracts.StepTopologyA0.LogicalGroup",
          },
        },
        optional: false,
        constraints: { max_items: 10000 },
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
  "Wavenumber.Geometer.Contracts.StepTopologyA0.StepTopologyApplyMetadataProbesRequestA0": {
    kind: "object",
    properties: {
      schema: {
        type: {
          kind: "literal",
          value_type: "string",
          value: "geometry.step_topology.apply_metadata_probes.request.a0",
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
      commands: {
        type: {
          kind: "array",
          element: {
            kind: "reference",
            target: "Wavenumber.Geometer.Contracts.StepTopologyA0.MetadataProbeCommand",
          },
        },
        optional: false,
        constraints: { min_items: 1, max_items: 10000 },
      },
    },
  },
  "Wavenumber.Geometer.Contracts.StepTopologyA0.StepTopologyApplyMetadataProbesResultA0": {
    kind: "object",
    properties: {
      schema: {
        type: {
          kind: "literal",
          value_type: "string",
          value: "geometry.step_topology.apply_metadata_probes.result.a0",
        },
        optional: false,
        constraints: {},
      },
      state: {
        type: {
          kind: "reference",
          target: "Wavenumber.Geometer.Contracts.StepTopologyA0.MutationSessionState",
        },
        optional: false,
        constraints: {},
      },
      groups: {
        type: {
          kind: "array",
          element: {
            kind: "reference",
            target: "Wavenumber.Geometer.Contracts.StepTopologyA0.LogicalGroup",
          },
        },
        optional: false,
        constraints: { max_items: 10000 },
      },
      probes: {
        type: {
          kind: "array",
          element: {
            kind: "reference",
            target: "Wavenumber.Geometer.Contracts.StepTopologyA0.MetadataProbe",
          },
        },
        optional: false,
        constraints: { max_items: 10000 },
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
  "Wavenumber.Geometer.Contracts.StepTopologyA0.StepTopologyCheckpointEditJournalRequestA0": {
    kind: "object",
    properties: {
      schema: {
        type: {
          kind: "literal",
          value_type: "string",
          value: "geometry.step_topology.checkpoint_edit_journal.request.a0",
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
  "Wavenumber.Geometer.Contracts.StepTopologyA0.StepTopologyCheckpointEditJournalResultA0": {
    kind: "object",
    properties: {
      schema: {
        type: {
          kind: "literal",
          value_type: "string",
          value: "geometry.step_topology.checkpoint_edit_journal.result.a0",
        },
        optional: false,
        constraints: {},
      },
      state: {
        type: {
          kind: "reference",
          target: "Wavenumber.Geometer.Contracts.StepTopologyA0.MutationSessionState",
        },
        optional: false,
        constraints: {},
      },
      source_sha256: {
        type: { kind: "primitive", name: "string" },
        optional: false,
        constraints: { min_length: 64, max_length: 64 },
      },
      source_brep_sha256: {
        type: { kind: "primitive", name: "string" },
        optional: false,
        constraints: { min_length: 64, max_length: 64 },
      },
      target_inventory_sha256: {
        type: { kind: "primitive", name: "string" },
        optional: false,
        constraints: { min_length: 64, max_length: 64 },
      },
      occt_version: {
        type: { kind: "primitive", name: "string" },
        optional: false,
        constraints: { min_length: 1, max_length: 64 },
      },
      transaction_count: {
        type: { kind: "primitive", name: "uint32" },
        optional: false,
        constraints: { max_value: 100000 },
      },
      journal: {
        type: {
          kind: "reference",
          target: "Wavenumber.Geometer.Contracts.StepTopologyA0.EditJournalAttachmentDescriptor",
        },
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
        constraints: { max_items: 256 },
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
          target:
            "Wavenumber.Geometer.Contracts.StepTopologyA0.TopologyBindingTableAttachmentDescriptor",
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
  "Wavenumber.Geometer.Contracts.StepTopologyA0.StepTopologyRestoreRequestA0": {
    kind: "object",
    properties: {
      schema: {
        type: {
          kind: "literal",
          value_type: "string",
          value: "geometry.step_topology.restore.request.a0",
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
      state_artifact: {
        type: {
          kind: "reference",
          target: "Wavenumber.Geometer.Contracts.StepTopologyA0.RestoreStateArtifact",
        },
        optional: false,
        constraints: {},
      },
      replay_preconditions: {
        type: {
          kind: "reference",
          target: "Wavenumber.Geometer.Contracts.StepTopologyA0.EditJournalReplayPreconditions",
        },
        optional: true,
        constraints: {},
      },
      include_diagnostics: {
        type: { kind: "primitive", name: "boolean" },
        optional: false,
        constraints: {},
      },
    },
  },
  "Wavenumber.Geometer.Contracts.StepTopologyA0.StepTopologyRestoreResultA0": {
    kind: "object",
    properties: {
      schema: {
        type: {
          kind: "literal",
          value_type: "string",
          value: "geometry.step_topology.restore.result.a0",
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
      replayed_transaction_count: {
        type: { kind: "primitive", name: "uint32" },
        optional: false,
        constraints: { max_value: 100000 },
      },
      evicted_session_handles: {
        type: { kind: "array", element: { kind: "primitive", name: "string" } },
        optional: true,
        constraints: { max_items: 64 },
      },
      recovery: {
        type: {
          kind: "array",
          element: {
            kind: "reference",
            target: "Wavenumber.Geometer.Contracts.StepTopologyA0.RecoveryGroupResult",
          },
        },
        optional: false,
        constraints: { max_items: 16 },
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
  "Wavenumber.Geometer.Contracts.StepTopologyA0.StepTopologySaveRequestA0": {
    kind: "object",
    properties: {
      schema: {
        type: {
          kind: "literal",
          value_type: "string",
          value: "geometry.step_topology.save.request.a0",
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
      carrier: {
        type: {
          kind: "reference",
          target: "Wavenumber.Geometer.Contracts.StepTopologyA0.SaveCarrier",
        },
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
  "Wavenumber.Geometer.Contracts.StepTopologyA0.StepTopologySaveResultA0": {
    kind: "object",
    properties: {
      schema: {
        type: {
          kind: "literal",
          value_type: "string",
          value: "geometry.step_topology.save.result.a0",
        },
        optional: false,
        constraints: {},
      },
      state: {
        type: {
          kind: "reference",
          target: "Wavenumber.Geometer.Contracts.StepTopologyA0.MutationSessionState",
        },
        optional: false,
        constraints: {},
      },
      source_sha256: {
        type: { kind: "primitive", name: "string" },
        optional: false,
        constraints: { min_length: 64, max_length: 64 },
      },
      artifact: {
        type: {
          kind: "reference",
          target: "Wavenumber.Geometer.Contracts.StepTopologyA0.SavePersistenceArtifact",
        },
        optional: false,
        constraints: {},
      },
      capabilities: {
        type: {
          kind: "array",
          element: {
            kind: "reference",
            target: "Wavenumber.Geometer.Contracts.StepTopologyA0.CarrierCapability",
          },
        },
        optional: false,
        constraints: { min_items: 5, max_items: 5 },
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
  "Wavenumber.Geometer.Contracts.StepTopologyA0.TopologyMembership": {
    kind: "object",
    properties: {
      kind: {
        type: {
          kind: "reference",
          target: "Wavenumber.Geometer.Contracts.StepTopologyA0.TopologyMembershipKind",
        },
        optional: false,
        constraints: {},
      },
      owner_handle: {
        type: { kind: "primitive", name: "string" },
        optional: false,
        constraints: { min_length: 68, max_length: 68 },
      },
      member_handle: {
        type: { kind: "primitive", name: "string" },
        optional: false,
        constraints: { min_length: 68, max_length: 68 },
      },
    },
  },
  "Wavenumber.Geometer.Contracts.StepTopologyA0.TopologyMembershipKind": {
    kind: "enum",
    values: ["body_shell", "body_face", "shell_face"],
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
      memberships: {
        type: {
          kind: "array",
          element: {
            kind: "reference",
            target: "Wavenumber.Geometer.Contracts.StepTopologyA0.TopologyMembership",
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
  "Wavenumber.Geometer.Contracts.StepTopologyA0.XbfPersistenceArtifact": {
    kind: "object",
    properties: {
      carrier: {
        type: { kind: "literal", value_type: "string", value: "xbf" },
        optional: false,
        constraints: {},
      },
      name: {
        type: { kind: "literal", value_type: "string", value: "state_artifact" },
        optional: false,
        constraints: {},
      },
      media_type: {
        type: { kind: "literal", value_type: "string", value: "application/vnd.opencascade.xbf" },
        optional: false,
        constraints: {},
      },
      format: {
        type: { kind: "literal", value_type: "string", value: "ocaf-xbf-version-12" },
        optional: false,
        constraints: {},
      },
      bytes: {
        type: { kind: "primitive", name: "uint32" },
        optional: false,
        constraints: { min_value: 1, max_value: 536870912 },
      },
      sha256: {
        type: { kind: "primitive", name: "string" },
        optional: false,
        constraints: { min_length: 64, max_length: 64 },
      },
    },
  },
  "Wavenumber.Geometer.Contracts.StepTopologyA0.XmlXcafPersistenceArtifact": {
    kind: "object",
    properties: {
      carrier: {
        type: { kind: "literal", value_type: "string", value: "xml_xcaf" },
        optional: false,
        constraints: {},
      },
      name: {
        type: { kind: "literal", value_type: "string", value: "state_artifact" },
        optional: false,
        constraints: {},
      },
      media_type: {
        type: {
          kind: "literal",
          value_type: "string",
          value: "application/vnd.opencascade.xml-xcaf",
        },
        optional: false,
        constraints: {},
      },
      format: {
        type: { kind: "literal", value_type: "string", value: "ocaf-xml-xcaf-version-12" },
        optional: false,
        constraints: {},
      },
      bytes: {
        type: { kind: "primitive", name: "uint32" },
        optional: false,
        constraints: { min_value: 1, max_value: 536870912 },
      },
      sha256: {
        type: { kind: "primitive", name: "string" },
        optional: false,
        constraints: { min_length: 64, max_length: 64 },
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

export function decodeHlrProjectionOptionsA0Json(
  data: string | Uint8Array,
): HlrProjectionOptionsA0 {
  return decodeContractJson(
    data,
    {
      kind: "reference",
      target: "Wavenumber.Geometer.Contracts.HlrProjectionA0.HlrProjectionOptionsA0",
    },
    declarations,
  ) as HlrProjectionOptionsA0;
}

export function encodeHlrProjectionOptionsA0Json(value: HlrProjectionOptionsA0): string {
  return encodeContractJson(
    value,
    {
      kind: "reference",
      target: "Wavenumber.Geometer.Contracts.HlrProjectionA0.HlrProjectionOptionsA0",
    },
    declarations,
  );
}

export function decodeHlrProjectionResultA0Json(data: string | Uint8Array): HlrProjectionResultA0 {
  return decodeContractJson(
    data,
    {
      kind: "reference",
      target: "Wavenumber.Geometer.Contracts.HlrProjectionA0.HlrProjectionResultA0",
    },
    declarations,
  ) as HlrProjectionResultA0;
}

export function encodeHlrProjectionResultA0Json(value: HlrProjectionResultA0): string {
  return encodeContractJson(
    value,
    {
      kind: "reference",
      target: "Wavenumber.Geometer.Contracts.HlrProjectionA0.HlrProjectionResultA0",
    },
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

export function decodeMeshIllustrationInputA0Json(
  data: string | Uint8Array,
): MeshIllustrationInputA0 {
  return decodeContractJson(
    data,
    {
      kind: "reference",
      target: "Wavenumber.Geometer.Contracts.MeshIllustrationA0.MeshIllustrationInputA0",
    },
    declarations,
  ) as MeshIllustrationInputA0;
}

export function encodeMeshIllustrationInputA0Json(value: MeshIllustrationInputA0): string {
  return encodeContractJson(
    value,
    {
      kind: "reference",
      target: "Wavenumber.Geometer.Contracts.MeshIllustrationA0.MeshIllustrationInputA0",
    },
    declarations,
  );
}

export function decodeMeshIllustrationResultA0Json(
  data: string | Uint8Array,
): MeshIllustrationResultA0 {
  return decodeContractJson(
    data,
    {
      kind: "reference",
      target: "Wavenumber.Geometer.Contracts.MeshIllustrationA0.MeshIllustrationResultA0",
    },
    declarations,
  ) as MeshIllustrationResultA0;
}

export function encodeMeshIllustrationResultA0Json(value: MeshIllustrationResultA0): string {
  return encodeContractJson(
    value,
    {
      kind: "reference",
      target: "Wavenumber.Geometer.Contracts.MeshIllustrationA0.MeshIllustrationResultA0",
    },
    declarations,
  );
}

export function decodeMeshIllustrationStyleA0Json(
  data: string | Uint8Array,
): MeshIllustrationStyleA0 {
  return decodeContractJson(
    data,
    {
      kind: "reference",
      target: "Wavenumber.Geometer.Contracts.MeshIllustrationA0.MeshIllustrationStyleA0",
    },
    declarations,
  ) as MeshIllustrationStyleA0;
}

export function encodeMeshIllustrationStyleA0Json(value: MeshIllustrationStyleA0): string {
  return encodeContractJson(
    value,
    {
      kind: "reference",
      target: "Wavenumber.Geometer.Contracts.MeshIllustrationA0.MeshIllustrationStyleA0",
    },
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

export function decodeMeshCollectionA0Json(data: string | Uint8Array): MeshCollectionA0 {
  return decodeContractJson(
    data,
    {
      kind: "reference",
      target: "Wavenumber.Geometer.Contracts.ModelTessellationA0.MeshCollectionA0",
    },
    declarations,
  ) as MeshCollectionA0;
}

export function encodeMeshCollectionA0Json(value: MeshCollectionA0): string {
  return encodeContractJson(
    value,
    {
      kind: "reference",
      target: "Wavenumber.Geometer.Contracts.ModelTessellationA0.MeshCollectionA0",
    },
    declarations,
  );
}

export function decodeModelTessellationRequestA0Json(
  data: string | Uint8Array,
): ModelTessellationRequestA0 {
  return decodeContractJson(
    data,
    {
      kind: "reference",
      target: "Wavenumber.Geometer.Contracts.ModelTessellationA0.ModelTessellationRequestA0",
    },
    declarations,
  ) as ModelTessellationRequestA0;
}

export function encodeModelTessellationRequestA0Json(value: ModelTessellationRequestA0): string {
  return encodeContractJson(
    value,
    {
      kind: "reference",
      target: "Wavenumber.Geometer.Contracts.ModelTessellationA0.ModelTessellationRequestA0",
    },
    declarations,
  );
}

export function decodeModelTessellationResultA0Json(
  data: string | Uint8Array,
): ModelTessellationResultA0 {
  return decodeContractJson(
    data,
    {
      kind: "reference",
      target: "Wavenumber.Geometer.Contracts.ModelTessellationA0.ModelTessellationResultA0",
    },
    declarations,
  ) as ModelTessellationResultA0;
}

export function encodeModelTessellationResultA0Json(value: ModelTessellationResultA0): string {
  return encodeContractJson(
    value,
    {
      kind: "reference",
      target: "Wavenumber.Geometer.Contracts.ModelTessellationA0.ModelTessellationResultA0",
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

export function decodeStepTopologyAnalyzeRecoveryRequestA0Json(
  data: string | Uint8Array,
): StepTopologyAnalyzeRecoveryRequestA0 {
  return decodeContractJson(
    data,
    {
      kind: "reference",
      target: "Wavenumber.Geometer.Contracts.StepTopologyA0.StepTopologyAnalyzeRecoveryRequestA0",
    },
    declarations,
  ) as StepTopologyAnalyzeRecoveryRequestA0;
}

export function encodeStepTopologyAnalyzeRecoveryRequestA0Json(
  value: StepTopologyAnalyzeRecoveryRequestA0,
): string {
  return encodeContractJson(
    value,
    {
      kind: "reference",
      target: "Wavenumber.Geometer.Contracts.StepTopologyA0.StepTopologyAnalyzeRecoveryRequestA0",
    },
    declarations,
  );
}

export function decodeStepTopologyAnalyzeRecoveryResultA0Json(
  data: string | Uint8Array,
): StepTopologyAnalyzeRecoveryResultA0 {
  return decodeContractJson(
    data,
    {
      kind: "reference",
      target: "Wavenumber.Geometer.Contracts.StepTopologyA0.StepTopologyAnalyzeRecoveryResultA0",
    },
    declarations,
  ) as StepTopologyAnalyzeRecoveryResultA0;
}

export function encodeStepTopologyAnalyzeRecoveryResultA0Json(
  value: StepTopologyAnalyzeRecoveryResultA0,
): string {
  return encodeContractJson(
    value,
    {
      kind: "reference",
      target: "Wavenumber.Geometer.Contracts.StepTopologyA0.StepTopologyAnalyzeRecoveryResultA0",
    },
    declarations,
  );
}

export function decodeStepTopologyApplyHierarchyRequestA0Json(
  data: string | Uint8Array,
): StepTopologyApplyHierarchyRequestA0 {
  return decodeContractJson(
    data,
    {
      kind: "reference",
      target: "Wavenumber.Geometer.Contracts.StepTopologyA0.StepTopologyApplyHierarchyRequestA0",
    },
    declarations,
  ) as StepTopologyApplyHierarchyRequestA0;
}

export function encodeStepTopologyApplyHierarchyRequestA0Json(
  value: StepTopologyApplyHierarchyRequestA0,
): string {
  return encodeContractJson(
    value,
    {
      kind: "reference",
      target: "Wavenumber.Geometer.Contracts.StepTopologyA0.StepTopologyApplyHierarchyRequestA0",
    },
    declarations,
  );
}

export function decodeStepTopologyApplyHierarchyResultA0Json(
  data: string | Uint8Array,
): StepTopologyApplyHierarchyResultA0 {
  return decodeContractJson(
    data,
    {
      kind: "reference",
      target: "Wavenumber.Geometer.Contracts.StepTopologyA0.StepTopologyApplyHierarchyResultA0",
    },
    declarations,
  ) as StepTopologyApplyHierarchyResultA0;
}

export function encodeStepTopologyApplyHierarchyResultA0Json(
  value: StepTopologyApplyHierarchyResultA0,
): string {
  return encodeContractJson(
    value,
    {
      kind: "reference",
      target: "Wavenumber.Geometer.Contracts.StepTopologyA0.StepTopologyApplyHierarchyResultA0",
    },
    declarations,
  );
}

export function decodeStepTopologyApplyLogicalGroupsRequestA0Json(
  data: string | Uint8Array,
): StepTopologyApplyLogicalGroupsRequestA0 {
  return decodeContractJson(
    data,
    {
      kind: "reference",
      target:
        "Wavenumber.Geometer.Contracts.StepTopologyA0.StepTopologyApplyLogicalGroupsRequestA0",
    },
    declarations,
  ) as StepTopologyApplyLogicalGroupsRequestA0;
}

export function encodeStepTopologyApplyLogicalGroupsRequestA0Json(
  value: StepTopologyApplyLogicalGroupsRequestA0,
): string {
  return encodeContractJson(
    value,
    {
      kind: "reference",
      target:
        "Wavenumber.Geometer.Contracts.StepTopologyA0.StepTopologyApplyLogicalGroupsRequestA0",
    },
    declarations,
  );
}

export function decodeStepTopologyApplyLogicalGroupsResultA0Json(
  data: string | Uint8Array,
): StepTopologyApplyLogicalGroupsResultA0 {
  return decodeContractJson(
    data,
    {
      kind: "reference",
      target: "Wavenumber.Geometer.Contracts.StepTopologyA0.StepTopologyApplyLogicalGroupsResultA0",
    },
    declarations,
  ) as StepTopologyApplyLogicalGroupsResultA0;
}

export function encodeStepTopologyApplyLogicalGroupsResultA0Json(
  value: StepTopologyApplyLogicalGroupsResultA0,
): string {
  return encodeContractJson(
    value,
    {
      kind: "reference",
      target: "Wavenumber.Geometer.Contracts.StepTopologyA0.StepTopologyApplyLogicalGroupsResultA0",
    },
    declarations,
  );
}

export function decodeStepTopologyApplyMetadataProbesRequestA0Json(
  data: string | Uint8Array,
): StepTopologyApplyMetadataProbesRequestA0 {
  return decodeContractJson(
    data,
    {
      kind: "reference",
      target:
        "Wavenumber.Geometer.Contracts.StepTopologyA0.StepTopologyApplyMetadataProbesRequestA0",
    },
    declarations,
  ) as StepTopologyApplyMetadataProbesRequestA0;
}

export function encodeStepTopologyApplyMetadataProbesRequestA0Json(
  value: StepTopologyApplyMetadataProbesRequestA0,
): string {
  return encodeContractJson(
    value,
    {
      kind: "reference",
      target:
        "Wavenumber.Geometer.Contracts.StepTopologyA0.StepTopologyApplyMetadataProbesRequestA0",
    },
    declarations,
  );
}

export function decodeStepTopologyApplyMetadataProbesResultA0Json(
  data: string | Uint8Array,
): StepTopologyApplyMetadataProbesResultA0 {
  return decodeContractJson(
    data,
    {
      kind: "reference",
      target:
        "Wavenumber.Geometer.Contracts.StepTopologyA0.StepTopologyApplyMetadataProbesResultA0",
    },
    declarations,
  ) as StepTopologyApplyMetadataProbesResultA0;
}

export function encodeStepTopologyApplyMetadataProbesResultA0Json(
  value: StepTopologyApplyMetadataProbesResultA0,
): string {
  return encodeContractJson(
    value,
    {
      kind: "reference",
      target:
        "Wavenumber.Geometer.Contracts.StepTopologyA0.StepTopologyApplyMetadataProbesResultA0",
    },
    declarations,
  );
}

export function decodeStepTopologyCheckpointEditJournalRequestA0Json(
  data: string | Uint8Array,
): StepTopologyCheckpointEditJournalRequestA0 {
  return decodeContractJson(
    data,
    {
      kind: "reference",
      target:
        "Wavenumber.Geometer.Contracts.StepTopologyA0.StepTopologyCheckpointEditJournalRequestA0",
    },
    declarations,
  ) as StepTopologyCheckpointEditJournalRequestA0;
}

export function encodeStepTopologyCheckpointEditJournalRequestA0Json(
  value: StepTopologyCheckpointEditJournalRequestA0,
): string {
  return encodeContractJson(
    value,
    {
      kind: "reference",
      target:
        "Wavenumber.Geometer.Contracts.StepTopologyA0.StepTopologyCheckpointEditJournalRequestA0",
    },
    declarations,
  );
}

export function decodeStepTopologyCheckpointEditJournalResultA0Json(
  data: string | Uint8Array,
): StepTopologyCheckpointEditJournalResultA0 {
  return decodeContractJson(
    data,
    {
      kind: "reference",
      target:
        "Wavenumber.Geometer.Contracts.StepTopologyA0.StepTopologyCheckpointEditJournalResultA0",
    },
    declarations,
  ) as StepTopologyCheckpointEditJournalResultA0;
}

export function encodeStepTopologyCheckpointEditJournalResultA0Json(
  value: StepTopologyCheckpointEditJournalResultA0,
): string {
  return encodeContractJson(
    value,
    {
      kind: "reference",
      target:
        "Wavenumber.Geometer.Contracts.StepTopologyA0.StepTopologyCheckpointEditJournalResultA0",
    },
    declarations,
  );
}

export function decodeStepTopologyCloseRequestA0Json(
  data: string | Uint8Array,
): StepTopologyCloseRequestA0 {
  return decodeContractJson(
    data,
    {
      kind: "reference",
      target: "Wavenumber.Geometer.Contracts.StepTopologyA0.StepTopologyCloseRequestA0",
    },
    declarations,
  ) as StepTopologyCloseRequestA0;
}

export function encodeStepTopologyCloseRequestA0Json(value: StepTopologyCloseRequestA0): string {
  return encodeContractJson(
    value,
    {
      kind: "reference",
      target: "Wavenumber.Geometer.Contracts.StepTopologyA0.StepTopologyCloseRequestA0",
    },
    declarations,
  );
}

export function decodeStepTopologyCloseResultA0Json(
  data: string | Uint8Array,
): StepTopologyCloseResultA0 {
  return decodeContractJson(
    data,
    {
      kind: "reference",
      target: "Wavenumber.Geometer.Contracts.StepTopologyA0.StepTopologyCloseResultA0",
    },
    declarations,
  ) as StepTopologyCloseResultA0;
}

export function encodeStepTopologyCloseResultA0Json(value: StepTopologyCloseResultA0): string {
  return encodeContractJson(
    value,
    {
      kind: "reference",
      target: "Wavenumber.Geometer.Contracts.StepTopologyA0.StepTopologyCloseResultA0",
    },
    declarations,
  );
}

export function decodeStepTopologyInspectRequestA0Json(
  data: string | Uint8Array,
): StepTopologyInspectRequestA0 {
  return decodeContractJson(
    data,
    {
      kind: "reference",
      target: "Wavenumber.Geometer.Contracts.StepTopologyA0.StepTopologyInspectRequestA0",
    },
    declarations,
  ) as StepTopologyInspectRequestA0;
}

export function encodeStepTopologyInspectRequestA0Json(
  value: StepTopologyInspectRequestA0,
): string {
  return encodeContractJson(
    value,
    {
      kind: "reference",
      target: "Wavenumber.Geometer.Contracts.StepTopologyA0.StepTopologyInspectRequestA0",
    },
    declarations,
  );
}

export function decodeStepTopologyInspectResultA0Json(
  data: string | Uint8Array,
): StepTopologyInspectResultA0 {
  return decodeContractJson(
    data,
    {
      kind: "reference",
      target: "Wavenumber.Geometer.Contracts.StepTopologyA0.StepTopologyInspectResultA0",
    },
    declarations,
  ) as StepTopologyInspectResultA0;
}

export function encodeStepTopologyInspectResultA0Json(value: StepTopologyInspectResultA0): string {
  return encodeContractJson(
    value,
    {
      kind: "reference",
      target: "Wavenumber.Geometer.Contracts.StepTopologyA0.StepTopologyInspectResultA0",
    },
    declarations,
  );
}

export function decodeStepTopologyOpenRequestA0Json(
  data: string | Uint8Array,
): StepTopologyOpenRequestA0 {
  return decodeContractJson(
    data,
    {
      kind: "reference",
      target: "Wavenumber.Geometer.Contracts.StepTopologyA0.StepTopologyOpenRequestA0",
    },
    declarations,
  ) as StepTopologyOpenRequestA0;
}

export function encodeStepTopologyOpenRequestA0Json(value: StepTopologyOpenRequestA0): string {
  return encodeContractJson(
    value,
    {
      kind: "reference",
      target: "Wavenumber.Geometer.Contracts.StepTopologyA0.StepTopologyOpenRequestA0",
    },
    declarations,
  );
}

export function decodeStepTopologyOpenResultA0Json(
  data: string | Uint8Array,
): StepTopologyOpenResultA0 {
  return decodeContractJson(
    data,
    {
      kind: "reference",
      target: "Wavenumber.Geometer.Contracts.StepTopologyA0.StepTopologyOpenResultA0",
    },
    declarations,
  ) as StepTopologyOpenResultA0;
}

export function encodeStepTopologyOpenResultA0Json(value: StepTopologyOpenResultA0): string {
  return encodeContractJson(
    value,
    {
      kind: "reference",
      target: "Wavenumber.Geometer.Contracts.StepTopologyA0.StepTopologyOpenResultA0",
    },
    declarations,
  );
}

export function decodeStepTopologyRenderRequestA0Json(
  data: string | Uint8Array,
): StepTopologyRenderRequestA0 {
  return decodeContractJson(
    data,
    {
      kind: "reference",
      target: "Wavenumber.Geometer.Contracts.StepTopologyA0.StepTopologyRenderRequestA0",
    },
    declarations,
  ) as StepTopologyRenderRequestA0;
}

export function encodeStepTopologyRenderRequestA0Json(value: StepTopologyRenderRequestA0): string {
  return encodeContractJson(
    value,
    {
      kind: "reference",
      target: "Wavenumber.Geometer.Contracts.StepTopologyA0.StepTopologyRenderRequestA0",
    },
    declarations,
  );
}

export function decodeStepTopologyRenderResultA0Json(
  data: string | Uint8Array,
): StepTopologyRenderResultA0 {
  return decodeContractJson(
    data,
    {
      kind: "reference",
      target: "Wavenumber.Geometer.Contracts.StepTopologyA0.StepTopologyRenderResultA0",
    },
    declarations,
  ) as StepTopologyRenderResultA0;
}

export function encodeStepTopologyRenderResultA0Json(value: StepTopologyRenderResultA0): string {
  return encodeContractJson(
    value,
    {
      kind: "reference",
      target: "Wavenumber.Geometer.Contracts.StepTopologyA0.StepTopologyRenderResultA0",
    },
    declarations,
  );
}

export function decodeStepTopologyResolveHitRequestA0Json(
  data: string | Uint8Array,
): StepTopologyResolveHitRequestA0 {
  return decodeContractJson(
    data,
    {
      kind: "reference",
      target: "Wavenumber.Geometer.Contracts.StepTopologyA0.StepTopologyResolveHitRequestA0",
    },
    declarations,
  ) as StepTopologyResolveHitRequestA0;
}

export function encodeStepTopologyResolveHitRequestA0Json(
  value: StepTopologyResolveHitRequestA0,
): string {
  return encodeContractJson(
    value,
    {
      kind: "reference",
      target: "Wavenumber.Geometer.Contracts.StepTopologyA0.StepTopologyResolveHitRequestA0",
    },
    declarations,
  );
}

export function decodeStepTopologyResolveHitResultA0Json(
  data: string | Uint8Array,
): StepTopologyResolveHitResultA0 {
  return decodeContractJson(
    data,
    {
      kind: "reference",
      target: "Wavenumber.Geometer.Contracts.StepTopologyA0.StepTopologyResolveHitResultA0",
    },
    declarations,
  ) as StepTopologyResolveHitResultA0;
}

export function encodeStepTopologyResolveHitResultA0Json(
  value: StepTopologyResolveHitResultA0,
): string {
  return encodeContractJson(
    value,
    {
      kind: "reference",
      target: "Wavenumber.Geometer.Contracts.StepTopologyA0.StepTopologyResolveHitResultA0",
    },
    declarations,
  );
}

export function decodeStepTopologyRestoreRequestA0Json(
  data: string | Uint8Array,
): StepTopologyRestoreRequestA0 {
  return decodeContractJson(
    data,
    {
      kind: "reference",
      target: "Wavenumber.Geometer.Contracts.StepTopologyA0.StepTopologyRestoreRequestA0",
    },
    declarations,
  ) as StepTopologyRestoreRequestA0;
}

export function encodeStepTopologyRestoreRequestA0Json(
  value: StepTopologyRestoreRequestA0,
): string {
  return encodeContractJson(
    value,
    {
      kind: "reference",
      target: "Wavenumber.Geometer.Contracts.StepTopologyA0.StepTopologyRestoreRequestA0",
    },
    declarations,
  );
}

export function decodeStepTopologyRestoreResultA0Json(
  data: string | Uint8Array,
): StepTopologyRestoreResultA0 {
  return decodeContractJson(
    data,
    {
      kind: "reference",
      target: "Wavenumber.Geometer.Contracts.StepTopologyA0.StepTopologyRestoreResultA0",
    },
    declarations,
  ) as StepTopologyRestoreResultA0;
}

export function encodeStepTopologyRestoreResultA0Json(value: StepTopologyRestoreResultA0): string {
  return encodeContractJson(
    value,
    {
      kind: "reference",
      target: "Wavenumber.Geometer.Contracts.StepTopologyA0.StepTopologyRestoreResultA0",
    },
    declarations,
  );
}

export function decodeStepTopologySaveRequestA0Json(
  data: string | Uint8Array,
): StepTopologySaveRequestA0 {
  return decodeContractJson(
    data,
    {
      kind: "reference",
      target: "Wavenumber.Geometer.Contracts.StepTopologyA0.StepTopologySaveRequestA0",
    },
    declarations,
  ) as StepTopologySaveRequestA0;
}

export function encodeStepTopologySaveRequestA0Json(value: StepTopologySaveRequestA0): string {
  return encodeContractJson(
    value,
    {
      kind: "reference",
      target: "Wavenumber.Geometer.Contracts.StepTopologyA0.StepTopologySaveRequestA0",
    },
    declarations,
  );
}

export function decodeStepTopologySaveResultA0Json(
  data: string | Uint8Array,
): StepTopologySaveResultA0 {
  return decodeContractJson(
    data,
    {
      kind: "reference",
      target: "Wavenumber.Geometer.Contracts.StepTopologyA0.StepTopologySaveResultA0",
    },
    declarations,
  ) as StepTopologySaveResultA0;
}

export function encodeStepTopologySaveResultA0Json(value: StepTopologySaveResultA0): string {
  return encodeContractJson(
    value,
    {
      kind: "reference",
      target: "Wavenumber.Geometer.Contracts.StepTopologyA0.StepTopologySaveResultA0",
    },
    declarations,
  );
}
