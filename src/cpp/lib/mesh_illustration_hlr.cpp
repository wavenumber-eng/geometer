#include "mesh_illustration_internal.h"

#include <algorithm>

namespace geometer::illustration_detail
{
void append_hlr(const contracts::MeshIllustrationInputA0& input,
                const contracts::HlrProjectionResultA0& hlr, const Scene& scene, const Style& style,
                Commands& commands)
{
    // Enforce the work limit before generated validation serializes large data.
    std::size_t count = 0;
    for (const auto& view : hlr.views)
        for (const auto* layer : {&view.modes.outline, &view.modes.detail, &view.modes.bbox})
        {
            if (layer->segments.size() > 1000000 - count)
                throw ResourceLimit("Illustration HLR exceeds 1,000,000 segments.");
            count += layer->segments.size();
            if (!layer->arcs.empty())
                throw std::runtime_error(
                    "Illustration HLR requires polyline geometry; arcs are unsupported.");
        }
    std::string validated;
    contracts::ContractError error;
    if (!contracts::encode_json(hlr, &validated, &error))
        throw std::runtime_error(error.message);
    if (hlr.units != "mm" || hlr.views.size() != 1)
        throw std::runtime_error("Illustration HLR requires millimeters and exactly one view.");
    const auto& view = hlr.views.front();
    const auto direction = normalize(vector3(view.direction), "HLR direction");
    const auto right =
        normalize(cross(normalize(vector3(view.up), "HLR up"), direction), "HLR right");
    const auto expected_right = normalize(
        cross(normalize(vector3(input.view.up), "View up"), scene.direction), "View right");
    for (std::size_t i = 0; i < 3; ++i)
        if (std::abs(direction[i] - scene.direction[i]) > 1e-9 ||
            std::abs(right[i] - expected_right[i]) > 1e-9)
            throw std::runtime_error(
                "Illustration HLR view does not match the mesh projection basis.");
    const double span = std::max(
        {scene.bounds.max_x - scene.bounds.min_x, scene.bounds.max_y - scene.bounds.min_y, 1e-9});
    const double mirror = input.view.mirror_x.value_or(false) ? -1 : 1;
    const auto append = [&](const contracts::ProjectedGeometry& layer, const std::string& color,
                            double width, std::uint32_t& stat)
    {
        for (const auto& segment : layer.segments)
        {
            commands.lines.push_back(
                {{{{mirror * segment[0], segment[1]}, {mirror * segment[2], segment[3]}}},
                 color,
                 span * width,
                 std::numeric_limits<double>::max(),
                 commands.lines.size()});
            ++stat;
            ++commands.stats.commands;
        }
    };
    // Same ordering as TypeScript: surfaces, diagnostic raw edges, HLR detail,
    // HLR outline. Keep native HLR visibility decisions; never redraw hidden edges.
    if (style.show_hlr_detail)
        append(view.modes.detail, style.crease_color, style.crease_width, commands.stats.details);
    if (style.show_hlr_outline)
        append(view.modes.outline, style.outline_color, style.outline_width,
               commands.stats.outlines);
}
} // namespace geometer::illustration_detail
