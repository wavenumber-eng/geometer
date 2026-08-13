export declare const operationCatalog: {
    readonly "geometry.model_bounds.a0": {
        readonly identity: "geometry.model_bounds.a0";
        readonly requestContract: "geometry.model_bounds.options.a0";
        readonly resultContract: "geometry.model_bounds.a0";
        readonly inputAttachments: readonly [{
            readonly name: "model";
            readonly required: true;
            readonly media_types: readonly ["application/step", "model/step"];
            readonly max_bytes: 268435456;
        }];
        readonly outputAttachments: readonly [];
        readonly documentation: "Compute axis-aligned model bounds from the required raw model attachment.";
    };
};
export type OperationIdentity = keyof typeof operationCatalog;
export type ModelBoundsInputMediaType = (typeof operationCatalog)["geometry.model_bounds.a0"]["inputAttachments"][0]["media_types"][number];
