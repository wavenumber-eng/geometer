export declare const NORMALIZED_CONTRACT_CATALOG_SHA256: "3d610e74fa16618a12806607c55be6823d6bf5e9144095ed281179aaeda1415d";
export declare const operationCatalog: {
    readonly "geometry.analytic_planar_boolean_batch.a0": {
        readonly identity: "geometry.analytic_planar_boolean_batch.a0";
        readonly requestContract: "geometry.analytic_planar_boolean_batch.request.a0";
        readonly resultContract: "geometry.analytic_planar_boolean_batch.result.a0";
        readonly runtimeAvailable: true;
        readonly nativeRuntimeAvailable: false;
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
    readonly "geometry.mesh_hlr_projection.a0": {
        readonly identity: "geometry.mesh_hlr_projection.a0";
        readonly requestContract: "geometry.hlr_projection.options.a0";
        readonly resultContract: "geometry.hlr_projection.result.a0";
        readonly runtimeAvailable: true;
        readonly nativeRuntimeAvailable: false;
        readonly runtimeDispatch: "logical_dto";
        readonly inputAttachments: readonly [{
            readonly name: "mesh";
            readonly required: true;
            readonly media_types: readonly ["application/vnd.wavenumber.geometer.indexed-triangle-mesh"];
            readonly max_bytes: 268435456;
        }];
        readonly outputAttachments: readonly [];
        readonly documentation: "Project a synthesized indexed triangle mesh through the Fast HLR backend.";
    };
    readonly "geometry.model_bounds.a0": {
        readonly identity: "geometry.model_bounds.a0";
        readonly requestContract: "geometry.model_bounds.options.a0";
        readonly resultContract: "geometry.model_bounds.a0";
        readonly runtimeAvailable: true;
        readonly nativeRuntimeAvailable: false;
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
    readonly "geometry.model_hlr_projection.a0": {
        readonly identity: "geometry.model_hlr_projection.a0";
        readonly requestContract: "geometry.hlr_projection.options.a0";
        readonly resultContract: "geometry.hlr_projection.result.a0";
        readonly runtimeAvailable: true;
        readonly nativeRuntimeAvailable: false;
        readonly runtimeDispatch: "logical_dto";
        readonly inputAttachments: readonly [{
            readonly name: "model";
            readonly required: true;
            readonly media_types: readonly ["application/step", "model/step"];
            readonly max_bytes: 268435456;
        }];
        readonly outputAttachments: readonly [];
        readonly documentation: "Project STEP model bytes through the selected polygonal, exact, or Fast HLR backend.";
    };
    readonly "geometry.step_topology.analyze_recovery.a0": {
        readonly identity: "geometry.step_topology.analyze_recovery.a0";
        readonly requestContract: "geometry.step_topology.analyze_recovery.request.a0";
        readonly resultContract: "geometry.step_topology.analyze_recovery.result.a0";
        readonly runtimeAvailable: false;
        readonly nativeRuntimeAvailable: false;
        readonly runtimeDispatch: "logical_dto";
        readonly inputAttachments: readonly [];
        readonly outputAttachments: readonly [];
        readonly documentation: "";
    };
    readonly "geometry.step_topology.apply_hierarchy.a0": {
        readonly identity: "geometry.step_topology.apply_hierarchy.a0";
        readonly requestContract: "geometry.step_topology.apply_hierarchy.request.a0";
        readonly resultContract: "geometry.step_topology.apply_hierarchy.result.a0";
        readonly runtimeAvailable: false;
        readonly nativeRuntimeAvailable: false;
        readonly runtimeDispatch: "logical_dto";
        readonly inputAttachments: readonly [];
        readonly outputAttachments: readonly [];
        readonly documentation: "";
    };
    readonly "geometry.step_topology.apply_logical_groups.a0": {
        readonly identity: "geometry.step_topology.apply_logical_groups.a0";
        readonly requestContract: "geometry.step_topology.apply_logical_groups.request.a0";
        readonly resultContract: "geometry.step_topology.apply_logical_groups.result.a0";
        readonly runtimeAvailable: false;
        readonly nativeRuntimeAvailable: true;
        readonly runtimeDispatch: "logical_dto";
        readonly inputAttachments: readonly [];
        readonly outputAttachments: readonly [];
        readonly documentation: "";
    };
    readonly "geometry.step_topology.apply_metadata_probes.a0": {
        readonly identity: "geometry.step_topology.apply_metadata_probes.a0";
        readonly requestContract: "geometry.step_topology.apply_metadata_probes.request.a0";
        readonly resultContract: "geometry.step_topology.apply_metadata_probes.result.a0";
        readonly runtimeAvailable: false;
        readonly nativeRuntimeAvailable: true;
        readonly runtimeDispatch: "logical_dto";
        readonly inputAttachments: readonly [];
        readonly outputAttachments: readonly [];
        readonly documentation: "";
    };
    readonly "geometry.step_topology.checkpoint_edit_journal.a0": {
        readonly identity: "geometry.step_topology.checkpoint_edit_journal.a0";
        readonly requestContract: "geometry.step_topology.checkpoint_edit_journal.request.a0";
        readonly resultContract: "geometry.step_topology.checkpoint_edit_journal.result.a0";
        readonly runtimeAvailable: false;
        readonly nativeRuntimeAvailable: true;
        readonly runtimeDispatch: "logical_dto";
        readonly inputAttachments: readonly [];
        readonly outputAttachments: readonly [{
            readonly name: "edit_journal";
            readonly required: true;
            readonly media_types: readonly ["application/vnd.wavenumber.geometer.step-topology-edit-journal"];
            readonly max_bytes: 67108864;
        }];
        readonly documentation: "";
    };
    readonly "geometry.step_topology.close.a0": {
        readonly identity: "geometry.step_topology.close.a0";
        readonly requestContract: "geometry.step_topology.close.request.a0";
        readonly resultContract: "geometry.step_topology.close.result.a0";
        readonly runtimeAvailable: false;
        readonly nativeRuntimeAvailable: true;
        readonly runtimeDispatch: "logical_dto";
        readonly inputAttachments: readonly [];
        readonly outputAttachments: readonly [];
        readonly documentation: "";
    };
    readonly "geometry.step_topology.inspect.a0": {
        readonly identity: "geometry.step_topology.inspect.a0";
        readonly requestContract: "geometry.step_topology.inspect.request.a0";
        readonly resultContract: "geometry.step_topology.inspect.result.a0";
        readonly runtimeAvailable: false;
        readonly nativeRuntimeAvailable: true;
        readonly runtimeDispatch: "logical_dto";
        readonly inputAttachments: readonly [];
        readonly outputAttachments: readonly [{
            readonly name: "topology_table";
            readonly required: false;
            readonly media_types: readonly ["application/vnd.wavenumber.geometer.step-topology-table"];
            readonly max_bytes: 134217728;
        }];
        readonly documentation: "";
    };
    readonly "geometry.step_topology.open.a0": {
        readonly identity: "geometry.step_topology.open.a0";
        readonly requestContract: "geometry.step_topology.open.request.a0";
        readonly resultContract: "geometry.step_topology.open.result.a0";
        readonly runtimeAvailable: false;
        readonly nativeRuntimeAvailable: true;
        readonly runtimeDispatch: "logical_dto";
        readonly inputAttachments: readonly [{
            readonly name: "step";
            readonly required: true;
            readonly media_types: readonly ["application/step", "model/step"];
            readonly max_bytes: 268435456;
        }];
        readonly outputAttachments: readonly [];
        readonly documentation: "";
    };
    readonly "geometry.step_topology.render.a0": {
        readonly identity: "geometry.step_topology.render.a0";
        readonly requestContract: "geometry.step_topology.render.request.a0";
        readonly resultContract: "geometry.step_topology.render.result.a0";
        readonly runtimeAvailable: false;
        readonly nativeRuntimeAvailable: true;
        readonly runtimeDispatch: "logical_dto";
        readonly inputAttachments: readonly [];
        readonly outputAttachments: readonly [{
            readonly name: "glb";
            readonly required: true;
            readonly media_types: readonly ["model/gltf-binary"];
            readonly max_bytes: 268435456;
        }, {
            readonly name: "topology_binding_table";
            readonly required: false;
            readonly media_types: readonly ["application/vnd.wavenumber.geometer.step-topology-binding-table"];
            readonly max_bytes: 134217728;
        }];
        readonly documentation: "";
    };
    readonly "geometry.step_topology.resolve_hit.a0": {
        readonly identity: "geometry.step_topology.resolve_hit.a0";
        readonly requestContract: "geometry.step_topology.resolve_hit.request.a0";
        readonly resultContract: "geometry.step_topology.resolve_hit.result.a0";
        readonly runtimeAvailable: false;
        readonly nativeRuntimeAvailable: true;
        readonly runtimeDispatch: "logical_dto";
        readonly inputAttachments: readonly [];
        readonly outputAttachments: readonly [];
        readonly documentation: "";
    };
    readonly "geometry.step_topology.restore.a0": {
        readonly identity: "geometry.step_topology.restore.a0";
        readonly requestContract: "geometry.step_topology.restore.request.a0";
        readonly resultContract: "geometry.step_topology.restore.result.a0";
        readonly runtimeAvailable: false;
        readonly nativeRuntimeAvailable: true;
        readonly runtimeDispatch: "logical_dto";
        readonly inputAttachments: readonly [{
            readonly name: "source";
            readonly required: true;
            readonly media_types: readonly ["application/step", "model/step"];
            readonly max_bytes: 268435456;
        }, {
            readonly name: "state_artifact";
            readonly required: true;
            readonly media_types: readonly ["application/vnd.wavenumber.geometer.step-topology-edit-journal"];
            readonly max_bytes: 67108864;
        }];
        readonly outputAttachments: readonly [];
        readonly documentation: "";
    };
    readonly "geometry.step_topology.save.a0": {
        readonly identity: "geometry.step_topology.save.a0";
        readonly requestContract: "geometry.step_topology.save.request.a0";
        readonly resultContract: "geometry.step_topology.save.result.a0";
        readonly runtimeAvailable: false;
        readonly nativeRuntimeAvailable: false;
        readonly runtimeDispatch: "logical_dto";
        readonly inputAttachments: readonly [];
        readonly outputAttachments: readonly [{
            readonly name: "state_artifact";
            readonly required: true;
            readonly media_types: readonly ["application/step", "application/vnd.opencascade.xbf", "application/vnd.opencascade.xml-xcaf", "application/vnd.wavenumber.geometer.step-topology-sidecar+json"];
            readonly max_bytes: 536870912;
        }];
        readonly documentation: "";
    };
};
export type OperationIdentity = keyof typeof operationCatalog;
export type ModelBoundsInputMediaType = (typeof operationCatalog)["geometry.model_bounds.a0"]["inputAttachments"][0]["media_types"][number];
