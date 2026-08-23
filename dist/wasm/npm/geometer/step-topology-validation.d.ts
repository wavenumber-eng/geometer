import type { SessionReference, StepTopologyInspectResultA0, StepTopologyRenderResultA0, StepTopologyResolveHitRequestA0 } from "./generated/contracts.js";
export interface StepTopologySessionExpectation {
    readonly sessionHandle: string;
    readonly generation: number;
}
export interface StepTopologyNamedAttachment {
    readonly name: string;
    readonly mediaType: string;
    readonly data: Uint8Array;
}
export declare class StepTopologySemanticError extends Error {
    readonly code: string;
    constructor(code: string, message: string);
}
export declare function validateStepTopologySession(session: SessionReference, expected: StepTopologySessionExpectation): void;
export declare function validateStepTopologyResolveHitContext(request: StepTopologyResolveHitRequestA0, expected: StepTopologySessionExpectation): void;
export declare function validateStepTopologyInspection(result: StepTopologyInspectResultA0): void;
export declare class StepTopologyInspectionAccumulator {
    private session;
    private counts;
    private readonly cursors;
    private readonly handles;
    private readonly definitions;
    private readonly occurrences;
    private readonly bodies;
    private readonly shells;
    private readonly faces;
    private readonly bodyCountsByDefinition;
    private readonly faceCountsByDefinition;
    private rootOccurrenceCount;
    private componentOccurrenceCount;
    private complete;
    addPage(result: StepTopologyInspectResultA0): boolean;
    private add;
    private validateCompleteSnapshot;
    private requireDefinition;
    private validateAccumulatedCounts;
    private resolveOccurrenceDepth;
}
export declare function validateStepTopologyRenderAttachments(result: StepTopologyRenderResultA0, attachments: readonly StepTopologyNamedAttachment[]): Promise<void>;
