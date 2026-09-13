#include "geometer/mesh_illustration.h"
#include "mesh_illustration_internal.h"

#include <algorithm>

namespace geometer
{
namespace
{
using namespace illustration_detail;

void consume(std::size_t amount, std::size_t& remaining)
{
    if (amount > remaining)
        throw ResourceLimit("Illustration geometry exceeds its aggregate count limit.");
    remaining -= amount;
}

std::vector<double> point(Vec2 value)
{
    if (!std::isfinite(value[0]) || !std::isfinite(value[1]))
        throw std::runtime_error("Illustration geometry coordinates must be finite.");
    return {value[0], value[1]};
}
} // namespace

namespace illustration_detail
{
contracts::MeshIllustrationGeometryA0
make_illustration_geometry(const PreparedIllustration& prepared,
                           const contracts::MeshIllustrationView& view)
{
    contracts::MeshIllustrationGeometryA0 result;
    const auto& commands = prepared.commands;
    const auto& bounds = prepared.scene.bounds;
    const auto& style = prepared.style;
    std::size_t surfaces_left = 2000000, layers_left = 2000000, rings_left = 2000000,
                points_left = 6000000, lines_left = 1000000;
    consume(commands.surfaces.size(), surfaces_left);
    consume(commands.lines.size(), lines_left);
    result.view = view;
    result.bounds.min = point({bounds.min_x, bounds.min_y});
    result.bounds.max = point({bounds.max_x, bounds.max_y});
    const double span = std::max({bounds.max_x - bounds.min_x, bounds.max_y - bounds.min_y, 1e-9});
    result.presentation.background = safe_illustration_color(style.background);
    result.presentation.transparent_background = style.transparent_background;
    result.presentation.padding = span * .06;
    result.presentation.seam_width = span * .003;
    result.stats = commands.stats;
    result.warnings = prepared.scene.warnings;
    result.surfaces.reserve(commands.surfaces.size());
    for (const auto& surface : commands.surfaces)
    {
        consume(surface.layers.size(), layers_left);
        contracts::IllustrationGeometrySurface output;
        output.kind = surface.layered   ? contracts::IllustrationSurfaceKind::layered
                      : surface.polygon ? contracts::IllustrationSurfaceKind::triangle
                                        : contracts::IllustrationSurfaceKind::fused;
        for (const auto& layer : surface.layers)
        {
            consume(layer.rings.size(), rings_left);
            contracts::IllustrationGeometryLayer paint;
            paint.fill = safe_illustration_color(layer.fill);
            paint.opacity = layer.opacity;
            for (const auto& ring : layer.rings)
            {
                consume(ring.size(), points_left);
                if (ring.size() < 3)
                    throw std::runtime_error(
                        "Illustration geometry ring has fewer than three points.");
                std::vector<std::vector<double>> output_ring;
                output_ring.reserve(ring.size());
                for (const auto& value : ring)
                    output_ring.push_back(point(value));
                paint.rings.push_back({std::move(output_ring)});
            }
            output.layers.push_back(std::move(paint));
        }
        result.surfaces.push_back(std::move(output));
    }
    result.lines.reserve(commands.lines.size());
    for (const auto& line : commands.lines)
    {
        if (!std::isfinite(line.width) || line.width < 0)
            throw std::runtime_error(
                "Illustration geometry line width must be finite and nonnegative.");
        contracts::IllustrationGeometryLine output;
        output.start = point(line.points[0]);
        output.end = point(line.points[1]);
        output.color = safe_illustration_color(line.color);
        output.width = line.width;
        result.lines.push_back(std::move(output));
    }
    return result;
}
} // namespace illustration_detail

namespace
{

int render_geometry(const contracts::MeshIllustrationGeometryInputA0& input,
                    const contracts::HlrProjectionResultA0* hlr,
                    contracts::MeshIllustrationGeometryA0* result, Status* status,
                    const MeshIllustrationExecutionLimits& limits)
{
    if (result)
        *result = {};
    if (status)
        *status = {};
    const auto fail = [&](int code, const std::string& message)
    {
        if (result)
            *result = {};
        if (status)
            *status = {code, message};
        return code;
    };
    if (!result)
        return fail(1, "Illustration geometry result pointer is null.");
    try
    {
        const auto prepared = prepare_illustration(input, hlr, limits.max_candidate_comparisons,
                                                   limits.max_drawing_commands);
        *result = make_illustration_geometry(prepared, input.view);
        return 0;
    }
    catch (const ResourceLimit& error)
    {
        return fail(102, error.what());
    }
    catch (const std::bad_alloc&)
    {
        return fail(102, "Illustration geometry allocation failed.");
    }
    catch (const std::exception& error)
    {
        return fail(1, error.what());
    }
}
} // namespace

int illustrate_mesh_geometry(const contracts::MeshIllustrationGeometryInputA0& input,
                             contracts::MeshIllustrationGeometryA0* result, Status* status)
{
    return render_geometry(input, nullptr, result, status, {});
}

int illustrate_mesh_geometry(const contracts::MeshIllustrationGeometryInputA0& input,
                             const contracts::HlrProjectionResultA0& hlr,
                             contracts::MeshIllustrationGeometryA0* result, Status* status)
{
    return render_geometry(input, &hlr, result, status, {});
}

int illustrate_mesh_geometry(const contracts::MeshIllustrationGeometryInputA0& input,
                             const contracts::HlrProjectionResultA0* hlr,
                             const MeshIllustrationExecutionLimits& limits,
                             contracts::MeshIllustrationGeometryA0* result, Status* status)
{
    return render_geometry(input, hlr, result, status, limits);
}
} // namespace geometer
