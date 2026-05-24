#include "geometer/projection.h"

#include <cmath>
#include <cstdlib>
#include <exception>
#include <fstream>
#include <iostream>
#include <limits>
#include <stdexcept>
#include <string>
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
    // Match the CLI: OCCT static teardown can crash after successful HLR work
    // on Windows, which would hide the test result.
    std::cout.flush();
    std::cerr.flush();
    std::_Exit(code);
}

std::vector<unsigned char> read_fixture_bytes()
{
    const std::string path =
        std::string(GEOMETER_TEST_SOURCE_DIR) + "/tests/fixtures/step/embedded_models/SOT-23.STEP";
    std::ifstream input(path, std::ios::binary);
    require(input.good(), "STEP fixture should be readable: " + path);
    return std::vector<unsigned char>(std::istreambuf_iterator<char>(input),
                                      std::istreambuf_iterator<char>());
}

struct Bounds
{
    double min_x = std::numeric_limits<double>::infinity();
    double min_y = std::numeric_limits<double>::infinity();
    double max_x = -std::numeric_limits<double>::infinity();
    double max_y = -std::numeric_limits<double>::infinity();

    void add(double x, double y)
    {
        min_x = std::min(min_x, x);
        min_y = std::min(min_y, y);
        max_x = std::max(max_x, x);
        max_y = std::max(max_y, y);
    }
};

Bounds detail_bounds(const geometer::HlrProjectionResult& result)
{
    require(!result.views.empty(), "projection should contain one view");
    const geometer::ProjectedModeGeometry& detail = result.views[0].detail;
    Bounds bounds;
    for (const geometer::ProjectedSegment& segment : detail.segments)
    {
        bounds.add(segment.x1, segment.y1);
        bounds.add(segment.x2, segment.y2);
    }
    for (const geometer::ProjectedArc& arc : detail.arcs)
    {
        bounds.add(arc.start[0], arc.start[1]);
        bounds.add(arc.end[0], arc.end[1]);
        bounds.add(arc.center[0], arc.center[1]);
    }
    require(std::isfinite(bounds.min_x), "detail projection should contain geometry");
    return bounds;
}

geometer::HlrProjectionOptions projection_options()
{
    geometer::ProjectionViewSpec top;
    top.id = "top";
    top.direction = {0.0, 0.0, 1.0};
    top.up = {0.0, 1.0, 0.0};

    geometer::HlrProjectionOptions options;
    options.views = {top};
    options.curve_mode = geometer::ProjectionCurveMode::Polyline;
    options.projection_algorithm = geometer::ProjectionAlgorithm::Exact;
    options.round_digits = 3;
    return options;
}

geometer::HlrProjectionResult project_or_throw(const std::vector<unsigned char>& step_bytes,
                                               const geometer::HlrProjectionOptions& options)
{
    geometer::HlrProjectionResult result;
    geometer::Status status;
    const int code = geometer::step_hlr_projection_from_bytes(step_bytes.data(), step_bytes.size(),
                                                              options, &result, &status);
    require(code == 0, "projection should succeed: " + status.message);
    return result;
}

void model_transform_projects_nonempty_geometry()
{
    const std::vector<unsigned char> step_bytes = read_fixture_bytes();

    const geometer::HlrProjectionResult base = project_or_throw(step_bytes, projection_options());
    const Bounds base_bounds = detail_bounds(base);

    geometer::HlrProjectionOptions moved_options = projection_options();
    moved_options.model_transform[3] = 10.0;
    moved_options.model_transform[7] = 20.0;
    const geometer::HlrProjectionResult moved = project_or_throw(step_bytes, moved_options);
    const Bounds moved_bounds = detail_bounds(moved);

    constexpr double tolerance = 0.01;
    require(std::fabs((moved_bounds.min_x - base_bounds.min_x) - 10.0) < tolerance,
            "model_transform should translate projected X bounds");
    require(std::fabs((moved_bounds.min_y - base_bounds.min_y) - 20.0) < tolerance,
            "model_transform should translate projected Y bounds");
}

} // namespace

int main()
{
    try
    {
        model_transform_projects_nonempty_geometry();
    }
    catch (const std::exception& e)
    {
        std::cerr << e.what() << "\n";
        clean_exit(1);
    }

    clean_exit(0);
}
