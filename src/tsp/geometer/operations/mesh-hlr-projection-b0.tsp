import "@typespec/json-schema";
import "@wavenumber/wn-geometer-contract-emitter";
import "./hlr-projection-a0.tsp";
import "./illustration-clipping-b0.tsp";

using TypeSpec.JsonSchema;
using Wavenumber.Geometer.ContractMetadata;

namespace Wavenumber.Geometer.Contracts.MeshHlrProjectionB0 {

/** Fast-only HLR request over the governed colored mesh collection. */
@jsonSchema
@id("urn:wavenumber:schema:geometer:geometry.mesh_hlr_projection.request:b0")
@contractIdentity("geometry.mesh_hlr_projection.request.b0")
model MeshHlrProjectionRequestB0 {
  schema: "geometry.mesh_hlr_projection.request.b0";
  @maxItems(64) views?: Wavenumber.Geometer.Contracts.HlrProjectionA0.HlrViewSpec[];
  output_outline?: boolean = true;
  output_detail?: boolean = true;
  output_bbox?: boolean = true;
  model_transform?: Wavenumber.Geometer.Contracts.HlrProjectionA0.HlrMatrix4x4;
  @minValue(0) @maxValue(9) round_digits?: uint32 = 3;
  fast?: Wavenumber.Geometer.Contracts.HlrProjectionA0.FastHlrOptionsA0;
  clipping?: Wavenumber.Geometer.Contracts.IllustrationClippingB0.IllustrationClipping;
}

model MeshCollectionHlrSource {
  kind: "mesh_collection";
  @minLength(64) @maxLength(64) hash: string;
}

@jsonSchema
@id("urn:wavenumber:schema:geometer:geometry.hlr_projection.result:b0")
@contractIdentity("geometry.hlr_projection.result.b0")
model HlrProjectionResultB0 {
  schema: "geometry.hlr_projection.result.b0";
  units: "mm";
  empty: boolean;
  source: MeshCollectionHlrSource;
  @maxItems(64) views: Wavenumber.Geometer.Contracts.HlrProjectionA0.HlrProjectedView[];
  timings: Wavenumber.Geometer.Contracts.HlrProjectionA0.HlrProjectionTimings;
  fragment: Wavenumber.Geometer.Contracts.IllustrationClippingB0.FragmentMetadata;
}
}

namespace Wavenumber.Geometer.Contracts.Operations {

/** Project a colored mesh collection through deterministic clipping and Fast HLR. */
@operationIdentity("geometry.mesh_hlr_projection.b0")
@inputAttachment("mesh_collection", true, "application/vnd.wavenumber.geometer.mesh-collection+json", 268435456)
op meshHlrProjectionB0(
  request: Wavenumber.Geometer.Contracts.MeshHlrProjectionB0.MeshHlrProjectionRequestB0
): Wavenumber.Geometer.Contracts.MeshHlrProjectionB0.HlrProjectionResultB0;
}
