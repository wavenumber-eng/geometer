#include "mesh_illustration_fusion.h"

#include <algorithm>
#include <queue>
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
    // Each overlapping pair of different paint, as (behind, in front).
    std::vector<Pair> ordered;
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
                            result.overlaps.emplace_back(front, back);
                            return;
                        }
                        result.ordered.emplace_back(back, front);
                        result.low[front] = std::max(result.low[front], back + 1);
                        result.high[back] = std::min(result.high[back], front - 1);
                    });
    return result;
}

// Draw order for `count` placements, today's order 0..count-1, in which each
// (behind, in front) edge's behind comes first. Placements that must each come
// before the other (a cycle) keep today's order among themselves, at the place
// of their first. Today's order is kept wherever it already respects the edges.
std::vector<std::size_t> respect_overlaps(std::size_t count, std::vector<Pair> edges)
{
    std::vector<std::size_t> result(count);
    std::iota(result.begin(), result.end(), 0);
    std::sort(edges.begin(), edges.end());
    edges.erase(std::unique(edges.begin(), edges.end()), edges.end());
    if (std::all_of(edges.begin(), edges.end(), [](Pair edge) { return edge.first < edge.second; }))
        return result;
    std::vector<std::vector<std::size_t>> outgoing(count), incoming(count);
    for (auto [behind, front] : edges)
    {
        outgoing[behind].push_back(front);
        incoming[front].push_back(behind);
    }
    // Strongly connected placements (Kosaraju), each named by its first.
    std::vector<bool> visited(count, false);
    std::vector<std::size_t> finish;
    for (std::size_t start = 0; start < count; ++start)
    {
        if (visited[start])
            continue;
        std::vector<std::pair<std::size_t, std::size_t>> stack{{start, 0}};
        visited[start] = true;
        while (!stack.empty())
        {
            auto& top = stack.back();
            const auto node = top.first;
            if (top.second < outgoing[node].size())
            {
                const auto next = outgoing[node][top.second++];
                if (!visited[next])
                {
                    visited[next] = true;
                    stack.emplace_back(next, 0);
                }
            }
            else
            {
                finish.push_back(node);
                stack.pop_back();
            }
        }
    }
    std::vector<std::size_t> owner(count, count);
    for (auto it = finish.rbegin(); it != finish.rend(); ++it)
    {
        if (owner[*it] != count)
            continue;
        std::vector<std::size_t> stack{*it};
        owner[*it] = *it;
        std::size_t first = *it;
        std::vector<std::size_t> members;
        while (!stack.empty())
        {
            const auto node = stack.back();
            stack.pop_back();
            members.push_back(node);
            first = std::min(first, node);
            for (auto next : incoming[node])
                if (owner[next] == count)
                {
                    owner[next] = *it;
                    stack.push_back(next);
                }
        }
        for (auto member : members)
            owner[member] = first;
    }
    std::vector<std::vector<std::size_t>> members(count), targets(count);
    std::vector<std::size_t> waiting(count, 0);
    for (std::size_t node = 0; node < count; ++node)
        members[owner[node]].push_back(node);
    for (auto [behind, front] : edges)
        if (owner[behind] != owner[front])
        {
            targets[owner[behind]].push_back(owner[front]);
            ++waiting[owner[front]];
        }
    std::priority_queue<std::size_t, std::vector<std::size_t>, std::greater<>> ready;
    for (std::size_t node = 0; node < count; ++node)
        if (owner[node] == node && waiting[node] == 0)
            ready.push(node);
    result.clear();
    while (!ready.empty())
    {
        const auto group = ready.top();
        ready.pop();
        result.insert(result.end(), members[group].begin(), members[group].end());
        for (auto front : targets[group])
            if (--waiting[front] == 0)
                ready.push(front);
    }
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
    // The placement each triangle is drawn in.
    std::vector<std::size_t> owner;

    Fusion(const std::vector<TriangleCommand>& c, const Bounds& bounds, WorkBudget& b)
        : commands(c), budget(b), mobility(mobility_intervals(c, bounds, b)), groups(c.size()),
          layer_groups(c.size()), group_low(mobility.low), group_high(mobility.high),
          owner(c.size(), 0)
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

    std::vector<bool> place_layers(WorkBudget& budget)
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
            {
                consumed[member] = true;
                owner[member] = placements.size();
            }
            budget.consume_commands(surface->layers.size());
            placements.push_back({placement(members, low, high), members[0], std::move(*surface)});
        }
        return consumed;
    }

    std::vector<Surface> finish(WorkBudget& budget)
    {
        const auto consumed = place_layers(budget);
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
            {
                // Fusion emits one command when successful. Admit that minimum
                // before constructing edge maps and boundary rings.
                budget.require_commands();
                fused = fused_component(commands, available, coordinate_tolerance, depth_tolerance,
                                        budget);
            }
            if (fused)
            {
                budget.consume_commands(fused->layers.size());
                for (auto member : available)
                    owner[member] = placements.size();
                placements.push_back({placement(available, group_low[root], group_high[root]),
                                      available[0], std::move(*fused)});
            }
            else
                for (auto member : available)
                {
                    budget.consume_commands();
                    owner[member] = placements.size();
                    placements.push_back({member, member, triangle_surface(commands[member])});
                }
        }
        // Today's order: by position, then order. Merged surfaces are placed by
        // their members' mean, which can put one before a surface it must be
        // drawn behind; the overlaps of different paint then decide.
        std::vector<std::size_t> sorted(placements.size()), rank(placements.size());
        std::iota(sorted.begin(), sorted.end(), 0);
        std::stable_sort(sorted.begin(), sorted.end(),
                         [&](std::size_t a, std::size_t b)
                         {
                             const auto& pa = placements[a];
                             const auto& pb = placements[b];
                             return pa.position != pb.position ? pa.position < pb.position
                                                               : pa.order < pb.order;
                         });
        for (std::size_t index = 0; index < sorted.size(); ++index)
            rank[sorted[index]] = index;
        std::vector<Pair> edges;
        for (auto [behind, front] : mobility.ordered)
            if (owner[behind] != owner[front])
                edges.emplace_back(rank[owner[behind]], rank[owner[front]]);
        std::vector<Surface> result;
        for (auto index : respect_overlaps(placements.size(), std::move(edges)))
            result.push_back(std::move(placements[sorted[index]].surface));
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
        {
            budget.consume_commands();
            result.push_back(triangle_surface(command));
        }
        return result;
    }
    Fusion fusion(commands, bounds, budget);
    fusion.find_candidates(layer_materials);
    return fusion.finish(budget);
}
} // namespace geometer::illustration_detail
