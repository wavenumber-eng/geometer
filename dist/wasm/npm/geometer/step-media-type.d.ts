/** Canonical STEP input attachment type; legacy application/step remains accepted. */
export declare const STEP_MEDIA_TYPE = "model/step";
/** Input syntax failure, distinct from transport or runtime failures. */
export declare class StepMediaTypeError extends Error {
}
/** Validate the complete bounded RFC 2045 field, without interpreting auxiliary parameters. */
export declare function canonicalStepMediaType(value: string): string | undefined;
/** Normalize only a negotiated STEP input slot; non-STEP attachment identities remain exact. */
export declare function stepInputMediaType(value: string, declared: readonly string[]): string;
/** Match STEP request aliases against either declared STEP token; all other types stay exact. */
export declare function stepInputMatchesDeclaration(value: string, declared: readonly string[]): boolean;
