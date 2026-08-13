import "@typespec/json-schema";
import "@wavenumber/wn-geometer-contract-emitter";
import "./common.tsp";
import "./operations/model-bounds-a0.tsp";

using TypeSpec.JsonSchema;
using Wavenumber.Geometer.ContractMetadata;

namespace Wavenumber.Geometer.Contracts.OperationOutcomeA0;

/** Results currently available through the generic operation transport. */
@oneOf
union OperationResultValueA0 {
  model_bounds: Wavenumber.Geometer.Contracts.ModelBoundsA0.ModelBoundsResultA0,
}

/** A completed operation with its operation-specific result. */
model OperationSuccessA0 {
  @minLength(1)
  @maxLength(128)
  operation: string;

  ok: true;
  result: OperationResultValueA0;
}

/** A rejected or failed operation with governed diagnostics. */
model OperationFailureA0 {
  @minLength(1)
  @maxLength(128)
  operation: string;

  ok: false;

  @minItems(1)
  diagnostics: Wavenumber.Geometer.Contracts.Common.DiagnosticA0[];
}

/** Transport-neutral typed outcome shared by the generic C ABI and executable IPC. */
@jsonSchema
@id("urn:wavenumber:schema:geometer:geometer.operation.outcome:a0")
@contractIdentity("geometer.operation.outcome.a0")
@oneOf
union OperationOutcomeA0 {
  success: OperationSuccessA0,
  failure: OperationFailureA0,
}
