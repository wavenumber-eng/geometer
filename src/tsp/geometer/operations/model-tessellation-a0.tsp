import "@typespec/json-schema";
import "@wavenumber/wn-geometer-contract-emitter";
import "./mesh-illustration-a0.tsp";

using TypeSpec.JsonSchema;
using Wavenumber.Geometer.ContractMetadata;

namespace Wavenumber.Geometer.Contracts.ModelTessellationA0;

/** Shared colored indexed meshes. Coordinates and matrix translations are millimeters. */
@jsonSchema
@id("urn:wavenumber:schema:geometer:geometry.mesh_collection:a0")
@contractIdentity("geometry.mesh_collection.a0")
model MeshCollectionA0 {
  schema: "geometry.mesh_collection.a0";
  length_unit: "millimeter";
  @minItems(1) @maxItems(65536)
  meshes: Wavenumber.Geometer.Contracts.MeshIllustrationA0.MeshIllustrationMesh[];
}

enum ModelRootPlacement {
  /** Match the existing STEP-to-GLB path: remove free-root placements only. */
  strip,
  /** Retain every source root and component placement. */
  preserve,
}

/** Stateless STEP tessellation; component placement is retained in either root mode. */
@jsonSchema
@id("urn:wavenumber:schema:geometer:geometry.model_tessellation.request:a0")
@contractIdentity("geometry.model_tessellation.request.a0")
model ModelTessellationRequestA0 {
  schema: "geometry.model_tessellation.request.a0";
  @minValue(0.000001) @maxValue(1000)
  linear_deflection_mm?: float64 = 0.1;
  @minValue(0.000001) @maxValue(3.141592653589793)
  angular_deflection_rad?: float64 = 0.5;
  root_placement?: ModelRootPlacement = ModelRootPlacement.strip;
  @minValue(1) @maxValue(2000000)
  max_triangles?: uint32 = 750000;
}

/** UTF-8 JSON attachment governed by geometry.mesh_collection.a0, not an opaque mesh format. */
model MeshCollectionAttachment {
  attachment: "mesh_collection";
  schema: "geometry.mesh_collection.a0";
  @minValue(1) @maxValue(268435456) byte_length: uint32;
  @minLength(64) @maxLength(64) sha256: string;
}

@jsonSchema
@id("urn:wavenumber:schema:geometer:geometry.model_tessellation.result:a0")
@contractIdentity("geometry.model_tessellation.result.a0")
model ModelTessellationResultA0 {
  schema: "geometry.model_tessellation.result.a0";
  mesh_collection: MeshCollectionAttachment;
  @minLength(64) @maxLength(64) source_sha256: string;
  @minValue(1) @maxValue(65536) meshes: uint32;
  @minValue(1) @maxValue(2000000) triangles: uint32;
  @maxItems(256) warnings: string[];
}
