import type { AnalyticPlanarBooleanBatchResultA0 } from "./generated/contracts.js";
export interface DirectoryTable {
    readonly count: number;
    readonly kind: number;
    readonly offset: number;
    readonly recordBytes: number;
    readonly view: DataView;
}
export interface JobRecord {
    readonly jobId: bigint;
    readonly status: number;
    readonly diagnosticBegin: number;
    readonly diagnosticCount: number;
    readonly regionBegin: number;
    readonly regionCount: number;
    readonly eventBegin: number;
    readonly eventCount: number;
}
export interface VertexRecord {
    id: bigint;
    x: bigint;
    y: bigint;
    sourceSet: number;
    flags: number;
}
export interface FragmentRecord {
    id: bigint;
    start: number;
    end: number;
    kind: number;
    direction: number;
    major: boolean;
    radius: bigint;
    positiveSet: number;
    subtractionSet: number;
}
export interface RingRecord {
    id: bigint;
    referenceBegin: number;
    referenceCount: number;
    parent: number;
    depth: number;
    flags: number;
}
export interface RegionRecord {
    id: bigint;
    outer: number;
    positiveSet: number;
}
export interface SourceSetRecord {
    begin: number;
    count: number;
}
export interface SourceRecord {
    kind: number;
    role: number;
    operandId: bigint;
    primaryId: bigint;
    secondaryId: bigint;
}
export interface EventRecord {
    operandId: bigint;
    kind: number;
    referenceBegin: number;
    referenceCount: number;
    sourceSet: number;
}
export interface RelationshipRecord {
    queryId: bigint;
    status: number;
    dimension: number;
    pairBegin: number;
    pairCount: number;
}
export interface PairRecord {
    left: bigint;
    right: bigint;
    dimension: number;
    equality: boolean;
    leftContains: boolean;
    rightContains: boolean;
}
export interface DiagnosticRecord {
    code: number;
    severity: number;
    presence: number;
    jobId: bigint;
    stageId: bigint;
    operandId: bigint;
    geometryId: bigint;
    pathToken: number;
}
export interface ResultRecords {
    jobs: JobRecord[];
    diagnostics: DiagnosticRecord[];
    vertices: VertexRecord[];
    fragments: FragmentRecord[];
    rings: RingRecord[];
    fragmentReferences: number[];
    regions: RegionRecord[];
    ringRegionReferences: bigint[];
    sourceSets: SourceSetRecord[];
    sources: SourceRecord[];
    events: EventRecord[];
    relationships: RelationshipRecord[];
    pairs: PairRecord[];
    sourceIndices: number[];
}
/** Strictly decode GMABRS01 and project it to the public logical bigint DTO. */
export declare function decodeAnalyticPlanarBooleanBatchResultA0Packet(bytes: Uint8Array): Promise<AnalyticPlanarBooleanBatchResultA0>;
