#include "analytic_filtered_normalization_replay.h"

#include "analytic_filtered_interval.h"
#include "analytic_wide_integer.h"

#include "geometer/analytic_curve_broad_phase.h"
#include "geometer/analytic_curve_narrow_phase.h"

#include <algorithm>
#include <cstdint>
#include <cstring>
#include <limits>
#include <tuple>
#include <utility>
#include <vector>

namespace geometer::analytic_normalization_detail
{
namespace
{
using analytic_detail::add;
using analytic_detail::complete_distance_squared;
using analytic_detail::cross;
using analytic_detail::exact;
using analytic_detail::Interval;
using analytic_detail::negate;
using analytic_detail::Point;
using analytic_detail::subtract;

constexpr std::uint64_t kIndexLogicalBytes = 8;
constexpr std::uint64_t kReplayGeometryLogicalBytesPerCurve = 512;
constexpr std::uint64_t kPersistentReplayGeometryLogicalBytesPerCurve =
    kAnalyticAtomicCurveLogicalBytes + 48 + 56;
constexpr std::uint64_t kReplayRingScratchLogicalBytes = 16;
constexpr std::uint64_t kReplayRegionScratchLogicalBytes = 16;
constexpr std::uint64_t kReplayFixedLogicalBytes = 512;
constexpr std::uint64_t kIntersectionValidationWork = 16;
constexpr std::uint32_t kNoIndex = std::numeric_limits<std::uint32_t>::max();

struct ArcCarrierEntry
{
    bool has_source_identity = false;
    std::uint64_t source_carrier = 0;
    std::uint64_t source_center_x_lower = 0;
    std::uint64_t source_center_x_upper = 0;
    std::uint64_t source_center_y_lower = 0;
    std::uint64_t source_center_y_upper = 0;
    std::uint64_t source_radius_lower = 0;
    std::uint64_t source_radius_upper = 0;
    bool exact_center = false;
    double center_x = 0.0;
    double center_y = 0.0;
    std::int64_t first_x = 0;
    std::int64_t first_y = 0;
    std::int64_t second_x = 0;
    std::int64_t second_y = 0;
    std::uint64_t radius = 0;
    bool center_on_positive_side = false;
    std::uint32_t curve = 0;

    auto key() const noexcept
    {
        return std::tie(has_source_identity, source_carrier, source_center_x_lower,
                        source_center_x_upper, source_center_y_lower, source_center_y_upper,
                        source_radius_lower, source_radius_upper, exact_center, center_x, center_y,
                        first_x, first_y, second_x, second_y, radius, center_on_positive_side);
    }
};

struct SourceCarrierEntry
{
    std::uint64_t source_carrier = 0;
    std::uint32_t curve = 0;

    auto key() const noexcept
    {
        return std::tie(source_carrier, curve);
    }
};

std::uint64_t double_bits(double value) noexcept
{
    std::uint64_t output = 0;
    std::memcpy(&output, &value, sizeof(output));
    return output;
}

bool valid_source_line_direction(const AnalyticAtomicCurveNm& curve) noexcept
{
    if (curve.kind != AnalyticAtomicCurveKind::line || !curve.has_integer_certificate ||
        (curve.integer_start.x == curve.integer_end.x &&
         curve.integer_start.y == curve.integer_end.y))
        return false;
    if (!curve.has_construction_line_direction)
        return true;
    if (curve.construction_line_dx == 0 && curve.construction_line_dy == 0)
        return false;
    const std::int64_t dx = curve.integer_end.x - curve.integer_start.x;
    const std::int64_t dy = curve.integer_end.y - curve.integer_start.y;
    const auto cross = analytic_detail::wide_subtract(
        analytic_detail::wide_multiply(dx, curve.construction_line_dy),
        analytic_detail::wide_multiply(dy, curve.construction_line_dx));
    return analytic_detail::wide_sign(cross) == 0;
}

struct EndpointColumnEntry
{
    std::int64_t x = 0;
    std::int64_t y = 0;
    std::uint32_t curve = 0;
    bool start = true;
    bool right = false;

    auto key() const noexcept
    {
        return std::tie(right, x, y, curve, start);
    }

    auto group_key() const noexcept
    {
        return std::tie(right, x, y);
    }
};

struct ReplayTangentEndpoint
{
    std::uint64_t token = 0;
    std::uint32_t curve = 0;
    bool start = false;

    auto key() const noexcept
    {
        return std::tie(token, curve, start);
    }
};

Point point(const AnalyticFilteredPointNm& value) noexcept
{
    return {{value.x.lower, value.x.upper}, {value.y.lower, value.y.upper}};
}

Point exact_point(std::int64_t x, std::int64_t y) noexcept
{
    return {exact(static_cast<double>(x)), exact(static_cast<double>(y))};
}

bool checked_add(std::uint64_t left, std::uint64_t right, std::uint64_t& output) noexcept
{
    if (right > std::numeric_limits<std::uint64_t>::max() - left)
        return false;
    output = left + right;
    return true;
}

bool checked_multiply(std::uint64_t left, std::uint64_t right, std::uint64_t& output) noexcept
{
    if (left != 0 && right > std::numeric_limits<std::uint64_t>::max() / left)
        return false;
    output = left * right;
    return true;
}

std::uint64_t sort_units(std::uint64_t count) noexcept
{
    if (count < 2)
        return 0;
    std::uint64_t levels = 0;
    for (std::uint64_t value = count - 1; value != 0; value >>= 1U)
        ++levels;
    std::uint64_t output = 0;
    return checked_multiply(count, levels, output) ? output
                                                   : std::numeric_limits<std::uint64_t>::max();
}

bool broad_fixed_work_upper_bound(std::uint64_t curves, std::uint64_t& output) noexcept
{
    std::uint64_t levels = 1;
    for (std::uint64_t value = curves > 1 ? curves - 1 : 0; value != 0; value >>= 1U)
        ++levels;
    std::uint64_t per_curve = 0;
    return checked_multiply(levels, 64, per_curve) && checked_add(per_curve, 64, per_curve) &&
           checked_multiply(curves, per_curve, output) && checked_add(output, 4096, output);
}

bool retained_pair_bytes(std::uint64_t pairs, std::uint64_t pair_ceiling,
                         std::uint64_t& output) noexcept
{
    if (pairs == 0)
    {
        output = 0;
        return true;
    }
    std::uint64_t capacity = std::min<std::uint64_t>(64, pair_ceiling);
    while (capacity < pairs)
    {
        const std::uint64_t doubled = capacity > std::numeric_limits<std::uint64_t>::max() / 2
                                          ? std::numeric_limits<std::uint64_t>::max()
                                          : capacity * 2;
        const std::uint64_t next = std::min(doubled, pair_ceiling);
        if (next <= capacity)
            return false;
        capacity = next;
    }
    return checked_multiply(capacity, kIndexLogicalBytes, output);
}

enum class Domain : std::uint8_t
{
    inside,
    outside,
    uncertain,
};

struct Arc
{
    Point center;
    Point start;
    Point end;
    bool counterclockwise = true;
    bool major_arc = false;
};

bool same_exact_point(Point left, Point right) noexcept
{
    return left.x.lower == left.x.upper && left.y.lower == left.y.upper &&
           right.x.lower == right.x.upper && right.y.lower == right.y.upper &&
           left.x.lower == right.x.lower && left.y.lower == right.y.lower;
}

Domain arc_domain(const Arc& arc, Point candidate) noexcept
{
    if (same_exact_point(candidate, arc.start) || same_exact_point(candidate, arc.end))
        return Domain::inside;
    const Point start = subtract(arc.start, arc.center);
    const Point end = subtract(arc.end, arc.center);
    const Point radial = subtract(candidate, arc.center);
    auto from_start = cross(start, radial);
    auto to_end = cross(radial, end);
    if (!arc.counterclockwise)
    {
        from_start = negate(from_start);
        to_end = negate(to_end);
    }
    if (!arc.major_arc)
    {
        if (from_start.lower >= 0.0 && to_end.lower >= 0.0)
            return Domain::inside;
        if (from_start.upper < 0.0 || to_end.upper < 0.0)
            return Domain::outside;
    }
    else
    {
        if (from_start.lower > 0.0 || to_end.lower > 0.0)
            return Domain::inside;
        if (from_start.upper <= 0.0 && to_end.upper <= 0.0)
            return Domain::outside;
    }
    return Domain::uncertain;
}

std::uint8_t shared_endpoint_count(const AnalyticAtomicCurveNm& left,
                                   const AnalyticAtomicCurveNm& right) noexcept
{
    std::uint8_t count = 0;
    for (const auto& a : {left.integer_start, left.integer_end})
        for (const auto& b : {right.integer_start, right.integer_end})
            if (a.x == b.x && a.y == b.y)
                ++count;
    return count;
}

bool contains_shared_endpoint(const AnalyticAtomicCurveNm& left, const AnalyticAtomicCurveNm& right,
                              const AnalyticFilteredPointNm& value) noexcept
{
    for (const auto& a : {left.integer_start, left.integer_end})
        for (const auto& b : {right.integer_start, right.integer_end})
            if (a.x == b.x && a.y == b.y && value.x.lower <= a.x && value.x.upper >= a.x &&
                value.y.lower <= a.y && value.y.upper >= a.y)
                return true;
    return false;
}

bool coincident_arc_domains_are_disjoint(const AnalyticAtomicCurveNm& left,
                                         const AnalyticAtomicCurveNm& right,
                                         std::uint8_t shared) noexcept
{
    if (left.kind != AnalyticAtomicCurveKind::circular_arc ||
        right.kind != AnalyticAtomicCurveKind::circular_arc || shared > 1)
        return false;
    const auto is_shared =
        [](const AnalyticIntegerPointNm& value, const AnalyticAtomicCurveNm& other)
    {
        return (value.x == other.integer_start.x && value.y == other.integer_start.y) ||
               (value.x == other.integer_end.x && value.y == other.integer_end.y);
    };
    const Arc left_arc{point(left.circle.center), point(left.start), point(left.end),
                       left.counterclockwise, left.major_arc};
    const Arc right_arc{point(right.circle.center), point(right.start), point(right.end),
                        right.counterclockwise, right.major_arc};
    for (const auto& endpoint : {left.integer_start, left.integer_end})
        if (!is_shared(endpoint, right) &&
            arc_domain(right_arc, exact_point(endpoint.x, endpoint.y)) != Domain::outside)
            return false;
    for (const auto& endpoint : {right.integer_start, right.integer_end})
        if (!is_shared(endpoint, left) &&
            arc_domain(left_arc, exact_point(endpoint.x, endpoint.y)) != Domain::outside)
            return false;
    return true;
}

bool complementary_arcs(const AnalyticAtomicCurveNm& left, const AnalyticAtomicCurveNm& right,
                        std::uint8_t shared) noexcept
{
    const bool reversed = left.integer_start.x == right.integer_end.x &&
                          left.integer_start.y == right.integer_end.y &&
                          left.integer_end.x == right.integer_start.x &&
                          left.integer_end.y == right.integer_start.y;
    const bool aligned = left.integer_start.x == right.integer_start.x &&
                         left.integer_start.y == right.integer_start.y &&
                         left.integer_end.x == right.integer_end.x &&
                         left.integer_end.y == right.integer_end.y;
    return left.kind == AnalyticAtomicCurveKind::circular_arc &&
           right.kind == AnalyticAtomicCurveKind::circular_arc && shared == 2 && !left.major_arc &&
           !right.major_arc &&
           ((reversed && left.counterclockwise == right.counterclockwise) ||
            (aligned && left.counterclockwise != right.counterclockwise));
}

bool valid_intersection(const AnalyticPairIntersection& intersection,
                        const std::vector<AnalyticAtomicCurveNm>& curves) noexcept
{
    if (intersection.pair.first == 0 || intersection.pair.second == 0 ||
        intersection.pair.first > curves.size() || intersection.pair.second > curves.size())
        return false;
    const auto& left = curves[intersection.pair.first - 1];
    const auto& right = curves[intersection.pair.second - 1];
    const std::uint8_t shared = shared_endpoint_count(left, right);
    if (intersection.resolution_collapsed)
        return false;
    if (intersection.relation == AnalyticPairRelation::disjoint)
        return shared == 0;
    if (intersection.relation == AnalyticPairRelation::coincident)
        return complementary_arcs(left, right, shared) ||
               coincident_arc_domains_are_disjoint(left, right, shared);
    if (intersection.point_count != shared)
        return false;
    for (std::uint8_t index = 0; index < intersection.point_count; ++index)
        if (!contains_shared_endpoint(left, right, intersection.points[index]))
            return false;
    return true;
}

class Validator
{
  public:
    Validator(std::int64_t origin_x_nm, std::int64_t origin_y_nm,
              const std::vector<AnalyticAtomicCurveNm>& curves,
              const std::vector<AnalyticCurveBoundsNm>& bounds,
              const AnalyticFilteredRegionsResult& original, const AnalyticSolverLimits& limits)
        : origin_x_nm_(origin_x_nm), origin_y_nm_(origin_y_nm), curves_(curves), bounds_(bounds),
          original_(original), limits_(limits)
    {
    }

    ReplayResult run()
    {
        std::uint64_t fixed_work = 0;
        if (!broad_fixed_work_upper_bound(bounds_.size(), fixed_work) || !charge(fixed_work))
            return result_;
        AnalyticSolverLimits broad_limits = limits_;
        const std::uint64_t dynamic_allowance = limits_.predicate_calls - result_.work_units;
        broad_limits.predicate_calls = dynamic_allowance / 2;
        broad_limits.examined_curve_pairs =
            std::min(broad_limits.examined_curve_pairs, dynamic_allowance / 34);
        AnalyticBroadPhaseResult broad = build_analytic_curve_candidates(bounds_, broad_limits);
        result_.candidate_pairs = broad.pairs.size();
        result_.peak_working_memory_bytes = broad.telemetry.peak_working_memory_bytes;
        std::uint64_t dynamic_work = 0;
        std::uint64_t pair_work = 0;
        if (broad.telemetry.candidate_pairs > broad.telemetry.examined_curve_pairs ||
            !checked_multiply(broad.telemetry.candidate_pairs, 16, pair_work) ||
            !checked_add(broad.telemetry.spatial_index_node_visits,
                         broad.telemetry.examined_curve_pairs, dynamic_work) ||
            !checked_add(dynamic_work, pair_work, dynamic_work) || !charge(dynamic_work) ||
            !retained_pair_bytes(broad.pairs.size(), limits_.examined_curve_pairs, pair_bytes_))
            return fail(ReplayError::resource_limit_exceeded);
        if (broad.error != AnalyticBroadPhaseError::none)
        {
            result_.required_working_memory_bytes = broad.telemetry.required_working_memory_bytes;
            return fail(ReplayError::resource_limit_exceeded);
        }
        if (!prepare_geometry())
            return result_;
        if (!validate_narrow(broad.pairs))
            return result_;
        return validate_regions(broad.pairs);
    }

  private:
    bool charge(std::uint64_t units)
    {
        if (result_.work_units > limits_.predicate_calls ||
            units > limits_.predicate_calls - result_.work_units)
        {
            result_.error = ReplayError::resource_limit_exceeded;
            return false;
        }
        result_.work_units += units;
        return true;
    }

    ReplayResult fail(ReplayError error)
    {
        result_.error = error;
        return result_;
    }

    bool validate_narrow(const std::vector<AnalyticCurvePair>& pairs)
    {
        AnalyticSolverLimits limits = limits_;
        limits.predicate_calls -= result_.work_units;
        if (persistent_geometry_bytes_ > limits.working_memory_bytes)
        {
            result_.required_working_memory_bytes = persistent_geometry_bytes_;
            result_.error = ReplayError::resource_limit_exceeded;
            return false;
        }
        limits.working_memory_bytes -= persistent_geometry_bytes_;
        AnalyticNarrowPhaseResult narrow =
            intersect_analytic_curve_candidates(geometry_.curves, pairs, limits);
        std::uint64_t narrow_peak = 0;
        if (!checked_add(persistent_geometry_bytes_, narrow.telemetry.peak_working_memory_bytes,
                         narrow_peak) ||
            narrow_peak > limits_.working_memory_bytes)
        {
            result_.required_working_memory_bytes = narrow_peak > limits_.working_memory_bytes
                                                        ? narrow_peak
                                                        : std::numeric_limits<std::uint64_t>::max();
            result_.error = ReplayError::resource_limit_exceeded;
            return false;
        }
        result_.peak_working_memory_bytes =
            std::max(result_.peak_working_memory_bytes, narrow_peak);
        if (!charge(narrow.telemetry.predicate_calls))
            return false;
        if (narrow.error != AnalyticNarrowPhaseError::none)
        {
            if (narrow.telemetry.required_working_memory_bytes > limits.working_memory_bytes)
            {
                if (!checked_add(persistent_geometry_bytes_,
                                 narrow.telemetry.required_working_memory_bytes,
                                 result_.required_working_memory_bytes))
                    result_.required_working_memory_bytes =
                        std::numeric_limits<std::uint64_t>::max();
            }
            result_.error = narrow.error == AnalyticNarrowPhaseError::invalid_argument
                                ? ReplayError::invalid_argument
                                : ReplayError::resource_limit_exceeded;
            return false;
        }
        std::uint64_t validation_work = 0;
        if (!checked_multiply(narrow.intersections.size(), kIntersectionValidationWork,
                              validation_work) ||
            !charge(validation_work))
            return false;
        for (const auto& intersection : narrow.intersections)
            if (!valid_intersection(intersection, geometry_.curves))
            {
                result_.error = ReplayError::topology_collapse;
                return false;
            }
        return true;
    }

    bool prepare_geometry()
    {
        std::uint64_t persistent_curve_bytes = 0;
        std::uint64_t curve_bytes = 0;
        std::uint64_t ring_bytes = 0;
        std::uint64_t region_bytes = 0;
        if (!checked_multiply(curves_.size(), kPersistentReplayGeometryLogicalBytesPerCurve,
                              persistent_curve_bytes) ||
            !checked_add(persistent_curve_bytes, pair_bytes_, persistent_geometry_bytes_) ||
            !checked_add(persistent_geometry_bytes_, kReplayFixedLogicalBytes,
                         persistent_geometry_bytes_) ||
            !checked_multiply(curves_.size(), kReplayGeometryLogicalBytesPerCurve, curve_bytes) ||
            !checked_multiply(original_.rings.size(), kReplayRingScratchLogicalBytes, ring_bytes) ||
            !checked_multiply(original_.regions.size(), kReplayRegionScratchLogicalBytes,
                              region_bytes) ||
            !checked_add(curve_bytes, ring_bytes, own_bytes_) ||
            !checked_add(own_bytes_, region_bytes, own_bytes_) ||
            !checked_add(own_bytes_, pair_bytes_, own_bytes_) ||
            !checked_add(own_bytes_, kReplayFixedLogicalBytes, own_bytes_) ||
            own_bytes_ > limits_.working_memory_bytes)
        {
            result_.required_working_memory_bytes = own_bytes_ > limits_.working_memory_bytes
                                                        ? own_bytes_
                                                        : std::numeric_limits<std::uint64_t>::max();
            result_.error = ReplayError::resource_limit_exceeded;
            return false;
        }
        std::uint64_t traversal_work = 0;
        std::uint64_t endpoint_count = 0;
        if (!checked_multiply(curves_.size(), 2, endpoint_count) ||
            !checked_multiply(curves_.size(), 10, traversal_work) ||
            !checked_add(traversal_work, sort_units(curves_.size()), traversal_work) ||
            !checked_add(traversal_work, sort_units(curves_.size()), traversal_work) ||
            !checked_add(traversal_work, sort_units(endpoint_count), traversal_work) ||
            !charge(traversal_work))
            return false;
        geometry_.origin_x_nm = origin_x_nm_;
        geometry_.origin_y_nm = origin_y_nm_;
        geometry_.curves = curves_;
        geometry_.bounds = bounds_;
        geometry_.occurrences.reserve(curves_.size());
        if (!validate_and_remint_source_carriers())
            return false;
        std::vector<ArcCarrierEntry> arc_carriers;
        arc_carriers.reserve(curves_.size());
        for (std::uint32_t index = 0; index < geometry_.curves.size(); ++index)
        {
            const auto& curve = geometry_.curves[index];
            if (curve.kind != AnalyticAtomicCurveKind::circular_arc)
                continue;
            const bool canonical_direction =
                std::tie(curve.integer_start.x, curve.integer_start.y) <
                std::tie(curve.integer_end.x, curve.integer_end.y);
            const auto& first = canonical_direction ? curve.integer_start : curve.integer_end;
            const auto& second = canonical_direction ? curve.integer_end : curve.integer_start;
            const std::uint64_t source_carrier = curves_[index].construction_carrier_id;
            const bool has_source_identity = source_carrier != 0;
            const bool exact_center = curve.circle.center.x.lower == curve.circle.center.x.upper &&
                                      curve.circle.center.y.lower == curve.circle.center.y.upper;
            arc_carriers.push_back(
                {has_source_identity, has_source_identity ? source_carrier : 0,
                 has_source_identity ? double_bits(curve.circle.center.x.lower) : 0,
                 has_source_identity ? double_bits(curve.circle.center.x.upper) : 0,
                 has_source_identity ? double_bits(curve.circle.center.y.lower) : 0,
                 has_source_identity ? double_bits(curve.circle.center.y.upper) : 0,
                 has_source_identity ? double_bits(curve.circle.radius.lower) : 0,
                 has_source_identity ? double_bits(curve.circle.radius.upper) : 0,
                 !has_source_identity && exact_center,
                 !has_source_identity && exact_center ? curve.circle.center.x.lower : 0.0,
                 !has_source_identity && exact_center ? curve.circle.center.y.lower : 0.0,
                 !has_source_identity && !exact_center ? first.x : 0,
                 !has_source_identity && !exact_center ? first.y : 0,
                 !has_source_identity && !exact_center ? second.x : 0,
                 !has_source_identity && !exact_center ? second.y : 0,
                 !has_source_identity ? curve.integer_radius : 0,
                 !has_source_identity && !exact_center &&
                     (curve.counterclockwise != curve.major_arc) == canonical_direction,
                 index});
        }
        std::sort(arc_carriers.begin(), arc_carriers.end(),
                  [](const ArcCarrierEntry& left, const ArcCarrierEntry& right)
                  { return left.key() < right.key(); });
        std::uint64_t group = 0;
        for (std::size_t index = 0; index < arc_carriers.size(); ++index)
        {
            const bool same_key =
                index != 0 && arc_carriers[index - 1].key() == arc_carriers[index].key();
            if (same_key && arc_carriers[index].has_source_identity &&
                !normalized_replay_arc_carrier_identity_matches(
                    curves_[arc_carriers[index - 1].curve], curves_[arc_carriers[index].curve]))
            {
                result_.error = ReplayError::invalid_argument;
                return false;
            }
            if (!same_key)
                ++group;
            auto& curve = geometry_.curves[arc_carriers[index].curve];
            curve.construction_carrier_id = curves_.size() + group;
            curve.construction_family_id = curve.construction_carrier_id;
        }
        std::vector<ArcCarrierEntry>().swap(arc_carriers);

        std::vector<EndpointColumnEntry> endpoint_columns;
        endpoint_columns.reserve(static_cast<std::size_t>(endpoint_count));
        for (std::uint32_t curve_index = 0; curve_index < geometry_.curves.size(); ++curve_index)
        {
            const auto& curve = geometry_.curves[curve_index];
            if (!curve.has_endpoint_authoritative_arc_certificate)
                continue;
            const Interval center_x{curve.circle.center.x.lower, curve.circle.center.x.upper};
            const Interval radius{curve.circle.radius.lower, curve.circle.radius.upper};
            const Interval left = subtract(center_x, radius);
            const Interval right = add(center_x, radius);
            const auto collect = [&](const AnalyticFilteredPointNm& endpoint,
                                     const AnalyticIntegerPointNm& integer, bool start)
            {
                const AnalyticFilteredPointNm left_seam = {{left.lower, left.upper},
                                                           curve.circle.center.y};
                const AnalyticFilteredPointNm right_seam = {{right.lower, right.upper},
                                                            curve.circle.center.y};
                constexpr double kResolutionSquared = static_cast<double>(
                    kAnalyticTopologyResolutionNm * kAnalyticTopologyResolutionNm);
                const bool near_left =
                    complete_distance_squared(point(left_seam), point(endpoint)).upper <=
                    kResolutionSquared;
                const bool near_right =
                    complete_distance_squared(point(right_seam), point(endpoint)).upper <=
                    kResolutionSquared;
                if (near_left == near_right)
                    return;
                const AnalyticFilteredPointNm& seam = near_right ? right_seam : left_seam;
                const bool same_seam =
                    endpoint.x.lower == seam.x.lower && endpoint.x.upper == seam.x.upper &&
                    endpoint.y.lower == seam.y.lower && endpoint.y.upper == seam.y.upper;
                if (!same_seam)
                    endpoint_columns.push_back(
                        {integer.x, integer.y, curve_index, start, near_right});
            };
            collect(curve.start, curve.integer_start, true);
            collect(curve.end, curve.integer_end, false);
        }
        std::sort(endpoint_columns.begin(), endpoint_columns.end(),
                  [](const EndpointColumnEntry& left, const EndpointColumnEntry& right)
                  { return left.key() < right.key(); });
        std::uint64_t endpoint_group = 0;
        for (std::size_t index = 0; index < endpoint_columns.size(); ++index)
        {
            if (index == 0 ||
                endpoint_columns[index - 1].group_key() != endpoint_columns[index].group_key())
                ++endpoint_group;
            auto& entry = endpoint_columns[index];
            auto& curve = geometry_.curves[entry.curve];
            (entry.start ? curve.start : curve.end).construction_x_column_id =
                analytic_endpoint_arc_partition_column_token(endpoint_group, entry.right);
        }
        for (std::uint32_t index = 0; index < geometry_.curves.size(); ++index)
            append_occurrence(index);
        return remint_tangent_tokens();
    }

    bool validate_and_remint_source_carriers()
    {
        std::vector<SourceCarrierEntry> entries;
        entries.reserve(geometry_.curves.size());
        for (std::uint32_t curve = 0; curve < geometry_.curves.size(); ++curve)
            if (geometry_.curves[curve].construction_carrier_id != 0)
                entries.push_back({geometry_.curves[curve].construction_carrier_id, curve});
        std::sort(entries.begin(), entries.end(),
                  [](const SourceCarrierEntry& left, const SourceCarrierEntry& right)
                  { return left.key() < right.key(); });

        std::uint64_t line_group = 0;
        for (std::size_t begin = 0; begin < entries.size();)
        {
            std::size_t end = begin + 1;
            while (end < entries.size() &&
                   entries[end].source_carrier == entries[begin].source_carrier)
                ++end;
            const auto& reference = geometry_.curves[entries[begin].curve];
            for (std::size_t index = begin + 1; index < end; ++index)
            {
                const auto& candidate = geometry_.curves[entries[index].curve];
                if (candidate.kind != reference.kind)
                {
                    result_.error = ReplayError::invalid_argument;
                    return false;
                }
            }
            if (reference.kind == AnalyticAtomicCurveKind::line)
            {
                for (std::size_t index = begin; index < end; ++index)
                {
                    auto& curve = geometry_.curves[entries[index].curve];
                    if (!valid_source_line_direction(curve))
                    {
                        result_.error = ReplayError::invalid_argument;
                        return false;
                    }
                    // Normalized sibling line fragments are not assumed to
                    // remain one exact carrier after independent endpoint
                    // rounding. They retain deterministic, separate replay
                    // identities unless a future exact normalized line key is
                    // transported explicitly.
                    curve.construction_carrier_id = ++line_group;
                    curve.construction_family_id = curve.construction_carrier_id;
                }
            }
            begin = end;
        }
        for (std::uint32_t index = 0; index < geometry_.curves.size(); ++index)
        {
            auto& curve = geometry_.curves[index];
            if (curve.kind != AnalyticAtomicCurveKind::line ||
                curves_[index].construction_carrier_id != 0)
                continue;
            if (!valid_source_line_direction(curve))
            {
                result_.error = ReplayError::invalid_argument;
                return false;
            }
            ++line_group;
            curve.construction_carrier_id = line_group;
            curve.construction_family_id = line_group;
        }
        return true;
    }

    bool remint_tangent_tokens()
    {
        std::vector<ReplayTangentEndpoint> endpoints;
        endpoints.reserve(geometry_.curves.size() * 2);
        for (std::uint32_t curve = 0; curve < geometry_.curves.size(); ++curve)
        {
            const auto& value = geometry_.curves[curve];
            if (value.construction_start_tangent_id != 0)
                endpoints.push_back({value.construction_start_tangent_id, curve, true});
            if (value.construction_end_tangent_id != 0)
                endpoints.push_back({value.construction_end_tangent_id, curve, false});
        }
        if (!charge(endpoints.size() + sort_units(endpoints.size())))
            return false;
        std::sort(endpoints.begin(), endpoints.end(),
                  [](const ReplayTangentEndpoint& left, const ReplayTangentEndpoint& right)
                  { return left.key() < right.key(); });
        for (std::size_t begin = 0; begin < endpoints.size();)
        {
            std::size_t end = begin + 1;
            while (end < endpoints.size() && endpoints[end].token == endpoints[begin].token)
                ++end;
            const auto clear = [&](const ReplayTangentEndpoint& endpoint)
            {
                auto& curve = geometry_.curves[endpoint.curve];
                (endpoint.start ? curve.construction_start_tangent_id
                                : curve.construction_end_tangent_id) = 0;
            };
            if (end - begin == 1)
            {
                clear(endpoints[begin]);
                begin = end;
                continue;
            }
            if (end - begin != 2)
            {
                result_.error = ReplayError::invalid_argument;
                return false;
            }
            ReplayTangentEndpoint& first_endpoint = endpoints[begin];
            ReplayTangentEndpoint& second_endpoint = endpoints[begin + 1U];
            auto& first_curve = geometry_.curves[first_endpoint.curve];
            auto& second_curve = geometry_.curves[second_endpoint.curve];
            const auto first_point =
                first_endpoint.start ? first_curve.integer_start : first_curve.integer_end;
            const auto second_point =
                second_endpoint.start ? second_curve.integer_start : second_curve.integer_end;
            if (first_point.x != second_point.x || first_point.y != second_point.y)
            {
                result_.error = ReplayError::invalid_argument;
                return false;
            }
            std::uint64_t reminted = 0;
            if (first_curve.kind != second_curve.kind)
            {
                auto& line_curve =
                    first_curve.kind == AnalyticAtomicCurveKind::line ? first_curve : second_curve;
                auto& arc_curve = first_curve.kind == AnalyticAtomicCurveKind::circular_arc
                                      ? first_curve
                                      : second_curve;
                const bool line_start = first_curve.kind == AnalyticAtomicCurveKind::line
                                            ? first_endpoint.start
                                            : second_endpoint.start;
                const bool arc_start = first_curve.kind == AnalyticAtomicCurveKind::circular_arc
                                           ? first_endpoint.start
                                           : second_endpoint.start;
                reminted =
                    analytic_endpoint_tangent_token(line_curve.construction_carrier_id, line_start,
                                                    arc_curve.construction_carrier_id, arc_start);
            }
            else if (first_curve.kind == AnalyticAtomicCurveKind::circular_arc)
                reminted = analytic_circle_endpoint_tangent_token(
                    first_curve.construction_carrier_id, first_endpoint.start,
                    second_curve.construction_carrier_id, second_endpoint.start,
                    analytic_circle_endpoint_tangent_identity(first_endpoint.token));
            else
            {
                result_.error = ReplayError::invalid_argument;
                return false;
            }
            if (reminted == 0)
            {
                result_.error = ReplayError::resource_limit_exceeded;
                return false;
            }
            (first_endpoint.start ? first_curve.construction_start_tangent_id
                                  : first_curve.construction_end_tangent_id) = reminted;
            (second_endpoint.start ? second_curve.construction_start_tangent_id
                                   : second_curve.construction_end_tangent_id) = reminted;
            begin = end;
        }
        return true;
    }

    void append_occurrence(std::uint32_t index)
    {
        auto& curve = geometry_.curves[index];
        bool agrees = true;
        if (curve.kind == AnalyticAtomicCurveKind::line)
        {
            const std::int64_t dx = curve.integer_end.x - curve.integer_start.x;
            const std::int64_t dy = curve.integer_end.y - curve.integer_start.y;
            agrees = dx > 0 || (dx == 0 && dy > 0);
            curve.has_construction_line_direction = true;
            curve.construction_line_dx = agrees ? dx : -dx;
            curve.construction_line_dy = agrees ? dy : -dy;
            if (dx == 0)
            {
                const std::uint64_t column =
                    analytic_vertical_x_column_token(curve.construction_carrier_id);
                curve.start.construction_x_column_id = column;
                curve.end.construction_x_column_id = column;
            }
        }
        else
        {
            agrees = curve.counterclockwise;
        }
        AnalyticFilteredOccurrence occurrence;
        occurrence.occurrence_id = index + 1;
        occurrence.coverage_id = 1;
        occurrence.agrees_with_carrier = agrees;
        occurrence.material_on_left = true;
        occurrence.source.kind = AnalyticFilteredSourceKind::authored_segment_curve;
        occurrence.source.role = curve.kind == AnalyticAtomicCurveKind::line
                                     ? AnalyticFilteredSourceRole::authored_line
                                     : AnalyticFilteredSourceRole::authored_circular_arc;
        occurrence.source.operand_id = 1;
        occurrence.source.primary_id = index + 1;
        occurrence.source.secondary_id = index + 1;
        geometry_.occurrences.push_back(occurrence);
    }

    ReplayResult validate_regions(const std::vector<AnalyticCurvePair>& pairs)
    {
        AnalyticRequestPacketRecords records;
        records.jobs.push_back({1, 0, 1});
        records.stages.push_back({1, 1, 0, 1});
        records.operands.push_back({1, 2, 0});
        AnalyticSolverLimits limits = limits_;
        limits.predicate_calls -= result_.work_units;
        if (own_bytes_ > limits.working_memory_bytes)
        {
            result_.required_working_memory_bytes = own_bytes_;
            return fail(ReplayError::resource_limit_exceeded);
        }
        limits.working_memory_bytes -= own_bytes_;
        AnalyticFilteredRegionsResult replay =
            build_analytic_filtered_regions(records, 0, geometry_, pairs, limits);
        std::uint64_t replay_peak = 0;
        if (!checked_add(own_bytes_, replay.telemetry.peak_working_memory_bytes, replay_peak) ||
            replay_peak > limits_.working_memory_bytes)
        {
            result_.required_working_memory_bytes = replay_peak > limits_.working_memory_bytes
                                                        ? replay_peak
                                                        : std::numeric_limits<std::uint64_t>::max();
            return fail(ReplayError::resource_limit_exceeded);
        }
        result_.peak_working_memory_bytes =
            std::max(result_.peak_working_memory_bytes, replay_peak);
        if (!charge(replay.telemetry.predicate_calls))
            return result_;
        if (replay.error != AnalyticFilteredRegionsError::none)
        {
            if (replay.telemetry.required_working_memory_bytes > limits.working_memory_bytes)
            {
                if (!checked_add(own_bytes_, replay.telemetry.required_working_memory_bytes,
                                 result_.required_working_memory_bytes))
                    result_.required_working_memory_bytes =
                        std::numeric_limits<std::uint64_t>::max();
            }
            return fail(replay.error == AnalyticFilteredRegionsError::resource_limit_exceeded
                            ? ReplayError::resource_limit_exceeded
                            : ReplayError::topology_collapse);
        }
        if (!charge(replay.selection.arrangement.collapsed_spans.size()))
            return result_;
        for (const auto& collapsed : replay.selection.arrangement.collapsed_spans)
        {
            if (collapsed.carrier_curve_index == 0 ||
                collapsed.carrier_curve_index > geometry_.curves.size() ||
                geometry_.curves[collapsed.carrier_curve_index - 1]
                    .has_endpoint_authoritative_arc_certificate)
                return fail(ReplayError::topology_collapse);
        }
        if (!validate_ring_mapping(replay) || !validate_region_mapping(replay))
            return fail(ReplayError::topology_collapse);
        return result_;
    }

    bool build_boundary_map(const AnalyticFilteredRegionsResult& replay,
                            std::vector<std::uint32_t>& boundary_ring,
                            std::vector<std::uint32_t>& boundary_visits,
                            std::vector<std::uint32_t>& ring_counts)
    {
        if (!charge(replay.ring_half_edges.size() +
                    replay.selection.arrangement.memberships.size()))
            return false;
        for (std::uint32_t ring = 0; ring < replay.rings.size(); ++ring)
            for (std::uint32_t offset = 0; offset < replay.rings[ring].half_edge_count; ++offset)
            {
                const std::uint32_t half_index =
                    replay.ring_half_edges[replay.rings[ring].half_edge_begin + offset];
                if (half_index >= replay.selection.arrangement.half_edges.size())
                    return false;
                const auto& half = replay.selection.arrangement.half_edges[half_index];
                if (half.edge >= replay.selection.arrangement.edges.size())
                    return false;
                const auto& edge = replay.selection.arrangement.edges[half.edge];
                if (edge.membership_begin > replay.selection.arrangement.memberships.size() ||
                    edge.membership_count == 0 ||
                    edge.membership_count >
                        replay.selection.arrangement.memberships.size() - edge.membership_begin)
                    return false;
                for (std::uint32_t at = 0; at < edge.membership_count; ++at)
                {
                    const std::uint32_t curve =
                        replay.selection.arrangement.memberships[edge.membership_begin + at]
                            .curve_index;
                    if (curve == 0 || curve > boundary_ring.size())
                        return false;
                    const std::uint32_t boundary = curve - 1;
                    if (boundary_ring[boundary] != kNoIndex && boundary_ring[boundary] != ring)
                        return false;
                    boundary_ring[boundary] = ring;
                    if (++boundary_visits[boundary] == 1)
                        ++ring_counts[ring];
                }
            }
        return true;
    }

    bool validate_ring_mapping(const AnalyticFilteredRegionsResult& replay)
    {
        if (replay.rings.size() != original_.rings.size() ||
            replay.regions.size() != original_.regions.size())
            return false;
        std::vector<std::uint32_t> boundary_ring(curves_.size(), kNoIndex);
        std::vector<std::uint32_t> boundary_visits(curves_.size());
        std::vector<std::uint32_t> ring_counts(replay.rings.size());
        if (!build_boundary_map(replay, boundary_ring, boundary_visits, ring_counts) ||
            !charge(original_.ring_half_edges.size() + original_.rings.size() * 2))
            return false;
        old_to_replay_.assign(original_.rings.size(), kNoIndex);
        for (std::uint32_t old = 0; old < original_.rings.size(); ++old)
            if (!match_ring(old, replay, boundary_ring, boundary_visits, ring_counts))
                return false;
        for (std::uint32_t old = 0; old < original_.rings.size(); ++old)
        {
            const std::uint32_t parent = original_.rings[old].parent_ring;
            if (parent != kNoAnalyticFilteredRing && parent >= old_to_replay_.size())
                return false;
            const std::uint32_t expected = parent == kNoAnalyticFilteredRing
                                               ? kNoAnalyticFilteredRing
                                               : old_to_replay_[parent];
            if (replay.rings[old_to_replay_[old]].parent_ring != expected)
                return false;
        }
        return true;
    }

    bool match_ring(std::uint32_t old, const AnalyticFilteredRegionsResult& replay,
                    const std::vector<std::uint32_t>& boundary_ring,
                    const std::vector<std::uint32_t>& boundary_visits,
                    const std::vector<std::uint32_t>& ring_counts)
    {
        const auto& source = original_.rings[old];
        std::uint32_t matched = kNoIndex;
        for (std::uint32_t offset = 0; offset < source.half_edge_count; ++offset)
        {
            const std::uint32_t boundary = source.half_edge_begin + offset;
            if (boundary >= boundary_ring.size() || boundary_visits[boundary] == 0 ||
                boundary_ring[boundary] == kNoIndex)
                return false;
            if (matched == kNoIndex)
                matched = boundary_ring[boundary];
            else if (matched != boundary_ring[boundary])
                return false;
        }
        if (matched == kNoIndex || ring_counts[matched] != source.half_edge_count)
            return false;
        const auto& target = replay.rings[matched];
        if (target.counterclockwise != source.counterclockwise || target.depth != source.depth)
            return false;
        old_to_replay_[old] = matched;
        return true;
    }

    bool validate_region_mapping(const AnalyticFilteredRegionsResult& replay)
    {
        if (!charge(sort_units(replay.regions.size()) + sort_units(original_.regions.size()) +
                    replay.regions.size() + original_.regions.size()))
            return false;
        std::vector<std::uint32_t> actual;
        actual.reserve(replay.regions.size());
        for (const auto& region : replay.regions)
            actual.push_back(region.outer_ring);
        std::sort(actual.begin(), actual.end());
        std::vector<std::uint32_t> expected;
        expected.reserve(original_.regions.size());
        for (const auto& region : original_.regions)
        {
            if (region.outer_ring >= old_to_replay_.size())
                return false;
            expected.push_back(old_to_replay_[region.outer_ring]);
        }
        std::sort(expected.begin(), expected.end());
        return actual == expected;
    }

    std::int64_t origin_x_nm_ = 0;
    std::int64_t origin_y_nm_ = 0;
    const std::vector<AnalyticAtomicCurveNm>& curves_;
    const std::vector<AnalyticCurveBoundsNm>& bounds_;
    const AnalyticFilteredRegionsResult& original_;
    const AnalyticSolverLimits& limits_;
    ReplayResult result_;
    AnalyticFilteredGeometry geometry_;
    std::vector<std::uint32_t> old_to_replay_;
    std::uint64_t pair_bytes_ = 0;
    std::uint64_t persistent_geometry_bytes_ = 0;
    std::uint64_t own_bytes_ = 0;
};
} // namespace

bool normalized_replay_arc_carrier_identity_matches(const AnalyticAtomicCurveNm& left,
                                                    const AnalyticAtomicCurveNm& right) noexcept
{
    return left.kind == AnalyticAtomicCurveKind::circular_arc && right.kind == left.kind &&
           left.construction_carrier_id != 0 &&
           left.construction_carrier_id == right.construction_carrier_id &&
           double_bits(left.circle.center.x.lower) == double_bits(right.circle.center.x.lower) &&
           double_bits(left.circle.center.x.upper) == double_bits(right.circle.center.x.upper) &&
           double_bits(left.circle.center.y.lower) == double_bits(right.circle.center.y.lower) &&
           double_bits(left.circle.center.y.upper) == double_bits(right.circle.center.y.upper) &&
           double_bits(left.circle.radius.lower) == double_bits(right.circle.radius.lower) &&
           double_bits(left.circle.radius.upper) == double_bits(right.circle.radius.upper);
}

ReplayResult validate_normalized_replay(std::int64_t origin_x_nm, std::int64_t origin_y_nm,
                                        const std::vector<AnalyticAtomicCurveNm>& curves,
                                        const std::vector<AnalyticCurveBoundsNm>& bounds,
                                        const AnalyticFilteredRegionsResult& original,
                                        const AnalyticSolverLimits& limits)
{
    return Validator(origin_x_nm, origin_y_nm, curves, bounds, original, limits).run();
}

static_assert(sizeof(AnalyticAtomicCurveNm) <= kAnalyticAtomicCurveLogicalBytes);
static_assert(sizeof(EndpointColumnEntry) <= 32);
static_assert(sizeof(ReplayTangentEndpoint) <= 16);
static_assert(sizeof(SourceCarrierEntry) <= 16);
static_assert(sizeof(AnalyticCurveBoundsNm) <= 48);
static_assert(sizeof(AnalyticFilteredOccurrence) <= 56);
static_assert(sizeof(ArcCarrierEntry) <= 144);

} // namespace geometer::analytic_normalization_detail
