#pragma once

#include "geometer/exact_construction.h"

#include <array>
#include <cstdint>

namespace geometer::exact
{

struct ExactPoint
{
    ConstructionNodeId x = 0;
    ConstructionNodeId y = 0;
};

struct ExactLine
{
    ExactPoint first;
    ExactPoint second;
};

struct ExactCircle
{
    ExactPoint center;
    ConstructionNodeId radius = 0;
};

enum class IntersectionRelation : std::uint8_t
{
    disjoint = 0,
    point = 1,
    two_points = 2,
    coincident = 3,
};

struct ExactIntersectionResult
{
    Error error = Error::none;
    IntersectionRelation relation = IntersectionRelation::disjoint;
    std::uint32_t point_count = 0;
    std::array<ExactPoint, 2> points{};
};

[[nodiscard]] ExactIntersectionResult
intersect_exact_lines(ConstructionArena& arena, const ExactLine& left, const ExactLine& right);

[[nodiscard]] ExactIntersectionResult intersect_exact_line_circle(ConstructionArena& arena,
                                                                  const ExactLine& line,
                                                                  const ExactCircle& circle);

[[nodiscard]] ExactIntersectionResult intersect_exact_circles(ConstructionArena& arena,
                                                              const ExactCircle& left,
                                                              const ExactCircle& right);

} // namespace geometer::exact
