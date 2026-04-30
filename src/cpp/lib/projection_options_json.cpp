#include "geometer/projection_options_json.h"

#include <array>
#include <cctype>
#include <cmath>
#include <cstdlib>
#include <map>
#include <sstream>
#include <string>
#include <utility>
#include <vector>

namespace geometer
{
namespace
{

struct JsonValue
{
    enum class Type
    {
        Null,
        Bool,
        Number,
        String,
        Array,
        Object
    };

    Type type = Type::Null;
    bool bool_value = false;
    double number_value = 0.0;
    std::string string_value;
    std::vector<JsonValue> array_value;
    std::map<std::string, JsonValue> object_value;
};

class Parser
{
  public:
    explicit Parser(const char* text) : text_(text == nullptr ? "" : text) {}

    bool parse(JsonValue* value, std::string* error)
    {
        skip_ws();
        if (eof())
        {
            *value = JsonValue();
            return true;
        }
        if (!parse_value(value, error))
        {
            return false;
        }
        skip_ws();
        if (!eof())
        {
            *error = "Unexpected trailing characters in projection options JSON.";
            return false;
        }
        return true;
    }

  private:
    bool eof() const
    {
        return pos_ >= text_.size();
    }

    char peek() const
    {
        return eof() ? '\0' : text_[pos_];
    }

    char take()
    {
        return eof() ? '\0' : text_[pos_++];
    }

    void skip_ws()
    {
        while (!eof() && std::isspace(static_cast<unsigned char>(peek())) != 0)
        {
            ++pos_;
        }
    }

    bool consume(char expected)
    {
        skip_ws();
        if (peek() != expected)
        {
            return false;
        }
        ++pos_;
        return true;
    }

    bool parse_value(JsonValue* value, std::string* error)
    {
        skip_ws();
        switch (peek())
        {
        case '{':
            return parse_object(value, error);
        case '[':
            return parse_array(value, error);
        case '"':
            value->type = JsonValue::Type::String;
            return parse_string(&value->string_value, error);
        case 't':
            return parse_literal("true", value, JsonValue::Type::Bool, true, error);
        case 'f':
            return parse_literal("false", value, JsonValue::Type::Bool, false, error);
        case 'n':
            return parse_literal("null", value, JsonValue::Type::Null, false, error);
        default:
            if (peek() == '-' || std::isdigit(static_cast<unsigned char>(peek())) != 0)
            {
                return parse_number(value, error);
            }
            *error = "Unexpected token in projection options JSON.";
            return false;
        }
    }

    bool parse_literal(const char* literal, JsonValue* value, JsonValue::Type type, bool bool_value,
                       std::string* error)
    {
        const std::string expected(literal);
        if (text_.compare(pos_, expected.size(), expected) != 0)
        {
            *error = "Invalid literal in projection options JSON.";
            return false;
        }
        pos_ += expected.size();
        value->type = type;
        value->bool_value = bool_value;
        return true;
    }

    bool parse_number(JsonValue* value, std::string* error)
    {
        const char* start = text_.c_str() + pos_;
        char* end = nullptr;
        const double number = std::strtod(start, &end);
        if (end == start || !std::isfinite(number))
        {
            *error = "Invalid number in projection options JSON.";
            return false;
        }
        pos_ += static_cast<std::size_t>(end - start);
        value->type = JsonValue::Type::Number;
        value->number_value = number;
        return true;
    }

    bool parse_string(std::string* value, std::string* error)
    {
        if (!consume('"'))
        {
            *error = "Expected string in projection options JSON.";
            return false;
        }

        value->clear();
        while (!eof())
        {
            const char ch = take();
            if (ch == '"')
            {
                return true;
            }
            if (ch != '\\')
            {
                value->push_back(ch);
                continue;
            }

            if (eof())
            {
                *error = "Invalid string escape in projection options JSON.";
                return false;
            }
            const char escaped = take();
            switch (escaped)
            {
            case '"':
            case '\\':
            case '/':
                value->push_back(escaped);
                break;
            case 'b':
                value->push_back('\b');
                break;
            case 'f':
                value->push_back('\f');
                break;
            case 'n':
                value->push_back('\n');
                break;
            case 'r':
                value->push_back('\r');
                break;
            case 't':
                value->push_back('\t');
                break;
            default:
                *error = "Unsupported string escape in projection options JSON.";
                return false;
            }
        }

        *error = "Unterminated string in projection options JSON.";
        return false;
    }

    bool parse_array(JsonValue* value, std::string* error)
    {
        if (!consume('['))
        {
            *error = "Expected array in projection options JSON.";
            return false;
        }
        value->type = JsonValue::Type::Array;
        value->array_value.clear();
        skip_ws();
        if (consume(']'))
        {
            return true;
        }

        while (true)
        {
            JsonValue item;
            if (!parse_value(&item, error))
            {
                return false;
            }
            value->array_value.push_back(std::move(item));
            skip_ws();
            if (consume(']'))
            {
                return true;
            }
            if (!consume(','))
            {
                *error = "Expected comma in projection options JSON array.";
                return false;
            }
        }
    }

    bool parse_object(JsonValue* value, std::string* error)
    {
        if (!consume('{'))
        {
            *error = "Expected object in projection options JSON.";
            return false;
        }
        value->type = JsonValue::Type::Object;
        value->object_value.clear();
        skip_ws();
        if (consume('}'))
        {
            return true;
        }

        while (true)
        {
            std::string key;
            if (!parse_string(&key, error))
            {
                return false;
            }
            if (!consume(':'))
            {
                *error = "Expected colon in projection options JSON object.";
                return false;
            }
            JsonValue item;
            if (!parse_value(&item, error))
            {
                return false;
            }
            value->object_value[key] = std::move(item);
            skip_ws();
            if (consume('}'))
            {
                return true;
            }
            if (!consume(','))
            {
                *error = "Expected comma in projection options JSON object.";
                return false;
            }
        }
    }

    std::string text_;
    std::size_t pos_ = 0;
};

void set_status(Status* status, int code, const std::string& message)
{
    if (status == nullptr)
    {
        return;
    }
    status->code = code;
    status->message = message;
}

const JsonValue* find_member(const JsonValue& object, const std::string& key)
{
    const auto found = object.object_value.find(key);
    return found == object.object_value.end() ? nullptr : &found->second;
}

const JsonValue* find_any_member(const JsonValue& object, std::initializer_list<const char*> keys)
{
    for (const char* key : keys)
    {
        const JsonValue* value = find_member(object, key);
        if (value != nullptr)
        {
            return value;
        }
    }
    return nullptr;
}

bool number_array3(const JsonValue& value, std::array<double, 3>* output, std::string* error,
                   const std::string& field_name)
{
    if (value.type != JsonValue::Type::Array || value.array_value.size() != 3)
    {
        *error = field_name + " must be an array of three numbers.";
        return false;
    }
    for (std::size_t i = 0; i < 3; ++i)
    {
        if (value.array_value[i].type != JsonValue::Type::Number)
        {
            *error = field_name + " must be an array of three numbers.";
            return false;
        }
        (*output)[i] = value.array_value[i].number_value;
    }
    return true;
}

bool bool_field(const JsonValue& value, bool* output, std::string* error,
                const std::string& field_name)
{
    if (value.type != JsonValue::Type::Bool)
    {
        *error = field_name + " must be a boolean.";
        return false;
    }
    *output = value.bool_value;
    return true;
}

bool int_field(const JsonValue& value, int* output, std::string* error,
               const std::string& field_name)
{
    if (value.type != JsonValue::Type::Number)
    {
        *error = field_name + " must be a number.";
        return false;
    }
    const double rounded = std::round(value.number_value);
    if (std::fabs(value.number_value - rounded) > 1.0e-9)
    {
        *error = field_name + " must be an integer.";
        return false;
    }
    *output = static_cast<int>(rounded);
    return true;
}

bool parse_curve_mode(const JsonValue& value, ProjectionCurveMode* output, std::string* error)
{
    if (value.type != JsonValue::Type::String)
    {
        *error = "curve_mode must be a string.";
        return false;
    }
    if (value.string_value == "native_arcs" || value.string_value == "native-arcs")
    {
        *output = ProjectionCurveMode::NativeArcs;
        return true;
    }
    if (value.string_value == "polyline")
    {
        *output = ProjectionCurveMode::Polyline;
        return true;
    }
    *error = "curve_mode must be native_arcs or polyline.";
    return false;
}

bool parse_views(const JsonValue& value, std::vector<ProjectionViewSpec>* views, std::string* error)
{
    if (value.type != JsonValue::Type::Array)
    {
        *error = "views must be an array.";
        return false;
    }

    views->clear();
    for (const JsonValue& item : value.array_value)
    {
        if (item.type != JsonValue::Type::Object)
        {
            *error = "Each view must be an object.";
            return false;
        }

        ProjectionViewSpec view;
        if (const JsonValue* id = find_member(item, "id"))
        {
            if (id->type != JsonValue::Type::String || id->string_value.empty())
            {
                *error = "view.id must be a non-empty string.";
                return false;
            }
            view.id = id->string_value;
        }
        if (const JsonValue* direction = find_member(item, "direction"))
        {
            if (!number_array3(*direction, &view.direction, error, "view.direction"))
            {
                return false;
            }
        }
        if (const JsonValue* up = find_member(item, "up"))
        {
            if (!number_array3(*up, &view.up, error, "view.up"))
            {
                return false;
            }
        }
        views->push_back(view);
    }
    return true;
}

} // namespace

int parse_hlr_projection_options_json(const char* json, HlrProjectionOptions* options,
                                      Status* status)
{
    if (options == nullptr)
    {
        set_status(status, 92, "Projection options pointer is null.");
        return 92;
    }

    JsonValue root;
    std::string error;
    Parser parser(json);
    if (!parser.parse(&root, &error))
    {
        set_status(status, 91, error);
        return 91;
    }
    if (root.type == JsonValue::Type::Null)
    {
        set_status(status, 0, "");
        return 0;
    }
    if (root.type != JsonValue::Type::Object)
    {
        set_status(status, 91, "Projection options JSON must be an object.");
        return 91;
    }

    HlrProjectionOptions parsed = *options;
    if (const JsonValue* views = find_member(root, "views"))
    {
        if (!parse_views(*views, &parsed.views, &error))
        {
            set_status(status, 91, error);
            return 91;
        }
    }
    if (const JsonValue* curve_mode = find_any_member(root, {"curve_mode", "curveMode"}))
    {
        if (!parse_curve_mode(*curve_mode, &parsed.curve_mode, &error))
        {
            set_status(status, 91, error);
            return 91;
        }
    }
    if (const JsonValue* samples = find_any_member(root, {"samples_per_curve", "samples"}))
    {
        if (!int_field(*samples, &parsed.samples_per_curve, &error, "samples_per_curve"))
        {
            set_status(status, 91, error);
            return 91;
        }
    }
    if (const JsonValue* round_digits = find_any_member(root, {"round_digits", "roundDigits"}))
    {
        if (!int_field(*round_digits, &parsed.round_digits, &error, "round_digits"))
        {
            set_status(status, 91, error);
            return 91;
        }
    }
    if (const JsonValue* include_visible =
            find_any_member(root, {"include_visible", "includeVisible"}))
    {
        if (!bool_field(*include_visible, &parsed.include_visible, &error, "include_visible"))
        {
            set_status(status, 91, error);
            return 91;
        }
    }
    if (const JsonValue* include_outline =
            find_any_member(root, {"include_outline", "includeOutline"}))
    {
        if (!bool_field(*include_outline, &parsed.include_outline, &error, "include_outline"))
        {
            set_status(status, 91, error);
            return 91;
        }
    }
    if (const JsonValue* union_polygons =
            find_any_member(root, {"union_simple_polygons", "unionPolygons"}))
    {
        if (!bool_field(*union_polygons, &parsed.union_simple_polygons, &error,
                        "union_simple_polygons"))
        {
            set_status(status, 91, error);
            return 91;
        }
    }

    *options = std::move(parsed);
    set_status(status, 0, "");
    return 0;
}

} // namespace geometer
