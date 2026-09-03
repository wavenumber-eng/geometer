// Generated from wn_geometer_contract_catalog.a0.json. Do not edit.
#include "geometer/generated/contracts/contracts.h"

#include <cmath>
#include <cstring>
#include <limits>
#include <rapidjson/document.h>
#include <rapidjson/error/en.h>
#include <rapidjson/stringbuffer.h>
#include <rapidjson/writer.h>
#include <type_traits>

namespace geometer::contracts
{
namespace
{

bool decode_DiagnosticCategory(const rapidjson::Value&, DiagnosticCategory*, const std::string&,
                               ContractError*);
bool write_DiagnosticCategory(rapidjson::Writer<rapidjson::StringBuffer>&,
                              const DiagnosticCategory&, ContractError*);
bool decode_DiagnosticA0(const rapidjson::Value&, DiagnosticA0*, const std::string&,
                         ContractError*);
bool write_DiagnosticA0(rapidjson::Writer<rapidjson::StringBuffer>&, const DiagnosticA0&,
                        ContractError*);
bool decode_PackedAttachmentReferenceA0(const rapidjson::Value&, PackedAttachmentReferenceA0*,
                                        const std::string&, ContractError*);
bool write_PackedAttachmentReferenceA0(rapidjson::Writer<rapidjson::StringBuffer>&,
                                       const PackedAttachmentReferenceA0&, ContractError*);
bool decode_PackedAttachmentProjectionA0(const rapidjson::Value&, PackedAttachmentProjectionA0*,
                                         const std::string&, ContractError*);
bool write_PackedAttachmentProjectionA0(rapidjson::Writer<rapidjson::StringBuffer>&,
                                        const PackedAttachmentProjectionA0&, ContractError*);
bool decode_FastHlrLimitsA0(const rapidjson::Value&, FastHlrLimitsA0*, const std::string&,
                            ContractError*);
bool write_FastHlrLimitsA0(rapidjson::Writer<rapidjson::StringBuffer>&, const FastHlrLimitsA0&,
                           ContractError*);
bool decode_FastHlrOptionsA0(const rapidjson::Value&, FastHlrOptionsA0*, const std::string&,
                             ContractError*);
bool write_FastHlrOptionsA0(rapidjson::Writer<rapidjson::StringBuffer>&, const FastHlrOptionsA0&,
                            ContractError*);
bool decode_HlrCurveMode(const rapidjson::Value&, HlrCurveMode*, const std::string&,
                         ContractError*);
bool write_HlrCurveMode(rapidjson::Writer<rapidjson::StringBuffer>&, const HlrCurveMode&,
                        ContractError*);
bool decode_HlrMatrix4x4(const rapidjson::Value&, HlrMatrix4x4*, const std::string&,
                         ContractError*);
bool write_HlrMatrix4x4(rapidjson::Writer<rapidjson::StringBuffer>&, const HlrMatrix4x4&,
                        ContractError*);
bool decode_HlrMeshDeflectionMode(const rapidjson::Value&, HlrMeshDeflectionMode*,
                                  const std::string&, ContractError*);
bool write_HlrMeshDeflectionMode(rapidjson::Writer<rapidjson::StringBuffer>&,
                                 const HlrMeshDeflectionMode&, ContractError*);
bool decode_HlrOutlineAlgorithm(const rapidjson::Value&, HlrOutlineAlgorithm*, const std::string&,
                                ContractError*);
bool write_HlrOutlineAlgorithm(rapidjson::Writer<rapidjson::StringBuffer>&,
                               const HlrOutlineAlgorithm&, ContractError*);
bool decode_HlrVector3(const rapidjson::Value&, HlrVector3*, const std::string&, ContractError*);
bool write_HlrVector3(rapidjson::Writer<rapidjson::StringBuffer>&, const HlrVector3&,
                      ContractError*);
bool decode_ProjectedSegment(const rapidjson::Value&, ProjectedSegment*, const std::string&,
                             ContractError*);
bool write_ProjectedSegment(rapidjson::Writer<rapidjson::StringBuffer>&, const ProjectedSegment&,
                            ContractError*);
bool decode_HlrVector2(const rapidjson::Value&, HlrVector2*, const std::string&, ContractError*);
bool write_HlrVector2(rapidjson::Writer<rapidjson::StringBuffer>&, const HlrVector2&,
                      ContractError*);
bool decode_ProjectedArc(const rapidjson::Value&, ProjectedArc*, const std::string&,
                         ContractError*);
bool write_ProjectedArc(rapidjson::Writer<rapidjson::StringBuffer>&, const ProjectedArc&,
                        ContractError*);
bool decode_ProjectionBounds(const rapidjson::Value&, ProjectionBounds*, const std::string&,
                             ContractError*);
bool write_ProjectionBounds(rapidjson::Writer<rapidjson::StringBuffer>&, const ProjectionBounds&,
                            ContractError*);
bool decode_ProjectedGeometry(const rapidjson::Value&, ProjectedGeometry*, const std::string&,
                              ContractError*);
bool write_ProjectedGeometry(rapidjson::Writer<rapidjson::StringBuffer>&, const ProjectedGeometry&,
                             ContractError*);
bool decode_HlrProjectionModes(const rapidjson::Value&, HlrProjectionModes*, const std::string&,
                               ContractError*);
bool write_HlrProjectionModes(rapidjson::Writer<rapidjson::StringBuffer>&,
                              const HlrProjectionModes&, ContractError*);
bool decode_HlrProjectedView(const rapidjson::Value&, HlrProjectedView*, const std::string&,
                             ContractError*);
bool write_HlrProjectedView(rapidjson::Writer<rapidjson::StringBuffer>&, const HlrProjectedView&,
                            ContractError*);
bool decode_HlrProjectionAlgorithm(const rapidjson::Value&, HlrProjectionAlgorithm*,
                                   const std::string&, ContractError*);
bool write_HlrProjectionAlgorithm(rapidjson::Writer<rapidjson::StringBuffer>&,
                                  const HlrProjectionAlgorithm&, ContractError*);
bool decode_HlrViewSpec(const rapidjson::Value&, HlrViewSpec*, const std::string&, ContractError*);
bool write_HlrViewSpec(rapidjson::Writer<rapidjson::StringBuffer>&, const HlrViewSpec&,
                       ContractError*);
bool decode_HlrProjectionOptionsA0(const rapidjson::Value&, HlrProjectionOptionsA0*,
                                   const std::string&, ContractError*);
bool write_HlrProjectionOptionsA0(rapidjson::Writer<rapidjson::StringBuffer>&,
                                  const HlrProjectionOptionsA0&, ContractError*);
bool decode_HlrSourceKind(const rapidjson::Value&, HlrSourceKind*, const std::string&,
                          ContractError*);
bool write_HlrSourceKind(rapidjson::Writer<rapidjson::StringBuffer>&, const HlrSourceKind&,
                         ContractError*);
bool decode_HlrProjectionSource(const rapidjson::Value&, HlrProjectionSource*, const std::string&,
                                ContractError*);
bool write_HlrProjectionSource(rapidjson::Writer<rapidjson::StringBuffer>&,
                               const HlrProjectionSource&, ContractError*);
bool decode_HlrProjectionTimings(const rapidjson::Value&, HlrProjectionTimings*, const std::string&,
                                 ContractError*);
bool write_HlrProjectionTimings(rapidjson::Writer<rapidjson::StringBuffer>&,
                                const HlrProjectionTimings&, ContractError*);
bool decode_HlrProjectionResultA0(const rapidjson::Value&, HlrProjectionResultA0*,
                                  const std::string&, ContractError*);
bool write_HlrProjectionResultA0(rapidjson::Writer<rapidjson::StringBuffer>&,
                                 const HlrProjectionResultA0&, ContractError*);
bool decode_IpcAttachmentDeclarationA0(const rapidjson::Value&, IpcAttachmentDeclarationA0*,
                                       const std::string&, ContractError*);
bool write_IpcAttachmentDeclarationA0(rapidjson::Writer<rapidjson::StringBuffer>&,
                                      const IpcAttachmentDeclarationA0&, ContractError*);
bool decode_IpcAttachmentOffsetsWasm32A0(const rapidjson::Value&, IpcAttachmentOffsetsWasm32A0*,
                                         const std::string&, ContractError*);
bool write_IpcAttachmentOffsetsWasm32A0(rapidjson::Writer<rapidjson::StringBuffer>&,
                                        const IpcAttachmentOffsetsWasm32A0&, ContractError*);
bool decode_IpcAttachmentLayoutWasm32A0(const rapidjson::Value&, IpcAttachmentLayoutWasm32A0*,
                                        const std::string&, ContractError*);
bool write_IpcAttachmentLayoutWasm32A0(rapidjson::Writer<rapidjson::StringBuffer>&,
                                       const IpcAttachmentLayoutWasm32A0&, ContractError*);
bool decode_IpcAttachmentOffsetsPointer64A0(const rapidjson::Value&,
                                            IpcAttachmentOffsetsPointer64A0*, const std::string&,
                                            ContractError*);
bool write_IpcAttachmentOffsetsPointer64A0(rapidjson::Writer<rapidjson::StringBuffer>&,
                                           const IpcAttachmentOffsetsPointer64A0&, ContractError*);
bool decode_IpcAttachmentLayoutPointer64A0(const rapidjson::Value&, IpcAttachmentLayoutPointer64A0*,
                                           const std::string&, ContractError*);
bool write_IpcAttachmentLayoutPointer64A0(rapidjson::Writer<rapidjson::StringBuffer>&,
                                          const IpcAttachmentLayoutPointer64A0&, ContractError*);
bool decode_IpcAttachmentDescriptorA0(const rapidjson::Value&, IpcAttachmentDescriptorA0*,
                                      const std::string&, ContractError*);
bool write_IpcAttachmentDescriptorA0(rapidjson::Writer<rapidjson::StringBuffer>&,
                                     const IpcAttachmentDescriptorA0&, ContractError*);
bool decode_IpcCancelledA0(const rapidjson::Value&, IpcCancelledA0*, const std::string&,
                           ContractError*);
bool write_IpcCancelledA0(rapidjson::Writer<rapidjson::StringBuffer>&, const IpcCancelledA0&,
                          ContractError*);
bool decode_IpcCancelRejectedA0(const rapidjson::Value&, IpcCancelRejectedA0*, const std::string&,
                                ContractError*);
bool write_IpcCancelRejectedA0(rapidjson::Writer<rapidjson::StringBuffer>&,
                               const IpcCancelRejectedA0&, ContractError*);
bool decode_IpcEffectiveLimitsA0(const rapidjson::Value&, IpcEffectiveLimitsA0*, const std::string&,
                                 ContractError*);
bool write_IpcEffectiveLimitsA0(rapidjson::Writer<rapidjson::StringBuffer>&,
                                const IpcEffectiveLimitsA0&, ContractError*);
bool decode_IpcGenericAbiLimitsA0(const rapidjson::Value&, IpcGenericAbiLimitsA0*,
                                  const std::string&, ContractError*);
bool write_IpcGenericAbiLimitsA0(rapidjson::Writer<rapidjson::StringBuffer>&,
                                 const IpcGenericAbiLimitsA0&, ContractError*);
bool decode_IpcHelloA0(const rapidjson::Value&, IpcHelloA0*, const std::string&, ContractError*);
bool write_IpcHelloA0(rapidjson::Writer<rapidjson::StringBuffer>&, const IpcHelloA0&,
                      ContractError*);
bool decode_IpcRuntimeDispatchA0(const rapidjson::Value&, IpcRuntimeDispatchA0*, const std::string&,
                                 ContractError*);
bool write_IpcRuntimeDispatchA0(rapidjson::Writer<rapidjson::StringBuffer>&,
                                const IpcRuntimeDispatchA0&, ContractError*);
bool decode_IpcPackedProjectionA0(const rapidjson::Value&, IpcPackedProjectionA0*,
                                  const std::string&, ContractError*);
bool write_IpcPackedProjectionA0(rapidjson::Writer<rapidjson::StringBuffer>&,
                                 const IpcPackedProjectionA0&, ContractError*);
bool decode_IpcOperationDeclarationA0(const rapidjson::Value&, IpcOperationDeclarationA0*,
                                      const std::string&, ContractError*);
bool write_IpcOperationDeclarationA0(rapidjson::Writer<rapidjson::StringBuffer>&,
                                     const IpcOperationDeclarationA0&, ContractError*);
bool decode_IpcOperationCatalogA0(const rapidjson::Value&, IpcOperationCatalogA0*,
                                  const std::string&, ContractError*);
bool write_IpcOperationCatalogA0(rapidjson::Writer<rapidjson::StringBuffer>&,
                                 const IpcOperationCatalogA0&, ContractError*);
bool decode_IpcProtocolErrorA0(const rapidjson::Value&, IpcProtocolErrorA0*, const std::string&,
                               ContractError*);
bool write_IpcProtocolErrorA0(rapidjson::Writer<rapidjson::StringBuffer>&,
                              const IpcProtocolErrorA0&, ContractError*);
bool decode_IpcReasonA0(const rapidjson::Value&, IpcReasonA0*, const std::string&, ContractError*);
bool write_IpcReasonA0(rapidjson::Writer<rapidjson::StringBuffer>&, const IpcReasonA0&,
                       ContractError*);
bool decode_ModelFormat(const rapidjson::Value&, ModelFormat*, const std::string&, ContractError*);
bool write_ModelFormat(rapidjson::Writer<rapidjson::StringBuffer>&, const ModelFormat&,
                       ContractError*);
bool decode_Matrix4x4(const rapidjson::Value&, Matrix4x4*, const std::string&, ContractError*);
bool write_Matrix4x4(rapidjson::Writer<rapidjson::StringBuffer>&, const Matrix4x4&, ContractError*);
bool decode_ModelBoundsOptionsA0(const rapidjson::Value&, ModelBoundsOptionsA0*, const std::string&,
                                 ContractError*);
bool write_ModelBoundsOptionsA0(rapidjson::Writer<rapidjson::StringBuffer>&,
                                const ModelBoundsOptionsA0&, ContractError*);
bool decode_StepTopologyOpenRequestA0(const rapidjson::Value&, StepTopologyOpenRequestA0*,
                                      const std::string&, ContractError*);
bool write_StepTopologyOpenRequestA0(rapidjson::Writer<rapidjson::StringBuffer>&,
                                     const StepTopologyOpenRequestA0&, ContractError*);
bool decode_SessionReference(const rapidjson::Value&, SessionReference*, const std::string&,
                             ContractError*);
bool write_SessionReference(rapidjson::Writer<rapidjson::StringBuffer>&, const SessionReference&,
                            ContractError*);
bool decode_StepTopologyCloseRequestA0(const rapidjson::Value&, StepTopologyCloseRequestA0*,
                                       const std::string&, ContractError*);
bool write_StepTopologyCloseRequestA0(rapidjson::Writer<rapidjson::StringBuffer>&,
                                      const StepTopologyCloseRequestA0&, ContractError*);
bool decode_PageRequest(const rapidjson::Value&, PageRequest*, const std::string&, ContractError*);
bool write_PageRequest(rapidjson::Writer<rapidjson::StringBuffer>&, const PageRequest&,
                       ContractError*);
bool decode_StepTopologyInspectRequestA0(const rapidjson::Value&, StepTopologyInspectRequestA0*,
                                         const std::string&, ContractError*);
bool write_StepTopologyInspectRequestA0(rapidjson::Writer<rapidjson::StringBuffer>&,
                                        const StepTopologyInspectRequestA0&, ContractError*);
bool decode_TessellationOptions(const rapidjson::Value&, TessellationOptions*, const std::string&,
                                ContractError*);
bool write_TessellationOptions(rapidjson::Writer<rapidjson::StringBuffer>&,
                               const TessellationOptions&, ContractError*);
bool decode_StepTopologyRenderRequestA0(const rapidjson::Value&, StepTopologyRenderRequestA0*,
                                        const std::string&, ContractError*);
bool write_StepTopologyRenderRequestA0(rapidjson::Writer<rapidjson::StringBuffer>&,
                                       const StepTopologyRenderRequestA0&, ContractError*);
bool decode_StepTopologyResolveHitRequestA0(const rapidjson::Value&,
                                            StepTopologyResolveHitRequestA0*, const std::string&,
                                            ContractError*);
bool write_StepTopologyResolveHitRequestA0(rapidjson::Writer<rapidjson::StringBuffer>&,
                                           const StepTopologyResolveHitRequestA0&, ContractError*);
bool decode_CreateLogicalGroupCommand(const rapidjson::Value&, CreateLogicalGroupCommand*,
                                      const std::string&, ContractError*);
bool write_CreateLogicalGroupCommand(rapidjson::Writer<rapidjson::StringBuffer>&,
                                     const CreateLogicalGroupCommand&, ContractError*);
bool decode_RenameLogicalGroupCommand(const rapidjson::Value&, RenameLogicalGroupCommand*,
                                      const std::string&, ContractError*);
bool write_RenameLogicalGroupCommand(rapidjson::Writer<rapidjson::StringBuffer>&,
                                     const RenameLogicalGroupCommand&, ContractError*);
bool decode_ReplaceLogicalGroupMembersCommand(const rapidjson::Value&,
                                              ReplaceLogicalGroupMembersCommand*,
                                              const std::string&, ContractError*);
bool write_ReplaceLogicalGroupMembersCommand(rapidjson::Writer<rapidjson::StringBuffer>&,
                                             const ReplaceLogicalGroupMembersCommand&,
                                             ContractError*);
bool decode_EraseLogicalGroupCommand(const rapidjson::Value&, EraseLogicalGroupCommand*,
                                     const std::string&, ContractError*);
bool write_EraseLogicalGroupCommand(rapidjson::Writer<rapidjson::StringBuffer>&,
                                    const EraseLogicalGroupCommand&, ContractError*);
bool decode_LogicalGroupCommand(const rapidjson::Value&, LogicalGroupCommand*, const std::string&,
                                ContractError*);
bool write_LogicalGroupCommand(rapidjson::Writer<rapidjson::StringBuffer>&,
                               const LogicalGroupCommand&, ContractError*);
bool decode_StepTopologyApplyLogicalGroupsRequestA0(const rapidjson::Value&,
                                                    StepTopologyApplyLogicalGroupsRequestA0*,
                                                    const std::string&, ContractError*);
bool write_StepTopologyApplyLogicalGroupsRequestA0(rapidjson::Writer<rapidjson::StringBuffer>&,
                                                   const StepTopologyApplyLogicalGroupsRequestA0&,
                                                   ContractError*);
bool decode_DocumentProbeTarget(const rapidjson::Value&, DocumentProbeTarget*, const std::string&,
                                ContractError*);
bool write_DocumentProbeTarget(rapidjson::Writer<rapidjson::StringBuffer>&,
                               const DocumentProbeTarget&, ContractError*);
bool decode_DefinitionProbeTarget(const rapidjson::Value&, DefinitionProbeTarget*,
                                  const std::string&, ContractError*);
bool write_DefinitionProbeTarget(rapidjson::Writer<rapidjson::StringBuffer>&,
                                 const DefinitionProbeTarget&, ContractError*);
bool decode_RootOccurrenceProbeTarget(const rapidjson::Value&, RootOccurrenceProbeTarget*,
                                      const std::string&, ContractError*);
bool write_RootOccurrenceProbeTarget(rapidjson::Writer<rapidjson::StringBuffer>&,
                                     const RootOccurrenceProbeTarget&, ContractError*);
bool decode_ComponentOccurrenceProbeTarget(const rapidjson::Value&, ComponentOccurrenceProbeTarget*,
                                           const std::string&, ContractError*);
bool write_ComponentOccurrenceProbeTarget(rapidjson::Writer<rapidjson::StringBuffer>&,
                                          const ComponentOccurrenceProbeTarget&, ContractError*);
bool decode_BodyProbeTarget(const rapidjson::Value&, BodyProbeTarget*, const std::string&,
                            ContractError*);
bool write_BodyProbeTarget(rapidjson::Writer<rapidjson::StringBuffer>&, const BodyProbeTarget&,
                           ContractError*);
bool decode_FaceProbeTarget(const rapidjson::Value&, FaceProbeTarget*, const std::string&,
                            ContractError*);
bool write_FaceProbeTarget(rapidjson::Writer<rapidjson::StringBuffer>&, const FaceProbeTarget&,
                           ContractError*);
bool decode_LogicalGroupProbeTarget(const rapidjson::Value&, LogicalGroupProbeTarget*,
                                    const std::string&, ContractError*);
bool write_LogicalGroupProbeTarget(rapidjson::Writer<rapidjson::StringBuffer>&,
                                   const LogicalGroupProbeTarget&, ContractError*);
bool decode_MetadataProbeTarget(const rapidjson::Value&, MetadataProbeTarget*, const std::string&,
                                ContractError*);
bool write_MetadataProbeTarget(rapidjson::Writer<rapidjson::StringBuffer>&,
                               const MetadataProbeTarget&, ContractError*);
bool decode_AttachMetadataProbeCommand(const rapidjson::Value&, AttachMetadataProbeCommand*,
                                       const std::string&, ContractError*);
bool write_AttachMetadataProbeCommand(rapidjson::Writer<rapidjson::StringBuffer>&,
                                      const AttachMetadataProbeCommand&, ContractError*);
bool decode_ReplaceMetadataProbeCommand(const rapidjson::Value&, ReplaceMetadataProbeCommand*,
                                        const std::string&, ContractError*);
bool write_ReplaceMetadataProbeCommand(rapidjson::Writer<rapidjson::StringBuffer>&,
                                       const ReplaceMetadataProbeCommand&, ContractError*);
bool decode_EraseMetadataProbeCommand(const rapidjson::Value&, EraseMetadataProbeCommand*,
                                      const std::string&, ContractError*);
bool write_EraseMetadataProbeCommand(rapidjson::Writer<rapidjson::StringBuffer>&,
                                     const EraseMetadataProbeCommand&, ContractError*);
bool decode_MetadataProbeCommand(const rapidjson::Value&, MetadataProbeCommand*, const std::string&,
                                 ContractError*);
bool write_MetadataProbeCommand(rapidjson::Writer<rapidjson::StringBuffer>&,
                                const MetadataProbeCommand&, ContractError*);
bool decode_StepTopologyApplyMetadataProbesRequestA0(const rapidjson::Value&,
                                                     StepTopologyApplyMetadataProbesRequestA0*,
                                                     const std::string&, ContractError*);
bool write_StepTopologyApplyMetadataProbesRequestA0(rapidjson::Writer<rapidjson::StringBuffer>&,
                                                    const StepTopologyApplyMetadataProbesRequestA0&,
                                                    ContractError*);
bool decode_StepTopologyCheckpointEditJournalRequestA0(const rapidjson::Value&,
                                                       StepTopologyCheckpointEditJournalRequestA0*,
                                                       const std::string&, ContractError*);
bool write_StepTopologyCheckpointEditJournalRequestA0(
    rapidjson::Writer<rapidjson::StringBuffer>&, const StepTopologyCheckpointEditJournalRequestA0&,
    ContractError*);
bool decode_HierarchySourceKind(const rapidjson::Value&, HierarchySourceKind*, const std::string&,
                                ContractError*);
bool write_HierarchySourceKind(rapidjson::Writer<rapidjson::StringBuffer>&,
                               const HierarchySourceKind&, ContractError*);
bool decode_CreateHierarchyProductCommand(const rapidjson::Value&, CreateHierarchyProductCommand*,
                                          const std::string&, ContractError*);
bool write_CreateHierarchyProductCommand(rapidjson::Writer<rapidjson::StringBuffer>&,
                                         const CreateHierarchyProductCommand&, ContractError*);
bool decode_CreateHierarchyAssemblyCommand(const rapidjson::Value&, CreateHierarchyAssemblyCommand*,
                                           const std::string&, ContractError*);
bool write_CreateHierarchyAssemblyCommand(rapidjson::Writer<rapidjson::StringBuffer>&,
                                          const CreateHierarchyAssemblyCommand&, ContractError*);
bool decode_CreateHierarchyOccurrenceCommand(const rapidjson::Value&,
                                             CreateHierarchyOccurrenceCommand*, const std::string&,
                                             ContractError*);
bool write_CreateHierarchyOccurrenceCommand(rapidjson::Writer<rapidjson::StringBuffer>&,
                                            const CreateHierarchyOccurrenceCommand&,
                                            ContractError*);
bool decode_ReparentHierarchyOccurrenceCommand(const rapidjson::Value&,
                                               ReparentHierarchyOccurrenceCommand*,
                                               const std::string&, ContractError*);
bool write_ReparentHierarchyOccurrenceCommand(rapidjson::Writer<rapidjson::StringBuffer>&,
                                              const ReparentHierarchyOccurrenceCommand&,
                                              ContractError*);
bool decode_RenameHierarchyNodeCommand(const rapidjson::Value&, RenameHierarchyNodeCommand*,
                                       const std::string&, ContractError*);
bool write_RenameHierarchyNodeCommand(rapidjson::Writer<rapidjson::StringBuffer>&,
                                      const RenameHierarchyNodeCommand&, ContractError*);
bool decode_EraseHierarchyOccurrenceCommand(const rapidjson::Value&,
                                            EraseHierarchyOccurrenceCommand*, const std::string&,
                                            ContractError*);
bool write_EraseHierarchyOccurrenceCommand(rapidjson::Writer<rapidjson::StringBuffer>&,
                                           const EraseHierarchyOccurrenceCommand&, ContractError*);
bool decode_EraseHierarchyNodeCommand(const rapidjson::Value&, EraseHierarchyNodeCommand*,
                                      const std::string&, ContractError*);
bool write_EraseHierarchyNodeCommand(rapidjson::Writer<rapidjson::StringBuffer>&,
                                     const EraseHierarchyNodeCommand&, ContractError*);
bool decode_HierarchyCommand(const rapidjson::Value&, HierarchyCommand*, const std::string&,
                             ContractError*);
bool write_HierarchyCommand(rapidjson::Writer<rapidjson::StringBuffer>&, const HierarchyCommand&,
                            ContractError*);
bool decode_StepTopologyApplyHierarchyRequestA0(const rapidjson::Value&,
                                                StepTopologyApplyHierarchyRequestA0*,
                                                const std::string&, ContractError*);
bool write_StepTopologyApplyHierarchyRequestA0(rapidjson::Writer<rapidjson::StringBuffer>&,
                                               const StepTopologyApplyHierarchyRequestA0&,
                                               ContractError*);
bool decode_SaveCarrier(const rapidjson::Value&, SaveCarrier*, const std::string&, ContractError*);
bool write_SaveCarrier(rapidjson::Writer<rapidjson::StringBuffer>&, const SaveCarrier&,
                       ContractError*);
bool decode_StepTopologySaveRequestA0(const rapidjson::Value&, StepTopologySaveRequestA0*,
                                      const std::string&, ContractError*);
bool write_StepTopologySaveRequestA0(rapidjson::Writer<rapidjson::StringBuffer>&,
                                     const StepTopologySaveRequestA0&, ContractError*);
bool decode_SourceDescriptor(const rapidjson::Value&, SourceDescriptor*, const std::string&,
                             ContractError*);
bool write_SourceDescriptor(rapidjson::Writer<rapidjson::StringBuffer>&, const SourceDescriptor&,
                            ContractError*);
bool decode_XbfPersistenceArtifact(const rapidjson::Value&, XbfPersistenceArtifact*,
                                   const std::string&, ContractError*);
bool write_XbfPersistenceArtifact(rapidjson::Writer<rapidjson::StringBuffer>&,
                                  const XbfPersistenceArtifact&, ContractError*);
bool decode_XmlXcafPersistenceArtifact(const rapidjson::Value&, XmlXcafPersistenceArtifact*,
                                       const std::string&, ContractError*);
bool write_XmlXcafPersistenceArtifact(rapidjson::Writer<rapidjson::StringBuffer>&,
                                      const XmlXcafPersistenceArtifact&, ContractError*);
bool decode_StepAp242PersistenceArtifact(const rapidjson::Value&, StepAp242PersistenceArtifact*,
                                         const std::string&, ContractError*);
bool write_StepAp242PersistenceArtifact(rapidjson::Writer<rapidjson::StringBuffer>&,
                                        const StepAp242PersistenceArtifact&, ContractError*);
bool decode_JsonSidecarPersistenceArtifact(const rapidjson::Value&, JsonSidecarPersistenceArtifact*,
                                           const std::string&, ContractError*);
bool write_JsonSidecarPersistenceArtifact(rapidjson::Writer<rapidjson::StringBuffer>&,
                                          const JsonSidecarPersistenceArtifact&, ContractError*);
bool decode_EditJournalPersistenceArtifact(const rapidjson::Value&, EditJournalPersistenceArtifact*,
                                           const std::string&, ContractError*);
bool write_EditJournalPersistenceArtifact(rapidjson::Writer<rapidjson::StringBuffer>&,
                                          const EditJournalPersistenceArtifact&, ContractError*);
bool decode_RestoreStateArtifact(const rapidjson::Value&, RestoreStateArtifact*, const std::string&,
                                 ContractError*);
bool write_RestoreStateArtifact(rapidjson::Writer<rapidjson::StringBuffer>&,
                                const RestoreStateArtifact&, ContractError*);
bool decode_EditJournalReplayPreconditions(const rapidjson::Value&, EditJournalReplayPreconditions*,
                                           const std::string&, ContractError*);
bool write_EditJournalReplayPreconditions(rapidjson::Writer<rapidjson::StringBuffer>&,
                                          const EditJournalReplayPreconditions&, ContractError*);
bool decode_StepTopologyRestoreRequestA0(const rapidjson::Value&, StepTopologyRestoreRequestA0*,
                                         const std::string&, ContractError*);
bool write_StepTopologyRestoreRequestA0(rapidjson::Writer<rapidjson::StringBuffer>&,
                                        const StepTopologyRestoreRequestA0&, ContractError*);
bool decode_RecoveryProvenance(const rapidjson::Value&, RecoveryProvenance*, const std::string&,
                               ContractError*);
bool write_RecoveryProvenance(rapidjson::Writer<rapidjson::StringBuffer>&,
                              const RecoveryProvenance&, ContractError*);
bool decode_RecoveryTolerances(const rapidjson::Value&, RecoveryTolerances*, const std::string&,
                               ContractError*);
bool write_RecoveryTolerances(rapidjson::Writer<rapidjson::StringBuffer>&,
                              const RecoveryTolerances&, ContractError*);
bool decode_LogicalGroupMemberKind(const rapidjson::Value&, LogicalGroupMemberKind*,
                                   const std::string&, ContractError*);
bool write_LogicalGroupMemberKind(rapidjson::Writer<rapidjson::StringBuffer>&,
                                  const LogicalGroupMemberKind&, ContractError*);
bool decode_RecoveryFingerprint(const rapidjson::Value&, RecoveryFingerprint*, const std::string&,
                                ContractError*);
bool write_RecoveryFingerprint(rapidjson::Writer<rapidjson::StringBuffer>&,
                               const RecoveryFingerprint&, ContractError*);
bool decode_RecoveryLineage(const rapidjson::Value&, RecoveryLineage*, const std::string&,
                            ContractError*);
bool write_RecoveryLineage(rapidjson::Writer<rapidjson::StringBuffer>&, const RecoveryLineage&,
                           ContractError*);
bool decode_RecoveryCandidate(const rapidjson::Value&, RecoveryCandidate*, const std::string&,
                              ContractError*);
bool write_RecoveryCandidate(rapidjson::Writer<rapidjson::StringBuffer>&, const RecoveryCandidate&,
                             ContractError*);
bool decode_RecoveryMemberRequest(const rapidjson::Value&, RecoveryMemberRequest*,
                                  const std::string&, ContractError*);
bool write_RecoveryMemberRequest(rapidjson::Writer<rapidjson::StringBuffer>&,
                                 const RecoveryMemberRequest&, ContractError*);
bool decode_RecoveryGroupRequest(const rapidjson::Value&, RecoveryGroupRequest*, const std::string&,
                                 ContractError*);
bool write_RecoveryGroupRequest(rapidjson::Writer<rapidjson::StringBuffer>&,
                                const RecoveryGroupRequest&, ContractError*);
bool decode_StepTopologyAnalyzeRecoveryRequestA0(const rapidjson::Value&,
                                                 StepTopologyAnalyzeRecoveryRequestA0*,
                                                 const std::string&, ContractError*);
bool write_StepTopologyAnalyzeRecoveryRequestA0(rapidjson::Writer<rapidjson::StringBuffer>&,
                                                const StepTopologyAnalyzeRecoveryRequestA0&,
                                                ContractError*);
bool decode_IpcRequestValueA0(const rapidjson::Value&, IpcRequestValueA0*, const std::string&,
                              ContractError*);
bool write_IpcRequestValueA0(rapidjson::Writer<rapidjson::StringBuffer>&, const IpcRequestValueA0&,
                             ContractError*);
bool decode_IpcRequestA0(const rapidjson::Value&, IpcRequestA0*, const std::string&,
                         ContractError*);
bool write_IpcRequestA0(rapidjson::Writer<rapidjson::StringBuffer>&, const IpcRequestA0&,
                        ContractError*);
bool decode_IpcShutdownAckA0(const rapidjson::Value&, IpcShutdownAckA0*, const std::string&,
                             ContractError*);
bool write_IpcShutdownAckA0(rapidjson::Writer<rapidjson::StringBuffer>&, const IpcShutdownAckA0&,
                            ContractError*);
bool decode_IpcWelcomeA0(const rapidjson::Value&, IpcWelcomeA0*, const std::string&,
                         ContractError*);
bool write_IpcWelcomeA0(rapidjson::Writer<rapidjson::StringBuffer>&, const IpcWelcomeA0&,
                        ContractError*);
bool decode_IllustrationMatrix4x4(const rapidjson::Value&, IllustrationMatrix4x4*,
                                  const std::string&, ContractError*);
bool write_IllustrationMatrix4x4(rapidjson::Writer<rapidjson::StringBuffer>&,
                                 const IllustrationMatrix4x4&, ContractError*);
bool decode_IllustrationVector3(const rapidjson::Value&, IllustrationVector3*, const std::string&,
                                ContractError*);
bool write_IllustrationVector3(rapidjson::Writer<rapidjson::StringBuffer>&,
                               const IllustrationVector3&, ContractError*);
bool decode_MeshIllustrationMaterial(const rapidjson::Value&, MeshIllustrationMaterial*,
                                     const std::string&, ContractError*);
bool write_MeshIllustrationMaterial(rapidjson::Writer<rapidjson::StringBuffer>&,
                                    const MeshIllustrationMaterial&, ContractError*);
bool decode_MeshIllustrationMesh(const rapidjson::Value&, MeshIllustrationMesh*, const std::string&,
                                 ContractError*);
bool write_MeshIllustrationMesh(rapidjson::Writer<rapidjson::StringBuffer>&,
                                const MeshIllustrationMesh&, ContractError*);
bool decode_MeshIllustrationView(const rapidjson::Value&, MeshIllustrationView*, const std::string&,
                                 ContractError*);
bool write_MeshIllustrationView(rapidjson::Writer<rapidjson::StringBuffer>&,
                                const MeshIllustrationView&, ContractError*);
bool decode_MeshIllustrationPrepareOptions(const rapidjson::Value&, MeshIllustrationPrepareOptions*,
                                           const std::string&, ContractError*);
bool write_MeshIllustrationPrepareOptions(rapidjson::Writer<rapidjson::StringBuffer>&,
                                          const MeshIllustrationPrepareOptions&, ContractError*);
bool decode_MeshIllustrationShading(const rapidjson::Value&, MeshIllustrationShading*,
                                    const std::string&, ContractError*);
bool write_MeshIllustrationShading(rapidjson::Writer<rapidjson::StringBuffer>&,
                                   const MeshIllustrationShading&, ContractError*);
bool decode_MeshIllustrationStyleA0(const rapidjson::Value&, MeshIllustrationStyleA0*,
                                    const std::string&, ContractError*);
bool write_MeshIllustrationStyleA0(rapidjson::Writer<rapidjson::StringBuffer>&,
                                   const MeshIllustrationStyleA0&, ContractError*);
bool decode_MeshIllustrationSvgOptions(const rapidjson::Value&, MeshIllustrationSvgOptions*,
                                       const std::string&, ContractError*);
bool write_MeshIllustrationSvgOptions(rapidjson::Writer<rapidjson::StringBuffer>&,
                                      const MeshIllustrationSvgOptions&, ContractError*);
bool decode_MeshIllustrationInputA0(const rapidjson::Value&, MeshIllustrationInputA0*,
                                    const std::string&, ContractError*);
bool write_MeshIllustrationInputA0(rapidjson::Writer<rapidjson::StringBuffer>&,
                                   const MeshIllustrationInputA0&, ContractError*);
bool decode_MeshIllustrationRenderStats(const rapidjson::Value&, MeshIllustrationRenderStats*,
                                        const std::string&, ContractError*);
bool write_MeshIllustrationRenderStats(rapidjson::Writer<rapidjson::StringBuffer>&,
                                       const MeshIllustrationRenderStats&, ContractError*);
bool decode_MeshIllustrationResultA0(const rapidjson::Value&, MeshIllustrationResultA0*,
                                     const std::string&, ContractError*);
bool write_MeshIllustrationResultA0(rapidjson::Writer<rapidjson::StringBuffer>&,
                                    const MeshIllustrationResultA0&, ContractError*);
bool decode_ModelBoundsSource(const rapidjson::Value&, ModelBoundsSource*, const std::string&,
                              ContractError*);
bool write_ModelBoundsSource(rapidjson::Writer<rapidjson::StringBuffer>&, const ModelBoundsSource&,
                             ContractError*);
bool decode_Vector3(const rapidjson::Value&, Vector3*, const std::string&, ContractError*);
bool write_Vector3(rapidjson::Writer<rapidjson::StringBuffer>&, const Vector3&, ContractError*);
bool decode_ModelBoundsValues(const rapidjson::Value&, ModelBoundsValues*, const std::string&,
                              ContractError*);
bool write_ModelBoundsValues(rapidjson::Writer<rapidjson::StringBuffer>&, const ModelBoundsValues&,
                             ContractError*);
bool decode_ModelBoundsTimings(const rapidjson::Value&, ModelBoundsTimings*, const std::string&,
                               ContractError*);
bool write_ModelBoundsTimings(rapidjson::Writer<rapidjson::StringBuffer>&,
                              const ModelBoundsTimings&, ContractError*);
bool decode_ModelBoundsResultA0(const rapidjson::Value&, ModelBoundsResultA0*, const std::string&,
                                ContractError*);
bool write_ModelBoundsResultA0(rapidjson::Writer<rapidjson::StringBuffer>&,
                               const ModelBoundsResultA0&, ContractError*);
bool decode_OperationFailureA0(const rapidjson::Value&, OperationFailureA0*, const std::string&,
                               ContractError*);
bool write_OperationFailureA0(rapidjson::Writer<rapidjson::StringBuffer>&,
                              const OperationFailureA0&, ContractError*);
bool decode_ToolDescriptor(const rapidjson::Value&, ToolDescriptor*, const std::string&,
                           ContractError*);
bool write_ToolDescriptor(rapidjson::Writer<rapidjson::StringBuffer>&, const ToolDescriptor&,
                          ContractError*);
bool decode_StepTopologyOpenResultA0(const rapidjson::Value&, StepTopologyOpenResultA0*,
                                     const std::string&, ContractError*);
bool write_StepTopologyOpenResultA0(rapidjson::Writer<rapidjson::StringBuffer>&,
                                    const StepTopologyOpenResultA0&, ContractError*);
bool decode_StepTopologyCloseResultA0(const rapidjson::Value&, StepTopologyCloseResultA0*,
                                      const std::string&, ContractError*);
bool write_StepTopologyCloseResultA0(rapidjson::Writer<rapidjson::StringBuffer>&,
                                     const StepTopologyCloseResultA0&, ContractError*);
bool decode_InspectionCounts(const rapidjson::Value&, InspectionCounts*, const std::string&,
                             ContractError*);
bool write_InspectionCounts(rapidjson::Writer<rapidjson::StringBuffer>&, const InspectionCounts&,
                            ContractError*);
bool decode_SourceEntityEvidence(const rapidjson::Value&, SourceEntityEvidence*, const std::string&,
                                 ContractError*);
bool write_SourceEntityEvidence(rapidjson::Writer<rapidjson::StringBuffer>&,
                                const SourceEntityEvidence&, ContractError*);
bool decode_DefinitionSummary(const rapidjson::Value&, DefinitionSummary*, const std::string&,
                              ContractError*);
bool write_DefinitionSummary(rapidjson::Writer<rapidjson::StringBuffer>&, const DefinitionSummary&,
                             ContractError*);
bool decode_RootOccurrenceSummary(const rapidjson::Value&, RootOccurrenceSummary*,
                                  const std::string&, ContractError*);
bool write_RootOccurrenceSummary(rapidjson::Writer<rapidjson::StringBuffer>&,
                                 const RootOccurrenceSummary&, ContractError*);
bool decode_ComponentOccurrenceSummary(const rapidjson::Value&, ComponentOccurrenceSummary*,
                                       const std::string&, ContractError*);
bool write_ComponentOccurrenceSummary(rapidjson::Writer<rapidjson::StringBuffer>&,
                                      const ComponentOccurrenceSummary&, ContractError*);
bool decode_OccurrenceSummary(const rapidjson::Value&, OccurrenceSummary*, const std::string&,
                              ContractError*);
bool write_OccurrenceSummary(rapidjson::Writer<rapidjson::StringBuffer>&, const OccurrenceSummary&,
                             ContractError*);
bool decode_BodySummary(const rapidjson::Value&, BodySummary*, const std::string&, ContractError*);
bool write_BodySummary(rapidjson::Writer<rapidjson::StringBuffer>&, const BodySummary&,
                       ContractError*);
bool decode_ShellSummary(const rapidjson::Value&, ShellSummary*, const std::string&,
                         ContractError*);
bool write_ShellSummary(rapidjson::Writer<rapidjson::StringBuffer>&, const ShellSummary&,
                        ContractError*);
bool decode_FaceSummary(const rapidjson::Value&, FaceSummary*, const std::string&, ContractError*);
bool write_FaceSummary(rapidjson::Writer<rapidjson::StringBuffer>&, const FaceSummary&,
                       ContractError*);
bool decode_TopologyMembershipKind(const rapidjson::Value&, TopologyMembershipKind*,
                                   const std::string&, ContractError*);
bool write_TopologyMembershipKind(rapidjson::Writer<rapidjson::StringBuffer>&,
                                  const TopologyMembershipKind&, ContractError*);
bool decode_TopologyMembership(const rapidjson::Value&, TopologyMembership*, const std::string&,
                               ContractError*);
bool write_TopologyMembership(rapidjson::Writer<rapidjson::StringBuffer>&,
                              const TopologyMembership&, ContractError*);
bool decode_TopologyPage(const rapidjson::Value&, TopologyPage*, const std::string&,
                         ContractError*);
bool write_TopologyPage(rapidjson::Writer<rapidjson::StringBuffer>&, const TopologyPage&,
                        ContractError*);
bool decode_TopologyTableAttachmentDescriptor(const rapidjson::Value&,
                                              TopologyTableAttachmentDescriptor*,
                                              const std::string&, ContractError*);
bool write_TopologyTableAttachmentDescriptor(rapidjson::Writer<rapidjson::StringBuffer>&,
                                             const TopologyTableAttachmentDescriptor&,
                                             ContractError*);
bool decode_StepTopologyInspectResultA0(const rapidjson::Value&, StepTopologyInspectResultA0*,
                                        const std::string&, ContractError*);
bool write_StepTopologyInspectResultA0(rapidjson::Writer<rapidjson::StringBuffer>&,
                                       const StepTopologyInspectResultA0&, ContractError*);
bool decode_RenderCounts(const rapidjson::Value&, RenderCounts*, const std::string&,
                         ContractError*);
bool write_RenderCounts(rapidjson::Writer<rapidjson::StringBuffer>&, const RenderCounts&,
                        ContractError*);
bool decode_RenderArtifactDescriptor(const rapidjson::Value&, RenderArtifactDescriptor*,
                                     const std::string&, ContractError*);
bool write_RenderArtifactDescriptor(rapidjson::Writer<rapidjson::StringBuffer>&,
                                    const RenderArtifactDescriptor&, ContractError*);
bool decode_GlbAttachmentDescriptor(const rapidjson::Value&, GlbAttachmentDescriptor*,
                                    const std::string&, ContractError*);
bool write_GlbAttachmentDescriptor(rapidjson::Writer<rapidjson::StringBuffer>&,
                                   const GlbAttachmentDescriptor&, ContractError*);
bool decode_TopologyBindingTableAttachmentDescriptor(const rapidjson::Value&,
                                                     TopologyBindingTableAttachmentDescriptor*,
                                                     const std::string&, ContractError*);
bool write_TopologyBindingTableAttachmentDescriptor(rapidjson::Writer<rapidjson::StringBuffer>&,
                                                    const TopologyBindingTableAttachmentDescriptor&,
                                                    ContractError*);
bool decode_StepTopologyRenderResultA0(const rapidjson::Value&, StepTopologyRenderResultA0*,
                                       const std::string&, ContractError*);
bool write_StepTopologyRenderResultA0(rapidjson::Writer<rapidjson::StringBuffer>&,
                                      const StepTopologyRenderResultA0&, ContractError*);
bool decode_StepTopologyResolveHitResultA0(const rapidjson::Value&, StepTopologyResolveHitResultA0*,
                                           const std::string&, ContractError*);
bool write_StepTopologyResolveHitResultA0(rapidjson::Writer<rapidjson::StringBuffer>&,
                                          const StepTopologyResolveHitResultA0&, ContractError*);
bool decode_MutationSessionState(const rapidjson::Value&, MutationSessionState*, const std::string&,
                                 ContractError*);
bool write_MutationSessionState(rapidjson::Writer<rapidjson::StringBuffer>&,
                                const MutationSessionState&, ContractError*);
bool decode_LogicalGroupMember(const rapidjson::Value&, LogicalGroupMember*, const std::string&,
                               ContractError*);
bool write_LogicalGroupMember(rapidjson::Writer<rapidjson::StringBuffer>&,
                              const LogicalGroupMember&, ContractError*);
bool decode_LogicalGroup(const rapidjson::Value&, LogicalGroup*, const std::string&,
                         ContractError*);
bool write_LogicalGroup(rapidjson::Writer<rapidjson::StringBuffer>&, const LogicalGroup&,
                        ContractError*);
bool decode_StepTopologyApplyLogicalGroupsResultA0(const rapidjson::Value&,
                                                   StepTopologyApplyLogicalGroupsResultA0*,
                                                   const std::string&, ContractError*);
bool write_StepTopologyApplyLogicalGroupsResultA0(rapidjson::Writer<rapidjson::StringBuffer>&,
                                                  const StepTopologyApplyLogicalGroupsResultA0&,
                                                  ContractError*);
bool decode_MetadataProbe(const rapidjson::Value&, MetadataProbe*, const std::string&,
                          ContractError*);
bool write_MetadataProbe(rapidjson::Writer<rapidjson::StringBuffer>&, const MetadataProbe&,
                         ContractError*);
bool decode_StepTopologyApplyMetadataProbesResultA0(const rapidjson::Value&,
                                                    StepTopologyApplyMetadataProbesResultA0*,
                                                    const std::string&, ContractError*);
bool write_StepTopologyApplyMetadataProbesResultA0(rapidjson::Writer<rapidjson::StringBuffer>&,
                                                   const StepTopologyApplyMetadataProbesResultA0&,
                                                   ContractError*);
bool decode_EditJournalAttachmentDescriptor(const rapidjson::Value&,
                                            EditJournalAttachmentDescriptor*, const std::string&,
                                            ContractError*);
bool write_EditJournalAttachmentDescriptor(rapidjson::Writer<rapidjson::StringBuffer>&,
                                           const EditJournalAttachmentDescriptor&, ContractError*);
bool decode_StepTopologyCheckpointEditJournalResultA0(const rapidjson::Value&,
                                                      StepTopologyCheckpointEditJournalResultA0*,
                                                      const std::string&, ContractError*);
bool write_StepTopologyCheckpointEditJournalResultA0(
    rapidjson::Writer<rapidjson::StringBuffer>&, const StepTopologyCheckpointEditJournalResultA0&,
    ContractError*);
bool decode_HierarchyNodeKind(const rapidjson::Value&, HierarchyNodeKind*, const std::string&,
                              ContractError*);
bool write_HierarchyNodeKind(rapidjson::Writer<rapidjson::StringBuffer>&, const HierarchyNodeKind&,
                             ContractError*);
bool decode_HierarchyNode(const rapidjson::Value&, HierarchyNode*, const std::string&,
                          ContractError*);
bool write_HierarchyNode(rapidjson::Writer<rapidjson::StringBuffer>&, const HierarchyNode&,
                         ContractError*);
bool decode_HierarchyOccurrence(const rapidjson::Value&, HierarchyOccurrence*, const std::string&,
                                ContractError*);
bool write_HierarchyOccurrence(rapidjson::Writer<rapidjson::StringBuffer>&,
                               const HierarchyOccurrence&, ContractError*);
bool decode_HierarchyState(const rapidjson::Value&, HierarchyState*, const std::string&,
                           ContractError*);
bool write_HierarchyState(rapidjson::Writer<rapidjson::StringBuffer>&, const HierarchyState&,
                          ContractError*);
bool decode_StepTopologyApplyHierarchyResultA0(const rapidjson::Value&,
                                               StepTopologyApplyHierarchyResultA0*,
                                               const std::string&, ContractError*);
bool write_StepTopologyApplyHierarchyResultA0(rapidjson::Writer<rapidjson::StringBuffer>&,
                                              const StepTopologyApplyHierarchyResultA0&,
                                              ContractError*);
bool decode_SavePersistenceArtifact(const rapidjson::Value&, SavePersistenceArtifact*,
                                    const std::string&, ContractError*);
bool write_SavePersistenceArtifact(rapidjson::Writer<rapidjson::StringBuffer>&,
                                   const SavePersistenceArtifact&, ContractError*);
bool decode_PersistenceCarrier(const rapidjson::Value&, PersistenceCarrier*, const std::string&,
                               ContractError*);
bool write_PersistenceCarrier(rapidjson::Writer<rapidjson::StringBuffer>&,
                              const PersistenceCarrier&, ContractError*);
bool decode_CarrierSupportState(const rapidjson::Value&, CarrierSupportState*, const std::string&,
                                ContractError*);
bool write_CarrierSupportState(rapidjson::Writer<rapidjson::StringBuffer>&,
                               const CarrierSupportState&, ContractError*);
bool decode_CarrierCapabilityNote(const rapidjson::Value&, CarrierCapabilityNote*,
                                  const std::string&, ContractError*);
bool write_CarrierCapabilityNote(rapidjson::Writer<rapidjson::StringBuffer>&,
                                 const CarrierCapabilityNote&, ContractError*);
bool decode_CarrierCapability(const rapidjson::Value&, CarrierCapability*, const std::string&,
                              ContractError*);
bool write_CarrierCapability(rapidjson::Writer<rapidjson::StringBuffer>&, const CarrierCapability&,
                             ContractError*);
bool decode_StepTopologySaveResultA0(const rapidjson::Value&, StepTopologySaveResultA0*,
                                     const std::string&, ContractError*);
bool write_StepTopologySaveResultA0(rapidjson::Writer<rapidjson::StringBuffer>&,
                                    const StepTopologySaveResultA0&, ContractError*);
bool decode_RecoveryResolutionState(const rapidjson::Value&, RecoveryResolutionState*,
                                    const std::string&, ContractError*);
bool write_RecoveryResolutionState(rapidjson::Writer<rapidjson::StringBuffer>&,
                                   const RecoveryResolutionState&, ContractError*);
bool decode_RecoveryGroupCompleteness(const rapidjson::Value&, RecoveryGroupCompleteness*,
                                      const std::string&, ContractError*);
bool write_RecoveryGroupCompleteness(rapidjson::Writer<rapidjson::StringBuffer>&,
                                     const RecoveryGroupCompleteness&, ContractError*);
bool decode_RecoveryResolutionMethod(const rapidjson::Value&, RecoveryResolutionMethod*,
                                     const std::string&, ContractError*);
bool write_RecoveryResolutionMethod(rapidjson::Writer<rapidjson::StringBuffer>&,
                                    const RecoveryResolutionMethod&, ContractError*);
bool decode_RecoveryTopologyComparison(const rapidjson::Value&, RecoveryTopologyComparison*,
                                       const std::string&, ContractError*);
bool write_RecoveryTopologyComparison(rapidjson::Writer<rapidjson::StringBuffer>&,
                                      const RecoveryTopologyComparison&, ContractError*);
bool decode_RecoveryConfidence(const rapidjson::Value&, RecoveryConfidence*, const std::string&,
                               ContractError*);
bool write_RecoveryConfidence(rapidjson::Writer<rapidjson::StringBuffer>&,
                              const RecoveryConfidence&, ContractError*);
bool decode_RecoveryComparedField(const rapidjson::Value&, RecoveryComparedField*,
                                  const std::string&, ContractError*);
bool write_RecoveryComparedField(rapidjson::Writer<rapidjson::StringBuffer>&,
                                 const RecoveryComparedField&, ContractError*);
bool decode_RecoveryCarrierRecord(const rapidjson::Value&, RecoveryCarrierRecord*,
                                  const std::string&, ContractError*);
bool write_RecoveryCarrierRecord(rapidjson::Writer<rapidjson::StringBuffer>&,
                                 const RecoveryCarrierRecord&, ContractError*);
bool decode_RecoveryRejectedAlternative(const rapidjson::Value&, RecoveryRejectedAlternative*,
                                        const std::string&, ContractError*);
bool write_RecoveryRejectedAlternative(rapidjson::Writer<rapidjson::StringBuffer>&,
                                       const RecoveryRejectedAlternative&, ContractError*);
bool decode_RecoveryEvidence(const rapidjson::Value&, RecoveryEvidence*, const std::string&,
                             ContractError*);
bool write_RecoveryEvidence(rapidjson::Writer<rapidjson::StringBuffer>&, const RecoveryEvidence&,
                            ContractError*);
bool decode_RecoveryMemberResult(const rapidjson::Value&, RecoveryMemberResult*, const std::string&,
                                 ContractError*);
bool write_RecoveryMemberResult(rapidjson::Writer<rapidjson::StringBuffer>&,
                                const RecoveryMemberResult&, ContractError*);
bool decode_RecoveryGroupResult(const rapidjson::Value&, RecoveryGroupResult*, const std::string&,
                                ContractError*);
bool write_RecoveryGroupResult(rapidjson::Writer<rapidjson::StringBuffer>&,
                               const RecoveryGroupResult&, ContractError*);
bool decode_StepTopologyRestoreResultA0(const rapidjson::Value&, StepTopologyRestoreResultA0*,
                                        const std::string&, ContractError*);
bool write_StepTopologyRestoreResultA0(rapidjson::Writer<rapidjson::StringBuffer>&,
                                       const StepTopologyRestoreResultA0&, ContractError*);
bool decode_StepTopologyAnalyzeRecoveryResultA0(const rapidjson::Value&,
                                                StepTopologyAnalyzeRecoveryResultA0*,
                                                const std::string&, ContractError*);
bool write_StepTopologyAnalyzeRecoveryResultA0(rapidjson::Writer<rapidjson::StringBuffer>&,
                                               const StepTopologyAnalyzeRecoveryResultA0&,
                                               ContractError*);
bool decode_OperationResultValueA0(const rapidjson::Value&, OperationResultValueA0*,
                                   const std::string&, ContractError*);
bool write_OperationResultValueA0(rapidjson::Writer<rapidjson::StringBuffer>&,
                                  const OperationResultValueA0&, ContractError*);
bool decode_OperationSuccessA0(const rapidjson::Value&, OperationSuccessA0*, const std::string&,
                               ContractError*);
bool write_OperationSuccessA0(rapidjson::Writer<rapidjson::StringBuffer>&,
                              const OperationSuccessA0&, ContractError*);
bool decode_OperationOutcomeA0(const rapidjson::Value&, OperationOutcomeA0*, const std::string&,
                               ContractError*);
bool write_OperationOutcomeA0(rapidjson::Writer<rapidjson::StringBuffer>&,
                              const OperationOutcomeA0&, ContractError*);

constexpr std::size_t kMaxJsonBytes = 8U * 1024U * 1024U;

bool fail(ContractError* error, const char* code, const std::string& path,
          const std::string& message)
{
    if (error != nullptr)
        *error = {code, path, message};
    return false;
}

bool valid_utf8(const char* data, std::size_t size)
{
    const auto* bytes = reinterpret_cast<const unsigned char*>(data);
    std::size_t index = 0;
    while (index < size)
    {
        const unsigned char first = bytes[index++];
        if (first <= 0x7fU)
            continue;
        unsigned int remaining = 0;
        unsigned int code_point = 0;
        if (first >= 0xc2U && first <= 0xdfU)
        {
            remaining = 1;
            code_point = first & 0x1fU;
        }
        else if (first >= 0xe0U && first <= 0xefU)
        {
            remaining = 2;
            code_point = first & 0x0fU;
        }
        else if (first >= 0xf0U && first <= 0xf4U)
        {
            remaining = 3;
            code_point = first & 0x07U;
        }
        else
            return false;
        if (index + remaining > size)
            return false;
        for (unsigned int offset = 0; offset < remaining; ++offset)
        {
            const unsigned char next = bytes[index++];
            if ((next & 0xc0U) != 0x80U)
                return false;
            code_point = (code_point << 6U) | (next & 0x3fU);
        }
        if ((remaining == 2 && code_point < 0x800U) || (remaining == 3 && code_point < 0x10000U) ||
            code_point > 0x10ffffU || (code_point >= 0xd800U && code_point <= 0xdfffU))
            return false;
    }
    return true;
}

std::string child_path(const std::string& parent, const char* name, std::size_t size)
{
    std::string escaped;
    for (std::size_t index = 0; index < size; ++index)
    {
        const char c = name[index];
        if (c == '~')
            escaped += "~0";
        else if (c == '/')
            escaped += "~1";
        else
            escaped += c;
    }
    return parent + "/" + escaped;
}

std::string child_path(const std::string& parent, const char* name)
{
    return child_path(parent, name, std::strlen(name));
}

bool validate_object(const rapidjson::Value& value, const char* const* names, std::size_t count,
                     const std::string& path, ContractError* error)
{
    if (!value.IsObject())
        return fail(error, "geometer.contract.type_mismatch", path, "Expected an object.");
    for (auto it = value.MemberBegin(); it != value.MemberEnd(); ++it)
    {
        if (!it->name.IsString())
            return fail(error, "geometer.contract.invalid_member", path,
                        "Object member name is invalid.");
        bool known = false;
        for (std::size_t i = 0; i < count; ++i)
            if (it->name.GetStringLength() == std::strlen(names[i]) &&
                std::memcmp(it->name.GetString(), names[i], it->name.GetStringLength()) == 0)
                known = true;
        if (!known)
            return fail(error, "geometer.contract.unknown_field",
                        child_path(path, it->name.GetString(), it->name.GetStringLength()),
                        "Unknown field.");
        for (auto jt = value.MemberBegin(); jt != it; ++jt)
            if (jt->name.GetStringLength() == it->name.GetStringLength() &&
                std::memcmp(jt->name.GetString(), it->name.GetString(),
                            it->name.GetStringLength()) == 0)
                return fail(error, "geometer.contract.duplicate_field",
                            child_path(path, it->name.GetString(), it->name.GetStringLength()),
                            "Duplicate field.");
    }
    return true;
}

bool decode_string(const rapidjson::Value& value, std::string* out, const std::string& path,
                   ContractError* error, std::size_t minimum, std::size_t maximum)
{
    if (!value.IsString())
        return fail(error, "geometer.contract.type_mismatch", path, "Expected a string.");
    const std::size_t size = value.GetStringLength();
    if (size < minimum || size > maximum)
        return fail(error, "geometer.contract.string_length", path,
                    "String length is outside its contract bounds.");
    out->assign(value.GetString(), size);
    return true;
}

bool decode_boolean(const rapidjson::Value& value, bool* out, const std::string& path,
                    ContractError* error)
{
    if (!value.IsBool())
        return fail(error, "geometer.contract.type_mismatch", path, "Expected a boolean.");
    *out = value.GetBool();
    return true;
}

bool decode_uint32(const rapidjson::Value& value, std::uint32_t* out, const std::string& path,
                   ContractError* error, std::uint64_t minimum, std::uint64_t maximum)
{
    if (!value.IsUint64() || value.GetUint64() < minimum || value.GetUint64() > maximum ||
        value.GetUint64() > std::numeric_limits<std::uint32_t>::max())
        return fail(error, "geometer.contract.number_range", path,
                    "Expected an unsigned 32-bit integer within its contract bounds.");
    *out = static_cast<std::uint32_t>(value.GetUint64());
    return true;
}

bool decode_uint64(const rapidjson::Value& value, std::uint64_t* out, const std::string& path,
                   ContractError* error, std::uint64_t minimum, std::uint64_t maximum)
{
    if (!value.IsUint64() || value.GetUint64() < minimum || value.GetUint64() > maximum)
        return fail(error, "geometer.contract.number_range", path,
                    "Expected an unsigned 64-bit integer within its contract bounds.");
    *out = value.GetUint64();
    return true;
}

bool decode_double(const rapidjson::Value& value, double* out, const std::string& path,
                   ContractError* error, double minimum, double maximum, bool minimum_exclusive,
                   bool maximum_exclusive)
{
    if (!value.IsNumber() || !std::isfinite(value.GetDouble()))
        return fail(error, "geometer.contract.type_mismatch", path, "Expected a finite number.");
    const double number = value.GetDouble();
    if (number < minimum || number > maximum || (minimum_exclusive && number == minimum) ||
        (maximum_exclusive && number == maximum))
        return fail(error, "geometer.contract.number_range", path,
                    "Number is outside its contract bounds.");
    *out = number;
    return true;
}

bool decode_literal_string(const rapidjson::Value& value, std::string* out, const std::string& path,
                           ContractError* error, const char* expected)
{
    if (!value.IsString() || value.GetStringLength() != std::strlen(expected) ||
        std::memcmp(value.GetString(), expected, value.GetStringLength()) != 0)
        return fail(error, "geometer.contract.literal_mismatch", path,
                    "String literal does not match.");
    *out = expected;
    return true;
}

bool decode_literal_boolean(const rapidjson::Value& value, bool* out, const std::string& path,
                            ContractError* error, bool expected)
{
    if (!value.IsBool() || value.GetBool() != expected)
        return fail(error, "geometer.contract.literal_mismatch", path,
                    "Boolean literal does not match.");
    *out = expected;
    return true;
}

template <typename T>
bool decode_array(const rapidjson::Value& value, std::vector<T>* out, const std::string& path,
                  ContractError* error, std::size_t minimum, std::size_t maximum,
                  bool (*decode_item)(const rapidjson::Value&, T*, const std::string&,
                                      ContractError*))
{
    if (!value.IsArray() || value.Size() < minimum || value.Size() > maximum)
        return fail(error, "geometer.contract.array_size", path,
                    "Array length is outside its contract bounds.");
    out->clear();
    out->reserve(value.Size());
    for (rapidjson::SizeType i = 0; i < value.Size(); ++i)
    {
        T item{};
        if (!decode_item(value[i], &item, path + "/" + std::to_string(i), error))
            return false;
        out->push_back(std::move(item));
    }
    return true;
}

bool write_double(rapidjson::Writer<rapidjson::StringBuffer>& writer, double value,
                  ContractError* error, double minimum, double maximum, bool minimum_exclusive,
                  bool maximum_exclusive)
{
    if (!std::isfinite(value) || value < minimum || value > maximum ||
        (minimum_exclusive && value == minimum) || (maximum_exclusive && value == maximum))
        return fail(error, "geometer.contract.number_range", "",
                    "Number is outside its contract bounds.");
    writer.Double(value);
    return true;
}

bool write_uint32(rapidjson::Writer<rapidjson::StringBuffer>& writer, std::uint32_t value,
                  ContractError* error, std::uint64_t minimum, std::uint64_t maximum)
{
    if (value < minimum || value > maximum)
        return fail(error, "geometer.contract.number_range", "",
                    "Unsigned integer is outside its contract bounds.");
    writer.Uint(value);
    return true;
}

bool write_uint64(rapidjson::Writer<rapidjson::StringBuffer>& writer, std::uint64_t value,
                  ContractError* error, std::uint64_t minimum, std::uint64_t maximum)
{
    if (value < minimum || value > maximum)
        return fail(error, "geometer.contract.number_range", "",
                    "Unsigned integer is outside its contract bounds.");
    writer.Uint64(value);
    return true;
}

bool write_string(rapidjson::Writer<rapidjson::StringBuffer>& writer, const std::string& value,
                  ContractError* error, std::size_t minimum, std::size_t maximum)
{
    if (value.size() < minimum || value.size() > maximum)
        return fail(error, "geometer.contract.string_length", "",
                    "String length is outside its contract bounds.");
    if (!valid_utf8(value.data(), value.size()))
        return fail(error, "geometer.contract.invalid_utf8", "", "String is not valid UTF-8.");
    writer.String(value.data(), static_cast<rapidjson::SizeType>(value.size()));
    return true;
}

bool write_literal_string(rapidjson::Writer<rapidjson::StringBuffer>& writer,
                          const std::string& value, ContractError* error, const char* expected)
{
    if (value != expected)
        return fail(error, "geometer.contract.literal_mismatch", "",
                    "String literal does not match.");
    writer.String(expected);
    return true;
}

bool write_literal_boolean(rapidjson::Writer<rapidjson::StringBuffer>& writer, bool value,
                           ContractError* error, bool expected)
{
    if (value != expected)
        return fail(error, "geometer.contract.literal_mismatch", "",
                    "Boolean literal does not match.");
    writer.Bool(expected);
    return true;
}

bool decode_string_item(const rapidjson::Value& value, std::string* out, const std::string& path,
                        ContractError* error)
{
    return decode_string(value, out, path, error, 0U, std::numeric_limits<std::size_t>::max());
}

bool write_string_item(rapidjson::Writer<rapidjson::StringBuffer>& writer, const std::string& value,
                       ContractError* error)
{
    return write_string(writer, value, error, 0U, std::numeric_limits<std::size_t>::max());
}

bool decode_uint32_item(const rapidjson::Value& value, std::uint32_t* out, const std::string& path,
                        ContractError* error)
{
    return decode_uint32(value, out, path, error, 0U, std::numeric_limits<std::uint32_t>::max());
}

bool write_uint32_item(rapidjson::Writer<rapidjson::StringBuffer>& writer,
                       const std::uint32_t& value, ContractError* error)
{
    return write_uint32(writer, value, error, 0U, std::numeric_limits<std::uint32_t>::max());
}

bool decode_double_item(const rapidjson::Value& value, double* out, const std::string& path,
                        ContractError* error)
{
    return decode_double(value, out, path, error, -std::numeric_limits<double>::infinity(),
                         std::numeric_limits<double>::infinity(), false, false);
}

bool write_double_item(rapidjson::Writer<rapidjson::StringBuffer>& writer, const double& value,
                       ContractError* error)
{
    return write_double(writer, value, error, -std::numeric_limits<double>::infinity(),
                        std::numeric_limits<double>::infinity(), false, false);
}

template <typename T>
bool write_array(rapidjson::Writer<rapidjson::StringBuffer>& writer, const std::vector<T>& value,
                 ContractError* error, std::size_t minimum, std::size_t maximum,
                 bool (*write_item)(rapidjson::Writer<rapidjson::StringBuffer>&, const T&,
                                    ContractError*))
{
    if (value.size() < minimum || value.size() > maximum)
        return fail(error, "geometer.contract.array_size", "",
                    "Array length is outside its contract bounds.");
    writer.StartArray();
    for (const auto& item : value)
        if (!write_item(writer, item, error))
            return false;
    writer.EndArray();
    return true;
}

bool decode_DiagnosticCategory(const rapidjson::Value& value, DiagnosticCategory* out,
                               const std::string& path, ContractError* error)
{
    if (!value.IsString())
        return fail(error, "geometer.contract.type_mismatch", path, "Expected a string enum.");
    const std::string text(value.GetString(), value.GetStringLength());
    if (text == "transport")
    {
        *out = DiagnosticCategory::transport;
        return true;
    }
    if (text == "contract")
    {
        *out = DiagnosticCategory::contract;
        return true;
    }
    if (text == "operation")
    {
        *out = DiagnosticCategory::operation;
        return true;
    }
    return fail(error, "geometer.contract.enum_mismatch", path, "Unknown enum value.");
}

bool write_DiagnosticCategory(rapidjson::Writer<rapidjson::StringBuffer>& writer,
                              const DiagnosticCategory& value, ContractError* error)
{
    switch (value)
    {
    case DiagnosticCategory::transport:
        writer.String("transport");
        return true;
    case DiagnosticCategory::contract:
        writer.String("contract");
        return true;
    case DiagnosticCategory::operation:
        writer.String("operation");
        return true;
    }
    return fail(error, "geometer.contract.enum_mismatch", "", "Unknown enum value.");
}

bool decode_DiagnosticA0(const rapidjson::Value& value, DiagnosticA0* out, const std::string& path,
                         ContractError* error)
{
    static const char* const names[] = {"code", "category",  "message",   "retryable",
                                        "path", "operation", "request_id"};
    if (!validate_object(value, names, 7U, path, error))
        return false;
    {
        const auto member = value.FindMember("code");
        if (member == value.MemberEnd())
            return fail(error, "geometer.contract.missing_field", child_path(path, "code"),
                        "Required field is missing.");
        if (!decode_string(member->value, &out->code, child_path(path, "code"), error, 1U,
                           std::numeric_limits<std::size_t>::max()))
            return false;
    }
    {
        const auto member = value.FindMember("category");
        if (member == value.MemberEnd())
            return fail(error, "geometer.contract.missing_field", child_path(path, "category"),
                        "Required field is missing.");
        if (!decode_DiagnosticCategory(member->value, &out->category, child_path(path, "category"),
                                       error))
            return false;
    }
    {
        const auto member = value.FindMember("message");
        if (member == value.MemberEnd())
            return fail(error, "geometer.contract.missing_field", child_path(path, "message"),
                        "Required field is missing.");
        if (!decode_string(member->value, &out->message, child_path(path, "message"), error, 0U,
                           std::numeric_limits<std::size_t>::max()))
            return false;
    }
    {
        const auto member = value.FindMember("retryable");
        if (member == value.MemberEnd())
            return fail(error, "geometer.contract.missing_field", child_path(path, "retryable"),
                        "Required field is missing.");
        if (!decode_boolean(member->value, &out->retryable, child_path(path, "retryable"), error))
            return false;
    }
    {
        const auto member = value.FindMember("path");
        if (member != value.MemberEnd())
        {
            std::string decoded{};
            if (!decode_string(member->value, &decoded, child_path(path, "path"), error, 0U,
                               std::numeric_limits<std::size_t>::max()))
                return false;
            out->path = std::move(decoded);
        }
        else
            out->path.reset();
    }
    {
        const auto member = value.FindMember("operation");
        if (member != value.MemberEnd())
        {
            std::string decoded{};
            if (!decode_string(member->value, &decoded, child_path(path, "operation"), error, 0U,
                               std::numeric_limits<std::size_t>::max()))
                return false;
            out->operation = std::move(decoded);
        }
        else
            out->operation.reset();
    }
    {
        const auto member = value.FindMember("request_id");
        if (member != value.MemberEnd())
        {
            std::string decoded{};
            if (!decode_string(member->value, &decoded, child_path(path, "request_id"), error, 0U,
                               std::numeric_limits<std::size_t>::max()))
                return false;
            out->request_id = std::move(decoded);
        }
        else
            out->request_id.reset();
    }
    return true;
}

bool write_DiagnosticA0(rapidjson::Writer<rapidjson::StringBuffer>& writer,
                        const DiagnosticA0& value, ContractError* error)
{
    writer.StartObject();
    writer.Key("code");
    if (!write_string(writer, value.code, error, 1U, std::numeric_limits<std::size_t>::max()))
        return false;
    writer.Key("category");
    if (!write_DiagnosticCategory(writer, value.category, error))
        return false;
    writer.Key("message");
    if (!write_string(writer, value.message, error, 0U, std::numeric_limits<std::size_t>::max()))
        return false;
    writer.Key("retryable");
    if (!(writer.Bool(value.retryable), true))
        return false;
    if (value.path.has_value())
    {
        writer.Key("path");
        if (!write_string(writer, *value.path, error, 0U, std::numeric_limits<std::size_t>::max()))
            return false;
    }
    if (value.operation.has_value())
    {
        writer.Key("operation");
        if (!write_string(writer, *value.operation, error, 0U,
                          std::numeric_limits<std::size_t>::max()))
            return false;
    }
    if (value.request_id.has_value())
    {
        writer.Key("request_id");
        if (!write_string(writer, *value.request_id, error, 0U,
                          std::numeric_limits<std::size_t>::max()))
            return false;
    }
    writer.EndObject();
    return true;
}

bool decode_PackedAttachmentReferenceA0(const rapidjson::Value& value,
                                        PackedAttachmentReferenceA0* out, const std::string& path,
                                        ContractError* error)
{
    static const char* const names[] = {"attachment", "format"};
    if (!validate_object(value, names, 2U, path, error))
        return false;
    {
        const auto member = value.FindMember("attachment");
        if (member == value.MemberEnd())
            return fail(error, "geometer.contract.missing_field", child_path(path, "attachment"),
                        "Required field is missing.");
        if (!decode_string(member->value, &out->attachment, child_path(path, "attachment"), error,
                           1U, 128U))
            return false;
    }
    {
        const auto member = value.FindMember("format");
        if (member == value.MemberEnd())
            return fail(error, "geometer.contract.missing_field", child_path(path, "format"),
                        "Required field is missing.");
        if (!decode_string(member->value, &out->format, child_path(path, "format"), error, 1U,
                           128U))
            return false;
    }
    return true;
}

bool write_PackedAttachmentReferenceA0(rapidjson::Writer<rapidjson::StringBuffer>& writer,
                                       const PackedAttachmentReferenceA0& value,
                                       ContractError* error)
{
    writer.StartObject();
    writer.Key("attachment");
    if (!write_string(writer, value.attachment, error, 1U, 128U))
        return false;
    writer.Key("format");
    if (!write_string(writer, value.format, error, 1U, 128U))
        return false;
    writer.EndObject();
    return true;
}

bool decode_PackedAttachmentProjectionA0(const rapidjson::Value& value,
                                         PackedAttachmentProjectionA0* out, const std::string& path,
                                         ContractError* error)
{
    static const char* const names[] = {"schema", "packet"};
    if (!validate_object(value, names, 2U, path, error))
        return false;
    {
        const auto member = value.FindMember("schema");
        if (member == value.MemberEnd())
            return fail(error, "geometer.contract.missing_field", child_path(path, "schema"),
                        "Required field is missing.");
        if (!decode_string(member->value, &out->schema, child_path(path, "schema"), error, 1U,
                           128U))
            return false;
    }
    {
        const auto member = value.FindMember("packet");
        if (member == value.MemberEnd())
            return fail(error, "geometer.contract.missing_field", child_path(path, "packet"),
                        "Required field is missing.");
        if (!decode_PackedAttachmentReferenceA0(member->value, &out->packet,
                                                child_path(path, "packet"), error))
            return false;
    }
    return true;
}

bool write_PackedAttachmentProjectionA0(rapidjson::Writer<rapidjson::StringBuffer>& writer,
                                        const PackedAttachmentProjectionA0& value,
                                        ContractError* error)
{
    writer.StartObject();
    writer.Key("schema");
    if (!write_string(writer, value.schema, error, 1U, 128U))
        return false;
    writer.Key("packet");
    if (!write_PackedAttachmentReferenceA0(writer, value.packet, error))
        return false;
    writer.EndObject();
    return true;
}

bool decode_FastHlrLimitsA0(const rapidjson::Value& value, FastHlrLimitsA0* out,
                            const std::string& path, ContractError* error)
{
    static const char* const names[] = {
        "max_vertices",        "max_triangles", "max_edges",          "max_grid_references",
        "max_candidate_pairs", "max_fragments", "max_output_segments"};
    if (!validate_object(value, names, 7U, path, error))
        return false;
    {
        const auto member = value.FindMember("max_vertices");
        if (member != value.MemberEnd())
        {
            std::uint32_t decoded{};
            if (!decode_uint32(member->value, &decoded, child_path(path, "max_vertices"), error,
                               1ULL, 4294967295ULL))
                return false;
            out->max_vertices = std::move(decoded);
        }
        else
            out->max_vertices.reset();
    }
    {
        const auto member = value.FindMember("max_triangles");
        if (member != value.MemberEnd())
        {
            std::uint32_t decoded{};
            if (!decode_uint32(member->value, &decoded, child_path(path, "max_triangles"), error,
                               1ULL, 4294967295ULL))
                return false;
            out->max_triangles = std::move(decoded);
        }
        else
            out->max_triangles.reset();
    }
    {
        const auto member = value.FindMember("max_edges");
        if (member != value.MemberEnd())
        {
            std::uint32_t decoded{};
            if (!decode_uint32(member->value, &decoded, child_path(path, "max_edges"), error, 1ULL,
                               4294967295ULL))
                return false;
            out->max_edges = std::move(decoded);
        }
        else
            out->max_edges.reset();
    }
    {
        const auto member = value.FindMember("max_grid_references");
        if (member != value.MemberEnd())
        {
            std::uint32_t decoded{};
            if (!decode_uint32(member->value, &decoded, child_path(path, "max_grid_references"),
                               error, 1ULL, 4294967295ULL))
                return false;
            out->max_grid_references = std::move(decoded);
        }
        else
            out->max_grid_references.reset();
    }
    {
        const auto member = value.FindMember("max_candidate_pairs");
        if (member != value.MemberEnd())
        {
            std::uint32_t decoded{};
            if (!decode_uint32(member->value, &decoded, child_path(path, "max_candidate_pairs"),
                               error, 1ULL, 4294967295ULL))
                return false;
            out->max_candidate_pairs = std::move(decoded);
        }
        else
            out->max_candidate_pairs.reset();
    }
    {
        const auto member = value.FindMember("max_fragments");
        if (member != value.MemberEnd())
        {
            std::uint32_t decoded{};
            if (!decode_uint32(member->value, &decoded, child_path(path, "max_fragments"), error,
                               1ULL, 4294967295ULL))
                return false;
            out->max_fragments = std::move(decoded);
        }
        else
            out->max_fragments.reset();
    }
    {
        const auto member = value.FindMember("max_output_segments");
        if (member != value.MemberEnd())
        {
            std::uint32_t decoded{};
            if (!decode_uint32(member->value, &decoded, child_path(path, "max_output_segments"),
                               error, 1ULL, 4294967295ULL))
                return false;
            out->max_output_segments = std::move(decoded);
        }
        else
            out->max_output_segments.reset();
    }
    return true;
}

bool write_FastHlrLimitsA0(rapidjson::Writer<rapidjson::StringBuffer>& writer,
                           const FastHlrLimitsA0& value, ContractError* error)
{
    writer.StartObject();
    if (value.max_vertices.has_value())
    {
        writer.Key("max_vertices");
        if (!write_uint32(writer, *value.max_vertices, error, 1ULL, 4294967295ULL))
            return false;
    }
    if (value.max_triangles.has_value())
    {
        writer.Key("max_triangles");
        if (!write_uint32(writer, *value.max_triangles, error, 1ULL, 4294967295ULL))
            return false;
    }
    if (value.max_edges.has_value())
    {
        writer.Key("max_edges");
        if (!write_uint32(writer, *value.max_edges, error, 1ULL, 4294967295ULL))
            return false;
    }
    if (value.max_grid_references.has_value())
    {
        writer.Key("max_grid_references");
        if (!write_uint32(writer, *value.max_grid_references, error, 1ULL, 4294967295ULL))
            return false;
    }
    if (value.max_candidate_pairs.has_value())
    {
        writer.Key("max_candidate_pairs");
        if (!write_uint32(writer, *value.max_candidate_pairs, error, 1ULL, 4294967295ULL))
            return false;
    }
    if (value.max_fragments.has_value())
    {
        writer.Key("max_fragments");
        if (!write_uint32(writer, *value.max_fragments, error, 1ULL, 4294967295ULL))
            return false;
    }
    if (value.max_output_segments.has_value())
    {
        writer.Key("max_output_segments");
        if (!write_uint32(writer, *value.max_output_segments, error, 1ULL, 4294967295ULL))
            return false;
    }
    writer.EndObject();
    return true;
}

bool decode_FastHlrOptionsA0(const rapidjson::Value& value, FastHlrOptionsA0* out,
                             const std::string& path, ContractError* error)
{
    static const char* const names[] = {"include_boundaries",
                                        "include_creases",
                                        "include_silhouettes",
                                        "include_hidden",
                                        "suppress_coplanar_seams",
                                        "crease_angle_rad",
                                        "weld_tolerance",
                                        "projected_tolerance",
                                        "depth_tolerance",
                                        "coplanar_seam_angle_rad",
                                        "coplanar_seam_depth_tolerance",
                                        "coplanar_seam_lateral_tolerance",
                                        "limits"};
    if (!validate_object(value, names, 13U, path, error))
        return false;
    {
        const auto member = value.FindMember("include_boundaries");
        if (member != value.MemberEnd())
        {
            bool decoded{};
            if (!decode_boolean(member->value, &decoded, child_path(path, "include_boundaries"),
                                error))
                return false;
            out->include_boundaries = std::move(decoded);
        }
        else
            out->include_boundaries.reset();
    }
    {
        const auto member = value.FindMember("include_creases");
        if (member != value.MemberEnd())
        {
            bool decoded{};
            if (!decode_boolean(member->value, &decoded, child_path(path, "include_creases"),
                                error))
                return false;
            out->include_creases = std::move(decoded);
        }
        else
            out->include_creases.reset();
    }
    {
        const auto member = value.FindMember("include_silhouettes");
        if (member != value.MemberEnd())
        {
            bool decoded{};
            if (!decode_boolean(member->value, &decoded, child_path(path, "include_silhouettes"),
                                error))
                return false;
            out->include_silhouettes = std::move(decoded);
        }
        else
            out->include_silhouettes.reset();
    }
    {
        const auto member = value.FindMember("include_hidden");
        if (member != value.MemberEnd())
        {
            bool decoded{};
            if (!decode_boolean(member->value, &decoded, child_path(path, "include_hidden"), error))
                return false;
            out->include_hidden = std::move(decoded);
        }
        else
            out->include_hidden.reset();
    }
    {
        const auto member = value.FindMember("suppress_coplanar_seams");
        if (member != value.MemberEnd())
        {
            bool decoded{};
            if (!decode_boolean(member->value, &decoded,
                                child_path(path, "suppress_coplanar_seams"), error))
                return false;
            out->suppress_coplanar_seams = std::move(decoded);
        }
        else
            out->suppress_coplanar_seams.reset();
    }
    {
        const auto member = value.FindMember("crease_angle_rad");
        if (member != value.MemberEnd())
        {
            double decoded{};
            if (!decode_double(member->value, &decoded, child_path(path, "crease_angle_rad"), error,
                               0, 3.141592653589793, false, false))
                return false;
            out->crease_angle_rad = std::move(decoded);
        }
        else
            out->crease_angle_rad.reset();
    }
    {
        const auto member = value.FindMember("weld_tolerance");
        if (member != value.MemberEnd())
        {
            double decoded{};
            if (!decode_double(member->value, &decoded, child_path(path, "weld_tolerance"), error,
                               0, std::numeric_limits<double>::infinity(), true, false))
                return false;
            out->weld_tolerance = std::move(decoded);
        }
        else
            out->weld_tolerance.reset();
    }
    {
        const auto member = value.FindMember("projected_tolerance");
        if (member != value.MemberEnd())
        {
            double decoded{};
            if (!decode_double(member->value, &decoded, child_path(path, "projected_tolerance"),
                               error, 0, std::numeric_limits<double>::infinity(), true, false))
                return false;
            out->projected_tolerance = std::move(decoded);
        }
        else
            out->projected_tolerance.reset();
    }
    {
        const auto member = value.FindMember("depth_tolerance");
        if (member != value.MemberEnd())
        {
            double decoded{};
            if (!decode_double(member->value, &decoded, child_path(path, "depth_tolerance"), error,
                               0, std::numeric_limits<double>::infinity(), false, false))
                return false;
            out->depth_tolerance = std::move(decoded);
        }
        else
            out->depth_tolerance.reset();
    }
    {
        const auto member = value.FindMember("coplanar_seam_angle_rad");
        if (member != value.MemberEnd())
        {
            double decoded{};
            if (!decode_double(member->value, &decoded, child_path(path, "coplanar_seam_angle_rad"),
                               error, 0, 1.5707963267948966, false, false))
                return false;
            out->coplanar_seam_angle_rad = std::move(decoded);
        }
        else
            out->coplanar_seam_angle_rad.reset();
    }
    {
        const auto member = value.FindMember("coplanar_seam_depth_tolerance");
        if (member != value.MemberEnd())
        {
            double decoded{};
            if (!decode_double(member->value, &decoded,
                               child_path(path, "coplanar_seam_depth_tolerance"), error, 0,
                               std::numeric_limits<double>::infinity(), false, false))
                return false;
            out->coplanar_seam_depth_tolerance = std::move(decoded);
        }
        else
            out->coplanar_seam_depth_tolerance.reset();
    }
    {
        const auto member = value.FindMember("coplanar_seam_lateral_tolerance");
        if (member != value.MemberEnd())
        {
            double decoded{};
            if (!decode_double(member->value, &decoded,
                               child_path(path, "coplanar_seam_lateral_tolerance"), error, 0,
                               std::numeric_limits<double>::infinity(), false, false))
                return false;
            out->coplanar_seam_lateral_tolerance = std::move(decoded);
        }
        else
            out->coplanar_seam_lateral_tolerance.reset();
    }
    {
        const auto member = value.FindMember("limits");
        if (member != value.MemberEnd())
        {
            FastHlrLimitsA0 decoded{};
            if (!decode_FastHlrLimitsA0(member->value, &decoded, child_path(path, "limits"), error))
                return false;
            out->limits = std::move(decoded);
        }
        else
            out->limits.reset();
    }
    return true;
}

bool write_FastHlrOptionsA0(rapidjson::Writer<rapidjson::StringBuffer>& writer,
                            const FastHlrOptionsA0& value, ContractError* error)
{
    writer.StartObject();
    if (value.include_boundaries.has_value())
    {
        writer.Key("include_boundaries");
        if (!(writer.Bool(*value.include_boundaries), true))
            return false;
    }
    if (value.include_creases.has_value())
    {
        writer.Key("include_creases");
        if (!(writer.Bool(*value.include_creases), true))
            return false;
    }
    if (value.include_silhouettes.has_value())
    {
        writer.Key("include_silhouettes");
        if (!(writer.Bool(*value.include_silhouettes), true))
            return false;
    }
    if (value.include_hidden.has_value())
    {
        writer.Key("include_hidden");
        if (!(writer.Bool(*value.include_hidden), true))
            return false;
    }
    if (value.suppress_coplanar_seams.has_value())
    {
        writer.Key("suppress_coplanar_seams");
        if (!(writer.Bool(*value.suppress_coplanar_seams), true))
            return false;
    }
    if (value.crease_angle_rad.has_value())
    {
        writer.Key("crease_angle_rad");
        if (!write_double(writer, *value.crease_angle_rad, error, 0, 3.141592653589793, false,
                          false))
            return false;
    }
    if (value.weld_tolerance.has_value())
    {
        writer.Key("weld_tolerance");
        if (!write_double(writer, *value.weld_tolerance, error, 0,
                          std::numeric_limits<double>::infinity(), true, false))
            return false;
    }
    if (value.projected_tolerance.has_value())
    {
        writer.Key("projected_tolerance");
        if (!write_double(writer, *value.projected_tolerance, error, 0,
                          std::numeric_limits<double>::infinity(), true, false))
            return false;
    }
    if (value.depth_tolerance.has_value())
    {
        writer.Key("depth_tolerance");
        if (!write_double(writer, *value.depth_tolerance, error, 0,
                          std::numeric_limits<double>::infinity(), false, false))
            return false;
    }
    if (value.coplanar_seam_angle_rad.has_value())
    {
        writer.Key("coplanar_seam_angle_rad");
        if (!write_double(writer, *value.coplanar_seam_angle_rad, error, 0, 1.5707963267948966,
                          false, false))
            return false;
    }
    if (value.coplanar_seam_depth_tolerance.has_value())
    {
        writer.Key("coplanar_seam_depth_tolerance");
        if (!write_double(writer, *value.coplanar_seam_depth_tolerance, error, 0,
                          std::numeric_limits<double>::infinity(), false, false))
            return false;
    }
    if (value.coplanar_seam_lateral_tolerance.has_value())
    {
        writer.Key("coplanar_seam_lateral_tolerance");
        if (!write_double(writer, *value.coplanar_seam_lateral_tolerance, error, 0,
                          std::numeric_limits<double>::infinity(), false, false))
            return false;
    }
    if (value.limits.has_value())
    {
        writer.Key("limits");
        if (!write_FastHlrLimitsA0(writer, *value.limits, error))
            return false;
    }
    writer.EndObject();
    return true;
}

bool decode_HlrCurveMode(const rapidjson::Value& value, HlrCurveMode* out, const std::string& path,
                         ContractError* error)
{
    if (!value.IsString())
        return fail(error, "geometer.contract.type_mismatch", path, "Expected a string enum.");
    const std::string text(value.GetString(), value.GetStringLength());
    if (text == "native_arcs")
    {
        *out = HlrCurveMode::native_arcs;
        return true;
    }
    if (text == "polyline")
    {
        *out = HlrCurveMode::polyline;
        return true;
    }
    return fail(error, "geometer.contract.enum_mismatch", path, "Unknown enum value.");
}

bool write_HlrCurveMode(rapidjson::Writer<rapidjson::StringBuffer>& writer,
                        const HlrCurveMode& value, ContractError* error)
{
    switch (value)
    {
    case HlrCurveMode::native_arcs:
        writer.String("native_arcs");
        return true;
    case HlrCurveMode::polyline:
        writer.String("polyline");
        return true;
    }
    return fail(error, "geometer.contract.enum_mismatch", "", "Unknown enum value.");
}

bool decode_HlrMatrix4x4(const rapidjson::Value& value, HlrMatrix4x4* out, const std::string& path,
                         ContractError* error)
{
    if (!value.IsArray() || value.Size() < 16U || value.Size() > 16U)
        return fail(error, "geometer.contract.array_size", path,
                    "Array length is outside its contract bounds.");
    out->clear();
    out->reserve(value.Size());
    for (rapidjson::SizeType i = 0; i < value.Size(); ++i)
    {
        double item_value{};
        if (!decode_double(value[i], &item_value, path + "/" + std::to_string(i), error,
                           -std::numeric_limits<double>::infinity(),
                           std::numeric_limits<double>::infinity(), false, false))
            return false;
        out->push_back(std::move(item_value));
    }
    return true;
}

bool write_HlrMatrix4x4(rapidjson::Writer<rapidjson::StringBuffer>& writer,
                        const HlrMatrix4x4& value, ContractError* error)
{
    if (value.size() < 16U || value.size() > 16U)
        return fail(error, "geometer.contract.array_size", "",
                    "Array length is outside its contract bounds.");
    writer.StartArray();
    for (const auto& item_value : value)
        if (!write_double(writer, item_value, error, -std::numeric_limits<double>::infinity(),
                          std::numeric_limits<double>::infinity(), false, false))
            return false;
    writer.EndArray();
    return true;
}

bool decode_HlrMeshDeflectionMode(const rapidjson::Value& value, HlrMeshDeflectionMode* out,
                                  const std::string& path, ContractError* error)
{
    if (!value.IsString())
        return fail(error, "geometer.contract.type_mismatch", path, "Expected a string enum.");
    const std::string text(value.GetString(), value.GetStringLength());
    if (text == "absolute")
    {
        *out = HlrMeshDeflectionMode::absolute;
        return true;
    }
    if (text == "bbox-relative")
    {
        *out = HlrMeshDeflectionMode::bbox_relative;
        return true;
    }
    return fail(error, "geometer.contract.enum_mismatch", path, "Unknown enum value.");
}

bool write_HlrMeshDeflectionMode(rapidjson::Writer<rapidjson::StringBuffer>& writer,
                                 const HlrMeshDeflectionMode& value, ContractError* error)
{
    switch (value)
    {
    case HlrMeshDeflectionMode::absolute:
        writer.String("absolute");
        return true;
    case HlrMeshDeflectionMode::bbox_relative:
        writer.String("bbox-relative");
        return true;
    }
    return fail(error, "geometer.contract.enum_mismatch", "", "Unknown enum value.");
}

bool decode_HlrOutlineAlgorithm(const rapidjson::Value& value, HlrOutlineAlgorithm* out,
                                const std::string& path, ContractError* error)
{
    if (!value.IsString())
        return fail(error, "geometer.contract.type_mismatch", path, "Expected a string enum.");
    const std::string text(value.GetString(), value.GetStringLength());
    if (text == "hlr-close")
    {
        *out = HlrOutlineAlgorithm::hlr_close;
        return true;
    }
    if (text == "mesh-shadow")
    {
        *out = HlrOutlineAlgorithm::mesh_shadow;
        return true;
    }
    if (text == "fast-mesh-shadow")
    {
        *out = HlrOutlineAlgorithm::fast_mesh_shadow;
        return true;
    }
    return fail(error, "geometer.contract.enum_mismatch", path, "Unknown enum value.");
}

bool write_HlrOutlineAlgorithm(rapidjson::Writer<rapidjson::StringBuffer>& writer,
                               const HlrOutlineAlgorithm& value, ContractError* error)
{
    switch (value)
    {
    case HlrOutlineAlgorithm::hlr_close:
        writer.String("hlr-close");
        return true;
    case HlrOutlineAlgorithm::mesh_shadow:
        writer.String("mesh-shadow");
        return true;
    case HlrOutlineAlgorithm::fast_mesh_shadow:
        writer.String("fast-mesh-shadow");
        return true;
    }
    return fail(error, "geometer.contract.enum_mismatch", "", "Unknown enum value.");
}

bool decode_HlrVector3(const rapidjson::Value& value, HlrVector3* out, const std::string& path,
                       ContractError* error)
{
    if (!value.IsArray() || value.Size() < 3U || value.Size() > 3U)
        return fail(error, "geometer.contract.array_size", path,
                    "Array length is outside its contract bounds.");
    out->clear();
    out->reserve(value.Size());
    for (rapidjson::SizeType i = 0; i < value.Size(); ++i)
    {
        double item_value{};
        if (!decode_double(value[i], &item_value, path + "/" + std::to_string(i), error,
                           -std::numeric_limits<double>::infinity(),
                           std::numeric_limits<double>::infinity(), false, false))
            return false;
        out->push_back(std::move(item_value));
    }
    return true;
}

bool write_HlrVector3(rapidjson::Writer<rapidjson::StringBuffer>& writer, const HlrVector3& value,
                      ContractError* error)
{
    if (value.size() < 3U || value.size() > 3U)
        return fail(error, "geometer.contract.array_size", "",
                    "Array length is outside its contract bounds.");
    writer.StartArray();
    for (const auto& item_value : value)
        if (!write_double(writer, item_value, error, -std::numeric_limits<double>::infinity(),
                          std::numeric_limits<double>::infinity(), false, false))
            return false;
    writer.EndArray();
    return true;
}

bool decode_ProjectedSegment(const rapidjson::Value& value, ProjectedSegment* out,
                             const std::string& path, ContractError* error)
{
    if (!value.IsArray() || value.Size() < 4U || value.Size() > 4U)
        return fail(error, "geometer.contract.array_size", path,
                    "Array length is outside its contract bounds.");
    out->clear();
    out->reserve(value.Size());
    for (rapidjson::SizeType i = 0; i < value.Size(); ++i)
    {
        double item_value{};
        if (!decode_double(value[i], &item_value, path + "/" + std::to_string(i), error,
                           -std::numeric_limits<double>::infinity(),
                           std::numeric_limits<double>::infinity(), false, false))
            return false;
        out->push_back(std::move(item_value));
    }
    return true;
}

bool write_ProjectedSegment(rapidjson::Writer<rapidjson::StringBuffer>& writer,
                            const ProjectedSegment& value, ContractError* error)
{
    if (value.size() < 4U || value.size() > 4U)
        return fail(error, "geometer.contract.array_size", "",
                    "Array length is outside its contract bounds.");
    writer.StartArray();
    for (const auto& item_value : value)
        if (!write_double(writer, item_value, error, -std::numeric_limits<double>::infinity(),
                          std::numeric_limits<double>::infinity(), false, false))
            return false;
    writer.EndArray();
    return true;
}

bool decode_HlrVector2(const rapidjson::Value& value, HlrVector2* out, const std::string& path,
                       ContractError* error)
{
    if (!value.IsArray() || value.Size() < 2U || value.Size() > 2U)
        return fail(error, "geometer.contract.array_size", path,
                    "Array length is outside its contract bounds.");
    out->clear();
    out->reserve(value.Size());
    for (rapidjson::SizeType i = 0; i < value.Size(); ++i)
    {
        double item_value{};
        if (!decode_double(value[i], &item_value, path + "/" + std::to_string(i), error,
                           -std::numeric_limits<double>::infinity(),
                           std::numeric_limits<double>::infinity(), false, false))
            return false;
        out->push_back(std::move(item_value));
    }
    return true;
}

bool write_HlrVector2(rapidjson::Writer<rapidjson::StringBuffer>& writer, const HlrVector2& value,
                      ContractError* error)
{
    if (value.size() < 2U || value.size() > 2U)
        return fail(error, "geometer.contract.array_size", "",
                    "Array length is outside its contract bounds.");
    writer.StartArray();
    for (const auto& item_value : value)
        if (!write_double(writer, item_value, error, -std::numeric_limits<double>::infinity(),
                          std::numeric_limits<double>::infinity(), false, false))
            return false;
    writer.EndArray();
    return true;
}

bool decode_ProjectedArc(const rapidjson::Value& value, ProjectedArc* out, const std::string& path,
                         ContractError* error)
{
    static const char* const names[] = {"start",      "end", "center",     "radius",
                                        "extent_rad", "ccw", "full_circle"};
    if (!validate_object(value, names, 7U, path, error))
        return false;
    {
        const auto member = value.FindMember("start");
        if (member == value.MemberEnd())
            return fail(error, "geometer.contract.missing_field", child_path(path, "start"),
                        "Required field is missing.");
        if (!decode_HlrVector2(member->value, &out->start, child_path(path, "start"), error))
            return false;
    }
    {
        const auto member = value.FindMember("end");
        if (member == value.MemberEnd())
            return fail(error, "geometer.contract.missing_field", child_path(path, "end"),
                        "Required field is missing.");
        if (!decode_HlrVector2(member->value, &out->end, child_path(path, "end"), error))
            return false;
    }
    {
        const auto member = value.FindMember("center");
        if (member == value.MemberEnd())
            return fail(error, "geometer.contract.missing_field", child_path(path, "center"),
                        "Required field is missing.");
        if (!decode_HlrVector2(member->value, &out->center, child_path(path, "center"), error))
            return false;
    }
    {
        const auto member = value.FindMember("radius");
        if (member == value.MemberEnd())
            return fail(error, "geometer.contract.missing_field", child_path(path, "radius"),
                        "Required field is missing.");
        if (!decode_double(member->value, &out->radius, child_path(path, "radius"), error, 0,
                           std::numeric_limits<double>::infinity(), false, false))
            return false;
    }
    {
        const auto member = value.FindMember("extent_rad");
        if (member == value.MemberEnd())
            return fail(error, "geometer.contract.missing_field", child_path(path, "extent_rad"),
                        "Required field is missing.");
        if (!decode_double(member->value, &out->extent_rad, child_path(path, "extent_rad"), error,
                           0, 6.283185307179586, false, false))
            return false;
    }
    {
        const auto member = value.FindMember("ccw");
        if (member == value.MemberEnd())
            return fail(error, "geometer.contract.missing_field", child_path(path, "ccw"),
                        "Required field is missing.");
        if (!decode_boolean(member->value, &out->ccw, child_path(path, "ccw"), error))
            return false;
    }
    {
        const auto member = value.FindMember("full_circle");
        if (member == value.MemberEnd())
            return fail(error, "geometer.contract.missing_field", child_path(path, "full_circle"),
                        "Required field is missing.");
        if (!decode_boolean(member->value, &out->full_circle, child_path(path, "full_circle"),
                            error))
            return false;
    }
    return true;
}

bool write_ProjectedArc(rapidjson::Writer<rapidjson::StringBuffer>& writer,
                        const ProjectedArc& value, ContractError* error)
{
    writer.StartObject();
    writer.Key("start");
    if (!write_HlrVector2(writer, value.start, error))
        return false;
    writer.Key("end");
    if (!write_HlrVector2(writer, value.end, error))
        return false;
    writer.Key("center");
    if (!write_HlrVector2(writer, value.center, error))
        return false;
    writer.Key("radius");
    if (!write_double(writer, value.radius, error, 0, std::numeric_limits<double>::infinity(),
                      false, false))
        return false;
    writer.Key("extent_rad");
    if (!write_double(writer, value.extent_rad, error, 0, 6.283185307179586, false, false))
        return false;
    writer.Key("ccw");
    if (!(writer.Bool(value.ccw), true))
        return false;
    writer.Key("full_circle");
    if (!(writer.Bool(value.full_circle), true))
        return false;
    writer.EndObject();
    return true;
}

bool decode_ProjectionBounds(const rapidjson::Value& value, ProjectionBounds* out,
                             const std::string& path, ContractError* error)
{
    static const char* const names[] = {"min_x", "min_y", "max_x", "max_y", "width", "height"};
    if (!validate_object(value, names, 6U, path, error))
        return false;
    {
        const auto member = value.FindMember("min_x");
        if (member == value.MemberEnd())
            return fail(error, "geometer.contract.missing_field", child_path(path, "min_x"),
                        "Required field is missing.");
        if (!decode_double(member->value, &out->min_x, child_path(path, "min_x"), error,
                           -std::numeric_limits<double>::infinity(),
                           std::numeric_limits<double>::infinity(), false, false))
            return false;
    }
    {
        const auto member = value.FindMember("min_y");
        if (member == value.MemberEnd())
            return fail(error, "geometer.contract.missing_field", child_path(path, "min_y"),
                        "Required field is missing.");
        if (!decode_double(member->value, &out->min_y, child_path(path, "min_y"), error,
                           -std::numeric_limits<double>::infinity(),
                           std::numeric_limits<double>::infinity(), false, false))
            return false;
    }
    {
        const auto member = value.FindMember("max_x");
        if (member == value.MemberEnd())
            return fail(error, "geometer.contract.missing_field", child_path(path, "max_x"),
                        "Required field is missing.");
        if (!decode_double(member->value, &out->max_x, child_path(path, "max_x"), error,
                           -std::numeric_limits<double>::infinity(),
                           std::numeric_limits<double>::infinity(), false, false))
            return false;
    }
    {
        const auto member = value.FindMember("max_y");
        if (member == value.MemberEnd())
            return fail(error, "geometer.contract.missing_field", child_path(path, "max_y"),
                        "Required field is missing.");
        if (!decode_double(member->value, &out->max_y, child_path(path, "max_y"), error,
                           -std::numeric_limits<double>::infinity(),
                           std::numeric_limits<double>::infinity(), false, false))
            return false;
    }
    {
        const auto member = value.FindMember("width");
        if (member == value.MemberEnd())
            return fail(error, "geometer.contract.missing_field", child_path(path, "width"),
                        "Required field is missing.");
        if (!decode_double(member->value, &out->width, child_path(path, "width"), error, 0,
                           std::numeric_limits<double>::infinity(), false, false))
            return false;
    }
    {
        const auto member = value.FindMember("height");
        if (member == value.MemberEnd())
            return fail(error, "geometer.contract.missing_field", child_path(path, "height"),
                        "Required field is missing.");
        if (!decode_double(member->value, &out->height, child_path(path, "height"), error, 0,
                           std::numeric_limits<double>::infinity(), false, false))
            return false;
    }
    return true;
}

bool write_ProjectionBounds(rapidjson::Writer<rapidjson::StringBuffer>& writer,
                            const ProjectionBounds& value, ContractError* error)
{
    writer.StartObject();
    writer.Key("min_x");
    if (!write_double(writer, value.min_x, error, -std::numeric_limits<double>::infinity(),
                      std::numeric_limits<double>::infinity(), false, false))
        return false;
    writer.Key("min_y");
    if (!write_double(writer, value.min_y, error, -std::numeric_limits<double>::infinity(),
                      std::numeric_limits<double>::infinity(), false, false))
        return false;
    writer.Key("max_x");
    if (!write_double(writer, value.max_x, error, -std::numeric_limits<double>::infinity(),
                      std::numeric_limits<double>::infinity(), false, false))
        return false;
    writer.Key("max_y");
    if (!write_double(writer, value.max_y, error, -std::numeric_limits<double>::infinity(),
                      std::numeric_limits<double>::infinity(), false, false))
        return false;
    writer.Key("width");
    if (!write_double(writer, value.width, error, 0, std::numeric_limits<double>::infinity(), false,
                      false))
        return false;
    writer.Key("height");
    if (!write_double(writer, value.height, error, 0, std::numeric_limits<double>::infinity(),
                      false, false))
        return false;
    writer.EndObject();
    return true;
}

bool decode_ProjectedGeometry(const rapidjson::Value& value, ProjectedGeometry* out,
                              const std::string& path, ContractError* error)
{
    static const char* const names[] = {"segments", "arcs", "bounds"};
    if (!validate_object(value, names, 3U, path, error))
        return false;
    {
        const auto member = value.FindMember("segments");
        if (member == value.MemberEnd())
            return fail(error, "geometer.contract.missing_field", child_path(path, "segments"),
                        "Required field is missing.");
        if (!decode_array(member->value, &out->segments, child_path(path, "segments"), error, 0U,
                          4000000U, decode_ProjectedSegment))
            return false;
    }
    {
        const auto member = value.FindMember("arcs");
        if (member == value.MemberEnd())
            return fail(error, "geometer.contract.missing_field", child_path(path, "arcs"),
                        "Required field is missing.");
        if (!decode_array(member->value, &out->arcs, child_path(path, "arcs"), error, 0U, 4000000U,
                          decode_ProjectedArc))
            return false;
    }
    {
        const auto member = value.FindMember("bounds");
        if (member != value.MemberEnd())
        {
            ProjectionBounds decoded{};
            if (!decode_ProjectionBounds(member->value, &decoded, child_path(path, "bounds"),
                                         error))
                return false;
            out->bounds = std::move(decoded);
        }
        else
            out->bounds.reset();
    }
    return true;
}

bool write_ProjectedGeometry(rapidjson::Writer<rapidjson::StringBuffer>& writer,
                             const ProjectedGeometry& value, ContractError* error)
{
    writer.StartObject();
    writer.Key("segments");
    if (!write_array(writer, value.segments, error, 0U, 4000000U, write_ProjectedSegment))
        return false;
    writer.Key("arcs");
    if (!write_array(writer, value.arcs, error, 0U, 4000000U, write_ProjectedArc))
        return false;
    if (value.bounds.has_value())
    {
        writer.Key("bounds");
        if (!write_ProjectionBounds(writer, *value.bounds, error))
            return false;
    }
    writer.EndObject();
    return true;
}

bool decode_HlrProjectionModes(const rapidjson::Value& value, HlrProjectionModes* out,
                               const std::string& path, ContractError* error)
{
    static const char* const names[] = {"outline", "detail", "bbox"};
    if (!validate_object(value, names, 3U, path, error))
        return false;
    {
        const auto member = value.FindMember("outline");
        if (member == value.MemberEnd())
            return fail(error, "geometer.contract.missing_field", child_path(path, "outline"),
                        "Required field is missing.");
        if (!decode_ProjectedGeometry(member->value, &out->outline, child_path(path, "outline"),
                                      error))
            return false;
    }
    {
        const auto member = value.FindMember("detail");
        if (member == value.MemberEnd())
            return fail(error, "geometer.contract.missing_field", child_path(path, "detail"),
                        "Required field is missing.");
        if (!decode_ProjectedGeometry(member->value, &out->detail, child_path(path, "detail"),
                                      error))
            return false;
    }
    {
        const auto member = value.FindMember("bbox");
        if (member == value.MemberEnd())
            return fail(error, "geometer.contract.missing_field", child_path(path, "bbox"),
                        "Required field is missing.");
        if (!decode_ProjectedGeometry(member->value, &out->bbox, child_path(path, "bbox"), error))
            return false;
    }
    return true;
}

bool write_HlrProjectionModes(rapidjson::Writer<rapidjson::StringBuffer>& writer,
                              const HlrProjectionModes& value, ContractError* error)
{
    writer.StartObject();
    writer.Key("outline");
    if (!write_ProjectedGeometry(writer, value.outline, error))
        return false;
    writer.Key("detail");
    if (!write_ProjectedGeometry(writer, value.detail, error))
        return false;
    writer.Key("bbox");
    if (!write_ProjectedGeometry(writer, value.bbox, error))
        return false;
    writer.EndObject();
    return true;
}

bool decode_HlrProjectedView(const rapidjson::Value& value, HlrProjectedView* out,
                             const std::string& path, ContractError* error)
{
    static const char* const names[] = {"id", "direction", "up", "modes"};
    if (!validate_object(value, names, 4U, path, error))
        return false;
    {
        const auto member = value.FindMember("id");
        if (member == value.MemberEnd())
            return fail(error, "geometer.contract.missing_field", child_path(path, "id"),
                        "Required field is missing.");
        if (!decode_string(member->value, &out->id, child_path(path, "id"), error, 1U, 128U))
            return false;
    }
    {
        const auto member = value.FindMember("direction");
        if (member == value.MemberEnd())
            return fail(error, "geometer.contract.missing_field", child_path(path, "direction"),
                        "Required field is missing.");
        if (!decode_HlrVector3(member->value, &out->direction, child_path(path, "direction"),
                               error))
            return false;
    }
    {
        const auto member = value.FindMember("up");
        if (member == value.MemberEnd())
            return fail(error, "geometer.contract.missing_field", child_path(path, "up"),
                        "Required field is missing.");
        if (!decode_HlrVector3(member->value, &out->up, child_path(path, "up"), error))
            return false;
    }
    {
        const auto member = value.FindMember("modes");
        if (member == value.MemberEnd())
            return fail(error, "geometer.contract.missing_field", child_path(path, "modes"),
                        "Required field is missing.");
        if (!decode_HlrProjectionModes(member->value, &out->modes, child_path(path, "modes"),
                                       error))
            return false;
    }
    return true;
}

bool write_HlrProjectedView(rapidjson::Writer<rapidjson::StringBuffer>& writer,
                            const HlrProjectedView& value, ContractError* error)
{
    writer.StartObject();
    writer.Key("id");
    if (!write_string(writer, value.id, error, 1U, 128U))
        return false;
    writer.Key("direction");
    if (!write_HlrVector3(writer, value.direction, error))
        return false;
    writer.Key("up");
    if (!write_HlrVector3(writer, value.up, error))
        return false;
    writer.Key("modes");
    if (!write_HlrProjectionModes(writer, value.modes, error))
        return false;
    writer.EndObject();
    return true;
}

bool decode_HlrProjectionAlgorithm(const rapidjson::Value& value, HlrProjectionAlgorithm* out,
                                   const std::string& path, ContractError* error)
{
    if (!value.IsString())
        return fail(error, "geometer.contract.type_mismatch", path, "Expected a string enum.");
    const std::string text(value.GetString(), value.GetStringLength());
    if (text == "poly")
    {
        *out = HlrProjectionAlgorithm::poly;
        return true;
    }
    if (text == "exact")
    {
        *out = HlrProjectionAlgorithm::exact;
        return true;
    }
    if (text == "fast")
    {
        *out = HlrProjectionAlgorithm::fast;
        return true;
    }
    return fail(error, "geometer.contract.enum_mismatch", path, "Unknown enum value.");
}

bool write_HlrProjectionAlgorithm(rapidjson::Writer<rapidjson::StringBuffer>& writer,
                                  const HlrProjectionAlgorithm& value, ContractError* error)
{
    switch (value)
    {
    case HlrProjectionAlgorithm::poly:
        writer.String("poly");
        return true;
    case HlrProjectionAlgorithm::exact:
        writer.String("exact");
        return true;
    case HlrProjectionAlgorithm::fast:
        writer.String("fast");
        return true;
    }
    return fail(error, "geometer.contract.enum_mismatch", "", "Unknown enum value.");
}

bool decode_HlrViewSpec(const rapidjson::Value& value, HlrViewSpec* out, const std::string& path,
                        ContractError* error)
{
    static const char* const names[] = {"id", "direction", "up"};
    if (!validate_object(value, names, 3U, path, error))
        return false;
    {
        const auto member = value.FindMember("id");
        if (member == value.MemberEnd())
            return fail(error, "geometer.contract.missing_field", child_path(path, "id"),
                        "Required field is missing.");
        if (!decode_string(member->value, &out->id, child_path(path, "id"), error, 1U, 128U))
            return false;
    }
    {
        const auto member = value.FindMember("direction");
        if (member == value.MemberEnd())
            return fail(error, "geometer.contract.missing_field", child_path(path, "direction"),
                        "Required field is missing.");
        if (!decode_HlrVector3(member->value, &out->direction, child_path(path, "direction"),
                               error))
            return false;
    }
    {
        const auto member = value.FindMember("up");
        if (member == value.MemberEnd())
            return fail(error, "geometer.contract.missing_field", child_path(path, "up"),
                        "Required field is missing.");
        if (!decode_HlrVector3(member->value, &out->up, child_path(path, "up"), error))
            return false;
    }
    return true;
}

bool write_HlrViewSpec(rapidjson::Writer<rapidjson::StringBuffer>& writer, const HlrViewSpec& value,
                       ContractError* error)
{
    writer.StartObject();
    writer.Key("id");
    if (!write_string(writer, value.id, error, 1U, 128U))
        return false;
    writer.Key("direction");
    if (!write_HlrVector3(writer, value.direction, error))
        return false;
    writer.Key("up");
    if (!write_HlrVector3(writer, value.up, error))
        return false;
    writer.EndObject();
    return true;
}

bool decode_HlrProjectionOptionsA0(const rapidjson::Value& value, HlrProjectionOptionsA0* out,
                                   const std::string& path, ContractError* error)
{
    static const char* const names[] = {"views",
                                        "output_outline",
                                        "output_detail",
                                        "output_bbox",
                                        "model_transform",
                                        "strip_root_placement",
                                        "curve_mode",
                                        "samples_per_curve",
                                        "round_digits",
                                        "edge_v_sharp",
                                        "edge_v_outline",
                                        "edge_v_smooth",
                                        "edge_v_sewn",
                                        "edge_v_iso",
                                        "edge_h_sharp",
                                        "edge_h_outline",
                                        "edge_h_smooth",
                                        "edge_h_sewn",
                                        "edge_h_iso",
                                        "union_outline_polygons",
                                        "projection_algorithm",
                                        "mesh_linear_deflection",
                                        "mesh_angular_deflection",
                                        "mesh_relative",
                                        "mesh_deflection_mode",
                                        "mesh_deflection_coefficient",
                                        "outline_algorithm",
                                        "hlr_angle_tolerance",
                                        "fast"};
    if (!validate_object(value, names, 29U, path, error))
        return false;
    {
        const auto member = value.FindMember("views");
        if (member != value.MemberEnd())
        {
            std::vector<HlrViewSpec> decoded{};
            if (!decode_array(member->value, &decoded, child_path(path, "views"), error, 0U, 64U,
                              decode_HlrViewSpec))
                return false;
            out->views = std::move(decoded);
        }
        else
            out->views.reset();
    }
    {
        const auto member = value.FindMember("output_outline");
        if (member != value.MemberEnd())
        {
            bool decoded{};
            if (!decode_boolean(member->value, &decoded, child_path(path, "output_outline"), error))
                return false;
            out->output_outline = std::move(decoded);
        }
        else
            out->output_outline.reset();
    }
    {
        const auto member = value.FindMember("output_detail");
        if (member != value.MemberEnd())
        {
            bool decoded{};
            if (!decode_boolean(member->value, &decoded, child_path(path, "output_detail"), error))
                return false;
            out->output_detail = std::move(decoded);
        }
        else
            out->output_detail.reset();
    }
    {
        const auto member = value.FindMember("output_bbox");
        if (member != value.MemberEnd())
        {
            bool decoded{};
            if (!decode_boolean(member->value, &decoded, child_path(path, "output_bbox"), error))
                return false;
            out->output_bbox = std::move(decoded);
        }
        else
            out->output_bbox.reset();
    }
    {
        const auto member = value.FindMember("model_transform");
        if (member != value.MemberEnd())
        {
            HlrMatrix4x4 decoded{};
            if (!decode_HlrMatrix4x4(member->value, &decoded, child_path(path, "model_transform"),
                                     error))
                return false;
            out->model_transform = std::move(decoded);
        }
        else
            out->model_transform.reset();
    }
    {
        const auto member = value.FindMember("strip_root_placement");
        if (member != value.MemberEnd())
        {
            bool decoded{};
            if (!decode_boolean(member->value, &decoded, child_path(path, "strip_root_placement"),
                                error))
                return false;
            out->strip_root_placement = std::move(decoded);
        }
        else
            out->strip_root_placement.reset();
    }
    {
        const auto member = value.FindMember("curve_mode");
        if (member != value.MemberEnd())
        {
            HlrCurveMode decoded{};
            if (!decode_HlrCurveMode(member->value, &decoded, child_path(path, "curve_mode"),
                                     error))
                return false;
            out->curve_mode = std::move(decoded);
        }
        else
            out->curve_mode.reset();
    }
    {
        const auto member = value.FindMember("samples_per_curve");
        if (member != value.MemberEnd())
        {
            std::uint32_t decoded{};
            if (!decode_uint32(member->value, &decoded, child_path(path, "samples_per_curve"),
                               error, 1ULL, 100000ULL))
                return false;
            out->samples_per_curve = std::move(decoded);
        }
        else
            out->samples_per_curve.reset();
    }
    {
        const auto member = value.FindMember("round_digits");
        if (member != value.MemberEnd())
        {
            std::uint32_t decoded{};
            if (!decode_uint32(member->value, &decoded, child_path(path, "round_digits"), error,
                               0ULL, 9ULL))
                return false;
            out->round_digits = std::move(decoded);
        }
        else
            out->round_digits.reset();
    }
    {
        const auto member = value.FindMember("edge_v_sharp");
        if (member != value.MemberEnd())
        {
            bool decoded{};
            if (!decode_boolean(member->value, &decoded, child_path(path, "edge_v_sharp"), error))
                return false;
            out->edge_v_sharp = std::move(decoded);
        }
        else
            out->edge_v_sharp.reset();
    }
    {
        const auto member = value.FindMember("edge_v_outline");
        if (member != value.MemberEnd())
        {
            bool decoded{};
            if (!decode_boolean(member->value, &decoded, child_path(path, "edge_v_outline"), error))
                return false;
            out->edge_v_outline = std::move(decoded);
        }
        else
            out->edge_v_outline.reset();
    }
    {
        const auto member = value.FindMember("edge_v_smooth");
        if (member != value.MemberEnd())
        {
            bool decoded{};
            if (!decode_boolean(member->value, &decoded, child_path(path, "edge_v_smooth"), error))
                return false;
            out->edge_v_smooth = std::move(decoded);
        }
        else
            out->edge_v_smooth.reset();
    }
    {
        const auto member = value.FindMember("edge_v_sewn");
        if (member != value.MemberEnd())
        {
            bool decoded{};
            if (!decode_boolean(member->value, &decoded, child_path(path, "edge_v_sewn"), error))
                return false;
            out->edge_v_sewn = std::move(decoded);
        }
        else
            out->edge_v_sewn.reset();
    }
    {
        const auto member = value.FindMember("edge_v_iso");
        if (member != value.MemberEnd())
        {
            bool decoded{};
            if (!decode_boolean(member->value, &decoded, child_path(path, "edge_v_iso"), error))
                return false;
            out->edge_v_iso = std::move(decoded);
        }
        else
            out->edge_v_iso.reset();
    }
    {
        const auto member = value.FindMember("edge_h_sharp");
        if (member != value.MemberEnd())
        {
            bool decoded{};
            if (!decode_boolean(member->value, &decoded, child_path(path, "edge_h_sharp"), error))
                return false;
            out->edge_h_sharp = std::move(decoded);
        }
        else
            out->edge_h_sharp.reset();
    }
    {
        const auto member = value.FindMember("edge_h_outline");
        if (member != value.MemberEnd())
        {
            bool decoded{};
            if (!decode_boolean(member->value, &decoded, child_path(path, "edge_h_outline"), error))
                return false;
            out->edge_h_outline = std::move(decoded);
        }
        else
            out->edge_h_outline.reset();
    }
    {
        const auto member = value.FindMember("edge_h_smooth");
        if (member != value.MemberEnd())
        {
            bool decoded{};
            if (!decode_boolean(member->value, &decoded, child_path(path, "edge_h_smooth"), error))
                return false;
            out->edge_h_smooth = std::move(decoded);
        }
        else
            out->edge_h_smooth.reset();
    }
    {
        const auto member = value.FindMember("edge_h_sewn");
        if (member != value.MemberEnd())
        {
            bool decoded{};
            if (!decode_boolean(member->value, &decoded, child_path(path, "edge_h_sewn"), error))
                return false;
            out->edge_h_sewn = std::move(decoded);
        }
        else
            out->edge_h_sewn.reset();
    }
    {
        const auto member = value.FindMember("edge_h_iso");
        if (member != value.MemberEnd())
        {
            bool decoded{};
            if (!decode_boolean(member->value, &decoded, child_path(path, "edge_h_iso"), error))
                return false;
            out->edge_h_iso = std::move(decoded);
        }
        else
            out->edge_h_iso.reset();
    }
    {
        const auto member = value.FindMember("union_outline_polygons");
        if (member != value.MemberEnd())
        {
            bool decoded{};
            if (!decode_boolean(member->value, &decoded, child_path(path, "union_outline_polygons"),
                                error))
                return false;
            out->union_outline_polygons = std::move(decoded);
        }
        else
            out->union_outline_polygons.reset();
    }
    {
        const auto member = value.FindMember("projection_algorithm");
        if (member != value.MemberEnd())
        {
            HlrProjectionAlgorithm decoded{};
            if (!decode_HlrProjectionAlgorithm(member->value, &decoded,
                                               child_path(path, "projection_algorithm"), error))
                return false;
            out->projection_algorithm = std::move(decoded);
        }
        else
            out->projection_algorithm.reset();
    }
    {
        const auto member = value.FindMember("mesh_linear_deflection");
        if (member != value.MemberEnd())
        {
            double decoded{};
            if (!decode_double(member->value, &decoded, child_path(path, "mesh_linear_deflection"),
                               error, 0, std::numeric_limits<double>::infinity(), false, false))
                return false;
            out->mesh_linear_deflection = std::move(decoded);
        }
        else
            out->mesh_linear_deflection.reset();
    }
    {
        const auto member = value.FindMember("mesh_angular_deflection");
        if (member != value.MemberEnd())
        {
            double decoded{};
            if (!decode_double(member->value, &decoded, child_path(path, "mesh_angular_deflection"),
                               error, 0, 3.141592653589793, false, false))
                return false;
            out->mesh_angular_deflection = std::move(decoded);
        }
        else
            out->mesh_angular_deflection.reset();
    }
    {
        const auto member = value.FindMember("mesh_relative");
        if (member != value.MemberEnd())
        {
            bool decoded{};
            if (!decode_boolean(member->value, &decoded, child_path(path, "mesh_relative"), error))
                return false;
            out->mesh_relative = std::move(decoded);
        }
        else
            out->mesh_relative.reset();
    }
    {
        const auto member = value.FindMember("mesh_deflection_mode");
        if (member != value.MemberEnd())
        {
            HlrMeshDeflectionMode decoded{};
            if (!decode_HlrMeshDeflectionMode(member->value, &decoded,
                                              child_path(path, "mesh_deflection_mode"), error))
                return false;
            out->mesh_deflection_mode = std::move(decoded);
        }
        else
            out->mesh_deflection_mode.reset();
    }
    {
        const auto member = value.FindMember("mesh_deflection_coefficient");
        if (member != value.MemberEnd())
        {
            double decoded{};
            if (!decode_double(member->value, &decoded,
                               child_path(path, "mesh_deflection_coefficient"), error, 0,
                               std::numeric_limits<double>::infinity(), false, false))
                return false;
            out->mesh_deflection_coefficient = std::move(decoded);
        }
        else
            out->mesh_deflection_coefficient.reset();
    }
    {
        const auto member = value.FindMember("outline_algorithm");
        if (member != value.MemberEnd())
        {
            HlrOutlineAlgorithm decoded{};
            if (!decode_HlrOutlineAlgorithm(member->value, &decoded,
                                            child_path(path, "outline_algorithm"), error))
                return false;
            out->outline_algorithm = std::move(decoded);
        }
        else
            out->outline_algorithm.reset();
    }
    {
        const auto member = value.FindMember("hlr_angle_tolerance");
        if (member != value.MemberEnd())
        {
            double decoded{};
            if (!decode_double(member->value, &decoded, child_path(path, "hlr_angle_tolerance"),
                               error, 0, 3.141592653589793, false, false))
                return false;
            out->hlr_angle_tolerance = std::move(decoded);
        }
        else
            out->hlr_angle_tolerance.reset();
    }
    {
        const auto member = value.FindMember("fast");
        if (member != value.MemberEnd())
        {
            FastHlrOptionsA0 decoded{};
            if (!decode_FastHlrOptionsA0(member->value, &decoded, child_path(path, "fast"), error))
                return false;
            out->fast = std::move(decoded);
        }
        else
            out->fast.reset();
    }
    return true;
}

bool write_HlrProjectionOptionsA0(rapidjson::Writer<rapidjson::StringBuffer>& writer,
                                  const HlrProjectionOptionsA0& value, ContractError* error)
{
    writer.StartObject();
    if (value.views.has_value())
    {
        writer.Key("views");
        if (!write_array(writer, *value.views, error, 0U, 64U, write_HlrViewSpec))
            return false;
    }
    if (value.output_outline.has_value())
    {
        writer.Key("output_outline");
        if (!(writer.Bool(*value.output_outline), true))
            return false;
    }
    if (value.output_detail.has_value())
    {
        writer.Key("output_detail");
        if (!(writer.Bool(*value.output_detail), true))
            return false;
    }
    if (value.output_bbox.has_value())
    {
        writer.Key("output_bbox");
        if (!(writer.Bool(*value.output_bbox), true))
            return false;
    }
    if (value.model_transform.has_value())
    {
        writer.Key("model_transform");
        if (!write_HlrMatrix4x4(writer, *value.model_transform, error))
            return false;
    }
    if (value.strip_root_placement.has_value())
    {
        writer.Key("strip_root_placement");
        if (!(writer.Bool(*value.strip_root_placement), true))
            return false;
    }
    if (value.curve_mode.has_value())
    {
        writer.Key("curve_mode");
        if (!write_HlrCurveMode(writer, *value.curve_mode, error))
            return false;
    }
    if (value.samples_per_curve.has_value())
    {
        writer.Key("samples_per_curve");
        if (!write_uint32(writer, *value.samples_per_curve, error, 1ULL, 100000ULL))
            return false;
    }
    if (value.round_digits.has_value())
    {
        writer.Key("round_digits");
        if (!write_uint32(writer, *value.round_digits, error, 0ULL, 9ULL))
            return false;
    }
    if (value.edge_v_sharp.has_value())
    {
        writer.Key("edge_v_sharp");
        if (!(writer.Bool(*value.edge_v_sharp), true))
            return false;
    }
    if (value.edge_v_outline.has_value())
    {
        writer.Key("edge_v_outline");
        if (!(writer.Bool(*value.edge_v_outline), true))
            return false;
    }
    if (value.edge_v_smooth.has_value())
    {
        writer.Key("edge_v_smooth");
        if (!(writer.Bool(*value.edge_v_smooth), true))
            return false;
    }
    if (value.edge_v_sewn.has_value())
    {
        writer.Key("edge_v_sewn");
        if (!(writer.Bool(*value.edge_v_sewn), true))
            return false;
    }
    if (value.edge_v_iso.has_value())
    {
        writer.Key("edge_v_iso");
        if (!(writer.Bool(*value.edge_v_iso), true))
            return false;
    }
    if (value.edge_h_sharp.has_value())
    {
        writer.Key("edge_h_sharp");
        if (!(writer.Bool(*value.edge_h_sharp), true))
            return false;
    }
    if (value.edge_h_outline.has_value())
    {
        writer.Key("edge_h_outline");
        if (!(writer.Bool(*value.edge_h_outline), true))
            return false;
    }
    if (value.edge_h_smooth.has_value())
    {
        writer.Key("edge_h_smooth");
        if (!(writer.Bool(*value.edge_h_smooth), true))
            return false;
    }
    if (value.edge_h_sewn.has_value())
    {
        writer.Key("edge_h_sewn");
        if (!(writer.Bool(*value.edge_h_sewn), true))
            return false;
    }
    if (value.edge_h_iso.has_value())
    {
        writer.Key("edge_h_iso");
        if (!(writer.Bool(*value.edge_h_iso), true))
            return false;
    }
    if (value.union_outline_polygons.has_value())
    {
        writer.Key("union_outline_polygons");
        if (!(writer.Bool(*value.union_outline_polygons), true))
            return false;
    }
    if (value.projection_algorithm.has_value())
    {
        writer.Key("projection_algorithm");
        if (!write_HlrProjectionAlgorithm(writer, *value.projection_algorithm, error))
            return false;
    }
    if (value.mesh_linear_deflection.has_value())
    {
        writer.Key("mesh_linear_deflection");
        if (!write_double(writer, *value.mesh_linear_deflection, error, 0,
                          std::numeric_limits<double>::infinity(), false, false))
            return false;
    }
    if (value.mesh_angular_deflection.has_value())
    {
        writer.Key("mesh_angular_deflection");
        if (!write_double(writer, *value.mesh_angular_deflection, error, 0, 3.141592653589793,
                          false, false))
            return false;
    }
    if (value.mesh_relative.has_value())
    {
        writer.Key("mesh_relative");
        if (!(writer.Bool(*value.mesh_relative), true))
            return false;
    }
    if (value.mesh_deflection_mode.has_value())
    {
        writer.Key("mesh_deflection_mode");
        if (!write_HlrMeshDeflectionMode(writer, *value.mesh_deflection_mode, error))
            return false;
    }
    if (value.mesh_deflection_coefficient.has_value())
    {
        writer.Key("mesh_deflection_coefficient");
        if (!write_double(writer, *value.mesh_deflection_coefficient, error, 0,
                          std::numeric_limits<double>::infinity(), false, false))
            return false;
    }
    if (value.outline_algorithm.has_value())
    {
        writer.Key("outline_algorithm");
        if (!write_HlrOutlineAlgorithm(writer, *value.outline_algorithm, error))
            return false;
    }
    if (value.hlr_angle_tolerance.has_value())
    {
        writer.Key("hlr_angle_tolerance");
        if (!write_double(writer, *value.hlr_angle_tolerance, error, 0, 3.141592653589793, false,
                          false))
            return false;
    }
    if (value.fast.has_value())
    {
        writer.Key("fast");
        if (!write_FastHlrOptionsA0(writer, *value.fast, error))
            return false;
    }
    writer.EndObject();
    return true;
}

bool decode_HlrSourceKind(const rapidjson::Value& value, HlrSourceKind* out,
                          const std::string& path, ContractError* error)
{
    if (!value.IsString())
        return fail(error, "geometer.contract.type_mismatch", path, "Expected a string enum.");
    const std::string text(value.GetString(), value.GetStringLength());
    if (text == "step")
    {
        *out = HlrSourceKind::step;
        return true;
    }
    if (text == "indexed_mesh")
    {
        *out = HlrSourceKind::indexed_mesh;
        return true;
    }
    return fail(error, "geometer.contract.enum_mismatch", path, "Unknown enum value.");
}

bool write_HlrSourceKind(rapidjson::Writer<rapidjson::StringBuffer>& writer,
                         const HlrSourceKind& value, ContractError* error)
{
    switch (value)
    {
    case HlrSourceKind::step:
        writer.String("step");
        return true;
    case HlrSourceKind::indexed_mesh:
        writer.String("indexed_mesh");
        return true;
    }
    return fail(error, "geometer.contract.enum_mismatch", "", "Unknown enum value.");
}

bool decode_HlrProjectionSource(const rapidjson::Value& value, HlrProjectionSource* out,
                                const std::string& path, ContractError* error)
{
    static const char* const names[] = {"kind", "hash"};
    if (!validate_object(value, names, 2U, path, error))
        return false;
    {
        const auto member = value.FindMember("kind");
        if (member == value.MemberEnd())
            return fail(error, "geometer.contract.missing_field", child_path(path, "kind"),
                        "Required field is missing.");
        if (!decode_HlrSourceKind(member->value, &out->kind, child_path(path, "kind"), error))
            return false;
    }
    {
        const auto member = value.FindMember("hash");
        if (member == value.MemberEnd())
            return fail(error, "geometer.contract.missing_field", child_path(path, "hash"),
                        "Required field is missing.");
        if (!decode_string(member->value, &out->hash, child_path(path, "hash"), error, 64U, 64U))
            return false;
    }
    return true;
}

bool write_HlrProjectionSource(rapidjson::Writer<rapidjson::StringBuffer>& writer,
                               const HlrProjectionSource& value, ContractError* error)
{
    writer.StartObject();
    writer.Key("kind");
    if (!write_HlrSourceKind(writer, value.kind, error))
        return false;
    writer.Key("hash");
    if (!write_string(writer, value.hash, error, 64U, 64U))
        return false;
    writer.EndObject();
    return true;
}

bool decode_HlrProjectionTimings(const rapidjson::Value& value, HlrProjectionTimings* out,
                                 const std::string& path, ContractError* error)
{
    static const char* const names[] = {"step_read_ms", "mesh_ms", "hlr_ms", "extract_ms"};
    if (!validate_object(value, names, 4U, path, error))
        return false;
    {
        const auto member = value.FindMember("step_read_ms");
        if (member == value.MemberEnd())
            return fail(error, "geometer.contract.missing_field", child_path(path, "step_read_ms"),
                        "Required field is missing.");
        if (!decode_double(member->value, &out->step_read_ms, child_path(path, "step_read_ms"),
                           error, 0, std::numeric_limits<double>::infinity(), false, false))
            return false;
    }
    {
        const auto member = value.FindMember("mesh_ms");
        if (member == value.MemberEnd())
            return fail(error, "geometer.contract.missing_field", child_path(path, "mesh_ms"),
                        "Required field is missing.");
        if (!decode_double(member->value, &out->mesh_ms, child_path(path, "mesh_ms"), error, 0,
                           std::numeric_limits<double>::infinity(), false, false))
            return false;
    }
    {
        const auto member = value.FindMember("hlr_ms");
        if (member == value.MemberEnd())
            return fail(error, "geometer.contract.missing_field", child_path(path, "hlr_ms"),
                        "Required field is missing.");
        if (!decode_double(member->value, &out->hlr_ms, child_path(path, "hlr_ms"), error, 0,
                           std::numeric_limits<double>::infinity(), false, false))
            return false;
    }
    {
        const auto member = value.FindMember("extract_ms");
        if (member == value.MemberEnd())
            return fail(error, "geometer.contract.missing_field", child_path(path, "extract_ms"),
                        "Required field is missing.");
        if (!decode_double(member->value, &out->extract_ms, child_path(path, "extract_ms"), error,
                           0, std::numeric_limits<double>::infinity(), false, false))
            return false;
    }
    return true;
}

bool write_HlrProjectionTimings(rapidjson::Writer<rapidjson::StringBuffer>& writer,
                                const HlrProjectionTimings& value, ContractError* error)
{
    writer.StartObject();
    writer.Key("step_read_ms");
    if (!write_double(writer, value.step_read_ms, error, 0, std::numeric_limits<double>::infinity(),
                      false, false))
        return false;
    writer.Key("mesh_ms");
    if (!write_double(writer, value.mesh_ms, error, 0, std::numeric_limits<double>::infinity(),
                      false, false))
        return false;
    writer.Key("hlr_ms");
    if (!write_double(writer, value.hlr_ms, error, 0, std::numeric_limits<double>::infinity(),
                      false, false))
        return false;
    writer.Key("extract_ms");
    if (!write_double(writer, value.extract_ms, error, 0, std::numeric_limits<double>::infinity(),
                      false, false))
        return false;
    writer.EndObject();
    return true;
}

bool decode_HlrProjectionResultA0(const rapidjson::Value& value, HlrProjectionResultA0* out,
                                  const std::string& path, ContractError* error)
{
    static const char* const names[] = {"schema", "units", "source", "views", "timings"};
    if (!validate_object(value, names, 5U, path, error))
        return false;
    {
        const auto member = value.FindMember("schema");
        if (member == value.MemberEnd())
            return fail(error, "geometer.contract.missing_field", child_path(path, "schema"),
                        "Required field is missing.");
        if (!decode_literal_string(member->value, &out->schema, child_path(path, "schema"), error,
                                   "geometry.hlr_projection.result.a0"))
            return false;
    }
    {
        const auto member = value.FindMember("units");
        if (member == value.MemberEnd())
            return fail(error, "geometer.contract.missing_field", child_path(path, "units"),
                        "Required field is missing.");
        if (!decode_literal_string(member->value, &out->units, child_path(path, "units"), error,
                                   "mm"))
            return false;
    }
    {
        const auto member = value.FindMember("source");
        if (member == value.MemberEnd())
            return fail(error, "geometer.contract.missing_field", child_path(path, "source"),
                        "Required field is missing.");
        if (!decode_HlrProjectionSource(member->value, &out->source, child_path(path, "source"),
                                        error))
            return false;
    }
    {
        const auto member = value.FindMember("views");
        if (member == value.MemberEnd())
            return fail(error, "geometer.contract.missing_field", child_path(path, "views"),
                        "Required field is missing.");
        if (!decode_array(member->value, &out->views, child_path(path, "views"), error, 0U, 64U,
                          decode_HlrProjectedView))
            return false;
    }
    {
        const auto member = value.FindMember("timings");
        if (member == value.MemberEnd())
            return fail(error, "geometer.contract.missing_field", child_path(path, "timings"),
                        "Required field is missing.");
        if (!decode_HlrProjectionTimings(member->value, &out->timings, child_path(path, "timings"),
                                         error))
            return false;
    }
    return true;
}

bool write_HlrProjectionResultA0(rapidjson::Writer<rapidjson::StringBuffer>& writer,
                                 const HlrProjectionResultA0& value, ContractError* error)
{
    writer.StartObject();
    writer.Key("schema");
    if (!write_literal_string(writer, value.schema, error, "geometry.hlr_projection.result.a0"))
        return false;
    writer.Key("units");
    if (!write_literal_string(writer, value.units, error, "mm"))
        return false;
    writer.Key("source");
    if (!write_HlrProjectionSource(writer, value.source, error))
        return false;
    writer.Key("views");
    if (!write_array(writer, value.views, error, 0U, 64U, write_HlrProjectedView))
        return false;
    writer.Key("timings");
    if (!write_HlrProjectionTimings(writer, value.timings, error))
        return false;
    writer.EndObject();
    return true;
}

bool decode_IpcAttachmentDeclarationA0(const rapidjson::Value& value,
                                       IpcAttachmentDeclarationA0* out, const std::string& path,
                                       ContractError* error)
{
    static const char* const names[] = {"name", "required", "media_types", "max_bytes"};
    if (!validate_object(value, names, 4U, path, error))
        return false;
    {
        const auto member = value.FindMember("name");
        if (member == value.MemberEnd())
            return fail(error, "geometer.contract.missing_field", child_path(path, "name"),
                        "Required field is missing.");
        if (!decode_string(member->value, &out->name, child_path(path, "name"), error, 1U, 128U))
            return false;
    }
    {
        const auto member = value.FindMember("required");
        if (member == value.MemberEnd())
            return fail(error, "geometer.contract.missing_field", child_path(path, "required"),
                        "Required field is missing.");
        if (!decode_boolean(member->value, &out->required, child_path(path, "required"), error))
            return false;
    }
    {
        const auto member = value.FindMember("media_types");
        if (member == value.MemberEnd())
            return fail(error, "geometer.contract.missing_field", child_path(path, "media_types"),
                        "Required field is missing.");
        if (!decode_array(member->value, &out->media_types, child_path(path, "media_types"), error,
                          1U, 16U, decode_string_item))
            return false;
    }
    {
        const auto member = value.FindMember("max_bytes");
        if (member == value.MemberEnd())
            return fail(error, "geometer.contract.missing_field", child_path(path, "max_bytes"),
                        "Required field is missing.");
        if (!decode_uint32(member->value, &out->max_bytes, child_path(path, "max_bytes"), error,
                           0ULL, 268435456ULL))
            return false;
    }
    return true;
}

bool write_IpcAttachmentDeclarationA0(rapidjson::Writer<rapidjson::StringBuffer>& writer,
                                      const IpcAttachmentDeclarationA0& value, ContractError* error)
{
    writer.StartObject();
    writer.Key("name");
    if (!write_string(writer, value.name, error, 1U, 128U))
        return false;
    writer.Key("required");
    if (!(writer.Bool(value.required), true))
        return false;
    writer.Key("media_types");
    if (!write_array(writer, value.media_types, error, 1U, 16U, write_string_item))
        return false;
    writer.Key("max_bytes");
    if (!write_uint32(writer, value.max_bytes, error, 0ULL, 268435456ULL))
        return false;
    writer.EndObject();
    return true;
}

bool decode_IpcAttachmentOffsetsWasm32A0(const rapidjson::Value& value,
                                         IpcAttachmentOffsetsWasm32A0* out, const std::string& path,
                                         ContractError* error)
{
    static const char* const names[] = {"struct_size", "flags",      "name",
                                        "name_size",   "media_type", "media_type_size",
                                        "data",        "data_size",  "reserved0"};
    if (!validate_object(value, names, 9U, path, error))
        return false;
    {
        const auto member = value.FindMember("struct_size");
        if (member == value.MemberEnd())
            return fail(error, "geometer.contract.missing_field", child_path(path, "struct_size"),
                        "Required field is missing.");
        if (!decode_uint32(member->value, &out->struct_size, child_path(path, "struct_size"), error,
                           0ULL, 0ULL))
            return false;
    }
    {
        const auto member = value.FindMember("flags");
        if (member == value.MemberEnd())
            return fail(error, "geometer.contract.missing_field", child_path(path, "flags"),
                        "Required field is missing.");
        if (!decode_uint32(member->value, &out->flags, child_path(path, "flags"), error, 4ULL,
                           4ULL))
            return false;
    }
    {
        const auto member = value.FindMember("name");
        if (member == value.MemberEnd())
            return fail(error, "geometer.contract.missing_field", child_path(path, "name"),
                        "Required field is missing.");
        if (!decode_uint32(member->value, &out->name, child_path(path, "name"), error, 8ULL, 8ULL))
            return false;
    }
    {
        const auto member = value.FindMember("name_size");
        if (member == value.MemberEnd())
            return fail(error, "geometer.contract.missing_field", child_path(path, "name_size"),
                        "Required field is missing.");
        if (!decode_uint32(member->value, &out->name_size, child_path(path, "name_size"), error,
                           12ULL, 12ULL))
            return false;
    }
    {
        const auto member = value.FindMember("media_type");
        if (member == value.MemberEnd())
            return fail(error, "geometer.contract.missing_field", child_path(path, "media_type"),
                        "Required field is missing.");
        if (!decode_uint32(member->value, &out->media_type, child_path(path, "media_type"), error,
                           16ULL, 16ULL))
            return false;
    }
    {
        const auto member = value.FindMember("media_type_size");
        if (member == value.MemberEnd())
            return fail(error, "geometer.contract.missing_field",
                        child_path(path, "media_type_size"), "Required field is missing.");
        if (!decode_uint32(member->value, &out->media_type_size,
                           child_path(path, "media_type_size"), error, 20ULL, 20ULL))
            return false;
    }
    {
        const auto member = value.FindMember("data");
        if (member == value.MemberEnd())
            return fail(error, "geometer.contract.missing_field", child_path(path, "data"),
                        "Required field is missing.");
        if (!decode_uint32(member->value, &out->data, child_path(path, "data"), error, 24ULL,
                           24ULL))
            return false;
    }
    {
        const auto member = value.FindMember("data_size");
        if (member == value.MemberEnd())
            return fail(error, "geometer.contract.missing_field", child_path(path, "data_size"),
                        "Required field is missing.");
        if (!decode_uint32(member->value, &out->data_size, child_path(path, "data_size"), error,
                           28ULL, 28ULL))
            return false;
    }
    {
        const auto member = value.FindMember("reserved0");
        if (member == value.MemberEnd())
            return fail(error, "geometer.contract.missing_field", child_path(path, "reserved0"),
                        "Required field is missing.");
        if (!decode_uint32(member->value, &out->reserved0, child_path(path, "reserved0"), error,
                           32ULL, 32ULL))
            return false;
    }
    return true;
}

bool write_IpcAttachmentOffsetsWasm32A0(rapidjson::Writer<rapidjson::StringBuffer>& writer,
                                        const IpcAttachmentOffsetsWasm32A0& value,
                                        ContractError* error)
{
    writer.StartObject();
    writer.Key("struct_size");
    if (!write_uint32(writer, value.struct_size, error, 0ULL, 0ULL))
        return false;
    writer.Key("flags");
    if (!write_uint32(writer, value.flags, error, 4ULL, 4ULL))
        return false;
    writer.Key("name");
    if (!write_uint32(writer, value.name, error, 8ULL, 8ULL))
        return false;
    writer.Key("name_size");
    if (!write_uint32(writer, value.name_size, error, 12ULL, 12ULL))
        return false;
    writer.Key("media_type");
    if (!write_uint32(writer, value.media_type, error, 16ULL, 16ULL))
        return false;
    writer.Key("media_type_size");
    if (!write_uint32(writer, value.media_type_size, error, 20ULL, 20ULL))
        return false;
    writer.Key("data");
    if (!write_uint32(writer, value.data, error, 24ULL, 24ULL))
        return false;
    writer.Key("data_size");
    if (!write_uint32(writer, value.data_size, error, 28ULL, 28ULL))
        return false;
    writer.Key("reserved0");
    if (!write_uint32(writer, value.reserved0, error, 32ULL, 32ULL))
        return false;
    writer.EndObject();
    return true;
}

bool decode_IpcAttachmentLayoutWasm32A0(const rapidjson::Value& value,
                                        IpcAttachmentLayoutWasm32A0* out, const std::string& path,
                                        ContractError* error)
{
    static const char* const names[] = {"size", "offsets"};
    if (!validate_object(value, names, 2U, path, error))
        return false;
    {
        const auto member = value.FindMember("size");
        if (member == value.MemberEnd())
            return fail(error, "geometer.contract.missing_field", child_path(path, "size"),
                        "Required field is missing.");
        if (!decode_uint32(member->value, &out->size, child_path(path, "size"), error, 36ULL,
                           36ULL))
            return false;
    }
    {
        const auto member = value.FindMember("offsets");
        if (member == value.MemberEnd())
            return fail(error, "geometer.contract.missing_field", child_path(path, "offsets"),
                        "Required field is missing.");
        if (!decode_IpcAttachmentOffsetsWasm32A0(member->value, &out->offsets,
                                                 child_path(path, "offsets"), error))
            return false;
    }
    return true;
}

bool write_IpcAttachmentLayoutWasm32A0(rapidjson::Writer<rapidjson::StringBuffer>& writer,
                                       const IpcAttachmentLayoutWasm32A0& value,
                                       ContractError* error)
{
    writer.StartObject();
    writer.Key("size");
    if (!write_uint32(writer, value.size, error, 36ULL, 36ULL))
        return false;
    writer.Key("offsets");
    if (!write_IpcAttachmentOffsetsWasm32A0(writer, value.offsets, error))
        return false;
    writer.EndObject();
    return true;
}

bool decode_IpcAttachmentOffsetsPointer64A0(const rapidjson::Value& value,
                                            IpcAttachmentOffsetsPointer64A0* out,
                                            const std::string& path, ContractError* error)
{
    static const char* const names[] = {"struct_size", "flags",      "name",
                                        "name_size",   "media_type", "media_type_size",
                                        "data",        "data_size",  "reserved0"};
    if (!validate_object(value, names, 9U, path, error))
        return false;
    {
        const auto member = value.FindMember("struct_size");
        if (member == value.MemberEnd())
            return fail(error, "geometer.contract.missing_field", child_path(path, "struct_size"),
                        "Required field is missing.");
        if (!decode_uint32(member->value, &out->struct_size, child_path(path, "struct_size"), error,
                           0ULL, 0ULL))
            return false;
    }
    {
        const auto member = value.FindMember("flags");
        if (member == value.MemberEnd())
            return fail(error, "geometer.contract.missing_field", child_path(path, "flags"),
                        "Required field is missing.");
        if (!decode_uint32(member->value, &out->flags, child_path(path, "flags"), error, 4ULL,
                           4ULL))
            return false;
    }
    {
        const auto member = value.FindMember("name");
        if (member == value.MemberEnd())
            return fail(error, "geometer.contract.missing_field", child_path(path, "name"),
                        "Required field is missing.");
        if (!decode_uint32(member->value, &out->name, child_path(path, "name"), error, 8ULL, 8ULL))
            return false;
    }
    {
        const auto member = value.FindMember("name_size");
        if (member == value.MemberEnd())
            return fail(error, "geometer.contract.missing_field", child_path(path, "name_size"),
                        "Required field is missing.");
        if (!decode_uint32(member->value, &out->name_size, child_path(path, "name_size"), error,
                           16ULL, 16ULL))
            return false;
    }
    {
        const auto member = value.FindMember("media_type");
        if (member == value.MemberEnd())
            return fail(error, "geometer.contract.missing_field", child_path(path, "media_type"),
                        "Required field is missing.");
        if (!decode_uint32(member->value, &out->media_type, child_path(path, "media_type"), error,
                           24ULL, 24ULL))
            return false;
    }
    {
        const auto member = value.FindMember("media_type_size");
        if (member == value.MemberEnd())
            return fail(error, "geometer.contract.missing_field",
                        child_path(path, "media_type_size"), "Required field is missing.");
        if (!decode_uint32(member->value, &out->media_type_size,
                           child_path(path, "media_type_size"), error, 32ULL, 32ULL))
            return false;
    }
    {
        const auto member = value.FindMember("data");
        if (member == value.MemberEnd())
            return fail(error, "geometer.contract.missing_field", child_path(path, "data"),
                        "Required field is missing.");
        if (!decode_uint32(member->value, &out->data, child_path(path, "data"), error, 40ULL,
                           40ULL))
            return false;
    }
    {
        const auto member = value.FindMember("data_size");
        if (member == value.MemberEnd())
            return fail(error, "geometer.contract.missing_field", child_path(path, "data_size"),
                        "Required field is missing.");
        if (!decode_uint32(member->value, &out->data_size, child_path(path, "data_size"), error,
                           48ULL, 48ULL))
            return false;
    }
    {
        const auto member = value.FindMember("reserved0");
        if (member == value.MemberEnd())
            return fail(error, "geometer.contract.missing_field", child_path(path, "reserved0"),
                        "Required field is missing.");
        if (!decode_uint32(member->value, &out->reserved0, child_path(path, "reserved0"), error,
                           52ULL, 52ULL))
            return false;
    }
    return true;
}

bool write_IpcAttachmentOffsetsPointer64A0(rapidjson::Writer<rapidjson::StringBuffer>& writer,
                                           const IpcAttachmentOffsetsPointer64A0& value,
                                           ContractError* error)
{
    writer.StartObject();
    writer.Key("struct_size");
    if (!write_uint32(writer, value.struct_size, error, 0ULL, 0ULL))
        return false;
    writer.Key("flags");
    if (!write_uint32(writer, value.flags, error, 4ULL, 4ULL))
        return false;
    writer.Key("name");
    if (!write_uint32(writer, value.name, error, 8ULL, 8ULL))
        return false;
    writer.Key("name_size");
    if (!write_uint32(writer, value.name_size, error, 16ULL, 16ULL))
        return false;
    writer.Key("media_type");
    if (!write_uint32(writer, value.media_type, error, 24ULL, 24ULL))
        return false;
    writer.Key("media_type_size");
    if (!write_uint32(writer, value.media_type_size, error, 32ULL, 32ULL))
        return false;
    writer.Key("data");
    if (!write_uint32(writer, value.data, error, 40ULL, 40ULL))
        return false;
    writer.Key("data_size");
    if (!write_uint32(writer, value.data_size, error, 48ULL, 48ULL))
        return false;
    writer.Key("reserved0");
    if (!write_uint32(writer, value.reserved0, error, 52ULL, 52ULL))
        return false;
    writer.EndObject();
    return true;
}

bool decode_IpcAttachmentLayoutPointer64A0(const rapidjson::Value& value,
                                           IpcAttachmentLayoutPointer64A0* out,
                                           const std::string& path, ContractError* error)
{
    static const char* const names[] = {"size", "offsets"};
    if (!validate_object(value, names, 2U, path, error))
        return false;
    {
        const auto member = value.FindMember("size");
        if (member == value.MemberEnd())
            return fail(error, "geometer.contract.missing_field", child_path(path, "size"),
                        "Required field is missing.");
        if (!decode_uint32(member->value, &out->size, child_path(path, "size"), error, 56ULL,
                           56ULL))
            return false;
    }
    {
        const auto member = value.FindMember("offsets");
        if (member == value.MemberEnd())
            return fail(error, "geometer.contract.missing_field", child_path(path, "offsets"),
                        "Required field is missing.");
        if (!decode_IpcAttachmentOffsetsPointer64A0(member->value, &out->offsets,
                                                    child_path(path, "offsets"), error))
            return false;
    }
    return true;
}

bool write_IpcAttachmentLayoutPointer64A0(rapidjson::Writer<rapidjson::StringBuffer>& writer,
                                          const IpcAttachmentLayoutPointer64A0& value,
                                          ContractError* error)
{
    writer.StartObject();
    writer.Key("size");
    if (!write_uint32(writer, value.size, error, 56ULL, 56ULL))
        return false;
    writer.Key("offsets");
    if (!write_IpcAttachmentOffsetsPointer64A0(writer, value.offsets, error))
        return false;
    writer.EndObject();
    return true;
}

bool decode_IpcAttachmentDescriptorA0(const rapidjson::Value& value, IpcAttachmentDescriptorA0* out,
                                      const std::string& path, ContractError* error)
{
    static const char* const names[] = {"wasm32", "pointer64"};
    if (!validate_object(value, names, 2U, path, error))
        return false;
    {
        const auto member = value.FindMember("wasm32");
        if (member == value.MemberEnd())
            return fail(error, "geometer.contract.missing_field", child_path(path, "wasm32"),
                        "Required field is missing.");
        if (!decode_IpcAttachmentLayoutWasm32A0(member->value, &out->wasm32,
                                                child_path(path, "wasm32"), error))
            return false;
    }
    {
        const auto member = value.FindMember("pointer64");
        if (member == value.MemberEnd())
            return fail(error, "geometer.contract.missing_field", child_path(path, "pointer64"),
                        "Required field is missing.");
        if (!decode_IpcAttachmentLayoutPointer64A0(member->value, &out->pointer64,
                                                   child_path(path, "pointer64"), error))
            return false;
    }
    return true;
}

bool write_IpcAttachmentDescriptorA0(rapidjson::Writer<rapidjson::StringBuffer>& writer,
                                     const IpcAttachmentDescriptorA0& value, ContractError* error)
{
    writer.StartObject();
    writer.Key("wasm32");
    if (!write_IpcAttachmentLayoutWasm32A0(writer, value.wasm32, error))
        return false;
    writer.Key("pointer64");
    if (!write_IpcAttachmentLayoutPointer64A0(writer, value.pointer64, error))
        return false;
    writer.EndObject();
    return true;
}

bool decode_IpcCancelledA0(const rapidjson::Value& value, IpcCancelledA0* out,
                           const std::string& path, ContractError* error)
{
    static const char* const names[] = {"status"};
    if (!validate_object(value, names, 1U, path, error))
        return false;
    {
        const auto member = value.FindMember("status");
        if (member == value.MemberEnd())
            return fail(error, "geometer.contract.missing_field", child_path(path, "status"),
                        "Required field is missing.");
        if (!decode_literal_string(member->value, &out->status, child_path(path, "status"), error,
                                   "cancelled"))
            return false;
    }
    return true;
}

bool write_IpcCancelledA0(rapidjson::Writer<rapidjson::StringBuffer>& writer,
                          const IpcCancelledA0& value, ContractError* error)
{
    writer.StartObject();
    writer.Key("status");
    if (!write_literal_string(writer, value.status, error, "cancelled"))
        return false;
    writer.EndObject();
    return true;
}

bool decode_IpcCancelRejectedA0(const rapidjson::Value& value, IpcCancelRejectedA0* out,
                                const std::string& path, ContractError* error)
{
    static const char* const names[] = {"status", "diagnostic"};
    if (!validate_object(value, names, 2U, path, error))
        return false;
    {
        const auto member = value.FindMember("status");
        if (member == value.MemberEnd())
            return fail(error, "geometer.contract.missing_field", child_path(path, "status"),
                        "Required field is missing.");
        if (!decode_literal_string(member->value, &out->status, child_path(path, "status"), error,
                                   "rejected"))
            return false;
    }
    {
        const auto member = value.FindMember("diagnostic");
        if (member == value.MemberEnd())
            return fail(error, "geometer.contract.missing_field", child_path(path, "diagnostic"),
                        "Required field is missing.");
        if (!decode_DiagnosticA0(member->value, &out->diagnostic, child_path(path, "diagnostic"),
                                 error))
            return false;
    }
    return true;
}

bool write_IpcCancelRejectedA0(rapidjson::Writer<rapidjson::StringBuffer>& writer,
                               const IpcCancelRejectedA0& value, ContractError* error)
{
    writer.StartObject();
    writer.Key("status");
    if (!write_literal_string(writer, value.status, error, "rejected"))
        return false;
    writer.Key("diagnostic");
    if (!write_DiagnosticA0(writer, value.diagnostic, error))
        return false;
    writer.EndObject();
    return true;
}

bool decode_IpcEffectiveLimitsA0(const rapidjson::Value& value, IpcEffectiveLimitsA0* out,
                                 const std::string& path, ContractError* error)
{
    static const char* const names[] = {"json_bytes",
                                        "attachment_count",
                                        "attachment_name_bytes",
                                        "attachment_media_type_bytes",
                                        "attachment_bytes",
                                        "frame_bytes",
                                        "queued_requests",
                                        "queued_bytes",
                                        "resident_request_bytes",
                                        "pending_writer_bytes"};
    if (!validate_object(value, names, 10U, path, error))
        return false;
    {
        const auto member = value.FindMember("json_bytes");
        if (member == value.MemberEnd())
            return fail(error, "geometer.contract.missing_field", child_path(path, "json_bytes"),
                        "Required field is missing.");
        if (!decode_uint32(member->value, &out->json_bytes, child_path(path, "json_bytes"), error,
                           0ULL, 8388608ULL))
            return false;
    }
    {
        const auto member = value.FindMember("attachment_count");
        if (member == value.MemberEnd())
            return fail(error, "geometer.contract.missing_field",
                        child_path(path, "attachment_count"), "Required field is missing.");
        if (!decode_uint32(member->value, &out->attachment_count,
                           child_path(path, "attachment_count"), error, 0ULL, 16ULL))
            return false;
    }
    {
        const auto member = value.FindMember("attachment_name_bytes");
        if (member == value.MemberEnd())
            return fail(error, "geometer.contract.missing_field",
                        child_path(path, "attachment_name_bytes"), "Required field is missing.");
        if (!decode_uint32(member->value, &out->attachment_name_bytes,
                           child_path(path, "attachment_name_bytes"), error, 0ULL, 128ULL))
            return false;
    }
    {
        const auto member = value.FindMember("attachment_media_type_bytes");
        if (member == value.MemberEnd())
            return fail(error, "geometer.contract.missing_field",
                        child_path(path, "attachment_media_type_bytes"),
                        "Required field is missing.");
        if (!decode_uint32(member->value, &out->attachment_media_type_bytes,
                           child_path(path, "attachment_media_type_bytes"), error, 0ULL, 128ULL))
            return false;
    }
    {
        const auto member = value.FindMember("attachment_bytes");
        if (member == value.MemberEnd())
            return fail(error, "geometer.contract.missing_field",
                        child_path(path, "attachment_bytes"), "Required field is missing.");
        if (!decode_uint32(member->value, &out->attachment_bytes,
                           child_path(path, "attachment_bytes"), error, 0ULL, 268435456ULL))
            return false;
    }
    {
        const auto member = value.FindMember("frame_bytes");
        if (member == value.MemberEnd())
            return fail(error, "geometer.contract.missing_field", child_path(path, "frame_bytes"),
                        "Required field is missing.");
        if (!decode_uint32(member->value, &out->frame_bytes, child_path(path, "frame_bytes"), error,
                           0ULL, 536870912ULL))
            return false;
    }
    {
        const auto member = value.FindMember("queued_requests");
        if (member == value.MemberEnd())
            return fail(error, "geometer.contract.missing_field",
                        child_path(path, "queued_requests"), "Required field is missing.");
        if (!decode_uint32(member->value, &out->queued_requests,
                           child_path(path, "queued_requests"), error, 0ULL, 8ULL))
            return false;
    }
    {
        const auto member = value.FindMember("queued_bytes");
        if (member == value.MemberEnd())
            return fail(error, "geometer.contract.missing_field", child_path(path, "queued_bytes"),
                        "Required field is missing.");
        if (!decode_uint32(member->value, &out->queued_bytes, child_path(path, "queued_bytes"),
                           error, 0ULL, 536870912ULL))
            return false;
    }
    {
        const auto member = value.FindMember("resident_request_bytes");
        if (member == value.MemberEnd())
            return fail(error, "geometer.contract.missing_field",
                        child_path(path, "resident_request_bytes"), "Required field is missing.");
        if (!decode_uint32(member->value, &out->resident_request_bytes,
                           child_path(path, "resident_request_bytes"), error, 0ULL, 536870912ULL))
            return false;
    }
    {
        const auto member = value.FindMember("pending_writer_bytes");
        if (member == value.MemberEnd())
            return fail(error, "geometer.contract.missing_field",
                        child_path(path, "pending_writer_bytes"), "Required field is missing.");
        if (!decode_uint32(member->value, &out->pending_writer_bytes,
                           child_path(path, "pending_writer_bytes"), error, 0ULL, 536870912ULL))
            return false;
    }
    return true;
}

bool write_IpcEffectiveLimitsA0(rapidjson::Writer<rapidjson::StringBuffer>& writer,
                                const IpcEffectiveLimitsA0& value, ContractError* error)
{
    writer.StartObject();
    writer.Key("json_bytes");
    if (!write_uint32(writer, value.json_bytes, error, 0ULL, 8388608ULL))
        return false;
    writer.Key("attachment_count");
    if (!write_uint32(writer, value.attachment_count, error, 0ULL, 16ULL))
        return false;
    writer.Key("attachment_name_bytes");
    if (!write_uint32(writer, value.attachment_name_bytes, error, 0ULL, 128ULL))
        return false;
    writer.Key("attachment_media_type_bytes");
    if (!write_uint32(writer, value.attachment_media_type_bytes, error, 0ULL, 128ULL))
        return false;
    writer.Key("attachment_bytes");
    if (!write_uint32(writer, value.attachment_bytes, error, 0ULL, 268435456ULL))
        return false;
    writer.Key("frame_bytes");
    if (!write_uint32(writer, value.frame_bytes, error, 0ULL, 536870912ULL))
        return false;
    writer.Key("queued_requests");
    if (!write_uint32(writer, value.queued_requests, error, 0ULL, 8ULL))
        return false;
    writer.Key("queued_bytes");
    if (!write_uint32(writer, value.queued_bytes, error, 0ULL, 536870912ULL))
        return false;
    writer.Key("resident_request_bytes");
    if (!write_uint32(writer, value.resident_request_bytes, error, 0ULL, 536870912ULL))
        return false;
    writer.Key("pending_writer_bytes");
    if (!write_uint32(writer, value.pending_writer_bytes, error, 0ULL, 536870912ULL))
        return false;
    writer.EndObject();
    return true;
}

bool decode_IpcGenericAbiLimitsA0(const rapidjson::Value& value, IpcGenericAbiLimitsA0* out,
                                  const std::string& path, ContractError* error)
{
    static const char* const names[] = {"operation_id_bytes",
                                        "request_json_bytes",
                                        "response_json_bytes",
                                        "attachment_count",
                                        "attachment_name_bytes",
                                        "attachment_media_type_bytes",
                                        "attachment_bytes",
                                        "aggregate_attachment_bytes_native",
                                        "aggregate_attachment_bytes_wasm"};
    if (!validate_object(value, names, 9U, path, error))
        return false;
    {
        const auto member = value.FindMember("operation_id_bytes");
        if (member == value.MemberEnd())
            return fail(error, "geometer.contract.missing_field",
                        child_path(path, "operation_id_bytes"), "Required field is missing.");
        if (!decode_uint32(member->value, &out->operation_id_bytes,
                           child_path(path, "operation_id_bytes"), error, 0ULL, 128ULL))
            return false;
    }
    {
        const auto member = value.FindMember("request_json_bytes");
        if (member == value.MemberEnd())
            return fail(error, "geometer.contract.missing_field",
                        child_path(path, "request_json_bytes"), "Required field is missing.");
        if (!decode_uint32(member->value, &out->request_json_bytes,
                           child_path(path, "request_json_bytes"), error, 0ULL, 8388608ULL))
            return false;
    }
    {
        const auto member = value.FindMember("response_json_bytes");
        if (member == value.MemberEnd())
            return fail(error, "geometer.contract.missing_field",
                        child_path(path, "response_json_bytes"), "Required field is missing.");
        if (!decode_uint32(member->value, &out->response_json_bytes,
                           child_path(path, "response_json_bytes"), error, 0ULL, 8388608ULL))
            return false;
    }
    {
        const auto member = value.FindMember("attachment_count");
        if (member == value.MemberEnd())
            return fail(error, "geometer.contract.missing_field",
                        child_path(path, "attachment_count"), "Required field is missing.");
        if (!decode_uint32(member->value, &out->attachment_count,
                           child_path(path, "attachment_count"), error, 0ULL, 16ULL))
            return false;
    }
    {
        const auto member = value.FindMember("attachment_name_bytes");
        if (member == value.MemberEnd())
            return fail(error, "geometer.contract.missing_field",
                        child_path(path, "attachment_name_bytes"), "Required field is missing.");
        if (!decode_uint32(member->value, &out->attachment_name_bytes,
                           child_path(path, "attachment_name_bytes"), error, 0ULL, 128ULL))
            return false;
    }
    {
        const auto member = value.FindMember("attachment_media_type_bytes");
        if (member == value.MemberEnd())
            return fail(error, "geometer.contract.missing_field",
                        child_path(path, "attachment_media_type_bytes"),
                        "Required field is missing.");
        if (!decode_uint32(member->value, &out->attachment_media_type_bytes,
                           child_path(path, "attachment_media_type_bytes"), error, 0ULL, 128ULL))
            return false;
    }
    {
        const auto member = value.FindMember("attachment_bytes");
        if (member == value.MemberEnd())
            return fail(error, "geometer.contract.missing_field",
                        child_path(path, "attachment_bytes"), "Required field is missing.");
        if (!decode_uint32(member->value, &out->attachment_bytes,
                           child_path(path, "attachment_bytes"), error, 0ULL, 268435456ULL))
            return false;
    }
    {
        const auto member = value.FindMember("aggregate_attachment_bytes_native");
        if (member == value.MemberEnd())
            return fail(error, "geometer.contract.missing_field",
                        child_path(path, "aggregate_attachment_bytes_native"),
                        "Required field is missing.");
        if (!decode_uint32(member->value, &out->aggregate_attachment_bytes_native,
                           child_path(path, "aggregate_attachment_bytes_native"), error, 0ULL,
                           536870912ULL))
            return false;
    }
    {
        const auto member = value.FindMember("aggregate_attachment_bytes_wasm");
        if (member == value.MemberEnd())
            return fail(error, "geometer.contract.missing_field",
                        child_path(path, "aggregate_attachment_bytes_wasm"),
                        "Required field is missing.");
        if (!decode_uint32(member->value, &out->aggregate_attachment_bytes_wasm,
                           child_path(path, "aggregate_attachment_bytes_wasm"), error, 0ULL,
                           268435456ULL))
            return false;
    }
    return true;
}

bool write_IpcGenericAbiLimitsA0(rapidjson::Writer<rapidjson::StringBuffer>& writer,
                                 const IpcGenericAbiLimitsA0& value, ContractError* error)
{
    writer.StartObject();
    writer.Key("operation_id_bytes");
    if (!write_uint32(writer, value.operation_id_bytes, error, 0ULL, 128ULL))
        return false;
    writer.Key("request_json_bytes");
    if (!write_uint32(writer, value.request_json_bytes, error, 0ULL, 8388608ULL))
        return false;
    writer.Key("response_json_bytes");
    if (!write_uint32(writer, value.response_json_bytes, error, 0ULL, 8388608ULL))
        return false;
    writer.Key("attachment_count");
    if (!write_uint32(writer, value.attachment_count, error, 0ULL, 16ULL))
        return false;
    writer.Key("attachment_name_bytes");
    if (!write_uint32(writer, value.attachment_name_bytes, error, 0ULL, 128ULL))
        return false;
    writer.Key("attachment_media_type_bytes");
    if (!write_uint32(writer, value.attachment_media_type_bytes, error, 0ULL, 128ULL))
        return false;
    writer.Key("attachment_bytes");
    if (!write_uint32(writer, value.attachment_bytes, error, 0ULL, 268435456ULL))
        return false;
    writer.Key("aggregate_attachment_bytes_native");
    if (!write_uint32(writer, value.aggregate_attachment_bytes_native, error, 0ULL, 536870912ULL))
        return false;
    writer.Key("aggregate_attachment_bytes_wasm");
    if (!write_uint32(writer, value.aggregate_attachment_bytes_wasm, error, 0ULL, 268435456ULL))
        return false;
    writer.EndObject();
    return true;
}

bool decode_IpcHelloA0(const rapidjson::Value& value, IpcHelloA0* out, const std::string& path,
                       ContractError* error)
{
    static const char* const names[] = {"client_name", "client_version", "protocols",
                                        "capabilities"};
    if (!validate_object(value, names, 4U, path, error))
        return false;
    {
        const auto member = value.FindMember("client_name");
        if (member == value.MemberEnd())
            return fail(error, "geometer.contract.missing_field", child_path(path, "client_name"),
                        "Required field is missing.");
        if (!decode_string(member->value, &out->client_name, child_path(path, "client_name"), error,
                           1U, 128U))
            return false;
    }
    {
        const auto member = value.FindMember("client_version");
        if (member == value.MemberEnd())
            return fail(error, "geometer.contract.missing_field",
                        child_path(path, "client_version"), "Required field is missing.");
        if (!decode_string(member->value, &out->client_version, child_path(path, "client_version"),
                           error, 1U, 128U))
            return false;
    }
    {
        const auto member = value.FindMember("protocols");
        if (member == value.MemberEnd())
            return fail(error, "geometer.contract.missing_field", child_path(path, "protocols"),
                        "Required field is missing.");
        if (!decode_array(member->value, &out->protocols, child_path(path, "protocols"), error, 1U,
                          16U, decode_string_item))
            return false;
    }
    {
        const auto member = value.FindMember("capabilities");
        if (member != value.MemberEnd())
        {
            std::vector<std::string> decoded{};
            if (!decode_array(member->value, &decoded, child_path(path, "capabilities"), error, 0U,
                              64U, decode_string_item))
                return false;
            out->capabilities = std::move(decoded);
        }
        else
            out->capabilities.reset();
    }
    return true;
}

bool write_IpcHelloA0(rapidjson::Writer<rapidjson::StringBuffer>& writer, const IpcHelloA0& value,
                      ContractError* error)
{
    writer.StartObject();
    writer.Key("client_name");
    if (!write_string(writer, value.client_name, error, 1U, 128U))
        return false;
    writer.Key("client_version");
    if (!write_string(writer, value.client_version, error, 1U, 128U))
        return false;
    writer.Key("protocols");
    if (!write_array(writer, value.protocols, error, 1U, 16U, write_string_item))
        return false;
    if (value.capabilities.has_value())
    {
        writer.Key("capabilities");
        if (!write_array(writer, *value.capabilities, error, 0U, 64U, write_string_item))
            return false;
    }
    writer.EndObject();
    return true;
}

bool decode_IpcRuntimeDispatchA0(const rapidjson::Value& value, IpcRuntimeDispatchA0* out,
                                 const std::string& path, ContractError* error)
{
    if (!value.IsString())
        return fail(error, "geometer.contract.type_mismatch", path, "Expected a string enum.");
    const std::string text(value.GetString(), value.GetStringLength());
    if (text == "logical_dto")
    {
        *out = IpcRuntimeDispatchA0::logical_dto;
        return true;
    }
    if (text == "packed_attachment")
    {
        *out = IpcRuntimeDispatchA0::packed_attachment;
        return true;
    }
    return fail(error, "geometer.contract.enum_mismatch", path, "Unknown enum value.");
}

bool write_IpcRuntimeDispatchA0(rapidjson::Writer<rapidjson::StringBuffer>& writer,
                                const IpcRuntimeDispatchA0& value, ContractError* error)
{
    switch (value)
    {
    case IpcRuntimeDispatchA0::logical_dto:
        writer.String("logical_dto");
        return true;
    case IpcRuntimeDispatchA0::packed_attachment:
        writer.String("packed_attachment");
        return true;
    }
    return fail(error, "geometer.contract.enum_mismatch", "", "Unknown enum value.");
}

bool decode_IpcPackedProjectionA0(const rapidjson::Value& value, IpcPackedProjectionA0* out,
                                  const std::string& path, ContractError* error)
{
    static const char* const names[] = {"kind", "attachment_name", "format"};
    if (!validate_object(value, names, 3U, path, error))
        return false;
    {
        const auto member = value.FindMember("kind");
        if (member == value.MemberEnd())
            return fail(error, "geometer.contract.missing_field", child_path(path, "kind"),
                        "Required field is missing.");
        if (!decode_literal_string(member->value, &out->kind, child_path(path, "kind"), error,
                                   "packed_attachment"))
            return false;
    }
    {
        const auto member = value.FindMember("attachment_name");
        if (member == value.MemberEnd())
            return fail(error, "geometer.contract.missing_field",
                        child_path(path, "attachment_name"), "Required field is missing.");
        if (!decode_string(member->value, &out->attachment_name,
                           child_path(path, "attachment_name"), error, 1U, 128U))
            return false;
    }
    {
        const auto member = value.FindMember("format");
        if (member == value.MemberEnd())
            return fail(error, "geometer.contract.missing_field", child_path(path, "format"),
                        "Required field is missing.");
        if (!decode_string(member->value, &out->format, child_path(path, "format"), error, 1U,
                           128U))
            return false;
    }
    return true;
}

bool write_IpcPackedProjectionA0(rapidjson::Writer<rapidjson::StringBuffer>& writer,
                                 const IpcPackedProjectionA0& value, ContractError* error)
{
    writer.StartObject();
    writer.Key("kind");
    if (!write_literal_string(writer, value.kind, error, "packed_attachment"))
        return false;
    writer.Key("attachment_name");
    if (!write_string(writer, value.attachment_name, error, 1U, 128U))
        return false;
    writer.Key("format");
    if (!write_string(writer, value.format, error, 1U, 128U))
        return false;
    writer.EndObject();
    return true;
}

bool decode_IpcOperationDeclarationA0(const rapidjson::Value& value, IpcOperationDeclarationA0* out,
                                      const std::string& path, ContractError* error)
{
    static const char* const names[] = {
        "identity",          "request_contract",   "result_contract",    "runtime_dispatch",
        "input_attachments", "output_attachments", "request_projection", "result_projection"};
    if (!validate_object(value, names, 8U, path, error))
        return false;
    {
        const auto member = value.FindMember("identity");
        if (member == value.MemberEnd())
            return fail(error, "geometer.contract.missing_field", child_path(path, "identity"),
                        "Required field is missing.");
        if (!decode_string(member->value, &out->identity, child_path(path, "identity"), error, 1U,
                           128U))
            return false;
    }
    {
        const auto member = value.FindMember("request_contract");
        if (member == value.MemberEnd())
            return fail(error, "geometer.contract.missing_field",
                        child_path(path, "request_contract"), "Required field is missing.");
        if (!decode_string(member->value, &out->request_contract,
                           child_path(path, "request_contract"), error, 1U, 128U))
            return false;
    }
    {
        const auto member = value.FindMember("result_contract");
        if (member == value.MemberEnd())
            return fail(error, "geometer.contract.missing_field",
                        child_path(path, "result_contract"), "Required field is missing.");
        if (!decode_string(member->value, &out->result_contract,
                           child_path(path, "result_contract"), error, 1U, 128U))
            return false;
    }
    {
        const auto member = value.FindMember("runtime_dispatch");
        if (member == value.MemberEnd())
            return fail(error, "geometer.contract.missing_field",
                        child_path(path, "runtime_dispatch"), "Required field is missing.");
        if (!decode_IpcRuntimeDispatchA0(member->value, &out->runtime_dispatch,
                                         child_path(path, "runtime_dispatch"), error))
            return false;
    }
    {
        const auto member = value.FindMember("input_attachments");
        if (member == value.MemberEnd())
            return fail(error, "geometer.contract.missing_field",
                        child_path(path, "input_attachments"), "Required field is missing.");
        if (!decode_array(member->value, &out->input_attachments,
                          child_path(path, "input_attachments"), error, 0U, 16U,
                          decode_IpcAttachmentDeclarationA0))
            return false;
    }
    {
        const auto member = value.FindMember("output_attachments");
        if (member == value.MemberEnd())
            return fail(error, "geometer.contract.missing_field",
                        child_path(path, "output_attachments"), "Required field is missing.");
        if (!decode_array(member->value, &out->output_attachments,
                          child_path(path, "output_attachments"), error, 0U, 16U,
                          decode_IpcAttachmentDeclarationA0))
            return false;
    }
    {
        const auto member = value.FindMember("request_projection");
        if (member != value.MemberEnd())
        {
            IpcPackedProjectionA0 decoded{};
            if (!decode_IpcPackedProjectionA0(member->value, &decoded,
                                              child_path(path, "request_projection"), error))
                return false;
            out->request_projection = std::move(decoded);
        }
        else
            out->request_projection.reset();
    }
    {
        const auto member = value.FindMember("result_projection");
        if (member != value.MemberEnd())
        {
            IpcPackedProjectionA0 decoded{};
            if (!decode_IpcPackedProjectionA0(member->value, &decoded,
                                              child_path(path, "result_projection"), error))
                return false;
            out->result_projection = std::move(decoded);
        }
        else
            out->result_projection.reset();
    }
    return true;
}

bool write_IpcOperationDeclarationA0(rapidjson::Writer<rapidjson::StringBuffer>& writer,
                                     const IpcOperationDeclarationA0& value, ContractError* error)
{
    writer.StartObject();
    writer.Key("identity");
    if (!write_string(writer, value.identity, error, 1U, 128U))
        return false;
    writer.Key("request_contract");
    if (!write_string(writer, value.request_contract, error, 1U, 128U))
        return false;
    writer.Key("result_contract");
    if (!write_string(writer, value.result_contract, error, 1U, 128U))
        return false;
    writer.Key("runtime_dispatch");
    if (!write_IpcRuntimeDispatchA0(writer, value.runtime_dispatch, error))
        return false;
    writer.Key("input_attachments");
    if (!write_array(writer, value.input_attachments, error, 0U, 16U,
                     write_IpcAttachmentDeclarationA0))
        return false;
    writer.Key("output_attachments");
    if (!write_array(writer, value.output_attachments, error, 0U, 16U,
                     write_IpcAttachmentDeclarationA0))
        return false;
    if (value.request_projection.has_value())
    {
        writer.Key("request_projection");
        if (!write_IpcPackedProjectionA0(writer, *value.request_projection, error))
            return false;
    }
    if (value.result_projection.has_value())
    {
        writer.Key("result_projection");
        if (!write_IpcPackedProjectionA0(writer, *value.result_projection, error))
            return false;
    }
    writer.EndObject();
    return true;
}

bool decode_IpcOperationCatalogA0(const rapidjson::Value& value, IpcOperationCatalogA0* out,
                                  const std::string& path, ContractError* error)
{
    static const char* const names[] = {
        "catalog",    "generic_abi",           "release_version", "c_abi_generation",
        "operations", "attachment_descriptor", "limits"};
    if (!validate_object(value, names, 7U, path, error))
        return false;
    {
        const auto member = value.FindMember("catalog");
        if (member == value.MemberEnd())
            return fail(error, "geometer.contract.missing_field", child_path(path, "catalog"),
                        "Required field is missing.");
        if (!decode_literal_string(member->value, &out->catalog, child_path(path, "catalog"), error,
                                   "wn.geometer.operation_catalog.a0"))
            return false;
    }
    {
        const auto member = value.FindMember("generic_abi");
        if (member == value.MemberEnd())
            return fail(error, "geometer.contract.missing_field", child_path(path, "generic_abi"),
                        "Required field is missing.");
        if (!decode_literal_string(member->value, &out->generic_abi,
                                   child_path(path, "generic_abi"), error, "a0"))
            return false;
    }
    {
        const auto member = value.FindMember("release_version");
        if (member == value.MemberEnd())
            return fail(error, "geometer.contract.missing_field",
                        child_path(path, "release_version"), "Required field is missing.");
        if (!decode_string(member->value, &out->release_version,
                           child_path(path, "release_version"), error, 1U, 32U))
            return false;
    }
    {
        const auto member = value.FindMember("c_abi_generation");
        if (member == value.MemberEnd())
            return fail(error, "geometer.contract.missing_field",
                        child_path(path, "c_abi_generation"), "Required field is missing.");
        if (!decode_uint32(member->value, &out->c_abi_generation,
                           child_path(path, "c_abi_generation"), error, 0ULL,
                           std::numeric_limits<std::uint32_t>::max()))
            return false;
    }
    {
        const auto member = value.FindMember("operations");
        if (member == value.MemberEnd())
            return fail(error, "geometer.contract.missing_field", child_path(path, "operations"),
                        "Required field is missing.");
        if (!decode_array(member->value, &out->operations, child_path(path, "operations"), error,
                          1U, std::numeric_limits<std::size_t>::max(),
                          decode_IpcOperationDeclarationA0))
            return false;
    }
    {
        const auto member = value.FindMember("attachment_descriptor");
        if (member == value.MemberEnd())
            return fail(error, "geometer.contract.missing_field",
                        child_path(path, "attachment_descriptor"), "Required field is missing.");
        if (!decode_IpcAttachmentDescriptorA0(member->value, &out->attachment_descriptor,
                                              child_path(path, "attachment_descriptor"), error))
            return false;
    }
    {
        const auto member = value.FindMember("limits");
        if (member == value.MemberEnd())
            return fail(error, "geometer.contract.missing_field", child_path(path, "limits"),
                        "Required field is missing.");
        if (!decode_IpcGenericAbiLimitsA0(member->value, &out->limits, child_path(path, "limits"),
                                          error))
            return false;
    }
    return true;
}

bool write_IpcOperationCatalogA0(rapidjson::Writer<rapidjson::StringBuffer>& writer,
                                 const IpcOperationCatalogA0& value, ContractError* error)
{
    writer.StartObject();
    writer.Key("catalog");
    if (!write_literal_string(writer, value.catalog, error, "wn.geometer.operation_catalog.a0"))
        return false;
    writer.Key("generic_abi");
    if (!write_literal_string(writer, value.generic_abi, error, "a0"))
        return false;
    writer.Key("release_version");
    if (!write_string(writer, value.release_version, error, 1U, 32U))
        return false;
    writer.Key("c_abi_generation");
    if (!write_uint32(writer, value.c_abi_generation, error, 0ULL,
                      std::numeric_limits<std::uint32_t>::max()))
        return false;
    writer.Key("operations");
    if (!write_array(writer, value.operations, error, 1U, std::numeric_limits<std::size_t>::max(),
                     write_IpcOperationDeclarationA0))
        return false;
    writer.Key("attachment_descriptor");
    if (!write_IpcAttachmentDescriptorA0(writer, value.attachment_descriptor, error))
        return false;
    writer.Key("limits");
    if (!write_IpcGenericAbiLimitsA0(writer, value.limits, error))
        return false;
    writer.EndObject();
    return true;
}

bool decode_IpcProtocolErrorA0(const rapidjson::Value& value, IpcProtocolErrorA0* out,
                               const std::string& path, ContractError* error)
{
    static const char* const names[] = {"status", "diagnostic"};
    if (!validate_object(value, names, 2U, path, error))
        return false;
    {
        const auto member = value.FindMember("status");
        if (member == value.MemberEnd())
            return fail(error, "geometer.contract.missing_field", child_path(path, "status"),
                        "Required field is missing.");
        if (!decode_literal_string(member->value, &out->status, child_path(path, "status"), error,
                                   "protocol_error"))
            return false;
    }
    {
        const auto member = value.FindMember("diagnostic");
        if (member == value.MemberEnd())
            return fail(error, "geometer.contract.missing_field", child_path(path, "diagnostic"),
                        "Required field is missing.");
        if (!decode_DiagnosticA0(member->value, &out->diagnostic, child_path(path, "diagnostic"),
                                 error))
            return false;
    }
    return true;
}

bool write_IpcProtocolErrorA0(rapidjson::Writer<rapidjson::StringBuffer>& writer,
                              const IpcProtocolErrorA0& value, ContractError* error)
{
    writer.StartObject();
    writer.Key("status");
    if (!write_literal_string(writer, value.status, error, "protocol_error"))
        return false;
    writer.Key("diagnostic");
    if (!write_DiagnosticA0(writer, value.diagnostic, error))
        return false;
    writer.EndObject();
    return true;
}

bool decode_IpcReasonA0(const rapidjson::Value& value, IpcReasonA0* out, const std::string& path,
                        ContractError* error)
{
    static const char* const names[] = {"reason"};
    if (!validate_object(value, names, 1U, path, error))
        return false;
    {
        const auto member = value.FindMember("reason");
        if (member != value.MemberEnd())
        {
            std::string decoded{};
            if (!decode_string(member->value, &decoded, child_path(path, "reason"), error, 0U,
                               1024U))
                return false;
            out->reason = std::move(decoded);
        }
        else
            out->reason.reset();
    }
    return true;
}

bool write_IpcReasonA0(rapidjson::Writer<rapidjson::StringBuffer>& writer, const IpcReasonA0& value,
                       ContractError* error)
{
    writer.StartObject();
    if (value.reason.has_value())
    {
        writer.Key("reason");
        if (!write_string(writer, *value.reason, error, 0U, 1024U))
            return false;
    }
    writer.EndObject();
    return true;
}

bool decode_ModelFormat(const rapidjson::Value& value, ModelFormat* out, const std::string& path,
                        ContractError* error)
{
    if (!value.IsString())
        return fail(error, "geometer.contract.type_mismatch", path, "Expected a string enum.");
    const std::string text(value.GetString(), value.GetStringLength());
    if (text == "step")
    {
        *out = ModelFormat::step;
        return true;
    }
    return fail(error, "geometer.contract.enum_mismatch", path, "Unknown enum value.");
}

bool write_ModelFormat(rapidjson::Writer<rapidjson::StringBuffer>& writer, const ModelFormat& value,
                       ContractError* error)
{
    switch (value)
    {
    case ModelFormat::step:
        writer.String("step");
        return true;
    }
    return fail(error, "geometer.contract.enum_mismatch", "", "Unknown enum value.");
}

bool decode_Matrix4x4(const rapidjson::Value& value, Matrix4x4* out, const std::string& path,
                      ContractError* error)
{
    if (!value.IsArray() || value.Size() < 16U || value.Size() > 16U)
        return fail(error, "geometer.contract.array_size", path,
                    "Array length is outside its contract bounds.");
    out->clear();
    out->reserve(value.Size());
    for (rapidjson::SizeType i = 0; i < value.Size(); ++i)
    {
        double item_value{};
        if (!decode_double(value[i], &item_value, path + "/" + std::to_string(i), error,
                           -std::numeric_limits<double>::infinity(),
                           std::numeric_limits<double>::infinity(), false, false))
            return false;
        out->push_back(std::move(item_value));
    }
    return true;
}

bool write_Matrix4x4(rapidjson::Writer<rapidjson::StringBuffer>& writer, const Matrix4x4& value,
                     ContractError* error)
{
    if (value.size() < 16U || value.size() > 16U)
        return fail(error, "geometer.contract.array_size", "",
                    "Array length is outside its contract bounds.");
    writer.StartArray();
    for (const auto& item_value : value)
        if (!write_double(writer, item_value, error, -std::numeric_limits<double>::infinity(),
                          std::numeric_limits<double>::infinity(), false, false))
            return false;
    writer.EndArray();
    return true;
}

bool decode_ModelBoundsOptionsA0(const rapidjson::Value& value, ModelBoundsOptionsA0* out,
                                 const std::string& path, ContractError* error)
{
    static const char* const names[] = {"format", "model_transform"};
    if (!validate_object(value, names, 2U, path, error))
        return false;
    {
        const auto member = value.FindMember("format");
        if (member != value.MemberEnd())
        {
            ModelFormat decoded{};
            if (!decode_ModelFormat(member->value, &decoded, child_path(path, "format"), error))
                return false;
            out->format = std::move(decoded);
        }
        else
            out->format.reset();
    }
    {
        const auto member = value.FindMember("model_transform");
        if (member != value.MemberEnd())
        {
            Matrix4x4 decoded{};
            if (!decode_Matrix4x4(member->value, &decoded, child_path(path, "model_transform"),
                                  error))
                return false;
            out->model_transform = std::move(decoded);
        }
        else
            out->model_transform.reset();
    }
    return true;
}

bool write_ModelBoundsOptionsA0(rapidjson::Writer<rapidjson::StringBuffer>& writer,
                                const ModelBoundsOptionsA0& value, ContractError* error)
{
    writer.StartObject();
    if (value.format.has_value())
    {
        writer.Key("format");
        if (!write_ModelFormat(writer, *value.format, error))
            return false;
    }
    if (value.model_transform.has_value())
    {
        writer.Key("model_transform");
        if (!write_Matrix4x4(writer, *value.model_transform, error))
            return false;
    }
    writer.EndObject();
    return true;
}

bool decode_StepTopologyOpenRequestA0(const rapidjson::Value& value, StepTopologyOpenRequestA0* out,
                                      const std::string& path, ContractError* error)
{
    static const char* const names[] = {"schema"};
    if (!validate_object(value, names, 1U, path, error))
        return false;
    {
        const auto member = value.FindMember("schema");
        if (member == value.MemberEnd())
            return fail(error, "geometer.contract.missing_field", child_path(path, "schema"),
                        "Required field is missing.");
        if (!decode_literal_string(member->value, &out->schema, child_path(path, "schema"), error,
                                   "geometry.step_topology.open.request.a0"))
            return false;
    }
    return true;
}

bool write_StepTopologyOpenRequestA0(rapidjson::Writer<rapidjson::StringBuffer>& writer,
                                     const StepTopologyOpenRequestA0& value, ContractError* error)
{
    writer.StartObject();
    writer.Key("schema");
    if (!write_literal_string(writer, value.schema, error,
                              "geometry.step_topology.open.request.a0"))
        return false;
    writer.EndObject();
    return true;
}

bool decode_SessionReference(const rapidjson::Value& value, SessionReference* out,
                             const std::string& path, ContractError* error)
{
    static const char* const names[] = {"session_handle", "generation"};
    if (!validate_object(value, names, 2U, path, error))
        return false;
    {
        const auto member = value.FindMember("session_handle");
        if (member == value.MemberEnd())
            return fail(error, "geometer.contract.missing_field",
                        child_path(path, "session_handle"), "Required field is missing.");
        if (!decode_string(member->value, &out->session_handle, child_path(path, "session_handle"),
                           error, 68U, 68U))
            return false;
    }
    {
        const auto member = value.FindMember("generation");
        if (member == value.MemberEnd())
            return fail(error, "geometer.contract.missing_field", child_path(path, "generation"),
                        "Required field is missing.");
        if (!decode_uint32(member->value, &out->generation, child_path(path, "generation"), error,
                           1ULL, std::numeric_limits<std::uint32_t>::max()))
            return false;
    }
    return true;
}

bool write_SessionReference(rapidjson::Writer<rapidjson::StringBuffer>& writer,
                            const SessionReference& value, ContractError* error)
{
    writer.StartObject();
    writer.Key("session_handle");
    if (!write_string(writer, value.session_handle, error, 68U, 68U))
        return false;
    writer.Key("generation");
    if (!write_uint32(writer, value.generation, error, 1ULL,
                      std::numeric_limits<std::uint32_t>::max()))
        return false;
    writer.EndObject();
    return true;
}

bool decode_StepTopologyCloseRequestA0(const rapidjson::Value& value,
                                       StepTopologyCloseRequestA0* out, const std::string& path,
                                       ContractError* error)
{
    static const char* const names[] = {"schema", "session"};
    if (!validate_object(value, names, 2U, path, error))
        return false;
    {
        const auto member = value.FindMember("schema");
        if (member == value.MemberEnd())
            return fail(error, "geometer.contract.missing_field", child_path(path, "schema"),
                        "Required field is missing.");
        if (!decode_literal_string(member->value, &out->schema, child_path(path, "schema"), error,
                                   "geometry.step_topology.close.request.a0"))
            return false;
    }
    {
        const auto member = value.FindMember("session");
        if (member == value.MemberEnd())
            return fail(error, "geometer.contract.missing_field", child_path(path, "session"),
                        "Required field is missing.");
        if (!decode_SessionReference(member->value, &out->session, child_path(path, "session"),
                                     error))
            return false;
    }
    return true;
}

bool write_StepTopologyCloseRequestA0(rapidjson::Writer<rapidjson::StringBuffer>& writer,
                                      const StepTopologyCloseRequestA0& value, ContractError* error)
{
    writer.StartObject();
    writer.Key("schema");
    if (!write_literal_string(writer, value.schema, error,
                              "geometry.step_topology.close.request.a0"))
        return false;
    writer.Key("session");
    if (!write_SessionReference(writer, value.session, error))
        return false;
    writer.EndObject();
    return true;
}

bool decode_PageRequest(const rapidjson::Value& value, PageRequest* out, const std::string& path,
                        ContractError* error)
{
    static const char* const names[] = {"cursor", "limit"};
    if (!validate_object(value, names, 2U, path, error))
        return false;
    {
        const auto member = value.FindMember("cursor");
        if (member != value.MemberEnd())
        {
            std::string decoded{};
            if (!decode_string(member->value, &decoded, child_path(path, "cursor"), error, 0U,
                               256U))
                return false;
            out->cursor = std::move(decoded);
        }
        else
            out->cursor.reset();
    }
    {
        const auto member = value.FindMember("limit");
        if (member == value.MemberEnd())
            return fail(error, "geometer.contract.missing_field", child_path(path, "limit"),
                        "Required field is missing.");
        if (!decode_uint32(member->value, &out->limit, child_path(path, "limit"), error, 1ULL,
                           1024ULL))
            return false;
    }
    return true;
}

bool write_PageRequest(rapidjson::Writer<rapidjson::StringBuffer>& writer, const PageRequest& value,
                       ContractError* error)
{
    writer.StartObject();
    if (value.cursor.has_value())
    {
        writer.Key("cursor");
        if (!write_string(writer, *value.cursor, error, 0U, 256U))
            return false;
    }
    writer.Key("limit");
    if (!write_uint32(writer, value.limit, error, 1ULL, 1024ULL))
        return false;
    writer.EndObject();
    return true;
}

bool decode_StepTopologyInspectRequestA0(const rapidjson::Value& value,
                                         StepTopologyInspectRequestA0* out, const std::string& path,
                                         ContractError* error)
{
    static const char* const names[] = {"schema", "session", "page",
                                        "include_source_entity_evidence", "include_diagnostics"};
    if (!validate_object(value, names, 5U, path, error))
        return false;
    {
        const auto member = value.FindMember("schema");
        if (member == value.MemberEnd())
            return fail(error, "geometer.contract.missing_field", child_path(path, "schema"),
                        "Required field is missing.");
        if (!decode_literal_string(member->value, &out->schema, child_path(path, "schema"), error,
                                   "geometry.step_topology.inspect.request.a0"))
            return false;
    }
    {
        const auto member = value.FindMember("session");
        if (member == value.MemberEnd())
            return fail(error, "geometer.contract.missing_field", child_path(path, "session"),
                        "Required field is missing.");
        if (!decode_SessionReference(member->value, &out->session, child_path(path, "session"),
                                     error))
            return false;
    }
    {
        const auto member = value.FindMember("page");
        if (member == value.MemberEnd())
            return fail(error, "geometer.contract.missing_field", child_path(path, "page"),
                        "Required field is missing.");
        if (!decode_PageRequest(member->value, &out->page, child_path(path, "page"), error))
            return false;
    }
    {
        const auto member = value.FindMember("include_source_entity_evidence");
        if (member == value.MemberEnd())
            return fail(error, "geometer.contract.missing_field",
                        child_path(path, "include_source_entity_evidence"),
                        "Required field is missing.");
        if (!decode_boolean(member->value, &out->include_source_entity_evidence,
                            child_path(path, "include_source_entity_evidence"), error))
            return false;
    }
    {
        const auto member = value.FindMember("include_diagnostics");
        if (member == value.MemberEnd())
            return fail(error, "geometer.contract.missing_field",
                        child_path(path, "include_diagnostics"), "Required field is missing.");
        if (!decode_boolean(member->value, &out->include_diagnostics,
                            child_path(path, "include_diagnostics"), error))
            return false;
    }
    return true;
}

bool write_StepTopologyInspectRequestA0(rapidjson::Writer<rapidjson::StringBuffer>& writer,
                                        const StepTopologyInspectRequestA0& value,
                                        ContractError* error)
{
    writer.StartObject();
    writer.Key("schema");
    if (!write_literal_string(writer, value.schema, error,
                              "geometry.step_topology.inspect.request.a0"))
        return false;
    writer.Key("session");
    if (!write_SessionReference(writer, value.session, error))
        return false;
    writer.Key("page");
    if (!write_PageRequest(writer, value.page, error))
        return false;
    writer.Key("include_source_entity_evidence");
    if (!(writer.Bool(value.include_source_entity_evidence), true))
        return false;
    writer.Key("include_diagnostics");
    if (!(writer.Bool(value.include_diagnostics), true))
        return false;
    writer.EndObject();
    return true;
}

bool decode_TessellationOptions(const rapidjson::Value& value, TessellationOptions* out,
                                const std::string& path, ContractError* error)
{
    static const char* const names[] = {"linear_deflection_mm", "angular_deflection_rad",
                                        "relative", "parallel", "source_to_render"};
    if (!validate_object(value, names, 5U, path, error))
        return false;
    {
        const auto member = value.FindMember("linear_deflection_mm");
        if (member == value.MemberEnd())
            return fail(error, "geometer.contract.missing_field",
                        child_path(path, "linear_deflection_mm"), "Required field is missing.");
        if (!decode_double(member->value, &out->linear_deflection_mm,
                           child_path(path, "linear_deflection_mm"), error, 0.000001, 1000, false,
                           false))
            return false;
    }
    {
        const auto member = value.FindMember("angular_deflection_rad");
        if (member == value.MemberEnd())
            return fail(error, "geometer.contract.missing_field",
                        child_path(path, "angular_deflection_rad"), "Required field is missing.");
        if (!decode_double(member->value, &out->angular_deflection_rad,
                           child_path(path, "angular_deflection_rad"), error, 0.000001,
                           3.141592653589793, false, false))
            return false;
    }
    {
        const auto member = value.FindMember("relative");
        if (member == value.MemberEnd())
            return fail(error, "geometer.contract.missing_field", child_path(path, "relative"),
                        "Required field is missing.");
        if (!decode_boolean(member->value, &out->relative, child_path(path, "relative"), error))
            return false;
    }
    {
        const auto member = value.FindMember("parallel");
        if (member == value.MemberEnd())
            return fail(error, "geometer.contract.missing_field", child_path(path, "parallel"),
                        "Required field is missing.");
        if (!decode_boolean(member->value, &out->parallel, child_path(path, "parallel"), error))
            return false;
    }
    {
        const auto member = value.FindMember("source_to_render");
        if (member == value.MemberEnd())
            return fail(error, "geometer.contract.missing_field",
                        child_path(path, "source_to_render"), "Required field is missing.");
        if (!decode_array(member->value, &out->source_to_render,
                          child_path(path, "source_to_render"), error, 12U, 12U,
                          decode_double_item))
            return false;
    }
    return true;
}

bool write_TessellationOptions(rapidjson::Writer<rapidjson::StringBuffer>& writer,
                               const TessellationOptions& value, ContractError* error)
{
    writer.StartObject();
    writer.Key("linear_deflection_mm");
    if (!write_double(writer, value.linear_deflection_mm, error, 0.000001, 1000, false, false))
        return false;
    writer.Key("angular_deflection_rad");
    if (!write_double(writer, value.angular_deflection_rad, error, 0.000001, 3.141592653589793,
                      false, false))
        return false;
    writer.Key("relative");
    if (!(writer.Bool(value.relative), true))
        return false;
    writer.Key("parallel");
    if (!(writer.Bool(value.parallel), true))
        return false;
    writer.Key("source_to_render");
    if (!write_array(writer, value.source_to_render, error, 12U, 12U, write_double_item))
        return false;
    writer.EndObject();
    return true;
}

bool decode_StepTopologyRenderRequestA0(const rapidjson::Value& value,
                                        StepTopologyRenderRequestA0* out, const std::string& path,
                                        ContractError* error)
{
    static const char* const names[] = {"schema", "session", "tessellation"};
    if (!validate_object(value, names, 3U, path, error))
        return false;
    {
        const auto member = value.FindMember("schema");
        if (member == value.MemberEnd())
            return fail(error, "geometer.contract.missing_field", child_path(path, "schema"),
                        "Required field is missing.");
        if (!decode_literal_string(member->value, &out->schema, child_path(path, "schema"), error,
                                   "geometry.step_topology.render.request.a0"))
            return false;
    }
    {
        const auto member = value.FindMember("session");
        if (member == value.MemberEnd())
            return fail(error, "geometer.contract.missing_field", child_path(path, "session"),
                        "Required field is missing.");
        if (!decode_SessionReference(member->value, &out->session, child_path(path, "session"),
                                     error))
            return false;
    }
    {
        const auto member = value.FindMember("tessellation");
        if (member == value.MemberEnd())
            return fail(error, "geometer.contract.missing_field", child_path(path, "tessellation"),
                        "Required field is missing.");
        if (!decode_TessellationOptions(member->value, &out->tessellation,
                                        child_path(path, "tessellation"), error))
            return false;
    }
    return true;
}

bool write_StepTopologyRenderRequestA0(rapidjson::Writer<rapidjson::StringBuffer>& writer,
                                       const StepTopologyRenderRequestA0& value,
                                       ContractError* error)
{
    writer.StartObject();
    writer.Key("schema");
    if (!write_literal_string(writer, value.schema, error,
                              "geometry.step_topology.render.request.a0"))
        return false;
    writer.Key("session");
    if (!write_SessionReference(writer, value.session, error))
        return false;
    writer.Key("tessellation");
    if (!write_TessellationOptions(writer, value.tessellation, error))
        return false;
    writer.EndObject();
    return true;
}

bool decode_StepTopologyResolveHitRequestA0(const rapidjson::Value& value,
                                            StepTopologyResolveHitRequestA0* out,
                                            const std::string& path, ContractError* error)
{
    static const char* const names[] = {
        "schema",         "session",         "artifact_handle",          "content_sha256",
        "instance_index", "primitive_index", "primitive_triangle_index", "occurrence_handle",
        "body_handle",    "face_handle"};
    if (!validate_object(value, names, 10U, path, error))
        return false;
    {
        const auto member = value.FindMember("schema");
        if (member == value.MemberEnd())
            return fail(error, "geometer.contract.missing_field", child_path(path, "schema"),
                        "Required field is missing.");
        if (!decode_literal_string(member->value, &out->schema, child_path(path, "schema"), error,
                                   "geometry.step_topology.resolve_hit.request.a0"))
            return false;
    }
    {
        const auto member = value.FindMember("session");
        if (member == value.MemberEnd())
            return fail(error, "geometer.contract.missing_field", child_path(path, "session"),
                        "Required field is missing.");
        if (!decode_SessionReference(member->value, &out->session, child_path(path, "session"),
                                     error))
            return false;
    }
    {
        const auto member = value.FindMember("artifact_handle");
        if (member == value.MemberEnd())
            return fail(error, "geometer.contract.missing_field",
                        child_path(path, "artifact_handle"), "Required field is missing.");
        if (!decode_string(member->value, &out->artifact_handle,
                           child_path(path, "artifact_handle"), error, 68U, 68U))
            return false;
    }
    {
        const auto member = value.FindMember("content_sha256");
        if (member == value.MemberEnd())
            return fail(error, "geometer.contract.missing_field",
                        child_path(path, "content_sha256"), "Required field is missing.");
        if (!decode_string(member->value, &out->content_sha256, child_path(path, "content_sha256"),
                           error, 64U, 64U))
            return false;
    }
    {
        const auto member = value.FindMember("instance_index");
        if (member == value.MemberEnd())
            return fail(error, "geometer.contract.missing_field",
                        child_path(path, "instance_index"), "Required field is missing.");
        if (!decode_uint32(member->value, &out->instance_index, child_path(path, "instance_index"),
                           error, 0ULL, 99999ULL))
            return false;
    }
    {
        const auto member = value.FindMember("primitive_index");
        if (member == value.MemberEnd())
            return fail(error, "geometer.contract.missing_field",
                        child_path(path, "primitive_index"), "Required field is missing.");
        if (!decode_uint32(member->value, &out->primitive_index,
                           child_path(path, "primitive_index"), error, 0ULL, 999999ULL))
            return false;
    }
    {
        const auto member = value.FindMember("primitive_triangle_index");
        if (member == value.MemberEnd())
            return fail(error, "geometer.contract.missing_field",
                        child_path(path, "primitive_triangle_index"), "Required field is missing.");
        if (!decode_uint32(member->value, &out->primitive_triangle_index,
                           child_path(path, "primitive_triangle_index"), error, 0ULL, 9999999ULL))
            return false;
    }
    {
        const auto member = value.FindMember("occurrence_handle");
        if (member == value.MemberEnd())
            return fail(error, "geometer.contract.missing_field",
                        child_path(path, "occurrence_handle"), "Required field is missing.");
        if (!decode_string(member->value, &out->occurrence_handle,
                           child_path(path, "occurrence_handle"), error, 68U, 68U))
            return false;
    }
    {
        const auto member = value.FindMember("body_handle");
        if (member == value.MemberEnd())
            return fail(error, "geometer.contract.missing_field", child_path(path, "body_handle"),
                        "Required field is missing.");
        if (!decode_string(member->value, &out->body_handle, child_path(path, "body_handle"), error,
                           68U, 68U))
            return false;
    }
    {
        const auto member = value.FindMember("face_handle");
        if (member == value.MemberEnd())
            return fail(error, "geometer.contract.missing_field", child_path(path, "face_handle"),
                        "Required field is missing.");
        if (!decode_string(member->value, &out->face_handle, child_path(path, "face_handle"), error,
                           68U, 68U))
            return false;
    }
    return true;
}

bool write_StepTopologyResolveHitRequestA0(rapidjson::Writer<rapidjson::StringBuffer>& writer,
                                           const StepTopologyResolveHitRequestA0& value,
                                           ContractError* error)
{
    writer.StartObject();
    writer.Key("schema");
    if (!write_literal_string(writer, value.schema, error,
                              "geometry.step_topology.resolve_hit.request.a0"))
        return false;
    writer.Key("session");
    if (!write_SessionReference(writer, value.session, error))
        return false;
    writer.Key("artifact_handle");
    if (!write_string(writer, value.artifact_handle, error, 68U, 68U))
        return false;
    writer.Key("content_sha256");
    if (!write_string(writer, value.content_sha256, error, 64U, 64U))
        return false;
    writer.Key("instance_index");
    if (!write_uint32(writer, value.instance_index, error, 0ULL, 99999ULL))
        return false;
    writer.Key("primitive_index");
    if (!write_uint32(writer, value.primitive_index, error, 0ULL, 999999ULL))
        return false;
    writer.Key("primitive_triangle_index");
    if (!write_uint32(writer, value.primitive_triangle_index, error, 0ULL, 9999999ULL))
        return false;
    writer.Key("occurrence_handle");
    if (!write_string(writer, value.occurrence_handle, error, 68U, 68U))
        return false;
    writer.Key("body_handle");
    if (!write_string(writer, value.body_handle, error, 68U, 68U))
        return false;
    writer.Key("face_handle");
    if (!write_string(writer, value.face_handle, error, 68U, 68U))
        return false;
    writer.EndObject();
    return true;
}

bool decode_CreateLogicalGroupCommand(const rapidjson::Value& value, CreateLogicalGroupCommand* out,
                                      const std::string& path, ContractError* error)
{
    static const char* const names[] = {"kind", "authored_id", "name", "member_handles"};
    if (!validate_object(value, names, 4U, path, error))
        return false;
    {
        const auto member = value.FindMember("kind");
        if (member == value.MemberEnd())
            return fail(error, "geometer.contract.missing_field", child_path(path, "kind"),
                        "Required field is missing.");
        if (!decode_literal_string(member->value, &out->kind, child_path(path, "kind"), error,
                                   "create"))
            return false;
    }
    {
        const auto member = value.FindMember("authored_id");
        if (member == value.MemberEnd())
            return fail(error, "geometer.contract.missing_field", child_path(path, "authored_id"),
                        "Required field is missing.");
        if (!decode_string(member->value, &out->authored_id, child_path(path, "authored_id"), error,
                           28U, 128U))
            return false;
    }
    {
        const auto member = value.FindMember("name");
        if (member == value.MemberEnd())
            return fail(error, "geometer.contract.missing_field", child_path(path, "name"),
                        "Required field is missing.");
        if (!decode_string(member->value, &out->name, child_path(path, "name"), error, 1U, 4096U))
            return false;
    }
    {
        const auto member = value.FindMember("member_handles");
        if (member == value.MemberEnd())
            return fail(error, "geometer.contract.missing_field",
                        child_path(path, "member_handles"), "Required field is missing.");
        if (!decode_array(member->value, &out->member_handles, child_path(path, "member_handles"),
                          error, 1U, 100000U, decode_string_item))
            return false;
    }
    return true;
}

bool write_CreateLogicalGroupCommand(rapidjson::Writer<rapidjson::StringBuffer>& writer,
                                     const CreateLogicalGroupCommand& value, ContractError* error)
{
    writer.StartObject();
    writer.Key("kind");
    if (!write_literal_string(writer, value.kind, error, "create"))
        return false;
    writer.Key("authored_id");
    if (!write_string(writer, value.authored_id, error, 28U, 128U))
        return false;
    writer.Key("name");
    if (!write_string(writer, value.name, error, 1U, 4096U))
        return false;
    writer.Key("member_handles");
    if (!write_array(writer, value.member_handles, error, 1U, 100000U, write_string_item))
        return false;
    writer.EndObject();
    return true;
}

bool decode_RenameLogicalGroupCommand(const rapidjson::Value& value, RenameLogicalGroupCommand* out,
                                      const std::string& path, ContractError* error)
{
    static const char* const names[] = {"kind", "authored_id", "expected_revision", "name"};
    if (!validate_object(value, names, 4U, path, error))
        return false;
    {
        const auto member = value.FindMember("kind");
        if (member == value.MemberEnd())
            return fail(error, "geometer.contract.missing_field", child_path(path, "kind"),
                        "Required field is missing.");
        if (!decode_literal_string(member->value, &out->kind, child_path(path, "kind"), error,
                                   "rename"))
            return false;
    }
    {
        const auto member = value.FindMember("authored_id");
        if (member == value.MemberEnd())
            return fail(error, "geometer.contract.missing_field", child_path(path, "authored_id"),
                        "Required field is missing.");
        if (!decode_string(member->value, &out->authored_id, child_path(path, "authored_id"), error,
                           28U, 128U))
            return false;
    }
    {
        const auto member = value.FindMember("expected_revision");
        if (member == value.MemberEnd())
            return fail(error, "geometer.contract.missing_field",
                        child_path(path, "expected_revision"), "Required field is missing.");
        if (!decode_uint32(member->value, &out->expected_revision,
                           child_path(path, "expected_revision"), error, 1ULL,
                           std::numeric_limits<std::uint32_t>::max()))
            return false;
    }
    {
        const auto member = value.FindMember("name");
        if (member == value.MemberEnd())
            return fail(error, "geometer.contract.missing_field", child_path(path, "name"),
                        "Required field is missing.");
        if (!decode_string(member->value, &out->name, child_path(path, "name"), error, 1U, 4096U))
            return false;
    }
    return true;
}

bool write_RenameLogicalGroupCommand(rapidjson::Writer<rapidjson::StringBuffer>& writer,
                                     const RenameLogicalGroupCommand& value, ContractError* error)
{
    writer.StartObject();
    writer.Key("kind");
    if (!write_literal_string(writer, value.kind, error, "rename"))
        return false;
    writer.Key("authored_id");
    if (!write_string(writer, value.authored_id, error, 28U, 128U))
        return false;
    writer.Key("expected_revision");
    if (!write_uint32(writer, value.expected_revision, error, 1ULL,
                      std::numeric_limits<std::uint32_t>::max()))
        return false;
    writer.Key("name");
    if (!write_string(writer, value.name, error, 1U, 4096U))
        return false;
    writer.EndObject();
    return true;
}

bool decode_ReplaceLogicalGroupMembersCommand(const rapidjson::Value& value,
                                              ReplaceLogicalGroupMembersCommand* out,
                                              const std::string& path, ContractError* error)
{
    static const char* const names[] = {"kind", "authored_id", "expected_revision",
                                        "member_handles"};
    if (!validate_object(value, names, 4U, path, error))
        return false;
    {
        const auto member = value.FindMember("kind");
        if (member == value.MemberEnd())
            return fail(error, "geometer.contract.missing_field", child_path(path, "kind"),
                        "Required field is missing.");
        if (!decode_literal_string(member->value, &out->kind, child_path(path, "kind"), error,
                                   "replace_members"))
            return false;
    }
    {
        const auto member = value.FindMember("authored_id");
        if (member == value.MemberEnd())
            return fail(error, "geometer.contract.missing_field", child_path(path, "authored_id"),
                        "Required field is missing.");
        if (!decode_string(member->value, &out->authored_id, child_path(path, "authored_id"), error,
                           28U, 128U))
            return false;
    }
    {
        const auto member = value.FindMember("expected_revision");
        if (member == value.MemberEnd())
            return fail(error, "geometer.contract.missing_field",
                        child_path(path, "expected_revision"), "Required field is missing.");
        if (!decode_uint32(member->value, &out->expected_revision,
                           child_path(path, "expected_revision"), error, 1ULL,
                           std::numeric_limits<std::uint32_t>::max()))
            return false;
    }
    {
        const auto member = value.FindMember("member_handles");
        if (member == value.MemberEnd())
            return fail(error, "geometer.contract.missing_field",
                        child_path(path, "member_handles"), "Required field is missing.");
        if (!decode_array(member->value, &out->member_handles, child_path(path, "member_handles"),
                          error, 1U, 100000U, decode_string_item))
            return false;
    }
    return true;
}

bool write_ReplaceLogicalGroupMembersCommand(rapidjson::Writer<rapidjson::StringBuffer>& writer,
                                             const ReplaceLogicalGroupMembersCommand& value,
                                             ContractError* error)
{
    writer.StartObject();
    writer.Key("kind");
    if (!write_literal_string(writer, value.kind, error, "replace_members"))
        return false;
    writer.Key("authored_id");
    if (!write_string(writer, value.authored_id, error, 28U, 128U))
        return false;
    writer.Key("expected_revision");
    if (!write_uint32(writer, value.expected_revision, error, 1ULL,
                      std::numeric_limits<std::uint32_t>::max()))
        return false;
    writer.Key("member_handles");
    if (!write_array(writer, value.member_handles, error, 1U, 100000U, write_string_item))
        return false;
    writer.EndObject();
    return true;
}

bool decode_EraseLogicalGroupCommand(const rapidjson::Value& value, EraseLogicalGroupCommand* out,
                                     const std::string& path, ContractError* error)
{
    static const char* const names[] = {"kind", "authored_id", "expected_revision"};
    if (!validate_object(value, names, 3U, path, error))
        return false;
    {
        const auto member = value.FindMember("kind");
        if (member == value.MemberEnd())
            return fail(error, "geometer.contract.missing_field", child_path(path, "kind"),
                        "Required field is missing.");
        if (!decode_literal_string(member->value, &out->kind, child_path(path, "kind"), error,
                                   "erase"))
            return false;
    }
    {
        const auto member = value.FindMember("authored_id");
        if (member == value.MemberEnd())
            return fail(error, "geometer.contract.missing_field", child_path(path, "authored_id"),
                        "Required field is missing.");
        if (!decode_string(member->value, &out->authored_id, child_path(path, "authored_id"), error,
                           28U, 128U))
            return false;
    }
    {
        const auto member = value.FindMember("expected_revision");
        if (member == value.MemberEnd())
            return fail(error, "geometer.contract.missing_field",
                        child_path(path, "expected_revision"), "Required field is missing.");
        if (!decode_uint32(member->value, &out->expected_revision,
                           child_path(path, "expected_revision"), error, 1ULL,
                           std::numeric_limits<std::uint32_t>::max()))
            return false;
    }
    return true;
}

bool write_EraseLogicalGroupCommand(rapidjson::Writer<rapidjson::StringBuffer>& writer,
                                    const EraseLogicalGroupCommand& value, ContractError* error)
{
    writer.StartObject();
    writer.Key("kind");
    if (!write_literal_string(writer, value.kind, error, "erase"))
        return false;
    writer.Key("authored_id");
    if (!write_string(writer, value.authored_id, error, 28U, 128U))
        return false;
    writer.Key("expected_revision");
    if (!write_uint32(writer, value.expected_revision, error, 1ULL,
                      std::numeric_limits<std::uint32_t>::max()))
        return false;
    writer.EndObject();
    return true;
}

bool decode_LogicalGroupCommand(const rapidjson::Value& value, LogicalGroupCommand* out,
                                const std::string& path, ContractError* error)
{
    int matches = 0;
    LogicalGroupCommand selected{};
    {
        CreateLogicalGroupCommand candidate{};
        ContractError ignored;
        if (decode_CreateLogicalGroupCommand(value, &candidate, path, &ignored))
        {
            ++matches;
            selected = LogicalGroupCommand(std::in_place_index<0>, std::move(candidate));
        }
    }
    {
        RenameLogicalGroupCommand candidate{};
        ContractError ignored;
        if (decode_RenameLogicalGroupCommand(value, &candidate, path, &ignored))
        {
            ++matches;
            selected = LogicalGroupCommand(std::in_place_index<1>, std::move(candidate));
        }
    }
    {
        ReplaceLogicalGroupMembersCommand candidate{};
        ContractError ignored;
        if (decode_ReplaceLogicalGroupMembersCommand(value, &candidate, path, &ignored))
        {
            ++matches;
            selected = LogicalGroupCommand(std::in_place_index<2>, std::move(candidate));
        }
    }
    {
        EraseLogicalGroupCommand candidate{};
        ContractError ignored;
        if (decode_EraseLogicalGroupCommand(value, &candidate, path, &ignored))
        {
            ++matches;
            selected = LogicalGroupCommand(std::in_place_index<3>, std::move(candidate));
        }
    }
    if (matches != 1)
        return fail(error, "geometer.contract.union_mismatch", path,
                    "Expected exactly one union variant.");
    *out = std::move(selected);
    return true;
}

bool write_LogicalGroupCommand(rapidjson::Writer<rapidjson::StringBuffer>& writer,
                               const LogicalGroupCommand& value, ContractError* error)
{
    switch (value.index())
    {
    case 0:
        return write_CreateLogicalGroupCommand(writer, std::get<0>(value), error);
    case 1:
        return write_RenameLogicalGroupCommand(writer, std::get<1>(value), error);
    case 2:
        return write_ReplaceLogicalGroupMembersCommand(writer, std::get<2>(value), error);
    case 3:
        return write_EraseLogicalGroupCommand(writer, std::get<3>(value), error);
    default:
        return fail(error, "geometer.contract.union_mismatch", "", "Unknown union variant.");
    }
}

bool decode_StepTopologyApplyLogicalGroupsRequestA0(const rapidjson::Value& value,
                                                    StepTopologyApplyLogicalGroupsRequestA0* out,
                                                    const std::string& path, ContractError* error)
{
    static const char* const names[] = {"schema", "session", "commands"};
    if (!validate_object(value, names, 3U, path, error))
        return false;
    {
        const auto member = value.FindMember("schema");
        if (member == value.MemberEnd())
            return fail(error, "geometer.contract.missing_field", child_path(path, "schema"),
                        "Required field is missing.");
        if (!decode_literal_string(member->value, &out->schema, child_path(path, "schema"), error,
                                   "geometry.step_topology.apply_logical_groups.request.a0"))
            return false;
    }
    {
        const auto member = value.FindMember("session");
        if (member == value.MemberEnd())
            return fail(error, "geometer.contract.missing_field", child_path(path, "session"),
                        "Required field is missing.");
        if (!decode_SessionReference(member->value, &out->session, child_path(path, "session"),
                                     error))
            return false;
    }
    {
        const auto member = value.FindMember("commands");
        if (member == value.MemberEnd())
            return fail(error, "geometer.contract.missing_field", child_path(path, "commands"),
                        "Required field is missing.");
        if (!decode_array(member->value, &out->commands, child_path(path, "commands"), error, 1U,
                          10000U, decode_LogicalGroupCommand))
            return false;
    }
    return true;
}

bool write_StepTopologyApplyLogicalGroupsRequestA0(
    rapidjson::Writer<rapidjson::StringBuffer>& writer,
    const StepTopologyApplyLogicalGroupsRequestA0& value, ContractError* error)
{
    writer.StartObject();
    writer.Key("schema");
    if (!write_literal_string(writer, value.schema, error,
                              "geometry.step_topology.apply_logical_groups.request.a0"))
        return false;
    writer.Key("session");
    if (!write_SessionReference(writer, value.session, error))
        return false;
    writer.Key("commands");
    if (!write_array(writer, value.commands, error, 1U, 10000U, write_LogicalGroupCommand))
        return false;
    writer.EndObject();
    return true;
}

bool decode_DocumentProbeTarget(const rapidjson::Value& value, DocumentProbeTarget* out,
                                const std::string& path, ContractError* error)
{
    static const char* const names[] = {"kind"};
    if (!validate_object(value, names, 1U, path, error))
        return false;
    {
        const auto member = value.FindMember("kind");
        if (member == value.MemberEnd())
            return fail(error, "geometer.contract.missing_field", child_path(path, "kind"),
                        "Required field is missing.");
        if (!decode_literal_string(member->value, &out->kind, child_path(path, "kind"), error,
                                   "document"))
            return false;
    }
    return true;
}

bool write_DocumentProbeTarget(rapidjson::Writer<rapidjson::StringBuffer>& writer,
                               const DocumentProbeTarget& value, ContractError* error)
{
    writer.StartObject();
    writer.Key("kind");
    if (!write_literal_string(writer, value.kind, error, "document"))
        return false;
    writer.EndObject();
    return true;
}

bool decode_DefinitionProbeTarget(const rapidjson::Value& value, DefinitionProbeTarget* out,
                                  const std::string& path, ContractError* error)
{
    static const char* const names[] = {"kind", "target_handle"};
    if (!validate_object(value, names, 2U, path, error))
        return false;
    {
        const auto member = value.FindMember("kind");
        if (member == value.MemberEnd())
            return fail(error, "geometer.contract.missing_field", child_path(path, "kind"),
                        "Required field is missing.");
        if (!decode_literal_string(member->value, &out->kind, child_path(path, "kind"), error,
                                   "definition"))
            return false;
    }
    {
        const auto member = value.FindMember("target_handle");
        if (member == value.MemberEnd())
            return fail(error, "geometer.contract.missing_field", child_path(path, "target_handle"),
                        "Required field is missing.");
        if (!decode_string(member->value, &out->target_handle, child_path(path, "target_handle"),
                           error, 68U, 68U))
            return false;
    }
    return true;
}

bool write_DefinitionProbeTarget(rapidjson::Writer<rapidjson::StringBuffer>& writer,
                                 const DefinitionProbeTarget& value, ContractError* error)
{
    writer.StartObject();
    writer.Key("kind");
    if (!write_literal_string(writer, value.kind, error, "definition"))
        return false;
    writer.Key("target_handle");
    if (!write_string(writer, value.target_handle, error, 68U, 68U))
        return false;
    writer.EndObject();
    return true;
}

bool decode_RootOccurrenceProbeTarget(const rapidjson::Value& value, RootOccurrenceProbeTarget* out,
                                      const std::string& path, ContractError* error)
{
    static const char* const names[] = {"kind", "target_handle"};
    if (!validate_object(value, names, 2U, path, error))
        return false;
    {
        const auto member = value.FindMember("kind");
        if (member == value.MemberEnd())
            return fail(error, "geometer.contract.missing_field", child_path(path, "kind"),
                        "Required field is missing.");
        if (!decode_literal_string(member->value, &out->kind, child_path(path, "kind"), error,
                                   "root_occurrence"))
            return false;
    }
    {
        const auto member = value.FindMember("target_handle");
        if (member == value.MemberEnd())
            return fail(error, "geometer.contract.missing_field", child_path(path, "target_handle"),
                        "Required field is missing.");
        if (!decode_string(member->value, &out->target_handle, child_path(path, "target_handle"),
                           error, 68U, 68U))
            return false;
    }
    return true;
}

bool write_RootOccurrenceProbeTarget(rapidjson::Writer<rapidjson::StringBuffer>& writer,
                                     const RootOccurrenceProbeTarget& value, ContractError* error)
{
    writer.StartObject();
    writer.Key("kind");
    if (!write_literal_string(writer, value.kind, error, "root_occurrence"))
        return false;
    writer.Key("target_handle");
    if (!write_string(writer, value.target_handle, error, 68U, 68U))
        return false;
    writer.EndObject();
    return true;
}

bool decode_ComponentOccurrenceProbeTarget(const rapidjson::Value& value,
                                           ComponentOccurrenceProbeTarget* out,
                                           const std::string& path, ContractError* error)
{
    static const char* const names[] = {"kind", "target_handle"};
    if (!validate_object(value, names, 2U, path, error))
        return false;
    {
        const auto member = value.FindMember("kind");
        if (member == value.MemberEnd())
            return fail(error, "geometer.contract.missing_field", child_path(path, "kind"),
                        "Required field is missing.");
        if (!decode_literal_string(member->value, &out->kind, child_path(path, "kind"), error,
                                   "occurrence"))
            return false;
    }
    {
        const auto member = value.FindMember("target_handle");
        if (member == value.MemberEnd())
            return fail(error, "geometer.contract.missing_field", child_path(path, "target_handle"),
                        "Required field is missing.");
        if (!decode_string(member->value, &out->target_handle, child_path(path, "target_handle"),
                           error, 68U, 68U))
            return false;
    }
    return true;
}

bool write_ComponentOccurrenceProbeTarget(rapidjson::Writer<rapidjson::StringBuffer>& writer,
                                          const ComponentOccurrenceProbeTarget& value,
                                          ContractError* error)
{
    writer.StartObject();
    writer.Key("kind");
    if (!write_literal_string(writer, value.kind, error, "occurrence"))
        return false;
    writer.Key("target_handle");
    if (!write_string(writer, value.target_handle, error, 68U, 68U))
        return false;
    writer.EndObject();
    return true;
}

bool decode_BodyProbeTarget(const rapidjson::Value& value, BodyProbeTarget* out,
                            const std::string& path, ContractError* error)
{
    static const char* const names[] = {"kind", "target_handle"};
    if (!validate_object(value, names, 2U, path, error))
        return false;
    {
        const auto member = value.FindMember("kind");
        if (member == value.MemberEnd())
            return fail(error, "geometer.contract.missing_field", child_path(path, "kind"),
                        "Required field is missing.");
        if (!decode_literal_string(member->value, &out->kind, child_path(path, "kind"), error,
                                   "body"))
            return false;
    }
    {
        const auto member = value.FindMember("target_handle");
        if (member == value.MemberEnd())
            return fail(error, "geometer.contract.missing_field", child_path(path, "target_handle"),
                        "Required field is missing.");
        if (!decode_string(member->value, &out->target_handle, child_path(path, "target_handle"),
                           error, 68U, 68U))
            return false;
    }
    return true;
}

bool write_BodyProbeTarget(rapidjson::Writer<rapidjson::StringBuffer>& writer,
                           const BodyProbeTarget& value, ContractError* error)
{
    writer.StartObject();
    writer.Key("kind");
    if (!write_literal_string(writer, value.kind, error, "body"))
        return false;
    writer.Key("target_handle");
    if (!write_string(writer, value.target_handle, error, 68U, 68U))
        return false;
    writer.EndObject();
    return true;
}

bool decode_FaceProbeTarget(const rapidjson::Value& value, FaceProbeTarget* out,
                            const std::string& path, ContractError* error)
{
    static const char* const names[] = {"kind", "target_handle"};
    if (!validate_object(value, names, 2U, path, error))
        return false;
    {
        const auto member = value.FindMember("kind");
        if (member == value.MemberEnd())
            return fail(error, "geometer.contract.missing_field", child_path(path, "kind"),
                        "Required field is missing.");
        if (!decode_literal_string(member->value, &out->kind, child_path(path, "kind"), error,
                                   "face"))
            return false;
    }
    {
        const auto member = value.FindMember("target_handle");
        if (member == value.MemberEnd())
            return fail(error, "geometer.contract.missing_field", child_path(path, "target_handle"),
                        "Required field is missing.");
        if (!decode_string(member->value, &out->target_handle, child_path(path, "target_handle"),
                           error, 68U, 68U))
            return false;
    }
    return true;
}

bool write_FaceProbeTarget(rapidjson::Writer<rapidjson::StringBuffer>& writer,
                           const FaceProbeTarget& value, ContractError* error)
{
    writer.StartObject();
    writer.Key("kind");
    if (!write_literal_string(writer, value.kind, error, "face"))
        return false;
    writer.Key("target_handle");
    if (!write_string(writer, value.target_handle, error, 68U, 68U))
        return false;
    writer.EndObject();
    return true;
}

bool decode_LogicalGroupProbeTarget(const rapidjson::Value& value, LogicalGroupProbeTarget* out,
                                    const std::string& path, ContractError* error)
{
    static const char* const names[] = {"kind", "group_authored_id"};
    if (!validate_object(value, names, 2U, path, error))
        return false;
    {
        const auto member = value.FindMember("kind");
        if (member == value.MemberEnd())
            return fail(error, "geometer.contract.missing_field", child_path(path, "kind"),
                        "Required field is missing.");
        if (!decode_literal_string(member->value, &out->kind, child_path(path, "kind"), error,
                                   "logical_group"))
            return false;
    }
    {
        const auto member = value.FindMember("group_authored_id");
        if (member == value.MemberEnd())
            return fail(error, "geometer.contract.missing_field",
                        child_path(path, "group_authored_id"), "Required field is missing.");
        if (!decode_string(member->value, &out->group_authored_id,
                           child_path(path, "group_authored_id"), error, 28U, 128U))
            return false;
    }
    return true;
}

bool write_LogicalGroupProbeTarget(rapidjson::Writer<rapidjson::StringBuffer>& writer,
                                   const LogicalGroupProbeTarget& value, ContractError* error)
{
    writer.StartObject();
    writer.Key("kind");
    if (!write_literal_string(writer, value.kind, error, "logical_group"))
        return false;
    writer.Key("group_authored_id");
    if (!write_string(writer, value.group_authored_id, error, 28U, 128U))
        return false;
    writer.EndObject();
    return true;
}

bool decode_MetadataProbeTarget(const rapidjson::Value& value, MetadataProbeTarget* out,
                                const std::string& path, ContractError* error)
{
    int matches = 0;
    MetadataProbeTarget selected{};
    {
        DocumentProbeTarget candidate{};
        ContractError ignored;
        if (decode_DocumentProbeTarget(value, &candidate, path, &ignored))
        {
            ++matches;
            selected = MetadataProbeTarget(std::in_place_index<0>, std::move(candidate));
        }
    }
    {
        DefinitionProbeTarget candidate{};
        ContractError ignored;
        if (decode_DefinitionProbeTarget(value, &candidate, path, &ignored))
        {
            ++matches;
            selected = MetadataProbeTarget(std::in_place_index<1>, std::move(candidate));
        }
    }
    {
        RootOccurrenceProbeTarget candidate{};
        ContractError ignored;
        if (decode_RootOccurrenceProbeTarget(value, &candidate, path, &ignored))
        {
            ++matches;
            selected = MetadataProbeTarget(std::in_place_index<2>, std::move(candidate));
        }
    }
    {
        ComponentOccurrenceProbeTarget candidate{};
        ContractError ignored;
        if (decode_ComponentOccurrenceProbeTarget(value, &candidate, path, &ignored))
        {
            ++matches;
            selected = MetadataProbeTarget(std::in_place_index<3>, std::move(candidate));
        }
    }
    {
        BodyProbeTarget candidate{};
        ContractError ignored;
        if (decode_BodyProbeTarget(value, &candidate, path, &ignored))
        {
            ++matches;
            selected = MetadataProbeTarget(std::in_place_index<4>, std::move(candidate));
        }
    }
    {
        FaceProbeTarget candidate{};
        ContractError ignored;
        if (decode_FaceProbeTarget(value, &candidate, path, &ignored))
        {
            ++matches;
            selected = MetadataProbeTarget(std::in_place_index<5>, std::move(candidate));
        }
    }
    {
        LogicalGroupProbeTarget candidate{};
        ContractError ignored;
        if (decode_LogicalGroupProbeTarget(value, &candidate, path, &ignored))
        {
            ++matches;
            selected = MetadataProbeTarget(std::in_place_index<6>, std::move(candidate));
        }
    }
    if (matches != 1)
        return fail(error, "geometer.contract.union_mismatch", path,
                    "Expected exactly one union variant.");
    *out = std::move(selected);
    return true;
}

bool write_MetadataProbeTarget(rapidjson::Writer<rapidjson::StringBuffer>& writer,
                               const MetadataProbeTarget& value, ContractError* error)
{
    switch (value.index())
    {
    case 0:
        return write_DocumentProbeTarget(writer, std::get<0>(value), error);
    case 1:
        return write_DefinitionProbeTarget(writer, std::get<1>(value), error);
    case 2:
        return write_RootOccurrenceProbeTarget(writer, std::get<2>(value), error);
    case 3:
        return write_ComponentOccurrenceProbeTarget(writer, std::get<3>(value), error);
    case 4:
        return write_BodyProbeTarget(writer, std::get<4>(value), error);
    case 5:
        return write_FaceProbeTarget(writer, std::get<5>(value), error);
    case 6:
        return write_LogicalGroupProbeTarget(writer, std::get<6>(value), error);
    default:
        return fail(error, "geometer.contract.union_mismatch", "", "Unknown union variant.");
    }
}

bool decode_AttachMetadataProbeCommand(const rapidjson::Value& value,
                                       AttachMetadataProbeCommand* out, const std::string& path,
                                       ContractError* error)
{
    static const char* const names[] = {"kind", "authored_id", "target", "key", "value"};
    if (!validate_object(value, names, 5U, path, error))
        return false;
    {
        const auto member = value.FindMember("kind");
        if (member == value.MemberEnd())
            return fail(error, "geometer.contract.missing_field", child_path(path, "kind"),
                        "Required field is missing.");
        if (!decode_literal_string(member->value, &out->kind, child_path(path, "kind"), error,
                                   "attach"))
            return false;
    }
    {
        const auto member = value.FindMember("authored_id");
        if (member == value.MemberEnd())
            return fail(error, "geometer.contract.missing_field", child_path(path, "authored_id"),
                        "Required field is missing.");
        if (!decode_string(member->value, &out->authored_id, child_path(path, "authored_id"), error,
                           28U, 128U))
            return false;
    }
    {
        const auto member = value.FindMember("target");
        if (member == value.MemberEnd())
            return fail(error, "geometer.contract.missing_field", child_path(path, "target"),
                        "Required field is missing.");
        if (!decode_MetadataProbeTarget(member->value, &out->target, child_path(path, "target"),
                                        error))
            return false;
    }
    {
        const auto member = value.FindMember("key");
        if (member == value.MemberEnd())
            return fail(error, "geometer.contract.missing_field", child_path(path, "key"),
                        "Required field is missing.");
        if (!decode_string(member->value, &out->key, child_path(path, "key"), error, 32U, 128U))
            return false;
    }
    {
        const auto member = value.FindMember("value");
        if (member == value.MemberEnd())
            return fail(error, "geometer.contract.missing_field", child_path(path, "value"),
                        "Required field is missing.");
        if (!decode_string(member->value, &out->value, child_path(path, "value"), error, 1U, 4096U))
            return false;
    }
    return true;
}

bool write_AttachMetadataProbeCommand(rapidjson::Writer<rapidjson::StringBuffer>& writer,
                                      const AttachMetadataProbeCommand& value, ContractError* error)
{
    writer.StartObject();
    writer.Key("kind");
    if (!write_literal_string(writer, value.kind, error, "attach"))
        return false;
    writer.Key("authored_id");
    if (!write_string(writer, value.authored_id, error, 28U, 128U))
        return false;
    writer.Key("target");
    if (!write_MetadataProbeTarget(writer, value.target, error))
        return false;
    writer.Key("key");
    if (!write_string(writer, value.key, error, 32U, 128U))
        return false;
    writer.Key("value");
    if (!write_string(writer, value.value, error, 1U, 4096U))
        return false;
    writer.EndObject();
    return true;
}

bool decode_ReplaceMetadataProbeCommand(const rapidjson::Value& value,
                                        ReplaceMetadataProbeCommand* out, const std::string& path,
                                        ContractError* error)
{
    static const char* const names[] = {"kind",   "authored_id", "expected_revision",
                                        "target", "key",         "value"};
    if (!validate_object(value, names, 6U, path, error))
        return false;
    {
        const auto member = value.FindMember("kind");
        if (member == value.MemberEnd())
            return fail(error, "geometer.contract.missing_field", child_path(path, "kind"),
                        "Required field is missing.");
        if (!decode_literal_string(member->value, &out->kind, child_path(path, "kind"), error,
                                   "replace"))
            return false;
    }
    {
        const auto member = value.FindMember("authored_id");
        if (member == value.MemberEnd())
            return fail(error, "geometer.contract.missing_field", child_path(path, "authored_id"),
                        "Required field is missing.");
        if (!decode_string(member->value, &out->authored_id, child_path(path, "authored_id"), error,
                           28U, 128U))
            return false;
    }
    {
        const auto member = value.FindMember("expected_revision");
        if (member == value.MemberEnd())
            return fail(error, "geometer.contract.missing_field",
                        child_path(path, "expected_revision"), "Required field is missing.");
        if (!decode_uint32(member->value, &out->expected_revision,
                           child_path(path, "expected_revision"), error, 1ULL,
                           std::numeric_limits<std::uint32_t>::max()))
            return false;
    }
    {
        const auto member = value.FindMember("target");
        if (member == value.MemberEnd())
            return fail(error, "geometer.contract.missing_field", child_path(path, "target"),
                        "Required field is missing.");
        if (!decode_MetadataProbeTarget(member->value, &out->target, child_path(path, "target"),
                                        error))
            return false;
    }
    {
        const auto member = value.FindMember("key");
        if (member == value.MemberEnd())
            return fail(error, "geometer.contract.missing_field", child_path(path, "key"),
                        "Required field is missing.");
        if (!decode_string(member->value, &out->key, child_path(path, "key"), error, 32U, 128U))
            return false;
    }
    {
        const auto member = value.FindMember("value");
        if (member == value.MemberEnd())
            return fail(error, "geometer.contract.missing_field", child_path(path, "value"),
                        "Required field is missing.");
        if (!decode_string(member->value, &out->value, child_path(path, "value"), error, 1U, 4096U))
            return false;
    }
    return true;
}

bool write_ReplaceMetadataProbeCommand(rapidjson::Writer<rapidjson::StringBuffer>& writer,
                                       const ReplaceMetadataProbeCommand& value,
                                       ContractError* error)
{
    writer.StartObject();
    writer.Key("kind");
    if (!write_literal_string(writer, value.kind, error, "replace"))
        return false;
    writer.Key("authored_id");
    if (!write_string(writer, value.authored_id, error, 28U, 128U))
        return false;
    writer.Key("expected_revision");
    if (!write_uint32(writer, value.expected_revision, error, 1ULL,
                      std::numeric_limits<std::uint32_t>::max()))
        return false;
    writer.Key("target");
    if (!write_MetadataProbeTarget(writer, value.target, error))
        return false;
    writer.Key("key");
    if (!write_string(writer, value.key, error, 32U, 128U))
        return false;
    writer.Key("value");
    if (!write_string(writer, value.value, error, 1U, 4096U))
        return false;
    writer.EndObject();
    return true;
}

bool decode_EraseMetadataProbeCommand(const rapidjson::Value& value, EraseMetadataProbeCommand* out,
                                      const std::string& path, ContractError* error)
{
    static const char* const names[] = {"kind", "authored_id", "expected_revision"};
    if (!validate_object(value, names, 3U, path, error))
        return false;
    {
        const auto member = value.FindMember("kind");
        if (member == value.MemberEnd())
            return fail(error, "geometer.contract.missing_field", child_path(path, "kind"),
                        "Required field is missing.");
        if (!decode_literal_string(member->value, &out->kind, child_path(path, "kind"), error,
                                   "erase"))
            return false;
    }
    {
        const auto member = value.FindMember("authored_id");
        if (member == value.MemberEnd())
            return fail(error, "geometer.contract.missing_field", child_path(path, "authored_id"),
                        "Required field is missing.");
        if (!decode_string(member->value, &out->authored_id, child_path(path, "authored_id"), error,
                           28U, 128U))
            return false;
    }
    {
        const auto member = value.FindMember("expected_revision");
        if (member == value.MemberEnd())
            return fail(error, "geometer.contract.missing_field",
                        child_path(path, "expected_revision"), "Required field is missing.");
        if (!decode_uint32(member->value, &out->expected_revision,
                           child_path(path, "expected_revision"), error, 1ULL,
                           std::numeric_limits<std::uint32_t>::max()))
            return false;
    }
    return true;
}

bool write_EraseMetadataProbeCommand(rapidjson::Writer<rapidjson::StringBuffer>& writer,
                                     const EraseMetadataProbeCommand& value, ContractError* error)
{
    writer.StartObject();
    writer.Key("kind");
    if (!write_literal_string(writer, value.kind, error, "erase"))
        return false;
    writer.Key("authored_id");
    if (!write_string(writer, value.authored_id, error, 28U, 128U))
        return false;
    writer.Key("expected_revision");
    if (!write_uint32(writer, value.expected_revision, error, 1ULL,
                      std::numeric_limits<std::uint32_t>::max()))
        return false;
    writer.EndObject();
    return true;
}

bool decode_MetadataProbeCommand(const rapidjson::Value& value, MetadataProbeCommand* out,
                                 const std::string& path, ContractError* error)
{
    int matches = 0;
    MetadataProbeCommand selected{};
    {
        AttachMetadataProbeCommand candidate{};
        ContractError ignored;
        if (decode_AttachMetadataProbeCommand(value, &candidate, path, &ignored))
        {
            ++matches;
            selected = MetadataProbeCommand(std::in_place_index<0>, std::move(candidate));
        }
    }
    {
        ReplaceMetadataProbeCommand candidate{};
        ContractError ignored;
        if (decode_ReplaceMetadataProbeCommand(value, &candidate, path, &ignored))
        {
            ++matches;
            selected = MetadataProbeCommand(std::in_place_index<1>, std::move(candidate));
        }
    }
    {
        EraseMetadataProbeCommand candidate{};
        ContractError ignored;
        if (decode_EraseMetadataProbeCommand(value, &candidate, path, &ignored))
        {
            ++matches;
            selected = MetadataProbeCommand(std::in_place_index<2>, std::move(candidate));
        }
    }
    if (matches != 1)
        return fail(error, "geometer.contract.union_mismatch", path,
                    "Expected exactly one union variant.");
    *out = std::move(selected);
    return true;
}

bool write_MetadataProbeCommand(rapidjson::Writer<rapidjson::StringBuffer>& writer,
                                const MetadataProbeCommand& value, ContractError* error)
{
    switch (value.index())
    {
    case 0:
        return write_AttachMetadataProbeCommand(writer, std::get<0>(value), error);
    case 1:
        return write_ReplaceMetadataProbeCommand(writer, std::get<1>(value), error);
    case 2:
        return write_EraseMetadataProbeCommand(writer, std::get<2>(value), error);
    default:
        return fail(error, "geometer.contract.union_mismatch", "", "Unknown union variant.");
    }
}

bool decode_StepTopologyApplyMetadataProbesRequestA0(const rapidjson::Value& value,
                                                     StepTopologyApplyMetadataProbesRequestA0* out,
                                                     const std::string& path, ContractError* error)
{
    static const char* const names[] = {"schema", "session", "commands"};
    if (!validate_object(value, names, 3U, path, error))
        return false;
    {
        const auto member = value.FindMember("schema");
        if (member == value.MemberEnd())
            return fail(error, "geometer.contract.missing_field", child_path(path, "schema"),
                        "Required field is missing.");
        if (!decode_literal_string(member->value, &out->schema, child_path(path, "schema"), error,
                                   "geometry.step_topology.apply_metadata_probes.request.a0"))
            return false;
    }
    {
        const auto member = value.FindMember("session");
        if (member == value.MemberEnd())
            return fail(error, "geometer.contract.missing_field", child_path(path, "session"),
                        "Required field is missing.");
        if (!decode_SessionReference(member->value, &out->session, child_path(path, "session"),
                                     error))
            return false;
    }
    {
        const auto member = value.FindMember("commands");
        if (member == value.MemberEnd())
            return fail(error, "geometer.contract.missing_field", child_path(path, "commands"),
                        "Required field is missing.");
        if (!decode_array(member->value, &out->commands, child_path(path, "commands"), error, 1U,
                          10000U, decode_MetadataProbeCommand))
            return false;
    }
    return true;
}

bool write_StepTopologyApplyMetadataProbesRequestA0(
    rapidjson::Writer<rapidjson::StringBuffer>& writer,
    const StepTopologyApplyMetadataProbesRequestA0& value, ContractError* error)
{
    writer.StartObject();
    writer.Key("schema");
    if (!write_literal_string(writer, value.schema, error,
                              "geometry.step_topology.apply_metadata_probes.request.a0"))
        return false;
    writer.Key("session");
    if (!write_SessionReference(writer, value.session, error))
        return false;
    writer.Key("commands");
    if (!write_array(writer, value.commands, error, 1U, 10000U, write_MetadataProbeCommand))
        return false;
    writer.EndObject();
    return true;
}

bool decode_StepTopologyCheckpointEditJournalRequestA0(
    const rapidjson::Value& value, StepTopologyCheckpointEditJournalRequestA0* out,
    const std::string& path, ContractError* error)
{
    static const char* const names[] = {"schema", "session"};
    if (!validate_object(value, names, 2U, path, error))
        return false;
    {
        const auto member = value.FindMember("schema");
        if (member == value.MemberEnd())
            return fail(error, "geometer.contract.missing_field", child_path(path, "schema"),
                        "Required field is missing.");
        if (!decode_literal_string(member->value, &out->schema, child_path(path, "schema"), error,
                                   "geometry.step_topology.checkpoint_edit_journal.request.a0"))
            return false;
    }
    {
        const auto member = value.FindMember("session");
        if (member == value.MemberEnd())
            return fail(error, "geometer.contract.missing_field", child_path(path, "session"),
                        "Required field is missing.");
        if (!decode_SessionReference(member->value, &out->session, child_path(path, "session"),
                                     error))
            return false;
    }
    return true;
}

bool write_StepTopologyCheckpointEditJournalRequestA0(
    rapidjson::Writer<rapidjson::StringBuffer>& writer,
    const StepTopologyCheckpointEditJournalRequestA0& value, ContractError* error)
{
    writer.StartObject();
    writer.Key("schema");
    if (!write_literal_string(writer, value.schema, error,
                              "geometry.step_topology.checkpoint_edit_journal.request.a0"))
        return false;
    writer.Key("session");
    if (!write_SessionReference(writer, value.session, error))
        return false;
    writer.EndObject();
    return true;
}

bool decode_HierarchySourceKind(const rapidjson::Value& value, HierarchySourceKind* out,
                                const std::string& path, ContractError* error)
{
    if (!value.IsString())
        return fail(error, "geometer.contract.type_mismatch", path, "Expected a string enum.");
    const std::string text(value.GetString(), value.GetStringLength());
    if (text == "definition")
    {
        *out = HierarchySourceKind::definition;
        return true;
    }
    if (text == "body")
    {
        *out = HierarchySourceKind::body;
        return true;
    }
    return fail(error, "geometer.contract.enum_mismatch", path, "Unknown enum value.");
}

bool write_HierarchySourceKind(rapidjson::Writer<rapidjson::StringBuffer>& writer,
                               const HierarchySourceKind& value, ContractError* error)
{
    switch (value)
    {
    case HierarchySourceKind::definition:
        writer.String("definition");
        return true;
    case HierarchySourceKind::body:
        writer.String("body");
        return true;
    }
    return fail(error, "geometer.contract.enum_mismatch", "", "Unknown enum value.");
}

bool decode_CreateHierarchyProductCommand(const rapidjson::Value& value,
                                          CreateHierarchyProductCommand* out,
                                          const std::string& path, ContractError* error)
{
    static const char* const names[] = {"kind", "authored_id", "name", "source_kind",
                                        "source_handle"};
    if (!validate_object(value, names, 5U, path, error))
        return false;
    {
        const auto member = value.FindMember("kind");
        if (member == value.MemberEnd())
            return fail(error, "geometer.contract.missing_field", child_path(path, "kind"),
                        "Required field is missing.");
        if (!decode_literal_string(member->value, &out->kind, child_path(path, "kind"), error,
                                   "create_product"))
            return false;
    }
    {
        const auto member = value.FindMember("authored_id");
        if (member == value.MemberEnd())
            return fail(error, "geometer.contract.missing_field", child_path(path, "authored_id"),
                        "Required field is missing.");
        if (!decode_string(member->value, &out->authored_id, child_path(path, "authored_id"), error,
                           28U, 128U))
            return false;
    }
    {
        const auto member = value.FindMember("name");
        if (member == value.MemberEnd())
            return fail(error, "geometer.contract.missing_field", child_path(path, "name"),
                        "Required field is missing.");
        if (!decode_string(member->value, &out->name, child_path(path, "name"), error, 1U, 4096U))
            return false;
    }
    {
        const auto member = value.FindMember("source_kind");
        if (member == value.MemberEnd())
            return fail(error, "geometer.contract.missing_field", child_path(path, "source_kind"),
                        "Required field is missing.");
        if (!decode_HierarchySourceKind(member->value, &out->source_kind,
                                        child_path(path, "source_kind"), error))
            return false;
    }
    {
        const auto member = value.FindMember("source_handle");
        if (member == value.MemberEnd())
            return fail(error, "geometer.contract.missing_field", child_path(path, "source_handle"),
                        "Required field is missing.");
        if (!decode_string(member->value, &out->source_handle, child_path(path, "source_handle"),
                           error, 68U, 68U))
            return false;
    }
    return true;
}

bool write_CreateHierarchyProductCommand(rapidjson::Writer<rapidjson::StringBuffer>& writer,
                                         const CreateHierarchyProductCommand& value,
                                         ContractError* error)
{
    writer.StartObject();
    writer.Key("kind");
    if (!write_literal_string(writer, value.kind, error, "create_product"))
        return false;
    writer.Key("authored_id");
    if (!write_string(writer, value.authored_id, error, 28U, 128U))
        return false;
    writer.Key("name");
    if (!write_string(writer, value.name, error, 1U, 4096U))
        return false;
    writer.Key("source_kind");
    if (!write_HierarchySourceKind(writer, value.source_kind, error))
        return false;
    writer.Key("source_handle");
    if (!write_string(writer, value.source_handle, error, 68U, 68U))
        return false;
    writer.EndObject();
    return true;
}

bool decode_CreateHierarchyAssemblyCommand(const rapidjson::Value& value,
                                           CreateHierarchyAssemblyCommand* out,
                                           const std::string& path, ContractError* error)
{
    static const char* const names[] = {"kind", "authored_id", "name"};
    if (!validate_object(value, names, 3U, path, error))
        return false;
    {
        const auto member = value.FindMember("kind");
        if (member == value.MemberEnd())
            return fail(error, "geometer.contract.missing_field", child_path(path, "kind"),
                        "Required field is missing.");
        if (!decode_literal_string(member->value, &out->kind, child_path(path, "kind"), error,
                                   "create_assembly"))
            return false;
    }
    {
        const auto member = value.FindMember("authored_id");
        if (member == value.MemberEnd())
            return fail(error, "geometer.contract.missing_field", child_path(path, "authored_id"),
                        "Required field is missing.");
        if (!decode_string(member->value, &out->authored_id, child_path(path, "authored_id"), error,
                           28U, 128U))
            return false;
    }
    {
        const auto member = value.FindMember("name");
        if (member == value.MemberEnd())
            return fail(error, "geometer.contract.missing_field", child_path(path, "name"),
                        "Required field is missing.");
        if (!decode_string(member->value, &out->name, child_path(path, "name"), error, 1U, 4096U))
            return false;
    }
    return true;
}

bool write_CreateHierarchyAssemblyCommand(rapidjson::Writer<rapidjson::StringBuffer>& writer,
                                          const CreateHierarchyAssemblyCommand& value,
                                          ContractError* error)
{
    writer.StartObject();
    writer.Key("kind");
    if (!write_literal_string(writer, value.kind, error, "create_assembly"))
        return false;
    writer.Key("authored_id");
    if (!write_string(writer, value.authored_id, error, 28U, 128U))
        return false;
    writer.Key("name");
    if (!write_string(writer, value.name, error, 1U, 4096U))
        return false;
    writer.EndObject();
    return true;
}

bool decode_CreateHierarchyOccurrenceCommand(const rapidjson::Value& value,
                                             CreateHierarchyOccurrenceCommand* out,
                                             const std::string& path, ContractError* error)
{
    static const char* const names[] = {"kind", "authored_id", "child_authored_id",
                                        "parent_assembly_authored_id", "transform"};
    if (!validate_object(value, names, 5U, path, error))
        return false;
    {
        const auto member = value.FindMember("kind");
        if (member == value.MemberEnd())
            return fail(error, "geometer.contract.missing_field", child_path(path, "kind"),
                        "Required field is missing.");
        if (!decode_literal_string(member->value, &out->kind, child_path(path, "kind"), error,
                                   "create_occurrence"))
            return false;
    }
    {
        const auto member = value.FindMember("authored_id");
        if (member == value.MemberEnd())
            return fail(error, "geometer.contract.missing_field", child_path(path, "authored_id"),
                        "Required field is missing.");
        if (!decode_string(member->value, &out->authored_id, child_path(path, "authored_id"), error,
                           28U, 128U))
            return false;
    }
    {
        const auto member = value.FindMember("child_authored_id");
        if (member == value.MemberEnd())
            return fail(error, "geometer.contract.missing_field",
                        child_path(path, "child_authored_id"), "Required field is missing.");
        if (!decode_string(member->value, &out->child_authored_id,
                           child_path(path, "child_authored_id"), error, 28U, 128U))
            return false;
    }
    {
        const auto member = value.FindMember("parent_assembly_authored_id");
        if (member == value.MemberEnd())
            return fail(error, "geometer.contract.missing_field",
                        child_path(path, "parent_assembly_authored_id"),
                        "Required field is missing.");
        if (!decode_string(member->value, &out->parent_assembly_authored_id,
                           child_path(path, "parent_assembly_authored_id"), error, 28U, 128U))
            return false;
    }
    {
        const auto member = value.FindMember("transform");
        if (member == value.MemberEnd())
            return fail(error, "geometer.contract.missing_field", child_path(path, "transform"),
                        "Required field is missing.");
        if (!decode_array(member->value, &out->transform, child_path(path, "transform"), error, 12U,
                          12U, decode_double_item))
            return false;
    }
    return true;
}

bool write_CreateHierarchyOccurrenceCommand(rapidjson::Writer<rapidjson::StringBuffer>& writer,
                                            const CreateHierarchyOccurrenceCommand& value,
                                            ContractError* error)
{
    writer.StartObject();
    writer.Key("kind");
    if (!write_literal_string(writer, value.kind, error, "create_occurrence"))
        return false;
    writer.Key("authored_id");
    if (!write_string(writer, value.authored_id, error, 28U, 128U))
        return false;
    writer.Key("child_authored_id");
    if (!write_string(writer, value.child_authored_id, error, 28U, 128U))
        return false;
    writer.Key("parent_assembly_authored_id");
    if (!write_string(writer, value.parent_assembly_authored_id, error, 28U, 128U))
        return false;
    writer.Key("transform");
    if (!write_array(writer, value.transform, error, 12U, 12U, write_double_item))
        return false;
    writer.EndObject();
    return true;
}

bool decode_ReparentHierarchyOccurrenceCommand(const rapidjson::Value& value,
                                               ReparentHierarchyOccurrenceCommand* out,
                                               const std::string& path, ContractError* error)
{
    static const char* const names[] = {"kind", "authored_id", "expected_revision",
                                        "parent_assembly_authored_id", "transform"};
    if (!validate_object(value, names, 5U, path, error))
        return false;
    {
        const auto member = value.FindMember("kind");
        if (member == value.MemberEnd())
            return fail(error, "geometer.contract.missing_field", child_path(path, "kind"),
                        "Required field is missing.");
        if (!decode_literal_string(member->value, &out->kind, child_path(path, "kind"), error,
                                   "reparent_occurrence"))
            return false;
    }
    {
        const auto member = value.FindMember("authored_id");
        if (member == value.MemberEnd())
            return fail(error, "geometer.contract.missing_field", child_path(path, "authored_id"),
                        "Required field is missing.");
        if (!decode_string(member->value, &out->authored_id, child_path(path, "authored_id"), error,
                           28U, 128U))
            return false;
    }
    {
        const auto member = value.FindMember("expected_revision");
        if (member == value.MemberEnd())
            return fail(error, "geometer.contract.missing_field",
                        child_path(path, "expected_revision"), "Required field is missing.");
        if (!decode_uint32(member->value, &out->expected_revision,
                           child_path(path, "expected_revision"), error, 1ULL,
                           std::numeric_limits<std::uint32_t>::max()))
            return false;
    }
    {
        const auto member = value.FindMember("parent_assembly_authored_id");
        if (member == value.MemberEnd())
            return fail(error, "geometer.contract.missing_field",
                        child_path(path, "parent_assembly_authored_id"),
                        "Required field is missing.");
        if (!decode_string(member->value, &out->parent_assembly_authored_id,
                           child_path(path, "parent_assembly_authored_id"), error, 28U, 128U))
            return false;
    }
    {
        const auto member = value.FindMember("transform");
        if (member == value.MemberEnd())
            return fail(error, "geometer.contract.missing_field", child_path(path, "transform"),
                        "Required field is missing.");
        if (!decode_array(member->value, &out->transform, child_path(path, "transform"), error, 12U,
                          12U, decode_double_item))
            return false;
    }
    return true;
}

bool write_ReparentHierarchyOccurrenceCommand(rapidjson::Writer<rapidjson::StringBuffer>& writer,
                                              const ReparentHierarchyOccurrenceCommand& value,
                                              ContractError* error)
{
    writer.StartObject();
    writer.Key("kind");
    if (!write_literal_string(writer, value.kind, error, "reparent_occurrence"))
        return false;
    writer.Key("authored_id");
    if (!write_string(writer, value.authored_id, error, 28U, 128U))
        return false;
    writer.Key("expected_revision");
    if (!write_uint32(writer, value.expected_revision, error, 1ULL,
                      std::numeric_limits<std::uint32_t>::max()))
        return false;
    writer.Key("parent_assembly_authored_id");
    if (!write_string(writer, value.parent_assembly_authored_id, error, 28U, 128U))
        return false;
    writer.Key("transform");
    if (!write_array(writer, value.transform, error, 12U, 12U, write_double_item))
        return false;
    writer.EndObject();
    return true;
}

bool decode_RenameHierarchyNodeCommand(const rapidjson::Value& value,
                                       RenameHierarchyNodeCommand* out, const std::string& path,
                                       ContractError* error)
{
    static const char* const names[] = {"kind", "authored_id", "expected_revision", "name"};
    if (!validate_object(value, names, 4U, path, error))
        return false;
    {
        const auto member = value.FindMember("kind");
        if (member == value.MemberEnd())
            return fail(error, "geometer.contract.missing_field", child_path(path, "kind"),
                        "Required field is missing.");
        if (!decode_literal_string(member->value, &out->kind, child_path(path, "kind"), error,
                                   "rename_node"))
            return false;
    }
    {
        const auto member = value.FindMember("authored_id");
        if (member == value.MemberEnd())
            return fail(error, "geometer.contract.missing_field", child_path(path, "authored_id"),
                        "Required field is missing.");
        if (!decode_string(member->value, &out->authored_id, child_path(path, "authored_id"), error,
                           28U, 128U))
            return false;
    }
    {
        const auto member = value.FindMember("expected_revision");
        if (member == value.MemberEnd())
            return fail(error, "geometer.contract.missing_field",
                        child_path(path, "expected_revision"), "Required field is missing.");
        if (!decode_uint32(member->value, &out->expected_revision,
                           child_path(path, "expected_revision"), error, 1ULL,
                           std::numeric_limits<std::uint32_t>::max()))
            return false;
    }
    {
        const auto member = value.FindMember("name");
        if (member == value.MemberEnd())
            return fail(error, "geometer.contract.missing_field", child_path(path, "name"),
                        "Required field is missing.");
        if (!decode_string(member->value, &out->name, child_path(path, "name"), error, 1U, 4096U))
            return false;
    }
    return true;
}

bool write_RenameHierarchyNodeCommand(rapidjson::Writer<rapidjson::StringBuffer>& writer,
                                      const RenameHierarchyNodeCommand& value, ContractError* error)
{
    writer.StartObject();
    writer.Key("kind");
    if (!write_literal_string(writer, value.kind, error, "rename_node"))
        return false;
    writer.Key("authored_id");
    if (!write_string(writer, value.authored_id, error, 28U, 128U))
        return false;
    writer.Key("expected_revision");
    if (!write_uint32(writer, value.expected_revision, error, 1ULL,
                      std::numeric_limits<std::uint32_t>::max()))
        return false;
    writer.Key("name");
    if (!write_string(writer, value.name, error, 1U, 4096U))
        return false;
    writer.EndObject();
    return true;
}

bool decode_EraseHierarchyOccurrenceCommand(const rapidjson::Value& value,
                                            EraseHierarchyOccurrenceCommand* out,
                                            const std::string& path, ContractError* error)
{
    static const char* const names[] = {"kind", "authored_id", "expected_revision"};
    if (!validate_object(value, names, 3U, path, error))
        return false;
    {
        const auto member = value.FindMember("kind");
        if (member == value.MemberEnd())
            return fail(error, "geometer.contract.missing_field", child_path(path, "kind"),
                        "Required field is missing.");
        if (!decode_literal_string(member->value, &out->kind, child_path(path, "kind"), error,
                                   "erase_occurrence"))
            return false;
    }
    {
        const auto member = value.FindMember("authored_id");
        if (member == value.MemberEnd())
            return fail(error, "geometer.contract.missing_field", child_path(path, "authored_id"),
                        "Required field is missing.");
        if (!decode_string(member->value, &out->authored_id, child_path(path, "authored_id"), error,
                           28U, 128U))
            return false;
    }
    {
        const auto member = value.FindMember("expected_revision");
        if (member == value.MemberEnd())
            return fail(error, "geometer.contract.missing_field",
                        child_path(path, "expected_revision"), "Required field is missing.");
        if (!decode_uint32(member->value, &out->expected_revision,
                           child_path(path, "expected_revision"), error, 1ULL,
                           std::numeric_limits<std::uint32_t>::max()))
            return false;
    }
    return true;
}

bool write_EraseHierarchyOccurrenceCommand(rapidjson::Writer<rapidjson::StringBuffer>& writer,
                                           const EraseHierarchyOccurrenceCommand& value,
                                           ContractError* error)
{
    writer.StartObject();
    writer.Key("kind");
    if (!write_literal_string(writer, value.kind, error, "erase_occurrence"))
        return false;
    writer.Key("authored_id");
    if (!write_string(writer, value.authored_id, error, 28U, 128U))
        return false;
    writer.Key("expected_revision");
    if (!write_uint32(writer, value.expected_revision, error, 1ULL,
                      std::numeric_limits<std::uint32_t>::max()))
        return false;
    writer.EndObject();
    return true;
}

bool decode_EraseHierarchyNodeCommand(const rapidjson::Value& value, EraseHierarchyNodeCommand* out,
                                      const std::string& path, ContractError* error)
{
    static const char* const names[] = {"kind", "authored_id", "expected_revision"};
    if (!validate_object(value, names, 3U, path, error))
        return false;
    {
        const auto member = value.FindMember("kind");
        if (member == value.MemberEnd())
            return fail(error, "geometer.contract.missing_field", child_path(path, "kind"),
                        "Required field is missing.");
        if (!decode_literal_string(member->value, &out->kind, child_path(path, "kind"), error,
                                   "erase_node"))
            return false;
    }
    {
        const auto member = value.FindMember("authored_id");
        if (member == value.MemberEnd())
            return fail(error, "geometer.contract.missing_field", child_path(path, "authored_id"),
                        "Required field is missing.");
        if (!decode_string(member->value, &out->authored_id, child_path(path, "authored_id"), error,
                           28U, 128U))
            return false;
    }
    {
        const auto member = value.FindMember("expected_revision");
        if (member == value.MemberEnd())
            return fail(error, "geometer.contract.missing_field",
                        child_path(path, "expected_revision"), "Required field is missing.");
        if (!decode_uint32(member->value, &out->expected_revision,
                           child_path(path, "expected_revision"), error, 1ULL,
                           std::numeric_limits<std::uint32_t>::max()))
            return false;
    }
    return true;
}

bool write_EraseHierarchyNodeCommand(rapidjson::Writer<rapidjson::StringBuffer>& writer,
                                     const EraseHierarchyNodeCommand& value, ContractError* error)
{
    writer.StartObject();
    writer.Key("kind");
    if (!write_literal_string(writer, value.kind, error, "erase_node"))
        return false;
    writer.Key("authored_id");
    if (!write_string(writer, value.authored_id, error, 28U, 128U))
        return false;
    writer.Key("expected_revision");
    if (!write_uint32(writer, value.expected_revision, error, 1ULL,
                      std::numeric_limits<std::uint32_t>::max()))
        return false;
    writer.EndObject();
    return true;
}

bool decode_HierarchyCommand(const rapidjson::Value& value, HierarchyCommand* out,
                             const std::string& path, ContractError* error)
{
    int matches = 0;
    HierarchyCommand selected{};
    {
        CreateHierarchyProductCommand candidate{};
        ContractError ignored;
        if (decode_CreateHierarchyProductCommand(value, &candidate, path, &ignored))
        {
            ++matches;
            selected = HierarchyCommand(std::in_place_index<0>, std::move(candidate));
        }
    }
    {
        CreateHierarchyAssemblyCommand candidate{};
        ContractError ignored;
        if (decode_CreateHierarchyAssemblyCommand(value, &candidate, path, &ignored))
        {
            ++matches;
            selected = HierarchyCommand(std::in_place_index<1>, std::move(candidate));
        }
    }
    {
        CreateHierarchyOccurrenceCommand candidate{};
        ContractError ignored;
        if (decode_CreateHierarchyOccurrenceCommand(value, &candidate, path, &ignored))
        {
            ++matches;
            selected = HierarchyCommand(std::in_place_index<2>, std::move(candidate));
        }
    }
    {
        ReparentHierarchyOccurrenceCommand candidate{};
        ContractError ignored;
        if (decode_ReparentHierarchyOccurrenceCommand(value, &candidate, path, &ignored))
        {
            ++matches;
            selected = HierarchyCommand(std::in_place_index<3>, std::move(candidate));
        }
    }
    {
        RenameHierarchyNodeCommand candidate{};
        ContractError ignored;
        if (decode_RenameHierarchyNodeCommand(value, &candidate, path, &ignored))
        {
            ++matches;
            selected = HierarchyCommand(std::in_place_index<4>, std::move(candidate));
        }
    }
    {
        EraseHierarchyOccurrenceCommand candidate{};
        ContractError ignored;
        if (decode_EraseHierarchyOccurrenceCommand(value, &candidate, path, &ignored))
        {
            ++matches;
            selected = HierarchyCommand(std::in_place_index<5>, std::move(candidate));
        }
    }
    {
        EraseHierarchyNodeCommand candidate{};
        ContractError ignored;
        if (decode_EraseHierarchyNodeCommand(value, &candidate, path, &ignored))
        {
            ++matches;
            selected = HierarchyCommand(std::in_place_index<6>, std::move(candidate));
        }
    }
    if (matches != 1)
        return fail(error, "geometer.contract.union_mismatch", path,
                    "Expected exactly one union variant.");
    *out = std::move(selected);
    return true;
}

bool write_HierarchyCommand(rapidjson::Writer<rapidjson::StringBuffer>& writer,
                            const HierarchyCommand& value, ContractError* error)
{
    switch (value.index())
    {
    case 0:
        return write_CreateHierarchyProductCommand(writer, std::get<0>(value), error);
    case 1:
        return write_CreateHierarchyAssemblyCommand(writer, std::get<1>(value), error);
    case 2:
        return write_CreateHierarchyOccurrenceCommand(writer, std::get<2>(value), error);
    case 3:
        return write_ReparentHierarchyOccurrenceCommand(writer, std::get<3>(value), error);
    case 4:
        return write_RenameHierarchyNodeCommand(writer, std::get<4>(value), error);
    case 5:
        return write_EraseHierarchyOccurrenceCommand(writer, std::get<5>(value), error);
    case 6:
        return write_EraseHierarchyNodeCommand(writer, std::get<6>(value), error);
    default:
        return fail(error, "geometer.contract.union_mismatch", "", "Unknown union variant.");
    }
}

bool decode_StepTopologyApplyHierarchyRequestA0(const rapidjson::Value& value,
                                                StepTopologyApplyHierarchyRequestA0* out,
                                                const std::string& path, ContractError* error)
{
    static const char* const names[] = {"schema", "session", "expected_hierarchy_revision",
                                        "commands"};
    if (!validate_object(value, names, 4U, path, error))
        return false;
    {
        const auto member = value.FindMember("schema");
        if (member == value.MemberEnd())
            return fail(error, "geometer.contract.missing_field", child_path(path, "schema"),
                        "Required field is missing.");
        if (!decode_literal_string(member->value, &out->schema, child_path(path, "schema"), error,
                                   "geometry.step_topology.apply_hierarchy.request.a0"))
            return false;
    }
    {
        const auto member = value.FindMember("session");
        if (member == value.MemberEnd())
            return fail(error, "geometer.contract.missing_field", child_path(path, "session"),
                        "Required field is missing.");
        if (!decode_SessionReference(member->value, &out->session, child_path(path, "session"),
                                     error))
            return false;
    }
    {
        const auto member = value.FindMember("expected_hierarchy_revision");
        if (member == value.MemberEnd())
            return fail(error, "geometer.contract.missing_field",
                        child_path(path, "expected_hierarchy_revision"),
                        "Required field is missing.");
        if (!decode_uint32(member->value, &out->expected_hierarchy_revision,
                           child_path(path, "expected_hierarchy_revision"), error, 0ULL,
                           std::numeric_limits<std::uint32_t>::max()))
            return false;
    }
    {
        const auto member = value.FindMember("commands");
        if (member == value.MemberEnd())
            return fail(error, "geometer.contract.missing_field", child_path(path, "commands"),
                        "Required field is missing.");
        if (!decode_array(member->value, &out->commands, child_path(path, "commands"), error, 1U,
                          10000U, decode_HierarchyCommand))
            return false;
    }
    return true;
}

bool write_StepTopologyApplyHierarchyRequestA0(rapidjson::Writer<rapidjson::StringBuffer>& writer,
                                               const StepTopologyApplyHierarchyRequestA0& value,
                                               ContractError* error)
{
    writer.StartObject();
    writer.Key("schema");
    if (!write_literal_string(writer, value.schema, error,
                              "geometry.step_topology.apply_hierarchy.request.a0"))
        return false;
    writer.Key("session");
    if (!write_SessionReference(writer, value.session, error))
        return false;
    writer.Key("expected_hierarchy_revision");
    if (!write_uint32(writer, value.expected_hierarchy_revision, error, 0ULL,
                      std::numeric_limits<std::uint32_t>::max()))
        return false;
    writer.Key("commands");
    if (!write_array(writer, value.commands, error, 1U, 10000U, write_HierarchyCommand))
        return false;
    writer.EndObject();
    return true;
}

bool decode_SaveCarrier(const rapidjson::Value& value, SaveCarrier* out, const std::string& path,
                        ContractError* error)
{
    if (!value.IsString())
        return fail(error, "geometer.contract.type_mismatch", path, "Expected a string enum.");
    const std::string text(value.GetString(), value.GetStringLength());
    if (text == "xbf")
    {
        *out = SaveCarrier::xbf;
        return true;
    }
    if (text == "xml_xcaf")
    {
        *out = SaveCarrier::xml_xcaf;
        return true;
    }
    if (text == "step_ap242")
    {
        *out = SaveCarrier::step_ap242;
        return true;
    }
    if (text == "json_sidecar")
    {
        *out = SaveCarrier::json_sidecar;
        return true;
    }
    return fail(error, "geometer.contract.enum_mismatch", path, "Unknown enum value.");
}

bool write_SaveCarrier(rapidjson::Writer<rapidjson::StringBuffer>& writer, const SaveCarrier& value,
                       ContractError* error)
{
    switch (value)
    {
    case SaveCarrier::xbf:
        writer.String("xbf");
        return true;
    case SaveCarrier::xml_xcaf:
        writer.String("xml_xcaf");
        return true;
    case SaveCarrier::step_ap242:
        writer.String("step_ap242");
        return true;
    case SaveCarrier::json_sidecar:
        writer.String("json_sidecar");
        return true;
    }
    return fail(error, "geometer.contract.enum_mismatch", "", "Unknown enum value.");
}

bool decode_StepTopologySaveRequestA0(const rapidjson::Value& value, StepTopologySaveRequestA0* out,
                                      const std::string& path, ContractError* error)
{
    static const char* const names[] = {"schema", "session", "carrier", "include_diagnostics"};
    if (!validate_object(value, names, 4U, path, error))
        return false;
    {
        const auto member = value.FindMember("schema");
        if (member == value.MemberEnd())
            return fail(error, "geometer.contract.missing_field", child_path(path, "schema"),
                        "Required field is missing.");
        if (!decode_literal_string(member->value, &out->schema, child_path(path, "schema"), error,
                                   "geometry.step_topology.save.request.a0"))
            return false;
    }
    {
        const auto member = value.FindMember("session");
        if (member == value.MemberEnd())
            return fail(error, "geometer.contract.missing_field", child_path(path, "session"),
                        "Required field is missing.");
        if (!decode_SessionReference(member->value, &out->session, child_path(path, "session"),
                                     error))
            return false;
    }
    {
        const auto member = value.FindMember("carrier");
        if (member == value.MemberEnd())
            return fail(error, "geometer.contract.missing_field", child_path(path, "carrier"),
                        "Required field is missing.");
        if (!decode_SaveCarrier(member->value, &out->carrier, child_path(path, "carrier"), error))
            return false;
    }
    {
        const auto member = value.FindMember("include_diagnostics");
        if (member == value.MemberEnd())
            return fail(error, "geometer.contract.missing_field",
                        child_path(path, "include_diagnostics"), "Required field is missing.");
        if (!decode_boolean(member->value, &out->include_diagnostics,
                            child_path(path, "include_diagnostics"), error))
            return false;
    }
    return true;
}

bool write_StepTopologySaveRequestA0(rapidjson::Writer<rapidjson::StringBuffer>& writer,
                                     const StepTopologySaveRequestA0& value, ContractError* error)
{
    writer.StartObject();
    writer.Key("schema");
    if (!write_literal_string(writer, value.schema, error,
                              "geometry.step_topology.save.request.a0"))
        return false;
    writer.Key("session");
    if (!write_SessionReference(writer, value.session, error))
        return false;
    writer.Key("carrier");
    if (!write_SaveCarrier(writer, value.carrier, error))
        return false;
    writer.Key("include_diagnostics");
    if (!(writer.Bool(value.include_diagnostics), true))
        return false;
    writer.EndObject();
    return true;
}

bool decode_SourceDescriptor(const rapidjson::Value& value, SourceDescriptor* out,
                             const std::string& path, ContractError* error)
{
    static const char* const names[] = {"format", "sha256", "bytes", "normalized_length_unit"};
    if (!validate_object(value, names, 4U, path, error))
        return false;
    {
        const auto member = value.FindMember("format");
        if (member == value.MemberEnd())
            return fail(error, "geometer.contract.missing_field", child_path(path, "format"),
                        "Required field is missing.");
        if (!decode_literal_string(member->value, &out->format, child_path(path, "format"), error,
                                   "step"))
            return false;
    }
    {
        const auto member = value.FindMember("sha256");
        if (member == value.MemberEnd())
            return fail(error, "geometer.contract.missing_field", child_path(path, "sha256"),
                        "Required field is missing.");
        if (!decode_string(member->value, &out->sha256, child_path(path, "sha256"), error, 64U,
                           64U))
            return false;
    }
    {
        const auto member = value.FindMember("bytes");
        if (member == value.MemberEnd())
            return fail(error, "geometer.contract.missing_field", child_path(path, "bytes"),
                        "Required field is missing.");
        if (!decode_uint32(member->value, &out->bytes, child_path(path, "bytes"), error, 1ULL,
                           268435456ULL))
            return false;
    }
    {
        const auto member = value.FindMember("normalized_length_unit");
        if (member == value.MemberEnd())
            return fail(error, "geometer.contract.missing_field",
                        child_path(path, "normalized_length_unit"), "Required field is missing.");
        if (!decode_literal_string(member->value, &out->normalized_length_unit,
                                   child_path(path, "normalized_length_unit"), error, "millimeter"))
            return false;
    }
    return true;
}

bool write_SourceDescriptor(rapidjson::Writer<rapidjson::StringBuffer>& writer,
                            const SourceDescriptor& value, ContractError* error)
{
    writer.StartObject();
    writer.Key("format");
    if (!write_literal_string(writer, value.format, error, "step"))
        return false;
    writer.Key("sha256");
    if (!write_string(writer, value.sha256, error, 64U, 64U))
        return false;
    writer.Key("bytes");
    if (!write_uint32(writer, value.bytes, error, 1ULL, 268435456ULL))
        return false;
    writer.Key("normalized_length_unit");
    if (!write_literal_string(writer, value.normalized_length_unit, error, "millimeter"))
        return false;
    writer.EndObject();
    return true;
}

bool decode_XbfPersistenceArtifact(const rapidjson::Value& value, XbfPersistenceArtifact* out,
                                   const std::string& path, ContractError* error)
{
    static const char* const names[] = {"carrier", "name",  "media_type",
                                        "format",  "bytes", "sha256"};
    if (!validate_object(value, names, 6U, path, error))
        return false;
    {
        const auto member = value.FindMember("carrier");
        if (member == value.MemberEnd())
            return fail(error, "geometer.contract.missing_field", child_path(path, "carrier"),
                        "Required field is missing.");
        if (!decode_literal_string(member->value, &out->carrier, child_path(path, "carrier"), error,
                                   "xbf"))
            return false;
    }
    {
        const auto member = value.FindMember("name");
        if (member == value.MemberEnd())
            return fail(error, "geometer.contract.missing_field", child_path(path, "name"),
                        "Required field is missing.");
        if (!decode_literal_string(member->value, &out->name, child_path(path, "name"), error,
                                   "state_artifact"))
            return false;
    }
    {
        const auto member = value.FindMember("media_type");
        if (member == value.MemberEnd())
            return fail(error, "geometer.contract.missing_field", child_path(path, "media_type"),
                        "Required field is missing.");
        if (!decode_literal_string(member->value, &out->media_type, child_path(path, "media_type"),
                                   error, "application/vnd.opencascade.xbf"))
            return false;
    }
    {
        const auto member = value.FindMember("format");
        if (member == value.MemberEnd())
            return fail(error, "geometer.contract.missing_field", child_path(path, "format"),
                        "Required field is missing.");
        if (!decode_literal_string(member->value, &out->format, child_path(path, "format"), error,
                                   "ocaf-xbf-version-12"))
            return false;
    }
    {
        const auto member = value.FindMember("bytes");
        if (member == value.MemberEnd())
            return fail(error, "geometer.contract.missing_field", child_path(path, "bytes"),
                        "Required field is missing.");
        if (!decode_uint32(member->value, &out->bytes, child_path(path, "bytes"), error, 1ULL,
                           536870912ULL))
            return false;
    }
    {
        const auto member = value.FindMember("sha256");
        if (member == value.MemberEnd())
            return fail(error, "geometer.contract.missing_field", child_path(path, "sha256"),
                        "Required field is missing.");
        if (!decode_string(member->value, &out->sha256, child_path(path, "sha256"), error, 64U,
                           64U))
            return false;
    }
    return true;
}

bool write_XbfPersistenceArtifact(rapidjson::Writer<rapidjson::StringBuffer>& writer,
                                  const XbfPersistenceArtifact& value, ContractError* error)
{
    writer.StartObject();
    writer.Key("carrier");
    if (!write_literal_string(writer, value.carrier, error, "xbf"))
        return false;
    writer.Key("name");
    if (!write_literal_string(writer, value.name, error, "state_artifact"))
        return false;
    writer.Key("media_type");
    if (!write_literal_string(writer, value.media_type, error, "application/vnd.opencascade.xbf"))
        return false;
    writer.Key("format");
    if (!write_literal_string(writer, value.format, error, "ocaf-xbf-version-12"))
        return false;
    writer.Key("bytes");
    if (!write_uint32(writer, value.bytes, error, 1ULL, 536870912ULL))
        return false;
    writer.Key("sha256");
    if (!write_string(writer, value.sha256, error, 64U, 64U))
        return false;
    writer.EndObject();
    return true;
}

bool decode_XmlXcafPersistenceArtifact(const rapidjson::Value& value,
                                       XmlXcafPersistenceArtifact* out, const std::string& path,
                                       ContractError* error)
{
    static const char* const names[] = {"carrier", "name",  "media_type",
                                        "format",  "bytes", "sha256"};
    if (!validate_object(value, names, 6U, path, error))
        return false;
    {
        const auto member = value.FindMember("carrier");
        if (member == value.MemberEnd())
            return fail(error, "geometer.contract.missing_field", child_path(path, "carrier"),
                        "Required field is missing.");
        if (!decode_literal_string(member->value, &out->carrier, child_path(path, "carrier"), error,
                                   "xml_xcaf"))
            return false;
    }
    {
        const auto member = value.FindMember("name");
        if (member == value.MemberEnd())
            return fail(error, "geometer.contract.missing_field", child_path(path, "name"),
                        "Required field is missing.");
        if (!decode_literal_string(member->value, &out->name, child_path(path, "name"), error,
                                   "state_artifact"))
            return false;
    }
    {
        const auto member = value.FindMember("media_type");
        if (member == value.MemberEnd())
            return fail(error, "geometer.contract.missing_field", child_path(path, "media_type"),
                        "Required field is missing.");
        if (!decode_literal_string(member->value, &out->media_type, child_path(path, "media_type"),
                                   error, "application/vnd.opencascade.xml-xcaf"))
            return false;
    }
    {
        const auto member = value.FindMember("format");
        if (member == value.MemberEnd())
            return fail(error, "geometer.contract.missing_field", child_path(path, "format"),
                        "Required field is missing.");
        if (!decode_literal_string(member->value, &out->format, child_path(path, "format"), error,
                                   "ocaf-xml-xcaf-version-12"))
            return false;
    }
    {
        const auto member = value.FindMember("bytes");
        if (member == value.MemberEnd())
            return fail(error, "geometer.contract.missing_field", child_path(path, "bytes"),
                        "Required field is missing.");
        if (!decode_uint32(member->value, &out->bytes, child_path(path, "bytes"), error, 1ULL,
                           536870912ULL))
            return false;
    }
    {
        const auto member = value.FindMember("sha256");
        if (member == value.MemberEnd())
            return fail(error, "geometer.contract.missing_field", child_path(path, "sha256"),
                        "Required field is missing.");
        if (!decode_string(member->value, &out->sha256, child_path(path, "sha256"), error, 64U,
                           64U))
            return false;
    }
    return true;
}

bool write_XmlXcafPersistenceArtifact(rapidjson::Writer<rapidjson::StringBuffer>& writer,
                                      const XmlXcafPersistenceArtifact& value, ContractError* error)
{
    writer.StartObject();
    writer.Key("carrier");
    if (!write_literal_string(writer, value.carrier, error, "xml_xcaf"))
        return false;
    writer.Key("name");
    if (!write_literal_string(writer, value.name, error, "state_artifact"))
        return false;
    writer.Key("media_type");
    if (!write_literal_string(writer, value.media_type, error,
                              "application/vnd.opencascade.xml-xcaf"))
        return false;
    writer.Key("format");
    if (!write_literal_string(writer, value.format, error, "ocaf-xml-xcaf-version-12"))
        return false;
    writer.Key("bytes");
    if (!write_uint32(writer, value.bytes, error, 1ULL, 536870912ULL))
        return false;
    writer.Key("sha256");
    if (!write_string(writer, value.sha256, error, 64U, 64U))
        return false;
    writer.EndObject();
    return true;
}

bool decode_StepAp242PersistenceArtifact(const rapidjson::Value& value,
                                         StepAp242PersistenceArtifact* out, const std::string& path,
                                         ContractError* error)
{
    static const char* const names[] = {"carrier", "name",  "media_type",
                                        "format",  "bytes", "sha256"};
    if (!validate_object(value, names, 6U, path, error))
        return false;
    {
        const auto member = value.FindMember("carrier");
        if (member == value.MemberEnd())
            return fail(error, "geometer.contract.missing_field", child_path(path, "carrier"),
                        "Required field is missing.");
        if (!decode_literal_string(member->value, &out->carrier, child_path(path, "carrier"), error,
                                   "step_ap242"))
            return false;
    }
    {
        const auto member = value.FindMember("name");
        if (member == value.MemberEnd())
            return fail(error, "geometer.contract.missing_field", child_path(path, "name"),
                        "Required field is missing.");
        if (!decode_literal_string(member->value, &out->name, child_path(path, "name"), error,
                                   "state_artifact"))
            return false;
    }
    {
        const auto member = value.FindMember("media_type");
        if (member == value.MemberEnd())
            return fail(error, "geometer.contract.missing_field", child_path(path, "media_type"),
                        "Required field is missing.");
        if (!decode_literal_string(member->value, &out->media_type, child_path(path, "media_type"),
                                   error, "application/step"))
            return false;
    }
    {
        const auto member = value.FindMember("format");
        if (member == value.MemberEnd())
            return fail(error, "geometer.contract.missing_field", child_path(path, "format"),
                        "Required field is missing.");
        if (!decode_literal_string(member->value, &out->format, child_path(path, "format"), error,
                                   "ap242-managed-model-based-3d-engineering"))
            return false;
    }
    {
        const auto member = value.FindMember("bytes");
        if (member == value.MemberEnd())
            return fail(error, "geometer.contract.missing_field", child_path(path, "bytes"),
                        "Required field is missing.");
        if (!decode_uint32(member->value, &out->bytes, child_path(path, "bytes"), error, 1ULL,
                           536870912ULL))
            return false;
    }
    {
        const auto member = value.FindMember("sha256");
        if (member == value.MemberEnd())
            return fail(error, "geometer.contract.missing_field", child_path(path, "sha256"),
                        "Required field is missing.");
        if (!decode_string(member->value, &out->sha256, child_path(path, "sha256"), error, 64U,
                           64U))
            return false;
    }
    return true;
}

bool write_StepAp242PersistenceArtifact(rapidjson::Writer<rapidjson::StringBuffer>& writer,
                                        const StepAp242PersistenceArtifact& value,
                                        ContractError* error)
{
    writer.StartObject();
    writer.Key("carrier");
    if (!write_literal_string(writer, value.carrier, error, "step_ap242"))
        return false;
    writer.Key("name");
    if (!write_literal_string(writer, value.name, error, "state_artifact"))
        return false;
    writer.Key("media_type");
    if (!write_literal_string(writer, value.media_type, error, "application/step"))
        return false;
    writer.Key("format");
    if (!write_literal_string(writer, value.format, error,
                              "ap242-managed-model-based-3d-engineering"))
        return false;
    writer.Key("bytes");
    if (!write_uint32(writer, value.bytes, error, 1ULL, 536870912ULL))
        return false;
    writer.Key("sha256");
    if (!write_string(writer, value.sha256, error, 64U, 64U))
        return false;
    writer.EndObject();
    return true;
}

bool decode_JsonSidecarPersistenceArtifact(const rapidjson::Value& value,
                                           JsonSidecarPersistenceArtifact* out,
                                           const std::string& path, ContractError* error)
{
    static const char* const names[] = {"carrier", "name",  "media_type",
                                        "format",  "bytes", "sha256"};
    if (!validate_object(value, names, 6U, path, error))
        return false;
    {
        const auto member = value.FindMember("carrier");
        if (member == value.MemberEnd())
            return fail(error, "geometer.contract.missing_field", child_path(path, "carrier"),
                        "Required field is missing.");
        if (!decode_literal_string(member->value, &out->carrier, child_path(path, "carrier"), error,
                                   "json_sidecar"))
            return false;
    }
    {
        const auto member = value.FindMember("name");
        if (member == value.MemberEnd())
            return fail(error, "geometer.contract.missing_field", child_path(path, "name"),
                        "Required field is missing.");
        if (!decode_literal_string(member->value, &out->name, child_path(path, "name"), error,
                                   "state_artifact"))
            return false;
    }
    {
        const auto member = value.FindMember("media_type");
        if (member == value.MemberEnd())
            return fail(error, "geometer.contract.missing_field", child_path(path, "media_type"),
                        "Required field is missing.");
        if (!decode_literal_string(
                member->value, &out->media_type, child_path(path, "media_type"), error,
                "application/vnd.wavenumber.geometer.step-topology-sidecar+json"))
            return false;
    }
    {
        const auto member = value.FindMember("format");
        if (member == value.MemberEnd())
            return fail(error, "geometer.contract.missing_field", child_path(path, "format"),
                        "Required field is missing.");
        if (!decode_literal_string(member->value, &out->format, child_path(path, "format"), error,
                                   "geometer.step_topology_sidecar.a0"))
            return false;
    }
    {
        const auto member = value.FindMember("bytes");
        if (member == value.MemberEnd())
            return fail(error, "geometer.contract.missing_field", child_path(path, "bytes"),
                        "Required field is missing.");
        if (!decode_uint32(member->value, &out->bytes, child_path(path, "bytes"), error, 1ULL,
                           67108864ULL))
            return false;
    }
    {
        const auto member = value.FindMember("sha256");
        if (member == value.MemberEnd())
            return fail(error, "geometer.contract.missing_field", child_path(path, "sha256"),
                        "Required field is missing.");
        if (!decode_string(member->value, &out->sha256, child_path(path, "sha256"), error, 64U,
                           64U))
            return false;
    }
    return true;
}

bool write_JsonSidecarPersistenceArtifact(rapidjson::Writer<rapidjson::StringBuffer>& writer,
                                          const JsonSidecarPersistenceArtifact& value,
                                          ContractError* error)
{
    writer.StartObject();
    writer.Key("carrier");
    if (!write_literal_string(writer, value.carrier, error, "json_sidecar"))
        return false;
    writer.Key("name");
    if (!write_literal_string(writer, value.name, error, "state_artifact"))
        return false;
    writer.Key("media_type");
    if (!write_literal_string(writer, value.media_type, error,
                              "application/vnd.wavenumber.geometer.step-topology-sidecar+json"))
        return false;
    writer.Key("format");
    if (!write_literal_string(writer, value.format, error, "geometer.step_topology_sidecar.a0"))
        return false;
    writer.Key("bytes");
    if (!write_uint32(writer, value.bytes, error, 1ULL, 67108864ULL))
        return false;
    writer.Key("sha256");
    if (!write_string(writer, value.sha256, error, 64U, 64U))
        return false;
    writer.EndObject();
    return true;
}

bool decode_EditJournalPersistenceArtifact(const rapidjson::Value& value,
                                           EditJournalPersistenceArtifact* out,
                                           const std::string& path, ContractError* error)
{
    static const char* const names[] = {"carrier", "name",  "media_type",
                                        "format",  "bytes", "sha256"};
    if (!validate_object(value, names, 6U, path, error))
        return false;
    {
        const auto member = value.FindMember("carrier");
        if (member == value.MemberEnd())
            return fail(error, "geometer.contract.missing_field", child_path(path, "carrier"),
                        "Required field is missing.");
        if (!decode_literal_string(member->value, &out->carrier, child_path(path, "carrier"), error,
                                   "edit_journal"))
            return false;
    }
    {
        const auto member = value.FindMember("name");
        if (member == value.MemberEnd())
            return fail(error, "geometer.contract.missing_field", child_path(path, "name"),
                        "Required field is missing.");
        if (!decode_literal_string(member->value, &out->name, child_path(path, "name"), error,
                                   "state_artifact"))
            return false;
    }
    {
        const auto member = value.FindMember("media_type");
        if (member == value.MemberEnd())
            return fail(error, "geometer.contract.missing_field", child_path(path, "media_type"),
                        "Required field is missing.");
        if (!decode_literal_string(
                member->value, &out->media_type, child_path(path, "media_type"), error,
                "application/vnd.wavenumber.geometer.step-topology-edit-journal"))
            return false;
    }
    {
        const auto member = value.FindMember("format");
        if (member == value.MemberEnd())
            return fail(error, "geometer.contract.missing_field", child_path(path, "format"),
                        "Required field is missing.");
        if (!decode_literal_string(member->value, &out->format, child_path(path, "format"), error,
                                   "geometer.step_topology_edit_journal.a0"))
            return false;
    }
    {
        const auto member = value.FindMember("bytes");
        if (member == value.MemberEnd())
            return fail(error, "geometer.contract.missing_field", child_path(path, "bytes"),
                        "Required field is missing.");
        if (!decode_uint32(member->value, &out->bytes, child_path(path, "bytes"), error, 1ULL,
                           67108864ULL))
            return false;
    }
    {
        const auto member = value.FindMember("sha256");
        if (member == value.MemberEnd())
            return fail(error, "geometer.contract.missing_field", child_path(path, "sha256"),
                        "Required field is missing.");
        if (!decode_string(member->value, &out->sha256, child_path(path, "sha256"), error, 64U,
                           64U))
            return false;
    }
    return true;
}

bool write_EditJournalPersistenceArtifact(rapidjson::Writer<rapidjson::StringBuffer>& writer,
                                          const EditJournalPersistenceArtifact& value,
                                          ContractError* error)
{
    writer.StartObject();
    writer.Key("carrier");
    if (!write_literal_string(writer, value.carrier, error, "edit_journal"))
        return false;
    writer.Key("name");
    if (!write_literal_string(writer, value.name, error, "state_artifact"))
        return false;
    writer.Key("media_type");
    if (!write_literal_string(writer, value.media_type, error,
                              "application/vnd.wavenumber.geometer.step-topology-edit-journal"))
        return false;
    writer.Key("format");
    if (!write_literal_string(writer, value.format, error,
                              "geometer.step_topology_edit_journal.a0"))
        return false;
    writer.Key("bytes");
    if (!write_uint32(writer, value.bytes, error, 1ULL, 67108864ULL))
        return false;
    writer.Key("sha256");
    if (!write_string(writer, value.sha256, error, 64U, 64U))
        return false;
    writer.EndObject();
    return true;
}

bool decode_RestoreStateArtifact(const rapidjson::Value& value, RestoreStateArtifact* out,
                                 const std::string& path, ContractError* error)
{
    int matches = 0;
    RestoreStateArtifact selected{};
    {
        XbfPersistenceArtifact candidate{};
        ContractError ignored;
        if (decode_XbfPersistenceArtifact(value, &candidate, path, &ignored))
        {
            ++matches;
            selected = RestoreStateArtifact(std::in_place_index<0>, std::move(candidate));
        }
    }
    {
        XmlXcafPersistenceArtifact candidate{};
        ContractError ignored;
        if (decode_XmlXcafPersistenceArtifact(value, &candidate, path, &ignored))
        {
            ++matches;
            selected = RestoreStateArtifact(std::in_place_index<1>, std::move(candidate));
        }
    }
    {
        StepAp242PersistenceArtifact candidate{};
        ContractError ignored;
        if (decode_StepAp242PersistenceArtifact(value, &candidate, path, &ignored))
        {
            ++matches;
            selected = RestoreStateArtifact(std::in_place_index<2>, std::move(candidate));
        }
    }
    {
        JsonSidecarPersistenceArtifact candidate{};
        ContractError ignored;
        if (decode_JsonSidecarPersistenceArtifact(value, &candidate, path, &ignored))
        {
            ++matches;
            selected = RestoreStateArtifact(std::in_place_index<3>, std::move(candidate));
        }
    }
    {
        EditJournalPersistenceArtifact candidate{};
        ContractError ignored;
        if (decode_EditJournalPersistenceArtifact(value, &candidate, path, &ignored))
        {
            ++matches;
            selected = RestoreStateArtifact(std::in_place_index<4>, std::move(candidate));
        }
    }
    if (matches != 1)
        return fail(error, "geometer.contract.union_mismatch", path,
                    "Expected exactly one union variant.");
    *out = std::move(selected);
    return true;
}

bool write_RestoreStateArtifact(rapidjson::Writer<rapidjson::StringBuffer>& writer,
                                const RestoreStateArtifact& value, ContractError* error)
{
    switch (value.index())
    {
    case 0:
        return write_XbfPersistenceArtifact(writer, std::get<0>(value), error);
    case 1:
        return write_XmlXcafPersistenceArtifact(writer, std::get<1>(value), error);
    case 2:
        return write_StepAp242PersistenceArtifact(writer, std::get<2>(value), error);
    case 3:
        return write_JsonSidecarPersistenceArtifact(writer, std::get<3>(value), error);
    case 4:
        return write_EditJournalPersistenceArtifact(writer, std::get<4>(value), error);
    default:
        return fail(error, "geometer.contract.union_mismatch", "", "Unknown union variant.");
    }
}

bool decode_EditJournalReplayPreconditions(const rapidjson::Value& value,
                                           EditJournalReplayPreconditions* out,
                                           const std::string& path, ContractError* error)
{
    static const char* const names[] = {"source_sha256", "source_brep_sha256",
                                        "target_inventory_sha256", "occt_version",
                                        "transaction_count"};
    if (!validate_object(value, names, 5U, path, error))
        return false;
    {
        const auto member = value.FindMember("source_sha256");
        if (member == value.MemberEnd())
            return fail(error, "geometer.contract.missing_field", child_path(path, "source_sha256"),
                        "Required field is missing.");
        if (!decode_string(member->value, &out->source_sha256, child_path(path, "source_sha256"),
                           error, 64U, 64U))
            return false;
    }
    {
        const auto member = value.FindMember("source_brep_sha256");
        if (member == value.MemberEnd())
            return fail(error, "geometer.contract.missing_field",
                        child_path(path, "source_brep_sha256"), "Required field is missing.");
        if (!decode_string(member->value, &out->source_brep_sha256,
                           child_path(path, "source_brep_sha256"), error, 64U, 64U))
            return false;
    }
    {
        const auto member = value.FindMember("target_inventory_sha256");
        if (member == value.MemberEnd())
            return fail(error, "geometer.contract.missing_field",
                        child_path(path, "target_inventory_sha256"), "Required field is missing.");
        if (!decode_string(member->value, &out->target_inventory_sha256,
                           child_path(path, "target_inventory_sha256"), error, 64U, 64U))
            return false;
    }
    {
        const auto member = value.FindMember("occt_version");
        if (member == value.MemberEnd())
            return fail(error, "geometer.contract.missing_field", child_path(path, "occt_version"),
                        "Required field is missing.");
        if (!decode_string(member->value, &out->occt_version, child_path(path, "occt_version"),
                           error, 1U, 64U))
            return false;
    }
    {
        const auto member = value.FindMember("transaction_count");
        if (member == value.MemberEnd())
            return fail(error, "geometer.contract.missing_field",
                        child_path(path, "transaction_count"), "Required field is missing.");
        if (!decode_uint32(member->value, &out->transaction_count,
                           child_path(path, "transaction_count"), error, 0ULL, 100000ULL))
            return false;
    }
    return true;
}

bool write_EditJournalReplayPreconditions(rapidjson::Writer<rapidjson::StringBuffer>& writer,
                                          const EditJournalReplayPreconditions& value,
                                          ContractError* error)
{
    writer.StartObject();
    writer.Key("source_sha256");
    if (!write_string(writer, value.source_sha256, error, 64U, 64U))
        return false;
    writer.Key("source_brep_sha256");
    if (!write_string(writer, value.source_brep_sha256, error, 64U, 64U))
        return false;
    writer.Key("target_inventory_sha256");
    if (!write_string(writer, value.target_inventory_sha256, error, 64U, 64U))
        return false;
    writer.Key("occt_version");
    if (!write_string(writer, value.occt_version, error, 1U, 64U))
        return false;
    writer.Key("transaction_count");
    if (!write_uint32(writer, value.transaction_count, error, 0ULL, 100000ULL))
        return false;
    writer.EndObject();
    return true;
}

bool decode_StepTopologyRestoreRequestA0(const rapidjson::Value& value,
                                         StepTopologyRestoreRequestA0* out, const std::string& path,
                                         ContractError* error)
{
    static const char* const names[] = {"schema", "source", "state_artifact",
                                        "replay_preconditions", "include_diagnostics"};
    if (!validate_object(value, names, 5U, path, error))
        return false;
    {
        const auto member = value.FindMember("schema");
        if (member == value.MemberEnd())
            return fail(error, "geometer.contract.missing_field", child_path(path, "schema"),
                        "Required field is missing.");
        if (!decode_literal_string(member->value, &out->schema, child_path(path, "schema"), error,
                                   "geometry.step_topology.restore.request.a0"))
            return false;
    }
    {
        const auto member = value.FindMember("source");
        if (member == value.MemberEnd())
            return fail(error, "geometer.contract.missing_field", child_path(path, "source"),
                        "Required field is missing.");
        if (!decode_SourceDescriptor(member->value, &out->source, child_path(path, "source"),
                                     error))
            return false;
    }
    {
        const auto member = value.FindMember("state_artifact");
        if (member == value.MemberEnd())
            return fail(error, "geometer.contract.missing_field",
                        child_path(path, "state_artifact"), "Required field is missing.");
        if (!decode_RestoreStateArtifact(member->value, &out->state_artifact,
                                         child_path(path, "state_artifact"), error))
            return false;
    }
    {
        const auto member = value.FindMember("replay_preconditions");
        if (member != value.MemberEnd())
        {
            EditJournalReplayPreconditions decoded{};
            if (!decode_EditJournalReplayPreconditions(
                    member->value, &decoded, child_path(path, "replay_preconditions"), error))
                return false;
            out->replay_preconditions = std::move(decoded);
        }
        else
            out->replay_preconditions.reset();
    }
    {
        const auto member = value.FindMember("include_diagnostics");
        if (member == value.MemberEnd())
            return fail(error, "geometer.contract.missing_field",
                        child_path(path, "include_diagnostics"), "Required field is missing.");
        if (!decode_boolean(member->value, &out->include_diagnostics,
                            child_path(path, "include_diagnostics"), error))
            return false;
    }
    return true;
}

bool write_StepTopologyRestoreRequestA0(rapidjson::Writer<rapidjson::StringBuffer>& writer,
                                        const StepTopologyRestoreRequestA0& value,
                                        ContractError* error)
{
    writer.StartObject();
    writer.Key("schema");
    if (!write_literal_string(writer, value.schema, error,
                              "geometry.step_topology.restore.request.a0"))
        return false;
    writer.Key("source");
    if (!write_SourceDescriptor(writer, value.source, error))
        return false;
    writer.Key("state_artifact");
    if (!write_RestoreStateArtifact(writer, value.state_artifact, error))
        return false;
    if (value.replay_preconditions.has_value())
    {
        writer.Key("replay_preconditions");
        if (!write_EditJournalReplayPreconditions(writer, *value.replay_preconditions, error))
            return false;
    }
    writer.Key("include_diagnostics");
    if (!(writer.Bool(value.include_diagnostics), true))
        return false;
    writer.EndObject();
    return true;
}

bool decode_RecoveryProvenance(const rapidjson::Value& value, RecoveryProvenance* out,
                               const std::string& path, ContractError* error)
{
    static const char* const names[] = {
        "source_artifact_sha256", "candidate_artifact_sha256",
        "source_occt_version",    "candidate_occt_version",
        "source_driver",          "candidate_driver",
        "source_writer_settings", "candidate_writer_settings",
        "command_provenance",     "measured_wall_time_milliseconds"};
    if (!validate_object(value, names, 10U, path, error))
        return false;
    {
        const auto member = value.FindMember("source_artifact_sha256");
        if (member == value.MemberEnd())
            return fail(error, "geometer.contract.missing_field",
                        child_path(path, "source_artifact_sha256"), "Required field is missing.");
        if (!decode_string(member->value, &out->source_artifact_sha256,
                           child_path(path, "source_artifact_sha256"), error, 64U, 64U))
            return false;
    }
    {
        const auto member = value.FindMember("candidate_artifact_sha256");
        if (member == value.MemberEnd())
            return fail(error, "geometer.contract.missing_field",
                        child_path(path, "candidate_artifact_sha256"),
                        "Required field is missing.");
        if (!decode_string(member->value, &out->candidate_artifact_sha256,
                           child_path(path, "candidate_artifact_sha256"), error, 64U, 64U))
            return false;
    }
    {
        const auto member = value.FindMember("source_occt_version");
        if (member == value.MemberEnd())
            return fail(error, "geometer.contract.missing_field",
                        child_path(path, "source_occt_version"), "Required field is missing.");
        if (!decode_string(member->value, &out->source_occt_version,
                           child_path(path, "source_occt_version"), error, 1U, 64U))
            return false;
    }
    {
        const auto member = value.FindMember("candidate_occt_version");
        if (member == value.MemberEnd())
            return fail(error, "geometer.contract.missing_field",
                        child_path(path, "candidate_occt_version"), "Required field is missing.");
        if (!decode_string(member->value, &out->candidate_occt_version,
                           child_path(path, "candidate_occt_version"), error, 1U, 64U))
            return false;
    }
    {
        const auto member = value.FindMember("source_driver");
        if (member == value.MemberEnd())
            return fail(error, "geometer.contract.missing_field", child_path(path, "source_driver"),
                        "Required field is missing.");
        if (!decode_string(member->value, &out->source_driver, child_path(path, "source_driver"),
                           error, 1U, 128U))
            return false;
    }
    {
        const auto member = value.FindMember("candidate_driver");
        if (member == value.MemberEnd())
            return fail(error, "geometer.contract.missing_field",
                        child_path(path, "candidate_driver"), "Required field is missing.");
        if (!decode_string(member->value, &out->candidate_driver,
                           child_path(path, "candidate_driver"), error, 1U, 128U))
            return false;
    }
    {
        const auto member = value.FindMember("source_writer_settings");
        if (member == value.MemberEnd())
            return fail(error, "geometer.contract.missing_field",
                        child_path(path, "source_writer_settings"), "Required field is missing.");
        if (!decode_string(member->value, &out->source_writer_settings,
                           child_path(path, "source_writer_settings"), error, 1U, 4096U))
            return false;
    }
    {
        const auto member = value.FindMember("candidate_writer_settings");
        if (member == value.MemberEnd())
            return fail(error, "geometer.contract.missing_field",
                        child_path(path, "candidate_writer_settings"),
                        "Required field is missing.");
        if (!decode_string(member->value, &out->candidate_writer_settings,
                           child_path(path, "candidate_writer_settings"), error, 1U, 4096U))
            return false;
    }
    {
        const auto member = value.FindMember("command_provenance");
        if (member == value.MemberEnd())
            return fail(error, "geometer.contract.missing_field",
                        child_path(path, "command_provenance"), "Required field is missing.");
        if (!decode_string(member->value, &out->command_provenance,
                           child_path(path, "command_provenance"), error, 1U, 8192U))
            return false;
    }
    {
        const auto member = value.FindMember("measured_wall_time_milliseconds");
        if (member == value.MemberEnd())
            return fail(error, "geometer.contract.missing_field",
                        child_path(path, "measured_wall_time_milliseconds"),
                        "Required field is missing.");
        if (!decode_double(member->value, &out->measured_wall_time_milliseconds,
                           child_path(path, "measured_wall_time_milliseconds"), error, 0,
                           std::numeric_limits<double>::infinity(), false, false))
            return false;
    }
    return true;
}

bool write_RecoveryProvenance(rapidjson::Writer<rapidjson::StringBuffer>& writer,
                              const RecoveryProvenance& value, ContractError* error)
{
    writer.StartObject();
    writer.Key("source_artifact_sha256");
    if (!write_string(writer, value.source_artifact_sha256, error, 64U, 64U))
        return false;
    writer.Key("candidate_artifact_sha256");
    if (!write_string(writer, value.candidate_artifact_sha256, error, 64U, 64U))
        return false;
    writer.Key("source_occt_version");
    if (!write_string(writer, value.source_occt_version, error, 1U, 64U))
        return false;
    writer.Key("candidate_occt_version");
    if (!write_string(writer, value.candidate_occt_version, error, 1U, 64U))
        return false;
    writer.Key("source_driver");
    if (!write_string(writer, value.source_driver, error, 1U, 128U))
        return false;
    writer.Key("candidate_driver");
    if (!write_string(writer, value.candidate_driver, error, 1U, 128U))
        return false;
    writer.Key("source_writer_settings");
    if (!write_string(writer, value.source_writer_settings, error, 1U, 4096U))
        return false;
    writer.Key("candidate_writer_settings");
    if (!write_string(writer, value.candidate_writer_settings, error, 1U, 4096U))
        return false;
    writer.Key("command_provenance");
    if (!write_string(writer, value.command_provenance, error, 1U, 8192U))
        return false;
    writer.Key("measured_wall_time_milliseconds");
    if (!write_double(writer, value.measured_wall_time_milliseconds, error, 0,
                      std::numeric_limits<double>::infinity(), false, false))
        return false;
    writer.EndObject();
    return true;
}

bool decode_RecoveryTolerances(const rapidjson::Value& value, RecoveryTolerances* out,
                               const std::string& path, ContractError* error)
{
    static const char* const names[] = {"length_mm", "area_mm2", "volume_mm3"};
    if (!validate_object(value, names, 3U, path, error))
        return false;
    {
        const auto member = value.FindMember("length_mm");
        if (member == value.MemberEnd())
            return fail(error, "geometer.contract.missing_field", child_path(path, "length_mm"),
                        "Required field is missing.");
        if (!decode_double(member->value, &out->length_mm, child_path(path, "length_mm"), error,
                           1e-9, std::numeric_limits<double>::infinity(), false, false))
            return false;
    }
    {
        const auto member = value.FindMember("area_mm2");
        if (member == value.MemberEnd())
            return fail(error, "geometer.contract.missing_field", child_path(path, "area_mm2"),
                        "Required field is missing.");
        if (!decode_double(member->value, &out->area_mm2, child_path(path, "area_mm2"), error, 1e-9,
                           std::numeric_limits<double>::infinity(), false, false))
            return false;
    }
    {
        const auto member = value.FindMember("volume_mm3");
        if (member == value.MemberEnd())
            return fail(error, "geometer.contract.missing_field", child_path(path, "volume_mm3"),
                        "Required field is missing.");
        if (!decode_double(member->value, &out->volume_mm3, child_path(path, "volume_mm3"), error,
                           1e-9, std::numeric_limits<double>::infinity(), false, false))
            return false;
    }
    return true;
}

bool write_RecoveryTolerances(rapidjson::Writer<rapidjson::StringBuffer>& writer,
                              const RecoveryTolerances& value, ContractError* error)
{
    writer.StartObject();
    writer.Key("length_mm");
    if (!write_double(writer, value.length_mm, error, 1e-9, std::numeric_limits<double>::infinity(),
                      false, false))
        return false;
    writer.Key("area_mm2");
    if (!write_double(writer, value.area_mm2, error, 1e-9, std::numeric_limits<double>::infinity(),
                      false, false))
        return false;
    writer.Key("volume_mm3");
    if (!write_double(writer, value.volume_mm3, error, 1e-9,
                      std::numeric_limits<double>::infinity(), false, false))
        return false;
    writer.EndObject();
    return true;
}

bool decode_LogicalGroupMemberKind(const rapidjson::Value& value, LogicalGroupMemberKind* out,
                                   const std::string& path, ContractError* error)
{
    if (!value.IsString())
        return fail(error, "geometer.contract.type_mismatch", path, "Expected a string enum.");
    const std::string text(value.GetString(), value.GetStringLength());
    if (text == "body")
    {
        *out = LogicalGroupMemberKind::body;
        return true;
    }
    if (text == "face")
    {
        *out = LogicalGroupMemberKind::face;
        return true;
    }
    return fail(error, "geometer.contract.enum_mismatch", path, "Unknown enum value.");
}

bool write_LogicalGroupMemberKind(rapidjson::Writer<rapidjson::StringBuffer>& writer,
                                  const LogicalGroupMemberKind& value, ContractError* error)
{
    switch (value)
    {
    case LogicalGroupMemberKind::body:
        writer.String("body");
        return true;
    case LogicalGroupMemberKind::face:
        writer.String("face");
        return true;
    }
    return fail(error, "geometer.contract.enum_mismatch", "", "Unknown enum value.");
}

bool decode_RecoveryFingerprint(const rapidjson::Value& value, RecoveryFingerprint* out,
                                const std::string& path, ContractError* error)
{
    static const char* const names[] = {"normalized_length_unit",
                                        "coordinate_frame",
                                        "occurrence_context",
                                        "geometry_kind",
                                        "area_mm2",
                                        "volume_mm3",
                                        "centroid_mm",
                                        "bounds_mm",
                                        "adjacency_sha256"};
    if (!validate_object(value, names, 9U, path, error))
        return false;
    {
        const auto member = value.FindMember("normalized_length_unit");
        if (member == value.MemberEnd())
            return fail(error, "geometer.contract.missing_field",
                        child_path(path, "normalized_length_unit"), "Required field is missing.");
        if (!decode_literal_string(member->value, &out->normalized_length_unit,
                                   child_path(path, "normalized_length_unit"), error, "millimeter"))
            return false;
    }
    {
        const auto member = value.FindMember("coordinate_frame");
        if (member == value.MemberEnd())
            return fail(error, "geometer.contract.missing_field",
                        child_path(path, "coordinate_frame"), "Required field is missing.");
        if (!decode_string(member->value, &out->coordinate_frame,
                           child_path(path, "coordinate_frame"), error, 1U, 128U))
            return false;
    }
    {
        const auto member = value.FindMember("occurrence_context");
        if (member == value.MemberEnd())
            return fail(error, "geometer.contract.missing_field",
                        child_path(path, "occurrence_context"), "Required field is missing.");
        if (!decode_string(member->value, &out->occurrence_context,
                           child_path(path, "occurrence_context"), error, 1U, 128U))
            return false;
    }
    {
        const auto member = value.FindMember("geometry_kind");
        if (member == value.MemberEnd())
            return fail(error, "geometer.contract.missing_field", child_path(path, "geometry_kind"),
                        "Required field is missing.");
        if (!decode_string(member->value, &out->geometry_kind, child_path(path, "geometry_kind"),
                           error, 1U, 128U))
            return false;
    }
    {
        const auto member = value.FindMember("area_mm2");
        if (member == value.MemberEnd())
            return fail(error, "geometer.contract.missing_field", child_path(path, "area_mm2"),
                        "Required field is missing.");
        if (!decode_double(member->value, &out->area_mm2, child_path(path, "area_mm2"), error, 0,
                           std::numeric_limits<double>::infinity(), false, false))
            return false;
    }
    {
        const auto member = value.FindMember("volume_mm3");
        if (member == value.MemberEnd())
            return fail(error, "geometer.contract.missing_field", child_path(path, "volume_mm3"),
                        "Required field is missing.");
        if (!decode_double(member->value, &out->volume_mm3, child_path(path, "volume_mm3"), error,
                           0, std::numeric_limits<double>::infinity(), false, false))
            return false;
    }
    {
        const auto member = value.FindMember("centroid_mm");
        if (member == value.MemberEnd())
            return fail(error, "geometer.contract.missing_field", child_path(path, "centroid_mm"),
                        "Required field is missing.");
        if (!decode_array(member->value, &out->centroid_mm, child_path(path, "centroid_mm"), error,
                          3U, 3U, decode_double_item))
            return false;
    }
    {
        const auto member = value.FindMember("bounds_mm");
        if (member == value.MemberEnd())
            return fail(error, "geometer.contract.missing_field", child_path(path, "bounds_mm"),
                        "Required field is missing.");
        if (!decode_array(member->value, &out->bounds_mm, child_path(path, "bounds_mm"), error, 6U,
                          6U, decode_double_item))
            return false;
    }
    {
        const auto member = value.FindMember("adjacency_sha256");
        if (member == value.MemberEnd())
            return fail(error, "geometer.contract.missing_field",
                        child_path(path, "adjacency_sha256"), "Required field is missing.");
        if (!decode_string(member->value, &out->adjacency_sha256,
                           child_path(path, "adjacency_sha256"), error, 64U, 64U))
            return false;
    }
    return true;
}

bool write_RecoveryFingerprint(rapidjson::Writer<rapidjson::StringBuffer>& writer,
                               const RecoveryFingerprint& value, ContractError* error)
{
    writer.StartObject();
    writer.Key("normalized_length_unit");
    if (!write_literal_string(writer, value.normalized_length_unit, error, "millimeter"))
        return false;
    writer.Key("coordinate_frame");
    if (!write_string(writer, value.coordinate_frame, error, 1U, 128U))
        return false;
    writer.Key("occurrence_context");
    if (!write_string(writer, value.occurrence_context, error, 1U, 128U))
        return false;
    writer.Key("geometry_kind");
    if (!write_string(writer, value.geometry_kind, error, 1U, 128U))
        return false;
    writer.Key("area_mm2");
    if (!write_double(writer, value.area_mm2, error, 0, std::numeric_limits<double>::infinity(),
                      false, false))
        return false;
    writer.Key("volume_mm3");
    if (!write_double(writer, value.volume_mm3, error, 0, std::numeric_limits<double>::infinity(),
                      false, false))
        return false;
    writer.Key("centroid_mm");
    if (!write_array(writer, value.centroid_mm, error, 3U, 3U, write_double_item))
        return false;
    writer.Key("bounds_mm");
    if (!write_array(writer, value.bounds_mm, error, 6U, 6U, write_double_item))
        return false;
    writer.Key("adjacency_sha256");
    if (!write_string(writer, value.adjacency_sha256, error, 64U, 64U))
        return false;
    writer.EndObject();
    return true;
}

bool decode_RecoveryLineage(const rapidjson::Value& value, RecoveryLineage* out,
                            const std::string& path, ContractError* error)
{
    if (!value.IsString())
        return fail(error, "geometer.contract.type_mismatch", path, "Expected a string enum.");
    const std::string text(value.GetString(), value.GetStringLength());
    if (text == "none")
    {
        *out = RecoveryLineage::none;
        return true;
    }
    if (text == "split_from_source")
    {
        *out = RecoveryLineage::split_from_source;
        return true;
    }
    if (text == "merged_from_sources")
    {
        *out = RecoveryLineage::merged_from_sources;
        return true;
    }
    return fail(error, "geometer.contract.enum_mismatch", path, "Unknown enum value.");
}

bool write_RecoveryLineage(rapidjson::Writer<rapidjson::StringBuffer>& writer,
                           const RecoveryLineage& value, ContractError* error)
{
    switch (value)
    {
    case RecoveryLineage::none:
        writer.String("none");
        return true;
    case RecoveryLineage::split_from_source:
        writer.String("split_from_source");
        return true;
    case RecoveryLineage::merged_from_sources:
        writer.String("merged_from_sources");
        return true;
    }
    return fail(error, "geometer.contract.enum_mismatch", "", "Unknown enum value.");
}

bool decode_RecoveryCandidate(const rapidjson::Value& value, RecoveryCandidate* out,
                              const std::string& path, ContractError* error)
{
    static const char* const names[] = {"target_handle",      "kind",
                                        "authored_target_id", "topology_link_verified",
                                        "carrier_locator",    "carrier_locator_validated",
                                        "carrier_record",     "lineage",
                                        "fingerprint"};
    if (!validate_object(value, names, 9U, path, error))
        return false;
    {
        const auto member = value.FindMember("target_handle");
        if (member == value.MemberEnd())
            return fail(error, "geometer.contract.missing_field", child_path(path, "target_handle"),
                        "Required field is missing.");
        if (!decode_string(member->value, &out->target_handle, child_path(path, "target_handle"),
                           error, 68U, 68U))
            return false;
    }
    {
        const auto member = value.FindMember("kind");
        if (member == value.MemberEnd())
            return fail(error, "geometer.contract.missing_field", child_path(path, "kind"),
                        "Required field is missing.");
        if (!decode_LogicalGroupMemberKind(member->value, &out->kind, child_path(path, "kind"),
                                           error))
            return false;
    }
    {
        const auto member = value.FindMember("authored_target_id");
        if (member != value.MemberEnd())
        {
            std::string decoded{};
            if (!decode_string(member->value, &decoded, child_path(path, "authored_target_id"),
                               error, 28U, 128U))
                return false;
            out->authored_target_id = std::move(decoded);
        }
        else
            out->authored_target_id.reset();
    }
    {
        const auto member = value.FindMember("topology_link_verified");
        if (member == value.MemberEnd())
            return fail(error, "geometer.contract.missing_field",
                        child_path(path, "topology_link_verified"), "Required field is missing.");
        if (!decode_boolean(member->value, &out->topology_link_verified,
                            child_path(path, "topology_link_verified"), error))
            return false;
    }
    {
        const auto member = value.FindMember("carrier_locator");
        if (member == value.MemberEnd())
            return fail(error, "geometer.contract.missing_field",
                        child_path(path, "carrier_locator"), "Required field is missing.");
        if (!decode_string(member->value, &out->carrier_locator,
                           child_path(path, "carrier_locator"), error, 0U, 4096U))
            return false;
    }
    {
        const auto member = value.FindMember("carrier_locator_validated");
        if (member == value.MemberEnd())
            return fail(error, "geometer.contract.missing_field",
                        child_path(path, "carrier_locator_validated"),
                        "Required field is missing.");
        if (!decode_boolean(member->value, &out->carrier_locator_validated,
                            child_path(path, "carrier_locator_validated"), error))
            return false;
    }
    {
        const auto member = value.FindMember("carrier_record");
        if (member == value.MemberEnd())
            return fail(error, "geometer.contract.missing_field",
                        child_path(path, "carrier_record"), "Required field is missing.");
        if (!decode_string(member->value, &out->carrier_record, child_path(path, "carrier_record"),
                           error, 0U, 4096U))
            return false;
    }
    {
        const auto member = value.FindMember("lineage");
        if (member == value.MemberEnd())
            return fail(error, "geometer.contract.missing_field", child_path(path, "lineage"),
                        "Required field is missing.");
        if (!decode_RecoveryLineage(member->value, &out->lineage, child_path(path, "lineage"),
                                    error))
            return false;
    }
    {
        const auto member = value.FindMember("fingerprint");
        if (member != value.MemberEnd())
        {
            RecoveryFingerprint decoded{};
            if (!decode_RecoveryFingerprint(member->value, &decoded,
                                            child_path(path, "fingerprint"), error))
                return false;
            out->fingerprint = std::move(decoded);
        }
        else
            out->fingerprint.reset();
    }
    return true;
}

bool write_RecoveryCandidate(rapidjson::Writer<rapidjson::StringBuffer>& writer,
                             const RecoveryCandidate& value, ContractError* error)
{
    writer.StartObject();
    writer.Key("target_handle");
    if (!write_string(writer, value.target_handle, error, 68U, 68U))
        return false;
    writer.Key("kind");
    if (!write_LogicalGroupMemberKind(writer, value.kind, error))
        return false;
    if (value.authored_target_id.has_value())
    {
        writer.Key("authored_target_id");
        if (!write_string(writer, *value.authored_target_id, error, 28U, 128U))
            return false;
    }
    writer.Key("topology_link_verified");
    if (!(writer.Bool(value.topology_link_verified), true))
        return false;
    writer.Key("carrier_locator");
    if (!write_string(writer, value.carrier_locator, error, 0U, 4096U))
        return false;
    writer.Key("carrier_locator_validated");
    if (!(writer.Bool(value.carrier_locator_validated), true))
        return false;
    writer.Key("carrier_record");
    if (!write_string(writer, value.carrier_record, error, 0U, 4096U))
        return false;
    writer.Key("lineage");
    if (!write_RecoveryLineage(writer, value.lineage, error))
        return false;
    if (value.fingerprint.has_value())
    {
        writer.Key("fingerprint");
        if (!write_RecoveryFingerprint(writer, *value.fingerprint, error))
            return false;
    }
    writer.EndObject();
    return true;
}

bool decode_RecoveryMemberRequest(const rapidjson::Value& value, RecoveryMemberRequest* out,
                                  const std::string& path, ContractError* error)
{
    static const char* const names[] = {"member_record_id",   "kind",
                                        "authored_target_id", "carrier_locator",
                                        "source_fingerprint", "candidates"};
    if (!validate_object(value, names, 6U, path, error))
        return false;
    {
        const auto member = value.FindMember("member_record_id");
        if (member == value.MemberEnd())
            return fail(error, "geometer.contract.missing_field",
                        child_path(path, "member_record_id"), "Required field is missing.");
        if (!decode_string(member->value, &out->member_record_id,
                           child_path(path, "member_record_id"), error, 28U, 128U))
            return false;
    }
    {
        const auto member = value.FindMember("kind");
        if (member == value.MemberEnd())
            return fail(error, "geometer.contract.missing_field", child_path(path, "kind"),
                        "Required field is missing.");
        if (!decode_LogicalGroupMemberKind(member->value, &out->kind, child_path(path, "kind"),
                                           error))
            return false;
    }
    {
        const auto member = value.FindMember("authored_target_id");
        if (member == value.MemberEnd())
            return fail(error, "geometer.contract.missing_field",
                        child_path(path, "authored_target_id"), "Required field is missing.");
        if (!decode_string(member->value, &out->authored_target_id,
                           child_path(path, "authored_target_id"), error, 0U, 128U))
            return false;
    }
    {
        const auto member = value.FindMember("carrier_locator");
        if (member == value.MemberEnd())
            return fail(error, "geometer.contract.missing_field",
                        child_path(path, "carrier_locator"), "Required field is missing.");
        if (!decode_string(member->value, &out->carrier_locator,
                           child_path(path, "carrier_locator"), error, 0U, 4096U))
            return false;
    }
    {
        const auto member = value.FindMember("source_fingerprint");
        if (member != value.MemberEnd())
        {
            RecoveryFingerprint decoded{};
            if (!decode_RecoveryFingerprint(member->value, &decoded,
                                            child_path(path, "source_fingerprint"), error))
                return false;
            out->source_fingerprint = std::move(decoded);
        }
        else
            out->source_fingerprint.reset();
    }
    {
        const auto member = value.FindMember("candidates");
        if (member == value.MemberEnd())
            return fail(error, "geometer.contract.missing_field", child_path(path, "candidates"),
                        "Required field is missing.");
        if (!decode_array(member->value, &out->candidates, child_path(path, "candidates"), error,
                          0U, 16U, decode_RecoveryCandidate))
            return false;
    }
    return true;
}

bool write_RecoveryMemberRequest(rapidjson::Writer<rapidjson::StringBuffer>& writer,
                                 const RecoveryMemberRequest& value, ContractError* error)
{
    writer.StartObject();
    writer.Key("member_record_id");
    if (!write_string(writer, value.member_record_id, error, 28U, 128U))
        return false;
    writer.Key("kind");
    if (!write_LogicalGroupMemberKind(writer, value.kind, error))
        return false;
    writer.Key("authored_target_id");
    if (!write_string(writer, value.authored_target_id, error, 0U, 128U))
        return false;
    writer.Key("carrier_locator");
    if (!write_string(writer, value.carrier_locator, error, 0U, 4096U))
        return false;
    if (value.source_fingerprint.has_value())
    {
        writer.Key("source_fingerprint");
        if (!write_RecoveryFingerprint(writer, *value.source_fingerprint, error))
            return false;
    }
    writer.Key("candidates");
    if (!write_array(writer, value.candidates, error, 0U, 16U, write_RecoveryCandidate))
        return false;
    writer.EndObject();
    return true;
}

bool decode_RecoveryGroupRequest(const rapidjson::Value& value, RecoveryGroupRequest* out,
                                 const std::string& path, ContractError* error)
{
    static const char* const names[] = {"group_authored_id", "provenance", "tolerances", "members"};
    if (!validate_object(value, names, 4U, path, error))
        return false;
    {
        const auto member = value.FindMember("group_authored_id");
        if (member == value.MemberEnd())
            return fail(error, "geometer.contract.missing_field",
                        child_path(path, "group_authored_id"), "Required field is missing.");
        if (!decode_string(member->value, &out->group_authored_id,
                           child_path(path, "group_authored_id"), error, 28U, 128U))
            return false;
    }
    {
        const auto member = value.FindMember("provenance");
        if (member == value.MemberEnd())
            return fail(error, "geometer.contract.missing_field", child_path(path, "provenance"),
                        "Required field is missing.");
        if (!decode_RecoveryProvenance(member->value, &out->provenance,
                                       child_path(path, "provenance"), error))
            return false;
    }
    {
        const auto member = value.FindMember("tolerances");
        if (member == value.MemberEnd())
            return fail(error, "geometer.contract.missing_field", child_path(path, "tolerances"),
                        "Required field is missing.");
        if (!decode_RecoveryTolerances(member->value, &out->tolerances,
                                       child_path(path, "tolerances"), error))
            return false;
    }
    {
        const auto member = value.FindMember("members");
        if (member == value.MemberEnd())
            return fail(error, "geometer.contract.missing_field", child_path(path, "members"),
                        "Required field is missing.");
        if (!decode_array(member->value, &out->members, child_path(path, "members"), error, 1U,
                          256U, decode_RecoveryMemberRequest))
            return false;
    }
    return true;
}

bool write_RecoveryGroupRequest(rapidjson::Writer<rapidjson::StringBuffer>& writer,
                                const RecoveryGroupRequest& value, ContractError* error)
{
    writer.StartObject();
    writer.Key("group_authored_id");
    if (!write_string(writer, value.group_authored_id, error, 28U, 128U))
        return false;
    writer.Key("provenance");
    if (!write_RecoveryProvenance(writer, value.provenance, error))
        return false;
    writer.Key("tolerances");
    if (!write_RecoveryTolerances(writer, value.tolerances, error))
        return false;
    writer.Key("members");
    if (!write_array(writer, value.members, error, 1U, 256U, write_RecoveryMemberRequest))
        return false;
    writer.EndObject();
    return true;
}

bool decode_StepTopologyAnalyzeRecoveryRequestA0(const rapidjson::Value& value,
                                                 StepTopologyAnalyzeRecoveryRequestA0* out,
                                                 const std::string& path, ContractError* error)
{
    static const char* const names[] = {"schema", "groups"};
    if (!validate_object(value, names, 2U, path, error))
        return false;
    {
        const auto member = value.FindMember("schema");
        if (member == value.MemberEnd())
            return fail(error, "geometer.contract.missing_field", child_path(path, "schema"),
                        "Required field is missing.");
        if (!decode_literal_string(member->value, &out->schema, child_path(path, "schema"), error,
                                   "geometry.step_topology.analyze_recovery.request.a0"))
            return false;
    }
    {
        const auto member = value.FindMember("groups");
        if (member == value.MemberEnd())
            return fail(error, "geometer.contract.missing_field", child_path(path, "groups"),
                        "Required field is missing.");
        if (!decode_array(member->value, &out->groups, child_path(path, "groups"), error, 1U, 16U,
                          decode_RecoveryGroupRequest))
            return false;
    }
    return true;
}

bool write_StepTopologyAnalyzeRecoveryRequestA0(rapidjson::Writer<rapidjson::StringBuffer>& writer,
                                                const StepTopologyAnalyzeRecoveryRequestA0& value,
                                                ContractError* error)
{
    writer.StartObject();
    writer.Key("schema");
    if (!write_literal_string(writer, value.schema, error,
                              "geometry.step_topology.analyze_recovery.request.a0"))
        return false;
    writer.Key("groups");
    if (!write_array(writer, value.groups, error, 1U, 16U, write_RecoveryGroupRequest))
        return false;
    writer.EndObject();
    return true;
}

bool decode_IpcRequestValueA0(const rapidjson::Value& value, IpcRequestValueA0* out,
                              const std::string& path, ContractError* error)
{
    {
        ModelBoundsOptionsA0 candidate{};
        ContractError ignored;
        if (decode_ModelBoundsOptionsA0(value, &candidate, path, &ignored))
        {
            *out = IpcRequestValueA0(std::in_place_index<0>, std::move(candidate));
            return true;
        }
    }
    {
        HlrProjectionOptionsA0 candidate{};
        ContractError ignored;
        if (decode_HlrProjectionOptionsA0(value, &candidate, path, &ignored))
        {
            *out = IpcRequestValueA0(std::in_place_index<1>, std::move(candidate));
            return true;
        }
    }
    {
        PackedAttachmentProjectionA0 candidate{};
        ContractError ignored;
        if (decode_PackedAttachmentProjectionA0(value, &candidate, path, &ignored))
        {
            *out = IpcRequestValueA0(std::in_place_index<2>, std::move(candidate));
            return true;
        }
    }
    {
        StepTopologyOpenRequestA0 candidate{};
        ContractError ignored;
        if (decode_StepTopologyOpenRequestA0(value, &candidate, path, &ignored))
        {
            *out = IpcRequestValueA0(std::in_place_index<3>, std::move(candidate));
            return true;
        }
    }
    {
        StepTopologyCloseRequestA0 candidate{};
        ContractError ignored;
        if (decode_StepTopologyCloseRequestA0(value, &candidate, path, &ignored))
        {
            *out = IpcRequestValueA0(std::in_place_index<4>, std::move(candidate));
            return true;
        }
    }
    {
        StepTopologyInspectRequestA0 candidate{};
        ContractError ignored;
        if (decode_StepTopologyInspectRequestA0(value, &candidate, path, &ignored))
        {
            *out = IpcRequestValueA0(std::in_place_index<5>, std::move(candidate));
            return true;
        }
    }
    {
        StepTopologyRenderRequestA0 candidate{};
        ContractError ignored;
        if (decode_StepTopologyRenderRequestA0(value, &candidate, path, &ignored))
        {
            *out = IpcRequestValueA0(std::in_place_index<6>, std::move(candidate));
            return true;
        }
    }
    {
        StepTopologyResolveHitRequestA0 candidate{};
        ContractError ignored;
        if (decode_StepTopologyResolveHitRequestA0(value, &candidate, path, &ignored))
        {
            *out = IpcRequestValueA0(std::in_place_index<7>, std::move(candidate));
            return true;
        }
    }
    {
        StepTopologyApplyLogicalGroupsRequestA0 candidate{};
        ContractError ignored;
        if (decode_StepTopologyApplyLogicalGroupsRequestA0(value, &candidate, path, &ignored))
        {
            *out = IpcRequestValueA0(std::in_place_index<8>, std::move(candidate));
            return true;
        }
    }
    {
        StepTopologyApplyMetadataProbesRequestA0 candidate{};
        ContractError ignored;
        if (decode_StepTopologyApplyMetadataProbesRequestA0(value, &candidate, path, &ignored))
        {
            *out = IpcRequestValueA0(std::in_place_index<9>, std::move(candidate));
            return true;
        }
    }
    {
        StepTopologyCheckpointEditJournalRequestA0 candidate{};
        ContractError ignored;
        if (decode_StepTopologyCheckpointEditJournalRequestA0(value, &candidate, path, &ignored))
        {
            *out = IpcRequestValueA0(std::in_place_index<10>, std::move(candidate));
            return true;
        }
    }
    {
        StepTopologyApplyHierarchyRequestA0 candidate{};
        ContractError ignored;
        if (decode_StepTopologyApplyHierarchyRequestA0(value, &candidate, path, &ignored))
        {
            *out = IpcRequestValueA0(std::in_place_index<11>, std::move(candidate));
            return true;
        }
    }
    {
        StepTopologySaveRequestA0 candidate{};
        ContractError ignored;
        if (decode_StepTopologySaveRequestA0(value, &candidate, path, &ignored))
        {
            *out = IpcRequestValueA0(std::in_place_index<12>, std::move(candidate));
            return true;
        }
    }
    {
        StepTopologyRestoreRequestA0 candidate{};
        ContractError ignored;
        if (decode_StepTopologyRestoreRequestA0(value, &candidate, path, &ignored))
        {
            *out = IpcRequestValueA0(std::in_place_index<13>, std::move(candidate));
            return true;
        }
    }
    {
        StepTopologyAnalyzeRecoveryRequestA0 candidate{};
        ContractError ignored;
        if (decode_StepTopologyAnalyzeRecoveryRequestA0(value, &candidate, path, &ignored))
        {
            *out = IpcRequestValueA0(std::in_place_index<14>, std::move(candidate));
            return true;
        }
    }
    return fail(error, "geometer.contract.union_mismatch", path,
                "Value does not match a union variant.");
}

bool write_IpcRequestValueA0(rapidjson::Writer<rapidjson::StringBuffer>& writer,
                             const IpcRequestValueA0& value, ContractError* error)
{
    switch (value.index())
    {
    case 0:
        return write_ModelBoundsOptionsA0(writer, std::get<0>(value), error);
    case 1:
        return write_HlrProjectionOptionsA0(writer, std::get<1>(value), error);
    case 2:
        return write_PackedAttachmentProjectionA0(writer, std::get<2>(value), error);
    case 3:
        return write_StepTopologyOpenRequestA0(writer, std::get<3>(value), error);
    case 4:
        return write_StepTopologyCloseRequestA0(writer, std::get<4>(value), error);
    case 5:
        return write_StepTopologyInspectRequestA0(writer, std::get<5>(value), error);
    case 6:
        return write_StepTopologyRenderRequestA0(writer, std::get<6>(value), error);
    case 7:
        return write_StepTopologyResolveHitRequestA0(writer, std::get<7>(value), error);
    case 8:
        return write_StepTopologyApplyLogicalGroupsRequestA0(writer, std::get<8>(value), error);
    case 9:
        return write_StepTopologyApplyMetadataProbesRequestA0(writer, std::get<9>(value), error);
    case 10:
        return write_StepTopologyCheckpointEditJournalRequestA0(writer, std::get<10>(value), error);
    case 11:
        return write_StepTopologyApplyHierarchyRequestA0(writer, std::get<11>(value), error);
    case 12:
        return write_StepTopologySaveRequestA0(writer, std::get<12>(value), error);
    case 13:
        return write_StepTopologyRestoreRequestA0(writer, std::get<13>(value), error);
    case 14:
        return write_StepTopologyAnalyzeRecoveryRequestA0(writer, std::get<14>(value), error);
    default:
        return fail(error, "geometer.contract.union_mismatch", "", "Unknown union variant.");
    }
}

bool decode_IpcRequestA0(const rapidjson::Value& value, IpcRequestA0* out, const std::string& path,
                         ContractError* error)
{
    static const char* const names[] = {"operation", "request"};
    if (!validate_object(value, names, 2U, path, error))
        return false;
    {
        const auto member = value.FindMember("operation");
        if (member == value.MemberEnd())
            return fail(error, "geometer.contract.missing_field", child_path(path, "operation"),
                        "Required field is missing.");
        if (!decode_string(member->value, &out->operation, child_path(path, "operation"), error, 1U,
                           128U))
            return false;
    }
    {
        const auto member = value.FindMember("request");
        if (member == value.MemberEnd())
            return fail(error, "geometer.contract.missing_field", child_path(path, "request"),
                        "Required field is missing.");
        if (!decode_IpcRequestValueA0(member->value, &out->request, child_path(path, "request"),
                                      error))
            return false;
    }
    return true;
}

bool write_IpcRequestA0(rapidjson::Writer<rapidjson::StringBuffer>& writer,
                        const IpcRequestA0& value, ContractError* error)
{
    writer.StartObject();
    writer.Key("operation");
    if (!write_string(writer, value.operation, error, 1U, 128U))
        return false;
    writer.Key("request");
    if (!write_IpcRequestValueA0(writer, value.request, error))
        return false;
    writer.EndObject();
    return true;
}

bool decode_IpcShutdownAckA0(const rapidjson::Value& value, IpcShutdownAckA0* out,
                             const std::string& path, ContractError* error)
{
    static const char* const names[] = {"status", "activeRequestCompleted",
                                        "rejectedQueuedRequestCount"};
    if (!validate_object(value, names, 3U, path, error))
        return false;
    {
        const auto member = value.FindMember("status");
        if (member == value.MemberEnd())
            return fail(error, "geometer.contract.missing_field", child_path(path, "status"),
                        "Required field is missing.");
        if (!decode_literal_string(member->value, &out->status, child_path(path, "status"), error,
                                   "complete"))
            return false;
    }
    {
        const auto member = value.FindMember("activeRequestCompleted");
        if (member == value.MemberEnd())
            return fail(error, "geometer.contract.missing_field",
                        child_path(path, "activeRequestCompleted"), "Required field is missing.");
        if (!decode_boolean(member->value, &out->activeRequestCompleted,
                            child_path(path, "activeRequestCompleted"), error))
            return false;
    }
    {
        const auto member = value.FindMember("rejectedQueuedRequestCount");
        if (member == value.MemberEnd())
            return fail(error, "geometer.contract.missing_field",
                        child_path(path, "rejectedQueuedRequestCount"),
                        "Required field is missing.");
        if (!decode_uint32(member->value, &out->rejectedQueuedRequestCount,
                           child_path(path, "rejectedQueuedRequestCount"), error, 0ULL,
                           std::numeric_limits<std::uint32_t>::max()))
            return false;
    }
    return true;
}

bool write_IpcShutdownAckA0(rapidjson::Writer<rapidjson::StringBuffer>& writer,
                            const IpcShutdownAckA0& value, ContractError* error)
{
    writer.StartObject();
    writer.Key("status");
    if (!write_literal_string(writer, value.status, error, "complete"))
        return false;
    writer.Key("activeRequestCompleted");
    if (!(writer.Bool(value.activeRequestCompleted), true))
        return false;
    writer.Key("rejectedQueuedRequestCount");
    if (!write_uint32(writer, value.rejectedQueuedRequestCount, error, 0ULL,
                      std::numeric_limits<std::uint32_t>::max()))
        return false;
    writer.EndObject();
    return true;
}

bool decode_IpcWelcomeA0(const rapidjson::Value& value, IpcWelcomeA0* out, const std::string& path,
                         ContractError* error)
{
    static const char* const names[] = {"release_version", "c_abi_generation",  "ipc",
                                        "catalog_sha256",  "operation_catalog", "limits",
                                        "capabilities"};
    if (!validate_object(value, names, 7U, path, error))
        return false;
    {
        const auto member = value.FindMember("release_version");
        if (member == value.MemberEnd())
            return fail(error, "geometer.contract.missing_field",
                        child_path(path, "release_version"), "Required field is missing.");
        if (!decode_string(member->value, &out->release_version,
                           child_path(path, "release_version"), error, 1U, 32U))
            return false;
    }
    {
        const auto member = value.FindMember("c_abi_generation");
        if (member == value.MemberEnd())
            return fail(error, "geometer.contract.missing_field",
                        child_path(path, "c_abi_generation"), "Required field is missing.");
        if (!decode_uint32(member->value, &out->c_abi_generation,
                           child_path(path, "c_abi_generation"), error, 0ULL,
                           std::numeric_limits<std::uint32_t>::max()))
            return false;
    }
    {
        const auto member = value.FindMember("ipc");
        if (member == value.MemberEnd())
            return fail(error, "geometer.contract.missing_field", child_path(path, "ipc"),
                        "Required field is missing.");
        if (!decode_literal_string(member->value, &out->ipc, child_path(path, "ipc"), error, "a0"))
            return false;
    }
    {
        const auto member = value.FindMember("catalog_sha256");
        if (member == value.MemberEnd())
            return fail(error, "geometer.contract.missing_field",
                        child_path(path, "catalog_sha256"), "Required field is missing.");
        if (!decode_string(member->value, &out->catalog_sha256, child_path(path, "catalog_sha256"),
                           error, 64U, 64U))
            return false;
    }
    {
        const auto member = value.FindMember("operation_catalog");
        if (member == value.MemberEnd())
            return fail(error, "geometer.contract.missing_field",
                        child_path(path, "operation_catalog"), "Required field is missing.");
        if (!decode_IpcOperationCatalogA0(member->value, &out->operation_catalog,
                                          child_path(path, "operation_catalog"), error))
            return false;
    }
    {
        const auto member = value.FindMember("limits");
        if (member == value.MemberEnd())
            return fail(error, "geometer.contract.missing_field", child_path(path, "limits"),
                        "Required field is missing.");
        if (!decode_IpcEffectiveLimitsA0(member->value, &out->limits, child_path(path, "limits"),
                                         error))
            return false;
    }
    {
        const auto member = value.FindMember("capabilities");
        if (member == value.MemberEnd())
            return fail(error, "geometer.contract.missing_field", child_path(path, "capabilities"),
                        "Required field is missing.");
        if (!decode_array(member->value, &out->capabilities, child_path(path, "capabilities"),
                          error, 0U, 64U, decode_string_item))
            return false;
    }
    return true;
}

bool write_IpcWelcomeA0(rapidjson::Writer<rapidjson::StringBuffer>& writer,
                        const IpcWelcomeA0& value, ContractError* error)
{
    writer.StartObject();
    writer.Key("release_version");
    if (!write_string(writer, value.release_version, error, 1U, 32U))
        return false;
    writer.Key("c_abi_generation");
    if (!write_uint32(writer, value.c_abi_generation, error, 0ULL,
                      std::numeric_limits<std::uint32_t>::max()))
        return false;
    writer.Key("ipc");
    if (!write_literal_string(writer, value.ipc, error, "a0"))
        return false;
    writer.Key("catalog_sha256");
    if (!write_string(writer, value.catalog_sha256, error, 64U, 64U))
        return false;
    writer.Key("operation_catalog");
    if (!write_IpcOperationCatalogA0(writer, value.operation_catalog, error))
        return false;
    writer.Key("limits");
    if (!write_IpcEffectiveLimitsA0(writer, value.limits, error))
        return false;
    writer.Key("capabilities");
    if (!write_array(writer, value.capabilities, error, 0U, 64U, write_string_item))
        return false;
    writer.EndObject();
    return true;
}

bool decode_IllustrationMatrix4x4(const rapidjson::Value& value, IllustrationMatrix4x4* out,
                                  const std::string& path, ContractError* error)
{
    if (!value.IsArray() || value.Size() < 16U || value.Size() > 16U)
        return fail(error, "geometer.contract.array_size", path,
                    "Array length is outside its contract bounds.");
    out->clear();
    out->reserve(value.Size());
    for (rapidjson::SizeType i = 0; i < value.Size(); ++i)
    {
        double item_value{};
        if (!decode_double(value[i], &item_value, path + "/" + std::to_string(i), error,
                           -std::numeric_limits<double>::infinity(),
                           std::numeric_limits<double>::infinity(), false, false))
            return false;
        out->push_back(std::move(item_value));
    }
    return true;
}

bool write_IllustrationMatrix4x4(rapidjson::Writer<rapidjson::StringBuffer>& writer,
                                 const IllustrationMatrix4x4& value, ContractError* error)
{
    if (value.size() < 16U || value.size() > 16U)
        return fail(error, "geometer.contract.array_size", "",
                    "Array length is outside its contract bounds.");
    writer.StartArray();
    for (const auto& item_value : value)
        if (!write_double(writer, item_value, error, -std::numeric_limits<double>::infinity(),
                          std::numeric_limits<double>::infinity(), false, false))
            return false;
    writer.EndArray();
    return true;
}

bool decode_IllustrationVector3(const rapidjson::Value& value, IllustrationVector3* out,
                                const std::string& path, ContractError* error)
{
    if (!value.IsArray() || value.Size() < 3U || value.Size() > 3U)
        return fail(error, "geometer.contract.array_size", path,
                    "Array length is outside its contract bounds.");
    out->clear();
    out->reserve(value.Size());
    for (rapidjson::SizeType i = 0; i < value.Size(); ++i)
    {
        double item_value{};
        if (!decode_double(value[i], &item_value, path + "/" + std::to_string(i), error,
                           -std::numeric_limits<double>::infinity(),
                           std::numeric_limits<double>::infinity(), false, false))
            return false;
        out->push_back(std::move(item_value));
    }
    return true;
}

bool write_IllustrationVector3(rapidjson::Writer<rapidjson::StringBuffer>& writer,
                               const IllustrationVector3& value, ContractError* error)
{
    if (value.size() < 3U || value.size() > 3U)
        return fail(error, "geometer.contract.array_size", "",
                    "Array length is outside its contract bounds.");
    writer.StartArray();
    for (const auto& item_value : value)
        if (!write_double(writer, item_value, error, -std::numeric_limits<double>::infinity(),
                          std::numeric_limits<double>::infinity(), false, false))
            return false;
    writer.EndArray();
    return true;
}

bool decode_MeshIllustrationMaterial(const rapidjson::Value& value, MeshIllustrationMaterial* out,
                                     const std::string& path, ContractError* error)
{
    static const char* const names[] = {"color", "opacity", "name"};
    if (!validate_object(value, names, 3U, path, error))
        return false;
    {
        const auto member = value.FindMember("color");
        if (member == value.MemberEnd())
            return fail(error, "geometer.contract.missing_field", child_path(path, "color"),
                        "Required field is missing.");
        if (!decode_IllustrationVector3(member->value, &out->color, child_path(path, "color"),
                                        error))
            return false;
    }
    {
        const auto member = value.FindMember("opacity");
        if (member != value.MemberEnd())
        {
            double decoded{};
            if (!decode_double(member->value, &decoded, child_path(path, "opacity"), error, 0, 1,
                               false, false))
                return false;
            out->opacity = std::move(decoded);
        }
        else
            out->opacity.reset();
    }
    {
        const auto member = value.FindMember("name");
        if (member != value.MemberEnd())
        {
            std::string decoded{};
            if (!decode_string(member->value, &decoded, child_path(path, "name"), error, 0U, 1024U))
                return false;
            out->name = std::move(decoded);
        }
        else
            out->name.reset();
    }
    return true;
}

bool write_MeshIllustrationMaterial(rapidjson::Writer<rapidjson::StringBuffer>& writer,
                                    const MeshIllustrationMaterial& value, ContractError* error)
{
    writer.StartObject();
    writer.Key("color");
    if (!write_IllustrationVector3(writer, value.color, error))
        return false;
    if (value.opacity.has_value())
    {
        writer.Key("opacity");
        if (!write_double(writer, *value.opacity, error, 0, 1, false, false))
            return false;
    }
    if (value.name.has_value())
    {
        writer.Key("name");
        if (!write_string(writer, *value.name, error, 0U, 1024U))
            return false;
    }
    writer.EndObject();
    return true;
}

bool decode_MeshIllustrationMesh(const rapidjson::Value& value, MeshIllustrationMesh* out,
                                 const std::string& path, ContractError* error)
{
    static const char* const names[] = {"id",
                                        "positions",
                                        "normals",
                                        "indices",
                                        "matrix",
                                        "materials",
                                        "triangle_material_indices",
                                        "double_sided"};
    if (!validate_object(value, names, 8U, path, error))
        return false;
    {
        const auto member = value.FindMember("id");
        if (member == value.MemberEnd())
            return fail(error, "geometer.contract.missing_field", child_path(path, "id"),
                        "Required field is missing.");
        if (!decode_string(member->value, &out->id, child_path(path, "id"), error, 1U, 1024U))
            return false;
    }
    {
        const auto member = value.FindMember("positions");
        if (member == value.MemberEnd())
            return fail(error, "geometer.contract.missing_field", child_path(path, "positions"),
                        "Required field is missing.");
        if (!decode_array(member->value, &out->positions, child_path(path, "positions"), error, 9U,
                          6000000U, decode_double_item))
            return false;
    }
    {
        const auto member = value.FindMember("normals");
        if (member != value.MemberEnd())
        {
            std::vector<double> decoded{};
            if (!decode_array(member->value, &decoded, child_path(path, "normals"), error, 0U,
                              6000000U, decode_double_item))
                return false;
            out->normals = std::move(decoded);
        }
        else
            out->normals.reset();
    }
    {
        const auto member = value.FindMember("indices");
        if (member != value.MemberEnd())
        {
            std::vector<std::uint32_t> decoded{};
            if (!decode_array(member->value, &decoded, child_path(path, "indices"), error, 0U,
                              6000000U, decode_uint32_item))
                return false;
            out->indices = std::move(decoded);
        }
        else
            out->indices.reset();
    }
    {
        const auto member = value.FindMember("matrix");
        if (member != value.MemberEnd())
        {
            IllustrationMatrix4x4 decoded{};
            if (!decode_IllustrationMatrix4x4(member->value, &decoded, child_path(path, "matrix"),
                                              error))
                return false;
            out->matrix = std::move(decoded);
        }
        else
            out->matrix.reset();
    }
    {
        const auto member = value.FindMember("materials");
        if (member == value.MemberEnd())
            return fail(error, "geometer.contract.missing_field", child_path(path, "materials"),
                        "Required field is missing.");
        if (!decode_array(member->value, &out->materials, child_path(path, "materials"), error, 1U,
                          65536U, decode_MeshIllustrationMaterial))
            return false;
    }
    {
        const auto member = value.FindMember("triangle_material_indices");
        if (member != value.MemberEnd())
        {
            std::vector<std::uint32_t> decoded{};
            if (!decode_array(member->value, &decoded,
                              child_path(path, "triangle_material_indices"), error, 0U, 2000000U,
                              decode_uint32_item))
                return false;
            out->triangle_material_indices = std::move(decoded);
        }
        else
            out->triangle_material_indices.reset();
    }
    {
        const auto member = value.FindMember("double_sided");
        if (member != value.MemberEnd())
        {
            bool decoded{};
            if (!decode_boolean(member->value, &decoded, child_path(path, "double_sided"), error))
                return false;
            out->double_sided = std::move(decoded);
        }
        else
            out->double_sided.reset();
    }
    return true;
}

bool write_MeshIllustrationMesh(rapidjson::Writer<rapidjson::StringBuffer>& writer,
                                const MeshIllustrationMesh& value, ContractError* error)
{
    writer.StartObject();
    writer.Key("id");
    if (!write_string(writer, value.id, error, 1U, 1024U))
        return false;
    writer.Key("positions");
    if (!write_array(writer, value.positions, error, 9U, 6000000U, write_double_item))
        return false;
    if (value.normals.has_value())
    {
        writer.Key("normals");
        if (!write_array(writer, *value.normals, error, 0U, 6000000U, write_double_item))
            return false;
    }
    if (value.indices.has_value())
    {
        writer.Key("indices");
        if (!write_array(writer, *value.indices, error, 0U, 6000000U, write_uint32_item))
            return false;
    }
    if (value.matrix.has_value())
    {
        writer.Key("matrix");
        if (!write_IllustrationMatrix4x4(writer, *value.matrix, error))
            return false;
    }
    writer.Key("materials");
    if (!write_array(writer, value.materials, error, 1U, 65536U, write_MeshIllustrationMaterial))
        return false;
    if (value.triangle_material_indices.has_value())
    {
        writer.Key("triangle_material_indices");
        if (!write_array(writer, *value.triangle_material_indices, error, 0U, 2000000U,
                         write_uint32_item))
            return false;
    }
    if (value.double_sided.has_value())
    {
        writer.Key("double_sided");
        if (!(writer.Bool(*value.double_sided), true))
            return false;
    }
    writer.EndObject();
    return true;
}

bool decode_MeshIllustrationView(const rapidjson::Value& value, MeshIllustrationView* out,
                                 const std::string& path, ContractError* error)
{
    static const char* const names[] = {"direction", "up", "mirror_x"};
    if (!validate_object(value, names, 3U, path, error))
        return false;
    {
        const auto member = value.FindMember("direction");
        if (member == value.MemberEnd())
            return fail(error, "geometer.contract.missing_field", child_path(path, "direction"),
                        "Required field is missing.");
        if (!decode_IllustrationVector3(member->value, &out->direction,
                                        child_path(path, "direction"), error))
            return false;
    }
    {
        const auto member = value.FindMember("up");
        if (member == value.MemberEnd())
            return fail(error, "geometer.contract.missing_field", child_path(path, "up"),
                        "Required field is missing.");
        if (!decode_IllustrationVector3(member->value, &out->up, child_path(path, "up"), error))
            return false;
    }
    {
        const auto member = value.FindMember("mirror_x");
        if (member != value.MemberEnd())
        {
            bool decoded{};
            if (!decode_boolean(member->value, &decoded, child_path(path, "mirror_x"), error))
                return false;
            out->mirror_x = std::move(decoded);
        }
        else
            out->mirror_x.reset();
    }
    return true;
}

bool write_MeshIllustrationView(rapidjson::Writer<rapidjson::StringBuffer>& writer,
                                const MeshIllustrationView& value, ContractError* error)
{
    writer.StartObject();
    writer.Key("direction");
    if (!write_IllustrationVector3(writer, value.direction, error))
        return false;
    writer.Key("up");
    if (!write_IllustrationVector3(writer, value.up, error))
        return false;
    if (value.mirror_x.has_value())
    {
        writer.Key("mirror_x");
        if (!(writer.Bool(*value.mirror_x), true))
            return false;
    }
    writer.EndObject();
    return true;
}

bool decode_MeshIllustrationPrepareOptions(const rapidjson::Value& value,
                                           MeshIllustrationPrepareOptions* out,
                                           const std::string& path, ContractError* error)
{
    static const char* const names[] = {"max_triangles", "weld_tolerance"};
    if (!validate_object(value, names, 2U, path, error))
        return false;
    {
        const auto member = value.FindMember("max_triangles");
        if (member != value.MemberEnd())
        {
            std::uint32_t decoded{};
            if (!decode_uint32(member->value, &decoded, child_path(path, "max_triangles"), error,
                               1ULL, 2000000ULL))
                return false;
            out->max_triangles = std::move(decoded);
        }
        else
            out->max_triangles.reset();
    }
    {
        const auto member = value.FindMember("weld_tolerance");
        if (member != value.MemberEnd())
        {
            double decoded{};
            if (!decode_double(member->value, &decoded, child_path(path, "weld_tolerance"), error,
                               0, std::numeric_limits<double>::infinity(), true, false))
                return false;
            out->weld_tolerance = std::move(decoded);
        }
        else
            out->weld_tolerance.reset();
    }
    return true;
}

bool write_MeshIllustrationPrepareOptions(rapidjson::Writer<rapidjson::StringBuffer>& writer,
                                          const MeshIllustrationPrepareOptions& value,
                                          ContractError* error)
{
    writer.StartObject();
    if (value.max_triangles.has_value())
    {
        writer.Key("max_triangles");
        if (!write_uint32(writer, *value.max_triangles, error, 1ULL, 2000000ULL))
            return false;
    }
    if (value.weld_tolerance.has_value())
    {
        writer.Key("weld_tolerance");
        if (!write_double(writer, *value.weld_tolerance, error, 0,
                          std::numeric_limits<double>::infinity(), true, false))
            return false;
    }
    writer.EndObject();
    return true;
}

bool decode_MeshIllustrationShading(const rapidjson::Value& value, MeshIllustrationShading* out,
                                    const std::string& path, ContractError* error)
{
    if (!value.IsString())
        return fail(error, "geometer.contract.type_mismatch", path, "Expected a string enum.");
    const std::string text(value.GetString(), value.GetStringLength());
    if (text == "unlit")
    {
        *out = MeshIllustrationShading::unlit;
        return true;
    }
    if (text == "flat")
    {
        *out = MeshIllustrationShading::flat;
        return true;
    }
    if (text == "lambert")
    {
        *out = MeshIllustrationShading::lambert;
        return true;
    }
    if (text == "banded")
    {
        *out = MeshIllustrationShading::banded;
        return true;
    }
    if (text == "toon")
    {
        *out = MeshIllustrationShading::toon;
        return true;
    }
    return fail(error, "geometer.contract.enum_mismatch", path, "Unknown enum value.");
}

bool write_MeshIllustrationShading(rapidjson::Writer<rapidjson::StringBuffer>& writer,
                                   const MeshIllustrationShading& value, ContractError* error)
{
    switch (value)
    {
    case MeshIllustrationShading::unlit:
        writer.String("unlit");
        return true;
    case MeshIllustrationShading::flat:
        writer.String("flat");
        return true;
    case MeshIllustrationShading::lambert:
        writer.String("lambert");
        return true;
    case MeshIllustrationShading::banded:
        writer.String("banded");
        return true;
    case MeshIllustrationShading::toon:
        writer.String("toon");
        return true;
    }
    return fail(error, "geometer.contract.enum_mismatch", "", "Unknown enum value.");
}

bool decode_MeshIllustrationStyleA0(const rapidjson::Value& value, MeshIllustrationStyleA0* out,
                                    const std::string& path, ContractError* error)
{
    static const char* const names[] = {"shading",
                                        "ambient",
                                        "key_intensity",
                                        "light_direction",
                                        "bands",
                                        "source_colors",
                                        "fallback_color",
                                        "background",
                                        "transparent_background",
                                        "fuse_surfaces",
                                        "layer_coplanar_materials",
                                        "show_hlr_outline",
                                        "show_hlr_detail",
                                        "show_outlines",
                                        "show_creases",
                                        "crease_angle_degrees",
                                        "outline_color",
                                        "crease_color",
                                        "outline_width",
                                        "crease_width",
                                        "double_sided",
                                        "rim_amount"};
    if (!validate_object(value, names, 22U, path, error))
        return false;
    {
        const auto member = value.FindMember("shading");
        if (member != value.MemberEnd())
        {
            MeshIllustrationShading decoded{};
            if (!decode_MeshIllustrationShading(member->value, &decoded,
                                                child_path(path, "shading"), error))
                return false;
            out->shading = std::move(decoded);
        }
        else
            out->shading.reset();
    }
    {
        const auto member = value.FindMember("ambient");
        if (member != value.MemberEnd())
        {
            double decoded{};
            if (!decode_double(member->value, &decoded, child_path(path, "ambient"), error, 0, 1,
                               false, false))
                return false;
            out->ambient = std::move(decoded);
        }
        else
            out->ambient.reset();
    }
    {
        const auto member = value.FindMember("key_intensity");
        if (member != value.MemberEnd())
        {
            double decoded{};
            if (!decode_double(member->value, &decoded, child_path(path, "key_intensity"), error, 0,
                               4, false, false))
                return false;
            out->key_intensity = std::move(decoded);
        }
        else
            out->key_intensity.reset();
    }
    {
        const auto member = value.FindMember("light_direction");
        if (member != value.MemberEnd())
        {
            IllustrationVector3 decoded{};
            if (!decode_IllustrationVector3(member->value, &decoded,
                                            child_path(path, "light_direction"), error))
                return false;
            out->light_direction = std::move(decoded);
        }
        else
            out->light_direction.reset();
    }
    {
        const auto member = value.FindMember("bands");
        if (member != value.MemberEnd())
        {
            std::uint32_t decoded{};
            if (!decode_uint32(member->value, &decoded, child_path(path, "bands"), error, 1ULL,
                               256ULL))
                return false;
            out->bands = std::move(decoded);
        }
        else
            out->bands.reset();
    }
    {
        const auto member = value.FindMember("source_colors");
        if (member != value.MemberEnd())
        {
            bool decoded{};
            if (!decode_boolean(member->value, &decoded, child_path(path, "source_colors"), error))
                return false;
            out->source_colors = std::move(decoded);
        }
        else
            out->source_colors.reset();
    }
    {
        const auto member = value.FindMember("fallback_color");
        if (member != value.MemberEnd())
        {
            IllustrationVector3 decoded{};
            if (!decode_IllustrationVector3(member->value, &decoded,
                                            child_path(path, "fallback_color"), error))
                return false;
            out->fallback_color = std::move(decoded);
        }
        else
            out->fallback_color.reset();
    }
    {
        const auto member = value.FindMember("background");
        if (member != value.MemberEnd())
        {
            std::string decoded{};
            if (!decode_string(member->value, &decoded, child_path(path, "background"), error, 1U,
                               128U))
                return false;
            out->background = std::move(decoded);
        }
        else
            out->background.reset();
    }
    {
        const auto member = value.FindMember("transparent_background");
        if (member != value.MemberEnd())
        {
            bool decoded{};
            if (!decode_boolean(member->value, &decoded, child_path(path, "transparent_background"),
                                error))
                return false;
            out->transparent_background = std::move(decoded);
        }
        else
            out->transparent_background.reset();
    }
    {
        const auto member = value.FindMember("fuse_surfaces");
        if (member != value.MemberEnd())
        {
            bool decoded{};
            if (!decode_boolean(member->value, &decoded, child_path(path, "fuse_surfaces"), error))
                return false;
            out->fuse_surfaces = std::move(decoded);
        }
        else
            out->fuse_surfaces.reset();
    }
    {
        const auto member = value.FindMember("layer_coplanar_materials");
        if (member != value.MemberEnd())
        {
            bool decoded{};
            if (!decode_boolean(member->value, &decoded,
                                child_path(path, "layer_coplanar_materials"), error))
                return false;
            out->layer_coplanar_materials = std::move(decoded);
        }
        else
            out->layer_coplanar_materials.reset();
    }
    {
        const auto member = value.FindMember("show_hlr_outline");
        if (member != value.MemberEnd())
        {
            bool decoded{};
            if (!decode_boolean(member->value, &decoded, child_path(path, "show_hlr_outline"),
                                error))
                return false;
            out->show_hlr_outline = std::move(decoded);
        }
        else
            out->show_hlr_outline.reset();
    }
    {
        const auto member = value.FindMember("show_hlr_detail");
        if (member != value.MemberEnd())
        {
            bool decoded{};
            if (!decode_boolean(member->value, &decoded, child_path(path, "show_hlr_detail"),
                                error))
                return false;
            out->show_hlr_detail = std::move(decoded);
        }
        else
            out->show_hlr_detail.reset();
    }
    {
        const auto member = value.FindMember("show_outlines");
        if (member != value.MemberEnd())
        {
            bool decoded{};
            if (!decode_boolean(member->value, &decoded, child_path(path, "show_outlines"), error))
                return false;
            out->show_outlines = std::move(decoded);
        }
        else
            out->show_outlines.reset();
    }
    {
        const auto member = value.FindMember("show_creases");
        if (member != value.MemberEnd())
        {
            bool decoded{};
            if (!decode_boolean(member->value, &decoded, child_path(path, "show_creases"), error))
                return false;
            out->show_creases = std::move(decoded);
        }
        else
            out->show_creases.reset();
    }
    {
        const auto member = value.FindMember("crease_angle_degrees");
        if (member != value.MemberEnd())
        {
            double decoded{};
            if (!decode_double(member->value, &decoded, child_path(path, "crease_angle_degrees"),
                               error, 0, 180, false, false))
                return false;
            out->crease_angle_degrees = std::move(decoded);
        }
        else
            out->crease_angle_degrees.reset();
    }
    {
        const auto member = value.FindMember("outline_color");
        if (member != value.MemberEnd())
        {
            std::string decoded{};
            if (!decode_string(member->value, &decoded, child_path(path, "outline_color"), error,
                               1U, 128U))
                return false;
            out->outline_color = std::move(decoded);
        }
        else
            out->outline_color.reset();
    }
    {
        const auto member = value.FindMember("crease_color");
        if (member != value.MemberEnd())
        {
            std::string decoded{};
            if (!decode_string(member->value, &decoded, child_path(path, "crease_color"), error, 1U,
                               128U))
                return false;
            out->crease_color = std::move(decoded);
        }
        else
            out->crease_color.reset();
    }
    {
        const auto member = value.FindMember("outline_width");
        if (member != value.MemberEnd())
        {
            double decoded{};
            if (!decode_double(member->value, &decoded, child_path(path, "outline_width"), error, 0,
                               std::numeric_limits<double>::infinity(), false, false))
                return false;
            out->outline_width = std::move(decoded);
        }
        else
            out->outline_width.reset();
    }
    {
        const auto member = value.FindMember("crease_width");
        if (member != value.MemberEnd())
        {
            double decoded{};
            if (!decode_double(member->value, &decoded, child_path(path, "crease_width"), error, 0,
                               std::numeric_limits<double>::infinity(), false, false))
                return false;
            out->crease_width = std::move(decoded);
        }
        else
            out->crease_width.reset();
    }
    {
        const auto member = value.FindMember("double_sided");
        if (member != value.MemberEnd())
        {
            bool decoded{};
            if (!decode_boolean(member->value, &decoded, child_path(path, "double_sided"), error))
                return false;
            out->double_sided = std::move(decoded);
        }
        else
            out->double_sided.reset();
    }
    {
        const auto member = value.FindMember("rim_amount");
        if (member != value.MemberEnd())
        {
            double decoded{};
            if (!decode_double(member->value, &decoded, child_path(path, "rim_amount"), error, 0, 1,
                               false, false))
                return false;
            out->rim_amount = std::move(decoded);
        }
        else
            out->rim_amount.reset();
    }
    return true;
}

bool write_MeshIllustrationStyleA0(rapidjson::Writer<rapidjson::StringBuffer>& writer,
                                   const MeshIllustrationStyleA0& value, ContractError* error)
{
    writer.StartObject();
    if (value.shading.has_value())
    {
        writer.Key("shading");
        if (!write_MeshIllustrationShading(writer, *value.shading, error))
            return false;
    }
    if (value.ambient.has_value())
    {
        writer.Key("ambient");
        if (!write_double(writer, *value.ambient, error, 0, 1, false, false))
            return false;
    }
    if (value.key_intensity.has_value())
    {
        writer.Key("key_intensity");
        if (!write_double(writer, *value.key_intensity, error, 0, 4, false, false))
            return false;
    }
    if (value.light_direction.has_value())
    {
        writer.Key("light_direction");
        if (!write_IllustrationVector3(writer, *value.light_direction, error))
            return false;
    }
    if (value.bands.has_value())
    {
        writer.Key("bands");
        if (!write_uint32(writer, *value.bands, error, 1ULL, 256ULL))
            return false;
    }
    if (value.source_colors.has_value())
    {
        writer.Key("source_colors");
        if (!(writer.Bool(*value.source_colors), true))
            return false;
    }
    if (value.fallback_color.has_value())
    {
        writer.Key("fallback_color");
        if (!write_IllustrationVector3(writer, *value.fallback_color, error))
            return false;
    }
    if (value.background.has_value())
    {
        writer.Key("background");
        if (!write_string(writer, *value.background, error, 1U, 128U))
            return false;
    }
    if (value.transparent_background.has_value())
    {
        writer.Key("transparent_background");
        if (!(writer.Bool(*value.transparent_background), true))
            return false;
    }
    if (value.fuse_surfaces.has_value())
    {
        writer.Key("fuse_surfaces");
        if (!(writer.Bool(*value.fuse_surfaces), true))
            return false;
    }
    if (value.layer_coplanar_materials.has_value())
    {
        writer.Key("layer_coplanar_materials");
        if (!(writer.Bool(*value.layer_coplanar_materials), true))
            return false;
    }
    if (value.show_hlr_outline.has_value())
    {
        writer.Key("show_hlr_outline");
        if (!(writer.Bool(*value.show_hlr_outline), true))
            return false;
    }
    if (value.show_hlr_detail.has_value())
    {
        writer.Key("show_hlr_detail");
        if (!(writer.Bool(*value.show_hlr_detail), true))
            return false;
    }
    if (value.show_outlines.has_value())
    {
        writer.Key("show_outlines");
        if (!(writer.Bool(*value.show_outlines), true))
            return false;
    }
    if (value.show_creases.has_value())
    {
        writer.Key("show_creases");
        if (!(writer.Bool(*value.show_creases), true))
            return false;
    }
    if (value.crease_angle_degrees.has_value())
    {
        writer.Key("crease_angle_degrees");
        if (!write_double(writer, *value.crease_angle_degrees, error, 0, 180, false, false))
            return false;
    }
    if (value.outline_color.has_value())
    {
        writer.Key("outline_color");
        if (!write_string(writer, *value.outline_color, error, 1U, 128U))
            return false;
    }
    if (value.crease_color.has_value())
    {
        writer.Key("crease_color");
        if (!write_string(writer, *value.crease_color, error, 1U, 128U))
            return false;
    }
    if (value.outline_width.has_value())
    {
        writer.Key("outline_width");
        if (!write_double(writer, *value.outline_width, error, 0,
                          std::numeric_limits<double>::infinity(), false, false))
            return false;
    }
    if (value.crease_width.has_value())
    {
        writer.Key("crease_width");
        if (!write_double(writer, *value.crease_width, error, 0,
                          std::numeric_limits<double>::infinity(), false, false))
            return false;
    }
    if (value.double_sided.has_value())
    {
        writer.Key("double_sided");
        if (!(writer.Bool(*value.double_sided), true))
            return false;
    }
    if (value.rim_amount.has_value())
    {
        writer.Key("rim_amount");
        if (!write_double(writer, *value.rim_amount, error, 0, 1, false, false))
            return false;
    }
    writer.EndObject();
    return true;
}

bool decode_MeshIllustrationSvgOptions(const rapidjson::Value& value,
                                       MeshIllustrationSvgOptions* out, const std::string& path,
                                       ContractError* error)
{
    static const char* const names[] = {"coordinate_span", "title"};
    if (!validate_object(value, names, 2U, path, error))
        return false;
    {
        const auto member = value.FindMember("coordinate_span");
        if (member != value.MemberEnd())
        {
            std::uint32_t decoded{};
            if (!decode_uint32(member->value, &decoded, child_path(path, "coordinate_span"), error,
                               10000ULL, 1000000000ULL))
                return false;
            out->coordinate_span = std::move(decoded);
        }
        else
            out->coordinate_span.reset();
    }
    {
        const auto member = value.FindMember("title");
        if (member != value.MemberEnd())
        {
            std::string decoded{};
            if (!decode_string(member->value, &decoded, child_path(path, "title"), error, 1U,
                               1024U))
                return false;
            out->title = std::move(decoded);
        }
        else
            out->title.reset();
    }
    return true;
}

bool write_MeshIllustrationSvgOptions(rapidjson::Writer<rapidjson::StringBuffer>& writer,
                                      const MeshIllustrationSvgOptions& value, ContractError* error)
{
    writer.StartObject();
    if (value.coordinate_span.has_value())
    {
        writer.Key("coordinate_span");
        if (!write_uint32(writer, *value.coordinate_span, error, 10000ULL, 1000000000ULL))
            return false;
    }
    if (value.title.has_value())
    {
        writer.Key("title");
        if (!write_string(writer, *value.title, error, 1U, 1024U))
            return false;
    }
    writer.EndObject();
    return true;
}

bool decode_MeshIllustrationInputA0(const rapidjson::Value& value, MeshIllustrationInputA0* out,
                                    const std::string& path, ContractError* error)
{
    static const char* const names[] = {"schema", "meshes", "view", "prepare", "style", "svg"};
    if (!validate_object(value, names, 6U, path, error))
        return false;
    {
        const auto member = value.FindMember("schema");
        if (member == value.MemberEnd())
            return fail(error, "geometer.contract.missing_field", child_path(path, "schema"),
                        "Required field is missing.");
        if (!decode_literal_string(member->value, &out->schema, child_path(path, "schema"), error,
                                   "geometry.mesh_illustration.input.a0"))
            return false;
    }
    {
        const auto member = value.FindMember("meshes");
        if (member == value.MemberEnd())
            return fail(error, "geometer.contract.missing_field", child_path(path, "meshes"),
                        "Required field is missing.");
        if (!decode_array(member->value, &out->meshes, child_path(path, "meshes"), error, 1U,
                          65536U, decode_MeshIllustrationMesh))
            return false;
    }
    {
        const auto member = value.FindMember("view");
        if (member == value.MemberEnd())
            return fail(error, "geometer.contract.missing_field", child_path(path, "view"),
                        "Required field is missing.");
        if (!decode_MeshIllustrationView(member->value, &out->view, child_path(path, "view"),
                                         error))
            return false;
    }
    {
        const auto member = value.FindMember("prepare");
        if (member != value.MemberEnd())
        {
            MeshIllustrationPrepareOptions decoded{};
            if (!decode_MeshIllustrationPrepareOptions(member->value, &decoded,
                                                       child_path(path, "prepare"), error))
                return false;
            out->prepare = std::move(decoded);
        }
        else
            out->prepare.reset();
    }
    {
        const auto member = value.FindMember("style");
        if (member != value.MemberEnd())
        {
            MeshIllustrationStyleA0 decoded{};
            if (!decode_MeshIllustrationStyleA0(member->value, &decoded, child_path(path, "style"),
                                                error))
                return false;
            out->style = std::move(decoded);
        }
        else
            out->style.reset();
    }
    {
        const auto member = value.FindMember("svg");
        if (member != value.MemberEnd())
        {
            MeshIllustrationSvgOptions decoded{};
            if (!decode_MeshIllustrationSvgOptions(member->value, &decoded, child_path(path, "svg"),
                                                   error))
                return false;
            out->svg = std::move(decoded);
        }
        else
            out->svg.reset();
    }
    return true;
}

bool write_MeshIllustrationInputA0(rapidjson::Writer<rapidjson::StringBuffer>& writer,
                                   const MeshIllustrationInputA0& value, ContractError* error)
{
    writer.StartObject();
    writer.Key("schema");
    if (!write_literal_string(writer, value.schema, error, "geometry.mesh_illustration.input.a0"))
        return false;
    writer.Key("meshes");
    if (!write_array(writer, value.meshes, error, 1U, 65536U, write_MeshIllustrationMesh))
        return false;
    writer.Key("view");
    if (!write_MeshIllustrationView(writer, value.view, error))
        return false;
    if (value.prepare.has_value())
    {
        writer.Key("prepare");
        if (!write_MeshIllustrationPrepareOptions(writer, *value.prepare, error))
            return false;
    }
    if (value.style.has_value())
    {
        writer.Key("style");
        if (!write_MeshIllustrationStyleA0(writer, *value.style, error))
            return false;
    }
    if (value.svg.has_value())
    {
        writer.Key("svg");
        if (!write_MeshIllustrationSvgOptions(writer, *value.svg, error))
            return false;
    }
    writer.EndObject();
    return true;
}

bool decode_MeshIllustrationRenderStats(const rapidjson::Value& value,
                                        MeshIllustrationRenderStats* out, const std::string& path,
                                        ContractError* error)
{
    static const char* const names[] = {"triangles", "surface_draws", "layered_surfaces",
                                        "outlines",  "details",       "creases",
                                        "commands"};
    if (!validate_object(value, names, 7U, path, error))
        return false;
    {
        const auto member = value.FindMember("triangles");
        if (member == value.MemberEnd())
            return fail(error, "geometer.contract.missing_field", child_path(path, "triangles"),
                        "Required field is missing.");
        if (!decode_uint32(member->value, &out->triangles, child_path(path, "triangles"), error,
                           0ULL, std::numeric_limits<std::uint32_t>::max()))
            return false;
    }
    {
        const auto member = value.FindMember("surface_draws");
        if (member == value.MemberEnd())
            return fail(error, "geometer.contract.missing_field", child_path(path, "surface_draws"),
                        "Required field is missing.");
        if (!decode_uint32(member->value, &out->surface_draws, child_path(path, "surface_draws"),
                           error, 0ULL, std::numeric_limits<std::uint32_t>::max()))
            return false;
    }
    {
        const auto member = value.FindMember("layered_surfaces");
        if (member == value.MemberEnd())
            return fail(error, "geometer.contract.missing_field",
                        child_path(path, "layered_surfaces"), "Required field is missing.");
        if (!decode_uint32(member->value, &out->layered_surfaces,
                           child_path(path, "layered_surfaces"), error, 0ULL,
                           std::numeric_limits<std::uint32_t>::max()))
            return false;
    }
    {
        const auto member = value.FindMember("outlines");
        if (member == value.MemberEnd())
            return fail(error, "geometer.contract.missing_field", child_path(path, "outlines"),
                        "Required field is missing.");
        if (!decode_uint32(member->value, &out->outlines, child_path(path, "outlines"), error, 0ULL,
                           std::numeric_limits<std::uint32_t>::max()))
            return false;
    }
    {
        const auto member = value.FindMember("details");
        if (member == value.MemberEnd())
            return fail(error, "geometer.contract.missing_field", child_path(path, "details"),
                        "Required field is missing.");
        if (!decode_uint32(member->value, &out->details, child_path(path, "details"), error, 0ULL,
                           std::numeric_limits<std::uint32_t>::max()))
            return false;
    }
    {
        const auto member = value.FindMember("creases");
        if (member == value.MemberEnd())
            return fail(error, "geometer.contract.missing_field", child_path(path, "creases"),
                        "Required field is missing.");
        if (!decode_uint32(member->value, &out->creases, child_path(path, "creases"), error, 0ULL,
                           std::numeric_limits<std::uint32_t>::max()))
            return false;
    }
    {
        const auto member = value.FindMember("commands");
        if (member == value.MemberEnd())
            return fail(error, "geometer.contract.missing_field", child_path(path, "commands"),
                        "Required field is missing.");
        if (!decode_uint32(member->value, &out->commands, child_path(path, "commands"), error, 0ULL,
                           std::numeric_limits<std::uint32_t>::max()))
            return false;
    }
    return true;
}

bool write_MeshIllustrationRenderStats(rapidjson::Writer<rapidjson::StringBuffer>& writer,
                                       const MeshIllustrationRenderStats& value,
                                       ContractError* error)
{
    writer.StartObject();
    writer.Key("triangles");
    if (!write_uint32(writer, value.triangles, error, 0ULL,
                      std::numeric_limits<std::uint32_t>::max()))
        return false;
    writer.Key("surface_draws");
    if (!write_uint32(writer, value.surface_draws, error, 0ULL,
                      std::numeric_limits<std::uint32_t>::max()))
        return false;
    writer.Key("layered_surfaces");
    if (!write_uint32(writer, value.layered_surfaces, error, 0ULL,
                      std::numeric_limits<std::uint32_t>::max()))
        return false;
    writer.Key("outlines");
    if (!write_uint32(writer, value.outlines, error, 0ULL,
                      std::numeric_limits<std::uint32_t>::max()))
        return false;
    writer.Key("details");
    if (!write_uint32(writer, value.details, error, 0ULL,
                      std::numeric_limits<std::uint32_t>::max()))
        return false;
    writer.Key("creases");
    if (!write_uint32(writer, value.creases, error, 0ULL,
                      std::numeric_limits<std::uint32_t>::max()))
        return false;
    writer.Key("commands");
    if (!write_uint32(writer, value.commands, error, 0ULL,
                      std::numeric_limits<std::uint32_t>::max()))
        return false;
    writer.EndObject();
    return true;
}

bool decode_MeshIllustrationResultA0(const rapidjson::Value& value, MeshIllustrationResultA0* out,
                                     const std::string& path, ContractError* error)
{
    static const char* const names[] = {"schema", "svg", "stats", "warnings"};
    if (!validate_object(value, names, 4U, path, error))
        return false;
    {
        const auto member = value.FindMember("schema");
        if (member == value.MemberEnd())
            return fail(error, "geometer.contract.missing_field", child_path(path, "schema"),
                        "Required field is missing.");
        if (!decode_literal_string(member->value, &out->schema, child_path(path, "schema"), error,
                                   "geometry.mesh_illustration.result.a0"))
            return false;
    }
    {
        const auto member = value.FindMember("svg");
        if (member == value.MemberEnd())
            return fail(error, "geometer.contract.missing_field", child_path(path, "svg"),
                        "Required field is missing.");
        if (!decode_string(member->value, &out->svg, child_path(path, "svg"), error, 0U,
                           std::numeric_limits<std::size_t>::max()))
            return false;
    }
    {
        const auto member = value.FindMember("stats");
        if (member == value.MemberEnd())
            return fail(error, "geometer.contract.missing_field", child_path(path, "stats"),
                        "Required field is missing.");
        if (!decode_MeshIllustrationRenderStats(member->value, &out->stats,
                                                child_path(path, "stats"), error))
            return false;
    }
    {
        const auto member = value.FindMember("warnings");
        if (member == value.MemberEnd())
            return fail(error, "geometer.contract.missing_field", child_path(path, "warnings"),
                        "Required field is missing.");
        if (!decode_array(member->value, &out->warnings, child_path(path, "warnings"), error, 0U,
                          256U, decode_string_item))
            return false;
    }
    return true;
}

bool write_MeshIllustrationResultA0(rapidjson::Writer<rapidjson::StringBuffer>& writer,
                                    const MeshIllustrationResultA0& value, ContractError* error)
{
    writer.StartObject();
    writer.Key("schema");
    if (!write_literal_string(writer, value.schema, error, "geometry.mesh_illustration.result.a0"))
        return false;
    writer.Key("svg");
    if (!write_string(writer, value.svg, error, 0U, std::numeric_limits<std::size_t>::max()))
        return false;
    writer.Key("stats");
    if (!write_MeshIllustrationRenderStats(writer, value.stats, error))
        return false;
    writer.Key("warnings");
    if (!write_array(writer, value.warnings, error, 0U, 256U, write_string_item))
        return false;
    writer.EndObject();
    return true;
}

bool decode_ModelBoundsSource(const rapidjson::Value& value, ModelBoundsSource* out,
                              const std::string& path, ContractError* error)
{
    static const char* const names[] = {"format", "hash"};
    if (!validate_object(value, names, 2U, path, error))
        return false;
    {
        const auto member = value.FindMember("format");
        if (member == value.MemberEnd())
            return fail(error, "geometer.contract.missing_field", child_path(path, "format"),
                        "Required field is missing.");
        if (!decode_ModelFormat(member->value, &out->format, child_path(path, "format"), error))
            return false;
    }
    {
        const auto member = value.FindMember("hash");
        if (member == value.MemberEnd())
            return fail(error, "geometer.contract.missing_field", child_path(path, "hash"),
                        "Required field is missing.");
        if (!decode_string(member->value, &out->hash, child_path(path, "hash"), error, 0U,
                           std::numeric_limits<std::size_t>::max()))
            return false;
    }
    return true;
}

bool write_ModelBoundsSource(rapidjson::Writer<rapidjson::StringBuffer>& writer,
                             const ModelBoundsSource& value, ContractError* error)
{
    writer.StartObject();
    writer.Key("format");
    if (!write_ModelFormat(writer, value.format, error))
        return false;
    writer.Key("hash");
    if (!write_string(writer, value.hash, error, 0U, std::numeric_limits<std::size_t>::max()))
        return false;
    writer.EndObject();
    return true;
}

bool decode_Vector3(const rapidjson::Value& value, Vector3* out, const std::string& path,
                    ContractError* error)
{
    if (!value.IsArray() || value.Size() < 3U || value.Size() > 3U)
        return fail(error, "geometer.contract.array_size", path,
                    "Array length is outside its contract bounds.");
    out->clear();
    out->reserve(value.Size());
    for (rapidjson::SizeType i = 0; i < value.Size(); ++i)
    {
        double item_value{};
        if (!decode_double(value[i], &item_value, path + "/" + std::to_string(i), error,
                           -std::numeric_limits<double>::infinity(),
                           std::numeric_limits<double>::infinity(), false, false))
            return false;
        out->push_back(std::move(item_value));
    }
    return true;
}

bool write_Vector3(rapidjson::Writer<rapidjson::StringBuffer>& writer, const Vector3& value,
                   ContractError* error)
{
    if (value.size() < 3U || value.size() > 3U)
        return fail(error, "geometer.contract.array_size", "",
                    "Array length is outside its contract bounds.");
    writer.StartArray();
    for (const auto& item_value : value)
        if (!write_double(writer, item_value, error, -std::numeric_limits<double>::infinity(),
                          std::numeric_limits<double>::infinity(), false, false))
            return false;
    writer.EndArray();
    return true;
}

bool decode_ModelBoundsValues(const rapidjson::Value& value, ModelBoundsValues* out,
                              const std::string& path, ContractError* error)
{
    static const char* const names[] = {"min", "max", "size", "center"};
    if (!validate_object(value, names, 4U, path, error))
        return false;
    {
        const auto member = value.FindMember("min");
        if (member == value.MemberEnd())
            return fail(error, "geometer.contract.missing_field", child_path(path, "min"),
                        "Required field is missing.");
        if (!decode_Vector3(member->value, &out->min, child_path(path, "min"), error))
            return false;
    }
    {
        const auto member = value.FindMember("max");
        if (member == value.MemberEnd())
            return fail(error, "geometer.contract.missing_field", child_path(path, "max"),
                        "Required field is missing.");
        if (!decode_Vector3(member->value, &out->max, child_path(path, "max"), error))
            return false;
    }
    {
        const auto member = value.FindMember("size");
        if (member == value.MemberEnd())
            return fail(error, "geometer.contract.missing_field", child_path(path, "size"),
                        "Required field is missing.");
        if (!decode_Vector3(member->value, &out->size, child_path(path, "size"), error))
            return false;
    }
    {
        const auto member = value.FindMember("center");
        if (member == value.MemberEnd())
            return fail(error, "geometer.contract.missing_field", child_path(path, "center"),
                        "Required field is missing.");
        if (!decode_Vector3(member->value, &out->center, child_path(path, "center"), error))
            return false;
    }
    return true;
}

bool write_ModelBoundsValues(rapidjson::Writer<rapidjson::StringBuffer>& writer,
                             const ModelBoundsValues& value, ContractError* error)
{
    writer.StartObject();
    writer.Key("min");
    if (!write_Vector3(writer, value.min, error))
        return false;
    writer.Key("max");
    if (!write_Vector3(writer, value.max, error))
        return false;
    writer.Key("size");
    if (!write_Vector3(writer, value.size, error))
        return false;
    writer.Key("center");
    if (!write_Vector3(writer, value.center, error))
        return false;
    writer.EndObject();
    return true;
}

bool decode_ModelBoundsTimings(const rapidjson::Value& value, ModelBoundsTimings* out,
                               const std::string& path, ContractError* error)
{
    static const char* const names[] = {"model_read_ms", "bounds_ms"};
    if (!validate_object(value, names, 2U, path, error))
        return false;
    {
        const auto member = value.FindMember("model_read_ms");
        if (member == value.MemberEnd())
            return fail(error, "geometer.contract.missing_field", child_path(path, "model_read_ms"),
                        "Required field is missing.");
        if (!decode_double(member->value, &out->model_read_ms, child_path(path, "model_read_ms"),
                           error, 0, std::numeric_limits<double>::infinity(), false, false))
            return false;
    }
    {
        const auto member = value.FindMember("bounds_ms");
        if (member == value.MemberEnd())
            return fail(error, "geometer.contract.missing_field", child_path(path, "bounds_ms"),
                        "Required field is missing.");
        if (!decode_double(member->value, &out->bounds_ms, child_path(path, "bounds_ms"), error, 0,
                           std::numeric_limits<double>::infinity(), false, false))
            return false;
    }
    return true;
}

bool write_ModelBoundsTimings(rapidjson::Writer<rapidjson::StringBuffer>& writer,
                              const ModelBoundsTimings& value, ContractError* error)
{
    writer.StartObject();
    writer.Key("model_read_ms");
    if (!write_double(writer, value.model_read_ms, error, 0,
                      std::numeric_limits<double>::infinity(), false, false))
        return false;
    writer.Key("bounds_ms");
    if (!write_double(writer, value.bounds_ms, error, 0, std::numeric_limits<double>::infinity(),
                      false, false))
        return false;
    writer.EndObject();
    return true;
}

bool decode_ModelBoundsResultA0(const rapidjson::Value& value, ModelBoundsResultA0* out,
                                const std::string& path, ContractError* error)
{
    static const char* const names[] = {"schema", "units", "source", "bounds", "timings"};
    if (!validate_object(value, names, 5U, path, error))
        return false;
    {
        const auto member = value.FindMember("schema");
        if (member == value.MemberEnd())
            return fail(error, "geometer.contract.missing_field", child_path(path, "schema"),
                        "Required field is missing.");
        if (!decode_literal_string(member->value, &out->schema, child_path(path, "schema"), error,
                                   "geometry.model_bounds.a0"))
            return false;
    }
    {
        const auto member = value.FindMember("units");
        if (member == value.MemberEnd())
            return fail(error, "geometer.contract.missing_field", child_path(path, "units"),
                        "Required field is missing.");
        if (!decode_literal_string(member->value, &out->units, child_path(path, "units"), error,
                                   "mm"))
            return false;
    }
    {
        const auto member = value.FindMember("source");
        if (member == value.MemberEnd())
            return fail(error, "geometer.contract.missing_field", child_path(path, "source"),
                        "Required field is missing.");
        if (!decode_ModelBoundsSource(member->value, &out->source, child_path(path, "source"),
                                      error))
            return false;
    }
    {
        const auto member = value.FindMember("bounds");
        if (member == value.MemberEnd())
            return fail(error, "geometer.contract.missing_field", child_path(path, "bounds"),
                        "Required field is missing.");
        if (!decode_ModelBoundsValues(member->value, &out->bounds, child_path(path, "bounds"),
                                      error))
            return false;
    }
    {
        const auto member = value.FindMember("timings");
        if (member == value.MemberEnd())
            return fail(error, "geometer.contract.missing_field", child_path(path, "timings"),
                        "Required field is missing.");
        if (!decode_ModelBoundsTimings(member->value, &out->timings, child_path(path, "timings"),
                                       error))
            return false;
    }
    return true;
}

bool write_ModelBoundsResultA0(rapidjson::Writer<rapidjson::StringBuffer>& writer,
                               const ModelBoundsResultA0& value, ContractError* error)
{
    writer.StartObject();
    writer.Key("schema");
    if (!write_literal_string(writer, value.schema, error, "geometry.model_bounds.a0"))
        return false;
    writer.Key("units");
    if (!write_literal_string(writer, value.units, error, "mm"))
        return false;
    writer.Key("source");
    if (!write_ModelBoundsSource(writer, value.source, error))
        return false;
    writer.Key("bounds");
    if (!write_ModelBoundsValues(writer, value.bounds, error))
        return false;
    writer.Key("timings");
    if (!write_ModelBoundsTimings(writer, value.timings, error))
        return false;
    writer.EndObject();
    return true;
}

bool decode_OperationFailureA0(const rapidjson::Value& value, OperationFailureA0* out,
                               const std::string& path, ContractError* error)
{
    static const char* const names[] = {"operation", "ok", "diagnostics"};
    if (!validate_object(value, names, 3U, path, error))
        return false;
    {
        const auto member = value.FindMember("operation");
        if (member == value.MemberEnd())
            return fail(error, "geometer.contract.missing_field", child_path(path, "operation"),
                        "Required field is missing.");
        if (!decode_string(member->value, &out->operation, child_path(path, "operation"), error, 1U,
                           128U))
            return false;
    }
    {
        const auto member = value.FindMember("ok");
        if (member == value.MemberEnd())
            return fail(error, "geometer.contract.missing_field", child_path(path, "ok"),
                        "Required field is missing.");
        if (!decode_literal_boolean(member->value, &out->ok, child_path(path, "ok"), error, false))
            return false;
    }
    {
        const auto member = value.FindMember("diagnostics");
        if (member == value.MemberEnd())
            return fail(error, "geometer.contract.missing_field", child_path(path, "diagnostics"),
                        "Required field is missing.");
        if (!decode_array(member->value, &out->diagnostics, child_path(path, "diagnostics"), error,
                          1U, std::numeric_limits<std::size_t>::max(), decode_DiagnosticA0))
            return false;
    }
    return true;
}

bool write_OperationFailureA0(rapidjson::Writer<rapidjson::StringBuffer>& writer,
                              const OperationFailureA0& value, ContractError* error)
{
    writer.StartObject();
    writer.Key("operation");
    if (!write_string(writer, value.operation, error, 1U, 128U))
        return false;
    writer.Key("ok");
    if (!write_literal_boolean(writer, value.ok, error, false))
        return false;
    writer.Key("diagnostics");
    if (!write_array(writer, value.diagnostics, error, 1U, std::numeric_limits<std::size_t>::max(),
                     write_DiagnosticA0))
        return false;
    writer.EndObject();
    return true;
}

bool decode_ToolDescriptor(const rapidjson::Value& value, ToolDescriptor* out,
                           const std::string& path, ContractError* error)
{
    static const char* const names[] = {"name", "release_version", "occt_version"};
    if (!validate_object(value, names, 3U, path, error))
        return false;
    {
        const auto member = value.FindMember("name");
        if (member == value.MemberEnd())
            return fail(error, "geometer.contract.missing_field", child_path(path, "name"),
                        "Required field is missing.");
        if (!decode_literal_string(member->value, &out->name, child_path(path, "name"), error,
                                   "geometer"))
            return false;
    }
    {
        const auto member = value.FindMember("release_version");
        if (member == value.MemberEnd())
            return fail(error, "geometer.contract.missing_field",
                        child_path(path, "release_version"), "Required field is missing.");
        if (!decode_string(member->value, &out->release_version,
                           child_path(path, "release_version"), error, 1U, 64U))
            return false;
    }
    {
        const auto member = value.FindMember("occt_version");
        if (member == value.MemberEnd())
            return fail(error, "geometer.contract.missing_field", child_path(path, "occt_version"),
                        "Required field is missing.");
        if (!decode_string(member->value, &out->occt_version, child_path(path, "occt_version"),
                           error, 1U, 64U))
            return false;
    }
    return true;
}

bool write_ToolDescriptor(rapidjson::Writer<rapidjson::StringBuffer>& writer,
                          const ToolDescriptor& value, ContractError* error)
{
    writer.StartObject();
    writer.Key("name");
    if (!write_literal_string(writer, value.name, error, "geometer"))
        return false;
    writer.Key("release_version");
    if (!write_string(writer, value.release_version, error, 1U, 64U))
        return false;
    writer.Key("occt_version");
    if (!write_string(writer, value.occt_version, error, 1U, 64U))
        return false;
    writer.EndObject();
    return true;
}

bool decode_StepTopologyOpenResultA0(const rapidjson::Value& value, StepTopologyOpenResultA0* out,
                                     const std::string& path, ContractError* error)
{
    static const char* const names[] = {"schema", "session", "source", "tool",
                                        "evicted_session_handles"};
    if (!validate_object(value, names, 5U, path, error))
        return false;
    {
        const auto member = value.FindMember("schema");
        if (member == value.MemberEnd())
            return fail(error, "geometer.contract.missing_field", child_path(path, "schema"),
                        "Required field is missing.");
        if (!decode_literal_string(member->value, &out->schema, child_path(path, "schema"), error,
                                   "geometry.step_topology.open.result.a0"))
            return false;
    }
    {
        const auto member = value.FindMember("session");
        if (member == value.MemberEnd())
            return fail(error, "geometer.contract.missing_field", child_path(path, "session"),
                        "Required field is missing.");
        if (!decode_SessionReference(member->value, &out->session, child_path(path, "session"),
                                     error))
            return false;
    }
    {
        const auto member = value.FindMember("source");
        if (member == value.MemberEnd())
            return fail(error, "geometer.contract.missing_field", child_path(path, "source"),
                        "Required field is missing.");
        if (!decode_SourceDescriptor(member->value, &out->source, child_path(path, "source"),
                                     error))
            return false;
    }
    {
        const auto member = value.FindMember("tool");
        if (member == value.MemberEnd())
            return fail(error, "geometer.contract.missing_field", child_path(path, "tool"),
                        "Required field is missing.");
        if (!decode_ToolDescriptor(member->value, &out->tool, child_path(path, "tool"), error))
            return false;
    }
    {
        const auto member = value.FindMember("evicted_session_handles");
        if (member == value.MemberEnd())
            return fail(error, "geometer.contract.missing_field",
                        child_path(path, "evicted_session_handles"), "Required field is missing.");
        if (!decode_array(member->value, &out->evicted_session_handles,
                          child_path(path, "evicted_session_handles"), error, 0U, 8U,
                          decode_string_item))
            return false;
    }
    return true;
}

bool write_StepTopologyOpenResultA0(rapidjson::Writer<rapidjson::StringBuffer>& writer,
                                    const StepTopologyOpenResultA0& value, ContractError* error)
{
    writer.StartObject();
    writer.Key("schema");
    if (!write_literal_string(writer, value.schema, error, "geometry.step_topology.open.result.a0"))
        return false;
    writer.Key("session");
    if (!write_SessionReference(writer, value.session, error))
        return false;
    writer.Key("source");
    if (!write_SourceDescriptor(writer, value.source, error))
        return false;
    writer.Key("tool");
    if (!write_ToolDescriptor(writer, value.tool, error))
        return false;
    writer.Key("evicted_session_handles");
    if (!write_array(writer, value.evicted_session_handles, error, 0U, 8U, write_string_item))
        return false;
    writer.EndObject();
    return true;
}

bool decode_StepTopologyCloseResultA0(const rapidjson::Value& value, StepTopologyCloseResultA0* out,
                                      const std::string& path, ContractError* error)
{
    static const char* const names[] = {"schema", "session_handle", "closed"};
    if (!validate_object(value, names, 3U, path, error))
        return false;
    {
        const auto member = value.FindMember("schema");
        if (member == value.MemberEnd())
            return fail(error, "geometer.contract.missing_field", child_path(path, "schema"),
                        "Required field is missing.");
        if (!decode_literal_string(member->value, &out->schema, child_path(path, "schema"), error,
                                   "geometry.step_topology.close.result.a0"))
            return false;
    }
    {
        const auto member = value.FindMember("session_handle");
        if (member == value.MemberEnd())
            return fail(error, "geometer.contract.missing_field",
                        child_path(path, "session_handle"), "Required field is missing.");
        if (!decode_string(member->value, &out->session_handle, child_path(path, "session_handle"),
                           error, 68U, 68U))
            return false;
    }
    {
        const auto member = value.FindMember("closed");
        if (member == value.MemberEnd())
            return fail(error, "geometer.contract.missing_field", child_path(path, "closed"),
                        "Required field is missing.");
        if (!decode_literal_boolean(member->value, &out->closed, child_path(path, "closed"), error,
                                    true))
            return false;
    }
    return true;
}

bool write_StepTopologyCloseResultA0(rapidjson::Writer<rapidjson::StringBuffer>& writer,
                                     const StepTopologyCloseResultA0& value, ContractError* error)
{
    writer.StartObject();
    writer.Key("schema");
    if (!write_literal_string(writer, value.schema, error,
                              "geometry.step_topology.close.result.a0"))
        return false;
    writer.Key("session_handle");
    if (!write_string(writer, value.session_handle, error, 68U, 68U))
        return false;
    writer.Key("closed");
    if (!write_literal_boolean(writer, value.closed, error, true))
        return false;
    writer.EndObject();
    return true;
}

bool decode_InspectionCounts(const rapidjson::Value& value, InspectionCounts* out,
                             const std::string& path, ContractError* error)
{
    static const char* const names[] = {"definitions", "root_occurrences", "component_occurrences",
                                        "bodies",      "shells",           "faces",
                                        "memberships"};
    if (!validate_object(value, names, 7U, path, error))
        return false;
    {
        const auto member = value.FindMember("definitions");
        if (member == value.MemberEnd())
            return fail(error, "geometer.contract.missing_field", child_path(path, "definitions"),
                        "Required field is missing.");
        if (!decode_uint32(member->value, &out->definitions, child_path(path, "definitions"), error,
                           0ULL, 10000ULL))
            return false;
    }
    {
        const auto member = value.FindMember("root_occurrences");
        if (member == value.MemberEnd())
            return fail(error, "geometer.contract.missing_field",
                        child_path(path, "root_occurrences"), "Required field is missing.");
        if (!decode_uint32(member->value, &out->root_occurrences,
                           child_path(path, "root_occurrences"), error, 0ULL, 100000ULL))
            return false;
    }
    {
        const auto member = value.FindMember("component_occurrences");
        if (member == value.MemberEnd())
            return fail(error, "geometer.contract.missing_field",
                        child_path(path, "component_occurrences"), "Required field is missing.");
        if (!decode_uint32(member->value, &out->component_occurrences,
                           child_path(path, "component_occurrences"), error, 0ULL, 100000ULL))
            return false;
    }
    {
        const auto member = value.FindMember("bodies");
        if (member == value.MemberEnd())
            return fail(error, "geometer.contract.missing_field", child_path(path, "bodies"),
                        "Required field is missing.");
        if (!decode_uint32(member->value, &out->bodies, child_path(path, "bodies"), error, 0ULL,
                           100000ULL))
            return false;
    }
    {
        const auto member = value.FindMember("shells");
        if (member == value.MemberEnd())
            return fail(error, "geometer.contract.missing_field", child_path(path, "shells"),
                        "Required field is missing.");
        if (!decode_uint32(member->value, &out->shells, child_path(path, "shells"), error, 0ULL,
                           250000ULL))
            return false;
    }
    {
        const auto member = value.FindMember("faces");
        if (member == value.MemberEnd())
            return fail(error, "geometer.contract.missing_field", child_path(path, "faces"),
                        "Required field is missing.");
        if (!decode_uint32(member->value, &out->faces, child_path(path, "faces"), error, 0ULL,
                           1000000ULL))
            return false;
    }
    {
        const auto member = value.FindMember("memberships");
        if (member == value.MemberEnd())
            return fail(error, "geometer.contract.missing_field", child_path(path, "memberships"),
                        "Required field is missing.");
        if (!decode_uint32(member->value, &out->memberships, child_path(path, "memberships"), error,
                           0ULL, 5000000ULL))
            return false;
    }
    return true;
}

bool write_InspectionCounts(rapidjson::Writer<rapidjson::StringBuffer>& writer,
                            const InspectionCounts& value, ContractError* error)
{
    writer.StartObject();
    writer.Key("definitions");
    if (!write_uint32(writer, value.definitions, error, 0ULL, 10000ULL))
        return false;
    writer.Key("root_occurrences");
    if (!write_uint32(writer, value.root_occurrences, error, 0ULL, 100000ULL))
        return false;
    writer.Key("component_occurrences");
    if (!write_uint32(writer, value.component_occurrences, error, 0ULL, 100000ULL))
        return false;
    writer.Key("bodies");
    if (!write_uint32(writer, value.bodies, error, 0ULL, 100000ULL))
        return false;
    writer.Key("shells");
    if (!write_uint32(writer, value.shells, error, 0ULL, 250000ULL))
        return false;
    writer.Key("faces");
    if (!write_uint32(writer, value.faces, error, 0ULL, 1000000ULL))
        return false;
    writer.Key("memberships");
    if (!write_uint32(writer, value.memberships, error, 0ULL, 5000000ULL))
        return false;
    writer.EndObject();
    return true;
}

bool decode_SourceEntityEvidence(const rapidjson::Value& value, SourceEntityEvidence* out,
                                 const std::string& path, ContractError* error)
{
    static const char* const names[] = {"mapped", "shape_result_round_trip", "model_number",
                                        "entity_type", "mapping_method"};
    if (!validate_object(value, names, 5U, path, error))
        return false;
    {
        const auto member = value.FindMember("mapped");
        if (member == value.MemberEnd())
            return fail(error, "geometer.contract.missing_field", child_path(path, "mapped"),
                        "Required field is missing.");
        if (!decode_boolean(member->value, &out->mapped, child_path(path, "mapped"), error))
            return false;
    }
    {
        const auto member = value.FindMember("shape_result_round_trip");
        if (member == value.MemberEnd())
            return fail(error, "geometer.contract.missing_field",
                        child_path(path, "shape_result_round_trip"), "Required field is missing.");
        if (!decode_boolean(member->value, &out->shape_result_round_trip,
                            child_path(path, "shape_result_round_trip"), error))
            return false;
    }
    {
        const auto member = value.FindMember("model_number");
        if (member != value.MemberEnd())
        {
            std::uint32_t decoded{};
            if (!decode_uint32(member->value, &decoded, child_path(path, "model_number"), error,
                               1ULL, 5000000ULL))
                return false;
            out->model_number = std::move(decoded);
        }
        else
            out->model_number.reset();
    }
    {
        const auto member = value.FindMember("entity_type");
        if (member != value.MemberEnd())
        {
            std::string decoded{};
            if (!decode_string(member->value, &decoded, child_path(path, "entity_type"), error, 1U,
                               128U))
                return false;
            out->entity_type = std::move(decoded);
        }
        else
            out->entity_type.reset();
    }
    {
        const auto member = value.FindMember("mapping_method");
        if (member != value.MemberEnd())
        {
            std::string decoded{};
            if (!decode_string(member->value, &decoded, child_path(path, "mapping_method"), error,
                               1U, 128U))
                return false;
            out->mapping_method = std::move(decoded);
        }
        else
            out->mapping_method.reset();
    }
    return true;
}

bool write_SourceEntityEvidence(rapidjson::Writer<rapidjson::StringBuffer>& writer,
                                const SourceEntityEvidence& value, ContractError* error)
{
    writer.StartObject();
    writer.Key("mapped");
    if (!(writer.Bool(value.mapped), true))
        return false;
    writer.Key("shape_result_round_trip");
    if (!(writer.Bool(value.shape_result_round_trip), true))
        return false;
    if (value.model_number.has_value())
    {
        writer.Key("model_number");
        if (!write_uint32(writer, *value.model_number, error, 1ULL, 5000000ULL))
            return false;
    }
    if (value.entity_type.has_value())
    {
        writer.Key("entity_type");
        if (!write_string(writer, *value.entity_type, error, 1U, 128U))
            return false;
    }
    if (value.mapping_method.has_value())
    {
        writer.Key("mapping_method");
        if (!write_string(writer, *value.mapping_method, error, 1U, 128U))
            return false;
    }
    writer.EndObject();
    return true;
}

bool decode_DefinitionSummary(const rapidjson::Value& value, DefinitionSummary* out,
                              const std::string& path, ContractError* error)
{
    static const char* const names[] = {"handle",     "name",       "assembly",
                                        "body_count", "face_count", "source_entity"};
    if (!validate_object(value, names, 6U, path, error))
        return false;
    {
        const auto member = value.FindMember("handle");
        if (member == value.MemberEnd())
            return fail(error, "geometer.contract.missing_field", child_path(path, "handle"),
                        "Required field is missing.");
        if (!decode_string(member->value, &out->handle, child_path(path, "handle"), error, 68U,
                           68U))
            return false;
    }
    {
        const auto member = value.FindMember("name");
        if (member == value.MemberEnd())
            return fail(error, "geometer.contract.missing_field", child_path(path, "name"),
                        "Required field is missing.");
        if (!decode_string(member->value, &out->name, child_path(path, "name"), error, 0U, 4096U))
            return false;
    }
    {
        const auto member = value.FindMember("assembly");
        if (member == value.MemberEnd())
            return fail(error, "geometer.contract.missing_field", child_path(path, "assembly"),
                        "Required field is missing.");
        if (!decode_boolean(member->value, &out->assembly, child_path(path, "assembly"), error))
            return false;
    }
    {
        const auto member = value.FindMember("body_count");
        if (member == value.MemberEnd())
            return fail(error, "geometer.contract.missing_field", child_path(path, "body_count"),
                        "Required field is missing.");
        if (!decode_uint32(member->value, &out->body_count, child_path(path, "body_count"), error,
                           0ULL, 100000ULL))
            return false;
    }
    {
        const auto member = value.FindMember("face_count");
        if (member == value.MemberEnd())
            return fail(error, "geometer.contract.missing_field", child_path(path, "face_count"),
                        "Required field is missing.");
        if (!decode_uint32(member->value, &out->face_count, child_path(path, "face_count"), error,
                           0ULL, 1000000ULL))
            return false;
    }
    {
        const auto member = value.FindMember("source_entity");
        if (member != value.MemberEnd())
        {
            SourceEntityEvidence decoded{};
            if (!decode_SourceEntityEvidence(member->value, &decoded,
                                             child_path(path, "source_entity"), error))
                return false;
            out->source_entity = std::move(decoded);
        }
        else
            out->source_entity.reset();
    }
    return true;
}

bool write_DefinitionSummary(rapidjson::Writer<rapidjson::StringBuffer>& writer,
                             const DefinitionSummary& value, ContractError* error)
{
    writer.StartObject();
    writer.Key("handle");
    if (!write_string(writer, value.handle, error, 68U, 68U))
        return false;
    writer.Key("name");
    if (!write_string(writer, value.name, error, 0U, 4096U))
        return false;
    writer.Key("assembly");
    if (!(writer.Bool(value.assembly), true))
        return false;
    writer.Key("body_count");
    if (!write_uint32(writer, value.body_count, error, 0ULL, 100000ULL))
        return false;
    writer.Key("face_count");
    if (!write_uint32(writer, value.face_count, error, 0ULL, 1000000ULL))
        return false;
    if (value.source_entity.has_value())
    {
        writer.Key("source_entity");
        if (!write_SourceEntityEvidence(writer, *value.source_entity, error))
            return false;
    }
    writer.EndObject();
    return true;
}

bool decode_RootOccurrenceSummary(const rapidjson::Value& value, RootOccurrenceSummary* out,
                                  const std::string& path, ContractError* error)
{
    static const char* const names[] = {"kind", "handle", "definition_handle", "name", "transform"};
    if (!validate_object(value, names, 5U, path, error))
        return false;
    {
        const auto member = value.FindMember("kind");
        if (member == value.MemberEnd())
            return fail(error, "geometer.contract.missing_field", child_path(path, "kind"),
                        "Required field is missing.");
        if (!decode_literal_string(member->value, &out->kind, child_path(path, "kind"), error,
                                   "root"))
            return false;
    }
    {
        const auto member = value.FindMember("handle");
        if (member == value.MemberEnd())
            return fail(error, "geometer.contract.missing_field", child_path(path, "handle"),
                        "Required field is missing.");
        if (!decode_string(member->value, &out->handle, child_path(path, "handle"), error, 68U,
                           68U))
            return false;
    }
    {
        const auto member = value.FindMember("definition_handle");
        if (member == value.MemberEnd())
            return fail(error, "geometer.contract.missing_field",
                        child_path(path, "definition_handle"), "Required field is missing.");
        if (!decode_string(member->value, &out->definition_handle,
                           child_path(path, "definition_handle"), error, 68U, 68U))
            return false;
    }
    {
        const auto member = value.FindMember("name");
        if (member == value.MemberEnd())
            return fail(error, "geometer.contract.missing_field", child_path(path, "name"),
                        "Required field is missing.");
        if (!decode_string(member->value, &out->name, child_path(path, "name"), error, 0U, 4096U))
            return false;
    }
    {
        const auto member = value.FindMember("transform");
        if (member == value.MemberEnd())
            return fail(error, "geometer.contract.missing_field", child_path(path, "transform"),
                        "Required field is missing.");
        if (!decode_array(member->value, &out->transform, child_path(path, "transform"), error, 12U,
                          12U, decode_double_item))
            return false;
    }
    return true;
}

bool write_RootOccurrenceSummary(rapidjson::Writer<rapidjson::StringBuffer>& writer,
                                 const RootOccurrenceSummary& value, ContractError* error)
{
    writer.StartObject();
    writer.Key("kind");
    if (!write_literal_string(writer, value.kind, error, "root"))
        return false;
    writer.Key("handle");
    if (!write_string(writer, value.handle, error, 68U, 68U))
        return false;
    writer.Key("definition_handle");
    if (!write_string(writer, value.definition_handle, error, 68U, 68U))
        return false;
    writer.Key("name");
    if (!write_string(writer, value.name, error, 0U, 4096U))
        return false;
    writer.Key("transform");
    if (!write_array(writer, value.transform, error, 12U, 12U, write_double_item))
        return false;
    writer.EndObject();
    return true;
}

bool decode_ComponentOccurrenceSummary(const rapidjson::Value& value,
                                       ComponentOccurrenceSummary* out, const std::string& path,
                                       ContractError* error)
{
    static const char* const names[] = {
        "kind",  "handle", "definition_handle", "parent_occurrence_handle",
        "depth", "name",   "transform"};
    if (!validate_object(value, names, 7U, path, error))
        return false;
    {
        const auto member = value.FindMember("kind");
        if (member == value.MemberEnd())
            return fail(error, "geometer.contract.missing_field", child_path(path, "kind"),
                        "Required field is missing.");
        if (!decode_literal_string(member->value, &out->kind, child_path(path, "kind"), error,
                                   "component"))
            return false;
    }
    {
        const auto member = value.FindMember("handle");
        if (member == value.MemberEnd())
            return fail(error, "geometer.contract.missing_field", child_path(path, "handle"),
                        "Required field is missing.");
        if (!decode_string(member->value, &out->handle, child_path(path, "handle"), error, 68U,
                           68U))
            return false;
    }
    {
        const auto member = value.FindMember("definition_handle");
        if (member == value.MemberEnd())
            return fail(error, "geometer.contract.missing_field",
                        child_path(path, "definition_handle"), "Required field is missing.");
        if (!decode_string(member->value, &out->definition_handle,
                           child_path(path, "definition_handle"), error, 68U, 68U))
            return false;
    }
    {
        const auto member = value.FindMember("parent_occurrence_handle");
        if (member == value.MemberEnd())
            return fail(error, "geometer.contract.missing_field",
                        child_path(path, "parent_occurrence_handle"), "Required field is missing.");
        if (!decode_string(member->value, &out->parent_occurrence_handle,
                           child_path(path, "parent_occurrence_handle"), error, 68U, 68U))
            return false;
    }
    {
        const auto member = value.FindMember("depth");
        if (member == value.MemberEnd())
            return fail(error, "geometer.contract.missing_field", child_path(path, "depth"),
                        "Required field is missing.");
        if (!decode_uint32(member->value, &out->depth, child_path(path, "depth"), error, 1ULL,
                           64ULL))
            return false;
    }
    {
        const auto member = value.FindMember("name");
        if (member == value.MemberEnd())
            return fail(error, "geometer.contract.missing_field", child_path(path, "name"),
                        "Required field is missing.");
        if (!decode_string(member->value, &out->name, child_path(path, "name"), error, 0U, 4096U))
            return false;
    }
    {
        const auto member = value.FindMember("transform");
        if (member == value.MemberEnd())
            return fail(error, "geometer.contract.missing_field", child_path(path, "transform"),
                        "Required field is missing.");
        if (!decode_array(member->value, &out->transform, child_path(path, "transform"), error, 12U,
                          12U, decode_double_item))
            return false;
    }
    return true;
}

bool write_ComponentOccurrenceSummary(rapidjson::Writer<rapidjson::StringBuffer>& writer,
                                      const ComponentOccurrenceSummary& value, ContractError* error)
{
    writer.StartObject();
    writer.Key("kind");
    if (!write_literal_string(writer, value.kind, error, "component"))
        return false;
    writer.Key("handle");
    if (!write_string(writer, value.handle, error, 68U, 68U))
        return false;
    writer.Key("definition_handle");
    if (!write_string(writer, value.definition_handle, error, 68U, 68U))
        return false;
    writer.Key("parent_occurrence_handle");
    if (!write_string(writer, value.parent_occurrence_handle, error, 68U, 68U))
        return false;
    writer.Key("depth");
    if (!write_uint32(writer, value.depth, error, 1ULL, 64ULL))
        return false;
    writer.Key("name");
    if (!write_string(writer, value.name, error, 0U, 4096U))
        return false;
    writer.Key("transform");
    if (!write_array(writer, value.transform, error, 12U, 12U, write_double_item))
        return false;
    writer.EndObject();
    return true;
}

bool decode_OccurrenceSummary(const rapidjson::Value& value, OccurrenceSummary* out,
                              const std::string& path, ContractError* error)
{
    int matches = 0;
    OccurrenceSummary selected{};
    {
        RootOccurrenceSummary candidate{};
        ContractError ignored;
        if (decode_RootOccurrenceSummary(value, &candidate, path, &ignored))
        {
            ++matches;
            selected = OccurrenceSummary(std::in_place_index<0>, std::move(candidate));
        }
    }
    {
        ComponentOccurrenceSummary candidate{};
        ContractError ignored;
        if (decode_ComponentOccurrenceSummary(value, &candidate, path, &ignored))
        {
            ++matches;
            selected = OccurrenceSummary(std::in_place_index<1>, std::move(candidate));
        }
    }
    if (matches != 1)
        return fail(error, "geometer.contract.union_mismatch", path,
                    "Expected exactly one union variant.");
    *out = std::move(selected);
    return true;
}

bool write_OccurrenceSummary(rapidjson::Writer<rapidjson::StringBuffer>& writer,
                             const OccurrenceSummary& value, ContractError* error)
{
    switch (value.index())
    {
    case 0:
        return write_RootOccurrenceSummary(writer, std::get<0>(value), error);
    case 1:
        return write_ComponentOccurrenceSummary(writer, std::get<1>(value), error);
    default:
        return fail(error, "geometer.contract.union_mismatch", "", "Unknown union variant.");
    }
}

bool decode_BodySummary(const rapidjson::Value& value, BodySummary* out, const std::string& path,
                        ContractError* error)
{
    static const char* const names[] = {"handle",      "definition_handle", "topology_kind",
                                        "shell_count", "face_count",        "bounds_mm",
                                        "volume_mm3",  "source_entity"};
    if (!validate_object(value, names, 8U, path, error))
        return false;
    {
        const auto member = value.FindMember("handle");
        if (member == value.MemberEnd())
            return fail(error, "geometer.contract.missing_field", child_path(path, "handle"),
                        "Required field is missing.");
        if (!decode_string(member->value, &out->handle, child_path(path, "handle"), error, 68U,
                           68U))
            return false;
    }
    {
        const auto member = value.FindMember("definition_handle");
        if (member == value.MemberEnd())
            return fail(error, "geometer.contract.missing_field",
                        child_path(path, "definition_handle"), "Required field is missing.");
        if (!decode_string(member->value, &out->definition_handle,
                           child_path(path, "definition_handle"), error, 68U, 68U))
            return false;
    }
    {
        const auto member = value.FindMember("topology_kind");
        if (member == value.MemberEnd())
            return fail(error, "geometer.contract.missing_field", child_path(path, "topology_kind"),
                        "Required field is missing.");
        if (!decode_string(member->value, &out->topology_kind, child_path(path, "topology_kind"),
                           error, 1U, 64U))
            return false;
    }
    {
        const auto member = value.FindMember("shell_count");
        if (member == value.MemberEnd())
            return fail(error, "geometer.contract.missing_field", child_path(path, "shell_count"),
                        "Required field is missing.");
        if (!decode_uint32(member->value, &out->shell_count, child_path(path, "shell_count"), error,
                           0ULL, 250000ULL))
            return false;
    }
    {
        const auto member = value.FindMember("face_count");
        if (member == value.MemberEnd())
            return fail(error, "geometer.contract.missing_field", child_path(path, "face_count"),
                        "Required field is missing.");
        if (!decode_uint32(member->value, &out->face_count, child_path(path, "face_count"), error,
                           0ULL, 1000000ULL))
            return false;
    }
    {
        const auto member = value.FindMember("bounds_mm");
        if (member == value.MemberEnd())
            return fail(error, "geometer.contract.missing_field", child_path(path, "bounds_mm"),
                        "Required field is missing.");
        if (!decode_array(member->value, &out->bounds_mm, child_path(path, "bounds_mm"), error, 6U,
                          6U, decode_double_item))
            return false;
    }
    {
        const auto member = value.FindMember("volume_mm3");
        if (member == value.MemberEnd())
            return fail(error, "geometer.contract.missing_field", child_path(path, "volume_mm3"),
                        "Required field is missing.");
        if (!decode_double(member->value, &out->volume_mm3, child_path(path, "volume_mm3"), error,
                           0, std::numeric_limits<double>::infinity(), false, false))
            return false;
    }
    {
        const auto member = value.FindMember("source_entity");
        if (member != value.MemberEnd())
        {
            SourceEntityEvidence decoded{};
            if (!decode_SourceEntityEvidence(member->value, &decoded,
                                             child_path(path, "source_entity"), error))
                return false;
            out->source_entity = std::move(decoded);
        }
        else
            out->source_entity.reset();
    }
    return true;
}

bool write_BodySummary(rapidjson::Writer<rapidjson::StringBuffer>& writer, const BodySummary& value,
                       ContractError* error)
{
    writer.StartObject();
    writer.Key("handle");
    if (!write_string(writer, value.handle, error, 68U, 68U))
        return false;
    writer.Key("definition_handle");
    if (!write_string(writer, value.definition_handle, error, 68U, 68U))
        return false;
    writer.Key("topology_kind");
    if (!write_string(writer, value.topology_kind, error, 1U, 64U))
        return false;
    writer.Key("shell_count");
    if (!write_uint32(writer, value.shell_count, error, 0ULL, 250000ULL))
        return false;
    writer.Key("face_count");
    if (!write_uint32(writer, value.face_count, error, 0ULL, 1000000ULL))
        return false;
    writer.Key("bounds_mm");
    if (!write_array(writer, value.bounds_mm, error, 6U, 6U, write_double_item))
        return false;
    writer.Key("volume_mm3");
    if (!write_double(writer, value.volume_mm3, error, 0, std::numeric_limits<double>::infinity(),
                      false, false))
        return false;
    if (value.source_entity.has_value())
    {
        writer.Key("source_entity");
        if (!write_SourceEntityEvidence(writer, *value.source_entity, error))
            return false;
    }
    writer.EndObject();
    return true;
}

bool decode_ShellSummary(const rapidjson::Value& value, ShellSummary* out, const std::string& path,
                         ContractError* error)
{
    static const char* const names[] = {"handle", "definition_handle", "body_count", "face_count",
                                        "source_entity"};
    if (!validate_object(value, names, 5U, path, error))
        return false;
    {
        const auto member = value.FindMember("handle");
        if (member == value.MemberEnd())
            return fail(error, "geometer.contract.missing_field", child_path(path, "handle"),
                        "Required field is missing.");
        if (!decode_string(member->value, &out->handle, child_path(path, "handle"), error, 68U,
                           68U))
            return false;
    }
    {
        const auto member = value.FindMember("definition_handle");
        if (member == value.MemberEnd())
            return fail(error, "geometer.contract.missing_field",
                        child_path(path, "definition_handle"), "Required field is missing.");
        if (!decode_string(member->value, &out->definition_handle,
                           child_path(path, "definition_handle"), error, 68U, 68U))
            return false;
    }
    {
        const auto member = value.FindMember("body_count");
        if (member == value.MemberEnd())
            return fail(error, "geometer.contract.missing_field", child_path(path, "body_count"),
                        "Required field is missing.");
        if (!decode_uint32(member->value, &out->body_count, child_path(path, "body_count"), error,
                           0ULL, 100000ULL))
            return false;
    }
    {
        const auto member = value.FindMember("face_count");
        if (member == value.MemberEnd())
            return fail(error, "geometer.contract.missing_field", child_path(path, "face_count"),
                        "Required field is missing.");
        if (!decode_uint32(member->value, &out->face_count, child_path(path, "face_count"), error,
                           0ULL, 1000000ULL))
            return false;
    }
    {
        const auto member = value.FindMember("source_entity");
        if (member != value.MemberEnd())
        {
            SourceEntityEvidence decoded{};
            if (!decode_SourceEntityEvidence(member->value, &decoded,
                                             child_path(path, "source_entity"), error))
                return false;
            out->source_entity = std::move(decoded);
        }
        else
            out->source_entity.reset();
    }
    return true;
}

bool write_ShellSummary(rapidjson::Writer<rapidjson::StringBuffer>& writer,
                        const ShellSummary& value, ContractError* error)
{
    writer.StartObject();
    writer.Key("handle");
    if (!write_string(writer, value.handle, error, 68U, 68U))
        return false;
    writer.Key("definition_handle");
    if (!write_string(writer, value.definition_handle, error, 68U, 68U))
        return false;
    writer.Key("body_count");
    if (!write_uint32(writer, value.body_count, error, 0ULL, 100000ULL))
        return false;
    writer.Key("face_count");
    if (!write_uint32(writer, value.face_count, error, 0ULL, 1000000ULL))
        return false;
    if (value.source_entity.has_value())
    {
        writer.Key("source_entity");
        if (!write_SourceEntityEvidence(writer, *value.source_entity, error))
            return false;
    }
    writer.EndObject();
    return true;
}

bool decode_FaceSummary(const rapidjson::Value& value, FaceSummary* out, const std::string& path,
                        ContractError* error)
{
    static const char* const names[] = {"handle",      "definition_handle", "body_count",
                                        "shell_count", "bounds_mm",         "area_mm2",
                                        "centroid_mm", "source_entity"};
    if (!validate_object(value, names, 8U, path, error))
        return false;
    {
        const auto member = value.FindMember("handle");
        if (member == value.MemberEnd())
            return fail(error, "geometer.contract.missing_field", child_path(path, "handle"),
                        "Required field is missing.");
        if (!decode_string(member->value, &out->handle, child_path(path, "handle"), error, 68U,
                           68U))
            return false;
    }
    {
        const auto member = value.FindMember("definition_handle");
        if (member == value.MemberEnd())
            return fail(error, "geometer.contract.missing_field",
                        child_path(path, "definition_handle"), "Required field is missing.");
        if (!decode_string(member->value, &out->definition_handle,
                           child_path(path, "definition_handle"), error, 68U, 68U))
            return false;
    }
    {
        const auto member = value.FindMember("body_count");
        if (member == value.MemberEnd())
            return fail(error, "geometer.contract.missing_field", child_path(path, "body_count"),
                        "Required field is missing.");
        if (!decode_uint32(member->value, &out->body_count, child_path(path, "body_count"), error,
                           0ULL, 100000ULL))
            return false;
    }
    {
        const auto member = value.FindMember("shell_count");
        if (member == value.MemberEnd())
            return fail(error, "geometer.contract.missing_field", child_path(path, "shell_count"),
                        "Required field is missing.");
        if (!decode_uint32(member->value, &out->shell_count, child_path(path, "shell_count"), error,
                           0ULL, 250000ULL))
            return false;
    }
    {
        const auto member = value.FindMember("bounds_mm");
        if (member == value.MemberEnd())
            return fail(error, "geometer.contract.missing_field", child_path(path, "bounds_mm"),
                        "Required field is missing.");
        if (!decode_array(member->value, &out->bounds_mm, child_path(path, "bounds_mm"), error, 6U,
                          6U, decode_double_item))
            return false;
    }
    {
        const auto member = value.FindMember("area_mm2");
        if (member == value.MemberEnd())
            return fail(error, "geometer.contract.missing_field", child_path(path, "area_mm2"),
                        "Required field is missing.");
        if (!decode_double(member->value, &out->area_mm2, child_path(path, "area_mm2"), error, 0,
                           std::numeric_limits<double>::infinity(), false, false))
            return false;
    }
    {
        const auto member = value.FindMember("centroid_mm");
        if (member == value.MemberEnd())
            return fail(error, "geometer.contract.missing_field", child_path(path, "centroid_mm"),
                        "Required field is missing.");
        if (!decode_array(member->value, &out->centroid_mm, child_path(path, "centroid_mm"), error,
                          3U, 3U, decode_double_item))
            return false;
    }
    {
        const auto member = value.FindMember("source_entity");
        if (member != value.MemberEnd())
        {
            SourceEntityEvidence decoded{};
            if (!decode_SourceEntityEvidence(member->value, &decoded,
                                             child_path(path, "source_entity"), error))
                return false;
            out->source_entity = std::move(decoded);
        }
        else
            out->source_entity.reset();
    }
    return true;
}

bool write_FaceSummary(rapidjson::Writer<rapidjson::StringBuffer>& writer, const FaceSummary& value,
                       ContractError* error)
{
    writer.StartObject();
    writer.Key("handle");
    if (!write_string(writer, value.handle, error, 68U, 68U))
        return false;
    writer.Key("definition_handle");
    if (!write_string(writer, value.definition_handle, error, 68U, 68U))
        return false;
    writer.Key("body_count");
    if (!write_uint32(writer, value.body_count, error, 0ULL, 100000ULL))
        return false;
    writer.Key("shell_count");
    if (!write_uint32(writer, value.shell_count, error, 0ULL, 250000ULL))
        return false;
    writer.Key("bounds_mm");
    if (!write_array(writer, value.bounds_mm, error, 6U, 6U, write_double_item))
        return false;
    writer.Key("area_mm2");
    if (!write_double(writer, value.area_mm2, error, 0, std::numeric_limits<double>::infinity(),
                      false, false))
        return false;
    writer.Key("centroid_mm");
    if (!write_array(writer, value.centroid_mm, error, 3U, 3U, write_double_item))
        return false;
    if (value.source_entity.has_value())
    {
        writer.Key("source_entity");
        if (!write_SourceEntityEvidence(writer, *value.source_entity, error))
            return false;
    }
    writer.EndObject();
    return true;
}

bool decode_TopologyMembershipKind(const rapidjson::Value& value, TopologyMembershipKind* out,
                                   const std::string& path, ContractError* error)
{
    if (!value.IsString())
        return fail(error, "geometer.contract.type_mismatch", path, "Expected a string enum.");
    const std::string text(value.GetString(), value.GetStringLength());
    if (text == "body_shell")
    {
        *out = TopologyMembershipKind::body_shell;
        return true;
    }
    if (text == "body_face")
    {
        *out = TopologyMembershipKind::body_face;
        return true;
    }
    if (text == "shell_face")
    {
        *out = TopologyMembershipKind::shell_face;
        return true;
    }
    return fail(error, "geometer.contract.enum_mismatch", path, "Unknown enum value.");
}

bool write_TopologyMembershipKind(rapidjson::Writer<rapidjson::StringBuffer>& writer,
                                  const TopologyMembershipKind& value, ContractError* error)
{
    switch (value)
    {
    case TopologyMembershipKind::body_shell:
        writer.String("body_shell");
        return true;
    case TopologyMembershipKind::body_face:
        writer.String("body_face");
        return true;
    case TopologyMembershipKind::shell_face:
        writer.String("shell_face");
        return true;
    }
    return fail(error, "geometer.contract.enum_mismatch", "", "Unknown enum value.");
}

bool decode_TopologyMembership(const rapidjson::Value& value, TopologyMembership* out,
                               const std::string& path, ContractError* error)
{
    static const char* const names[] = {"kind", "owner_handle", "member_handle"};
    if (!validate_object(value, names, 3U, path, error))
        return false;
    {
        const auto member = value.FindMember("kind");
        if (member == value.MemberEnd())
            return fail(error, "geometer.contract.missing_field", child_path(path, "kind"),
                        "Required field is missing.");
        if (!decode_TopologyMembershipKind(member->value, &out->kind, child_path(path, "kind"),
                                           error))
            return false;
    }
    {
        const auto member = value.FindMember("owner_handle");
        if (member == value.MemberEnd())
            return fail(error, "geometer.contract.missing_field", child_path(path, "owner_handle"),
                        "Required field is missing.");
        if (!decode_string(member->value, &out->owner_handle, child_path(path, "owner_handle"),
                           error, 68U, 68U))
            return false;
    }
    {
        const auto member = value.FindMember("member_handle");
        if (member == value.MemberEnd())
            return fail(error, "geometer.contract.missing_field", child_path(path, "member_handle"),
                        "Required field is missing.");
        if (!decode_string(member->value, &out->member_handle, child_path(path, "member_handle"),
                           error, 68U, 68U))
            return false;
    }
    return true;
}

bool write_TopologyMembership(rapidjson::Writer<rapidjson::StringBuffer>& writer,
                              const TopologyMembership& value, ContractError* error)
{
    writer.StartObject();
    writer.Key("kind");
    if (!write_TopologyMembershipKind(writer, value.kind, error))
        return false;
    writer.Key("owner_handle");
    if (!write_string(writer, value.owner_handle, error, 68U, 68U))
        return false;
    writer.Key("member_handle");
    if (!write_string(writer, value.member_handle, error, 68U, 68U))
        return false;
    writer.EndObject();
    return true;
}

bool decode_TopologyPage(const rapidjson::Value& value, TopologyPage* out, const std::string& path,
                         ContractError* error)
{
    static const char* const names[] = {"definitions", "occurrences", "bodies",     "shells",
                                        "faces",       "memberships", "next_cursor"};
    if (!validate_object(value, names, 7U, path, error))
        return false;
    {
        const auto member = value.FindMember("definitions");
        if (member == value.MemberEnd())
            return fail(error, "geometer.contract.missing_field", child_path(path, "definitions"),
                        "Required field is missing.");
        if (!decode_array(member->value, &out->definitions, child_path(path, "definitions"), error,
                          0U, 1024U, decode_DefinitionSummary))
            return false;
    }
    {
        const auto member = value.FindMember("occurrences");
        if (member == value.MemberEnd())
            return fail(error, "geometer.contract.missing_field", child_path(path, "occurrences"),
                        "Required field is missing.");
        if (!decode_array(member->value, &out->occurrences, child_path(path, "occurrences"), error,
                          0U, 1024U, decode_OccurrenceSummary))
            return false;
    }
    {
        const auto member = value.FindMember("bodies");
        if (member == value.MemberEnd())
            return fail(error, "geometer.contract.missing_field", child_path(path, "bodies"),
                        "Required field is missing.");
        if (!decode_array(member->value, &out->bodies, child_path(path, "bodies"), error, 0U, 1024U,
                          decode_BodySummary))
            return false;
    }
    {
        const auto member = value.FindMember("shells");
        if (member == value.MemberEnd())
            return fail(error, "geometer.contract.missing_field", child_path(path, "shells"),
                        "Required field is missing.");
        if (!decode_array(member->value, &out->shells, child_path(path, "shells"), error, 0U, 1024U,
                          decode_ShellSummary))
            return false;
    }
    {
        const auto member = value.FindMember("faces");
        if (member == value.MemberEnd())
            return fail(error, "geometer.contract.missing_field", child_path(path, "faces"),
                        "Required field is missing.");
        if (!decode_array(member->value, &out->faces, child_path(path, "faces"), error, 0U, 1024U,
                          decode_FaceSummary))
            return false;
    }
    {
        const auto member = value.FindMember("memberships");
        if (member == value.MemberEnd())
            return fail(error, "geometer.contract.missing_field", child_path(path, "memberships"),
                        "Required field is missing.");
        if (!decode_array(member->value, &out->memberships, child_path(path, "memberships"), error,
                          0U, 1024U, decode_TopologyMembership))
            return false;
    }
    {
        const auto member = value.FindMember("next_cursor");
        if (member != value.MemberEnd())
        {
            std::string decoded{};
            if (!decode_string(member->value, &decoded, child_path(path, "next_cursor"), error, 0U,
                               256U))
                return false;
            out->next_cursor = std::move(decoded);
        }
        else
            out->next_cursor.reset();
    }
    return true;
}

bool write_TopologyPage(rapidjson::Writer<rapidjson::StringBuffer>& writer,
                        const TopologyPage& value, ContractError* error)
{
    writer.StartObject();
    writer.Key("definitions");
    if (!write_array(writer, value.definitions, error, 0U, 1024U, write_DefinitionSummary))
        return false;
    writer.Key("occurrences");
    if (!write_array(writer, value.occurrences, error, 0U, 1024U, write_OccurrenceSummary))
        return false;
    writer.Key("bodies");
    if (!write_array(writer, value.bodies, error, 0U, 1024U, write_BodySummary))
        return false;
    writer.Key("shells");
    if (!write_array(writer, value.shells, error, 0U, 1024U, write_ShellSummary))
        return false;
    writer.Key("faces");
    if (!write_array(writer, value.faces, error, 0U, 1024U, write_FaceSummary))
        return false;
    writer.Key("memberships");
    if (!write_array(writer, value.memberships, error, 0U, 1024U, write_TopologyMembership))
        return false;
    if (value.next_cursor.has_value())
    {
        writer.Key("next_cursor");
        if (!write_string(writer, *value.next_cursor, error, 0U, 256U))
            return false;
    }
    writer.EndObject();
    return true;
}

bool decode_TopologyTableAttachmentDescriptor(const rapidjson::Value& value,
                                              TopologyTableAttachmentDescriptor* out,
                                              const std::string& path, ContractError* error)
{
    static const char* const names[] = {"name", "media_type", "format", "bytes", "sha256"};
    if (!validate_object(value, names, 5U, path, error))
        return false;
    {
        const auto member = value.FindMember("name");
        if (member == value.MemberEnd())
            return fail(error, "geometer.contract.missing_field", child_path(path, "name"),
                        "Required field is missing.");
        if (!decode_literal_string(member->value, &out->name, child_path(path, "name"), error,
                                   "topology_table"))
            return false;
    }
    {
        const auto member = value.FindMember("media_type");
        if (member == value.MemberEnd())
            return fail(error, "geometer.contract.missing_field", child_path(path, "media_type"),
                        "Required field is missing.");
        if (!decode_literal_string(member->value, &out->media_type, child_path(path, "media_type"),
                                   error,
                                   "application/vnd.wavenumber.geometer.step-topology-table"))
            return false;
    }
    {
        const auto member = value.FindMember("format");
        if (member == value.MemberEnd())
            return fail(error, "geometer.contract.missing_field", child_path(path, "format"),
                        "Required field is missing.");
        if (!decode_literal_string(member->value, &out->format, child_path(path, "format"), error,
                                   "wn.geometer.step-topology-table.a0"))
            return false;
    }
    {
        const auto member = value.FindMember("bytes");
        if (member == value.MemberEnd())
            return fail(error, "geometer.contract.missing_field", child_path(path, "bytes"),
                        "Required field is missing.");
        if (!decode_uint32(member->value, &out->bytes, child_path(path, "bytes"), error, 1ULL,
                           134217728ULL))
            return false;
    }
    {
        const auto member = value.FindMember("sha256");
        if (member == value.MemberEnd())
            return fail(error, "geometer.contract.missing_field", child_path(path, "sha256"),
                        "Required field is missing.");
        if (!decode_string(member->value, &out->sha256, child_path(path, "sha256"), error, 64U,
                           64U))
            return false;
    }
    return true;
}

bool write_TopologyTableAttachmentDescriptor(rapidjson::Writer<rapidjson::StringBuffer>& writer,
                                             const TopologyTableAttachmentDescriptor& value,
                                             ContractError* error)
{
    writer.StartObject();
    writer.Key("name");
    if (!write_literal_string(writer, value.name, error, "topology_table"))
        return false;
    writer.Key("media_type");
    if (!write_literal_string(writer, value.media_type, error,
                              "application/vnd.wavenumber.geometer.step-topology-table"))
        return false;
    writer.Key("format");
    if (!write_literal_string(writer, value.format, error, "wn.geometer.step-topology-table.a0"))
        return false;
    writer.Key("bytes");
    if (!write_uint32(writer, value.bytes, error, 1ULL, 134217728ULL))
        return false;
    writer.Key("sha256");
    if (!write_string(writer, value.sha256, error, 64U, 64U))
        return false;
    writer.EndObject();
    return true;
}

bool decode_StepTopologyInspectResultA0(const rapidjson::Value& value,
                                        StepTopologyInspectResultA0* out, const std::string& path,
                                        ContractError* error)
{
    static const char* const names[] = {"schema", "session",       "counts",
                                        "page",   "compact_table", "diagnostics"};
    if (!validate_object(value, names, 6U, path, error))
        return false;
    {
        const auto member = value.FindMember("schema");
        if (member == value.MemberEnd())
            return fail(error, "geometer.contract.missing_field", child_path(path, "schema"),
                        "Required field is missing.");
        if (!decode_literal_string(member->value, &out->schema, child_path(path, "schema"), error,
                                   "geometry.step_topology.inspect.result.a0"))
            return false;
    }
    {
        const auto member = value.FindMember("session");
        if (member == value.MemberEnd())
            return fail(error, "geometer.contract.missing_field", child_path(path, "session"),
                        "Required field is missing.");
        if (!decode_SessionReference(member->value, &out->session, child_path(path, "session"),
                                     error))
            return false;
    }
    {
        const auto member = value.FindMember("counts");
        if (member == value.MemberEnd())
            return fail(error, "geometer.contract.missing_field", child_path(path, "counts"),
                        "Required field is missing.");
        if (!decode_InspectionCounts(member->value, &out->counts, child_path(path, "counts"),
                                     error))
            return false;
    }
    {
        const auto member = value.FindMember("page");
        if (member == value.MemberEnd())
            return fail(error, "geometer.contract.missing_field", child_path(path, "page"),
                        "Required field is missing.");
        if (!decode_TopologyPage(member->value, &out->page, child_path(path, "page"), error))
            return false;
    }
    {
        const auto member = value.FindMember("compact_table");
        if (member != value.MemberEnd())
        {
            TopologyTableAttachmentDescriptor decoded{};
            if (!decode_TopologyTableAttachmentDescriptor(member->value, &decoded,
                                                          child_path(path, "compact_table"), error))
                return false;
            out->compact_table = std::move(decoded);
        }
        else
            out->compact_table.reset();
    }
    {
        const auto member = value.FindMember("diagnostics");
        if (member == value.MemberEnd())
            return fail(error, "geometer.contract.missing_field", child_path(path, "diagnostics"),
                        "Required field is missing.");
        if (!decode_array(member->value, &out->diagnostics, child_path(path, "diagnostics"), error,
                          0U, 256U, decode_DiagnosticA0))
            return false;
    }
    return true;
}

bool write_StepTopologyInspectResultA0(rapidjson::Writer<rapidjson::StringBuffer>& writer,
                                       const StepTopologyInspectResultA0& value,
                                       ContractError* error)
{
    writer.StartObject();
    writer.Key("schema");
    if (!write_literal_string(writer, value.schema, error,
                              "geometry.step_topology.inspect.result.a0"))
        return false;
    writer.Key("session");
    if (!write_SessionReference(writer, value.session, error))
        return false;
    writer.Key("counts");
    if (!write_InspectionCounts(writer, value.counts, error))
        return false;
    writer.Key("page");
    if (!write_TopologyPage(writer, value.page, error))
        return false;
    if (value.compact_table.has_value())
    {
        writer.Key("compact_table");
        if (!write_TopologyTableAttachmentDescriptor(writer, *value.compact_table, error))
            return false;
    }
    writer.Key("diagnostics");
    if (!write_array(writer, value.diagnostics, error, 0U, 256U, write_DiagnosticA0))
        return false;
    writer.EndObject();
    return true;
}

bool decode_RenderCounts(const rapidjson::Value& value, RenderCounts* out, const std::string& path,
                         ContractError* error)
{
    static const char* const names[] = {"meshes", "instances", "primitives", "geometry_triangles",
                                        "instanced_triangles"};
    if (!validate_object(value, names, 5U, path, error))
        return false;
    {
        const auto member = value.FindMember("meshes");
        if (member == value.MemberEnd())
            return fail(error, "geometer.contract.missing_field", child_path(path, "meshes"),
                        "Required field is missing.");
        if (!decode_uint32(member->value, &out->meshes, child_path(path, "meshes"), error, 0ULL,
                           10000ULL))
            return false;
    }
    {
        const auto member = value.FindMember("instances");
        if (member == value.MemberEnd())
            return fail(error, "geometer.contract.missing_field", child_path(path, "instances"),
                        "Required field is missing.");
        if (!decode_uint32(member->value, &out->instances, child_path(path, "instances"), error,
                           0ULL, 100000ULL))
            return false;
    }
    {
        const auto member = value.FindMember("primitives");
        if (member == value.MemberEnd())
            return fail(error, "geometer.contract.missing_field", child_path(path, "primitives"),
                        "Required field is missing.");
        if (!decode_uint32(member->value, &out->primitives, child_path(path, "primitives"), error,
                           0ULL, 1000000ULL))
            return false;
    }
    {
        const auto member = value.FindMember("geometry_triangles");
        if (member == value.MemberEnd())
            return fail(error, "geometer.contract.missing_field",
                        child_path(path, "geometry_triangles"), "Required field is missing.");
        if (!decode_uint32(member->value, &out->geometry_triangles,
                           child_path(path, "geometry_triangles"), error, 0ULL, 10000000ULL))
            return false;
    }
    {
        const auto member = value.FindMember("instanced_triangles");
        if (member == value.MemberEnd())
            return fail(error, "geometer.contract.missing_field",
                        child_path(path, "instanced_triangles"), "Required field is missing.");
        if (!decode_uint32(member->value, &out->instanced_triangles,
                           child_path(path, "instanced_triangles"), error, 0ULL, 50000000ULL))
            return false;
    }
    return true;
}

bool write_RenderCounts(rapidjson::Writer<rapidjson::StringBuffer>& writer,
                        const RenderCounts& value, ContractError* error)
{
    writer.StartObject();
    writer.Key("meshes");
    if (!write_uint32(writer, value.meshes, error, 0ULL, 10000ULL))
        return false;
    writer.Key("instances");
    if (!write_uint32(writer, value.instances, error, 0ULL, 100000ULL))
        return false;
    writer.Key("primitives");
    if (!write_uint32(writer, value.primitives, error, 0ULL, 1000000ULL))
        return false;
    writer.Key("geometry_triangles");
    if (!write_uint32(writer, value.geometry_triangles, error, 0ULL, 10000000ULL))
        return false;
    writer.Key("instanced_triangles");
    if (!write_uint32(writer, value.instanced_triangles, error, 0ULL, 50000000ULL))
        return false;
    writer.EndObject();
    return true;
}

bool decode_RenderArtifactDescriptor(const rapidjson::Value& value, RenderArtifactDescriptor* out,
                                     const std::string& path, ContractError* error)
{
    static const char* const names[] = {"artifact_handle",        "content_sha256",
                                        "render_artifact_handle", "render_content_sha256",
                                        "binding_layout",         "geometry_length_unit",
                                        "source_length_unit",     "counts"};
    if (!validate_object(value, names, 8U, path, error))
        return false;
    {
        const auto member = value.FindMember("artifact_handle");
        if (member == value.MemberEnd())
            return fail(error, "geometer.contract.missing_field",
                        child_path(path, "artifact_handle"), "Required field is missing.");
        if (!decode_string(member->value, &out->artifact_handle,
                           child_path(path, "artifact_handle"), error, 68U, 68U))
            return false;
    }
    {
        const auto member = value.FindMember("content_sha256");
        if (member == value.MemberEnd())
            return fail(error, "geometer.contract.missing_field",
                        child_path(path, "content_sha256"), "Required field is missing.");
        if (!decode_string(member->value, &out->content_sha256, child_path(path, "content_sha256"),
                           error, 64U, 64U))
            return false;
    }
    {
        const auto member = value.FindMember("render_artifact_handle");
        if (member == value.MemberEnd())
            return fail(error, "geometer.contract.missing_field",
                        child_path(path, "render_artifact_handle"), "Required field is missing.");
        if (!decode_string(member->value, &out->render_artifact_handle,
                           child_path(path, "render_artifact_handle"), error, 68U, 68U))
            return false;
    }
    {
        const auto member = value.FindMember("render_content_sha256");
        if (member == value.MemberEnd())
            return fail(error, "geometer.contract.missing_field",
                        child_path(path, "render_content_sha256"), "Required field is missing.");
        if (!decode_string(member->value, &out->render_content_sha256,
                           child_path(path, "render_content_sha256"), error, 64U, 64U))
            return false;
    }
    {
        const auto member = value.FindMember("binding_layout");
        if (member == value.MemberEnd())
            return fail(error, "geometer.contract.missing_field",
                        child_path(path, "binding_layout"), "Required field is missing.");
        if (!decode_literal_string(member->value, &out->binding_layout,
                                   child_path(path, "binding_layout"), error, "node-primitive-a0"))
            return false;
    }
    {
        const auto member = value.FindMember("geometry_length_unit");
        if (member == value.MemberEnd())
            return fail(error, "geometer.contract.missing_field",
                        child_path(path, "geometry_length_unit"), "Required field is missing.");
        if (!decode_literal_string(member->value, &out->geometry_length_unit,
                                   child_path(path, "geometry_length_unit"), error, "meter"))
            return false;
    }
    {
        const auto member = value.FindMember("source_length_unit");
        if (member == value.MemberEnd())
            return fail(error, "geometer.contract.missing_field",
                        child_path(path, "source_length_unit"), "Required field is missing.");
        if (!decode_literal_string(member->value, &out->source_length_unit,
                                   child_path(path, "source_length_unit"), error, "millimeter"))
            return false;
    }
    {
        const auto member = value.FindMember("counts");
        if (member == value.MemberEnd())
            return fail(error, "geometer.contract.missing_field", child_path(path, "counts"),
                        "Required field is missing.");
        if (!decode_RenderCounts(member->value, &out->counts, child_path(path, "counts"), error))
            return false;
    }
    return true;
}

bool write_RenderArtifactDescriptor(rapidjson::Writer<rapidjson::StringBuffer>& writer,
                                    const RenderArtifactDescriptor& value, ContractError* error)
{
    writer.StartObject();
    writer.Key("artifact_handle");
    if (!write_string(writer, value.artifact_handle, error, 68U, 68U))
        return false;
    writer.Key("content_sha256");
    if (!write_string(writer, value.content_sha256, error, 64U, 64U))
        return false;
    writer.Key("render_artifact_handle");
    if (!write_string(writer, value.render_artifact_handle, error, 68U, 68U))
        return false;
    writer.Key("render_content_sha256");
    if (!write_string(writer, value.render_content_sha256, error, 64U, 64U))
        return false;
    writer.Key("binding_layout");
    if (!write_literal_string(writer, value.binding_layout, error, "node-primitive-a0"))
        return false;
    writer.Key("geometry_length_unit");
    if (!write_literal_string(writer, value.geometry_length_unit, error, "meter"))
        return false;
    writer.Key("source_length_unit");
    if (!write_literal_string(writer, value.source_length_unit, error, "millimeter"))
        return false;
    writer.Key("counts");
    if (!write_RenderCounts(writer, value.counts, error))
        return false;
    writer.EndObject();
    return true;
}

bool decode_GlbAttachmentDescriptor(const rapidjson::Value& value, GlbAttachmentDescriptor* out,
                                    const std::string& path, ContractError* error)
{
    static const char* const names[] = {"name", "media_type", "format", "bytes", "sha256"};
    if (!validate_object(value, names, 5U, path, error))
        return false;
    {
        const auto member = value.FindMember("name");
        if (member == value.MemberEnd())
            return fail(error, "geometer.contract.missing_field", child_path(path, "name"),
                        "Required field is missing.");
        if (!decode_literal_string(member->value, &out->name, child_path(path, "name"), error,
                                   "glb"))
            return false;
    }
    {
        const auto member = value.FindMember("media_type");
        if (member == value.MemberEnd())
            return fail(error, "geometer.contract.missing_field", child_path(path, "media_type"),
                        "Required field is missing.");
        if (!decode_literal_string(member->value, &out->media_type, child_path(path, "media_type"),
                                   error, "model/gltf-binary"))
            return false;
    }
    {
        const auto member = value.FindMember("format");
        if (member == value.MemberEnd())
            return fail(error, "geometer.contract.missing_field", child_path(path, "format"),
                        "Required field is missing.");
        if (!decode_literal_string(member->value, &out->format, child_path(path, "format"), error,
                                   "glb-2.0"))
            return false;
    }
    {
        const auto member = value.FindMember("bytes");
        if (member == value.MemberEnd())
            return fail(error, "geometer.contract.missing_field", child_path(path, "bytes"),
                        "Required field is missing.");
        if (!decode_uint32(member->value, &out->bytes, child_path(path, "bytes"), error, 1ULL,
                           268435456ULL))
            return false;
    }
    {
        const auto member = value.FindMember("sha256");
        if (member == value.MemberEnd())
            return fail(error, "geometer.contract.missing_field", child_path(path, "sha256"),
                        "Required field is missing.");
        if (!decode_string(member->value, &out->sha256, child_path(path, "sha256"), error, 64U,
                           64U))
            return false;
    }
    return true;
}

bool write_GlbAttachmentDescriptor(rapidjson::Writer<rapidjson::StringBuffer>& writer,
                                   const GlbAttachmentDescriptor& value, ContractError* error)
{
    writer.StartObject();
    writer.Key("name");
    if (!write_literal_string(writer, value.name, error, "glb"))
        return false;
    writer.Key("media_type");
    if (!write_literal_string(writer, value.media_type, error, "model/gltf-binary"))
        return false;
    writer.Key("format");
    if (!write_literal_string(writer, value.format, error, "glb-2.0"))
        return false;
    writer.Key("bytes");
    if (!write_uint32(writer, value.bytes, error, 1ULL, 268435456ULL))
        return false;
    writer.Key("sha256");
    if (!write_string(writer, value.sha256, error, 64U, 64U))
        return false;
    writer.EndObject();
    return true;
}

bool decode_TopologyBindingTableAttachmentDescriptor(const rapidjson::Value& value,
                                                     TopologyBindingTableAttachmentDescriptor* out,
                                                     const std::string& path, ContractError* error)
{
    static const char* const names[] = {"name", "media_type", "format", "bytes", "sha256"};
    if (!validate_object(value, names, 5U, path, error))
        return false;
    {
        const auto member = value.FindMember("name");
        if (member == value.MemberEnd())
            return fail(error, "geometer.contract.missing_field", child_path(path, "name"),
                        "Required field is missing.");
        if (!decode_literal_string(member->value, &out->name, child_path(path, "name"), error,
                                   "topology_binding_table"))
            return false;
    }
    {
        const auto member = value.FindMember("media_type");
        if (member == value.MemberEnd())
            return fail(error, "geometer.contract.missing_field", child_path(path, "media_type"),
                        "Required field is missing.");
        if (!decode_literal_string(
                member->value, &out->media_type, child_path(path, "media_type"), error,
                "application/vnd.wavenumber.geometer.step-topology-binding-table"))
            return false;
    }
    {
        const auto member = value.FindMember("format");
        if (member == value.MemberEnd())
            return fail(error, "geometer.contract.missing_field", child_path(path, "format"),
                        "Required field is missing.");
        if (!decode_literal_string(member->value, &out->format, child_path(path, "format"), error,
                                   "wn.geometer.step-topology-binding-table.a0"))
            return false;
    }
    {
        const auto member = value.FindMember("bytes");
        if (member == value.MemberEnd())
            return fail(error, "geometer.contract.missing_field", child_path(path, "bytes"),
                        "Required field is missing.");
        if (!decode_uint32(member->value, &out->bytes, child_path(path, "bytes"), error, 1ULL,
                           134217728ULL))
            return false;
    }
    {
        const auto member = value.FindMember("sha256");
        if (member == value.MemberEnd())
            return fail(error, "geometer.contract.missing_field", child_path(path, "sha256"),
                        "Required field is missing.");
        if (!decode_string(member->value, &out->sha256, child_path(path, "sha256"), error, 64U,
                           64U))
            return false;
    }
    return true;
}

bool write_TopologyBindingTableAttachmentDescriptor(
    rapidjson::Writer<rapidjson::StringBuffer>& writer,
    const TopologyBindingTableAttachmentDescriptor& value, ContractError* error)
{
    writer.StartObject();
    writer.Key("name");
    if (!write_literal_string(writer, value.name, error, "topology_binding_table"))
        return false;
    writer.Key("media_type");
    if (!write_literal_string(writer, value.media_type, error,
                              "application/vnd.wavenumber.geometer.step-topology-binding-table"))
        return false;
    writer.Key("format");
    if (!write_literal_string(writer, value.format, error,
                              "wn.geometer.step-topology-binding-table.a0"))
        return false;
    writer.Key("bytes");
    if (!write_uint32(writer, value.bytes, error, 1ULL, 134217728ULL))
        return false;
    writer.Key("sha256");
    if (!write_string(writer, value.sha256, error, 64U, 64U))
        return false;
    writer.EndObject();
    return true;
}

bool decode_StepTopologyRenderResultA0(const rapidjson::Value& value,
                                       StepTopologyRenderResultA0* out, const std::string& path,
                                       ContractError* error)
{
    static const char* const names[] = {"schema", "session", "artifact", "glb",
                                        "compact_binding_table"};
    if (!validate_object(value, names, 5U, path, error))
        return false;
    {
        const auto member = value.FindMember("schema");
        if (member == value.MemberEnd())
            return fail(error, "geometer.contract.missing_field", child_path(path, "schema"),
                        "Required field is missing.");
        if (!decode_literal_string(member->value, &out->schema, child_path(path, "schema"), error,
                                   "geometry.step_topology.render.result.a0"))
            return false;
    }
    {
        const auto member = value.FindMember("session");
        if (member == value.MemberEnd())
            return fail(error, "geometer.contract.missing_field", child_path(path, "session"),
                        "Required field is missing.");
        if (!decode_SessionReference(member->value, &out->session, child_path(path, "session"),
                                     error))
            return false;
    }
    {
        const auto member = value.FindMember("artifact");
        if (member == value.MemberEnd())
            return fail(error, "geometer.contract.missing_field", child_path(path, "artifact"),
                        "Required field is missing.");
        if (!decode_RenderArtifactDescriptor(member->value, &out->artifact,
                                             child_path(path, "artifact"), error))
            return false;
    }
    {
        const auto member = value.FindMember("glb");
        if (member == value.MemberEnd())
            return fail(error, "geometer.contract.missing_field", child_path(path, "glb"),
                        "Required field is missing.");
        if (!decode_GlbAttachmentDescriptor(member->value, &out->glb, child_path(path, "glb"),
                                            error))
            return false;
    }
    {
        const auto member = value.FindMember("compact_binding_table");
        if (member != value.MemberEnd())
        {
            TopologyBindingTableAttachmentDescriptor decoded{};
            if (!decode_TopologyBindingTableAttachmentDescriptor(
                    member->value, &decoded, child_path(path, "compact_binding_table"), error))
                return false;
            out->compact_binding_table = std::move(decoded);
        }
        else
            out->compact_binding_table.reset();
    }
    return true;
}

bool write_StepTopologyRenderResultA0(rapidjson::Writer<rapidjson::StringBuffer>& writer,
                                      const StepTopologyRenderResultA0& value, ContractError* error)
{
    writer.StartObject();
    writer.Key("schema");
    if (!write_literal_string(writer, value.schema, error,
                              "geometry.step_topology.render.result.a0"))
        return false;
    writer.Key("session");
    if (!write_SessionReference(writer, value.session, error))
        return false;
    writer.Key("artifact");
    if (!write_RenderArtifactDescriptor(writer, value.artifact, error))
        return false;
    writer.Key("glb");
    if (!write_GlbAttachmentDescriptor(writer, value.glb, error))
        return false;
    if (value.compact_binding_table.has_value())
    {
        writer.Key("compact_binding_table");
        if (!write_TopologyBindingTableAttachmentDescriptor(writer, *value.compact_binding_table,
                                                            error))
            return false;
    }
    writer.EndObject();
    return true;
}

bool decode_StepTopologyResolveHitResultA0(const rapidjson::Value& value,
                                           StepTopologyResolveHitResultA0* out,
                                           const std::string& path, ContractError* error)
{
    static const char* const names[] = {"schema",          "session",        "instance_index",
                                        "primitive_index", "triangle_index", "occurrence_handle",
                                        "body_handle",     "face_handle"};
    if (!validate_object(value, names, 8U, path, error))
        return false;
    {
        const auto member = value.FindMember("schema");
        if (member == value.MemberEnd())
            return fail(error, "geometer.contract.missing_field", child_path(path, "schema"),
                        "Required field is missing.");
        if (!decode_literal_string(member->value, &out->schema, child_path(path, "schema"), error,
                                   "geometry.step_topology.resolve_hit.result.a0"))
            return false;
    }
    {
        const auto member = value.FindMember("session");
        if (member == value.MemberEnd())
            return fail(error, "geometer.contract.missing_field", child_path(path, "session"),
                        "Required field is missing.");
        if (!decode_SessionReference(member->value, &out->session, child_path(path, "session"),
                                     error))
            return false;
    }
    {
        const auto member = value.FindMember("instance_index");
        if (member == value.MemberEnd())
            return fail(error, "geometer.contract.missing_field",
                        child_path(path, "instance_index"), "Required field is missing.");
        if (!decode_uint32(member->value, &out->instance_index, child_path(path, "instance_index"),
                           error, 0ULL, 99999ULL))
            return false;
    }
    {
        const auto member = value.FindMember("primitive_index");
        if (member == value.MemberEnd())
            return fail(error, "geometer.contract.missing_field",
                        child_path(path, "primitive_index"), "Required field is missing.");
        if (!decode_uint32(member->value, &out->primitive_index,
                           child_path(path, "primitive_index"), error, 0ULL, 999999ULL))
            return false;
    }
    {
        const auto member = value.FindMember("triangle_index");
        if (member == value.MemberEnd())
            return fail(error, "geometer.contract.missing_field",
                        child_path(path, "triangle_index"), "Required field is missing.");
        if (!decode_uint32(member->value, &out->triangle_index, child_path(path, "triangle_index"),
                           error, 0ULL, 9999999ULL))
            return false;
    }
    {
        const auto member = value.FindMember("occurrence_handle");
        if (member == value.MemberEnd())
            return fail(error, "geometer.contract.missing_field",
                        child_path(path, "occurrence_handle"), "Required field is missing.");
        if (!decode_string(member->value, &out->occurrence_handle,
                           child_path(path, "occurrence_handle"), error, 68U, 68U))
            return false;
    }
    {
        const auto member = value.FindMember("body_handle");
        if (member == value.MemberEnd())
            return fail(error, "geometer.contract.missing_field", child_path(path, "body_handle"),
                        "Required field is missing.");
        if (!decode_string(member->value, &out->body_handle, child_path(path, "body_handle"), error,
                           68U, 68U))
            return false;
    }
    {
        const auto member = value.FindMember("face_handle");
        if (member == value.MemberEnd())
            return fail(error, "geometer.contract.missing_field", child_path(path, "face_handle"),
                        "Required field is missing.");
        if (!decode_string(member->value, &out->face_handle, child_path(path, "face_handle"), error,
                           68U, 68U))
            return false;
    }
    return true;
}

bool write_StepTopologyResolveHitResultA0(rapidjson::Writer<rapidjson::StringBuffer>& writer,
                                          const StepTopologyResolveHitResultA0& value,
                                          ContractError* error)
{
    writer.StartObject();
    writer.Key("schema");
    if (!write_literal_string(writer, value.schema, error,
                              "geometry.step_topology.resolve_hit.result.a0"))
        return false;
    writer.Key("session");
    if (!write_SessionReference(writer, value.session, error))
        return false;
    writer.Key("instance_index");
    if (!write_uint32(writer, value.instance_index, error, 0ULL, 99999ULL))
        return false;
    writer.Key("primitive_index");
    if (!write_uint32(writer, value.primitive_index, error, 0ULL, 999999ULL))
        return false;
    writer.Key("triangle_index");
    if (!write_uint32(writer, value.triangle_index, error, 0ULL, 9999999ULL))
        return false;
    writer.Key("occurrence_handle");
    if (!write_string(writer, value.occurrence_handle, error, 68U, 68U))
        return false;
    writer.Key("body_handle");
    if (!write_string(writer, value.body_handle, error, 68U, 68U))
        return false;
    writer.Key("face_handle");
    if (!write_string(writer, value.face_handle, error, 68U, 68U))
        return false;
    writer.EndObject();
    return true;
}

bool decode_MutationSessionState(const rapidjson::Value& value, MutationSessionState* out,
                                 const std::string& path, ContractError* error)
{
    static const char* const names[] = {"session", "edit_journal_revision",
                                        "accounted_string_bytes", "estimated_resident_bytes"};
    if (!validate_object(value, names, 4U, path, error))
        return false;
    {
        const auto member = value.FindMember("session");
        if (member == value.MemberEnd())
            return fail(error, "geometer.contract.missing_field", child_path(path, "session"),
                        "Required field is missing.");
        if (!decode_SessionReference(member->value, &out->session, child_path(path, "session"),
                                     error))
            return false;
    }
    {
        const auto member = value.FindMember("edit_journal_revision");
        if (member == value.MemberEnd())
            return fail(error, "geometer.contract.missing_field",
                        child_path(path, "edit_journal_revision"), "Required field is missing.");
        if (!decode_uint32(member->value, &out->edit_journal_revision,
                           child_path(path, "edit_journal_revision"), error, 0ULL, 100000ULL))
            return false;
    }
    {
        const auto member = value.FindMember("accounted_string_bytes");
        if (member == value.MemberEnd())
            return fail(error, "geometer.contract.missing_field",
                        child_path(path, "accounted_string_bytes"), "Required field is missing.");
        if (!decode_uint32(member->value, &out->accounted_string_bytes,
                           child_path(path, "accounted_string_bytes"), error, 0ULL, 16777216ULL))
            return false;
    }
    {
        const auto member = value.FindMember("estimated_resident_bytes");
        if (member == value.MemberEnd())
            return fail(error, "geometer.contract.missing_field",
                        child_path(path, "estimated_resident_bytes"), "Required field is missing.");
        if (!decode_uint32(member->value, &out->estimated_resident_bytes,
                           child_path(path, "estimated_resident_bytes"), error, 0ULL, 536870912ULL))
            return false;
    }
    return true;
}

bool write_MutationSessionState(rapidjson::Writer<rapidjson::StringBuffer>& writer,
                                const MutationSessionState& value, ContractError* error)
{
    writer.StartObject();
    writer.Key("session");
    if (!write_SessionReference(writer, value.session, error))
        return false;
    writer.Key("edit_journal_revision");
    if (!write_uint32(writer, value.edit_journal_revision, error, 0ULL, 100000ULL))
        return false;
    writer.Key("accounted_string_bytes");
    if (!write_uint32(writer, value.accounted_string_bytes, error, 0ULL, 16777216ULL))
        return false;
    writer.Key("estimated_resident_bytes");
    if (!write_uint32(writer, value.estimated_resident_bytes, error, 0ULL, 536870912ULL))
        return false;
    writer.EndObject();
    return true;
}

bool decode_LogicalGroupMember(const rapidjson::Value& value, LogicalGroupMember* out,
                               const std::string& path, ContractError* error)
{
    static const char* const names[] = {"kind", "target_handle"};
    if (!validate_object(value, names, 2U, path, error))
        return false;
    {
        const auto member = value.FindMember("kind");
        if (member == value.MemberEnd())
            return fail(error, "geometer.contract.missing_field", child_path(path, "kind"),
                        "Required field is missing.");
        if (!decode_LogicalGroupMemberKind(member->value, &out->kind, child_path(path, "kind"),
                                           error))
            return false;
    }
    {
        const auto member = value.FindMember("target_handle");
        if (member == value.MemberEnd())
            return fail(error, "geometer.contract.missing_field", child_path(path, "target_handle"),
                        "Required field is missing.");
        if (!decode_string(member->value, &out->target_handle, child_path(path, "target_handle"),
                           error, 68U, 68U))
            return false;
    }
    return true;
}

bool write_LogicalGroupMember(rapidjson::Writer<rapidjson::StringBuffer>& writer,
                              const LogicalGroupMember& value, ContractError* error)
{
    writer.StartObject();
    writer.Key("kind");
    if (!write_LogicalGroupMemberKind(writer, value.kind, error))
        return false;
    writer.Key("target_handle");
    if (!write_string(writer, value.target_handle, error, 68U, 68U))
        return false;
    writer.EndObject();
    return true;
}

bool decode_LogicalGroup(const rapidjson::Value& value, LogicalGroup* out, const std::string& path,
                         ContractError* error)
{
    static const char* const names[] = {"authored_id", "revision", "name", "members"};
    if (!validate_object(value, names, 4U, path, error))
        return false;
    {
        const auto member = value.FindMember("authored_id");
        if (member == value.MemberEnd())
            return fail(error, "geometer.contract.missing_field", child_path(path, "authored_id"),
                        "Required field is missing.");
        if (!decode_string(member->value, &out->authored_id, child_path(path, "authored_id"), error,
                           28U, 128U))
            return false;
    }
    {
        const auto member = value.FindMember("revision");
        if (member == value.MemberEnd())
            return fail(error, "geometer.contract.missing_field", child_path(path, "revision"),
                        "Required field is missing.");
        if (!decode_uint32(member->value, &out->revision, child_path(path, "revision"), error, 1ULL,
                           std::numeric_limits<std::uint32_t>::max()))
            return false;
    }
    {
        const auto member = value.FindMember("name");
        if (member == value.MemberEnd())
            return fail(error, "geometer.contract.missing_field", child_path(path, "name"),
                        "Required field is missing.");
        if (!decode_string(member->value, &out->name, child_path(path, "name"), error, 1U, 4096U))
            return false;
    }
    {
        const auto member = value.FindMember("members");
        if (member == value.MemberEnd())
            return fail(error, "geometer.contract.missing_field", child_path(path, "members"),
                        "Required field is missing.");
        if (!decode_array(member->value, &out->members, child_path(path, "members"), error, 1U,
                          100000U, decode_LogicalGroupMember))
            return false;
    }
    return true;
}

bool write_LogicalGroup(rapidjson::Writer<rapidjson::StringBuffer>& writer,
                        const LogicalGroup& value, ContractError* error)
{
    writer.StartObject();
    writer.Key("authored_id");
    if (!write_string(writer, value.authored_id, error, 28U, 128U))
        return false;
    writer.Key("revision");
    if (!write_uint32(writer, value.revision, error, 1ULL,
                      std::numeric_limits<std::uint32_t>::max()))
        return false;
    writer.Key("name");
    if (!write_string(writer, value.name, error, 1U, 4096U))
        return false;
    writer.Key("members");
    if (!write_array(writer, value.members, error, 1U, 100000U, write_LogicalGroupMember))
        return false;
    writer.EndObject();
    return true;
}

bool decode_StepTopologyApplyLogicalGroupsResultA0(const rapidjson::Value& value,
                                                   StepTopologyApplyLogicalGroupsResultA0* out,
                                                   const std::string& path, ContractError* error)
{
    static const char* const names[] = {"schema", "state", "groups", "diagnostics"};
    if (!validate_object(value, names, 4U, path, error))
        return false;
    {
        const auto member = value.FindMember("schema");
        if (member == value.MemberEnd())
            return fail(error, "geometer.contract.missing_field", child_path(path, "schema"),
                        "Required field is missing.");
        if (!decode_literal_string(member->value, &out->schema, child_path(path, "schema"), error,
                                   "geometry.step_topology.apply_logical_groups.result.a0"))
            return false;
    }
    {
        const auto member = value.FindMember("state");
        if (member == value.MemberEnd())
            return fail(error, "geometer.contract.missing_field", child_path(path, "state"),
                        "Required field is missing.");
        if (!decode_MutationSessionState(member->value, &out->state, child_path(path, "state"),
                                         error))
            return false;
    }
    {
        const auto member = value.FindMember("groups");
        if (member == value.MemberEnd())
            return fail(error, "geometer.contract.missing_field", child_path(path, "groups"),
                        "Required field is missing.");
        if (!decode_array(member->value, &out->groups, child_path(path, "groups"), error, 0U,
                          10000U, decode_LogicalGroup))
            return false;
    }
    {
        const auto member = value.FindMember("diagnostics");
        if (member == value.MemberEnd())
            return fail(error, "geometer.contract.missing_field", child_path(path, "diagnostics"),
                        "Required field is missing.");
        if (!decode_array(member->value, &out->diagnostics, child_path(path, "diagnostics"), error,
                          0U, 256U, decode_DiagnosticA0))
            return false;
    }
    return true;
}

bool write_StepTopologyApplyLogicalGroupsResultA0(
    rapidjson::Writer<rapidjson::StringBuffer>& writer,
    const StepTopologyApplyLogicalGroupsResultA0& value, ContractError* error)
{
    writer.StartObject();
    writer.Key("schema");
    if (!write_literal_string(writer, value.schema, error,
                              "geometry.step_topology.apply_logical_groups.result.a0"))
        return false;
    writer.Key("state");
    if (!write_MutationSessionState(writer, value.state, error))
        return false;
    writer.Key("groups");
    if (!write_array(writer, value.groups, error, 0U, 10000U, write_LogicalGroup))
        return false;
    writer.Key("diagnostics");
    if (!write_array(writer, value.diagnostics, error, 0U, 256U, write_DiagnosticA0))
        return false;
    writer.EndObject();
    return true;
}

bool decode_MetadataProbe(const rapidjson::Value& value, MetadataProbe* out,
                          const std::string& path, ContractError* error)
{
    static const char* const names[] = {"authored_id", "revision", "target", "key", "value"};
    if (!validate_object(value, names, 5U, path, error))
        return false;
    {
        const auto member = value.FindMember("authored_id");
        if (member == value.MemberEnd())
            return fail(error, "geometer.contract.missing_field", child_path(path, "authored_id"),
                        "Required field is missing.");
        if (!decode_string(member->value, &out->authored_id, child_path(path, "authored_id"), error,
                           28U, 128U))
            return false;
    }
    {
        const auto member = value.FindMember("revision");
        if (member == value.MemberEnd())
            return fail(error, "geometer.contract.missing_field", child_path(path, "revision"),
                        "Required field is missing.");
        if (!decode_uint32(member->value, &out->revision, child_path(path, "revision"), error, 1ULL,
                           std::numeric_limits<std::uint32_t>::max()))
            return false;
    }
    {
        const auto member = value.FindMember("target");
        if (member == value.MemberEnd())
            return fail(error, "geometer.contract.missing_field", child_path(path, "target"),
                        "Required field is missing.");
        if (!decode_MetadataProbeTarget(member->value, &out->target, child_path(path, "target"),
                                        error))
            return false;
    }
    {
        const auto member = value.FindMember("key");
        if (member == value.MemberEnd())
            return fail(error, "geometer.contract.missing_field", child_path(path, "key"),
                        "Required field is missing.");
        if (!decode_string(member->value, &out->key, child_path(path, "key"), error, 32U, 128U))
            return false;
    }
    {
        const auto member = value.FindMember("value");
        if (member == value.MemberEnd())
            return fail(error, "geometer.contract.missing_field", child_path(path, "value"),
                        "Required field is missing.");
        if (!decode_string(member->value, &out->value, child_path(path, "value"), error, 1U, 4096U))
            return false;
    }
    return true;
}

bool write_MetadataProbe(rapidjson::Writer<rapidjson::StringBuffer>& writer,
                         const MetadataProbe& value, ContractError* error)
{
    writer.StartObject();
    writer.Key("authored_id");
    if (!write_string(writer, value.authored_id, error, 28U, 128U))
        return false;
    writer.Key("revision");
    if (!write_uint32(writer, value.revision, error, 1ULL,
                      std::numeric_limits<std::uint32_t>::max()))
        return false;
    writer.Key("target");
    if (!write_MetadataProbeTarget(writer, value.target, error))
        return false;
    writer.Key("key");
    if (!write_string(writer, value.key, error, 32U, 128U))
        return false;
    writer.Key("value");
    if (!write_string(writer, value.value, error, 1U, 4096U))
        return false;
    writer.EndObject();
    return true;
}

bool decode_StepTopologyApplyMetadataProbesResultA0(const rapidjson::Value& value,
                                                    StepTopologyApplyMetadataProbesResultA0* out,
                                                    const std::string& path, ContractError* error)
{
    static const char* const names[] = {"schema", "state", "groups", "probes", "diagnostics"};
    if (!validate_object(value, names, 5U, path, error))
        return false;
    {
        const auto member = value.FindMember("schema");
        if (member == value.MemberEnd())
            return fail(error, "geometer.contract.missing_field", child_path(path, "schema"),
                        "Required field is missing.");
        if (!decode_literal_string(member->value, &out->schema, child_path(path, "schema"), error,
                                   "geometry.step_topology.apply_metadata_probes.result.a0"))
            return false;
    }
    {
        const auto member = value.FindMember("state");
        if (member == value.MemberEnd())
            return fail(error, "geometer.contract.missing_field", child_path(path, "state"),
                        "Required field is missing.");
        if (!decode_MutationSessionState(member->value, &out->state, child_path(path, "state"),
                                         error))
            return false;
    }
    {
        const auto member = value.FindMember("groups");
        if (member == value.MemberEnd())
            return fail(error, "geometer.contract.missing_field", child_path(path, "groups"),
                        "Required field is missing.");
        if (!decode_array(member->value, &out->groups, child_path(path, "groups"), error, 0U,
                          10000U, decode_LogicalGroup))
            return false;
    }
    {
        const auto member = value.FindMember("probes");
        if (member == value.MemberEnd())
            return fail(error, "geometer.contract.missing_field", child_path(path, "probes"),
                        "Required field is missing.");
        if (!decode_array(member->value, &out->probes, child_path(path, "probes"), error, 0U,
                          10000U, decode_MetadataProbe))
            return false;
    }
    {
        const auto member = value.FindMember("diagnostics");
        if (member == value.MemberEnd())
            return fail(error, "geometer.contract.missing_field", child_path(path, "diagnostics"),
                        "Required field is missing.");
        if (!decode_array(member->value, &out->diagnostics, child_path(path, "diagnostics"), error,
                          0U, 256U, decode_DiagnosticA0))
            return false;
    }
    return true;
}

bool write_StepTopologyApplyMetadataProbesResultA0(
    rapidjson::Writer<rapidjson::StringBuffer>& writer,
    const StepTopologyApplyMetadataProbesResultA0& value, ContractError* error)
{
    writer.StartObject();
    writer.Key("schema");
    if (!write_literal_string(writer, value.schema, error,
                              "geometry.step_topology.apply_metadata_probes.result.a0"))
        return false;
    writer.Key("state");
    if (!write_MutationSessionState(writer, value.state, error))
        return false;
    writer.Key("groups");
    if (!write_array(writer, value.groups, error, 0U, 10000U, write_LogicalGroup))
        return false;
    writer.Key("probes");
    if (!write_array(writer, value.probes, error, 0U, 10000U, write_MetadataProbe))
        return false;
    writer.Key("diagnostics");
    if (!write_array(writer, value.diagnostics, error, 0U, 256U, write_DiagnosticA0))
        return false;
    writer.EndObject();
    return true;
}

bool decode_EditJournalAttachmentDescriptor(const rapidjson::Value& value,
                                            EditJournalAttachmentDescriptor* out,
                                            const std::string& path, ContractError* error)
{
    static const char* const names[] = {"name", "media_type", "format", "bytes", "sha256"};
    if (!validate_object(value, names, 5U, path, error))
        return false;
    {
        const auto member = value.FindMember("name");
        if (member == value.MemberEnd())
            return fail(error, "geometer.contract.missing_field", child_path(path, "name"),
                        "Required field is missing.");
        if (!decode_literal_string(member->value, &out->name, child_path(path, "name"), error,
                                   "edit_journal"))
            return false;
    }
    {
        const auto member = value.FindMember("media_type");
        if (member == value.MemberEnd())
            return fail(error, "geometer.contract.missing_field", child_path(path, "media_type"),
                        "Required field is missing.");
        if (!decode_literal_string(
                member->value, &out->media_type, child_path(path, "media_type"), error,
                "application/vnd.wavenumber.geometer.step-topology-edit-journal"))
            return false;
    }
    {
        const auto member = value.FindMember("format");
        if (member == value.MemberEnd())
            return fail(error, "geometer.contract.missing_field", child_path(path, "format"),
                        "Required field is missing.");
        if (!decode_literal_string(member->value, &out->format, child_path(path, "format"), error,
                                   "geometer.step_topology_edit_journal.a0"))
            return false;
    }
    {
        const auto member = value.FindMember("bytes");
        if (member == value.MemberEnd())
            return fail(error, "geometer.contract.missing_field", child_path(path, "bytes"),
                        "Required field is missing.");
        if (!decode_uint32(member->value, &out->bytes, child_path(path, "bytes"), error, 1ULL,
                           67108864ULL))
            return false;
    }
    {
        const auto member = value.FindMember("sha256");
        if (member == value.MemberEnd())
            return fail(error, "geometer.contract.missing_field", child_path(path, "sha256"),
                        "Required field is missing.");
        if (!decode_string(member->value, &out->sha256, child_path(path, "sha256"), error, 64U,
                           64U))
            return false;
    }
    return true;
}

bool write_EditJournalAttachmentDescriptor(rapidjson::Writer<rapidjson::StringBuffer>& writer,
                                           const EditJournalAttachmentDescriptor& value,
                                           ContractError* error)
{
    writer.StartObject();
    writer.Key("name");
    if (!write_literal_string(writer, value.name, error, "edit_journal"))
        return false;
    writer.Key("media_type");
    if (!write_literal_string(writer, value.media_type, error,
                              "application/vnd.wavenumber.geometer.step-topology-edit-journal"))
        return false;
    writer.Key("format");
    if (!write_literal_string(writer, value.format, error,
                              "geometer.step_topology_edit_journal.a0"))
        return false;
    writer.Key("bytes");
    if (!write_uint32(writer, value.bytes, error, 1ULL, 67108864ULL))
        return false;
    writer.Key("sha256");
    if (!write_string(writer, value.sha256, error, 64U, 64U))
        return false;
    writer.EndObject();
    return true;
}

bool decode_StepTopologyCheckpointEditJournalResultA0(
    const rapidjson::Value& value, StepTopologyCheckpointEditJournalResultA0* out,
    const std::string& path, ContractError* error)
{
    static const char* const names[] = {"schema",
                                        "state",
                                        "source_sha256",
                                        "source_brep_sha256",
                                        "target_inventory_sha256",
                                        "occt_version",
                                        "transaction_count",
                                        "journal",
                                        "diagnostics"};
    if (!validate_object(value, names, 9U, path, error))
        return false;
    {
        const auto member = value.FindMember("schema");
        if (member == value.MemberEnd())
            return fail(error, "geometer.contract.missing_field", child_path(path, "schema"),
                        "Required field is missing.");
        if (!decode_literal_string(member->value, &out->schema, child_path(path, "schema"), error,
                                   "geometry.step_topology.checkpoint_edit_journal.result.a0"))
            return false;
    }
    {
        const auto member = value.FindMember("state");
        if (member == value.MemberEnd())
            return fail(error, "geometer.contract.missing_field", child_path(path, "state"),
                        "Required field is missing.");
        if (!decode_MutationSessionState(member->value, &out->state, child_path(path, "state"),
                                         error))
            return false;
    }
    {
        const auto member = value.FindMember("source_sha256");
        if (member == value.MemberEnd())
            return fail(error, "geometer.contract.missing_field", child_path(path, "source_sha256"),
                        "Required field is missing.");
        if (!decode_string(member->value, &out->source_sha256, child_path(path, "source_sha256"),
                           error, 64U, 64U))
            return false;
    }
    {
        const auto member = value.FindMember("source_brep_sha256");
        if (member == value.MemberEnd())
            return fail(error, "geometer.contract.missing_field",
                        child_path(path, "source_brep_sha256"), "Required field is missing.");
        if (!decode_string(member->value, &out->source_brep_sha256,
                           child_path(path, "source_brep_sha256"), error, 64U, 64U))
            return false;
    }
    {
        const auto member = value.FindMember("target_inventory_sha256");
        if (member == value.MemberEnd())
            return fail(error, "geometer.contract.missing_field",
                        child_path(path, "target_inventory_sha256"), "Required field is missing.");
        if (!decode_string(member->value, &out->target_inventory_sha256,
                           child_path(path, "target_inventory_sha256"), error, 64U, 64U))
            return false;
    }
    {
        const auto member = value.FindMember("occt_version");
        if (member == value.MemberEnd())
            return fail(error, "geometer.contract.missing_field", child_path(path, "occt_version"),
                        "Required field is missing.");
        if (!decode_string(member->value, &out->occt_version, child_path(path, "occt_version"),
                           error, 1U, 64U))
            return false;
    }
    {
        const auto member = value.FindMember("transaction_count");
        if (member == value.MemberEnd())
            return fail(error, "geometer.contract.missing_field",
                        child_path(path, "transaction_count"), "Required field is missing.");
        if (!decode_uint32(member->value, &out->transaction_count,
                           child_path(path, "transaction_count"), error, 0ULL, 100000ULL))
            return false;
    }
    {
        const auto member = value.FindMember("journal");
        if (member == value.MemberEnd())
            return fail(error, "geometer.contract.missing_field", child_path(path, "journal"),
                        "Required field is missing.");
        if (!decode_EditJournalAttachmentDescriptor(member->value, &out->journal,
                                                    child_path(path, "journal"), error))
            return false;
    }
    {
        const auto member = value.FindMember("diagnostics");
        if (member == value.MemberEnd())
            return fail(error, "geometer.contract.missing_field", child_path(path, "diagnostics"),
                        "Required field is missing.");
        if (!decode_array(member->value, &out->diagnostics, child_path(path, "diagnostics"), error,
                          0U, 256U, decode_DiagnosticA0))
            return false;
    }
    return true;
}

bool write_StepTopologyCheckpointEditJournalResultA0(
    rapidjson::Writer<rapidjson::StringBuffer>& writer,
    const StepTopologyCheckpointEditJournalResultA0& value, ContractError* error)
{
    writer.StartObject();
    writer.Key("schema");
    if (!write_literal_string(writer, value.schema, error,
                              "geometry.step_topology.checkpoint_edit_journal.result.a0"))
        return false;
    writer.Key("state");
    if (!write_MutationSessionState(writer, value.state, error))
        return false;
    writer.Key("source_sha256");
    if (!write_string(writer, value.source_sha256, error, 64U, 64U))
        return false;
    writer.Key("source_brep_sha256");
    if (!write_string(writer, value.source_brep_sha256, error, 64U, 64U))
        return false;
    writer.Key("target_inventory_sha256");
    if (!write_string(writer, value.target_inventory_sha256, error, 64U, 64U))
        return false;
    writer.Key("occt_version");
    if (!write_string(writer, value.occt_version, error, 1U, 64U))
        return false;
    writer.Key("transaction_count");
    if (!write_uint32(writer, value.transaction_count, error, 0ULL, 100000ULL))
        return false;
    writer.Key("journal");
    if (!write_EditJournalAttachmentDescriptor(writer, value.journal, error))
        return false;
    writer.Key("diagnostics");
    if (!write_array(writer, value.diagnostics, error, 0U, 256U, write_DiagnosticA0))
        return false;
    writer.EndObject();
    return true;
}

bool decode_HierarchyNodeKind(const rapidjson::Value& value, HierarchyNodeKind* out,
                              const std::string& path, ContractError* error)
{
    if (!value.IsString())
        return fail(error, "geometer.contract.type_mismatch", path, "Expected a string enum.");
    const std::string text(value.GetString(), value.GetStringLength());
    if (text == "product")
    {
        *out = HierarchyNodeKind::product;
        return true;
    }
    if (text == "assembly")
    {
        *out = HierarchyNodeKind::assembly;
        return true;
    }
    return fail(error, "geometer.contract.enum_mismatch", path, "Unknown enum value.");
}

bool write_HierarchyNodeKind(rapidjson::Writer<rapidjson::StringBuffer>& writer,
                             const HierarchyNodeKind& value, ContractError* error)
{
    switch (value)
    {
    case HierarchyNodeKind::product:
        writer.String("product");
        return true;
    case HierarchyNodeKind::assembly:
        writer.String("assembly");
        return true;
    }
    return fail(error, "geometer.contract.enum_mismatch", "", "Unknown enum value.");
}

bool decode_HierarchyNode(const rapidjson::Value& value, HierarchyNode* out,
                          const std::string& path, ContractError* error)
{
    static const char* const names[] = {"authored_id", "revision",    "kind",
                                        "name",        "source_kind", "source_handle"};
    if (!validate_object(value, names, 6U, path, error))
        return false;
    {
        const auto member = value.FindMember("authored_id");
        if (member == value.MemberEnd())
            return fail(error, "geometer.contract.missing_field", child_path(path, "authored_id"),
                        "Required field is missing.");
        if (!decode_string(member->value, &out->authored_id, child_path(path, "authored_id"), error,
                           28U, 128U))
            return false;
    }
    {
        const auto member = value.FindMember("revision");
        if (member == value.MemberEnd())
            return fail(error, "geometer.contract.missing_field", child_path(path, "revision"),
                        "Required field is missing.");
        if (!decode_uint32(member->value, &out->revision, child_path(path, "revision"), error, 1ULL,
                           std::numeric_limits<std::uint32_t>::max()))
            return false;
    }
    {
        const auto member = value.FindMember("kind");
        if (member == value.MemberEnd())
            return fail(error, "geometer.contract.missing_field", child_path(path, "kind"),
                        "Required field is missing.");
        if (!decode_HierarchyNodeKind(member->value, &out->kind, child_path(path, "kind"), error))
            return false;
    }
    {
        const auto member = value.FindMember("name");
        if (member == value.MemberEnd())
            return fail(error, "geometer.contract.missing_field", child_path(path, "name"),
                        "Required field is missing.");
        if (!decode_string(member->value, &out->name, child_path(path, "name"), error, 1U, 4096U))
            return false;
    }
    {
        const auto member = value.FindMember("source_kind");
        if (member != value.MemberEnd())
        {
            HierarchySourceKind decoded{};
            if (!decode_HierarchySourceKind(member->value, &decoded,
                                            child_path(path, "source_kind"), error))
                return false;
            out->source_kind = std::move(decoded);
        }
        else
            out->source_kind.reset();
    }
    {
        const auto member = value.FindMember("source_handle");
        if (member != value.MemberEnd())
        {
            std::string decoded{};
            if (!decode_string(member->value, &decoded, child_path(path, "source_handle"), error,
                               68U, 68U))
                return false;
            out->source_handle = std::move(decoded);
        }
        else
            out->source_handle.reset();
    }
    return true;
}

bool write_HierarchyNode(rapidjson::Writer<rapidjson::StringBuffer>& writer,
                         const HierarchyNode& value, ContractError* error)
{
    writer.StartObject();
    writer.Key("authored_id");
    if (!write_string(writer, value.authored_id, error, 28U, 128U))
        return false;
    writer.Key("revision");
    if (!write_uint32(writer, value.revision, error, 1ULL,
                      std::numeric_limits<std::uint32_t>::max()))
        return false;
    writer.Key("kind");
    if (!write_HierarchyNodeKind(writer, value.kind, error))
        return false;
    writer.Key("name");
    if (!write_string(writer, value.name, error, 1U, 4096U))
        return false;
    if (value.source_kind.has_value())
    {
        writer.Key("source_kind");
        if (!write_HierarchySourceKind(writer, *value.source_kind, error))
            return false;
    }
    if (value.source_handle.has_value())
    {
        writer.Key("source_handle");
        if (!write_string(writer, *value.source_handle, error, 68U, 68U))
            return false;
    }
    writer.EndObject();
    return true;
}

bool decode_HierarchyOccurrence(const rapidjson::Value& value, HierarchyOccurrence* out,
                                const std::string& path, ContractError* error)
{
    static const char* const names[] = {"authored_id", "revision", "child_authored_id",
                                        "parent_assembly_authored_id", "transform"};
    if (!validate_object(value, names, 5U, path, error))
        return false;
    {
        const auto member = value.FindMember("authored_id");
        if (member == value.MemberEnd())
            return fail(error, "geometer.contract.missing_field", child_path(path, "authored_id"),
                        "Required field is missing.");
        if (!decode_string(member->value, &out->authored_id, child_path(path, "authored_id"), error,
                           28U, 128U))
            return false;
    }
    {
        const auto member = value.FindMember("revision");
        if (member == value.MemberEnd())
            return fail(error, "geometer.contract.missing_field", child_path(path, "revision"),
                        "Required field is missing.");
        if (!decode_uint32(member->value, &out->revision, child_path(path, "revision"), error, 1ULL,
                           std::numeric_limits<std::uint32_t>::max()))
            return false;
    }
    {
        const auto member = value.FindMember("child_authored_id");
        if (member == value.MemberEnd())
            return fail(error, "geometer.contract.missing_field",
                        child_path(path, "child_authored_id"), "Required field is missing.");
        if (!decode_string(member->value, &out->child_authored_id,
                           child_path(path, "child_authored_id"), error, 28U, 128U))
            return false;
    }
    {
        const auto member = value.FindMember("parent_assembly_authored_id");
        if (member == value.MemberEnd())
            return fail(error, "geometer.contract.missing_field",
                        child_path(path, "parent_assembly_authored_id"),
                        "Required field is missing.");
        if (!decode_string(member->value, &out->parent_assembly_authored_id,
                           child_path(path, "parent_assembly_authored_id"), error, 28U, 128U))
            return false;
    }
    {
        const auto member = value.FindMember("transform");
        if (member == value.MemberEnd())
            return fail(error, "geometer.contract.missing_field", child_path(path, "transform"),
                        "Required field is missing.");
        if (!decode_array(member->value, &out->transform, child_path(path, "transform"), error, 12U,
                          12U, decode_double_item))
            return false;
    }
    return true;
}

bool write_HierarchyOccurrence(rapidjson::Writer<rapidjson::StringBuffer>& writer,
                               const HierarchyOccurrence& value, ContractError* error)
{
    writer.StartObject();
    writer.Key("authored_id");
    if (!write_string(writer, value.authored_id, error, 28U, 128U))
        return false;
    writer.Key("revision");
    if (!write_uint32(writer, value.revision, error, 1ULL,
                      std::numeric_limits<std::uint32_t>::max()))
        return false;
    writer.Key("child_authored_id");
    if (!write_string(writer, value.child_authored_id, error, 28U, 128U))
        return false;
    writer.Key("parent_assembly_authored_id");
    if (!write_string(writer, value.parent_assembly_authored_id, error, 28U, 128U))
        return false;
    writer.Key("transform");
    if (!write_array(writer, value.transform, error, 12U, 12U, write_double_item))
        return false;
    writer.EndObject();
    return true;
}

bool decode_HierarchyState(const rapidjson::Value& value, HierarchyState* out,
                           const std::string& path, ContractError* error)
{
    static const char* const names[] = {"hierarchy_revision", "source_brep_sha256", "nodes",
                                        "occurrences"};
    if (!validate_object(value, names, 4U, path, error))
        return false;
    {
        const auto member = value.FindMember("hierarchy_revision");
        if (member == value.MemberEnd())
            return fail(error, "geometer.contract.missing_field",
                        child_path(path, "hierarchy_revision"), "Required field is missing.");
        if (!decode_uint32(member->value, &out->hierarchy_revision,
                           child_path(path, "hierarchy_revision"), error, 0ULL,
                           std::numeric_limits<std::uint32_t>::max()))
            return false;
    }
    {
        const auto member = value.FindMember("source_brep_sha256");
        if (member == value.MemberEnd())
            return fail(error, "geometer.contract.missing_field",
                        child_path(path, "source_brep_sha256"), "Required field is missing.");
        if (!decode_string(member->value, &out->source_brep_sha256,
                           child_path(path, "source_brep_sha256"), error, 64U, 64U))
            return false;
    }
    {
        const auto member = value.FindMember("nodes");
        if (member == value.MemberEnd())
            return fail(error, "geometer.contract.missing_field", child_path(path, "nodes"),
                        "Required field is missing.");
        if (!decode_array(member->value, &out->nodes, child_path(path, "nodes"), error, 0U, 10000U,
                          decode_HierarchyNode))
            return false;
    }
    {
        const auto member = value.FindMember("occurrences");
        if (member == value.MemberEnd())
            return fail(error, "geometer.contract.missing_field", child_path(path, "occurrences"),
                        "Required field is missing.");
        if (!decode_array(member->value, &out->occurrences, child_path(path, "occurrences"), error,
                          0U, 100000U, decode_HierarchyOccurrence))
            return false;
    }
    return true;
}

bool write_HierarchyState(rapidjson::Writer<rapidjson::StringBuffer>& writer,
                          const HierarchyState& value, ContractError* error)
{
    writer.StartObject();
    writer.Key("hierarchy_revision");
    if (!write_uint32(writer, value.hierarchy_revision, error, 0ULL,
                      std::numeric_limits<std::uint32_t>::max()))
        return false;
    writer.Key("source_brep_sha256");
    if (!write_string(writer, value.source_brep_sha256, error, 64U, 64U))
        return false;
    writer.Key("nodes");
    if (!write_array(writer, value.nodes, error, 0U, 10000U, write_HierarchyNode))
        return false;
    writer.Key("occurrences");
    if (!write_array(writer, value.occurrences, error, 0U, 100000U, write_HierarchyOccurrence))
        return false;
    writer.EndObject();
    return true;
}

bool decode_StepTopologyApplyHierarchyResultA0(const rapidjson::Value& value,
                                               StepTopologyApplyHierarchyResultA0* out,
                                               const std::string& path, ContractError* error)
{
    static const char* const names[] = {"schema", "state", "hierarchy", "diagnostics"};
    if (!validate_object(value, names, 4U, path, error))
        return false;
    {
        const auto member = value.FindMember("schema");
        if (member == value.MemberEnd())
            return fail(error, "geometer.contract.missing_field", child_path(path, "schema"),
                        "Required field is missing.");
        if (!decode_literal_string(member->value, &out->schema, child_path(path, "schema"), error,
                                   "geometry.step_topology.apply_hierarchy.result.a0"))
            return false;
    }
    {
        const auto member = value.FindMember("state");
        if (member == value.MemberEnd())
            return fail(error, "geometer.contract.missing_field", child_path(path, "state"),
                        "Required field is missing.");
        if (!decode_MutationSessionState(member->value, &out->state, child_path(path, "state"),
                                         error))
            return false;
    }
    {
        const auto member = value.FindMember("hierarchy");
        if (member == value.MemberEnd())
            return fail(error, "geometer.contract.missing_field", child_path(path, "hierarchy"),
                        "Required field is missing.");
        if (!decode_HierarchyState(member->value, &out->hierarchy, child_path(path, "hierarchy"),
                                   error))
            return false;
    }
    {
        const auto member = value.FindMember("diagnostics");
        if (member == value.MemberEnd())
            return fail(error, "geometer.contract.missing_field", child_path(path, "diagnostics"),
                        "Required field is missing.");
        if (!decode_array(member->value, &out->diagnostics, child_path(path, "diagnostics"), error,
                          0U, 256U, decode_DiagnosticA0))
            return false;
    }
    return true;
}

bool write_StepTopologyApplyHierarchyResultA0(rapidjson::Writer<rapidjson::StringBuffer>& writer,
                                              const StepTopologyApplyHierarchyResultA0& value,
                                              ContractError* error)
{
    writer.StartObject();
    writer.Key("schema");
    if (!write_literal_string(writer, value.schema, error,
                              "geometry.step_topology.apply_hierarchy.result.a0"))
        return false;
    writer.Key("state");
    if (!write_MutationSessionState(writer, value.state, error))
        return false;
    writer.Key("hierarchy");
    if (!write_HierarchyState(writer, value.hierarchy, error))
        return false;
    writer.Key("diagnostics");
    if (!write_array(writer, value.diagnostics, error, 0U, 256U, write_DiagnosticA0))
        return false;
    writer.EndObject();
    return true;
}

bool decode_SavePersistenceArtifact(const rapidjson::Value& value, SavePersistenceArtifact* out,
                                    const std::string& path, ContractError* error)
{
    int matches = 0;
    SavePersistenceArtifact selected{};
    {
        XbfPersistenceArtifact candidate{};
        ContractError ignored;
        if (decode_XbfPersistenceArtifact(value, &candidate, path, &ignored))
        {
            ++matches;
            selected = SavePersistenceArtifact(std::in_place_index<0>, std::move(candidate));
        }
    }
    {
        XmlXcafPersistenceArtifact candidate{};
        ContractError ignored;
        if (decode_XmlXcafPersistenceArtifact(value, &candidate, path, &ignored))
        {
            ++matches;
            selected = SavePersistenceArtifact(std::in_place_index<1>, std::move(candidate));
        }
    }
    {
        StepAp242PersistenceArtifact candidate{};
        ContractError ignored;
        if (decode_StepAp242PersistenceArtifact(value, &candidate, path, &ignored))
        {
            ++matches;
            selected = SavePersistenceArtifact(std::in_place_index<2>, std::move(candidate));
        }
    }
    {
        JsonSidecarPersistenceArtifact candidate{};
        ContractError ignored;
        if (decode_JsonSidecarPersistenceArtifact(value, &candidate, path, &ignored))
        {
            ++matches;
            selected = SavePersistenceArtifact(std::in_place_index<3>, std::move(candidate));
        }
    }
    if (matches != 1)
        return fail(error, "geometer.contract.union_mismatch", path,
                    "Expected exactly one union variant.");
    *out = std::move(selected);
    return true;
}

bool write_SavePersistenceArtifact(rapidjson::Writer<rapidjson::StringBuffer>& writer,
                                   const SavePersistenceArtifact& value, ContractError* error)
{
    switch (value.index())
    {
    case 0:
        return write_XbfPersistenceArtifact(writer, std::get<0>(value), error);
    case 1:
        return write_XmlXcafPersistenceArtifact(writer, std::get<1>(value), error);
    case 2:
        return write_StepAp242PersistenceArtifact(writer, std::get<2>(value), error);
    case 3:
        return write_JsonSidecarPersistenceArtifact(writer, std::get<3>(value), error);
    default:
        return fail(error, "geometer.contract.union_mismatch", "", "Unknown union variant.");
    }
}

bool decode_PersistenceCarrier(const rapidjson::Value& value, PersistenceCarrier* out,
                               const std::string& path, ContractError* error)
{
    if (!value.IsString())
        return fail(error, "geometer.contract.type_mismatch", path, "Expected a string enum.");
    const std::string text(value.GetString(), value.GetStringLength());
    if (text == "xbf")
    {
        *out = PersistenceCarrier::xbf;
        return true;
    }
    if (text == "xml_xcaf")
    {
        *out = PersistenceCarrier::xml_xcaf;
        return true;
    }
    if (text == "step_ap242")
    {
        *out = PersistenceCarrier::step_ap242;
        return true;
    }
    if (text == "json_sidecar")
    {
        *out = PersistenceCarrier::json_sidecar;
        return true;
    }
    if (text == "edit_journal")
    {
        *out = PersistenceCarrier::edit_journal;
        return true;
    }
    return fail(error, "geometer.contract.enum_mismatch", path, "Unknown enum value.");
}

bool write_PersistenceCarrier(rapidjson::Writer<rapidjson::StringBuffer>& writer,
                              const PersistenceCarrier& value, ContractError* error)
{
    switch (value)
    {
    case PersistenceCarrier::xbf:
        writer.String("xbf");
        return true;
    case PersistenceCarrier::xml_xcaf:
        writer.String("xml_xcaf");
        return true;
    case PersistenceCarrier::step_ap242:
        writer.String("step_ap242");
        return true;
    case PersistenceCarrier::json_sidecar:
        writer.String("json_sidecar");
        return true;
    case PersistenceCarrier::edit_journal:
        writer.String("edit_journal");
        return true;
    }
    return fail(error, "geometer.contract.enum_mismatch", "", "Unknown enum value.");
}

bool decode_CarrierSupportState(const rapidjson::Value& value, CarrierSupportState* out,
                                const std::string& path, ContractError* error)
{
    if (!value.IsString())
        return fail(error, "geometer.contract.type_mismatch", path, "Expected a string enum.");
    const std::string text(value.GetString(), value.GetStringLength());
    if (text == "supported")
    {
        *out = CarrierSupportState::supported;
        return true;
    }
    if (text == "experimental")
    {
        *out = CarrierSupportState::experimental;
        return true;
    }
    if (text == "unsupported")
    {
        *out = CarrierSupportState::unsupported;
        return true;
    }
    return fail(error, "geometer.contract.enum_mismatch", path, "Unknown enum value.");
}

bool write_CarrierSupportState(rapidjson::Writer<rapidjson::StringBuffer>& writer,
                               const CarrierSupportState& value, ContractError* error)
{
    switch (value)
    {
    case CarrierSupportState::supported:
        writer.String("supported");
        return true;
    case CarrierSupportState::experimental:
        writer.String("experimental");
        return true;
    case CarrierSupportState::unsupported:
        writer.String("unsupported");
        return true;
    }
    return fail(error, "geometer.contract.enum_mismatch", "", "Unknown enum value.");
}

bool decode_CarrierCapabilityNote(const rapidjson::Value& value, CarrierCapabilityNote* out,
                                  const std::string& path, ContractError* error)
{
    static const char* const names[] = {"value"};
    if (!validate_object(value, names, 1U, path, error))
        return false;
    {
        const auto member = value.FindMember("value");
        if (member == value.MemberEnd())
            return fail(error, "geometer.contract.missing_field", child_path(path, "value"),
                        "Required field is missing.");
        if (!decode_string(member->value, &out->value, child_path(path, "value"), error, 1U, 256U))
            return false;
    }
    return true;
}

bool write_CarrierCapabilityNote(rapidjson::Writer<rapidjson::StringBuffer>& writer,
                                 const CarrierCapabilityNote& value, ContractError* error)
{
    writer.StartObject();
    writer.Key("value");
    if (!write_string(writer, value.value, error, 1U, 256U))
        return false;
    writer.EndObject();
    return true;
}

bool decode_CarrierCapability(const rapidjson::Value& value, CarrierCapability* out,
                              const std::string& path, ContractError* error)
{
    static const char* const names[] = {"carrier",          "save",           "restore",
                                        "authored_payload", "topology_links", "notes"};
    if (!validate_object(value, names, 6U, path, error))
        return false;
    {
        const auto member = value.FindMember("carrier");
        if (member == value.MemberEnd())
            return fail(error, "geometer.contract.missing_field", child_path(path, "carrier"),
                        "Required field is missing.");
        if (!decode_PersistenceCarrier(member->value, &out->carrier, child_path(path, "carrier"),
                                       error))
            return false;
    }
    {
        const auto member = value.FindMember("save");
        if (member == value.MemberEnd())
            return fail(error, "geometer.contract.missing_field", child_path(path, "save"),
                        "Required field is missing.");
        if (!decode_CarrierSupportState(member->value, &out->save, child_path(path, "save"), error))
            return false;
    }
    {
        const auto member = value.FindMember("restore");
        if (member == value.MemberEnd())
            return fail(error, "geometer.contract.missing_field", child_path(path, "restore"),
                        "Required field is missing.");
        if (!decode_CarrierSupportState(member->value, &out->restore, child_path(path, "restore"),
                                        error))
            return false;
    }
    {
        const auto member = value.FindMember("authored_payload");
        if (member == value.MemberEnd())
            return fail(error, "geometer.contract.missing_field",
                        child_path(path, "authored_payload"), "Required field is missing.");
        if (!decode_CarrierSupportState(member->value, &out->authored_payload,
                                        child_path(path, "authored_payload"), error))
            return false;
    }
    {
        const auto member = value.FindMember("topology_links");
        if (member == value.MemberEnd())
            return fail(error, "geometer.contract.missing_field",
                        child_path(path, "topology_links"), "Required field is missing.");
        if (!decode_CarrierSupportState(member->value, &out->topology_links,
                                        child_path(path, "topology_links"), error))
            return false;
    }
    {
        const auto member = value.FindMember("notes");
        if (member == value.MemberEnd())
            return fail(error, "geometer.contract.missing_field", child_path(path, "notes"),
                        "Required field is missing.");
        if (!decode_array(member->value, &out->notes, child_path(path, "notes"), error, 0U, 16U,
                          decode_CarrierCapabilityNote))
            return false;
    }
    return true;
}

bool write_CarrierCapability(rapidjson::Writer<rapidjson::StringBuffer>& writer,
                             const CarrierCapability& value, ContractError* error)
{
    writer.StartObject();
    writer.Key("carrier");
    if (!write_PersistenceCarrier(writer, value.carrier, error))
        return false;
    writer.Key("save");
    if (!write_CarrierSupportState(writer, value.save, error))
        return false;
    writer.Key("restore");
    if (!write_CarrierSupportState(writer, value.restore, error))
        return false;
    writer.Key("authored_payload");
    if (!write_CarrierSupportState(writer, value.authored_payload, error))
        return false;
    writer.Key("topology_links");
    if (!write_CarrierSupportState(writer, value.topology_links, error))
        return false;
    writer.Key("notes");
    if (!write_array(writer, value.notes, error, 0U, 16U, write_CarrierCapabilityNote))
        return false;
    writer.EndObject();
    return true;
}

bool decode_StepTopologySaveResultA0(const rapidjson::Value& value, StepTopologySaveResultA0* out,
                                     const std::string& path, ContractError* error)
{
    static const char* const names[] = {"schema",   "state",        "source_sha256",
                                        "artifact", "capabilities", "diagnostics"};
    if (!validate_object(value, names, 6U, path, error))
        return false;
    {
        const auto member = value.FindMember("schema");
        if (member == value.MemberEnd())
            return fail(error, "geometer.contract.missing_field", child_path(path, "schema"),
                        "Required field is missing.");
        if (!decode_literal_string(member->value, &out->schema, child_path(path, "schema"), error,
                                   "geometry.step_topology.save.result.a0"))
            return false;
    }
    {
        const auto member = value.FindMember("state");
        if (member == value.MemberEnd())
            return fail(error, "geometer.contract.missing_field", child_path(path, "state"),
                        "Required field is missing.");
        if (!decode_MutationSessionState(member->value, &out->state, child_path(path, "state"),
                                         error))
            return false;
    }
    {
        const auto member = value.FindMember("source_sha256");
        if (member == value.MemberEnd())
            return fail(error, "geometer.contract.missing_field", child_path(path, "source_sha256"),
                        "Required field is missing.");
        if (!decode_string(member->value, &out->source_sha256, child_path(path, "source_sha256"),
                           error, 64U, 64U))
            return false;
    }
    {
        const auto member = value.FindMember("artifact");
        if (member == value.MemberEnd())
            return fail(error, "geometer.contract.missing_field", child_path(path, "artifact"),
                        "Required field is missing.");
        if (!decode_SavePersistenceArtifact(member->value, &out->artifact,
                                            child_path(path, "artifact"), error))
            return false;
    }
    {
        const auto member = value.FindMember("capabilities");
        if (member == value.MemberEnd())
            return fail(error, "geometer.contract.missing_field", child_path(path, "capabilities"),
                        "Required field is missing.");
        if (!decode_array(member->value, &out->capabilities, child_path(path, "capabilities"),
                          error, 5U, 5U, decode_CarrierCapability))
            return false;
    }
    {
        const auto member = value.FindMember("diagnostics");
        if (member == value.MemberEnd())
            return fail(error, "geometer.contract.missing_field", child_path(path, "diagnostics"),
                        "Required field is missing.");
        if (!decode_array(member->value, &out->diagnostics, child_path(path, "diagnostics"), error,
                          0U, 256U, decode_DiagnosticA0))
            return false;
    }
    return true;
}

bool write_StepTopologySaveResultA0(rapidjson::Writer<rapidjson::StringBuffer>& writer,
                                    const StepTopologySaveResultA0& value, ContractError* error)
{
    writer.StartObject();
    writer.Key("schema");
    if (!write_literal_string(writer, value.schema, error, "geometry.step_topology.save.result.a0"))
        return false;
    writer.Key("state");
    if (!write_MutationSessionState(writer, value.state, error))
        return false;
    writer.Key("source_sha256");
    if (!write_string(writer, value.source_sha256, error, 64U, 64U))
        return false;
    writer.Key("artifact");
    if (!write_SavePersistenceArtifact(writer, value.artifact, error))
        return false;
    writer.Key("capabilities");
    if (!write_array(writer, value.capabilities, error, 5U, 5U, write_CarrierCapability))
        return false;
    writer.Key("diagnostics");
    if (!write_array(writer, value.diagnostics, error, 0U, 256U, write_DiagnosticA0))
        return false;
    writer.EndObject();
    return true;
}

bool decode_RecoveryResolutionState(const rapidjson::Value& value, RecoveryResolutionState* out,
                                    const std::string& path, ContractError* error)
{
    if (!value.IsString())
        return fail(error, "geometer.contract.type_mismatch", path, "Expected a string enum.");
    const std::string text(value.GetString(), value.GetStringLength());
    if (text == "resolved")
    {
        *out = RecoveryResolutionState::resolved;
        return true;
    }
    if (text == "ambiguous")
    {
        *out = RecoveryResolutionState::ambiguous;
        return true;
    }
    if (text == "unresolved")
    {
        *out = RecoveryResolutionState::unresolved;
        return true;
    }
    if (text == "unsupported")
    {
        *out = RecoveryResolutionState::unsupported;
        return true;
    }
    return fail(error, "geometer.contract.enum_mismatch", path, "Unknown enum value.");
}

bool write_RecoveryResolutionState(rapidjson::Writer<rapidjson::StringBuffer>& writer,
                                   const RecoveryResolutionState& value, ContractError* error)
{
    switch (value)
    {
    case RecoveryResolutionState::resolved:
        writer.String("resolved");
        return true;
    case RecoveryResolutionState::ambiguous:
        writer.String("ambiguous");
        return true;
    case RecoveryResolutionState::unresolved:
        writer.String("unresolved");
        return true;
    case RecoveryResolutionState::unsupported:
        writer.String("unsupported");
        return true;
    }
    return fail(error, "geometer.contract.enum_mismatch", "", "Unknown enum value.");
}

bool decode_RecoveryGroupCompleteness(const rapidjson::Value& value, RecoveryGroupCompleteness* out,
                                      const std::string& path, ContractError* error)
{
    if (!value.IsString())
        return fail(error, "geometer.contract.type_mismatch", path, "Expected a string enum.");
    const std::string text(value.GetString(), value.GetStringLength());
    if (text == "fully_recovered")
    {
        *out = RecoveryGroupCompleteness::fully_recovered;
        return true;
    }
    if (text == "partially_recovered")
    {
        *out = RecoveryGroupCompleteness::partially_recovered;
        return true;
    }
    if (text == "unrecovered")
    {
        *out = RecoveryGroupCompleteness::unrecovered;
        return true;
    }
    if (text == "unsupported")
    {
        *out = RecoveryGroupCompleteness::unsupported;
        return true;
    }
    return fail(error, "geometer.contract.enum_mismatch", path, "Unknown enum value.");
}

bool write_RecoveryGroupCompleteness(rapidjson::Writer<rapidjson::StringBuffer>& writer,
                                     const RecoveryGroupCompleteness& value, ContractError* error)
{
    switch (value)
    {
    case RecoveryGroupCompleteness::fully_recovered:
        writer.String("fully_recovered");
        return true;
    case RecoveryGroupCompleteness::partially_recovered:
        writer.String("partially_recovered");
        return true;
    case RecoveryGroupCompleteness::unrecovered:
        writer.String("unrecovered");
        return true;
    case RecoveryGroupCompleteness::unsupported:
        writer.String("unsupported");
        return true;
    }
    return fail(error, "geometer.contract.enum_mismatch", "", "Unknown enum value.");
}

bool decode_RecoveryResolutionMethod(const rapidjson::Value& value, RecoveryResolutionMethod* out,
                                     const std::string& path, ContractError* error)
{
    if (!value.IsString())
        return fail(error, "geometer.contract.type_mismatch", path, "Expected a string enum.");
    const std::string text(value.GetString(), value.GetStringLength());
    if (text == "authored_id_topology_link")
    {
        *out = RecoveryResolutionMethod::authored_id_topology_link;
        return true;
    }
    if (text == "validated_carrier_locator")
    {
        *out = RecoveryResolutionMethod::validated_carrier_locator;
        return true;
    }
    if (text == "unique_geometry_adjacency_fingerprint")
    {
        *out = RecoveryResolutionMethod::unique_geometry_adjacency_fingerprint;
        return true;
    }
    if (text == "none")
    {
        *out = RecoveryResolutionMethod::none;
        return true;
    }
    return fail(error, "geometer.contract.enum_mismatch", path, "Unknown enum value.");
}

bool write_RecoveryResolutionMethod(rapidjson::Writer<rapidjson::StringBuffer>& writer,
                                    const RecoveryResolutionMethod& value, ContractError* error)
{
    switch (value)
    {
    case RecoveryResolutionMethod::authored_id_topology_link:
        writer.String("authored_id_topology_link");
        return true;
    case RecoveryResolutionMethod::validated_carrier_locator:
        writer.String("validated_carrier_locator");
        return true;
    case RecoveryResolutionMethod::unique_geometry_adjacency_fingerprint:
        writer.String("unique_geometry_adjacency_fingerprint");
        return true;
    case RecoveryResolutionMethod::none:
        writer.String("none");
        return true;
    }
    return fail(error, "geometer.contract.enum_mismatch", "", "Unknown enum value.");
}

bool decode_RecoveryTopologyComparison(const rapidjson::Value& value,
                                       RecoveryTopologyComparison* out, const std::string& path,
                                       ContractError* error)
{
    if (!value.IsString())
        return fail(error, "geometer.contract.type_mismatch", path, "Expected a string enum.");
    const std::string text(value.GetString(), value.GetStringLength());
    if (text == "unchanged")
    {
        *out = RecoveryTopologyComparison::unchanged;
        return true;
    }
    if (text == "relocated")
    {
        *out = RecoveryTopologyComparison::relocated;
        return true;
    }
    if (text == "split")
    {
        *out = RecoveryTopologyComparison::split;
        return true;
    }
    if (text == "merged")
    {
        *out = RecoveryTopologyComparison::merged;
        return true;
    }
    if (text == "otherwise_changed")
    {
        *out = RecoveryTopologyComparison::otherwise_changed;
        return true;
    }
    if (text == "not_compared")
    {
        *out = RecoveryTopologyComparison::not_compared;
        return true;
    }
    if (text == "unavailable")
    {
        *out = RecoveryTopologyComparison::unavailable;
        return true;
    }
    return fail(error, "geometer.contract.enum_mismatch", path, "Unknown enum value.");
}

bool write_RecoveryTopologyComparison(rapidjson::Writer<rapidjson::StringBuffer>& writer,
                                      const RecoveryTopologyComparison& value, ContractError* error)
{
    switch (value)
    {
    case RecoveryTopologyComparison::unchanged:
        writer.String("unchanged");
        return true;
    case RecoveryTopologyComparison::relocated:
        writer.String("relocated");
        return true;
    case RecoveryTopologyComparison::split:
        writer.String("split");
        return true;
    case RecoveryTopologyComparison::merged:
        writer.String("merged");
        return true;
    case RecoveryTopologyComparison::otherwise_changed:
        writer.String("otherwise_changed");
        return true;
    case RecoveryTopologyComparison::not_compared:
        writer.String("not_compared");
        return true;
    case RecoveryTopologyComparison::unavailable:
        writer.String("unavailable");
        return true;
    }
    return fail(error, "geometer.contract.enum_mismatch", "", "Unknown enum value.");
}

bool decode_RecoveryConfidence(const rapidjson::Value& value, RecoveryConfidence* out,
                               const std::string& path, ContractError* error)
{
    if (!value.IsString())
        return fail(error, "geometer.contract.type_mismatch", path, "Expected a string enum.");
    const std::string text(value.GetString(), value.GetStringLength());
    if (text == "high")
    {
        *out = RecoveryConfidence::high;
        return true;
    }
    if (text == "medium")
    {
        *out = RecoveryConfidence::medium;
        return true;
    }
    if (text == "low")
    {
        *out = RecoveryConfidence::low;
        return true;
    }
    if (text == "none")
    {
        *out = RecoveryConfidence::none;
        return true;
    }
    return fail(error, "geometer.contract.enum_mismatch", path, "Unknown enum value.");
}

bool write_RecoveryConfidence(rapidjson::Writer<rapidjson::StringBuffer>& writer,
                              const RecoveryConfidence& value, ContractError* error)
{
    switch (value)
    {
    case RecoveryConfidence::high:
        writer.String("high");
        return true;
    case RecoveryConfidence::medium:
        writer.String("medium");
        return true;
    case RecoveryConfidence::low:
        writer.String("low");
        return true;
    case RecoveryConfidence::none:
        writer.String("none");
        return true;
    }
    return fail(error, "geometer.contract.enum_mismatch", "", "Unknown enum value.");
}

bool decode_RecoveryComparedField(const rapidjson::Value& value, RecoveryComparedField* out,
                                  const std::string& path, ContractError* error)
{
    static const char* const names[] = {"value"};
    if (!validate_object(value, names, 1U, path, error))
        return false;
    {
        const auto member = value.FindMember("value");
        if (member == value.MemberEnd())
            return fail(error, "geometer.contract.missing_field", child_path(path, "value"),
                        "Required field is missing.");
        if (!decode_string(member->value, &out->value, child_path(path, "value"), error, 1U, 128U))
            return false;
    }
    return true;
}

bool write_RecoveryComparedField(rapidjson::Writer<rapidjson::StringBuffer>& writer,
                                 const RecoveryComparedField& value, ContractError* error)
{
    writer.StartObject();
    writer.Key("value");
    if (!write_string(writer, value.value, error, 1U, 128U))
        return false;
    writer.EndObject();
    return true;
}

bool decode_RecoveryCarrierRecord(const rapidjson::Value& value, RecoveryCarrierRecord* out,
                                  const std::string& path, ContractError* error)
{
    static const char* const names[] = {"value"};
    if (!validate_object(value, names, 1U, path, error))
        return false;
    {
        const auto member = value.FindMember("value");
        if (member == value.MemberEnd())
            return fail(error, "geometer.contract.missing_field", child_path(path, "value"),
                        "Required field is missing.");
        if (!decode_string(member->value, &out->value, child_path(path, "value"), error, 1U, 4096U))
            return false;
    }
    return true;
}

bool write_RecoveryCarrierRecord(rapidjson::Writer<rapidjson::StringBuffer>& writer,
                                 const RecoveryCarrierRecord& value, ContractError* error)
{
    writer.StartObject();
    writer.Key("value");
    if (!write_string(writer, value.value, error, 1U, 4096U))
        return false;
    writer.EndObject();
    return true;
}

bool decode_RecoveryRejectedAlternative(const rapidjson::Value& value,
                                        RecoveryRejectedAlternative* out, const std::string& path,
                                        ContractError* error)
{
    static const char* const names[] = {"target_handle", "reason"};
    if (!validate_object(value, names, 2U, path, error))
        return false;
    {
        const auto member = value.FindMember("target_handle");
        if (member == value.MemberEnd())
            return fail(error, "geometer.contract.missing_field", child_path(path, "target_handle"),
                        "Required field is missing.");
        if (!decode_string(member->value, &out->target_handle, child_path(path, "target_handle"),
                           error, 68U, 68U))
            return false;
    }
    {
        const auto member = value.FindMember("reason");
        if (member == value.MemberEnd())
            return fail(error, "geometer.contract.missing_field", child_path(path, "reason"),
                        "Required field is missing.");
        if (!decode_string(member->value, &out->reason, child_path(path, "reason"), error, 1U,
                           4096U))
            return false;
    }
    return true;
}

bool write_RecoveryRejectedAlternative(rapidjson::Writer<rapidjson::StringBuffer>& writer,
                                       const RecoveryRejectedAlternative& value,
                                       ContractError* error)
{
    writer.StartObject();
    writer.Key("target_handle");
    if (!write_string(writer, value.target_handle, error, 68U, 68U))
        return false;
    writer.Key("reason");
    if (!write_string(writer, value.reason, error, 1U, 4096U))
        return false;
    writer.EndObject();
    return true;
}

bool decode_RecoveryEvidence(const rapidjson::Value& value, RecoveryEvidence* out,
                             const std::string& path, ContractError* error)
{
    static const char* const names[] = {"candidate_count", "matching_candidate_count",
                                        "compared_fields", "tolerances",
                                        "carrier_records", "rejected_alternatives"};
    if (!validate_object(value, names, 6U, path, error))
        return false;
    {
        const auto member = value.FindMember("candidate_count");
        if (member == value.MemberEnd())
            return fail(error, "geometer.contract.missing_field",
                        child_path(path, "candidate_count"), "Required field is missing.");
        if (!decode_uint32(member->value, &out->candidate_count,
                           child_path(path, "candidate_count"), error, 0ULL, 16ULL))
            return false;
    }
    {
        const auto member = value.FindMember("matching_candidate_count");
        if (member == value.MemberEnd())
            return fail(error, "geometer.contract.missing_field",
                        child_path(path, "matching_candidate_count"), "Required field is missing.");
        if (!decode_uint32(member->value, &out->matching_candidate_count,
                           child_path(path, "matching_candidate_count"), error, 0ULL, 16ULL))
            return false;
    }
    {
        const auto member = value.FindMember("compared_fields");
        if (member == value.MemberEnd())
            return fail(error, "geometer.contract.missing_field",
                        child_path(path, "compared_fields"), "Required field is missing.");
        if (!decode_array(member->value, &out->compared_fields, child_path(path, "compared_fields"),
                          error, 0U, 16U, decode_RecoveryComparedField))
            return false;
    }
    {
        const auto member = value.FindMember("tolerances");
        if (member == value.MemberEnd())
            return fail(error, "geometer.contract.missing_field", child_path(path, "tolerances"),
                        "Required field is missing.");
        if (!decode_RecoveryTolerances(member->value, &out->tolerances,
                                       child_path(path, "tolerances"), error))
            return false;
    }
    {
        const auto member = value.FindMember("carrier_records");
        if (member == value.MemberEnd())
            return fail(error, "geometer.contract.missing_field",
                        child_path(path, "carrier_records"), "Required field is missing.");
        if (!decode_array(member->value, &out->carrier_records, child_path(path, "carrier_records"),
                          error, 0U, 16U, decode_RecoveryCarrierRecord))
            return false;
    }
    {
        const auto member = value.FindMember("rejected_alternatives");
        if (member == value.MemberEnd())
            return fail(error, "geometer.contract.missing_field",
                        child_path(path, "rejected_alternatives"), "Required field is missing.");
        if (!decode_array(member->value, &out->rejected_alternatives,
                          child_path(path, "rejected_alternatives"), error, 0U, 16U,
                          decode_RecoveryRejectedAlternative))
            return false;
    }
    return true;
}

bool write_RecoveryEvidence(rapidjson::Writer<rapidjson::StringBuffer>& writer,
                            const RecoveryEvidence& value, ContractError* error)
{
    writer.StartObject();
    writer.Key("candidate_count");
    if (!write_uint32(writer, value.candidate_count, error, 0ULL, 16ULL))
        return false;
    writer.Key("matching_candidate_count");
    if (!write_uint32(writer, value.matching_candidate_count, error, 0ULL, 16ULL))
        return false;
    writer.Key("compared_fields");
    if (!write_array(writer, value.compared_fields, error, 0U, 16U, write_RecoveryComparedField))
        return false;
    writer.Key("tolerances");
    if (!write_RecoveryTolerances(writer, value.tolerances, error))
        return false;
    writer.Key("carrier_records");
    if (!write_array(writer, value.carrier_records, error, 0U, 16U, write_RecoveryCarrierRecord))
        return false;
    writer.Key("rejected_alternatives");
    if (!write_array(writer, value.rejected_alternatives, error, 0U, 16U,
                     write_RecoveryRejectedAlternative))
        return false;
    writer.EndObject();
    return true;
}

bool decode_RecoveryMemberResult(const rapidjson::Value& value, RecoveryMemberResult* out,
                                 const std::string& path, ContractError* error)
{
    static const char* const names[] = {"member_record_id",
                                        "kind",
                                        "authored_target_id",
                                        "resolution_state",
                                        "resolution_method",
                                        "topology_comparison",
                                        "confidence",
                                        "resolved_target_handle",
                                        "evidence"};
    if (!validate_object(value, names, 9U, path, error))
        return false;
    {
        const auto member = value.FindMember("member_record_id");
        if (member == value.MemberEnd())
            return fail(error, "geometer.contract.missing_field",
                        child_path(path, "member_record_id"), "Required field is missing.");
        if (!decode_string(member->value, &out->member_record_id,
                           child_path(path, "member_record_id"), error, 28U, 128U))
            return false;
    }
    {
        const auto member = value.FindMember("kind");
        if (member == value.MemberEnd())
            return fail(error, "geometer.contract.missing_field", child_path(path, "kind"),
                        "Required field is missing.");
        if (!decode_LogicalGroupMemberKind(member->value, &out->kind, child_path(path, "kind"),
                                           error))
            return false;
    }
    {
        const auto member = value.FindMember("authored_target_id");
        if (member == value.MemberEnd())
            return fail(error, "geometer.contract.missing_field",
                        child_path(path, "authored_target_id"), "Required field is missing.");
        if (!decode_string(member->value, &out->authored_target_id,
                           child_path(path, "authored_target_id"), error, 0U, 128U))
            return false;
    }
    {
        const auto member = value.FindMember("resolution_state");
        if (member == value.MemberEnd())
            return fail(error, "geometer.contract.missing_field",
                        child_path(path, "resolution_state"), "Required field is missing.");
        if (!decode_RecoveryResolutionState(member->value, &out->resolution_state,
                                            child_path(path, "resolution_state"), error))
            return false;
    }
    {
        const auto member = value.FindMember("resolution_method");
        if (member == value.MemberEnd())
            return fail(error, "geometer.contract.missing_field",
                        child_path(path, "resolution_method"), "Required field is missing.");
        if (!decode_RecoveryResolutionMethod(member->value, &out->resolution_method,
                                             child_path(path, "resolution_method"), error))
            return false;
    }
    {
        const auto member = value.FindMember("topology_comparison");
        if (member == value.MemberEnd())
            return fail(error, "geometer.contract.missing_field",
                        child_path(path, "topology_comparison"), "Required field is missing.");
        if (!decode_RecoveryTopologyComparison(member->value, &out->topology_comparison,
                                               child_path(path, "topology_comparison"), error))
            return false;
    }
    {
        const auto member = value.FindMember("confidence");
        if (member == value.MemberEnd())
            return fail(error, "geometer.contract.missing_field", child_path(path, "confidence"),
                        "Required field is missing.");
        if (!decode_RecoveryConfidence(member->value, &out->confidence,
                                       child_path(path, "confidence"), error))
            return false;
    }
    {
        const auto member = value.FindMember("resolved_target_handle");
        if (member != value.MemberEnd())
        {
            std::string decoded{};
            if (!decode_string(member->value, &decoded, child_path(path, "resolved_target_handle"),
                               error, 68U, 68U))
                return false;
            out->resolved_target_handle = std::move(decoded);
        }
        else
            out->resolved_target_handle.reset();
    }
    {
        const auto member = value.FindMember("evidence");
        if (member == value.MemberEnd())
            return fail(error, "geometer.contract.missing_field", child_path(path, "evidence"),
                        "Required field is missing.");
        if (!decode_RecoveryEvidence(member->value, &out->evidence, child_path(path, "evidence"),
                                     error))
            return false;
    }
    return true;
}

bool write_RecoveryMemberResult(rapidjson::Writer<rapidjson::StringBuffer>& writer,
                                const RecoveryMemberResult& value, ContractError* error)
{
    writer.StartObject();
    writer.Key("member_record_id");
    if (!write_string(writer, value.member_record_id, error, 28U, 128U))
        return false;
    writer.Key("kind");
    if (!write_LogicalGroupMemberKind(writer, value.kind, error))
        return false;
    writer.Key("authored_target_id");
    if (!write_string(writer, value.authored_target_id, error, 0U, 128U))
        return false;
    writer.Key("resolution_state");
    if (!write_RecoveryResolutionState(writer, value.resolution_state, error))
        return false;
    writer.Key("resolution_method");
    if (!write_RecoveryResolutionMethod(writer, value.resolution_method, error))
        return false;
    writer.Key("topology_comparison");
    if (!write_RecoveryTopologyComparison(writer, value.topology_comparison, error))
        return false;
    writer.Key("confidence");
    if (!write_RecoveryConfidence(writer, value.confidence, error))
        return false;
    if (value.resolved_target_handle.has_value())
    {
        writer.Key("resolved_target_handle");
        if (!write_string(writer, *value.resolved_target_handle, error, 68U, 68U))
            return false;
    }
    writer.Key("evidence");
    if (!write_RecoveryEvidence(writer, value.evidence, error))
        return false;
    writer.EndObject();
    return true;
}

bool decode_RecoveryGroupResult(const rapidjson::Value& value, RecoveryGroupResult* out,
                                const std::string& path, ContractError* error)
{
    static const char* const names[] = {"group_authored_id",
                                        "provenance",
                                        "resolution_state",
                                        "completeness",
                                        "resolved_member_count",
                                        "ambiguous_member_count",
                                        "unresolved_member_count",
                                        "unsupported_member_count",
                                        "members"};
    if (!validate_object(value, names, 9U, path, error))
        return false;
    {
        const auto member = value.FindMember("group_authored_id");
        if (member == value.MemberEnd())
            return fail(error, "geometer.contract.missing_field",
                        child_path(path, "group_authored_id"), "Required field is missing.");
        if (!decode_string(member->value, &out->group_authored_id,
                           child_path(path, "group_authored_id"), error, 28U, 128U))
            return false;
    }
    {
        const auto member = value.FindMember("provenance");
        if (member == value.MemberEnd())
            return fail(error, "geometer.contract.missing_field", child_path(path, "provenance"),
                        "Required field is missing.");
        if (!decode_RecoveryProvenance(member->value, &out->provenance,
                                       child_path(path, "provenance"), error))
            return false;
    }
    {
        const auto member = value.FindMember("resolution_state");
        if (member == value.MemberEnd())
            return fail(error, "geometer.contract.missing_field",
                        child_path(path, "resolution_state"), "Required field is missing.");
        if (!decode_RecoveryResolutionState(member->value, &out->resolution_state,
                                            child_path(path, "resolution_state"), error))
            return false;
    }
    {
        const auto member = value.FindMember("completeness");
        if (member == value.MemberEnd())
            return fail(error, "geometer.contract.missing_field", child_path(path, "completeness"),
                        "Required field is missing.");
        if (!decode_RecoveryGroupCompleteness(member->value, &out->completeness,
                                              child_path(path, "completeness"), error))
            return false;
    }
    {
        const auto member = value.FindMember("resolved_member_count");
        if (member == value.MemberEnd())
            return fail(error, "geometer.contract.missing_field",
                        child_path(path, "resolved_member_count"), "Required field is missing.");
        if (!decode_uint32(member->value, &out->resolved_member_count,
                           child_path(path, "resolved_member_count"), error, 0ULL, 256ULL))
            return false;
    }
    {
        const auto member = value.FindMember("ambiguous_member_count");
        if (member == value.MemberEnd())
            return fail(error, "geometer.contract.missing_field",
                        child_path(path, "ambiguous_member_count"), "Required field is missing.");
        if (!decode_uint32(member->value, &out->ambiguous_member_count,
                           child_path(path, "ambiguous_member_count"), error, 0ULL, 256ULL))
            return false;
    }
    {
        const auto member = value.FindMember("unresolved_member_count");
        if (member == value.MemberEnd())
            return fail(error, "geometer.contract.missing_field",
                        child_path(path, "unresolved_member_count"), "Required field is missing.");
        if (!decode_uint32(member->value, &out->unresolved_member_count,
                           child_path(path, "unresolved_member_count"), error, 0ULL, 256ULL))
            return false;
    }
    {
        const auto member = value.FindMember("unsupported_member_count");
        if (member == value.MemberEnd())
            return fail(error, "geometer.contract.missing_field",
                        child_path(path, "unsupported_member_count"), "Required field is missing.");
        if (!decode_uint32(member->value, &out->unsupported_member_count,
                           child_path(path, "unsupported_member_count"), error, 0ULL, 256ULL))
            return false;
    }
    {
        const auto member = value.FindMember("members");
        if (member == value.MemberEnd())
            return fail(error, "geometer.contract.missing_field", child_path(path, "members"),
                        "Required field is missing.");
        if (!decode_array(member->value, &out->members, child_path(path, "members"), error, 1U,
                          256U, decode_RecoveryMemberResult))
            return false;
    }
    return true;
}

bool write_RecoveryGroupResult(rapidjson::Writer<rapidjson::StringBuffer>& writer,
                               const RecoveryGroupResult& value, ContractError* error)
{
    writer.StartObject();
    writer.Key("group_authored_id");
    if (!write_string(writer, value.group_authored_id, error, 28U, 128U))
        return false;
    writer.Key("provenance");
    if (!write_RecoveryProvenance(writer, value.provenance, error))
        return false;
    writer.Key("resolution_state");
    if (!write_RecoveryResolutionState(writer, value.resolution_state, error))
        return false;
    writer.Key("completeness");
    if (!write_RecoveryGroupCompleteness(writer, value.completeness, error))
        return false;
    writer.Key("resolved_member_count");
    if (!write_uint32(writer, value.resolved_member_count, error, 0ULL, 256ULL))
        return false;
    writer.Key("ambiguous_member_count");
    if (!write_uint32(writer, value.ambiguous_member_count, error, 0ULL, 256ULL))
        return false;
    writer.Key("unresolved_member_count");
    if (!write_uint32(writer, value.unresolved_member_count, error, 0ULL, 256ULL))
        return false;
    writer.Key("unsupported_member_count");
    if (!write_uint32(writer, value.unsupported_member_count, error, 0ULL, 256ULL))
        return false;
    writer.Key("members");
    if (!write_array(writer, value.members, error, 1U, 256U, write_RecoveryMemberResult))
        return false;
    writer.EndObject();
    return true;
}

bool decode_StepTopologyRestoreResultA0(const rapidjson::Value& value,
                                        StepTopologyRestoreResultA0* out, const std::string& path,
                                        ContractError* error)
{
    static const char* const names[] = {"schema",
                                        "session",
                                        "source",
                                        "tool",
                                        "replayed_transaction_count",
                                        "evicted_session_handles",
                                        "recovery",
                                        "diagnostics"};
    if (!validate_object(value, names, 8U, path, error))
        return false;
    {
        const auto member = value.FindMember("schema");
        if (member == value.MemberEnd())
            return fail(error, "geometer.contract.missing_field", child_path(path, "schema"),
                        "Required field is missing.");
        if (!decode_literal_string(member->value, &out->schema, child_path(path, "schema"), error,
                                   "geometry.step_topology.restore.result.a0"))
            return false;
    }
    {
        const auto member = value.FindMember("session");
        if (member == value.MemberEnd())
            return fail(error, "geometer.contract.missing_field", child_path(path, "session"),
                        "Required field is missing.");
        if (!decode_SessionReference(member->value, &out->session, child_path(path, "session"),
                                     error))
            return false;
    }
    {
        const auto member = value.FindMember("source");
        if (member == value.MemberEnd())
            return fail(error, "geometer.contract.missing_field", child_path(path, "source"),
                        "Required field is missing.");
        if (!decode_SourceDescriptor(member->value, &out->source, child_path(path, "source"),
                                     error))
            return false;
    }
    {
        const auto member = value.FindMember("tool");
        if (member == value.MemberEnd())
            return fail(error, "geometer.contract.missing_field", child_path(path, "tool"),
                        "Required field is missing.");
        if (!decode_ToolDescriptor(member->value, &out->tool, child_path(path, "tool"), error))
            return false;
    }
    {
        const auto member = value.FindMember("replayed_transaction_count");
        if (member == value.MemberEnd())
            return fail(error, "geometer.contract.missing_field",
                        child_path(path, "replayed_transaction_count"),
                        "Required field is missing.");
        if (!decode_uint32(member->value, &out->replayed_transaction_count,
                           child_path(path, "replayed_transaction_count"), error, 0ULL, 100000ULL))
            return false;
    }
    {
        const auto member = value.FindMember("evicted_session_handles");
        if (member != value.MemberEnd())
        {
            std::vector<std::string> decoded{};
            if (!decode_array(member->value, &decoded, child_path(path, "evicted_session_handles"),
                              error, 0U, 64U, decode_string_item))
                return false;
            out->evicted_session_handles = std::move(decoded);
        }
        else
            out->evicted_session_handles.reset();
    }
    {
        const auto member = value.FindMember("recovery");
        if (member == value.MemberEnd())
            return fail(error, "geometer.contract.missing_field", child_path(path, "recovery"),
                        "Required field is missing.");
        if (!decode_array(member->value, &out->recovery, child_path(path, "recovery"), error, 0U,
                          16U, decode_RecoveryGroupResult))
            return false;
    }
    {
        const auto member = value.FindMember("diagnostics");
        if (member == value.MemberEnd())
            return fail(error, "geometer.contract.missing_field", child_path(path, "diagnostics"),
                        "Required field is missing.");
        if (!decode_array(member->value, &out->diagnostics, child_path(path, "diagnostics"), error,
                          0U, 256U, decode_DiagnosticA0))
            return false;
    }
    return true;
}

bool write_StepTopologyRestoreResultA0(rapidjson::Writer<rapidjson::StringBuffer>& writer,
                                       const StepTopologyRestoreResultA0& value,
                                       ContractError* error)
{
    writer.StartObject();
    writer.Key("schema");
    if (!write_literal_string(writer, value.schema, error,
                              "geometry.step_topology.restore.result.a0"))
        return false;
    writer.Key("session");
    if (!write_SessionReference(writer, value.session, error))
        return false;
    writer.Key("source");
    if (!write_SourceDescriptor(writer, value.source, error))
        return false;
    writer.Key("tool");
    if (!write_ToolDescriptor(writer, value.tool, error))
        return false;
    writer.Key("replayed_transaction_count");
    if (!write_uint32(writer, value.replayed_transaction_count, error, 0ULL, 100000ULL))
        return false;
    if (value.evicted_session_handles.has_value())
    {
        writer.Key("evicted_session_handles");
        if (!write_array(writer, *value.evicted_session_handles, error, 0U, 64U, write_string_item))
            return false;
    }
    writer.Key("recovery");
    if (!write_array(writer, value.recovery, error, 0U, 16U, write_RecoveryGroupResult))
        return false;
    writer.Key("diagnostics");
    if (!write_array(writer, value.diagnostics, error, 0U, 256U, write_DiagnosticA0))
        return false;
    writer.EndObject();
    return true;
}

bool decode_StepTopologyAnalyzeRecoveryResultA0(const rapidjson::Value& value,
                                                StepTopologyAnalyzeRecoveryResultA0* out,
                                                const std::string& path, ContractError* error)
{
    static const char* const names[] = {"schema", "groups", "diagnostics"};
    if (!validate_object(value, names, 3U, path, error))
        return false;
    {
        const auto member = value.FindMember("schema");
        if (member == value.MemberEnd())
            return fail(error, "geometer.contract.missing_field", child_path(path, "schema"),
                        "Required field is missing.");
        if (!decode_literal_string(member->value, &out->schema, child_path(path, "schema"), error,
                                   "geometry.step_topology.analyze_recovery.result.a0"))
            return false;
    }
    {
        const auto member = value.FindMember("groups");
        if (member == value.MemberEnd())
            return fail(error, "geometer.contract.missing_field", child_path(path, "groups"),
                        "Required field is missing.");
        if (!decode_array(member->value, &out->groups, child_path(path, "groups"), error, 0U, 16U,
                          decode_RecoveryGroupResult))
            return false;
    }
    {
        const auto member = value.FindMember("diagnostics");
        if (member == value.MemberEnd())
            return fail(error, "geometer.contract.missing_field", child_path(path, "diagnostics"),
                        "Required field is missing.");
        if (!decode_array(member->value, &out->diagnostics, child_path(path, "diagnostics"), error,
                          0U, 256U, decode_DiagnosticA0))
            return false;
    }
    return true;
}

bool write_StepTopologyAnalyzeRecoveryResultA0(rapidjson::Writer<rapidjson::StringBuffer>& writer,
                                               const StepTopologyAnalyzeRecoveryResultA0& value,
                                               ContractError* error)
{
    writer.StartObject();
    writer.Key("schema");
    if (!write_literal_string(writer, value.schema, error,
                              "geometry.step_topology.analyze_recovery.result.a0"))
        return false;
    writer.Key("groups");
    if (!write_array(writer, value.groups, error, 0U, 16U, write_RecoveryGroupResult))
        return false;
    writer.Key("diagnostics");
    if (!write_array(writer, value.diagnostics, error, 0U, 256U, write_DiagnosticA0))
        return false;
    writer.EndObject();
    return true;
}

bool decode_OperationResultValueA0(const rapidjson::Value& value, OperationResultValueA0* out,
                                   const std::string& path, ContractError* error)
{
    int matches = 0;
    OperationResultValueA0 selected{};
    {
        ModelBoundsResultA0 candidate{};
        ContractError ignored;
        if (decode_ModelBoundsResultA0(value, &candidate, path, &ignored))
        {
            ++matches;
            selected = OperationResultValueA0(std::in_place_index<0>, std::move(candidate));
        }
    }
    {
        HlrProjectionResultA0 candidate{};
        ContractError ignored;
        if (decode_HlrProjectionResultA0(value, &candidate, path, &ignored))
        {
            ++matches;
            selected = OperationResultValueA0(std::in_place_index<1>, std::move(candidate));
        }
    }
    {
        PackedAttachmentProjectionA0 candidate{};
        ContractError ignored;
        if (decode_PackedAttachmentProjectionA0(value, &candidate, path, &ignored))
        {
            ++matches;
            selected = OperationResultValueA0(std::in_place_index<2>, std::move(candidate));
        }
    }
    {
        StepTopologyOpenResultA0 candidate{};
        ContractError ignored;
        if (decode_StepTopologyOpenResultA0(value, &candidate, path, &ignored))
        {
            ++matches;
            selected = OperationResultValueA0(std::in_place_index<3>, std::move(candidate));
        }
    }
    {
        StepTopologyCloseResultA0 candidate{};
        ContractError ignored;
        if (decode_StepTopologyCloseResultA0(value, &candidate, path, &ignored))
        {
            ++matches;
            selected = OperationResultValueA0(std::in_place_index<4>, std::move(candidate));
        }
    }
    {
        StepTopologyInspectResultA0 candidate{};
        ContractError ignored;
        if (decode_StepTopologyInspectResultA0(value, &candidate, path, &ignored))
        {
            ++matches;
            selected = OperationResultValueA0(std::in_place_index<5>, std::move(candidate));
        }
    }
    {
        StepTopologyRenderResultA0 candidate{};
        ContractError ignored;
        if (decode_StepTopologyRenderResultA0(value, &candidate, path, &ignored))
        {
            ++matches;
            selected = OperationResultValueA0(std::in_place_index<6>, std::move(candidate));
        }
    }
    {
        StepTopologyResolveHitResultA0 candidate{};
        ContractError ignored;
        if (decode_StepTopologyResolveHitResultA0(value, &candidate, path, &ignored))
        {
            ++matches;
            selected = OperationResultValueA0(std::in_place_index<7>, std::move(candidate));
        }
    }
    {
        StepTopologyApplyLogicalGroupsResultA0 candidate{};
        ContractError ignored;
        if (decode_StepTopologyApplyLogicalGroupsResultA0(value, &candidate, path, &ignored))
        {
            ++matches;
            selected = OperationResultValueA0(std::in_place_index<8>, std::move(candidate));
        }
    }
    {
        StepTopologyApplyMetadataProbesResultA0 candidate{};
        ContractError ignored;
        if (decode_StepTopologyApplyMetadataProbesResultA0(value, &candidate, path, &ignored))
        {
            ++matches;
            selected = OperationResultValueA0(std::in_place_index<9>, std::move(candidate));
        }
    }
    {
        StepTopologyCheckpointEditJournalResultA0 candidate{};
        ContractError ignored;
        if (decode_StepTopologyCheckpointEditJournalResultA0(value, &candidate, path, &ignored))
        {
            ++matches;
            selected = OperationResultValueA0(std::in_place_index<10>, std::move(candidate));
        }
    }
    {
        StepTopologyApplyHierarchyResultA0 candidate{};
        ContractError ignored;
        if (decode_StepTopologyApplyHierarchyResultA0(value, &candidate, path, &ignored))
        {
            ++matches;
            selected = OperationResultValueA0(std::in_place_index<11>, std::move(candidate));
        }
    }
    {
        StepTopologySaveResultA0 candidate{};
        ContractError ignored;
        if (decode_StepTopologySaveResultA0(value, &candidate, path, &ignored))
        {
            ++matches;
            selected = OperationResultValueA0(std::in_place_index<12>, std::move(candidate));
        }
    }
    {
        StepTopologyRestoreResultA0 candidate{};
        ContractError ignored;
        if (decode_StepTopologyRestoreResultA0(value, &candidate, path, &ignored))
        {
            ++matches;
            selected = OperationResultValueA0(std::in_place_index<13>, std::move(candidate));
        }
    }
    {
        StepTopologyAnalyzeRecoveryResultA0 candidate{};
        ContractError ignored;
        if (decode_StepTopologyAnalyzeRecoveryResultA0(value, &candidate, path, &ignored))
        {
            ++matches;
            selected = OperationResultValueA0(std::in_place_index<14>, std::move(candidate));
        }
    }
    if (matches != 1)
        return fail(error, "geometer.contract.union_mismatch", path,
                    "Expected exactly one union variant.");
    *out = std::move(selected);
    return true;
}

bool write_OperationResultValueA0(rapidjson::Writer<rapidjson::StringBuffer>& writer,
                                  const OperationResultValueA0& value, ContractError* error)
{
    switch (value.index())
    {
    case 0:
        return write_ModelBoundsResultA0(writer, std::get<0>(value), error);
    case 1:
        return write_HlrProjectionResultA0(writer, std::get<1>(value), error);
    case 2:
        return write_PackedAttachmentProjectionA0(writer, std::get<2>(value), error);
    case 3:
        return write_StepTopologyOpenResultA0(writer, std::get<3>(value), error);
    case 4:
        return write_StepTopologyCloseResultA0(writer, std::get<4>(value), error);
    case 5:
        return write_StepTopologyInspectResultA0(writer, std::get<5>(value), error);
    case 6:
        return write_StepTopologyRenderResultA0(writer, std::get<6>(value), error);
    case 7:
        return write_StepTopologyResolveHitResultA0(writer, std::get<7>(value), error);
    case 8:
        return write_StepTopologyApplyLogicalGroupsResultA0(writer, std::get<8>(value), error);
    case 9:
        return write_StepTopologyApplyMetadataProbesResultA0(writer, std::get<9>(value), error);
    case 10:
        return write_StepTopologyCheckpointEditJournalResultA0(writer, std::get<10>(value), error);
    case 11:
        return write_StepTopologyApplyHierarchyResultA0(writer, std::get<11>(value), error);
    case 12:
        return write_StepTopologySaveResultA0(writer, std::get<12>(value), error);
    case 13:
        return write_StepTopologyRestoreResultA0(writer, std::get<13>(value), error);
    case 14:
        return write_StepTopologyAnalyzeRecoveryResultA0(writer, std::get<14>(value), error);
    default:
        return fail(error, "geometer.contract.union_mismatch", "", "Unknown union variant.");
    }
}

bool decode_OperationSuccessA0(const rapidjson::Value& value, OperationSuccessA0* out,
                               const std::string& path, ContractError* error)
{
    static const char* const names[] = {"operation", "ok", "result"};
    if (!validate_object(value, names, 3U, path, error))
        return false;
    {
        const auto member = value.FindMember("operation");
        if (member == value.MemberEnd())
            return fail(error, "geometer.contract.missing_field", child_path(path, "operation"),
                        "Required field is missing.");
        if (!decode_string(member->value, &out->operation, child_path(path, "operation"), error, 1U,
                           128U))
            return false;
    }
    {
        const auto member = value.FindMember("ok");
        if (member == value.MemberEnd())
            return fail(error, "geometer.contract.missing_field", child_path(path, "ok"),
                        "Required field is missing.");
        if (!decode_literal_boolean(member->value, &out->ok, child_path(path, "ok"), error, true))
            return false;
    }
    {
        const auto member = value.FindMember("result");
        if (member == value.MemberEnd())
            return fail(error, "geometer.contract.missing_field", child_path(path, "result"),
                        "Required field is missing.");
        if (!decode_OperationResultValueA0(member->value, &out->result, child_path(path, "result"),
                                           error))
            return false;
    }
    return true;
}

bool write_OperationSuccessA0(rapidjson::Writer<rapidjson::StringBuffer>& writer,
                              const OperationSuccessA0& value, ContractError* error)
{
    writer.StartObject();
    writer.Key("operation");
    if (!write_string(writer, value.operation, error, 1U, 128U))
        return false;
    writer.Key("ok");
    if (!write_literal_boolean(writer, value.ok, error, true))
        return false;
    writer.Key("result");
    if (!write_OperationResultValueA0(writer, value.result, error))
        return false;
    writer.EndObject();
    return true;
}

bool decode_OperationOutcomeA0(const rapidjson::Value& value, OperationOutcomeA0* out,
                               const std::string& path, ContractError* error)
{
    int matches = 0;
    OperationOutcomeA0 selected{};
    {
        OperationSuccessA0 candidate{};
        ContractError ignored;
        if (decode_OperationSuccessA0(value, &candidate, path, &ignored))
        {
            ++matches;
            selected = OperationOutcomeA0(std::in_place_index<0>, std::move(candidate));
        }
    }
    {
        OperationFailureA0 candidate{};
        ContractError ignored;
        if (decode_OperationFailureA0(value, &candidate, path, &ignored))
        {
            ++matches;
            selected = OperationOutcomeA0(std::in_place_index<1>, std::move(candidate));
        }
    }
    if (matches != 1)
        return fail(error, "geometer.contract.union_mismatch", path,
                    "Expected exactly one union variant.");
    *out = std::move(selected);
    return true;
}

bool write_OperationOutcomeA0(rapidjson::Writer<rapidjson::StringBuffer>& writer,
                              const OperationOutcomeA0& value, ContractError* error)
{
    switch (value.index())
    {
    case 0:
        return write_OperationSuccessA0(writer, std::get<0>(value), error);
    case 1:
        return write_OperationFailureA0(writer, std::get<1>(value), error);
    default:
        return fail(error, "geometer.contract.union_mismatch", "", "Unknown union variant.");
    }
}

bool parse_document(const unsigned char* data, std::size_t size, rapidjson::Document* document,
                    ContractError* error)
{
    if (document == nullptr || (data == nullptr && size != 0))
        return fail(error, "geometer.contract.invalid_argument", "", "Invalid JSON buffer.");
    if (size > kMaxJsonBytes)
        return fail(error, "geometer.contract.limit_exceeded", "",
                    "JSON exceeds the 8 MiB contract limit.");
    document->Parse<rapidjson::kParseValidateEncodingFlag>(reinterpret_cast<const char*>(data),
                                                           size);
    if (document->HasParseError())
        return fail(error, "geometer.contract.invalid_json", "",
                    rapidjson::GetParseError_En(document->GetParseError()));
    return true;
}

template <typename T>
bool encode_root(const T& value,
                 bool (*write)(rapidjson::Writer<rapidjson::StringBuffer>&, const T&,
                               ContractError*),
                 std::string* json, ContractError* error)
{
    if (json == nullptr)
        return fail(error, "geometer.contract.invalid_argument", "",
                    "Output JSON pointer is null.");
    rapidjson::StringBuffer buffer;
    rapidjson::Writer<rapidjson::StringBuffer> writer(buffer);
    if (!write(writer, value, error))
        return false;
    json->assign(buffer.GetString(), buffer.GetSize());
    return true;
}

} // namespace

bool decode_json(const unsigned char* data, std::size_t size, DiagnosticA0* value,
                 ContractError* error)
{
    if (value == nullptr)
        return fail(error, "geometer.contract.invalid_argument", "",
                    "Output value pointer is null.");
    rapidjson::Document document;
    if (!parse_document(data, size, &document, error))
        return false;
    DiagnosticA0 decoded{};
    if (!decode_DiagnosticA0(document, &decoded, "", error))
        return false;
    *value = std::move(decoded);
    return true;
}

bool encode_json(const DiagnosticA0& value, std::string* json, ContractError* error)
{
    return encode_root<DiagnosticA0>(value, write_DiagnosticA0, json, error);
}

bool decode_json(const unsigned char* data, std::size_t size, HlrProjectionOptionsA0* value,
                 ContractError* error)
{
    if (value == nullptr)
        return fail(error, "geometer.contract.invalid_argument", "",
                    "Output value pointer is null.");
    rapidjson::Document document;
    if (!parse_document(data, size, &document, error))
        return false;
    HlrProjectionOptionsA0 decoded{};
    if (!decode_HlrProjectionOptionsA0(document, &decoded, "", error))
        return false;
    *value = std::move(decoded);
    return true;
}

bool encode_json(const HlrProjectionOptionsA0& value, std::string* json, ContractError* error)
{
    return encode_root<HlrProjectionOptionsA0>(value, write_HlrProjectionOptionsA0, json, error);
}

bool decode_json(const unsigned char* data, std::size_t size, HlrProjectionResultA0* value,
                 ContractError* error)
{
    if (value == nullptr)
        return fail(error, "geometer.contract.invalid_argument", "",
                    "Output value pointer is null.");
    rapidjson::Document document;
    if (!parse_document(data, size, &document, error))
        return false;
    HlrProjectionResultA0 decoded{};
    if (!decode_HlrProjectionResultA0(document, &decoded, "", error))
        return false;
    *value = std::move(decoded);
    return true;
}

bool encode_json(const HlrProjectionResultA0& value, std::string* json, ContractError* error)
{
    return encode_root<HlrProjectionResultA0>(value, write_HlrProjectionResultA0, json, error);
}

bool decode_json(const unsigned char* data, std::size_t size, IpcCancelledA0* value,
                 ContractError* error)
{
    if (value == nullptr)
        return fail(error, "geometer.contract.invalid_argument", "",
                    "Output value pointer is null.");
    rapidjson::Document document;
    if (!parse_document(data, size, &document, error))
        return false;
    IpcCancelledA0 decoded{};
    if (!decode_IpcCancelledA0(document, &decoded, "", error))
        return false;
    *value = std::move(decoded);
    return true;
}

bool encode_json(const IpcCancelledA0& value, std::string* json, ContractError* error)
{
    return encode_root<IpcCancelledA0>(value, write_IpcCancelledA0, json, error);
}

bool decode_json(const unsigned char* data, std::size_t size, IpcCancelRejectedA0* value,
                 ContractError* error)
{
    if (value == nullptr)
        return fail(error, "geometer.contract.invalid_argument", "",
                    "Output value pointer is null.");
    rapidjson::Document document;
    if (!parse_document(data, size, &document, error))
        return false;
    IpcCancelRejectedA0 decoded{};
    if (!decode_IpcCancelRejectedA0(document, &decoded, "", error))
        return false;
    *value = std::move(decoded);
    return true;
}

bool encode_json(const IpcCancelRejectedA0& value, std::string* json, ContractError* error)
{
    return encode_root<IpcCancelRejectedA0>(value, write_IpcCancelRejectedA0, json, error);
}

bool decode_json(const unsigned char* data, std::size_t size, IpcHelloA0* value,
                 ContractError* error)
{
    if (value == nullptr)
        return fail(error, "geometer.contract.invalid_argument", "",
                    "Output value pointer is null.");
    rapidjson::Document document;
    if (!parse_document(data, size, &document, error))
        return false;
    IpcHelloA0 decoded{};
    if (!decode_IpcHelloA0(document, &decoded, "", error))
        return false;
    *value = std::move(decoded);
    return true;
}

bool encode_json(const IpcHelloA0& value, std::string* json, ContractError* error)
{
    return encode_root<IpcHelloA0>(value, write_IpcHelloA0, json, error);
}

bool decode_json(const unsigned char* data, std::size_t size, IpcOperationCatalogA0* value,
                 ContractError* error)
{
    if (value == nullptr)
        return fail(error, "geometer.contract.invalid_argument", "",
                    "Output value pointer is null.");
    rapidjson::Document document;
    if (!parse_document(data, size, &document, error))
        return false;
    IpcOperationCatalogA0 decoded{};
    if (!decode_IpcOperationCatalogA0(document, &decoded, "", error))
        return false;
    *value = std::move(decoded);
    return true;
}

bool encode_json(const IpcOperationCatalogA0& value, std::string* json, ContractError* error)
{
    return encode_root<IpcOperationCatalogA0>(value, write_IpcOperationCatalogA0, json, error);
}

bool decode_json(const unsigned char* data, std::size_t size, IpcProtocolErrorA0* value,
                 ContractError* error)
{
    if (value == nullptr)
        return fail(error, "geometer.contract.invalid_argument", "",
                    "Output value pointer is null.");
    rapidjson::Document document;
    if (!parse_document(data, size, &document, error))
        return false;
    IpcProtocolErrorA0 decoded{};
    if (!decode_IpcProtocolErrorA0(document, &decoded, "", error))
        return false;
    *value = std::move(decoded);
    return true;
}

bool encode_json(const IpcProtocolErrorA0& value, std::string* json, ContractError* error)
{
    return encode_root<IpcProtocolErrorA0>(value, write_IpcProtocolErrorA0, json, error);
}

bool decode_json(const unsigned char* data, std::size_t size, IpcReasonA0* value,
                 ContractError* error)
{
    if (value == nullptr)
        return fail(error, "geometer.contract.invalid_argument", "",
                    "Output value pointer is null.");
    rapidjson::Document document;
    if (!parse_document(data, size, &document, error))
        return false;
    IpcReasonA0 decoded{};
    if (!decode_IpcReasonA0(document, &decoded, "", error))
        return false;
    *value = std::move(decoded);
    return true;
}

bool encode_json(const IpcReasonA0& value, std::string* json, ContractError* error)
{
    return encode_root<IpcReasonA0>(value, write_IpcReasonA0, json, error);
}

bool decode_json(const unsigned char* data, std::size_t size, IpcRequestA0* value,
                 ContractError* error)
{
    if (value == nullptr)
        return fail(error, "geometer.contract.invalid_argument", "",
                    "Output value pointer is null.");
    rapidjson::Document document;
    if (!parse_document(data, size, &document, error))
        return false;
    IpcRequestA0 decoded{};
    if (!decode_IpcRequestA0(document, &decoded, "", error))
        return false;
    *value = std::move(decoded);
    return true;
}

bool encode_json(const IpcRequestA0& value, std::string* json, ContractError* error)
{
    return encode_root<IpcRequestA0>(value, write_IpcRequestA0, json, error);
}

bool decode_json(const unsigned char* data, std::size_t size, IpcShutdownAckA0* value,
                 ContractError* error)
{
    if (value == nullptr)
        return fail(error, "geometer.contract.invalid_argument", "",
                    "Output value pointer is null.");
    rapidjson::Document document;
    if (!parse_document(data, size, &document, error))
        return false;
    IpcShutdownAckA0 decoded{};
    if (!decode_IpcShutdownAckA0(document, &decoded, "", error))
        return false;
    *value = std::move(decoded);
    return true;
}

bool encode_json(const IpcShutdownAckA0& value, std::string* json, ContractError* error)
{
    return encode_root<IpcShutdownAckA0>(value, write_IpcShutdownAckA0, json, error);
}

bool decode_json(const unsigned char* data, std::size_t size, IpcWelcomeA0* value,
                 ContractError* error)
{
    if (value == nullptr)
        return fail(error, "geometer.contract.invalid_argument", "",
                    "Output value pointer is null.");
    rapidjson::Document document;
    if (!parse_document(data, size, &document, error))
        return false;
    IpcWelcomeA0 decoded{};
    if (!decode_IpcWelcomeA0(document, &decoded, "", error))
        return false;
    *value = std::move(decoded);
    return true;
}

bool encode_json(const IpcWelcomeA0& value, std::string* json, ContractError* error)
{
    return encode_root<IpcWelcomeA0>(value, write_IpcWelcomeA0, json, error);
}

bool decode_json(const unsigned char* data, std::size_t size, MeshIllustrationInputA0* value,
                 ContractError* error)
{
    if (value == nullptr)
        return fail(error, "geometer.contract.invalid_argument", "",
                    "Output value pointer is null.");
    rapidjson::Document document;
    if (!parse_document(data, size, &document, error))
        return false;
    MeshIllustrationInputA0 decoded{};
    if (!decode_MeshIllustrationInputA0(document, &decoded, "", error))
        return false;
    *value = std::move(decoded);
    return true;
}

bool encode_json(const MeshIllustrationInputA0& value, std::string* json, ContractError* error)
{
    return encode_root<MeshIllustrationInputA0>(value, write_MeshIllustrationInputA0, json, error);
}

bool decode_json(const unsigned char* data, std::size_t size, MeshIllustrationResultA0* value,
                 ContractError* error)
{
    if (value == nullptr)
        return fail(error, "geometer.contract.invalid_argument", "",
                    "Output value pointer is null.");
    rapidjson::Document document;
    if (!parse_document(data, size, &document, error))
        return false;
    MeshIllustrationResultA0 decoded{};
    if (!decode_MeshIllustrationResultA0(document, &decoded, "", error))
        return false;
    *value = std::move(decoded);
    return true;
}

bool encode_json(const MeshIllustrationResultA0& value, std::string* json, ContractError* error)
{
    return encode_root<MeshIllustrationResultA0>(value, write_MeshIllustrationResultA0, json,
                                                 error);
}

bool decode_json(const unsigned char* data, std::size_t size, MeshIllustrationStyleA0* value,
                 ContractError* error)
{
    if (value == nullptr)
        return fail(error, "geometer.contract.invalid_argument", "",
                    "Output value pointer is null.");
    rapidjson::Document document;
    if (!parse_document(data, size, &document, error))
        return false;
    MeshIllustrationStyleA0 decoded{};
    if (!decode_MeshIllustrationStyleA0(document, &decoded, "", error))
        return false;
    *value = std::move(decoded);
    return true;
}

bool encode_json(const MeshIllustrationStyleA0& value, std::string* json, ContractError* error)
{
    return encode_root<MeshIllustrationStyleA0>(value, write_MeshIllustrationStyleA0, json, error);
}

bool decode_json(const unsigned char* data, std::size_t size, ModelBoundsOptionsA0* value,
                 ContractError* error)
{
    if (value == nullptr)
        return fail(error, "geometer.contract.invalid_argument", "",
                    "Output value pointer is null.");
    rapidjson::Document document;
    if (!parse_document(data, size, &document, error))
        return false;
    ModelBoundsOptionsA0 decoded{};
    if (!decode_ModelBoundsOptionsA0(document, &decoded, "", error))
        return false;
    *value = std::move(decoded);
    return true;
}

bool encode_json(const ModelBoundsOptionsA0& value, std::string* json, ContractError* error)
{
    return encode_root<ModelBoundsOptionsA0>(value, write_ModelBoundsOptionsA0, json, error);
}

bool decode_json(const unsigned char* data, std::size_t size, ModelBoundsResultA0* value,
                 ContractError* error)
{
    if (value == nullptr)
        return fail(error, "geometer.contract.invalid_argument", "",
                    "Output value pointer is null.");
    rapidjson::Document document;
    if (!parse_document(data, size, &document, error))
        return false;
    ModelBoundsResultA0 decoded{};
    if (!decode_ModelBoundsResultA0(document, &decoded, "", error))
        return false;
    *value = std::move(decoded);
    return true;
}

bool encode_json(const ModelBoundsResultA0& value, std::string* json, ContractError* error)
{
    return encode_root<ModelBoundsResultA0>(value, write_ModelBoundsResultA0, json, error);
}

bool decode_json(const unsigned char* data, std::size_t size, OperationOutcomeA0* value,
                 ContractError* error)
{
    if (value == nullptr)
        return fail(error, "geometer.contract.invalid_argument", "",
                    "Output value pointer is null.");
    rapidjson::Document document;
    if (!parse_document(data, size, &document, error))
        return false;
    OperationOutcomeA0 decoded{};
    if (!decode_OperationOutcomeA0(document, &decoded, "", error))
        return false;
    *value = std::move(decoded);
    return true;
}

bool encode_json(const OperationOutcomeA0& value, std::string* json, ContractError* error)
{
    return encode_root<OperationOutcomeA0>(value, write_OperationOutcomeA0, json, error);
}

bool decode_json(const unsigned char* data, std::size_t size,
                 StepTopologyAnalyzeRecoveryRequestA0* value, ContractError* error)
{
    if (value == nullptr)
        return fail(error, "geometer.contract.invalid_argument", "",
                    "Output value pointer is null.");
    rapidjson::Document document;
    if (!parse_document(data, size, &document, error))
        return false;
    StepTopologyAnalyzeRecoveryRequestA0 decoded{};
    if (!decode_StepTopologyAnalyzeRecoveryRequestA0(document, &decoded, "", error))
        return false;
    *value = std::move(decoded);
    return true;
}

bool encode_json(const StepTopologyAnalyzeRecoveryRequestA0& value, std::string* json,
                 ContractError* error)
{
    return encode_root<StepTopologyAnalyzeRecoveryRequestA0>(
        value, write_StepTopologyAnalyzeRecoveryRequestA0, json, error);
}

bool decode_json(const unsigned char* data, std::size_t size,
                 StepTopologyAnalyzeRecoveryResultA0* value, ContractError* error)
{
    if (value == nullptr)
        return fail(error, "geometer.contract.invalid_argument", "",
                    "Output value pointer is null.");
    rapidjson::Document document;
    if (!parse_document(data, size, &document, error))
        return false;
    StepTopologyAnalyzeRecoveryResultA0 decoded{};
    if (!decode_StepTopologyAnalyzeRecoveryResultA0(document, &decoded, "", error))
        return false;
    *value = std::move(decoded);
    return true;
}

bool encode_json(const StepTopologyAnalyzeRecoveryResultA0& value, std::string* json,
                 ContractError* error)
{
    return encode_root<StepTopologyAnalyzeRecoveryResultA0>(
        value, write_StepTopologyAnalyzeRecoveryResultA0, json, error);
}

bool decode_json(const unsigned char* data, std::size_t size,
                 StepTopologyApplyHierarchyRequestA0* value, ContractError* error)
{
    if (value == nullptr)
        return fail(error, "geometer.contract.invalid_argument", "",
                    "Output value pointer is null.");
    rapidjson::Document document;
    if (!parse_document(data, size, &document, error))
        return false;
    StepTopologyApplyHierarchyRequestA0 decoded{};
    if (!decode_StepTopologyApplyHierarchyRequestA0(document, &decoded, "", error))
        return false;
    *value = std::move(decoded);
    return true;
}

bool encode_json(const StepTopologyApplyHierarchyRequestA0& value, std::string* json,
                 ContractError* error)
{
    return encode_root<StepTopologyApplyHierarchyRequestA0>(
        value, write_StepTopologyApplyHierarchyRequestA0, json, error);
}

bool decode_json(const unsigned char* data, std::size_t size,
                 StepTopologyApplyHierarchyResultA0* value, ContractError* error)
{
    if (value == nullptr)
        return fail(error, "geometer.contract.invalid_argument", "",
                    "Output value pointer is null.");
    rapidjson::Document document;
    if (!parse_document(data, size, &document, error))
        return false;
    StepTopologyApplyHierarchyResultA0 decoded{};
    if (!decode_StepTopologyApplyHierarchyResultA0(document, &decoded, "", error))
        return false;
    *value = std::move(decoded);
    return true;
}

bool encode_json(const StepTopologyApplyHierarchyResultA0& value, std::string* json,
                 ContractError* error)
{
    return encode_root<StepTopologyApplyHierarchyResultA0>(
        value, write_StepTopologyApplyHierarchyResultA0, json, error);
}

bool decode_json(const unsigned char* data, std::size_t size,
                 StepTopologyApplyLogicalGroupsRequestA0* value, ContractError* error)
{
    if (value == nullptr)
        return fail(error, "geometer.contract.invalid_argument", "",
                    "Output value pointer is null.");
    rapidjson::Document document;
    if (!parse_document(data, size, &document, error))
        return false;
    StepTopologyApplyLogicalGroupsRequestA0 decoded{};
    if (!decode_StepTopologyApplyLogicalGroupsRequestA0(document, &decoded, "", error))
        return false;
    *value = std::move(decoded);
    return true;
}

bool encode_json(const StepTopologyApplyLogicalGroupsRequestA0& value, std::string* json,
                 ContractError* error)
{
    return encode_root<StepTopologyApplyLogicalGroupsRequestA0>(
        value, write_StepTopologyApplyLogicalGroupsRequestA0, json, error);
}

bool decode_json(const unsigned char* data, std::size_t size,
                 StepTopologyApplyLogicalGroupsResultA0* value, ContractError* error)
{
    if (value == nullptr)
        return fail(error, "geometer.contract.invalid_argument", "",
                    "Output value pointer is null.");
    rapidjson::Document document;
    if (!parse_document(data, size, &document, error))
        return false;
    StepTopologyApplyLogicalGroupsResultA0 decoded{};
    if (!decode_StepTopologyApplyLogicalGroupsResultA0(document, &decoded, "", error))
        return false;
    *value = std::move(decoded);
    return true;
}

bool encode_json(const StepTopologyApplyLogicalGroupsResultA0& value, std::string* json,
                 ContractError* error)
{
    return encode_root<StepTopologyApplyLogicalGroupsResultA0>(
        value, write_StepTopologyApplyLogicalGroupsResultA0, json, error);
}

bool decode_json(const unsigned char* data, std::size_t size,
                 StepTopologyApplyMetadataProbesRequestA0* value, ContractError* error)
{
    if (value == nullptr)
        return fail(error, "geometer.contract.invalid_argument", "",
                    "Output value pointer is null.");
    rapidjson::Document document;
    if (!parse_document(data, size, &document, error))
        return false;
    StepTopologyApplyMetadataProbesRequestA0 decoded{};
    if (!decode_StepTopologyApplyMetadataProbesRequestA0(document, &decoded, "", error))
        return false;
    *value = std::move(decoded);
    return true;
}

bool encode_json(const StepTopologyApplyMetadataProbesRequestA0& value, std::string* json,
                 ContractError* error)
{
    return encode_root<StepTopologyApplyMetadataProbesRequestA0>(
        value, write_StepTopologyApplyMetadataProbesRequestA0, json, error);
}

bool decode_json(const unsigned char* data, std::size_t size,
                 StepTopologyApplyMetadataProbesResultA0* value, ContractError* error)
{
    if (value == nullptr)
        return fail(error, "geometer.contract.invalid_argument", "",
                    "Output value pointer is null.");
    rapidjson::Document document;
    if (!parse_document(data, size, &document, error))
        return false;
    StepTopologyApplyMetadataProbesResultA0 decoded{};
    if (!decode_StepTopologyApplyMetadataProbesResultA0(document, &decoded, "", error))
        return false;
    *value = std::move(decoded);
    return true;
}

bool encode_json(const StepTopologyApplyMetadataProbesResultA0& value, std::string* json,
                 ContractError* error)
{
    return encode_root<StepTopologyApplyMetadataProbesResultA0>(
        value, write_StepTopologyApplyMetadataProbesResultA0, json, error);
}

bool decode_json(const unsigned char* data, std::size_t size,
                 StepTopologyCheckpointEditJournalRequestA0* value, ContractError* error)
{
    if (value == nullptr)
        return fail(error, "geometer.contract.invalid_argument", "",
                    "Output value pointer is null.");
    rapidjson::Document document;
    if (!parse_document(data, size, &document, error))
        return false;
    StepTopologyCheckpointEditJournalRequestA0 decoded{};
    if (!decode_StepTopologyCheckpointEditJournalRequestA0(document, &decoded, "", error))
        return false;
    *value = std::move(decoded);
    return true;
}

bool encode_json(const StepTopologyCheckpointEditJournalRequestA0& value, std::string* json,
                 ContractError* error)
{
    return encode_root<StepTopologyCheckpointEditJournalRequestA0>(
        value, write_StepTopologyCheckpointEditJournalRequestA0, json, error);
}

bool decode_json(const unsigned char* data, std::size_t size,
                 StepTopologyCheckpointEditJournalResultA0* value, ContractError* error)
{
    if (value == nullptr)
        return fail(error, "geometer.contract.invalid_argument", "",
                    "Output value pointer is null.");
    rapidjson::Document document;
    if (!parse_document(data, size, &document, error))
        return false;
    StepTopologyCheckpointEditJournalResultA0 decoded{};
    if (!decode_StepTopologyCheckpointEditJournalResultA0(document, &decoded, "", error))
        return false;
    *value = std::move(decoded);
    return true;
}

bool encode_json(const StepTopologyCheckpointEditJournalResultA0& value, std::string* json,
                 ContractError* error)
{
    return encode_root<StepTopologyCheckpointEditJournalResultA0>(
        value, write_StepTopologyCheckpointEditJournalResultA0, json, error);
}

bool decode_json(const unsigned char* data, std::size_t size, StepTopologyCloseRequestA0* value,
                 ContractError* error)
{
    if (value == nullptr)
        return fail(error, "geometer.contract.invalid_argument", "",
                    "Output value pointer is null.");
    rapidjson::Document document;
    if (!parse_document(data, size, &document, error))
        return false;
    StepTopologyCloseRequestA0 decoded{};
    if (!decode_StepTopologyCloseRequestA0(document, &decoded, "", error))
        return false;
    *value = std::move(decoded);
    return true;
}

bool encode_json(const StepTopologyCloseRequestA0& value, std::string* json, ContractError* error)
{
    return encode_root<StepTopologyCloseRequestA0>(value, write_StepTopologyCloseRequestA0, json,
                                                   error);
}

bool decode_json(const unsigned char* data, std::size_t size, StepTopologyCloseResultA0* value,
                 ContractError* error)
{
    if (value == nullptr)
        return fail(error, "geometer.contract.invalid_argument", "",
                    "Output value pointer is null.");
    rapidjson::Document document;
    if (!parse_document(data, size, &document, error))
        return false;
    StepTopologyCloseResultA0 decoded{};
    if (!decode_StepTopologyCloseResultA0(document, &decoded, "", error))
        return false;
    *value = std::move(decoded);
    return true;
}

bool encode_json(const StepTopologyCloseResultA0& value, std::string* json, ContractError* error)
{
    return encode_root<StepTopologyCloseResultA0>(value, write_StepTopologyCloseResultA0, json,
                                                  error);
}

bool decode_json(const unsigned char* data, std::size_t size, StepTopologyInspectRequestA0* value,
                 ContractError* error)
{
    if (value == nullptr)
        return fail(error, "geometer.contract.invalid_argument", "",
                    "Output value pointer is null.");
    rapidjson::Document document;
    if (!parse_document(data, size, &document, error))
        return false;
    StepTopologyInspectRequestA0 decoded{};
    if (!decode_StepTopologyInspectRequestA0(document, &decoded, "", error))
        return false;
    *value = std::move(decoded);
    return true;
}

bool encode_json(const StepTopologyInspectRequestA0& value, std::string* json, ContractError* error)
{
    return encode_root<StepTopologyInspectRequestA0>(value, write_StepTopologyInspectRequestA0,
                                                     json, error);
}

bool decode_json(const unsigned char* data, std::size_t size, StepTopologyInspectResultA0* value,
                 ContractError* error)
{
    if (value == nullptr)
        return fail(error, "geometer.contract.invalid_argument", "",
                    "Output value pointer is null.");
    rapidjson::Document document;
    if (!parse_document(data, size, &document, error))
        return false;
    StepTopologyInspectResultA0 decoded{};
    if (!decode_StepTopologyInspectResultA0(document, &decoded, "", error))
        return false;
    *value = std::move(decoded);
    return true;
}

bool encode_json(const StepTopologyInspectResultA0& value, std::string* json, ContractError* error)
{
    return encode_root<StepTopologyInspectResultA0>(value, write_StepTopologyInspectResultA0, json,
                                                    error);
}

bool decode_json(const unsigned char* data, std::size_t size, StepTopologyOpenRequestA0* value,
                 ContractError* error)
{
    if (value == nullptr)
        return fail(error, "geometer.contract.invalid_argument", "",
                    "Output value pointer is null.");
    rapidjson::Document document;
    if (!parse_document(data, size, &document, error))
        return false;
    StepTopologyOpenRequestA0 decoded{};
    if (!decode_StepTopologyOpenRequestA0(document, &decoded, "", error))
        return false;
    *value = std::move(decoded);
    return true;
}

bool encode_json(const StepTopologyOpenRequestA0& value, std::string* json, ContractError* error)
{
    return encode_root<StepTopologyOpenRequestA0>(value, write_StepTopologyOpenRequestA0, json,
                                                  error);
}

bool decode_json(const unsigned char* data, std::size_t size, StepTopologyOpenResultA0* value,
                 ContractError* error)
{
    if (value == nullptr)
        return fail(error, "geometer.contract.invalid_argument", "",
                    "Output value pointer is null.");
    rapidjson::Document document;
    if (!parse_document(data, size, &document, error))
        return false;
    StepTopologyOpenResultA0 decoded{};
    if (!decode_StepTopologyOpenResultA0(document, &decoded, "", error))
        return false;
    *value = std::move(decoded);
    return true;
}

bool encode_json(const StepTopologyOpenResultA0& value, std::string* json, ContractError* error)
{
    return encode_root<StepTopologyOpenResultA0>(value, write_StepTopologyOpenResultA0, json,
                                                 error);
}

bool decode_json(const unsigned char* data, std::size_t size, StepTopologyRenderRequestA0* value,
                 ContractError* error)
{
    if (value == nullptr)
        return fail(error, "geometer.contract.invalid_argument", "",
                    "Output value pointer is null.");
    rapidjson::Document document;
    if (!parse_document(data, size, &document, error))
        return false;
    StepTopologyRenderRequestA0 decoded{};
    if (!decode_StepTopologyRenderRequestA0(document, &decoded, "", error))
        return false;
    *value = std::move(decoded);
    return true;
}

bool encode_json(const StepTopologyRenderRequestA0& value, std::string* json, ContractError* error)
{
    return encode_root<StepTopologyRenderRequestA0>(value, write_StepTopologyRenderRequestA0, json,
                                                    error);
}

bool decode_json(const unsigned char* data, std::size_t size, StepTopologyRenderResultA0* value,
                 ContractError* error)
{
    if (value == nullptr)
        return fail(error, "geometer.contract.invalid_argument", "",
                    "Output value pointer is null.");
    rapidjson::Document document;
    if (!parse_document(data, size, &document, error))
        return false;
    StepTopologyRenderResultA0 decoded{};
    if (!decode_StepTopologyRenderResultA0(document, &decoded, "", error))
        return false;
    *value = std::move(decoded);
    return true;
}

bool encode_json(const StepTopologyRenderResultA0& value, std::string* json, ContractError* error)
{
    return encode_root<StepTopologyRenderResultA0>(value, write_StepTopologyRenderResultA0, json,
                                                   error);
}

bool decode_json(const unsigned char* data, std::size_t size,
                 StepTopologyResolveHitRequestA0* value, ContractError* error)
{
    if (value == nullptr)
        return fail(error, "geometer.contract.invalid_argument", "",
                    "Output value pointer is null.");
    rapidjson::Document document;
    if (!parse_document(data, size, &document, error))
        return false;
    StepTopologyResolveHitRequestA0 decoded{};
    if (!decode_StepTopologyResolveHitRequestA0(document, &decoded, "", error))
        return false;
    *value = std::move(decoded);
    return true;
}

bool encode_json(const StepTopologyResolveHitRequestA0& value, std::string* json,
                 ContractError* error)
{
    return encode_root<StepTopologyResolveHitRequestA0>(
        value, write_StepTopologyResolveHitRequestA0, json, error);
}

bool decode_json(const unsigned char* data, std::size_t size, StepTopologyResolveHitResultA0* value,
                 ContractError* error)
{
    if (value == nullptr)
        return fail(error, "geometer.contract.invalid_argument", "",
                    "Output value pointer is null.");
    rapidjson::Document document;
    if (!parse_document(data, size, &document, error))
        return false;
    StepTopologyResolveHitResultA0 decoded{};
    if (!decode_StepTopologyResolveHitResultA0(document, &decoded, "", error))
        return false;
    *value = std::move(decoded);
    return true;
}

bool encode_json(const StepTopologyResolveHitResultA0& value, std::string* json,
                 ContractError* error)
{
    return encode_root<StepTopologyResolveHitResultA0>(value, write_StepTopologyResolveHitResultA0,
                                                       json, error);
}

bool decode_json(const unsigned char* data, std::size_t size, StepTopologyRestoreRequestA0* value,
                 ContractError* error)
{
    if (value == nullptr)
        return fail(error, "geometer.contract.invalid_argument", "",
                    "Output value pointer is null.");
    rapidjson::Document document;
    if (!parse_document(data, size, &document, error))
        return false;
    StepTopologyRestoreRequestA0 decoded{};
    if (!decode_StepTopologyRestoreRequestA0(document, &decoded, "", error))
        return false;
    *value = std::move(decoded);
    return true;
}

bool encode_json(const StepTopologyRestoreRequestA0& value, std::string* json, ContractError* error)
{
    return encode_root<StepTopologyRestoreRequestA0>(value, write_StepTopologyRestoreRequestA0,
                                                     json, error);
}

bool decode_json(const unsigned char* data, std::size_t size, StepTopologyRestoreResultA0* value,
                 ContractError* error)
{
    if (value == nullptr)
        return fail(error, "geometer.contract.invalid_argument", "",
                    "Output value pointer is null.");
    rapidjson::Document document;
    if (!parse_document(data, size, &document, error))
        return false;
    StepTopologyRestoreResultA0 decoded{};
    if (!decode_StepTopologyRestoreResultA0(document, &decoded, "", error))
        return false;
    *value = std::move(decoded);
    return true;
}

bool encode_json(const StepTopologyRestoreResultA0& value, std::string* json, ContractError* error)
{
    return encode_root<StepTopologyRestoreResultA0>(value, write_StepTopologyRestoreResultA0, json,
                                                    error);
}

bool decode_json(const unsigned char* data, std::size_t size, StepTopologySaveRequestA0* value,
                 ContractError* error)
{
    if (value == nullptr)
        return fail(error, "geometer.contract.invalid_argument", "",
                    "Output value pointer is null.");
    rapidjson::Document document;
    if (!parse_document(data, size, &document, error))
        return false;
    StepTopologySaveRequestA0 decoded{};
    if (!decode_StepTopologySaveRequestA0(document, &decoded, "", error))
        return false;
    *value = std::move(decoded);
    return true;
}

bool encode_json(const StepTopologySaveRequestA0& value, std::string* json, ContractError* error)
{
    return encode_root<StepTopologySaveRequestA0>(value, write_StepTopologySaveRequestA0, json,
                                                  error);
}

bool decode_json(const unsigned char* data, std::size_t size, StepTopologySaveResultA0* value,
                 ContractError* error)
{
    if (value == nullptr)
        return fail(error, "geometer.contract.invalid_argument", "",
                    "Output value pointer is null.");
    rapidjson::Document document;
    if (!parse_document(data, size, &document, error))
        return false;
    StepTopologySaveResultA0 decoded{};
    if (!decode_StepTopologySaveResultA0(document, &decoded, "", error))
        return false;
    *value = std::move(decoded);
    return true;
}

bool encode_json(const StepTopologySaveResultA0& value, std::string* json, ContractError* error)
{
    return encode_root<StepTopologySaveResultA0>(value, write_StepTopologySaveResultA0, json,
                                                 error);
}

} // namespace geometer::contracts
