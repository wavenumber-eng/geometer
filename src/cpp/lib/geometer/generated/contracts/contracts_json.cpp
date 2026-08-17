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
                   ContractError* error, std::uint64_t maximum)
{
    if (!value.IsUint64() || value.GetUint64() > maximum ||
        value.GetUint64() > std::numeric_limits<std::uint32_t>::max())
        return fail(error, "geometer.contract.number_range", path,
                    "Expected an unsigned 32-bit integer within its contract bounds.");
    *out = static_cast<std::uint32_t>(value.GetUint64());
    return true;
}

bool decode_uint64(const rapidjson::Value& value, std::uint64_t* out, const std::string& path,
                   ContractError* error, std::uint64_t maximum)
{
    if (!value.IsUint64() || value.GetUint64() > maximum)
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
                  ContractError* error, std::uint64_t maximum)
{
    if (value > maximum)
        return fail(error, "geometer.contract.number_range", "",
                    "Unsigned integer exceeds its contract bounds.");
    writer.Uint(value);
    return true;
}

bool write_uint64(rapidjson::Writer<rapidjson::StringBuffer>& writer, std::uint64_t value,
                  ContractError* error, std::uint64_t maximum)
{
    if (value > maximum)
        return fail(error, "geometer.contract.number_range", "",
                    "Unsigned integer exceeds its contract bounds.");
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
                           268435456ULL))
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
    if (!write_uint32(writer, value.max_bytes, error, 268435456ULL))
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
                           0ULL))
            return false;
    }
    {
        const auto member = value.FindMember("flags");
        if (member == value.MemberEnd())
            return fail(error, "geometer.contract.missing_field", child_path(path, "flags"),
                        "Required field is missing.");
        if (!decode_uint32(member->value, &out->flags, child_path(path, "flags"), error, 4ULL))
            return false;
    }
    {
        const auto member = value.FindMember("name");
        if (member == value.MemberEnd())
            return fail(error, "geometer.contract.missing_field", child_path(path, "name"),
                        "Required field is missing.");
        if (!decode_uint32(member->value, &out->name, child_path(path, "name"), error, 8ULL))
            return false;
    }
    {
        const auto member = value.FindMember("name_size");
        if (member == value.MemberEnd())
            return fail(error, "geometer.contract.missing_field", child_path(path, "name_size"),
                        "Required field is missing.");
        if (!decode_uint32(member->value, &out->name_size, child_path(path, "name_size"), error,
                           12ULL))
            return false;
    }
    {
        const auto member = value.FindMember("media_type");
        if (member == value.MemberEnd())
            return fail(error, "geometer.contract.missing_field", child_path(path, "media_type"),
                        "Required field is missing.");
        if (!decode_uint32(member->value, &out->media_type, child_path(path, "media_type"), error,
                           16ULL))
            return false;
    }
    {
        const auto member = value.FindMember("media_type_size");
        if (member == value.MemberEnd())
            return fail(error, "geometer.contract.missing_field",
                        child_path(path, "media_type_size"), "Required field is missing.");
        if (!decode_uint32(member->value, &out->media_type_size,
                           child_path(path, "media_type_size"), error, 20ULL))
            return false;
    }
    {
        const auto member = value.FindMember("data");
        if (member == value.MemberEnd())
            return fail(error, "geometer.contract.missing_field", child_path(path, "data"),
                        "Required field is missing.");
        if (!decode_uint32(member->value, &out->data, child_path(path, "data"), error, 24ULL))
            return false;
    }
    {
        const auto member = value.FindMember("data_size");
        if (member == value.MemberEnd())
            return fail(error, "geometer.contract.missing_field", child_path(path, "data_size"),
                        "Required field is missing.");
        if (!decode_uint32(member->value, &out->data_size, child_path(path, "data_size"), error,
                           28ULL))
            return false;
    }
    {
        const auto member = value.FindMember("reserved0");
        if (member == value.MemberEnd())
            return fail(error, "geometer.contract.missing_field", child_path(path, "reserved0"),
                        "Required field is missing.");
        if (!decode_uint32(member->value, &out->reserved0, child_path(path, "reserved0"), error,
                           32ULL))
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
    if (!write_uint32(writer, value.struct_size, error, 0ULL))
        return false;
    writer.Key("flags");
    if (!write_uint32(writer, value.flags, error, 4ULL))
        return false;
    writer.Key("name");
    if (!write_uint32(writer, value.name, error, 8ULL))
        return false;
    writer.Key("name_size");
    if (!write_uint32(writer, value.name_size, error, 12ULL))
        return false;
    writer.Key("media_type");
    if (!write_uint32(writer, value.media_type, error, 16ULL))
        return false;
    writer.Key("media_type_size");
    if (!write_uint32(writer, value.media_type_size, error, 20ULL))
        return false;
    writer.Key("data");
    if (!write_uint32(writer, value.data, error, 24ULL))
        return false;
    writer.Key("data_size");
    if (!write_uint32(writer, value.data_size, error, 28ULL))
        return false;
    writer.Key("reserved0");
    if (!write_uint32(writer, value.reserved0, error, 32ULL))
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
        if (!decode_uint32(member->value, &out->size, child_path(path, "size"), error, 36ULL))
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
    if (!write_uint32(writer, value.size, error, 36ULL))
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
                           0ULL))
            return false;
    }
    {
        const auto member = value.FindMember("flags");
        if (member == value.MemberEnd())
            return fail(error, "geometer.contract.missing_field", child_path(path, "flags"),
                        "Required field is missing.");
        if (!decode_uint32(member->value, &out->flags, child_path(path, "flags"), error, 4ULL))
            return false;
    }
    {
        const auto member = value.FindMember("name");
        if (member == value.MemberEnd())
            return fail(error, "geometer.contract.missing_field", child_path(path, "name"),
                        "Required field is missing.");
        if (!decode_uint32(member->value, &out->name, child_path(path, "name"), error, 8ULL))
            return false;
    }
    {
        const auto member = value.FindMember("name_size");
        if (member == value.MemberEnd())
            return fail(error, "geometer.contract.missing_field", child_path(path, "name_size"),
                        "Required field is missing.");
        if (!decode_uint32(member->value, &out->name_size, child_path(path, "name_size"), error,
                           16ULL))
            return false;
    }
    {
        const auto member = value.FindMember("media_type");
        if (member == value.MemberEnd())
            return fail(error, "geometer.contract.missing_field", child_path(path, "media_type"),
                        "Required field is missing.");
        if (!decode_uint32(member->value, &out->media_type, child_path(path, "media_type"), error,
                           24ULL))
            return false;
    }
    {
        const auto member = value.FindMember("media_type_size");
        if (member == value.MemberEnd())
            return fail(error, "geometer.contract.missing_field",
                        child_path(path, "media_type_size"), "Required field is missing.");
        if (!decode_uint32(member->value, &out->media_type_size,
                           child_path(path, "media_type_size"), error, 32ULL))
            return false;
    }
    {
        const auto member = value.FindMember("data");
        if (member == value.MemberEnd())
            return fail(error, "geometer.contract.missing_field", child_path(path, "data"),
                        "Required field is missing.");
        if (!decode_uint32(member->value, &out->data, child_path(path, "data"), error, 40ULL))
            return false;
    }
    {
        const auto member = value.FindMember("data_size");
        if (member == value.MemberEnd())
            return fail(error, "geometer.contract.missing_field", child_path(path, "data_size"),
                        "Required field is missing.");
        if (!decode_uint32(member->value, &out->data_size, child_path(path, "data_size"), error,
                           48ULL))
            return false;
    }
    {
        const auto member = value.FindMember("reserved0");
        if (member == value.MemberEnd())
            return fail(error, "geometer.contract.missing_field", child_path(path, "reserved0"),
                        "Required field is missing.");
        if (!decode_uint32(member->value, &out->reserved0, child_path(path, "reserved0"), error,
                           52ULL))
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
    if (!write_uint32(writer, value.struct_size, error, 0ULL))
        return false;
    writer.Key("flags");
    if (!write_uint32(writer, value.flags, error, 4ULL))
        return false;
    writer.Key("name");
    if (!write_uint32(writer, value.name, error, 8ULL))
        return false;
    writer.Key("name_size");
    if (!write_uint32(writer, value.name_size, error, 16ULL))
        return false;
    writer.Key("media_type");
    if (!write_uint32(writer, value.media_type, error, 24ULL))
        return false;
    writer.Key("media_type_size");
    if (!write_uint32(writer, value.media_type_size, error, 32ULL))
        return false;
    writer.Key("data");
    if (!write_uint32(writer, value.data, error, 40ULL))
        return false;
    writer.Key("data_size");
    if (!write_uint32(writer, value.data_size, error, 48ULL))
        return false;
    writer.Key("reserved0");
    if (!write_uint32(writer, value.reserved0, error, 52ULL))
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
        if (!decode_uint32(member->value, &out->size, child_path(path, "size"), error, 56ULL))
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
    if (!write_uint32(writer, value.size, error, 56ULL))
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
                           8388608ULL))
            return false;
    }
    {
        const auto member = value.FindMember("attachment_count");
        if (member == value.MemberEnd())
            return fail(error, "geometer.contract.missing_field",
                        child_path(path, "attachment_count"), "Required field is missing.");
        if (!decode_uint32(member->value, &out->attachment_count,
                           child_path(path, "attachment_count"), error, 16ULL))
            return false;
    }
    {
        const auto member = value.FindMember("attachment_name_bytes");
        if (member == value.MemberEnd())
            return fail(error, "geometer.contract.missing_field",
                        child_path(path, "attachment_name_bytes"), "Required field is missing.");
        if (!decode_uint32(member->value, &out->attachment_name_bytes,
                           child_path(path, "attachment_name_bytes"), error, 128ULL))
            return false;
    }
    {
        const auto member = value.FindMember("attachment_media_type_bytes");
        if (member == value.MemberEnd())
            return fail(error, "geometer.contract.missing_field",
                        child_path(path, "attachment_media_type_bytes"),
                        "Required field is missing.");
        if (!decode_uint32(member->value, &out->attachment_media_type_bytes,
                           child_path(path, "attachment_media_type_bytes"), error, 128ULL))
            return false;
    }
    {
        const auto member = value.FindMember("attachment_bytes");
        if (member == value.MemberEnd())
            return fail(error, "geometer.contract.missing_field",
                        child_path(path, "attachment_bytes"), "Required field is missing.");
        if (!decode_uint32(member->value, &out->attachment_bytes,
                           child_path(path, "attachment_bytes"), error, 268435456ULL))
            return false;
    }
    {
        const auto member = value.FindMember("frame_bytes");
        if (member == value.MemberEnd())
            return fail(error, "geometer.contract.missing_field", child_path(path, "frame_bytes"),
                        "Required field is missing.");
        if (!decode_uint32(member->value, &out->frame_bytes, child_path(path, "frame_bytes"), error,
                           536870912ULL))
            return false;
    }
    {
        const auto member = value.FindMember("queued_requests");
        if (member == value.MemberEnd())
            return fail(error, "geometer.contract.missing_field",
                        child_path(path, "queued_requests"), "Required field is missing.");
        if (!decode_uint32(member->value, &out->queued_requests,
                           child_path(path, "queued_requests"), error, 8ULL))
            return false;
    }
    {
        const auto member = value.FindMember("queued_bytes");
        if (member == value.MemberEnd())
            return fail(error, "geometer.contract.missing_field", child_path(path, "queued_bytes"),
                        "Required field is missing.");
        if (!decode_uint32(member->value, &out->queued_bytes, child_path(path, "queued_bytes"),
                           error, 536870912ULL))
            return false;
    }
    {
        const auto member = value.FindMember("resident_request_bytes");
        if (member == value.MemberEnd())
            return fail(error, "geometer.contract.missing_field",
                        child_path(path, "resident_request_bytes"), "Required field is missing.");
        if (!decode_uint32(member->value, &out->resident_request_bytes,
                           child_path(path, "resident_request_bytes"), error, 536870912ULL))
            return false;
    }
    {
        const auto member = value.FindMember("pending_writer_bytes");
        if (member == value.MemberEnd())
            return fail(error, "geometer.contract.missing_field",
                        child_path(path, "pending_writer_bytes"), "Required field is missing.");
        if (!decode_uint32(member->value, &out->pending_writer_bytes,
                           child_path(path, "pending_writer_bytes"), error, 536870912ULL))
            return false;
    }
    return true;
}

bool write_IpcEffectiveLimitsA0(rapidjson::Writer<rapidjson::StringBuffer>& writer,
                                const IpcEffectiveLimitsA0& value, ContractError* error)
{
    writer.StartObject();
    writer.Key("json_bytes");
    if (!write_uint32(writer, value.json_bytes, error, 8388608ULL))
        return false;
    writer.Key("attachment_count");
    if (!write_uint32(writer, value.attachment_count, error, 16ULL))
        return false;
    writer.Key("attachment_name_bytes");
    if (!write_uint32(writer, value.attachment_name_bytes, error, 128ULL))
        return false;
    writer.Key("attachment_media_type_bytes");
    if (!write_uint32(writer, value.attachment_media_type_bytes, error, 128ULL))
        return false;
    writer.Key("attachment_bytes");
    if (!write_uint32(writer, value.attachment_bytes, error, 268435456ULL))
        return false;
    writer.Key("frame_bytes");
    if (!write_uint32(writer, value.frame_bytes, error, 536870912ULL))
        return false;
    writer.Key("queued_requests");
    if (!write_uint32(writer, value.queued_requests, error, 8ULL))
        return false;
    writer.Key("queued_bytes");
    if (!write_uint32(writer, value.queued_bytes, error, 536870912ULL))
        return false;
    writer.Key("resident_request_bytes");
    if (!write_uint32(writer, value.resident_request_bytes, error, 536870912ULL))
        return false;
    writer.Key("pending_writer_bytes");
    if (!write_uint32(writer, value.pending_writer_bytes, error, 536870912ULL))
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
                           child_path(path, "operation_id_bytes"), error, 128ULL))
            return false;
    }
    {
        const auto member = value.FindMember("request_json_bytes");
        if (member == value.MemberEnd())
            return fail(error, "geometer.contract.missing_field",
                        child_path(path, "request_json_bytes"), "Required field is missing.");
        if (!decode_uint32(member->value, &out->request_json_bytes,
                           child_path(path, "request_json_bytes"), error, 8388608ULL))
            return false;
    }
    {
        const auto member = value.FindMember("response_json_bytes");
        if (member == value.MemberEnd())
            return fail(error, "geometer.contract.missing_field",
                        child_path(path, "response_json_bytes"), "Required field is missing.");
        if (!decode_uint32(member->value, &out->response_json_bytes,
                           child_path(path, "response_json_bytes"), error, 8388608ULL))
            return false;
    }
    {
        const auto member = value.FindMember("attachment_count");
        if (member == value.MemberEnd())
            return fail(error, "geometer.contract.missing_field",
                        child_path(path, "attachment_count"), "Required field is missing.");
        if (!decode_uint32(member->value, &out->attachment_count,
                           child_path(path, "attachment_count"), error, 16ULL))
            return false;
    }
    {
        const auto member = value.FindMember("attachment_name_bytes");
        if (member == value.MemberEnd())
            return fail(error, "geometer.contract.missing_field",
                        child_path(path, "attachment_name_bytes"), "Required field is missing.");
        if (!decode_uint32(member->value, &out->attachment_name_bytes,
                           child_path(path, "attachment_name_bytes"), error, 128ULL))
            return false;
    }
    {
        const auto member = value.FindMember("attachment_media_type_bytes");
        if (member == value.MemberEnd())
            return fail(error, "geometer.contract.missing_field",
                        child_path(path, "attachment_media_type_bytes"),
                        "Required field is missing.");
        if (!decode_uint32(member->value, &out->attachment_media_type_bytes,
                           child_path(path, "attachment_media_type_bytes"), error, 128ULL))
            return false;
    }
    {
        const auto member = value.FindMember("attachment_bytes");
        if (member == value.MemberEnd())
            return fail(error, "geometer.contract.missing_field",
                        child_path(path, "attachment_bytes"), "Required field is missing.");
        if (!decode_uint32(member->value, &out->attachment_bytes,
                           child_path(path, "attachment_bytes"), error, 268435456ULL))
            return false;
    }
    {
        const auto member = value.FindMember("aggregate_attachment_bytes_native");
        if (member == value.MemberEnd())
            return fail(error, "geometer.contract.missing_field",
                        child_path(path, "aggregate_attachment_bytes_native"),
                        "Required field is missing.");
        if (!decode_uint32(member->value, &out->aggregate_attachment_bytes_native,
                           child_path(path, "aggregate_attachment_bytes_native"), error,
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
                           child_path(path, "aggregate_attachment_bytes_wasm"), error,
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
    if (!write_uint32(writer, value.operation_id_bytes, error, 128ULL))
        return false;
    writer.Key("request_json_bytes");
    if (!write_uint32(writer, value.request_json_bytes, error, 8388608ULL))
        return false;
    writer.Key("response_json_bytes");
    if (!write_uint32(writer, value.response_json_bytes, error, 8388608ULL))
        return false;
    writer.Key("attachment_count");
    if (!write_uint32(writer, value.attachment_count, error, 16ULL))
        return false;
    writer.Key("attachment_name_bytes");
    if (!write_uint32(writer, value.attachment_name_bytes, error, 128ULL))
        return false;
    writer.Key("attachment_media_type_bytes");
    if (!write_uint32(writer, value.attachment_media_type_bytes, error, 128ULL))
        return false;
    writer.Key("attachment_bytes");
    if (!write_uint32(writer, value.attachment_bytes, error, 268435456ULL))
        return false;
    writer.Key("aggregate_attachment_bytes_native");
    if (!write_uint32(writer, value.aggregate_attachment_bytes_native, error, 536870912ULL))
        return false;
    writer.Key("aggregate_attachment_bytes_wasm");
    if (!write_uint32(writer, value.aggregate_attachment_bytes_wasm, error, 268435456ULL))
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
                           child_path(path, "c_abi_generation"), error,
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
    if (!write_uint32(writer, value.c_abi_generation, error,
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
        if (!decode_ModelBoundsOptionsA0(member->value, &out->request, child_path(path, "request"),
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
    if (!write_ModelBoundsOptionsA0(writer, value.request, error))
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
                           child_path(path, "rejectedQueuedRequestCount"), error,
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
    if (!write_uint32(writer, value.rejectedQueuedRequestCount, error,
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
                           child_path(path, "c_abi_generation"), error,
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
    if (!write_uint32(writer, value.c_abi_generation, error,
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

} // namespace geometer::contracts
