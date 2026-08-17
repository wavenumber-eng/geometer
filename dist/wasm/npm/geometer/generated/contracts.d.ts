/** Stable diagnostic category with distinct transport, contract, and operation domains. */
export type DiagnosticCategory = "transport" | "contract" | "operation";
/** A strict cross-transport diagnostic; message text is informative rather than identity. */
export interface DiagnosticA0 {
    /** Stable namespaced diagnostic identity. */
    readonly code: string;
    readonly category: DiagnosticCategory;
    /** Human-readable non-normative detail. */
    readonly message: string;
    readonly retryable: boolean;
    /** RFC 6901 JSON Pointer when a document location is meaningful. */
    readonly path?: string;
    /** Stable operation identity when the diagnostic is operation-scoped. */
    readonly operation?: string;
    /** Transport correlation identifier when available. */
    readonly request_id?: string;
}
/** A named binary attachment carrying a separately governed packed projection. */
export interface PackedAttachmentReferenceA0 {
    readonly attachment: string;
    readonly format: string;
}
/** Minimal JSON value used while an operation's logical DTO projection remains deferred. */
export interface PackedAttachmentProjectionA0 {
    readonly schema: string;
    readonly packet: PackedAttachmentReferenceA0;
}
/** Named raw-attachment declaration in the negotiated operation catalog. */
export interface IpcAttachmentDeclarationA0 {
    readonly name: string;
    readonly required: boolean;
    readonly media_types: readonly string[];
    readonly max_bytes: number;
}
/** wasm32 attachment-descriptor member offsets. */
export interface IpcAttachmentOffsetsWasm32A0 {
    readonly struct_size: number;
    readonly flags: number;
    readonly name: number;
    readonly name_size: number;
    readonly media_type: number;
    readonly media_type_size: number;
    readonly data: number;
    readonly data_size: number;
    readonly reserved0: number;
}
export interface IpcAttachmentLayoutWasm32A0 {
    readonly size: number;
    readonly offsets: IpcAttachmentOffsetsWasm32A0;
}
/** 64-bit attachment-descriptor member offsets. */
export interface IpcAttachmentOffsetsPointer64A0 {
    readonly struct_size: number;
    readonly flags: number;
    readonly name: number;
    readonly name_size: number;
    readonly media_type: number;
    readonly media_type_size: number;
    readonly data: number;
    readonly data_size: number;
    readonly reserved0: number;
}
export interface IpcAttachmentLayoutPointer64A0 {
    readonly size: number;
    readonly offsets: IpcAttachmentOffsetsPointer64A0;
}
export interface IpcAttachmentDescriptorA0 {
    readonly wasm32: IpcAttachmentLayoutWasm32A0;
    readonly pointer64: IpcAttachmentLayoutPointer64A0;
}
/** Successful queue-only cancellation outcome. */
export interface IpcCancelledA0 {
    readonly status: "cancelled";
}
/** Nonterminal cancellation rejection for an active or unknown request. */
export interface IpcCancelRejectedA0 {
    readonly status: "rejected";
    readonly diagnostic: DiagnosticA0;
}
/** Effective executable IPC A0 limits selected by the server. */
export interface IpcEffectiveLimitsA0 {
    readonly json_bytes: number;
    readonly attachment_count: number;
    readonly attachment_name_bytes: number;
    readonly attachment_media_type_bytes: number;
    readonly attachment_bytes: number;
    readonly frame_bytes: number;
    readonly queued_requests: number;
    readonly queued_bytes: number;
    readonly resident_request_bytes: number;
    readonly pending_writer_bytes: number;
}
/** Bounds shared by the generic C ABI operation catalog. */
export interface IpcGenericAbiLimitsA0 {
    readonly operation_id_bytes: number;
    readonly request_json_bytes: number;
    readonly response_json_bytes: number;
    readonly attachment_count: number;
    readonly attachment_name_bytes: number;
    readonly attachment_media_type_bytes: number;
    readonly attachment_bytes: number;
    readonly aggregate_attachment_bytes_native: number;
    readonly aggregate_attachment_bytes_wasm: number;
}
/** Client handshake for executable IPC A0. */
export interface IpcHelloA0 {
    readonly client_name: string;
    readonly client_version: string;
    readonly protocols: readonly string[];
    readonly capabilities?: readonly string[];
}
export type IpcRuntimeDispatchA0 = "logical_dto" | "packed_attachment";
export interface IpcPackedProjectionA0 {
    readonly kind: "packed_attachment";
    readonly attachment_name: string;
    readonly format: string;
}
/** One operation exposed by the negotiated generic transport. */
export interface IpcOperationDeclarationA0 {
    readonly identity: string;
    readonly request_contract: string;
    readonly result_contract: string;
    readonly runtime_dispatch: IpcRuntimeDispatchA0;
    readonly input_attachments: readonly IpcAttachmentDeclarationA0[];
    readonly output_attachments: readonly IpcAttachmentDeclarationA0[];
    readonly request_projection?: IpcPackedProjectionA0;
    readonly result_projection?: IpcPackedProjectionA0;
}
/** Exact operation catalog embedded into the welcome frame. */
export interface IpcOperationCatalogA0 {
    readonly catalog: "wn.geometer.operation_catalog.a0";
    readonly generic_abi: "a0";
    readonly release_version: string;
    readonly c_abi_generation: number;
    readonly operations: readonly IpcOperationDeclarationA0[];
    readonly attachment_descriptor: IpcAttachmentDescriptorA0;
    readonly limits: IpcGenericAbiLimitsA0;
}
/** Fatal connection-level protocol diagnostic. */
export interface IpcProtocolErrorA0 {
    readonly status: "protocol_error";
    readonly diagnostic: DiagnosticA0;
}
/** Optional human-readable reason used by cancel and shutdown controls. */
export interface IpcReasonA0 {
    readonly reason?: string;
}
/** Canonical model source format. Compatibility readers may additionally accept STEP. */
export type ModelFormat = "step";
/** Row-major affine 4x4 matrix. Semantic validation requires final row [0,0,0,1]. */
export type Matrix4x4 = readonly [
    number,
    number,
    number,
    number,
    number,
    number,
    number,
    number,
    number,
    number,
    number,
    number,
    number,
    number,
    number,
    number
];
/** Presence-preserving patch applied over focused C++ defaults. */
export interface ModelBoundsOptionsA0 {
    /** Absent preserves the inherited value; canonical default intent is step. */
    readonly format?: ModelFormat;
    /** Absent preserves the inherited transform; canonical default intent is identity. */
    readonly model_transform?: Matrix4x4;
}
/** Strict generic request envelope for executable IPC A0. */
export interface IpcRequestA0 {
    readonly operation: string;
    readonly request: ModelBoundsOptionsA0;
}
/** Final acknowledgment after every accepted request is terminal and flushed. */
export interface IpcShutdownAckA0 {
    readonly status: "complete";
    readonly activeRequestCompleted: boolean;
    readonly rejectedQueuedRequestCount: number;
}
/** Server handshake selecting executable IPC A0 and its effective limits. */
export interface IpcWelcomeA0 {
    readonly release_version: string;
    readonly c_abi_generation: number;
    readonly ipc: "a0";
    readonly catalog_sha256: string;
    readonly operation_catalog: IpcOperationCatalogA0;
    readonly limits: IpcEffectiveLimitsA0;
    readonly capabilities: readonly string[];
}
/** Source identity included in a successful model-bounds result. */
export interface ModelBoundsSource {
    readonly format: ModelFormat;
    readonly hash: string;
}
/** Exactly three finite millimeter coordinates. */
export type Vector3 = readonly [number, number, number];
/** Axis-aligned bounds in millimeters. */
export interface ModelBoundsValues {
    readonly min: Vector3;
    readonly max: Vector3;
    readonly size: Vector3;
    readonly center: Vector3;
}
/** Nondeterministic timings excluded only by explicit conformance projection. */
export interface ModelBoundsTimings {
    readonly model_read_ms: number;
    readonly bounds_ms: number;
}
/** Successful model-bounds result matching the compatible public JSON shape. */
export interface ModelBoundsResultA0 {
    readonly schema: "geometry.model_bounds.a0";
    readonly units: "mm";
    readonly source: ModelBoundsSource;
    readonly bounds: ModelBoundsValues;
    readonly timings: ModelBoundsTimings;
}
/** A rejected or failed operation with governed diagnostics. */
export interface OperationFailureA0 {
    readonly operation: string;
    readonly ok: false;
    readonly diagnostics: readonly DiagnosticA0[];
}
/** Results currently available through the generic operation transport. */
export type OperationResultValueA0 = ModelBoundsResultA0 | PackedAttachmentProjectionA0;
/** A completed operation with its operation-specific result. */
export interface OperationSuccessA0 {
    readonly operation: string;
    readonly ok: true;
    readonly result: OperationResultValueA0;
}
/** Transport-neutral typed outcome shared by the generic C ABI and executable IPC. */
export type OperationOutcomeA0 = OperationSuccessA0 | OperationFailureA0;
