import type { DirectoryTable, ResultRecords } from "./analytic-packet-a0-result.js";
export declare function validateResultRecords(records: ResultRecords, tables: readonly DirectoryTable[]): Selection[];
export interface Selection {
    vertices: number[];
    fragments: number[];
    rings: number[];
    regions: number[];
    events: number[];
    sets: number[];
    sources: number[];
}
export declare function encodeStandalone(input: ResultRecords, jobIndex: number, selection: Selection): Uint8Array;
export declare function encodeResultRecords(records: ResultRecords): Uint8Array;
