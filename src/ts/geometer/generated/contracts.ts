// Generated from wn_geometer_contract_catalog.a0.json. Do not edit.

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
  number,
];

/** Canonical model source format. Compatibility readers may additionally accept STEP. */
export type ModelFormat = "step";

/** Presence-preserving patch applied over focused C++ defaults. */
export interface ModelBoundsOptionsA0 {
  /** Absent preserves the inherited value; canonical default intent is step. */
  readonly format?: ModelFormat;
  /** Absent preserves the inherited transform; canonical default intent is identity. */
  readonly model_transform?: Matrix4x4;
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
export type OperationResultValueA0 = ModelBoundsResultA0;

/** A completed operation with its operation-specific result. */
export interface OperationSuccessA0 {
  readonly operation: string;
  readonly ok: true;
  readonly result: OperationResultValueA0;
}

/** Transport-neutral typed outcome shared by the generic C ABI and executable IPC. */
export type OperationOutcomeA0 = OperationSuccessA0 | OperationFailureA0;
