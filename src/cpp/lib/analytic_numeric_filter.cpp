#include "geometer/analytic_numeric_filter.h"

#include "geometer/analytic_solver_limits.h"

#include <algorithm>
#include <cmath>

namespace geometer
{

AnalyticResolutionClass
classify_analytic_resolution(const AnalyticFilteredDistanceNm& distance) noexcept
{
    if (!std::isfinite(distance.value) || !std::isfinite(distance.absolute_error) ||
        distance.value < 0.0 || distance.absolute_error < 0.0)
        return AnalyticResolutionClass::invalid;

    const double lower = std::max(0.0, distance.value - distance.absolute_error);
    const double upper = distance.value + distance.absolute_error;
    const double resolution = static_cast<double>(kAnalyticTopologyResolutionNm);
    if (upper <= resolution)
        return AnalyticResolutionClass::at_or_below_resolution;
    if (lower > resolution)
        return AnalyticResolutionClass::above_resolution;
    return AnalyticResolutionClass::uncertain;
}

} // namespace geometer
