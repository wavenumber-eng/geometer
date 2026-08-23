// Generated from wn_geometer_contract_catalog.a0.json. Do not edit.

export const operationCatalog = {
  "geometry.analytic_planar_boolean_batch.a0": {
    identity: "geometry.analytic_planar_boolean_batch.a0",
    requestContract: "geometry.analytic_planar_boolean_batch.request.a0",
    resultContract: "geometry.analytic_planar_boolean_batch.result.a0",
    runtimeAvailable: true,
    runtimeDispatch: "packed_attachment",
    inputAttachments: [
      {
        name: "analytic_planar_boolean_request",
        required: true,
        media_types: ["application/vnd.wavenumber.geometer.analytic-planar-boolean-request"],
        max_bytes: 268435456,
      },
    ],
    outputAttachments: [
      {
        name: "analytic_planar_boolean_result",
        required: true,
        media_types: ["application/vnd.wavenumber.geometer.analytic-planar-boolean-result"],
        max_bytes: 268435456,
      },
    ],
    requestProjection: {
      kind: "packed_attachment",
      attachment_name: "analytic_planar_boolean_request",
      format: "geometry.analytic_planar_boolean.packet.a0",
    },
    resultProjection: {
      kind: "packed_attachment",
      attachment_name: "analytic_planar_boolean_result",
      format: "geometry.analytic_planar_boolean.packet.a0",
    },
    documentation:
      "Execute independent ordered analytic planar Boolean jobs through packed attachments.",
  },
  "geometry.model_bounds.a0": {
    identity: "geometry.model_bounds.a0",
    requestContract: "geometry.model_bounds.options.a0",
    resultContract: "geometry.model_bounds.a0",
    runtimeAvailable: true,
    runtimeDispatch: "logical_dto",
    inputAttachments: [
      {
        name: "model",
        required: true,
        media_types: ["application/step", "model/step"],
        max_bytes: 268435456,
      },
    ],
    outputAttachments: [],
    documentation: "Compute axis-aligned model bounds from the required raw model attachment.",
  },
  "geometry.step_topology.close.a0": {
    identity: "geometry.step_topology.close.a0",
    requestContract: "geometry.step_topology.close.request.a0",
    resultContract: "geometry.step_topology.close.result.a0",
    runtimeAvailable: false,
    runtimeDispatch: "logical_dto",
    inputAttachments: [],
    outputAttachments: [],
    documentation: "",
  },
  "geometry.step_topology.inspect.a0": {
    identity: "geometry.step_topology.inspect.a0",
    requestContract: "geometry.step_topology.inspect.request.a0",
    resultContract: "geometry.step_topology.inspect.result.a0",
    runtimeAvailable: false,
    runtimeDispatch: "logical_dto",
    inputAttachments: [],
    outputAttachments: [
      {
        name: "topology_table",
        required: false,
        media_types: ["application/vnd.wavenumber.geometer.step-topology-table"],
        max_bytes: 134217728,
      },
    ],
    documentation: "",
  },
  "geometry.step_topology.open.a0": {
    identity: "geometry.step_topology.open.a0",
    requestContract: "geometry.step_topology.open.request.a0",
    resultContract: "geometry.step_topology.open.result.a0",
    runtimeAvailable: false,
    runtimeDispatch: "logical_dto",
    inputAttachments: [
      {
        name: "step",
        required: true,
        media_types: ["application/step", "model/step"],
        max_bytes: 268435456,
      },
    ],
    outputAttachments: [],
    documentation: "",
  },
  "geometry.step_topology.render.a0": {
    identity: "geometry.step_topology.render.a0",
    requestContract: "geometry.step_topology.render.request.a0",
    resultContract: "geometry.step_topology.render.result.a0",
    runtimeAvailable: false,
    runtimeDispatch: "logical_dto",
    inputAttachments: [],
    outputAttachments: [
      { name: "glb", required: true, media_types: ["model/gltf-binary"], max_bytes: 268435456 },
      {
        name: "topology_binding_table",
        required: false,
        media_types: ["application/vnd.wavenumber.geometer.step-topology-binding-table"],
        max_bytes: 134217728,
      },
    ],
    documentation: "",
  },
  "geometry.step_topology.resolve_hit.a0": {
    identity: "geometry.step_topology.resolve_hit.a0",
    requestContract: "geometry.step_topology.resolve_hit.request.a0",
    resultContract: "geometry.step_topology.resolve_hit.result.a0",
    runtimeAvailable: false,
    runtimeDispatch: "logical_dto",
    inputAttachments: [],
    outputAttachments: [],
    documentation: "",
  },
} as const;

export type OperationIdentity = keyof typeof operationCatalog;
export type ModelBoundsInputMediaType =
  (typeof operationCatalog)["geometry.model_bounds.a0"]["inputAttachments"][0]["media_types"][number];
