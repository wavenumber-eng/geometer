#include "geometer/mesh_illustration.h"
#include "mesh_illustration_internal.h"

#include <algorithm>

namespace geometer::illustration_detail
{
Commands render_commands(const Scene& scene, const Style& style, WorkBudget& budget)
{
    Commands result;
    std::vector<TriangleCommand> triangles;
    std::size_t order = 0;
    for (const auto& triangle : scene.triangles)
    {
        if (!triangle.front && !(style.double_sided || triangle.double_sided))
            continue;
        triangles.push_back({&triangle, triangle.depth, order++,
                             triangle_fill(triangle, scene, style), triangle.opacity});
    }
    result.stats.triangles = static_cast<std::uint32_t>(triangles.size());
    const double span = std::max(
        {scene.bounds.max_x - scene.bounds.min_x, scene.bounds.max_y - scene.bounds.min_y, 1e-9});
    const double threshold =
        std::cos(clamp(style.crease_angle_degrees, 0, 180) * std::acos(-1.0) / 180);
    for (const auto& edge : scene.edges)
    {
        const bool outline = style.show_outlines && edge.front_b && edge.front_a != *edge.front_b;
        const bool crease = style.show_creases && edge.front_a && edge.front_b.value_or(false) &&
                            edge.normal_b && dot(edge.normal_a, *edge.normal_b) < threshold;
        if (!outline && !crease)
            continue;
        result.lines.push_back({edge.points, outline ? style.outline_color : style.crease_color,
                                span * (outline ? style.outline_width : style.crease_width),
                                edge.depth, order++});
        if (outline)
            ++result.stats.outlines;
        else
            ++result.stats.creases;
    }
    const auto ordered = order_triangles(triangles, scene.bounds, budget);
    if (style.fuse_surfaces)
        result.surfaces =
            fuse_triangles(ordered, scene.bounds, style.layer_coplanar_materials, budget);
    else
        for (const auto& triangle : ordered)
            result.surfaces.push_back(triangle_surface(triangle));
    for (const auto& surface : result.surfaces)
    {
        result.stats.surface_draws += static_cast<std::uint32_t>(surface.layers.size());
        if (surface.layered)
            ++result.stats.layered_surfaces;
    }
    std::sort(result.lines.begin(), result.lines.end(), [](const auto& a, const auto& b)
              { return a.depth != b.depth ? a.depth < b.depth : a.order < b.order; });
    result.stats.commands =
        result.stats.surface_draws + static_cast<std::uint32_t>(result.lines.size());
    return result;
}
} // namespace geometer::illustration_detail

namespace geometer
{
namespace
{
int render(const contracts::MeshIllustrationInputA0& input,
           const contracts::HlrProjectionResultA0* hlr, contracts::MeshIllustrationResultA0* result,
           Status* status)
{
    if (result)
        *result = {};
    if (status)
        *status = {};
    const auto fail = [&](int code, const std::string& message)
    {
        if (status)
            *status = {code, message};
        return code;
    };
    if (!result)
        return fail(1, "Mesh illustration result pointer is null.");
    try
    {
        // Reuse generated TypeSpec validation for direct callers as well as IPC.
        std::string validated;
        contracts::ContractError error;
        if (!contracts::encode_json(input, &validated, &error))
            return fail(1, error.message);
        validated.clear();
        validated.shrink_to_fit();
        const auto scene = illustration_detail::prepare_scene(input);
        const auto style = illustration_detail::resolve_style(
            input.style.value_or(contracts::MeshIllustrationStyleA0{}));
        illustration_detail::WorkBudget budget;
        auto commands = illustration_detail::render_commands(scene, style, budget);
        if (hlr)
            illustration_detail::append_hlr(input, *hlr, scene, style, commands);
        auto svg = illustration_detail::render_svg(
            scene, style, commands, input.svg.value_or(contracts::MeshIllustrationSvgOptions{}));
        result->svg = std::move(svg);
        result->stats = commands.stats;
        result->warnings = scene.warnings;
        return 0;
    }
    catch (const illustration_detail::ResourceLimit& error)
    {
        *result = {};
        return fail(102, error.what());
    }
    catch (const std::bad_alloc&)
    {
        *result = {};
        return fail(102, "Mesh illustration allocation failed.");
    }
    catch (const std::exception& error)
    {
        *result = {};
        return fail(1, error.what());
    }
}
} // namespace

int illustrate_mesh(const contracts::MeshIllustrationInputA0& input,
                    contracts::MeshIllustrationResultA0* result, Status* status)
{
    return render(input, nullptr, result, status);
}

int illustrate_mesh(const contracts::MeshIllustrationInputA0& input,
                    const contracts::HlrProjectionResultA0& hlr,
                    contracts::MeshIllustrationResultA0* result, Status* status)
{
    return render(input, &hlr, result, status);
}
} // namespace geometer
