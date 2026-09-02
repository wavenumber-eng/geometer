#include "geometer/projection.h"

#include <cmath>
#include <cstdint>
#include <cstdlib>
#include <exception>
#include <fstream>
#include <iostream>
#include <iterator>
#include <limits>
#include <map>
#include <set>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

namespace
{

void require(bool condition, const std::string& message)
{
    if (!condition)
    {
        throw std::runtime_error(message);
    }
}

[[noreturn]] void clean_exit(int code)
{
    std::cout.flush();
    std::cerr.flush();
#if defined(__EMSCRIPTEN__)
    std::exit(code);
#else
    std::_Exit(code);
#endif
}

std::vector<unsigned char> read_fixture_bytes(const std::string& name)
{
    const std::string path =
        std::string(GEOMETER_TEST_SOURCE_DIR) + "/tests/fixtures/step/embedded_models/" + name;
    std::ifstream input(path, std::ios::binary);
    require(input.good(), "STEP fixture should be readable: " + path);
    return std::vector<unsigned char>(std::istreambuf_iterator<char>(input),
                                      std::istreambuf_iterator<char>());
}

std::vector<unsigned char> read_relative_step_fixture(const std::string& relative_path)
{
    const std::string path =
        std::string(GEOMETER_TEST_SOURCE_DIR) + "/tests/fixtures/step/" + relative_path;
    std::ifstream input(path, std::ios::binary);
    require(input.good(), "STEP fixture should be readable: " + path);
    return std::vector<unsigned char>(std::istreambuf_iterator<char>(input),
                                      std::istreambuf_iterator<char>());
}

geometer::ProjectionViewSpec view_spec(const std::string& id)
{
    if (id == "top")
    {
        return {"top", {0.0, 0.0, 1.0}, {0.0, 1.0, 0.0}};
    }
    if (id == "front")
    {
        return {"front", {0.0, -1.0, 0.0}, {0.0, 0.0, 1.0}};
    }
    throw std::runtime_error("Unsupported test view: " + id);
}

geometer::HlrProjectionOptions projection_options(std::vector<geometer::ProjectionViewSpec> views)
{
    geometer::HlrProjectionOptions options;
    options.views = std::move(views);
    options.curve_mode = geometer::ProjectionCurveMode::Polyline;
    options.projection_algorithm = geometer::ProjectionAlgorithm::Poly;
    options.round_digits = 3;
    return options;
}

geometer::HlrProjectionResult project_or_throw(const std::string& fixture_name,
                                               const geometer::HlrProjectionOptions& options)
{
    const std::vector<unsigned char> step_bytes = read_fixture_bytes(fixture_name);
    geometer::HlrProjectionResult result;
    geometer::Status status;
    const int code = geometer::step_hlr_projection_from_bytes(step_bytes.data(), step_bytes.size(),
                                                              options, &result, &status);
    require(code == 0, "projection should succeed: " + status.message);
    return result;
}

const geometer::ProjectedViewGeometry& find_view(const geometer::HlrProjectionResult& result,
                                                 const std::string& id)
{
    for (const geometer::ProjectedViewGeometry& view : result.views)
    {
        if (view.view.id == id)
        {
            return view;
        }
    }
    throw std::runtime_error("Projection view not found: " + id);
}

struct PointKey
{
    std::int64_t x = 0;
    std::int64_t y = 0;

    bool operator<(const PointKey& other) const
    {
        if (x != other.x)
        {
            return x < other.x;
        }
        return y < other.y;
    }
};

PointKey point_key(double x, double y)
{
    return {static_cast<std::int64_t>(std::llround(x * 1000.0)),
            static_cast<std::int64_t>(std::llround(y * 1000.0))};
}

std::size_t non_degree_two_endpoint_count(const geometer::ProjectedModeGeometry& geometry)
{
    std::map<PointKey, int> degree;
    for (const geometer::ProjectedSegment& segment : geometry.segments)
    {
        ++degree[point_key(segment.x1, segment.y1)];
        ++degree[point_key(segment.x2, segment.y2)];
    }
    std::size_t count = 0;
    for (const auto& entry : degree)
    {
        if (entry.second != 2)
        {
            ++count;
        }
    }
    return count;
}

void representative_outlines_are_closed()
{
    const geometer::HlrProjectionOptions options =
        projection_options({view_spec("top"), view_spec("front")});

    struct Case
    {
        const char* fixture;
        const char* view;
        std::size_t expected_segments;
    };
    const Case cases[] = {
        {"TSOT-23-5.STEP", "top", 94},
        {"TSOT-23-5.STEP", "front", 73},
        {"SOT-23.STEP", "front", 16},
        {"sot223.stp", "front", 20},
    };

    for (const Case& test_case : cases)
    {
        const geometer::HlrProjectionResult result = project_or_throw(test_case.fixture, options);
        const geometer::ProjectedModeGeometry& outline = find_view(result, test_case.view).outline;
        require(outline.segments.size() == test_case.expected_segments,
                std::string(test_case.fixture) + " " + test_case.view +
                    " outline segment count changed");
        require(non_degree_two_endpoint_count(outline) == 0, std::string(test_case.fixture) + " " +
                                                                 test_case.view +
                                                                 " outline should be closed");
    }
}

void raw_outline_option_is_still_observable()
{
    geometer::HlrProjectionOptions closed_options = projection_options({view_spec("top")});
    geometer::HlrProjectionOptions raw_options = closed_options;
    raw_options.union_outline_polygons = false;

    const geometer::HlrProjectionResult closed = project_or_throw("TSOT-23-5.STEP", closed_options);
    const geometer::HlrProjectionResult raw = project_or_throw("TSOT-23-5.STEP", raw_options);

    const geometer::ProjectedModeGeometry& closed_outline = find_view(closed, "top").outline;
    const geometer::ProjectedModeGeometry& raw_outline = find_view(raw, "top").outline;
    require(closed_outline.segments.size() != raw_outline.segments.size(),
            "union_outline_polygons=false should expose raw contour segments");
}

void json_and_svg_use_outline_bbox_modes()
{
    const geometer::HlrProjectionResult result =
        project_or_throw("SOT-23.STEP", projection_options({view_spec("front")}));

    std::string json;
    geometer::Status status;
    int code = geometer::write_hlr_projection_json(result, &json, &status);
    require(code == 0, "projection JSON should write: " + status.message);
    require(json.find("\"schema\":\"geometry.projection.b0\"") != std::string::npos,
            "JSON should expose b0 projection schema");
    require(json.find("\"outline\"") != std::string::npos, "JSON should expose outline mode");
    require(json.find("\"simple\"") == std::string::npos, "JSON should not expose simple mode");
    require(json.find("\"bbox\"") != std::string::npos, "JSON should expose bbox mode");
    require(json.find("\"bounds\"") != std::string::npos, "JSON should expose mode bounds");

    std::string svg;
    code = geometer::write_hlr_projection_svg(result, "front", "outline", &svg, &status);
    require(code == 0, "outline SVG should write: " + status.message);
    require(svg.find("<svg") != std::string::npos, "outline SVG should contain SVG markup");

    code = geometer::write_hlr_projection_svg(result, "front", "bbox", &svg, &status);
    require(code == 0, "bbox SVG should write: " + status.message);

    code = geometer::write_hlr_projection_svg(result, "front", "simple", &svg, &status);
    require(code == 4, "legacy simple SVG mode should be rejected");

    code = geometer::write_hlr_projection_svg(result, "front", "bad-mode", &svg, &status);
    require(code == 4, "invalid SVG mode should be rejected");
}

void mesh_shadow_outline_avoids_side_view_edge_explosion()
{
    geometer::HlrProjectionOptions options = projection_options({view_spec("front")});
    options.outline_algorithm = geometer::ProjectionOutlineAlgorithm::MeshShadow;

    const geometer::HlrProjectionResult result = project_or_throw("BGA90-8X13mm.step", options);
    const geometer::ProjectedModeGeometry& outline = find_view(result, "front").outline;

    require(!outline.segments.empty(), "mesh-shadow outline should produce geometry");
    require(outline.segments.size() <= 20,
            "mesh-shadow outline should not emit one contour per projected ball");
    require(non_degree_two_endpoint_count(outline) == 0, "mesh-shadow outline should be closed");
}

void root_placement_stripping_matches_step_to_glb_definition_frame()
{
    const std::vector<unsigned char> step_bytes =
        read_relative_step_fixture("generated_topology/generated_fused_slab.step");
    geometer::HlrProjectionOptions placed_options = projection_options({view_spec("top")});
    placed_options.outline_algorithm = geometer::ProjectionOutlineAlgorithm::MeshShadow;
    placed_options.round_digits = 6;
    geometer::HlrProjectionOptions stripped_options = placed_options;
    stripped_options.strip_root_placement = true;

    const auto project = [&step_bytes](const geometer::HlrProjectionOptions& options)
    {
        geometer::HlrProjectionResult result;
        geometer::Status status;
        const int code = geometer::step_hlr_projection_from_bytes(
            step_bytes.data(), step_bytes.size(), options, &result, &status);
        require(code == 0, "root-placement projection should succeed: " + status.message);
        return result;
    };
    const geometer::HlrProjectionResult placed_result = project(placed_options);
    const geometer::HlrProjectionResult stripped_result = project(stripped_options);
    const geometer::ProjectedModeGeometry& placed = find_view(placed_result, "top").outline;
    const geometer::ProjectedModeGeometry& stripped = find_view(stripped_result, "top").outline;
    require(!placed.segments.empty() && !stripped.segments.empty(),
            "root-placement projections should contain outlines");

    const auto x_bounds = [](const geometer::ProjectedModeGeometry& geometry)
    {
        double minimum = std::numeric_limits<double>::infinity();
        double maximum = -std::numeric_limits<double>::infinity();
        for (const geometer::ProjectedSegment& segment : geometry.segments)
        {
            minimum = std::min(minimum, std::min(segment.x1, segment.x2));
            maximum = std::max(maximum, std::max(segment.x1, segment.x2));
        }
        return std::pair<double, double>{minimum, maximum};
    };
    const auto placed_x = x_bounds(placed);
    const auto stripped_x = x_bounds(stripped);
    require(std::fabs(stripped_x.first) < 1.0e-6 && std::fabs(stripped_x.second - 9.0) < 1.0e-6,
            "stripped HLR outline should use the STEP-to-GLB definition-local x frame");
    require(std::fabs(placed_x.first - stripped_x.first) > 1.0,
            "nonidentity root placement should remain observable when stripping is disabled");
}

} // namespace

int main()
{
    try
    {
        representative_outlines_are_closed();
        raw_outline_option_is_still_observable();
        json_and_svg_use_outline_bbox_modes();
        mesh_shadow_outline_avoids_side_view_edge_explosion();
        root_placement_stripping_matches_step_to_glb_definition_frame();
    }
    catch (const std::exception& e)
    {
        std::cerr << e.what() << "\n";
        clean_exit(1);
    }

    clean_exit(0);
}
