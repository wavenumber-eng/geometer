#include "geometer/fast_hlr.h"

#include "fast_hlr_reconstruct.h"

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <limits>
#include <map>
#include <utility>
#include <vector>

namespace geometer
{
namespace
{

constexpr std::uint32_t kNoTriangle = std::numeric_limits<std::uint32_t>::max();
constexpr std::uint8_t kBoundaryCategory = 1U;
constexpr std::uint8_t kCreaseCategory = 2U;
constexpr std::uint8_t kSilhouetteCategory = 4U;

struct EdgeKey
{
    std::uint32_t first = 0;
    std::uint32_t second = 0;

    bool operator<(const EdgeKey& other) const
    {
        return first < other.first || (first == other.first && second < other.second);
    }
};

struct Vec2
{
    double x = 0.0;
    double y = 0.0;
};

struct ProjectedPoint
{
    double x = 0.0;
    double y = 0.0;
    double z = 0.0;
};

struct ProjectedTriangle
{
    std::array<ProjectedPoint, 3> points;
    double min_x = 0.0;
    double min_y = 0.0;
    double max_x = 0.0;
    double max_y = 0.0;
    double depth_x = 0.0;
    double depth_y = 0.0;
    double depth_constant = 0.0;
    bool active = false;
};

struct Interval
{
    double first = 0.0;
    double second = 0.0;
};

struct ViewBasis
{
    FastHlrVec3 x;
    FastHlrVec3 y;
    FastHlrVec3 z;
};

void set_status(Status* status, int code, const char* message)
{
    if (status != nullptr)
    {
        status->code = code;
        status->message = message == nullptr ? "" : message;
    }
}

bool finite(const FastHlrVec3& value)
{
    return std::isfinite(value.x) && std::isfinite(value.y) && std::isfinite(value.z);
}

FastHlrVec3 subtract(const FastHlrVec3& left, const FastHlrVec3& right)
{
    return {left.x - right.x, left.y - right.y, left.z - right.z};
}

FastHlrVec3 cross(const FastHlrVec3& left, const FastHlrVec3& right)
{
    return {left.y * right.z - left.z * right.y, left.z * right.x - left.x * right.z,
            left.x * right.y - left.y * right.x};
}

double dot(const FastHlrVec3& left, const FastHlrVec3& right)
{
    return left.x * right.x + left.y * right.y + left.z * right.z;
}

double length(const FastHlrVec3& value)
{
    return std::sqrt(dot(value, value));
}

bool normalized(const FastHlrVec3& value, FastHlrVec3* result)
{
    const double magnitude = length(value);
    if (!std::isfinite(magnitude) || magnitude <= 1.0e-15)
    {
        return false;
    }
    *result = {value.x / magnitude, value.y / magnitude, value.z / magnitude};
    return true;
}

bool make_view_basis(const ProjectionViewSpec& view, ViewBasis* basis)
{
    const FastHlrVec3 direction = {view.direction[0], view.direction[1], view.direction[2]};
    const FastHlrVec3 up = {view.up[0], view.up[1], view.up[2]};
    if (!normalized(direction, &basis->z))
    {
        return false;
    }
    const double up_depth = dot(up, basis->z);
    const FastHlrVec3 projected_up = {up.x - basis->z.x * up_depth, up.y - basis->z.y * up_depth,
                                      up.z - basis->z.z * up_depth};
    if (!normalized(projected_up, &basis->y))
    {
        return false;
    }
    return normalized(cross(basis->y, basis->z), &basis->x);
}

ProjectedPoint project_point(const FastHlrVec3& point, const ViewBasis& basis)
{
    return {dot(point, basis.x), dot(point, basis.y), dot(point, basis.z)};
}

double signed_area2(const ProjectedPoint& first, const ProjectedPoint& second,
                    const ProjectedPoint& third)
{
    return (second.x - first.x) * (third.y - first.y) - (second.y - first.y) * (third.x - first.x);
}

EdgeKey edge_key(std::uint32_t first, std::uint32_t second)
{
    return first < second ? EdgeKey{first, second} : EdgeKey{second, first};
}

bool incident_to(const FastHlrPreparedMesh& prepared, const FastHlrPreparedEdge& edge,
                 std::uint32_t triangle)
{
    for (std::uint32_t index = 0; index < edge.incident_count; ++index)
    {
        if (prepared.incident_triangles[edge.first_incident + index] == triangle)
        {
            return true;
        }
    }
    return false;
}

bool clip_segment_to_triangle(const ProjectedPoint& start, const ProjectedPoint& end,
                              const ProjectedTriangle& triangle, double tolerance,
                              Interval* interval)
{
    double first = 0.0;
    double second = 1.0;
    const double dx = end.x - start.x;
    const double dy = end.y - start.y;
    for (std::size_t index = 0; index < 3; ++index)
    {
        const ProjectedPoint& a = triangle.points[index];
        const ProjectedPoint& b = triangle.points[(index + 1) % 3];
        const double edge_x = b.x - a.x;
        const double edge_y = b.y - a.y;
        const double constant = edge_x * (start.y - a.y) - edge_y * (start.x - a.x);
        const double slope = edge_x * dy - edge_y * dx;
        const double threshold = -tolerance * std::hypot(edge_x, edge_y);
        if (std::fabs(slope) <= 1.0e-18)
        {
            if (constant < threshold)
            {
                return false;
            }
            continue;
        }
        const double crossing = (threshold - constant) / slope;
        if (slope > 0.0)
        {
            first = std::max(first, crossing);
        }
        else
        {
            second = std::min(second, crossing);
        }
        if (first >= second)
        {
            return false;
        }
    }
    interval->first = std::max(0.0, first);
    interval->second = std::min(1.0, second);
    return interval->first < interval->second;
}

bool hidden_subinterval(const ProjectedPoint& start, const ProjectedPoint& end,
                        const ProjectedTriangle& triangle, const Interval& overlap,
                        double depth_tolerance, Interval* hidden)
{
    const double dx = end.x - start.x;
    const double dy = end.y - start.y;
    const double dz = end.z - start.z;
    const double triangle_start =
        triangle.depth_x * start.x + triangle.depth_y * start.y + triangle.depth_constant;
    const double difference_start = triangle_start - start.z;
    const double difference_slope = triangle.depth_x * dx + triangle.depth_y * dy - dz;
    const double first_difference = difference_start + difference_slope * overlap.first;
    const double second_difference = difference_start + difference_slope * overlap.second;
    const bool first_hidden = first_difference > depth_tolerance;
    const bool second_hidden = second_difference > depth_tolerance;
    if (!first_hidden && !second_hidden)
    {
        return false;
    }
    if (first_hidden && second_hidden)
    {
        *hidden = overlap;
        return true;
    }
    if (std::fabs(difference_slope) <= 1.0e-18)
    {
        return false;
    }
    const double crossing = (depth_tolerance - difference_start) / difference_slope;
    if (first_hidden)
    {
        *hidden = {overlap.first, std::min(overlap.second, crossing)};
    }
    else
    {
        *hidden = {std::max(overlap.first, crossing), overlap.second};
    }
    return hidden->first < hidden->second;
}

ProjectedSegment segment_interval(const ProjectedPoint& start, const ProjectedPoint& end,
                                  const Interval& interval)
{
    return {start.x + (end.x - start.x) * interval.first,
            start.y + (end.y - start.y) * interval.first,
            start.x + (end.x - start.x) * interval.second,
            start.y + (end.y - start.y) * interval.second};
}

fast_hlr_internal::FragmentProvenance fragment_provenance(const FastHlrPreparedMesh& prepared,
                                                          const FastHlrPreparedEdge& edge,
                                                          std::size_t edge_index, bool boundary,
                                                          bool crease, bool silhouette)
{
    fast_hlr_internal::FragmentProvenance provenance;
    provenance.category_mask = static_cast<std::uint8_t>((boundary ? kBoundaryCategory : 0U) |
                                                         (crease ? kCreaseCategory : 0U) |
                                                         (silhouette ? kSilhouetteCategory : 0U));
    std::array<std::uint32_t, 2> source_faces = {kFastHlrUnspecifiedSourceFace,
                                                 kFastHlrUnspecifiedSourceFace};
    if (edge.incident_count >= 1 && edge.incident_count <= 2)
    {
        source_faces[0] =
            prepared.triangles[prepared.incident_triangles[edge.first_incident]].source_face;
        if (edge.incident_count == 2)
        {
            source_faces[1] =
                prepared.triangles[prepared.incident_triangles[edge.first_incident + 1]]
                    .source_face;
            if (source_faces[1] < source_faces[0])
            {
                std::swap(source_faces[0], source_faces[1]);
            }
        }
    }
    provenance.first_source_face = source_faces[0];
    provenance.second_source_face = source_faces[1];
    provenance.unique_edge =
        edge.incident_count < 1 || edge.incident_count > 2 ||
                source_faces[0] == kFastHlrUnspecifiedSourceFace ||
                (edge.incident_count == 2 && source_faces[1] == kFastHlrUnspecifiedSourceFace)
            ? static_cast<std::uint32_t>(edge_index)
            : kNoTriangle;
    return provenance;
}

fast_hlr_internal::ProjectedFragment
projected_fragment(const ProjectedPoint& start, const ProjectedPoint& end, const Interval& interval,
                   const FastHlrPreparedEdge& edge,
                   const fast_hlr_internal::FragmentProvenance& provenance, bool reaches_start,
                   bool reaches_end)
{
    fast_hlr_internal::ProjectedFragment fragment;
    fragment.segment = segment_interval(start, end, interval);
    fragment.start_vertex = reaches_start ? edge.vertices[0] : fast_hlr_internal::kNoTopologyVertex;
    fragment.end_vertex = reaches_end ? edge.vertices[1] : fast_hlr_internal::kNoTopologyVertex;
    fragment.provenance = provenance;
    return fragment;
}

bool append_projected_fragment(const ProjectedPoint& start, const ProjectedPoint& end,
                               const Interval& interval, const FastHlrPreparedEdge& edge,
                               const fast_hlr_internal::FragmentProvenance& provenance,
                               bool reaches_start, bool reaches_end,
                               std::size_t other_fragment_count, std::size_t fragment_limit,
                               std::vector<fast_hlr_internal::ProjectedFragment>* fragments)
{
    if (other_fragment_count > fragment_limit ||
        fragments->size() >= fragment_limit - other_fragment_count)
    {
        return false;
    }
    fragments->push_back(
        projected_fragment(start, end, interval, edge, provenance, reaches_start, reaches_end));
    return true;
}

bool edge_is_crease(const FastHlrPreparedMesh& prepared, const FastHlrPreparedEdge& edge,
                    double cosine_threshold)
{
    if (edge.incident_count != 2)
    {
        return false;
    }
    const FastHlrPreparedTriangle& first =
        prepared.triangles[prepared.incident_triangles[edge.first_incident]];
    const FastHlrPreparedTriangle& second =
        prepared.triangles[prepared.incident_triangles[edge.first_incident + 1]];
    if (first.source_face != kFastHlrUnspecifiedSourceFace &&
        first.source_face == second.source_face)
    {
        return false;
    }
    return dot(first.normal, second.normal) < cosine_threshold;
}

bool edge_is_silhouette(const FastHlrPreparedMesh& prepared, const FastHlrPreparedEdge& edge,
                        const FastHlrVec3& direction, double tolerance)
{
    if (edge.incident_count != 2)
    {
        return false;
    }
    const FastHlrPreparedTriangle& first =
        prepared.triangles[prepared.incident_triangles[edge.first_incident]];
    const FastHlrPreparedTriangle& second =
        prepared.triangles[prepared.incident_triangles[edge.first_incident + 1]];
    const double first_facing = dot(first.normal, direction);
    const double second_facing = dot(second.normal, direction);
    const bool first_grazing = std::fabs(first_facing) <= tolerance;
    const bool second_grazing = std::fabs(second_facing) <= tolerance;
    return (first_facing > tolerance && second_facing < -tolerance) ||
           (first_facing < -tolerance && second_facing > tolerance) ||
           (first_grazing != second_grazing);
}

class TriangleGrid
{
  public:
    TriangleGrid(const std::vector<ProjectedTriangle>& triangles, double min_x, double min_y,
                 double max_x, double max_y, std::size_t max_references, bool* limit_exceeded)
        : min_x_(min_x), min_y_(min_y), max_x_(max_x), max_y_(max_y)
    {
        *limit_exceeded = false;
        const double count = static_cast<double>(std::max<std::size_t>(triangles.size(), 1));
        dimension_ = static_cast<std::size_t>(std::ceil(std::sqrt(count / 8.0)));
        dimension_ = std::max<std::size_t>(1, std::min<std::size_t>(dimension_, 256));
        cells_.resize(dimension_ * dimension_);
        for (std::uint32_t triangle = 0; triangle < triangles.size(); ++triangle)
        {
            const ProjectedTriangle& item = triangles[triangle];
            if (!item.active)
            {
                continue;
            }
            const auto x_range = cell_range(item.min_x, item.max_x, min_x_, max_x_);
            const auto y_range = cell_range(item.min_y, item.max_y, min_y_, max_y_);
            for (std::size_t y = y_range.first; y <= y_range.second; ++y)
            {
                for (std::size_t x = x_range.first; x <= x_range.second; ++x)
                {
                    if (reference_count_ >= max_references)
                    {
                        *limit_exceeded = true;
                        return;
                    }
                    cells_[y * dimension_ + x].push_back(triangle);
                    ++reference_count_;
                }
            }
        }
    }

    void query(double min_x, double min_y, double max_x, double max_y,
               std::vector<std::uint32_t>* triangles) const
    {
        triangles->clear();
        const auto x_range = cell_range(min_x, max_x, min_x_, max_x_);
        const auto y_range = cell_range(min_y, max_y, min_y_, max_y_);
        for (std::size_t y = y_range.first; y <= y_range.second; ++y)
        {
            for (std::size_t x = x_range.first; x <= x_range.second; ++x)
            {
                const std::vector<std::uint32_t>& cell = cells_[y * dimension_ + x];
                triangles->insert(triangles->end(), cell.begin(), cell.end());
            }
        }
        std::sort(triangles->begin(), triangles->end());
        triangles->erase(std::unique(triangles->begin(), triangles->end()), triangles->end());
    }

  private:
    std::pair<std::size_t, std::size_t> cell_range(double first, double second, double minimum,
                                                   double maximum) const
    {
        if (!(maximum > minimum))
        {
            return {0, 0};
        }
        const auto index = [&](double value)
        {
            const double normalized_value = (value - minimum) / (maximum - minimum);
            const double scaled = normalized_value * static_cast<double>(dimension_);
            const auto candidate = static_cast<std::int64_t>(std::floor(scaled));
            return static_cast<std::size_t>(std::max<std::int64_t>(
                0, std::min<std::int64_t>(candidate, static_cast<std::int64_t>(dimension_ - 1))));
        };
        const std::size_t first_index = index(std::min(first, second));
        const std::size_t second_index = index(std::max(first, second));
        return {first_index, second_index};
    }

    double min_x_ = 0.0;
    double min_y_ = 0.0;
    double max_x_ = 0.0;
    double max_y_ = 0.0;
    std::size_t dimension_ = 1;
    std::size_t reference_count_ = 0;
    std::vector<std::vector<std::uint32_t>> cells_;
};

bool valid_options(const FastHlrOptions& options)
{
    return std::isfinite(options.crease_angle_rad) && options.crease_angle_rad >= 0.0 &&
           options.crease_angle_rad <= 3.14159265358979323846 &&
           std::isfinite(options.weld_tolerance) && options.weld_tolerance > 0.0 &&
           std::isfinite(options.projected_tolerance) && options.projected_tolerance > 0.0 &&
           std::isfinite(options.depth_tolerance) && options.depth_tolerance >= 0.0;
}

bool valid_prepared_mesh(const FastHlrPreparedMesh& prepared)
{
    for (const FastHlrVec3& vertex : prepared.vertices)
    {
        if (!finite(vertex))
        {
            return false;
        }
    }
    for (const FastHlrPreparedTriangle& triangle : prepared.triangles)
    {
        if (!finite(triangle.normal))
        {
            return false;
        }
        for (std::uint32_t vertex : triangle.vertices)
        {
            if (vertex >= prepared.vertices.size())
            {
                return false;
            }
        }
    }
    for (const FastHlrPreparedEdge& edge : prepared.edges)
    {
        if (edge.vertices[0] >= prepared.vertices.size() ||
            edge.vertices[1] >= prepared.vertices.size() ||
            edge.first_incident > prepared.incident_triangles.size() ||
            edge.incident_count > prepared.incident_triangles.size() - edge.first_incident)
        {
            return false;
        }
        for (std::uint32_t index = 0; index < edge.incident_count; ++index)
        {
            if (prepared.incident_triangles[edge.first_incident + index] >=
                prepared.triangles.size())
            {
                return false;
            }
        }
    }
    return true;
}

} // namespace

int prepare_fast_hlr_mesh(const FastHlrIndexedMesh& mesh, const FastHlrOptions& options,
                          FastHlrPreparedMesh* prepared, Status* status)
{
    if (prepared == nullptr)
    {
        set_status(status, 2, "Fast HLR prepared-mesh pointer is null.");
        return 2;
    }
    if (!valid_options(options))
    {
        set_status(status, 4, "Fast HLR options contain an invalid angle or tolerance.");
        return 4;
    }
    if (mesh.vertices.size() > options.limits.max_vertices ||
        mesh.triangles.size() > options.limits.max_triangles ||
        mesh.vertices.size() > std::numeric_limits<std::uint32_t>::max() ||
        mesh.triangles.size() > std::numeric_limits<std::uint32_t>::max())
    {
        set_status(status, 3, "Fast HLR input exceeds vertex or triangle limits.");
        return 3;
    }
    for (const FastHlrVec3& vertex : mesh.vertices)
    {
        if (!finite(vertex))
        {
            set_status(status, 4, "Fast HLR input contains a non-finite vertex.");
            return 4;
        }
    }

    FastHlrPreparedMesh output;
    output.vertices = mesh.vertices;
    output.triangles.reserve(mesh.triangles.size());
    std::map<EdgeKey, std::vector<std::uint32_t>> incidents;
    for (const FastHlrIndexedTriangle& triangle : mesh.triangles)
    {
        for (std::uint32_t vertex : triangle.vertices)
        {
            if (vertex >= mesh.vertices.size())
            {
                set_status(status, 5, "Fast HLR triangle index is outside the vertex array.");
                return 5;
            }
        }
        const FastHlrVec3& first = mesh.vertices[triangle.vertices[0]];
        const FastHlrVec3& second = mesh.vertices[triangle.vertices[1]];
        const FastHlrVec3& third = mesh.vertices[triangle.vertices[2]];
        FastHlrVec3 normal;
        if (!normalized(cross(subtract(second, first), subtract(third, first)), &normal))
        {
            continue;
        }
        if (output.triangles.size() >= options.limits.max_triangles)
        {
            set_status(status, 3, "Fast HLR prepared triangles exceed the configured limit.");
            return 3;
        }
        const auto triangle_index = static_cast<std::uint32_t>(output.triangles.size());
        output.triangles.push_back({triangle.vertices, normal, triangle.source_face});
        incidents[edge_key(triangle.vertices[0], triangle.vertices[1])].push_back(triangle_index);
        incidents[edge_key(triangle.vertices[1], triangle.vertices[2])].push_back(triangle_index);
        incidents[edge_key(triangle.vertices[2], triangle.vertices[0])].push_back(triangle_index);
    }
    if (incidents.size() > options.limits.max_edges)
    {
        set_status(status, 3, "Fast HLR prepared edges exceed the configured limit.");
        return 3;
    }
    output.edges.reserve(incidents.size());
    const std::size_t max_incident_triangles =
        options.limits.max_triangles > std::numeric_limits<std::size_t>::max() / 3
            ? std::numeric_limits<std::size_t>::max()
            : options.limits.max_triangles * 3;
    for (const auto& entry : incidents)
    {
        if (output.incident_triangles.size() > max_incident_triangles ||
            entry.second.size() > max_incident_triangles - output.incident_triangles.size() ||
            output.incident_triangles.size() > std::numeric_limits<std::uint32_t>::max() ||
            entry.second.size() > std::numeric_limits<std::uint32_t>::max())
        {
            set_status(status, 3, "Fast HLR edge incidence exceeds the configured limit.");
            return 3;
        }
        FastHlrPreparedEdge edge;
        edge.vertices = {entry.first.first, entry.first.second};
        edge.first_incident = static_cast<std::uint32_t>(output.incident_triangles.size());
        edge.incident_count = static_cast<std::uint32_t>(entry.second.size());
        output.incident_triangles.insert(output.incident_triangles.end(), entry.second.begin(),
                                         entry.second.end());
        output.edges.push_back(edge);
    }

    *prepared = std::move(output);
    set_status(status, 0, "");
    return 0;
}

int project_fast_hlr_detail(const FastHlrPreparedMesh& prepared, const ProjectionViewSpec& view,
                            const FastHlrOptions& options, ProjectedModeGeometry* visible,
                            ProjectedModeGeometry* hidden, FastHlrStatistics* statistics,
                            Status* status)
{
    if (visible == nullptr)
    {
        set_status(status, 2, "Fast HLR visible-output pointer is null.");
        return 2;
    }
    if (!valid_options(options))
    {
        set_status(status, 4, "Fast HLR options contain an invalid angle or tolerance.");
        return 4;
    }
    const std::size_t max_incident_triangles =
        options.limits.max_triangles > std::numeric_limits<std::size_t>::max() / 3
            ? std::numeric_limits<std::size_t>::max()
            : options.limits.max_triangles * 3;
    if (prepared.vertices.size() > options.limits.max_vertices ||
        prepared.triangles.size() > options.limits.max_triangles ||
        prepared.edges.size() > options.limits.max_edges ||
        prepared.incident_triangles.size() > max_incident_triangles)
    {
        set_status(status, 3, "Fast HLR prepared mesh exceeds configured limits.");
        return 3;
    }
    if (!valid_prepared_mesh(prepared))
    {
        set_status(status, 5, "Fast HLR prepared mesh contains invalid indices or values.");
        return 5;
    }
    ViewBasis basis;
    if (!make_view_basis(view, &basis))
    {
        set_status(status, 4, "Fast HLR view direction and up vectors must define a basis.");
        return 4;
    }

    std::vector<ProjectedPoint> vertices;
    vertices.reserve(prepared.vertices.size());
    for (const FastHlrVec3& vertex : prepared.vertices)
    {
        vertices.push_back(project_point(vertex, basis));
    }

    std::vector<ProjectedTriangle> triangles(prepared.triangles.size());
    double bounds_min_x = std::numeric_limits<double>::infinity();
    double bounds_min_y = std::numeric_limits<double>::infinity();
    double bounds_max_x = -std::numeric_limits<double>::infinity();
    double bounds_max_y = -std::numeric_limits<double>::infinity();
    for (std::size_t index = 0; index < prepared.triangles.size(); ++index)
    {
        const FastHlrPreparedTriangle& source = prepared.triangles[index];
        ProjectedTriangle& triangle = triangles[index];
        triangle.points = {vertices[source.vertices[0]], vertices[source.vertices[1]],
                           vertices[source.vertices[2]]};
        double area2 = signed_area2(triangle.points[0], triangle.points[1], triangle.points[2]);
        const double longest_edge =
            std::max({std::hypot(triangle.points[1].x - triangle.points[0].x,
                                 triangle.points[1].y - triangle.points[0].y),
                      std::hypot(triangle.points[2].x - triangle.points[1].x,
                                 triangle.points[2].y - triangle.points[1].y),
                      std::hypot(triangle.points[0].x - triangle.points[2].x,
                                 triangle.points[0].y - triangle.points[2].y)});
        if (longest_edge <= options.projected_tolerance ||
            std::fabs(area2) <= options.projected_tolerance * longest_edge)
        {
            continue;
        }
        if (area2 < 0.0)
        {
            std::swap(triangle.points[1], triangle.points[2]);
            area2 = -area2;
        }
        triangle.min_x =
            std::min({triangle.points[0].x, triangle.points[1].x, triangle.points[2].x});
        triangle.min_y =
            std::min({triangle.points[0].y, triangle.points[1].y, triangle.points[2].y});
        triangle.max_x =
            std::max({triangle.points[0].x, triangle.points[1].x, triangle.points[2].x});
        triangle.max_y =
            std::max({triangle.points[0].y, triangle.points[1].y, triangle.points[2].y});
        const ProjectedPoint& first = triangle.points[0];
        const ProjectedPoint& second = triangle.points[1];
        const ProjectedPoint& third = triangle.points[2];
        triangle.depth_x = ((second.z - first.z) * (third.y - first.y) -
                            (third.z - first.z) * (second.y - first.y)) /
                           area2;
        triangle.depth_y = ((second.x - first.x) * (third.z - first.z) -
                            (third.x - first.x) * (second.z - first.z)) /
                           area2;
        triangle.depth_constant = first.z - triangle.depth_x * first.x - triangle.depth_y * first.y;
        triangle.active = true;
        bounds_min_x = std::min(bounds_min_x, triangle.min_x);
        bounds_min_y = std::min(bounds_min_y, triangle.min_y);
        bounds_max_x = std::max(bounds_max_x, triangle.max_x);
        bounds_max_y = std::max(bounds_max_y, triangle.max_y);
    }

    ProjectedModeGeometry visible_output;
    ProjectedModeGeometry hidden_output;
    std::vector<fast_hlr_internal::ProjectedFragment> visible_fragments;
    std::vector<fast_hlr_internal::ProjectedFragment> hidden_fragments;
    FastHlrStatistics output_statistics;
    if (!std::isfinite(bounds_min_x))
    {
        *visible = std::move(visible_output);
        if (hidden != nullptr)
        {
            *hidden = std::move(hidden_output);
        }
        if (statistics != nullptr)
        {
            *statistics = output_statistics;
        }
        set_status(status, 0, "");
        return 0;
    }

    bool grid_limit_exceeded = false;
    TriangleGrid grid(triangles, bounds_min_x, bounds_min_y, bounds_max_x, bounds_max_y,
                      options.limits.max_grid_references, &grid_limit_exceeded);
    if (grid_limit_exceeded)
    {
        set_status(status, 6, "Fast HLR spatial-index reference limit exceeded.");
        return 6;
    }
    const double cosine_threshold = std::cos(options.crease_angle_rad);
    std::vector<std::uint32_t> possible_occluders;
    std::vector<Interval> hidden_intervals;
    for (std::size_t edge_index = 0; edge_index < prepared.edges.size(); ++edge_index)
    {
        const FastHlrPreparedEdge& edge = prepared.edges[edge_index];
        const bool boundary = edge.incident_count != 2;
        const bool crease =
            options.include_creases && edge_is_crease(prepared, edge, cosine_threshold);
        const bool silhouette =
            options.include_silhouettes && edge_is_silhouette(prepared, edge, basis.z, 1.0e-12);
        if (!((options.include_boundaries && boundary) || crease || silhouette))
        {
            continue;
        }
        ++output_statistics.candidate_edges;
        output_statistics.boundary_edges += boundary ? 1 : 0;
        output_statistics.crease_edges += crease ? 1 : 0;
        output_statistics.silhouette_edges += silhouette ? 1 : 0;
        const fast_hlr_internal::FragmentProvenance provenance =
            fragment_provenance(prepared, edge, edge_index, boundary, crease, silhouette);

        const ProjectedPoint& start = vertices[edge.vertices[0]];
        const ProjectedPoint& end = vertices[edge.vertices[1]];
        if (std::hypot(end.x - start.x, end.y - start.y) <= options.projected_tolerance)
        {
            continue;
        }
        grid.query(std::min(start.x, end.x), std::min(start.y, end.y), std::max(start.x, end.x),
                   std::max(start.y, end.y), &possible_occluders);
        hidden_intervals.clear();
        for (std::uint32_t triangle_index : possible_occluders)
        {
            if (incident_to(prepared, edge, triangle_index))
            {
                continue;
            }
            ++output_statistics.candidate_triangle_pairs;
            if (output_statistics.candidate_triangle_pairs > options.limits.max_candidate_pairs)
            {
                set_status(status, 6, "Fast HLR candidate-pair limit exceeded.");
                return 6;
            }
            const ProjectedTriangle& triangle = triangles[triangle_index];
            Interval overlap;
            if (!triangle.active ||
                !clip_segment_to_triangle(start, end, triangle, options.projected_tolerance,
                                          &overlap))
            {
                continue;
            }
            Interval occluded;
            if (hidden_subinterval(start, end, triangle, overlap, options.depth_tolerance,
                                   &occluded))
            {
                hidden_intervals.push_back(occluded);
            }
        }
        std::sort(hidden_intervals.begin(), hidden_intervals.end(),
                  [](const Interval& left, const Interval& right)
                  {
                      return left.first < right.first ||
                             (left.first == right.first && left.second < right.second);
                  });
        std::vector<Interval> merged;
        for (const Interval& interval : hidden_intervals)
        {
            if (merged.empty() || interval.first > merged.back().second + 1.0e-12)
            {
                merged.push_back(interval);
            }
            else
            {
                merged.back().second = std::max(merged.back().second, interval.second);
            }
        }
        output_statistics.hidden_intervals += merged.size();
        double cursor = 0.0;
        for (const Interval& interval : merged)
        {
            if (interval.first > cursor + 1.0e-12)
            {
                if (!append_projected_fragment(start, end, {cursor, interval.first}, edge,
                                               provenance, cursor == 0.0, false,
                                               hidden_fragments.size(),
                                               options.limits.max_fragments, &visible_fragments))
                {
                    set_status(status, 6, "Fast HLR raw-fragment limit exceeded.");
                    return 6;
                }
            }
            if (options.include_hidden && hidden != nullptr)
            {
                if (!append_projected_fragment(start, end, interval, edge, provenance,
                                               interval.first == 0.0, interval.second == 1.0,
                                               visible_fragments.size(),
                                               options.limits.max_fragments, &hidden_fragments))
                {
                    set_status(status, 6, "Fast HLR raw-fragment limit exceeded.");
                    return 6;
                }
            }
            cursor = std::max(cursor, interval.second);
        }
        if (cursor < 1.0 - 1.0e-12)
        {
            if (!append_projected_fragment(start, end, {cursor, 1.0}, edge, provenance,
                                           cursor == 0.0, true, hidden_fragments.size(),
                                           options.limits.max_fragments, &visible_fragments))
            {
                set_status(status, 6, "Fast HLR raw-fragment limit exceeded.");
                return 6;
            }
        }
    }
    output_statistics.raw_visible_segments = visible_fragments.size();
    output_statistics.raw_hidden_segments = hidden_fragments.size();
    fast_hlr_internal::ReconstructionStatistics visible_reconstruction;
    fast_hlr_internal::ReconstructionStatistics hidden_reconstruction;
    visible_output.segments = fast_hlr_internal::reconstruct_collinear_fragments(
        visible_fragments, &visible_reconstruction);
    hidden_output.segments = fast_hlr_internal::reconstruct_collinear_fragments(
        hidden_fragments, &hidden_reconstruction);
    if (visible_output.segments.size() > options.limits.max_output_segments ||
        hidden_output.segments.size() >
            options.limits.max_output_segments - visible_output.segments.size())
    {
        set_status(status, 7, "Fast HLR output-segment limit exceeded.");
        return 7;
    }
    output_statistics.collinear_joins = visible_reconstruction.joins + hidden_reconstruction.joins;
    output_statistics.collinear_component_rejections =
        visible_reconstruction.rejected + hidden_reconstruction.rejected;
    output_statistics.visible_segments = visible_output.segments.size();
    output_statistics.hidden_segments = hidden_output.segments.size();
    *visible = std::move(visible_output);
    if (hidden != nullptr)
    {
        *hidden = std::move(hidden_output);
    }
    if (statistics != nullptr)
    {
        *statistics = output_statistics;
    }
    set_status(status, 0, "");
    return 0;
}

} // namespace geometer
