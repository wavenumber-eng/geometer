#include "fast_mesh_shadow_outline.h"

#include <clipper2/clipper.h>

#include <algorithm>
#include <array>
#include <cmath>
#include <cstdint>
#include <limits>
#include <map>
#include <queue>
#include <utility>
#include <vector>

namespace geometer
{
namespace
{

struct Vec3
{
    double x = 0.0;
    double y = 0.0;
    double z = 0.0;
};

struct ViewBasis
{
    Vec3 x;
    Vec3 y;
    Vec3 z;
};

struct DirectedEdge
{
    std::uint32_t source_face = 0;
    std::uint32_t start = 0;
    std::uint32_t end = 0;
};

struct FaceVertexKey
{
    std::uint32_t source_face = 0;
    std::uint32_t vertex = 0;

    bool operator<(const FaceVertexKey& other) const
    {
        return source_face < other.source_face ||
               (source_face == other.source_face && vertex < other.vertex);
    }

    bool operator==(const FaceVertexKey& other) const
    {
        return source_face == other.source_face && vertex == other.vertex;
    }
};

struct FaceEdgeKey
{
    std::uint32_t source_face = 0;
    std::uint32_t first = 0;
    std::uint32_t second = 0;

    bool operator<(const FaceEdgeKey& other) const
    {
        if (source_face != other.source_face)
        {
            return source_face < other.source_face;
        }
        return first < other.first || (first == other.first && second < other.second);
    }
};

struct PointKey
{
    std::int64_t x = 0;
    std::int64_t y = 0;
};

void set_status(Status* status, int code, const char* message)
{
    if (status != nullptr)
    {
        status->code = code;
        status->message = message == nullptr ? "" : message;
    }
}

double dot(const Vec3& left, const Vec3& right)
{
    return left.x * right.x + left.y * right.y + left.z * right.z;
}

Vec3 cross(const Vec3& left, const Vec3& right)
{
    return {left.y * right.z - left.z * right.y, left.z * right.x - left.x * right.z,
            left.x * right.y - left.y * right.x};
}

bool normalize(Vec3* value)
{
    const double magnitude = std::sqrt(dot(*value, *value));
    if (!std::isfinite(magnitude) || magnitude <= 1.0e-15)
    {
        return false;
    }
    value->x /= magnitude;
    value->y /= magnitude;
    value->z /= magnitude;
    return true;
}

bool make_view_basis(const ProjectionViewSpec& view, ViewBasis* basis)
{
    basis->z = {view.direction[0], view.direction[1], view.direction[2]};
    Vec3 up = {view.up[0], view.up[1], view.up[2]};
    if (!normalize(&basis->z))
    {
        return false;
    }
    const double depth = dot(up, basis->z);
    basis->y = {up.x - basis->z.x * depth, up.y - basis->z.y * depth, up.z - basis->z.z * depth};
    if (!normalize(&basis->y))
    {
        return false;
    }
    basis->x = cross(basis->y, basis->z);
    return normalize(&basis->x);
}

Clipper2Lib::PointD project_point(const FastHlrVec3& point, const ViewBasis& basis)
{
    const Vec3 value = {point.x, point.y, point.z};
    return {dot(value, basis.x), dot(value, basis.y)};
}

double signed_area2(const Clipper2Lib::PointD& a, const Clipper2Lib::PointD& b,
                    const Clipper2Lib::PointD& c)
{
    return (b.x - a.x) * (c.y - a.y) - (b.y - a.y) * (c.x - a.x);
}

bool patch_paths(const FastHlrPreparedMesh& prepared,
                 const std::vector<Clipper2Lib::PointD>& vertices, const std::vector<int>& signs,
                 Clipper2Lib::PathsD* paths, FastMeshShadowStatistics* statistics)
{
    std::map<FaceEdgeKey, DirectedEdge> face_boundaries;
    for (std::size_t triangle_index = 0; triangle_index < prepared.triangles.size();
         ++triangle_index)
    {
        if (signs[triangle_index] == 0)
        {
            continue;
        }
        const FastHlrPreparedTriangle& triangle = prepared.triangles[triangle_index];
        const std::array<std::uint32_t, 3> oriented_vertices =
            signs[triangle_index] > 0
                ? triangle.vertices
                : std::array<std::uint32_t, 3>{triangle.vertices[0], triangle.vertices[2],
                                               triangle.vertices[1]};
        for (std::size_t edge_index = 0; edge_index < 3; ++edge_index)
        {
            const DirectedEdge directed = {triangle.source_face, oriented_vertices[edge_index],
                                           oriented_vertices[(edge_index + 1) % 3]};
            const FaceEdgeKey key = {triangle.source_face, std::min(directed.start, directed.end),
                                     std::max(directed.start, directed.end)};
            const auto found = face_boundaries.find(key);
            if (found == face_boundaries.end())
            {
                face_boundaries.emplace(key, directed);
            }
            else if (found->second.start == directed.end && found->second.end == directed.start)
            {
                face_boundaries.erase(found);
            }
            else
            {
                return false;
            }
        }
    }
    std::vector<DirectedEdge> boundaries;
    boundaries.reserve(face_boundaries.size());
    for (const auto& entry : face_boundaries)
    {
        const DirectedEdge& boundary = entry.second;
        const Clipper2Lib::PointD& start = vertices[boundary.start];
        const Clipper2Lib::PointD& end = vertices[boundary.end];
        if (start.x != end.x || start.y != end.y)
        {
            boundaries.push_back(boundary);
        }
    }
    statistics->boundary_edges += boundaries.size();
    if (boundaries.empty())
    {
        return true;
    }

    std::map<FaceVertexKey, std::size_t> outgoing;
    std::map<FaceVertexKey, std::size_t> incoming_count;
    for (std::size_t index = 0; index < boundaries.size(); ++index)
    {
        const FaceVertexKey start = {boundaries[index].source_face, boundaries[index].start};
        const FaceVertexKey end = {boundaries[index].source_face, boundaries[index].end};
        if (!outgoing.emplace(start, index).second)
        {
            return false;
        }
        ++incoming_count[end];
    }
    for (const auto& entry : outgoing)
    {
        const auto incoming = incoming_count.find(entry.first);
        if (incoming == incoming_count.end() || incoming->second != 1)
        {
            return false;
        }
    }

    std::vector<bool> used(boundaries.size(), false);
    for (std::size_t first_edge = 0; first_edge < boundaries.size(); ++first_edge)
    {
        if (used[first_edge])
        {
            continue;
        }
        Clipper2Lib::PathD path;
        std::size_t edge_index = first_edge;
        const FaceVertexKey first_vertex = {boundaries[first_edge].source_face,
                                            boundaries[first_edge].start};
        for (std::size_t step = 0; step <= boundaries.size(); ++step)
        {
            if (used[edge_index])
            {
                return false;
            }
            used[edge_index] = true;
            path.push_back(vertices[boundaries[edge_index].start]);
            const FaceVertexKey next_vertex = {boundaries[edge_index].source_face,
                                               boundaries[edge_index].end};
            if (next_vertex == first_vertex)
            {
                break;
            }
            const auto next = outgoing.find(next_vertex);
            if (next == outgoing.end())
            {
                return false;
            }
            edge_index = next->second;
        }
        const FaceVertexKey last_vertex = {boundaries[edge_index].source_face,
                                           boundaries[edge_index].end};
        if (path.size() < 3 || !(last_vertex == first_vertex))
        {
            return false;
        }
        paths->push_back(std::move(path));
        ++statistics->patch_loops;
    }
    return true;
}

struct SegmentBounds
{
    double min_x = 0.0;
    double max_x = 0.0;
    double min_y = 0.0;
    double max_y = 0.0;
};

class FenwickCounts
{
  public:
    explicit FenwickCounts(std::size_t size) : tree_(size + 1, 0) {}

    void add(std::size_t index, int delta)
    {
        for (++index; index < tree_.size(); index += index & (~index + 1))
        {
            if (delta > 0)
                tree_[index] += static_cast<std::size_t>(delta);
            else
                tree_[index] -= static_cast<std::size_t>(-delta);
        }
    }

    std::size_t prefix_sum(std::size_t count) const
    {
        std::size_t result = 0;
        for (; count != 0; count &= count - 1)
            result += tree_[count];
        return result;
    }

  private:
    std::vector<std::size_t> tree_;
};

bool charge_union_pairs(const Clipper2Lib::PathsD& paths, std::size_t* remaining_pairs)
{
    std::vector<SegmentBounds> segments;
    for (const Clipper2Lib::PathD& path : paths)
    {
        if (path.size() < 2)
            continue;
        for (std::size_t index = 0; index < path.size(); ++index)
        {
            const Clipper2Lib::PointD& first = path[index];
            const Clipper2Lib::PointD& second = path[(index + 1) % path.size()];
            if (first.x == second.x && first.y == second.y)
                continue;
            segments.push_back({std::min(first.x, second.x), std::max(first.x, second.x),
                                std::min(first.y, second.y), std::max(first.y, second.y)});
        }
    }
    if (segments.size() < 2)
        return true;

    std::sort(segments.begin(), segments.end(),
              [](const SegmentBounds& left, const SegmentBounds& right)
              {
                  if (left.min_x != right.min_x)
                      return left.min_x < right.min_x;
                  return left.max_x < right.max_x;
              });

    std::vector<double> y_coordinates;
    y_coordinates.reserve(segments.size() * 2);
    for (const SegmentBounds& segment : segments)
    {
        y_coordinates.push_back(segment.min_y);
        y_coordinates.push_back(segment.max_y);
    }
    std::sort(y_coordinates.begin(), y_coordinates.end());
    y_coordinates.erase(std::unique(y_coordinates.begin(), y_coordinates.end()),
                        y_coordinates.end());

    using ActiveSegment = std::pair<double, std::size_t>;
    std::priority_queue<ActiveSegment, std::vector<ActiveSegment>, std::greater<>> active_by_max_x;
    FenwickCounts active_min_y(y_coordinates.size());
    FenwickCounts active_max_y(y_coordinates.size());
    for (std::size_t index = 0; index < segments.size(); ++index)
    {
        const SegmentBounds& current = segments[index];
        while (!active_by_max_x.empty() && active_by_max_x.top().first < current.min_x)
        {
            const SegmentBounds& expired = segments[active_by_max_x.top().second];
            active_min_y.add(
                static_cast<std::size_t>(
                    std::lower_bound(y_coordinates.begin(), y_coordinates.end(), expired.min_y) -
                    y_coordinates.begin()),
                -1);
            active_max_y.add(
                static_cast<std::size_t>(
                    std::lower_bound(y_coordinates.begin(), y_coordinates.end(), expired.max_y) -
                    y_coordinates.begin()),
                -1);
            active_by_max_x.pop();
        }

        const std::size_t min_y_at_most_max = active_min_y.prefix_sum(static_cast<std::size_t>(
            std::upper_bound(y_coordinates.begin(), y_coordinates.end(), current.max_y) -
            y_coordinates.begin()));
        const std::size_t max_y_below_min = active_max_y.prefix_sum(static_cast<std::size_t>(
            std::lower_bound(y_coordinates.begin(), y_coordinates.end(), current.min_y) -
            y_coordinates.begin()));
        const std::size_t candidates = min_y_at_most_max - max_y_below_min;
        if (candidates > *remaining_pairs)
            return false;
        *remaining_pairs -= candidates;

        active_min_y.add(
            static_cast<std::size_t>(
                std::lower_bound(y_coordinates.begin(), y_coordinates.end(), current.min_y) -
                y_coordinates.begin()),
            1);
        active_max_y.add(
            static_cast<std::size_t>(
                std::lower_bound(y_coordinates.begin(), y_coordinates.end(), current.max_y) -
                y_coordinates.begin()),
            1);
        active_by_max_x.emplace(current.max_x, index);
    }
    return true;
}

bool union_paths(const Clipper2Lib::PathsD& paths, std::size_t* remaining_pairs,
                 Clipper2Lib::PathsD* solution)
{
    if (paths.empty())
    {
        solution->clear();
        return true;
    }
    if (!charge_union_pairs(paths, remaining_pairs))
        return false;
    Clipper2Lib::ClipperD clipper(/*precision=*/6);
    clipper.AddSubject(paths);
    solution->clear();
    clipper.Execute(Clipper2Lib::ClipType::Union, Clipper2Lib::FillRule::NonZero, *solution);
    return true;
}

bool union_triangle_faces(const FastHlrPreparedMesh& prepared,
                          const std::vector<Clipper2Lib::PointD>& vertices,
                          const std::vector<int>& signs, std::size_t* remaining_pairs,
                          Clipper2Lib::PathsD* solutions, FastMeshShadowStatistics* statistics)
{
    std::map<std::uint32_t, Clipper2Lib::PathsD> face_triangles;
    for (std::size_t index = 0; index < prepared.triangles.size(); ++index)
    {
        if (signs[index] == 0)
        {
            continue;
        }
        const auto& triangle = prepared.triangles[index];
        Clipper2Lib::PointD first = vertices[triangle.vertices[0]];
        Clipper2Lib::PointD second = vertices[triangle.vertices[1]];
        Clipper2Lib::PointD third = vertices[triangle.vertices[2]];
        if (signs[index] < 0)
        {
            std::swap(second, third);
        }
        face_triangles[triangle.source_face].push_back({first, second, third});
        ++statistics->fallback_triangles;
    }
    solutions->clear();
    for (const auto& entry : face_triangles)
    {
        Clipper2Lib::PathsD face_solution;
        if (!union_paths(entry.second, remaining_pairs, &face_solution))
            return false;
        solutions->insert(solutions->end(), face_solution.begin(), face_solution.end());
    }
    return true;
}

bool snap(double value, std::int64_t scale, std::int64_t* result)
{
    const double scaled = value * static_cast<double>(scale);
    const double limit = static_cast<double>(std::numeric_limits<std::int64_t>::max()) * 0.5;
    if (!std::isfinite(scaled) || std::fabs(scaled) > limit)
    {
        return false;
    }
    *result = static_cast<std::int64_t>(std::llround(scaled));
    return true;
}

bool add_path_segments(const Clipper2Lib::PathD& path, std::int64_t scale, std::size_t max_segments,
                       std::vector<ProjectedSegment>* segments, bool* limit_exceeded)
{
    *limit_exceeded = false;
    std::vector<PointKey> points;
    points.reserve(path.size());
    for (const Clipper2Lib::PointD& point : path)
    {
        PointKey key;
        if (!snap(point.x, scale, &key.x) || !snap(point.y, scale, &key.y))
        {
            return false;
        }
        if (points.empty() || points.back().x != key.x || points.back().y != key.y)
        {
            points.push_back(key);
        }
    }
    if (points.size() > 1 && points.front().x == points.back().x &&
        points.front().y == points.back().y)
    {
        points.pop_back();
    }
    if (points.size() < 3)
    {
        return true;
    }
    for (std::size_t index = 0; index < points.size(); ++index)
    {
        const PointKey& start = points[index];
        const PointKey& end = points[(index + 1) % points.size()];
        if (start.x == end.x && start.y == end.y)
        {
            continue;
        }
        if (segments->size() >= max_segments)
        {
            *limit_exceeded = true;
            return false;
        }
        const double inverse = 1.0 / static_cast<double>(scale);
        segments->push_back(
            {start.x * inverse, start.y * inverse, end.x * inverse, end.y * inverse});
    }
    return true;
}

} // namespace

int fast_mesh_shadow_outline_geometry(const FastHlrPreparedMesh& prepared,
                                      const ProjectionViewSpec& view, const FastHlrOptions& options,
                                      std::int64_t scale, ProjectedModeGeometry* geometry,
                                      FastMeshShadowStatistics* statistics, Status* status)
{
    if (geometry == nullptr)
    {
        set_status(status, 2, "Fast mesh-shadow output pointer is null.");
        return 2;
    }
    if (prepared.vertices.size() > options.limits.max_vertices ||
        prepared.triangles.size() > options.limits.max_triangles)
    {
        set_status(status, 3, "Fast mesh-shadow prepared mesh exceeds configured limits.");
        return 3;
    }
    if (!std::isfinite(options.projected_tolerance) || options.projected_tolerance <= 0.0)
    {
        set_status(status, 4, "Fast mesh-shadow projected tolerance must be finite and positive.");
        return 4;
    }
    for (const FastHlrPreparedTriangle& triangle : prepared.triangles)
    {
        for (std::uint32_t vertex : triangle.vertices)
        {
            if (vertex >= prepared.vertices.size())
            {
                set_status(status, 5,
                           "Fast mesh-shadow triangle index is outside the vertex array.");
                return 5;
            }
        }
    }
    ProjectedModeGeometry output;
    FastMeshShadowStatistics output_statistics;
    ViewBasis basis;
    if (!make_view_basis(view, &basis) || scale <= 0)
    {
        set_status(status, 4, "Fast mesh-shadow view or output scale is invalid.");
        return 4;
    }

    std::vector<Clipper2Lib::PointD> vertices;
    vertices.reserve(prepared.vertices.size());
    constexpr double clipper_scale = 1'000'000.0;
    const double clipper_limit =
        static_cast<double>(std::numeric_limits<std::int64_t>::max()) * 0.25 / clipper_scale;
    for (const FastHlrVec3& vertex : prepared.vertices)
    {
        const Clipper2Lib::PointD projected = project_point(vertex, basis);
        if (!std::isfinite(projected.x) || !std::isfinite(projected.y) ||
            std::fabs(projected.x) > clipper_limit || std::fabs(projected.y) > clipper_limit)
        {
            set_status(status, 5, "Fast mesh-shadow coordinates exceed the Clipper grid range.");
            return 5;
        }
        vertices.push_back(projected);
    }
    std::vector<int> signs(prepared.triangles.size(), 0);
    for (std::size_t index = 0; index < prepared.triangles.size(); ++index)
    {
        const FastHlrPreparedTriangle& triangle = prepared.triangles[index];
        const auto& first = vertices[triangle.vertices[0]];
        const auto& second = vertices[triangle.vertices[1]];
        const auto& third = vertices[triangle.vertices[2]];
        const double area = signed_area2(first, second, third);
        const double longest = std::max({std::hypot(second.x - first.x, second.y - first.y),
                                         std::hypot(third.x - second.x, third.y - second.y),
                                         std::hypot(first.x - third.x, first.y - third.y)});
        if (longest > options.projected_tolerance &&
            std::fabs(area) > options.projected_tolerance * longest)
        {
            signs[index] = area > 0.0 ? 1 : -1;
            ++output_statistics.projected_triangles;
        }
    }

    const double simplify_tolerance = 1.0 / static_cast<double>(scale);
    std::size_t remaining_union_pairs = options.limits.max_candidate_pairs;
    Clipper2Lib::PathsD patches;
    Clipper2Lib::PathsD solution;
    if (!patch_paths(prepared, vertices, signs, &patches, &output_statistics))
    {
        if (!union_triangle_faces(prepared, vertices, signs, &remaining_union_pairs, &solution,
                                  &output_statistics))
        {
            set_status(status, 6, "Fast mesh-shadow candidate-pair limit exceeded.");
            return 6;
        }
    }
    else if (!union_paths(patches, &remaining_union_pairs, &solution))
    {
        set_status(status, 6, "Fast mesh-shadow candidate-pair limit exceeded.");
        return 6;
    }
    Clipper2Lib::PathsD final_solution;
    if (!union_paths(solution, &remaining_union_pairs, &final_solution))
    {
        set_status(status, 6, "Fast mesh-shadow candidate-pair limit exceeded.");
        return 6;
    }
    solution = std::move(final_solution);
    solution = Clipper2Lib::SimplifyPaths(solution, simplify_tolerance, false);
    for (const Clipper2Lib::PathD& path : solution)
    {
        bool limit_exceeded = false;
        if (!add_path_segments(path, scale, options.limits.max_output_segments, &output.segments,
                               &limit_exceeded))
        {
            if (limit_exceeded)
            {
                set_status(status, 7, "Fast mesh-shadow output-segment limit exceeded.");
                return 7;
            }
            set_status(status, 5, "Fast mesh-shadow coordinates exceed the output grid range.");
            return 5;
        }
    }
    *geometry = std::move(output);
    if (statistics != nullptr)
    {
        *statistics = output_statistics;
    }
    set_status(status, 0, "");
    return 0;
}

} // namespace geometer
