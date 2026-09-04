import type { RecoveryGroupRequest, RecoveryGroupResult, SessionReference, StepTopologyApplyHierarchyRequestA0, StepTopologyApplyHierarchyResultA0, StepTopologyApplyLogicalGroupsRequestA0, StepTopologyApplyLogicalGroupsResultA0, StepTopologyApplyMetadataProbesRequestA0, StepTopologyApplyMetadataProbesResultA0, StepTopologyCheckpointEditJournalResultA0, StepTopologyInspectResultA0, StepTopologyRenderResultA0, StepTopologyResolveHitRequestA0, StepTopologyRestoreRequestA0, StepTopologyRestoreResultA0, StepTopologySaveRequestA0, StepTopologySaveResultA0 } from "./generated/contracts.js";
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
export declare function validateStepTopologyLogicalGroupCommands(request: StepTopologyApplyLogicalGroupsRequestA0): void;
export declare function validateStepTopologyLogicalGroupResult(result: StepTopologyApplyLogicalGroupsResultA0 | StepTopologyApplyMetadataProbesResultA0): void;
export declare function validateStepTopologyMetadataProbeCommands(request: StepTopologyApplyMetadataProbesRequestA0): void;
export declare function validateStepTopologyHierarchyCommands(request: StepTopologyApplyHierarchyRequestA0): void;
export declare function validateStepTopologyHierarchyResult(result: StepTopologyApplyHierarchyResultA0): void;
export declare function validateStepTopologyRecoveryRequest(groups: readonly RecoveryGroupRequest[]): void;
export declare function validateStepTopologyRecoveryResults(groups: readonly RecoveryGroupResult[]): void;
export declare function validateStepTopologySaveAttachments(request: StepTopologySaveRequestA0, result: StepTopologySaveResultA0, attachments: readonly StepTopologyNamedAttachment[]): Promise<void>;
export declare function validateStepTopologyRestoreAttachments(request: StepTopologyRestoreRequestA0, attachments: readonly StepTopologyNamedAttachment[]): Promise<void>;
export declare function validateStepTopologyRestoreResult(request: StepTopologyRestoreRequestA0, result: StepTopologyRestoreResultA0): void;
export declare function validateStepTopologyCheckpointAttachment(result: StepTopologyCheckpointEditJournalResultA0, attachments: readonly StepTopologyNamedAttachment[]): Promise<void>;
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
    private readonly memberships;
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
