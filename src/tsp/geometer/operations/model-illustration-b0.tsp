import "@typespec/json-schema";
import "@wavenumber/wn-geometer-contract-emitter";
import "./illustration-clipping-b0.tsp";
import "./mesh-illustration-a0.tsp";
import "./mesh-illustration-geometry-b0.tsp";
import "./model-illustration-a0.tsp";

using TypeSpec.JsonSchema;
using Wavenumber.Geometer.ContractMetadata;

namespace Wavenumber.Geometer.Contracts.ModelIllustrationB0 {

@jsonSchema
@id("urn:wavenumber:schema:geometer:geometry.model_illustration.request:b0")
@contractIdentity("geometry.model_illustration.request.b0")
model ModelIllustrationRequestB0 {
  schema: "geometry.model_illustration.request.b0";
  source: Wavenumber.Geometer.Contracts.ModelIllustrationA0.ModelIllustrationSourceA0;
  view: Wavenumber.Geometer.Contracts.MeshIllustrationA0.MeshIllustrationView;
  prepare?: Wavenumber.Geometer.Contracts.MeshIllustrationA0.MeshIllustrationPrepareOptions;
  linework?: Wavenumber.Geometer.Contracts.ModelIllustrationA0.ModelIllustrationLineworkOptionsA0;
  style?: Wavenumber.Geometer.Contracts.MeshIllustrationA0.MeshIllustrationStyleA0;
  svg?: Wavenumber.Geometer.Contracts.MeshIllustrationA0.MeshIllustrationSvgOptions;
  work_limits?: Wavenumber.Geometer.Contracts.ModelIllustrationA0.ModelIllustrationWorkLimitsA0;
  clipping?: Wavenumber.Geometer.Contracts.IllustrationClippingB0.IllustrationClipping;
}

@jsonSchema
@id("urn:wavenumber:schema:geometer:geometry.model_illustration_geometry.request:b0")
@contractIdentity("geometry.model_illustration_geometry.request.b0")
model ModelIllustrationGeometryRequestB0 {
  schema: "geometry.model_illustration_geometry.request.b0";
  source: Wavenumber.Geometer.Contracts.ModelIllustrationA0.ModelIllustrationSourceA0;
  view: Wavenumber.Geometer.Contracts.MeshIllustrationA0.MeshIllustrationView;
  prepare?: Wavenumber.Geometer.Contracts.MeshIllustrationA0.MeshIllustrationPrepareOptions;
  linework?: Wavenumber.Geometer.Contracts.ModelIllustrationA0.ModelIllustrationLineworkOptionsA0;
  style?: Wavenumber.Geometer.Contracts.MeshIllustrationA0.MeshIllustrationStyleA0;
  work_limits?: Wavenumber.Geometer.Contracts.ModelIllustrationA0.ModelIllustrationWorkLimitsA0;
  clipping?: Wavenumber.Geometer.Contracts.IllustrationClippingB0.IllustrationClipping;
}

@jsonSchema
@id("urn:wavenumber:schema:geometer:geometry.model_illustration.result:b0")
@contractIdentity("geometry.model_illustration.result.b0")
model ModelIllustrationResultB0 {
  schema: "geometry.model_illustration.result.b0";
  empty: boolean;
  svg: string;
  bounds_mm?: Wavenumber.Geometer.Contracts.ModelIllustrationA0.ModelIllustrationBounds3MmA0;
  source: Wavenumber.Geometer.Contracts.ModelIllustrationA0.ModelIllustrationSourceSummaryA0;
  stats: Wavenumber.Geometer.Contracts.MeshIllustrationA0.MeshIllustrationRenderStats;
  timings: Wavenumber.Geometer.Contracts.ModelIllustrationA0.ModelIllustrationTimingsA0;
  fragment: Wavenumber.Geometer.Contracts.IllustrationClippingB0.FragmentMetadata;
  @maxItems(256) warnings: string[];
}

@jsonSchema
@id("urn:wavenumber:schema:geometer:geometry.model_illustration_geometry.result:b0")
@contractIdentity("geometry.model_illustration_geometry.result.b0")
model ModelIllustrationGeometryResultB0 {
  schema: "geometry.model_illustration_geometry.result.b0";
  empty: boolean;
  geometry: Wavenumber.Geometer.Contracts.MeshIllustrationGeometryB0.IllustrationGeometryAttachmentB0;
  bounds_mm?: Wavenumber.Geometer.Contracts.ModelIllustrationA0.ModelIllustrationBounds3MmA0;
  source: Wavenumber.Geometer.Contracts.ModelIllustrationA0.ModelIllustrationSourceSummaryA0;
  stats: Wavenumber.Geometer.Contracts.MeshIllustrationA0.MeshIllustrationRenderStats;
  timings: Wavenumber.Geometer.Contracts.ModelIllustrationA0.ModelIllustrationTimingsA0;
  fragment: Wavenumber.Geometer.Contracts.IllustrationClippingB0.FragmentMetadata;
  @maxItems(256) warnings: string[];
}
}

namespace Wavenumber.Geometer.Contracts.Operations {

@operationIdentity("geometry.model_illustration.b0")
@inputAttachment("model", false, "application/step,model/step", 268435456)
op modelIllustrationB0(
  request: Wavenumber.Geometer.Contracts.ModelIllustrationB0.ModelIllustrationRequestB0
): Wavenumber.Geometer.Contracts.ModelIllustrationB0.ModelIllustrationResultB0;

@operationIdentity("geometry.model_illustration_geometry.b0")
@inputAttachment("model", false, "application/step,model/step", 268435456)
@outputAttachment("illustration_geometry", true, "application/vnd.wavenumber.geometer.illustration-geometry+json", 268435456)
op modelIllustrationGeometryB0(
  request: Wavenumber.Geometer.Contracts.ModelIllustrationB0.ModelIllustrationGeometryRequestB0
): Wavenumber.Geometer.Contracts.ModelIllustrationB0.ModelIllustrationGeometryResultB0;
}
