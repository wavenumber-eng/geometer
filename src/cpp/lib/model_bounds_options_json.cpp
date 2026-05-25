#include "geometer/model_bounds_options_json.h"

#include <rapidjson/document.h>
#include <rapidjson/error/en.h>

#include <cmath>
#include <string>

namespace geometer
{
namespace
{

void set_status(Status* status, int code, const std::string& message)
{
    if (status == nullptr)
    {
        return;
    }
    status->code = code;
    status->message = message;
}

const rapidjson::Value* find_member(const rapidjson::Value& object, const char* key)
{
    const auto found = object.FindMember(key);
    return found == object.MemberEnd() ? nullptr : &found->value;
}

const rapidjson::Value* find_any_member(const rapidjson::Value& object,
                                        std::initializer_list<const char*> keys)
{
    for (const char* key : keys)
    {
        const rapidjson::Value* value = find_member(object, key);
        if (value != nullptr)
        {
            return value;
        }
    }
    return nullptr;
}

bool parse_model_format(const rapidjson::Value& value, ModelFormat* output, std::string* error)
{
    if (!value.IsString())
    {
        *error = "format must be a string.";
        return false;
    }
    const std::string format = value.GetString();
    if (format == "step" || format == "STEP")
    {
        *output = ModelFormat::Step;
        return true;
    }
    *error = "model_bounds currently supports only format=\"step\".";
    return false;
}

bool parse_model_transform(const rapidjson::Value& value, std::array<double, 16>* output,
                           std::string* error)
{
    if (!value.IsArray())
    {
        *error = "model_transform must be a row-major 4x4 number matrix.";
        return false;
    }

    std::array<double, 16> parsed{};
    if (value.Size() == 16)
    {
        for (rapidjson::SizeType i = 0; i < value.Size(); ++i)
        {
            if (!value[i].IsNumber() || !std::isfinite(value[i].GetDouble()))
            {
                *error = "model_transform must contain only finite numbers.";
                return false;
            }
            parsed[i] = value[i].GetDouble();
        }
    }
    else if (value.Size() == 4)
    {
        for (rapidjson::SizeType row = 0; row < value.Size(); ++row)
        {
            const rapidjson::Value& row_value = value[row];
            if (!row_value.IsArray() || row_value.Size() != 4)
            {
                *error = "model_transform must be a row-major 4x4 number matrix.";
                return false;
            }
            for (rapidjson::SizeType col = 0; col < row_value.Size(); ++col)
            {
                const rapidjson::Value& item = row_value[col];
                if (!item.IsNumber() || !std::isfinite(item.GetDouble()))
                {
                    *error = "model_transform must contain only finite numbers.";
                    return false;
                }
                parsed[(static_cast<std::size_t>(row) * 4U) + static_cast<std::size_t>(col)] =
                    item.GetDouble();
            }
        }
    }
    else
    {
        *error = "model_transform must be a row-major 4x4 number matrix.";
        return false;
    }

    constexpr double tol = 1.0e-12;
    if (std::fabs(parsed[12]) > tol || std::fabs(parsed[13]) > tol || std::fabs(parsed[14]) > tol ||
        std::fabs(parsed[15] - 1.0) > tol)
    {
        *error = "model_transform final row must be [0, 0, 0, 1].";
        return false;
    }

    *output = parsed;
    return true;
}

} // namespace

int parse_model_bounds_options_json(const char* json, ModelBoundsOptions* options, Status* status)
{
    if (options == nullptr)
    {
        set_status(status, 92, "Model bounds options pointer is null.");
        return 92;
    }

    rapidjson::Document document;
    const char* text = json == nullptr ? "" : json;
    if (text[0] == '\0')
    {
        set_status(status, 0, "");
        return 0;
    }
    document.Parse(text);
    if (document.HasParseError())
    {
        set_status(status, 91,
                   std::string("Invalid model bounds options JSON: ") +
                       rapidjson::GetParseError_En(document.GetParseError()));
        return 91;
    }
    if (document.IsNull())
    {
        set_status(status, 0, "");
        return 0;
    }
    if (!document.IsObject())
    {
        set_status(status, 91, "Model bounds options JSON must be an object.");
        return 91;
    }

    ModelBoundsOptions parsed = *options;
    std::string error;
    if (const rapidjson::Value* format = find_any_member(document, {"format", "model_format"}))
    {
        if (!parse_model_format(*format, &parsed.format, &error))
        {
            set_status(status, 91, error);
            return 91;
        }
    }
    if (const rapidjson::Value* transform =
            find_any_member(document, {"model_transform", "modelTransform"}))
    {
        if (!parse_model_transform(*transform, &parsed.model_transform, &error))
        {
            set_status(status, 91, error);
            return 91;
        }
    }

    *options = parsed;
    set_status(status, 0, "");
    return 0;
}

} // namespace geometer
