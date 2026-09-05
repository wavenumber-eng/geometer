import "@typespec/json-schema";
import "@wavenumber/wn-geometer-contract-emitter";
import "./mesh-illustration-a0.tsp";
import "./model-tessellation-a0.tsp";

using TypeSpec.JsonSchema;
using Wavenumber.Geometer.ContractMetadata;

namespace Wavenumber.Geometer.Contracts.MeshIllustrationOperationA0 {
  /**
   * Native one-shot illustration settings. Meshes arrive in the required
   * mesh_collection attachment governed by geometry.mesh_collection.a0.
   * Reuses existing illustration A0 options and result; does not compute HLR.
   * Optional hlr_projection attachment is geometry.hlr_projection.result.a0:
   * exactly one matching view, millimeters, polyline outline/detail only.
   * Supply visibility-filtered HLR from the same model, placement and transform
   * as the meshes. The renderer mirrors and composes detail then outline over
   * surfaces according to show_hlr_detail/show_hlr_outline; it does not infer
   * visibility from arbitrary supplied segments. Maximum 1,000,000 segments.
   */
  @jsonSchema
  @id("urn:wavenumber:schema:geometer:geometry.mesh_illustration.request:a0")
  @contractIdentity("geometry.mesh_illustration.request.a0")
  model MeshIllustrationRequestA0 {
    schema: "geometry.mesh_illustration.request.a0";
    view: Wavenumber.Geometer.Contracts.MeshIllustrationA0.MeshIllustrationView;
    prepare?: Wavenumber.Geometer.Contracts.MeshIllustrationA0.MeshIllustrationPrepareOptions;
    style?: Wavenumber.Geometer.Contracts.MeshIllustrationA0.MeshIllustrationStyleA0;
    svg?: Wavenumber.Geometer.Contracts.MeshIllustrationA0.MeshIllustrationSvgOptions;
  }
}

namespace Wavenumber.Geometer.Contracts.Operations {
  /** Render colored meshes to the existing deterministic A0 SVG result without JavaScript. */
  @operationIdentity("geometry.mesh_illustration.a0")
  @inputAttachment("mesh_collection", true, "application/vnd.wavenumber.geometer.mesh-collection+json", 268435456)
  @inputAttachment("hlr_projection", false, "application/vnd.wavenumber.geometer.hlr-projection+json", 67108864)
  op meshIllustration(
    request: Wavenumber.Geometer.Contracts.MeshIllustrationOperationA0.MeshIllustrationRequestA0
  ): Wavenumber.Geometer.Contracts.MeshIllustrationA0.MeshIllustrationResultA0;
}
