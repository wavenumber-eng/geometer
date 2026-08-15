#include "geometer/analytic_curve_broad_phase.h"

#include "analytic_interval_index.h"

#include <algorithm>
#include <array>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <limits>
#include <queue>
#include <tuple>
#include <utility>
#include <vector>

namespace geometer
{
namespace
{

struct ExpiryEntry
{
    double primary_maximum = 0.0;
    double secondary_minimum = 0.0;
    std::size_t payload = 0;
    std::uint32_t curve_index = 0;
};

struct ExpiryLater
{
    bool operator()(const ExpiryEntry& left, const ExpiryEntry& right) const noexcept
    {
        return std::tie(left.primary_maximum, left.curve_index) >
               std::tie(right.primary_maximum, right.curve_index);
    }
};

bool valid_bounds(const AnalyticCurveBoundsNm& bounds)
{
    return std::isfinite(bounds.min_x) && std::isfinite(bounds.min_y) &&
           std::isfinite(bounds.max_x) && std::isfinite(bounds.max_y) &&
           bounds.min_x <= bounds.max_x && bounds.min_y <= bounds.max_y;
}

double axis_min(const AnalyticCurveBoundsNm& bounds, std::uint8_t axis)
{
    return axis == 0 ? bounds.min_x : bounds.min_y;
}

double axis_max(const AnalyticCurveBoundsNm& bounds, std::uint8_t axis)
{
    return axis == 0 ? bounds.max_x : bounds.max_y;
}

double conservative_query_minimum(double minimum)
{
    return std::nextafter(minimum - static_cast<double>(kAnalyticTopologyResolutionNm),
                          -std::numeric_limits<double>::infinity());
}

double conservative_query_maximum(double maximum)
{
    return std::nextafter(maximum + static_cast<double>(kAnalyticTopologyResolutionNm),
                          std::numeric_limits<double>::infinity());
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
        const double query_minimum =
            conservative_query_minimum(axis_min(bounds[order[position]], axis));
        while (expired < position && ordered_ends[expired] < query_minimum)
            ++expired;
        const std::uint64_t active_count = position - expired;
        if (active_count > std::numeric_limits<std::uint64_t>::max() - count)
            return std::numeric_limits<std::uint64_t>::max();
        count += active_count;
    }
    return count;
}

std::uint64_t bytes_for(std::size_t count, std::uint64_t item_size)
{
    if (count > std::numeric_limits<std::uint64_t>::max() / item_size)
        return std::numeric_limits<std::uint64_t>::max();
    return static_cast<std::uint64_t>(count) * item_size;
}

std::uint64_t sum_bytes(std::initializer_list<std::uint64_t> values)
{
    std::uint64_t result = 0;
    for (const std::uint64_t value : values)
    {
        if (value > std::numeric_limits<std::uint64_t>::max() - result)
            return std::numeric_limits<std::uint64_t>::max();
        result += value;
    }
    return result;
}

std::uint64_t sweep_base_bytes(std::size_t count)
{
    return sum_bytes({bytes_for(count, sizeof(std::size_t)),
                      detail::AnalyticIntervalIndex::storage_bytes(count),
                      bytes_for(count, sizeof(ExpiryEntry))});
}

std::uint64_t pair_phase_bytes(std::uint64_t base, std::size_t pair_capacity,
                               std::size_t scratch_size)
{
    return sum_bytes({base, bytes_for(pair_capacity, sizeof(AnalyticCurvePair)),
                      bytes_for(scratch_size, sizeof(AnalyticCurvePair))});
}

bool reserve_next_pair(std::vector<AnalyticCurvePair>& pairs, std::uint64_t base_memory,
                       const AnalyticSolverLimits& limits, AnalyticBroadPhaseTelemetry& telemetry)
{
    if (pairs.size() < pairs.capacity())
        return true;
    std::size_t requested = pairs.capacity() == 0 ? 64 : pairs.capacity() * 2;
    requested =
        std::min<std::size_t>(requested, static_cast<std::size_t>(limits.examined_curve_pairs));
    if (requested <= pairs.capacity() ||
        pair_phase_bytes(base_memory, requested, requested) > limits.working_memory_bytes)
        return false;
    pairs.reserve(requested);
    const std::uint64_t peak = pair_phase_bytes(base_memory, pairs.capacity(), pairs.capacity());
    if (peak > limits.working_memory_bytes)
        return false;
    telemetry.peak_working_memory_bytes = std::max(telemetry.peak_working_memory_bytes, peak);
    return true;
}

std::uint64_t pair_key(const AnalyticCurvePair& pair)
{
    return (static_cast<std::uint64_t>(pair.first) << 32U) | pair.second;
}

void radix_sort_pairs(std::vector<AnalyticCurvePair>& pairs)
{
    if (pairs.size() < 2)
        return;
    std::vector<AnalyticCurvePair> scratch(pairs.size());
    bool source_is_pairs = true;
    for (std::uint32_t shift = 0; shift < 64; shift += 8)
    {
        std::array<std::size_t, 256> offsets{};
        const std::vector<AnalyticCurvePair>& source = source_is_pairs ? pairs : scratch;
        std::vector<AnalyticCurvePair>& destination = source_is_pairs ? scratch : pairs;
        for (const AnalyticCurvePair& pair : source)
            ++offsets[(pair_key(pair) >> shift) & 0xffU];
        std::size_t total = 0;
        for (std::size_t& offset : offsets)
        {
            const std::size_t count = offset;
            offset = total;
            total += count;
        }
        for (const AnalyticCurvePair& pair : source)
            destination[offsets[(pair_key(pair) >> shift) & 0xffU]++] = pair;
        source_is_pairs = !source_is_pairs;
    }
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

    const std::uint64_t validation_memory = bytes_for(bounds.size(), sizeof(std::uint32_t));
    const std::uint64_t preindex_memory =
        sum_bytes({bytes_for(bounds.size(), 2 * sizeof(std::size_t)),
                   bytes_for(bounds.size(), sizeof(double))});
    const std::uint64_t base_sweep_memory = sweep_base_bytes(bounds.size());
    const std::uint64_t base_memory =
        std::max({validation_memory, preindex_memory, base_sweep_memory});
    if (base_memory > limits.working_memory_bytes)
        return failure(AnalyticBroadPhaseError::resource_limit_exceeded, telemetry);
    telemetry.peak_working_memory_bytes = base_memory;

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

    std::vector<std::size_t> order;
    {
        std::vector<std::size_t> x_order = sorted_on_axis(bounds, 0, telemetry.sort_comparisons);
        std::vector<std::size_t> y_order = sorted_on_axis(bounds, 1, telemetry.sort_comparisons);
        const std::uint64_t x_pairs = axis_overlap_count(bounds, x_order, 0);
        const std::uint64_t y_pairs = axis_overlap_count(bounds, y_order, 1);
        telemetry.primary_axis = y_pairs < x_pairs ? 1 : 0;
        telemetry.primary_axis_pairs = telemetry.primary_axis == 0 ? x_pairs : y_pairs;
        order = telemetry.primary_axis == 0 ? std::move(x_order) : std::move(y_order);
    }
    const std::uint8_t secondary_axis = telemetry.primary_axis == 0 ? 1 : 0;

    detail::AnalyticIntervalIndex secondary_index(bounds.size());
    std::priority_queue<ExpiryEntry, std::vector<ExpiryEntry>, ExpiryLater> expiry;
    std::vector<AnalyticCurvePair> pairs;
    for (const std::size_t current_index : order)
    {
        const AnalyticCurveBoundsNm& current = bounds[current_index];
        const double primary_query_minimum =
            conservative_query_minimum(axis_min(current, telemetry.primary_axis));
        while (!expiry.empty() && expiry.top().primary_maximum < primary_query_minimum)
        {
            const ExpiryEntry expired = expiry.top();
            expiry.pop();
            secondary_index.erase(expired.secondary_minimum, expired.curve_index);
        }

        const bool query_completed = secondary_index.query(
            conservative_query_minimum(axis_min(current, secondary_axis)),
            conservative_query_maximum(axis_max(current, secondary_axis)),
            telemetry.spatial_index_node_visits, limits.predicate_calls,
            [&](std::size_t other_index, std::uint32_t other_curve_index)
            {
                if (telemetry.examined_curve_pairs == limits.examined_curve_pairs ||
                    !reserve_next_pair(pairs, base_sweep_memory, limits, telemetry))
                {
                    return false;
                }
                ++telemetry.examined_curve_pairs;
                pairs.push_back({std::min(other_curve_index, current.curve_index),
                                 std::max(other_curve_index, current.curve_index)});
                return true;
            });
        if (!query_completed)
            return failure(AnalyticBroadPhaseError::resource_limit_exceeded, telemetry);

        secondary_index.insert(axis_min(current, secondary_axis), axis_max(current, secondary_axis),
                               current_index, current.curve_index);
        expiry.push({axis_max(current, telemetry.primary_axis), axis_min(current, secondary_axis),
                     current_index, current.curve_index});
    }

    telemetry.peak_working_memory_bytes =
        std::max(telemetry.peak_working_memory_bytes,
                 pair_phase_bytes(base_sweep_memory, pairs.capacity(), pairs.size()));
    if (telemetry.peak_working_memory_bytes > limits.working_memory_bytes)
        return failure(AnalyticBroadPhaseError::resource_limit_exceeded, telemetry);
    radix_sort_pairs(pairs);
    telemetry.candidate_pairs = pairs.size();
    AnalyticBroadPhaseResult result;
    result.pairs = std::move(pairs);
    result.telemetry = telemetry;
    return result;
}

} // namespace geometer
