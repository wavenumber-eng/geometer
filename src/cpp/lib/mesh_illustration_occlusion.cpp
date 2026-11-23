#include "mesh_illustration_internal.h"

#include <algorithm>

namespace geometer::illustration_detail
{
namespace
{
// Occlusion cells per view span. The cell grid is tied to the view span, the
// base every illustration tolerance is derived from; it sets how much hidden
// geometry the cull finds, never whether it may drop visible geometry.
constexpr double occlusion_cells_per_span = 512;
// Each drawn surface is also stroked with a seam of 0.003 of the span (the SVG
// and geometry renderers). Half of it falls outside the surface, and the SVG
// grid rounds it by at most one unit of its coarsest grid (a ten-thousandth
// of the span), so a triangle's paint reaches that far beyond it.
constexpr double seam_reach_per_span = 0.003 / 2 + 1.0 / 10000;

struct Span
{
    bool found = false;
    double low = 0, high = 0;
    void include(double x)
    {
        low = found ? std::min(low, x) : x;
        high = found ? std::max(high, x) : x;
        found = true;
    }
};

// The triangle's horizontal extent on the line at `y`.
Span span_at(const std::array<Vec2, 3>& points, double y)
{
    Span span;
    for (unsigned i = 0; i < 3; ++i)
    {
        const auto a = points[i], b = points[(i + 1) % 3];
        if (y < std::min(a[1], b[1]) || y > std::max(a[1], b[1]))
            continue;
        if (a[1] == b[1])
        {
            span.include(a[0]);
            span.include(b[0]);
        }
        else
            span.include(a[0] + (y - a[1]) * (b[0] - a[0]) / (b[1] - a[1]));
    }
    return span;
}

// The triangle's horizontal extent within the band from `low` to `high`.
Span band_extent(const std::array<Vec2, 3>& points, double low, double high)
{
    Span extent;
    for (double y : {low, high})
    {
        const auto span = span_at(points, y);
        if (span.found)
        {
            extent.include(span.low);
            extent.include(span.high);
        }
    }
    for (const auto& point : points)
        if (point[1] >= low && point[1] <= high)
            extent.include(point[0]);
    return extent;
}

struct Grid
{
    double min_x, min_y, cell;
    long long columns, rows;

    long long index(double value, double origin, long long count) const
    {
        const double cell_index = std::floor((value - origin) / cell);
        return static_cast<long long>(clamp(cell_index, 0, static_cast<double>(count - 1)));
    }
    long long column(double x) const
    {
        return index(x, min_x, columns);
    }
    long long row(double y) const
    {
        return index(y, min_y, rows);
    }
};
} // namespace

std::vector<TriangleCommand> cull_hidden_triangles(const std::vector<TriangleCommand>& commands,
                                                   const Bounds& bounds)
{
    // Drop only triangles the view proves hidden. An opaque triangle that
    // covers a whole cell, each corner inside it by more than the coordinate
    // tolerance, occludes that cell down to its farthest corner depth there.
    // A triangle is hidden when every cell its paint (grown by the seam reach)
    // touches is occluded nearer than its nearest vertex by more than the depth
    // tolerance. A partly covered cell, a near tie or a translucent occluder
    // keeps it, so painting the survivors draws the same picture.
    if (commands.size() < 2)
        return commands;
    const double width = std::max(bounds.max_x - bounds.min_x, 1e-12);
    const double height = std::max(bounds.max_y - bounds.min_y, 1e-12);
    const double span = std::max(width, height);
    const double coordinate_tolerance = std::max(span * 1e-9, 1e-12);
    double minimum = std::numeric_limits<double>::infinity(), maximum = -minimum;
    for (const auto& command : commands)
        for (double depth : command.triangle->depths)
        {
            minimum = std::min(minimum, depth);
            maximum = std::max(maximum, depth);
        }
    const double depth_tolerance = std::max((maximum - minimum) * 1e-9, 1e-12);
    if (!std::isfinite(depth_tolerance))
        throw std::runtime_error("Illustration depth range overflow.");
    const double cell = span / occlusion_cells_per_span;
    const double reach = span * seam_reach_per_span + coordinate_tolerance;
    const Grid grid{bounds.min_x, bounds.min_y, cell,
                    std::max(1LL, static_cast<long long>(std::ceil(width / cell))),
                    std::max(1LL, static_cast<long long>(std::ceil(height / cell)))};
    std::vector<double> occluded(static_cast<std::size_t>(grid.columns * grid.rows),
                                 -std::numeric_limits<double>::infinity());
    bool any = false;
    for (const auto& command : commands)
    {
        if (command.opacity < 1 - 1e-12)
            continue;
        const auto& triangle = *command.triangle;
        const auto box = projected_bounds(triangle);
        for (long long row = grid.row(box.min_y); row <= grid.row(box.max_y); ++row)
        {
            const double low = grid.min_y + static_cast<double>(row) * cell, high = low + cell;
            const auto lower = span_at(triangle.points, low),
                       upper = span_at(triangle.points, high);
            if (!lower.found || !upper.found)
                continue;
            const double left = std::max(lower.low, upper.low) + coordinate_tolerance;
            const double right = std::min(lower.high, upper.high) - coordinate_tolerance;
            if (right <= left)
                continue;
            const auto first =
                static_cast<long long>(std::max(std::ceil((left - grid.min_x) / cell), 0.0));
            const auto last =
                static_cast<long long>(std::min(std::floor((right - grid.min_x) / cell) - 1,
                                                static_cast<double>(grid.columns - 1)));
            for (long long column = first; column <= last; ++column)
            {
                const double x0 = grid.min_x + static_cast<double>(column) * cell, x1 = x0 + cell;
                const double depth =
                    std::min({depth_at(triangle, {x0, low}), depth_at(triangle, {x1, low}),
                              depth_at(triangle, {x0, high}), depth_at(triangle, {x1, high})});
                auto& value = occluded[static_cast<std::size_t>(row * grid.columns + column)];
                value = std::max(value, depth);
                any = true;
            }
        }
    }
    if (!any)
        return commands;
    std::vector<TriangleCommand> result;
    result.reserve(commands.size());
    for (const auto& command : commands)
    {
        const auto& triangle = *command.triangle;
        const double nearest =
            std::max({triangle.depths[0], triangle.depths[1], triangle.depths[2]});
        const auto box = projected_bounds(triangle);
        bool tested = false, hidden = true;
        for (long long row = grid.row(box.min_y - reach);
             hidden && row <= grid.row(box.max_y + reach); ++row)
        {
            const double low = grid.min_y + static_cast<double>(row) * cell, high = low + cell;
            const auto extent = band_extent(triangle.points, std::max(low - reach, box.min_y),
                                            std::min(high + reach, box.max_y));
            if (!extent.found)
                continue;
            for (long long column = grid.column(extent.low - reach);
                 column <= grid.column(extent.high + reach); ++column)
            {
                tested = true;
                if (!(occluded[static_cast<std::size_t>(row * grid.columns + column)] >
                      nearest + depth_tolerance))
                {
                    hidden = false;
                    break;
                }
            }
        }
        if (!(tested && hidden))
            result.push_back(command);
    }
    return result;
}
} // namespace geometer::illustration_detail
