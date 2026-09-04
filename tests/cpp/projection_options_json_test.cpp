#include "geometer/c_api.h"
#include "geometer/projection_options_json.h"
#include "geometer/step_to_glb_options_json.h"

#include <array>
#include <cstddef>
#include <exception>
#include <stdexcept>
#include <string>
#include <utility>

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
    require(options.output_outline && options.output_detail && options.output_bbox,
            "historical output layers should remain enabled by default");
    require(options.edge_v_sharp && options.edge_v_outline && !options.edge_v_smooth &&
                !options.edge_v_sewn && !options.edge_v_iso && !options.edge_h_sharp &&
                !options.edge_h_outline && !options.edge_h_smooth && !options.edge_h_sewn &&
                !options.edge_h_iso,
            "historical edge-category defaults should remain unchanged");
    require(options.projection_algorithm == geometer::ProjectionAlgorithm::Poly,
            "poly should remain the default projection algorithm");
    require(options.outline_algorithm == geometer::ProjectionOutlineAlgorithm::HlrClosedEdges,
            "HLR close should remain the default outline algorithm");
    require(options.fast.include_boundaries && options.fast.include_creases &&
                options.fast.include_silhouettes && !options.fast.include_hidden &&
                !options.fast.suppress_coplanar_seams,
            "Fast defaults should preserve the reviewed V1 edge selection");
    require(options.fast.crease_angle_rad == 0.5235987755982988,
            "Fast crease angle should default to 30 degrees in radians");
}

void parse_compatibility_matrix()
{
    const std::array<std::pair<const char*, geometer::ProjectionAlgorithm>, 2> algorithms = {{
        {"poly", geometer::ProjectionAlgorithm::Poly},
        {"exact", geometer::ProjectionAlgorithm::Exact},
    }};
    const std::array<std::pair<const char*, geometer::ProjectionOutlineAlgorithm>, 2> outlines = {{
        {"hlr-close", geometer::ProjectionOutlineAlgorithm::HlrClosedEdges},
        {"mesh-shadow", geometer::ProjectionOutlineAlgorithm::MeshShadow},
    }};
    for (const auto& algorithm : algorithms)
    {
        for (const auto& outline : outlines)
        {
            const std::string json = std::string("{\"projection_algorithm\":\"") + algorithm.first +
                                     "\",\"outline_algorithm\":\"" + outline.first + "\"}";
            geometer::HlrProjectionOptions options;
            geometer::Status status;
            require(geometer::parse_hlr_projection_options_json(json.c_str(), &options, &status) ==
                        0,
                    "every historical algorithm/outline combination should parse");
            require(options.projection_algorithm == algorithm.second &&
                        options.outline_algorithm == outline.second,
                    "historical algorithm/outline selection should remain exact");
        }
    }

    for (unsigned mask = 0; mask < 8; ++mask)
    {
        const bool outline = (mask & 1U) != 0U;
        const bool detail = (mask & 2U) != 0U;
        const bool bbox = (mask & 4U) != 0U;
        const std::string json = std::string("{\"output_outline\":") +
                                 (outline ? "true" : "false") +
                                 ",\"output_detail\":" + (detail ? "true" : "false") +
                                 ",\"output_bbox\":" + (bbox ? "true" : "false") + "}";
        geometer::HlrProjectionOptions options;
        geometer::Status status;
        require(geometer::parse_hlr_projection_options_json(json.c_str(), &options, &status) == 0,
                "every output-layer combination should parse");
        require(options.output_outline == outline && options.output_detail == detail &&
                    options.output_bbox == bbox,
                "output layers should remain independently selectable");
    }

    const char* edge_json =
        "{\"edge_v_sharp\":false,\"edge_v_outline\":false,\"edge_v_smooth\":true,"
        "\"edge_v_sewn\":true,\"edge_v_iso\":true,\"edge_h_sharp\":true,"
        "\"edge_h_outline\":true,\"edge_h_smooth\":true,\"edge_h_sewn\":true,"
        "\"edge_h_iso\":true}";
    geometer::HlrProjectionOptions edge_options;
    geometer::Status edge_status;
    require(geometer::parse_hlr_projection_options_json(edge_json, &edge_options, &edge_status) ==
                0,
            "all historical edge-category flags should parse");
    require(!edge_options.edge_v_sharp && !edge_options.edge_v_outline &&
                edge_options.edge_v_smooth && edge_options.edge_v_sewn && edge_options.edge_v_iso &&
                edge_options.edge_h_sharp && edge_options.edge_h_outline &&
                edge_options.edge_h_smooth && edge_options.edge_h_sewn && edge_options.edge_h_iso,
            "all historical edge-category flags should retain their meaning");
}

void parse_explicit_options()
{
    const char* json = "{"
                       "\"views\":[{\"id\":\"front\",\"direction\":[1,0,0],\"up\":[0,0,1]}],"
                       "\"model_transform\":["
                       "[1,0,0,10],"
                       "[0,2,0,20],"
                       "[0,0,3,30],"
                       "[0,0,0,1]"
                       "],"
                       "\"strip_root_placement\":true,"
                       "\"output_outline\":false,"
                       "\"outputDetail\":false,"
                       "\"output_bbox\":true,"
                       "\"curve_mode\":\"polyline\","
                       "\"samples_per_curve\":12,"
                       "\"round_digits\":4,"
                       "\"mesh_deflection_mode\":\"absolute\","
                       "\"mesh_deflection_coefficient\":0.002,"
                       "\"projection_algorithm\":\"fast\","
                       "\"outline_algorithm\":\"mesh-shadow\","
                       "\"fast\":{"
                       "\"include_boundaries\":false,"
                       "\"includeCreases\":true,"
                       "\"include_silhouettes\":false,"
                       "\"includeHidden\":true,"
                       "\"suppress_coplanar_seams\":true,"
                       "\"crease_angle_rad\":0.25,"
                       "\"weldTolerance\":0.000001,"
                       "\"projected_tolerance\":0.000002,"
                       "\"depthTolerance\":0.000003,"
                       "\"coplanar_seam_angle_rad\":0.01,"
                       "\"coplanar_seam_depth_tolerance\":0.000004,"
                       "\"coplanarSeamLateralTolerance\":0.000005,"
                       "\"limits\":{\"max_candidate_pairs\":1234,\"max_fragments\":2468,"
                       "\"maxGridReferences\":4321}"
                       "},"
                       "\"include_visible\":false,"
                       "\"include_outline\":true,"
                       "\"union_outline_polygons\":false"
                       "}";

    geometer::HlrProjectionOptions options;
    geometer::Status status;
    const int code = geometer::parse_hlr_projection_options_json(json, &options, &status);
    require(code == 0, "explicit options should parse");
    require(options.views.size() == 1, "one explicit view should be parsed");
    require(options.views[0].id == "front", "view id should parse");
    require(options.views[0].direction[0] == 1.0, "view direction should parse");
    require(options.views[0].up[2] == 1.0, "view up should parse");
    require(options.model_transform[3] == 10.0, "model_transform x translation should parse");
    require(options.model_transform[5] == 2.0, "model_transform y scale should parse");
    require(options.model_transform[11] == 30.0, "model_transform z translation should parse");
    require(options.strip_root_placement, "strip_root_placement should parse");
    require(!options.output_outline, "output_outline should parse");
    require(!options.output_detail, "outputDetail alias should parse");
    require(options.output_bbox, "output_bbox should parse");
    require(options.curve_mode == geometer::ProjectionCurveMode::Polyline,
            "curve mode should parse");
    require(options.samples_per_curve == 12, "samples should parse");
    require(options.round_digits == 4, "round digits should parse");
    require(options.mesh_deflection_mode == geometer::MeshDeflectionMode::Absolute,
            "mesh_deflection_mode should parse");
    require(options.mesh_deflection_coefficient == 0.002,
            "mesh_deflection_coefficient should parse");
    require(options.projection_algorithm == geometer::ProjectionAlgorithm::Fast,
            "fast projection_algorithm should parse");
    require(options.outline_algorithm == geometer::ProjectionOutlineAlgorithm::MeshShadow,
            "outline_algorithm should parse");
    // Legacy include_visible:false zeroes both visible-sharp and visible-outline.
    // Then include_outline:true re-enables visible-outline only.
    require(!options.edge_v_sharp, "include_visible should clear edge_v_sharp");
    require(options.edge_v_outline, "include_outline should set edge_v_outline");
    require(!options.union_outline_polygons, "union_outline_polygons should parse");
    require(!options.fast.include_boundaries, "fast.include_boundaries should parse");
    require(options.fast.include_creases, "fast.includeCreases should parse");
    require(!options.fast.include_silhouettes, "fast.include_silhouettes should parse");
    require(options.fast.include_hidden, "fast.includeHidden should parse");
    require(options.fast.suppress_coplanar_seams, "fast.suppress_coplanar_seams should parse");
    require(options.fast.crease_angle_rad == 0.25, "fast.crease_angle_rad should parse");
    require(options.fast.weld_tolerance == 0.000001, "fast.weldTolerance should parse");
    require(options.fast.projected_tolerance == 0.000002, "fast.projected_tolerance should parse");
    require(options.fast.depth_tolerance == 0.000003, "fast.depthTolerance should parse");
    require(options.fast.coplanar_seam_angle_rad == 0.01,
            "fast.coplanar_seam_angle_rad should parse");
    require(options.fast.coplanar_seam_depth_tolerance == 0.000004,
            "fast.coplanar_seam_depth_tolerance should parse");
    require(options.fast.coplanar_seam_lateral_tolerance == 0.000005,
            "fast.coplanarSeamLateralTolerance should parse");
    require(options.fast.limits.max_candidate_pairs == 1234,
            "fast.limits.max_candidate_pairs should parse");
    require(options.fast.limits.max_fragments == 2468, "fast.limits.max_fragments should parse");
    require(options.fast.limits.max_grid_references == 4321,
            "fast.limits.maxGridReferences should parse");
}

void parse_aliases()
{
    const char* json = "{"
                       "\"curveMode\":\"native-arcs\","
                       "\"samples\":6,"
                       "\"roundDigits\":2,"
                       "\"includeVisible\":true,"
                       "\"includeOutline\":false,"
                       "\"meshDeflectionMode\":\"bbox_relative\","
                       "\"meshDeflectionCoefficient\":0.005,"
                       "\"outlineAlgorithm\":\"hlr_close\","
                       "\"unionPolygons\":true,"
                       "\"modelTransform\":[1,0,0,4,0,1,0,5,0,0,1,6,0,0,0,1],"
                       "\"stripRootPlacement\":true"
                       "}";

    geometer::HlrProjectionOptions options;
    options.edge_v_outline = true;

    geometer::Status status;
    const int code = geometer::parse_hlr_projection_options_json(json, &options, &status);
    require(code == 0, "alias options should parse");
    require(options.curve_mode == geometer::ProjectionCurveMode::NativeArcs,
            "curveMode alias should parse");
    require(options.samples_per_curve == 6, "samples alias should parse");
    require(options.round_digits == 2, "roundDigits alias should parse");
    require(options.edge_v_sharp, "includeVisible alias should set edge_v_sharp");
    require(!options.edge_v_outline, "includeOutline alias should clear edge_v_outline");
    require(options.mesh_deflection_mode == geometer::MeshDeflectionMode::BboxRelative,
            "meshDeflectionMode alias should parse");
    require(options.mesh_deflection_coefficient == 0.005,
            "meshDeflectionCoefficient alias should parse");
    require(options.outline_algorithm == geometer::ProjectionOutlineAlgorithm::HlrClosedEdges,
            "outlineAlgorithm alias should parse");
    require(options.union_outline_polygons, "unionPolygons alias should parse");
    require(options.model_transform[3] == 4.0, "flat modelTransform alias should parse");
    require(options.model_transform[7] == 5.0, "flat modelTransform y translation should parse");
    require(options.model_transform[11] == 6.0, "flat modelTransform z translation should parse");
    require(options.strip_root_placement, "stripRootPlacement alias should parse");
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

    code = geometer::parse_hlr_projection_options_json("{\"outline_algorithm\":\"bad\"}", &options,
                                                       &status);
    require(code == 91, "invalid outline_algorithm should return parse error");

    code = geometer::parse_hlr_projection_options_json("{\"fast\":true}", &options, &status);
    require(code == 91, "non-object fast options should return parse error");

    code = geometer::parse_hlr_projection_options_json(
        "{\"outline_algorithm\":\"fast-mesh-shadow\"}", &options, &status);
    require(code == 0, "fast mesh-shadow outline should parse");
    require(options.outline_algorithm == geometer::ProjectionOutlineAlgorithm::FastMeshShadow,
            "fast mesh-shadow should select its additive outline backend");

    code = geometer::parse_hlr_projection_options_json(
        "{\"fast\":{\"limits\":{\"max_edges\":1.5}}}", &options, &status);
    require(code == 91, "fractional fast limit should return parse error");

    code = geometer::parse_hlr_projection_options_json(
        "{\"fast\":{\"limits\":{\"max_edges\":4294967296}}}", &options, &status);
    require(code == 91, "fast limits above the portable 32-bit range should be rejected");

    code = geometer::parse_hlr_projection_options_json("{\"fast\":{\"projected_tolerance\":0}}",
                                                       &options, &status);
    require(code == 91, "non-positive projected tolerance should be rejected");

    code = geometer::parse_hlr_projection_options_json("{\"fast\":{\"crease_angle_rad\":4}}",
                                                       &options, &status);
    require(code == 91, "crease angles above pi should be rejected");

    code = geometer::parse_hlr_projection_options_json(
        "{\"fast\":{\"coplanar_seam_lateral_tolerance\":0}}", &options, &status);
    require(code == 91, "non-positive coplanar seam lateral tolerance should be rejected");

    code = geometer::parse_hlr_projection_options_json("{\"fast\":{\"coplanar_seam_angle_rad\":2}}",
                                                       &options, &status);
    require(code == 91, "coplanar seam angles above pi/2 should be rejected");

    code = geometer::parse_hlr_projection_options_json("{\"model_transform\":[[1,0,0,0]]}",
                                                       &options, &status);
    require(code == 91, "invalid model_transform shape should return parse error");

    code = geometer::parse_hlr_projection_options_json(
        "{\"model_transform\":[1,0,0,0,0,1,0,0,0,0,1,0,0,0,1,1]}", &options, &status);
    require(code == 91, "invalid model_transform final row should return parse error");
}

void parse_step_to_glb_options()
{
    geometer::StepToGlbOptions options;
    geometer::Status status;
    int code = geometer::parse_step_to_glb_options_json(nullptr, &options, &status);
    require(code == 0, "null STEP-to-GLB options should parse as defaults");
    require(options.linear_deflection == 0.1, "default linear deflection should remain 0.1");
    require(options.angular_deflection == 0.5, "default angular deflection should remain 0.5");

    code = geometer::parse_step_to_glb_options_json(
        "{\"linearDeflection\":0.05,\"angular_deflection\":0.3,\"ignored\":{\"x\":1}}", &options,
        &status);
    require(code == 0, "STEP-to-GLB options aliases should parse");
    require(options.linear_deflection == 0.05, "linearDeflection alias should parse");
    require(options.angular_deflection == 0.3, "angular_deflection should parse");

    code = geometer::parse_step_to_glb_options_json("{\"deflection\":0}", &options, &status);
    require(code == 91, "zero deflection should be rejected");
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

void c_api_step_to_glb_uses_options_parser()
{
    unsigned char* value = nullptr;
    std::size_t value_size = 0;
    char* error = nullptr;
    int code = geometer_step_to_glb_bytes(nullptr, 0, "{\"linearDeflection\":\"bad\"}", &value,
                                          &value_size, &error);
    require(code == 91, "STEP-to-GLB C ABI should reject invalid options JSON");
    require(value == nullptr, "STEP-to-GLB invalid options should not return bytes");
    require(value_size == 0, "STEP-to-GLB invalid options should not return byte size");
    require(error != nullptr, "STEP-to-GLB invalid options should return an error");
    geometer_free_string(error);

    value = nullptr;
    value_size = 0;
    error = nullptr;
    code = geometer_step_to_glb_bytes(nullptr, 0, "{\"linearDeflection\":0.05}", &value,
                                      &value_size, &error);
    require(code == 1, "STEP-to-GLB valid options should reach STEP input validation");
    require(value == nullptr, "STEP-to-GLB empty STEP should not return bytes");
    require(value_size == 0, "STEP-to-GLB empty STEP should not return byte size");
    require(error != nullptr, "STEP-to-GLB empty STEP should return an error");
    geometer_free_string(error);

    GeometerBuffer empty = {nullptr, 0};
    GeometerByteResult result = geometer_step_to_glb(empty, "{\"angular\":0.25}");
    require(result.code == 1, "STEP-to-GLB result wrapper should reach STEP validation");
    require(result.value == nullptr, "STEP-to-GLB result wrapper should not return bytes on error");
    require(result.size == 0, "STEP-to-GLB result wrapper should not return byte size on error");
    require(result.error != nullptr, "STEP-to-GLB result wrapper should return an error");
    geometer_free_string(result.error);

    code = geometer_step_to_glb_bytes(nullptr, 0, nullptr, nullptr, &value_size, &error);
    require(code == 93, "STEP-to-GLB flat C ABI should reject null value output pointer");
}

} // namespace

int main()
{
    try
    {
        parse_defaults();
        parse_compatibility_matrix();
        parse_explicit_options();
        parse_aliases();
        reject_invalid_options();
        parse_step_to_glb_options();
        c_api_uses_options_parser();
        c_api_flat_bytes_export_uses_options_parser();
        c_api_step_to_glb_uses_options_parser();
    }
    catch (const std::exception&)
    {
        return 1;
    }

    return 0;
}
