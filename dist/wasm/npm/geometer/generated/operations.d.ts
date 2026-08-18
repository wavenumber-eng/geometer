export declare const operationCatalog: {
    readonly "geometry.analytic_planar_boolean_batch.a0": {
        readonly identity: "geometry.analytic_planar_boolean_batch.a0";
        readonly requestContract: "geometry.analytic_planar_boolean_batch.request.a0";
        readonly resultContract: "geometry.analytic_planar_boolean_batch.result.a0";
        readonly runtimeDispatch: "packed_attachment";
        readonly inputAttachments: readonly [{
            readonly name: "analytic_planar_boolean_request";
            readonly required: true;
            readonly media_types: readonly ["application/vnd.wavenumber.geometer.analytic-planar-boolean-request"];
            readonly max_bytes: 268435456;
        }];
        readonly outputAttachments: readonly [{
            readonly name: "analytic_planar_boolean_result";
            readonly required: true;
            readonly media_types: readonly ["application/vnd.wavenumber.geometer.analytic-planar-boolean-result"];
            readonly max_bytes: 268435456;
        }];
        readonly requestProjection: {
            readonly kind: "packed_attachment";
            readonly attachment_name: "analytic_planar_boolean_request";
            readonly format: "geometry.analytic_planar_boolean.packet.a0";
        };
        readonly resultProjection: {
            readonly kind: "packed_attachment";
            readonly attachment_name: "analytic_planar_boolean_result";
            readonly format: "geometry.analytic_planar_boolean.packet.a0";
        };
        readonly documentation: "Execute independent ordered analytic planar Boolean jobs through packed attachments.";
    };
    readonly "geometry.model_bounds.a0": {
        readonly identity: "geometry.model_bounds.a0";
        readonly requestContract: "geometry.model_bounds.options.a0";
        readonly resultContract: "geometry.model_bounds.a0";
        readonly runtimeDispatch: "logical_dto";
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
