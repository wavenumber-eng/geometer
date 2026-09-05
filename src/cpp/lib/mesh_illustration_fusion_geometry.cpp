#include "mesh_illustration_fusion.h"

#include <algorithm>
#include <unordered_set>

namespace geometer::illustration_detail
{
std::array<unsigned, 3> oriented_indices(const Triangle& triangle)
{
    return signed_area(triangle.points) >= 0 ? std::array<unsigned, 3>{0, 1, 2}
                                             : std::array<unsigned, 3>{0, 2, 1};
}

OrderedGroups<std::string, FusionEdge> fusion_edges(const std::vector<TriangleCommand>& commands,
                                                    const std::vector<std::size_t>& members,
                                                    double coordinate_tolerance,
                                                    double depth_tolerance, bool opaque_only)
{
    OrderedGroups<std::string, FusionEdge> result;
    for (auto member : members)
    {
        const auto& command = commands[member];
        if (opaque_only && command.opacity < .999)
            continue;
        const auto& t = *command.triangle;
        const auto indices = oriented_indices(t);
        const auto key = [&](unsigned index)
        {
            return integer_text(t.points[index][0] / coordinate_tolerance) + "," +
                   integer_text(t.points[index][1] / coordinate_tolerance) + "," +
                   integer_text(t.depths[index] / depth_tolerance);
        };
        for (unsigned i = 0; i < 3; ++i)
        {
            const auto a = indices[i], b = indices[(i + 1) % 3], c = indices[(i + 2) % 3];
            const auto ak = key(a), bk = key(b);
            const auto ek = ak < bk ? ak + "|" + bk : bk + "|" + ak;
            result.add(ek, {member, t.points[a], t.points[b], t.points[c], ak, bk, ek});
        }
    }
    return result;
}

namespace
{
bool segments_intersect(Vec2 a, Vec2 b, Vec2 c, Vec2 d, double epsilon)
{
    const double abc = cross2(a, b, c), abd = cross2(a, b, d), cda = cross2(c, d, a),
                 cdb = cross2(c, d, b);
    const auto opposite = [epsilon](double x, double y)
    { return (x > epsilon && y < -epsilon) || (x < -epsilon && y > epsilon); };
    if (opposite(abc, abd) && opposite(cda, cdb))
        return true;
    const auto within = [epsilon](double value, double first, double second)
    {
        return value >= std::min(first, second) - epsilon &&
               value <= std::max(first, second) + epsilon;
    };
    const auto on_segment = [&](Vec2 p, Vec2 first, Vec2 second, double area)
    {
        return std::abs(area) <= epsilon && within(p[0], first[0], second[0]) &&
               within(p[1], first[1], second[1]);
    };
    return on_segment(c, a, b, abc) || on_segment(d, a, b, abd) || on_segment(a, c, d, cda) ||
           on_segment(b, c, d, cdb);
}

bool valid_rings(const Rings& rings, double epsilon, WorkBudget& budget)
{
    struct Segment
    {
        Vec2 a, b;
        std::size_t ring, edge, count;
    };
    std::vector<Segment> segments;
    std::vector<Bounds> boxes;
    Bounds bounds;
    for (std::size_t ring = 0; ring < rings.size(); ++ring)
    {
        const auto& points = rings[ring];
        if (points.size() < 3 || std::abs(signed_area(points)) <= epsilon * epsilon)
            return false;
        for (std::size_t edge = 0; edge < points.size(); ++edge)
        {
            const auto a = points[edge], b = points[(edge + 1) % points.size()];
            segments.push_back({a, b, ring, edge, points.size()});
            Bounds box;
            box.include(a);
            box.include(b);
            boxes.push_back(box);
            bounds.include(a);
            bounds.include(b);
        }
    }
    const auto grid =
        static_cast<unsigned>(clamp(std::ceil(std::sqrt(segments.size() / 2.0)), 4, 256));
    bool valid = true;
    candidate_pairs(boxes, bounds, grid, std::max(32u, grid), epsilon, budget,
                    [&](std::size_t first, std::size_t second)
                    {
                        if (!valid)
                            return;
                        const auto& a = segments[first];
                        const auto& b = segments[second];
                        if (a.ring == b.ring &&
                            (a.edge == b.edge || (a.edge + 1) % a.count == b.edge ||
                             (b.edge + 1) % b.count == a.edge))
                            return;
                        valid = !segments_intersect(a.a, a.b, b.a, b.b, epsilon);
                    });
    return valid;
}

Ring simplify_ring(Ring points, double tolerance, WorkBudget& budget)
{
    bool changed = true;
    while (changed && points.size() > 3)
    {
        changed = false;
        for (std::size_t index = 0; index < points.size(); ++index)
        {
            budget.consume();
            const auto previous = points[(index + points.size() - 1) % points.size()];
            const auto current = points[index], next = points[(index + 1) % points.size()];
            const double baseline = std::hypot(next[0] - previous[0], next[1] - previous[1]);
            if (baseline <= tolerance ||
                std::abs(cross2(previous, next, current)) / baseline <= tolerance)
            {
                points.erase(points.begin() + index);
                changed = true;
                break;
            }
        }
    }
    return points;
}
} // namespace

std::optional<Surface> fused_component(const std::vector<TriangleCommand>& commands,
                                       const std::vector<std::size_t>& members,
                                       double coordinate_tolerance, double depth_tolerance,
                                       WorkBudget& budget)
{
    const auto edges =
        fusion_edges(commands, members, coordinate_tolerance, depth_tolerance, false);
    std::vector<FusionEdge> boundary;
    for (const auto& entry : edges.entries)
    {
        const auto& values = entry.second;
        if (values.size() == 1)
            boundary.push_back(values[0]);
        else if (values.size() != 2 || values[0].start_key != values[1].end_key ||
                 values[0].end_key != values[1].start_key)
            return {};
    }
    if (boundary.size() < 3)
        return {};
    std::unordered_map<std::string, std::size_t> outgoing, incoming;
    for (std::size_t i = 0; i < boundary.size(); ++i)
    {
        if (!outgoing.emplace(boundary[i].start_key, i).second)
            return {};
        ++incoming[boundary[i].end_key];
    }
    for (const auto& entry : outgoing)
        if (incoming[entry.first] != 1)
            return {};
    for (const auto& entry : incoming)
        if (outgoing.find(entry.first) == outgoing.end())
            return {};
    std::vector<bool> used(boundary.size(), false);
    Rings rings;
    for (std::size_t first = 0; first < boundary.size(); ++first)
    {
        if (used[first])
            continue;
        Ring ring;
        auto current = first;
        for (std::size_t step = 0; step <= boundary.size(); ++step)
        {
            if (used[current])
                return {};
            used[current] = true;
            const auto& edge = boundary[current];
            ring.push_back(edge.start);
            if (edge.end_key == boundary[first].start_key)
                break;
            const auto next = outgoing.find(edge.end_key);
            if (next == outgoing.end() || step == boundary.size())
                return {};
            current = next->second;
        }
        rings.push_back(simplify_ring(std::move(ring), coordinate_tolerance, budget));
    }
    if (!valid_rings(rings, coordinate_tolerance, budget))
        return {};
    const auto& first = commands[members[0]];
    return Surface{false, false, {{std::move(rings), first.fill, first.opacity}}};
}
} // namespace geometer::illustration_detail
