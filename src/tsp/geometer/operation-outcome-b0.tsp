import "@typespec/json-schema";
import "@wavenumber/wn-geometer-contract-emitter";
import "./common.tsp";
import "./operations/mesh-hlr-projection-b0.tsp";
import "./operations/mesh-illustration-b0.tsp";
import "./operations/mesh-illustration-geometry-b0.tsp";
import "./operations/model-illustration-b0.tsp";

using TypeSpec.JsonSchema;
using Wavenumber.Geometer.ContractMetadata;

namespace Wavenumber.Geometer.Contracts.OperationOutcomeB0;

@oneOf
union OperationResultValueB0 {
  model_illustration_geometry: Wavenumber.Geometer.Contracts.ModelIllustrationB0.ModelIllustrationGeometryResultB0,
  model_illustration: Wavenumber.Geometer.Contracts.ModelIllustrationB0.ModelIllustrationResultB0,
  mesh_illustration_geometry: Wavenumber.Geometer.Contracts.MeshIllustrationGeometryB0.MeshIllustrationGeometryResultB0,
  mesh_illustration: Wavenumber.Geometer.Contracts.MeshIllustrationB0.MeshIllustrationResultB0,
  mesh_hlr_projection: Wavenumber.Geometer.Contracts.MeshHlrProjectionB0.HlrProjectionResultB0,
}

model OperationSuccessB0 {
  @minLength(1) @maxLength(128) operation: string;
  ok: true;
  result: OperationResultValueB0;
}

model OperationFailureB0 {
  @minLength(1) @maxLength(128) operation: string;
  ok: false;
  @minItems(1) diagnostics: Wavenumber.Geometer.Contracts.Common.DiagnosticA0[];
}

@jsonSchema
@id("urn:wavenumber:schema:geometer:geometer.operation.outcome:b0")
@contractIdentity("geometer.operation.outcome.b0")
@oneOf
union OperationOutcomeB0 {
  success: OperationSuccessB0,
  failure: OperationFailureB0,
}
