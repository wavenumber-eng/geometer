#include "geometer/analytic_filtered_overlay.h"

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
using analytic_detail::cross;
using analytic_detail::dot;
using analytic_detail::exact;
using analytic_detail::Interval;
using analytic_detail::negate;
using analytic_detail::Point;
using analytic_detail::square;
using analytic_detail::subtract;

constexpr std::uint32_t kNoIndex = std::numeric_limits<std::uint32_t>::max();
constexpr std::uint64_t kCurveLogicalBytes = 48;
constexpr std::uint64_t kGroupLogicalBytes = 48;
constexpr std::uint64_t kRawEventLogicalBytes = 96;
constexpr std::uint64_t kUniqueEventLogicalBytes = 48;
constexpr std::uint64_t kActionLogicalBytes = 24;
constexpr std::uint64_t kSpanLogicalBytes = 96;
constexpr std::uint64_t kMembershipLogicalBytes = 8;

enum class EventRole : std::uint8_t
{
    domain_end = 0,
    domain_start = 1,
    split = 2,
    circle_seam = 3,
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
    bool has_intersection = false;
    bool has_endpoint = false;
    bool has_circle_seam = false;
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
    const double half_x = (value.x.upper - value.x.lower) * 0.5;
    const double half_y = (value.y.upper - value.y.lower) * 0.5;
    return half_x * half_x + half_y * half_y <=
           static_cast<double>(kAnalyticTopologyResolutionNm * kAnalyticTopologyResolutionNm);
}

bool points_within_resolution(const AnalyticFilteredPointNm& left,
                              const AnalyticFilteredPointNm& right) noexcept
{
    const Point delta = subtract(point(left), point(right));
    const Interval distance_squared = add(square(delta.x), square(delta.y));
    return distance_squared.upper <=
           static_cast<double>(kAnalyticTopologyResolutionNm * kAnalyticTopologyResolutionNm);
}

AnalyticFilteredPointNm point_hull(const AnalyticFilteredPointNm& left,
                                   const AnalyticFilteredPointNm& right) noexcept
{
    return {{std::min(left.x.lower, right.x.lower), std::max(left.x.upper, right.x.upper)},
            {std::min(left.y.lower, right.y.lower), std::max(left.y.upper, right.y.upper)}};
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

class OverlayBuilder
{
  public:
    OverlayBuilder(const AnalyticFilteredGeometry& geometry,
                   const AnalyticNarrowPhaseResult& narrow_phase, AnalyticSolverLimits limits)
        : geometry_(geometry), narrow_(narrow_phase), limits_(limits)
    {
        result_.telemetry.narrow_phase_predicate_calls = narrow_.telemetry.predicate_calls;
        result_.telemetry.narrow_phase_peak_working_memory_bytes =
            narrow_.telemetry.peak_working_memory_bytes;
        result_.telemetry.predicate_calls = narrow_.telemetry.predicate_calls;
        result_.telemetry.peak_working_memory_bytes = narrow_.telemetry.peak_working_memory_bytes;
        result_.telemetry.algebraic_fallback_calls = narrow_.telemetry.algebraic_fallback_calls;
    }

    AnalyticFilteredOverlayResult build()
    {
        try
        {
            if (!validate_inputs() || !build_groups() || !build_events() || !deduplicate_events() ||
                !build_actions() || !count_output() || !allocate_output() || !emit_output())
            {
                result_.spans.clear();
                result_.memberships.clear();
                return result_;
            }
            result_.telemetry.emitted_spans = result_.spans.size();
            result_.telemetry.emitted_memberships = result_.memberships.size();
        }
        catch (const std::bad_alloc&)
        {
            result_.error = AnalyticFilteredOverlayError::resource_limit_exceeded;
            result_.spans.clear();
            result_.memberships.clear();
        }
        return result_;
    }

  private:
    bool fail(AnalyticFilteredOverlayError error)
    {
        result_.error = error;
        return false;
    }

    bool charge(std::uint64_t units)
    {
        if (units > limits_.predicate_calls - result_.telemetry.predicate_calls)
            return fail(AnalyticFilteredOverlayError::resource_limit_exceeded);
        result_.telemetry.predicate_calls += units;
        return true;
    }

    bool charge_sort(std::uint64_t count)
    {
        const std::uint64_t units = sort_units(count);
        if (!charge(units))
            return false;
        result_.telemetry.sort_work_units += units;
        return true;
    }

    bool set_base_memory(std::uint64_t raw_count)
    {
        bool valid = true;
        std::uint64_t bytes = narrow_.telemetry.peak_working_memory_bytes;
        bytes = checked_add(
            bytes, checked_multiply(geometry_.curves.size(), kCurveLogicalBytes, valid), valid);
        bytes = checked_add(
            bytes, checked_multiply(geometry_.curves.size(), kGroupLogicalBytes, valid), valid);
        bytes =
            checked_add(bytes, checked_multiply(raw_count, kRawEventLogicalBytes, valid), valid);
        bytes =
            checked_add(bytes, checked_multiply(raw_count, kUniqueEventLogicalBytes, valid), valid);
        bytes = checked_add(
            bytes, checked_multiply(geometry_.curves.size() * 2, kActionLogicalBytes, valid),
            valid);
        if (!valid || bytes > limits_.working_memory_bytes)
            return fail(AnalyticFilteredOverlayError::resource_limit_exceeded);
        base_memory_bytes_ = bytes;
        result_.telemetry.peak_working_memory_bytes = bytes;
        return true;
    }

    bool validate_inputs()
    {
        if (!analytic_solver_limits_within_hard_ceilings(limits_) ||
            narrow_.error != AnalyticNarrowPhaseError::none ||
            geometry_.curves.size() != geometry_.bounds.size() ||
            geometry_.curves.size() != geometry_.occurrences.size())
            return fail(AnalyticFilteredOverlayError::invalid_argument);
        if (geometry_.curves.size() > limits_.boundary_occurrences ||
            narrow_.intersections.size() > limits_.examined_curve_pairs)
            return fail(AnalyticFilteredOverlayError::resource_limit_exceeded);
        bool valid_memory = true;
        std::uint64_t initial_memory = narrow_.telemetry.peak_working_memory_bytes;
        initial_memory =
            checked_add(initial_memory,
                        checked_multiply(geometry_.curves.size(),
                                         kCurveLogicalBytes + kGroupLogicalBytes, valid_memory),
                        valid_memory);
        if (!valid_memory || initial_memory > limits_.working_memory_bytes)
            return fail(AnalyticFilteredOverlayError::resource_limit_exceeded);
        result_.telemetry.peak_working_memory_bytes = initial_memory;
        result_.telemetry.input_curves = geometry_.curves.size();
        result_.telemetry.input_pair_results = narrow_.intersections.size();
        if (!charge(geometry_.curves.size() + narrow_.intersections.size()))
            return false;
        for (std::size_t index = 0; index < geometry_.curves.size(); ++index)
        {
            const AnalyticAtomicCurveNm& curve = geometry_.curves[index];
            const AnalyticFilteredOccurrence& occurrence = geometry_.occurrences[index];
            if (curve.curve_index != index + 1 || curve.construction_carrier_id == 0 ||
                occurrence.occurrence_id != index + 1 || !valid_point(curve.start) ||
                !valid_point(curve.end) || !analytic_filtered_curve_is_valid(curve))
                return fail(AnalyticFilteredOverlayError::invalid_argument);
            if (curve.kind == AnalyticAtomicCurveKind::circular_arc &&
                occurrence.agrees_with_carrier != curve.counterclockwise)
                return fail(AnalyticFilteredOverlayError::invalid_argument);
        }
        AnalyticCurvePair previous{};
        std::uint64_t point_count = 0;
        for (std::size_t index = 0; index < narrow_.intersections.size(); ++index)
        {
            const AnalyticPairIntersection& value = narrow_.intersections[index];
            if (value.pair.first == 0 || value.pair.first >= value.pair.second ||
                value.pair.second > geometry_.curves.size() ||
                (index != 0 && std::tie(value.pair.first, value.pair.second) <=
                                   std::tie(previous.first, previous.second)))
                return fail(AnalyticFilteredOverlayError::invalid_argument);
            previous = value.pair;
            std::uint8_t expected_points = 0;
            switch (value.relation)
            {
            case AnalyticPairRelation::disjoint:
            case AnalyticPairRelation::coincident:
                break;
            case AnalyticPairRelation::point:
                expected_points = 1;
                break;
            case AnalyticPairRelation::two_points:
                expected_points = 2;
                break;
            default:
                return fail(AnalyticFilteredOverlayError::invalid_argument);
            }
            if (value.point_count != expected_points)
                return fail(AnalyticFilteredOverlayError::invalid_argument);
            const std::uint64_t left_carrier =
                geometry_.curves[value.pair.first - 1].construction_carrier_id;
            const std::uint64_t right_carrier =
                geometry_.curves[value.pair.second - 1].construction_carrier_id;
            if ((value.relation == AnalyticPairRelation::coincident) !=
                (left_carrier == right_carrier))
                return fail(AnalyticFilteredOverlayError::invalid_argument);
            for (std::uint8_t point_index = 0; point_index < value.point_count; ++point_index)
            {
                if (!charge(2))
                    return false;
                if (!valid_point(value.points[point_index]))
                    return fail(AnalyticFilteredOverlayError::invalid_argument);
                const AnalyticFilteredPointCurveStatus left_status =
                    classify_analytic_filtered_point_on_curve(
                        geometry_.curves[value.pair.first - 1], value.points[point_index]);
                const AnalyticFilteredPointCurveStatus right_status =
                    classify_analytic_filtered_point_on_curve(
                        geometry_.curves[value.pair.second - 1], value.points[point_index]);
                if (left_status == AnalyticFilteredPointCurveStatus::uncertain ||
                    right_status == AnalyticFilteredPointCurveStatus::uncertain)
                    return fail(AnalyticFilteredOverlayError::resource_limit_exceeded);
                if (left_status != AnalyticFilteredPointCurveStatus::certified_on_domain ||
                    right_status != AnalyticFilteredPointCurveStatus::certified_on_domain)
                    return fail(AnalyticFilteredOverlayError::invalid_argument);
            }
            point_count += value.point_count;
            if (point_count > limits_.intersections)
                return fail(AnalyticFilteredOverlayError::resource_limit_exceeded);
        }
        result_.telemetry.input_point_intersections = point_count;
        return true;
    }

    bool build_groups()
    {
        try
        {
            curve_order_.resize(geometry_.curves.size());
            for (std::uint32_t index = 0; index < curve_order_.size(); ++index)
                curve_order_[index] = index;
            if (!charge_sort(curve_order_.size()))
                return false;
            std::sort(curve_order_.begin(), curve_order_.end(),
                      [&](std::uint32_t left, std::uint32_t right)
                      {
                          return std::tie(geometry_.curves[left].construction_carrier_id, left) <
                                 std::tie(geometry_.curves[right].construction_carrier_id, right);
                      });
            groups_.reserve(curve_order_.size());
            for (std::uint32_t begin = 0; begin < curve_order_.size();)
            {
                const AnalyticAtomicCurveNm& representative = geometry_.curves[curve_order_[begin]];
                std::uint32_t end = begin + 1;
                while (end < curve_order_.size() &&
                       geometry_.curves[curve_order_[end]].construction_carrier_id ==
                           representative.construction_carrier_id)
                    ++end;
                for (std::uint32_t index = begin + 1; index < end; ++index)
                    if (geometry_.curves[curve_order_[index]].kind != representative.kind)
                        return fail(AnalyticFilteredOverlayError::invalid_argument);
                groups_.push_back({representative.construction_carrier_id, begin, end - begin, 0, 0,
                                   0, 0, 0, representative.curve_index, representative.kind});
                begin = end;
            }
        }
        catch (const std::bad_alloc&)
        {
            return fail(AnalyticFilteredOverlayError::resource_limit_exceeded);
        }
        result_.telemetry.carrier_groups = groups_.size();
        curve_local_.resize(geometry_.curves.size());
        for (const CarrierGroup& group : groups_)
            for (std::uint32_t local = 0; local < group.curve_count; ++local)
                curve_local_[curve_order_[group.curve_begin + local]] = local;
        return true;
    }

    std::pair<double, double> event_key(const AnalyticAtomicCurveNm& curve,
                                        const AnalyticFilteredOccurrence& occurrence,
                                        const AnalyticFilteredPointNm& event) const noexcept
    {
        if (curve.kind == AnalyticAtomicCurveKind::circular_arc)
        {
            const double radial_x = midpoint(event.x) - midpoint(curve.circle.center.x);
            const double radial_y = midpoint(event.y) - midpoint(curve.circle.center.y);
            return {approximate_circle_key(radial_x, radial_y), 0.0};
        }
        const AnalyticFilteredPointNm& canonical_start =
            occurrence.agrees_with_carrier ? curve.start : curve.end;
        const AnalyticFilteredPointNm& canonical_end =
            occurrence.agrees_with_carrier ? curve.end : curve.start;
        const double delta_x = midpoint(canonical_end.x) - midpoint(canonical_start.x);
        const double delta_y = midpoint(canonical_end.y) - midpoint(canonical_start.y);
        const bool use_x = std::fabs(delta_x) >= std::fabs(delta_y);
        const double direction = use_x ? delta_x : delta_y;
        const double coordinate = use_x ? midpoint(event.x) : midpoint(event.y);
        return {direction >= 0.0 ? coordinate : -coordinate, 0.0};
    }

    bool append_event(std::uint32_t curve_offset, const AnalyticFilteredPointNm& value,
                      EventRole role)
    {
        const AnalyticAtomicCurveNm& curve = geometry_.curves[curve_offset];
        const AnalyticFilteredOccurrence& occurrence = geometry_.occurrences[curve_offset];
        const auto [key_x, key_y] = event_key(curve, occurrence, value);
        events_.push_back({value, curve.construction_carrier_id, curve.curve_index, 0, 0, role,
                           curve.kind, key_x, key_y});
        return true;
    }

    AnalyticFilteredPointNm circle_left_seam(const AnalyticAtomicCurveNm& curve) const noexcept
    {
        const Interval center_x{curve.circle.center.x.lower, curve.circle.center.x.upper};
        const Interval radius{curve.circle.radius.lower, curve.circle.radius.upper};
        return {{subtract(center_x, radius).lower, subtract(center_x, radius).upper},
                curve.circle.center.y};
    }

    AnalyticFilteredPointNm circle_right_seam(const AnalyticAtomicCurveNm& curve) const noexcept
    {
        const Interval center_x{curve.circle.center.x.lower, curve.circle.center.x.upper};
        const Interval radius{curve.circle.radius.lower, curve.circle.radius.upper};
        return {{add(center_x, radius).lower, add(center_x, radius).upper}, curve.circle.center.y};
    }

    bool build_events()
    {
        bool valid = true;
        std::uint64_t raw_count = checked_multiply(geometry_.curves.size(), 2, valid);
        raw_count = checked_add(
            raw_count, checked_multiply(result_.telemetry.input_point_intersections, 2, valid),
            valid);
        std::uint64_t circle_groups = 0;
        for (const CarrierGroup& group : groups_)
            circle_groups += group.kind == AnalyticAtomicCurveKind::circular_arc ? 1 : 0;
        raw_count = checked_add(raw_count, circle_groups, valid);
        if (!valid || raw_count > std::numeric_limits<std::uint32_t>::max() ||
            !set_base_memory(raw_count))
            return false;
        try
        {
            events_.reserve(static_cast<std::size_t>(raw_count));
            unique_events_.reserve(static_cast<std::size_t>(raw_count));
            start_rank_.assign(geometry_.curves.size(), kNoIndex);
            end_rank_.assign(geometry_.curves.size(), kNoIndex);
            domain_modes_.assign(geometry_.curves.size(), DomainMode::normal);
            for (std::uint32_t curve_offset = 0; curve_offset < geometry_.curves.size();
                 ++curve_offset)
            {
                const AnalyticAtomicCurveNm& curve = geometry_.curves[curve_offset];
                const AnalyticFilteredOccurrence& occurrence = geometry_.occurrences[curve_offset];
                const bool forward = curve.kind == AnalyticAtomicCurveKind::line
                                         ? occurrence.agrees_with_carrier
                                         : curve.counterclockwise;
                append_event(curve_offset, forward ? curve.start : curve.end,
                             EventRole::domain_start);
                append_event(curve_offset, forward ? curve.end : curve.start,
                             EventRole::domain_end);
            }
            for (const CarrierGroup& group : groups_)
                if (group.kind == AnalyticAtomicCurveKind::circular_arc)
                {
                    const std::uint32_t curve_offset = group.representative_curve - 1;
                    append_event(curve_offset, circle_left_seam(geometry_.curves[curve_offset]),
                                 EventRole::circle_seam);
                }
            for (const AnalyticPairIntersection& intersection : narrow_.intersections)
                for (std::uint8_t point_index = 0; point_index < intersection.point_count;
                     ++point_index)
                {
                    append_event(intersection.pair.first - 1, intersection.points[point_index],
                                 EventRole::split);
                    append_event(intersection.pair.second - 1, intersection.points[point_index],
                                 EventRole::split);
                }
        }
        catch (const std::bad_alloc&)
        {
            return fail(AnalyticFilteredOverlayError::resource_limit_exceeded);
        }
        if (events_.size() != raw_count || !charge_sort(events_.size()))
            return false;
        std::sort(events_.begin(), events_.end(), raw_event_less);
        result_.telemetry.raw_events = events_.size();
        return true;
    }

    int certified_half(const AnalyticFilteredPointNm& value,
                       const AnalyticAtomicCurveNm& circle) const noexcept
    {
        const Interval x =
            subtract(Interval{value.x.lower, value.x.upper},
                     Interval{circle.circle.center.x.lower, circle.circle.center.x.upper});
        const Interval y =
            subtract(Interval{value.y.lower, value.y.upper},
                     Interval{circle.circle.center.y.lower, circle.circle.center.y.upper});
        if (y.lower > 0.0 || (y.lower == 0.0 && x.lower >= 0.0))
            return 0;
        if (y.upper < 0.0 || (y.upper == 0.0 && x.upper < 0.0))
            return 1;
        return -1;
    }

    bool certified_order(const AnalyticFilteredPointNm& left, const AnalyticFilteredPointNm& right,
                         const CarrierGroup& group)
    {
        if (!charge(1))
            return false;
        const AnalyticAtomicCurveNm& curve = geometry_.curves[group.representative_curve - 1];
        if (group.kind == AnalyticAtomicCurveKind::line)
        {
            const AnalyticFilteredOccurrence& occurrence =
                geometry_.occurrences[group.representative_curve - 1];
            const AnalyticFilteredPointNm& canonical_start =
                occurrence.agrees_with_carrier ? curve.start : curve.end;
            const AnalyticFilteredPointNm& canonical_end =
                occurrence.agrees_with_carrier ? curve.end : curve.start;
            const double delta_x = midpoint(canonical_end.x) - midpoint(canonical_start.x);
            const double delta_y = midpoint(canonical_end.y) - midpoint(canonical_start.y);
            const bool use_x = std::fabs(delta_x) >= std::fabs(delta_y);
            const bool ascending = (use_x ? delta_x : delta_y) >= 0.0;
            const auto& left_value = use_x ? left.x : left.y;
            const auto& right_value = use_x ? right.x : right.y;
            return ascending ? left_value.upper < right_value.lower
                             : right_value.upper < left_value.lower;
        }
        const int left_half = certified_half(left, curve);
        const int right_half = certified_half(right, curve);
        if (left_half < 0 || right_half < 0)
            return fail(AnalyticFilteredOverlayError::resource_limit_exceeded);
        if (left_half != right_half)
            return left_half < right_half;
        const Point center = point(curve.circle.center);
        const Interval determinant =
            cross(subtract(point(left), center), subtract(point(right), center));
        return determinant.lower > 0.0;
    }

    bool deduplicate_events()
    {
        std::size_t event_cursor = 0;
        for (CarrierGroup& group : groups_)
        {
            group.event_begin = static_cast<std::uint32_t>(event_cursor);
            while (event_cursor < events_.size() &&
                   events_[event_cursor].carrier_id == group.carrier_id)
                ++event_cursor;
            group.event_count = static_cast<std::uint32_t>(event_cursor - group.event_begin);
            group.point_begin = static_cast<std::uint32_t>(unique_events_.size());
            for (std::uint32_t local = 0; local < group.event_count; ++local)
            {
                RawEvent& event = events_[group.event_begin + local];
                if (!charge(1))
                    return false;
                bool merge = false;
                if (unique_events_.size() > group.point_begin)
                    merge = points_within_resolution(unique_events_.back().point, event.point);
                if (merge)
                {
                    AnalyticFilteredPointNm representative =
                        point_hull(unique_events_.back().point, event.point);
                    if (group.kind == AnalyticAtomicCurveKind::circular_arc &&
                        (unique_events_.back().has_circle_seam ||
                         event.role == EventRole::circle_seam))
                        representative =
                            circle_left_seam(geometry_.curves[group.representative_curve - 1]);
                    if (!valid_point(representative))
                        return fail(AnalyticFilteredOverlayError::resource_limit_exceeded);
                    unique_events_.back().point = representative;
                    ++result_.telemetry.resolution_merges;
                }
                else
                {
                    if (unique_events_.size() > group.point_begin &&
                        !certified_order(unique_events_.back().point, event.point, group))
                    {
                        if (result_.error == AnalyticFilteredOverlayError::none)
                            return fail(AnalyticFilteredOverlayError::resource_limit_exceeded);
                        return false;
                    }
                    unique_events_.push_back({event.point, false, false, false});
                }
                UniqueEvent& unique = unique_events_.back();
                unique.has_intersection = unique.has_intersection || event.role == EventRole::split;
                unique.has_endpoint = unique.has_endpoint ||
                                      event.role == EventRole::domain_start ||
                                      event.role == EventRole::domain_end;
                unique.has_circle_seam =
                    unique.has_circle_seam || event.role == EventRole::circle_seam;
                event.point_index = static_cast<std::uint32_t>(unique_events_.size() - 1);
            }
            group.point_count =
                static_cast<std::uint32_t>(unique_events_.size() - group.point_begin);
            if (group.kind == AnalyticAtomicCurveKind::circular_arc && group.point_count > 1)
            {
                if (!charge(1))
                    return false;
                UniqueEvent& first = unique_events_[group.point_begin];
                const std::uint32_t last_index = group.point_begin + group.point_count - 1;
                const UniqueEvent& last = unique_events_[last_index];
                if (points_within_resolution(first.point, last.point))
                {
                    AnalyticFilteredPointNm representative = point_hull(first.point, last.point);
                    const AnalyticFilteredPointNm right_seam =
                        circle_right_seam(geometry_.curves[group.representative_curve - 1]);
                    if (points_within_resolution(first.point, right_seam) &&
                        points_within_resolution(last.point, right_seam))
                        representative = right_seam;
                    if (!valid_point(representative))
                        return fail(AnalyticFilteredOverlayError::resource_limit_exceeded);
                    first.point = representative;
                    first.has_intersection = first.has_intersection || last.has_intersection;
                    first.has_endpoint = first.has_endpoint || last.has_endpoint;
                    first.has_circle_seam = first.has_circle_seam || last.has_circle_seam;
                    for (std::uint32_t local = 0; local < group.event_count; ++local)
                    {
                        RawEvent& event = events_[group.event_begin + local];
                        if (event.point_index == last_index)
                            event.point_index = group.point_begin;
                    }
                    unique_events_.pop_back();
                    --group.point_count;
                    ++result_.telemetry.resolution_merges;
                }
            }
            if (group.point_count < 2)
                return fail(AnalyticFilteredOverlayError::resource_limit_exceeded);
            group.seam_local = 0;
            if (group.kind == AnalyticAtomicCurveKind::circular_arc)
            {
                bool found_seam = false;
                for (std::uint32_t local = 0; local < group.point_count; ++local)
                    if (unique_events_[group.point_begin + local].has_circle_seam)
                    {
                        group.seam_local = local;
                        found_seam = true;
                        break;
                    }
                if (!found_seam)
                    return fail(AnalyticFilteredOverlayError::invalid_argument);
            }
            for (std::uint32_t local = 0; local < group.event_count; ++local)
            {
                RawEvent& event = events_[group.event_begin + local];
                const std::uint32_t point_local = event.point_index - group.point_begin;
                event.rank =
                    group.kind == AnalyticAtomicCurveKind::circular_arc
                        ? (point_local + group.point_count - group.seam_local) % group.point_count
                        : point_local;
            }
        }
        if (event_cursor != events_.size())
            return fail(AnalyticFilteredOverlayError::invalid_argument);
        result_.telemetry.unique_events = unique_events_.size();
        return true;
    }

    bool build_actions()
    {
        try
        {
            actions_.reserve(geometry_.curves.size() * 2);
            for (const RawEvent& event : events_)
            {
                if (event.role != EventRole::domain_start && event.role != EventRole::domain_end)
                    continue;
                std::uint32_t& rank = event.role == EventRole::domain_start
                                          ? start_rank_[event.curve_index - 1]
                                          : end_rank_[event.curve_index - 1];
                if (rank != kNoIndex)
                    return fail(AnalyticFilteredOverlayError::invalid_argument);
                rank = event.rank;
            }
            for (std::uint32_t curve_offset = 0; curve_offset < geometry_.curves.size();
                 ++curve_offset)
            {
                if (start_rank_[curve_offset] == kNoIndex || end_rank_[curve_offset] == kNoIndex)
                    return fail(AnalyticFilteredOverlayError::invalid_argument);
                if (start_rank_[curve_offset] == end_rank_[curve_offset])
                {
                    const AnalyticAtomicCurveNm& curve = geometry_.curves[curve_offset];
                    domain_modes_[curve_offset] =
                        curve.kind == AnalyticAtomicCurveKind::circular_arc && curve.major_arc
                            ? DomainMode::full_circle
                            : DomainMode::collapsed;
                    ++result_.telemetry.collapsed_domains;
                    continue;
                }
                const std::uint64_t carrier_id =
                    geometry_.curves[curve_offset].construction_carrier_id;
                actions_.push_back({carrier_id, end_rank_[curve_offset], curve_offset + 1,
                                    curve_local_[curve_offset], EventRole::domain_end});
                actions_.push_back({carrier_id, start_rank_[curve_offset], curve_offset + 1,
                                    curve_local_[curve_offset], EventRole::domain_start});
            }
            if (!charge_sort(actions_.size()))
                return false;
            std::sort(actions_.begin(), actions_.end(),
                      [](const EndpointAction& left, const EndpointAction& right)
                      {
                          return std::tie(left.carrier_id, left.rank, left.role, left.curve_index) <
                                 std::tie(right.carrier_id, right.rank, right.role,
                                          right.curve_index);
                      });
        }
        catch (const std::bad_alloc&)
        {
            return fail(AnalyticFilteredOverlayError::resource_limit_exceeded);
        }
        return true;
    }

    std::uint64_t active_update_charge(std::uint32_t group_size) const noexcept
    {
        std::uint64_t levels = 1;
        for (std::uint32_t width = 1; width < group_size; width <<= 1)
            ++levels;
        return levels * 3 + 3;
    }

    bool update_active(ActiveCurves& active, std::uint32_t local, bool insert,
                       std::uint32_t group_size)
    {
        if (!charge(active_update_charge(group_size)))
            return false;
        const bool changed = insert ? active.insert(local) : active.erase(local);
        if (!changed)
            return fail(AnalyticFilteredOverlayError::invalid_argument);
        ++result_.telemetry.active_set_updates;
        return true;
    }

    const UniqueEvent& point_at_rank(const CarrierGroup& group, std::uint32_t rank) const noexcept
    {
        const std::uint32_t local = group.kind == AnalyticAtomicCurveKind::circular_arc
                                        ? (group.seam_local + rank) % group.point_count
                                        : rank;
        return unique_events_[group.point_begin + local];
    }

    bool span_major_arc(const CarrierGroup& group, const ActiveCurves& active,
                        const AnalyticFilteredPointNm& start, const AnalyticFilteredPointNm& end,
                        bool& major)
    {
        major = false;
        if (group.kind == AnalyticAtomicCurveKind::line)
            return true;
        if (!charge(1))
            return false;
        const AnalyticAtomicCurveNm& curve = geometry_.curves[group.representative_curve - 1];
        const Point center = point(curve.circle.center);
        const Point start_radial = subtract(point(start), center);
        const Point end_radial = subtract(point(end), center);
        const Interval determinant = cross(start_radial, end_radial);
        if (determinant.lower > 0.0)
            return true;
        if (determinant.upper < 0.0)
        {
            major = true;
            return true;
        }
        const Interval product = dot(start_radial, end_radial);
        if (determinant.lower == 0.0 && determinant.upper == 0.0 && product.upper < 0.0)
            return true;
        if (product.upper < 0.0)
        {
            bool found_certificate = false;
            bool certified_major = false;
            for (std::uint32_t local = active.head(); local != kNoIndex; local = active.next(local))
            {
                if (!charge(1))
                    return false;
                const std::uint32_t curve_offset = curve_order_[group.curve_begin + local];
                const AnalyticAtomicCurveNm& member = geometry_.curves[curve_offset];
                const AnalyticFilteredPointNm& member_start =
                    member.counterclockwise ? member.start : member.end;
                const AnalyticFilteredPointNm& member_end =
                    member.counterclockwise ? member.end : member.start;
                if (!member.has_arc_sweep_certificate ||
                    !points_within_resolution(start, member_start) ||
                    !points_within_resolution(end, member_end))
                    continue;
                if (found_certificate && certified_major != member.major_arc)
                    return fail(AnalyticFilteredOverlayError::invalid_argument);
                found_certificate = true;
                certified_major = member.major_arc;
            }
            if (found_certificate)
            {
                major = certified_major;
                return true;
            }
        }
        return fail(AnalyticFilteredOverlayError::resource_limit_exceeded);
    }

    bool sweep_group(const CarrierGroup& group, std::size_t action_begin, std::size_t action_end,
                     ActiveCurves& active, bool emit, std::uint64_t& span_cursor,
                     std::uint64_t& membership_cursor)
    {
        active.reset(group.curve_count);
        for (std::uint32_t local = 0; local < group.curve_count; ++local)
        {
            const std::uint32_t curve_offset = curve_order_[group.curve_begin + local];
            const DomainMode mode = domain_modes_[curve_offset];
            const bool initially_active =
                mode == DomainMode::full_circle ||
                (mode == DomainMode::normal &&
                 geometry_.curves[curve_offset].kind == AnalyticAtomicCurveKind::circular_arc &&
                 start_rank_[curve_offset] > end_rank_[curve_offset]);
            if (initially_active && !update_active(active, local, true, group.curve_count))
                return false;
        }
        std::size_t action_cursor = action_begin;
        for (std::uint32_t rank = 0; rank < group.point_count; ++rank)
        {
            const bool incoming_active = active.count() != 0;
            while (action_cursor < action_end && actions_[action_cursor].rank == rank)
            {
                const EndpointAction& action = actions_[action_cursor++];
                if (action.local_curve >= group.curve_count ||
                    curve_order_[group.curve_begin + action.local_curve] != action.curve_index - 1)
                    return fail(AnalyticFilteredOverlayError::invalid_argument);
                if (!update_active(active, action.local_curve,
                                   action.role == EventRole::domain_start, group.curve_count))
                    return false;
            }
            const bool has_next =
                group.kind == AnalyticAtomicCurveKind::circular_arc || rank + 1 < group.point_count;
            const bool outgoing_active = has_next && active.count() != 0;
            if (!emit && (incoming_active || outgoing_active))
            {
                if (output_vertex_count_ == limits_.arrangement_vertices)
                    return fail(AnalyticFilteredOverlayError::resource_limit_exceeded);
                ++output_vertex_count_;
            }
            if (!has_next || active.count() == 0)
                continue;
            const std::uint32_t next_rank = (rank + 1) % group.point_count;
            const AnalyticFilteredPointNm& start = point_at_rank(group, rank).point;
            const AnalyticFilteredPointNm& end = point_at_rank(group, next_rank).point;
            bool major_arc = false;
            if (!span_major_arc(group, active, start, end, major_arc))
                return false;
            if (!charge(active.count()))
                return false;
            result_.telemetry.membership_visits += active.count();
            if (!emit)
            {
                const std::uint64_t membership_limit =
                    std::min(limits_.source_reference_memberships, limits_.provenance_references);
                if (span_cursor == limits_.arrangement_half_edges / 2 ||
                    active.count() > membership_limit - membership_cursor)
                    return fail(AnalyticFilteredOverlayError::resource_limit_exceeded);
                ++span_cursor;
                membership_cursor += active.count();
                continue;
            }
            AnalyticAtomicSpanNm& span = result_.spans[span_cursor];
            span = {static_cast<std::uint32_t>(span_cursor + 1),
                    group.representative_curve,
                    group.kind,
                    start,
                    end,
                    major_arc,
                    static_cast<std::uint32_t>(membership_cursor),
                    active.count()};
            for (std::uint32_t local = active.head(); local != kNoIndex; local = active.next(local))
            {
                const std::uint32_t curve_offset = curve_order_[group.curve_begin + local];
                const AnalyticFilteredOccurrence& occurrence = geometry_.occurrences[curve_offset];
                result_.memberships[membership_cursor++] = {
                    curve_offset + 1, occurrence.agrees_with_carrier,
                    occurrence.agrees_with_carrier ? occurrence.material_on_left
                                                   : !occurrence.material_on_left};
            }
            ++span_cursor;
        }
        return action_cursor == action_end;
    }

    bool run_sweeps(bool emit, std::uint64_t& span_cursor, std::uint64_t& membership_cursor)
    {
        std::uint32_t maximum_group_size = 0;
        for (const CarrierGroup& group : groups_)
            maximum_group_size = std::max(maximum_group_size, group.curve_count);
        ActiveCurves active(maximum_group_size);
        std::size_t action_cursor = 0;
        for (const CarrierGroup& group : groups_)
        {
            const std::size_t begin = action_cursor;
            while (action_cursor < actions_.size() &&
                   actions_[action_cursor].carrier_id == group.carrier_id)
                ++action_cursor;
            if (!sweep_group(group, begin, action_cursor, active, emit, span_cursor,
                             membership_cursor))
                return false;
        }
        return action_cursor == actions_.size();
    }

    bool count_output()
    {
        return run_sweeps(false, output_span_count_, output_membership_count_);
    }

    bool allocate_output()
    {
        bool valid = true;
        std::uint64_t bytes =
            checked_add(base_memory_bytes_,
                        checked_multiply(output_span_count_, kSpanLogicalBytes, valid), valid);
        bytes = checked_add(
            bytes, checked_multiply(output_membership_count_, kMembershipLogicalBytes, valid),
            valid);
        if (!valid || bytes > limits_.working_memory_bytes ||
            output_span_count_ > std::numeric_limits<std::uint32_t>::max() ||
            output_membership_count_ > std::numeric_limits<std::uint32_t>::max())
            return fail(AnalyticFilteredOverlayError::resource_limit_exceeded);
        result_.telemetry.peak_working_memory_bytes = bytes;
        try
        {
            result_.spans.resize(static_cast<std::size_t>(output_span_count_));
            result_.memberships.resize(static_cast<std::size_t>(output_membership_count_));
        }
        catch (const std::bad_alloc&)
        {
            return fail(AnalyticFilteredOverlayError::resource_limit_exceeded);
        }
        return true;
    }

    bool emit_output()
    {
        std::uint64_t span_cursor = 0;
        std::uint64_t membership_cursor = 0;
        return run_sweeps(true, span_cursor, membership_cursor) &&
               span_cursor == output_span_count_ && membership_cursor == output_membership_count_;
    }

    const AnalyticFilteredGeometry& geometry_;
    const AnalyticNarrowPhaseResult& narrow_;
    AnalyticSolverLimits limits_;
    AnalyticFilteredOverlayResult result_;
    std::vector<std::uint32_t> curve_order_;
    std::vector<CarrierGroup> groups_;
    std::vector<RawEvent> events_;
    std::vector<UniqueEvent> unique_events_;
    std::vector<EndpointAction> actions_;
    std::vector<std::uint32_t> start_rank_;
    std::vector<std::uint32_t> end_rank_;
    std::vector<std::uint32_t> curve_local_;
    std::vector<DomainMode> domain_modes_;
    std::uint64_t base_memory_bytes_ = 0;
    std::uint64_t output_span_count_ = 0;
    std::uint64_t output_membership_count_ = 0;
    std::uint64_t output_vertex_count_ = 0;
};

static_assert(sizeof(RawEvent) <= kRawEventLogicalBytes);
static_assert(sizeof(UniqueEvent) <= kUniqueEventLogicalBytes);
static_assert(sizeof(EndpointAction) <= kActionLogicalBytes);
static_assert(sizeof(AnalyticAtomicSpanNm) <= kSpanLogicalBytes);
static_assert(sizeof(AnalyticSpanMembership) <= kMembershipLogicalBytes);

} // namespace

AnalyticFilteredOverlayResult
build_analytic_filtered_overlay(const AnalyticFilteredGeometry& geometry,
                                const std::vector<AnalyticCurvePair>& candidate_pairs,
                                const AnalyticSolverLimits& limits)
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
        checked_multiply(curve_count, kCurveLogicalBytes + kGroupLogicalBytes, valid), valid);

    // Every valid pair consumes at least one narrow predicate, and overlay
    // input validation consumes one unit per curve and retained pair.
    std::uint64_t minimum_work = checked_multiply(pair_count, 2, valid);
    minimum_work = checked_add(minimum_work, curve_count, valid);
    if (!valid || minimum_memory > limits.working_memory_bytes ||
        minimum_work > limits.predicate_calls)
    {
        preflight.error = AnalyticFilteredOverlayError::resource_limit_exceeded;
        return preflight;
    }

    const AnalyticNarrowPhaseResult narrow_phase =
        intersect_analytic_curve_candidates(geometry.curves, candidate_pairs, limits);
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
        result.telemetry.predicate_calls = narrow_phase.telemetry.predicate_calls;
        result.telemetry.peak_working_memory_bytes =
            narrow_phase.telemetry.peak_working_memory_bytes;
        result.telemetry.algebraic_fallback_calls = narrow_phase.telemetry.algebraic_fallback_calls;
        return result;
    }
    return OverlayBuilder(geometry, narrow_phase, limits).build();
}

} // namespace geometer
