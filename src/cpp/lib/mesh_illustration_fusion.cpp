#include "mesh_illustration_fusion.h"

#include <algorithm>
#include <unordered_set>

namespace geometer::illustration_detail
{
Surface triangle_surface(const TriangleCommand& command)
{
    const auto& points = command.triangle->points;
    return {true, false, {{{Ring(points.begin(), points.end())}, command.fill, command.opacity}}};
}

namespace
{
struct Mobility
{
    std::vector<std::size_t> low, high;
    std::vector<Pair> overlaps;
};

Mobility mobility_intervals(const std::vector<TriangleCommand>& commands, const Bounds& bounds,
                            WorkBudget& budget)
{
    Mobility result{std::vector<std::size_t>(commands.size(), 0),
                    std::vector<std::size_t>(commands.size(), commands.size() - 1),
                    {}};
    const double width = std::max(bounds.max_x - bounds.min_x, 1e-12);
    const double height = std::max(bounds.max_y - bounds.min_y, 1e-12);
    const double epsilon = std::max(width, height) * 1e-10;
    const auto grid =
        static_cast<unsigned>(clamp(std::ceil(std::sqrt(commands.size() / 4.0)), 8, 192));
    std::vector<Bounds> boxes;
    for (const auto& command : commands)
        boxes.push_back(projected_bounds(*command.triangle));
    candidate_pairs(boxes,
                    {bounds.min_x, bounds.min_y, bounds.min_x + width, bounds.min_y + height}, grid,
                    std::max(64u, grid * 2), 1e-12 / grid, budget,
                    [&](std::size_t front, std::size_t back)
                    {
                        const auto& a = commands[front];
                        const auto& b = commands[back];
                        if (!bounds_overlap(boxes[front], boxes[back], epsilon) ||
                            !significant_overlap(clip_polygon(*a.triangle, *b.triangle, epsilon),
                                                 *a.triangle, *b.triangle, epsilon))
                            return;
                        if (a.fill == b.fill && std::abs(a.opacity - b.opacity) <= 1e-12)
                        {
                            if (result.overlaps.size() >= 4000000)
                                throw ResourceLimit("Mesh illustration exceeds the overlap limit.");
                            result.overlaps.emplace_back(front, back);
                            return;
                        }
                        result.low[front] = std::max(result.low[front], back + 1);
                        result.high[back] = std::min(result.high[back], front - 1);
                    });
    return result;
}

struct Fusion
{
    const std::vector<TriangleCommand>& commands;
    WorkBudget& budget;
    double coordinate_tolerance, depth_tolerance;
    Mobility mobility;
    Disjoint groups, layer_groups;
    std::vector<std::size_t> group_low, group_high;
    std::vector<Pair> candidates, coplanar;

    struct Placement
    {
        std::size_t position, order;
        Surface surface;
    };
    std::vector<Placement> placements;

    Fusion(const std::vector<TriangleCommand>& c, const Bounds& bounds, WorkBudget& b)
        : commands(c), budget(b), mobility(mobility_intervals(c, bounds, b)), groups(c.size()),
          layer_groups(c.size()), group_low(mobility.low), group_high(mobility.high)
    {
        const double span =
            std::max({bounds.max_x - bounds.min_x, bounds.max_y - bounds.min_y, 1e-9});
        coordinate_tolerance = std::max(span * 1e-9, 1e-12);
        double minimum = std::numeric_limits<double>::infinity(), maximum = -minimum;
        for (const auto& command : commands)
            for (double depth : command.triangle->depths)
            {
                minimum = std::min(minimum, depth);
                maximum = std::max(maximum, depth);
            }
        depth_tolerance = std::max((maximum - minimum) * 1e-9, 1e-12);
        if (!std::isfinite(depth_tolerance))
            throw std::runtime_error("Illustration depth range overflow.");
    }

    void find_candidates(bool layer_materials)
    {
        std::vector<std::size_t> all(commands.size());
        std::iota(all.begin(), all.end(), 0);
        const auto edges = fusion_edges(commands, all, coordinate_tolerance, depth_tolerance, true);
        for (const auto& entry : edges.entries)
        {
            const auto& values = entry.second;
            if (values.size() != 2)
                continue;
            const auto& a = values[0];
            const auto& b = values[1];
            const auto& ca = commands[a.triangle];
            const auto& cb = commands[b.triangle];
            if (a.start_key != b.end_key || a.end_key != b.start_key ||
                cross2(a.start, a.end, a.third) * cross2(a.start, a.end, b.third) >=
                    -coordinate_tolerance * coordinate_tolerance)
                continue;
            if (ca.fill == cb.fill && std::abs(ca.opacity - cb.opacity) <= 1e-12)
                candidates.emplace_back(a.triangle, b.triangle);
            if (layer_materials && ca.opacity >= 1 - 1e-12 && cb.opacity >= 1 - 1e-12 &&
                dot(ca.triangle->geometric_normal, cb.triangle->geometric_normal) >= 1 - 1e-10)
                coplanar.emplace_back(a.triangle, b.triangle);
        }
        const auto distance = [](Pair pair)
        { return pair.first > pair.second ? pair.first - pair.second : pair.second - pair.first; };
        const auto less = [&](Pair a, Pair b) { return distance(a) < distance(b); };
        std::stable_sort(candidates.begin(), candidates.end(), less);
        std::stable_sort(coplanar.begin(), coplanar.end(), less);
        for (auto pair : candidates)
        {
            const auto a = groups.find(pair.first), b = groups.find(pair.second);
            const auto low = std::max(group_low[a], group_low[b]);
            const auto high = std::min(group_high[a], group_high[b]);
            if (a == b || low > high)
                continue;
            groups.parent[b] = a;
            group_low[a] = low;
            group_high[a] = high;
        }
        for (auto pair : coplanar)
            layer_groups.unite(pair.first, pair.second);
    }

    static std::size_t placement(const std::vector<std::size_t>& members, std::size_t low,
                                 std::size_t high)
    {
        double sum = 0;
        for (auto member : members)
            sum += member;
        return std::max(low,
                        std::min(high, static_cast<std::size_t>(js_round(sum / members.size()))));
    }

    std::vector<bool> place_layers()
    {
        std::vector<std::vector<std::size_t>> adjacency(commands.size());
        for (auto pair : candidates)
        {
            adjacency[pair.first].push_back(pair.second);
            adjacency[pair.second].push_back(pair.first);
        }
        OrderedGroups<std::size_t, std::size_t> components;
        std::vector<bool> seen(commands.size(), false), consumed(commands.size(), false);
        for (auto pair : coplanar)
            for (auto triangle : {pair.first, pair.second})
                if (!seen[triangle])
                {
                    seen[triangle] = true;
                    components.add(layer_groups.find(triangle), triangle);
                }
        for (const auto& entry : components.entries)
        {
            const auto& members = entry.second;
            std::size_t low = 0, high = commands.size() - 1;
            for (auto member : members)
            {
                low = std::max(low, mobility.low[member]);
                high = std::min(high, mobility.high[member]);
            }
            if (members.size() < 2 || low > high)
                continue;
            auto surface = layered_surface(commands, members, adjacency, coordinate_tolerance,
                                           depth_tolerance, budget);
            if (!surface)
                continue;
            for (auto member : members)
                consumed[member] = true;
            placements.push_back({placement(members, low, high), members[0], std::move(*surface)});
        }
        return consumed;
    }

    std::vector<Surface> finish()
    {
        const auto consumed = place_layers();
        OrderedGroups<std::size_t, std::size_t> components;
        for (std::size_t i = 0; i < commands.size(); ++i)
            components.add(groups.find(i), i);
        std::unordered_set<std::size_t> unsafe;
        for (auto pair : mobility.overlaps)
            if (groups.find(pair.first) == groups.find(pair.second))
                unsafe.insert(groups.find(pair.first));
        for (const auto& entry : components.entries)
        {
            const auto root = entry.first;
            std::vector<std::size_t> available;
            for (auto member : entry.second)
                if (!consumed[member])
                    available.push_back(member);
            if (available.empty())
                continue;
            std::optional<Surface> fused;
            if (available.size() > 1 && unsafe.find(root) == unsafe.end())
                fused = fused_component(commands, available, coordinate_tolerance, depth_tolerance,
                                        budget);
            if (fused)
                placements.push_back({placement(available, group_low[root], group_high[root]),
                                      available[0], std::move(*fused)});
            else
                for (auto member : available)
                    placements.push_back({member, member, triangle_surface(commands[member])});
        }
        std::stable_sort(
            placements.begin(), placements.end(), [](const auto& a, const auto& b)
            { return a.position != b.position ? a.position < b.position : a.order < b.order; });
        std::vector<Surface> result;
        for (auto& placement : placements)
            result.push_back(std::move(placement.surface));
        return result;
    }
};
} // namespace

std::vector<Surface> fuse_triangles(const std::vector<TriangleCommand>& commands,
                                    const Bounds& bounds, bool layer_materials, WorkBudget& budget)
{
    if (commands.size() < 2)
    {
        std::vector<Surface> result;
        for (const auto& command : commands)
            result.push_back(triangle_surface(command));
        return result;
    }
    Fusion fusion(commands, bounds, budget);
    fusion.find_candidates(layer_materials);
    return fusion.finish();
}
} // namespace geometer::illustration_detail
