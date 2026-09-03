#pragma once

#include "projection.h"
#include "status.h"

#include <array>
#include <cstddef>
#include <cstdint>
#include <limits>
#include <vector>

namespace geometer
{

constexpr std::uint32_t kFastHlrUnspecifiedSourceFace = std::numeric_limits<std::uint32_t>::max();

struct FastHlrVec3
{
    double x = 0.0;
    double y = 0.0;
    double z = 0.0;
};

struct FastHlrIndexedTriangle
{
    std::array<std::uint32_t, 3> vertices = {0, 0, 0};
    // Equal, non-sentinel values identify triangles tessellated from the same
    // smooth source face and suppress false crease edges between them.
    std::uint32_t source_face = kFastHlrUnspecifiedSourceFace;
};

struct FastHlrIndexedMesh
{
    std::vector<FastHlrVec3> vertices;
    std::vector<FastHlrIndexedTriangle> triangles;
};

struct FastHlrPreparedTriangle
{
    std::array<std::uint32_t, 3> vertices = {0, 0, 0};
    FastHlrVec3 normal;
    std::uint32_t source_face = kFastHlrUnspecifiedSourceFace;
};

struct FastHlrPreparedEdge
{
    std::array<std::uint32_t, 2> vertices = {0, 0};
    std::uint32_t first_incident = 0;
    std::uint32_t incident_count = 0;
};

struct FastHlrPreparedMesh
{
    std::vector<FastHlrVec3> vertices;
    std::vector<FastHlrPreparedTriangle> triangles;
    std::vector<FastHlrPreparedEdge> edges;
    std::vector<std::uint32_t> incident_triangles;
};

struct FastHlrStatistics
{
    std::size_t candidate_edges = 0;
    std::size_t boundary_edges = 0;
    std::size_t crease_edges = 0;
    std::size_t silhouette_edges = 0;
    std::size_t candidate_triangle_pairs = 0;
    std::size_t hidden_intervals = 0;
    std::size_t coplanar_seam_intervals = 0;
    std::size_t raw_visible_segments = 0;
    std::size_t raw_hidden_segments = 0;
    std::size_t collinear_joins = 0;
    std::size_t collinear_component_rejections = 0;
    std::size_t visible_segments = 0;
    std::size_t hidden_segments = 0;
};

int prepare_fast_hlr_mesh(const FastHlrIndexedMesh& mesh, const FastHlrOptions& options,
                          FastHlrPreparedMesh* prepared, Status* status = nullptr);

int project_fast_hlr_detail(const FastHlrPreparedMesh& prepared, const ProjectionViewSpec& view,
                            const FastHlrOptions& options, ProjectedModeGeometry* visible,
                            ProjectedModeGeometry* hidden = nullptr,
                            FastHlrStatistics* statistics = nullptr, Status* status = nullptr);

} // namespace geometer
