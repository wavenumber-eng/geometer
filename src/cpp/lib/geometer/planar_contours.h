#pragma once

#include "status.h"

#include <vector>

namespace geometer
{

struct PlanarContourPoint
{
    double x = 0.0;
    double y = 0.0;
};

struct PlanarContourSegment
{
    PlanarContourPoint start;
    PlanarContourPoint end;
};

struct PlanarContourRing
{
    std::vector<PlanarContourPoint> points;
    bool hole = false;
    int nesting_depth = 0;
    double signed_area = 0.0;
};

struct PlanarContourOptions
{
    int round_digits = 3;
    bool union_polygons = true;
    double area_epsilon = 1.0e-9;
};

struct PlanarContourResult
{
    std::vector<PlanarContourRing> rings;
    std::vector<PlanarContourSegment> segments;
};

int build_planar_contours(const std::vector<PlanarContourSegment>& segments,
                          const PlanarContourOptions& options, PlanarContourResult* result,
                          Status* status = nullptr);

} // namespace geometer
