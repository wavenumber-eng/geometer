import "@typespec/json-schema";
import "@wavenumber/wn-geometer-contract-emitter";

using TypeSpec.JsonSchema;
using Wavenumber.Geometer.ContractMetadata;

namespace Wavenumber.Geometer.Contracts.ModelBoundsA0;

/** Canonical model source format. Compatibility readers may additionally accept STEP. */
enum ModelFormat {
  step: "step",
}

/** Exactly three finite millimeter coordinates. */
@minItems(3)
@maxItems(3)
model Vector3 is float64[];

/** Row-major affine 4x4 matrix. Semantic validation requires final row [0,0,0,1]. */
@minItems(16)
@maxItems(16)
model Matrix4x4 is float64[];

/** Presence-preserving patch applied over focused C++ defaults. */
@jsonSchema
@id("urn:wavenumber:schema:geometer:geometry.model_bounds.options:a0")
@contractIdentity("geometry.model_bounds.options.a0")
@optionPatch
model ModelBoundsOptionsA0 {
  /** Absent preserves the inherited value; canonical default intent is step. */
  format?: ModelFormat = ModelFormat.step;

  /** Absent preserves the inherited transform; canonical default intent is identity. */
  @extension("x-wn-default-intent", "identity_4x4")
  model_transform?: Matrix4x4;
}

/** Source identity included in a successful model-bounds result. */
model ModelBoundsSource {
  format: ModelFormat;
  hash: string;
}

/** Axis-aligned bounds in millimeters. */
model ModelBoundsValues {
  min: Vector3;
  max: Vector3;
  size: Vector3;
  center: Vector3;
}

/** Nondeterministic timings excluded only by explicit conformance projection. */
model ModelBoundsTimings {
  @minValue(0)
  model_read_ms: float64;

  @minValue(0)
  bounds_ms: float64;
}

/** Successful model-bounds result matching the compatible public JSON shape. */
@jsonSchema
@id("urn:wavenumber:schema:geometer:geometry.model_bounds:a0")
@contractIdentity("geometry.model_bounds.a0")
model ModelBoundsResultA0 {
  schema: "geometry.model_bounds.a0";
  units: "mm";
  source: ModelBoundsSource;
  bounds: ModelBoundsValues;
  timings: ModelBoundsTimings;
}
