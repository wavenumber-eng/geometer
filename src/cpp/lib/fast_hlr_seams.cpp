#include "fast_hlr_seams.h"

#include <array>
#include <cmath>

namespace geometer::fast_hlr_internal
{
namespace
{

double dot(const FastHlrVec3& first, const FastHlrVec3& second)
{
    return first.x * second.x + first.y * second.y + first.z * second.z;
}

double signed_area2(const ProjectedPoint& first, const ProjectedPoint& second,
                    const ProjectedPoint& third)
{
    return (second.x - first.x) * (third.y - first.y) - (second.y - first.y) * (third.x - first.x);
}

double triangle_depth(const ProjectedTriangle& triangle, double x, double y)
{
    return triangle.depth_x * x + triangle.depth_y * y + triangle.depth_constant;
}

bool point_in_projected_triangle(const ProjectedTriangle& triangle, const ProjectedPoint& point,
                                 double tolerance)
{
    for (std::size_t index = 0; index < 3; ++index)
    {
        const ProjectedPoint& first = triangle.points[index];
        const ProjectedPoint& second = triangle.points[(index + 1) % 3];
        const double edge_length = std::hypot(second.x - first.x, second.y - first.y);
        if (signed_area2(first, second, point) < -tolerance * edge_length)
        {
            return false;
        }
    }
    return true;
}

} // namespace

bool coplanar_continuation_interval(const FastHlrPreparedMesh& prepared,
                                    const FastHlrPreparedEdge& edge,
                                    std::uint32_t support_triangle_index,
                                    const std::vector<ProjectedPoint>& vertices,
                                    const std::vector<ProjectedTriangle>& projected_triangles,
                                    const FastHlrVec3& view_direction, const ProjectedPoint& start,
                                    const ProjectedPoint& end, const Interval& overlap,
                                    const FastHlrOptions& options)
{
    if (edge.incident_count < 1 || edge.incident_count > 2)
    {
        return false;
    }
    const FastHlrPreparedTriangle& support = prepared.triangles[support_triangle_index];
    if (support.source_face == kFastHlrUnspecifiedSourceFace)
    {
        return false;
    }
    const double dx = end.x - start.x;
    const double dy = end.y - start.y;
    const double edge_length = std::hypot(dx, dy);
    const double span = overlap.second - overlap.first;
    const double inset = 2.0 * options.coplanar_seam_lateral_tolerance / edge_length;
    if (!(span > 2.0 * inset))
    {
        return false;
    }
    const ProjectedTriangle& projected_support = projected_triangles[support_triangle_index];
    const double cosine_threshold = std::cos(options.coplanar_seam_angle_rad);
    for (std::uint32_t incident = 0; incident < edge.incident_count; ++incident)
    {
        const std::uint32_t incident_index =
            prepared.incident_triangles[edge.first_incident + incident];
        const FastHlrPreparedTriangle& continuation = prepared.triangles[incident_index];
        const double normal_alignment = dot(continuation.normal, support.normal);
        if (continuation.source_face == kFastHlrUnspecifiedSourceFace ||
            continuation.source_face == support.source_face ||
            dot(continuation.normal, view_direction) <= 1.0e-12 ||
            dot(support.normal, view_direction) <= 1.0e-12 || normal_alignment <= 0.0 ||
            normal_alignment < cosine_threshold)
        {
            continue;
        }

        std::uint32_t third_vertex = continuation.vertices[0];
        for (std::uint32_t vertex : continuation.vertices)
        {
            if (vertex != edge.vertices[0] && vertex != edge.vertices[1])
            {
                third_vertex = vertex;
                break;
            }
        }
        const double continuation_side = signed_area2(start, end, vertices[third_vertex]);
        if (std::fabs(continuation_side) <= options.coplanar_seam_lateral_tolerance * edge_length)
        {
            continue;
        }

        bool depth_matches = true;
        for (double parameter : {overlap.first, overlap.second})
        {
            const double x = start.x + dx * parameter;
            const double y = start.y + dy * parameter;
            const double z = start.z + (end.z - start.z) * parameter;
            if (std::fabs(triangle_depth(projected_support, x, y) - z) >
                options.coplanar_seam_depth_tolerance)
            {
                depth_matches = false;
                break;
            }
        }
        if (!depth_matches)
        {
            continue;
        }

        const double support_side_limit = options.projected_tolerance * edge_length;
        const double required_opposite_side = options.coplanar_seam_lateral_tolerance * edge_length;
        bool support_crosses_candidate = false;
        bool support_has_opposite_interior = false;
        for (const ProjectedPoint& point : projected_support.points)
        {
            const double side = signed_area2(start, end, point);
            if ((continuation_side > 0.0 && side > support_side_limit) ||
                (continuation_side < 0.0 && side < -support_side_limit))
            {
                support_crosses_candidate = true;
                break;
            }
            support_has_opposite_interior =
                support_has_opposite_interior ||
                (continuation_side > 0.0 ? side < -required_opposite_side
                                         : side > required_opposite_side);
        }
        if (support_crosses_candidate || !support_has_opposite_interior)
        {
            continue;
        }

        const double normal_x = -dy / edge_length;
        const double normal_y = dx / edge_length;
        const double opposite_sign = continuation_side > 0.0 ? -1.0 : 1.0;
        const std::array<double, 3> probes = {
            overlap.first + inset, (overlap.first + overlap.second) * 0.5, overlap.second - inset};
        bool fills_opposite_side = true;
        for (double parameter : probes)
        {
            const ProjectedPoint probe = {
                start.x + dx * parameter +
                    opposite_sign * normal_x * options.coplanar_seam_lateral_tolerance,
                start.y + dy * parameter +
                    opposite_sign * normal_y * options.coplanar_seam_lateral_tolerance,
                0.0};
            if (!point_in_projected_triangle(projected_support, probe, options.projected_tolerance))
            {
                fills_opposite_side = false;
                break;
            }
        }
        if (fills_opposite_side)
        {
            return true;
        }
    }
    return false;
}

} // namespace geometer::fast_hlr_internal
