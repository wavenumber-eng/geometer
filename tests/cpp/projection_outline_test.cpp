#include "geometer/projection.h"

#include <algorithm>
#include <array>
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

double outline_area(const geometer::ProjectedModeGeometry& geometry)
{
    double twice_area = 0.0;
    for (const geometer::ProjectedSegment& segment : geometry.segments)
    {
        twice_area += segment.x1 * segment.y2 - segment.x2 * segment.y1;
    }
    return std::fabs(twice_area) * 0.5;
}

std::vector<double> outline_loop_areas(const geometer::ProjectedModeGeometry& geometry)
{
    std::vector<double> areas;
    std::size_t index = 0;
    while (index < geometry.segments.size())
    {
        const geometer::ProjectedSegment& first = geometry.segments[index];
        const double start_x = first.x1;
        const double start_y = first.y1;
        double current_x = start_x;
        double current_y = start_y;
        double twice_area = 0.0;
        do
        {
            const geometer::ProjectedSegment& segment = geometry.segments[index];
            require(segment.x1 == current_x && segment.y1 == current_y,
                    "outline segments should form contiguous loops");
            twice_area += segment.x1 * segment.y2 - segment.x2 * segment.y1;
            current_x = segment.x2;
            current_y = segment.y2;
            ++index;
            require(index <= geometry.segments.size(), "outline loop should terminate");
        } while ((current_x != start_x || current_y != start_y) &&
                 index < geometry.segments.size());
        require(current_x == start_x && current_y == start_y, "outline loop should be closed");
        areas.push_back(twice_area * 0.5);
    }
    std::sort(areas.begin(), areas.end());
    return areas;
}

std::array<double, 4> outline_bounds(const geometer::ProjectedModeGeometry& geometry)
{
    std::array<double, 4> bounds = {
        std::numeric_limits<double>::infinity(), std::numeric_limits<double>::infinity(),
        -std::numeric_limits<double>::infinity(), -std::numeric_limits<double>::infinity()};
    for (const geometer::ProjectedSegment& segment : geometry.segments)
    {
        bounds[0] = std::min(bounds[0], std::min(segment.x1, segment.x2));
        bounds[1] = std::min(bounds[1], std::min(segment.y1, segment.y2));
        bounds[2] = std::max(bounds[2], std::max(segment.x1, segment.x2));
        bounds[3] = std::max(bounds[3], std::max(segment.y1, segment.y2));
    }
    return bounds;
}

void fast_mesh_shadow_matches_triangle_union()
{
    for (const std::string& fixture :
         {"SOT-23.STEP", "SOIC-8-W.step", "sot223.stp", "TSOT-23-5.STEP", "BGA90-8X13mm.step"})
    {
        for (const std::string& view_id : {"top", "front"})
        {
            geometer::HlrProjectionOptions reference_options =
                projection_options({view_spec(view_id)});
            reference_options.outline_algorithm = geometer::ProjectionOutlineAlgorithm::MeshShadow;
            reference_options.output_detail = false;
            reference_options.output_bbox = false;
            geometer::HlrProjectionOptions fast_options = reference_options;
            fast_options.outline_algorithm = geometer::ProjectionOutlineAlgorithm::FastMeshShadow;

            const geometer::HlrProjectionResult reference_result =
                project_or_throw(fixture, reference_options);
            const geometer::HlrProjectionResult fast_result =
                project_or_throw(fixture, fast_options);
            const geometer::ProjectedModeGeometry& reference =
                find_view(reference_result, view_id).outline;
            const geometer::ProjectedModeGeometry& fast = find_view(fast_result, view_id).outline;
            require(!fast.segments.empty(), "fast mesh-shadow should produce an outline");
            require(non_degree_two_endpoint_count(fast) == 0,
                    "fast mesh-shadow output should contain closed loops");
            const auto reference_bounds = outline_bounds(reference);
            const auto fast_bounds = outline_bounds(fast);
            for (std::size_t index = 0; index < reference_bounds.size(); ++index)
            {
                require(std::fabs(reference_bounds[index] - fast_bounds[index]) <= 0.002,
                        "fast mesh-shadow bounds should match the triangle union");
            }
            const double reference_area = outline_area(reference);
            const double area_tolerance = std::max(0.002, reference_area * 0.001);
            require(std::fabs(reference_area - outline_area(fast)) <= area_tolerance,
                    "fast mesh-shadow area should match the triangle union");
            const std::vector<double> reference_loop_areas = outline_loop_areas(reference);
            const std::vector<double> fast_loop_areas = outline_loop_areas(fast);
            require(reference_loop_areas.size() == fast_loop_areas.size(),
                    "fast mesh-shadow should preserve the triangle-union loop topology");
            for (std::size_t index = 0; index < reference_loop_areas.size(); ++index)
            {
                const double loop_tolerance =
                    std::max(0.002, std::fabs(reference_loop_areas[index]) * 0.001);
                require(std::fabs(reference_loop_areas[index] - fast_loop_areas[index]) <=
                            loop_tolerance,
                        "fast mesh-shadow loop areas should match the triangle union");
            }
        }
    }
}

void output_layers_are_independently_selectable()
{
    geometer::HlrProjectionOptions outline_options = projection_options({view_spec("top")});
    outline_options.outline_algorithm = geometer::ProjectionOutlineAlgorithm::MeshShadow;
    outline_options.output_detail = false;
    outline_options.output_bbox = false;

    const geometer::HlrProjectionResult outline_result =
        project_or_throw("SOT-23.STEP", outline_options);
    const geometer::ProjectedViewGeometry& outline_view = find_view(outline_result, "top");
    require(!outline_view.outline.segments.empty(), "outline-only should produce outline geometry");
    require(outline_view.detail.segments.empty() && outline_view.detail.arcs.empty(),
            "outline-only should leave detail empty");
    require(outline_view.bbox.segments.empty() && outline_view.bbox.arcs.empty(),
            "outline-only should leave bbox empty");
    require(outline_result.timings.hlr_ms == 0.0,
            "mesh-shadow outline-only should bypass detail HLR");

    geometer::HlrProjectionOptions detail_options = projection_options({view_spec("top")});
    detail_options.output_outline = false;
    detail_options.output_bbox = false;

    const geometer::HlrProjectionResult detail_result =
        project_or_throw("SOT-23.STEP", detail_options);
    const geometer::ProjectedViewGeometry& detail_view = find_view(detail_result, "top");
    require(!detail_view.detail.segments.empty(), "detail-only should produce detail geometry");
    require(detail_view.outline.segments.empty() && detail_view.outline.arcs.empty(),
            "detail-only should leave outline empty");
    require(detail_view.bbox.segments.empty() && detail_view.bbox.arcs.empty(),
            "detail-only should leave bbox empty");
}

void fast_detail_projects_real_step_mesh()
{
    geometer::HlrProjectionOptions options = projection_options({view_spec("top")});
    options.projection_algorithm = geometer::ProjectionAlgorithm::Fast;
    options.output_outline = false;
    options.output_bbox = false;

    const geometer::HlrProjectionResult result = project_or_throw("SOT-23.STEP", options);
    const geometer::ProjectedViewGeometry& view = find_view(result, "top");
    require(!view.detail.segments.empty(), "fast detail should emit visible mesh edges");
    require(view.detail.arcs.empty(), "first fast evaluation should emit straight segments only");
    require(view.outline.segments.empty(), "fast detail-only should preserve the outline boundary");
    require(view.bbox.segments.empty(), "fast detail-only should preserve the bbox boundary");
    require(result.timings.mesh_ms > 0.0, "fast detail should report mesh preparation time");
    require(result.timings.hlr_ms > 0.0, "fast detail should report visibility time");
}

void fast_hlr_close_outline_can_run_without_detail()
{
    geometer::HlrProjectionOptions options = projection_options({view_spec("top")});
    options.projection_algorithm = geometer::ProjectionAlgorithm::Fast;
    options.outline_algorithm = geometer::ProjectionOutlineAlgorithm::HlrClosedEdges;
    options.output_detail = false;
    options.output_bbox = false;

    const geometer::HlrProjectionResult result = project_or_throw("SOT-23.STEP", options);
    const geometer::ProjectedViewGeometry& view = find_view(result, "top");
    require(!view.outline.segments.empty() || !view.outline.arcs.empty(),
            "fast plus HLR-close should delegate a nonempty outline without detail");
    require(view.detail.segments.empty() && view.detail.arcs.empty(),
            "outline-only fast request should keep detail empty");
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
        fast_mesh_shadow_matches_triangle_union();
        output_layers_are_independently_selectable();
        fast_detail_projects_real_step_mesh();
        fast_hlr_close_outline_can_run_without_detail();
        root_placement_stripping_matches_step_to_glb_definition_frame();
    }
    catch (const std::exception& e)
    {
        std::cerr << e.what() << "\n";
        clean_exit(1);
    }

    clean_exit(0);
}
