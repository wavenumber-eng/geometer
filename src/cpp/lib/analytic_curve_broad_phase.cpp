#include "geometer/analytic_curve_broad_phase.h"

#include "analytic_filtered_execution_policy.h"
#include "analytic_interval_index.h"

#include <algorithm>
#include <array>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <limits>
#include <memory>
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

constexpr std::uint64_t kCanonicalOrderSlotBytes = 8;
constexpr std::uint64_t kCanonicalExpiryEntryBytes = 32;
constexpr std::uint64_t kCanonicalPairBytes = 8;
static_assert(sizeof(std::size_t) <= kCanonicalOrderSlotBytes,
              "size_t exceeds the governed canonical memory charge");
static_assert(sizeof(ExpiryEntry) <= kCanonicalExpiryEntryBytes,
              "expiry entry exceeds the governed canonical memory charge");
static_assert(sizeof(AnalyticCurvePair) <= kCanonicalPairBytes,
              "curve pair exceeds the governed canonical memory charge");

struct BroadWorkBudget
{
    std::uint64_t limit = 0;
    std::uint64_t used = 0;

    bool charge(std::uint64_t units = 1) noexcept
    {
        if (units > limit - used)
            return false;
        used += units;
        return true;
    }

    std::uint64_t remaining() const noexcept
    {
        return limit - used;
    }
};

std::uint64_t ceil_log2(std::uint64_t count) noexcept
{
    std::uint64_t result = 0;
    std::uint64_t value = count > 1 ? count - 1 : 0;
    while (value != 0)
    {
        ++result;
        value >>= 1U;
    }
    return result;
}

std::uint64_t fixed_work_upper_bound(std::uint64_t count) noexcept
{
    // Covers validation/dedup, four deterministic sorts, both axis-overlap
    // scans, the primary sweep, and fixed-capacity heap/AVL updates. Dynamic
    // interval-node visits, emitted candidates, and radix passes are charged
    // separately at the point of use.
    const std::uint64_t depth = ceil_log2(count) + 1;
    if (count > std::numeric_limits<std::uint64_t>::max() / (24 * depth))
        return std::numeric_limits<std::uint64_t>::max();
    return count * 24 * depth;
}

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

double conservative_query_minimum(double minimum, analytic_execution_detail::TopologyPolicy policy)
{
    const double expansion = analytic_execution_detail::allows_resolution_topology(policy)
                                 ? static_cast<double>(kAnalyticTopologyResolutionNm)
                                 : 0.0;
    return std::nextafter(minimum - expansion, -std::numeric_limits<double>::infinity());
}

double conservative_query_maximum(double maximum, analytic_execution_detail::TopologyPolicy policy)
{
    const double expansion = analytic_execution_detail::allows_resolution_topology(policy)
                                 ? static_cast<double>(kAnalyticTopologyResolutionNm)
                                 : 0.0;
    return std::nextafter(maximum + expansion, std::numeric_limits<double>::infinity());
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
                                 const std::vector<std::size_t>& order, std::uint8_t axis,
                                 analytic_execution_detail::TopologyPolicy policy)
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
            conservative_query_minimum(axis_min(bounds[order[position]], axis), policy);
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
    return sum_bytes({bytes_for(count, kCanonicalOrderSlotBytes),
                      detail::AnalyticIntervalIndex::canonical_storage_bytes(count),
                      bytes_for(count, kCanonicalExpiryEntryBytes)});
}

std::uint64_t pair_phase_bytes(std::uint64_t base, std::size_t pair_capacity,
                               std::size_t scratch_size)
{
    return sum_bytes({base, bytes_for(pair_capacity, kCanonicalPairBytes),
                      bytes_for(scratch_size, kCanonicalPairBytes)});
}

bool reserve_next_pair(std::vector<AnalyticCurvePair>& pairs, std::uint64_t base_memory,
                       const AnalyticSolverLimits& limits, std::size_t& logical_capacity,
                       AnalyticBroadPhaseTelemetry& telemetry)
{
    if (pairs.size() < logical_capacity)
        return true;
    std::size_t requested = logical_capacity == 0 ? 64 : logical_capacity * 2;
    requested =
        std::min<std::size_t>(requested, static_cast<std::size_t>(limits.examined_curve_pairs));
    if (requested <= logical_capacity)
        return false;
    const std::uint64_t required_bytes = pair_phase_bytes(base_memory, requested, requested);
    if (required_bytes > limits.working_memory_bytes)
    {
        telemetry.required_working_memory_bytes = required_bytes;
        return false;
    }
    pairs.reserve(requested);
    logical_capacity = requested;
    const std::uint64_t peak = pair_phase_bytes(base_memory, logical_capacity, logical_capacity);
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
analytic_execution_detail::build_curve_candidates(const std::vector<AnalyticCurveBoundsNm>& bounds,
                                                  const AnalyticSolverLimits& limits,
                                                  TopologyPolicy policy)
{
    AnalyticBroadPhaseTelemetry telemetry;
    telemetry.input_curves = bounds.size();
    if (!analytic_solver_limits_within_hard_ceilings(limits) ||
        bounds.size() > limits.boundary_occurrences)
        return failure(AnalyticBroadPhaseError::resource_limit_exceeded, telemetry);

    BroadWorkBudget work{limits.predicate_calls, 0};
    if (!work.charge(fixed_work_upper_bound(bounds.size())))
        return failure(AnalyticBroadPhaseError::resource_limit_exceeded, telemetry);
    telemetry.work_units = work.used;

    const std::uint64_t validation_memory = bytes_for(bounds.size(), sizeof(std::uint32_t));
    const std::uint64_t preindex_memory =
        sum_bytes({bytes_for(bounds.size(), 2 * kCanonicalOrderSlotBytes),
                   bytes_for(bounds.size(), sizeof(double))});
    const std::uint64_t base_sweep_memory = sweep_base_bytes(bounds.size());
    const std::uint64_t base_memory =
        std::max({validation_memory, preindex_memory, base_sweep_memory});
    telemetry.required_working_memory_bytes = base_memory;
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
        const std::uint64_t x_pairs = axis_overlap_count(bounds, x_order, 0, policy);
        const std::uint64_t y_pairs = axis_overlap_count(bounds, y_order, 1, policy);
        telemetry.primary_axis = y_pairs < x_pairs ? 1 : 0;
        telemetry.primary_axis_pairs = telemetry.primary_axis == 0 ? x_pairs : y_pairs;
        order = telemetry.primary_axis == 0 ? std::move(x_order) : std::move(y_order);
    }
    const std::uint8_t secondary_axis = telemetry.primary_axis == 0 ? 1 : 0;

    detail::AnalyticIntervalIndex secondary_index(bounds.size());
    std::unique_ptr<ExpiryEntry[]> expiry =
        bounds.empty() ? nullptr : std::make_unique<ExpiryEntry[]>(bounds.size());
    std::size_t expiry_size = 0;
    std::vector<AnalyticCurvePair> pairs;
    std::size_t logical_pair_capacity = 0;
    for (const std::size_t current_index : order)
    {
        const AnalyticCurveBoundsNm& current = bounds[current_index];
        const double primary_query_minimum =
            conservative_query_minimum(axis_min(current, telemetry.primary_axis), policy);
        while (expiry_size != 0 && expiry[0].primary_maximum < primary_query_minimum)
        {
            std::pop_heap(expiry.get(), expiry.get() + expiry_size, ExpiryLater{});
            const ExpiryEntry expired = expiry[--expiry_size];
            secondary_index.erase(expired.secondary_minimum, expired.curve_index);
        }

        std::uint64_t query_node_visits = 0;
        const bool query_completed = secondary_index.query(
            conservative_query_minimum(axis_min(current, secondary_axis), policy),
            conservative_query_maximum(axis_max(current, secondary_axis), policy),
            query_node_visits, work.remaining() / 2,
            [&](std::size_t other_index, std::uint32_t other_curve_index)
            {
                if (telemetry.examined_curve_pairs == limits.examined_curve_pairs ||
                    !work.charge() ||
                    !reserve_next_pair(pairs, base_sweep_memory, limits, logical_pair_capacity,
                                       telemetry))
                {
                    return false;
                }
                ++telemetry.examined_curve_pairs;
                pairs.push_back({std::min(other_curve_index, current.curve_index),
                                 std::max(other_curve_index, current.curve_index)});
                return true;
            });
        if (!work.charge(query_node_visits))
            return failure(AnalyticBroadPhaseError::resource_limit_exceeded, telemetry);
        telemetry.spatial_index_node_visits += query_node_visits;
        telemetry.work_units = work.used;
        if (!query_completed)
            return failure(AnalyticBroadPhaseError::resource_limit_exceeded, telemetry);

        if (!secondary_index.insert(axis_min(current, secondary_axis),
                                    axis_max(current, secondary_axis), current_index,
                                    current.curve_index))
            return failure(AnalyticBroadPhaseError::resource_limit_exceeded, telemetry);
        expiry[expiry_size++] = {axis_max(current, telemetry.primary_axis),
                                 axis_min(current, secondary_axis), current_index,
                                 current.curve_index};
        std::push_heap(expiry.get(), expiry.get() + expiry_size, ExpiryLater{});
    }

    telemetry.peak_working_memory_bytes =
        std::max(telemetry.peak_working_memory_bytes,
                 pair_phase_bytes(base_sweep_memory, logical_pair_capacity, pairs.size()));
    if (telemetry.peak_working_memory_bytes > limits.working_memory_bytes)
    {
        telemetry.required_working_memory_bytes = telemetry.peak_working_memory_bytes;
        return failure(AnalyticBroadPhaseError::resource_limit_exceeded, telemetry);
    }
    const std::uint64_t radix_work =
        pairs.empty() ? 0 : static_cast<std::uint64_t>(pairs.size()) * 16 + 2'048;
    if (!work.charge(radix_work))
        return failure(AnalyticBroadPhaseError::resource_limit_exceeded, telemetry);
    radix_sort_pairs(pairs);
    telemetry.candidate_pairs = pairs.size();
    telemetry.work_units = work.used;
    telemetry.retained_pair_bytes =
        static_cast<std::uint64_t>(logical_pair_capacity) * kCanonicalPairBytes;
    AnalyticBroadPhaseResult result;
    result.pairs = std::move(pairs);
    result.telemetry = telemetry;
    return result;
}

AnalyticBroadPhaseResult
build_analytic_curve_candidates(const std::vector<AnalyticCurveBoundsNm>& bounds,
                                const AnalyticSolverLimits& limits)
{
    return analytic_execution_detail::build_curve_candidates(
        bounds, limits, analytic_execution_detail::kDefaultTopologyPolicy);
}

} // namespace geometer
