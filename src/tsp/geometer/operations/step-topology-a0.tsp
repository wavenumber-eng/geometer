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
  @maxValue(250000) shell_count: uint32;
  @maxValue(1000000) face_count: uint32;
  @minItems(6) @maxItems(6) bounds_mm: float64[];
  @minValue(0) volume_mm3: float64;
  source_entity?: SourceEntityEvidence;
}

model ShellSummary {
  @minLength(68) @maxLength(68) handle: string;
  @minLength(68) @maxLength(68) definition_handle: string;
  @maxValue(100000) body_count: uint32;
  @maxValue(1000000) face_count: uint32;
  source_entity?: SourceEntityEvidence;
}

model FaceSummary {
  @minLength(68) @maxLength(68) handle: string;
  @minLength(68) @maxLength(68) definition_handle: string;
  @maxValue(100000) body_count: uint32;
  @maxValue(250000) shell_count: uint32;
  @minItems(6) @maxItems(6) bounds_mm: float64[];
  @minValue(0) area_mm2: float64;
  @minItems(3) @maxItems(3) centroid_mm: float64[];
  source_entity?: SourceEntityEvidence;
}

enum TopologyMembershipKind {
  body_shell,
  body_face,
  shell_face,
}

model TopologyMembership {
  kind: TopologyMembershipKind;
  @minLength(68) @maxLength(68) owner_handle: string;
  @minLength(68) @maxLength(68) member_handle: string;
}

model TopologyPage {
  @maxItems(1024) definitions: DefinitionSummary[];
  @maxItems(1024) occurrences: OccurrenceSummary[];
  @maxItems(1024) bodies: BodySummary[];
  @maxItems(1024) shells: ShellSummary[];
  @maxItems(1024) faces: FaceSummary[];
  @maxItems(1024) memberships: TopologyMembership[];
  @maxLength(256) next_cursor?: string;
}

model InspectionCounts {
  @maxValue(10000) definitions: uint32;
  @maxValue(100000) root_occurrences: uint32;
  @maxValue(100000) component_occurrences: uint32;
  @maxValue(100000) bodies: uint32;
  @maxValue(250000) shells: uint32;
  @maxValue(1000000) faces: uint32;
  @maxValue(5000000) memberships: uint32;
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

enum LogicalGroupMemberKind {
  body,
  face,
}

model LogicalGroupMember {
  kind: LogicalGroupMemberKind;
  @minLength(68) @maxLength(68) target_handle: string;
}

model LogicalGroup {
  @minLength(28) @maxLength(128) authored_id: string;
  @minValue(1) revision: uint32;
  @minLength(1) @maxLength(4096) name: string;
  @minItems(1) @maxItems(100000) members: LogicalGroupMember[];
}

model CreateLogicalGroupCommand {
  kind: "create";
  @minLength(28) @maxLength(128) authored_id: string;
  @minLength(1) @maxLength(4096) name: string;
  @minItems(1) @maxItems(100000) member_handles: string[];
}

model RenameLogicalGroupCommand {
  kind: "rename";
  @minLength(28) @maxLength(128) authored_id: string;
  @minValue(1) expected_revision: uint32;
  @minLength(1) @maxLength(4096) name: string;
}

model ReplaceLogicalGroupMembersCommand {
  kind: "replace_members";
  @minLength(28) @maxLength(128) authored_id: string;
  @minValue(1) expected_revision: uint32;
  @minItems(1) @maxItems(100000) member_handles: string[];
}

model EraseLogicalGroupCommand {
  kind: "erase";
  @minLength(28) @maxLength(128) authored_id: string;
  @minValue(1) expected_revision: uint32;
}

@oneOf
union LogicalGroupCommand {
  create: CreateLogicalGroupCommand,
  rename: RenameLogicalGroupCommand,
  replace_members: ReplaceLogicalGroupMembersCommand,
  erase: EraseLogicalGroupCommand,
}

model DocumentProbeTarget {
  kind: "document";
}

model DefinitionProbeTarget {
  kind: "definition";
  @minLength(68) @maxLength(68) target_handle: string;
}

model RootOccurrenceProbeTarget {
  kind: "root_occurrence";
  @minLength(68) @maxLength(68) target_handle: string;
}

model ComponentOccurrenceProbeTarget {
  kind: "occurrence";
  @minLength(68) @maxLength(68) target_handle: string;
}

model BodyProbeTarget {
  kind: "body";
  @minLength(68) @maxLength(68) target_handle: string;
}

model FaceProbeTarget {
  kind: "face";
  @minLength(68) @maxLength(68) target_handle: string;
}

model LogicalGroupProbeTarget {
  kind: "logical_group";
  @minLength(28) @maxLength(128) group_authored_id: string;
}

@oneOf
union MetadataProbeTarget {
  document: DocumentProbeTarget,
  definition: DefinitionProbeTarget,
  root_occurrence: RootOccurrenceProbeTarget,
  occurrence: ComponentOccurrenceProbeTarget,
  body: BodyProbeTarget,
  face: FaceProbeTarget,
  logical_group: LogicalGroupProbeTarget,
}

model MetadataProbe {
  @minLength(28) @maxLength(128) authored_id: string;
  @minValue(1) revision: uint32;
  target: MetadataProbeTarget;
  @minLength(32) @maxLength(128) key: string;
  @minLength(1) @maxLength(4096) value: string;
}

model AttachMetadataProbeCommand {
  kind: "attach";
  @minLength(28) @maxLength(128) authored_id: string;
  target: MetadataProbeTarget;
  @minLength(32) @maxLength(128) key: string;
  @minLength(1) @maxLength(4096) value: string;
}

model ReplaceMetadataProbeCommand {
  kind: "replace";
  @minLength(28) @maxLength(128) authored_id: string;
  @minValue(1) expected_revision: uint32;
  target: MetadataProbeTarget;
  @minLength(32) @maxLength(128) key: string;
  @minLength(1) @maxLength(4096) value: string;
}

model EraseMetadataProbeCommand {
  kind: "erase";
  @minLength(28) @maxLength(128) authored_id: string;
  @minValue(1) expected_revision: uint32;
}

@oneOf
union MetadataProbeCommand {
  attach: AttachMetadataProbeCommand,
  replace: ReplaceMetadataProbeCommand,
  erase: EraseMetadataProbeCommand,
}

model MutationSessionState {
  session: SessionReference;
  @maxValue(100000) edit_journal_revision: uint32;
  @maxValue(16777216) accounted_string_bytes: uint32;
  @maxValue(536870912) estimated_resident_bytes: uint32;
}

model EditJournalAttachmentDescriptor {
  name: "edit_journal";
  media_type: "application/vnd.wavenumber.geometer.step-topology-edit-journal";
  format: "geometer.step_topology_edit_journal.a0";
  @minValue(1) @maxValue(67108864) bytes: uint32;
  @minLength(64) @maxLength(64) sha256: string;
}

@jsonSchema
@id("urn:wavenumber:schema:geometer:geometry.step_topology.apply_logical_groups.request:a0")
@contractIdentity("geometry.step_topology.apply_logical_groups.request.a0")
model StepTopologyApplyLogicalGroupsRequestA0 {
  schema: "geometry.step_topology.apply_logical_groups.request.a0";
  session: SessionReference;
  @minItems(1) @maxItems(10000) commands: LogicalGroupCommand[];
}

@jsonSchema
@id("urn:wavenumber:schema:geometer:geometry.step_topology.apply_logical_groups.result:a0")
@contractIdentity("geometry.step_topology.apply_logical_groups.result.a0")
model StepTopologyApplyLogicalGroupsResultA0 {
  schema: "geometry.step_topology.apply_logical_groups.result.a0";
  state: MutationSessionState;
  @maxItems(10000) groups: LogicalGroup[];
  @maxItems(256) diagnostics: Wavenumber.Geometer.Contracts.Common.DiagnosticA0[];
}

@jsonSchema
@id("urn:wavenumber:schema:geometer:geometry.step_topology.apply_metadata_probes.request:a0")
@contractIdentity("geometry.step_topology.apply_metadata_probes.request.a0")
model StepTopologyApplyMetadataProbesRequestA0 {
  schema: "geometry.step_topology.apply_metadata_probes.request.a0";
  session: SessionReference;
  @minItems(1) @maxItems(10000) commands: MetadataProbeCommand[];
}

@jsonSchema
@id("urn:wavenumber:schema:geometer:geometry.step_topology.apply_metadata_probes.result:a0")
@contractIdentity("geometry.step_topology.apply_metadata_probes.result.a0")
model StepTopologyApplyMetadataProbesResultA0 {
  schema: "geometry.step_topology.apply_metadata_probes.result.a0";
  state: MutationSessionState;
  @maxItems(10000) groups: LogicalGroup[];
  @maxItems(10000) probes: MetadataProbe[];
  @maxItems(256) diagnostics: Wavenumber.Geometer.Contracts.Common.DiagnosticA0[];
}

@jsonSchema
@id("urn:wavenumber:schema:geometer:geometry.step_topology.checkpoint_edit_journal.request:a0")
@contractIdentity("geometry.step_topology.checkpoint_edit_journal.request.a0")
model StepTopologyCheckpointEditJournalRequestA0 {
  schema: "geometry.step_topology.checkpoint_edit_journal.request.a0";
  session: SessionReference;
}

@jsonSchema
@id("urn:wavenumber:schema:geometer:geometry.step_topology.checkpoint_edit_journal.result:a0")
@contractIdentity("geometry.step_topology.checkpoint_edit_journal.result.a0")
model StepTopologyCheckpointEditJournalResultA0 {
  schema: "geometry.step_topology.checkpoint_edit_journal.result.a0";
  state: MutationSessionState;
  @minLength(64) @maxLength(64) source_sha256: string;
  @minLength(64) @maxLength(64) source_brep_sha256: string;
  @minLength(64) @maxLength(64) target_inventory_sha256: string;
  @minLength(1) @maxLength(64) occt_version: string;
  @maxValue(100000) transaction_count: uint32;
  journal: EditJournalAttachmentDescriptor;
  @maxItems(256) diagnostics: Wavenumber.Geometer.Contracts.Common.DiagnosticA0[];
}

enum HierarchySourceKind {
  definition,
  body,
}

enum HierarchyNodeKind {
  product,
  assembly,
}

model HierarchyNode {
  @minLength(28) @maxLength(128) authored_id: string;
  @minValue(1) revision: uint32;
  kind: HierarchyNodeKind;
  @minLength(1) @maxLength(4096) name: string;
  source_kind?: HierarchySourceKind;
  @minLength(68) @maxLength(68) source_handle?: string;
}

model HierarchyOccurrence {
  @minLength(28) @maxLength(128) authored_id: string;
  @minValue(1) revision: uint32;
  @minLength(28) @maxLength(128) child_authored_id: string;
  @minLength(28) @maxLength(128) parent_assembly_authored_id: string;
  @minItems(12) @maxItems(12) transform: float64[];
}

model CreateHierarchyProductCommand {
  kind: "create_product";
  @minLength(28) @maxLength(128) authored_id: string;
  @minLength(1) @maxLength(4096) name: string;
  source_kind: HierarchySourceKind;
  @minLength(68) @maxLength(68) source_handle: string;
}

model CreateHierarchyAssemblyCommand {
  kind: "create_assembly";
  @minLength(28) @maxLength(128) authored_id: string;
  @minLength(1) @maxLength(4096) name: string;
}

model CreateHierarchyOccurrenceCommand {
  kind: "create_occurrence";
  @minLength(28) @maxLength(128) authored_id: string;
  @minLength(28) @maxLength(128) child_authored_id: string;
  @minLength(28) @maxLength(128) parent_assembly_authored_id: string;
  @minItems(12) @maxItems(12) transform: float64[];
}

model ReparentHierarchyOccurrenceCommand {
  kind: "reparent_occurrence";
  @minLength(28) @maxLength(128) authored_id: string;
  @minValue(1) expected_revision: uint32;
  @minLength(28) @maxLength(128) parent_assembly_authored_id: string;
  @minItems(12) @maxItems(12) transform: float64[];
}

model RenameHierarchyNodeCommand {
  kind: "rename_node";
  @minLength(28) @maxLength(128) authored_id: string;
  @minValue(1) expected_revision: uint32;
  @minLength(1) @maxLength(4096) name: string;
}

model EraseHierarchyOccurrenceCommand {
  kind: "erase_occurrence";
  @minLength(28) @maxLength(128) authored_id: string;
  @minValue(1) expected_revision: uint32;
}

model EraseHierarchyNodeCommand {
  kind: "erase_node";
  @minLength(28) @maxLength(128) authored_id: string;
  @minValue(1) expected_revision: uint32;
}

@oneOf
union HierarchyCommand {
  create_product: CreateHierarchyProductCommand,
  create_assembly: CreateHierarchyAssemblyCommand,
  create_occurrence: CreateHierarchyOccurrenceCommand,
  reparent_occurrence: ReparentHierarchyOccurrenceCommand,
  rename_node: RenameHierarchyNodeCommand,
  erase_occurrence: EraseHierarchyOccurrenceCommand,
  erase_node: EraseHierarchyNodeCommand,
}

model HierarchyState {
  hierarchy_revision: uint32;
  @minLength(64) @maxLength(64) source_brep_sha256: string;
  @maxItems(10000) nodes: HierarchyNode[];
  @maxItems(100000) occurrences: HierarchyOccurrence[];
}

@jsonSchema
@id("urn:wavenumber:schema:geometer:geometry.step_topology.apply_hierarchy.request:a0")
@contractIdentity("geometry.step_topology.apply_hierarchy.request.a0")
model StepTopologyApplyHierarchyRequestA0 {
  schema: "geometry.step_topology.apply_hierarchy.request.a0";
  session: SessionReference;
  expected_hierarchy_revision: uint32;
  @minItems(1) @maxItems(10000) commands: HierarchyCommand[];
}

@jsonSchema
@id("urn:wavenumber:schema:geometer:geometry.step_topology.apply_hierarchy.result:a0")
@contractIdentity("geometry.step_topology.apply_hierarchy.result.a0")
model StepTopologyApplyHierarchyResultA0 {
  schema: "geometry.step_topology.apply_hierarchy.result.a0";
  state: MutationSessionState;
  hierarchy: HierarchyState;
  @maxItems(256) diagnostics: Wavenumber.Geometer.Contracts.Common.DiagnosticA0[];
}

enum PersistenceCarrier {
  xbf,
  xml_xcaf,
  step_ap242,
  json_sidecar,
  edit_journal,
}

enum SaveCarrier {
  xbf,
  xml_xcaf,
  step_ap242,
  json_sidecar,
}

enum CarrierSupportState {
  supported,
  experimental,
  unsupported,
}

model CarrierCapability {
  carrier: PersistenceCarrier;
  save: CarrierSupportState;
  restore: CarrierSupportState;
  authored_payload: CarrierSupportState;
  topology_links: CarrierSupportState;
  @maxItems(16) notes: CarrierCapabilityNote[];
}

model CarrierCapabilityNote {
  @minLength(1) @maxLength(256) value: string;
}

model XbfPersistenceArtifact {
  carrier: "xbf";
  name: "state_artifact";
  media_type: "application/vnd.opencascade.xbf";
  format: "ocaf-xbf-version-12";
  @minValue(1) @maxValue(536870912) bytes: uint32;
  @minLength(64) @maxLength(64) sha256: string;
}

model XmlXcafPersistenceArtifact {
  carrier: "xml_xcaf";
  name: "state_artifact";
  media_type: "application/vnd.opencascade.xml-xcaf";
  format: "ocaf-xml-xcaf-version-12";
  @minValue(1) @maxValue(536870912) bytes: uint32;
  @minLength(64) @maxLength(64) sha256: string;
}

model StepAp242PersistenceArtifact {
  carrier: "step_ap242";
  name: "state_artifact";
  media_type: "application/step";
  format: "ap242-managed-model-based-3d-engineering";
  @minValue(1) @maxValue(536870912) bytes: uint32;
  @minLength(64) @maxLength(64) sha256: string;
}

model JsonSidecarPersistenceArtifact {
  carrier: "json_sidecar";
  name: "state_artifact";
  media_type: "application/vnd.wavenumber.geometer.step-topology-sidecar+json";
  format: "geometer.step_topology_sidecar.a0";
  @minValue(1) @maxValue(67108864) bytes: uint32;
  @minLength(64) @maxLength(64) sha256: string;
}

model EditJournalPersistenceArtifact {
  carrier: "edit_journal";
  name: "state_artifact";
  media_type: "application/vnd.wavenumber.geometer.step-topology-edit-journal";
  format: "geometer.step_topology_edit_journal.a0";
  @minValue(1) @maxValue(67108864) bytes: uint32;
  @minLength(64) @maxLength(64) sha256: string;
}

@oneOf
union SavePersistenceArtifact {
  xbf: XbfPersistenceArtifact,
  xml_xcaf: XmlXcafPersistenceArtifact,
  step_ap242: StepAp242PersistenceArtifact,
  json_sidecar: JsonSidecarPersistenceArtifact,
}

@oneOf
union RestoreStateArtifact {
  xbf: XbfPersistenceArtifact,
  xml_xcaf: XmlXcafPersistenceArtifact,
  step_ap242: StepAp242PersistenceArtifact,
  json_sidecar: JsonSidecarPersistenceArtifact,
  edit_journal: EditJournalPersistenceArtifact,
}

enum RecoveryResolutionState {
  resolved,
  ambiguous,
  unresolved,
  unsupported,
}

enum RecoveryResolutionMethod {
  authored_id_topology_link,
  validated_carrier_locator,
  unique_geometry_adjacency_fingerprint,
  none,
}

enum RecoveryTopologyComparison {
  unchanged,
  relocated,
  split,
  merged,
  otherwise_changed,
  not_compared,
  unavailable,
}

enum RecoveryConfidence {
  high,
  medium,
  low,
  none,
}

enum RecoveryLineage {
  none,
  split_from_source,
  merged_from_sources,
}

enum RecoveryGroupCompleteness {
  fully_recovered,
  partially_recovered,
  unrecovered,
  unsupported,
}

model RecoveryTolerances {
  @minValue(0.000000001) length_mm: float64;
  @minValue(0.000000001) area_mm2: float64;
  @minValue(0.000000001) volume_mm3: float64;
}

model RecoveryFingerprint {
  normalized_length_unit: "millimeter";
  @minLength(1) @maxLength(128) coordinate_frame: string;
  @minLength(1) @maxLength(128) occurrence_context: string;
  @minLength(1) @maxLength(128) geometry_kind: string;
  @minValue(0) area_mm2: float64;
  @minValue(0) volume_mm3: float64;
  @minItems(3) @maxItems(3) centroid_mm: float64[];
  @minItems(6) @maxItems(6) bounds_mm: float64[];
  @minLength(64) @maxLength(64) adjacency_sha256: string;
}

model RecoveryCandidate {
  @minLength(68) @maxLength(68) target_handle: string;
  kind: LogicalGroupMemberKind;
  @minLength(28) @maxLength(128) authored_target_id?: string;
  topology_link_verified: boolean;
  @maxLength(4096) carrier_locator: string;
  carrier_locator_validated: boolean;
  @maxLength(4096) carrier_record: string;
  lineage: RecoveryLineage;
  fingerprint?: RecoveryFingerprint;
}

model RecoveryMemberRequest {
  @minLength(28) @maxLength(128) member_record_id: string;
  kind: LogicalGroupMemberKind;
  @maxLength(128) authored_target_id: string;
  @maxLength(4096) carrier_locator: string;
  source_fingerprint?: RecoveryFingerprint;
  @maxItems(16) candidates: RecoveryCandidate[];
}

model RecoveryProvenance {
  @minLength(64) @maxLength(64) source_artifact_sha256: string;
  @minLength(64) @maxLength(64) candidate_artifact_sha256: string;
  @minLength(1) @maxLength(64) source_occt_version: string;
  @minLength(1) @maxLength(64) candidate_occt_version: string;
  @minLength(1) @maxLength(128) source_driver: string;
  @minLength(1) @maxLength(128) candidate_driver: string;
  @minLength(1) @maxLength(4096) source_writer_settings: string;
  @minLength(1) @maxLength(4096) candidate_writer_settings: string;
  @minLength(1) @maxLength(8192) command_provenance: string;
  @minValue(0) measured_wall_time_milliseconds: float64;
}

model RecoveryGroupRequest {
  @minLength(28) @maxLength(128) group_authored_id: string;
  provenance: RecoveryProvenance;
  tolerances: RecoveryTolerances;
  @minItems(1) @maxItems(256) members: RecoveryMemberRequest[];
}

model RecoveryRejectedAlternative {
  @minLength(68) @maxLength(68) target_handle: string;
  @minLength(1) @maxLength(4096) reason: string;
}

model RecoveryEvidence {
  @maxValue(16) candidate_count: uint32;
  @maxValue(16) matching_candidate_count: uint32;
  @maxItems(16) compared_fields: RecoveryComparedField[];
  tolerances: RecoveryTolerances;
  @maxItems(16) carrier_records: RecoveryCarrierRecord[];
  @maxItems(16) rejected_alternatives: RecoveryRejectedAlternative[];
}

model RecoveryComparedField {
  @minLength(1) @maxLength(128) value: string;
}

model RecoveryCarrierRecord {
  @minLength(1) @maxLength(4096) value: string;
}

model RecoveryMemberResult {
  @minLength(28) @maxLength(128) member_record_id: string;
  kind: LogicalGroupMemberKind;
  @maxLength(128) authored_target_id: string;
  resolution_state: RecoveryResolutionState;
  resolution_method: RecoveryResolutionMethod;
  topology_comparison: RecoveryTopologyComparison;
  confidence: RecoveryConfidence;
  @minLength(68) @maxLength(68) resolved_target_handle?: string;
  evidence: RecoveryEvidence;
}

model RecoveryGroupResult {
  @minLength(28) @maxLength(128) group_authored_id: string;
  provenance: RecoveryProvenance;
  resolution_state: RecoveryResolutionState;
  completeness: RecoveryGroupCompleteness;
  @maxValue(256) resolved_member_count: uint32;
  @maxValue(256) ambiguous_member_count: uint32;
  @maxValue(256) unresolved_member_count: uint32;
  @maxValue(256) unsupported_member_count: uint32;
  @minItems(1) @maxItems(256) members: RecoveryMemberResult[];
}

@jsonSchema
@id("urn:wavenumber:schema:geometer:geometry.step_topology.save.request:a0")
@contractIdentity("geometry.step_topology.save.request.a0")
model StepTopologySaveRequestA0 {
  schema: "geometry.step_topology.save.request.a0";
  session: SessionReference;
  carrier: SaveCarrier;
  include_diagnostics: boolean;
}

@jsonSchema
@id("urn:wavenumber:schema:geometer:geometry.step_topology.save.result:a0")
@contractIdentity("geometry.step_topology.save.result.a0")
model StepTopologySaveResultA0 {
  schema: "geometry.step_topology.save.result.a0";
  state: MutationSessionState;
  @minLength(64) @maxLength(64) source_sha256: string;
  artifact: SavePersistenceArtifact;
  @minItems(5) @maxItems(5) capabilities: CarrierCapability[];
  @maxItems(256) diagnostics: Wavenumber.Geometer.Contracts.Common.DiagnosticA0[];
}

@jsonSchema
@id("urn:wavenumber:schema:geometer:geometry.step_topology.restore.request:a0")
@contractIdentity("geometry.step_topology.restore.request.a0")
model StepTopologyRestoreRequestA0 {
  schema: "geometry.step_topology.restore.request.a0";
  source: SourceDescriptor;
  state_artifact: RestoreStateArtifact;
  replay_preconditions?: EditJournalReplayPreconditions;
  include_diagnostics: boolean;
}

model EditJournalReplayPreconditions {
  @minLength(64) @maxLength(64) source_sha256: string;
  @minLength(64) @maxLength(64) source_brep_sha256: string;
  @minLength(64) @maxLength(64) target_inventory_sha256: string;
  @minLength(1) @maxLength(64) occt_version: string;
  @maxValue(100000) transaction_count: uint32;
}

@jsonSchema
@id("urn:wavenumber:schema:geometer:geometry.step_topology.restore.result:a0")
@contractIdentity("geometry.step_topology.restore.result.a0")
model StepTopologyRestoreResultA0 {
  schema: "geometry.step_topology.restore.result.a0";
  session: SessionReference;
  source: SourceDescriptor;
  tool: ToolDescriptor;
  @maxValue(100000) replayed_transaction_count: uint32;
  @maxItems(64) evicted_session_handles?: string[];
  @maxItems(16) recovery: RecoveryGroupResult[];
  @maxItems(256) diagnostics: Wavenumber.Geometer.Contracts.Common.DiagnosticA0[];
}

@jsonSchema
@id("urn:wavenumber:schema:geometer:geometry.step_topology.analyze_recovery.request:a0")
@contractIdentity("geometry.step_topology.analyze_recovery.request.a0")
model StepTopologyAnalyzeRecoveryRequestA0 {
  schema: "geometry.step_topology.analyze_recovery.request.a0";
  @minItems(1) @maxItems(16) groups: RecoveryGroupRequest[];
}

@jsonSchema
@id("urn:wavenumber:schema:geometer:geometry.step_topology.analyze_recovery.result:a0")
@contractIdentity("geometry.step_topology.analyze_recovery.result.a0")
model StepTopologyAnalyzeRecoveryResultA0 {
  schema: "geometry.step_topology.analyze_recovery.result.a0";
  @maxItems(16) groups: RecoveryGroupResult[];
  @maxItems(256) diagnostics: Wavenumber.Geometer.Contracts.Common.DiagnosticA0[];
}
