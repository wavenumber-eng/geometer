#include "mesh_illustration_internal.h"

#include <algorithm>
#include <numeric>
#include <queue>
#include <unordered_set>

namespace geometer::illustration_detail
{
namespace
{
using Graph = std::vector<std::vector<std::size_t>>;
struct Components
{
    Graph members;
    std::vector<std::size_t> owner;
};

Components strongly_connected(const Graph& outgoing, const Graph& reverse)
{
    std::vector<bool> visited(outgoing.size(), false);
    std::vector<std::size_t> finish;
    for (std::size_t start = 0; start < outgoing.size(); ++start)
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
    Components result;
    result.owner.assign(outgoing.size(), outgoing.size());
    for (auto it = finish.rbegin(); it != finish.rend(); ++it)
    {
        const auto start = *it;
        if (result.owner[start] != outgoing.size())
            continue;
        const auto component = result.members.size();
        std::vector<std::size_t> members, stack{start};
        result.owner[start] = component;
        while (!stack.empty())
        {
            const auto node = stack.back();
            stack.pop_back();
            members.push_back(node);
            for (auto next : reverse[node])
            {
                if (result.owner[next] != outgoing.size())
                    continue;
                result.owner[next] = component;
                stack.push_back(next);
            }
        }
        result.members.push_back(std::move(members));
    }
    return result;
}

int overlap_order(const Triangle& a, const Triangle& b, double epsilon)
{
    const auto overlap = clip_polygon(a, b, epsilon);
    if (!significant_overlap(overlap, a, b, epsilon))
        return 0;
    double minimum = std::numeric_limits<double>::infinity(), maximum = -minimum, depth = 1;
    for (auto point : overlap)
    {
        const double da = depth_at(a, point), db = depth_at(b, point);
        minimum = std::min(minimum, da - db);
        maximum = std::max(maximum, da - db);
        depth = std::max({depth, std::abs(da), std::abs(db)});
    }
    const double depth_epsilon = depth * 1e-10;
    if (minimum >= -depth_epsilon && maximum > depth_epsilon)
        return 1;
    if (maximum <= depth_epsilon && minimum < -depth_epsilon)
        return -1;
    return 0;
}

std::vector<TriangleCommand> order_components(const std::vector<TriangleCommand>& commands,
                                              const Graph& outgoing, const Graph& reverse)
{
    auto components = strongly_connected(outgoing, reverse);
    const auto count = components.members.size();
    Graph targets(count);
    std::vector<std::unordered_set<std::size_t>> seen(count);
    std::vector<std::size_t> incoming(count, 0), orders(count, commands.size());
    std::vector<double> depths(count, 0);
    for (std::size_t behind = 0; behind < outgoing.size(); ++behind)
    {
        const auto source = components.owner[behind];
        for (auto front : outgoing[behind])
        {
            const auto target = components.owner[front];
            if (source == target || !seen[source].insert(target).second)
                continue;
            targets[source].push_back(target);
            ++incoming[target];
        }
    }
    for (std::size_t i = 0; i < count; ++i)
    {
        for (auto member : components.members[i])
        {
            depths[i] += commands[member].depth;
            orders[i] = std::min(orders[i], commands[member].order);
        }
        depths[i] /= components.members[i].size();
        if (!std::isfinite(depths[i]))
            throw std::runtime_error("Illustration component depth overflow.");
    }
    const auto greater = [&](std::size_t a, std::size_t b)
    { return depths[a] != depths[b] ? depths[a] > depths[b] : orders[a] > orders[b]; };
    std::priority_queue<std::size_t, std::vector<std::size_t>, decltype(greater)> ready(greater);
    for (std::size_t i = 0; i < count; ++i)
        if (incoming[i] == 0)
            ready.push(i);
    std::vector<TriangleCommand> result;
    result.reserve(commands.size());
    while (!ready.empty())
    {
        const auto component = ready.top();
        ready.pop();
        auto& members = components.members[component];
        std::sort(members.begin(), members.end(),
                  [&](std::size_t a, std::size_t b)
                  {
                      return commands[a].depth != commands[b].depth
                                 ? commands[a].depth < commands[b].depth
                                 : commands[a].order < commands[b].order;
                  });
        for (auto member : members)
            result.push_back(commands[member]);
        for (auto front : targets[component])
            if (--incoming[front] == 0)
                ready.push(front);
    }
    return result;
}
} // namespace

std::vector<TriangleCommand> order_triangles(const std::vector<TriangleCommand>& commands,
                                             const Bounds& bounds, WorkBudget& budget)
{
    if (commands.size() < 2)
        return commands;
    const double width = std::max(bounds.max_x - bounds.min_x, 1e-12);
    const double height = std::max(bounds.max_y - bounds.min_y, 1e-12);
    const double epsilon = std::max(width, height) * 1e-10;
    const auto grid =
        static_cast<unsigned>(clamp(std::ceil(std::sqrt(commands.size() / 4.0)), 8, 192));
    std::vector<Bounds> boxes;
    boxes.reserve(commands.size());
    for (const auto& command : commands)
        boxes.push_back(projected_bounds(*command.triangle));
    Graph outgoing(commands.size()), reverse(commands.size());
    std::size_t constraints = 0;
    const Bounds grid_bounds{bounds.min_x, bounds.min_y, bounds.min_x + width,
                             bounds.min_y + height};
    candidate_pairs(
        boxes, grid_bounds, grid, std::max(64u, grid * 2), 1e-12 / grid, budget,
        [&](std::size_t a, std::size_t b)
        {
            if (!bounds_overlap(boxes[a], boxes[b], epsilon))
                return;
            const int order = overlap_order(*commands[a].triangle, *commands[b].triangle, epsilon);
            if (!order)
                return;
            if (++constraints > 4000000)
                throw ResourceLimit("Mesh illustration exceeds the visibility constraint limit.");
            const auto behind = order > 0 ? b : a, front = order > 0 ? a : b;
            outgoing[behind].push_back(front);
            reverse[front].push_back(behind);
        });
    return order_components(commands, outgoing, reverse);
}
} // namespace geometer::illustration_detail
