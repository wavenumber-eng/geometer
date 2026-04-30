#include "geometer/planar_contours.h"

#include <algorithm>
#include <cmath>
#include <map>
#include <set>
#include <string>
#include <vector>

namespace geometer
{
namespace
{

struct IPoint
{
    long long x = 0;
    long long y = 0;

    bool operator==(const IPoint& other) const
    {
        return x == other.x && y == other.y;
    }

    bool operator<(const IPoint& other) const
    {
        if (x != other.x)
        {
            return x < other.x;
        }
        return y < other.y;
    }
};

struct ISegment
{
    IPoint a;
    IPoint b;
};

struct DirectedEdge
{
    int from = -1;
    int to = -1;
    int twin = -1;
    double angle = 0.0;
};

struct TraceRing
{
    std::vector<IPoint> points;
    double signed_area = 0.0;
    bool hole = false;
    int nesting_depth = 0;
};

void set_status(Status* status, int code, const std::string& message)
{
    if (status == nullptr)
    {
        return;
    }
    status->code = code;
    status->message = message;
}

long long pow10_int(int digits)
{
    long long value = 1;
    for (int i = 0; i < digits; ++i)
    {
        value *= 10;
    }
    return value;
}

IPoint snap_point(const PlanarContourPoint& point, long long scale)
{
    return {
        static_cast<long long>(std::llround(point.x * static_cast<double>(scale))),
        static_cast<long long>(std::llround(point.y * static_cast<double>(scale))),
    };
}

PlanarContourPoint to_point(const IPoint& point, long long scale)
{
    return {
        static_cast<double>(point.x) / static_cast<double>(scale),
        static_cast<double>(point.y) / static_cast<double>(scale),
    };
}

ISegment normalize_segment(IPoint a, IPoint b)
{
    if (b < a)
    {
        std::swap(a, b);
    }
    return {a, b};
}

long double cross_value(const IPoint& a, const IPoint& b, const IPoint& c)
{
    const long double ab_x = static_cast<long double>(b.x - a.x);
    const long double ab_y = static_cast<long double>(b.y - a.y);
    const long double ac_x = static_cast<long double>(c.x - a.x);
    const long double ac_y = static_cast<long double>(c.y - a.y);
    return (ab_x * ac_y) - (ab_y * ac_x);
}

long double cross_vectors(long double ax, long double ay, long double bx, long double by)
{
    return (ax * by) - (ay * bx);
}

int sign_of(long double value)
{
    if (value > 0.0L)
    {
        return 1;
    }
    if (value < 0.0L)
    {
        return -1;
    }
    return 0;
}

bool point_on_segment(const IPoint& point, const ISegment& segment)
{
    if (sign_of(cross_value(segment.a, segment.b, point)) != 0)
    {
        return false;
    }
    return point.x >= std::min(segment.a.x, segment.b.x) &&
           point.x <= std::max(segment.a.x, segment.b.x) &&
           point.y >= std::min(segment.a.y, segment.b.y) &&
           point.y <= std::max(segment.a.y, segment.b.y);
}

bool segments_intersect(const ISegment& first, const ISegment& second)
{
    const int o1 = sign_of(cross_value(first.a, first.b, second.a));
    const int o2 = sign_of(cross_value(first.a, first.b, second.b));
    const int o3 = sign_of(cross_value(second.a, second.b, first.a));
    const int o4 = sign_of(cross_value(second.a, second.b, first.b));

    if (o1 == 0 && point_on_segment(second.a, first))
    {
        return true;
    }
    if (o2 == 0 && point_on_segment(second.b, first))
    {
        return true;
    }
    if (o3 == 0 && point_on_segment(first.a, second))
    {
        return true;
    }
    if (o4 == 0 && point_on_segment(first.b, second))
    {
        return true;
    }

    return o1 != o2 && o3 != o4;
}

bool collinear(const ISegment& first, const ISegment& second)
{
    return sign_of(cross_value(first.a, first.b, second.a)) == 0 &&
           sign_of(cross_value(first.a, first.b, second.b)) == 0;
}

IPoint line_intersection_point(const ISegment& first, const ISegment& second)
{
    const long double ax = static_cast<long double>(first.a.x);
    const long double ay = static_cast<long double>(first.a.y);
    const long double bx = static_cast<long double>(first.b.x);
    const long double by = static_cast<long double>(first.b.y);
    const long double cx = static_cast<long double>(second.a.x);
    const long double cy = static_cast<long double>(second.a.y);
    const long double dx = static_cast<long double>(second.b.x);
    const long double dy = static_cast<long double>(second.b.y);

    const long double rx = bx - ax;
    const long double ry = by - ay;
    const long double sx = dx - cx;
    const long double sy = dy - cy;
    const long double denominator = cross_vectors(rx, ry, sx, sy);
    if (denominator == 0.0L)
    {
        return first.a;
    }

    const long double t = cross_vectors(cx - ax, cy - ay, sx, sy) / denominator;
    return {
        static_cast<long long>(std::llround(ax + (t * rx))),
        static_cast<long long>(std::llround(ay + (t * ry))),
    };
}

bool less_along_segment(const IPoint& left, const IPoint& right, const ISegment& segment)
{
    const long long dx = segment.b.x - segment.a.x;
    const long long dy = segment.b.y - segment.a.y;
    const bool use_x = std::llabs(dx) >= std::llabs(dy);

    if (use_x)
    {
        if (left.x != right.x)
        {
            return dx >= 0 ? left.x < right.x : left.x > right.x;
        }
    }
    else if (left.y != right.y)
    {
        return dy >= 0 ? left.y < right.y : left.y > right.y;
    }

    return left < right;
}

std::vector<ISegment> node_segments(const std::vector<ISegment>& source_segments)
{
    std::vector<std::set<IPoint>> split_points(source_segments.size());
    for (std::size_t i = 0; i < source_segments.size(); ++i)
    {
        split_points[i].insert(source_segments[i].a);
        split_points[i].insert(source_segments[i].b);
    }

    for (std::size_t i = 0; i < source_segments.size(); ++i)
    {
        for (std::size_t j = i + 1; j < source_segments.size(); ++j)
        {
            const ISegment& first = source_segments[i];
            const ISegment& second = source_segments[j];
            if (!segments_intersect(first, second))
            {
                continue;
            }

            if (collinear(first, second))
            {
                if (point_on_segment(first.a, second))
                {
                    split_points[j].insert(first.a);
                }
                if (point_on_segment(first.b, second))
                {
                    split_points[j].insert(first.b);
                }
                if (point_on_segment(second.a, first))
                {
                    split_points[i].insert(second.a);
                }
                if (point_on_segment(second.b, first))
                {
                    split_points[i].insert(second.b);
                }
                continue;
            }

            const IPoint intersection = line_intersection_point(first, second);
            split_points[i].insert(intersection);
            split_points[j].insert(intersection);
        }
    }

    std::set<std::pair<IPoint, IPoint>> unique_segments;
    for (std::size_t i = 0; i < source_segments.size(); ++i)
    {
        const ISegment& segment = source_segments[i];
        std::vector<IPoint> points(split_points[i].begin(), split_points[i].end());
        std::sort(points.begin(), points.end(), [&segment](const IPoint& left, const IPoint& right)
                  { return less_along_segment(left, right, segment); });

        for (std::size_t point_index = 1; point_index < points.size(); ++point_index)
        {
            if (points[point_index - 1] == points[point_index])
            {
                continue;
            }
            const ISegment normalized =
                normalize_segment(points[point_index - 1], points[point_index]);
            if (!(normalized.a == normalized.b))
            {
                unique_segments.insert({normalized.a, normalized.b});
            }
        }
    }

    std::vector<ISegment> result;
    result.reserve(unique_segments.size());
    for (const auto& segment : unique_segments)
    {
        result.push_back({segment.first, segment.second});
    }
    return result;
}

long double signed_area2(const std::vector<IPoint>& points)
{
    long double area = 0.0L;
    if (points.size() < 3)
    {
        return area;
    }

    for (std::size_t i = 0; i < points.size(); ++i)
    {
        const IPoint& current = points[i];
        const IPoint& next = points[(i + 1) % points.size()];
        area += (static_cast<long double>(current.x) * static_cast<long double>(next.y)) -
                (static_cast<long double>(next.x) * static_cast<long double>(current.y));
    }
    return area;
}

double signed_area(const std::vector<IPoint>& points, long long scale)
{
    const long double scale_area =
        static_cast<long double>(scale) * static_cast<long double>(scale);
    return static_cast<double>(signed_area2(points) / (2.0L * scale_area));
}

std::map<IPoint, int> build_vertex_map(const std::vector<ISegment>& segments,
                                       std::vector<IPoint>* vertices)
{
    std::set<IPoint> unique_points;
    for (const ISegment& segment : segments)
    {
        unique_points.insert(segment.a);
        unique_points.insert(segment.b);
    }

    std::map<IPoint, int> vertex_by_point;
    vertices->assign(unique_points.begin(), unique_points.end());
    for (std::size_t i = 0; i < vertices->size(); ++i)
    {
        vertex_by_point[(*vertices)[i]] = static_cast<int>(i);
    }
    return vertex_by_point;
}

void sort_adjacency(const std::vector<DirectedEdge>& edges,
                    std::vector<std::vector<int>>* adjacency)
{
    for (std::vector<int>& outgoing : *adjacency)
    {
        std::sort(outgoing.begin(), outgoing.end(),
                  [&edges](int left, int right)
                  {
                      if (edges[left].angle != edges[right].angle)
                      {
                          return edges[left].angle < edges[right].angle;
                      }
                      return edges[left].to < edges[right].to;
                  });
    }
}

std::vector<TraceRing> trace_positive_faces(const std::vector<IPoint>& vertices,
                                            const std::vector<DirectedEdge>& edges,
                                            const std::vector<std::vector<int>>& adjacency,
                                            double area_epsilon, long long scale,
                                            std::vector<int>* face_by_edge)
{
    std::vector<int> position_by_edge(edges.size(), -1);
    for (std::size_t vertex = 0; vertex < adjacency.size(); ++vertex)
    {
        for (std::size_t index = 0; index < adjacency[vertex].size(); ++index)
        {
            position_by_edge[adjacency[vertex][index]] = static_cast<int>(index);
        }
    }

    std::vector<int> next_edge(edges.size(), -1);
    for (std::size_t edge_index = 0; edge_index < edges.size(); ++edge_index)
    {
        const DirectedEdge& edge = edges[edge_index];
        const std::vector<int>& outgoing = adjacency[edge.to];
        const int twin_position = position_by_edge[edge.twin];
        if (twin_position < 0 || outgoing.empty())
        {
            continue;
        }
        const int next_position = (twin_position + static_cast<int>(outgoing.size()) - 1) %
                                  static_cast<int>(outgoing.size());
        next_edge[edge_index] = outgoing[next_position];
    }

    face_by_edge->assign(edges.size(), -1);
    std::vector<char> visited(edges.size(), 0);
    std::vector<TraceRing> faces;

    for (std::size_t start = 0; start < edges.size(); ++start)
    {
        if (visited[start])
        {
            continue;
        }

        std::vector<int> loop_edges;
        int current = static_cast<int>(start);
        bool closed = false;
        for (std::size_t guard = 0; guard <= edges.size(); ++guard)
        {
            if (current < 0)
            {
                break;
            }
            if (visited[current])
            {
                closed = current == static_cast<int>(start);
                break;
            }
            visited[current] = 1;
            loop_edges.push_back(current);
            current = next_edge[current];
        }

        if (!closed || loop_edges.size() < 3)
        {
            continue;
        }

        std::vector<IPoint> points;
        points.reserve(loop_edges.size());
        for (int edge_index : loop_edges)
        {
            points.push_back(vertices[edges[edge_index].from]);
        }

        const double area = signed_area(points, scale);
        if (area <= area_epsilon)
        {
            continue;
        }

        const int face_index = static_cast<int>(faces.size());
        for (int edge_index : loop_edges)
        {
            (*face_by_edge)[edge_index] = face_index;
        }
        faces.push_back({points, area, false, 0});
    }

    return faces;
}

int choose_next_boundary_edge(const std::vector<DirectedEdge>& edges,
                              const std::vector<std::vector<int>>& adjacency, int current)
{
    const int vertex = edges[current].to;
    const std::vector<int>& outgoing = adjacency[vertex];
    if (outgoing.empty())
    {
        return -1;
    }

    const double back_angle = edges[edges[current].twin].angle;
    int selected = outgoing.back();
    for (int edge_index : outgoing)
    {
        if (edges[edge_index].angle < back_angle)
        {
            selected = edge_index;
            continue;
        }
        break;
    }
    return selected;
}

std::vector<TraceRing> trace_boundary_rings(const std::vector<IPoint>& vertices,
                                            const std::vector<DirectedEdge>& edges,
                                            const std::vector<int>& selected_edges,
                                            double area_epsilon, long long scale)
{
    std::vector<std::vector<int>> boundary_adjacency(vertices.size());
    std::vector<char> selected(edges.size(), 0);
    for (int edge_index : selected_edges)
    {
        selected[edge_index] = 1;
        boundary_adjacency[edges[edge_index].from].push_back(edge_index);
    }
    sort_adjacency(edges, &boundary_adjacency);

    std::vector<char> visited(edges.size(), 0);
    std::vector<TraceRing> rings;
    for (int start : selected_edges)
    {
        if (visited[start])
        {
            continue;
        }

        std::vector<int> loop_edges;
        int current = start;
        bool closed = false;
        for (std::size_t guard = 0; guard <= selected_edges.size() + 1; ++guard)
        {
            if (current < 0 || !selected[current])
            {
                break;
            }
            if (visited[current])
            {
                closed = current == start;
                break;
            }
            visited[current] = 1;
            loop_edges.push_back(current);
            current = choose_next_boundary_edge(edges, boundary_adjacency, current);
        }

        if (!closed || loop_edges.size() < 3)
        {
            continue;
        }

        std::vector<IPoint> points;
        points.reserve(loop_edges.size());
        for (int edge_index : loop_edges)
        {
            points.push_back(vertices[edges[edge_index].from]);
        }

        const double area = signed_area(points, scale);
        if (std::fabs(area) <= area_epsilon)
        {
            continue;
        }
        rings.push_back({points, area, false, 0});
    }

    return rings;
}

IPoint min_point(const std::vector<IPoint>& points)
{
    return *std::min_element(points.begin(), points.end());
}

PlanarContourPoint representative_point(const std::vector<IPoint>& points, long long scale)
{
    return to_point(min_point(points), scale);
}

bool point_in_ring(const PlanarContourPoint& point, const std::vector<IPoint>& ring,
                   long long scale)
{
    bool inside = false;
    for (std::size_t i = 0, j = ring.size() - 1; i < ring.size(); j = i++)
    {
        const PlanarContourPoint pi = to_point(ring[i], scale);
        const PlanarContourPoint pj = to_point(ring[j], scale);
        const bool crosses = (pi.y > point.y) != (pj.y > point.y);
        if (!crosses)
        {
            continue;
        }
        const double x_intersection = ((pj.x - pi.x) * (point.y - pi.y) / (pj.y - pi.y)) + pi.x;
        if (point.x < x_intersection)
        {
            inside = !inside;
        }
    }
    return inside;
}

void classify_and_sort_rings(std::vector<TraceRing>* rings, long long scale)
{
    for (std::size_t i = 0; i < rings->size(); ++i)
    {
        TraceRing& ring = (*rings)[i];
        int depth = 0;
        const PlanarContourPoint point = representative_point(ring.points, scale);
        const double abs_area = std::fabs(ring.signed_area);
        for (std::size_t j = 0; j < rings->size(); ++j)
        {
            if (i == j)
            {
                continue;
            }
            if (std::fabs((*rings)[j].signed_area) <= abs_area)
            {
                continue;
            }
            if (point_in_ring(point, (*rings)[j].points, scale))
            {
                ++depth;
            }
        }

        ring.nesting_depth = depth;
        ring.hole = (depth % 2) == 1;
        if (ring.hole && ring.signed_area > 0.0)
        {
            std::reverse(ring.points.begin(), ring.points.end());
            ring.signed_area = -ring.signed_area;
        }
        else if (!ring.hole && ring.signed_area < 0.0)
        {
            std::reverse(ring.points.begin(), ring.points.end());
            ring.signed_area = -ring.signed_area;
        }
    }

    std::sort(rings->begin(), rings->end(),
              [](const TraceRing& left, const TraceRing& right)
              {
                  if (left.nesting_depth != right.nesting_depth)
                  {
                      return left.nesting_depth < right.nesting_depth;
                  }
                  if (left.hole != right.hole)
                  {
                      return !left.hole;
                  }
                  const double left_abs_area = std::fabs(left.signed_area);
                  const double right_abs_area = std::fabs(right.signed_area);
                  if (left_abs_area != right_abs_area)
                  {
                      return left_abs_area > right_abs_area;
                  }
                  const IPoint left_min = min_point(left.points);
                  const IPoint right_min = min_point(right.points);
                  return left_min < right_min;
              });
}

void append_result_segments(const std::vector<TraceRing>& rings, long long scale,
                            PlanarContourResult* result)
{
    std::set<std::pair<IPoint, IPoint>> unique_segments;
    for (const TraceRing& ring : rings)
    {
        for (std::size_t i = 0; i < ring.points.size(); ++i)
        {
            const IPoint& start = ring.points[i];
            const IPoint& end = ring.points[(i + 1) % ring.points.size()];
            const ISegment normalized = normalize_segment(start, end);
            unique_segments.insert({normalized.a, normalized.b});
        }
    }

    result->segments.reserve(unique_segments.size());
    for (const auto& segment : unique_segments)
    {
        result->segments.push_back(
            {to_point(segment.first, scale), to_point(segment.second, scale)});
    }
}

} // namespace

int build_planar_contours(const std::vector<PlanarContourSegment>& segments,
                          const PlanarContourOptions& options, PlanarContourResult* result,
                          Status* status)
{
    if (result == nullptr)
    {
        set_status(status, 2, "Planar contour result pointer is null.");
        return 2;
    }
    result->rings.clear();
    result->segments.clear();

    if (options.round_digits < 0 || options.round_digits > 9)
    {
        set_status(status, 3, "Planar contour round_digits must be between 0 and 9.");
        return 3;
    }

    const long long scale = pow10_int(options.round_digits);
    std::vector<ISegment> snapped_segments;
    snapped_segments.reserve(segments.size());
    for (const PlanarContourSegment& segment : segments)
    {
        const IPoint start = snap_point(segment.start, scale);
        const IPoint end = snap_point(segment.end, scale);
        if (start == end)
        {
            continue;
        }
        snapped_segments.push_back({start, end});
    }

    if (snapped_segments.empty())
    {
        set_status(status, 0, "");
        return 0;
    }

    const std::vector<ISegment> noded_segments = node_segments(snapped_segments);
    if (noded_segments.empty())
    {
        set_status(status, 0, "");
        return 0;
    }

    std::vector<IPoint> vertices;
    const std::map<IPoint, int> vertex_by_point = build_vertex_map(noded_segments, &vertices);
    std::vector<DirectedEdge> edges;
    std::vector<std::vector<int>> adjacency(vertices.size());
    edges.reserve(noded_segments.size() * 2);

    for (const ISegment& segment : noded_segments)
    {
        const int from = vertex_by_point.at(segment.a);
        const int to = vertex_by_point.at(segment.b);
        const int forward = static_cast<int>(edges.size());
        const int reverse = forward + 1;
        const double forward_angle = std::atan2(static_cast<double>(segment.b.y - segment.a.y),
                                                static_cast<double>(segment.b.x - segment.a.x));
        const double reverse_angle = std::atan2(static_cast<double>(segment.a.y - segment.b.y),
                                                static_cast<double>(segment.a.x - segment.b.x));
        edges.push_back({from, to, reverse, forward_angle});
        edges.push_back({to, from, forward, reverse_angle});
        adjacency[from].push_back(forward);
        adjacency[to].push_back(reverse);
    }
    sort_adjacency(edges, &adjacency);

    std::vector<int> face_by_edge;
    std::vector<TraceRing> rings = trace_positive_faces(vertices, edges, adjacency,
                                                        options.area_epsilon, scale, &face_by_edge);

    if (options.union_polygons && !rings.empty())
    {
        std::vector<int> boundary_edges;
        for (std::size_t edge_index = 0; edge_index < edges.size(); ++edge_index)
        {
            if (face_by_edge[edge_index] >= 0 && face_by_edge[edges[edge_index].twin] < 0)
            {
                boundary_edges.push_back(static_cast<int>(edge_index));
            }
        }
        rings = trace_boundary_rings(vertices, edges, boundary_edges, options.area_epsilon, scale);
    }

    classify_and_sort_rings(&rings, scale);

    result->rings.reserve(rings.size());
    for (const TraceRing& ring : rings)
    {
        PlanarContourRing output_ring;
        output_ring.hole = ring.hole;
        output_ring.nesting_depth = ring.nesting_depth;
        output_ring.signed_area = ring.signed_area;
        output_ring.points.reserve(ring.points.size());
        for (const IPoint& point : ring.points)
        {
            output_ring.points.push_back(to_point(point, scale));
        }
        result->rings.push_back(output_ring);
    }
    append_result_segments(rings, scale, result);

    set_status(status, 0, "");
    return 0;
}

} // namespace geometer
