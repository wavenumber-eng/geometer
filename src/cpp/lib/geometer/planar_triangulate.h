#pragma once

#include "status.h"

#include <cstddef>
#include <cstdint>
#include <vector>

namespace geometer
{

struct PlanarTriangulatePoint
{
    double x = 0.0;
    double y = 0.0;
};

using PlanarTriangulateRing = std::vector<PlanarTriangulatePoint>;

struct PlanarTriangulateRegion
{
    PlanarTriangulateRing outline;
    std::vector<PlanarTriangulateRing> holes;
};

struct PlanarTriangulateOptions
{
    int decimal_precision = 6;
};

struct PlanarTriangulateInput
{
    PlanarTriangulateOptions options;
    std::vector<PlanarTriangulateRegion> regions;
};

enum class PlanarTriangulateStatus : std::uint32_t
{
    Ok = 0,
    Fail = 1,
    NoPolygons = 2,
    PathsIntersect = 3,
    InputTooSmall = 4,
};

struct PlanarTriangulateRegionResult
{
    PlanarTriangulateStatus status = PlanarTriangulateStatus::Ok;
    // Indices reference into the merged [outline, hole_0, hole_1, ...] point list,
    // in input order. Each consecutive triple (a, b, c) defines a triangle.
    std::vector<std::uint32_t> indices;
};

struct PlanarTriangulateResult
{
    std::vector<PlanarTriangulateRegionResult> regions;
};

int triangulate_planar(const PlanarTriangulateInput& input, PlanarTriangulateResult* result,
                       Status* status = nullptr);

int triangulate_planar_from_bytes(const unsigned char* request_data, std::size_t request_size,
                                  std::vector<unsigned char>* response_bytes,
                                  Status* status = nullptr);

} // namespace geometer
