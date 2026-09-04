#pragma once

#include "geometer/fast_hlr.h"

#include <cstdint>

namespace geometer
{

struct FastMeshShadowStatistics
{
    std::size_t projected_triangles = 0;
    std::size_t boundary_edges = 0;
    std::size_t patch_loops = 0;
    std::size_t fallback_triangles = 0;
};

int fast_mesh_shadow_outline_geometry(const FastHlrPreparedMesh& prepared,
                                      const ProjectionViewSpec& view, const FastHlrOptions& options,
                                      std::int64_t scale, ProjectedModeGeometry* geometry,
                                      FastMeshShadowStatistics* statistics = nullptr,
                                      Status* status = nullptr);

} // namespace geometer
