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
bool decode_SourceEntityEvidence(const rapidjson::Value&, SourceEntityEvidence*, const std::string&,
                                 ContractError*);
bool write_SourceEntityEvidence(rapidjson::Writer<rapidjson::StringBuffer>&,
                                const SourceEntityEvidence&, ContractError*);
bool decode_BodySummary(const rapidjson::Value&, BodySummary*, const std::string&, ContractError*);
bool write_BodySummary(rapidjson::Writer<rapidjson::StringBuffer>&, const BodySummary&,
                       ContractError*);
bool decode_ComponentOccurrenceSummary(const rapidjson::Value&, ComponentOccurrenceSummary*,
                                       const std::string&, ContractError*);
bool write_ComponentOccurrenceSummary(rapidjson::Writer<rapidjson::StringBuffer>&,
                                      const ComponentOccurrenceSummary&, ContractError*);
bool decode_DefinitionSummary(const rapidjson::Value&, DefinitionSummary*, const std::string&,
                              ContractError*);
bool write_DefinitionSummary(rapidjson::Writer<rapidjson::StringBuffer>&, const DefinitionSummary&,
                             ContractError*);
bool decode_FaceSummary(const rapidjson::Value&, FaceSummary*, const std::string&, ContractError*);
bool write_FaceSummary(rapidjson::Writer<rapidjson::StringBuffer>&, const FaceSummary&,
                       ContractError*);
bool decode_GlbAttachmentDescriptor(const rapidjson::Value&, GlbAttachmentDescriptor*,
                                    const std::string&, ContractError*);
bool write_GlbAttachmentDescriptor(rapidjson::Writer<rapidjson::StringBuffer>&,
                                   const GlbAttachmentDescriptor&, ContractError*);
bool decode_InspectionCounts(const rapidjson::Value&, InspectionCounts*, const std::string&,
                             ContractError*);
bool write_InspectionCounts(rapidjson::Writer<rapidjson::StringBuffer>&, const InspectionCounts&,
                            ContractError*);
bool decode_RootOccurrenceSummary(const rapidjson::Value&, RootOccurrenceSummary*,
                                  const std::string&, ContractError*);
bool write_RootOccurrenceSummary(rapidjson::Writer<rapidjson::StringBuffer>&,
                                 const RootOccurrenceSummary&, ContractError*);
bool decode_OccurrenceSummary(const rapidjson::Value&, OccurrenceSummary*, const std::string&,
                              ContractError*);
bool write_OccurrenceSummary(rapidjson::Writer<rapidjson::StringBuffer>&, const OccurrenceSummary&,
                             ContractError*);
bool decode_PageRequest(const rapidjson::Value&, PageRequest*, const std::string&, ContractError*);
bool write_PageRequest(rapidjson::Writer<rapidjson::StringBuffer>&, const PageRequest&,
                       ContractError*);
bool decode_RenderCounts(const rapidjson::Value&, RenderCounts*, const std::string&,
                         ContractError*);
bool write_RenderCounts(rapidjson::Writer<rapidjson::StringBuffer>&, const RenderCounts&,
                        ContractError*);
bool decode_RenderArtifactDescriptor(const rapidjson::Value&, RenderArtifactDescriptor*,
                                     const std::string&, ContractError*);
bool write_RenderArtifactDescriptor(rapidjson::Writer<rapidjson::StringBuffer>&,
                                    const RenderArtifactDescriptor&, ContractError*);
bool decode_SessionReference(const rapidjson::Value&, SessionReference*, const std::string&,
                             ContractError*);
bool write_SessionReference(rapidjson::Writer<rapidjson::StringBuffer>&, const SessionReference&,
                            ContractError*);
bool decode_ShellSummary(const rapidjson::Value&, ShellSummary*, const std::string&,
                         ContractError*);
bool write_ShellSummary(rapidjson::Writer<rapidjson::StringBuffer>&, const ShellSummary&,
                        ContractError*);
bool decode_SourceDescriptor(const rapidjson::Value&, SourceDescriptor*, const std::string&,
                             ContractError*);
bool write_SourceDescriptor(rapidjson::Writer<rapidjson::StringBuffer>&, const SourceDescriptor&,
                            ContractError*);
bool decode_StepTopologyCloseRequestA0(const rapidjson::Value&, StepTopologyCloseRequestA0*,
                                       const std::string&, ContractError*);
bool write_StepTopologyCloseRequestA0(rapidjson::Writer<rapidjson::StringBuffer>&,
                                      const StepTopologyCloseRequestA0&, ContractError*);
bool decode_StepTopologyCloseResultA0(const rapidjson::Value&, StepTopologyCloseResultA0*,
                                      const std::string&, ContractError*);
bool write_StepTopologyCloseResultA0(rapidjson::Writer<rapidjson::StringBuffer>&,
                                     const StepTopologyCloseResultA0&, ContractError*);
bool decode_StepTopologyInspectRequestA0(const rapidjson::Value&, StepTopologyInspectRequestA0*,
                                         const std::string&, ContractError*);
bool write_StepTopologyInspectRequestA0(rapidjson::Writer<rapidjson::StringBuffer>&,
                                        const StepTopologyInspectRequestA0&, ContractError*);
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
bool decode_StepTopologyOpenRequestA0(const rapidjson::Value&, StepTopologyOpenRequestA0*,
                                      const std::string&, ContractError*);
bool write_StepTopologyOpenRequestA0(rapidjson::Writer<rapidjson::StringBuffer>&,
                                     const StepTopologyOpenRequestA0&, ContractError*);
bool decode_ToolDescriptor(const rapidjson::Value&, ToolDescriptor*, const std::string&,
                           ContractError*);
bool write_ToolDescriptor(rapidjson::Writer<rapidjson::StringBuffer>&, const ToolDescriptor&,
                          ContractError*);
bool decode_StepTopologyOpenResultA0(const rapidjson::Value&, StepTopologyOpenResultA0*,
                                     const std::string&, ContractError*);
bool write_StepTopologyOpenResultA0(rapidjson::Writer<rapidjson::StringBuffer>&,
                                    const StepTopologyOpenResultA0&, ContractError*);
bool decode_TessellationOptions(const rapidjson::Value&, TessellationOptions*, const std::string&,
                                ContractError*);
bool write_TessellationOptions(rapidjson::Writer<rapidjson::StringBuffer>&,
                               const TessellationOptions&, ContractError*);
bool decode_StepTopologyRenderRequestA0(const rapidjson::Value&, StepTopologyRenderRequestA0*,
                                        const std::string&, ContractError*);
bool write_StepTopologyRenderRequestA0(rapidjson::Writer<rapidjson::StringBuffer>&,
                                       const StepTopologyRenderRequestA0&, ContractError*);
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
bool decode_StepTopologyResolveHitRequestA0(const rapidjson::Value&,
                                            StepTopologyResolveHitRequestA0*, const std::string&,
                                            ContractError*);
bool write_StepTopologyResolveHitRequestA0(rapidjson::Writer<rapidjson::StringBuffer>&,
                                           const StepTopologyResolveHitRequestA0&, ContractError*);
bool decode_StepTopologyResolveHitResultA0(const rapidjson::Value&, StepTopologyResolveHitResultA0*,
                                           const std::string&, ContractError*);
bool write_StepTopologyResolveHitResultA0(rapidjson::Writer<rapidjson::StringBuffer>&,
                                          const StepTopologyResolveHitResultA0&, ContractError*);

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
                   ContractError* error, double minimum, double maximum)
{
    if (!value.IsNumber() || !std::isfinite(value.GetDouble()))
        return fail(error, "geometer.contract.type_mismatch", path, "Expected a finite number.");
    const double number = value.GetDouble();
    if (number < minimum || number > maximum)
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
                  ContractError* error, double minimum, double maximum)
{
    if (!std::isfinite(value) || value < minimum || value > maximum)
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

bool decode_double_item(const rapidjson::Value& value, double* out, const std::string& path,
                        ContractError* error)
{
    return decode_double(value, out, path, error, -std::numeric_limits<double>::infinity(),
                         std::numeric_limits<double>::infinity());
}

bool write_double_item(rapidjson::Writer<rapidjson::StringBuffer>& writer, const double& value,
                       ContractError* error)
{
    return write_double(writer, value, error, -std::numeric_limits<double>::infinity(),
                        std::numeric_limits<double>::infinity());
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
                           std::numeric_limits<double>::infinity()))
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
                          std::numeric_limits<double>::infinity()))
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

bool decode_IpcRequestValueA0(const rapidjson::Value& value, IpcRequestValueA0* out,
                              const std::string& path, ContractError* error)
{
    int matches = 0;
    IpcRequestValueA0 selected{};
    {
        ModelBoundsOptionsA0 candidate{};
        ContractError ignored;
        if (decode_ModelBoundsOptionsA0(value, &candidate, path, &ignored))
        {
            ++matches;
            selected = IpcRequestValueA0(std::in_place_index<0>, std::move(candidate));
        }
    }
    {
        PackedAttachmentProjectionA0 candidate{};
        ContractError ignored;
        if (decode_PackedAttachmentProjectionA0(value, &candidate, path, &ignored))
        {
            ++matches;
            selected = IpcRequestValueA0(std::in_place_index<1>, std::move(candidate));
        }
    }
    if (matches != 1)
        return fail(error, "geometer.contract.union_mismatch", path,
                    "Expected exactly one union variant.");
    *out = std::move(selected);
    return true;
}

bool write_IpcRequestValueA0(rapidjson::Writer<rapidjson::StringBuffer>& writer,
                             const IpcRequestValueA0& value, ContractError* error)
{
    switch (value.index())
    {
    case 0:
        return write_ModelBoundsOptionsA0(writer, std::get<0>(value), error);
    case 1:
        return write_PackedAttachmentProjectionA0(writer, std::get<1>(value), error);
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
                           std::numeric_limits<double>::infinity()))
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
                          std::numeric_limits<double>::infinity()))
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
                           error, 0, std::numeric_limits<double>::infinity()))
            return false;
    }
    {
        const auto member = value.FindMember("bounds_ms");
        if (member == value.MemberEnd())
            return fail(error, "geometer.contract.missing_field", child_path(path, "bounds_ms"),
                        "Required field is missing.");
        if (!decode_double(member->value, &out->bounds_ms, child_path(path, "bounds_ms"), error, 0,
                           std::numeric_limits<double>::infinity()))
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
                      std::numeric_limits<double>::infinity()))
        return false;
    writer.Key("bounds_ms");
    if (!write_double(writer, value.bounds_ms, error, 0, std::numeric_limits<double>::infinity()))
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
        PackedAttachmentProjectionA0 candidate{};
        ContractError ignored;
        if (decode_PackedAttachmentProjectionA0(value, &candidate, path, &ignored))
        {
            ++matches;
            selected = OperationResultValueA0(std::in_place_index<1>, std::move(candidate));
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
        return write_PackedAttachmentProjectionA0(writer, std::get<1>(value), error);
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

bool decode_BodySummary(const rapidjson::Value& value, BodySummary* out, const std::string& path,
                        ContractError* error)
{
    static const char* const names[] = {"handle",        "definition_handle", "topology_kind",
                                        "shell_handles", "face_handles",      "bounds_mm",
                                        "volume_mm3",    "source_entity"};
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
        const auto member = value.FindMember("shell_handles");
        if (member == value.MemberEnd())
            return fail(error, "geometer.contract.missing_field", child_path(path, "shell_handles"),
                        "Required field is missing.");
        if (!decode_array(member->value, &out->shell_handles, child_path(path, "shell_handles"),
                          error, 0U, 250000U, decode_string_item))
            return false;
    }
    {
        const auto member = value.FindMember("face_handles");
        if (member == value.MemberEnd())
            return fail(error, "geometer.contract.missing_field", child_path(path, "face_handles"),
                        "Required field is missing.");
        if (!decode_array(member->value, &out->face_handles, child_path(path, "face_handles"),
                          error, 0U, 1000000U, decode_string_item))
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
                           0, std::numeric_limits<double>::infinity()))
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
    writer.Key("shell_handles");
    if (!write_array(writer, value.shell_handles, error, 0U, 250000U, write_string_item))
        return false;
    writer.Key("face_handles");
    if (!write_array(writer, value.face_handles, error, 0U, 1000000U, write_string_item))
        return false;
    writer.Key("bounds_mm");
    if (!write_array(writer, value.bounds_mm, error, 6U, 6U, write_double_item))
        return false;
    writer.Key("volume_mm3");
    if (!write_double(writer, value.volume_mm3, error, 0, std::numeric_limits<double>::infinity()))
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

bool decode_FaceSummary(const rapidjson::Value& value, FaceSummary* out, const std::string& path,
                        ContractError* error)
{
    static const char* const names[] = {"handle",        "definition_handle", "body_handles",
                                        "shell_handles", "bounds_mm",         "area_mm2",
                                        "centroid_mm",   "source_entity"};
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
        const auto member = value.FindMember("body_handles");
        if (member == value.MemberEnd())
            return fail(error, "geometer.contract.missing_field", child_path(path, "body_handles"),
                        "Required field is missing.");
        if (!decode_array(member->value, &out->body_handles, child_path(path, "body_handles"),
                          error, 0U, 100000U, decode_string_item))
            return false;
    }
    {
        const auto member = value.FindMember("shell_handles");
        if (member == value.MemberEnd())
            return fail(error, "geometer.contract.missing_field", child_path(path, "shell_handles"),
                        "Required field is missing.");
        if (!decode_array(member->value, &out->shell_handles, child_path(path, "shell_handles"),
                          error, 0U, 250000U, decode_string_item))
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
                           std::numeric_limits<double>::infinity()))
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
    writer.Key("body_handles");
    if (!write_array(writer, value.body_handles, error, 0U, 100000U, write_string_item))
        return false;
    writer.Key("shell_handles");
    if (!write_array(writer, value.shell_handles, error, 0U, 250000U, write_string_item))
        return false;
    writer.Key("bounds_mm");
    if (!write_array(writer, value.bounds_mm, error, 6U, 6U, write_double_item))
        return false;
    writer.Key("area_mm2");
    if (!write_double(writer, value.area_mm2, error, 0, std::numeric_limits<double>::infinity()))
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

bool decode_InspectionCounts(const rapidjson::Value& value, InspectionCounts* out,
                             const std::string& path, ContractError* error)
{
    static const char* const names[] = {"definitions", "root_occurrences", "component_occurrences",
                                        "bodies",      "shells",           "faces"};
    if (!validate_object(value, names, 6U, path, error))
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

bool decode_ShellSummary(const rapidjson::Value& value, ShellSummary* out, const std::string& path,
                         ContractError* error)
{
    static const char* const names[] = {"handle", "definition_handle", "body_handles",
                                        "face_handles", "source_entity"};
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
        const auto member = value.FindMember("body_handles");
        if (member == value.MemberEnd())
            return fail(error, "geometer.contract.missing_field", child_path(path, "body_handles"),
                        "Required field is missing.");
        if (!decode_array(member->value, &out->body_handles, child_path(path, "body_handles"),
                          error, 0U, 100000U, decode_string_item))
            return false;
    }
    {
        const auto member = value.FindMember("face_handles");
        if (member == value.MemberEnd())
            return fail(error, "geometer.contract.missing_field", child_path(path, "face_handles"),
                        "Required field is missing.");
        if (!decode_array(member->value, &out->face_handles, child_path(path, "face_handles"),
                          error, 0U, 1000000U, decode_string_item))
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
    writer.Key("body_handles");
    if (!write_array(writer, value.body_handles, error, 0U, 100000U, write_string_item))
        return false;
    writer.Key("face_handles");
    if (!write_array(writer, value.face_handles, error, 0U, 1000000U, write_string_item))
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

bool decode_TopologyPage(const rapidjson::Value& value, TopologyPage* out, const std::string& path,
                         ContractError* error)
{
    static const char* const names[] = {"definitions", "occurrences", "bodies",
                                        "shells",      "faces",       "next_cursor"};
    if (!validate_object(value, names, 6U, path, error))
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
                           child_path(path, "linear_deflection_mm"), error, 0.000001, 1000))
            return false;
    }
    {
        const auto member = value.FindMember("angular_deflection_rad");
        if (member == value.MemberEnd())
            return fail(error, "geometer.contract.missing_field",
                        child_path(path, "angular_deflection_rad"), "Required field is missing.");
        if (!decode_double(member->value, &out->angular_deflection_rad,
                           child_path(path, "angular_deflection_rad"), error, 0.000001,
                           3.141592653589793))
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
    if (!write_double(writer, value.linear_deflection_mm, error, 0.000001, 1000))
        return false;
    writer.Key("angular_deflection_rad");
    if (!write_double(writer, value.angular_deflection_rad, error, 0.000001, 3.141592653589793))
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

} // namespace geometer::contracts
