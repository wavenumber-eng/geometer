export interface ContractConstraints {
    readonly max_items?: number;
    readonly max_length?: number;
    readonly max_value?: number;
    readonly max_value_exclusive?: number;
    readonly min_items?: number;
    readonly min_length?: number;
    readonly min_value?: number;
    readonly min_value_exclusive?: number;
}
export type ContractTypeDescriptor = {
    readonly kind: "array";
    readonly element: ContractTypeDescriptor;
} | {
    readonly kind: "literal";
    readonly value: unknown;
    readonly value_type: string;
} | {
    readonly kind: "primitive";
    readonly name: string;
} | {
    readonly kind: "reference";
    readonly target: string;
};
interface ContractPropertyDescriptor {
    readonly constraints: ContractConstraints;
    readonly optional: boolean;
    readonly type: ContractTypeDescriptor;
}
export type ContractDescriptor = {
    readonly constraints: ContractConstraints;
    readonly element: ContractTypeDescriptor;
    readonly kind: "array";
} | {
    readonly kind: "enum";
    readonly values: readonly unknown[];
} | {
    readonly kind: "object";
    readonly properties: Readonly<Record<string, ContractPropertyDescriptor>>;
} | {
    readonly kind: "union";
    readonly variants: readonly ContractTypeDescriptor[];
};
export type ContractDescriptorMap = Readonly<Record<string, ContractDescriptor>>;
export declare class ContractCodecError extends Error {
    readonly code: string;
    readonly path: string;
    constructor(code: string, path: string, message: string);
}
export declare function decodeContractJson(data: string | Uint8Array, type: ContractTypeDescriptor, declarations: ContractDescriptorMap): unknown;
export declare function encodeContractJson(value: unknown, type: ContractTypeDescriptor, declarations: ContractDescriptorMap): string;
export {};
