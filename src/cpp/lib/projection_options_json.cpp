#include "geometer/projection_options_json.h"

#include <array>
#include <cctype>
#include <cmath>
#include <cstdint>
#include <cstdlib>
#include <limits>
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

bool parse_model_transform(const JsonValue& value, std::array<double, 16>* output,
                           std::string* error)
{
    if (value.type != JsonValue::Type::Array)
    {
        *error = "model_transform must be a row-major 4x4 number matrix.";
        return false;
    }

    std::array<double, 16> parsed{};
    if (value.array_value.size() == 16)
    {
        for (std::size_t i = 0; i < 16; ++i)
        {
            if (value.array_value[i].type != JsonValue::Type::Number ||
                !std::isfinite(value.array_value[i].number_value))
            {
                *error = "model_transform must contain only finite numbers.";
                return false;
            }
            parsed[i] = value.array_value[i].number_value;
        }
    }
    else if (value.array_value.size() == 4)
    {
        for (std::size_t row = 0; row < 4; ++row)
        {
            const JsonValue& row_value = value.array_value[row];
            if (row_value.type != JsonValue::Type::Array || row_value.array_value.size() != 4)
            {
                *error = "model_transform must be a row-major 4x4 number matrix.";
                return false;
            }
            for (std::size_t col = 0; col < 4; ++col)
            {
                const JsonValue& item = row_value.array_value[col];
                if (item.type != JsonValue::Type::Number || !std::isfinite(item.number_value))
                {
                    *error = "model_transform must contain only finite numbers.";
                    return false;
                }
                parsed[(row * 4) + col] = item.number_value;
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

bool parse_projection_algorithm(const JsonValue& value, ProjectionAlgorithm* output,
                                std::string* error)
{
    if (value.type != JsonValue::Type::String)
    {
        *error = "projection_algorithm must be a string.";
        return false;
    }
    if (value.string_value == "poly")
    {
        *output = ProjectionAlgorithm::Poly;
        return true;
    }
    if (value.string_value == "exact")
    {
        *output = ProjectionAlgorithm::Exact;
        return true;
    }
    if (value.string_value == "fast")
    {
        *output = ProjectionAlgorithm::Fast;
        return true;
    }
    *error = "projection_algorithm must be poly, exact, or fast.";
    return false;
}

bool parse_mesh_deflection_mode(const JsonValue& value, MeshDeflectionMode* output,
                                std::string* error)
{
    if (value.type != JsonValue::Type::String)
    {
        *error = "mesh_deflection_mode must be a string.";
        return false;
    }
    if (value.string_value == "absolute")
    {
        *output = MeshDeflectionMode::Absolute;
        return true;
    }
    if (value.string_value == "bbox-relative" || value.string_value == "bbox_relative")
    {
        *output = MeshDeflectionMode::BboxRelative;
        return true;
    }
    *error = "mesh_deflection_mode must be absolute or bbox-relative.";
    return false;
}

bool parse_projection_outline_algorithm(const JsonValue& value, ProjectionOutlineAlgorithm* output,
                                        std::string* error)
{
    if (value.type != JsonValue::Type::String)
    {
        *error = "outline_algorithm must be a string.";
        return false;
    }
    if (value.string_value == "hlr-close" || value.string_value == "hlr_close" ||
        value.string_value == "hlr")
    {
        *output = ProjectionOutlineAlgorithm::HlrClosedEdges;
        return true;
    }
    if (value.string_value == "mesh-shadow" || value.string_value == "mesh_shadow" ||
        value.string_value == "shadow")
    {
        *output = ProjectionOutlineAlgorithm::MeshShadow;
        return true;
    }
    if (value.string_value == "fast-mesh-shadow" || value.string_value == "fast_mesh_shadow")
    {
        *output = ProjectionOutlineAlgorithm::FastMeshShadow;
        return true;
    }
    *error = "outline_algorithm must be hlr-close, mesh-shadow, or fast-mesh-shadow.";
    return false;
}

bool double_field(const JsonValue& value, double* output, std::string* error,
                  const std::string& field_name)
{
    if (value.type != JsonValue::Type::Number)
    {
        *error = field_name + " must be a number.";
        return false;
    }
    *output = value.number_value;
    return true;
}

bool size_field(const JsonValue& value, std::size_t* output, std::string* error,
                const std::string& field_name)
{
    constexpr double maximum = static_cast<double>(std::numeric_limits<std::uint32_t>::max());
    if (value.type != JsonValue::Type::Number || !std::isfinite(value.number_value) ||
        value.number_value < 0.0 || value.number_value > maximum)
    {
        *error = field_name + " must be a nonnegative 32-bit integer.";
        return false;
    }
    const double rounded = std::round(value.number_value);
    if (std::fabs(value.number_value - rounded) > 1.0e-9)
    {
        *error = field_name + " must be an integer.";
        return false;
    }
    *output = static_cast<std::size_t>(rounded);
    return true;
}

bool parse_fast_options(const JsonValue& value, FastHlrOptions* options, std::string* error)
{
    if (value.type != JsonValue::Type::Object)
    {
        *error = "fast must be an object.";
        return false;
    }
    struct BoolOption
    {
        const char* snake;
        const char* camel;
        bool FastHlrOptions::* member;
    };
    const BoolOption booleans[] = {
        {"include_boundaries", "includeBoundaries", &FastHlrOptions::include_boundaries},
        {"include_creases", "includeCreases", &FastHlrOptions::include_creases},
        {"include_silhouettes", "includeSilhouettes", &FastHlrOptions::include_silhouettes},
        {"include_hidden", "includeHidden", &FastHlrOptions::include_hidden},
        {"suppress_coplanar_seams", "suppressCoplanarSeams",
         &FastHlrOptions::suppress_coplanar_seams},
    };
    for (const BoolOption& option : booleans)
    {
        if (const JsonValue* node = find_any_member(value, {option.snake, option.camel}))
        {
            if (!bool_field(*node, &(options->*(option.member)), error,
                            std::string("fast.") + option.snake))
            {
                return false;
            }
        }
    }
    struct DoubleOption
    {
        const char* snake;
        const char* camel;
        double FastHlrOptions::* member;
    };
    const DoubleOption doubles[] = {
        {"crease_angle_rad", "creaseAngleRad", &FastHlrOptions::crease_angle_rad},
        {"weld_tolerance", "weldTolerance", &FastHlrOptions::weld_tolerance},
        {"projected_tolerance", "projectedTolerance", &FastHlrOptions::projected_tolerance},
        {"depth_tolerance", "depthTolerance", &FastHlrOptions::depth_tolerance},
        {"coplanar_seam_angle_rad", "coplanarSeamAngleRad",
         &FastHlrOptions::coplanar_seam_angle_rad},
        {"coplanar_seam_depth_tolerance", "coplanarSeamDepthTolerance",
         &FastHlrOptions::coplanar_seam_depth_tolerance},
        {"coplanar_seam_lateral_tolerance", "coplanarSeamLateralTolerance",
         &FastHlrOptions::coplanar_seam_lateral_tolerance},
    };
    for (const DoubleOption& option : doubles)
    {
        if (const JsonValue* node = find_any_member(value, {option.snake, option.camel}))
        {
            if (!double_field(*node, &(options->*(option.member)), error,
                              std::string("fast.") + option.snake))
            {
                return false;
            }
        }
    }
    if (const JsonValue* limits = find_member(value, "limits"))
    {
        if (limits->type != JsonValue::Type::Object)
        {
            *error = "fast.limits must be an object.";
            return false;
        }
        struct LimitOption
        {
            const char* snake;
            const char* camel;
            std::size_t FastHlrLimits::* member;
        };
        const LimitOption limit_options[] = {
            {"max_vertices", "maxVertices", &FastHlrLimits::max_vertices},
            {"max_triangles", "maxTriangles", &FastHlrLimits::max_triangles},
            {"max_edges", "maxEdges", &FastHlrLimits::max_edges},
            {"max_grid_references", "maxGridReferences", &FastHlrLimits::max_grid_references},
            {"max_candidate_pairs", "maxCandidatePairs", &FastHlrLimits::max_candidate_pairs},
            {"max_fragments", "maxFragments", &FastHlrLimits::max_fragments},
            {"max_output_segments", "maxOutputSegments", &FastHlrLimits::max_output_segments},
        };
        for (const LimitOption& option : limit_options)
        {
            if (const JsonValue* node = find_any_member(*limits, {option.snake, option.camel}))
            {
                if (!size_field(*node, &(options->limits.*(option.member)), error,
                                std::string("fast.limits.") + option.snake))
                {
                    return false;
                }
            }
        }
    }
    if (!std::isfinite(options->crease_angle_rad) || options->crease_angle_rad < 0.0 ||
        options->crease_angle_rad > 3.14159265358979323846)
    {
        *error = "fast.crease_angle_rad must be finite and between 0 and pi.";
        return false;
    }
    if (!std::isfinite(options->weld_tolerance) || options->weld_tolerance <= 0.0 ||
        !std::isfinite(options->projected_tolerance) || options->projected_tolerance <= 0.0 ||
        !std::isfinite(options->depth_tolerance) || options->depth_tolerance < 0.0 ||
        !std::isfinite(options->coplanar_seam_angle_rad) ||
        options->coplanar_seam_angle_rad < 0.0 ||
        options->coplanar_seam_angle_rad > 1.57079632679489661923 ||
        !std::isfinite(options->coplanar_seam_depth_tolerance) ||
        options->coplanar_seam_depth_tolerance < 0.0 ||
        !std::isfinite(options->coplanar_seam_lateral_tolerance) ||
        options->coplanar_seam_lateral_tolerance <= options->projected_tolerance)
    {
        *error = "fast tolerances must be finite; weld/projected must be positive, seam lateral "
                 "must exceed projected tolerance, depth/seam depth must be nonnegative, crease "
                 "angle must be between 0 and pi, and seam angle must be between 0 and pi/2.";
        return false;
    }
    return true;
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
    if (const JsonValue* transform = find_any_member(root, {"model_transform", "modelTransform"}))
    {
        if (!parse_model_transform(*transform, &parsed.model_transform, &error))
        {
            set_status(status, 91, error);
            return 91;
        }
    }
    if (const JsonValue* strip_root =
            find_any_member(root, {"strip_root_placement", "stripRootPlacement"}))
    {
        if (!bool_field(*strip_root, &parsed.strip_root_placement, &error, "strip_root_placement"))
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
    struct OutputLayerFlag
    {
        const char* snake;
        const char* camel;
        bool HlrProjectionOptions::* member;
    };
    const OutputLayerFlag output_layer_flags[] = {
        {"output_outline", "outputOutline", &HlrProjectionOptions::output_outline},
        {"output_detail", "outputDetail", &HlrProjectionOptions::output_detail},
        {"output_bbox", "outputBbox", &HlrProjectionOptions::output_bbox},
    };
    for (const OutputLayerFlag& flag : output_layer_flags)
    {
        if (const JsonValue* node = find_any_member(root, {flag.snake, flag.camel}))
        {
            if (!bool_field(*node, &(parsed.*(flag.member)), &error, flag.snake))
            {
                set_status(status, 91, error);
                return 91;
            }
        }
    }
    // Back-compat: include_visible toggles all visible edge categories on/off,
    // include_outline toggles only the visible-outline category. These map onto
    // the new granular fields; granular fields below override if both provided.
    if (const JsonValue* include_visible =
            find_any_member(root, {"include_visible", "includeVisible"}))
    {
        bool v = true;
        if (!bool_field(*include_visible, &v, &error, "include_visible"))
        {
            set_status(status, 91, error);
            return 91;
        }
        parsed.edge_v_sharp = v;
        parsed.edge_v_outline = v;
    }
    if (const JsonValue* include_outline =
            find_any_member(root, {"include_outline", "includeOutline"}))
    {
        bool v = true;
        if (!bool_field(*include_outline, &v, &error, "include_outline"))
        {
            set_status(status, 91, error);
            return 91;
        }
        parsed.edge_v_outline = v;
    }
    // Granular OCCT HLR edge category flags.
    struct EdgeFlag
    {
        const char* snake;
        const char* camel;
        bool HlrProjectionOptions::* member;
    };
    const EdgeFlag edge_flags[] = {
        {"edge_v_sharp", "edgeVSharp", &HlrProjectionOptions::edge_v_sharp},
        {"edge_v_outline", "edgeVOutline", &HlrProjectionOptions::edge_v_outline},
        {"edge_v_smooth", "edgeVSmooth", &HlrProjectionOptions::edge_v_smooth},
        {"edge_v_sewn", "edgeVSewn", &HlrProjectionOptions::edge_v_sewn},
        {"edge_v_iso", "edgeVIso", &HlrProjectionOptions::edge_v_iso},
        {"edge_h_sharp", "edgeHSharp", &HlrProjectionOptions::edge_h_sharp},
        {"edge_h_outline", "edgeHOutline", &HlrProjectionOptions::edge_h_outline},
        {"edge_h_smooth", "edgeHSmooth", &HlrProjectionOptions::edge_h_smooth},
        {"edge_h_sewn", "edgeHSewn", &HlrProjectionOptions::edge_h_sewn},
        {"edge_h_iso", "edgeHIso", &HlrProjectionOptions::edge_h_iso},
    };
    for (const EdgeFlag& flag : edge_flags)
    {
        if (const JsonValue* node = find_any_member(root, {flag.snake, flag.camel}))
        {
            if (!bool_field(*node, &(parsed.*(flag.member)), &error, flag.snake))
            {
                set_status(status, 91, error);
                return 91;
            }
        }
    }
    if (const JsonValue* union_polygons = find_any_member(
            root, {"union_outline_polygons", "unionOutlinePolygons", "unionPolygons"}))
    {
        if (!bool_field(*union_polygons, &parsed.union_outline_polygons, &error,
                        "union_outline_polygons"))
        {
            set_status(status, 91, error);
            return 91;
        }
    }
    if (const JsonValue* algorithm =
            find_any_member(root, {"projection_algorithm", "projectionAlgorithm"}))
    {
        if (!parse_projection_algorithm(*algorithm, &parsed.projection_algorithm, &error))
        {
            set_status(status, 91, error);
            return 91;
        }
    }
    if (const JsonValue* linear =
            find_any_member(root, {"mesh_linear_deflection", "meshLinearDeflection"}))
    {
        if (!double_field(*linear, &parsed.mesh_linear_deflection, &error,
                          "mesh_linear_deflection"))
        {
            set_status(status, 91, error);
            return 91;
        }
    }
    if (const JsonValue* angular =
            find_any_member(root, {"mesh_angular_deflection", "meshAngularDeflection"}))
    {
        if (!double_field(*angular, &parsed.mesh_angular_deflection, &error,
                          "mesh_angular_deflection"))
        {
            set_status(status, 91, error);
            return 91;
        }
    }
    if (const JsonValue* relative = find_any_member(root, {"mesh_relative", "meshRelative"}))
    {
        if (!bool_field(*relative, &parsed.mesh_relative, &error, "mesh_relative"))
        {
            set_status(status, 91, error);
            return 91;
        }
    }
    if (const JsonValue* mode =
            find_any_member(root, {"mesh_deflection_mode", "meshDeflectionMode"}))
    {
        if (!parse_mesh_deflection_mode(*mode, &parsed.mesh_deflection_mode, &error))
        {
            set_status(status, 91, error);
            return 91;
        }
    }
    if (const JsonValue* coeff =
            find_any_member(root, {"mesh_deflection_coefficient", "meshDeflectionCoefficient"}))
    {
        if (!double_field(*coeff, &parsed.mesh_deflection_coefficient, &error,
                          "mesh_deflection_coefficient"))
        {
            set_status(status, 91, error);
            return 91;
        }
    }
    if (const JsonValue* outline_algorithm =
            find_any_member(root, {"outline_algorithm", "outlineAlgorithm"}))
    {
        if (!parse_projection_outline_algorithm(*outline_algorithm, &parsed.outline_algorithm,
                                                &error))
        {
            set_status(status, 91, error);
            return 91;
        }
    }
    if (const JsonValue* angle =
            find_any_member(root, {"hlr_angle_tolerance", "hlrAngleTolerance"}))
    {
        if (!double_field(*angle, &parsed.hlr_angle_tolerance, &error, "hlr_angle_tolerance"))
        {
            set_status(status, 91, error);
            return 91;
        }
    }
    if (const JsonValue* fast = find_member(root, "fast"))
    {
        if (!parse_fast_options(*fast, &parsed.fast, &error))
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
