// Generated from wn_geometer_contract_catalog.a0.json. Do not edit.
export const operationCatalog = {
    "geometry.analytic_planar_boolean_batch.a0": {
        identity: "geometry.analytic_planar_boolean_batch.a0",
        requestContract: "geometry.analytic_planar_boolean_batch.request.a0",
        resultContract: "geometry.analytic_planar_boolean_batch.result.a0",
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
        documentation: "Execute independent ordered analytic planar Boolean jobs through packed attachments.",
    },
    "geometry.model_bounds.a0": {
        identity: "geometry.model_bounds.a0",
        requestContract: "geometry.model_bounds.options.a0",
        resultContract: "geometry.model_bounds.a0",
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
};
