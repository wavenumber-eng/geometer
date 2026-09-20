import type { AnalyticPlanarBooleanBatchRequestA0, AnalyticPlanarBooleanBatchResultA0, DiagnosticA0, HlrProjectionOptionsA0, HlrProjectionResultA0, HlrProjectionResultB0, MeshCollectionA0, MeshHlrProjectionRequestB0, MeshIllustrationGeometryB0, MeshIllustrationGeometryInputB0, MeshIllustrationInputB0, MeshIllustrationResultB0, ModelBoundsInputMediaType, ModelBoundsOptionsA0, ModelBoundsResultA0, OperationOutcomeA0, OperationOutcomeB0 } from "./generated/index.js";
import type { IndexedTriangleMeshA0 } from "./indexed-mesh-packet-a0.js";
export interface EmscriptenGeometerModule {
    readonly HEAPU8: Uint8Array;
    readonly HEAPU32: Uint32Array;
    UTF8ToString(pointer: number): string;
    _free(pointer: number): void;
    _geometer_free_string(pointer: number): void;
    _geometer_operation_catalog_json(valueOut: number, errorOut: number): number;
    _geometer_operation_execute(operationId: number, operationIdSize: number, requestJson: number, requestJsonSize: number, attachments: number, attachmentCount: number, resultOut: number, errorOut: number): number;
    _geometer_operation_result_attachment_count(result: number): number;
    _geometer_operation_result_attachment_data(result: number, index: number, sizeOut: number): number;
    _geometer_operation_result_attachment_media_type(result: number, index: number, sizeOut: number): number;
    _geometer_operation_result_attachment_name(result: number, index: number, sizeOut: number): number;
    _geometer_operation_result_free(result: number): void;
    _geometer_operation_result_json_data(result: number): number;
    _geometer_operation_result_json_size(result: number): number;
    _malloc(size: number): number;
}
export type EmscriptenGeometerFactory = (options?: Readonly<Record<string, unknown>>) => Promise<EmscriptenGeometerModule>;
export interface GeometerOperationAttachment {
    readonly data: Uint8Array;
    readonly mediaType: string;
    readonly name: string;
}
export interface GeometerOperationResponse {
    readonly attachments: readonly GeometerOperationAttachment[];
    readonly outcome: OperationOutcomeA0 | OperationOutcomeB0;
}
export interface ModelBoundsRequest {
    readonly mediaType?: ModelBoundsInputMediaType;
    readonly model: Uint8Array;
    readonly options?: ModelBoundsOptionsA0;
}
export interface ModelHlrProjectionRequest {
    readonly mediaType?: "application/step" | "model/step";
    readonly model: Uint8Array;
    readonly options?: HlrProjectionOptionsA0;
}
export interface MeshHlrProjectionRequest {
    readonly meshCollection: MeshCollectionA0;
    readonly request: MeshHlrProjectionRequestB0;
}
export interface MeshHlrProjectionRequestA0 {
    readonly mesh: IndexedTriangleMeshA0 | Uint8Array;
    readonly options?: HlrProjectionOptionsA0;
}
export interface MeshIllustrationB0Request {
    readonly input: MeshIllustrationInputB0;
    readonly hlrProjection?: HlrProjectionResultB0;
}
export interface MeshIllustrationGeometryB0Request {
    readonly input: MeshIllustrationGeometryInputB0;
    readonly hlrProjection?: HlrProjectionResultB0;
}
export interface GeometerWasmCapabilityCatalog {
    readonly cAbiGeneration: number;
    readonly genericAbi: "a0";
    readonly releaseVersion: string;
    readonly operations: readonly string[];
}
export declare class GeometerWasmTransportError extends Error {
    readonly code: number;
    constructor(code: number, message: string);
}
export declare class GeometerOperationError extends Error {
    readonly diagnostics: readonly DiagnosticA0[];
    readonly operation: string;
    constructor(operation: string, diagnostics: readonly DiagnosticA0[]);
}
export declare class GeometerWasmClient {
    readonly capabilities: GeometerWasmCapabilityCatalog;
    private readonly module;
    private readonly runtimeCatalog;
    private constructor();
    static fromModule(module: EmscriptenGeometerModule): GeometerWasmClient;
    analyticPlanarBooleanBatch(request: AnalyticPlanarBooleanBatchRequestA0): Promise<AnalyticPlanarBooleanBatchResultA0>;
    modelBounds(request: ModelBoundsRequest): Promise<ModelBoundsResultA0>;
    modelHlrProjection(request: ModelHlrProjectionRequest): Promise<HlrProjectionResultA0>;
    meshHlrProjection(request: MeshHlrProjectionRequest): Promise<HlrProjectionResultB0>;
    meshHlrProjectionB0(request: MeshHlrProjectionRequest): Promise<HlrProjectionResultB0>;
    meshHlrProjectionA0(request: MeshHlrProjectionRequestA0): Promise<HlrProjectionResultA0>;
    meshIllustration(request: MeshIllustrationB0Request): Promise<MeshIllustrationResultB0>;
    meshIllustrationGeometry(request: MeshIllustrationGeometryB0Request): Promise<MeshIllustrationGeometryB0>;
    private hlrProjection;
    execute(operation: string, requestJson: string, attachments: readonly GeometerOperationAttachment[]): GeometerOperationResponse;
}
export declare function createGeometerWasmClient(moduleOrFactory: EmscriptenGeometerModule | EmscriptenGeometerFactory, moduleOptions?: Readonly<Record<string, unknown>>): Promise<GeometerWasmClient>;
