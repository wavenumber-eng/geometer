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
bool decode_Matrix4x4(const rapidjson::Value&, Matrix4x4*, const std::string&, ContractError*);
bool write_Matrix4x4(rapidjson::Writer<rapidjson::StringBuffer>&, const Matrix4x4&, ContractError*);
bool decode_ModelFormat(const rapidjson::Value&, ModelFormat*, const std::string&, ContractError*);
bool write_ModelFormat(rapidjson::Writer<rapidjson::StringBuffer>&, const ModelFormat&,
                       ContractError*);
bool decode_ModelBoundsOptionsA0(const rapidjson::Value&, ModelBoundsOptionsA0*, const std::string&,
                                 ContractError*);
bool write_ModelBoundsOptionsA0(rapidjson::Writer<rapidjson::StringBuffer>&,
                                const ModelBoundsOptionsA0&, ContractError*);
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
