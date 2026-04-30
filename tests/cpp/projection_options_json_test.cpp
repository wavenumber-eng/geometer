#include "geometer/c_api.h"
#include "geometer/projection_options_json.h"

#include <exception>
#include <stdexcept>
#include <string>

namespace
{

void require(bool condition, const std::string& message)
{
    if (!condition)
    {
        throw std::runtime_error(message);
    }
}

void parse_defaults()
{
    geometer::HlrProjectionOptions options;
    geometer::Status status;
    const int code = geometer::parse_hlr_projection_options_json(nullptr, &options, &status);
    require(code == 0, "null options should parse as defaults");
    require(options.views.empty(), "default views should remain implicit");
    require(options.curve_mode == geometer::ProjectionCurveMode::NativeArcs,
            "default curve mode should be native arcs");
    require(options.samples_per_curve == 24, "default sample count should remain 24");
}

void parse_explicit_options()
{
    const char* json = "{"
                       "\"views\":[{\"id\":\"front\",\"direction\":[1,0,0],\"up\":[0,0,1]}],"
                       "\"curve_mode\":\"polyline\","
                       "\"samples_per_curve\":12,"
                       "\"round_digits\":4,"
                       "\"include_visible\":false,"
                       "\"include_outline\":true,"
                       "\"union_simple_polygons\":false"
                       "}";

    geometer::HlrProjectionOptions options;
    geometer::Status status;
    const int code = geometer::parse_hlr_projection_options_json(json, &options, &status);
    require(code == 0, "explicit options should parse");
    require(options.views.size() == 1, "one explicit view should be parsed");
    require(options.views[0].id == "front", "view id should parse");
    require(options.views[0].direction[0] == 1.0, "view direction should parse");
    require(options.views[0].up[2] == 1.0, "view up should parse");
    require(options.curve_mode == geometer::ProjectionCurveMode::Polyline,
            "curve mode should parse");
    require(options.samples_per_curve == 12, "samples should parse");
    require(options.round_digits == 4, "round digits should parse");
    require(!options.include_visible, "include_visible should parse");
    require(options.include_outline, "include_outline should parse");
    require(!options.union_simple_polygons, "union_simple_polygons should parse");
}

void parse_aliases()
{
    const char* json = "{"
                       "\"curveMode\":\"native-arcs\","
                       "\"samples\":6,"
                       "\"roundDigits\":2,"
                       "\"includeVisible\":true,"
                       "\"includeOutline\":false,"
                       "\"unionPolygons\":true"
                       "}";

    geometer::HlrProjectionOptions options;
    options.include_outline = true;

    geometer::Status status;
    const int code = geometer::parse_hlr_projection_options_json(json, &options, &status);
    require(code == 0, "alias options should parse");
    require(options.curve_mode == geometer::ProjectionCurveMode::NativeArcs,
            "curveMode alias should parse");
    require(options.samples_per_curve == 6, "samples alias should parse");
    require(options.round_digits == 2, "roundDigits alias should parse");
    require(options.include_visible, "includeVisible alias should parse");
    require(!options.include_outline, "includeOutline alias should parse");
    require(options.union_simple_polygons, "unionPolygons alias should parse");
}

void reject_invalid_options()
{
    geometer::HlrProjectionOptions options;
    geometer::Status status;
    int code = geometer::parse_hlr_projection_options_json("{", &options, &status);
    require(code == 91, "invalid JSON should return parse error");

    code = geometer::parse_hlr_projection_options_json("{\"views\":[{\"direction\":[0,0]}]}",
                                                       &options, &status);
    require(code == 91, "invalid vector length should return parse error");

    code = geometer::parse_hlr_projection_options_json("{\"curve_mode\":\"splines\"}", &options,
                                                       &status);
    require(code == 91, "invalid curve mode should return parse error");
}

void c_api_uses_options_parser()
{
    GeometerBuffer empty = {nullptr, 0};

    GeometerStringResult invalid =
        geometer_step_hlr_projection_json(empty, "{\"curve_mode\":\"splines\"}");
    require(invalid.code == 91, "C ABI should reject invalid options JSON");
    require(invalid.value == nullptr, "C ABI invalid options should not return a value");
    require(invalid.error != nullptr, "C ABI invalid options should return an error");
    geometer_free_string(invalid.error);

    GeometerStringResult valid = geometer_step_hlr_projection_json(
        empty, "{\"views\":[{\"id\":\"top-only\",\"direction\":[0,0,1],\"up\":[0,1,0]}]}");
    require(valid.code == 1, "C ABI valid options should reach STEP input validation");
    require(valid.value == nullptr, "C ABI empty STEP should not return a value");
    require(valid.error != nullptr, "C ABI empty STEP should return an error");
    geometer_free_string(valid.error);
}

void c_api_flat_bytes_export_uses_options_parser()
{
    char* value = nullptr;
    char* error = nullptr;
    int code = geometer_step_hlr_projection_json_bytes(nullptr, 0, "{\"curveMode\":\"bad\"}",
                                                       &value, &error);
    require(code == 91, "flat C ABI should reject invalid options JSON");
    require(value == nullptr, "flat C ABI invalid options should not return a value");
    require(error != nullptr, "flat C ABI invalid options should return an error");
    geometer_free_string(error);

    value = nullptr;
    error = nullptr;
    code = geometer_step_hlr_projection_json_bytes(
        nullptr, 0, "{\"views\":[{\"id\":\"top-only\",\"direction\":[0,0,1],\"up\":[0,1,0]}]}",
        &value, &error);
    require(code == 1, "flat C ABI valid options should reach STEP input validation");
    require(value == nullptr, "flat C ABI empty STEP should not return a value");
    require(error != nullptr, "flat C ABI empty STEP should return an error");
    geometer_free_string(error);

    code = geometer_step_hlr_projection_json_bytes(nullptr, 0, nullptr, nullptr, &error);
    require(code == 93, "flat C ABI should reject null value output pointer");
}

} // namespace

int main()
{
    try
    {
        parse_defaults();
        parse_explicit_options();
        parse_aliases();
        reject_invalid_options();
        c_api_uses_options_parser();
        c_api_flat_bytes_export_uses_options_parser();
    }
    catch (const std::exception&)
    {
        return 1;
    }

    return 0;
}
