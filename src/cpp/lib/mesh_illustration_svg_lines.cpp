#include "mesh_illustration_fusion.h"
#include "mesh_illustration_svg.h"

namespace geometer::illustration_detail
{
std::string chained_line_path(const std::vector<Line>& lines, std::size_t begin, std::size_t end,
                              const PointMapper& map_point)
{
    struct Segment
    {
        Vec2 a, b;
        std::string ak, bk;
    };
    const auto key = [](Vec2 point) { return number_text(point[0]) + "," + number_text(point[1]); };
    std::vector<Segment> segments;
    for (auto i = begin; i < end; ++i)
    {
        const auto a = map_point(lines[i].points[0]), b = map_point(lines[i].points[1]);
        const auto ak = key(a), bk = key(b);
        if (ak != bk)
            segments.push_back({a, b, ak, bk});
    }
    OrderedGroups<std::string, std::size_t> adjacency;
    std::unordered_map<std::string, Vec2> points;
    for (std::size_t i = 0; i < segments.size(); ++i)
    {
        const auto& segment = segments[i];
        points[segment.ak] = segment.a;
        points[segment.bk] = segment.b;
        adjacency.add(segment.ak, i);
        adjacency.add(segment.bk, i);
    }
    std::vector<bool> used(segments.size(), false);
    std::vector<Ring> polylines;
    const auto walk = [&](std::string point_key, std::size_t edge)
    {
        Ring polyline{points.at(point_key)};
        while (!used[edge])
        {
            used[edge] = true;
            const auto& segment = segments[edge];
            point_key = segment.ak == point_key ? segment.bk : segment.ak;
            polyline.push_back(points.at(point_key));
            const auto& incident = adjacency.entries[adjacency.lookup.at(point_key)].second;
            if (incident.size() != 2)
                break;
            bool found = false;
            for (auto candidate : incident)
                if (!used[candidate])
                {
                    edge = candidate;
                    found = true;
                    break;
                }
            if (!found)
                break;
        }
        polylines.push_back(std::move(polyline));
    };
    for (const auto& entry : adjacency.entries)
    {
        if (entry.second.size() == 2)
            continue;
        for (auto edge : entry.second)
            if (!used[edge])
                walk(entry.first, edge);
    }
    for (std::size_t edge = 0; edge < segments.size(); ++edge)
        if (!used[edge])
            walk(segments[edge].ak, edge);
    std::string result;
    for (const auto& polyline : polylines)
    {
        const bool closed = polyline.size() > 2 && key(polyline.front()) == key(polyline.back());
        const auto count = polyline.size() - (closed ? 1 : 0);
        for (std::size_t i = 0; i < count; ++i)
            result += (i == 0   ? "M"
                       : i == 1 ? "L"
                                : " ") +
                      number_text(polyline[i][0]) + " " + number_text(polyline[i][1]);
        if (closed)
            result += "Z";
    }
    return result;
}
} // namespace geometer::illustration_detail
