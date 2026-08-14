#pragma once

#include "geometer/exact_geometry.h"

#include <cstdint>
#include <optional>

namespace geometer::exact
{

struct ExactCircularArc
{
    ExactCircle circle;
    ExactPoint start;
    ExactPoint end;
    bool counterclockwise = true;
    bool major_arc = false;
};

struct ExactPredicateResult
{
    Error error = Error::none;
    std::optional<bool> value;
};

struct ExactOrderingResult
{
    Error error = Error::none;
    std::optional<std::int8_t> ordering;
};

[[nodiscard]] ExactPredicateResult
exact_points_equal(ConstructionArena& arena, const ExactPoint& left, const ExactPoint& right);

[[nodiscard]] ExactPredicateResult
same_exact_line_carrier(ConstructionArena& arena, const ExactLine& left, const ExactLine& right);

[[nodiscard]] ExactPredicateResult same_exact_circle_carrier(ConstructionArena& arena,
                                                             const ExactCircle& left,
                                                             const ExactCircle& right);

[[nodiscard]] ExactPredicateResult point_on_closed_exact_segment(ConstructionArena& arena,
                                                                 const ExactLine& segment,
                                                                 const ExactPoint& point);

[[nodiscard]] ExactPredicateResult point_on_closed_exact_arc(ConstructionArena& arena,
                                                             const ExactCircularArc& arc,
                                                             const ExactPoint& point);

[[nodiscard]] ExactOrderingResult compare_points_on_exact_line(ConstructionArena& arena,
                                                               const ExactLine& carrier,
                                                               const ExactPoint& left,
                                                               const ExactPoint& right);

[[nodiscard]] ExactOrderingResult compare_points_on_exact_circle(ConstructionArena& arena,
                                                                 const ExactCircle& carrier,
                                                                 const ExactPoint& left,
                                                                 const ExactPoint& right);

} // namespace geometer::exact
