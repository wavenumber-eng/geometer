#pragma once

#include <array>

namespace geometer::fast_hlr_internal
{

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

} // namespace geometer::fast_hlr_internal
