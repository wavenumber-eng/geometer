#pragma once

#include "fast_hlr_view_types.h"
#include "geometer/fast_hlr.h"

#include <cstdint>
#include <vector>

namespace geometer::fast_hlr_internal
{

bool coplanar_continuation_interval(const FastHlrPreparedMesh& prepared,
                                    const FastHlrPreparedEdge& edge,
                                    std::uint32_t support_triangle_index,
                                    const std::vector<ProjectedPoint>& vertices,
                                    const std::vector<ProjectedTriangle>& projected_triangles,
                                    const FastHlrVec3& view_direction, const ProjectedPoint& start,
                                    const ProjectedPoint& end, const Interval& overlap,
                                    const FastHlrOptions& options);

} // namespace geometer::fast_hlr_internal
