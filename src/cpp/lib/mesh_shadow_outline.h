#pragma once

#include "geometer/projection.h"

#include <TopoDS_Shape.hxx>

#include <cstdint>

namespace geometer
{

ProjectedModeGeometry mesh_shadow_outline_geometry(const TopoDS_Shape& shape,
                                                   const ProjectionViewSpec& view,
                                                   const HlrProjectionOptions& options,
                                                   std::int64_t scale);

} // namespace geometer
