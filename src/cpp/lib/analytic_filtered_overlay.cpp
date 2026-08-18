#include "geometer/analytic_filtered_overlay.h"

#include "analytic_filtered_capacity.h"
#include "analytic_filtered_execution_policy.h"
#include "analytic_filtered_interval.h"

#include <algorithm>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <limits>
#include <new>
#include <tuple>
#include <utility>
#include <vector>

namespace geometer
{
namespace
{
using analytic_detail::add;
using analytic_detail::complete_distance_squared;
using analytic_detail::cross;
using analytic_detail::dot;
using analytic_detail::enclosure_radius_squared;
using analytic_detail::exact;
using analytic_detail::Interval;
using analytic_detail::negate;
using analytic_detail::Point;
using analytic_detail::subtract;

constexpr std::uint32_t kNoIndex = std::numeric_limits<std::uint32_t>::max();
constexpr std::uint64_t kCurveLogicalBytes = 48;
constexpr std::uint64_t kGroupLogicalBytes = 48;
constexpr std::uint64_t kRawEventLogicalBytes = analytic_detail::kOverlayRawEventLogicalBytes;
constexpr std::uint64_t kUniqueEventLogicalBytes = analytic_detail::kOverlayUniqueEventLogicalBytes;
constexpr std::uint64_t kActionLogicalBytes = analytic_detail::kOverlayActionLogicalBytes;
constexpr std::uint64_t kSpanLogicalBytes = kAnalyticOverlaySpanLogicalBytes;
constexpr std::uint64_t kMembershipLogicalBytes = 8;

bool singleton_integer_point(const AnalyticFilteredPointNm& point) noexcept
{
    return point.x.lower == point.x.upper && point.y.lower == point.y.upper &&
           std::isfinite(point.x.lower) && std::isfinite(point.y.lower) &&
           std::nearbyint(point.x.lower) == point.x.lower &&
           std::nearbyint(point.y.lower) == point.y.lower;
}

enum class EventRole : std::uint8_t
{
    domain_end = 0,
    domain_start = 1,
    split = 2,
    circle_seam = 3,
    circle_right_partition = 4,
};

enum class DomainMode : std::uint8_t
{
    normal = 0,
    collapsed = 1,
    full_circle = 2,
};

struct CarrierGroup
{
    std::uint64_t carrier_id = 0;
    std::uint32_t curve_begin = 0;
    std::uint32_t curve_count = 0;
    std::uint32_t event_begin = 0;
    std::uint32_t event_count = 0;
    std::uint32_t point_begin = 0;
    std::uint32_t point_count = 0;
    std::uint32_t seam_local = 0;
    std::uint32_t representative_curve = 0;
    AnalyticAtomicCurveKind kind = AnalyticAtomicCurveKind::line;
};

struct RawEvent
{
    AnalyticFilteredPointNm point;
    std::uint64_t carrier_id = 0;
    std::uint32_t curve_index = 0;
    std::uint32_t point_index = 0;
    std::uint32_t rank = 0;
    EventRole role = EventRole::split;
    AnalyticAtomicCurveKind kind = AnalyticAtomicCurveKind::line;
    double key_x = 0.0;
    double key_y = 0.0;
};

struct UniqueEvent
{
    AnalyticFilteredPointNm point;
    // The published point may be an exact construction seam. Retain the ordered
    // support endpoints separately so a later <=50 nm merge cannot compose two
    // individually valid repairs through that representative.
    AnalyticFilteredPointNm proof_first;
    AnalyticFilteredPointNm proof_last;
    bool has_intersection = false;
    bool has_endpoint = false;
    bool has_circle_seam = false;
    bool has_circle_right_partition = false;
};

struct EndpointAction
{
    std::uint64_t carrier_id = 0;
    std::uint32_t rank = 0;
    std::uint32_t curve_index = 0;
    std::uint32_t local_curve = 0;
    EventRole role = EventRole::domain_end;
};

std::uint64_t checked_add(std::uint64_t left, std::uint64_t right, bool& valid) noexcept
{
    if (right > std::numeric_limits<std::uint64_t>::max() - left)
    {
        valid = false;
        return 0;
    }
    return left + right;
}

std::uint64_t checked_multiply(std::uint64_t left, std::uint64_t right, bool& valid) noexcept
{
    if (left != 0 && right > std::numeric_limits<std::uint64_t>::max() / left)
    {
        valid = false;
        return 0;
    }
    return left * right;
}

std::uint64_t sort_units(std::uint64_t count) noexcept
{
    if (count < 2)
        return 0;
    std::uint64_t levels = 0;
    for (std::uint64_t width = 1; width < count; width <<= 1)
    {
        ++levels;
        if (width > std::numeric_limits<std::uint64_t>::max() / 2)
            break;
    }
    return count * levels;
}

double midpoint(const AnalyticCoordinateIntervalNm& value) noexcept
{
    return value.lower + (value.upper - value.lower) * 0.5;
}

Point point(const AnalyticFilteredPointNm& value) noexcept
{
    return {{value.x.lower, value.x.upper}, {value.y.lower, value.y.upper}};
}

AnalyticFilteredPointNm public_point(Point value) noexcept
{
    return {{value.x.lower, value.x.upper}, {value.y.lower, value.y.upper}};
}

bool valid_coordinate(const AnalyticCoordinateIntervalNm& value) noexcept
{
    return std::isfinite(value.lower) && std::isfinite(value.upper) && value.lower <= value.upper;
}

bool valid_point(const AnalyticFilteredPointNm& value) noexcept
{
    if (!valid_coordinate(value.x) || !valid_coordinate(value.y))
        return false;
    return enclosure_radius_squared(point(value)).upper <=
           static_cast<double>(kAnalyticTopologyResolutionNm * kAnalyticTopologyResolutionNm);
}

bool same_singleton_point(const AnalyticFilteredPointNm& left,
                          const AnalyticFilteredPointNm& right) noexcept
{
    return left.x.lower == left.x.upper && left.y.lower == left.y.upper &&
           right.x.lower == right.x.upper && right.y.lower == right.y.upper &&
           left.x.lower == right.x.lower && left.y.lower == right.y.lower;
}

bool points_within_resolution(const AnalyticFilteredPointNm& left,
                              const AnalyticFilteredPointNm& right) noexcept
{
    const Interval distance_squared = complete_distance_squared(point(left), point(right));
    return distance_squared.upper <=
           static_cast<double>(kAnalyticTopologyResolutionNm * kAnalyticTopologyResolutionNm);
}

AnalyticFilteredPointNm point_hull(const AnalyticFilteredPointNm& left,
                                   const AnalyticFilteredPointNm& right) noexcept
{
    const std::uint64_t column =
        left.construction_x_column_id == right.construction_x_column_id
            ? left.construction_x_column_id
        : left.construction_x_column_id == 0  ? right.construction_x_column_id
        : right.construction_x_column_id == 0 ? left.construction_x_column_id
                                              : 0;
    return {{std::min(left.x.lower, right.x.lower), std::max(left.x.upper, right.x.upper)},
            {std::min(left.y.lower, right.y.lower), std::max(left.y.upper, right.y.upper)},
            column};
}

double approximate_circle_key(double x, double y) noexcept
{
    const double absolute_x = std::fabs(x);
    const double absolute_y = std::fabs(y);
    const double denominator = absolute_x + absolute_y;
    if (denominator == 0.0)
        return 0.0;
    if (y >= 0.0)
        return x >= 0.0 ? absolute_y / denominator : 1.0 + absolute_x / denominator;
    return x < 0.0 ? 2.0 + absolute_y / denominator : 3.0 + absolute_x / denominator;
}

bool raw_event_less(const RawEvent& left, const RawEvent& right) noexcept
{
    if (left.carrier_id != right.carrier_id)
        return left.carrier_id < right.carrier_id;
    return std::tie(left.key_x, left.key_y, left.point.x.lower, left.point.x.upper,
                    left.point.y.lower, left.point.y.upper, left.curve_index, left.role) <
           std::tie(right.key_x, right.key_y, right.point.x.lower, right.point.x.upper,
                    right.point.y.lower, right.point.y.upper, right.curve_index, right.role);
}

class ActiveCurves
{
  public:
    explicit ActiveCurves(std::uint32_t capacity)
        : fenwick_(static_cast<std::size_t>(capacity) + 1), next_(capacity, kNoIndex),
          previous_(capacity, kNoIndex), active_(capacity, false)
    {
    }

    void reset(std::uint32_t size)
    {
        size_ = size;
        count_ = 0;
        head_ = kNoIndex;
        std::fill(fenwick_.begin(), fenwick_.begin() + size + 1, 0);
        std::fill(next_.begin(), next_.begin() + size, kNoIndex);
        std::fill(previous_.begin(), previous_.begin() + size, kNoIndex);
        std::fill(active_.begin(), active_.begin() + size, false);
    }

    bool contains(std::uint32_t index) const noexcept
    {
        return index < size_ && active_[index];
    }

    bool insert(std::uint32_t index) noexcept
    {
        if (index >= size_ || active_[index])
            return false;
        const std::uint32_t before = prefix(index);
        const std::uint32_t predecessor = before == 0 ? kNoIndex : select(before);
        const std::uint32_t successor = predecessor == kNoIndex ? head_ : next_[predecessor];
        previous_[index] = predecessor;
        next_[index] = successor;
        if (predecessor == kNoIndex)
            head_ = index;
        else
            next_[predecessor] = index;
        if (successor != kNoIndex)
            previous_[successor] = index;
        add(index, 1);
        active_[index] = true;
        ++count_;
        return true;
    }

    bool erase(std::uint32_t index) noexcept
    {
        if (index >= size_ || !active_[index])
            return false;
        const std::uint32_t predecessor = previous_[index];
        const std::uint32_t successor = next_[index];
        if (predecessor == kNoIndex)
            head_ = successor;
        else
            next_[predecessor] = successor;
        if (successor != kNoIndex)
            previous_[successor] = predecessor;
        add(index, -1);
        active_[index] = false;
        next_[index] = previous_[index] = kNoIndex;
        --count_;
        return true;
    }

    std::uint32_t count() const noexcept
    {
        return count_;
    }

    std::uint32_t head() const noexcept
    {
        return head_;
    }

    std::uint32_t next(std::uint32_t index) const noexcept
    {
        return next_[index];
    }

  private:
    void add(std::uint32_t index, int delta) noexcept
    {
        for (std::uint32_t cursor = index + 1; cursor <= size_; cursor += cursor & (~cursor + 1))
            fenwick_[cursor] =
                static_cast<std::uint32_t>(static_cast<int>(fenwick_[cursor]) + delta);
    }

    std::uint32_t prefix(std::uint32_t count) const noexcept
    {
        std::uint32_t result = 0;
        for (std::uint32_t cursor = count; cursor != 0; cursor &= cursor - 1)
            result += fenwick_[cursor];
        return result;
    }

    std::uint32_t select(std::uint32_t order) const noexcept
    {
        std::uint32_t index = 0;
        std::uint32_t bit = 1;
        while (bit <= size_ / 2)
            bit <<= 1;
        for (; bit != 0; bit >>= 1)
        {
            const std::uint32_t candidate = index + bit;
            if (candidate <= size_ && fenwick_[candidate] < order)
            {
                index = candidate;
                order -= fenwick_[candidate];
            }
        }
        return index;
    }

    std::vector<std::uint32_t> fenwick_;
    std::vector<std::uint32_t> next_;
    std::vector<std::uint32_t> previous_;
    std::vector<bool> active_;
    std::uint32_t size_ = 0;
    std::uint32_t count_ = 0;
    std::uint32_t head_ = kNoIndex;
};

#include "analytic_filtered_overlay_builder.h"

} // namespace

AnalyticFilteredOverlayResult
analytic_execution_detail::build_overlay(const AnalyticFilteredGeometry& geometry,
                                         const std::vector<AnalyticCurvePair>& candidate_pairs,
                                         const AnalyticSolverLimits& limits, TopologyPolicy policy)
{
    AnalyticFilteredOverlayResult preflight;
    preflight.telemetry.input_curves = geometry.curves.size();
    if (!analytic_solver_limits_within_hard_ceilings(limits) ||
        geometry.curves.size() != geometry.bounds.size() ||
        geometry.curves.size() != geometry.occurrences.size())
    {
        preflight.error = AnalyticFilteredOverlayError::invalid_argument;
        return preflight;
    }
    if (geometry.curves.size() > limits.boundary_occurrences ||
        candidate_pairs.size() > limits.examined_curve_pairs)
    {
        preflight.error = AnalyticFilteredOverlayError::resource_limit_exceeded;
        return preflight;
    }

    // A successful integrated stage must retain one narrow result for every
    // candidate while also holding the curve/group tables. Reject this known
    // minimum before narrow reserves or evaluates any pair.
    bool valid = true;
    const std::uint64_t pair_count = static_cast<std::uint64_t>(candidate_pairs.size());
    const std::uint64_t curve_count = static_cast<std::uint64_t>(geometry.curves.size());
    std::uint64_t minimum_memory =
        checked_multiply(pair_count, kAnalyticNarrowPhasePairLogicalBytes, valid);
    minimum_memory = checked_add(
        minimum_memory,
        checked_multiply(curve_count, kAnalyticOverlayCurveGroupLogicalBytes, valid), valid);

    // Every valid pair consumes at least one narrow predicate. Overlay input
    // validation consumes one unit per curve and retained pair, and canonical
    // endpoint/cardinal group validation consumes five units per curve.
    std::uint64_t minimum_work = checked_multiply(pair_count, 2, valid);
    minimum_work = checked_add(minimum_work, checked_multiply(curve_count, 6, valid), valid);
    if (!valid || minimum_memory > limits.working_memory_bytes ||
        minimum_work > limits.predicate_calls)
    {
        if (valid && minimum_memory > limits.working_memory_bytes)
            preflight.telemetry.required_working_memory_bytes = minimum_memory;
        preflight.error = AnalyticFilteredOverlayError::resource_limit_exceeded;
        return preflight;
    }

    const AnalyticNarrowPhaseResult narrow_phase =
        intersect_curve_candidates(geometry.curves, candidate_pairs, limits, policy);
    if (narrow_phase.error != AnalyticNarrowPhaseError::none)
    {
        AnalyticFilteredOverlayResult result;
        result.error = narrow_phase.error == AnalyticNarrowPhaseError::invalid_argument
                           ? AnalyticFilteredOverlayError::invalid_argument
                           : AnalyticFilteredOverlayError::resource_limit_exceeded;
        result.telemetry.input_curves = geometry.curves.size();
        result.telemetry.input_pair_results = narrow_phase.intersections.size();
        result.telemetry.narrow_phase_predicate_calls = narrow_phase.telemetry.predicate_calls;
        result.telemetry.narrow_phase_peak_working_memory_bytes =
            narrow_phase.telemetry.peak_working_memory_bytes;
        result.telemetry.unresolved_predicate_failure =
            narrow_phase.telemetry.unresolved_predicate_failure;
        result.telemetry.predicate_calls = narrow_phase.telemetry.predicate_calls;
        result.telemetry.peak_working_memory_bytes =
            narrow_phase.telemetry.peak_working_memory_bytes;
        result.telemetry.required_working_memory_bytes =
            narrow_phase.telemetry.required_working_memory_bytes;
        result.telemetry.algebraic_fallback_calls = narrow_phase.telemetry.algebraic_fallback_calls;
        return result;
    }
    return OverlayBuilder(geometry, narrow_phase, limits, policy).build();
}

AnalyticFilteredOverlayResult
build_analytic_filtered_overlay(const AnalyticFilteredGeometry& geometry,
                                const std::vector<AnalyticCurvePair>& candidate_pairs,
                                const AnalyticSolverLimits& limits)
{
    return analytic_execution_detail::build_overlay(
        geometry, candidate_pairs, limits, analytic_execution_detail::kDefaultTopologyPolicy);
}

} // namespace geometer
