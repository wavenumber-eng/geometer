#include "geometer/analytic_curve_broad_phase.h"

#include <algorithm>
#include <cmath>
#include <cstddef>
#include <limits>
#include <tuple>
#include <utility>

namespace geometer
{
namespace
{

bool valid_bounds(const AnalyticCurveBoundsNm& bounds)
{
    return std::isfinite(bounds.min_x) && std::isfinite(bounds.min_y) &&
           std::isfinite(bounds.max_x) && std::isfinite(bounds.max_y) &&
           bounds.min_x <= bounds.max_x && bounds.min_y <= bounds.max_y;
}

bool separated_above_resolution(double first_min, double first_max, double second_min,
                                double second_max)
{
    const double resolution = static_cast<double>(kAnalyticTopologyResolutionNm);
    if (second_min > first_max)
        return second_min - first_max > resolution;
    if (first_min > second_max)
        return first_min - second_max > resolution;
    return false;
}

bool pair_less(const AnalyticCurvePair& left, const AnalyticCurvePair& right)
{
    return std::tie(left.first, left.second) < std::tie(right.first, right.second);
}

double axis_min(const AnalyticCurveBoundsNm& bounds, std::uint8_t axis)
{
    return axis == 0 ? bounds.min_x : bounds.min_y;
}

double axis_max(const AnalyticCurveBoundsNm& bounds, std::uint8_t axis)
{
    return axis == 0 ? bounds.max_x : bounds.max_y;
}

std::vector<std::size_t> sorted_on_axis(const std::vector<AnalyticCurveBoundsNm>& bounds,
                                        std::uint8_t axis, std::uint64_t& comparisons)
{
    std::vector<std::size_t> order(bounds.size());
    for (std::size_t index = 0; index < order.size(); ++index)
        order[index] = index;
    std::sort(order.begin(), order.end(),
              [&](std::size_t left, std::size_t right)
              {
                  ++comparisons;
                  const AnalyticCurveBoundsNm& a = bounds[left];
                  const AnalyticCurveBoundsNm& b = bounds[right];
                  const std::uint8_t secondary = axis == 0 ? 1 : 0;
                  return std::make_tuple(axis_min(a, axis), axis_min(a, secondary),
                                         axis_max(a, axis), axis_max(a, secondary), a.curve_index) <
                         std::make_tuple(axis_min(b, axis), axis_min(b, secondary),
                                         axis_max(b, axis), axis_max(b, secondary), b.curve_index);
              });
    return order;
}

std::uint64_t axis_overlap_count(const std::vector<AnalyticCurveBoundsNm>& bounds,
                                 const std::vector<std::size_t>& order, std::uint8_t axis)
{
    std::vector<double> ordered_ends;
    ordered_ends.reserve(order.size());
    for (const std::size_t index : order)
        ordered_ends.push_back(axis_max(bounds[index], axis));
    std::sort(ordered_ends.begin(), ordered_ends.end());

    std::uint64_t count = 0;
    std::size_t expired = 0;
    for (std::size_t position = 0; position < order.size(); ++position)
    {
        const std::size_t current_index = order[position];
        const double current_min = axis_min(bounds[current_index], axis);
        while (expired < position && current_min - ordered_ends[expired] >
                                         static_cast<double>(kAnalyticTopologyResolutionNm))
            ++expired;
        const std::uint64_t active_count = position - expired;
        if (active_count > std::numeric_limits<std::uint64_t>::max() - count)
            return std::numeric_limits<std::uint64_t>::max();
        count += active_count;
    }
    return count;
}

AnalyticBroadPhaseResult failure(AnalyticBroadPhaseError error,
                                 AnalyticBroadPhaseTelemetry telemetry)
{
    AnalyticBroadPhaseResult result;
    result.error = error;
    result.telemetry = telemetry;
    return result;
}

} // namespace

AnalyticBroadPhaseResult
build_analytic_curve_candidates(const std::vector<AnalyticCurveBoundsNm>& bounds,
                                const AnalyticSolverLimits& limits)
{
    AnalyticBroadPhaseTelemetry telemetry;
    telemetry.input_curves = bounds.size();
    if (!analytic_solver_limits_within_hard_ceilings(limits) ||
        bounds.size() > limits.boundary_occurrences)
        return failure(AnalyticBroadPhaseError::resource_limit_exceeded, telemetry);

    constexpr std::uint64_t workspace_bytes_per_curve = 3 * sizeof(std::size_t);
    const std::uint64_t base_working_memory = bounds.size() * workspace_bytes_per_curve;
    if (base_working_memory > limits.working_memory_bytes)
        return failure(AnalyticBroadPhaseError::resource_limit_exceeded, telemetry);
    telemetry.peak_working_memory_bytes = base_working_memory;

    {
        std::vector<std::uint32_t> curve_indices;
        curve_indices.reserve(bounds.size());
        for (const AnalyticCurveBoundsNm& item : bounds)
        {
            if (!valid_bounds(item))
                return failure(AnalyticBroadPhaseError::invalid_argument, telemetry);
            curve_indices.push_back(item.curve_index);
        }
        std::sort(curve_indices.begin(), curve_indices.end());
        if (std::adjacent_find(curve_indices.begin(), curve_indices.end()) != curve_indices.end())
            return failure(AnalyticBroadPhaseError::invalid_argument, telemetry);
    }

    const std::vector<std::size_t> x_order = sorted_on_axis(bounds, 0, telemetry.sort_comparisons);
    const std::vector<std::size_t> y_order = sorted_on_axis(bounds, 1, telemetry.sort_comparisons);
    const std::uint64_t x_pairs = axis_overlap_count(bounds, x_order, 0);
    const std::uint64_t y_pairs = axis_overlap_count(bounds, y_order, 1);
    const std::uint8_t primary_axis = y_pairs < x_pairs ? 1 : 0;
    const std::uint8_t secondary_axis = primary_axis == 0 ? 1 : 0;
    const std::vector<std::size_t>& order = primary_axis == 0 ? x_order : y_order;
    telemetry.primary_axis = primary_axis;
    telemetry.primary_axis_pairs = primary_axis == 0 ? x_pairs : y_pairs;

    std::vector<std::size_t> active;
    active.reserve(bounds.size());
    std::vector<AnalyticCurvePair> pairs;
    const std::uint64_t pair_capacity = std::min(
        {telemetry.primary_axis_pairs, limits.predicate_calls, limits.candidate_curve_pairs,
         (limits.working_memory_bytes - base_working_memory) /
             static_cast<std::uint64_t>(sizeof(AnalyticCurvePair))});
    pairs.reserve(static_cast<std::size_t>(pair_capacity));
    telemetry.peak_working_memory_bytes =
        base_working_memory + pair_capacity * sizeof(AnalyticCurvePair);
    for (const std::size_t current_index : order)
    {
        const AnalyticCurveBoundsNm& current = bounds[current_index];
        active.erase(std::remove_if(active.begin(), active.end(),
                                    [&](std::size_t index)
                                    {
                                        return separated_above_resolution(
                                            axis_min(bounds[index], primary_axis),
                                            axis_max(bounds[index], primary_axis),
                                            axis_min(current, primary_axis),
                                            axis_max(current, primary_axis));
                                    }),
                     active.end());

        for (const std::size_t other_index : active)
        {
            if (telemetry.active_pair_tests == limits.predicate_calls)
                return failure(AnalyticBroadPhaseError::resource_limit_exceeded, telemetry);
            ++telemetry.active_pair_tests;
            const AnalyticCurveBoundsNm& other = bounds[other_index];
            if (separated_above_resolution(
                    axis_min(other, secondary_axis), axis_max(other, secondary_axis),
                    axis_min(current, secondary_axis), axis_max(current, secondary_axis)))
                continue;
            if (pairs.size() == pair_capacity)
                return failure(AnalyticBroadPhaseError::resource_limit_exceeded, telemetry);
            pairs.push_back({std::min(other.curve_index, current.curve_index),
                             std::max(other.curve_index, current.curve_index)});
        }
        active.push_back(current_index);
    }

    std::sort(pairs.begin(), pairs.end(), pair_less);
    telemetry.candidate_pairs = pairs.size();
    AnalyticBroadPhaseResult result;
    result.pairs = std::move(pairs);
    result.telemetry = telemetry;
    return result;
}

} // namespace geometer
