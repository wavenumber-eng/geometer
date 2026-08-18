#pragma once

#include <cstdint>

namespace geometer
{

struct AnalyticFilteredDistanceNm
{
    double value = 0.0;
    double absolute_error = 0.0;
};

enum class AnalyticResolutionClass : std::uint8_t
{
    invalid = 0,
    at_or_below_resolution = 1,
    above_resolution = 2,
    uncertain = 3,
};

// Classifies a nonnegative distance interval against the fixed contract
// resolution. An interval that touches 50 nm is uncertain unless its complete
// upper bound is at or below 50 nm.
[[nodiscard]] AnalyticResolutionClass
classify_analytic_resolution(const AnalyticFilteredDistanceNm& distance) noexcept;

} // namespace geometer
