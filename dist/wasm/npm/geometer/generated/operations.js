// Generated from wn_geometer_contract_catalog.a0.json. Do not edit.
export const operationCatalog = {
    "geometry.model_bounds.a0": {
        identity: "geometry.model_bounds.a0",
        requestContract: "geometry.model_bounds.options.a0",
        resultContract: "geometry.model_bounds.a0",
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
