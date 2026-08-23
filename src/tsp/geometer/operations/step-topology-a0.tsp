import "@typespec/json-schema";
import "@wavenumber/wn-geometer-contract-emitter";
import "../common.tsp";

using TypeSpec.JsonSchema;
using Wavenumber.Geometer.ContractMetadata;

namespace Wavenumber.Geometer.Contracts.StepTopologyA0;

model SessionReference {
  @minLength(68) @maxLength(68) session_handle: string;
  @minValue(1) generation: uint32;
}

model SourceDescriptor {
  format: "step";
  @minLength(64) @maxLength(64) sha256: string;
  @minValue(1)
  @maxValue(268435456)
  bytes: uint32;
  normalized_length_unit: "millimeter";
}

model ToolDescriptor {
  name: "geometer";
  @minLength(1) @maxLength(64) release_version: string;
  @minLength(1) @maxLength(64) occt_version: string;
}

model TopologyTableAttachmentDescriptor {
  name: "topology_table";
  media_type: "application/vnd.wavenumber.geometer.step-topology-table";
  format: "wn.geometer.step-topology-table.a0";
  @minValue(1) @maxValue(134217728) bytes: uint32;
  @minLength(64) @maxLength(64) sha256: string;
}

model GlbAttachmentDescriptor {
  name: "glb";
  media_type: "model/gltf-binary";
  format: "glb-2.0";
  @minValue(1) @maxValue(268435456) bytes: uint32;
  @minLength(64) @maxLength(64) sha256: string;
}

model TopologyBindingTableAttachmentDescriptor {
  name: "topology_binding_table";
  media_type: "application/vnd.wavenumber.geometer.step-topology-binding-table";
  format: "wn.geometer.step-topology-binding-table.a0";
  @minValue(1) @maxValue(134217728) bytes: uint32;
  @minLength(64) @maxLength(64) sha256: string;
}

model PageRequest {
  @maxLength(256)
  cursor?: string;
  @minValue(1)
  @maxValue(1024)
  limit: uint32;
}

model DefinitionSummary {
  @minLength(68) @maxLength(68) handle: string;
  @maxLength(4096) name: string;
  assembly: boolean;
  @maxValue(100000) body_count: uint32;
  @maxValue(1000000) face_count: uint32;
  source_entity?: SourceEntityEvidence;
}

model RootOccurrenceSummary {
  kind: "root";
  @minLength(68) @maxLength(68) handle: string;
  @minLength(68) @maxLength(68) definition_handle: string;
  @maxLength(4096) name: string;
  @minItems(12) @maxItems(12) transform: float64[];
}

model ComponentOccurrenceSummary {
  kind: "component";
  @minLength(68) @maxLength(68) handle: string;
  @minLength(68) @maxLength(68) definition_handle: string;
  @minLength(68) @maxLength(68) parent_occurrence_handle: string;
  @minValue(1) @maxValue(64) depth: uint32;
  @maxLength(4096) name: string;
  @minItems(12) @maxItems(12) transform: float64[];
}

@oneOf
union OccurrenceSummary {
  root: RootOccurrenceSummary;
  component: ComponentOccurrenceSummary;
}

model SourceEntityEvidence {
  mapped: boolean;
  shape_result_round_trip: boolean;
  @minValue(1) @maxValue(5000000) model_number?: uint32;
  @minLength(1) @maxLength(128) entity_type?: string;
  @minLength(1) @maxLength(128) mapping_method?: string;
}

model BodySummary {
  @minLength(68) @maxLength(68) handle: string;
  @minLength(68) @maxLength(68) definition_handle: string;
  @minLength(1) @maxLength(64) topology_kind: string;
  @maxItems(250000) shell_handles: string[];
  @maxItems(1000000) face_handles: string[];
  @minItems(6) @maxItems(6) bounds_mm: float64[];
  @minValue(0) volume_mm3: float64;
  source_entity?: SourceEntityEvidence;
}

model ShellSummary {
  @minLength(68) @maxLength(68) handle: string;
  @minLength(68) @maxLength(68) definition_handle: string;
  @maxItems(100000) body_handles: string[];
  @maxItems(1000000) face_handles: string[];
  source_entity?: SourceEntityEvidence;
}

model FaceSummary {
  @minLength(68) @maxLength(68) handle: string;
  @minLength(68) @maxLength(68) definition_handle: string;
  @maxItems(100000) body_handles: string[];
  @maxItems(250000) shell_handles: string[];
  @minItems(6) @maxItems(6) bounds_mm: float64[];
  @minValue(0) area_mm2: float64;
  @minItems(3) @maxItems(3) centroid_mm: float64[];
  source_entity?: SourceEntityEvidence;
}

model TopologyPage {
  @maxItems(1024) definitions: DefinitionSummary[];
  @maxItems(1024) occurrences: OccurrenceSummary[];
  @maxItems(1024) bodies: BodySummary[];
  @maxItems(1024) shells: ShellSummary[];
  @maxItems(1024) faces: FaceSummary[];
  @maxLength(256) next_cursor?: string;
}

model InspectionCounts {
  @maxValue(10000) definitions: uint32;
  @maxValue(100000) root_occurrences: uint32;
  @maxValue(100000) component_occurrences: uint32;
  @maxValue(100000) bodies: uint32;
  @maxValue(250000) shells: uint32;
  @maxValue(1000000) faces: uint32;
}

@jsonSchema
@id("urn:wavenumber:schema:geometer:geometry.step_topology.open.request:a0")
@contractIdentity("geometry.step_topology.open.request.a0")
model StepTopologyOpenRequestA0 {
  schema: "geometry.step_topology.open.request.a0";
}

@jsonSchema
@id("urn:wavenumber:schema:geometer:geometry.step_topology.open.result:a0")
@contractIdentity("geometry.step_topology.open.result.a0")
model StepTopologyOpenResultA0 {
  schema: "geometry.step_topology.open.result.a0";
  session: SessionReference;
  source: SourceDescriptor;
  tool: ToolDescriptor;
  @maxItems(8) evicted_session_handles: string[];
}

@jsonSchema
@id("urn:wavenumber:schema:geometer:geometry.step_topology.close.request:a0")
@contractIdentity("geometry.step_topology.close.request.a0")
model StepTopologyCloseRequestA0 {
  schema: "geometry.step_topology.close.request.a0";
  session: SessionReference;
}

@jsonSchema
@id("urn:wavenumber:schema:geometer:geometry.step_topology.close.result:a0")
@contractIdentity("geometry.step_topology.close.result.a0")
model StepTopologyCloseResultA0 {
  schema: "geometry.step_topology.close.result.a0";
  @minLength(68) @maxLength(68) session_handle: string;
  closed: true;
}

@jsonSchema
@id("urn:wavenumber:schema:geometer:geometry.step_topology.inspect.request:a0")
@contractIdentity("geometry.step_topology.inspect.request.a0")
model StepTopologyInspectRequestA0 {
  schema: "geometry.step_topology.inspect.request.a0";
  session: SessionReference;
  page: PageRequest;
  include_source_entity_evidence: boolean;
  include_diagnostics: boolean;
}

@jsonSchema
@id("urn:wavenumber:schema:geometer:geometry.step_topology.inspect.result:a0")
@contractIdentity("geometry.step_topology.inspect.result.a0")
model StepTopologyInspectResultA0 {
  schema: "geometry.step_topology.inspect.result.a0";
  session: SessionReference;
  counts: InspectionCounts;
  page: TopologyPage;
  compact_table?: TopologyTableAttachmentDescriptor;
  @maxItems(256)
  diagnostics: Wavenumber.Geometer.Contracts.Common.DiagnosticA0[];
}

model TessellationOptions {
  @minValue(0.000001) @maxValue(1000) linear_deflection_mm: float64;
  @minValue(0.000001) @maxValue(3.141592653589793) angular_deflection_rad: float64;
  relative: boolean;
  parallel: boolean;
  @minItems(12) @maxItems(12) source_to_render: float64[];
}

@jsonSchema
@id("urn:wavenumber:schema:geometer:geometry.step_topology.render.request:a0")
@contractIdentity("geometry.step_topology.render.request.a0")
model StepTopologyRenderRequestA0 {
  schema: "geometry.step_topology.render.request.a0";
  session: SessionReference;
  tessellation: TessellationOptions;
}

model RenderCounts {
  @maxValue(10000) meshes: uint32;
  @maxValue(100000) instances: uint32;
  @maxValue(1000000) primitives: uint32;
  @maxValue(10000000) geometry_triangles: uint32;
  @maxValue(50000000) instanced_triangles: uint32;
}

model RenderArtifactDescriptor {
  @minLength(68) @maxLength(68) artifact_handle: string;
  @minLength(64) @maxLength(64) content_sha256: string;
  @minLength(68) @maxLength(68) render_artifact_handle: string;
  @minLength(64) @maxLength(64) render_content_sha256: string;
  binding_layout: "node-primitive-a0";
  geometry_length_unit: "meter";
  source_length_unit: "millimeter";
  counts: RenderCounts;
}

@jsonSchema
@id("urn:wavenumber:schema:geometer:geometry.step_topology.render.result:a0")
@contractIdentity("geometry.step_topology.render.result.a0")
model StepTopologyRenderResultA0 {
  schema: "geometry.step_topology.render.result.a0";
  session: SessionReference;
  artifact: RenderArtifactDescriptor;
  glb: GlbAttachmentDescriptor;
  compact_binding_table?: TopologyBindingTableAttachmentDescriptor;
}

@jsonSchema
@id("urn:wavenumber:schema:geometer:geometry.step_topology.resolve_hit.request:a0")
@contractIdentity("geometry.step_topology.resolve_hit.request.a0")
model StepTopologyResolveHitRequestA0 {
  schema: "geometry.step_topology.resolve_hit.request.a0";
  session: SessionReference;
  @minLength(68) @maxLength(68) artifact_handle: string;
  @minLength(64) @maxLength(64) content_sha256: string;
  @maxValue(99999) instance_index: uint32;
  @maxValue(999999) primitive_index: uint32;
  @maxValue(9999999) primitive_triangle_index: uint32;
  @minLength(68) @maxLength(68) occurrence_handle: string;
  @minLength(68) @maxLength(68) body_handle: string;
  @minLength(68) @maxLength(68) face_handle: string;
}

@jsonSchema
@id("urn:wavenumber:schema:geometer:geometry.step_topology.resolve_hit.result:a0")
@contractIdentity("geometry.step_topology.resolve_hit.result.a0")
model StepTopologyResolveHitResultA0 {
  schema: "geometry.step_topology.resolve_hit.result.a0";
  session: SessionReference;
  @maxValue(99999) instance_index: uint32;
  @maxValue(999999) primitive_index: uint32;
  @maxValue(9999999) triangle_index: uint32;
  @minLength(68) @maxLength(68) occurrence_handle: string;
  @minLength(68) @maxLength(68) body_handle: string;
  @minLength(68) @maxLength(68) face_handle: string;
}
