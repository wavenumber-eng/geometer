import "@typespec/json-schema";
import "@wavenumber/wn-geometer-contract-emitter";
import "./operations/mesh-hlr-projection-b0.tsp";
import "./operations/mesh-illustration-b0.tsp";
import "./operations/mesh-illustration-geometry-b0.tsp";
import "./operations/model-illustration-b0.tsp";

using TypeSpec.JsonSchema;
using Wavenumber.Geometer.ContractMetadata;

namespace Wavenumber.Geometer.Contracts.IpcB0;

union IpcRequestValueB0 {
  model_illustration_geometry: Wavenumber.Geometer.Contracts.ModelIllustrationB0.ModelIllustrationGeometryRequestB0,
  model_illustration: Wavenumber.Geometer.Contracts.ModelIllustrationB0.ModelIllustrationRequestB0,
  mesh_illustration_geometry: Wavenumber.Geometer.Contracts.MeshIllustrationGeometryB0.MeshIllustrationGeometryRequestB0,
  mesh_illustration: Wavenumber.Geometer.Contracts.MeshIllustrationB0.MeshIllustrationRequestB0,
  mesh_hlr_projection: Wavenumber.Geometer.Contracts.MeshHlrProjectionB0.MeshHlrProjectionRequestB0,
}

@jsonSchema
@id("urn:wavenumber:schema:geometer:geometer.ipc.request:b0")
@contractIdentity("geometer.ipc.request.b0")
model IpcRequestB0 {
  @minLength(1) @maxLength(128) operation: string;
  request: IpcRequestValueB0;
}
