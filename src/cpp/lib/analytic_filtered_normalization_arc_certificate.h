#pragma once

#include "analytic_filtered_interval.h"

#include <algorithm>
#include <cmath>

namespace geometer::analytic_normalization_detail
{

struct ArcConstruction
{
    analytic_detail::Point center;
    analytic_detail::Interval radius;
    analytic_detail::Point start;
    analytic_detail::Point end;
    bool counterclockwise = true;
    bool major_arc = false;
};

inline double maximum_point_separation(analytic_detail::Point left,
                                       analytic_detail::Point right) noexcept
{
    return analytic_detail::square_root(analytic_detail::complete_distance_squared(left, right))
        .upper;
}

inline double maximum_radius_separation(analytic_detail::Interval left,
                                        analytic_detail::Interval right) noexcept
{
    const analytic_detail::Interval difference = analytic_detail::subtract(left, right);
    return std::max(std::fabs(difference.lower), std::fabs(difference.upper));
}

// A radial correspondence between the two complete carrier circles moves any
// point by at most C = center_gap + radius_gap. Matching direction/major flags
// choose the same oriented endpoint branch. Each unmatched endpoint tail is
// then bounded by E + C on its source carrier and another E to the matching
// target endpoint, so the complete directed arc distance is at most 2E + C.
// The strict 16 nm sub-budgets keep that proof below the governed 50 nm rule.
inline bool certifies_near_coincident_arc(const ArcConstruction& source,
                                          const ArcConstruction& target) noexcept
{
    constexpr double kCarrierBudgetNm = 16.0;
    constexpr double kEndpointBudgetNm = 16.0;
    if (source.counterclockwise != target.counterclockwise || source.major_arc != target.major_arc)
        return false;
    const double carrier_gap = maximum_point_separation(source.center, target.center) +
                               maximum_radius_separation(source.radius, target.radius);
    if (!std::isfinite(carrier_gap) || carrier_gap > kCarrierBudgetNm)
        return false;
    const double start_gap = maximum_point_separation(source.start, target.start);
    const double end_gap = maximum_point_separation(source.end, target.end);
    return std::isfinite(start_gap) && std::isfinite(end_gap) && start_gap <= kEndpointBudgetNm &&
           end_gap <= kEndpointBudgetNm;
}

} // namespace geometer::analytic_normalization_detail
