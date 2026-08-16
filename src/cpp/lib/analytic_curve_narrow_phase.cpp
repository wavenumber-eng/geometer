#include "geometer/analytic_curve_narrow_phase.h"

#include "analytic_filtered_interval.h"
#include "analytic_wide_integer.h"

#include <algorithm>
#include <cmath>
#include <cstddef>
#include <limits>
#include <new>
#include <utility>

namespace geometer
{

namespace
{

constexpr std::int64_t kLocalCoordinateSpanNm = 1'000'000'000'000;
constexpr std::uint64_t kPairLogicalBytes = 256;

using namespace analytic_detail;
using SignedWide = WideInteger;

enum class DomainResult : std::uint8_t
{
    outside,
    inside,
    inside_resolution,
    uncertain,
};

struct PairWork
{
    AnalyticPairIntersection value;
    bool uncertain = false;
};

Point point(AnalyticIntegerPointNm value) noexcept
{
    return {{static_cast<double>(value.x), static_cast<double>(value.x)},
            {static_cast<double>(value.y), static_cast<double>(value.y)}};
}

Point point(AnalyticFilteredPointNm value) noexcept
{
    return {{value.x.lower, value.x.upper}, {value.y.lower, value.y.upper}};
}

AnalyticFilteredPointNm public_point(Point value) noexcept
{
    return {{value.x.lower, value.x.upper}, {value.y.lower, value.y.upper}};
}

bool valid_interval(Interval value) noexcept
{
    return valid(value);
}

std::int64_t difference(std::int64_t left, std::int64_t right) noexcept
{
    return left - right;
}

SignedWide exact_cross(AnalyticIntegerPointNm first, AnalyticIntegerPointNm second,
                       AnalyticIntegerPointNm third) noexcept
{
    const std::int64_t first_x = difference(second.x, first.x);
    const std::int64_t first_y = difference(second.y, first.y);
    const std::int64_t second_x = difference(third.x, first.x);
    const std::int64_t second_y = difference(third.y, first.y);
    return wide_subtract(wide_multiply(first_x, second_y), wide_multiply(first_y, second_x));
}

SignedWide exact_squared_distance(AnalyticIntegerPointNm left,
                                  AnalyticIntegerPointNm right) noexcept
{
    const std::int64_t dx = difference(left.x, right.x);
    const std::int64_t dy = difference(left.y, right.y);
    return wide_add(wide_multiply(dx, dx), wide_multiply(dy, dy));
}

SignedWide exact_dot_from(AnalyticIntegerPointNm origin, AnalyticIntegerPointNm left,
                          AnalyticIntegerPointNm right) noexcept
{
    const std::int64_t left_x = difference(left.x, origin.x);
    const std::int64_t left_y = difference(left.y, origin.y);
    const std::int64_t right_x = difference(right.x, origin.x);
    const std::int64_t right_y = difference(right.y, origin.y);
    return wide_add(wide_multiply(left_x, right_x), wide_multiply(left_y, right_y));
}

bool exact_integer_length(AnalyticIntegerPointNm direction, std::uint64_t& length,
                          AnalyticNarrowPhaseTelemetry& telemetry) noexcept
{
    const SignedWide squared =
        wide_add(wide_multiply(direction.x, direction.x), wide_multiply(direction.y, direction.y));
    ++telemetry.square_root_calls;
    const double approximate =
        std::sqrt(static_cast<double>(direction.x) * static_cast<double>(direction.x) +
                  static_cast<double>(direction.y) * static_cast<double>(direction.y));
    const std::uint64_t center = static_cast<std::uint64_t>(approximate);
    constexpr std::uint64_t search_radius = 4;
    const std::uint64_t first = center > search_radius ? center - search_radius : 0;
    for (std::uint64_t candidate = first; candidate <= center + search_radius; ++candidate)
    {
        if (candidate <= static_cast<std::uint64_t>(std::numeric_limits<std::int64_t>::max()) &&
            wide_compare(wide_multiply(static_cast<std::int64_t>(candidate),
                                       static_cast<std::int64_t>(candidate)),
                         squared) == 0)
        {
            length = candidate;
            return true;
        }
    }
    return false;
}

bool certified_line_circle_tangent(const AnalyticAtomicCurveNm& line,
                                   const AnalyticAtomicCurveNm& arc,
                                   AnalyticNarrowPhaseTelemetry& telemetry) noexcept
{
    if (!line.has_integer_certificate || !arc.has_integer_certificate ||
        !arc.has_integer_radius_certificate)
        return false;
    const AnalyticIntegerPointNm direction{difference(line.integer_end.x, line.integer_start.x),
                                           difference(line.integer_end.y, line.integer_start.y)};
    std::uint64_t length = 0;
    if (!exact_integer_length(direction, length, telemetry))
        return false;
    const SignedWide distance_numerator =
        wide_absolute(exact_cross(line.integer_start, line.integer_end, arc.integer_center));
    const SignedWide expected = wide_multiply(static_cast<std::int64_t>(arc.integer_radius),
                                              static_cast<std::int64_t>(length));
    return wide_compare(distance_numerator, expected) == 0;
}

enum class CertifiedCircleRelation : std::uint8_t
{
    unavailable,
    disjoint,
    tangent,
    intersecting,
    coincident,
};

CertifiedCircleRelation certified_circle_relation(const AnalyticAtomicCurveNm& left,
                                                  const AnalyticAtomicCurveNm& right) noexcept
{
    if (!left.has_integer_certificate || !right.has_integer_certificate ||
        !left.has_integer_radius_certificate || !right.has_integer_radius_certificate)
        return CertifiedCircleRelation::unavailable;
    const SignedWide distance_squared =
        exact_squared_distance(left.integer_center, right.integer_center);
    const std::uint64_t radius_sum = left.integer_radius + right.integer_radius;
    const std::uint64_t radius_difference = left.integer_radius > right.integer_radius
                                                ? left.integer_radius - right.integer_radius
                                                : right.integer_radius - left.integer_radius;
    const SignedWide sum_squared =
        wide_multiply(static_cast<std::int64_t>(radius_sum), static_cast<std::int64_t>(radius_sum));
    const SignedWide difference_squared = wide_multiply(
        static_cast<std::int64_t>(radius_difference), static_cast<std::int64_t>(radius_difference));
    if (wide_sign(distance_squared) == 0)
        return radius_difference == 0 ? CertifiedCircleRelation::coincident
                                      : CertifiedCircleRelation::disjoint;
    const int external = wide_compare(distance_squared, sum_squared);
    const int internal = wide_compare(distance_squared, difference_squared);
    if (external > 0 || internal < 0)
        return CertifiedCircleRelation::disjoint;
    if (external == 0 || internal == 0)
        return CertifiedCircleRelation::tangent;
    return CertifiedCircleRelation::intersecting;
}

bool same_point(AnalyticIntegerPointNm left, AnalyticIntegerPointNm right) noexcept
{
    return left.x == right.x && left.y == right.y;
}

bool coordinate_in_span(std::int64_t value) noexcept
{
    return value >= -kLocalCoordinateSpanNm && value <= kLocalCoordinateSpanNm;
}

bool coordinate_in_span(AnalyticCoordinateIntervalNm value) noexcept
{
    return std::isfinite(value.lower) && std::isfinite(value.upper) && value.lower <= value.upper &&
           value.lower >= -static_cast<double>(kLocalCoordinateSpanNm) &&
           value.upper <= static_cast<double>(kLocalCoordinateSpanNm);
}

bool interval_equals_integer(AnalyticCoordinateIntervalNm interval, std::int64_t value) noexcept
{
    const double converted = static_cast<double>(value);
    return interval.lower == converted && interval.upper == converted;
}

bool point_equals_certificate(AnalyticFilteredPointNm point_value,
                              AnalyticIntegerPointNm certificate) noexcept
{
    return interval_equals_integer(point_value.x, certificate.x) &&
           interval_equals_integer(point_value.y, certificate.y);
}

Interval absolute(Interval value) noexcept
{
    if (value.lower >= 0.0)
        return value;
    if (value.upper <= 0.0)
        return negate(value);
    return {0.0, std::max(-value.lower, value.upper)};
}

bool point_interval_fits_resolution(Point candidate) noexcept
{
    const double half_width_x = upward((candidate.x.upper - candidate.x.lower) * 0.5);
    const double half_width_y = upward((candidate.y.upper - candidate.y.lower) * 0.5);
    const Interval radius_squared = add(square({0.0, half_width_x}), square({0.0, half_width_y}));
    constexpr double resolution_squared =
        static_cast<double>(kAnalyticTopologyResolutionNm * kAnalyticTopologyResolutionNm);
    return radius_squared.upper <= resolution_squared;
}

bool scalar_interval_fits_resolution(Interval value) noexcept
{
    return upward((value.upper - value.lower) * 0.5) <=
           static_cast<double>(kAnalyticTopologyResolutionNm);
}

bool valid_common_curve(const AnalyticAtomicCurveNm& curve) noexcept
{
    if (curve.curve_index == 0 || !coordinate_in_span(curve.start.x) ||
        !coordinate_in_span(curve.start.y) || !coordinate_in_span(curve.end.x) ||
        !coordinate_in_span(curve.end.y) || !point_interval_fits_resolution(point(curve.start)) ||
        !point_interval_fits_resolution(point(curve.end)))
        return false;
    const Point start = point(curve.start);
    const Point end = point(curve.end);
    if (dot(subtract(end, start), subtract(end, start)).lower <= 0.0)
        return false;
    if (!curve.has_integer_certificate)
        return !curve.has_integer_radius_certificate;
    return coordinate_in_span(curve.integer_start.x) && coordinate_in_span(curve.integer_start.y) &&
           coordinate_in_span(curve.integer_end.x) && coordinate_in_span(curve.integer_end.y) &&
           !same_point(curve.integer_start, curve.integer_end) &&
           point_equals_certificate(curve.start, curve.integer_start) &&
           point_equals_certificate(curve.end, curve.integer_end);
}

bool valid_arc_certificate(const AnalyticAtomicCurveNm& curve, Interval start_radius,
                           Interval end_radius) noexcept
{
    if (!curve.has_integer_certificate)
        return !curve.has_integer_radius_certificate;
    if (!coordinate_in_span(curve.integer_center.x) ||
        !coordinate_in_span(curve.integer_center.y) ||
        !point_equals_certificate(curve.circle.center, curve.integer_center))
        return false;
    const Interval radius = {curve.circle.radius.lower, curve.circle.radius.upper};
    const SignedWide start_squared =
        exact_squared_distance(curve.integer_start, curve.integer_center);
    if (wide_compare(start_squared,
                     exact_squared_distance(curve.integer_end, curve.integer_center)) != 0)
        return false;
    if (curve.has_integer_radius_certificate)
    {
        if (curve.integer_radius == 0 ||
            curve.integer_radius > static_cast<std::uint64_t>(kLocalCoordinateSpanNm) ||
            wide_compare(start_squared,
                         wide_multiply(static_cast<std::int64_t>(curve.integer_radius),
                                       static_cast<std::int64_t>(curve.integer_radius))) != 0)
            return false;
        const double certified_radius = static_cast<double>(curve.integer_radius);
        if (radius.lower > certified_radius || radius.upper < certified_radius)
            return false;
    }
    else if (radius.lower > start_radius.lower || radius.upper < start_radius.upper ||
             radius.lower > end_radius.lower || radius.upper < end_radius.upper)
        return false;
    return true;
}

bool valid_arc_sweep(const AnalyticAtomicCurveNm& curve, Point start, Point end,
                     Point center) noexcept
{
    if (curve.has_arc_sweep_certificate)
        return true;
    if (curve.has_integer_certificate)
    {
        int orientation =
            wide_sign(exact_cross(curve.integer_center, curve.integer_start, curve.integer_end));
        if (!curve.counterclockwise)
            orientation = -orientation;
        if (orientation == 0)
            return !curve.major_arc &&
                   wide_sign(exact_dot_from(curve.integer_center, curve.integer_start,
                                            curve.integer_end)) < 0;
        return curve.major_arc ? orientation < 0 : orientation > 0;
    }
    Interval orientation = cross(subtract(start, center), subtract(end, center));
    if (!curve.counterclockwise)
        orientation = negate(orientation);
    if (curve.major_arc)
        return orientation.upper < 0.0;
    if (orientation.lower > 0.0)
        return true;
    const Interval endpoint_dot = dot(subtract(start, center), subtract(end, center));
    return singleton(orientation) && orientation.lower == 0.0 && endpoint_dot.upper < 0.0;
}

bool valid_curve(const AnalyticAtomicCurveNm& curve) noexcept
{
    if (!valid_common_curve(curve))
        return false;
    if (curve.kind == AnalyticAtomicCurveKind::line)
        return !curve.has_arc_sweep_certificate;
    if (curve.kind != AnalyticAtomicCurveKind::circular_arc ||
        !coordinate_in_span(curve.circle.center.x) || !coordinate_in_span(curve.circle.center.y) ||
        !point_interval_fits_resolution(point(curve.circle.center)) ||
        !std::isfinite(curve.circle.radius.lower) || !std::isfinite(curve.circle.radius.upper) ||
        curve.circle.radius.lower <= 0.0 || curve.circle.radius.lower > curve.circle.radius.upper ||
        curve.circle.radius.upper > static_cast<double>(kLocalCoordinateSpanNm) ||
        !scalar_interval_fits_resolution({curve.circle.radius.lower, curve.circle.radius.upper}))
        return false;
    const Point start = point(curve.start);
    const Point end = point(curve.end);
    const Point center = point(curve.circle.center);
    const Interval radius = {curve.circle.radius.lower, curve.circle.radius.upper};
    const Interval start_radius =
        square_root(dot(subtract(start, center), subtract(start, center)));
    const Interval end_radius = square_root(dot(subtract(end, center), subtract(end, center)));
    if (std::max({radius.lower, start_radius.lower, end_radius.lower}) >
        std::min({radius.upper, start_radius.upper, end_radius.upper}))
        return false;
    return valid_arc_certificate(curve, start_radius, end_radius) &&
           valid_arc_sweep(curve, start, end, center);
}

bool charge_predicate(AnalyticNarrowPhaseTelemetry& telemetry, const AnalyticSolverLimits& limits,
                      bool domain = false) noexcept
{
    if (telemetry.predicate_calls == limits.predicate_calls)
        return false;
    ++telemetry.predicate_calls;
    if (domain)
        ++telemetry.domain_predicates;
    else
        ++telemetry.carrier_predicates;
    return true;
}

bool interval_within_resolution(Point candidate, Point endpoint) noexcept
{
    const Interval x_squared = square(subtract(candidate.x, endpoint.x));
    const Interval y_squared = square(subtract(candidate.y, endpoint.y));
    const Interval radial_squared = add(x_squared, y_squared);
    constexpr double resolution_squared =
        static_cast<double>(kAnalyticTopologyResolutionNm * kAnalyticTopologyResolutionNm);
    return radial_squared.upper <= resolution_squared;
}

bool interval_is_exact_point(Point candidate, Point endpoint) noexcept
{
    return singleton(candidate.x) && singleton(candidate.y) && singleton(endpoint.x) &&
           singleton(endpoint.y) && candidate.x.lower == endpoint.x.lower &&
           candidate.y.lower == endpoint.y.lower;
}

Interval measured_square_root(Interval value, AnalyticNarrowPhaseTelemetry& telemetry) noexcept
{
    ++telemetry.square_root_calls;
    return square_root(value);
}

bool circle_boundary_within_resolution(Point candidate, const AnalyticFilteredCircleNm& circle,
                                       AnalyticNarrowPhaseTelemetry& telemetry) noexcept
{
    const Point radial = subtract(candidate, point(circle.center));
    const Interval distance = measured_square_root(dot(radial, radial), telemetry);
    const Interval gap = subtract(distance, {circle.radius.lower, circle.radius.upper});
    const double maximum_gap = std::max(std::fabs(gap.lower), std::fabs(gap.upper));
    return maximum_gap <= static_cast<double>(kAnalyticTopologyResolutionNm);
}

bool circle_separation_within_resolution(Interval distance, Interval left_radius,
                                         Interval right_radius) noexcept
{
    const Interval external_gap = subtract(distance, add(left_radius, right_radius));
    const Interval internal_gap = subtract(absolute(subtract(left_radius, right_radius)), distance);
    const double maximum_separation = std::max({0.0, external_gap.upper, internal_gap.upper});
    return maximum_separation <= static_cast<double>(kAnalyticTopologyResolutionNm);
}

DomainResult line_domain(Point candidate, const AnalyticAtomicCurveNm& line,
                         AnalyticNarrowPhaseTelemetry& telemetry,
                         const AnalyticSolverLimits& limits) noexcept
{
    if (!charge_predicate(telemetry, limits, true))
        return DomainResult::uncertain;
    if (interval_is_exact_point(candidate, point(line.start)) ||
        interval_is_exact_point(candidate, point(line.end)))
        return DomainResult::inside;
    if (interval_within_resolution(candidate, point(line.start)) ||
        interval_within_resolution(candidate, point(line.end)))
        return DomainResult::inside_resolution;
    const Point start = point(line.start);
    const Point direction = subtract(point(line.end), start);
    const Interval parameter =
        divide(dot(subtract(candidate, start), direction), dot(direction, direction));
    if (parameter.upper < 0.0 || parameter.lower > 1.0)
        return DomainResult::outside;
    if (parameter.lower >= 0.0 && parameter.upper <= 1.0)
        return DomainResult::inside;
    return DomainResult::uncertain;
}

DomainResult arc_domain(Point candidate, const AnalyticAtomicCurveNm& arc,
                        AnalyticNarrowPhaseTelemetry& telemetry,
                        const AnalyticSolverLimits& limits) noexcept
{
    if (!charge_predicate(telemetry, limits, true))
        return DomainResult::uncertain;
    if (interval_is_exact_point(candidate, point(arc.start)) ||
        interval_is_exact_point(candidate, point(arc.end)))
        return DomainResult::inside;
    if (interval_within_resolution(candidate, point(arc.start)) ||
        interval_within_resolution(candidate, point(arc.end)))
        return DomainResult::inside_resolution;

    const Point center = point(arc.circle.center);
    const Point start = subtract(point(arc.start), center);
    const Point end = subtract(point(arc.end), center);
    const Point radial = subtract(candidate, center);
    Interval from_start = cross(start, radial);
    Interval to_end = cross(radial, end);
    if (!arc.counterclockwise)
    {
        from_start = negate(from_start);
        to_end = negate(to_end);
    }
    if (!arc.major_arc)
    {
        if (from_start.upper < 0.0 || to_end.upper < 0.0)
            return DomainResult::outside;
        if (from_start.lower > 0.0 && to_end.lower > 0.0)
            return DomainResult::inside;
    }
    else
    {
        if (from_start.lower > 0.0 || to_end.lower > 0.0)
            return DomainResult::inside;
        if (from_start.upper < 0.0 && to_end.upper < 0.0)
            return DomainResult::outside;
    }
    return DomainResult::uncertain;
}

DomainResult curve_domain(Point candidate, const AnalyticAtomicCurveNm& curve,
                          AnalyticNarrowPhaseTelemetry& telemetry,
                          const AnalyticSolverLimits& limits) noexcept
{
    return curve.kind == AnalyticAtomicCurveKind::line
               ? line_domain(candidate, curve, telemetry, limits)
               : arc_domain(candidate, curve, telemetry, limits);
}

bool retain_point(PairWork& work, Point candidate, const AnalyticAtomicCurveNm& left,
                  const AnalyticAtomicCurveNm& right, AnalyticNarrowPhaseTelemetry& telemetry,
                  const AnalyticSolverLimits& limits) noexcept
{
    if (!valid_interval(candidate.x) || !valid_interval(candidate.y) ||
        !point_interval_fits_resolution(candidate))
    {
        work.uncertain = true;
        return false;
    }
    const DomainResult left_domain = curve_domain(candidate, left, telemetry, limits);
    const DomainResult right_domain = curve_domain(candidate, right, telemetry, limits);
    if (left_domain == DomainResult::uncertain || right_domain == DomainResult::uncertain)
    {
        work.uncertain = true;
        return false;
    }
    if (left_domain == DomainResult::outside || right_domain == DomainResult::outside)
        return true;
    if (left_domain == DomainResult::inside_resolution ||
        right_domain == DomainResult::inside_resolution)
        work.value.resolution_collapsed = true;
    if (work.value.point_count == work.value.points.size())
    {
        work.uncertain = true;
        return false;
    }
    work.value.points[work.value.point_count++] = public_point(candidate);
    return true;
}

void finish_relation(PairWork& work) noexcept
{
    if (work.value.point_count == 2)
    {
        const AnalyticFilteredPointNm& first = work.value.points[0];
        const AnalyticFilteredPointNm& second = work.value.points[1];
        const bool already_ordered =
            first.x.upper < second.x.lower ||
            (!(second.x.upper < first.x.lower) && first.y.upper < second.y.lower);
        const bool reverse_ordered =
            second.x.upper < first.x.lower ||
            (!(first.x.upper < second.x.lower) && second.y.upper < first.y.lower);
        if (reverse_ordered)
            std::swap(work.value.points[0], work.value.points[1]);
        else if (!already_ordered)
        {
            work.uncertain = true;
            return;
        }
    }
    work.value.relation = work.value.point_count == 0   ? AnalyticPairRelation::disjoint
                          : work.value.point_count == 1 ? AnalyticPairRelation::point
                                                        : AnalyticPairRelation::two_points;
}

AnalyticIntegerPointNm endpoint_at_projection(const AnalyticAtomicCurveNm& curve,
                                              std::int64_t projection, bool use_x) noexcept
{
    const std::int64_t first = use_x ? curve.integer_start.x : curve.integer_start.y;
    return first == projection ? curve.integer_start : curve.integer_end;
}

PairWork intersect_lines(const AnalyticAtomicCurveNm& left, const AnalyticAtomicCurveNm& right,
                         AnalyticNarrowPhaseTelemetry& telemetry,
                         const AnalyticSolverLimits& limits) noexcept
{
    PairWork work;
    if (!charge_predicate(telemetry, limits))
    {
        work.uncertain = true;
        return work;
    }
    if (left.construction_carrier_id != 0 &&
        left.construction_carrier_id == right.construction_carrier_id)
    {
        work.value.relation = AnalyticPairRelation::coincident;
        return work;
    }
    if (left.construction_carrier_id != 0 && right.construction_carrier_id != 0 &&
        left.construction_family_id != 0 &&
        left.construction_family_id == right.construction_family_id)
        return work;
    if (left.has_integer_certificate && right.has_integer_certificate)
    {
        const AnalyticIntegerPointNm left_direction{
            difference(left.integer_end.x, left.integer_start.x),
            difference(left.integer_end.y, left.integer_start.y)};
        const AnalyticIntegerPointNm right_direction{
            difference(right.integer_end.x, right.integer_start.x),
            difference(right.integer_end.y, right.integer_start.y)};
        if (wide_sign(exact_cross({0, 0}, left_direction, right_direction)) == 0)
        {
            if (wide_sign(exact_cross(left.integer_start, left.integer_end, right.integer_start)) !=
                0)
                return work;
            const bool use_x = std::llabs(left_direction.x) >= std::llabs(left_direction.y);
            const std::int64_t left_first = use_x ? left.integer_start.x : left.integer_start.y;
            const std::int64_t left_second = use_x ? left.integer_end.x : left.integer_end.y;
            const std::int64_t right_first = use_x ? right.integer_start.x : right.integer_start.y;
            const std::int64_t right_second = use_x ? right.integer_end.x : right.integer_end.y;
            const std::int64_t overlap_start =
                std::max(std::min(left_first, left_second), std::min(right_first, right_second));
            const std::int64_t overlap_end =
                std::min(std::max(left_first, left_second), std::max(right_first, right_second));
            if (overlap_end < overlap_start)
                return work;
            if (overlap_end == overlap_start)
            {
                const AnalyticIntegerPointNm intersection =
                    (overlap_start == left_first || overlap_start == left_second)
                        ? endpoint_at_projection(left, overlap_start, use_x)
                        : endpoint_at_projection(right, overlap_start, use_x);
                work.value.points[0] = public_point(point(intersection));
                work.value.point_count = 1;
                work.value.relation = AnalyticPairRelation::point;
                return work;
            }
            work.value.relation = AnalyticPairRelation::coincident;
            return work;
        }
    }

    const Point first = point(left.start);
    const Point left_direction = subtract(point(left.end), first);
    const Point second = point(right.start);
    const Point right_direction = subtract(point(right.end), second);
    const Point offset = subtract(second, first);
    const Interval denominator = cross(left_direction, right_direction);
    if (denominator.lower <= 0.0 && denominator.upper >= 0.0)
    {
        const Interval same_domain = cross(offset, left_direction);
        if (singleton(denominator) && denominator.lower == 0.0 && singleton(same_domain))
        {
            if (same_domain.lower == 0.0)
                work.value.relation = AnalyticPairRelation::coincident;
            return work;
        }
        work.uncertain = true;
        return work;
    }
    const Interval left_parameter = divide(cross(offset, right_direction), denominator);
    const Point intersection = add(first, scale(left_direction, left_parameter));
    retain_point(work, intersection, left, right, telemetry, limits);
    finish_relation(work);
    return work;
}

PairWork intersect_line_circle(const AnalyticAtomicCurveNm& line, const AnalyticAtomicCurveNm& arc,
                               AnalyticNarrowPhaseTelemetry& telemetry,
                               const AnalyticSolverLimits& limits) noexcept
{
    PairWork work;
    if (!charge_predicate(telemetry, limits))
    {
        work.uncertain = true;
        return work;
    }
    const Point start = point(line.start);
    const Point direction = subtract(point(line.end), start);
    const Point center = point(arc.circle.center);
    const Point center_offset = subtract(center, start);
    const Interval length_squared = dot(direction, direction);
    const Interval along = divide(dot(center_offset, direction), length_squared);
    const Point base = add(start, scale(direction, along));
    const Interval distance_squared = dot(subtract(base, center), subtract(base, center));
    const Interval radius_squared = square({arc.circle.radius.lower, arc.circle.radius.upper});
    const Interval height_squared = subtract(radius_squared, distance_squared);
    if (height_squared.upper < 0.0)
        return work;

    bool certified_tangent = false;
    if (height_squared.lower <= 0.0 && line.has_integer_certificate &&
        arc.has_integer_certificate && arc.has_integer_radius_certificate)
    {
        if (!charge_predicate(telemetry, limits))
        {
            work.uncertain = true;
            return work;
        }
        certified_tangent = certified_line_circle_tangent(line, arc, telemetry);
    }
    if (certified_tangent || height_squared.lower <= 0.0)
    {
        const bool exact_tangent =
            certified_tangent || (height_squared.lower == 0.0 && height_squared.upper == 0.0);
        const double maximum_height = measured_square_root(height_squared, telemetry).upper;
        if ((!exact_tangent &&
             2.0 * maximum_height > static_cast<double>(kAnalyticTopologyResolutionNm)) ||
            !circle_boundary_within_resolution(base, arc.circle, telemetry))
        {
            work.uncertain = true;
            return work;
        }
        retain_point(work, base, line, arc, telemetry, limits);
        if (work.value.point_count != 0)
        {
            work.value.resolution_collapsed = !exact_tangent;
            if (exact_tangent)
                ++telemetry.tangent_contacts;
        }
        finish_relation(work);
        return work;
    }

    const Interval scale_value =
        measured_square_root(divide(height_squared, length_squared), telemetry);
    const Point displacement = scale(direction, scale_value);
    const double maximum_height = measured_square_root(height_squared, telemetry).upper;
    if (2.0 * maximum_height <= static_cast<double>(kAnalyticTopologyResolutionNm))
    {
        if (!circle_boundary_within_resolution(base, arc.circle, telemetry))
        {
            work.uncertain = true;
            return work;
        }
        retain_point(work, base, line, arc, telemetry, limits);
        if (work.value.point_count != 0)
            work.value.resolution_collapsed = true;
    }
    else
    {
        retain_point(work, subtract(base, displacement), line, arc, telemetry, limits);
        if (!work.uncertain)
            retain_point(work, add(base, displacement), line, arc, telemetry, limits);
    }
    finish_relation(work);
    return work;
}

PairWork intersect_circles(const AnalyticAtomicCurveNm& left, const AnalyticAtomicCurveNm& right,
                           AnalyticNarrowPhaseTelemetry& telemetry,
                           const AnalyticSolverLimits& limits) noexcept
{
    PairWork work;
    if (!charge_predicate(telemetry, limits))
    {
        work.uncertain = true;
        return work;
    }
    const Point first_center = point(left.circle.center);
    const Point center_delta = subtract(point(right.circle.center), first_center);
    const Interval distance_squared = dot(center_delta, center_delta);
    const Interval left_radius = {left.circle.radius.lower, left.circle.radius.upper};
    const Interval right_radius = {right.circle.radius.lower, right.circle.radius.upper};
    if (left.construction_carrier_id != 0 &&
        left.construction_carrier_id == right.construction_carrier_id)
    {
        work.value.relation = AnalyticPairRelation::coincident;
        return work;
    }
    if (left.construction_carrier_id != 0 && right.construction_carrier_id != 0 &&
        left.construction_family_id != 0 &&
        left.construction_family_id == right.construction_family_id)
        return work;
    const CertifiedCircleRelation certified_relation = certified_circle_relation(left, right);
    if (certified_relation == CertifiedCircleRelation::coincident)
    {
        work.value.relation = AnalyticPairRelation::coincident;
        return work;
    }
    if (certified_relation == CertifiedCircleRelation::disjoint)
        return work;
    if (distance_squared.lower <= 0.0)
    {
        if (distance_squared.upper == 0.0)
        {
            if (singleton(left_radius) && singleton(right_radius) &&
                left_radius.lower == right_radius.lower)
            {
                work.value.relation = AnalyticPairRelation::coincident;
                return work;
            }
            if (left_radius.upper < right_radius.lower || right_radius.upper < left_radius.lower)
                return work;
        }
        work.uncertain = true;
        return work;
    }
    const Interval distance = measured_square_root(distance_squared, telemetry);
    const Interval radius_sum = add(left_radius, right_radius);
    const Interval radius_difference = absolute(subtract(left_radius, right_radius));
    if (distance.lower > radius_sum.upper || distance.upper < radius_difference.lower)
        return work;
    const Interval left_radius_squared = square(left_radius);
    const Interval right_radius_squared = square(right_radius);
    const Interval numerator =
        add(subtract(left_radius_squared, right_radius_squared), distance_squared);
    const Interval along = divide(numerator, multiply(exact(2.0), distance_squared));
    const Point base = add(first_center, scale(center_delta, along));
    const Interval height_squared =
        subtract(left_radius_squared, multiply(square(along), distance_squared));
    const bool exact_tangent = certified_relation == CertifiedCircleRelation::tangent ||
                               (height_squared.lower == 0.0 && height_squared.upper == 0.0);
    if (exact_tangent || height_squared.lower <= 0.0)
    {
        const double maximum_height = measured_square_root(height_squared, telemetry).upper;
        if ((!exact_tangent &&
             (2.0 * maximum_height > static_cast<double>(kAnalyticTopologyResolutionNm) ||
              !circle_separation_within_resolution(distance, left_radius, right_radius))) ||
            !circle_boundary_within_resolution(base, left.circle, telemetry) ||
            !circle_boundary_within_resolution(base, right.circle, telemetry))
        {
            work.uncertain = true;
            return work;
        }
        retain_point(work, base, left, right, telemetry, limits);
        if (work.value.point_count != 0)
        {
            work.value.resolution_collapsed = !exact_tangent;
            if (exact_tangent)
                ++telemetry.tangent_contacts;
        }
        finish_relation(work);
        return work;
    }

    const Interval scale_value =
        measured_square_root(divide(height_squared, distance_squared), telemetry);
    const Point displacement = scale(perpendicular(center_delta), scale_value);
    const double maximum_height = measured_square_root(height_squared, telemetry).upper;
    if (2.0 * maximum_height <= static_cast<double>(kAnalyticTopologyResolutionNm))
    {
        if (!circle_boundary_within_resolution(base, left.circle, telemetry) ||
            !circle_boundary_within_resolution(base, right.circle, telemetry))
        {
            work.uncertain = true;
            return work;
        }
        retain_point(work, base, left, right, telemetry, limits);
        if (work.value.point_count != 0)
            work.value.resolution_collapsed = true;
    }
    else
    {
        retain_point(work, subtract(base, displacement), left, right, telemetry, limits);
        if (!work.uncertain)
            retain_point(work, add(base, displacement), left, right, telemetry, limits);
    }
    finish_relation(work);
    return work;
}

bool checked_memory(std::size_t count, std::uint64_t& bytes) noexcept
{
    if constexpr (sizeof(std::size_t) >= sizeof(std::uint64_t))
    {
        if (count > std::numeric_limits<std::uint64_t>::max() / kPairLogicalBytes)
            return false;
    }
    bytes = static_cast<std::uint64_t>(count) * kPairLogicalBytes;
    return true;
}

bool valid_curve_table(const std::vector<AnalyticAtomicCurveNm>& curves) noexcept
{
    for (std::size_t index = 0; index < curves.size(); ++index)
    {
        if (!valid_curve(curves[index]) || curves[index].curve_index != index + 1)
            return false;
    }
    return true;
}

bool valid_canonical_pair(AnalyticCurvePair pair, AnalyticCurvePair previous,
                          bool has_previous) noexcept
{
    return pair.first != 0 && pair.first < pair.second &&
           (!has_previous || previous.first < pair.first ||
            (previous.first == pair.first && previous.second < pair.second));
}

PairWork dispatch_pair(const AnalyticAtomicCurveNm& left, const AnalyticAtomicCurveNm& right,
                       AnalyticCurvePair pair, AnalyticNarrowPhaseTelemetry& telemetry,
                       const AnalyticSolverLimits& limits) noexcept
{
    PairWork work;
    if (left.kind == AnalyticAtomicCurveKind::line && right.kind == AnalyticAtomicCurveKind::line)
    {
        ++telemetry.line_line_pairs;
        work = intersect_lines(left, right, telemetry, limits);
    }
    else if (left.kind == AnalyticAtomicCurveKind::circular_arc &&
             right.kind == AnalyticAtomicCurveKind::circular_arc)
    {
        ++telemetry.circle_circle_pairs;
        work = intersect_circles(left, right, telemetry, limits);
    }
    else
    {
        ++telemetry.line_circle_pairs;
        const AnalyticAtomicCurveNm& line =
            left.kind == AnalyticAtomicCurveKind::line ? left : right;
        const AnalyticAtomicCurveNm& arc =
            left.kind == AnalyticAtomicCurveKind::circular_arc ? left : right;
        work = intersect_line_circle(line, arc, telemetry, limits);
    }
    work.value.pair = pair;
    return work;
}

bool append_pair_work(AnalyticNarrowPhaseResult& result, const PairWork& work,
                      const AnalyticSolverLimits& limits)
{
    if (work.uncertain)
    {
        ++result.telemetry.uncertain_predicates;
        result.error = AnalyticNarrowPhaseError::resource_limit_exceeded;
        return false;
    }
    if (work.value.point_count > limits.intersections - result.telemetry.point_intersections)
    {
        result.error = AnalyticNarrowPhaseError::resource_limit_exceeded;
        return false;
    }
    result.telemetry.point_intersections += work.value.point_count;
    if (work.value.relation == AnalyticPairRelation::coincident)
        ++result.telemetry.coincident_pairs;
    if (work.value.resolution_collapsed)
        ++result.telemetry.resolution_collapses;
    result.intersections.push_back(work.value);
    return true;
}

} // namespace

AnalyticNarrowPhaseResult
intersect_analytic_curve_candidates(const std::vector<AnalyticAtomicCurveNm>& curves,
                                    const std::vector<AnalyticCurvePair>& candidate_pairs,
                                    const AnalyticSolverLimits& limits)
{
    AnalyticNarrowPhaseResult result;
    if (!analytic_solver_limits_within_hard_ceilings(limits))
    {
        result.error = AnalyticNarrowPhaseError::invalid_argument;
        return result;
    }
    if (candidate_pairs.size() > limits.examined_curve_pairs)
    {
        result.error = AnalyticNarrowPhaseError::resource_limit_exceeded;
        return result;
    }
    if (curves.size() > limits.boundary_occurrences)
    {
        result.error = AnalyticNarrowPhaseError::resource_limit_exceeded;
        return result;
    }
    result.telemetry.curve_table_entries = curves.size();
    if (!valid_curve_table(curves))
    {
        result.error = AnalyticNarrowPhaseError::invalid_argument;
        return result;
    }
    std::uint64_t logical_bytes = 0;
    if (!checked_memory(candidate_pairs.size(), logical_bytes) ||
        logical_bytes > limits.working_memory_bytes)
    {
        result.error = AnalyticNarrowPhaseError::resource_limit_exceeded;
        return result;
    }
    result.telemetry.peak_working_memory_bytes = logical_bytes;
    static_assert(sizeof(AnalyticPairIntersection) <= kPairLogicalBytes,
                  "canonical narrow-phase pair charge must cover the native record");
    try
    {
        result.intersections.reserve(candidate_pairs.size());
    }
    catch (const std::bad_alloc&)
    {
        result.error = AnalyticNarrowPhaseError::resource_limit_exceeded;
        return result;
    }
    if (result.intersections.capacity() >
        logical_bytes / static_cast<std::uint64_t>(sizeof(AnalyticPairIntersection)))
    {
        result.error = AnalyticNarrowPhaseError::resource_limit_exceeded;
        result.intersections.clear();
        return result;
    }

    AnalyticCurvePair previous{};
    for (std::size_t pair_index = 0; pair_index < candidate_pairs.size(); ++pair_index)
    {
        const AnalyticCurvePair pair = candidate_pairs[pair_index];
        if (!valid_canonical_pair(pair, previous, pair_index != 0))
        {
            result.error = AnalyticNarrowPhaseError::invalid_argument;
            result.intersections.clear();
            return result;
        }
        previous = pair;
        if (pair.second > curves.size())
        {
            result.error = AnalyticNarrowPhaseError::invalid_argument;
            result.intersections.clear();
            return result;
        }
        const AnalyticAtomicCurveNm& left = curves[pair.first - 1];
        const AnalyticAtomicCurveNm& right = curves[pair.second - 1];
        result.telemetry.curve_references_resolved += 2;
        ++result.telemetry.candidate_pairs_consumed;
        const PairWork work = dispatch_pair(left, right, pair, result.telemetry, limits);
        if (!append_pair_work(result, work, limits))
        {
            result.intersections.clear();
            return result;
        }
    }
    return result;
}

} // namespace geometer
