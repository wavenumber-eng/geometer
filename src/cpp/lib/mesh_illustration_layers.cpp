#include "mesh_illustration_fusion.h"

#include <algorithm>
#include <unordered_set>

namespace geometer::illustration_detail
{
std::string surface_style_key(const TriangleCommand& command)
{
    return command.fill + '\0' + fixed_text(command.opacity);
}

namespace
{
bool multiple_coplanar_materials(const std::vector<TriangleCommand>& commands,
                                 const std::vector<std::size_t>& members, double tolerance)
{
    const auto& reference = *commands[members[0]].triangle;
    std::unordered_set<std::string> materials;
    for (auto member : members)
    {
        const auto& triangle = *commands[member].triangle;
        if (dot(triangle.geometric_normal, reference.geometric_normal) < 1 - 1e-10)
            return false;
        for (unsigned i = 0; i < 3; ++i)
            if (std::abs(triangle.depths[i] - depth_at(reference, triangle.points[i])) > tolerance)
                return false;
        materials.insert(fixed_text(triangle.color[0]) + "," + fixed_text(triangle.color[1]) + "," +
                         fixed_text(triangle.color[2]) + "|" + fixed_text(triangle.opacity));
    }
    return materials.size() >= 2;
}

std::vector<std::vector<std::size_t>>
style_components(const std::vector<std::size_t>& members,
                 const std::vector<std::vector<std::size_t>>& adjacency)
{
    std::unordered_map<std::size_t, std::size_t> local;
    for (std::size_t i = 0; i < members.size(); ++i)
        local.emplace(members[i], i);
    Disjoint groups(members.size());
    for (std::size_t i = 0; i < members.size(); ++i)
        for (auto neighbor : adjacency[members[i]])
        {
            const auto found = local.find(neighbor);
            if (found != local.end() && found->second > i)
                groups.unite(i, found->second);
        }
    OrderedGroups<std::size_t, std::size_t> components;
    for (std::size_t i = 0; i < members.size(); ++i)
        components.add(groups.find(i), members[i]);
    std::vector<std::vector<std::size_t>> result;
    for (auto& entry : components.entries)
        result.push_back(std::move(entry.second));
    std::sort(result.begin(), result.end(),
              [](const auto& a, const auto& b) { return a[0] < b[0]; });
    return result;
}
} // namespace

std::optional<Surface> layered_surface(const std::vector<TriangleCommand>& commands,
                                       const std::vector<std::size_t>& members,
                                       const std::vector<std::vector<std::size_t>>& adjacency,
                                       double coordinate_tolerance, double depth_tolerance,
                                       WorkBudget& budget)
{
    if (!multiple_coplanar_materials(commands, members,
                                     std::max(coordinate_tolerance, depth_tolerance) * 8))
        return {};
    struct Area
    {
        std::string key;
        double area;
        std::size_t first;
    };
    std::vector<Area> areas;
    std::unordered_map<std::string, std::size_t> area_indices;
    for (auto member : members)
    {
        const auto key = surface_style_key(commands[member]);
        const double area = std::abs(signed_area(commands[member].triangle->points));
        const auto entry = area_indices.emplace(key, areas.size());
        if (entry.second)
            areas.push_back({key, area, member});
        else
            areas[entry.first->second].area += area;
    }
    if (areas.size() < 2)
        return {};
    auto footprint =
        fused_component(commands, members, coordinate_tolerance, depth_tolerance, budget);
    if (!footprint)
        return {};
    const auto base =
        std::min_element(areas.begin(), areas.end(), [](const auto& a, const auto& b)
                         { return a.area != b.area ? a.area > b.area : a.first < b.first; });
    auto& base_layer = footprint->layers[0];
    base_layer.fill = commands[base->first].fill;
    base_layer.opacity = commands[base->first].opacity;
    footprint->layered = true;
    for (const auto& component : style_components(members, adjacency))
    {
        const auto& first = commands[component[0]];
        if (surface_style_key(first) == base->key)
            continue;
        if (component.size() == 1)
        {
            Ring ring;
            for (auto index : oriented_indices(*first.triangle))
                ring.push_back(first.triangle->points[index]);
            footprint->layers.push_back({{std::move(ring)}, first.fill, first.opacity});
        }
        else
        {
            auto fused =
                fused_component(commands, component, coordinate_tolerance, depth_tolerance, budget);
            if (!fused)
                return {};
            footprint->layers.push_back(std::move(fused->layers[0]));
        }
    }
    if (footprint->layers.size() < 2)
        return {};
    return footprint;
}
} // namespace geometer::illustration_detail
